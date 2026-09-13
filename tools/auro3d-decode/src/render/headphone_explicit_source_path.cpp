#include "headphone_explicit_source_path.hpp"

#include "headphone_pca_accumulator.hpp"

#include <cmath>
#include <cstdint>

namespace auro3d {

bool HeadphoneExplicitSourcePath::construct(
    std::size_t distance_delay_samples, int itd_delay_samples,
    float distance_gain, bool combined_distance_itd_mode) {
    if (!std::isfinite(distance_gain))
        return false;
    const std::size_t abs_itd = static_cast<std::size_t>(
        itd_delay_samples < 0
            ? -static_cast<std::int64_t>(itd_delay_samples)
            : itd_delay_samples);
    if (combined_distance_itd_mode) {
        if (!distance_delay_.construct(distance_delay_samples + abs_itd))
            return false;
    } else if (!distance_delay_.construct(distance_delay_samples)
               || !itd_delay_.construct(abs_itd)) {
        return false;
    }
    itd_delay_samples_ = itd_delay_samples;
    distance_gain_ = distance_gain;
    combined_distance_itd_mode_ = combined_distance_itd_mode;
    reset_audio_state();
    return true;
}

bool HeadphoneExplicitSourcePath::set_dynamic_gain(float distance_gain) noexcept {
    if (!std::isfinite(distance_gain))
        return false;
    distance_gain_ = distance_gain;
    return true;
}

void HeadphoneExplicitSourcePath::reset_audio_state() noexcept {
    distance_delay_.reset_audio_state();
    if (!combined_distance_itd_mode_)
        itd_delay_.reset_audio_state();
}

bool HeadphoneExplicitSourcePath::process(
    const std::array<float, 32>& input,
    std::array<float, 32>& first_ear,
    std::array<float, 32>& second_ear) noexcept {
    if (combined_distance_itd_mode_) {
        distance_delay_.add_buffer(input);
        const std::size_t abs_itd = static_cast<std::size_t>(
            itd_delay_samples_ < 0
                ? -static_cast<std::int64_t>(itd_delay_samples_)
                : itd_delay_samples_);
        // Native source_Explicit_explicit_distance_delay_with_itd keeps one
        // ear at the distance delay and adds the signed ITD to the other.
        // The previous portable mapping applied the opposite shift and, for
        // negative ITD, subtracted it from the base delay.
        const std::size_t distance_delay =
            distance_delay_.configured_delay() - abs_itd;
        const std::size_t first_delay = itd_delay_samples_ >= 0
            ? distance_delay + abs_itd
            : distance_delay;
        const std::size_t second_delay = itd_delay_samples_ >= 0
            ? distance_delay
            : distance_delay + abs_itd;
        if (!distance_delay_.get_delay(first_delay, first_ear)
            || !distance_delay_.get_delay(second_delay, second_ear))
            return false;
    } else {
        distance_delay_.add_buffer(input);
        std::array<float, 32> distance{};
        if (!distance_delay_.get_delay(distance_delay_.configured_delay(),
                                       distance))
            return false;
        for (float& sample : distance)
            sample *= distance_gain_;
        itd_delay_.add_buffer(distance);
        const std::size_t abs_itd = static_cast<std::size_t>(
            itd_delay_samples_ < 0
                ? -static_cast<std::int64_t>(itd_delay_samples_)
                : itd_delay_samples_);
        if (itd_delay_samples_ >= 0) {
            second_ear = distance;
            if (!itd_delay_.get_delay(abs_itd, first_ear))
                return false;
        } else {
            first_ear = distance;
            if (!itd_delay_.get_delay(abs_itd, second_ear))
                return false;
        }
        return true;
    }
    for (float& sample : first_ear)
        sample *= distance_gain_;
    for (float& sample : second_ear)
        sample *= distance_gain_;
    return true;
}

bool HeadphoneExplicitSourcePath::process_with_pca_gains(
    const std::array<float, 32>& input,
    const std::vector<float>& first_ear_gains,
    const std::vector<float>& second_ear_gains,
    std::vector<std::array<float, 32>>& first_ear_outputs,
    std::vector<std::array<float, 32>>& second_ear_outputs) noexcept {
    if (first_ear_gains.empty()
        || first_ear_gains.size() != second_ear_gains.size()
        || first_ear_outputs.size() < first_ear_gains.size()
        || second_ear_outputs.size() < second_ear_gains.size())
        return false;
    std::array<float, 32> first{}, second{};
    if (!process(input, first, second))
        return false;
    for (std::size_t score = 0u; score < first_ear_gains.size(); ++score) {
        HeadphonePcaAccumulator::add(first, first_ear_gains[score],
                                     first_ear_outputs[score]);
        HeadphonePcaAccumulator::add(second, second_ear_gains[score],
                                     second_ear_outputs[score]);
    }
    return true;
}

bool HeadphoneExplicitSourcePath::process_with_pca_state(
    const std::array<float, 32>& input,
    const HeadphonePcaScoreState& scores,
    std::vector<std::array<float, 32>>& first_ear_outputs,
    std::vector<std::array<float, 32>>& second_ear_outputs) noexcept {
    std::array<float, 32> first{}, second{};
    if (!process(input, first, second))
        return false;
    return scores.accumulate(first, second, first_ear_outputs,
                             second_ear_outputs);
}

}
