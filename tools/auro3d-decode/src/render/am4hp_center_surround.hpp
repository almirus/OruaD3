#pragma once

#include <array>
#include <cstdint>

namespace auro3d {

// Native AM4HP CenterSurround is a stateful 32-sample Centergen wrapper.
class Am4hpCenterSurround {
public:
    using Block = std::array<float, 32>;

    bool construct(std::uint32_t sample_rate, float tuning) noexcept;
    void reset_audio_state() noexcept;
    bool process(Block& input0,
                 Block& input1,
                 Block& output) noexcept;

private:
    alignas(16) std::array<std::uint8_t, 0x428u> centergen_{};
    bool configured_ = false;
};

// Captured 48-kHz DEFAULT CoreProcessor profile.
bool construct_am4hp_native_center_surround_48000(
    Am4hpCenterSurround& stage) noexcept;

}
