#include "cts_dmx.hpp"

#include "layout.hpp"
#include "original_channels.hpp"
#include "scaler.hpp"

#include <cmath>
#include <limits>

namespace auro3d::encode {
namespace {

float db_to_linear_limit(float gain_db) {
    // limit_new: absent → 1.0; present and <= -144 → 0.0; else 10^(dB/20).
    if (!(gain_db > -144.0f))
        return 0.0f;
    return std::pow(10.0f, gain_db * 0.05f);
}

float linear_to_db_limit(float linear) {
    if (!(linear > 0.0f))
        return -144.0f;
    return std::fmax(std::log10(linear) * 20.0f, -144.0f);
}

float gain_to_scaler_packed(float gain_db) {
    // auro::scaler::gain_to_scaler @ 0x53D830 with the packed form used by
    // cts_dmx_coeff_limit_: high dword = gain bits, low dword = (gain <= -inf).
    if (gain_db <= -std::numeric_limits<float>::infinity()) {
        const float v1 = -std::numeric_limits<float>::infinity();
        if (v1 >= 144.0f)
            return 0.0f;
        return std::pow(10.0f, v1 * -0.05f);
    }
    float scaler = 0.0f;
    if (!scaler_from_gain_db(gain_db, scaler))
        return 0.0f;
    return scaler;
}

void write_limited_gain(
    std::array<CtsGainEntry, kCodecV3ChannelCount>& out,
    std::uint32_t channel,
    float linear_gain) {
    float db = linear_to_db_limit(linear_gain);
    if (out[channel].present)
        db = std::fmin(db, out[channel].gain_db);
    out[channel].gain_db = db;
    out[channel].present = true;
}

} // namespace

bool cts_dmx_limiter_init(
    CtsDmxLimiter& limiter,
    std::uint32_t original_layout,
    std::uint32_t sample_count,
    std::int32_t sample_rate,
    std::string& error) {
    error.clear();
    limiter = {};
    if (sample_count == 0u || sample_rate <= 0) {
        error = "cts dmx limiter: invalid sample geometry";
        return false;
    }
    limiter.original_layout = original_layout;
    if (!codec_v3_carrier_layout(original_layout, limiter.carrier_layout)) {
        error = "cts dmx limiter: unknown original layout";
        return false;
    }
    // Limiter::Limiter @ 0x505800: ChannelLimiter for every carrier channel.
    for (std::uint32_t ch = 0; ch < kCodecV3ChannelCount; ++ch) {
        if ((limiter.carrier_layout & (std::uint32_t{1} << ch)) == 0u)
            continue;
        channel_limiter_init(
            limiter.channel_limiters[ch], sample_count, sample_rate);
        if (!channel_limiter_set_release(limiter.channel_limiters[ch], 1.0f)
            || !channel_limiter_set_max_gain_db(
                limiter.channel_limiters[ch], -1.0f)
            || !channel_limiter_set_distribution_factor(
                limiter.channel_limiters[ch], 0.5f)) {
            error = "cts dmx limiter: ChannelLimiter defaults rejected";
            return false;
        }
        limiter.limiter_enabled[ch] = true;
    }
    limiter.valid = true;
    return true;
}

bool cts_dmx_limit_new(
    CtsDmxLimiter& limiter,
    const std::array<CtsGainEntry, kCodecV3ChannelCount>& input_gains,
    const std::array<const float*, kCodecV3ChannelCount>& float_planes,
    std::array<CtsGainEntry, kCodecV3ChannelCount>& output_gains,
    std::string& error) {
    error.clear();
    output_gains = {};
    if (!limiter.valid) {
        error = "cts dmx limit_new: limiter is invalid";
        return false;
    }

    for (std::uint32_t carrier_ch = 0; carrier_ch < kCodecV3ChannelCount;
         ++carrier_ch) {
        if ((limiter.carrier_layout & (std::uint32_t{1} << carrier_ch)) == 0u)
            continue;

        OriginalChannelGroup group{};
        const std::uint32_t status = get_original_channels(
            limiter.original_layout, carrier_ch, group);
        if (status != 0u) {
            error = "cts dmx limit_new: get_original_channels failed";
            return false;
        }

        if (group.arity == 1u) {
            // Passthrough copies the carrier-channel input gain entry.
            output_gains[carrier_ch] = input_gains[carrier_ch];
            continue;
        }

        if (!limiter.limiter_enabled[carrier_ch])
            continue;

        if (group.arity == 2u) {
            const std::uint32_t ch0 = group.channels[0];
            const std::uint32_t ch1 = group.channels[1];
            float g0 = 1.0f;
            if (input_gains[ch0].present)
                g0 = db_to_linear_limit(input_gains[ch0].gain_db);
            float g1 = 1.0f;
            if (input_gains[ch1].present)
                g1 = db_to_linear_limit(input_gains[ch1].gain_db);
            const float* p0 = float_planes[ch0];
            const float* p1 = float_planes[ch1];
            if (p0 == nullptr || p1 == nullptr) {
                error = "cts dmx limit_new: missing float plane for mix2";
                return false;
            }
            if (!channel_limiter_dmx_limit_coeff2(
                    limiter.channel_limiters[carrier_ch],
                    g0,
                    g1,
                    p0,
                    p1,
                    error)) {
                return false;
            }
            write_limited_gain(output_gains, ch0, g0);
            write_limited_gain(output_gains, ch1, g1);
            continue;
        }

        if (group.arity == 3u) {
            const std::uint32_t ch0 = group.channels[0];
            const std::uint32_t ch1 = group.channels[1];
            const std::uint32_t ch2 = group.channels[2];
            float g0 = 1.0f;
            if (input_gains[ch0].present)
                g0 = db_to_linear_limit(input_gains[ch0].gain_db);
            float g1 = 1.0f;
            if (input_gains[ch1].present)
                g1 = db_to_linear_limit(input_gains[ch1].gain_db);
            float g2 = 1.0f;
            if (input_gains[ch2].present)
                g2 = db_to_linear_limit(input_gains[ch2].gain_db);
            const float* p0 = float_planes[ch0];
            const float* p1 = float_planes[ch1];
            const float* p2 = float_planes[ch2];
            if (p0 == nullptr || p1 == nullptr || p2 == nullptr) {
                error = "cts dmx limit_new: missing float plane for mix3";
                return false;
            }
            if (!channel_limiter_dmx_limit_coeff3(
                    limiter.channel_limiters[carrier_ch],
                    g0,
                    g1,
                    g2,
                    p0,
                    p1,
                    p2,
                    error)) {
                return false;
            }
            write_limited_gain(output_gains, ch0, g0);
            write_limited_gain(output_gains, ch1, g1);
            write_limited_gain(output_gains, ch2, g2);
            continue;
        }

        error = "cts dmx limit_new: unexpected original-channel arity";
        return false;
    }
    return true;
}

bool apply_cts_dmx_coeff_limit(
    const std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t original_layout,
    std::uint32_t sample_rate,
    const std::array<CtsGainEntry, kCodecV3ChannelCount>& input_gains,
    CtsDmxLimiter& limiter,
    std::array<std::uint8_t, kCodecV3ChannelCount>& scaler_indices,
    std::string& error) {
    error.clear();
    if (codec_planes.size() != kCodecV3ChannelCount || original_layout == 0u) {
        error = "cts_dmx_coeff_limit: invalid codec planes";
        return false;
    }

    std::size_t sample_count = 0u;
    bool have_count = false;
    for (std::uint32_t id = 0; id < kCodecV3ChannelCount; ++id) {
        const bool active =
            (original_layout & (std::uint32_t{1} << id)) != 0u;
        if (!active) {
            if (!codec_planes[id].empty()) {
                error = "cts_dmx_coeff_limit: inactive plane is non-empty";
                return false;
            }
            continue;
        }
        if (!have_count) {
            sample_count = codec_planes[id].size();
            have_count = true;
        } else if (codec_planes[id].size() != sample_count) {
            error = "cts_dmx_coeff_limit: mismatched plane lengths";
            return false;
        }
    }
    if (!have_count || sample_count == 0u) {
        error = "cts_dmx_coeff_limit: empty original layout";
        return false;
    }
    if (sample_count > std::numeric_limits<std::uint32_t>::max()) {
        error = "cts_dmx_coeff_limit: sample count exceeds native range";
        return false;
    }
    if (sample_rate == 0u
        || sample_rate
            > static_cast<std::uint32_t>(
                std::numeric_limits<std::int32_t>::max())) {
        error = "cts_dmx_coeff_limit: sample rate exceeds native range";
        return false;
    }

    if (!limiter.valid) {
        if (!cts_dmx_limiter_init(
                limiter,
                original_layout,
                static_cast<std::uint32_t>(sample_count),
                static_cast<std::int32_t>(sample_rate),
                error)) {
            return false;
        }
    } else {
        if (limiter.original_layout != original_layout) {
            error = "cts_dmx_coeff_limit: runtime layout changed";
            return false;
        }
        std::uint32_t expected_carrier = 0u;
        if (!codec_v3_carrier_layout(original_layout, expected_carrier)
            || limiter.carrier_layout != expected_carrier) {
            error = "cts_dmx_coeff_limit: runtime carrier layout changed";
            return false;
        }
        for (std::uint32_t ch = 0; ch < kCodecV3ChannelCount; ++ch) {
            if (!limiter.limiter_enabled[ch])
                continue;
            ChannelLimiter& channel = limiter.channel_limiters[ch];
            if (channel.sample_rate != static_cast<std::int32_t>(sample_rate)) {
                error = "cts_dmx_coeff_limit: runtime sample rate changed";
                return false;
            }
            // The standalone scheduler may use a shorter legal tail unit.
            // Preserve all smoothing state and adjust only the loop bound.
            channel.sample_count = static_cast<std::uint32_t>(sample_count);
        }
    }

    // PCM24 → float with scale 1 / ~(-1 << (bit_depth-1)) == 1/0x7FFFFF.
    const float pcm_scale =
        1.0f
        / static_cast<float>(
            ~(0xFFFFFFFFu << (kCodecV3PcmBits - 1u)));
    std::array<std::vector<float>, kCodecV3ChannelCount> float_storage{};
    std::array<const float*, kCodecV3ChannelCount> float_planes{};
    for (std::uint32_t id = 0; id < kCodecV3ChannelCount; ++id) {
        if ((original_layout & (std::uint32_t{1} << id)) == 0u)
            continue;
        float_storage[id].resize(sample_count);
        for (std::size_t i = 0; i < sample_count; ++i) {
            float_storage[id][i] =
                static_cast<float>(codec_planes[id][i]) * pcm_scale;
        }
        float_planes[id] = float_storage[id].data();
    }

    std::array<CtsGainEntry, kCodecV3ChannelCount> limited{};
    if (!cts_dmx_limit_new(
            limiter, input_gains, float_planes, limited, error)) {
        return false;
    }

    // Update Encoder+6368-equivalent indices for original-layout channels.
    for (std::uint32_t id = 0; id < kCodecV3ChannelCount; ++id) {
        if ((original_layout & (std::uint32_t{1} << id)) == 0u)
            continue;
        if (!limited[id].present)
            continue;
        const float gain_db = limited[id].gain_db;
        if (!input_gains[id].present && gain_db == 0.0f)
            continue;
        const float requested = gain_to_scaler_packed(gain_db);
        std::uint8_t index = 0u;
        float table_scaler = 0.0f;
        if (!scaler_to_index(requested, index, table_scaler)) {
            error = "cts_dmx_coeff_limit: scaler_to_ix rejected limited gain";
            return false;
        }
        scaler_indices[id] = index;
    }
    return true;
}

bool apply_cts_dmx_coeff_limit(
    const std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t original_layout,
    std::uint32_t sample_rate,
    const std::array<CtsGainEntry, kCodecV3ChannelCount>& input_gains,
    std::array<std::uint8_t, kCodecV3ChannelCount>& scaler_indices,
    std::string& error) {
    CtsDmxLimiter limiter{};
    return apply_cts_dmx_coeff_limit(
        codec_planes,
        original_layout,
        sample_rate,
        input_gains,
        limiter,
        scaler_indices,
        error);
}

} // namespace auro3d::encode
