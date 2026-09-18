#pragma once

#include <cstdint>

namespace auro3deng {

/// auro_a3deng_v3_strength_get_default
constexpr std::int32_t strength_get_default_value() { return 12; }

/// auro_a3deng_v3_strength_check_range — 0 ок, иначе 145
std::int32_t strength_check_range(std::uint32_t v);

/// auro_a3deng_v3_strength_translate (таблица, 15 float)
float strength_translate(std::uint32_t v);

} // namespace auro3deng
