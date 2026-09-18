#pragma once

#include <array>
#include <cstdint>

namespace auro3d::encode {

/// Result of auro:codec:v3:get_original_channels.
/// arity is 1 (passthrough), 2 (mix2) or 3 (mix3). Channel IDs are the
/// codec-v3 speaker indices that form one encode group for carrier_channel.
struct OriginalChannelGroup {
    std::uint32_t arity = 0;
    std::array<std::uint32_t, 3> channels{};
};

/// Native status: 0 on success, 407 when the original layout is unknown,
/// 408 when the carrier channel is not part of that layout's encode groups.
std::uint32_t get_original_channels(
    std::uint32_t original_layout,
    std::uint32_t carrier_channel,
    OriginalChannelGroup& out);

} // namespace auro3d:encode
