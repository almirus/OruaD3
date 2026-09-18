#include "carrier_output.hpp"

#include "../codec_v3/channel_metadata.hpp"
#include "../codec_v3/channel_config.hpp"
#include "../codec_v3/crc.hpp"
#include "../codec_v3/adol_syntax.hpp"
#include "../codec_v3/metadata_syntax.hpp"
#include "../codec_v3/layout.hpp"
#include "../codec_v3/golomb_rice.hpp"
#include "../codec_v3/error_quantization.hpp"
#include "../codec_v3/pcm_metadata.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

namespace auro3d::encode {
namespace {

std::string lower_extension(const std::string& path) {
    const std::size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return {};
    std::string extension = path.substr(dot);
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return extension;
}

bool channel_order_matches_mask(
    const std::vector<std::uint32_t>& channel_order,
    std::uint32_t channel_mask) {
    if (channel_mask == 0u)
        return true;
    std::uint32_t count = 0;
    std::uint32_t mask = channel_mask;
    while (mask != 0u) {
        count += mask & 1u;
        mask >>= 1u;
    }
    if (count != channel_order.size())
        return false;
    std::uint32_t supplied_mask = 0u;
    for (const std::uint32_t id : channel_order) {
        if (id >= 32u || (channel_mask & (std::uint32_t{1} << id)) == 0u)
            return false;
        const std::uint32_t bit = std::uint32_t{1} << id;
        if ((supplied_mask & bit) != 0u)
            return false;
        supplied_mask |= bit;
    }
    return supplied_mask == channel_mask;
}

int wave_speaker_bit_for_codec_channel(std::uint32_t channel_id) {
    switch (channel_id) {
    case 0u: return 0;  // FL -> FRONT_LEFT
    case 1u: return 1;  // FR -> FRONT_RIGHT
    case 2u: return 2;  // C -> FRONT_CENTER
    case 3u: return 3;  // LFE -> LOW_FREQUENCY
    case 7u: return 4;  // LB -> BACK_LEFT
    case 8u: return 5;  // RB -> BACK_RIGHT
    case 6u: return 8;  // CS -> BACK_CENTER
    case 4u: return 9;  // LS -> SIDE_LEFT
    case 5u: return 10; // RS -> SIDE_RIGHT
    default: return -1;
    }
}

bool make_wave_carrier_plan(
    const std::vector<std::uint32_t>& codec_order,
    std::vector<std::uint32_t>& wave_order,
    std::uint32_t& wave_mask,
    std::string& error) {
    struct Entry {
        std::uint32_t channel_id = 0u;
        int speaker_bit = -1;
    };
    std::vector<Entry> entries;
    entries.reserve(codec_order.size());
    wave_mask = 0u;
    for (const std::uint32_t channel_id : codec_order) {
        const int speaker_bit =
            wave_speaker_bit_for_codec_channel(channel_id);
        if (speaker_bit < 0) {
            error = "carrier channel has no WAVEFORMATEXTENSIBLE speaker mapping";
            return false;
        }
        const std::uint32_t speaker =
            std::uint32_t{1} << speaker_bit;
        if ((wave_mask & speaker) != 0u) {
            error = "carrier channels collide in the WAVE speaker mask";
            return false;
        }
        wave_mask |= speaker;
        entries.push_back({channel_id, speaker_bit});
    }
    std::sort(
        entries.begin(), entries.end(),
        [](const Entry& left, const Entry& right) {
            return left.speaker_bit < right.speaker_bit;
        });
    wave_order.clear();
    wave_order.reserve(entries.size());
    for (const Entry& entry : entries)
        wave_order.push_back(entry.channel_id);
    return true;
}

bool native_gvm_fit_state_valid(const MetadataGroupRecord& group) {
    if (!group.gvm_learned) {
        return group.gvm_fit_status == 0u
            && !group.gvm_compact_fit
            && group.quantizer_used_bits <= group.quantizer_bit_budget;
    }
    if (group.gvm_fit_status < 1u || group.gvm_fit_status > 3u)
        return false;
    const bool compact = group.gvm_fit_status >= 2u;
    const bool over_budget =
        group.quantizer_used_bits > group.quantizer_bit_budget;
    return group.gvm_compact_fit == compact
        && over_budget == (group.gvm_fit_status == 1u);
}

class ChannelPayloadReader {
public:
    ChannelPayloadReader(
        const std::vector<std::int32_t>& samples,
        std::uint32_t bitlines,
        std::uint64_t position = 32u)
        : samples_(samples),
          bitlines_(bitlines),
          position_(position) {}

    bool read(std::uint32_t bit_count, std::uint32_t& value) {
        value = 0u;
        if (bit_count > 32u || bitlines_ < 3u || bitlines_ > 16u)
            return false;
        for (std::uint32_t bit = 0u; bit < bit_count; ++bit) {
            PcmMetadataBitLocation location{};
            if (!locate_pcm_metadata_bit_false(
                    position_,
                    bitlines_,
                    16u * bitlines_ - 1u,
                    samples_.size(),
                    location)) {
                return false;
            }
            const std::uint32_t sample = static_cast<std::uint32_t>(
                samples_[location.sample_offset]);
            value = (value << 1u)
                | ((sample & location.bit_mask) != 0u ? 1u : 0u);
            ++position_;
        }
        return true;
    }

    std::uint64_t position() const {
        return position_;
    }

private:
    const std::vector<std::int32_t>& samples_;
    std::uint32_t bitlines_ = 0u;
    std::uint64_t position_ = 0u;
};

bool decode_native_gr_indices(
    ChannelPayloadReader reader,
    std::uint64_t stream_bits,
    std::uint32_t rice_parameter,
    std::size_t sample_count,
    std::vector<std::uint32_t>& decoded,
    std::string& error) {
    decoded.clear();
    if (rice_parameter > 15u || stream_bits == 0u)
        return false;
    const std::size_t word_count = static_cast<std::size_t>(
        (stream_bits + 31u) / 32u);
    std::vector<std::uint32_t> words(word_count + 2u, 0u);
    for (std::uint64_t bit = 0u; bit < stream_bits; ++bit) {
        std::uint32_t value = 0u;
        if (!reader.read(1u, value)) {
            error = "encoded channel native Golomb-Rice stream is truncated";
            return false;
        }
        if (value != 0u) {
            words[static_cast<std::size_t>(bit >> 5u)] |=
                std::uint32_t{1} << (31u - static_cast<std::uint32_t>(bit & 31u));
        }
    }

    std::uint32_t accumulator = words[0];
    std::size_t word_cursor = 1u;
    std::int32_t bit_cursor = 31;
    std::uint64_t consumed_refill_bits = 0u;
    const auto read_refill_bit = [&]() -> std::uint32_t {
        const std::uint32_t bit =
            (words[word_cursor] >> static_cast<std::uint32_t>(bit_cursor)) & 1u;
        ++consumed_refill_bits;
        if (bit_cursor == 0) {
            ++word_cursor;
            bit_cursor = 31;
        } else {
            --bit_cursor;
        }
        return bit;
    };
    const auto append_bits = [&](std::uint32_t count) {
        for (std::uint32_t bit = 0u; bit < count; ++bit)
            accumulator = read_refill_bit() + 2u * accumulator;
    };

    decoded.reserve(sample_count);
    for (std::size_t sample = 0u; sample < sample_count; ++sample) {
        std::uint32_t unary_mask = 0x80000000u;
        std::uint32_t quotient = 0u;
        if ((accumulator & unary_mask) != 0u) {
            unary_mask >>= 1u;
            for (;;) {
                if (unary_mask == 1u) {
                    append_bits(31u);
                    unary_mask = 0x80000000u;
                }
                ++quotient;
                if ((accumulator & unary_mask) == 0u)
                    break;
                unary_mask >>= 1u;
                if (consumed_refill_bits > stream_bits + 64u) {
                    error = "encoded channel native Golomb-Rice unary value has no terminator";
                    return false;
                }
            }
        }
        std::uint32_t value_mask = unary_mask >> 1u;
        if (value_mask == 1u) {
            append_bits(31u);
            value_mask = 0x80000000u;
        }
        std::uint32_t remainder = 0u;
        for (std::uint32_t bit = 0u; bit < rice_parameter; ++bit) {
            if ((accumulator & value_mask) != 0u)
                remainder |= std::uint32_t{1} << bit;
            value_mask >>= 1u;
            if (value_mask == 1u) {
                append_bits(31u);
                value_mask = 0x80000000u;
            }
        }
        decoded.push_back((quotient << rice_parameter) | remainder);
        std::uint32_t flush_scale = value_mask << 1u;
        while (flush_scale != 0u) {
            accumulator = read_refill_bit() + 2u * accumulator;
            flush_scale *= 2u;
        }
        if (consumed_refill_bits > stream_bits + 64u) {
            error = "encoded channel native Golomb-Rice decoder exceeded its padded stream";
            return false;
        }
    }
    return true;
}

bool validate_channel_adol_prefix(
    const EncodedChannelFrame& channel,
    const std::vector<std::int32_t>& samples,
    const MetadataGroupRecord& group,
    std::uint8_t expected_layout_config,
    bool validate_entropy,
    std::vector<AdolInstruction>& parsed,
    std::string& error) {
    parsed.clear();
    ChannelPayloadReader reader(samples, channel.quantization_shift);
    std::uint32_t value = 0u;
    const std::uint32_t expected_frame_code =
        (static_cast<std::uint32_t>(samples.size()) >> 4u) - 1u;
    if (!reader.read(8u, value) || value != expected_frame_code
        || !reader.read(4u, value)
        || value != (channel.a3d_config_flag ? 3u : 2u)
        || !reader.read(4u, value)
        || value != 14u - channel.quantization_shift) {
        error = "encoded channel fixed A3D header readback failed";
        return false;
    }
    if (!reader.read(16u, value)
        || value != channel.channel_header) {
        error = "encoded channel header readback failed";
        return false;
    }
    for (const std::uint32_t expected : channel.metadata_words) {
        if (!reader.read(32u, value) || value != expected) {
            error = "encoded channel metadata readback failed";
            return false;
        }
    }
    ChannelMetadataCombined combined{};
    if (!combine_channel_metadata(
            channel.channel_header,
            channel.metadata_words,
            combined)) {
        error = "encoded channel metadata cannot determine seed geometry";
        return false;
    }
    const std::uint32_t seed_count =
        combined.mode == 3u ? 5u : (combined.mode == 2u ? 2u : 0u);
    const std::int32_t group_seeds[] = {
        group.seed0, group.seed1, group.seed2, group.seed3, group.seed4};
    for (std::uint32_t seed = 0u; seed < seed_count; ++seed) {
        if (!reader.read(32u, value)) {
            error = "encoded channel extrapolate seed stream is truncated";
            return false;
        }
        const std::int32_t expected = combined.mode == 2u
            ? group_seeds[1u - seed]
            : group_seeds[seed];
        if (value != static_cast<std::uint32_t>(expected)) {
            error = "encoded channel extrapolate seed readback differs from its group record";
            return false;
        }
    }
    if (!reader.read(8u, value) || value != 1u) {
        error = "encoded channel is missing its native ADOL block tag";
        return false;
    }
    bool first = true;
    bool terminated = false;
    for (std::uint32_t count = 0u; count < 64u; ++count) {
        std::uint32_t opcode = 0u;
        if (!reader.read(8u, opcode)) {
            error = "encoded channel ADOL opcode stream is truncated";
            return false;
        }
        if (opcode == 0u) {
            if (first) {
                error = "encoded channel ADOL block is empty";
                return false;
            }
            if (reader.position() < 48u
                || reader.position() - 48u > channel.serialized_bits) {
                error = "encoded channel ADOL terminator lies outside its serialized stream";
                return false;
            }
            terminated = true;
            break;
        }
        if (first) {
            if (opcode != 0x1Eu
                || !reader.read(8u, value)
                || value != expected_layout_config) {
                error = "encoded channel does not begin with its layout ADOL";
                return false;
            }
            parsed.push_back({
                static_cast<std::uint8_t>(opcode), value, 0u});
            first = false;
            continue;
        }
        if (opcode == 0x40u) {
            std::uint32_t second = 0u;
            if (!reader.read(8u, value)
                || !reader.read(8u, second)) {
                error = "encoded channel primary-downmix ADOL is truncated";
                return false;
            }
            parsed.push_back({
                static_cast<std::uint8_t>(opcode), value, second});
            continue;
        }
        const std::uint32_t payload_bits =
            adol_scalar_payload_bits(static_cast<std::uint8_t>(opcode));
        if (payload_bits == 0u || !reader.read(payload_bits, value)) {
            error = "encoded channel ADOL payload is invalid or truncated";
            return false;
        }
        parsed.push_back({
            static_cast<std::uint8_t>(opcode), value, 0u});
    }
    if (!terminated) {
        error = "encoded channel ADOL stream has no terminator";
        return false;
    }
    if (combined.mode < 2u || !validate_entropy)
        return true;
    if (combined.mode != group.analysis_arity
        || combined.bit_width == 0u
        || combined.base_index == 0u) {
        error = "encoded channel codebook geometry disagrees with its group record";
        return false;
    }

    const std::vector<std::uint32_t>* indices = nullptr;
    std::size_t residual_count = 0u;
    if (combined.mode == 2u) {
        residual_count = group.residuals.size();
        indices = &group.indices;
    } else {
        residual_count = group.mix3_residuals.size();
        indices = &group.mix3_indices;
    }
    if (indices == nullptr || residual_count == 0u
        || residual_count > combined.base_index) {
        error = "encoded channel codebook tables disagree with their selector capacity";
        return false;
    }
    const std::uint32_t dimensions = combined.mode == 3u ? 2u : 1u;
    for (std::uint32_t dimension = 0u; dimension < dimensions; ++dimension) {
        for (std::uint32_t entry = 0u; entry < combined.base_index; ++entry) {
            std::int32_t expected_signed = 0;
            if (entry < residual_count) {
                if (combined.mode == 2u) {
                    expected_signed = group.residuals[entry];
                } else {
                    const std::uint64_t packed = static_cast<std::uint64_t>(
                        group.mix3_residuals[entry]);
                    expected_signed = static_cast<std::int32_t>(
                        dimension == 0u ? packed : packed >> 32u);
                }
            }
            std::uint32_t expected_packed = 0u;
            if (!pack_golomb_error(
                    expected_signed, combined.bit_width, expected_packed)
                || !reader.read(combined.bit_width, value)) {
                error = "encoded channel codebook readback is truncated or invalid";
                return false;
            }
            if (value != expected_packed) {
                error = "encoded channel codebook readback differs at dimension "
                    + std::to_string(dimension)
                    + ", entry " + std::to_string(entry);
                return false;
            }
        }
    }

    const std::uint32_t rice_parameter =
        (channel.metadata_words[0] >> 24u) & 0xFu;
    if (rice_parameter > 15u || indices->size() != samples.size()) {
        error = "encoded channel Golomb-Rice geometry disagrees with its group record";
        return false;
    }
    if (reader.position() < 48u
        || reader.position() - 48u >= channel.serialized_bits) {
        error = "encoded channel Golomb-Rice stream exceeds serialized payload accounting";
        return false;
    }
    const std::uint64_t stream_bits =
        channel.serialized_bits - (reader.position() - 48u);
    std::vector<std::uint32_t> decoded_indices;
    if (!decode_native_gr_indices(
            reader,
            stream_bits,
            rice_parameter,
            indices->size(),
            decoded_indices,
            error)) {
        return false;
    }
    for (std::size_t index = 0u; index < indices->size(); ++index) {
        if (decoded_indices[index] != (*indices)[index]) {
            error = "encoded channel native Golomb-Rice readback differs at sample "
                + std::to_string(index)
                + " (expected " + std::to_string((*indices)[index])
                + ", got " + std::to_string(decoded_indices[index]) + ")";
            return false;
        }
    }
    return true;
}

bool same_adol_instructions(
    const std::vector<AdolInstruction>& left,
    const std::vector<AdolInstruction>& right) {
    if (left.size() != right.size())
        return false;
    for (std::size_t index = 0u; index < left.size(); ++index) {
        if (left[index].opcode != right[index].opcode
            || left[index].value != right[index].value
            || left[index].value_2 != right[index].value_2) {
            return false;
        }
    }
    return true;
}

bool validate_channel_sync_geometry(
    const std::vector<std::int32_t>& samples) {
    if (samples.size() < 16u)
        return false;
    for (std::size_t sample = 0u; sample < 16u; ++sample) {
        if ((static_cast<std::uint32_t>(samples[sample]) & 1u) == 0u)
            return false;
    }
    // parse_sync_pcm24_at tests the Projector gap at every following
    // sixteen-sample boundary. MuxIteratorFalse never consumes these slots.
    for (std::size_t sample = 16u; sample < samples.size(); sample += 16u) {
        if ((static_cast<std::uint32_t>(samples[sample]) & 1u) != 0u)
            return false;
    }
    return true;
}

bool validate_metadata_group_record(
    const MetadataGroupRecord& group,
    std::uint32_t frame_count,
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    std::string& error) {
    if (group.carrier_channel_id >= kCodecV3ChannelCount
        || (carrier_layout & (std::uint32_t{1} << group.carrier_channel_id)) == 0u
        || group.analysis_arity == 0u || group.analysis_arity > 3u
        || group.headroom_bits > 0x17u) {
        error = "encoded carrier unit has an invalid native group record";
        return false;
    }
    if (!group.quality_present
        || !std::isfinite(group.quality_error_db)
        || group.frame_quality.empty()
        || group.frame_quality.size() > 3u
        || group.frame_quality.size() < group.analysis_arity) {
        error = "encoded carrier unit group has invalid reconstruction quality";
        return false;
    }
    if (group.scaler_attempts == 0u
        || (!group.has_scaler_ix && group.scaler_ix != 0u)
        || (group.has_scaler_ix && group.scaler_ix == 0u)) {
        error = "encoded carrier unit group has invalid Rescaler state";
        return false;
    }
    const std::uint32_t expected_rescaler_bit_cost =
        group.has_scaler_ix ? 48u : 32u;
    if (group.rescaler_bit_cost != expected_rescaler_bit_cost) {
        error = "encoded carrier unit group has inconsistent Rescaler bit accounting";
        return false;
    }
    for (const NativeFrameQuality& quality : group.frame_quality) {
        if (!std::isfinite(quality.mse_db)
            || !std::isfinite(quality.peak_db)
            || !std::isfinite(quality.peak_to_rms_ratio)) {
            error = "encoded carrier unit group frame quality is invalid";
            return false;
        }
    }
    const std::uint32_t source_ids[] = {
        group.source0.channel_id,
        group.source1.channel_id,
        group.source2.channel_id};
    for (std::uint32_t index = 0u; index < group.analysis_arity; ++index) {
        if (source_ids[index] >= kCodecV3ChannelCount) {
            error = "encoded carrier unit group source is outside codec-v3 range";
            return false;
        }
        if ((original_layout & (std::uint32_t{1} << source_ids[index])) == 0u) {
            error = "encoded carrier unit group source is absent from its original layout";
            return false;
        }
        for (std::uint32_t prior = 0u; prior < index; ++prior) {
            if (source_ids[index] == source_ids[prior]) {
                error = "encoded carrier unit group sources are duplicated";
                return false;
            }
        }
    }
    if (group.analysis_arity == 1u) {
        if (!group.residuals.empty() || !group.indices.empty()
            || !group.levels.empty() || !group.mix3_residuals.empty()
            || !group.mix3_indices.empty() || !group.mix3_levels.empty()
            || group.gvm_learned
            || group.gvm_input.sample_count != 0u
            || group.gvm_input.residual_count != 0u
            || group.gvm_input.dimensions != 0u
            || group.quantizer_bit_budget != 0u
            || group.quantizer_fixed_bit_cost != 0u
            || group.quantizer_used_bits != 0u
            || group.golomb_index_bit_cost != 0u
            || group.gvm_search.initial_clusters != 1u
            || group.gvm_fit_status != 0u || group.gvm_compact_fit
            || group.gvm_learner_points != 0u
            || group.gvm_deterministic_seed
            || group.gvm_selected_clusters != 0u
            || group.gvm_learn_attempts != 0u
            || group.gvm_forced_zero_center
            || group.gvm_forced_zero_index != 0u) {
            error = "encoded carrier unit arity-1 group contains residual metadata";
            return false;
        }
        return true;
    }
    const std::uint32_t expected_dimensions =
        group.analysis_arity == 3u ? 2u : 1u;
    const std::uint64_t bit_line = 24u - group.headroom_bits;
    const std::uint64_t gross_bit_budget =
        static_cast<std::uint64_t>(frame_count) * bit_line;
    const std::uint64_t block_overhead =
        (static_cast<std::uint64_t>(frame_count) - 1u) >> 4u;
    if (group.gvm_input.sample_count != frame_count
        || group.gvm_input.dimensions != expected_dimensions
        || group.gvm_input.residual_count
            != static_cast<std::uint64_t>(frame_count) * expected_dimensions
        || group.quantizer_fixed_bit_cost
            < 0x20u + 0x80u + group.rescaler_bit_cost
        || group.quantizer_fixed_bit_cost > gross_bit_budget
        || block_overhead
            > gross_bit_budget - group.quantizer_fixed_bit_cost
        || group.quantizer_bit_budget
            != gross_bit_budget
                - group.quantizer_fixed_bit_cost
                - block_overhead
        || group.gvm_search.initial_clusters == 0u
        || group.gvm_search.initial_clusters > group.gvm_search.common_limit
        || group.gvm_search.minimum_clusters > group.gvm_search.initial_clusters) {
        error = "encoded carrier unit group has invalid native quantizer geometry or budget";
        return false;
    }
    const bool uses_gvm = group.gvm_learned;
    if (!native_gvm_fit_state_valid(group)) {
        error = "encoded carrier unit group has contradictory native GVM fit state";
        return false;
    }
    if ((group.cluster_backend == NativeClusterDeltasBackend::quantization
            && (group.analysis_arity != 2u || uses_gvm
                || group.mix2_shift_attempts == 0u
                || group.mix2_shift_attempts > 31u
                || group.gvm_learner_points != 0u
                || group.gvm_deterministic_seed))
        || (group.cluster_backend == NativeClusterDeltasBackend::gvm
            && (!uses_gvm || group.mix2_shift_attempts != 0u
                || group.gvm_learner_points != frame_count
                || (group.gvm_learner
                        == NativeGvmLearnerImplementation::modern
                    && !group.gvm_learned)
                || (group.gvm_learner
                        == NativeGvmLearnerImplementation::old_fast
                    && !group.gvm_learned)))) {
        error = "encoded carrier unit group disagrees with its cluster-deltas backend";
        return false;
    }
    if (group.cluster_backend == NativeClusterDeltasBackend::gvm
        && group.gvm_learner
            == NativeGvmLearnerImplementation::unavailable_mode1) {
        error = "encoded carrier unit references an unavailable native GVM learner";
        return false;
    }
    if ((group.gvm_learned
            && (group.gvm_selected_clusters == 0u
                || group.gvm_selected_clusters > group.gvm_search.common_limit
                || group.gvm_selected_clusters
                    != (group.analysis_arity == 3u
                        ? group.mix3_residuals.size()
                        : group.residuals.size())
                || group.gvm_learn_attempts == 0u
                || (group.gvm_forced_zero_center
                    && group.gvm_forced_zero_index
                        >= group.gvm_selected_clusters)))
        || (!group.gvm_learned
            && (group.gvm_selected_clusters != 0u
                || group.gvm_learn_attempts != 0u
                || group.gvm_forced_zero_center
                || group.gvm_forced_zero_index != 0u))) {
        error = "encoded carrier unit group has contradictory native GVM search output";
        return false;
    }
    const std::vector<std::uint32_t>* indices = &group.indices;
    const std::vector<std::uint64_t>* levels = &group.levels;
    if (group.analysis_arity == 3u) {
        if (!group.residuals.empty() || !group.indices.empty()
            || !group.levels.empty()) {
            error = "encoded carrier unit mix3 group contains mix2 residual metadata";
            return false;
        }
        indices = &group.mix3_indices;
        levels = &group.mix3_levels;
        if (group.mix3_residuals.empty()) {
            error = "encoded carrier unit mix3 group has no residual table";
            return false;
        }
    } else if (group.residuals.empty()) {
        error = "encoded carrier unit mix2 group has no residual table";
        return false;
    } else if (!group.mix3_residuals.empty() || !group.mix3_indices.empty()
        || !group.mix3_levels.empty()) {
        error = "encoded carrier unit mix2 group contains mix3 residual metadata";
        return false;
    }
    const std::size_t entry_count = group.analysis_arity == 3u
        ? group.mix3_residuals.size()
        : group.residuals.size();
    // Case-2 native metadata trims residuals to max(index)+1 while the
    // quantizer level schedule retained by this standalone result may still
    // contain the untrimmed tail. It only needs to cover every serialized
    // entry; case-3 normally has an exact one-to-one schedule.
    if (indices->size() != frame_count || levels->size() < entry_count) {
        error = "encoded carrier unit group table lengths do not match native geometry";
        return false;
    }
    if (entry_count == 0u || entry_count > kCodecV3ChannelCodebookMaxEntries) {
        error = "encoded carrier unit group residual table exceeds native selector capacity";
        return false;
    }
    if (group.cluster_backend == NativeClusterDeltasBackend::gvm) {
        std::uint64_t level_sum = 0u;
        for (const std::uint64_t level : *levels) {
            if (level_sum > std::numeric_limits<std::uint64_t>::max() - level) {
                error = "encoded carrier unit group level population overflows";
                return false;
            }
            level_sum += level;
        }
        if (level_sum != frame_count) {
            error = "encoded carrier unit group level population disagrees with frame count";
            return false;
        }
    }
    for (const std::uint32_t index : *indices) {
        if (index >= entry_count) {
            error = "encoded carrier unit group index exceeds its residual table";
            return false;
        }
    }
    const std::uint32_t level_pack_mode = group.analysis_arity == 3u
        ? group.mix3_level_pack_mode
        : group.level_pack_mode;
    if (group.residual_bit_width == 0u || group.residual_bit_width > 32u
        || level_pack_mode == 0u || level_pack_mode > 7u) {
        error = "encoded carrier unit group quantization fields are invalid";
        return false;
    }
    std::uint64_t golomb_bits = 0u;
    if (!golomb_rice_bit_count(
            *indices, level_pack_mode, golomb_bits)
        || golomb_bits != group.golomb_index_bit_cost
        || golomb_bits > group.quantizer_used_bits) {
        error = "encoded carrier unit group Rice cost is inconsistent";
        return false;
    }
    return true;
}

} // namespace

bool CarrierOutput::open(
    const std::string& path,
    std::uint32_t sample_rate,
    const std::vector<std::uint32_t>& channel_order,
    std::uint64_t frame_count,
    std::uint32_t channel_mask,
    std::string& error) {
    error.clear();
    writer_.emplace<std::monostate>();
    remaining_output_frames_ = frame_count;
    if (!channel_order_matches_mask(channel_order, channel_mask)) {
        error = "carrier output channel order does not match its channel mask";
        return false;
    }
    const std::string extension = lower_extension(path);
    if (extension == ".flac") {
        writer_.emplace<FlacPcm24Writer>();
        return std::get<FlacPcm24Writer>(writer_).open(
            path, sample_rate, channel_order, frame_count, error);
    }
    if (extension == ".wav" || extension == ".wave") {
        std::vector<std::uint32_t> wave_order;
        std::uint32_t wave_mask = 0u;
        if (!make_wave_carrier_plan(
                channel_order, wave_order, wave_mask, error)) {
            return false;
        }
        writer_.emplace<WavPcm24Writer>();
        return std::get<WavPcm24Writer>(writer_).open(
            path, sample_rate, wave_order, frame_count, error, wave_mask);
    }
    error = "output extension must be .wav, .wave or .flac";
    return false;
}

bool CarrierOutput::write(
    const std::vector<std::vector<std::int32_t>>& planes,
    std::uint32_t frame_count,
    std::string& error) {
    return std::visit([&](auto& writer) -> bool {
        using Writer = std::decay_t<decltype(writer)>;
        if constexpr (std::is_same_v<Writer, std::monostate>) {
            error = "carrier output is not open";
            return false;
        } else {
            return writer.write(planes, frame_count, error);
        }
    }, writer_);
}

bool CarrierOutput::write_unit(const EncodedCarrierUnit& unit, std::string& error) {
    error.clear();
    if (unit.carrier.descriptor.frame_count == 0u
        || unit.carrier.descriptor.layout == 0u) {
        error = "encoded carrier unit has no valid frame descriptor";
        return false;
    }
    if (unit.metadata_header.original_layout != 0u || !unit.metadata_groups.empty()) {
        std::uint32_t expected_carrier_layout = 0u;
        if (unit.metadata_header.original_layout == 0u
            || !codec_v3_carrier_layout(
                unit.metadata_header.original_layout, expected_carrier_layout)
            || expected_carrier_layout != unit.carrier.descriptor.layout
            || unit.metadata_header.unit_block_size
                != unit.carrier.descriptor.frame_count
            || unit.metadata_header.fixed_word != 2561u) {
            error = "encoded carrier unit has an invalid native metadata header";
            return false;
        }
    }
    if (unit.carrier.planes.size() != kCodecV3ChannelCount) {
        error = "encoded carrier unit has an invalid plane count";
        return false;
    }
    if (unit.metadata.mux_m < 3u || unit.metadata.mux_m > 14u
        || unit.metadata.adol_blocks.size() != unit.metadata.prefix.adol_block_count) {
        error = "encoded carrier unit has invalid PCM metadata geometry";
        return false;
    }
    std::uint8_t expected_config_id = 0u;
    if (!codec_v3_layout_to_channel_config(
            unit.metadata_header.original_layout, expected_config_id)
        || unit.metadata.prefix.channel_config[0] != expected_config_id
        || unit.metadata.prefix.channel_config[1] != 0xFFu
        || unit.metadata.prefix.channel_config[2] != 0xFFu
        || unit.metadata.prefix.channel_config[3] != 0xFFu) {
        error = "encoded carrier unit PCM metadata channel config disagrees with its layout";
        return false;
    }
    if (!validate_pcm_metadata_prefix(unit.metadata.prefix, error)) {
        error = "encoded carrier unit contains an invalid PCM metadata prefix: " + error;
        return false;
    }
    for (const std::vector<AdolInstruction>& block : unit.metadata.adol_blocks) {
        if (!validate_adol_block(block, error)) {
            error = "encoded carrier unit contains an invalid ADOL block: " + error;
            return false;
        }
    }
    if (unit.metadata_groups.size() > 8u) {
        error = "encoded carrier unit has invalid native group-record count";
        return false;
    }
    std::uint32_t group_mask = 0u;
    for (const MetadataGroupRecord& group : unit.metadata_groups) {
        if (!validate_metadata_group_record(
                group, unit.carrier.descriptor.frame_count,
                unit.metadata_header.original_layout,
                unit.carrier.descriptor.layout, error)) {
            return false;
        }
        if ((group_mask & (std::uint32_t{1} << group.carrier_channel_id)) != 0u) {
            error = "encoded carrier unit has duplicate native group records";
            return false;
        }
        group_mask |= std::uint32_t{1} << group.carrier_channel_id;
    }
    // prepare_metadata_unit_block_ walks the complete native carrier-group
    // list. A partial record set would leave a carrier channel without the
    // reconstruction metadata needed by the decoder, even if its PCM frame
    // happens to be present.
    if (group_mask != unit.carrier.descriptor.layout) {
        error = "encoded carrier unit metadata groups do not cover its layout";
        return false;
    }
    std::uint32_t native_metadata_channel = 0u;
    if (!codec_v3_metadata_carrier_channel(
            unit.carrier.descriptor.layout,
            native_metadata_channel)
        || unit.metadata_channel_id != native_metadata_channel
        || unit.metadata_channel_id >= kCodecV3ChannelCount
        || (unit.carrier.descriptor.layout
            & (std::uint32_t{1} << unit.metadata_channel_id)) == 0u) {
        error = "encoded carrier unit metadata channel differs from set_carrier_";
        return false;
    }
    std::vector<bool> serialized(kCodecV3ChannelCount, false);
    for (const EncodedChannelFrame& channel : unit.channels) {
        if (channel.channel_id >= kCodecV3ChannelCount
            || (unit.carrier.descriptor.layout
                & (std::uint32_t{1} << channel.channel_id)) == 0u
            || channel.word_count != channel.words.size()
            || channel.word_count > unit.carrier.descriptor.frame_count
            || channel.channel_header
                != kCodecV3SerializedChannelHeader
            || serialized[channel.channel_id]) {
            error = "encoded carrier unit contains a duplicate or inactive channel frame";
            return false;
        }
        std::uint64_t payload_capacity = 0u;
        if (!codec_v3_channel_payload_capacity(
                channel.quantization_shift,
                unit.carrier.descriptor.frame_count,
                payload_capacity)
            || channel.serialized_bits == 0u
            || channel.serialized_bits > payload_capacity) {
            error = "encoded carrier unit channel has invalid serialized bit accounting";
            return false;
        }
        serialized[channel.channel_id] = true;
        const MetadataGroupRecord* group_record = nullptr;
        for (const MetadataGroupRecord& candidate : unit.metadata_groups) {
            if (candidate.carrier_channel_id == channel.channel_id) {
                group_record = &candidate;
                break;
            }
        }
        if (group_record == nullptr) {
            error = "encoded carrier unit channel has no matching native group record";
            return false;
        }
        if (channel.base_scaler_present != group_record->has_scaler_ix
            || (channel.base_scaler_present
                && channel.base_scaler_index
                    != group_record->scaler_ix)
            || ((channel.metadata_words[0] >> 8u) & 0xFFu)
                != 1u) {
            error = "encoded channel ADOL/scaler state disagrees with its native group record";
            return false;
        }
        Crc16 crc;
        for (const std::int32_t word : unit.carrier.planes[channel.channel_id]) {
            const std::uint32_t raw = static_cast<std::uint32_t>(word);
            crc.process_words(&raw, 1u);
        }
        if (crc.stored_word() != channel.crc_word) {
            error = "encoded carrier unit channel CRC does not match its final plane";
            return false;
        }
        if (!validate_channel_sync_geometry(
                unit.carrier.planes[channel.channel_id])) {
            error = "encoded carrier unit channel has invalid mux sync-gap geometry";
            return false;
        }
        if (!validate_pcm_metadata_block(
                unit.carrier.planes[channel.channel_id],
                0u,
                unit.carrier.descriptor.frame_count,
                channel.quantization_shift)) {
            error = "encoded carrier unit channel mux sync/CRC failed validation";
            return false;
        }
        std::vector<AdolInstruction> parsed_adol;
        if (!validate_channel_adol_prefix(
                channel,
                unit.carrier.planes[channel.channel_id],
                *group_record,
                expected_config_id,
                validation_only_,
                parsed_adol,
                error)) {
            return false;
        }
        if (!same_adol_instructions(
                parsed_adol,
                unit.channel_adol[channel.channel_id])) {
            error = "encoded channel ADOL readback differs from its placement plan";
            return false;
        }
        ChannelMetadataCombined combined{};
        if (!combine_channel_metadata(
                channel.channel_header, channel.metadata_words, combined)
            || combined.mode != group_record->analysis_arity
            || channel.quantization_shift != 24u - group_record->headroom_bits
            || ((channel.metadata_words[0] >> 24u) & 0xFu)
                != (group_record->analysis_arity == 1u
                    ? 0u
                    : (group_record->analysis_arity == 3u
                        ? group_record->mix3_level_pack_mode
                        : group_record->level_pack_mode))) {
            error = "encoded carrier unit channel metadata disagrees with its native group record";
            return false;
        }
        const std::uint32_t source_ids[] = {
            group_record->source0.channel_id,
            group_record->source1.channel_id,
            group_record->source2.channel_id};
        for (std::uint32_t source = 0u; source < combined.mode; ++source) {
            if (combined.channel_ids[source] != source_ids[source]) {
                error = "encoded carrier unit channel source order disagrees with its native group record";
                return false;
            }
        }
    }
    for (std::uint32_t id = 0; id < kCodecV3ChannelCount; ++id) {
        const bool active = (unit.carrier.descriptor.layout & (std::uint32_t{1} << id)) != 0u;
        if (active && !serialized[id]) {
            error = "encoded carrier unit is missing a channel frame";
            return false;
        }
    }
    for (std::uint32_t id = 0; id < kCodecV3ChannelCount; ++id) {
        const bool active = (unit.carrier.descriptor.layout & (std::uint32_t{1} << id)) != 0u;
        if ((active && unit.carrier.planes[id].size() != unit.carrier.descriptor.frame_count)
            || (!active && !unit.carrier.planes[id].empty())) {
            error = "encoded carrier unit plane lengths do not match its layout";
            return false;
        }
    }
    if (validation_only_)
        return true;
    if (remaining_output_frames_ == 0u) {
        error = "encoded carrier exceeds the declared source frame count";
        return false;
    }
    const std::uint32_t output_frames = static_cast<std::uint32_t>(std::min<std::uint64_t>(
        remaining_output_frames_, unit.carrier.descriptor.frame_count));
    std::vector<std::vector<std::int32_t>> ordered_planes;
    ordered_planes.reserve(kCodecV3ChannelCount);
    for (std::uint32_t id = 0; id < kCodecV3ChannelCount; ++id) {
        const auto& source = unit.carrier.planes[id];
        if (source.empty())
            ordered_planes.emplace_back();
        else
            ordered_planes.emplace_back(source.begin(), source.begin() + output_frames);
    }
    if (!write(ordered_planes, output_frames, error))
        return false;
    remaining_output_frames_ -= output_frames;
    return true;
}

bool CarrierOutput::validate_unit(
    const EncodedCarrierUnit& unit,
    std::string& error) {
    validation_only_ = true;
    const bool valid = write_unit(unit, error);
    validation_only_ = false;
    return valid;
}

bool CarrierOutput::close(std::string& error) {
    return std::visit([&](auto& writer) -> bool {
        using Writer = std::decay_t<decltype(writer)>;
        if constexpr (std::is_same_v<Writer, std::monostate>) {
            error.clear();
            return true;
        } else {
            return writer.close(error);
        }
    }, writer_);
}

} // namespace auro3d:encode
