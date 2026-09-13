#pragma once

#include "headphone_all_pass.hpp"
#include "headphone_multi_delay.hpp"
#include "headphone_pca_score_state.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace auro3d {

// Native source_EarlyReflection path. Distance processing is mono; signed ITD
// produces separate ear blocks before the two PCA gain arrays are accumulated.
class HeadphoneEarlyReflectionPath {
public:
    bool construct(std::size_t distance_delay_samples,
                   int itd_delay_samples,
                   float distance_gain,
                   std::size_t diffusion_delay_samples = 0u);
    bool set_distance_gain_scale(float scale) noexcept;
    bool set_gain_reference_linear(float reference_linear) noexcept;
    bool set_dynamic_geometry_gain(float geometry_gain) noexcept;
    bool set_distance_gain_from_db(float gain_db) noexcept;
    bool set_diffusion_rt60(unsigned sample_rate, float rt60_seconds) noexcept;
    bool set_diffusion_coefficient(float coefficient) noexcept;
    void reset_audio_state() noexcept;
    bool process(const std::array<float, 32>& input,
                 std::array<float, 32>& first_ear,
                 std::array<float, 32>& second_ear) noexcept;
    bool process_from_shared_delay(
        const HeadphoneMultiDelay& input_delay,
        std::array<float, 32>& first_ear,
        std::array<float, 32>& second_ear) const noexcept;
    std::size_t distance_delay_samples() const noexcept {
        return distance_delay_.configured_delay();
    }
    float distance_gain() const noexcept { return distance_gain_; }
    const std::array<float, 32>& debug_pre_gain_first() const noexcept { return debug_pre_gain_first_; }
    const std::array<float, 32>& debug_pre_gain_second() const noexcept { return debug_pre_gain_second_; }
    int itd_delay_samples() const noexcept { return itd_delay_samples_; }
    bool process_with_pca_gains(
        const std::array<float, 32>& input,
        const std::vector<float>& first_ear_gains,
        const std::vector<float>& second_ear_gains,
        std::vector<std::array<float, 32>>& first_ear_outputs,
        std::vector<std::array<float, 32>>& second_ear_outputs) noexcept;
    bool process_with_pca_state(
        const std::array<float, 32>& input,
        const HeadphonePcaScoreState& scores,
        std::vector<std::array<float, 32>>& first_ear_outputs,
        std::vector<std::array<float, 32>>& second_ear_outputs) noexcept;

private:
    HeadphoneMultiDelay distance_delay_;
    HeadphoneMultiDelay itd_delay_;
    HeadphoneAllPass diffusion_;
    int itd_delay_samples_ = 0;
    float constructed_distance_gain_ = 0.0f;
    float distance_gain_ = 0.0f;
    float reference_linear_ = 1.0f;
    float dynamic_geometry_gain_ = 0.0f;
    bool has_dynamic_geometry_gain_ = false;
    bool has_diffusion_ = false;
    mutable std::array<float, 32> debug_pre_gain_first_{};
    mutable std::array<float, 32> debug_pre_gain_second_{};
};

}
