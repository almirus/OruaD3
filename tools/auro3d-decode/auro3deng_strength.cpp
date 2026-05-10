#include "auro3deng_strength.hpp"

#include <cmath>

namespace auro3deng {

namespace {

// IDA: .rodata flt_1C3CF0 — 15 значений для powf(10, x * 0.05)
constexpr float kFlt1C3CF0[15] = {
    -30.0f, -27.0f, -21.0f, -18.0f, -15.0f, -12.0f, -9.0f, -6.0f,
    -3.0f, -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f,
};

} // namespace

std::int32_t strength_check_range(std::uint32_t v) {
    if (v < 0x10u)
        return 0;
    return 145;
}

float strength_translate(std::uint32_t v) {
    if (v == 0)
        return 0.0f;
    if (v > 0xFu)
        return 1.0f;
    return std::pow(10.0f, kFlt1C3CF0[v - 1] * 0.05f);
}

} // namespace auro3deng
