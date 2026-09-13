#pragma once

#include "headphone_all_pass.hpp"
#include "headphone_multi_delay.hpp"
#include "headphone_wall_material_filter.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace auro3d {

// One normal-mode LateReverb band from sub_4F7934: the block is passed through
// the configured diffusion AllPass chain, then damping, and finally retained
// in the band's MultiDelay for subsequent blocks.
class HeadphoneLateReverbBand {
public:
    bool construct(std::size_t delay_samples,
                   const std::vector<std::size_t>& allpass_delays,
                   const std::array<float, 5>& damping_coefficients);
    bool set_allpass_rt60(unsigned sample_rate, float rt60_seconds) noexcept;
    bool set_allpass_coefficients(const std::vector<float>& coefficients) noexcept;
    void set_damping_coefficients(
        const std::array<float, 5>& coefficients) noexcept;
    void reset_audio_state() noexcept;
    bool process(const std::array<float, 32>& input,
                 std::array<float, 32>& output) noexcept;
    bool mix_delayed_feedback(float gain,
                              std::array<float, 32>& accumulator,
                              std::array<float, 32>* delayed_debug = nullptr) noexcept;

    std::size_t delay_samples() const noexcept {
        return delay_.configured_delay();
    }
    // Native LateReverb stores this at band+320 and uses it for the RT60
    // feedback calculation: comb delay plus every diffusion all-pass delay.
    std::size_t feedback_delay_samples() const noexcept {
        std::size_t total = delay_.configured_delay();
        for (const auto& allpass : allpasses_)
            total += allpass.delay_samples();
        return total;
    }
    std::size_t allpass_count() const noexcept { return allpasses_.size(); }

private:
    HeadphoneMultiDelay delay_;
    std::vector<HeadphoneAllPass> allpasses_;
    HeadphoneWallMaterialFilter damping_;
};

}
