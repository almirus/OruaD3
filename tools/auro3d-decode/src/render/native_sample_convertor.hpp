#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace auro3d {

// Artist Connection 1.21.31 A3DENG v3 SampleConvertor callback 0x4CFF00:
// clamp Float32 to [-255, 255], scale by 2^23, truncate toward zero, then
// saturate to signed PCM24.  The converter is stateless and applies no dither.
inline std::int32_t native_float_to_pcm24(float sample) noexcept {
    const float bounded = std::min(255.0f, std::max(-255.0f, sample));
    const float scaled = bounded * 8388608.0f;
    if (!std::isfinite(scaled))
        return -8388608;
    const double truncated = std::trunc(static_cast<double>(scaled));
    if (truncated >= 8388607.0)
        return 8388607;
    if (truncated <= -8388608.0)
        return -8388608;
    return static_cast<std::int32_t>(truncated);
}

} // namespace auro3d
