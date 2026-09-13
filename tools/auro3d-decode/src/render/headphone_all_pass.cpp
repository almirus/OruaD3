#include "headphone_all_pass.hpp"

#include <cmath>

namespace auro3d {

bool HeadphoneAllPass::construct(std::size_t delay_samples) {
    if (delay_samples < 32u || !delay_.construct(delay_samples))
        return false;
    delay_samples_ = delay_samples;
    coefficient_ = 0.0f;
    return true;
}

float HeadphoneAllPass::set_rt60(unsigned sample_rate, float rt60_seconds) noexcept {
    coefficient_ = static_cast<float>(std::pow(
        10.0, (static_cast<double>(delay_samples_) * -3.0)
            / (static_cast<double>(sample_rate) * rt60_seconds)));
    return coefficient_;
}

bool HeadphoneAllPass::set_coefficient(float coefficient) noexcept {
    if (!std::isfinite(coefficient)) return false;
    coefficient_ = coefficient;
    return true;
}

void HeadphoneAllPass::reset_audio_state() noexcept {
    delay_.reset_audio_state();
}

bool HeadphoneAllPass::process(const std::array<float, 32>& input,
                               std::array<float, 32>& output) noexcept {
    if (delay_samples_ < 32u) {
        output.fill(0.0f);
        return false;
    }
    std::array<float, 32> delayed{};
    delay_.get_delay(delay_samples_ - 32u, delayed);
    std::array<float, 32> feedback{};
    for (std::size_t i = 0; i < 32u; ++i) {
        volatile float product = delayed[i] * coefficient_;
        feedback[i] = input[i] + product;
        volatile float feedback_product = feedback[i] * coefficient_;
        output[i] = delayed[i] - feedback_product;
    }
    delay_.add_buffer(feedback);
    for (std::size_t i = 0; i < 32u; ++i)
        output[i] = delayed[i] - feedback[i] * coefficient_;
    return true;
}

bool HeadphoneAllPass::process_in_place(std::array<float, 32>& block) noexcept {
    std::array<float, 32> output{};
    if (!process(block, output))
        return false;
    block = output;
    return true;
}


}
