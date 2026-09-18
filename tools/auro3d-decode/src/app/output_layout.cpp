#include "output_layout.hpp"

#include "decoder.hpp"
#include "../auro3deng/detail/codec_v3.hpp"
#include "../auro3deng/detail/runtime_api.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace auro3d {
namespace {

std::string to_lower_ascii(std::string s) {
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string normalize_layout_name(std::string s) {
    s = to_lower_ascii(std::move(s));
    // 5.1.4 7.1.4 → 5.1_4h 7.1_4h style used by auro_channel_layout_to_string.
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        if (c == '.' && i + 1 < s.size() && std::isdigit(static_cast<unsigned char>(s[i + 1]))) {
            // Keep first two dots of "5.1" "7.1"; turn height/top dots into '_'.
            const std::size_t dots = static_cast<std::size_t>(
                std::count(out.begin(), out.end(), '.'));
            if (dots >= 1)
                out.push_back('_');
            else
                out.push_back('.');
            continue;
        }
        if (c == '+' || c == ' ')
            continue;
        out.push_back(c);
    }
    return out;
}

unsigned popcount_u32(std::uint32_t v) {
    unsigned n = 0;
    while (v != 0) {
        n += static_cast<unsigned>(v & 1u);
        v >>= 1u;
    }
    return n;
}

bool try_parse_unsigned(const std::string& text, unsigned long long& value) {
    if (text.empty())
        return false;
    try {
        std::size_t idx = 0;
        value = std::stoull(text, &idx, 0);
        return idx == text.size();
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace

std::uint32_t legacy_dsp_channel_count_to_mask(unsigned channel_count) {
    switch (channel_count) {
    case 1: return 1u;          // 1.0 (C-less FL-only shorthand → FL)
    case 2: return 3u;          // 2.0
    case 3: return 11u;         // 2.1
    case 4: return 51u;         // 4.0
    case 5: return 59u;         // 4.1
    case 6: return 63u;         // 5.1
    case 7: return 127u;        // 6.1
    case 8: return 447u;        // 7.1
    case 9: return 26167u;      // 5.0_4H
    case 10: return 26175u;     // 5.1_4H
    case 11: return 30271u;     // 5.1_4H_1T
    case 12: return 26559u;     // 7.1_4H
    case 13: return 30655u;     // 7.1_4H_1T
    case 14: return 32703u;     // 7.1_5H_1T
    default: return 0u;
    }
}

bool auro_channel_layout_from_string(const std::string& name, std::uint32_t& mask) {
    const std::string key = normalize_layout_name(name);
    if (key.empty())
        return false;

    struct Alias {
        const char* name;
        std::uint32_t mask;
    };
    static constexpr Alias kAliases[] = {
        {"2.0", 3u},
        {"stereo", 3u},
        {"1.0", 4u},
        {"mono", 4u},
        {"3.0", 7u},
        {"0.1", 8u},
        {"2.1", 11u},
        {"1.1", 12u},
        {"3.1", 15u},
        {"4.0", 51u},
        {"5.0", 55u},
        {"4.1", 59u},
        {"5.1", 63u},
        {"lcrs", 71u},
        {"6.0", 119u},
        {"6.1", 127u},
        {"7.0", 439u},
        {"7.1", 447u},
        {"2.0_2h", 1539u},
        {"4.0_2h", 1587u},
        {"5.1_2h", 1599u},
        {"7.1_2h", 1983u},
        {"4.0_4h", 26163u},
        {"8.0", 26163u},
        {"auro8.0", 26163u},
        {"5.0_4h", 26167u},
        {"5.1_4h", 26175u},
        {"5.1_4", 26175u},
        {"5.1.4", 26175u},
        {"9.1", 26175u},
        {"auro9.1", 26175u},
        {"7.1_4h", 26559u},
        {"7.1_4", 26559u},
        {"7.1.4", 26559u},
        {"11.1", 26559u},
        {"auro11.1", 26559u},
        {"5.1_4h_1t", 30271u},
        {"7.1_4h_1t", 30655u},
        {"5.1_5h_1t", 32319u},
        {"7.1_5h_1t", 32703u},
        {"7.1_5", 32703u},
        {"7.1.5", 32703u},
        {"13.1", 32703u},
        {"auro13.1", 32703u},
    };
    for (const Alias& a : kAliases) {
        if (key == a.name) {
            mask = a.mask;
            return true;
        }
    }

    // Exhaustive reverse of auro_channel_layout_to_string known masks.
    static constexpr std::uint32_t kKnown[] = {
        3u, 4u, 7u, 8u, 11u, 12u, 15u, 51u, 55u, 59u, 63u, 71u, 119u, 127u,
        435u, 439u, 443u, 447u, 1539u, 1543u, 1547u, 1551u, 1587u, 1591u, 1595u,
        1599u, 1971u, 1975u, 1979u, 1983u, 3591u, 3599u, 26163u, 26167u, 26171u,
        26175u, 26547u, 26551u, 26555u, 26559u, 28211u, 28215u, 28219u, 28223u,
        28595u, 28599u, 28603u, 28607u, 30259u, 30263u, 30267u, 30271u, 30643u,
        30647u, 30651u, 30655u, 32307u, 32311u, 32315u, 32319u, 32691u, 32695u,
        32699u, 32703u, 805332543u, 805332927u, 805334591u, 805334975u,
        1006659519u, 1006661567u, 0xFFFFFFu, 15728631u, 56649216u, 2052u, 6148u,
    };
    for (const std::uint32_t candidate : kKnown) {
        const char* label = auro_channel_layout_to_string(candidate);
        if (!label || !label[0])
            continue;
        if (normalize_layout_name(label) == key) {
            mask = candidate;
            return true;
        }
    }
    return false;
}

bool parse_dsp_output_layout_arg(
    const std::string& arg,
    DspOutputLayoutRequest& out,
    std::string& error) {
    out = {};
    out.raw = arg;
    if (arg.empty() || arg == "0" || arg == "auto") {
        out.specified = false;
        return true;
    }

    out.specified = true;
    std::uint32_t named_mask = 0;
    if (auro_channel_layout_from_string(arg, named_mask)) {
        out.mask = named_mask;
        out.mask_resolved = true;
        out.legacy_count = popcount_u32(named_mask);
        return true;
    }

    unsigned long long value = 0;
    if (!try_parse_unsigned(arg, value)) {
        error = "unrecognized output layout \"" + arg
            + "\" (use layout name, 0x mask, or legacy channel count 1..14)";
        return false;
    }
    if (value > 0xFFFFFFFFull) {
        error = "output layout value out of range (uint32 mask expected)";
        return false;
    }

    const bool hex_prefixed =
        arg.size() > 2
        && arg[0] == '0'
        && (arg[1] == 'x' || arg[1] == 'X');

    if (!hex_prefixed && value >= 1ul && value <= 14ul) {
        out.is_legacy_count = true;
        out.legacy_count = static_cast<unsigned>(value);
        out.mask = legacy_dsp_channel_count_to_mask(out.legacy_count);
        out.mask_resolved = out.mask != 0u;
        if (!out.mask_resolved) {
            error = "unsupported legacy --dsp-output-channels count";
            return false;
        }
        return true;
    }

    out.mask = static_cast<std::uint32_t>(value);
    out.mask_resolved = true;
    out.legacy_count = popcount_u32(out.mask);
    if (out.mask == 0u) {
        out.specified = false;
        out.mask_resolved = false;
    }
    return true;
}

bool codec_v3_output_layout_mask_allowed(std::uint32_t mask) {
    return mask != 0u && (mask & ~kCodecV3MaxAllowedOutputLayout) == 0u;
}

bool is_compatible_post_dematrix_upmix(
    std::uint32_t stream_layout,
    std::uint32_t requested) {
    if (!codec_v3_output_layout_mask_allowed(stream_layout)
        || !codec_v3_output_layout_mask_allowed(requested)) {
        return false;
    }
    if (requested == stream_layout)
        return false;
    if ((requested & stream_layout) != stream_layout)
        return false;

    const char* stream_name = auro_channel_layout_to_string(stream_layout);
    const char* request_name = auro_channel_layout_to_string(requested);
    if (!stream_name || !stream_name[0] || !request_name || !request_name[0])
        return false;

    const std::uint32_t additions = requested & ~stream_layout;
    // XinN height-oriented additions by prepare mode from the *decoded* bed.
    const std::uint32_t mode =
        auro3deng::xinn_prepare_mode_from_input_mask_portable(stream_layout);
    const std::uint32_t xinn_additions = mode == 1u
        ? 0x6630u
        : (mode == 2u ? 0x7E00u : 0u);
    // Same simple bed holes as legacy Auro-Matic path (C average, silent LFE).
    constexpr std::uint32_t kBedSynth =
        (1u << auro_codec_v3::kAuroChMapSlotFrontCenter)
        | (1u << auro_codec_v3::kAuroChMapSlotLfe);
    const std::uint32_t allowed = xinn_additions | kBedSynth;
    return (additions & ~allowed) == 0u;
}

std::uint32_t auro_cx_api_supported_layout_mask(std::uint32_t requested) {
    // Port of auro:cx:object_renderer:speaker_layout:get_api_supported_layout_cicp
    // Returns the largest API layout whose bits are all set in requested.
    const std::uint32_t m = requested;
    const std::uint8_t lo = static_cast<std::uint8_t>(m);
    const std::uint8_t nlo = static_cast<std::uint8_t>(~lo);

    if ((~m & 0x7FB7u) == 0u)
        return 32703u;
    if ((~m & 0x67B7u) == 0u)
        return 26559u;
    if ((~m & 0x7E37u) == 0u)
        return 32319u;
    if ((~m & 0x7637u) == 0u)
        return 30271u;
    if ((~m & 0x6637u) == 0u)
        return 26175u;
    if ((~m & 0x6633u) == 0u)
        return 26163u;
    if ((~m & 0x633u) == 0u)
        return 1587u;
    if ((~m & 0x1B7u) == 0u)
        return 447u;
    if ((nlo & 0x77u) == 0u)
        return 127u;
    if ((nlo & 0x37u) == 0u)
        return 55u;
    if ((nlo & 0x47u) == 0u)
        return 71u;
    if ((~m & 0x603u) == 0u)
        return 1539u;
    if ((nlo & 0x33u) == 0u)
        return 51u;
    if ((nlo & 0x07u) == 0u)
        return 7u;
    if ((nlo & 0x03u) == 0u)
        return 3u;
    if ((lo & 0x04u) != 0u)
        return 4u;
    if ((m & 0x40000000u) != 0u)
        return 0x40000000u;
    return 0u;
}

bool auro_cx_output_layout_mask_allowed(std::uint32_t mask) {
    if (mask == 0u)
        return false;
    const std::uint32_t canonical = auro_cx_api_supported_layout_mask(mask);
    return canonical != 0u && canonical == mask;
}

std::string format_layout_label(std::uint32_t mask) {
    const char* name = auro_channel_layout_to_string(mask);
    std::ostringstream oss;
    if (name && name[0])
        oss << name;
    else
        oss << "custom";
    oss << " (0x" << std::hex << mask << std::dec << ", " << popcount_u32(mask) << " ch)";
    return oss.str();
}

std::string codec_v3_invalid_output_layout_message(
    std::uint32_t requested_mask,
    std::uint32_t stream_layout_mask,
    unsigned stream_channels) {
    std::ostringstream oss;
    oss << "Codec-v3: invalid output layout " << format_layout_label(requested_mask);
    if (!codec_v3_output_layout_mask_allowed(requested_mask)) {
        oss << "; exceeds ACV3 max_allowed_output_layout 0x"
            << std::hex << kCodecV3MaxAllowedOutputLayout << std::dec
            << " (bits above channel 14 are not allowed)";
        return oss.str();
    }
    if (stream_layout_mask != 0u) {
        oss << "; stream encodes " << format_layout_label(stream_layout_mask)
            << " — dematrix cannot expand to a different bed/height set";
    } else if (stream_channels != 0u) {
        oss << "; not supported for this " << stream_channels
            << "-channel input (legacy upmix: 6=5.1, 10=5.1_4H, 12=7.1_4H)";
    }
    return oss.str();
}

std::string auro_cx_invalid_output_layout_message(std::uint32_t requested_mask) {
    std::ostringstream oss;
    oss << "AuroCX: invalid output layout " << format_layout_label(requested_mask)
        << "; not in ObjectRenderer API-supported CICP set "
           "(see speaker_layout::get_api_supported_layout_cicp)";
    const std::uint32_t canonical = auro_cx_api_supported_layout_mask(requested_mask);
    if (canonical != 0u && canonical != requested_mask) {
        oss << "; closest supported is " << format_layout_label(canonical);
    }
    return oss.str();
}

std::string auro_cx_unsupported_remap_message(
    std::uint32_t stream_layout_mask,
    std::uint32_t requested_mask) {
    std::ostringstream oss;
    oss << "AuroCX: remapping stream layout " << format_layout_label(stream_layout_mask)
        << " to " << format_layout_label(requested_mask)
        << " is not supported; omit --dsp-output-channels/--dsp-output-layout "
           "or request the stream layout exactly";
    return oss.str();
}

} // namespace auro3d
