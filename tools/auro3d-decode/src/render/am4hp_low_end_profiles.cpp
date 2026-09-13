#include "am4hp_low_end_profiles.hpp"

#include <cstdint>
#include <cstring>

namespace auro3d {
namespace {
float f32(std::uint32_t bits) noexcept {
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
}

std::pair<Am4hpLowEnd::Coefficients, Am4hpLowEnd::Coefficients>
am4hp_native_low_end_48000_coefficients() noexcept {
    // Native LowEnd stores the high-pass set at a1+32..48 (the in-place
    // mutation of the front work planes) and the small low-pass set for the
    // final accumulated output filter.
    return {
        {f32(0x3f7b4df5u), f32(0xbffb4df5u), f32(0x3f7b4df5u),
         f32(0xbffb42eeu), f32(0x3f76b1f6u)},
        {f32(0x39306535u), f32(0x39b06535u), f32(0x39306535u),
         f32(0xbffb42eeu), f32(0x3f76b1f6u)}
    };
}

// Live native AM4HP LowEnd object field +8 is 0x00000003 for the selected
// stereo profile. In particular, bits 2/3/4/5 are clear, so optional LFE and
// height inputs must not be consumed for this layout.
std::uint32_t am4hp_native_low_end_48000_flags() noexcept { return 0x3u; }

bool construct_am4hp_native_low_end_48000(Am4hpLowEnd& stage) noexcept {
    const auto coefficients = am4hp_native_low_end_48000_coefficients();
    return stage.construct(coefficients.first, coefficients.second,
                           am4hp_native_low_end_48000_flags());
}

}
