#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace auro3d {

// Native auro_matic_hp_v4_Headroom_process (x86_64 0x54C5E0): copy each
// mapped 32-sample plane from input to output while applying one scalar gain.
class Am4hpHeadroom {
public:
    bool construct(const std::vector<unsigned>& plane_mapping,
                   float gain) noexcept;
    void set_gain(float gain) noexcept { gain_ = gain; }
    void reset_audio_state() noexcept {}
    bool process(const std::vector<const std::array<float, 32>*>& input,
                 const std::vector<std::array<float, 32>*>& output) const noexcept;

private:
    std::vector<unsigned> plane_mapping_;
    float gain_ = 1.0f;
};

}
