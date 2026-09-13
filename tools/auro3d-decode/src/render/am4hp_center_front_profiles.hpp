#pragma once

#include "am4hp_center_front.hpp"

#include <utility>

namespace auro3d {

// Captured from the 48-kHz Artist Connection 1.21.31 x86_64 AM4HP core
// snapshot (CenterFront offsets +1072 and +1100). Values are raw native
// float32 coefficients, not coefficients reconstructed from decimal values.
std::pair<Am4hpCenterFront::Coefficients,
          Am4hpCenterFront::Coefficients>
am4hp_native_center_front_48000_coefficients() noexcept;

bool construct_am4hp_native_center_front_48000(
    Am4hpCenterFront& stage,
    std::uint32_t layout_mask = 0x3fu) noexcept;

}
