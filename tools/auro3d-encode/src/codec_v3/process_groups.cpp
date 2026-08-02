#include "process_groups.hpp"

#include "cluster_deltas_quant.hpp"
#include "channel_metadata.hpp"
#include "frame_descriptor.hpp"
#include "mix_deltas.hpp"
#include "mix_mix2.hpp"
#include "mix_mix3.hpp"
#include "mix_unmix.hpp"
#include "native_dither.hpp"
#include "pcm_shift.hpp"
#include "quality_measure.hpp"
#include "scaler.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace auro3d::encode {
namespace {

bool frame_is_silent(
    const std::vector<std::int32_t>& samples,
    std::uint32_t bit_line) {
    if (bit_line >= 31u)
        return false;
    const std::int32_t limit = static_cast<std::int32_t>(1u << bit_line);
    for (std::int32_t sample : samples) {
        if (sample < -limit || sample > limit)
            return false;
    }
    return true;
}

bool carrier_fits_bit_line(
    const std::vector<std::int32_t>& samples,
    std::uint32_t bit_line) {
    if (bit_line > 23u)
        return false;
    const std::int32_t limit = static_cast<std::int32_t>(
        std::uint32_t{1} << (23u - bit_line));
    for (const std::int32_t sample : samples) {
        if (sample < -limit || sample > limit - 1) {
            return false;
        }
    }
    return true;
}

bool finish_carrier(
    std::uint32_t bit_line,
    AnalyzedEncodeGroup& out,
    std::string& error) {
    // Mixer::operator() @ 0x4EAAC0 sets Group+664 only when every mixed
    // carrier sample fits [-2^(23-bit_line), 2^(23-bit_line)-1].
    if (!carrier_fits_bit_line(out.carrier.samples, bit_line)) {
        out.carrier_overflow = true;
        error = "mixed carrier exceeds native bit-line range";
        return false;
    }
    out.carrier_ready = true;
    return true;
}

std::int32_t arithmetic_shift_right(
    std::int32_t value,
    std::uint32_t shift) {
    if (shift == 0u)
        return value;
    const std::uint32_t raw =
        static_cast<std::uint32_t>(value);
    const std::uint32_t fill =
        value < 0 ? 0xFFFFFFFFu << (32u - shift) : 0u;
    return static_cast<std::int32_t>((raw >> shift) | fill);
}

bool analysis_scaler_index(
    const EncodeGroup& group,
    const std::vector<std::size_t>& active_indices,
    std::uint8_t& scaler_index,
    std::string& error) {
    scaler_index = 0u;
    if (active_indices.size() < 2u) {
        scaler_index = group.minimum_scaler_index;
        return true;
    }
    std::uint32_t maximum = 0u;
    for (std::size_t sample = 0u;
         sample < group.frames[0].samples.size();
         ++sample) {
        std::int64_t sum = 0;
        for (const std::size_t frame : active_indices) {
            sum += group.frames[frame].samples[sample];
        }
        const std::uint64_t magnitude = static_cast<std::uint64_t>(
            sum < 0 ? -sum : sum);
        if (magnitude > maximum)
            maximum = static_cast<std::uint32_t>(magnitude);
    }
    // Rescaler @ 0x4EA080 leaves index zero below 0x7EB851. At or above
    // that threshold scaler_to_ix receives max / 8304720.0f.
    if (maximum < 0x7EB851u)
    {
        scaler_index = group.minimum_scaler_index;
        return true;
    }
    const float requested =
        static_cast<float>(maximum) / 8304720.0f;
    float selected = 0.0f;
    if (!scaler_to_index(
            requested, scaler_index, selected)) {
        error = "native group scaler cannot represent mixed PCM peak";
        return false;
    }
    scaler_index = std::max(
        scaler_index,
        group.minimum_scaler_index);
    return true;
}

bool rescale_for_analysis(
    const std::vector<std::int32_t>& source,
    std::uint32_t bit_line,
    std::uint8_t scaler_index,
    NativeDitherState* dither,
    std::vector<std::int32_t>& destination,
    std::string& error) {
    destination.clear();
    float scaler = 0.0f;
    if (bit_line > 24u
        || !scaler_from_index(scaler_index, scaler)
        || !std::isfinite(scaler) || scaler <= 0.0f) {
        error = "native analysis scaler configuration is invalid";
        return false;
    }
    const float reciprocal = 1.0f / scaler;
    const std::int64_t coefficient = static_cast<std::int64_t>(
        static_cast<double>(reciprocal) * 549755813888.0);
    const std::uint32_t shift = bit_line + 8u;
    destination.resize(source.size());
    for (std::size_t sample = 0u; sample < source.size(); ++sample) {
        std::int64_t product =
            coefficient * static_cast<std::int64_t>(source[sample]);
        if (product < 0)
            product += 0x7FFFFFFFll;
        std::int64_t scaled = product >= 0
            ? product / 0x80000000ll
            : -static_cast<std::int64_t>(
                (static_cast<std::uint64_t>(-product)
                    + 0x7FFFFFFFull)
                / 0x80000000ull);
        if (scaled > std::numeric_limits<std::int32_t>::max())
            scaled = std::numeric_limits<std::int32_t>::max();
        else if (scaled < std::numeric_limits<std::int32_t>::min())
            scaled = std::numeric_limits<std::int32_t>::min();
        destination[sample] = static_cast<std::int32_t>(scaled);
    }
    if (dither != nullptr
        && !apply_native_dither(destination, shift, *dither, error)) {
        return false;
    }
    for (std::int32_t& sample : destination) {
        sample = arithmetic_shift_right(sample, shift);
    }
    return true;
}

bool unscale_quality_plane(
    std::vector<std::int32_t>& samples,
    std::uint32_t bit_line,
    bool has_scaler,
    std::uint8_t scaler_index,
    std::string& error) {
    if (!has_scaler)
        return pcm_shift_left(samples, bit_line, error);
    float scaler = 0.0f;
    if (!scaler_from_index(scaler_index, scaler)
        || !std::isfinite(scaler)) {
        error = "native quality scaler index is invalid";
        return false;
    }
    const float factor =
        static_cast<float>(std::uint32_t{1} << bit_line)
        * scaler;
    for (std::int32_t& sample : samples) {
        const float value = static_cast<float>(sample) * factor;
        if (!std::isfinite(value)
            || value >= 2147483648.0f
            || value < -2147483648.0f) {
            sample = std::numeric_limits<std::int32_t>::min();
        } else {
            sample = static_cast<std::int32_t>(value);
        }
    }
    return true;
}

bool compute_reconstruction_quality(
    const EncodeGroup& group,
    const std::vector<std::size_t>& active_indices,
    std::vector<std::vector<std::int32_t>> reconstructed,
    AnalyzedEncodeGroup& out,
    std::string& error) {
    if (active_indices.size() != reconstructed.size()) {
        error = "native quality reconstruction arity mismatch";
        return false;
    }
    NativeQualityAccumulator accumulator{};
    out.frame_quality.clear();
    out.frame_quality.reserve(group.frames.size());
    for (std::size_t plane = 0u; plane < reconstructed.size(); ++plane) {
        // ComputeQuality::unscale_ @ 0x4EF160 applies only Group+24 here.
        // The encoder's external PCM24 samples are right-aligned whereas the
        // native audio-frame comparison domain has its eight padding bits
        // removed. Put both operands in that same domain.
        if (!unscale_quality_plane(
                reconstructed[plane],
                group.bit_line,
                out.has_scaler_ix,
                out.scaler_ix,
                error)) {
            error = "native quality unscale: " + error;
            return false;
        }
    }
    for (std::size_t source = 0u; source < group.frames.size(); ++source) {
        const std::vector<std::int32_t>& reference =
            group.frames[source].samples;
        std::vector<std::int32_t> silence(reference.size(), 0);
        const std::vector<std::int32_t>* decoded = &silence;
        const auto active = std::find(
            active_indices.begin(), active_indices.end(), source);
        if (active != active_indices.end()) {
            decoded = &reconstructed[
                static_cast<std::size_t>(
                    active - active_indices.begin())];
        }
        NativeFrameQuality frame{};
        if (!measure_native_frame_quality(
                reference, *decoded, frame, error)
            || !accumulator.add(frame, error)) {
            error = "native quality measurement: " + error;
            return false;
        }
        out.frame_quality.push_back(frame);
    }
    out.quality_error_db = accumulator.error_level_db();
    out.quality_present = true;
    return true;
}

bool compute_direct_quality(
    const EncodeGroup& group,
    std::size_t active_index,
    const std::vector<std::int32_t>& reconstructed,
    AnalyzedEncodeGroup& out,
    std::string& error) {
    return compute_reconstruction_quality(
        group,
        std::vector<std::size_t>{active_index},
        std::vector<std::vector<std::int32_t>>{reconstructed},
        out,
        error);
}

bool encode_mix3_gvm(
    const std::vector<std::int32_t>& primary,
    const std::vector<std::int32_t>& secondary,
    const std::vector<std::int32_t>& tertiary,
    const NativeGvmLearnerInput& learner_input,
    const NativeGvmSearchPlan& search_plan,
    const NativeGvmConfiguration& gvm,
    std::uint32_t bit_line,
    std::uint32_t group_field_208,
    std::uint32_t rescaler_bit_cost,
    AnalyzedEncodeGroup& out,
    std::string& error) {
    std::uint64_t bit_budget = 0u;
    std::uint64_t fixed_bit_cost = 0u;
    if (!calculate_native_quantizer_budget(
            out.gvm_input.sample_count,
            bit_line,
            group_field_208,
            rescaler_bit_cost,
            bit_budget,
            fixed_bit_cost,
            error)) {
        error = "mix3 GVM: " + error;
        return false;
    }
    NativeGvmSearchResult search_result{};
    const bool searched =
        gvm.learner == NativeGvmLearnerImplementation::modern
        ? run_native_gvm_modern_search(
            learner_input,
            search_plan,
            bit_budget,
            gvm,
            search_result,
            error)
        : gvm.learner == NativeGvmLearnerImplementation::old_fast
            && run_native_gvm_old_fast_search(
                learner_input,
                search_plan,
                bit_budget,
                gvm,
                search_result,
                error);
    if (!searched) {
        if (error.empty())
            error = "mix3 GVM learner implementation is unavailable";
        else
            error = "mix3 GVM: " + error;
        return false;
    }
    const std::vector<std::int32_t>& scalars =
        search_result.learned.centers.scalars;
    if ((scalars.size() & 1u) != 0u
        || scalars.size() / 2u != search_result.learned.sizes.size()) {
        error = "mix3 GVM returned an invalid two-dimensional center table";
        return false;
    }
    std::vector<std::array<std::int32_t, 2>> pairs;
    pairs.reserve(scalars.size() / 2u);
    for (std::size_t index = 0u; index < scalars.size(); index += 2u)
        pairs.push_back({scalars[index], scalars[index + 1u]});
    if (!pack_mix3_residual_table(pairs, out.mix3_residuals, error))
        return false;

    out.mix3_levels = std::move(search_result.learned.sizes);
    out.mix3_indices = std::move(search_result.learned.indices);
    out.mix3_level_pack_mode =
        search_result.learned.cost.level_pack_mode;
    out.vq_shift = 0u;
    out.residual_bit_width =
        search_result.learned.cost.residual_bit_width;
    out.quantizer_bit_budget = bit_budget;
    out.quantizer_fixed_bit_cost = fixed_bit_cost;
    out.quantizer_used_bits =
        search_result.learned.cost.residual_bit_cost
        + search_result.learned.cost.level_bit_cost;
    out.golomb_index_bit_cost =
        search_result.learned.cost.level_bit_cost;
    out.gvm_fit_status = search_result.learned.fit_status;
    out.gvm_compact_fit = search_result.learned.compact_fit;
    out.gvm_learned = true;
    out.gvm_selected_clusters = search_result.clusters;
    out.gvm_learn_attempts = search_result.learn_attempts;
    out.gvm_forced_zero_center =
        search_result.learned.centers.forced_zero_center;
    out.gvm_forced_zero_index =
        search_result.learned.centers.forced_zero_index;

    Mix3MixerSeeds seeds{};
    if (!mix3_mixer_reconstruct(
            primary,
            secondary,
            tertiary,
            out.mix3_indices,
            pairs,
            out.carrier.samples,
            seeds,
            error)) {
        return false;
    }
    out.mix3_seeds = seeds.values;
    return true;
}

} // namespace

bool analyze_encode_group_impl(
    const EncodeGroup& group,
    bool dither_enabled,
    NativeDitherState* dither,
    AnalyzedEncodeGroup& out,
    std::string& error) {
    error.clear();
    out = {};
    out.cluster_backend = group.cluster_backend;
    out.gvm_learner = group.gvm.learner;
    if (group.frames.size() != group.sources.arity || group.sources.arity == 0u
        || group.sources.arity > 3u) {
        error = "encode group frame count does not match source arity";
        return false;
    }
    if (group.carrier_channel >= kCodecV3ChannelCount
        || group.bit_line < 3u || group.bit_line > 12u) {
        error = "encode group carrier or bit line is outside codec-v3 range";
        return false;
    }
    if (group.cluster_backend == NativeClusterDeltasBackend::gvm
        && group.gvm.learner
            == NativeGvmLearnerImplementation::unavailable_mode1) {
        error = "native GVM factory has no learner implementation for mode 1";
        return false;
    }
    for (std::uint32_t i = 0; i < group.sources.arity; ++i) {
        if (group.sources.channels[i] >= kCodecV3ChannelCount) {
            error = "encode group source channel is outside codec-v3 range";
            return false;
        }
        for (std::uint32_t j = 0; j < i; ++j) {
            if (group.sources.channels[i] == group.sources.channels[j]) {
                error = "encode group source channels are duplicated";
                return false;
            }
        }
        if (group.frames[i].channel_id != group.sources.channels[i]) {
            error = "encode group frame order does not match source channel order";
            return false;
        }
    }
    const std::size_t sample_count = group.frames[0].samples.size();
    if (sample_count == 0u) {
        error = "encode group has empty PCM";
        return false;
    }
    if (sample_count > std::numeric_limits<std::uint32_t>::max()) {
        error = "encode group sample count exceeds native 32-bit storage";
        return false;
    }
    for (const EncodeGroupFrame& frame : group.frames) {
        if (frame.samples.size() != sample_count) {
            error = "encode group source planes have mismatched lengths";
            return false;
        }
    }

    out.carrier.carrier_channel_id = group.carrier_channel;
    // prepare_mix_ @ 0x4E6AB0 left-shifts by Group+24 (bit_line), not VQ shift.
    out.carrier.quantization_shift = group.bit_line;
    if (!calculate_native_rescaler_bit_cost(
            group.rescaler_accounting,
            false,
            out.rescaler_bit_cost,
            error)) {
        return false;
    }

    bool all_silent = true;
    for (const EncodeGroupFrame& frame : group.frames) {
        if (!frame_is_silent(frame.samples, group.bit_line)) {
            all_silent = false;
            break;
        }
    }
    if (all_silent) {
        // DetectSilence @ 0x4E9E50 all-silent path: one zero analysis frame with
        // channel id from the first source Frame+24, then
        // Group+192 = dword_287F20[0] = 0. Carrier stays zero through Mixer case 1.
        out.silent = true;
        out.carrier_ready = true;
        out.silence_mode = 0u;
        out.analysis_arity = 1u;
        out.analysis_source_ids = {group.frames[0].channel_id};
        const std::vector<std::int32_t> zero_frame(sample_count, 0);
        if (!rescale_for_analysis(
                zero_frame,
                group.bit_line,
                0u,
                dither_enabled ? dither : nullptr,
                out.carrier.samples,
                error)) {
            error = "native silent-frame rescaler: " + error;
            return false;
        }
        out.scaler_attempts = 1u;
        if (!compute_direct_quality(
                group, 0u, out.carrier.samples, out, error)) {
            return false;
        }
        // ComputeQuality @ 0x4EB090 recognizes the retained Group+360
        // synthetic zero frame and replaces the aggregate RMS result with
        // Group+24 * 0.01. Per-source measurements above still run.
        out.quality_error_db =
            static_cast<double>(group.bit_line) * 0.01;
        return finish_carrier(group.bit_line, out, error);
    }

    std::vector<std::size_t> active_indices;
    active_indices.reserve(group.frames.size());
    for (std::size_t index = 0; index < group.frames.size(); ++index) {
        if (!frame_is_silent(group.frames[index].samples, group.bit_line))
            active_indices.push_back(index);
    }
    if (active_indices.empty()) {
        error = "non-empty encode group lost every active source after silence detection";
        return false;
    }

    // Rescaler @ 0x4EA080 first applies a Q31 coefficient scaled by 2^39,
    // then shift_right(bit_line+8). For scaler 1 this is exactly
    // shift_right(bit_line); keeping the coefficient step also covers the
    // automatic mix-peak scaler selected below.
    std::uint8_t scaler_index = 0u;
    if (!analysis_scaler_index(
            group, active_indices, scaler_index, error)) {
        return false;
    }
    out.has_scaler_ix = scaler_index != 0u;
    out.scaler_ix = scaler_index;
    out.scaler_attempts = 1u;
    if (out.has_scaler_ix
        && !calculate_native_rescaler_bit_cost(
            group.rescaler_accounting,
            true,
            out.rescaler_bit_cost,
            error)) {
        return false;
    }
    std::vector<std::vector<std::int32_t>> shifted;
    shifted.reserve(group.frames.size());
    for (const EncodeGroupFrame& frame : group.frames) {
        std::vector<std::int32_t> rescaled;
        if (!rescale_for_analysis(
                frame.samples,
                group.bit_line,
                scaler_index,
                nullptr,
                rescaled,
                error)) {
            return false;
        }
        shifted.push_back(std::move(rescaled));
    }

    if (active_indices.size() == 1u) {
        // DetectSilence removes silent source frames before Mixer dispatch;
        // Mixer case 1 @ 0x4EAAC0 then memmoves the shifted plane.
        const std::size_t active = active_indices[0];
        out.analysis_arity = 1u;
        out.analysis_source_ids = {group.frames[active].channel_id};
        out.carrier.samples = std::move(shifted[active]);
        if (!compute_direct_quality(
                group, active, out.carrier.samples, out, error)) {
            return false;
        }
        return finish_carrier(group.bit_line, out, error);
    }

    if (active_indices.size() == 2u) {
        // Mixer case 2 @ 0x4EE540 consumes the two remaining analysis frames.
        const std::size_t first = active_indices[0];
        const std::size_t second = active_indices[1];
        out.analysis_arity = 2u;
        out.analysis_source_ids = {
            group.frames[first].channel_id,
            group.frames[second].channel_id};
        if (!compute_mix2_deltas(shifted[first], shifted[second], out.deltas, error))
            return false;
        NativeGvmSearchPlan gvm_search{};
        if (group.cluster_backend == NativeClusterDeltasBackend::gvm) {
            NativeGvmInputShape gvm_shape{};
            if (!validate_native_gvm_input_shape(
                    out.deltas.size(), sample_count, gvm_shape, error)
                || !prepare_native_gvm_search(
                    gvm_shape,
                    group.gvm_common_limit,
                    group.gvm_dimension1_start,
                    group.gvm_dimension2_start,
                    group.gvm_minimum_clusters,
                    gvm_search,
                    error)) {
                return false;
            }
        }
        // Quantization::run_ mix2 @ 0x501750 then Mixer mix2 @ 0x4EE540.
        ClusterDeltasQuantizationResult quant{};
        if (!cluster_deltas_quantize_mix2(
                out.deltas,
                static_cast<std::uint32_t>(sample_count),
                group.bit_line,
                group.group_field_208,
                out.rescaler_bit_cost,
                group.cluster_backend,
                group.gvm,
                gvm_search,
                quant,
                error)) {
            return false;
        }
        if (!mix2_mixer_reconstruct(
                shifted[first],
                shifted[second],
                quant.indices,
                quant.residuals,
                out.carrier.samples,
                out.mix2_seeds,
                error)) {
            return false;
        }
        out.mix2_indices = std::move(quant.indices);
        out.mix2_residuals = std::move(quant.residuals);
        out.mix2_levels = std::move(quant.levels);
        out.mix2_level_pack_mode = quant.level_pack_mode;
        out.vq_shift = quant.shift;
        out.residual_bit_width = quant.residual_bit_width;
        out.mix2_shift_attempts = quant.shift_attempts;
        out.gvm_input = quant.gvm_input;
        out.gvm_search = gvm_search;
        out.quantizer_bit_budget = quant.bit_budget;
        out.quantizer_fixed_bit_cost = quant.fixed_bit_cost;
        out.quantizer_used_bits =
            quant.residual_bit_cost + quant.level_bit_cost;
        out.golomb_index_bit_cost =
            quant.level_bit_cost;
        out.gvm_fit_status = quant.gvm_fit_status;
        out.gvm_compact_fit = quant.gvm_compact_fit;
        out.gvm_learner_points = quant.gvm_learner_points;
        out.gvm_deterministic_seed = quant.gvm_deterministic_seed;
        out.gvm_learned = quant.gvm_learned;
        out.gvm_selected_clusters = quant.gvm_selected_clusters;
        out.gvm_learn_attempts = quant.gvm_learn_attempts;
        out.gvm_forced_zero_center = quant.gvm_forced_zero_center;
        out.gvm_forced_zero_index = quant.gvm_forced_zero_index;
        std::vector<std::int32_t> reconstructed_first;
        std::vector<std::int32_t> reconstructed_second;
        if (!mix2_unmix_reconstruct(
                out.carrier.samples,
                out.mix2_indices,
                out.mix2_residuals,
                out.mix2_seeds,
                reconstructed_first,
                reconstructed_second,
                error)
            || !compute_reconstruction_quality(
                group,
                std::vector<std::size_t>{first, second},
                std::vector<std::vector<std::int32_t>>{
                    std::move(reconstructed_first),
                    std::move(reconstructed_second)},
                out,
                error)) {
            return false;
        }
        return finish_carrier(group.bit_line, out, error);
    }

    if (group.cluster_backend == NativeClusterDeltasBackend::quantization) {
        error = "native Quantization backend accepts only one-dimensional mix2 residuals";
        return false;
    }
    std::vector<std::int32_t> mix3_deltas;
    if (!compute_mix3_deltas(
            shifted[active_indices[0]],
            shifted[active_indices[1]],
            shifted[active_indices[2]],
            mix3_deltas,
            error)) {
        return false;
    }
    out.analysis_arity = 3u;
    out.analysis_source_ids = {
        group.frames[active_indices[0]].channel_id,
        group.frames[active_indices[1]].channel_id,
        group.frames[active_indices[2]].channel_id};
    out.deltas = mix3_deltas;
    NativeGvmInputShape mix3_shape{};
    if (!validate_native_gvm_input_shape(
            mix3_deltas.size(), sample_count, mix3_shape, error)) {
        error = "mix3 GVM preflight: " + error;
        return false;
    }
    out.gvm_input = mix3_shape;
    NativeGvmLearnerInput mix3_learner_input{};
    if (!prepare_native_gvm_learner_input(
            mix3_deltas,
            mix3_shape,
            group.gvm,
            mix3_learner_input,
            error)) {
        error = "mix3 GVM preflight: " + error;
        return false;
    }
    out.gvm_learner_points =
        static_cast<std::uint32_t>(mix3_learner_input.points.size());
    out.gvm_deterministic_seed = mix3_learner_input.deterministic_seed;
    if (!prepare_native_gvm_search(
            out.gvm_input,
            group.gvm_common_limit,
            group.gvm_dimension1_start,
            group.gvm_dimension2_start,
            group.gvm_minimum_clusters,
            out.gvm_search,
            error)) {
        return false;
    }
    if (!encode_mix3_gvm(
            shifted[active_indices[0]],
            shifted[active_indices[1]],
            shifted[active_indices[2]],
            mix3_learner_input,
            out.gvm_search,
            group.gvm,
            group.bit_line,
            group.group_field_208,
            out.rescaler_bit_cost,
            out,
            error)) {
        return false;
    }
    std::vector<std::array<std::int32_t, 2>> residual_pairs;
    if (!unpack_mix3_residual_table(
            out.mix3_residuals, residual_pairs, error)) {
        return false;
    }
    Mix3MixerSeeds mix3_seeds{};
    mix3_seeds.values = out.mix3_seeds;
    std::vector<std::int32_t> reconstructed_primary;
    std::vector<std::int32_t> reconstructed_secondary;
    std::vector<std::int32_t> reconstructed_tertiary;
    if (!mix3_unmix_reconstruct(
            out.carrier.samples,
            out.mix3_indices,
            residual_pairs,
            mix3_seeds,
            reconstructed_primary,
            reconstructed_secondary,
            reconstructed_tertiary,
            error)
        || !compute_reconstruction_quality(
            group,
            active_indices,
            std::vector<std::vector<std::int32_t>>{
                std::move(reconstructed_primary),
                std::move(reconstructed_secondary),
                std::move(reconstructed_tertiary)},
            out,
            error)) {
        return false;
    }
    return finish_carrier(group.bit_line, out, error);
}

bool analyze_encode_group(
    const EncodeGroup& group,
    AnalyzedEncodeGroup& out,
    std::string& error) {
    return analyze_encode_group_impl(
        group, false, nullptr, out, error);
}

bool analyze_encode_group(
    const EncodeGroup& group,
    bool dither_enabled,
    NativeDitherState& dither,
    AnalyzedEncodeGroup& out,
    std::string& error) {
    if (dither_enabled && !dither.initialized) {
        error = "native dither state was not initialized";
        return false;
    }
    return analyze_encode_group_impl(
        group, dither_enabled, &dither, out, error);
}

bool analyze_encode_groups(
    const std::vector<EncodeGroup>& groups,
    std::vector<AnalyzedEncodeGroup>& out,
    std::string& error) {
    error.clear();
    out.clear();
    out.reserve(groups.size());
    for (const EncodeGroup& group : groups) {
        AnalyzedEncodeGroup analyzed{};
        if (!analyze_encode_group(group, analyzed, error))
            return false;
        out.push_back(std::move(analyzed));
    }
    return true;
}

bool analyzed_groups_to_encoded_pcm(
    const std::vector<AnalyzedEncodeGroup>& analyzed,
    std::vector<EncodedGroupPcm>& out,
    std::string& error) {
    error.clear();
    out.clear();
    out.reserve(analyzed.size());
    std::uint32_t carrier_mask = 0u;
    std::size_t expected_samples = 0u;
    for (const AnalyzedEncodeGroup& group : analyzed) {
        if (!group.carrier_ready || group.carrier.samples.empty()) {
            error = "analyzed group has no carrier PCM";
            return false;
        }
        if (group.carrier.carrier_channel_id >= kCodecV3ChannelCount
            || (carrier_mask & (std::uint32_t{1} << group.carrier.carrier_channel_id)) != 0u) {
            error = "analyzed groups contain duplicate or invalid carrier channels";
            return false;
        }
        if (expected_samples == 0u)
            expected_samples = group.carrier.samples.size();
        else if (group.carrier.samples.size() != expected_samples) {
            error = "analyzed groups have mismatched carrier sample counts";
            return false;
        }
        carrier_mask |= std::uint32_t{1} << group.carrier.carrier_channel_id;
        out.push_back(group.carrier);
    }
    return true;
}

} // namespace auro3d::encode
