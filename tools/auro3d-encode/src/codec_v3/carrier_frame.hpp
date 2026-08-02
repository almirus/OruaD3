#pragma once

#include "channel_frame.hpp"
#include "frame_descriptor.hpp"
#include "metadata_block.hpp"
#include "prepare_metadata.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Serialized carrier unit ready for the PCM/WAV writer.
struct EncodedCarrierUnit {
    CarrierUnit carrier;
    MetadataUnitHeader metadata_header;
    std::uint32_t metadata_channel_id = 0;
    PcmMetadataBlock metadata;
    /// Native prepare_metadata_unit_block_ records retained alongside the
    /// carrier for output validation and trace diagnostics.
    std::vector<MetadataGroupRecord> metadata_groups;
    /// Exact ADOL instruction order projected into each carrier channel.
    std::array<std::vector<AdolInstruction>, 31> channel_adol;
    std::vector<EncodedChannelFrame> channels;
};

/// Places already encoded and CRC-closed channel words into carrier PCM
/// planes. It performs no downmix, quantization, padding policy, or layout
/// inference.
bool assemble_carrier_unit(
    std::uint32_t carrier_layout,
    std::uint32_t sample_rate,
    std::uint32_t frame_count,
    std::uint32_t metadata_channel_id,
    const std::vector<EncodedChannelFrame>& channels,
    const PcmMetadataBlock& metadata,
    EncodedCarrierUnit& out,
    std::string& error);

} // namespace auro3d::encode
