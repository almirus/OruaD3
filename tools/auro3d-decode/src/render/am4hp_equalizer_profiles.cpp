#include "am4hp_equalizer_profiles.hpp"

#include <cstdint>
#include <cstring>

namespace auro3d {
namespace {

using Band = Am4hpEqualizer::Band;

float from_bits(std::uint32_t bits) noexcept {
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

Band make_band(const std::array<std::uint32_t, 5>& bits,
               float gain_db = 0.0f) noexcept {
    return Band{{from_bits(bits[0]), from_bits(bits[1]), from_bits(bits[2]),
                 from_bits(bits[3]), from_bits(bits[4])}, gain_db};
}

} // namespace

std::array<Band, 3> am4hp_native_eq_hl_hr_48000() noexcept {
    return {{
        make_band({0x3f56d8d4u, 0xbf54b3e2u, 0x3ec68becu, 0xbf8c740bu, 0x3efca5fbu},
                  -2.0f),
        make_band({0x3f81df7cu, 0xbf614cc4u, 0x3f3eda8fu, 0xbf614cc4u, 0x3f429988u}),
        make_band({0x3f7f2d25u, 0xbffb156eu, 0x3f78d074u, 0xbffb156eu, 0x3f77fd99u}),
    }};
}

std::array<Band, 3> am4hp_native_eq_hls_hrs_48000() noexcept {
    return {{
        make_band({0x3f56d8d4u, 0xbf54b3e2u, 0x3ec68becu, 0xbf8c740bu, 0x3efca5fbu},
                  -2.0f),
        make_band({0x3f83c70cu, 0xbf62d24bu, 0x3f3e167eu, 0xbf62d24bu, 0x3f45a497u}),
        make_band({0x3f7ec323u, 0xbffad9e7u, 0x3f78c2fau, 0xbffad9e7u, 0x3f77861eu}),
    }};
}

std::array<Band, 3> am4hp_native_surround_eq_hl_hr_48000() noexcept {
    return {{
        make_band({0x3f44d490u, 0xbf3d4d78u, 0x3eb2e6d3u, 0xbf8fb34au, 0x3f006116u},
                  -3.0f),
        make_band({0x3f81df7cu, 0xbf614cc4u, 0x3f3eda8fu, 0xbf614cc4u, 0x3f429988u}),
        make_band({0x3f7f2d25u, 0xbffb156eu, 0x3f78d074u, 0xbffb156eu, 0x3f77fd99u}),
    }};
}

std::array<Band, 3> am4hp_native_surround_eq_hls_hrs_48000() noexcept {
    return {{
        make_band({0x3f44d490u, 0xbf3d4d78u, 0x3eb2e6d3u, 0xbf8fb34au, 0x3f006116u},
                  -3.0f),
        make_band({0x3f83c70cu, 0xbf62d24bu, 0x3f3e167eu, 0xbf62d24bu, 0x3f45a497u}),
        make_band({0x3f7ec323u, 0xbffad9e7u, 0x3f78c2fau, 0xbffad9e7u, 0x3f77861eu}),
    }};
}

}
