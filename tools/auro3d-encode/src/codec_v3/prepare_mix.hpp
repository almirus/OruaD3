#pragma once

#include "frame_descriptor.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Completed per-channel output of the group processor. Native
/// prepare_mix_ identifies the carrier destination by channel ID and either
/// copies this stream or restores its left quantization shift.
struct EncodedGroupPcm {
    std::uint32_t carrier_channel_id = 0;
    std::uint32_t quantization_shift = 0;
    std::vector<std::int32_t> samples;
};

/// Direct scalar port of Encoder:prepare_mix_. `copy_raw` is its
/// final boolean argument: false applies the native 32-bit left shift, true
/// copies samples unchanged.
bool prepare_mix(
    const std::vector<EncodedGroupPcm>& groups,
    MutableFrameDescriptor& carrier,
    bool copy_raw,
    std::string& error);

} // namespace auro3d:encode
