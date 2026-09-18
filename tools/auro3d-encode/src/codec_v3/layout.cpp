#include "layout.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>
#include <utility>

namespace auro3d::encode {
namespace {

std::string normalise(std::string value) {
    for (char& c : value)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    std::replace(value.begin(), value.end(), '+', '_');
    std::replace(value.begin(), value.end(), '-', '_');
    return value;
}

struct ChannelName { const char* name; std::uint32_t id; };
constexpr ChannelName kChannelNames[] = {
    {"FL", 0u}, {"L", 0u}, {"FR", 1u}, {"R", 1u},
    {"C", 2u}, {"FC", 2u}, {"CENTER", 2u}, {"LFE", 3u},
    {"LS", 4u}, {"SL", 4u}, {"RS", 5u}, {"SR", 5u}, {"CS", 6u},
    {"LB", 7u}, {"BL", 7u}, {"RB", 8u}, {"BR", 8u}, {"HL", 9u},
    {"HR", 10u}, {"HC", 11u}, {"T", 12u}, {"TOP", 12u},
    {"HLS", 13u}, {"HRS", 14u}, {"HCS", 15u}, {"HLB", 16u}, {"HRB", 17u},
    {"LC", 18u}, {"LEFT_CENTER", 18u}, {"LEFTCENTER", 18u},
    {"RC", 19u}, {"RIGHT_CENTER", 19u}, {"RIGHTCENTER", 19u},
    {"LFE2", 20u}, {"BOTTOM_LEFT", 21u}, {"BOTTOMLEFT", 21u},
    {"BOTTOM_RIGHT", 22u}, {"BOTTOMRIGHT", 22u},
    {"BOTTOM_CENTER", 23u}, {"BOTTOMCENTER", 23u},
    {"BOTTOM_LEFT_SURROUND", 24u}, {"BOTTOMLEFTSURROUND", 24u},
    {"BOTTOM_RIGHT_SURROUND", 25u}, {"BOTTOMRIGHTSURROUND", 25u},
    {"OBJECT", 26u}, {"TOP_LEFT", 28u}, {"TOPLEFT", 28u},
    {"TOP_RIGHT", 29u}, {"TOPRIGHT", 29u}, {"MONO", 30u},
};

const char* canonical_channel_name(std::uint32_t id) {
    static constexpr const char* kNames[] = {
        "FL", "FR", "C", "LFE", "LS", "RS", "CS", "LB", "RB", "HL", "HR", "HC", "T",
        "HLS", "HRS", "HCS", "HLB", "HRB", "LC", "RC", "LFE2",
        "BOTTOM_LEFT", "BOTTOM_RIGHT", "BOTTOM_CENTER",
        "BOTTOM_LEFT_SURROUND", "BOTTOM_RIGHT_SURROUND",
        "OBJECT", "INVALID", "TOP_LEFT", "TOP_RIGHT", "MONO",
    };
    return id < sizeof(kNames) / sizeof(kNames[0]) ? kNames[id] : nullptr;
}

} // namespace

unsigned layout_channel_count(std::uint32_t mask) {
    unsigned result = 0;
    while (mask != 0u) {
        result += mask & 1u;
        mask >>= 1u;
    }
    return result;
}

const char* layout_label(std::uint32_t mask) {
    switch (mask) {
    case 3u: return "2.0";
    case 11u: return "2.1";
    case 63u: return "5.1";
    case 447u: return "7.1";
    case 26175u: return "5.1_4H";
    case 26559u: return "7.1_4H";
    case 30271u: return "5.1_4H_1T";
    case 30655u: return "7.1_4H_1T";
    case 32703u: return "7.1_5H_1T";
    default: return nullptr;
    }
}

bool codec_v3_layout_channel_order(std::uint32_t mask, std::vector<std::uint32_t>& channel_ids) {
    channel_ids.clear();
    for (std::uint32_t id = 0; id < 31u; ++id) {
        if ((mask & (std::uint32_t{1} << id)) != 0u)
            channel_ids.push_back(id);
    }
    return !channel_ids.empty();
}

bool codec_v3_carrier_layout(std::uint32_t original_layout, std::uint32_t& carrier_layout) {
    // auro_codec_v3_get_carrier_layout (native import), reproduced
    // from the codec-v3 decoder port's direct native switch.
    switch (original_layout) {
    case 3u: case 4u: case 2052u: case 6148u:
        carrier_layout = 4u; return true;
    case 7u: case 51u: case 55u: case 71u: case 1587u:
        carrier_layout = 3u; return true;
    case 63u:
        carrier_layout = 11u; return true;
    case 119u: case 439u: case 26167u: case 30263u: case 32311u:
        carrier_layout = 55u; return true;
    case 127u: case 447u: case 1599u: case 26175u: case 30271u: case 32319u:
        carrier_layout = 63u; return true;
    case 26163u:
        carrier_layout = 51u; return true;
    case 26551u: case 30647u: case 32695u:
        carrier_layout = 439u; return true;
    case 1983u: case 26559u: case 30655u: case 32703u:
        carrier_layout = 447u; return true;
    default:
        return false;
    }
}

bool codec_v3_output_layout(
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    std::uint32_t& output_layout) {
    std::uint32_t expected_carrier = 0u;
    if (!codec_v3_carrier_layout(original_layout, expected_carrier)
        || expected_carrier != carrier_layout) {
        output_layout = 0u;
        return false;
    }
    // Native auro2d.flac for original 5.1 carries the logical 2.1 mask in
    // codec-v3 metadata but writes only FL/FR to the physical container. The
    // absent carrier LFE consequently decodes as silence, matching the native
    // file rather than inventing a third physical channel.
    output_layout =
        original_layout == 63u && carrier_layout == 11u
        ? 3u
        : carrier_layout;
    return true;
}

bool codec_v3_metadata_carrier_channel(
    std::uint32_t carrier_layout,
    std::uint32_t& channel_id) {
    // Encoder:set_carrier_ initializes the candidate to LFE and
    // then tests this exact priority chain. Every native carrier layout has
    // one of these channels; FL is deliberately never the fallback.
    constexpr std::uint32_t kPriority[] = {
        3u, // LFE
        8u, // RB
        2u, // C
        5u, // RS
        1u, // FR
    };
    for (const std::uint32_t candidate : kPriority) {
        if ((carrier_layout & (std::uint32_t{1} << candidate)) != 0u) {
            channel_id = candidate;
            return true;
        }
    }
    channel_id = 0u;
    return false;
}

bool codec_v3_sample_rate_supported(std::uint32_t sample_rate) {
    // auro_codec_v3_is_sample_rate_supported (native import).
    return sample_rate == 44100u || sample_rate == 48000u
        || sample_rate == 88200u || sample_rate == 96000u;
}

bool codec_v3_unit_block_size_supported(std::uint32_t block_size) {
    // auro_codec_v3_is_unit_block_size_supported (native import).
    if (block_size == 1000u)
        return true;
    if (block_size == 992u)
        return false;
    const std::uint32_t delta = block_size - 1025u;
    return delta >= 0xFFFFFCFFu && (block_size & 0xFu) == 0u;
}

bool plan_codec_v3_unit_blocks(
    std::uint64_t frame_count,
    std::uint32_t preferred_block_size,
    std::vector<std::uint32_t>& blocks,
    std::string& error) {
    error.clear();
    blocks.clear();
    if (frame_count == 0u) {
        error = "codec-v3 source contains no frames";
        return false;
    }
    if (!codec_v3_unit_block_size_supported(preferred_block_size)) {
        error = "preferred codec-v3 UnitBlock size is unsupported";
        return false;
    }

    // Native codec-v3 UnitBlocks are all multiples of eight samples. The
    // final carrier block may therefore require zero padding; the container
    // writer trims those carrier samples back to the source length.
    const std::uint64_t padding = (8u - (frame_count & 7u)) & 7u;
    if (frame_count > std::numeric_limits<std::uint64_t>::max() - padding) {
        error = "codec-v3 padded frame count overflows 64-bit range";
        return false;
    }
    const std::uint64_t planned_frame_count = frame_count + padding;

    std::vector<std::uint32_t> candidates;
    candidates.reserve(50u);
    candidates.push_back(preferred_block_size);
    for (std::uint32_t size = 1024u; size >= 256u; size -= 16u) {
        if (size != preferred_block_size)
            candidates.push_back(size);
        if (size == 256u)
            break;
    }
    if (preferred_block_size != 1000u)
        candidates.push_back(1000u);

    // Keep the dynamic suffix small for long files. Sixteen rollback slots
    // cover both residues introduced by the 1000-sample special size and all
    // 16-sample-aligned candidates while leaving ample room for rebalancing.
    constexpr std::uint64_t kTailTarget = 8192u;
    constexpr std::uint64_t kRollbackLimit = 16u;
    std::uint64_t prefix_count = 0u;
    if (planned_frame_count > kTailTarget) {
        prefix_count =
            (planned_frame_count - kTailTarget) / preferred_block_size;
    }

    std::vector<std::uint32_t> tail;
    bool found = false;
    for (std::uint64_t rollback = 0u;
         rollback <= kRollbackLimit && rollback <= prefix_count;
         ++rollback) {
        const std::uint64_t candidate_prefix = prefix_count - rollback;
        if (candidate_prefix
            > std::numeric_limits<std::uint64_t>::max()
                / preferred_block_size) {
            error = "codec-v3 UnitBlock prefix length overflows";
            return false;
        }
        const std::uint64_t prefix_frames =
            candidate_prefix * preferred_block_size;
        if (prefix_frames > planned_frame_count)
            continue;
        const std::uint64_t tail_frames = planned_frame_count - prefix_frames;
        if (tail_frames > std::numeric_limits<std::size_t>::max() - 1u)
            continue;
        std::vector<std::uint32_t> parent(
            static_cast<std::size_t>(tail_frames) + 1u, 0u);
        parent[0] = std::numeric_limits<std::uint32_t>::max();
        for (std::uint64_t sum = 0u; sum <= tail_frames; ++sum) {
            if (parent[static_cast<std::size_t>(sum)] == 0u)
                continue;
            for (const std::uint32_t size : candidates) {
                if (size > tail_frames - sum)
                    continue;
                const std::size_t next =
                    static_cast<std::size_t>(sum + size);
                if (parent[next] == 0u)
                    parent[next] = size;
            }
        }
        if (parent[static_cast<std::size_t>(tail_frames)] == 0u)
            continue;

        tail.clear();
        std::uint64_t cursor = tail_frames;
        while (cursor != 0u) {
            const std::uint32_t size =
                parent[static_cast<std::size_t>(cursor)];
            if (!codec_v3_unit_block_size_supported(size)
                || size > cursor) {
                error = "codec-v3 UnitBlock tail planner produced an invalid predecessor";
                return false;
            }
            tail.push_back(size);
            cursor -= size;
        }
        // `parent` is walked from the end of the stream towards its start.
        // Keep that order: the DP visits the preferred size first, so this
        // places complete preferred UnitBlocks before the rebalanced short
        // block. Reversing the predecessor chain put the short block first;
        // codec-v3 then retained its length for the following seven frames
        // until the next eight-frame configuration refresh.
        std::stable_partition(
            tail.begin(), tail.end(),
            [preferred_block_size](std::uint32_t size) {
                return size == preferred_block_size;
            });
        prefix_count = candidate_prefix;
        found = true;
        break;
    }
    if (!found) {
        error = "source frame count cannot be represented by native codec-v3 UnitBlock sizes without padding";
        return false;
    }
    if (prefix_count > blocks.max_size()
        || tail.size() > blocks.max_size() - prefix_count) {
        error = "codec-v3 UnitBlock schedule exceeds storage capacity";
        return false;
    }
    blocks.assign(
        static_cast<std::size_t>(prefix_count), preferred_block_size);
    blocks.insert(blocks.end(), tail.begin(), tail.end());

    std::uint64_t scheduled = 0u;
    for (const std::uint32_t size : blocks) {
        if (!codec_v3_unit_block_size_supported(size)
            || scheduled > planned_frame_count
            || size > planned_frame_count - scheduled) {
            error = "codec-v3 UnitBlock schedule failed final validation";
            blocks.clear();
            return false;
        }
        scheduled += size;
    }
    if (scheduled != planned_frame_count) {
        error = "codec-v3 UnitBlock schedule does not cover the padded source";
        blocks.clear();
        return false;
    }
    return true;
}

bool parse_codec_v3_layout(const std::string& text, std::uint32_t& mask, std::string& error) {
    const std::string key = normalise(text);
    struct Alias { const char* name; std::uint32_t value; };
    static constexpr Alias kAliases[] = {
        {"2.0", 3u}, {"stereo", 3u}, {"2.1", 11u}, {"5.1", 63u},
        {"7.1", 447u}, {"5.1_4h", 26175u}, {"5.1.4", 26175u},
        {"7.1_4h", 26559u}, {"7.1.4", 26559u},
        {"5.1_4h_1t", 30271u}, {"7.1_4h_1t", 30655u},
        {"7.1_5h_1t", 32703u}, {"7.1.5", 32703u},
    };
    for (const Alias& alias : kAliases) {
        if (key == alias.name) {
            mask = alias.value;
            return true;
        }
    }
    const std::string prefix = "mask:";
    if (key.rfind(prefix, 0u) == 0u || key.rfind("0x", 0u) == 0u) {
        const std::string number = key.rfind(prefix, 0u) == 0u
            ? key.substr(prefix.size()) : key;
        try {
            std::size_t consumed = 0;
            const unsigned long parsed = std::stoul(number, &consumed, 0);
            if (consumed == number.size() && parsed != 0ul && parsed <= 0x7FFFFFFFul) {
                std::uint32_t carrier = 0;
                if (codec_v3_carrier_layout(static_cast<std::uint32_t>(parsed), carrier)) {
                    mask = static_cast<std::uint32_t>(parsed);
                    return true;
                }
            }
        } catch (...) {
            // Fall through to the same explicit unsupported-layout error.
        }
    }
    error = "unsupported codec-v3 layout \"" + text
        + "\" (use a named layout or mask:0x... for a native codec-v3 mask)";
    return false;
}

bool parse_input_channel_order(
    const std::string& csv,
    std::uint32_t layout_mask,
    std::vector<std::uint32_t>& channel_ids,
    std::string& error) {
    channel_ids.clear();
    if (csv.empty() || csv.front() == ',' || csv.back() == ',') {
        error = "--input-channel-order contains an empty channel name";
        return false;
    }
    std::uint32_t supplied_mask = 0;
    std::stringstream input(csv);
    std::string token;
    while (std::getline(input, token, ',')) {
        token.erase(std::remove_if(token.begin(), token.end(), [](unsigned char c) { return std::isspace(c) != 0; }), token.end());
        if (token.empty()) {
            error = "--input-channel-order contains an empty channel name";
            return false;
        }
        std::transform(token.begin(), token.end(), token.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        const ChannelName* channel = nullptr;
        for (const ChannelName& candidate : kChannelNames) {
            if (token == candidate.name) { channel = &candidate; break; }
        }
        if (!channel) {
            error = "unknown input channel name \"" + token + "\"";
            return false;
        }
        const std::uint32_t bit = 1u << channel->id;
        if ((layout_mask & bit) == 0u) {
            error = "input channel \"" + token + "\" is absent from --input-layout";
            return false;
        }
        if ((supplied_mask & bit) != 0u) {
            error = "input channel \"" + token + "\" is specified more than once";
            return false;
        }
        supplied_mask |= bit;
        channel_ids.push_back(channel->id);
    }
    if (channel_ids.empty() || supplied_mask != layout_mask) {
        error = "--input-channel-order must name every channel in --input-layout exactly once";
        return false;
    }
    return true;
}

bool derive_layout_from_channel_order(
    const std::string& csv,
    std::uint32_t& layout_mask,
    std::vector<std::uint32_t>& channel_ids,
    std::string& error) {
    error.clear();
    layout_mask = 0u;
    channel_ids.clear();
    if (csv.empty() || csv.front() == ',' || csv.back() == ',') {
        error = "--input-channel-order contains an empty channel name";
        return false;
    }
    std::stringstream input(csv);
    std::string token;
    while (std::getline(input, token, ',')) {
        token.erase(std::remove_if(
            token.begin(), token.end(),
            [](unsigned char c) { return std::isspace(c) != 0; }),
            token.end());
        if (token.empty()) {
            error = "--input-channel-order contains an empty channel name";
            return false;
        }
        std::transform(
            token.begin(), token.end(), token.begin(),
            [](unsigned char c) {
                return static_cast<char>(std::toupper(c));
            });
        const ChannelName* channel = nullptr;
        for (const ChannelName& candidate : kChannelNames) {
            if (token == candidate.name) {
                channel = &candidate;
                break;
            }
        }
        if (!channel) {
            error = "unknown input channel name \"" + token + "\"";
            return false;
        }
        const std::uint32_t bit = std::uint32_t{1} << channel->id;
        if ((layout_mask & bit) != 0u) {
            error = "input channel \"" + token
                + "\" is specified more than once";
            return false;
        }
        layout_mask |= bit;
        channel_ids.push_back(channel->id);
    }
    if (channel_ids.empty()) {
        error = "--input-channel-order contains no channels";
        return false;
    }
    std::uint32_t carrier_layout = 0u;
    if (!codec_v3_carrier_layout(layout_mask, carrier_layout)) {
        error = "channel order forms unsupported codec-v3 original layout mask:0x";
        std::ostringstream mask;
        mask << std::hex << layout_mask;
        error += mask.str();
        return false;
    }
    return true;
}

bool derive_layout_from_wav_channel_mask(
    std::uint32_t wav_mask,
    std::uint32_t& layout_mask,
    std::vector<std::uint32_t>& channel_ids,
    std::string& error) {
    error.clear();
    layout_mask = 0u;
    channel_ids.clear();
    if (wav_mask == 0u) {
        error = "WAVEFORMATEXTENSIBLE channel mask is absent";
        return false;
    }
    static constexpr std::pair<std::uint32_t, std::uint32_t> kWavToCodec[] = {
        {0u, 0u}, {1u, 1u}, {2u, 2u}, {3u, 3u},
        {4u, 7u}, {5u, 8u}, {9u, 4u}, {10u, 5u},
        {12u, 9u}, {14u, 10u}, {15u, 13u}, {17u, 14u},
    };
    std::uint32_t remaining = wav_mask;
    for (const auto [wav_bit, codec_id] : kWavToCodec) {
        const std::uint32_t wav_speaker = std::uint32_t{1} << wav_bit;
        if ((wav_mask & wav_speaker) == 0u)
            continue;
        channel_ids.push_back(codec_id);
        layout_mask |= std::uint32_t{1} << codec_id;
        remaining &= ~wav_speaker;
    }
    if (remaining != 0u || channel_ids.empty()) {
        error = "WAVEFORMATEXTENSIBLE mask contains unsupported speaker bits";
        layout_mask = 0u;
        channel_ids.clear();
        return false;
    }
    std::uint32_t carrier_layout = 0u;
    if (!codec_v3_carrier_layout(layout_mask, carrier_layout)) {
        error = "WAVEFORMATEXTENSIBLE mask forms an unsupported codec-v3 layout";
        layout_mask = 0u;
        channel_ids.clear();
        return false;
    }
    return true;
}

std::string format_channel_order(const std::vector<std::uint32_t>& channel_ids) {
    std::string text;
    for (const std::uint32_t id : channel_ids) {
        const char* name = canonical_channel_name(id);
        if (!name) return {};
        if (!text.empty()) text += ',';
        text += name;
    }
    return text;
}

} // namespace auro3d:encode
