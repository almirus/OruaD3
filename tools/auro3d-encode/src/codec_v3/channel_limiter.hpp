#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Direct port of auro:codec:v3:cts:dmx:ChannelLimiter.
/// Used by Limiter:limit_new cts_dmx_coeff_limit_ when Config+136 is set.
struct ChannelLimiter {
    float release = 1.0f;              // +0
    float inv_release = 1.0f;          // +4
    float distribution_factor = 0.5f;   // +8
    float max_gain = 0.98f;            // +12; native dword 1065161169
    std::int32_t sample_rate = 0;      // +16
    std::uint32_t sample_count = 0;    // +20
    float smoothed0 = 1.0f;            // +24
    float smoothed1 = 1.0f;            // +28
    float smoothed2 = 1.0f;            // +32
    float peak0 = 0.0f;                // +36
    float peak1 = 0.0f;                // +40
    float peak2 = 0.0f;                // +44
    float peak_floor = 0.0f;           // +48
};

/// ChannelLimiter:ChannelLimiter.
void channel_limiter_init(
    ChannelLimiter& limiter,
    std::uint32_t sample_count,
    std::int32_t sample_rate);

/// set_gain_distrubution_factor. Accepts (0,1) exclusive.
bool channel_limiter_set_distribution_factor(
    ChannelLimiter& limiter,
    float factor);

/// set_release. `seconds` must be >= 0.
bool channel_limiter_set_release(
    ChannelLimiter& limiter,
    float seconds);

/// set_max_gain_dB. Accepts gains <= 0.
bool channel_limiter_set_max_gain_db(
    ChannelLimiter& limiter,
    float gain_db);

/// Two-source dmx_limit_coeff. Returns false on native failure
/// (any input gain > 1).
bool channel_limiter_dmx_limit_coeff2(
    ChannelLimiter& limiter,
    float& gain0,
    float& gain1,
    const float* samples0,
    const float* samples1,
    std::string& error);

/// Three-source dmx_limit_coeff.
bool channel_limiter_dmx_limit_coeff3(
    ChannelLimiter& limiter,
    float& gain0,
    float& gain1,
    float& gain2,
    const float* samples0,
    const float* samples1,
    const float* samples2,
    std::string& error);

} // namespace auro3d:encode
