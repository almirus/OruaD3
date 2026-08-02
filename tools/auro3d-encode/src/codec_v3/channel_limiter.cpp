#include "channel_limiter.hpp"

#include <cmath>
#include <cstring>
#include <limits>

namespace auro3d::encode {
namespace {

constexpr float kNativeMaxGain = 0.9885531067848206f;

float release_smooth(const ChannelLimiter& limiter, float previous, float target) {
    // get_next_dmx_coeff_with_release @ 0x506C30
    if (previous <= target) {
        const float released = limiter.release * previous;
        if (target <= released)
            return limiter.inv_release > previous ? previous : 1.0f;
        return released;
    }
    return target;
}

void distribute_gain_3(
    ChannelLimiter& limiter,
    float& out0,
    float& out1,
    float& out2,
    float mag0,
    float mag1,
    float mag2) {
    // distribute_gain_3_ @ 0x506F90
    const float max_gain = limiter.max_gain;
    float scale2 = 1.0f;
    float scale1 = mag1 > max_gain ? max_gain / mag1 : 1.0f;
    const float dist = limiter.distribution_factor;
    const float p0 = limiter.peak0;
    const float p1 = limiter.peak1;
    const float p2 = limiter.peak2;
    limiter.peak_floor = std::fmax(((p0 + p1 + p2) - 1.0f) * 0.5f, limiter.peak_floor);
    const float floor = limiter.peak_floor;

    float scale0 = 1.0f;
    if (mag0 > max_gain)
        scale0 = max_gain / mag0;
    const float max01 = std::fmax(p1, p0);
    if (mag2 > max_gain)
        scale2 = max_gain / mag2;
    const float max012 = std::fmax(p2, max01);

    float w0 = 0.0f;
    float w1 = 0.0f;
    float w2 = 0.0f;
    if (max012 == p0) {
        const float one_minus = 1.0f - dist;
        w0 = (dist * (1.0f - floor)) + floor;
        w1 = (one_minus * (1.0f - floor)) + floor;
        w2 = w0;
    } else {
        const bool peak1_wins = max012 == p1;
        const float w_alt = ((1.0f - dist) * (1.0f - floor)) + floor;
        const float w_dist = ((1.0f - floor) * dist) + floor;
        w0 = w_alt;
        w2 = w_dist;
        w1 = w_alt;
        if (peak1_wins) {
            w2 = w_alt;
            w1 = w_dist;
        }
    }

    const float denom = (mag2 * w2) + ((mag0 * w0) + (w1 * mag1));
    const float ratio =
        (((max_gain - (mag0 * (1.0f - w0))) - (mag1 * (1.0f - w1)))
            - (mag2 * (1.0f - w2)))
        / denom;
    out0 = ((w0 * ratio) + (1.0f - w0)) * scale0;
    out1 = ((w1 * ratio) + (1.0f - w1)) * scale1;
    out2 = ((ratio * w2) + (1.0f - w2)) * scale2;
    const float total = (out2 * mag2) + ((mag0 * out0) + (mag1 * out1));
    if (total > max_gain) {
        const float shrink = max_gain / total;
        out0 *= shrink;
        out1 *= shrink;
        out2 *= shrink;
    }
}

} // namespace

void channel_limiter_init(
    ChannelLimiter& limiter,
    std::uint32_t sample_count,
    std::int32_t sample_rate) {
    limiter = {};
    limiter.sample_count = sample_count;
    limiter.sample_rate = sample_rate;
    limiter.distribution_factor = 0.5f;
    limiter.max_gain = kNativeMaxGain;
    limiter.smoothed0 = 1.0f;
    limiter.smoothed1 = 1.0f;
    limiter.smoothed2 = 1.0f;
    std::int32_t periods = 1;
    if (sample_count != 0u
        && static_cast<std::int32_t>(static_cast<float>(sample_rate))
        >= static_cast<std::int32_t>(sample_count)) {
        periods = static_cast<std::int32_t>(
            static_cast<float>(sample_rate) / static_cast<float>(sample_count));
    }
    const float release_db = std::fmin(0.1f, 3.0f / static_cast<float>(periods));
    float release = 0.0f;
    if (release_db > -144.0f)
        release = std::pow(10.0f, release_db * 0.05f);
    limiter.release = release;
    limiter.inv_release =
        release == 0.0f ? std::numeric_limits<float>::infinity() : 1.0f / release;
}

bool channel_limiter_set_distribution_factor(ChannelLimiter& limiter, float factor) {
    if (!(factor > 0.0f && factor < 1.0f))
        return false;
    limiter.distribution_factor = factor;
    return true;
}

bool channel_limiter_set_release(ChannelLimiter& limiter, float seconds) {
    if (seconds < 0.0f
        || limiter.sample_count == 0u
        || limiter.sample_rate <= 0) {
        return false;
    }
    const float scaled = static_cast<float>(limiter.sample_rate) * seconds;
    std::int32_t periods = 1;
    if (limiter.sample_count
        <= static_cast<std::uint32_t>(static_cast<std::int32_t>(scaled))) {
        periods = static_cast<std::int32_t>(scaled)
            / static_cast<std::int32_t>(limiter.sample_count);
    }
    const float release_db = std::fmin(0.1f, 3.0f / static_cast<float>(periods));
    float release = 0.0f;
    if (release_db > -144.0f)
        release = std::pow(10.0f, release_db * 0.05f);
    limiter.release = release;
    limiter.inv_release =
        release == 0.0f ? std::numeric_limits<float>::infinity() : 1.0f / release;
    return true;
}

bool channel_limiter_set_max_gain_db(ChannelLimiter& limiter, float gain_db) {
    if (gain_db > 0.0f)
        return false;
    float linear = 0.0f;
    if (gain_db > -144.0f)
        linear = std::pow(10.0f, 0.05f * gain_db);
    limiter.max_gain = linear;
    return true;
}

bool channel_limiter_dmx_limit_coeff2(
    ChannelLimiter& limiter,
    float& gain0,
    float& gain1,
    const float* samples0,
    const float* samples1,
    std::string& error) {
    error.clear();
    if (samples0 == nullptr || samples1 == nullptr) {
        error = "channel limiter mix2 samples are null";
        return false;
    }
    if (gain0 > 1.0f || gain1 > 1.0f) {
        error = "channel limiter mix2 gain exceeds native unity limit";
        return false;
    }

    // dmx_limit_coeff (2-source) @ 0x506880
    float common_scale = 1.0f;
    const float dist = limiter.distribution_factor;
    if (std::fabs(dist + -0.5f) < 0.001f && limiter.sample_count != 0u) {
        float peak = 0.0f;
        std::uint32_t peak_index = 0u;
        std::uint32_t index = 0u;
        const std::uint32_t count = limiter.sample_count;
        const std::uint32_t even = count & ~1u;
        while (index != even) {
            const float a = std::fabs(
                (samples0[index] * gain0) + (samples1[index] * gain1));
            std::uint32_t best = peak_index;
            if (a > peak)
                best = index;
            float next_peak = std::fmax(a, peak);
            const float b = std::fabs(
                (samples0[index + 1u] * gain0) + (samples1[index + 1u] * gain1));
            peak_index = index + 1u;
            if (b <= next_peak)
                peak_index = best;
            peak = std::fmax(b, next_peak);
            index += 2u;
        }
        if ((count & 1u) != 0u) {
            const float a = std::fabs(
                (samples0[index] * gain0) + (samples1[index] * gain1));
            if (a > peak)
                peak_index = index;
            peak = std::fmax(a, peak);
        }
        if (peak > limiter.max_gain) {
            const float denom = std::fabs(gain1 * samples1[peak_index])
                + std::fabs(gain0 * samples0[peak_index]);
            common_scale = limiter.max_gain / denom;
        }
    }

    float scale0 = common_scale;
    float scale1 = common_scale;
    if (limiter.sample_count != 0u) {
        const float max_gain = limiter.max_gain;
        for (std::uint32_t i = 0; i < limiter.sample_count; ++i) {
            const float c0 = samples0[i] * gain0;
            const float c1 = samples1[i] * gain1;
            if (std::fabs(c1 + c0) > max_gain) {
                limiter.peak0 = std::fmax(std::fabs(samples0[i]), limiter.peak0);
                limiter.peak1 = std::fmax(std::fabs(samples1[i]), limiter.peak1);
                const float weight0 =
                    limiter.peak1 < limiter.peak0 ? (1.0f - dist) : dist;
                const float weight1 = 1.0f - weight0;
                const float abs0 = std::fabs(c0);
                const float abs1 = std::fabs(c1);
                const float clamp0 = max_gain < abs0 ? max_gain / abs0 : 1.0f;
                const float clamp1 = max_gain < abs1 ? max_gain / abs1 : 1.0f;
                const float clamped0 = abs0 * clamp0;
                const float clamped1 = abs1 * clamp1;
                const float ratio =
                    (max_gain - (clamped0 * weight0) - (clamped1 * weight1))
                    / ((clamped0 * weight1) + (weight0 * clamped1));
                float s0 = ((weight1 * ratio) + weight0) * clamp0;
                float s1 = ((weight0 * ratio) + weight1) * clamp1;
                const float check = (abs0 * s0) + (abs1 * s1);
                if (check > max_gain) {
                    const float shrink = max_gain / check;
                    s0 *= shrink;
                    s1 *= shrink;
                }
                scale0 = std::fmin(s0, scale0);
                scale1 = std::fmin(s1, scale1);
            }
            if (scale0 > 0.9998849f && scale1 > 0.9998849f) {
                // Native decays the packed peak0/peak1 pair together.
                limiter.peak0 *= 0.999f;
                limiter.peak1 *= 0.999f;
            }
        }
    }

    limiter.smoothed0 = release_smooth(limiter, limiter.smoothed0, scale0);
    limiter.smoothed1 = release_smooth(limiter, limiter.smoothed1, scale1);
    gain0 *= limiter.smoothed0;
    gain1 *= limiter.smoothed1;
    return true;
}

bool channel_limiter_dmx_limit_coeff3(
    ChannelLimiter& limiter,
    float& gain0,
    float& gain1,
    float& gain2,
    const float* samples0,
    const float* samples1,
    const float* samples2,
    std::string& error) {
    error.clear();
    if (samples0 == nullptr || samples1 == nullptr || samples2 == nullptr) {
        error = "channel limiter mix3 samples are null";
        return false;
    }
    if (gain0 > 1.0f || gain1 > 1.0f || gain2 > 1.0f) {
        error = "channel limiter mix3 gain exceeds native unity limit";
        return false;
    }

    float scale0 = 1.0f;
    float scale1 = 1.0f;
    float scale2 = 1.0f;
    if (limiter.sample_count != 0u) {
        for (std::uint32_t i = 0; i < limiter.sample_count; ++i) {
            const float c0 = samples0[i] * gain0;
            const float c1 = samples1[i] * gain1;
            const float c2 = samples2[i] * gain2;
            const float mag0 = std::fabs(c0);
            const float mag1 = std::fabs(c1);
            const float mag2 = std::fabs(c2);
            limiter.peak0 = std::fmax(mag0, limiter.peak0);
            limiter.peak1 = std::fmax(mag1, limiter.peak1);
            limiter.peak2 = std::fmax(mag2, limiter.peak2);
            if (std::fabs((c0 + c1) + c2) > limiter.max_gain) {
                if (limiter.peak0 > mag0)
                    limiter.peak0 =
                        std::fmax(std::fabs(samples0[i]), limiter.peak0);
                if (limiter.peak1 > mag1)
                    limiter.peak1 =
                        std::fmax(std::fabs(samples1[i]), limiter.peak1);
                if (limiter.peak2 > mag2)
                    limiter.peak2 =
                        std::fmax(std::fabs(samples2[i]), limiter.peak2);
                float d0 = 0.0f;
                float d1 = 0.0f;
                float d2 = 0.0f;
                distribute_gain_3(limiter, d0, d1, d2, mag0, mag1, mag2);
                scale2 = std::fmin(d2, scale2);
                scale1 = std::fmin(d1, scale1);
                scale0 = std::fmin(d0, scale0);
            }
            if (scale0 > 0.9998849f && scale1 > 0.9998849f && scale2 > 0.9998849f) {
                limiter.peak0 *= 0.999f;
                limiter.peak1 *= 0.999f;
                limiter.peak2 *= 0.999f;
            }
        }
    }

    limiter.smoothed0 = release_smooth(limiter, limiter.smoothed0, scale0);
    limiter.smoothed1 = release_smooth(limiter, limiter.smoothed1, scale1);
    limiter.smoothed2 = release_smooth(limiter, limiter.smoothed2, scale2);
    gain0 *= limiter.smoothed0;
    gain1 *= limiter.smoothed1;
    gain2 *= limiter.smoothed2;
    return true;
}

} // namespace auro3d::encode
