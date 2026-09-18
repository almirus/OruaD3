#include "headphone_late_reverb_path.hpp"

#include "headphone_pca_accumulator.hpp"

#include <cmath>
#include <cstdint>

namespace auro3d {

bool HeadphoneLateReverbPath::construct(int itd_delay_samples,
                                         std::size_t score_count) {
    if (score_count == 0u || !delay_.construct(static_cast<std::size_t>(
            itd_delay_samples < 0
                ? -static_cast<std::int64_t>(itd_delay_samples)
                : itd_delay_samples)))
        return false;
    itd_delay_samples_ = itd_delay_samples;
    score_count_ = score_count;
    reset_audio_state();
    return true;
}

void HeadphoneLateReverbPath::reset_audio_state() noexcept {
    delay_.reset_audio_state();
}

bool HeadphoneLateReverbPath::process(
    const std::array<float, 32>& input,
    const std::vector<float>& first_ear_gains,
    const std::vector<float>& second_ear_gains,
    std::vector<std::array<float, 32>>& first_ear_outputs,
    std::vector<std::array<float, 32>>& second_ear_outputs) noexcept {
    if (first_ear_gains.size() != score_count_
        || second_ear_gains.size() != score_count_
        || first_ear_outputs.size() < score_count_
        || second_ear_outputs.size() < score_count_)
        return false;

    delay_.add_buffer(input);
    std::array<float, 32> delayed{};
    if (!delay_.get_delay(static_cast<std::size_t>(
            itd_delay_samples_ < 0
                ? -static_cast<std::int64_t>(itd_delay_samples_)
                : itd_delay_samples_), delayed))
        return false;

    const std::array<float, 32>& first_ear_input =
        itd_delay_samples_ < 0 ? input : delayed;
    const std::array<float, 32>& second_ear_input =
        itd_delay_samples_ < 0 ? delayed : input;
    // The selected oracle rounds the multiply before the add here, as
    // it does in source_Explicit and source_EarlyReflection accumulation.
    for (std::size_t score = 0u; score < score_count_; ++score) {
        HeadphonePcaAccumulator::add(first_ear_input, first_ear_gains[score],
                                     first_ear_outputs[score]);
        HeadphonePcaAccumulator::add(second_ear_input, second_ear_gains[score],
                                     second_ear_outputs[score]);
    }
    return true;
}

}
