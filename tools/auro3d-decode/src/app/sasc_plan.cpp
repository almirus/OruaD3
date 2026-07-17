#include "sasc_plan.hpp"
#include "cx_gain.hpp"
#include "../auro3deng/codec_dsp/downmix_plan.hpp"

#include <array>
#include <cstdint>
#include <unordered_map>

namespace auro3d {
namespace sasc {

namespace {

struct ScgLayouts {
    std::uint32_t values[3]{};
};

ScgLayouts get_scg_layouts(bool linked_objects, std::uint32_t layout) {
    const std::uint32_t filtered = linked_objects ? layout & 0xFFEFFFF7u : layout;
    ScgLayouts result{};
    result.values[2] = filtered;
    result.values[1] = filtered & 0xFC1011BFu;
    if ((filtered & 0xBFEFFFF7u) == 0u) {
        result.values[0] = 0x40000000u;
    } else if ((filtered & 0xFFEFFFF3u) == 0u) {
        result.values[0] = 4u;
    } else if ((~filtered & 3u) == 0u) {
        result.values[0] = 3u;
    } else {
        result.values[0] = filtered;
    }
    return result;
}

const std::array<std::int64_t, 111>& default_downmix_gains() {
    static const std::array<std::int64_t, 111> gains = [] {
        std::array<std::int64_t, 111> value{};
        const std::int64_t zero_db = cx::gain_to_scaler_q23({0u, 0});
        const std::int64_t minus_3_db = cx::gain_to_scaler_q23({0u, -30});
        const std::int64_t minus_4_5_db = cx::gain_to_scaler_q23({0u, -45});
        const std::int64_t minus_6_db = cx::gain_to_scaler_q23({0u, -60});
        const std::size_t zero_indices[] = {
            0,1,102,103,88,89,6,7,12,13,36,37,38,39,43,44,45,46,47,48,
            52,53,54,55,56,57,61,62,63,64,69,70,90,91,65,66,16,24,40,
            51,58,106,107,108,109,110,81,98,101
        };
        const std::size_t minus_3_indices[] = {
            82,83,75,76,2,3,4,5,8,9,10,11,84,85,32,33,34,35,71,72,73,
            74,92,93,94,95,14,15,41,42,49,50,59,60,77,78,96,97,99,100,
            104,105
        };
        const std::size_t minus_4_5_indices[] = {17,18,19,25,26,27};
        const std::size_t minus_6_indices[] = {79,80,20,21,22,23,28,29,30,31};
        for (const auto index : zero_indices)
            value[index] = zero_db;
        for (const auto index : minus_3_indices)
            value[index] = minus_3_db;
        for (const auto index : minus_4_5_indices)
            value[index] = minus_4_5_db;
        for (const auto index : minus_6_indices)
            value[index] = minus_6_db;
        return value;
    }();
    return gains;
}

// compensate_channel_gains (0x4349A0): truncating Q23 multiply via `/ 0x800000`.
std::int64_t multiply_q23(std::int64_t left, std::int64_t right) {
    return left * right / kUnityGain;
}

// calculate_gains_ (0x436D80) +3576/+3577 post-scale and same-stream
// coefficient update in sub_49EB20: bias negative products then `>> 23`.
std::int64_t multiply_q23_rounded(std::int64_t left, std::int64_t right) {
    const std::int64_t product = left * right;
    const std::int64_t biased = product < 0 ? product + 0x7FFFFF : product;
    return biased >> 23;
}

// details::inv_sqrt (0x433C20): qword_1DB190[n-1] for n in 1..4.
std::int64_t inv_sqrt_q23(unsigned n) {
    static const std::int64_t kTable[4] = {8388608, 5931642, 4843165, 4194304};
    if (n < 1u || n > 4u)
        return 0;
    return kTable[n - 1u];
}

// Downmixer::set_mono_top_ (0x435580). mono_top[0] → gains[16..23];
// mono_top[1] → gains[24..31] (Downmixer byte offsets 160..280, −4).
bool apply_mono_top_downmix(
    const AuroCxMonoTopDownmixInfo& mono,
    bool second_slot,
    std::array<std::int64_t, 111>& gains,
    std::string& error) {
    if (!mono.present)
        return true;
    const std::size_t base = second_slot ? 24u : 16u;
    auto put = [&](std::size_t index, std::int64_t value) {
        if (base + index >= gains.size()) {
            error = "SASC mono-top gain index out of range";
            return false;
        }
        gains[base + index] = value;
        return true;
    };
    if (mono.kind == 2u) {
        if (mono.gains.size() != 1u) {
            error = "SASC mono-top kind2 gain count mismatch";
            return false;
        }
        const std::int64_t g0 = cx::gain_to_scaler_q23(mono.gains[0]);
        if (!put(0, g0))
            return false;
        const std::int64_t s4 = multiply_q23(
            g0, multiply_q23(inv_sqrt_q23(1), inv_sqrt_q23(4)));
        for (std::size_t i = 4; i < 8; ++i)
            if (!put(i, s4))
                return false;
        const std::int64_t s3 = multiply_q23(
            g0, multiply_q23(inv_sqrt_q23(1), inv_sqrt_q23(3)));
        for (std::size_t i = 1; i < 4; ++i)
            if (!put(i, s3))
                return false;
        return true;
    }
    if (mono.kind == 1u) {
        if (mono.gains.size() != 3u) {
            error = "SASC mono-top kind1 gain count mismatch";
            return false;
        }
        const std::int64_t g0 = cx::gain_to_scaler_q23(mono.gains[0]);
        const std::int64_t g1 = cx::gain_to_scaler_q23(mono.gains[1]);
        const std::int64_t g2 = cx::gain_to_scaler_q23(mono.gains[2]);
        if (!put(1, g0) || !put(2, g1) || !put(3, g2))
            return false;
        const std::int64_t sum = g0 + g1 + g2;
        const std::int64_t s4 = multiply_q23(
            sum, multiply_q23(inv_sqrt_q23(3), inv_sqrt_q23(4)));
        for (std::size_t i = 4; i < 8; ++i)
            if (!put(i, s4))
                return false;
        const std::int64_t s1 = multiply_q23(
            sum, multiply_q23(inv_sqrt_q23(3), inv_sqrt_q23(1)));
        return put(0, s1);
    }
    if (mono.kind != 0u) {
        error = "SASC mono-top kind unsupported";
        return false;
    }
    if (mono.gains.size() != 4u) {
        error = "SASC mono-top kind0 gain count mismatch";
        return false;
    }
    const std::int64_t g0 = cx::gain_to_scaler_q23(mono.gains[0]);
    const std::int64_t g1 = cx::gain_to_scaler_q23(mono.gains[1]);
    const std::int64_t g2 = cx::gain_to_scaler_q23(mono.gains[2]);
    const std::int64_t g3 = cx::gain_to_scaler_q23(mono.gains[3]);
    if (!put(4, g0) || !put(5, g1) || !put(6, g2) || !put(7, g3))
        return false;
    const std::int64_t sum = g0 + g1 + g2 + g3;
    const std::int64_t s0 = multiply_q23(
        sum, multiply_q23(inv_sqrt_q23(4), inv_sqrt_q23(1)));
    if (!put(0, s0))
        return false;
    const std::int64_t s3 = multiply_q23(
        sum, multiply_q23(inv_sqrt_q23(4), inv_sqrt_q23(3)));
    return put(1, s3) && put(2, s3) && put(3, s3);
}

// Downmixer::set_stereo_top_ (0x435C50). ChannelDownmix +80 → gains even/odd
// slots 0..6; +104 → slots 8..12. channel_id==29 selects the odd twin slots
// (native a3==29 → +8 byte stride). gains[i] ↔ Downmixer+(32+8*i).
bool apply_stereo_top_downmix(
    const AuroCxStereoTopDownmixInfo& stereo,
    bool high_slot,  // true → +104 block; false → +80 block
    bool odd_channel,  // channel_id == 29
    std::array<std::int64_t, 111>& gains,
    std::string& error) {
    if (!stereo.present)
        return true;
    const std::size_t pair = odd_channel ? 1u : 0u;
    auto put = [&](std::size_t even_index, std::int64_t value) {
        const std::size_t index = even_index + pair;
        if (index >= gains.size()) {
            error = "SASC stereo-top gain index out of range";
            return false;
        }
        gains[index] = value;
        return true;
    };
    if (high_slot) {
        // Native block at ChannelDownmix +104 → a1+96/112/128.
        if (stereo.kind == 2u) {
            if (stereo.gains.size() != 1u) {
                error = "SASC stereo-top high kind2 gain count mismatch";
                return false;
            }
            const std::int64_t g0 = cx::gain_to_scaler_q23(stereo.gains[0]);
            if (!put(12, g0))
                return false;
            const std::int64_t s = multiply_q23(
                g0, multiply_q23(inv_sqrt_q23(1), inv_sqrt_q23(2)));
            return put(8, s) && put(10, s);
        }
        if (stereo.kind != 0u) {
            error = "SASC stereo-top high kind unsupported";
            return false;
        }
        if (stereo.gains.size() != 2u) {
            error = "SASC stereo-top high kind0 gain count mismatch";
            return false;
        }
        const std::int64_t g0 = cx::gain_to_scaler_q23(stereo.gains[0]);
        const std::int64_t g1 = cx::gain_to_scaler_q23(stereo.gains[1]);
        if (!put(8, g0) || !put(10, g1))
            return false;
        const std::int64_t s = multiply_q23(
            g0 + g1, multiply_q23(inv_sqrt_q23(2), inv_sqrt_q23(1)));
        return put(12, s);
    }
    // Native block at ChannelDownmix +80 → a1+32/48/64/80.
    if (stereo.kind == 3u) {
        if (stereo.gains.size() != 1u) {
            error = "SASC stereo-top low kind3 gain count mismatch";
            return false;
        }
        const std::int64_t g0 = cx::gain_to_scaler_q23(stereo.gains[0]);
        if (!put(0, g0))
            return false;
        const std::int64_t s6 = multiply_q23(
            g0, multiply_q23(inv_sqrt_q23(1), inv_sqrt_q23(1)));
        if (!put(6, s6))
            return false;
        const std::int64_t s24 = multiply_q23(
            g0, multiply_q23(inv_sqrt_q23(1), inv_sqrt_q23(2)));
        return put(2, s24) && put(4, s24);
    }
    if (stereo.kind == 2u) {
        if (stereo.gains.size() != 1u) {
            error = "SASC stereo-top low kind2 gain count mismatch";
            return false;
        }
        const std::int64_t g0 = cx::gain_to_scaler_q23(stereo.gains[0]);
        if (!put(6, g0))
            return false;
        const std::int64_t s24 = multiply_q23(
            g0, multiply_q23(inv_sqrt_q23(1), inv_sqrt_q23(2)));
        if (!put(2, s24) || !put(4, s24))
            return false;
        const std::int64_t s0 = multiply_q23(
            g0, multiply_q23(inv_sqrt_q23(1), inv_sqrt_q23(1)));
        return put(0, s0);
    }
    if (stereo.kind != 0u) {
        error = "SASC stereo-top low kind unsupported";
        return false;
    }
    if (stereo.gains.size() != 2u) {
        error = "SASC stereo-top low kind0 gain count mismatch";
        return false;
    }
    const std::int64_t g0 = cx::gain_to_scaler_q23(stereo.gains[0]);
    const std::int64_t g1 = cx::gain_to_scaler_q23(stereo.gains[1]);
    if (!put(2, g0) || !put(4, g1))
        return false;
    const std::int64_t sum = g0 + g1;
    const std::int64_t s6 = multiply_q23(
        sum, multiply_q23(inv_sqrt_q23(2), inv_sqrt_q23(1)));
    if (!put(6, s6))
        return false;
    const std::int64_t s0 = multiply_q23(
        sum, multiply_q23(inv_sqrt_q23(2), inv_sqrt_q23(1)));
    return put(0, s0);
}

// Native auro::cx::downmix::layout_dimension (0x433C60): map
// auro_channel_Layout_dimension through dword_1DC710 = {1,1,2,3}.
std::uint32_t cx_downmix_layout_dimension(std::uint32_t layout) {
    static const std::uint32_t kMap[4] = {1u, 1u, 2u, 3u};
    const std::uint32_t masked = layout & 0xFFFFEFFFu;
    std::uint32_t raw = 0;
    if ((masked & 0xFFEFFFF3u) == 0u)
        raw = 0;
    else if ((masked & 0x33E3FE00u) != 0u)
        raw = 3;
    else if ((masked & 0xC0C01F4u) != 0u)
        raw = 2;
    else
        raw = (masked & 3u) != 0u ? 1u : 0u;
    return raw <= 3u ? kMap[raw] : 0u;
}

// Downmixer::set_layout_independent_gains_ (0x434EA0). Table slots are
// Downmixer qword indices minus 4 (gain table starts at byte +32).
bool apply_layout_independent_gains(
    std::uint32_t channel_id,
    const AuroCxChannelDownmixInfo& downmix,
    std::array<std::int64_t, 111>& gains,
    std::string& error) {
    if (downmix.intra_layer_present) {
        const auto count = downmix.intra_layer_gains.size();
        auto set_slot = [&](std::size_t index, const AuroCxIntegralGainInfo& gain) {
            if (index >= gains.size()) {
                error = "SASC layout-independent gain index out of range";
                return false;
            }
            gains[index] = cx::gain_to_scaler_q23(gain);
            return true;
        };
        switch (channel_id) {
        case 4:
            if (count != 2u ||
                !set_slot(84, downmix.intra_layer_gains[0]) ||
                !set_slot(86, downmix.intra_layer_gains[1])) {
                error = "SASC layout-independent gain count mismatch for channel 4";
                return false;
            }
            break;
        case 5:
            if (count != 2u ||
                !set_slot(85, downmix.intra_layer_gains[0]) ||
                !set_slot(87, downmix.intra_layer_gains[1])) {
                error = "SASC layout-independent gain count mismatch for channel 5";
                return false;
            }
            break;
        case 6:
            if (count != 1u ||
                !set_slot(77, downmix.intra_layer_gains[0]) ||
                !set_slot(78, downmix.intra_layer_gains[0])) {
                error = "SASC layout-independent gain count mismatch for channel 6";
                return false;
            }
            break;
        case 7:
            if (count != 1u || !set_slot(69, downmix.intra_layer_gains[0])) {
                error = "SASC layout-independent gain count mismatch for channel 7";
                return false;
            }
            break;
        case 8:
            if (count != 1u || !set_slot(70, downmix.intra_layer_gains[0])) {
                error = "SASC layout-independent gain count mismatch for channel 8";
                return false;
            }
            break;
        case 15:
            if (count != 1u ||
                !set_slot(41, downmix.intra_layer_gains[0]) ||
                !set_slot(42, downmix.intra_layer_gains[0])) {
                error = "SASC layout-independent gain count mismatch for channel 15";
                return false;
            }
            break;
        case 16:
            if (count != 1u || !set_slot(36, downmix.intra_layer_gains[0])) {
                error = "SASC layout-independent gain count mismatch for channel 16";
                return false;
            }
            break;
        case 17:
            if (count != 1u || !set_slot(37, downmix.intra_layer_gains[0])) {
                error = "SASC layout-independent gain count mismatch for channel 17";
                return false;
            }
            break;
        case 26:
            if (count != 2u ||
                !set_slot(65, downmix.intra_layer_gains[0]) ||
                !set_slot(67, downmix.intra_layer_gains[1])) {
                error = "SASC layout-independent gain count mismatch for channel 26";
                return false;
            }
            break;
        case 27:
            if (count != 2u ||
                !set_slot(66, downmix.intra_layer_gains[0]) ||
                !set_slot(68, downmix.intra_layer_gains[1])) {
                error = "SASC layout-independent gain count mismatch for channel 27";
                return false;
            }
            break;
        default:
            if (count != 0u) {
                error = "SASC layout-independent gains unsupported for channel " +
                        std::to_string(channel_id);
                return false;
            }
            break;
        }
    }
    if ((downmix.gain_present_mask & 1u) != 0u) {
        if (downmix.gains.empty()) {
            error = "SASC channel downmix gain0 missing";
            return false;
        }
        const std::int64_t scaler = cx::gain_to_scaler_q23(downmix.gains[0]);
        switch (channel_id) {
        case 21:
            gains[61] = scaler;
            break;
        case 22:
            gains[62] = scaler;
            break;
        case 23:
            gains[58] = scaler;
            break;
        case 24:
            gains[54] = scaler;
            break;
        case 25:
            gains[55] = scaler;
            break;
        default:
            break;
        }
    }
    return true;
}

// Downmixer::add_2d_to_1d_src_gains_ (0x4354C0) + set_2d_to_1d_src_gains_
// (0x435370) when layout dims are 2d→1d (flag at Downmixer+3577).
// append_: no-height uses gain0; height uses gain1 into the same +2408 table.
bool apply_2d_to_1d_src_gains(
    std::uint32_t channel_id,
    const AuroCxIntegralGainInfo& gain,
    std::array<std::int64_t, 111>& gains,
    std::string& error) {
    constexpr std::uint32_t kAdd2dMask = 786932u;
    if (channel_id > 19u || ((kAdd2dMask >> channel_id) & 1u) == 0u)
        return true;
    const std::int64_t scaler = cx::gain_to_scaler_q23(gain);
    switch (channel_id) {
    case 2:
        gains[96] = scaler;
        gains[97] = scaler;
        break;
    case 4:
        gains[82] = scaler;
        break;
    case 5:
        gains[83] = scaler;
        break;
    case 6:
        gains[79] = scaler;
        gains[80] = scaler;
        break;
    case 7:
        gains[75] = scaler;
        break;
    case 8:
        gains[76] = scaler;
        break;
    case 18:
        gains[90] = scaler;
        break;
    case 19:
        gains[91] = scaler;
        break;
    default:
        break;
    }
    (void)error;
    return true;
}

// Downmixer::add_3d_to_2d_src_gains_ (0x435520) + set_3d_to_2d_src_gains_
// (0x435230) / calculate_ when +3576 (src_dim>=3 && tgt_dim<3).
// Slots are Downmixer qword indices minus 4.
bool apply_3d_to_2d_src_gains(
    std::uint32_t channel_id,
    const AuroCxChannelDownmixInfo& downmix,
    std::array<std::int64_t, 111>& gains,
    std::string& error) {
    if ((downmix.gain_present_mask & 1u) == 0u)
        return true;
    constexpr std::uint32_t kAdd3dMask = 257536u;
    if (channel_id > 0x11u || ((kAdd3dMask >> channel_id) & 1u) == 0u)
        return true;
    if (downmix.gains.empty()) {
        error = "SASC 3d-to-2d source gain missing";
        return false;
    }
    const std::int64_t scaler = cx::gain_to_scaler_q23(downmix.gains[0]);
    switch (channel_id) {
    case 9:
        gains[52] = scaler;
        break;
    case 10:
        gains[53] = scaler;
        break;
    case 11:
        gains[51] = scaler;
        break;
    case 13:
        gains[43] = scaler;
        break;
    case 14:
        gains[44] = scaler;
        break;
    case 15:
        gains[40] = scaler;
        break;
    case 16:
        gains[38] = scaler;
        break;
    case 17:
        gains[39] = scaler;
        break;
    default:
        break;
    }
    return true;
}

// Planner::compute_ (0x49D840) linked-object branch after level-2:
// get_object_group_ref_gain → ObjectRenderer::compute_panning_gains
// (0x4BF010 / ESPCAP RoomCentricPanner) → steps with layer=2.
// Corpus MP4s have object_groups=0; keep syntax/validation and fail only
// at the identified unported ESPCAP panning call.
bool append_linked_object_steps(
    const CxSchemaParseResult& schema,
    const AuroCxSchemaPduInfo& pdu,
    const cx::ReferenceGainSelection& reference_location,
    const std::unordered_map<std::uint32_t, const AuroCxSchemaChannelInfo*>&
        channels,
    const std::unordered_map<std::uint32_t, std::int64_t>& coefficients,
    std::uint32_t layout_for_panning,
    std::vector<Step>& steps,
    std::string& error) {
    if (pdu.sasc_linked_object_groups.empty())
        return true;

    const auto bed_ref =
        cx::get_bed_ref_gain(schema, pdu.sasc_bed_index, reference_location);
    if (bed_ref.selector != 0u) {
        error = "SASC bed reference gain is mute";
        return false;
    }

    for (const auto group_index : pdu.sasc_linked_object_groups) {
        if (group_index >= schema.object_groups.size()) {
            error = "SASC linked object group index out of range";
            return false;
        }
        const auto object_ref =
            cx::get_object_group_ref_gain(schema, group_index, reference_location);
        const auto relative = cx::relative_object_ref_gain(object_ref, bed_ref);
        (void)relative;
        (void)layout_for_panning;
        (void)channels;
        (void)coefficients;
        (void)steps;

        const auto& group = schema.object_groups[group_index];
        for (std::size_t object = 0; object < group.objects.size(); ++object) {
            cx::ResolvedObjectMetadata metadata;
            if (!cx::resolve_object_metadata(
                    schema, group_index, object, metadata)) {
                error = "SASC linked object metadata resolution failed";
                return false;
            }
            // Native emits one ObjectRenderer (296 bytes) per object in
            // Planner::initialize, then compute_panning_gains per object.
            error =
                "SASC linked-object ObjectRenderer::compute_panning_gains "
                "(ESPCAP 0x4BF010) not ported";
            return false;
        }
    }
    return true;
}

bool build_channel_bed_plan(
    const CxSchemaParseResult& schema,
    const AuroCxSchemaPduInfo& pdu,
    const cx::ReferenceGainSelection& reference_location,
    std::vector<Step>& steps,
    std::string& error) {
    if (pdu.sasc_bed_index >= schema.beds.size()) {
        error = "SASC bed index out of range";
        return false;
    }

    const auto& bed = schema.beds[pdu.sasc_bed_index];
    if (bed.ambisonics) {
        error = "SASC channel-bed cannot reference ambisonics";
        return false;
    }

    const bool has_linked_objects = !pdu.sasc_linked_object_groups.empty();
    std::uint32_t source_layout = 0;
    std::unordered_map<std::uint32_t, const AuroCxSchemaChannelInfo*> channels;
    std::unordered_map<std::uint32_t, std::int64_t> coefficients;
    std::array<std::int64_t, 31> dest_post_scale_2d1d{};
    dest_post_scale_2d1d.fill(kUnityGain);
    std::array<std::int64_t, 31> dest_post_scale_3d2d{};
    dest_post_scale_3d2d.fill(kUnityGain);
    bool has_height_source = false;
    for (const auto& channel : bed.channels) {
        if (channel.layer_index > pdu.sasc_channel_bed_layer)
            continue;
        if (channel.id >= 31u || channels.count(channel.id)) {
            error = "SASC channel-bed layout is invalid";
            return false;
        }
        source_layout |= std::uint32_t{1} << channel.id;
        channels.emplace(channel.id, &channel);
        coefficients.emplace(channel.audio_stream_index, kUnityGain);
    }
    if (!source_layout) {
        error = "SASC channel-bed layout is empty";
        return false;
    }
    // Downmixer::initialize (0x4363D0): byte+1 set when source has height bits.
    has_height_source = (source_layout & 0x33E3FE00u) != 0u;

    std::array<std::int64_t, 111> base_gains = default_downmix_gains();
    for (const auto& entry : channels) {
        const auto& channel = *entry.second;
        if (!channel.downmix_present)
            continue;
        // Downmixer::append_ (0x4365F0): set_mono_top_ (0x435580) then
        // set_stereo_top_ (0x435C50); layout-independent + 2d/3d gains follow.
        if (channel.downmix.mono_top.size() >= 1u &&
            !apply_mono_top_downmix(
                channel.downmix.mono_top[0], false, base_gains, error))
            return false;
        if (channel.downmix.mono_top.size() >= 2u &&
            !apply_mono_top_downmix(
                channel.downmix.mono_top[1], true, base_gains, error))
            return false;
        // set_stereo_top_ (0x435C50): schema types #3/#4 for channels 28/29.
        // stereo_top[0]=+80, stereo_top[1]=+104; id 29 selects odd slots.
        if (channel.downmix.stereo_top.size() >= 1u &&
            !apply_stereo_top_downmix(
                channel.downmix.stereo_top[0],
                false,
                channel.id == 29u,
                base_gains,
                error))
            return false;
        if (channel.downmix.stereo_top.size() >= 2u &&
            !apply_stereo_top_downmix(
                channel.downmix.stereo_top[1],
                true,
                channel.id == 29u,
                base_gains,
                error))
            return false;
        if (!apply_layout_independent_gains(
                channel.id, channel.downmix, base_gains, error))
            return false;
        if ((channel.downmix.gain_present_mask & 1u) != 0u) {
            if (channel.downmix.gains.empty()) {
                error = "SASC channel downmix gain0 missing";
                return false;
            }
            const std::int64_t scaler =
                cx::gain_to_scaler_q23(channel.downmix.gains[0]);
            if (!has_height_source) {
                // append_ LABEL_27: 2d→1d destination post-scale at +1416.
                if (channel.id <= 1u)
                    dest_post_scale_2d1d[channel.id] = scaler;
            } else {
                // append_ height+gain0: mask 202113527 → +920 post-scale
                // (calculate_gains_ when +3576). Mask 257536 → +1912 source
                // overrides applied per 3d→2d layer via set_3d_to_2d.
                constexpr std::uint32_t kPost3dMask = 202113527u;
                if (channel.id < 31u &&
                    ((kPost3dMask >> channel.id) & 1u) != 0u)
                    dest_post_scale_3d2d[channel.id] = scaler;
            }
        }
        // append_ height+a2[45] (gain1): mask 786932 → +2408 (2d→1d source
        // table); ch<=1 → +1416 destination post-scale.
        if (has_height_source &&
            (channel.downmix.gain_present_mask & 2u) != 0u) {
            if (channel.downmix.gains.size() < 2u) {
                error = "SASC channel downmix gain1 missing";
                return false;
            }
            if (channel.id <= 1u)
                dest_post_scale_2d1d[channel.id] =
                    cx::gain_to_scaler_q23(channel.downmix.gains[1]);
        }
    }

    // Planner::initialize (0x49D3F0): get_layouts(linked!=0, bed_layout).
    // Linked beds mask with 0xFFEFFFF7 before deriving SCG levels.
    const ScgLayouts layouts =
        get_scg_layouts(has_linked_objects, source_layout);
    std::uint32_t current_layout = source_layout;
    // Planner::compute_ (0x49D840) calls sub_49EB20 for levels 2/1/0, then
    // inserts linked-object panning steps between level 2 and level 1.
    // Without linked objects level 2 is identity (source==target) and is
    // skipped; with the linked mask it may demote bits before objects run.
    // Levels 1/0 use the Downmixer residual-mask check inside
    // cx_downmix_engine_plan (not auro_downmix_v1 plan[159]).
    for (int layer = 2; layer >= 0; --layer) {
        if (layer == 1) {
            if (!append_linked_object_steps(
                    schema,
                    pdu,
                    reference_location,
                    channels,
                    coefficients,
                    layouts.values[2],
                    steps,
                    error))
                return false;
        }
        const std::uint32_t target_layout = layouts.values[layer];
        if (current_layout == target_layout)
            continue;
        std::array<std::int64_t, 111> layer_gains = base_gains;
        const std::uint32_t src_dim = cx_downmix_layout_dimension(current_layout);
        const std::uint32_t tgt_dim = cx_downmix_layout_dimension(target_layout);
        // calculate_: +3576 = src>=3 && tgt<3; +3577 = src>=2 && tgt<2.
        const bool from_3d = src_dim >= 3u && tgt_dim < 3u;
        const bool to_1d = src_dim >= 2u && tgt_dim < 2u;
        if (from_3d) {
            for (const auto& entry : channels) {
                const auto& channel = *entry.second;
                if (!channel.downmix_present)
                    continue;
                if (!apply_3d_to_2d_src_gains(
                        channel.id, channel.downmix, layer_gains, error))
                    return false;
            }
        }
        if (to_1d) {
            for (const auto& entry : channels) {
                const auto& channel = *entry.second;
                if (!channel.downmix_present)
                    continue;
                // append_: no-height fills +2408 from gain0; height from gain1.
                const unsigned gain_index = has_height_source ? 1u : 0u;
                if ((channel.downmix.gain_present_mask & (1u << gain_index)) ==
                    0u)
                    continue;
                if (channel.downmix.gains.size() <= gain_index) {
                    error = "SASC 2d-to-1d source gain missing";
                    return false;
                }
                if (!apply_2d_to_1d_src_gains(
                        channel.id,
                        channel.downmix.gains[gain_index],
                        layer_gains,
                        error))
                    return false;
            }
        }
        std::vector<auro3deng::DownmixRule> rules;
        std::uint32_t output_layout = 0;
        if (!auro3deng::cx_downmix_engine_plan(
                current_layout, target_layout, rules, output_layout) ||
            output_layout != target_layout) {
            error = "SASC Downmixer plan failed layer=" +
                    std::to_string(layer) + " source=" +
                    std::to_string(current_layout) + " target=" +
                    std::to_string(target_layout) + " output=" +
                    std::to_string(output_layout);
            return false;
        }
        for (const auto& rule : rules) {
            const auto source = channels.find(rule.source_channel);
            const auto destination = channels.find(rule.destination_channel);
            if (source == channels.end() || destination == channels.end() ||
                rule.gain_index >= layer_gains.size()) {
                error = "SASC downmix rule channel is unavailable";
                return false;
            }
            const std::uint32_t source_stream = source->second->audio_stream_index;
            const std::uint32_t destination_stream = destination->second->audio_stream_index;
            auto source_coefficient = coefficients.find(source_stream);
            auto destination_coefficient = coefficients.find(destination_stream);
            if (source_coefficient == coefficients.end() ||
                destination_coefficient == coefficients.end() ||
                destination_coefficient->second == 0) {
                error = "SASC downmix coefficient is unavailable";
                return false;
            }

            AuroCxIntegralGainInfo relative{};
            relative.selector = source->second->gain.selector ? 1u : 0u;
            relative.value = source->second->gain.value - destination->second->gain.value;
            if (destination->second->gain.selector != 0u) {
                error = "SASC destination channel gain is mute";
                return false;
            }
            // Native order: calculate_gains_ applies +920/+1416 with rounded
            // Q23, then compensate_channel_gains multiplies relative with `/`.
            std::int64_t gain = layer_gains[rule.gain_index];
            if (from_3d &&
                rule.destination_channel < dest_post_scale_3d2d.size())
                gain = multiply_q23_rounded(
                    gain, dest_post_scale_3d2d[rule.destination_channel]);
            if (to_1d &&
                rule.destination_channel < dest_post_scale_2d1d.size())
                gain = multiply_q23_rounded(
                    gain, dest_post_scale_2d1d[rule.destination_channel]);
            gain = multiply_q23(gain, cx::gain_to_scaler_q23(relative));

            Step step{};
            step.src_stream = source_stream;
            step.dst_stream = destination_stream;
            step.layer = static_cast<std::uint32_t>(layer);
            if (source_stream == destination_stream) {
                source_coefficient->second = multiply_q23_rounded(
                    source_coefficient->second, gain);
                step.gain = static_cast<std::int32_t>(gain);
            } else {
                const std::int64_t numerator = gain * source_coefficient->second;
                const std::int64_t rounded = numerator < 0
                    ? numerator + 0x7FFFFF
                    : numerator;
                step.gain = static_cast<std::int32_t>(
                    (rounded & ~std::int64_t{0x7FFFFF}) /
                    destination_coefficient->second);
            }
            steps.push_back(step);
        }
        current_layout = output_layout;
    }
    return true;
}

bool validate_object_metadata(
    const CxSchemaParseResult& schema,
    std::uint32_t group_index,
    std::string& error) {
    if (group_index >= schema.object_groups.size()) {
        error = "SASC object group index out of range";
        return false;
    }
    for (std::size_t object = 0;
         object < schema.object_groups[group_index].objects.size(); ++object) {
        cx::ResolvedObjectMetadata metadata;
        if (!cx::resolve_object_metadata(schema, group_index, object, metadata)) {
            error = "SASC object metadata resolution failed";
            return false;
        }
    }
    return true;
}

} // namespace

bool build_plans(
    const CxSchemaParseResult& schema,
    std::uint32_t object_groups,
    std::uint16_t declared_layout,
    bool has_declared_layout,
    std::vector<std::vector<Step>>& plans,
    std::uint32_t& scratch_stream,
    std::string& error) {
    if (plans.size() != schema.pdus.size())
        plans.assign(schema.pdus.size(), {});
    scratch_stream = UINT32_MAX;
    error.clear();
    (void)object_groups;
    (void)declared_layout;
    (void)has_declared_layout;

    for (std::size_t pdu_index = 0; pdu_index < schema.pdus.size(); ++pdu_index) {
        const auto& pdu = schema.pdus[pdu_index];
        if (pdu.type != 6) {
            plans[pdu_index].clear();
            continue;
        }
        auto& steps = plans[pdu_index];

    if (pdu.header_value == 1 && !pdu.sasc_channel_bed_decoded) {
        error = "SASC channel_bed metadata missing from schema";
        return false;
    }

    if (pdu.sasc_channel_set_stream_indices.size() !=
        pdu.sasc_channel_sets.size()) {
        error = "SASC SCG stream assignment level mismatch";
        return false;
    }
    for (std::size_t level_index = 0;
         level_index < pdu.sasc_channel_sets.size(); ++level_index) {
        const auto& level = pdu.sasc_channel_sets[level_index];
        if (level.size() != pdu.sasc_audio_stream_indices.size()) {
            error = "SASC SCG channel mask size mismatch";
            return false;
        }
        const auto& indices = pdu.sasc_channel_set_stream_indices[level_index];
        if (indices.size() != level.size()) {
            error = "SASC SCG stream assignment size mismatch";
            return false;
        }
        for (std::size_t channel = 0; channel < level.size(); ++channel) {
            if (level[channel] == (indices[channel] == UINT32_MAX)) {
                error = "SASC SCG stream assignment mask mismatch";
                return false;
            }
        }
    }

    if (pdu.header_value == 1 && schema.beds.empty() && !schema.bed_channels_decoded) {
        error = "SASC channel_bed requires explicit schema channels";
        return false;
    }

    cx::ReferenceGainSelection reference_location;
    if (pdu.header_value == 1) {
        if (!cx::find_reference_location(
                schema,
                pdu.sasc_bed_index,
                pdu.sasc_linked_object_groups,
                reference_location)) {
            error = "SASC channel-bed reference location not found";
            return false;
        }
    }

    if (pdu.header_value == 0 &&
        !validate_object_metadata(schema, pdu.sasc_object_group_index, error))
        return false;
    for (const auto group : pdu.sasc_linked_object_groups)
        if (!validate_object_metadata(schema, group, error))
            return false;

    // Planner::initialize (0x49D3F0) invokes compute_ only for the channel-bed
    // configuration flag. Processor keeps the SCG context at +168 and run_
    // (0x492AA0) decodes it on later AUs without recreating it. Therefore an
    // AU with config_flag=0 retains the preceding plan; its channel-set vectors
    // are still consumed by the sample-rate decoder.
    if (!pdu.sasc_config_flag)
        continue;

    steps.clear();

    // header_value 0 = object-group PDU: not Planner::compute_ (needs bed).
    // header_value 2 = ambisonics bed: rejected inside build_channel_bed_plan.
    if (pdu.header_value != 1) {
        error =
            "SASC SCG config_flag plan requires channel-bed PDU "
            "(header_value=1); object-group/ambisonics planner not ported";
        return false;
    }
    if (!build_channel_bed_plan(
            schema, pdu, reference_location, steps, error))
        return false;
    }
    return true;
}

} // namespace sasc
} // namespace auro3d
