#include "extrapolate_math.hpp"

#include <array>
#include <cmath>
#include <limits>

namespace auro3d::encode {
namespace {

const std::array<float, 241u>& scale_table() {
    static const std::array<float, 241u> table = [] {
        std::array<float, 241u> values{};
        for (std::size_t index = 0; index < values.size(); ++index)
            values[index] = std::exp(static_cast<float>(index) * 0.011512925465f);
        values[60u] = 2.0f;
        values[120u] = 4.0f;
        values[180u] = 8.0f;
        values[240u] = 16.0f;
        return values;
    }();
    return table;
}

std::int32_t cvtt_float_to_i32(float value) {
    if (!std::isfinite(value) || value >= 2147483648.0f || value < -2147483648.0f)
        return std::numeric_limits<std::int32_t>::min();
    return static_cast<std::int32_t>(value);
}

std::int32_t arithmetic_shift_right(std::int32_t value, std::uint32_t shift) {
    if (shift == 0u)
        return value;
    if (shift >= 32u)
        return value < 0 ? -1 : 0;
    const std::uint32_t raw = static_cast<std::uint32_t>(value);
    const std::uint32_t fill = value < 0 ? 0xFFFFFFFFu << (32u - shift) : 0u;
    return static_cast<std::int32_t>((raw >> shift) | fill);
}

} // namespace

bool codec_v3_extrapolate_scale(std::uint32_t index, float& scale) {
    if (index >= scale_table().size())
        return false;
    scale = scale_table()[index];
    return true;
}

std::int32_t codec_v3_clamp_pcm24(std::int32_t value) {
    if (value >= 0x7FFFFF)
        return 0x7FFFFF;
    if (value < -8388607)
        return -8388608;
    return value;
}

std::int32_t codec_v3_scale_shift_clamp_pcm24(
    std::int32_t value,
    float scale,
    std::uint32_t shift) {
    const std::int32_t scaled = cvtt_float_to_i32(static_cast<float>(value) * scale);
    const std::int32_t shifted = shift < 32u
        ? static_cast<std::int32_t>(static_cast<std::uint32_t>(scaled) << shift)
        : 0;
    return codec_v3_clamp_pcm24(shifted);
}

std::int32_t codec_v3_mix3_div4(std::int32_t previous, std::int32_t predictor) {
    const std::int32_t sum = static_cast<std::int32_t>(
        3u * static_cast<std::uint32_t>(predictor)
        + static_cast<std::uint32_t>(previous));
    const std::int32_t adjusted = sum >= 0 ? sum : sum + 3;
    return arithmetic_shift_right(adjusted, 2u);
}

} // namespace auro3d::encode
