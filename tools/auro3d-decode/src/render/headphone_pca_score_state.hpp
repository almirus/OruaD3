#pragma once

#include "headphone_pca_accumulator.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace auro3d {

// Native PCAScore_t state: signed ITD in samples at offset 0 followed by two
// independent coefficient-array pointers at offsets 8 and 16. Bank
// decoding/population remains the responsibility of the PCABank loader.
class HeadphonePcaScoreState {
public:
    bool construct(std::size_t score_count);
    void reset() noexcept;
    void reset_audio_state() noexcept;
    bool set_gains(const std::vector<float>& first_ear,
                   const std::vector<float>& second_ear) noexcept;
    void set_itd_samples(std::int32_t samples) noexcept { itd_samples_ = samples; }
    bool accumulate(const std::array<float, 32>& first_ear_block,
                    const std::array<float, 32>& second_ear_block,
                    std::vector<std::array<float, 32>>& first_ear_outputs,
                    std::vector<std::array<float, 32>>& second_ear_outputs) const noexcept;
    bool accumulate_mono(const std::array<float, 32>& block,
                         std::vector<std::array<float, 32>>& outputs) const noexcept;

    std::size_t score_count() const noexcept { return first_ear_gains_.size(); }
    std::int32_t itd_samples() const noexcept { return itd_samples_; }
    const std::vector<float>& first_ear_gains() const noexcept {
        return first_ear_gains_;
    }
    const std::vector<float>& second_ear_gains() const noexcept {
        return second_ear_gains_;
    }

private:
    std::vector<float> first_ear_gains_;
    std::vector<float> second_ear_gains_;
    std::int32_t itd_samples_ = 0;
};

}
