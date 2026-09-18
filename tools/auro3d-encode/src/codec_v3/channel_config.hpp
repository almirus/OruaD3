#pragma once

#include <cstdint>

namespace auro3d::encode {

/// Direct (non-alias) codec-v3 ADOL channel-input configuration table. These
/// IDs are the values carried by opcode 0x1e; aliases accepted by the decoder
/// are intentionally excluded because an encoder must emit one native form.
bool codec_v3_channel_config_to_layout(
    std::uint8_t config_id,
    std::uint32_t& original_layout);

bool codec_v3_layout_to_channel_config(
    std::uint32_t original_layout,
    std::uint8_t& config_id);

bool codec_v3_channel_config_to_carrier_layout(
    std::uint8_t config_id,
    std::uint32_t& carrier_layout);

} // namespace auro3d:encode
