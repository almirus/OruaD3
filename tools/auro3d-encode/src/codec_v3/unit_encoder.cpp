#include "unit_encoder.hpp"

#include "channel_frame_plan.hpp"
#include "cts_dmx.hpp"
#include "dynamic_params.hpp"
#include "encoder_thread_pool.hpp"
#include "frame_descriptor.hpp"
#include "layout.hpp"
#include "layout_metadata.hpp"
#include "metadata_factory.hpp"
#include "prepare_metadata.hpp"
#include "prepare_mix.hpp"
#include "process_groups.hpp"
#include "input_rescale.hpp"
#include "scaler.hpp"
#include "select_loudness.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <future>
#include <limits>
#include <utility>

namespace auro3d::encode {
namespace {

bool validate_native_group_plan(
    std::uint32_t original_layout,
    const std::vector<EncodeGroupPlan>& supplied,
    std::string& error) {
    std::vector<EncodeGroupPlan> expected;
    if (!build_encode_group_plan(
            original_layout, expected, error)) {
        return false;
    }
    if (supplied.size() != expected.size()) {
        error = "unit encoder group plan does not cover the native carrier";
        return false;
    }
    for (std::size_t index = 0u; index < expected.size(); ++index) {
        const EncodeGroupPlan& left = supplied[index];
        const EncodeGroupPlan& right = expected[index];
        if (left.carrier_channel != right.carrier_channel
            || left.sources.arity != right.sources.arity) {
            error = "unit encoder group plan differs from get_original_channels";
            return false;
        }
        for (std::uint32_t source = 0u;
             source < right.sources.arity;
             ++source) {
            if (left.sources.channels[source]
                != right.sources.channels[source]) {
                error = "unit encoder group source order differs from native mapping";
                return false;
            }
        }
    }
    return true;
}

bool encode_v3_complete_unit_impl(
    const std::vector<std::vector<std::int32_t>>& unit_planes,
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    std::uint32_t sample_rate,
    const EncoderConfig& config,
    const std::vector<EncodeGroupPlan>& group_plan,
    std::uint32_t metadata_channel_id,
    std::array<NativeQualityFilter, 31>& quality_filters,
    NativeDitherState& dither,
    CtsDmxLimiter& cts_dmx_limiter,
    std::vector<AdolInstruction>& pending_delayable_adol,
    EncoderThreadPool* thread_pool,
    EncodedCarrierUnit& out,
    std::string& error) {
    error.clear();
    out = {};
    std::vector<std::vector<std::int32_t>> working_planes = unit_planes;
    if (config.sample_rate != sample_rate
        || config.original_layout != original_layout
        || config.unit_block_size == 0u) {
        error = "unit encoder arguments do not match its native configuration";
        return false;
    }
    const std::uint32_t config_status =
        encoder_config_validate(config);
    if (config_status != 0u) {
        error = "unit encoder Config::validate failed (status "
            + std::to_string(config_status) + ")";
        return false;
    }
    if (!validate_native_group_plan(
            original_layout, group_plan, error)) {
        return false;
    }
    if (config.primary_downmix_gain_present
        || config.limit_simple_present) {
        error =
            "primary-downmix and limit-simple ADOL are derived per group, "
            "not accepted as global optional metadata";
        return false;
    }
    std::uint32_t native_metadata_channel = 0u;
    if (!codec_v3_metadata_carrier_channel(
            carrier_layout, native_metadata_channel)
        || metadata_channel_id != native_metadata_channel) {
        error = "unit encoder metadata channel differs from set_carrier_";
        return false;
    }
    // Encoder::downmix_ @ 0x4E4520 is entered only when Config+132 equals
    // one. Config::init_defaults supplies that value through its final qword
    // write; accepting zero here would silently diverge from native failure.
    if (config.field_132.value != 1u) {
        error = "unit encoder native downmix gate is disabled";
        return false;
    }
    if (config.bit_line.present == 0u
        || config.bit_line.low > config.bit_line.high
        || config.bit_line.low < 3u
        || config.bit_line.high > 12u) {
        error = "unit encoder has an invalid configured bit-line range";
        return false;
    }
    if (!dither.initialized) {
        initialize_native_dither(
            dither,
            config.dither_seed_present,
            config.dither_seed);
    } else if (dither.config_seed_present
            != config.dither_seed_present
        || (config.dither_seed_present
            && dither.config_seed
                != static_cast<std::uint32_t>(
                    config.dither_seed))) {
        error = "unit encoder dither seed changed after runtime initialization";
        return false;
    }
    // downmix_ @ 0x4E4520: optional cts_dmx_coeff_limit_ when Config+136 is
    // set, then scalar divide from the Encoder+6368 scaler tree.
    std::array<std::uint8_t, 31> working_scaler_indices =
        config.input_scaler_indices;
    bool scalers_present = config.input_scalers_present;
    std::array<CtsGainEntry, 31> cts_input_gains{};
    bool cts_gains_from_dynamic = false;
    if (config.dynamic_params_present) {
        bool any_dynamic_gain = false;
        for (const DynamicChannelGain& gain :
             config.dynamic_params.original_gains) {
            if (gain.present) {
                any_dynamic_gain = true;
                break;
            }
        }
        if (any_dynamic_gain) {
            if (config.field_132.value != 1u) {
                error = "dynamic original gains require Config+132 value 1";
                return false;
            }
            bool dynamic_scalers = false;
            if (!apply_dynamic_original_gains(
                    config.dynamic_params,
                    original_layout,
                    carrier_layout,
                    config.field_132.present != 0u,
                    working_scaler_indices,
                    dynamic_scalers,
                    cts_input_gains,
                    error)) {
                return false;
            }
            if (dynamic_scalers)
                scalers_present = true;
            cts_gains_from_dynamic = config.field_132.present != 0u;
        }
    }
    if (config.field_132.present != 0u) {
        if (!cts_gains_from_dynamic && config.input_scalers_present) {
            // Native Dynamic AST writes Encoder+6384 from the same gains that
            // seeded the scaler tree. Reconstruct that table when explicit
            // scaler indices are present (gain_db = -20*log10(scaler)).
            for (std::uint32_t id = 0; id < 31u; ++id) {
                if ((original_layout & (std::uint32_t{1} << id)) == 0u)
                    continue;
                float scaler = 0.0f;
                if (!scaler_from_index(
                        working_scaler_indices[id], scaler)) {
                    error = "cts input gain reconstruction failed";
                    return false;
                }
                float gain_db = -144.0f;
                if (scaler > 0.0f && std::isfinite(scaler)) {
                    gain_db = -std::fmax(
                        std::log10(scaler) * 20.0f, -144.0f);
                } else if (std::isinf(scaler) && scaler > 0.0f) {
                    gain_db = -std::numeric_limits<float>::infinity();
                }
                cts_input_gains[id].gain_db = gain_db;
                cts_input_gains[id].present = true;
            }
        }
        if (!apply_cts_dmx_coeff_limit(
                working_planes,
                original_layout,
                sample_rate,
                cts_input_gains,
                cts_dmx_limiter,
                working_scaler_indices,
                error)) {
            return false;
        }
        for (std::uint32_t id = 0; id < 31u; ++id) {
            if (working_scaler_indices[id] != 0u) {
                scalers_present = true;
                break;
            }
        }
    }
    if (scalers_present
        && !apply_input_rescale_from_indices(
            working_planes,
            original_layout, working_scaler_indices, error)) {
        return false;
    }
    ConstFrameDescriptor original_descriptor{};
    CarrierUnit carrier_unit{};
    if (!make_input_descriptor(
            working_planes, original_layout, sample_rate,
            config.unit_block_size, original_descriptor, error)
        || !make_carrier_unit(
            carrier_layout, sample_rate, config.unit_block_size,
            carrier_unit, error)
        || !validate_encoder_frame_contract(
            original_descriptor, carrier_unit.descriptor, error)) {
        return false;
    }
    struct PlanAnalysisResult {
        AnalyzedEncodeGroup analysis{};
        NativeQualityFilter filter{};
        std::string error;
        bool accepted = false;
    };
    std::atomic<std::uint32_t> candidate_permits{0u};
    const auto active_at_every_bit_line =
        [&](const EncodeGroupPlan& plan) {
            const std::int32_t silence_limit = static_cast<std::int32_t>(
                std::uint32_t{1} << config.bit_line.high);
            for (std::uint32_t source = 0u;
                 source < plan.sources.arity;
                 ++source) {
                const std::uint32_t channel = plan.sources.channels[source];
                for (const std::int32_t sample : working_planes[channel]) {
                    if (sample < -silence_limit || sample > silence_limit)
                        return true;
                }
            }
            return false;
        };
    const auto analyze_plan =
        [&](const EncodeGroupPlan& plan,
            NativeQualityFilter filter,
            NativeDitherState* persistent_dither,
            bool allow_candidate_parallel) {
        PlanAnalysisResult plan_result{};
        plan_result.filter = filter;
        struct CandidateResult {
            AnalyzedEncodeGroup analysis{};
            std::string error;
            bool analyzed = false;
        };
        const auto analyze_one =
            [&](std::uint32_t bit_line,
                bool dither_enabled,
                NativeDitherState* dither_state) {
                CandidateResult result{};
                EncodeGroup candidate{};
                if (!materialize_encode_group(
                        plan,
                        working_planes,
                        bit_line,
                        config.field_52.value,
                        candidate,
                        result.error)) {
                    return result;
                }
                candidate.cluster_backend = config.cluster_backend;
                candidate.gvm = config.gvm;
                candidate.group_field_208 = config.reserve_extra_bits;
                NativeDitherState local_dither{};
                NativeDitherState& selected_dither =
                    dither_state != nullptr ? *dither_state : local_dither;
                std::uint32_t scaler_floor = 0u;
                std::uint32_t scaler_attempts = 0u;
                for (;;) {
                    ++scaler_attempts;
                    candidate.minimum_scaler_index =
                        static_cast<std::uint8_t>(scaler_floor);
                    if (analyze_encode_group(
                            candidate,
                            dither_enabled,
                            selected_dither,
                            result.analysis,
                            result.error)) {
                        result.analyzed = true;
                        result.analysis.scaler_attempts = scaler_attempts;
                        return result;
                    }
                    if (!result.analysis.carrier_overflow
                        || result.analysis.scaler_ix >= 0xF0u) {
                        return result;
                    }
                    scaler_floor =
                        static_cast<std::uint32_t>(
                            result.analysis.scaler_ix)
                        + 1u;
                }
            };

        // Active candidates never consume the persistent dither pool, and
        // their GVM searches are independent across bit lines. Native Config
        // exposes worker threads for this work; retain sequential processing
        // whenever the highest bit line can still classify the group silent.
        const bool always_active = active_at_every_bit_line(plan);
        std::vector<std::future<CandidateResult>> parallel_candidates;
        std::vector<bool> parallel_candidate_launched;
        const bool parallel_mode =
            allow_candidate_parallel
            && always_active
            && config.thread_workers.value != 0u;
        if (parallel_mode) {
            const std::size_t candidate_count =
                config.bit_line.high - config.bit_line.low + 1u;
            parallel_candidates.resize(candidate_count);
            parallel_candidate_launched.resize(candidate_count, false);
            for (std::uint32_t bit_line = config.bit_line.low;
                 bit_line <= config.bit_line.high;
                 ++bit_line) {
                // Filter::check follows DetectSilence/Rescaler/ComputeDeltas
                // in native process_groups_, but a definitely-active
                // candidate has no persistent dither or analysis side
                // effects before that check. Do not launch its expensive GVM
                // search when the filter state from earlier units already
                // rejects this bit line.
                if (filter.check(bit_line) == 2u) {
                    continue;
                }
                const std::size_t index =
                    bit_line - config.bit_line.low;
                std::uint32_t available =
                    candidate_permits.load(std::memory_order_relaxed);
                while (available != 0u
                    && !candidate_permits.compare_exchange_weak(
                        available,
                        available - 1u,
                        std::memory_order_acquire,
                        std::memory_order_relaxed)) {
                }
                if (available == 0u)
                    continue;
                parallel_candidate_launched[index] = true;
                parallel_candidates[index] = std::async(
                    std::launch::async,
                    [&, bit_line]() {
                        try {
                            CandidateResult result =
                                analyze_one(bit_line, false, nullptr);
                            candidate_permits.fetch_add(
                                1u, std::memory_order_release);
                            return result;
                        } catch (...) {
                            candidate_permits.fetch_add(
                                1u, std::memory_order_release);
                            throw;
                        }
                    });
            }
        }
        bool accepted = false;
        AnalyzedEncodeGroup best_analysis{};
        double best_error_db = 0.0;
        std::string candidate_error;
        for (std::uint32_t bit_line = config.bit_line.low;; ++bit_line) {
            const bool filtered = filter.check(bit_line) == 2u;
            const std::size_t candidate_index =
                bit_line - config.bit_line.low;
            if (filtered) {
                if (bit_line == config.bit_line.high)
                    break;
                continue;
            }
            CandidateResult result =
                !parallel_mode
                ? analyze_one(
                    bit_line,
                    persistent_dither != nullptr
                        && config.field_44.value != 0u,
                    persistent_dither)
                : parallel_candidate_launched[candidate_index]
                    ? parallel_candidates[candidate_index].get()
                    : analyze_one(bit_line, false, nullptr);
            candidate_error = std::move(result.error);
            AnalyzedEncodeGroup candidate_analysis =
                std::move(result.analysis);
            const bool analyzed_candidate =
                result.analyzed
                && (candidate_analysis.analysis_arity <= 1u
                    || candidate_analysis.quantizer_used_bits
                        <= candidate_analysis.quantizer_bit_budget);
            if (result.analyzed && !analyzed_candidate) {
                candidate_error =
                    "codec-v3 candidate entropy cost exceeds its channel budget";
            }
            // Native process_groups_ runs DetectSilence, Rescaler and
            // ComputeDeltas before Filter::check. In particular, a pruned
            // all-silent candidate still advances its persistent dither
            // pool. Analysis above preserves that state transition; only
            // candidate acceptance and filter update are skipped here.
            if (filtered) {
                if (bit_line == config.bit_line.high)
                    break;
                continue;
            }
            if (analyzed_candidate) {
                if (!candidate_analysis.quality_present
                    || !std::isfinite(candidate_analysis.quality_error_db)) {
                    candidate_error =
                        "codec-v3 candidate has no native reconstruction quality";
                } else if (candidate_analysis.silent) {
                    // Worker path around 0x4E8EF0 leaves Group+664 set for a
                    // silent candidate. It runs Mixer/ComputeQuality but
                    // deliberately skips Encoder::select_best_, whose
                    // ComputeQuality side effect updates the persistent
                    // carrier Filter. Feeding synthetic silence quality into
                    // that filter prunes every active bit line when audio
                    // starts after one or more silent UnitBlocks.
                    if (!accepted) {
                        best_error_db =
                            candidate_analysis.quality_error_db;
                        best_analysis =
                            std::move(candidate_analysis);
                        accepted = true;
                    }
                } else {
                    // ComputeQuality updates the carrier-keyed filter before
                    // Encoder::select_best_ compares Group+600. Lower (more
                    // negative) error dB wins; ties retain the earlier group.
                    filter.update(
                        bit_line,
                        static_cast<float>(
                            -candidate_analysis.quality_error_db));
                    if (!accepted
                        || candidate_analysis.quality_error_db
                            < best_error_db) {
                        best_error_db =
                            candidate_analysis.quality_error_db;
                        best_analysis =
                            std::move(candidate_analysis);
                        accepted = true;
                    }
                }
            }
            if (bit_line == config.bit_line.high)
                break;
        }
        if (!accepted && always_active) {
            // Filter is only a search accelerator. A stale three-point
            // window must not turn otherwise encodable PCM into a hard
            // failure when every candidate inside that window overflows.
            // Active candidates have no persistent dither side effects, so
            // retry the complete native bit-line range after resetting the
            // carrier-local filter.
            filter.clear();
            // Retry in the native low-to-high bit-line order and accept the
            // first candidate that both reconstructs and fits its complete
            // channel budget. The discarded filter window has supplied no
            // selectable candidate, so launching every more-lossy line would
            // only repeat GVM work after this fallback has recovered.
            for (std::uint32_t bit_line = config.bit_line.low;;
                 ++bit_line) {
                CandidateResult result =
                    analyze_one(bit_line, false, nullptr);
                candidate_error = std::move(result.error);
                const bool fits_budget =
                    result.analysis.analysis_arity <= 1u
                    || result.analysis.quantizer_used_bits
                        <= result.analysis.quantizer_bit_budget;
                if (result.analyzed
                    && fits_budget
                    && result.analysis.quality_present
                    && std::isfinite(
                        result.analysis.quality_error_db)) {
                    if (!result.analysis.silent) {
                        filter.update(
                            bit_line,
                            static_cast<float>(
                                -result.analysis.quality_error_db));
                    }
                    if (!accepted
                        || result.analysis.quality_error_db
                            < best_error_db) {
                        best_error_db =
                            result.analysis.quality_error_db;
                        best_analysis =
                            std::move(result.analysis);
                        accepted = true;
                    }
                    break;
                }
                if (bit_line == config.bit_line.high)
                    break;
            }
        }
        if (!accepted) {
            plan_result.error = candidate_error.empty()
                ? "no codec-v3 bit-line candidate was accepted"
                : candidate_error;
            return plan_result;
        }
        plan_result.analysis = std::move(best_analysis);
        plan_result.filter = std::move(filter);
        plan_result.accepted = true;
        return plan_result;
    };

    std::vector<PlanAnalysisResult> plan_results(group_plan.size());
    std::vector<std::future<PlanAnalysisResult>> group_futures(
        group_plan.size());
    std::vector<bool> group_launched(group_plan.size(), false);
    std::vector<std::size_t> active_groups;
    active_groups.reserve(group_plan.size());
    for (std::size_t index = 0u; index < group_plan.size(); ++index) {
        if (group_plan[index].sources.arity > 1u
            && active_at_every_bit_line(group_plan[index])) {
            active_groups.push_back(index);
        }
    }
    const bool parallel_groups =
        thread_pool != nullptr
        && thread_pool->worker_count() != 0u
        && active_groups.size() > 1u;
    const std::uint32_t total_threads =
        config.thread_workers.value + 1u;
    const std::uint32_t group_contexts = parallel_groups
        ? static_cast<std::uint32_t>(std::min(
            active_groups.size(), thread_pool->worker_count() + 1u))
        : 1u;
    candidate_permits.store(
        total_threads > group_contexts
            ? total_threads - group_contexts
            : 0u,
        std::memory_order_relaxed);
    if (parallel_groups) {
        // Keep the first active group on the caller thread. Remaining active
        // groups neither consume the shared dither pool nor share quality
        // filters, so their complete bit-line searches are independent.
        for (std::size_t active = 1u; active < active_groups.size(); ++active) {
            const std::size_t index = active_groups[active];
            const EncodeGroupPlan plan = group_plan[index];
            const NativeQualityFilter initial_filter =
                quality_filters[plan.carrier_channel];
            group_launched[index] = true;
            group_futures[index] = thread_pool->submit(
                [&, plan, initial_filter]() mutable {
                    return analyze_plan(
                        plan, std::move(initial_filter), nullptr, true);
                });
        }
    }
    for (std::size_t index = 0u; index < group_plan.size(); ++index) {
        if (group_launched[index])
            continue;
        const EncodeGroupPlan& plan = group_plan[index];
        const bool active = active_at_every_bit_line(plan);
        const bool expensive = plan.sources.arity > 1u;
        plan_results[index] = analyze_plan(
            plan,
            quality_filters[plan.carrier_channel],
            parallel_groups && active && expensive ? nullptr : &dither,
            active && expensive);
    }
    for (std::size_t index = 0u; index < group_plan.size(); ++index) {
        if (group_launched[index])
            plan_results[index] = group_futures[index].get();
    }

    std::vector<AnalyzedEncodeGroup> analyzed;
    analyzed.reserve(group_plan.size());
    for (std::size_t index = 0u; index < group_plan.size(); ++index) {
        PlanAnalysisResult& result = plan_results[index];
        if (!result.accepted) {
            error = result.error;
            return false;
        }
        const std::uint32_t carrier_channel =
            group_plan[index].carrier_channel;
        quality_filters[carrier_channel] = std::move(result.filter);
        analyzed.push_back(std::move(result.analysis));
    }
    std::vector<EncodedGroupPcm> encoded_groups;
    std::vector<MetadataGroupRecord> metadata_groups;
    if (!prepare_metadata_group_records(
            analyzed,
            scalers_present ? &working_scaler_indices : nullptr,
            metadata_groups,
            error)
        || !analyzed_groups_to_encoded_pcm(analyzed, encoded_groups, error)) {
        return false;
    }
    // Preserve the quantizer-path diagnostic while metadata records are
    // copied into the returned unit.  The wire serializer does not encode
    // this bit; it is consumed by validation/trace only.
    MetadataUnitHeader metadata_header{};
    if (!prepare_metadata_unit_header(
            original_layout, carrier_layout, sample_rate, config.unit_block_size,
            config.field_28.value, metadata_header, error)) {
        return false;
    }
    PcmMetadataBlock metadata;
    LayoutMetadataOptions metadata_options{};
    metadata_options.loudness_present =
        config.loudness_metadata_present;
    metadata_options.loudness =
        config.loudness_metadata;
    metadata_options.secondary_downmix_gains_present =
        config.secondary_downmix_gains_present;
    metadata_options.secondary_downmix_gains_db =
        config.secondary_downmix_gains_db;
    metadata_options.auromatic_present = config.auromatic_present;
    metadata_options.auromatic_profile = config.auromatic_profile;
    metadata_options.auromatic_mode = config.auromatic_mode;
    metadata_options.opcode_50_present = config.opcode_50_present;
    metadata_options.opcode_50_value = config.opcode_50_value;
    metadata_options.encoder_version_present =
        config.encoder_version_present;
    metadata_options.encoder_version = config.encoder_version;
    metadata_options.opcode_6e_present = config.opcode_6e_present;
    metadata_options.opcode_6e_value = config.opcode_6e_value;
    if (!make_layout_metadata_block(
            original_layout,
            carrier_layout,
            14u,
            metadata_options,
            metadata,
            error)
        || metadata.adol_blocks.size() != 1u
        || metadata.adol_blocks.front().empty()) {
        if (error.empty())
            error = "codec-v3 metadata factory did not produce the layout ADOL";
        return false;
    }
    const std::vector<AdolInstruction> common_adol{
        metadata.adol_blocks.front().front()};
    const std::vector<AdolInstruction> current_optional_adol(
        metadata.adol_blocks.front().begin() + 1,
        metadata.adol_blocks.front().end());
    // Encoder::encode @ 0x4E4330 calls prepare_metadata_unit_block_ before
    // prepare_mix_. Keep that order so a metadata construction failure cannot
    // leave a partially prepared carrier destination.
    if (!prepare_mix(encoded_groups, carrier_unit.descriptor, false, error))
        return false;
    std::array<std::vector<AdolInstruction>, 31> no_optional_adol{};
    std::vector<EncodedChannelFrame> base_frames;
    if (!build_analyzed_channel_frames(
            analyzed,
            config.unit_block_size,
            scalers_present ? &working_scaler_indices : nullptr,
            common_adol,
            no_optional_adol,
            base_frames,
            error)) {
        for (const AnalyzedEncodeGroup& group : analyzed) {
            error += " [carrier="
                + std::to_string(group.carrier.carrier_channel_id)
                + " arity=" + std::to_string(group.analysis_arity)
                + " bit_line="
                + std::to_string(group.carrier.quantization_shift)
                + " budget=" + std::to_string(group.quantizer_bit_budget)
                + " fixed="
                + std::to_string(group.quantizer_fixed_bit_cost)
                + " used=" + std::to_string(group.quantizer_used_bits)
                + " gr=" + std::to_string(group.golomb_index_bit_cost)
                + " rescaler=" + std::to_string(group.rescaler_bit_cost)
                + "]";
        }
        return false;
    }
    std::array<std::vector<AdolInstruction>, 31> optional_by_channel{};
    std::vector<std::uint64_t> remaining_bits;
    remaining_bits.reserve(base_frames.size());
    for (const EncodedChannelFrame& frame : base_frames) {
        std::uint64_t capacity = 0u;
        if (!codec_v3_channel_payload_capacity(
                frame.quantization_shift,
                config.unit_block_size,
                capacity)
            || frame.serialized_bits > capacity) {
            error = "base codec-v3 channel exceeds its mux capacity";
            return false;
        }
        remaining_bits.push_back(capacity - frame.serialized_bits);
    }
    const auto is_delayable_adol = [](std::uint8_t opcode) {
        return opcode == 70u
            || opcode == 71u
            || opcode == 80u
            || (opcode >= 128u && opcode <= 133u);
    };
    std::vector<AdolInstruction> delayable = pending_delayable_adol;
    for (const AdolInstruction& instruction : current_optional_adol) {
        if (!is_delayable_adol(instruction.opcode))
            continue;
        const auto existing = std::find_if(
            delayable.begin(),
            delayable.end(),
            [&instruction](const AdolInstruction& value) {
                return value.opcode == instruction.opcode;
            });
        if (existing == delayable.end())
            delayable.push_back(instruction);
        else
            *existing = instruction;
    }
    const auto instruction_bit_size = [](const AdolInstruction& instruction) {
        const std::uint32_t payload_bits = instruction.opcode == 64u
            ? 16u
            : adol_scalar_payload_bits(instruction.opcode);
        return payload_bits == 0u
            ? std::uint32_t{0}
            : 8u + payload_bits;
    };
    std::stable_sort(
        delayable.begin(),
        delayable.end(),
        [&instruction_bit_size](
            const AdolInstruction& left,
            const AdolInstruction& right) {
            return instruction_bit_size(left) > instruction_bit_size(right);
        });
    std::vector<AdolInstruction> optional_adol = delayable;
    for (const AdolInstruction& instruction : current_optional_adol) {
        if (!is_delayable_adol(instruction.opcode))
            optional_adol.push_back(instruction);
    }
    std::vector<AdolInstruction> next_pending_delayable;
    std::vector<AdolInstruction> emitted_optional_adol;
    for (const AdolInstruction& instruction : optional_adol) {
        const std::uint32_t payload_bits = instruction.opcode == 64u
            ? 16u
            : adol_scalar_payload_bits(instruction.opcode);
        if (instruction.opcode == 0u || payload_bits == 0u) {
            error = "optional codec-v3 ADOL instruction has no native width";
            return false;
        }
        const std::uint64_t instruction_bits = 8u + payload_bits;
        bool placed = false;
        for (std::size_t index = 0; index < base_frames.size(); ++index) {
            if (remaining_bits[index] < instruction_bits)
                continue;
            const std::uint32_t channel_id = base_frames[index].channel_id;
            optional_by_channel[channel_id].push_back(instruction);
            remaining_bits[index] -= instruction_bits;
            emitted_optional_adol.push_back(instruction);
            placed = true;
            break;
        }
        if (!placed && is_delayable_adol(instruction.opcode))
            next_pending_delayable.push_back(instruction);
    }
    pending_delayable_adol = std::move(next_pending_delayable);
    metadata.adol_blocks.front() = common_adol;
    metadata.adol_blocks.front().insert(
        metadata.adol_blocks.front().end(),
        emitted_optional_adol.begin(),
        emitted_optional_adol.end());
    std::vector<EncodedChannelFrame> frames;
    if (!build_analyzed_channel_frames(
            analyzed,
            config.unit_block_size,
            scalers_present ? &working_scaler_indices : nullptr,
            common_adol,
            optional_by_channel,
            frames,
            error)) {
        return false;
    }
    for (EncodedChannelFrame& frame : frames)
        frame.a3d_config_flag = config.field_28.value != 0u;
    if (!merge_channel_frames_into_carrier(carrier_unit, frames, error))
        return false;
    for (const EncodedChannelFrame& frame : frames) {
        if (frame.channel_id == metadata_channel_id) {
            metadata.mux_m = frame.quantization_shift;
            break;
        }
    }
    if (!assemble_carrier_unit(
            carrier_layout, sample_rate, config.unit_block_size,
            metadata_channel_id, frames, metadata, out, error)) {
        return false;
    }
    for (const MetadataGroupRecord& group : metadata_groups) {
        std::vector<AdolInstruction>& channel_adol =
            out.channel_adol[group.carrier_channel_id];
        channel_adol = common_adol;
        if (group.has_scaler_ix)
            channel_adol.push_back({65u, group.scaler_ix, 0u});
        const MetadataSourceRef sources[] = {
            group.source0, group.source1, group.source2};
        for (std::uint32_t source = 0u;
             source < group.analysis_arity;
             ++source) {
            if (sources[source].has_original_map) {
                channel_adol.push_back({
                    64u,
                    sources[source].channel_id,
                    sources[source].original_map,
                });
            }
        }
        const std::vector<AdolInstruction>& channel_optional =
            optional_by_channel[group.carrier_channel_id];
        channel_adol.insert(
            channel_adol.end(),
            channel_optional.begin(),
            channel_optional.end());
    }
    // Retain the native UnitBlock records beside the emitted channel streams
    // for exact output-boundary validation and trace diagnostics.
    out.metadata_header = metadata_header;
    out.metadata_groups = std::move(metadata_groups);
    return true;
}

/// set_dynamic_params carrier-gain / opcode_50 / auromatic → EncoderConfig
/// metadata fields (secondary ADOL 0x46 + UnitBlock cycler payloads). Call
/// before cycler gating so configured cyclers still suppress emission.
bool materialize_dynamic_metadata_fields(
    EncoderConfig& config,
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    std::string& error) {
    if (!config.dynamic_params_present)
        return true;
    bool secondary_from_dynamic = false;
    std::array<float, 31> secondary_gains = config.secondary_downmix_gains_db;
    if (!apply_dynamic_carrier_gains(
            config.dynamic_params,
            original_layout,
            carrier_layout,
            secondary_from_dynamic,
            secondary_gains,
            error)) {
        return false;
    }
    if (secondary_from_dynamic) {
        config.secondary_downmix_gains_present = true;
        config.secondary_downmix_gains_db = secondary_gains;
    }
    std::uint32_t opcode_50_value = config.opcode_50_value;
    std::uint32_t auromatic_profile = config.auromatic_profile;
    std::uint32_t auromatic_mode = config.auromatic_mode;
    bool opcode_50_present = config.opcode_50_present;
    bool auromatic_present = config.auromatic_present;
    if (!apply_dynamic_cycler_payloads(
            config.dynamic_params,
            original_layout,
            carrier_layout,
            opcode_50_present,
            opcode_50_value,
            auromatic_present,
            auromatic_profile,
            auromatic_mode,
            error)) {
        return false;
    }
    config.opcode_50_present = opcode_50_present;
    config.opcode_50_value = static_cast<std::uint8_t>(opcode_50_value);
    config.auromatic_present = auromatic_present;
    config.auromatic_profile = static_cast<std::uint8_t>(auromatic_profile);
    config.auromatic_mode = static_cast<std::uint8_t>(auromatic_mode);
    return true;
}

/// set_dynamic_params @ 0x4E3820 cycler arming after materializing payloads.
bool arm_dynamic_metadata_cyclers(
    const EncoderConfig& config,
    std::uint32_t carrier_layout,
    EncoderCyclers& cyclers,
    std::string& error) {
    if (!config.dynamic_params_present)
        return true;
    if (config.secondary_downmix_gains_present) {
        std::uint32_t packed = 0u;
        if (!pack_secondary_downmix_gains(
                carrier_layout,
                config.secondary_downmix_gains_db,
                packed,
                error)) {
            return false;
        }
        arm_metadata_cycler_on_value_change(
            cyclers.secondary_downmix, packed);
    }
    if (config.opcode_50_present) {
        arm_metadata_cycler_on_value_change(
            cyclers.opcode_50, config.opcode_50_value);
    }
    if (config.auromatic_present) {
        // Encoder+964 stores profile/mode as a little-endian word.
        const std::uint32_t word =
            static_cast<std::uint32_t>(config.auromatic_profile)
            | (static_cast<std::uint32_t>(config.auromatic_mode) << 8u);
        arm_auromatic_cycler_on_value_change(cyclers.auromatic, word);
    }
    return true;
}

bool same_loudness_measurement(
    const LoudnessMeasurement& lhs,
    const LoudnessMeasurement& rhs) {
    return lhs.type == rhs.type
        && lhs.value_absent == rhs.value_absent
        && lhs.value == rhs.value
        && lhs.value_2_present == rhs.value_2_present
        && lhs.value_2_absent == rhs.value_2_absent
        && lhs.value_2 == rhs.value_2
        && lhs.value_3_present == rhs.value_3_present
        && lhs.value_3_absent == rhs.value_3_absent
        && lhs.value_3 == rhs.value_3;
}

bool same_loudness_measurements(
    const std::vector<LoudnessMeasurement>& lhs,
    const std::vector<LoudnessMeasurement>& rhs) {
    if (lhs.size() != rhs.size())
        return false;
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        if (!same_loudness_measurement(lhs[i], rhs[i]))
            return false;
    }
    return true;
}

bool same_optional_u32(const OptionalU32& lhs, const OptionalU32& rhs) {
    return lhs.present == rhs.present && lhs.value == rhs.value;
}

bool same_native_gvm(
    const NativeGvmConfiguration& lhs,
    const NativeGvmConfiguration& rhs) {
    return lhs.mode == rhs.mode
        && lhs.learner == rhs.learner
        && lhs.qword_48 == rhs.qword_48
        && lhs.has_qword_48 == rhs.has_qword_48
        && lhs.flag_40 == rhs.flag_40
        && lhs.flag_41 == rhs.flag_41
        && lhs.flag_64 == rhs.flag_64
        && lhs.flag_65 == rhs.flag_65
        && lhs.flag_66 == rhs.flag_66
        && lhs.flag_67 == rhs.flag_67;
}

bool same_stream_encoder_config(
    const EncoderConfig& lhs,
    const EncoderConfig& rhs) {
    return lhs.profile == rhs.profile
        && lhs.bit_line.present == rhs.bit_line.present
        && lhs.bit_line.low == rhs.bit_line.low
        && lhs.bit_line.high == rhs.bit_line.high
        && same_optional_u32(lhs.field_28, rhs.field_28)
        && same_optional_u32(lhs.thread_workers, rhs.thread_workers)
        && same_optional_u32(lhs.field_44, rhs.field_44)
        && same_optional_u32(lhs.field_52, rhs.field_52)
        && same_optional_u32(lhs.field_60, rhs.field_60)
        && same_optional_u32(lhs.field_68, rhs.field_68)
        && same_optional_u32(lhs.field_76, rhs.field_76)
        && same_optional_u32(lhs.field_84, rhs.field_84)
        && same_optional_u32(lhs.field_92, rhs.field_92)
        && same_optional_u32(lhs.field_100, rhs.field_100)
        && same_optional_u32(lhs.field_108, rhs.field_108)
        && same_optional_u32(lhs.field_116, rhs.field_116)
        && same_optional_u32(lhs.field_124, rhs.field_124)
        && same_optional_u32(lhs.field_132, rhs.field_132)
        && lhs.dither_seed_present == rhs.dither_seed_present
        && lhs.dither_seed == rhs.dither_seed
        && lhs.reserve_extra_bits == rhs.reserve_extra_bits
        && lhs.cluster_backend == rhs.cluster_backend
        && same_native_gvm(lhs.gvm, rhs.gvm)
        && lhs.input_scalers_present == rhs.input_scalers_present
        && lhs.input_scaler_indices == rhs.input_scaler_indices;
}

bool bind_or_validate_stream_identity(
    EncoderRuntimeState& state,
    const EncoderConfig& config,
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    std::uint32_t sample_rate,
    std::uint32_t metadata_channel_id,
    std::string& error) {
    if (!state.stream_identity_initialized) {
        state.stream_identity_initialized = true;
        state.stream_sample_rate = sample_rate;
        state.stream_original_layout = original_layout;
        state.stream_carrier_layout = carrier_layout;
        state.stream_metadata_channel_id = metadata_channel_id;
        state.stream_initial_config = config;
        return true;
    }
    if (state.stream_sample_rate != sample_rate) {
        error = "codec-v3 runtime sample rate changed";
        return false;
    }
    if (state.stream_original_layout != original_layout) {
        error = "codec-v3 runtime original layout changed";
        return false;
    }
    if (state.stream_carrier_layout != carrier_layout) {
        error = "codec-v3 runtime carrier layout changed";
        return false;
    }
    if (state.stream_metadata_channel_id != metadata_channel_id) {
        error = "codec-v3 runtime metadata channel changed";
        return false;
    }
    if (!same_stream_encoder_config(state.stream_initial_config, config)) {
        error = "codec-v3 runtime constructor configuration changed";
        return false;
    }
    return true;
}

} // namespace

bool encode_v3_complete_unit(
    const std::vector<std::vector<std::int32_t>>& unit_planes,
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    std::uint32_t sample_rate,
    const EncoderConfig& config,
    const std::vector<EncodeGroupPlan>& group_plan,
    std::uint32_t metadata_channel_id,
    EncodedCarrierUnit& out,
    std::string& error) {
    std::array<NativeQualityFilter, 31> quality_filters{};
    NativeDitherState dither{};
    CtsDmxLimiter cts_dmx_limiter{};
    std::vector<AdolInstruction> pending_delayable_adol;
    EncoderConfig effective = config;
    if (!materialize_dynamic_metadata_fields(
            effective, original_layout, carrier_layout, error)) {
        return false;
    }
    if (effective.dynamic_params_present
        && !effective.dynamic_params.loudness_measurements.empty()) {
        std::vector<LoudnessScheduleEntry> loudness_schedule =
            make_default_loudness_schedule(sample_rate);
        if (!apply_dynamic_loudness_measurements(
                effective.dynamic_params,
                loudness_schedule,
                error)) {
            return false;
        }
        bool loudness_selected = false;
        LoudnessMetadata selected_loudness{};
        if (!select_loudness(
                loudness_schedule,
                0u,
                loudness_selected,
                selected_loudness,
                error)) {
            return false;
        }
        if (loudness_selected) {
            effective.loudness_metadata_present = true;
            effective.loudness_metadata = selected_loudness;
        }
    }
    return encode_v3_complete_unit_impl(
        unit_planes,
        original_layout,
        carrier_layout,
        sample_rate,
        effective,
        group_plan,
        metadata_channel_id,
        quality_filters,
        dither,
        cts_dmx_limiter,
        pending_delayable_adol,
        nullptr,
        out,
        error);
}

bool encode_v3_scheduled_unit(
    const std::vector<std::vector<std::int32_t>>& unit_planes,
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    std::uint32_t sample_rate,
    const EncoderConfig& config,
    const std::vector<EncodeGroupPlan>& group_plan,
    std::uint32_t metadata_channel_id,
    EncoderRuntimeState& state,
    EncodedCarrierUnit& out,
    std::string& error,
    EncoderThreadPool* thread_pool) {
    error.clear();
    if (state.processed_frames
        > std::numeric_limits<std::uint64_t>::max()
            - config.unit_block_size) {
        error = "codec-v3 runtime sample position overflows";
        return false;
    }
    EncoderRuntimeState candidate_state = state;
    if (!bind_or_validate_stream_identity(
            candidate_state,
            config,
            original_layout,
            carrier_layout,
            sample_rate,
            metadata_channel_id,
            error)) {
        return false;
    }
    const std::uint64_t unit_start_sample = state.processed_frames;
    const std::uint64_t current_sample_end =
        state.processed_frames + config.unit_block_size;
    if (candidate_state.loudness_schedule.empty()) {
        candidate_state.loudness_schedule =
            make_default_loudness_schedule(sample_rate);
        candidate_state.loudness_schedule_rate = sample_rate;
        candidate_state.dynamic_loudness_initialized = false;
        candidate_state.dynamic_params_were_present = false;
        candidate_state.dynamic_loudness_measurements.clear();
        // Construct LABEL_91 installs trunc(0.9583*sr) on all five periods
        // and the encoder_version / opcode_6e payload defaults.
        init_metadata_cycler_periods(
            candidate_state.metadata_cyclers, sample_rate);
        apply_construct_default_cycler_payloads(
            candidate_state.metadata_cyclers);
    }
    if (config.dynamic_params_present
        && (!candidate_state.dynamic_params_were_present
            || !candidate_state.dynamic_loudness_initialized
            || !same_loudness_measurements(
                candidate_state.dynamic_loudness_measurements,
                config.dynamic_params.loudness_measurements))) {
        if (!apply_dynamic_loudness_measurements(
                config.dynamic_params,
                candidate_state.loudness_schedule,
                error)) {
            return false;
        }
        candidate_state.dynamic_loudness_initialized = true;
        candidate_state.dynamic_loudness_measurements =
            config.dynamic_params.loudness_measurements;
    }
    candidate_state.dynamic_params_were_present =
        config.dynamic_params_present;
    EncoderConfig selected_config = config;
    // Materialize + arm before select_and_advance so a freshly armed cursor
    // of 0 is visible to prepare_metadata gating on this unit.
    if (!materialize_dynamic_metadata_fields(
            selected_config, original_layout, carrier_layout, error)
        || !arm_dynamic_metadata_cyclers(
            selected_config,
            carrier_layout,
            candidate_state.metadata_cyclers,
            error)) {
        return false;
    }
    bool loudness_selected = false;
    LoudnessMetadata selected_loudness{};
    if (!select_loudness(
            candidate_state.loudness_schedule,
            unit_start_sample,
            loudness_selected,
            selected_loudness,
            error)) {
        return false;
    }
    const EncoderMetadataSelection selection =
        candidate_state.metadata_cyclers.select_and_advance(
            current_sample_end);
    // select_loudness_ owns Encoder+12712 for this unit. Explicit config
    // loudness remains only when the schedule did not select a payload.
    if (loudness_selected) {
        selected_config.loudness_metadata_present = true;
        selected_config.loudness_metadata = selected_loudness;
    }
    // Primary 0x40 (parser 64) is emitted per original-map source inside
    // channel frames; limit-simple 0x41 (parser 65) uses Group+392.
    selected_config.secondary_downmix_gains_present =
        selected_config.secondary_downmix_gains_present
        && (!candidate_state.metadata_cyclers.secondary_downmix.configured
            || !candidate_state.metadata_cyclers.secondary_downmix.enabled
            || selection.secondary_downmix);
    selected_config.auromatic_present =
        selected_config.auromatic_present
        && (!candidate_state.metadata_cyclers.auromatic.configured
            || !candidate_state.metadata_cyclers.auromatic.enabled
            || selection.auromatic);
    selected_config.opcode_50_present =
        selected_config.opcode_50_present
        && (!candidate_state.metadata_cyclers.opcode_50.configured
            || !candidate_state.metadata_cyclers.opcode_50.enabled
            || selection.opcode_50);
    // set_cyclers_ writes dword 0x01060303 at +6208: the high byte enables
    // +6211, while the first three bytes carry version 3.3.6. Cursor -1
    // suppresses the first unit and makes the second unit eligible.
    if (candidate_state.metadata_cyclers.encoder_version.enabled) {
        selected_config.encoder_version_present = true;
        selected_config.encoder_version =
            candidate_state.metadata_cyclers.encoder_version.armed_value;
    }
    selected_config.encoder_version_present =
        selected_config.encoder_version_present
        && (!candidate_state.metadata_cyclers.encoder_version.configured
            || !candidate_state.metadata_cyclers.encoder_version.enabled
            || selection.encoder_version);
    // Construct always enables opcode_6e with the fixed dword; surface it on
    // the config so gating can emit when the cycler is due.
    if (candidate_state.metadata_cyclers.opcode_6e.enabled) {
        selected_config.opcode_6e_present = true;
        selected_config.opcode_6e_value =
            candidate_state.metadata_cyclers.opcode_6e.armed_value;
    }
    selected_config.opcode_6e_present =
        selected_config.opcode_6e_present
        && (!candidate_state.metadata_cyclers.opcode_6e.configured
            || !candidate_state.metadata_cyclers.opcode_6e.enabled
            || selection.opcode_6e);

    EncodedCarrierUnit candidate{};
    if (!encode_v3_complete_unit_impl(
            unit_planes,
            original_layout,
            carrier_layout,
            sample_rate,
            selected_config,
            group_plan,
            metadata_channel_id,
            candidate_state.quality_filters,
            candidate_state.dither,
            candidate_state.cts_dmx_limiter,
            candidate_state.pending_delayable_adol,
            thread_pool,
            candidate,
            error)) {
        return false;
    }
    candidate_state.processed_frames = current_sample_end;
    state = std::move(candidate_state);
    out = std::move(candidate);
    return true;
}

} // namespace auro3d::encode
