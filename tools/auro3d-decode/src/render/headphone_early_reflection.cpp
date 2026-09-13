#include "headphone_early_reflection.hpp"

#include "headphone_pca_accumulator.hpp"

#include <cmath>
#include <cstdint>

namespace auro3d {

bool HeadphoneEarlyReflectionPath::construct(
    std::size_t distance_delay_samples, int itd_delay_samples,
    float distance_gain, std::size_t diffusion_delay_samples) {
    if (!std::isfinite(distance_gain)
        || !distance_delay_.construct(distance_delay_samples)
        || !itd_delay_.construct(static_cast<std::size_t>(
               itd_delay_samples < 0 ? -static_cast<std::int64_t>(itd_delay_samples)
                                      : itd_delay_samples)))
        return false;
    if (diffusion_delay_samples != 0u) {
        if (!diffusion_.construct(diffusion_delay_samples))
            return false;
        has_diffusion_ = true;
    } else {
        has_diffusion_ = false;
    }
    itd_delay_samples_ = itd_delay_samples;
    constructed_distance_gain_ = distance_gain;
    distance_gain_ = distance_gain;
    reset_audio_state();
    return true;
}

bool HeadphoneEarlyReflectionPath::set_distance_gain_scale(
    float scale) noexcept {
    if (!std::isfinite(scale))
        return false;
    distance_gain_ = constructed_distance_gain_ * scale;
    return true;
}

bool HeadphoneEarlyReflectionPath::set_gain_reference_linear(
    float reference_linear) noexcept {
    if (!std::isfinite(reference_linear) || !(reference_linear > 0.0f))
        return false;
    reference_linear_ = reference_linear;
    return true;
}

bool HeadphoneEarlyReflectionPath::set_dynamic_geometry_gain(
    float geometry_gain) noexcept {
    if (!std::isfinite(geometry_gain) || !(geometry_gain > 0.0f))
        return false;
    dynamic_geometry_gain_ = geometry_gain;
    has_dynamic_geometry_gain_ = true;
    return true;
}

bool HeadphoneEarlyReflectionPath::set_distance_gain_from_db(
    float gain_db) noexcept {
    if (!std::isfinite(gain_db) || !(reference_linear_ > 0.0f))
        return false;
    const float linear = gain_db > -144.0f
        ? std::pow(10.0f, gain_db * 0.050000001f) : 0.0f;
    distance_gain_ = has_dynamic_geometry_gain_
        ? dynamic_geometry_gain_ * linear
        : constructed_distance_gain_ / reference_linear_ * linear;
    return true;
}

bool HeadphoneEarlyReflectionPath::set_diffusion_rt60(
    unsigned sample_rate, float rt60_seconds) noexcept {
    if (!has_diffusion_ || sample_rate == 0u || !(rt60_seconds > 0.0f)
        || !std::isfinite(rt60_seconds))
        return false;
    diffusion_.set_rt60(sample_rate, rt60_seconds);
    return true;
}

bool HeadphoneEarlyReflectionPath::set_diffusion_coefficient(
    float coefficient) noexcept {
    return has_diffusion_ && diffusion_.set_coefficient(coefficient);
}

void HeadphoneEarlyReflectionPath::reset_audio_state() noexcept {
    distance_delay_.reset_audio_state();
    itd_delay_.reset_audio_state();
    if (has_diffusion_)
        diffusion_.reset_audio_state();
}

bool HeadphoneEarlyReflectionPath::process(
    const std::array<float, 32>& input,
    std::array<float, 32>& first_ear,
    std::array<float, 32>& second_ear) noexcept {
    std::array<float, 32> block{};
    distance_delay_.add_buffer(input);
    if (!distance_delay_.get_delay(distance_delay_.configured_delay(), block)) {
        first_ear.fill(0.0f);
        second_ear.fill(0.0f);
        return false;
    }
    for (float& sample : block)
        sample *= distance_gain_;
    if (has_diffusion_ && !diffusion_.process_in_place(block)) {
        first_ear.fill(0.0f);
        second_ear.fill(0.0f);
        return false;
    }

    // Native source_ER_itd_delay adds the block first, then reads either the
    // signed positive or signed negative branch from the same ring.
    itd_delay_.add_buffer(block);
    if (itd_delay_samples_ < 0) {
        first_ear = block;
        if (!itd_delay_.get_delay(static_cast<std::size_t>(
                -static_cast<std::int64_t>(itd_delay_samples_)), second_ear)) {
            first_ear.fill(0.0f);
            second_ear.fill(0.0f);
            return false;
        }
    } else {
        second_ear = block;
        if (!itd_delay_.get_delay(static_cast<std::size_t>(itd_delay_samples_),
                                  first_ear)) {
            first_ear.fill(0.0f);
            second_ear.fill(0.0f);
            return false;
        }
    }
    return true;
}

bool HeadphoneEarlyReflectionPath::process_from_shared_delay(
    const HeadphoneMultiDelay& input_delay,
    std::array<float, 32>& first_ear,
    std::array<float, 32>& second_ear) const noexcept {
    if (has_diffusion_)
        return false;
    const std::size_t distance = distance_delay_.configured_delay();
    const std::size_t itd = static_cast<std::size_t>(
        itd_delay_samples_ < 0
            ? -static_cast<std::int64_t>(itd_delay_samples_)
            : itd_delay_samples_);
    std::array<float, 32> first{}, second{};
    if (itd_delay_samples_ < 0) {
        if (!input_delay.get_delay(distance, first)
            || !input_delay.get_delay(distance + itd, second))
            return false;
    } else {
        if (!input_delay.get_delay(distance + itd, first)
            || !input_delay.get_delay(distance, second))
            return false;
    }
    debug_pre_gain_first_ = first;
    debug_pre_gain_second_ = second;
    for (std::size_t sample = 0u; sample < first.size(); ++sample) {
        first_ear[sample] = first[sample] * distance_gain_;
        second_ear[sample] = second[sample] * distance_gain_;
    }
    return true;
}

bool HeadphoneEarlyReflectionPath::process_with_pca_gains(
    const std::array<float, 32>& input,
    const std::vector<float>& first_ear_gains,
    const std::vector<float>& second_ear_gains,
    std::vector<std::array<float, 32>>& first_ear_outputs,
    std::vector<std::array<float, 32>>& second_ear_outputs) noexcept {
    if (first_ear_gains.empty()
        || first_ear_gains.size() != second_ear_gains.size()
        || first_ear_outputs.size() < first_ear_gains.size()
        || second_ear_outputs.size() < first_ear_gains.size())
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

bool HeadphoneEarlyReflectionPath::process_with_pca_state(
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
