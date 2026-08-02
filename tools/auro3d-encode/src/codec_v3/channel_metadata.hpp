#pragma once

#include <array>
#include <cstdint>

namespace auro3d::encode {

constexpr std::uint32_t kCodecV3ChannelCodebookMaxEntries = 600u;

struct ChannelMetadataCombined {
    std::uint32_t mode = 0;
    std::uint32_t selector = 0;
    std::uint32_t base_index = 0;
    std::uint32_t bit_width = 0;
    std::array<std::uint32_t, 3> channel_ids{};
};

/// Inverse of channel_metadata_combine_info's fields at offsets 80..91. A
/// zero bit width is legal for the direct/arity-1 form; the caller supplies
/// the third native metadata word explicitly because it is consumed by later
/// parser states and has no universal default.
bool make_channel_metadata_words(
    std::uint32_t bit_width,
    std::uint32_t selector,
    const std::array<std::uint32_t, 3>& channel_ids,
    std::uint32_t third_word,
    std::array<std::uint32_t, 3>& metadata_words);

/// Exact inverse-side validation of channel_metadata_combine_info in the
/// decoder. `metadata_words` are the three 32-bit fields consumed after the
/// 16-bit channel header.
bool combine_channel_metadata(
    std::uint16_t channel_header,
    const std::array<std::uint32_t, 3>& metadata_words,
    ChannelMetadataCombined& combined);

/// Converts the decoder's derived codebook base index back to the compact
/// selector byte used in metadata word zero.  Only the three native ranges
/// accepted by channel_metadata_combine_info are representable.
bool channel_metadata_selector_from_base(
    std::uint32_t base_index,
    std::uint32_t& selector);

/// Maps an actual native codebook entry count to the selector and rounded
/// context capacity used by error_center_index/nr_error_centers_per_index.
/// Counts below the first capacity are valid and round up to selector 0.
bool channel_metadata_selector_from_count(
    std::uint32_t entry_count,
    std::uint32_t& selector,
    std::uint32_t& base_index);

} // namespace auro3d::encode
