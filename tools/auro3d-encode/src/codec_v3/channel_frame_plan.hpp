#pragma once

#include "channel_frame.hpp"
#include "process_groups.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Builds the native arity-1 channel-frame prefix for every active carrier
/// channel. Mixed groups are rejected here rather than being encoded as a
/// direct channel by accident; their VQ/parser stream must be supplied by the
/// corresponding mix serializer.
bool build_direct_channel_frames(
    const std::vector<AnalyzedEncodeGroup>& groups,
    std::uint32_t frame_count,
    std::vector<EncodedChannelFrame>& out,
    std::string& error);

/// Builds serialized channel frames for analyzed arity-1/2/3 groups. Mix2
/// and mix3 use the analyzed residual tables and a static native Golomb-Rice
/// stream; its k=1..7 value and cost come directly from BitSize:calculate.
/// No GVM clustering or output-generator policy is added.
bool build_analyzed_channel_frames(
    const std::vector<AnalyzedEncodeGroup>& groups,
    std::uint32_t frame_count,
    std::vector<EncodedChannelFrame>& out,
    std::string& error);

bool build_analyzed_channel_frames(
    const std::vector<AnalyzedEncodeGroup>& groups,
    std::uint32_t frame_count,
    const std::array<std::uint8_t, 31>* input_scaler_indices,
    const std::vector<AdolInstruction>& common_adol,
    const std::array<std::vector<AdolInstruction>, 31>& optional_adol_by_channel,
    std::vector<EncodedChannelFrame>& out,
    std::string& error);

/// Same as `build_analyzed_channel_frames`, and when `input_scaler_indices` is
/// non-null emits channel-parser opcode 64 for each source with a non-zero
/// original-map entry (compose:Channel /).
bool build_analyzed_channel_frames(
    const std::vector<AnalyzedEncodeGroup>& groups,
    std::uint32_t frame_count,
    const std::array<std::uint8_t, 31>* input_scaler_indices,
    const std::vector<AdolInstruction>& common_adol,
    std::uint32_t optional_adol_channel,
    const std::vector<AdolInstruction>& optional_adol,
    std::vector<EncodedChannelFrame>& out,
    std::string& error);

/// Overlays each serialized channel prefix onto its already prepared carrier
/// plane and closes the channel CRC over the complete unit span. This keeps
/// the carrier PCM produced by prepare_mix_ intact outside the payload bits.
bool merge_channel_frames_into_carrier(
    CarrierUnit& carrier,
    std::vector<EncodedChannelFrame>& frames,
    std::string& error);

} // namespace auro3d:encode
