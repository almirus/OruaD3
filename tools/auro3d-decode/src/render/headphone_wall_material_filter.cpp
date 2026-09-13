#include "headphone_wall_material_filter.hpp"

#include <cmath>

namespace auro3d {

void HeadphoneWallMaterialFilter::construct(
    const std::array<float, 5>& coefficients) noexcept {
    coefficients_ = coefficients;
    state0_ = 0.0f;
    state1_ = 0.0f;
}

void HeadphoneWallMaterialFilter::set_coefficients(
    const std::array<float, 5>& coefficients) noexcept {
    coefficients_ = coefficients;
}

bool HeadphoneWallMaterialFilter::construct_from_wall_material(
    float high_pass_hz, float low_pass_hz, unsigned sample_rate) noexcept {
    if (!(high_pass_hz > 0.0f) || !(low_pass_hz > high_pass_hz) || sample_rate == 0u)
        return false;

    // Native float operations from WallMaterial_t_construct (0x502300).
    const float log_ratio = std::log(low_pass_hz / high_pass_hz);
    const float octaves = log_ratio / 0.69315f;
    const float ratio = std::exp2(octaves);
    const float center_hz = std::sqrt(ratio) * (low_pass_hz - high_pass_hz) /
                            (ratio - 1.0f);
    const float omega = center_hz * 6.28318531f /
                        static_cast<float>(sample_rate);
    const float bandwidth_term = (octaves * 0.34657f) * omega;
    const float sine = std::sin(omega);
    if (!std::isfinite(sine) || sine == 0.0f)
        return false;
    const float q = 1.0f / (2.0f * std::sinh(bandwidth_term / sine));
    if (!std::isfinite(q))
        return false;

    // WallMaterial passes this already-normalized omega to the native IIR
    // constructor. The public float32 bridge accepts Hz and normalizes again,
    // so applying the recovered case-3 equation here preserves that ABI.
    const float alpha = sine / (2.0f * q);
    const float inverse = 1.0f / (1.0f + alpha);
    const std::array<float, 5> coefficients{
        alpha * inverse,
        0.0f,
        -alpha * inverse,
        -2.0f * std::cos(omega) * inverse,
        (1.0f - alpha) * inverse};
    construct(coefficients);
    return true;
}

void HeadphoneWallMaterialFilter::reset_audio_state() noexcept {
    state0_ = 0.0f;
    state1_ = 0.0f;
}

void HeadphoneWallMaterialFilter::process(const std::array<float, 32>& input,
                                          std::array<float, 32>& output) noexcept {
    const float b0 = coefficients_[0];
    const float b1 = coefficients_[1];
    const float b2 = coefficients_[2];
    const float a1 = coefficients_[3];
    const float a2 = coefficients_[4];
    for (std::size_t i = 0; i < input.size(); ++i) {
        const float x = input[i];
        const float y = state0_ + b0 * x;
        const float next_state0 = state1_ + b1 * x - a1 * y;
        const float next_state1 = -a2 * y + b2 * x;
        output[i] = y;
        state0_ = next_state0;
        state1_ = next_state1;
    }
}

}
