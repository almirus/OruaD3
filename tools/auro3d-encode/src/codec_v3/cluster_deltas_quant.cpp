#include "cluster_deltas_quant.hpp"

#include "channel_metadata.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <queue>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace auro3d::encode {
namespace {

constexpr std::uint32_t kBinCount = 201u;

std::int32_t truncate_double_to_i32_native(double value) {
    // cvttsd2si/cvttpd2dq return the integer-indefinite value for NaN or
    // values outside the signed dword range.
    if (!std::isfinite(value)
        || value >= 2147483648.0
        || value < -2147483648.0) {
        return std::numeric_limits<std::int32_t>::min();
    }
    return static_cast<std::int32_t>(value);
}

std::uint32_t wrapped_squared_norm(
    const std::int32_t* values,
    std::uint32_t dimensions) {
    std::uint32_t result = 0u;
    for (std::uint32_t dimension = 0u; dimension < dimensions; ++dimension) {
        const std::uint32_t raw = static_cast<std::uint32_t>(values[dimension]);
        result += raw * raw;
    }
    return result;
}

std::uint32_t error_center_index(std::uint64_t count) {
    // auro:codec:v3:a3d:details:error_center_index
    if (count >= 633u)
        return static_cast<std::uint32_t>(((count - 633u) >> 4u) + 85u);
    if (count > 600u)
        return 84u;
    if (count >= 65u)
        return static_cast<std::uint32_t>(((count - 65u) >> 3u) + 17u);
    if (count >= 17u)
        return static_cast<std::uint32_t>(((count - 17u) >> 2u) + 5u);
    if (count >= 9u)
        return static_cast<std::uint32_t>(((count - 9u) >> 1u) + 1u);
    return 0u;
}

std::uint32_t nr_error_centers_per_index(std::uint32_t index) {
    // auro:codec:v3:a3d:details:nr_error_centers_per_index
    if (index <= 4u)
        return 2u * index + 8u;
    if (index <= 0xFu)
        return 4u * index;
    if (index > 0x53u)
        return 16u * index - 712u;
    return 8u * index - 64u;
}

std::uint32_t residual_bit_width_from_max_abs(std::uint32_t max_abs) {
    if (max_abs == 0u)
        return 2u;
    unsigned long bsr = 0;
#if defined(_MSC_VER)
    _BitScanReverse(&bsr, max_abs);
#else
    bsr = 31u - static_cast<unsigned>(__builtin_clz(max_abs));
#endif
    // Native BitSize:calculate: 33 - (bsr ^ 0x1F).
    return 33u - (static_cast<std::uint32_t>(bsr) ^ 0x1Fu);
}

std::uint64_t bitshift100_bin(std::int32_t sample, std::uint32_t shift) {
    // BitShift<int,100u>:process_ stores both scale and bias in
    // qword fields (`100LL << shift`) and performs the sample addition after
    // the signed int has undergone the native unsigned-64 conversion. Keep
    // that modulo-2^64 behaviour; using a wrapped dword changes bins once
    // the trial shift is large enough for 100<<shift to cross bit 31.
    const std::uint64_t scale = std::uint64_t{100u} << shift;
    const std::uint64_t bias = scale
        + ((std::uint64_t{1} << shift) >> 1u);
    const std::uint64_t unsigned_sample = static_cast<std::uint64_t>(
        static_cast<std::int64_t>(sample));
    return (bias + unsigned_sample) >> shift;
}

bool bit_size_calculate(
    const std::vector<std::uint64_t>& levels,
    const std::vector<std::int32_t>& residuals,
    std::uint32_t dimensions,
    std::uint32_t& residual_bit_width,
    std::uint64_t& residual_bit_cost,
    std::uint32_t& level_pack_mode,
    std::uint64_t& level_bit_cost) {
    // BitSize:calculate
    if (dimensions == 0u || residuals.size() % dimensions != 0u)
        return false;
    const std::uint64_t entry_count = residuals.size() / dimensions;
    const std::uint32_t center_index = error_center_index(entry_count);
    const std::uint64_t centers = nr_error_centers_per_index(center_index);

    std::uint32_t max_abs = 0u;
    for (std::int32_t value : residuals) {
        const std::uint32_t magnitude = value < 0
            ? static_cast<std::uint32_t>(-static_cast<std::int64_t>(value))
            : static_cast<std::uint32_t>(value);
        if (magnitude > max_abs)
            max_abs = magnitude;
    }
    residual_bit_width = residual_bit_width_from_max_abs(max_abs);
    if (centers != 0u
        && dimensions > std::numeric_limits<std::uint64_t>::max() / centers) {
        return false;
    }
    const std::uint64_t scalar_centers = centers * dimensions;
    if (scalar_centers != 0u
        && residual_bit_width
            > std::numeric_limits<std::uint64_t>::max() / scalar_centers) {
        return false;
    }
    residual_bit_cost =
        static_cast<std::uint64_t>(residual_bit_width) * scalar_centers;

    if (levels.empty()) {
        level_pack_mode = 1u;
        level_bit_cost = 0u;
        return true;
    }

    // Levels are vector<unsigned long>; cost walks them as dword pairs and
    // multiplies the low dword only (native v27 += 2).
    auto cost_with_period = [&](std::uint32_t start_width, std::uint32_t period) -> std::uint64_t {
        std::uint64_t cost = 0u;
        std::uint32_t w = start_width;
        std::uint32_t counter = 0u;
        for (std::uint64_t level : levels) {
            const std::uint32_t low = static_cast<std::uint32_t>(level);
            if (low != 0u && static_cast<std::uint64_t>(w)
                > std::numeric_limits<std::uint64_t>::max() / low) {
                return std::numeric_limits<std::uint64_t>::max();
            }
            const std::uint64_t add = static_cast<std::uint64_t>(w) * low;
            if (cost > std::numeric_limits<std::uint64_t>::max() - add)
                return std::numeric_limits<std::uint64_t>::max();
            cost += add;
            ++counter;
            if (counter == period) {
                counter = 0u;
                if (w == std::numeric_limits<std::uint32_t>::max())
                    return std::numeric_limits<std::uint64_t>::max();
                ++w;
            }
        }
        return cost;
    };

    std::uint64_t best = cost_with_period(2u, 2u);
    std::uint32_t best_mode = 1u;
    // Native: v30 = 2 * (v25 != -1) - 1; reject only on all-ones overflow.
    if (best == static_cast<std::uint64_t>(-1))
        best_mode = static_cast<std::uint32_t>(-1);

    const std::uint64_t c3 = cost_with_period(3u, 4u);
    if (c3 < best) {
        best = c3;
        best_mode = 2u;
    }
    const std::uint64_t c4 = cost_with_period(4u, 8u);
    if (c4 < best) {
        best = c4;
        best_mode = 3u;
    }
    const std::uint64_t c5 = cost_with_period(5u, 16u);
    if (c5 < best) {
        best = c5;
        best_mode = 4u;
    }
    const std::uint64_t c6 = cost_with_period(6u, 32u);
    if (c6 < best) {
        best = c6;
        best_mode = 5u;
    }
    const std::uint64_t c7 = cost_with_period(7u, 64u);
    if (c7 < best) {
        best = c7;
        best_mode = 6u;
    }
    const std::uint64_t c8 = cost_with_period(8u, 128u);
    if (c8 < best) {
        best = c8;
        best_mode = 7u;
    }
    level_pack_mode = best_mode;
    level_bit_cost = best;
    return best_mode != static_cast<std::uint32_t>(-1) && best != static_cast<std::uint64_t>(-1);
}

} // namespace

bool validate_native_gvm_input_shape(
    std::size_t residual_count,
    std::size_t sample_count,
    NativeGvmInputShape& out,
    std::string& error) {
    error.clear();
    out = {};
    if (sample_count == 0u) {
        error = "native GVM input has no samples";
        return false;
    }
    if (sample_count > std::numeric_limits<std::uint32_t>::max()
        || residual_count > std::numeric_limits<std::uint32_t>::max()) {
        error = "native GVM input counters exceed 32-bit storage";
        return false;
    }
    const std::size_t dimensions = residual_count / sample_count;
    if (residual_count % sample_count != 0u) {
        error = "native GVM residual count is not divisible by sample count";
        return false;
    }
    // GVM:run_ tests ((dimensions + 1) & ~1) == 2. For non-negative
    // vector sizes this admits exactly dimensions 1 and 2.
    if (dimensions != 1u && dimensions != 2u) {
        error = "native GVM accepts only one- or two-dimensional residuals";
        return false;
    }
    out.sample_count = static_cast<std::uint32_t>(sample_count);
    out.residual_count = static_cast<std::uint32_t>(residual_count);
    out.dimensions = static_cast<std::uint32_t>(dimensions);
    return true;
}

bool calculate_native_quantization_cost(
    const std::vector<std::uint64_t>& levels,
    const std::vector<std::int32_t>& residual_scalars,
    std::uint32_t dimensions,
    NativeQuantizationCost& out,
    std::string& error) {
    error.clear();
    out = {};
    if (dimensions != 1u && dimensions != 2u) {
        error = "native quantization cost requires dimension one or two";
        return false;
    }
    if (residual_scalars.empty()
        || residual_scalars.size() % dimensions != 0u) {
        error = "native quantization residual table has invalid geometry";
        return false;
    }
    const std::size_t entry_count = residual_scalars.size() / dimensions;
    if (levels.size() < entry_count) {
        error = "native quantization level table is shorter than its codebook";
        return false;
    }
    if (!bit_size_calculate(
            levels,
            residual_scalars,
            dimensions,
            out.residual_bit_width,
            out.residual_bit_cost,
            out.level_pack_mode,
            out.level_bit_cost)) {
        error = "native quantization bit-size calculation failed";
        out = {};
        return false;
    }
    return true;
}

bool calculate_native_quantizer_budget(
    std::uint32_t sample_count,
    std::uint32_t bit_line,
    std::uint32_t group_field_208,
    std::uint32_t rescaler_bit_cost,
    std::uint64_t& bit_budget,
    std::uint64_t& fixed_bit_cost,
    std::string& error) {
    error.clear();
    bit_budget = 0u;
    fixed_bit_cost = 0u;
    if (sample_count == 0u) {
        error = "native quantizer budget requires at least one sample";
        return false;
    }
    if (bit_line < 3u || bit_line > 12u) {
        error = "native quantizer bit line is outside Config::validate range";
        return false;
    }
    constexpr std::uint64_t kGroup184Sum = 0x20u + 0x80u;
    const std::uint64_t n = sample_count;
    if (n > std::numeric_limits<std::uint64_t>::max() / bit_line) {
        error = "native quantizer gross bit budget overflows";
        return false;
    }
    const std::uint64_t gross_budget = n * bit_line;
    const std::uint64_t block_overhead = (n - 1u) >> 4u;
    fixed_bit_cost =
        static_cast<std::uint64_t>(group_field_208)
        + kGroup184Sum
        + rescaler_bit_cost;
    if (block_overhead > gross_budget
        || fixed_bit_cost > gross_budget - block_overhead) {
        error = "native quantizer budget is exhausted by fixed overhead";
        return false;
    }
    bit_budget = gross_budget - block_overhead - fixed_bit_cost;
    return true;
}

bool calculate_native_gvm_fit_status(
    std::uint32_t sample_count,
    std::uint64_t bit_budget,
    std::uint64_t used_bits,
    std::uint32_t& fit_status,
    bool& compact_fit,
    std::string& error) {
    error.clear();
    fit_status = 0u;
    compact_fit = false;
    if (sample_count == 0u) {
        error = "native GVM fit status requires at least one sample";
        return false;
    }
    if (used_bits > bit_budget) {
        fit_status = 1u;
        return true;
    }
    const std::uint64_t remaining = bit_budget - used_bits;
    // compute_fit_ uses: (double(sample_count) * 0.1 < remaining) + 2.
    // Both operands originate from 32-bit sample counts and the <=12-bit
    // budget, so the exact integer comparison cannot overflow uint64.
    fit_status = remaining * 10u > sample_count ? 3u : 2u;
    compact_fit = (fit_status & ~1u) == 2u;
    return true;
}

bool prepare_native_gvm_search(
    const NativeGvmInputShape& shape,
    std::uint32_t common_limit,
    std::uint32_t dimension1_start,
    std::uint32_t dimension2_start,
    std::uint32_t minimum_clusters,
    NativeGvmSearchPlan& out,
    std::string& error) {
    error.clear();
    out = {};
    if (shape.dimensions != 1u && shape.dimensions != 2u) {
        error = "native GVM search requires dimension one or two";
        return false;
    }
    if (common_limit == 0u || dimension1_start == 0u
        || dimension2_start == 0u || minimum_clusters == 0u
        || minimum_clusters > common_limit) {
        error = "native GVM cluster bounds are invalid";
        return false;
    }
    out.common_limit = common_limit;
    out.dimension1_start = dimension1_start;
    out.dimension2_start = dimension2_start;
    out.minimum_clusters = minimum_clusters;
    const std::uint32_t dimension_start = shape.dimensions == 2u
        ? dimension2_start
        : dimension1_start;
    out.initial_clusters = std::min(common_limit, dimension_start);
    return true;
}

bool advance_native_gvm_search(
    const NativeGvmSearchPlan& plan,
    std::uint32_t fit_status,
    std::uint32_t current_clusters,
    NativeGvmSearchStep& out,
    std::string& error) {
    error.clear();
    out = {};
    if (fit_status < 1u || fit_status > 3u
        || current_clusters == 0u
        || current_clusters > plan.common_limit
        || plan.minimum_clusters == 0u
        || plan.minimum_clusters > plan.common_limit) {
        error = "native GVM search transition has invalid state";
        return false;
    }
    if (fit_status == 1u && current_clusters > plan.minimum_clusters) {
        out.action = NativeGvmSearchAction::retry;
        out.clusters = current_clusters - 1u;
        return true;
    }
    if (fit_status <= 2u || current_clusters == plan.common_limit) {
        out.action = NativeGvmSearchAction::accept;
        out.clusters = current_clusters;
        // Group+520 = (fit_status & 0xfffffffe) == 2.
        out.compact_fit = (fit_status & ~1u) == 2u;
        return true;
    }
    // Native increments the GVM retry counter, adds 30, then clamps to
    // Group+36 at the top of the next loop.
    out.action = NativeGvmSearchAction::retry;
    const std::uint64_t expanded =
        static_cast<std::uint64_t>(current_clusters) + 30u;
    out.clusters = static_cast<std::uint32_t>(
        std::min<std::uint64_t>(expanded, plan.common_limit));
    return true;
}

bool prepare_native_gvm_learner_input(
    const std::vector<std::int32_t>& residual_scalars,
    const NativeGvmInputShape& shape,
    const NativeGvmConfiguration& config,
    NativeGvmLearnerInput& out,
    std::string& error) {
    error.clear();
    out = {};
    if (shape.sample_count == 0u
        || (shape.dimensions != 1u && shape.dimensions != 2u)
        || shape.residual_count != residual_scalars.size()
        || static_cast<std::uint64_t>(shape.sample_count) * shape.dimensions
            != shape.residual_count) {
        error = "native GVM learner input geometry is inconsistent";
        return false;
    }
    const NativeGvmLearnerImplementation expected =
        config.mode == 2u
        ? NativeGvmLearnerImplementation::old_fast
        : (config.mode == 1u
            ? NativeGvmLearnerImplementation::unavailable_mode1
            : NativeGvmLearnerImplementation::modern);
    if (config.mode > 2u || config.learner != expected) {
        error = "native GVM learner mode and implementation disagree";
        return false;
    }
    if (expected == NativeGvmLearnerImplementation::unavailable_mode1) {
        error = "native GVM factory has no learner implementation for mode 1";
        return false;
    }
    out.shape = shape;
    out.implementation = expected;
    out.deterministic_seed = config.has_qword_48;
    out.seed = config.qword_48;
    out.points.resize(shape.sample_count);
    if (shape.dimensions == 1u) {
        for (std::size_t index = 0; index < residual_scalars.size(); ++index)
            out.points[index].first = static_cast<double>(residual_scalars[index]);
    } else {
        for (std::size_t index = 0; index < out.points.size(); ++index) {
            out.points[index].first =
                static_cast<double>(residual_scalars[2u * index]);
            out.points[index].second =
                static_cast<double>(residual_scalars[2u * index + 1u]);
        }
    }
    return true;
}

bool quantize_native_gvm_centers(
    const std::vector<double>& center_scalars,
    std::uint32_t dimensions,
    bool force_zero_center,
    NativeGvmCenterTable& out,
    std::string& error) {
    error.clear();
    out = {};
    if ((dimensions != 1u && dimensions != 2u)
        || center_scalars.empty()
        || center_scalars.size() % dimensions != 0u) {
        error = "native GVM center table has invalid geometry";
        return false;
    }
    const std::size_t center_count = center_scalars.size() / dimensions;
    if (center_count > std::numeric_limits<std::uint32_t>::max()) {
        error = "native GVM center count exceeds 32-bit storage";
        return false;
    }
    out.dimensions = dimensions;
    out.scalars.reserve(center_scalars.size());
    for (const double value : center_scalars)
        out.scalars.push_back(truncate_double_to_i32_native(value));

    if (!force_zero_center)
        return true;
    std::size_t nearest = 0u;
    std::uint32_t nearest_norm =
        wrapped_squared_norm(out.scalars.data(), dimensions);
    for (std::size_t center = 1u; center < center_count; ++center) {
        const std::uint32_t norm = wrapped_squared_norm(
            out.scalars.data() + center * dimensions, dimensions);
        if (norm < nearest_norm) {
            nearest = center;
            nearest_norm = norm;
        }
    }
    // get_centers<int,2> clears only the first component, matching the
    // native qword table write through an int pointer.
    out.scalars[nearest * dimensions] = 0;
    out.forced_zero_center = true;
    out.forced_zero_index = static_cast<std::uint32_t>(nearest);
    return true;
}

bool finalize_native_gvm_learned_result(
    const std::vector<std::uint64_t>& sizes,
    const std::vector<std::uint64_t>& indices,
    const std::vector<double>& center_scalars,
    const NativeGvmInputShape& shape,
    const NativeGvmConfiguration& config,
    std::uint64_t bit_budget,
    NativeGvmLearnedResult& out,
    std::string& error) {
    error.clear();
    out = {};
    if (shape.sample_count == 0u
        || (shape.dimensions != 1u && shape.dimensions != 2u)
        || sizes.empty()
        || sizes.size() > kCodecV3ChannelCodebookMaxEntries
        || indices.size() != shape.sample_count
        || sizes.size() > std::numeric_limits<std::size_t>::max()
            / shape.dimensions
        || center_scalars.size() != sizes.size() * shape.dimensions) {
        error = "native GVM learned result has invalid geometry";
        return false;
    }
    std::vector<std::uint64_t> observed(sizes.size(), 0u);
    for (const std::uint64_t index : indices) {
        if (index >= observed.size()) {
            error = "native GVM learned index exceeds its center table";
            return false;
        }
        ++observed[static_cast<std::size_t>(index)];
    }
    std::uint64_t population = 0u;
    for (std::size_t index = 0; index < sizes.size(); ++index) {
        if (sizes[index] == 0u || sizes[index] != observed[index]) {
            error = "native GVM learned sizes disagree with index populations";
            return false;
        }
        if (population > std::numeric_limits<std::uint64_t>::max() - sizes[index]) {
            error = "native GVM learned population overflows";
            return false;
        }
        population += sizes[index];
    }
    if (population != shape.sample_count) {
        error = "native GVM learned population disagrees with sample count";
        return false;
    }
    NativeGvmCenterTable centers{};
    // Factory config +16 is copied to Learner+24 by Learner:initialize
    // Its first byte is GVM+40; get_centers<int,1/2>
    // / uses exactly Learner+24 to replace the nearest
    // center. GVM+48/+56 is unrelated and only seeds the learner RNG.
    const bool force_zero_center = config.flag_40;
    if (!quantize_native_gvm_centers(
            center_scalars,
            shape.dimensions,
            force_zero_center,
            centers,
            error)) {
        return false;
    }
    NativeQuantizationCost cost{};
    if (!calculate_native_quantization_cost(
            sizes,
            centers.scalars,
            shape.dimensions,
            cost,
            error)) {
        return false;
    }
    std::uint32_t fit_status = 0u;
    bool compact_fit = false;
    const std::uint64_t used_bits =
        cost.residual_bit_cost + cost.level_bit_cost;
    if (used_bits < cost.residual_bit_cost
        || !calculate_native_gvm_fit_status(
            shape.sample_count,
            bit_budget,
            used_bits,
            fit_status,
            compact_fit,
            error)) {
        if (error.empty())
            error = "native GVM learned bit cost overflows";
        return false;
    }
    out.sizes = sizes;
    out.indices = indices;
    out.centers = std::move(centers);
    out.cost = cost;
    out.fit_status = fit_status;
    out.compact_fit = compact_fit;
    return true;
}

bool make_native_gvm_singleton(
    const NativeGvmLearnerPoint& point,
    std::uint32_t dimensions,
    NativeGvmClusterSummary& out,
    std::string& error) {
    error.clear();
    out = {};
    if (dimensions != 1u && dimensions != 2u) {
        error = "native GVM singleton requires dimension one or two";
        return false;
    }
    out.population = 1u;
    out.sum0 = point.first;
    out.sum1 = dimensions == 2u ? point.second : 0.0;
    out.normalized_energy = dimensions == 2u
        ? (out.sum0 * out.sum0 + out.sum1 * out.sum1) * 1.0
        : (out.sum0 * out.sum0 + 0.0) * 1.0;
    return true;
}

bool merge_native_gvm_clusters(
    const NativeGvmClusterSummary& first,
    const NativeGvmClusterSummary& second,
    std::uint32_t dimensions,
    NativeGvmClusterSummary& out,
    std::string& error) {
    error.clear();
    out = {};
    if ((dimensions != 1u && dimensions != 2u)
        || first.population == 0u || second.population == 0u
        || first.population
            > std::numeric_limits<std::uint64_t>::max() - second.population) {
        error = "native GVM cluster merge has invalid population or dimension";
        return false;
    }
    out.population = first.population + second.population;
    out.sum0 = first.sum0 + second.sum0;
    out.sum1 = dimensions == 2u ? first.sum1 + second.sum1 : 0.0;
    const double reciprocal = 1.0 / static_cast<double>(out.population);
    out.normalized_energy = dimensions == 2u
        ? (out.sum0 * out.sum0 + out.sum1 * out.sum1) * reciprocal
        : (out.sum0 * out.sum0 + 0.0) * reciprocal;
    return true;
}

bool calculate_native_gvm_merge_cost(
    const NativeGvmClusterSummary& first,
    const NativeGvmClusterSummary& second,
    std::uint32_t dimensions,
    double& cost,
    std::string& error) {
    cost = 0.0;
    NativeGvmClusterSummary merged{};
    if (!merge_native_gvm_clusters(
            first, second, dimensions, merged, error)) {
        return false;
    }
    cost = first.normalized_energy
        + second.normalized_energy
        - merged.normalized_energy;
    return true;
}

static bool learn_native_gvm_online(
    const NativeGvmLearnerInput& input,
    std::uint32_t target_clusters,
    NativeGvmLearnerImplementation implementation,
    NativeGvmLearnerOutput& out,
    std::string& error) {
    error.clear();
    out = {};
    if (input.implementation != implementation
        || (implementation != NativeGvmLearnerImplementation::modern
            && implementation
                != NativeGvmLearnerImplementation::old_fast)
        || input.points.empty()
        || input.points.size() != input.shape.sample_count
        || (input.shape.dimensions != 1u && input.shape.dimensions != 2u)
        || target_clusters == 0u) {
        error = "native online GVM learner has invalid input or target";
        return false;
    }
    target_clusters = std::min<std::uint32_t>(
        target_clusters, input.shape.sample_count);

    struct LiveCluster {
        NativeGvmClusterSummary summary{};
        bool active = true;
    };
    struct MergeCandidate {
        double cost = 0.0;
        std::size_t first = 0u;
        std::size_t second = 0u;
        std::uint64_t first_version = 0u;
        std::uint64_t second_version = 0u;
    };
    struct MergeCandidateGreater {
        bool operator()(
            const MergeCandidate& first,
            const MergeCandidate& second) const {
            if (first.cost != second.cost)
                return first.cost > second.cost;
            if (first.first != second.first)
                return first.first > second.first;
            return first.second > second.second;
        }
    };
    constexpr std::size_t no_cluster =
        std::numeric_limits<std::size_t>::max();
    std::vector<LiveCluster> clusters;
    clusters.reserve(input.points.size());
    std::vector<std::size_t> assignments;
    assignments.reserve(input.points.size());
    std::vector<std::size_t> redirects;
    redirects.reserve(input.points.size());
    std::vector<std::size_t> active_order;
    active_order.reserve(target_clusters + 1u);
    std::vector<std::uint64_t> versions;
    versions.reserve(input.points.size());
    std::priority_queue<
        MergeCandidate,
        std::vector<MergeCandidate>,
        MergeCandidateGreater> merge_candidates;
    std::size_t active_clusters = 0u;

    const auto push_merge_candidate =
        [&](std::size_t first, std::size_t second) {
            if (second < first)
                std::swap(first, second);
            double cost = 0.0;
            if (!calculate_native_gvm_merge_cost(
                    clusters[first].summary,
                    clusters[second].summary,
                    input.shape.dimensions,
                    cost,
                    error)) {
                return false;
            }
            merge_candidates.push({
                cost,
                first,
                second,
                versions[first],
                versions[second]});
            return true;
        };

    for (const NativeGvmLearnerPoint& point : input.points) {
        NativeGvmClusterSummary singleton{};
        if (!make_native_gvm_singleton(
                point, input.shape.dimensions, singleton, error)) {
            return false;
        }
        const std::size_t singleton_index = clusters.size();
        clusters.push_back({singleton, true});
        assignments.push_back(singleton_index);
        redirects.push_back(singleton_index);
        versions.push_back(0u);
        for (const std::size_t index : active_order) {
            if (!push_merge_candidate(index, singleton_index))
                return false;
        }
        active_order.push_back(singleton_index);
        ++active_clusters;
        if (active_clusters <= target_clusters)
            continue;

        std::size_t best_first = no_cluster;
        std::size_t best_second = no_cluster;
        while (!merge_candidates.empty()) {
            const MergeCandidate candidate = merge_candidates.top();
            if (!clusters[candidate.first].active
                || !clusters[candidate.second].active
                || versions[candidate.first] != candidate.first_version
                || versions[candidate.second] != candidate.second_version) {
                merge_candidates.pop();
                continue;
            }
            best_first = candidate.first;
            best_second = candidate.second;
            merge_candidates.pop();
            break;
        }
        if (best_first == no_cluster || best_second == no_cluster) {
            error = "native online GVM learner has no merge candidate";
            return false;
        }
        NativeGvmClusterSummary merged{};
        if (!merge_native_gvm_clusters(
                clusters[best_first].summary,
                clusters[best_second].summary,
                input.shape.dimensions,
                merged,
                error)) {
            return false;
        }
        clusters[best_first].summary = merged;
        clusters[best_second].active = false;
        redirects[best_second] = best_first;
        ++versions[best_first];
        ++versions[best_second];
        active_order.erase(
            std::find(
                active_order.begin(), active_order.end(), best_second));
        --active_clusters;
        for (const std::size_t other : active_order) {
            if (other != best_first
                && !push_merge_candidate(best_first, other)) {
                return false;
            }
        }
    }

    // Learner:order_ sorts the live cluster-index vector by the first qword
    // of each 24/32-byte cluster record, which is population. The comparator
    // is descending (`population[first] < population[second]` triggers a
    // swap). get_sizes, get_centers, and cluster_membership all consume this
    // same order, so remap the three outputs together before finalization.
    std::vector<std::size_t> order = active_order;
    std::stable_sort(
        order.begin(),
        order.end(),
        [&](std::size_t first, std::size_t second) {
            return clusters[first].summary.population
                > clusters[second].summary.population;
        });
    std::vector<std::uint64_t> inverse(
        clusters.size(), std::numeric_limits<std::uint64_t>::max());
    std::vector<NativeGvmClusterSummary> ordered_clusters;
    ordered_clusters.reserve(order.size());
    for (std::size_t new_index = 0u; new_index < order.size(); ++new_index) {
        inverse[order[new_index]] = new_index;
        ordered_clusters.push_back(clusters[order[new_index]].summary);
    }
    for (std::size_t assignment : assignments) {
        if (assignment >= redirects.size()) {
            error = "native online GVM learner produced an invalid assignment";
            out = {};
            return false;
        }
        while (redirects[assignment] != assignment)
            assignment = redirects[assignment];
        if (assignment >= inverse.size()
            || inverse[assignment]
                == std::numeric_limits<std::uint64_t>::max()) {
            error = "native online GVM learner produced an invalid assignment";
            out = {};
            return false;
        }
        out.indices.push_back(inverse[assignment]);
    }

    out.sizes.reserve(ordered_clusters.size());
    for (const NativeGvmClusterSummary& cluster : ordered_clusters)
        out.sizes.push_back(cluster.population);
    out.center_scalars.reserve(
        ordered_clusters.size() * input.shape.dimensions);
    for (const NativeGvmClusterSummary& cluster : ordered_clusters) {
        if (cluster.population == 0u) {
            error = "native online GVM learner produced an empty cluster";
            out = {};
            return false;
        }
        const double reciprocal =
            1.0 / static_cast<double>(cluster.population);
        out.center_scalars.push_back(cluster.sum0 * reciprocal);
        if (input.shape.dimensions == 2u)
            out.center_scalars.push_back(cluster.sum1 * reciprocal);
    }
    return true;
}

bool learn_native_gvm_modern(
    const NativeGvmLearnerInput& input,
    std::uint32_t target_clusters,
    NativeGvmLearnerOutput& out,
    std::string& error) {
    return learn_native_gvm_online(
        input,
        target_clusters,
        NativeGvmLearnerImplementation::modern,
        out,
        error);
}

bool learn_native_gvm_old_fast(
    const NativeGvmLearnerInput& input,
    std::uint32_t target_clusters,
    NativeGvmLearnerOutput& out,
    std::string& error) {
    // fast:Clustering:process_ keeps at most the
    // requested cluster count. Once full, it either inserts the new sample
    // into the cheapest existing cluster or merges the cheapest existing
    // pair and reuses the freed record for the new singleton. Considering
    // the singleton as one extra cluster and selecting the cheapest merge is
    // the same decision, including the zero-cost duplicate path.
    return learn_native_gvm_online(
        input,
        target_clusters,
        NativeGvmLearnerImplementation::old_fast,
        out,
        error);
}

static bool run_native_gvm_search(
    const NativeGvmLearnerInput& input,
    const NativeGvmSearchPlan& plan,
    std::uint64_t bit_budget,
    const NativeGvmConfiguration& config,
    NativeGvmLearnerImplementation implementation,
    NativeGvmSearchResult& out,
    std::string& error) {
    error.clear();
    out = {};
    const std::uint32_t expected_mode =
        implementation == NativeGvmLearnerImplementation::old_fast ? 2u : 0u;
    if (input.implementation != implementation
        || config.learner != implementation
        || config.mode != expected_mode
        || (implementation != NativeGvmLearnerImplementation::modern
            && implementation
                != NativeGvmLearnerImplementation::old_fast)
        || plan.initial_clusters == 0u
        || plan.initial_clusters > plan.common_limit
        || plan.minimum_clusters > plan.initial_clusters) {
        error = "native GVM search has invalid input or cluster bounds";
        return false;
    }
    std::uint32_t clusters = plan.initial_clusters;
    bool reducing_clusters = false;
    std::vector<NativeGvmLearnedResult> learned_cache(
        static_cast<std::size_t>(plan.common_limit) + 1u);
    std::vector<bool> learned_cached(
        static_cast<std::size_t>(plan.common_limit) + 1u, false);
    for (;;) {
        if (out.learn_attempts == std::numeric_limits<std::uint32_t>::max()) {
            error = "native GVM search attempt counter overflows";
            return false;
        }
        ++out.learn_attempts;
        NativeGvmLearnedResult learned_result{};
        if (learned_cached[clusters]) {
            learned_result = learned_cache[clusters];
        } else {
            NativeGvmLearnerOutput learner_output{};
            const bool learned_ok =
                implementation == NativeGvmLearnerImplementation::modern
                ? learn_native_gvm_modern(
                    input, clusters, learner_output, error)
                : learn_native_gvm_old_fast(
                    input, clusters, learner_output, error);
            if (!learned_ok)
                return false;
            if (!finalize_native_gvm_learned_result(
                    learner_output.sizes,
                    learner_output.indices,
                    learner_output.center_scalars,
                    input.shape,
                    config,
                    bit_budget,
                    learned_result,
                    error)) {
                return false;
            }
            learned_cache[clusters] = learned_result;
            learned_cached[clusters] = true;
        }
        // GVM:run_ enters a nested decrement loop after status 1.
        // The first subsequent status other than 1 leaves that loop and is
        // accepted immediately; it must not return to the outer status-3
        // expansion path or the search can oscillate forever.
        if (reducing_clusters && learned_result.fit_status != 1u) {
            out.learned = std::move(learned_result);
            out.clusters =
                static_cast<std::uint32_t>(out.learned.sizes.size());
            return true;
        }
        NativeGvmSearchStep step{};
        if (!advance_native_gvm_search(
                plan, learned_result.fit_status, clusters, step, error)) {
            return false;
        }
        if (step.action == NativeGvmSearchAction::accept) {
            out.learned = std::move(learned_result);
            out.clusters =
                static_cast<std::uint32_t>(out.learned.sizes.size());
            return true;
        }
        if (step.action != NativeGvmSearchAction::retry
            || step.clusters == clusters) {
            error = "native GVM search cannot make progress";
            return false;
        }
        reducing_clusters = learned_result.fit_status == 1u;
        clusters = step.clusters;
    }
}

bool run_native_gvm_modern_search(
    const NativeGvmLearnerInput& input,
    const NativeGvmSearchPlan& plan,
    std::uint64_t bit_budget,
    const NativeGvmConfiguration& config,
    NativeGvmSearchResult& out,
    std::string& error) {
    return run_native_gvm_search(
        input,
        plan,
        bit_budget,
        config,
        NativeGvmLearnerImplementation::modern,
        out,
        error);
}

bool run_native_gvm_old_fast_search(
    const NativeGvmLearnerInput& input,
    const NativeGvmSearchPlan& plan,
    std::uint64_t bit_budget,
    const NativeGvmConfiguration& config,
    NativeGvmSearchResult& out,
    std::string& error) {
    return run_native_gvm_search(
        input,
        plan,
        bit_budget,
        config,
        NativeGvmLearnerImplementation::old_fast,
        out,
        error);
}

bool cluster_deltas_quantize_bitshift100(
    const std::vector<std::int32_t>& deltas,
    std::uint32_t shift,
    std::uint64_t bit_budget,
    ClusterDeltasQuantizationResult& out,
    std::string& error) {
    error.clear();
    out = {};
    out.shift_attempts = 1u;
    if (shift > 30u) {
        error = "cluster_deltas shift out of range";
        return false;
    }

    // process_: scale = 100<<shift, bias = scale + ((1<<shift)>>1).
    const std::uint32_t scale_lo = static_cast<std::uint32_t>(100u << shift);

    std::uint64_t histogram[kBinCount + 1u] = {};
    for (std::int32_t sample : deltas) {
        std::uint64_t bin = bitshift100_bin(sample, shift);
        if (bin >= 0xC9u)
            bin = 201u;
        ++histogram[static_cast<std::uint32_t>(bin)];
    }

    // Linked lists keyed by count for bins 0..200 (native inserts i,i+1,i+2).
    std::vector<std::uint64_t> head(deltas.size() + 2u, ~0ull);
    std::vector<std::uint64_t> next_bin(kBinCount, ~0ull);
    for (std::uint32_t bin = 0; bin < kBinCount; ++bin) {
        const std::uint64_t count = histogram[bin];
        if (count >= head.size()) {
            error = "cluster_deltas histogram count exceeds head table";
            return false;
        }
        next_bin[bin] = head[static_cast<std::size_t>(count)];
        head[static_cast<std::size_t>(count)] = bin;
    }

    std::uint64_t max_count = 0u;
    for (std::uint32_t bin = 0; bin < kBinCount; ++bin) {
        if (histogram[bin] > max_count)
            max_count = histogram[bin];
    }

    // bin_to_code maps occupied bins 0..200 onto codebook indices.
    std::vector<std::uint64_t> bin_to_code(kBinCount, ~0ull);
    std::uint64_t code_index = 0u;
    if (max_count != 0u) {
        for (std::uint64_t count = max_count; count > 0u; --count) {
            std::uint64_t bin = head[static_cast<std::size_t>(count)];
            while (bin != ~0ull) {
                bin_to_code[static_cast<std::size_t>(bin)] = code_index;
                // BitSize consumes codebook populations in code order; the
                // bin coordinate belongs only to the reconstructed center.
                out.levels.push_back(histogram[bin]);
                // Residual reconstruction uses scale low dword, not bias.
                const std::int32_t reconstructed = static_cast<std::int32_t>(
                    (static_cast<std::uint32_t>(bin) << shift) - scale_lo);
                out.residuals.push_back(reconstructed);
                ++code_index;
                bin = next_bin[static_cast<std::size_t>(bin)];
            }
        }
    }

    out.indices.reserve(deltas.size());
    for (std::int32_t sample : deltas) {
        std::uint64_t bin = bitshift100_bin(sample, shift);
        if (bin > 0xC8u) {
            // OOR: append raw sample residual, level=1, index = residual slot.
            const std::uint64_t residual_index = out.residuals.size();
            out.residuals.push_back(sample);
            out.levels.push_back(1u);
            out.indices.push_back(residual_index);
        } else {
            const std::uint64_t code = bin_to_code[static_cast<std::size_t>(bin)];
            if (code == ~0ull) {
                error = "cluster_deltas missing codebook entry for occupied bin";
                return false;
            }
            out.indices.push_back(code);
        }
    }

    out.shift = shift;
    if (!bit_size_calculate(
            out.levels,
            out.residuals,
            1u,
            out.residual_bit_width,
            out.residual_bit_cost,
            out.level_pack_mode,
            out.level_bit_cost)) {
        return false;
    }
    return out.residuals.size() <= kCodecV3ChannelCodebookMaxEntries
        && out.residual_bit_cost + out.level_bit_cost <= bit_budget;
}

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
    std::string& error) {
    error.clear();
    out = {};
    NativeGvmInputShape gvm_input{};
    if (!validate_native_gvm_input_shape(
            deltas.size(), sample_count, gvm_input, error)) {
        error = "mix2 quantization: " + error;
        return false;
    }
    if (gvm_input.dimensions != 1u) {
        error = "mix2 quantization expects one delta per sample";
        return false;
    }
    if (bit_line < 3u || bit_line > 12u) {
        error = "mix2 quantization bit line is outside Config::validate range";
        return false;
    }

    std::uint64_t bit_budget = 0u;
    std::uint64_t fixed_cost = 0u;
    if (!calculate_native_quantizer_budget(
            sample_count,
            bit_line,
            group_field_208,
            rescaler_bit_cost,
            bit_budget,
            fixed_cost,
            error)) {
        error = "mix2 quantization: " + error;
        return false;
    }
    std::uint32_t shift_attempts = 0u;
    std::string static_error;
    if (backend == NativeClusterDeltasBackend::quantization) {
        ++shift_attempts;
        if (cluster_deltas_quantize_bitshift100(
                deltas, 0u, bit_budget, out, error)) {
            out.shift_attempts = shift_attempts;
            out.gvm_input = gvm_input;
            out.bit_budget = bit_budget;
            out.fixed_bit_cost = fixed_cost;
            return true;
        }
        for (std::uint32_t shift = 1u; shift <= 30u; ++shift) {
            ++shift_attempts;
            ClusterDeltasQuantizationResult candidate{};
            std::string local_error;
            if (cluster_deltas_quantize_bitshift100(
                    deltas, shift, bit_budget, candidate, local_error)) {
                candidate.shift_attempts = shift_attempts;
                candidate.gvm_input = gvm_input;
                candidate.bit_budget = bit_budget;
                candidate.fixed_bit_cost = fixed_cost;
                out = std::move(candidate);
                error.clear();
                return true;
            }
            error = local_error;
        }
        static_error = error;
        error = "native Quantization exhausted BitShift candidates";
        if (!static_error.empty())
            error += "; last BitShift error: " + static_error;
        return false;
    }

    // create_and_configure selects exactly one backend. GVM
    // profiles do not run the BitShift Quantization path first.
    NativeGvmLearnerInput learner_input{};
    if (!prepare_native_gvm_learner_input(
            deltas, gvm_input, gvm, learner_input, error)) {
        error = "mix2 GVM preflight: " + error;
        return false;
    }
    NativeGvmSearchResult search_result{};
    const bool searched =
        gvm.learner == NativeGvmLearnerImplementation::modern
        ? run_native_gvm_modern_search(
            learner_input,
            gvm_search,
            bit_budget,
            gvm,
            search_result,
            error)
        : gvm.learner == NativeGvmLearnerImplementation::old_fast
            && run_native_gvm_old_fast_search(
                learner_input,
                gvm_search,
                bit_budget,
                gvm,
                search_result,
                error);
    if (!searched) {
        if (error.empty())
            error = "native GVM learner implementation is unavailable";
        else
            error = "mix2 GVM search: " + error;
        return false;
    }
    out.levels = std::move(search_result.learned.sizes);
    out.indices = std::move(search_result.learned.indices);
    out.residuals = std::move(search_result.learned.centers.scalars);
    out.residual_bit_width =
        search_result.learned.cost.residual_bit_width;
    out.level_pack_mode = search_result.learned.cost.level_pack_mode;
    out.residual_bit_cost =
        search_result.learned.cost.residual_bit_cost;
    out.level_bit_cost = search_result.learned.cost.level_bit_cost;
    out.bit_budget = bit_budget;
    out.fixed_bit_cost = fixed_cost;
    out.gvm_fit_status = search_result.learned.fit_status;
    out.gvm_compact_fit = search_result.learned.compact_fit;
    out.gvm_input = gvm_input;
    out.gvm_learner_points =
        static_cast<std::uint32_t>(learner_input.points.size());
    out.gvm_deterministic_seed = learner_input.deterministic_seed;
    out.gvm_learned = true;
    out.gvm_selected_clusters = search_result.clusters;
    out.gvm_learn_attempts = search_result.learn_attempts;
    out.gvm_forced_zero_center =
        search_result.learned.centers.forced_zero_center;
    out.gvm_forced_zero_index =
        search_result.learned.centers.forced_zero_index;
    return true;
}

} // namespace auro3d:encode
