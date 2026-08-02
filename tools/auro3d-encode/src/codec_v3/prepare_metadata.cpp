#include "prepare_metadata.hpp"

#include "frame_descriptor.hpp"
#include "channel_metadata.hpp"
#include "layout.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>

namespace auro3d::encode {
namespace {

bool native_gvm_fit_state_valid(const AnalyzedEncodeGroup& group) {
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

void apply_source_original_map(
    MetadataSourceRef& source,
    const std::array<std::uint8_t, 31>* input_scaler_indices) {
    // prepare_metadata_unit_block_ @ 0x4E60B9: tree byte at Encoder+6368/+32
    // is stored only when non-zero.
    if (input_scaler_indices == nullptr
        || source.channel_id >= input_scaler_indices->size()) {
        return;
    }
    const std::uint8_t index = (*input_scaler_indices)[source.channel_id];
    if (index == 0u)
        return;
    source.has_original_map = true;
    source.original_map = index;
}

bool fill_case1(
    const AnalyzedEncodeGroup& group,
    const std::array<std::uint8_t, 31>* input_scaler_indices,
    MetadataGroupRecord& out,
    std::string& error) {
    if (group.analysis_source_ids.size() != 1u) {
        error = "metadata case 1 expects exactly one analysis source id";
        return false;
    }
    if (group.analysis_source_ids[0] >= kCodecV3ChannelCount) {
        error = "metadata case 1 source channel is outside codec-v3 range";
        return false;
    }
    out.analysis_arity = 1u;
    out.source0.channel_id = group.analysis_source_ids[0];
    apply_source_original_map(out.source0, input_scaler_indices);
    return true;
}

bool fill_case2(
    const AnalyzedEncodeGroup& group,
    const std::array<std::uint8_t, 31>* input_scaler_indices,
    MetadataGroupRecord& out,
    std::string& error) {
    if (group.analysis_source_ids.size() != 2u) {
        error = "metadata case 2 expects two analysis source ids";
        return false;
    }
    if (group.analysis_source_ids[0] >= kCodecV3ChannelCount
        || group.analysis_source_ids[1] >= kCodecV3ChannelCount
        || group.analysis_source_ids[0] == group.analysis_source_ids[1]) {
        error = "metadata case 2 source channels are invalid or duplicated";
        return false;
    }
    if (group.mix2_indices.size() != group.carrier.samples.size()
        || group.carrier.samples.empty()) {
        error = "metadata case 2 index count must equal carrier sample count";
        return false;
    }
    if (group.gvm_input.sample_count != group.carrier.samples.size()
        || group.gvm_input.residual_count != group.carrier.samples.size()
        || group.gvm_input.dimensions != 1u) {
        error = "metadata case 2 has inconsistent native GVM input geometry";
        return false;
    }
    if ((group.cluster_backend == NativeClusterDeltasBackend::gvm
            && (group.mix2_shift_attempts != 0u
                || group.gvm_learner_points != group.carrier.samples.size()
                || !group.gvm_learned))
        || (group.cluster_backend == NativeClusterDeltasBackend::quantization
            && (group.gvm_learned
                || group.mix2_shift_attempts == 0u
                || group.mix2_shift_attempts > 31u
                || group.gvm_learner_points != 0u
                || group.gvm_deterministic_seed))) {
        error = "metadata case 2 disagrees with its cluster-deltas backend";
        return false;
    }
    if (!native_gvm_fit_state_valid(group)) {
        error = "metadata case 2 has inconsistent native GVM fit status";
        return false;
    }
    if ((group.gvm_learned
            && (group.gvm_selected_clusters == 0u
                || group.gvm_selected_clusters > group.gvm_search.common_limit
                || group.gvm_selected_clusters
                    != group.mix2_residuals.size()
                || group.gvm_learn_attempts == 0u
                || (group.gvm_forced_zero_center
                    && group.gvm_forced_zero_index
                        >= group.gvm_selected_clusters)))
        || (!group.gvm_learned
            && (group.gvm_selected_clusters != 0u
                || group.gvm_learn_attempts != 0u
                || group.gvm_forced_zero_center
                || group.gvm_forced_zero_index != 0u))) {
        error = "metadata case 2 has inconsistent native GVM search result";
        return false;
    }
    if (group.vq_shift > 0xFFu || group.residual_bit_width > 0xFFu) {
        error = "metadata case 2 VQ fields exceed byte range";
        return false;
    }

    std::uint64_t max_index = 0u;
    for (std::uint64_t index : group.mix2_indices) {
        if (index > max_index)
            max_index = index;
    }
    if (max_index >= group.mix2_residuals.size()
        || max_index == std::numeric_limits<std::uint64_t>::max()) {
        error = "metadata case 2 residual table shorter than max index";
        return false;
    }
    if (max_index >= std::numeric_limits<std::uint32_t>::max()) {
        error = "metadata case 2 residual table exceeds native selector storage";
        return false;
    }
    const std::size_t residual_count = static_cast<std::size_t>(max_index + 1u);
    std::uint32_t selector = 0u;
    std::uint32_t base_index = 0u;
    if (!channel_metadata_selector_from_count(
            static_cast<std::uint32_t>(residual_count), selector, base_index)) {
        error = "metadata case 2 residual table has no native channel selector";
        return false;
    }
    if (selector > 0x53u || base_index < residual_count) {
        error = "metadata case 2 selector capacity is below its residual table";
        return false;
    }
    // Quantization keeps its complete level schedule, while native metadata
    // copies only residual entries through max(index)+1.  The schedule may
    // therefore be longer than the trimmed table; it must merely cover every
    // entry that can be selected by the serialized indices.
    if (group.mix2_levels.size() < residual_count
        || group.mix2_level_pack_mode == 0u
        || group.mix2_level_pack_mode > 7u) {
        error = "metadata case 2 quantization level table is shorter than its residual table";
        return false;
    }
    if (group.cluster_backend == NativeClusterDeltasBackend::gvm) {
        std::uint64_t level_sum = 0u;
        for (const std::uint64_t level : group.mix2_levels) {
            if (level_sum > std::numeric_limits<std::uint64_t>::max() - level) {
                error = "metadata case 2 level population overflows";
                return false;
            }
            level_sum += level;
        }
        if (level_sum != group.carrier.samples.size()) {
            error = "metadata case 2 level population does not equal sample count";
            return false;
        }
    }

    out.analysis_arity = 2u;
    out.source0.channel_id = group.analysis_source_ids[0];
    out.source1.channel_id = group.analysis_source_ids[1];
    apply_source_original_map(out.source0, input_scaler_indices);
    apply_source_original_map(out.source1, input_scaler_indices);
    out.seed0 = group.mix2_seeds.seed0;
    out.seed1 = group.mix2_seeds.seed1;
    out.vq_shift = static_cast<std::uint8_t>(group.vq_shift);
    out.residual_bit_width = static_cast<std::uint8_t>(group.residual_bit_width);
    out.residuals.assign(
        group.mix2_residuals.begin(),
        group.mix2_residuals.begin() + static_cast<std::ptrdiff_t>(residual_count));
    out.levels = group.mix2_levels;
    out.level_pack_mode = group.mix2_level_pack_mode;
    out.indices.reserve(group.mix2_indices.size());
    for (std::uint64_t index : group.mix2_indices) {
        if (index > std::numeric_limits<std::uint32_t>::max()) {
            error = "metadata case 2 index exceeds 32-bit storage";
            return false;
        }
        out.indices.push_back(static_cast<std::uint32_t>(index));
    }
    return true;
}

bool fill_case3(
    const AnalyzedEncodeGroup& group,
    const std::array<std::uint8_t, 31>* input_scaler_indices,
    MetadataGroupRecord& out,
    std::string& error) {
    if (group.analysis_source_ids.size() != 3u) {
        error = "metadata case 3 expects three analysis source ids";
        return false;
    }
    for (std::size_t index = 0; index < group.analysis_source_ids.size(); ++index) {
        const std::uint32_t id = group.analysis_source_ids[index];
        if (id >= kCodecV3ChannelCount) {
            error = "metadata case 3 source channel is outside codec-v3 range";
            return false;
        }
        for (std::size_t prior = 0; prior < index; ++prior) {
            if (id == group.analysis_source_ids[prior]) {
                error = "metadata case 3 source channels are duplicated";
                return false;
            }
        }
    }
    if (group.mix3_indices.size() != group.carrier.samples.size()
        || group.carrier.samples.empty()) {
        error = "metadata case 3 index count must equal carrier sample count";
        return false;
    }
    if (group.carrier.samples.size() > std::numeric_limits<std::uint32_t>::max() / 2u
        || group.gvm_input.sample_count != group.carrier.samples.size()
        || group.gvm_input.residual_count != group.carrier.samples.size() * 2u
        || group.gvm_input.dimensions != 2u) {
        error = "metadata case 3 has inconsistent native GVM input geometry";
        return false;
    }
    if (!group.gvm_learned) {
        error = "metadata case 3 has inconsistent quantizer diagnostics";
        return false;
    }
    if (group.cluster_backend != NativeClusterDeltasBackend::gvm) {
        error = "metadata case 3 requires the native GVM backend";
        return false;
    }
    if (group.gvm_learner_points != group.carrier.samples.size()) {
        error = "metadata case 3 has incomplete native GVM learner input";
        return false;
    }
    if (!native_gvm_fit_state_valid(group)) {
        error = "metadata case 3 has inconsistent native GVM fit status";
        return false;
    }
    if ((group.gvm_learned
            && (group.gvm_selected_clusters == 0u
                || group.gvm_selected_clusters > group.gvm_search.common_limit
                || group.gvm_selected_clusters
                    != group.mix3_residuals.size()
                || group.gvm_learn_attempts == 0u
                || (group.gvm_forced_zero_center
                    && group.gvm_forced_zero_index
                        >= group.gvm_selected_clusters)))
        || (!group.gvm_learned
            && (group.gvm_selected_clusters != 0u
                || group.gvm_learn_attempts != 0u
                || group.gvm_forced_zero_center
                || group.gvm_forced_zero_index != 0u))) {
        error = "metadata case 3 has inconsistent native GVM search result";
        return false;
    }
    if (group.vq_shift > 0xFFu || group.residual_bit_width > 0xFFu) {
        error = "metadata case 3 VQ fields exceed byte range";
        return false;
    }
    std::uint64_t max_index = 0u;
    for (const std::uint64_t index : group.mix3_indices)
        max_index = std::max(max_index, index);
    if (max_index >= group.mix3_residuals.size()
        || max_index == std::numeric_limits<std::uint64_t>::max()) {
        error = "metadata case 3 residual table shorter than max index";
        return false;
    }
    if (max_index + 1u > std::numeric_limits<std::size_t>::max()) {
        error = "metadata case 3 residual table size overflows size_t";
        return false;
    }
    if (max_index >= std::numeric_limits<std::uint32_t>::max()) {
        error = "metadata case 3 residual table exceeds native selector storage";
        return false;
    }
    const std::size_t residual_count = static_cast<std::size_t>(max_index + 1u);
    std::uint32_t selector = 0u;
    std::uint32_t base_index = 0u;
    if (!channel_metadata_selector_from_count(
            static_cast<std::uint32_t>(residual_count), selector, base_index)) {
        error = "metadata case 3 residual table has no native channel selector";
        return false;
    }
    if (selector > 0x53u || base_index < residual_count) {
        error = "metadata case 3 selector capacity is below its residual table";
        return false;
    }
    // Keep the same lower-bound rule as case 2.  The native record's
    // residual vector is trimmed by the highest referenced index, whereas a
    // quantizer can retain a longer internal level schedule.
    if (group.mix3_levels.size() < residual_count
        || group.mix3_level_pack_mode == 0u
        || group.mix3_level_pack_mode > 7u) {
        error = "metadata case 3 quantization level table is shorter than its residual table";
        return false;
    }
    std::uint64_t level_sum = 0u;
    for (const std::uint64_t level : group.mix3_levels) {
        if (level_sum > std::numeric_limits<std::uint64_t>::max() - level) {
            error = "metadata case 3 level population overflows";
            return false;
        }
        level_sum += level;
    }
    if (level_sum != group.carrier.samples.size()) {
        error = "metadata case 3 level population does not equal sample count";
        return false;
    }

    out.analysis_arity = 3u;
    out.source0.channel_id = group.analysis_source_ids[0];
    out.source1.channel_id = group.analysis_source_ids[1];
    out.source2.channel_id = group.analysis_source_ids[2];
    apply_source_original_map(out.source0, input_scaler_indices);
    apply_source_original_map(out.source1, input_scaler_indices);
    apply_source_original_map(out.source2, input_scaler_indices);
    out.seed0 = group.mix3_seeds[0];
    out.seed1 = group.mix3_seeds[1];
    out.seed2 = group.mix3_seeds[2];
    out.seed3 = group.mix3_seeds[3];
    out.seed4 = group.mix3_seeds[4];
    out.vq_shift = static_cast<std::uint8_t>(group.vq_shift);
    out.residual_bit_width = static_cast<std::uint8_t>(group.residual_bit_width);
    out.mix3_residuals.assign(
        group.mix3_residuals.begin(),
        group.mix3_residuals.begin() + static_cast<std::ptrdiff_t>(residual_count));
    out.mix3_levels = group.mix3_levels;
    out.mix3_level_pack_mode = group.mix3_level_pack_mode;
    out.mix3_indices.reserve(group.mix3_indices.size());
    for (const std::uint64_t index : group.mix3_indices) {
        if (index > std::numeric_limits<std::uint32_t>::max()) {
            error = "metadata case 3 index exceeds 32-bit storage";
            return false;
        }
        out.mix3_indices.push_back(static_cast<std::uint32_t>(index));
    }
    return true;
}

} // namespace

bool prepare_metadata_unit_header(
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    std::uint32_t sample_rate,
    std::uint32_t unit_block_size,
    std::uint32_t field_28,
    MetadataUnitHeader& out,
    std::string& error) {
    error.clear();
    out = {};
    std::uint32_t expected_carrier = 0;
    if (!codec_v3_carrier_layout(original_layout, expected_carrier)
        || expected_carrier != carrier_layout) {
        error = "prepare_metadata header layouts are not a native carrier pair";
        return false;
    }
    if (!codec_v3_sample_rate_supported(sample_rate)
        || !codec_v3_unit_block_size_supported(unit_block_size)) {
        error = "prepare_metadata header config fields are unsupported";
        return false;
    }
    // prepare_metadata_unit_block_ @ 0x4E5BC0:
    //   *a2 = shuffle(Config+4, Config+8, 225): unit_block_size at +0,
    //   original_layout at +4; *(a2+8) = Config+32 (field_28 value);
    //   *(a2+12) = 2561.
    out.unit_block_size = unit_block_size;
    out.original_layout = original_layout;
    out.field_28 = field_28;
    out.fixed_word = 2561u;
    return true;
}

bool prepare_metadata_group_records(
    const std::vector<AnalyzedEncodeGroup>& analyzed,
    std::vector<MetadataGroupRecord>& out,
    std::string& error) {
    return prepare_metadata_group_records(analyzed, nullptr, out, error);
}

bool prepare_metadata_group_records(
    const std::vector<AnalyzedEncodeGroup>& analyzed,
    const std::array<std::uint8_t, 31>* input_scaler_indices,
    std::vector<MetadataGroupRecord>& out,
    std::string& error) {
    error.clear();
    out.clear();
    if (analyzed.size() > 8u) {
        // Native gate: Encoder+12744 group counter <= 8 before writing records.
        error = "prepare_metadata rejects more than 8 group records";
        return false;
    }
    out.reserve(analyzed.size());
    std::uint32_t carrier_mask = 0u;
    for (const AnalyzedEncodeGroup& group : analyzed) {
        if (!group.carrier_ready) {
            error = "prepare_metadata group is not carrier_ready";
            return false;
        }
        const std::uint32_t bit_line = group.carrier.quantization_shift;
        if (bit_line > 24u) {
            error = "prepare_metadata bit_line exceeds 24";
            return false;
        }
        const std::uint32_t headroom = 24u - bit_line;
        if (headroom > 0x17u) {
            error = "prepare_metadata headroom_bits out of range";
            return false;
        }

        MetadataGroupRecord record{};
        if (group.carrier.carrier_channel_id >= kCodecV3ChannelCount
            || (carrier_mask & (std::uint32_t{1} << group.carrier.carrier_channel_id)) != 0u) {
            error = "metadata groups contain a duplicate or invalid carrier channel";
            return false;
        }
        carrier_mask |= std::uint32_t{1} << group.carrier.carrier_channel_id;
        record.carrier_channel_id = group.carrier.carrier_channel_id;
        record.headroom_bits = static_cast<std::uint8_t>(headroom);
        record.gvm_learned = group.gvm_learned;
        record.mix2_shift_attempts = group.mix2_shift_attempts;
        record.gvm_input = group.gvm_input;
        record.quantizer_bit_budget = group.quantizer_bit_budget;
        record.quantizer_fixed_bit_cost = group.quantizer_fixed_bit_cost;
        record.quantizer_used_bits = group.quantizer_used_bits;
        record.golomb_index_bit_cost =
            group.golomb_index_bit_cost;
        record.rescaler_bit_cost = group.rescaler_bit_cost;
        record.gvm_search = group.gvm_search;
        record.gvm_fit_status = group.gvm_fit_status;
        record.gvm_compact_fit = group.gvm_compact_fit;
        record.cluster_backend = group.cluster_backend;
        record.gvm_learner = group.gvm_learner;
        record.gvm_learner_points = group.gvm_learner_points;
        record.gvm_deterministic_seed = group.gvm_deterministic_seed;
        record.gvm_selected_clusters = group.gvm_selected_clusters;
        record.gvm_learn_attempts = group.gvm_learn_attempts;
        record.gvm_forced_zero_center = group.gvm_forced_zero_center;
        record.gvm_forced_zero_index = group.gvm_forced_zero_index;
        record.quality_present = group.quality_present;
        record.quality_error_db = group.quality_error_db;
        record.frame_quality = group.frame_quality;
        if (group.has_scaler_ix) {
            record.has_scaler_ix = true;
            record.scaler_ix = group.scaler_ix;
        }
        record.scaler_attempts = group.scaler_attempts;

        // analysis_arity matches native switch on ((end-begin)>>5).
        if (group.analysis_arity == 1u) {
            if (!fill_case1(group, input_scaler_indices, record, error))
                return false;
        } else if (group.analysis_arity == 2u) {
            if (!fill_case2(group, input_scaler_indices, record, error))
                return false;
        } else if (group.analysis_arity == 3u) {
            if (!fill_case3(group, input_scaler_indices, record, error))
                return false;
        } else {
            error = "prepare_metadata unsupported analysis arity";
            return false;
        }
        out.push_back(std::move(record));
    }
    return true;
}

} // namespace auro3d::encode
