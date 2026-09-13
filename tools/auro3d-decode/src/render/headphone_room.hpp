#pragma once

#include <array>
#include <cstdint>

namespace auro3d {

// Geometry portion of auro_headphones_v2_Room_t. Coordinates use the native
// Q23 integer representation (1.0 float unit is 8,388,600 integer units).
class HeadphoneRoom {
public:
    bool construct(const std::array<float, 6>& bounds) noexcept;
    bool contains(std::int32_t x, std::int32_t y, std::int32_t z,
                  std::int32_t reflection_extent) const noexcept;
    std::int64_t reflection_position(std::int64_t xy, int wall) const noexcept;

    const std::array<std::int32_t, 6>& fixed_bounds() const noexcept { return bounds_; }

private:
    std::array<std::int32_t, 6> bounds_{};
};

}
