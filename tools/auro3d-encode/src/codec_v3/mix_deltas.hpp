#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Even-round toward zero used throughout mix2 deltas/mix: (x + (x>>31)) & ~1.
inline std::int32_t mix_even_round(std::int32_t value) {
    return static_cast<std::int32_t>(
        (static_cast<std::uint32_t>(value) + (static_cast<std::uint32_t>(value) >> 31u))
        & 0xFFFFFFFEu);
}

/// Portable spelling of the arithmetic right shift emitted by native PSRAD
/// and signed SAR instructions. The encoder keeps all intermediate additions
/// in 32-bit wraparound form, so the sign fill must happen after that wrap.
inline std::int32_t codec_v3_arithmetic_shift_right(
    std::int32_t value,
    std::uint32_t shift) {
    if (shift == 0u)
        return value;
    if (shift >= 32u)
        return value < 0 ? -1 : 0;
    const std::uint32_t raw = static_cast<std::uint32_t>(value);
    const std::uint32_t fill = value < 0 ? 0xFFFFFFFFu << (32u - shift) : 0u;
    return static_cast<std::int32_t>((raw >> shift) | fill);
}

/// Direct port of auro:mix:deltas mix2 overload.
/// Writes one delta sample per input sample into `deltas` (same length).
bool compute_mix2_deltas(
    const std::vector<std::int32_t>& primary,
    const std::vector<std::int32_t>& secondary,
    std::vector<std::int32_t>& deltas,
    std::string& error);

/// Direct port of the three `mix3:deltas` specializations used by
/// ComputeDeltas. The native output is a 2*N interleaved residual
/// vector; all four fixed zero slots are retained.
bool compute_mix3_deltas(
    const std::vector<std::int32_t>& primary,
    const std::vector<std::int32_t>& secondary,
    const std::vector<std::int32_t>& tertiary,
    std::vector<std::int32_t>& deltas,
    std::string& error);

} // namespace auro3d:encode
