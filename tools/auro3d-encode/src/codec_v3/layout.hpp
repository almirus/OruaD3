#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

bool parse_codec_v3_layout(const std::string& text, std::uint32_t& mask, std::string& error);
unsigned layout_channel_count(std::uint32_t mask);
const char* layout_label(std::uint32_t mask);
bool codec_v3_layout_channel_order(std::uint32_t mask, std::vector<std::uint32_t>& channel_ids);

/// Native auro_codec_v3_get_carrier_layout mapping. Returns false for an
/// original layout that the codec-v3 unit encoder rejects.
bool codec_v3_carrier_layout(std::uint32_t original_layout, std::uint32_t& carrier_layout);

/// Physical lossless-container layout selected by the native Auro 2D writer.
/// The codec carrier mask remains unchanged in embedded metadata.
bool codec_v3_output_layout(
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    std::uint32_t& output_layout);

/// Encoder::set_carrier_ metadata/optional-ADOL channel selection at
/// 0x4E8620: LFE, RB, C, RS, then FR in descending native priority.
bool codec_v3_metadata_carrier_channel(
    std::uint32_t carrier_layout,
    std::uint32_t& channel_id);

bool codec_v3_sample_rate_supported(std::uint32_t sample_rate);
bool codec_v3_unit_block_size_supported(std::uint32_t block_size);

/// Partitions an exact source length into native codec-v3 UnitBlock sizes
/// without padding or trimming. The preferred size is used for the long
/// prefix; a bounded dynamic tail planner rebalances the last blocks.
bool plan_codec_v3_unit_blocks(
    std::uint64_t frame_count,
    std::uint32_t preferred_block_size,
    std::vector<std::uint32_t>& blocks,
    std::string& error);

bool parse_input_channel_order(
    const std::string& csv,
    std::uint32_t layout_mask,
    std::vector<std::uint32_t>& channel_ids,
    std::string& error);

/// Parses the physical WAV order and derives the exact codec-v3 original
/// layout mask from the supplied channel names.
bool derive_layout_from_channel_order(
    const std::string& csv,
    std::uint32_t& layout_mask,
    std::vector<std::uint32_t>& channel_ids,
    std::string& error);

/// Converts a WAVEFORMATEXTENSIBLE speaker mask to the interleaved codec
/// channel IDs in the standard ascending-speaker-bit WAV order.
bool derive_layout_from_wav_channel_mask(
    std::uint32_t wav_mask,
    std::uint32_t& layout_mask,
    std::vector<std::uint32_t>& channel_ids,
    std::string& error);
std::string format_channel_order(const std::vector<std::uint32_t>& channel_ids);

} // namespace auro3d::encode
