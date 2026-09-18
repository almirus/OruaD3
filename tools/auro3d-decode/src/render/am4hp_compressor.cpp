#include "am4hp_compressor.hpp"

#include <algorithm>
#include <cmath>

namespace auro3d {

bool Am4hpCompressor::construct(std::uint32_t sample_rate) noexcept {
    if (sample_rate == 0u)
        return false;
    sample_rate_ = sample_rate;
    update_coefficients();
    reset();
    constructed_ = true;
    return true;
}

void Am4hpCompressor::update_coefficients() noexcept {
    // Native live AM4HP capture: follower [attack, release, 0.368] and
    // linked compressor fields [scale=1.244515, threshold=0.7943282,
    // inverse_threshold=1.258925, exponent=0.98].
    constexpr float target = 0.368f;
    constexpr float attack_seconds = 0.01f;
    constexpr float release_seconds = 0.5f;
    constexpr float ratio = 50.0f;
    constexpr float threshold_db = -2.0f;
    // Native field +56 is the exact float32 word 0x3f9f4c41.
    constexpr float native_makeup_gain = 1.2445145845413208f;
    const float rate = static_cast<float>(sample_rate_);
    attack_old_ = std::exp(std::log(target) / (attack_seconds * rate));
    attack_in_ = 1.0f - attack_old_;
    release_old_ = std::exp(std::log(target) / (release_seconds * rate));
    release_in_ = 1.0f - release_old_;
    threshold_ = std::pow(10.0f, threshold_db * 0.05f);
    inverse_threshold_ = 1.0f / threshold_;
    exponent_ = 1.0f - 1.0f / ratio;
    makeup_gain_ = native_makeup_gain;
}

void Am4hpCompressor::reset() noexcept {
    envelope_state_ = 0.0f;
    previous_gain_ = 1.0f;
}

void Am4hpCompressor::follow(const Block& peak, Block& envelope) noexcept {
    float value = envelope_state_;
    for (std::size_t i = 0; i < peak.size(); ++i) {
        const float input = peak[i];
        if (input > value)
            value = attack_in_ * input + attack_old_ * value;
        else
            value = release_in_ * input + release_old_ * value;
        envelope[i] = value;
    }
    envelope_state_ = value;
}

void Am4hpCompressor::compute_gains(const Block& envelope, Block& gains) noexcept {
    float maximum = envelope[0];
    std::size_t peak_index = 0u;
    for (std::size_t i = 1; i < envelope.size(); ++i) {
        if (envelope[i] >= maximum) {
            maximum = envelope[i];
            peak_index = i;
        }
    }
    const auto gain_for = [this](float value) noexcept {
        if (!(value > threshold_))
            return 1.0f;
        return std::min(std::pow(value * inverse_threshold_, -exponent_), 1.0f);
    };
    if (maximum <= threshold_) {
        if (previous_gain_ >= 1.0f) {
            gains.fill(1.0f);
            previous_gain_ = 1.0f;
            return;
        }
        const float step = (1.0f - previous_gain_) / 32.0f;
        float value = previous_gain_;
        for (float& gain : gains) {
            value += step;
            gain = value;
        }
        previous_gain_ = gains.back();
        return;
    }
    const float target = gain_for(maximum);
    const float first_step = (target - previous_gain_) /
                             static_cast<float>(peak_index + 1u);
    float value = previous_gain_;
    for (std::size_t i = 0; i <= peak_index; ++i) {
        value += first_step;
        gains[i] = value;
    }
    if (peak_index + 1u < gains.size()) {
        const float tail_target = gain_for(envelope.back());
        const float tail_step = (tail_target - target) /
                                static_cast<float>(gains.size() - 1u - peak_index);
        value = target;
        for (std::size_t i = peak_index + 1u; i < gains.size(); ++i) {
            value += tail_step;
            gains[i] = value;
        }
    }
    previous_gain_ = gains.back();
}

bool Am4hpCompressor::process(Block& left, Block& right) noexcept {
    if (!constructed_)
        return false;
    Block peaks{};
    for (std::size_t i = 0; i < peaks.size(); ++i)
        peaks[i] = std::max(std::fabs(left[i]), std::fabs(right[i]));
    Block envelope{};
    follow(peaks, envelope);
    Block gains{};
    compute_gains(envelope, gains);
    for (std::size_t i = 0; i < 32; ++i) {
        // Native performs two sequential float32 multiplies:
        // (input * reduction_gain) * makeup_gain. Keep that order instead
        // of folding the gains, which changes the final rounding.
        left[i] = (left[i] * gains[i]) * makeup_gain_;
        right[i] = (right[i] * gains[i]) * makeup_gain_;
    }
    return true;
}

} // namespace auro3d
