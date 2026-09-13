#pragma once

#include "headphone_multi_delay.hpp"
#include "headphone_pca_score_state.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace auro3d {

// Native source_Explicit delay/gain stages. PCA score accumulation is kept
// outside because it requires the external bank's interpolated coefficients.
class HeadphoneExplicitSourcePath {
public:
    bool construct(std::size_t distance_delay_samples,
                   int itd_delay_samples,
                   float distance_gain,
                   bool combined_distance_itd_mode);
    bool set_dynamic_gain(float distance_gain) noexcept;
    void reset_audio_state() noexcept;
    bool process(const std::array<float, 32>& input,
                 std::array<float, 32>& first_ear,
                 std::array<float, 32>& second_ear) noexcept;
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

    bool combined_distance_itd_mode() const noexcept {
        return combined_distance_itd_mode_;
    }
    int itd_delay_samples() const noexcept { return itd_delay_samples_; }

private:
    HeadphoneMultiDelay distance_delay_;
    HeadphoneMultiDelay itd_delay_;
    int itd_delay_samples_ = 0;
    float distance_gain_ = 0.0f;
    bool combined_distance_itd_mode_ = false;
};

}
