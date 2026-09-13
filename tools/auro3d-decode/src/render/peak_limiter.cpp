#include "peak_limiter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace auro3d {
namespace {

// PeakFollower_float32_t_construct passes 0x3EBC6A7F as the envelope target.
// Follower_update evaluates log/exp in float64, with an explicit float32
// rounding after log and before exp (libauro 0x599EED..0x599F34).
constexpr float kReleaseSeconds = 0.15f;
constexpr float kRatio = 50.0f;
constexpr float kThresholdDb = -0.5f;

float follower_target() {
    float value = 0.0f;
    const std::uint32_t bits = 0x3EBC6A7Fu;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

} // namespace

void NativePeakLimiter::reset() {
    envelope_ = 0.0f;
    prev_gain_ = 1.0f;
}

void NativePeakLimiter::set_sample_rate(std::uint32_t sample_rate) {
    if (sample_rate_ == sample_rate && sample_rate_ != 0u)
        return;
    sample_rate_ = sample_rate;
    ensure_coefficients();
}

void NativePeakLimiter::ensure_coefficients() {
    const float rate = sample_rate_ != 0u ? static_cast<float>(sample_rate_) : 48000.0f;
    const float target = follower_target();
    // attack=0 → attack coefficient stays 0, so env snaps to peak.
    attack_old_ = 0.0f;
    attack_in_ = 1.0f;
    const float log_target = static_cast<float>(
        std::log(static_cast<double>(target)));
    const float denominator = kReleaseSeconds * rate;
    const float release_exponent = log_target / denominator;
    const float release_old = static_cast<float>(
        std::exp(static_cast<double>(release_exponent)));
    release_old_ = release_old;
    release_in_ = 1.0f - release_old;
    threshold_ = std::pow(10.0f, kThresholdDb * 0.05f);
    inv_threshold_ = 1.0f / threshold_;
    exponent_ = 1.0f - 1.0f / kRatio;
}

void NativePeakLimiter::follow_block(const float* peak, float* envelope, std::size_t count) {
    float env = envelope_;
    for (std::size_t i = 0; i < count; ++i) {
        const float x = peak[i];
        if (x > env)
            env = attack_in_ * x + attack_old_ * env;
        else
            env = release_in_ * x + release_old_ * env;
        envelope[i] = env;
    }
    envelope_ = env;
}

void NativePeakLimiter::compute_gains(const float* envelope, float* gains, std::size_t count) {
    if (count == 0u)
        return;
    float max_env = envelope[0];
    std::size_t peak_i = 0;
    for (std::size_t i = 1; i < count; ++i) {
        if (envelope[i] >= max_env) {
            max_env = envelope[i];
            peak_i = i;
        }
    }

    auto gain_from_env = [this](float env) {
        if (!(env > threshold_))
            return 1.0f;
        const float scaled = env * inv_threshold_;
        if (scaled == 0.0f && exponent_ > 0.0f)
            return 0.0f;
        return std::fmin(std::pow(scaled, -exponent_), 1.0f);
    };

    if (max_env <= threshold_) {
        if (prev_gain_ >= 1.0f) {
            for (std::size_t i = 0; i < count; ++i)
                gains[i] = 1.0f;
            prev_gain_ = 1.0f;
            return;
        }
        const float step = (1.0f - prev_gain_) / static_cast<float>(count);
        float g = prev_gain_;
        for (std::size_t i = 0; i < count; ++i) {
            g += step;
            gains[i] = g;
        }
        prev_gain_ = gains[count - 1u];
        return;
    }

    const float target = gain_from_env(max_env);
    const float step = (target - prev_gain_) / static_cast<float>(peak_i + 1u);
    float g = prev_gain_;
    for (std::size_t i = 0; i <= peak_i; ++i) {
        g += step;
        gains[i] = g;
    }
    if (peak_i + 1u < count) {
        const float end_gain = gain_from_env(envelope[count - 1u]);
        const float tail = (end_gain - target) / static_cast<float>(count - 1u - peak_i);
        // Native continues from the cumulatively rounded gain at peak_i;
        // it does not reload the ideal `target` before the tail ramp
        // (libauro 0x598600 reads gains[index - 1]).
        for (std::size_t i = peak_i + 1u; i < count; ++i) {
            g += tail;
            gains[i] = g;
        }
    }
    prev_gain_ = gains[count - 1u];
}

void NativePeakLimiter::limit_gains_to_pcm24_ceiling(
    const float* peak, float* gains, std::size_t count) {
    // Native HDMI is Float32, so ratio-50 may leave |y| > 1. Integer WAV
    // convertor cannot store that; scale the 32-sample block instead of
    // hard-clipping PCM24 rails.
    if (count == 0u)
        return;
    float out_peak = 0.0f;
    for (std::size_t i = 0; i < count; ++i)
        out_peak = std::max(out_peak, peak[i] * std::fabs(gains[i]));
    constexpr float kCeil = 8388607.0f / 8388608.0f;
    if (!(out_peak > kCeil))
        return;
    const float scale = kCeil / out_peak;
    for (std::size_t i = 0; i < count; ++i)
        gains[i] *= scale;
    prev_gain_ = gains[count - 1u];
}

void NativePeakLimiter::process_count(
    float* const* channels,
    std::uint32_t channel_count,
    std::size_t frames,
    bool apply_pcm24_ceiling) {
    if (frames == 0u || channel_count == 0u)
        return;
    ensure_coefficients();
    float peak[kBlock];
    float envelope[kBlock];
    float gains[kBlock];
    std::size_t offset = 0;
    while (offset < frames) {
        const std::size_t n = std::min(kBlock, frames - offset);
        for (std::size_t i = 0; i < n; ++i) {
            float m = 0.0f;
            for (std::uint32_t ch = 0; ch < channel_count; ++ch) {
                if (channels[ch] == nullptr)
                    continue;
                m = std::max(m, std::fabs(channels[ch][offset + i]));
            }
            peak[i] = m;
        }
        follow_block(peak, envelope, n);
        compute_gains(envelope, gains, n);
        if (apply_pcm24_ceiling)
            limit_gains_to_pcm24_ceiling(peak, gains, n);
        for (std::uint32_t ch = 0; ch < channel_count; ++ch) {
            if (channels[ch] == nullptr)
                continue;
            float* lane = channels[ch] + offset;
            for (std::size_t i = 0; i < n; ++i)
                lane[i] *= gains[i];
        }
        offset += n;
    }
}

void NativePeakLimiter::process_planar(
    float* const* channels,
    std::uint32_t channel_count,
    std::size_t frames,
    bool apply_pcm24_ceiling) {
    process_count(channels, channel_count, frames, apply_pcm24_ceiling);
}

void NativePeakLimiter::process_stereo(double* left, double* right, std::size_t frames) {
    if (frames == 0u || left == nullptr || right == nullptr)
        return;
    ensure_coefficients();
    float peak[kBlock];
    float envelope[kBlock];
    float gains[kBlock];
    double peak_d[kBlock];
    constexpr double kPosCeil = 8388607.0 / 8388608.0;
    std::size_t offset = 0;
    while (offset < frames) {
        const std::size_t n = std::min(kBlock, frames - offset);
        for (std::size_t i = 0; i < n; ++i) {
            peak_d[i] = std::max(std::fabs(left[offset + i]), std::fabs(right[offset + i]));
            peak[i] = static_cast<float>(peak_d[i]);
        }
        follow_block(peak, envelope, n);
        compute_gains(envelope, gains, n);
        double out_peak = 0.0;
        for (std::size_t i = 0; i < n; ++i)
            out_peak = std::max(out_peak, peak_d[i] * std::fabs(static_cast<double>(gains[i])));
        if (out_peak > kPosCeil) {
            const double scale = kPosCeil / out_peak;
            for (std::size_t i = 0; i < n; ++i)
                gains[i] = static_cast<float>(static_cast<double>(gains[i]) * scale);
            prev_gain_ = gains[n - 1u];
        }
        for (std::size_t i = 0; i < n; ++i) {
            const double g = static_cast<double>(gains[i]);
            left[offset + i] = std::clamp(left[offset + i] * g, -1.0, kPosCeil);
            right[offset + i] = std::clamp(right[offset + i] * g, -1.0, kPosCeil);
        }
        offset += n;
    }
}

} // namespace auro3d
