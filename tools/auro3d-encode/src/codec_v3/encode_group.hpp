#pragma once

#include "channel_payload.hpp"
#include "encoder_defaults.hpp"
#include "original_channels.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// One original-layout plane copied into a group, corresponding to the Frame
/// emplace performed by sub_4E83F0 @ 0x4E83F0.
struct EncodeGroupFrame {
    std::uint32_t channel_id = 0;
    std::vector<std::int32_t> samples;
};

/// Minimal encode-group state filled by Encoder::add_ before analysis runs.
struct EncodeGroup {
    std::uint32_t carrier_channel = 0;
    std::uint32_t bit_line = 0;
    std::uint32_t bit_line_quality = 0;
    /// Native Group+208, copied from Encoder+6352 in Group::create after the
    /// public Encoder::reserve_extra_bits setter.
    std::uint32_t group_field_208 = 0;
    /// Encoder::create_group_ values at native offsets +36/+40/+44/+48.
    std::uint32_t gvm_common_limit = 150;
    std::uint32_t gvm_dimension1_start = 80;
    std::uint32_t gvm_dimension2_start = 80;
    std::uint32_t gvm_minimum_clusters = 1;
    /// Rescaler retry floor for Group+392. Zero lets Rescaler select its
    /// initial peak-derived index; Mixer overflow increments this value.
    std::uint8_t minimum_scaler_index = 0;
    /// Inputs to Group+196 accounting. The direct standalone branch leaves all
    /// optional records absent and therefore starts at the native 32 bits.
    NativeRescalerAccounting rescaler_accounting{};
    NativeClusterDeltasBackend cluster_backend =
        NativeClusterDeltasBackend::gvm;
    NativeGvmConfiguration gvm{};
    OriginalChannelGroup sources{};
    std::vector<EncodeGroupFrame> frames;
};

/// Copies one original channel plane into `group.frames`. Native sub_4E83F0
/// rejects empty/invalid ranges; this port does the same and owns a deep copy.
bool attach_group_channel_pcm(
    EncodeGroup& group,
    std::uint32_t channel_id,
    const std::int32_t* begin,
    const std::int32_t* end,
    std::string& error);

} // namespace auro3d::encode
