#include "headphone_lfe_processing.hpp"

#include <cmath>

namespace auro3d {
namespace {

float rounded_product(float first, float second) noexcept {
    volatile float product = first * second;
    return product;
}

float rounded_sum(float first, float second) noexcept {
    volatile float sum = first + second;
    return sum;
}

}

void HeadphoneLfeProcessing::construct(float input_scale, bool apply_input_scale) noexcept {
    input_scale_ = input_scale;
    apply_input_scale_ = apply_input_scale;
    dynamic_gain_ = 0.0f;
}

void HeadphoneLfeProcessing::set_dynamic_gain(float gain) noexcept {
    dynamic_gain_ = gain;
}

void HeadphoneLfeProcessing::reset_audio_state() noexcept {
    // Native LFEProcessing_reset_audio_state is a weak no-op. The block has
    // no delay or filter history; retain configuration and dynamic gain.
}

bool HeadphoneLfeProcessing::process(const float* const* input_planes,
                                     std::size_t input_count,
                                     float* output_plane) const noexcept {
    if ((!input_planes && input_count != 0u) || !output_plane)
        return false;

    const float gain = apply_input_scale_
        ? dynamic_gain_ * input_scale_
        : dynamic_gain_;
    for (std::size_t sample = 0; sample < 32u; ++sample) {
        float accumulated = 0.0f;
        for (std::size_t channel = 0; channel < input_count; ++channel) {
            if (!input_planes[channel])
                return false;
            // Native LFEProcessing uses separate SIMD multiply and add
            // operations, not a fused multiply-add.
            accumulated = rounded_sum(
                accumulated,
                rounded_product(gain, input_planes[channel][sample]));
        }
        output_plane[sample] = accumulated;
    }
    return true;
}

}
