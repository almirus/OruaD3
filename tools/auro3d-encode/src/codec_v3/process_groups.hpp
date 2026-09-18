#pragma once

#include "cluster_deltas_quant.hpp"
#include "encode_group.hpp"
#include "mix_mix2.hpp"
#include "native_dither.hpp"
#include "prepare_mix.hpp"
#include "quality_measure.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Per-group analysis result through DetectSilence Rescaler ComputeDeltas
/// mix2 Quantization+Mixer. Individually silent sources are removed before
/// dispatch, so carrier_ready is true for silent groups, arity-1 Mixer
/// copies, and mix2 groups that quantized under the native bit budget.

/// carrier.quantization_shift is Group+24 (bit_line) for prepare_mix_
/// (<< bit_line). The VQ codebook shift is vq_shift, not that field.
struct AnalyzedEncodeGroup {
    EncodedGroupPcm carrier;
    std::vector<std::int32_t> deltas;
    std::vector<std::uint64_t> mix2_indices;
    std::vector<std::int32_t> mix2_residuals;
    Mix2MixerSeeds mix2_seeds{};
    std::vector<std::uint64_t> mix2_levels;
    std::uint32_t mix2_level_pack_mode = 0;
    std::vector<std::uint64_t> mix3_indices;
    /// Packed [low32,high32] native learned mix3 residual pairs.
    std::vector<std::int64_t> mix3_residuals;
    std::vector<std::uint64_t> mix3_levels;
    std::uint32_t mix3_level_pack_mode = 0;
    std::array<std::int32_t, 5> mix3_seeds{};
    /// Channel ids of analysis frames after DetectSilence (Group+336).
    std::vector<std::uint32_t> analysis_source_ids;
    /// (analysis_end - analysis_begin) >> 5 used by prepare_metadata switch.
    std::uint32_t analysis_arity = 0;
    std::uint32_t vq_shift = 0;
    /// Group+67 from Quantization:run_ (mix2).
    std::uint32_t residual_bit_width = 0;
    /// The selected native GVM learner produced this group's codebook.
    bool gvm_learned = false;
    std::uint32_t mix2_shift_attempts = 0;
    /// Confirmed learner input geometry for the selected GVM implementation.
    NativeGvmInputShape gvm_input{};
    std::uint64_t quantizer_bit_budget = 0;
    std::uint64_t quantizer_fixed_bit_cost = 0;
    std::uint64_t quantizer_used_bits = 0;
    /// BitSize:calculate's selected Rice cost for the per-sample indices.
    std::uint64_t golomb_index_bit_cost = 0;
    /// Exact Group+196 value, including the optional Group+392 scaler record.
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
    bool silent = false;
    bool carrier_ready = false;
    std::uint32_t silence_mode = 0;
    /// Group+392 optional scaler index from Rescaler; default absent.
    bool has_scaler_ix = false;
    std::uint8_t scaler_ix = 0;
    std::uint32_t scaler_attempts = 0;
    bool carrier_overflow = false;
    /// ComputeQuality output for this exact reconstructed
    /// candidate. The filter stores its negation against bit_line.
    bool quality_present = false;
    double quality_error_db = -3000.0;
    std::vector<NativeFrameQuality> frame_quality;
};

bool analyze_encode_group(
    const EncodeGroup& group,
    AnalyzedEncodeGroup& out,
    std::string& error);

/// Stateful form used by Encoder:Rescaler. Dither is consumed only by the
/// synthetic zero frame created by DetectSilence for an all-silent group.
bool analyze_encode_group(
    const EncodeGroup& group,
    bool dither_enabled,
    NativeDitherState& dither,
    AnalyzedEncodeGroup& out,
    std::string& error);

bool analyze_encode_groups(
    const std::vector<EncodeGroup>& groups,
    std::vector<AnalyzedEncodeGroup>& out,
    std::string& error);

/// Builds EncodedGroupPcm list for prepare_mix_. Fails unless every group has
/// carrier_ready set.
bool analyzed_groups_to_encoded_pcm(
    const std::vector<AnalyzedEncodeGroup>& analyzed,
    std::vector<EncodedGroupPcm>& out,
    std::string& error);

} // namespace auro3d:encode
