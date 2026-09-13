#include "headphone_late_reverb_band.hpp"

#include <cmath>

namespace auro3d {

bool HeadphoneLateReverbBand::construct(
    std::size_t delay_samples,
    const std::vector<std::size_t>& allpass_delays,
    const std::array<float, 5>& damping_coefficients) {
    if (!delay_.construct(delay_samples))
        return false;

    std::vector<HeadphoneAllPass> allpasses;
    allpasses.reserve(allpass_delays.size());
    for (const std::size_t allpass_delay : allpass_delays) {
        HeadphoneAllPass allpass;
        if (!allpass.construct(allpass_delay))
            return false;
        allpasses.push_back(std::move(allpass));
    }
    allpasses_ = std::move(allpasses);
    damping_.construct(damping_coefficients);
    reset_audio_state();
    return true;
}

bool HeadphoneLateReverbBand::set_allpass_rt60(
    unsigned sample_rate, float rt60_seconds) noexcept {
    if (sample_rate == 0u || !(rt60_seconds > 0.0f)
        || !std::isfinite(rt60_seconds))
        return false;
    for (HeadphoneAllPass& allpass : allpasses_)
        allpass.set_rt60(sample_rate, rt60_seconds);
    return true;
}

bool HeadphoneLateReverbBand::set_allpass_coefficients(
    const std::vector<float>& coefficients) noexcept {
    if (coefficients.size() != allpasses_.size())
        return false;
    for (std::size_t i = 0; i < allpasses_.size(); ++i) {
        if (!allpasses_[i].set_coefficient(coefficients[i]))
            return false;
    }
    return true;
}

void HeadphoneLateReverbBand::set_damping_coefficients(
    const std::array<float, 5>& coefficients) noexcept {
    damping_.set_coefficients(coefficients);
}

void HeadphoneLateReverbBand::reset_audio_state() noexcept {
    delay_.reset_audio_state();
    for (HeadphoneAllPass& allpass : allpasses_)
        allpass.reset_audio_state();
    damping_.reset_audio_state();
}

bool HeadphoneLateReverbBand::process(
    const std::array<float, 32>& input,
    std::array<float, 32>& output) noexcept {
    output = input;
    for (HeadphoneAllPass& allpass : allpasses_) {
        if (!allpass.process_in_place(output))
            return false;
    }
    damping_.process(output, output);
    delay_.add_buffer(output);
    return true;
}

bool HeadphoneLateReverbBand::mix_delayed_feedback(
    float gain, std::array<float, 32>& accumulator,
    std::array<float, 32>* delayed_debug) noexcept {
    if (!std::isfinite(gain))
        return false;
    std::array<float, 32> delayed{};
    if (!delay_.get_delay_before_add(
            static_cast<int>(delay_.configured_delay()), delayed))
        return false;
    if (delayed_debug)
        *delayed_debug = delayed;
    for (std::size_t sample = 0u; sample < delayed.size(); ++sample) {
        volatile float product = delayed[sample] * gain;
        accumulator[sample] += product;
    }
    return true;
}

}
