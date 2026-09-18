#include "channel_metadata.hpp"

#include <array>

namespace auro3d::encode {

bool make_channel_metadata_words(
    std::uint32_t bit_width,
    std::uint32_t selector,
    const std::array<std::uint32_t, 3>& channel_ids,
    std::uint32_t third_word,
    std::array<std::uint32_t, 3>& metadata_words) {
    metadata_words = {};
    if (bit_width > 32u || selector > 0x53u)
        return false;
    std::array<bool, 31> seen{};
    for (const std::uint32_t id : channel_ids) {
        if (id > 30u && id != 255u)
            return false;
        if (id < 31u) {
            if (seen[id])
                return false;
            seen[id] = true;
        }
    }
    metadata_words[0] = bit_width | (selector << 16u);
    metadata_words[1] =
        ((channel_ids[0] & 0xFFu) << 24u)
        | ((channel_ids[1] & 0xFFu) << 16u)
        | ((channel_ids[2] & 0xFFu) << 8u)
        | 0xFFu;
    metadata_words[2] = third_word;
    return true;
}

bool combine_channel_metadata(
    std::uint16_t channel_header,
    const std::array<std::uint32_t, 3>& metadata_words,
    ChannelMetadataCombined& combined) {
    combined = {};
    if (!(channel_header <= 0x1FFu &&
        (channel_header < 0x100u || static_cast<std::uint8_t>(channel_header) <= 0x0Au))) {
        return false;
    }
    const std::uint32_t word0 = metadata_words[0];
    const std::uint32_t selector = (word0 >> 16u) & 0xFFu;
    combined.selector = selector;
    std::uint32_t base = 0;
    if (selector <= 4u) {
        base = 2u * selector + 8u;
    } else if (selector <= 15u) {
        base = 4u * selector;
    } else if (selector <= 0x53u) {
        base = 8u * selector - 64u;
    } else {
        return false;
    }
    combined.bit_width = word0 & 0xFFu;
    combined.base_index = base;
    combined.channel_ids[0] = (metadata_words[1] >> 24u) & 0xFFu;
    combined.channel_ids[1] = (metadata_words[1] >> 16u) & 0xFFu;
    combined.channel_ids[2] = (metadata_words[1] >> 8u) & 0xFFu;
    for (std::uint32_t& id : combined.channel_ids)
        id = id == 255u ? 31u : id;
    for (const std::uint32_t id : combined.channel_ids)
        combined.mode += id < 31u ? 1u : 0u;
    if (combined.mode > 3u)
        return false;
    return true;
}

bool channel_metadata_selector_from_base(
    std::uint32_t base_index,
    std::uint32_t& selector) {
    selector = 0u;
    for (std::uint32_t candidate = 0u; candidate <= 0x53u; ++candidate) {
        const std::uint32_t base = candidate <= 4u
            ? 2u * candidate + 8u
            : candidate <= 15u
                ? 4u * candidate
                : 8u * candidate - 64u;
        if (base == base_index) {
            selector = candidate;
            return true;
        }
    }
    return false;
}

bool channel_metadata_selector_from_count(
    std::uint32_t entry_count,
    std::uint32_t& selector,
    std::uint32_t& base_index) {
    selector = 0u;
    base_index = 0u;
    if (entry_count == 0u)
        return false;

    // Direct integer form of a3d:details:error_center_index.
    // The decompiler prints the negative constants as wrapped unsigned
    // values; spelling the piecewise ranges explicitly preserves their
    // intended signed arithmetic.
    if (entry_count >= 633u) {
        selector = ((entry_count - 633u) >> 4u) + 85u;
    } else if (entry_count > 600u) {
        selector = 84u;
    } else if (entry_count >= 65u) {
        selector = ((entry_count - 65u) >> 3u) + 17u;
    } else if (entry_count >= 17u) {
        selector = ((entry_count - 17u) >> 2u) + 5u;
    } else if (entry_count >= 9u) {
        selector = ((entry_count - 9u) >> 1u) + 1u;
    }
    if (selector > 0x53u)
        return false;
    base_index = selector <= 4u
        ? 2u * selector + 8u
        : selector <= 15u
            ? 4u * selector
            : 8u * selector - 64u;
    return entry_count <= base_index;
}

} // namespace auro3d:encode
