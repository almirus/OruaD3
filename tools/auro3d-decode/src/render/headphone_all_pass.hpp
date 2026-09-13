#pragma once

#include "headphone_multi_delay.hpp"

#include <array>
#include <cstddef>

namespace auro3d {

// Native auro_headphones_v2_AllPass is a 32-sample block all-pass backed by
// MultiDelay. The delay must be at least one block (32 samples).
class HeadphoneAllPass {
public:
    bool construct(std::size_t delay_samples);
    float set_rt60(unsigned sample_rate, float rt60_seconds) noexcept;
    bool set_coefficient(float coefficient) noexcept;
    void reset_audio_state() noexcept;
    bool process(const std::array<float, 32>& input,
                 std::array<float, 32>& output) noexcept;
    bool process_in_place(std::array<float, 32>& block) noexcept;


    std::size_t delay_samples() const noexcept { return delay_samples_; }
    float coefficient() const noexcept { return coefficient_; }

private:
    HeadphoneMultiDelay delay_;
    std::size_t delay_samples_ = 0;
    float coefficient_ = 0.0f;
};

}
