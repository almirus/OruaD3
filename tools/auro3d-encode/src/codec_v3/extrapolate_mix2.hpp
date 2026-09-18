#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Stateful fields used by decoder Extrapolate mode 2 before PCM scaling.
/// The caller supplies exact unscaled components selected by group processing;
/// this layer neither chooses a mix nor alters a component to force a fit.
struct ExtrapolateMix2State {
    std::uint32_t count = 0;
    std::int32_t last_residual = 0;
    std::int32_t prev_primary = 0;
    std::int32_t prev_secondary = 0;
};

struct ExtrapolateMix2Seeds {
    std::int32_t seed0_primary = 0;
    std::int32_t seed0_secondary = 0;
};

/// Inverse of Extrapolate mode 2 through the residual state update, before
/// scale_shift_clamp_pcm24. carrier is already right-shifted by the frame
/// quantization shift. Both components must add to every carrier sample and
/// satisfy native predictor slots exactly.
bool encode_extrapolate_mix2_errors(
    const std::vector<std::int32_t>& carrier,
    const std::vector<std::int32_t>& primary,
    const std::vector<std::int32_t>& secondary,
    const ExtrapolateMix2Seeds& seeds,
    ExtrapolateMix2State& state,
    std::vector<std::int32_t>& errors,
    std::string& error);

} // namespace auro3d:encode
