#pragma once

#include "encoder_defaults.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Input geometry passed by GVM:run_ to the native learner.
/// Mix2 produces one scalar residual per PCM sample; mix3 produces one
/// two-component residual vector per PCM sample.
struct NativeGvmInputShape {
    std::uint32_t sample_count = 0;
    std::uint32_t residual_count = 0;
    std::uint32_t dimensions = 0;
};

struct NativeQuantizationCost {
    std::uint32_t residual_bit_width = 0;
    std::uint32_t level_pack_mode = 0;
    std::uint64_t residual_bit_cost = 0;
    std::uint64_t level_bit_cost = 0;
};

/// Group+36/+40/+44/+48 values consumed by GVM:run_. Group:create starts
/// them at one, then Encoder:create_group_ installs the effective limits.
struct NativeGvmSearchPlan {
    std::uint32_t common_limit = 1;
    std::uint32_t dimension1_start = 1;
    std::uint32_t dimension2_start = 1;
    std::uint32_t minimum_clusters = 1;
    std::uint32_t initial_clusters = 1;
};

enum class NativeGvmSearchAction : std::uint8_t {
    retry,
    accept,
    reject,
};

struct NativeGvmSearchStep {
    NativeGvmSearchAction action = NativeGvmSearchAction::reject;
    std::uint32_t clusters = 0;
    bool compact_fit = false;
};

struct NativeGvmLearnerPoint {
    double first = 0.0;
    double second = 0.0;
};

struct NativeGvmLearnerInput {
    NativeGvmInputShape shape{};
    NativeGvmLearnerImplementation implementation =
        NativeGvmLearnerImplementation::modern;
    bool deterministic_seed = false;
    std::uint64_t seed = 0;
    std::vector<NativeGvmLearnerPoint> points;
};

struct NativeGvmCenterTable {
    std::uint32_t dimensions = 0;
    std::vector<std::int32_t> scalars;
    bool forced_zero_center = false;
    std::uint32_t forced_zero_index = 0;
};

struct NativeGvmLearnedResult {
    std::vector<std::uint64_t> sizes;
    std::vector<std::uint64_t> indices;
    NativeGvmCenterTable centers{};
    NativeQuantizationCost cost{};
    std::uint32_t fit_status = 0;
    bool compact_fit = false;
};

struct NativeGvmClusterSummary {
    std::uint64_t population = 0;
    double sum0 = 0.0;
    double sum1 = 0.0;
    double normalized_energy = 0.0;
};

struct NativeGvmLearnerOutput {
    std::vector<std::uint64_t> sizes;
    std::vector<std::uint64_t> indices;
    std::vector<double> center_scalars;
};

struct NativeGvmSearchResult {
    NativeGvmLearnedResult learned{};
    std::uint32_t clusters = 0;
    std::uint32_t learn_attempts = 0;
};

struct ClusterDeltasQuantizationResult {
    std::vector<std::uint64_t> levels;
    std::vector<std::int32_t> residuals;
    std::vector<std::uint64_t> indices;
    std::uint32_t shift = 0;
    std::uint32_t residual_bit_width = 0;
    std::uint32_t level_pack_mode = 0;
    std::uint64_t level_bit_cost = 0;
    std::uint64_t residual_bit_cost = 0;
    /// Payload allowance after Group+184, Group+208, and the native
    /// per-16-sample overhead have been removed.
    std::uint64_t bit_budget = 0;
    std::uint64_t fixed_bit_cost = 0;
    std::uint32_t gvm_fit_status = 0;
    bool gvm_compact_fit = false;
    /// Number of static BitShift candidates tested before acceptance.
    std::uint32_t shift_attempts = 0;
    NativeGvmInputShape gvm_input{};
    std::uint32_t gvm_learner_points = 0;
    bool gvm_deterministic_seed = false;
    bool gvm_learned = false;
    std::uint32_t gvm_selected_clusters = 0;
    std::uint32_t gvm_learn_attempts = 0;
    bool gvm_forced_zero_center = false;
    std::uint32_t gvm_forced_zero_index = 0;
};

/// Ports the input-shape gate at the start of GVM:run_. The
/// residual scalar count must divide the sample count exactly and the native
/// learner accepts only one- or two-dimensional samples.
bool validate_native_gvm_input_shape(
    std::size_t residual_count,
    std::size_t sample_count,
    NativeGvmInputShape& out,
    std::string& error);

/// Shared scalar equivalent of the cost portion of GVM:compute_fit_
/// and Quantization:BitSize:calculate.
bool calculate_native_quantization_cost(
    const std::vector<std::uint64_t>& levels,
    const std::vector<std::int32_t>& residual_scalars,
    std::uint32_t dimensions,
    NativeQuantizationCost& out,
    std::string& error);

bool calculate_native_quantizer_budget(
    std::uint32_t sample_count,
    std::uint32_t bit_line,
    std::uint32_t group_field_208,
    std::uint32_t rescaler_bit_cost,
    std::uint64_t& bit_budget,
    std::uint64_t& fixed_bit_cost,
    std::string& error);

bool calculate_native_gvm_fit_status(
    std::uint32_t sample_count,
    std::uint64_t bit_budget,
    std::uint64_t used_bits,
    std::uint32_t& fit_status,
    bool& compact_fit,
    std::string& error);

bool prepare_native_gvm_search(
    const NativeGvmInputShape& shape,
    std::uint32_t common_limit,
    std::uint32_t dimension1_start,
    std::uint32_t dimension2_start,
    std::uint32_t minimum_clusters,
    NativeGvmSearchPlan& out,
    std::string& error);

/// One control-flow transition from GVM:run_ after compute_fit_. Fit status
/// is the native 1/2/3 value: over budget, fit, or fit with >10% spare bits.
bool advance_native_gvm_search(
    const NativeGvmSearchPlan& plan,
    std::uint32_t fit_status,
    std::uint32_t current_clusters,
    NativeGvmSearchStep& out,
    std::string& error);

/// Ports the factory/set_data preflight before Learner:learn: verifies the
/// mode-to-implementation mapping, converts scalar int32 input to exact
/// doubles, and groups dimension-2 data into pairs.
bool prepare_native_gvm_learner_input(
    const std::vector<std::int32_t>& residual_scalars,
    const NativeGvmInputShape& shape,
    const NativeGvmConfiguration& config,
    NativeGvmLearnerInput& out,
    std::string& error);

/// Ports Learner:get_centers<int,1/2>: truncating double conversion and the
/// optional replacement of the minimum squared-magnitude center by zero.
bool quantize_native_gvm_centers(
    const std::vector<double>& center_scalars,
    std::uint32_t dimensions,
    bool force_zero_center,
    NativeGvmCenterTable& out,
    std::string& error);

/// Completes the confirmed post-learn portion of GVM:compute_fit_: validates
/// sizes/indices against the input population, converts centers, computes
/// BitSize cost, and returns native fit status 1..3.
bool finalize_native_gvm_learned_result(
    const std::vector<std::uint64_t>& sizes,
    const std::vector<std::uint64_t>& indices,
    const std::vector<double>& center_scalars,
    const NativeGvmInputShape& shape,
    const NativeGvmConfiguration& config,
    std::uint64_t bit_budget,
    NativeGvmLearnedResult& out,
    std::string& error);

bool make_native_gvm_singleton(
    const NativeGvmLearnerPoint& point,
    std::uint32_t dimensions,
    NativeGvmClusterSummary& out,
    std::string& error);

bool calculate_native_gvm_merge_cost(
    const NativeGvmClusterSummary& first,
    const NativeGvmClusterSummary& second,
    std::uint32_t dimensions,
    double& cost,
    std::string& error);

bool merge_native_gvm_clusters(
    const NativeGvmClusterSummary& first,
    const NativeGvmClusterSummary& second,
    std::uint32_t dimensions,
    NativeGvmClusterSummary& out,
    std::string& error);

/// Scalar modern-GVM path matching online singleton insertion and the minimum
/// merge-cost choice in Learner<1/2>:learn<double>.
bool learn_native_gvm_modern(
    const NativeGvmLearnerInput& input,
    std::uint32_t target_clusters,
    NativeGvmLearnerOutput& out,
    std::string& error);

bool learn_native_gvm_old_fast(
    const NativeGvmLearnerInput& input,
    std::uint32_t target_clusters,
    NativeGvmLearnerOutput& out,
    std::string& error);

bool run_native_gvm_modern_search(
    const NativeGvmLearnerInput& input,
    const NativeGvmSearchPlan& plan,
    std::uint64_t bit_budget,
    const NativeGvmConfiguration& config,
    NativeGvmSearchResult& out,
    std::string& error);

bool run_native_gvm_old_fast_search(
    const NativeGvmLearnerInput& input,
    const NativeGvmSearchPlan& plan,
    std::uint64_t bit_budget,
    const NativeGvmConfiguration& config,
    NativeGvmSearchResult& out,
    std::string& error);

/// Direct port of VQ BitShift<int,100> process_ for one shift.
/// Returns false when BitSize:calculate rejects the codebook or the bit
/// budget is exceeded (native process_ return).
bool cluster_deltas_quantize_bitshift100(
    const std::vector<std::int32_t>& deltas,
    std::uint32_t shift,
    std::uint64_t bit_budget,
    ClusterDeltasQuantizationResult& out,
    std::string& error);

/// Quantization:run_ mix2 path: try shift 0, then 1..30.
bool cluster_deltas_quantize_mix2(
    const std::vector<std::int32_t>& deltas,
    std::uint32_t sample_count,
    std::uint32_t bit_line,
    std::uint32_t group_field_208,
    std::uint32_t rescaler_bit_cost,
    NativeClusterDeltasBackend backend,
    const NativeGvmConfiguration& gvm,
    const NativeGvmSearchPlan& gvm_search,
    ClusterDeltasQuantizationResult& out,
    std::string& error);

} // namespace auro3d:encode
