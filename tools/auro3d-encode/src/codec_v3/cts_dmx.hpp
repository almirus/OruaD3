#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "channel_limiter.hpp"
#include "frame_descriptor.hpp"

namespace auro3d::encode {

/// Encoder+6384 limit_new gain table entry: float dB + present byte.
struct CtsGainEntry {
    float gain_db = 0.0f;
    bool present = false;
};

/// auro:codec:v3:cts:dmx:Limiter subset used by limit_new.
/// Large cts:Limiter objects at +16 are unused by limit_new and omitted.
struct CtsDmxLimiter {
    std::uint32_t original_layout = 0;
    std::uint32_t carrier_layout = 0;
    bool valid = false;
    std::array<ChannelLimiter, kCodecV3ChannelCount> channel_limiters{};
    std::array<bool, kCodecV3ChannelCount> limiter_enabled{};
};

/// Limiter:Limiter for the ChannelLimiter array path.
bool cts_dmx_limiter_init(
    CtsDmxLimiter& limiter,
    std::uint32_t original_layout,
    std::uint32_t sample_count,
    std::int32_t sample_rate,
    std::string& error);

/// Limiter:limit_new. `float_planes[ch]` must be non-null for
/// every original-layout channel that participates in an arity-2/3 group.
bool cts_dmx_limit_new(
    CtsDmxLimiter& limiter,
    const std::array<CtsGainEntry, kCodecV3ChannelCount>& input_gains,
    const std::array<const float*, kCodecV3ChannelCount>& float_planes,
    std::array<CtsGainEntry, kCodecV3ChannelCount>& output_gains,
    std::string& error);

/// Encoder:cts_dmx_coeff_limit_ without the Encoder object:
/// PCM24→float, limit_new, then gain_to_scaler/scaler_to_ix into the caller
/// scaler-index table (Encoder+6368 tree). Does not divide PCM; downmix_
/// applies that afterward.
bool apply_cts_dmx_coeff_limit(
    const std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t original_layout,
    std::uint32_t sample_rate,
    const std::array<CtsGainEntry, kCodecV3ChannelCount>& input_gains,
    std::array<std::uint8_t, kCodecV3ChannelCount>& scaler_indices,
    std::string& error);

/// Stateful Encoder-object form. ChannelLimiter release, peak, and smoothed
/// coefficients survive consecutive UnitBlocks exactly as Encoder+6632 does.
bool apply_cts_dmx_coeff_limit(
    const std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t original_layout,
    std::uint32_t sample_rate,
    const std::array<CtsGainEntry, kCodecV3ChannelCount>& input_gains,
    CtsDmxLimiter& limiter,
    std::array<std::uint8_t, kCodecV3ChannelCount>& scaler_indices,
    std::string& error);

} // namespace auro3d:encode
