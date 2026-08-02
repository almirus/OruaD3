#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

constexpr std::uint32_t kCodecV3ChannelCount = 31u;
constexpr std::uint32_t kCodecV3PcmBits = 24u;

struct ConstFrameDescriptor {
    std::uint32_t frame_count = 0;
    std::uint32_t sample_rate = 0;
    std::uint32_t sample_bits = 0;
    std::uint32_t layout = 0;
    std::array<const std::int32_t*, kCodecV3ChannelCount> channel_ptr{};
};

struct MutableFrameDescriptor {
    std::uint32_t frame_count = 0;
    std::uint32_t sample_rate = 0;
    std::uint32_t sample_bits = 0;
    std::uint32_t layout = 0;
    std::array<std::int32_t*, kCodecV3ChannelCount> channel_ptr{};
};

struct CarrierUnit {
    std::array<std::vector<std::int32_t>, kCodecV3ChannelCount> planes{};
    MutableFrameDescriptor descriptor{};
};

bool make_input_descriptor(
    const std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t layout,
    std::uint32_t sample_rate,
    std::uint32_t frame_count,
    ConstFrameDescriptor& descriptor,
    std::string& error);

bool make_carrier_unit(
    std::uint32_t carrier_layout,
    std::uint32_t sample_rate,
    std::uint32_t frame_count,
    CarrierUnit& unit,
    std::string& error);

/// Mirrors the descriptor validation at native Encoder::encode @ 0x4E4330.
bool validate_encoder_frame_contract(
    const ConstFrameDescriptor& original,
    const MutableFrameDescriptor& carrier,
    std::string& error);

} // namespace auro3d::encode
