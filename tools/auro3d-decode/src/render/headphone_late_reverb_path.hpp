#pragma once

#include "headphone_multi_delay.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace auro3d {

// Native source_LateReverb_process path after the reverb core has produced a
// 32-sample block: signed ITD delay followed by per-score PCA accumulation
// into the two ear output vectors. Bank lookup and the owning reverb core are
// deliberately outside this class.
class HeadphoneLateReverbPath {
public:
    bool construct(int itd_delay_samples, std::size_t score_count);
    void reset_audio_state() noexcept;

    bool process(const std::array<float, 32>& input,
                 const std::vector<float>& first_ear_gains,
                 const std::vector<float>& second_ear_gains,
                 std::vector<std::array<float, 32>>& first_ear_outputs,
                 std::vector<std::array<float, 32>>& second_ear_outputs) noexcept;

    int itd_delay_samples() const noexcept { return itd_delay_samples_; }
    std::size_t score_count() const noexcept { return score_count_; }

private:
    HeadphoneMultiDelay delay_;
    int itd_delay_samples_ = 0;
    std::size_t score_count_ = 0u;
};

}
