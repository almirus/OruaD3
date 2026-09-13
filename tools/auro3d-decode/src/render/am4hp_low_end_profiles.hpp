#pragma once

#include "am4hp_low_end.hpp"

#include <cstdint>
#include <utility>

namespace auro3d {

// Raw coefficients captured from the 48-kHz native AM4HP LowEnd object at
// stage offsets +12 and +32; flags are the captured object +8 field.
std::pair<Am4hpLowEnd::Coefficients, Am4hpLowEnd::Coefficients>
am4hp_native_low_end_48000_coefficients() noexcept;
std::uint32_t am4hp_native_low_end_48000_flags() noexcept;

bool construct_am4hp_native_low_end_48000(Am4hpLowEnd& stage) noexcept;

}
