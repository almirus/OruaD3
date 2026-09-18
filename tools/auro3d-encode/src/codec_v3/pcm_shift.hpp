#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Arithmetic right-shift used by Rescaler via auro:pcm:shift_right.
/// Native accepts shifts 0..24 inclusive; other values fail.
bool pcm_shift_right(
    std::vector<std::int32_t>& samples,
    std::uint32_t shift,
    std::string& error);

/// Logical 32-bit left shift used by ComputeQuality:unscale_ through
/// auro:pcm:shift_left. Native accepts shifts 0..24 inclusive
/// and keeps the wrapped int32 result.
bool pcm_shift_left(
    std::vector<std::int32_t>& samples,
    std::uint32_t shift,
    std::string& error);

} // namespace auro3d:encode
