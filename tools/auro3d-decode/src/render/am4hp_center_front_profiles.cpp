#include "am4hp_center_front_profiles.hpp"

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

std::pair<Am4hpCenterFront::Coefficients,
          Am4hpCenterFront::Coefficients>
am4hp_native_center_front_48000_coefficients() noexcept {
    return {
        {f32(0x3f88c0aeu), f32(0xbfc3c87du), f32(0x3f32a2f2u),
         f32(0xbfc3c87du), f32(0x3f44244fu)},
        {f32(0x3f9692c1u), f32(0xbf86dcabu), f32(0x3f449277u),
         f32(0xbf3ecda8u), f32(0x3f22cc49u)}
    };
}

bool construct_am4hp_native_center_front_48000(
    Am4hpCenterFront& stage, std::uint32_t layout_mask) noexcept {
    const auto coefficients = am4hp_native_center_front_48000_coefficients();
    // The live 0x37 wrapper stores -6 dB at +40 and its corresponding
    // 0.501187 gain at +144; stereo stores -9 dB 0.354813.
    const float tuning = layout_mask == 0x37u ? -6.0f : -9.0f;
    return stage.construct(48000u, layout_mask, tuning,
                           coefficients.first, coefficients.second)
        // Native stereo uses +2 dB (+1120=0x3f21866c,
        // +1124=0x3f0ff59a). The live 0x37 Core snapshot instead stores
        // 0x3f4b5918/0x3f353bef, the exact -2/-3 dB linear pair produced by
        // a +4 dB CenterFront setting.
        && stage.set_gain_db(layout_mask == 0x37u ? 4.0f : 2.0f);
}

}
