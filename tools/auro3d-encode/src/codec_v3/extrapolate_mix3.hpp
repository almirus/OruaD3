#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

struct ExtrapolateMix3State {
    std::uint32_t phase = 0;
    std::int32_t last_a = 0;
    std::int32_t last_b = 0;
    std::int32_t last_c = 0;
    std::int32_t predictor = 0;
};

struct ExtrapolateMix3Seeds {
    std::int32_t seed0_primary = 0;
    std::int32_t seed0_secondary = 0;
    std::int32_t seed1_primary = 0;
    std::int32_t seed1_secondary = 0;
    std::int32_t seed2_primary = 0;
};

/// Inverse of the decoder's first three mode-3 samples. The returned six
/// residuals are ordered as decoder errors[0..5], with pair injection phases
/// 0, 1, and 2 respectively. Later samples use the state returned here.
bool encode_extrapolate_mix3_initial(
    const std::vector<std::int32_t>& carrier,
    const std::vector<std::int32_t>& primary,
    const std::vector<std::int32_t>& secondary,
    const std::vector<std::int32_t>& tertiary,
    const ExtrapolateMix3Seeds& seeds,
    ExtrapolateMix3State& state,
    std::vector<std::int32_t>& errors,
    std::string& error);

/// Encodes later mode-3 samples when native group processing has already
/// selected the three decoder baseline components. This function only derives
/// the two residuals injected for the current phase and validates every
/// predictor/state transition.
bool encode_extrapolate_mix3_tail(
    const std::vector<std::int32_t>& carrier,
    const std::vector<std::int32_t>& primary,
    const std::vector<std::int32_t>& secondary,
    const std::vector<std::int32_t>& tertiary,
    const std::vector<std::int32_t>& baseline_primary,
    const std::vector<std::int32_t>& baseline_secondary,
    const std::vector<std::int32_t>& baseline_tertiary,
    ExtrapolateMix3State& state,
    std::vector<std::int32_t>& errors,
    std::string& error);

} // namespace auro3d::encode
