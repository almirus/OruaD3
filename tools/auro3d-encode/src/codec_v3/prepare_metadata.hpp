#pragma once

#include "process_groups.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Fixed prefix written by Encoder:prepare_metadata_unit_block_
/// before optional loudness downmix group records. Native construction
/// shuffles Config+4/+8 into metadata+0/+4 and copies Config+32 to +8.
struct MetadataUnitHeader {
    /// Native +0/+4 are the shuffled Config words: unit block size followed
    /// by original layout. The carrier layout is derived, not stored here.
    std::uint32_t unit_block_size = 0;
    std::uint32_t original_layout = 0;
    /// Config field_28 value copied at native metadata +8.
    std::uint32_t field_28 = 0;
    /// Native constant stored at metadata+12: 2561 (0x0A01).
    std::uint16_t fixed_word = 2561u;
};

/// Optional original-channel map looked up in Encoder+6368. Empty tree leaves
/// these flags clear (default encode path).
struct MetadataSourceRef {
    std::uint32_t channel_id = 0;
    bool has_original_map = false;
    std::uint8_t original_map = 0;
};

/// One group record at UnitBlock+552, stride 128 (`index<<7`), filled by
/// prepare_metadata_unit_block_.
struct MetadataGroupRecord {
    /// Native record offsets: +0 carrier id, +4 headroom, +8 arity.
    std::uint32_t carrier_channel_id = 0;
    /// `24 - Group+24` (bit_line); must be <= 0x17.
    std::uint8_t headroom_bits = 0;
    /// Analysis frame count (`(end-begin)>>5`); cases 1 and 2 only.
    std::uint32_t analysis_arity = 0;

    /// Source slots are native +16/+20/+24, +28/+32/+36, +40/+44/+48.
    MetadataSourceRef source0{};
    MetadataSourceRef source1{};
    MetadataSourceRef source2{};

    /// Mix2: first 8 bytes of Group+616 seed vector (two int32).
    /// Seed storage begins at native +40 for mix2 and +52 for mix3.
    std::int32_t seed0 = 0;
    std::int32_t seed1 = 0;
    std::int32_t seed2 = 0;
    std::int32_t seed3 = 0;
    std::int32_t seed4 = 0;
    /// Group+70 from Quantization:run_ (VQ shift).
    std::uint8_t vq_shift = 0;
    /// Group+67 from Quantization:run_ (residual bit width).
    std::uint8_t residual_bit_width = 0;
    /// Residuals trimmed to `max(index)+1` (native residual table copy).
    std::vector<std::int32_t> residuals;
    /// Low dwords of Group+448 per-sample indices.
    std::vector<std::uint32_t> indices;
    /// Quantization level schedule retained for channel-payload serialization.
    std::vector<std::uint64_t> levels;
    std::uint32_t level_pack_mode = 0;

    /// Mix3 stores eight-byte residual entries and the same low-dword index
    /// vector shape as the native record at offsets +80/+112.
    std::vector<std::int64_t> mix3_residuals;
    std::vector<std::uint32_t> mix3_indices;
    std::vector<std::uint64_t> mix3_levels;
    std::uint32_t mix3_level_pack_mode = 0;

    /// Diagnostic-only backend provenance retained outside the serialized
    /// native group record.
    bool gvm_learned = false;
    std::uint32_t mix2_shift_attempts = 0;
    NativeGvmInputShape gvm_input{};
    std::uint64_t quantizer_bit_budget = 0;
    std::uint64_t quantizer_fixed_bit_cost = 0;
    std::uint64_t quantizer_used_bits = 0;
    std::uint64_t golomb_index_bit_cost = 0;
    std::uint32_t rescaler_bit_cost = 0;
    NativeGvmSearchPlan gvm_search{};
    std::uint32_t gvm_fit_status = 0;
    bool gvm_compact_fit = false;
    NativeClusterDeltasBackend cluster_backend =
        NativeClusterDeltasBackend::gvm;
    NativeGvmLearnerImplementation gvm_learner =
        NativeGvmLearnerImplementation::modern;
    std::uint32_t gvm_learner_points = 0;
    bool gvm_deterministic_seed = false;
    std::uint32_t gvm_selected_clusters = 0;
    std::uint32_t gvm_learn_attempts = 0;
    bool gvm_forced_zero_center = false;
    std::uint32_t gvm_forced_zero_index = 0;
    bool quality_present = false;
    double quality_error_db = -3000.0;
    std::vector<NativeFrameQuality> frame_quality;

    /// Group+392 scaler index; default create leaves 0 absent.
    bool has_scaler_ix = false;
    std::uint8_t scaler_ix = 0;
    std::uint32_t scaler_attempts = 0;
};

/// Fills the confirmed header fields. Optional loudness secondary-downmix
/// tables remain separate ports.
bool prepare_metadata_unit_header(
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    std::uint32_t sample_rate,
    std::uint32_t unit_block_size,
    std::uint32_t field_28,
    MetadataUnitHeader& out,
    std::string& error);

/// Builds per-group records for silent/arity-1 (case 1), mix2 (case 2), and
/// a fully populated mix3 record (case 3) from AnalyzedEncodeGroup.
/// When `input_scaler_indices` is non-null it mirrors the Encoder+6368 scaler
/// tree written by downmix_/cts_dmx_coeff_limit_: a non-zero index becomes
/// MetadataSourceRef.original_map for that source channel.
bool prepare_metadata_group_records(
    const std::vector<AnalyzedEncodeGroup>& analyzed,
    std::vector<MetadataGroupRecord>& out,
    std::string& error);

bool prepare_metadata_group_records(
    const std::vector<AnalyzedEncodeGroup>& analyzed,
    const std::array<std::uint8_t, 31>* input_scaler_indices,
    std::vector<MetadataGroupRecord>& out,
    std::string& error);

} // namespace auro3d:encode
