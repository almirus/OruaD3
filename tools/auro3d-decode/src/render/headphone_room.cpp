#include "headphone_room.hpp"

namespace auro3d {
namespace {
constexpr float kQ23 = 8388600.0f;

std::int32_t native_round(float value) noexcept {
    return static_cast<std::int32_t>(value + (value >= 0.0f ? 0.5f : -0.5f));
}

std::int32_t floor_half(std::int32_t value) noexcept {
    return (value + (value < 0 ? 1 : 0)) >> 1;
}
}

bool HeadphoneRoom::construct(const std::array<float, 6>& bounds) noexcept {
    const auto min_x = native_round(-bounds[0] * kQ23);
    const auto max_x = native_round(bounds[1] * kQ23);
    const auto min_y = native_round(-bounds[5] * kQ23);
    const auto max_y = native_round(bounds[4] * kQ23);
    const auto min_z = native_round(-bounds[2] * kQ23);
    const auto max_z = native_round(bounds[3] * kQ23);
    bounds_[0] = (max_x + min_x) / 2;
    bounds_[1] = (max_y + min_y) / 2;
    bounds_[2] = floor_half(max_z + min_z);
    bounds_[3] = max_x + native_round(bounds[0] * kQ23);
    bounds_[4] = max_y + native_round(bounds[5] * kQ23);
    bounds_[5] = max_z + native_round(bounds[2] * kQ23);
    return true;
}

bool HeadphoneRoom::contains(std::int32_t x, std::int32_t y, std::int32_t z,
                             std::int32_t reflection_extent) const noexcept {
    const auto half_y = bounds_[4] / 2;
    const auto half_z = floor_half(reflection_extent);
    return x > bounds_[0] - bounds_[3] / 2
        && bounds_[0] + bounds_[3] / 2 > x
        && y > bounds_[1] - half_y
        && bounds_[1] + half_y > y
        && z > bounds_[2] - half_z
        && bounds_[2] + half_z > z;
}

std::int64_t HeadphoneRoom::reflection_position(std::int64_t xy, int wall) const noexcept {
    const auto packed = static_cast<std::uint64_t>(xy);
    const auto x = static_cast<std::int32_t>(static_cast<std::uint32_t>(packed));
    const auto y = static_cast<std::int32_t>(static_cast<std::uint32_t>(packed >> 32));
    auto reflected_x = x;
    auto reflected_y = y;
    switch (wall) {
    case 0: reflected_x = 2 * bounds_[0] - bounds_[3] - x; break;
    case 1: reflected_x = bounds_[3] - x + 2 * bounds_[0]; break;
    case 2: reflected_y = bounds_[4] - y + 2 * bounds_[1]; break;
    case 3: reflected_y = 2 * bounds_[1] - bounds_[4] - y; break;
    default: break;
    }
    const auto reflected_packed = static_cast<std::uint64_t>(static_cast<std::uint32_t>(reflected_x))
        | (static_cast<std::uint64_t>(static_cast<std::uint32_t>(reflected_y)) << 32);
    return static_cast<std::int64_t>(reflected_packed);
}

}
