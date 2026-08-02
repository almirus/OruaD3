#pragma once

#include <cstdint>

namespace auro3d::encode {

/// Decoder-compatible scale table used by codec-v3 Extrapolate frame fields.
/// Index range is 0..240 inclusive.
bool codec_v3_extrapolate_scale(std::uint32_t index, float& scale);

std::int32_t codec_v3_clamp_pcm24(std::int32_t value);
std::int32_t codec_v3_scale_shift_clamp_pcm24(
    std::int32_t value,
    float scale,
    std::uint32_t shift);

/// Exact signed rounding used by mix3 before residual injection.
std::int32_t codec_v3_mix3_div4(std::int32_t previous, std::int32_t predictor);

} // namespace auro3d::encode
