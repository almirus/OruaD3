#include "cx_decode.hpp"
#include "cx_probe.hpp"
#include "cx_bits.hpp"
#include "awc_lossless.hpp"
#include "awc_transparent.hpp"
#include "decoder.hpp"
#include "lfe_decode.hpp"
#include "sasc_apply.hpp"
#include "sasc_plan.hpp"
#include "sasc_resample.hpp"

#include "../io/wav_writer.hpp"
#include "../render/binaural_renderer.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <utility>
#include <vector>

namespace auro3d {
namespace {

std::uint16_t be16(const std::uint8_t* p) { return std::uint16_t((p[0] << 8) | p[1]); }
std::uint32_t be32(const std::uint8_t* p) {
    return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) | (std::uint32_t(p[2]) << 8) | p[3];
}
std::uint64_t be64(const std::uint8_t* p) { return (std::uint64_t(be32(p)) << 32) | be32(p + 4); }
bool is_box(const std::uint8_t* p, const char* s) {
    return p[0] == s[0] && p[1] == s[1] && p[2] == s[2] && p[3] == s[3];
}

struct Box {
    std::uint64_t payload = 0, end = 0;
    const std::uint8_t* type = nullptr;
};

bool next_box(const std::vector<std::uint8_t>& d, std::uint64_t& c, std::uint64_t lim, Box& b) {
    if (c + 8 > lim || lim > d.size())
        return false;
    std::uint64_t z = be32(d.data() + c), h = 8;
    b.type = d.data() + c + 4;
    if (z == 1) {
        if (c + 16 > lim)
            return false;
        z = be64(d.data() + c + 8);
        h = 16;
    } else if (z == 0)
        z = lim - c;
    if (z < h || z > lim - c)
        return false;
    b.payload = c + h;
    b.end = c + z;
    c = b.end;
    return true;
}

struct SampleToChunk { std::uint32_t first_chunk = 0, samples_per_chunk = 0; };

struct CxTrack {
    bool audio = false, cx = false;
    std::uint16_t channels = 0;
    std::uint16_t sample_bits = 0;
    std::uint32_t rate = 0, count = 0, duration = 0;
    std::vector<std::uint32_t> sizes;
    std::vector<std::uint64_t> chunk_offsets, offsets;
    std::vector<SampleToChunk> sample_to_chunk;
    std::vector<std::uint8_t> acxd;
};

void read_stsd(const std::vector<std::uint8_t>& d, const Box& b, CxTrack& t) {
    if (b.payload + 8 > b.end)
        return;
    std::uint64_t c = b.payload + 8;
    Box e{};
    if (!next_box(d, c, b.end, e) || !is_box(e.type, "a3ds"))
        return;
    t.cx = true;
    if (e.payload + 28 <= e.end) {
        t.channels = be16(d.data() + e.payload + 16);
        t.sample_bits = be16(d.data() + e.payload + 18);
        t.rate = be32(d.data() + e.payload + 24) >> 16;
    }
    c = e.payload + 28;
    while (c + 8 <= e.end) {
        Box x{};
        if (!next_box(d, c, e.end, x))
            break;
        if (is_box(x.type, "acxd"))
            t.acxd.assign(d.begin() + x.payload, d.begin() + x.end);
    }
}

void read_leaf(const std::vector<std::uint8_t>& d, const Box& b, CxTrack& t) {
    // Only mdia/hdlr=soun marks audio. Nested minf/hdlr (e.g. "url ") must not clear it.
    if (is_box(b.type, "hdlr") && b.payload + 12 <= b.end &&
        is_box(d.data() + b.payload + 8, "soun"))
        t.audio = true;
    else if (is_box(b.type, "stsd"))
        read_stsd(d, b, t);
    else if (is_box(b.type, "stts") && b.payload + 16 <= b.end && be32(d.data() + b.payload + 4)) {
        t.count = be32(d.data() + b.payload + 8);
        t.duration = be32(d.data() + b.payload + 12);
    } else if (is_box(b.type, "stsz") && b.payload + 12 <= b.end) {
        const auto u = be32(d.data() + b.payload + 4);
        const auto n = be32(d.data() + b.payload + 8);
        t.count = n;
        if (u)
            t.sizes.assign(n, u);
        else if (b.payload + 12ull + 4ull * n <= b.end)
            for (std::uint32_t i = 0; i < n; ++i)
                t.sizes.push_back(be32(d.data() + b.payload + 12 + 4ull * i));
    } else if (is_box(b.type, "stsc") && b.payload + 8 <= b.end) {
        const auto n = be32(d.data() + b.payload + 4);
        if (b.payload + 8ull + 12ull * n <= b.end)
            for (std::uint32_t i = 0; i < n; ++i) {
                const auto* p = d.data() + b.payload + 8 + 12ull * i;
                t.sample_to_chunk.push_back({be32(p), be32(p + 4)});
            }
    } else if ((is_box(b.type, "stco") || is_box(b.type, "co64")) && b.payload + 8 <= b.end) {
        const bool wide = is_box(b.type, "co64");
        const auto n = be32(d.data() + b.payload + 4);
        if (b.payload + 8ull + (wide ? 8ull : 4ull) * n <= b.end)
            for (std::uint32_t i = 0; i < n; ++i) {
                const auto* p = d.data() + b.payload + 8 + (wide ? 8ull : 4ull) * i;
                t.chunk_offsets.push_back(wide ? be64(p) : be32(p));
            }
    }
}

void walk_boxes(const std::vector<std::uint8_t>& d, std::uint64_t a, std::uint64_t z, CxTrack& t) {
    std::uint64_t c = a;
    while (c + 8 <= z) {
        Box b{};
        if (!next_box(d, c, z, b))
            break;
        read_leaf(d, b, t);
        if (is_box(b.type, "mdia") || is_box(b.type, "minf") || is_box(b.type, "stbl"))
            walk_boxes(d, b.payload, b.end, t);
    }
}

void build_offsets(CxTrack& t) {
    t.offsets.clear();
    if (t.sizes.empty() || t.chunk_offsets.empty() || t.sample_to_chunk.empty())
        return;
    t.offsets.reserve(t.sizes.size());
    std::size_t sample = 0, entry = 0;
    for (std::size_t chunk = 0; chunk < t.chunk_offsets.size() && sample < t.sizes.size(); ++chunk) {
        const std::uint32_t chunk_number = static_cast<std::uint32_t>(chunk + 1);
        while (entry + 1 < t.sample_to_chunk.size() && t.sample_to_chunk[entry + 1].first_chunk <= chunk_number)
            ++entry;
        std::uint64_t offset = t.chunk_offsets[chunk];
        for (std::uint32_t n = 0; n < t.sample_to_chunk[entry].samples_per_chunk && sample < t.sizes.size(); ++n, ++sample) {
            t.offsets.push_back(offset);
            offset += t.sizes[sample];
        }
    }
}

bool find_track(const std::vector<std::uint8_t>& d, CxTrack& t) {
    std::uint64_t c = 0;
    while (c + 8 <= d.size()) {
        Box r{};
        if (!next_box(d, c, d.size(), r))
            break;
        if (!is_box(r.type, "moov"))
            continue;
        std::uint64_t q = r.payload;
        while (q + 8 <= r.end) {
            Box b{};
            if (!next_box(d, q, r.end, b))
                break;
            if (!is_box(b.type, "trak"))
                continue;
            CxTrack candidate{};
            walk_boxes(d, b.payload, b.end, candidate);
            if (candidate.audio && candidate.cx) {
                build_offsets(candidate);
                t = std::move(candidate);
                return true;
            }
        }
    }
    return false;
}

bool file_starts_with_ftyp(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;
    unsigned char hdr[8]{};
    if (!file.read(reinterpret_cast<char*>(hdr), 8))
        return false;
    return hdr[4] == 'f' && hdr[5] == 't' && hdr[6] == 'y' && hdr[7] == 'p';
}

awc::AwcPayloadConfig awc_config_from_pdu(
    const AuroCxSchemaPduInfo& pdu,
    std::uint8_t error_scale_byte) {
    awc::AwcPayloadConfig cfg{};
    cfg.common_preamble = pdu.awc_common_preamble;
    cfg.stream_parameters = pdu.awc_stream_parameters;
    cfg.frame_divisor = pdu.header_flag1 ? (pdu.header_value ? 4u : 2u) : 1u;
    if (!pdu.awc_payload_config_decoded && pdu.payload_bits >= 9 && pdu.audio_stream_count) {
        cx::Bits payload{pdu.payload_data};
        std::uint32_t preamble = 0;
        if (payload.get(9, preamble)) {
            cfg.common_preamble = preamble;
            cfg.stream_parameters.clear();
            for (std::uint32_t s = 0; s < pdu.audio_stream_count; ++s) {
                std::uint32_t parameter = 0;
                if (!payload.get(4, parameter))
                    break;
                cfg.stream_parameters.push_back(parameter);
            }
        }
    }
    cfg.error_scale_byte = error_scale_byte;
    if (cfg.common_preamble)
        cfg.partition_threshold = static_cast<std::uint16_t>(cfg.common_preamble);
    return cfg;
}

std::int32_t clamp_pcm24(std::int32_t sample) {
    constexpr std::int32_t lo = -(1 << 23);
    constexpr std::int32_t hi = (1 << 23) - 1;
    return std::max(lo, std::min(hi, sample));
}

void append_pcm24(std::vector<std::uint8_t>& out, std::int32_t sample) {
    sample = clamp_pcm24(sample);
    out.push_back(static_cast<std::uint8_t>(sample & 0xFF));
    out.push_back(static_cast<std::uint8_t>((sample >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((sample >> 16) & 0xFF));
}

struct OutputMapping {
    std::vector<std::uint32_t> stream_for_output_channel;
    std::vector<std::uint32_t> channel_id_for_output_channel;
    std::uint16_t channels = 0;
    std::uint32_t channel_mask = 0;
};

const char* cx_channel_name(std::uint32_t channel_id) {
    static constexpr const char* names[] = {
        "FL", "FR", "C", "LFE", "LS", "RS", "CS", "LB",
        "RB", "HL", "HR", "HC", "T", "HLS", "HRS", "HCS"
    };
    return channel_id < sizeof(names) / sizeof(names[0]) ? names[channel_id] : nullptr;
}

const char* cx_layout_name(std::uint32_t layout) {
    switch (layout) {
    case 0x01BFu: return "7.1";
    case 0x663Fu: return "5.1+4H (9.1)";
    case 0x67BFu: return "7.1+4H (11.1)";
    case 0x7FBFu: return "7.1+5H+T (13.1)";
    default: return "custom";
    }
}

std::uint32_t cx_layout_from_mapping(const OutputMapping& mapping) {
    std::uint32_t layout = 0;
    for (const std::uint32_t channel_id : mapping.channel_id_for_output_channel) {
        if (channel_id < 32u)
            layout |= 1u << channel_id;
    }
    return layout;
}

std::string xml_escape(const std::string& value) {
    std::string escaped;
    for (const char c : value) {
        switch (c) {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '\"': escaped += "&quot;"; break;
        case '\'': escaped += "&apos;"; break;
        default: escaped += c; break;
        }
    }
    return escaped;
}

bool write_channel_mapping_xml(
    const std::filesystem::path& audio_path,
    const std::filesystem::path& source_path,
    std::uint32_t sample_rate,
    std::uint32_t source_sample_rate,
    const OutputMapping& mapping,
    const char* audio_coding,
    bool binaural,
    std::uint16_t container_channels,
    const std::string& declared_layout_name,
    std::string& error) {
    if (!binaural && (mapping.channel_id_for_output_channel.size() != mapping.channels ||
        mapping.stream_for_output_channel.size() != mapping.channels)) {
        error = "OruaCX output mapping is incomplete";
        return false;
    }
    std::filesystem::path xml_path = audio_path;
    xml_path.replace_extension(".xml");
    std::ofstream out(xml_path, std::ios::binary | std::ios::trunc);
    if (!out) {
        error = "cannot open channel mapping XML: " + xml_path.string();
        return false;
    }
    const std::uint32_t source_layout = cx_layout_from_mapping(mapping);
    const char* embedded = !declared_layout_name.empty()
        ? declared_layout_name.c_str()
        : cx_layout_name(source_layout);
    const char* container = nullptr;
    switch (container_channels) {
    case 2: container = "2.0"; break;
    case 3: container = "2.1"; break;
    case 4: container = "4.0"; break;
    case 6: container = "5.1"; break;
    case 8: container = "7.1"; break;
    default: break;
    }
    const std::uint32_t layout_channels = binaural
        ? (mapping.channels != 0u ? mapping.channels : static_cast<std::uint32_t>(container_channels))
        : mapping.channels;
    std::string source_layout_text = embedded && embedded[0] ? embedded : "";
    if (source_layout_text.empty() || source_layout_text == "custom")
        source_layout_text = std::to_string(layout_channels) + "ch";
    if (container && source_layout_text != container)
        source_layout_text = std::string(container) + " embedded " + source_layout_text;

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<channelMapping audioFile=\""
        << xml_escape(audio_path.filename().string())
        << "\" sourceFile=\""
        << xml_escape(source_path.filename().string())
        << "\" decoder=\"OruaCX\" audioCoding=\"" << audio_coding
        << "\" sampleRate=\"" << sample_rate
        << "\" sourceSampleRate=\""
        << (source_sample_rate != 0u ? source_sample_rate : sample_rate)
        << "\" sourceLayout=\"" << xml_escape(source_layout_text)
        << "\" sourceLayoutMask=\"0x" << std::hex << source_layout << std::dec
        << "\" bitsPerSample=\"24\" channelCount=\"" << (binaural ? 2u : mapping.channels)
        << "\">\n";
    if (binaural) {
        out << "  <channel index=\"0\" number=\"1\" slot=\"0\" name=\"FL\" source=\"binaural_renderer\"/>\n"
            << "  <channel index=\"1\" number=\"2\" slot=\"1\" name=\"FR\" source=\"binaural_renderer\"/>\n";
    }
    for (std::uint32_t index = 0; !binaural && index < mapping.channels; ++index) {
        const std::uint32_t channel_id = mapping.channel_id_for_output_channel[index];
        const char* name = cx_channel_name(channel_id);
        out << "  <channel index=\"" << index
            << "\" number=\"" << (index + 1u)
            << "\" slot=\"" << channel_id << "\" name=\"";
        if (name)
            out << name;
        else
            out << "ch" << channel_id;
        out << "\" source=\"native_aurocx\" audioStream=\""
            << mapping.stream_for_output_channel[index] << "\"/>\n";
    }
    out << "</channelMapping>\n";
    if (!out) {
        error = "failed to write channel mapping XML: " + xml_path.string();
        return false;
    }
    return true;
}

std::uint32_t wave_speaker_mask(std::uint32_t channel_id) {
    static constexpr std::uint32_t masks[] = {
        0x00000001u, // FL
        0x00000002u, // FR
        0x00000004u, // C
        0x00000008u, // LFE
        0x00000200u, // LS
        0x00000400u, // RS
        0x00000100u, // CS
        0x00000010u, // LB
        0x00000020u, // RB
        0x00001000u, // HL
        0x00004000u, // HR
        0x00002000u, // HC
        0x00000800u, // T
        0x00008000u, // HLS
        0x00020000u, // HRS
        0x00010000u  // HCS
    };
    return channel_id < sizeof(masks) / sizeof(masks[0]) ? masks[channel_id] : 0u;
}

std::int32_t normalize_pcm24(std::int32_t sample, std::uint32_t bitdepth) {
    if (!bitdepth || bitdepth == 24u)
        return sample;
    if (bitdepth < 24u) {
        const unsigned shift = 24u - bitdepth;
        return static_cast<std::int32_t>(
            static_cast<std::uint32_t>(sample) << shift);
    }
    if (bitdepth >= 33u)
        return sample;
    const unsigned shift = bitdepth - 24u;
    const std::uint32_t mask = (std::uint32_t{1} << shift) - 1u;
    const std::uint32_t correction = sample < 0 ? mask : 0u;
    const std::int32_t adjusted = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(sample) + correction);
    return adjusted >> shift;
}

OutputMapping make_wave_output_mapping(
    const std::vector<std::pair<std::uint32_t, std::uint32_t>>& channels) {
    OutputMapping map{};
    if (channels.empty() || channels.size() > UINT16_MAX)
        return map;

    std::vector<std::pair<
        std::uint32_t,
        std::pair<std::uint32_t, std::uint32_t>>> ordered;
    ordered.reserve(channels.size());
    std::uint32_t channel_mask = 0;
    for (const auto& channel : channels) {
        const std::uint32_t speaker = wave_speaker_mask(channel.first);
        if (!speaker || (channel_mask & speaker)) {
            map.channels = static_cast<std::uint16_t>(channels.size());
            map.stream_for_output_channel.reserve(channels.size());
            map.channel_id_for_output_channel.reserve(channels.size());
            for (const auto& original : channels) {
                map.channel_id_for_output_channel.push_back(original.first);
                map.stream_for_output_channel.push_back(original.second);
            }
            return map;
        }
        channel_mask |= speaker;
        ordered.emplace_back(speaker, channel);
    }
    std::sort(ordered.begin(), ordered.end());
    map.channels = static_cast<std::uint16_t>(ordered.size());
    map.channel_mask = channel_mask;
    map.stream_for_output_channel.reserve(ordered.size());
    map.channel_id_for_output_channel.reserve(ordered.size());
    for (const auto& channel : ordered) {
        map.channel_id_for_output_channel.push_back(channel.second.first);
        map.stream_for_output_channel.push_back(channel.second.second);
    }
    return map;
}

OutputMapping build_output_mapping(
    const CxSchemaParseResult& schema,
    std::uint16_t declared_layout,
    bool has_declared_layout) {
    if (schema.bed_channels_decoded && !schema.bed_channels.empty()) {
        std::vector<std::pair<std::uint32_t, std::uint32_t>> channels;
        channels.reserve(schema.bed_channels.size());
        for (const auto& ch : schema.bed_channels)
            channels.emplace_back(ch.id, ch.audio_stream_index);
        return make_wave_output_mapping(channels);
    }
    if (has_declared_layout) {
        std::vector<std::pair<std::uint32_t, std::uint32_t>> channels;
        std::uint32_t stream_index = 0;
        for (unsigned bit = 0; bit < 16; ++bit) {
            if ((declared_layout >> bit) & 1u) {
                channels.emplace_back(bit, stream_index);
                ++stream_index;
            }
        }
        return make_wave_output_mapping(channels);
    }
    return {};
}

} // namespace

bool mp4_has_auro_cx_a3ds(const std::string& path) {
    // Cheap reject for WAV/FLAC/raw: avoid loading large non-MP4 files.
    if (!file_starts_with_ftyp(path))
        return false;
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;
    const std::vector<std::uint8_t> mp4(
        (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    CxTrack track{};
    return find_track(mp4, track);
}

bool decode_auro_cx_mp4(
    const std::string& path,
    const std::string& out_wav,
    std::string& error,
    float headroom_db,
    bool binaural,
    unsigned room_preset,
    unsigned hrtf_preset,
    const std::string& output_format,
    const ProgressFn& progress) {
    if (!std::isfinite(headroom_db) || headroom_db < 0.0f) {
        error = "invalid output headroom";
        return false;
    }
    const bool want_flac = output_format == "flac";
    std::filesystem::path temp_wav_path;
    std::string pcm_path = out_wav;
    if (want_flac) {
        const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        temp_wav_path = std::filesystem::temp_directory_path()
            / ("orua3d-decode-" + std::to_string(stamp) + ".wav");
        pcm_path = temp_wav_path.string();
    }
    const auto remove_temp_wav = [&]() {
        if (temp_wav_path.empty())
            return;
        std::error_code remove_error;
        std::filesystem::remove(temp_wav_path, remove_error);
    };
    const auto finalize_output = [&]() -> bool {
        if (want_flac) {
            if (progress)
                progress("encode flac", -1);
            if (!wav::encode_wav_to_flac(pcm_path, out_wav, error)) {
                remove_temp_wav();
                return false;
            }
            remove_temp_wav();
            if (progress)
                progress("encode flac", 100);
            return true;
        }
        if (progress)
            progress("save wav", 100);
        return true;
    };
    const float headroom_gain = std::pow(10.0f, -headroom_db / 20.0f);
    if (progress)
        progress("demux", -1);
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "cannot open input";
        return false;
    }
    const std::vector<std::uint8_t> mp4(
        (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (progress)
        progress("demux", 100);

    CxTrack track{};
    if (!find_track(mp4, track)) {
        error = "OruaCX a3ds audio track not found";
        return false;
    }
    if (track.offsets.empty() || track.sizes.empty() || !track.duration) {
        error = "missing MP4 sample table data";
        return false;
    }

    std::uint16_t declared_layout = 0;
    bool has_declared_layout = false;
    if (track.acxd.size() >= 38) {
        has_declared_layout = true;
        declared_layout = be16(track.acxd.data() + 36);
    }

    const std::uint8_t error_scale_byte = track.sample_bits
        ? static_cast<std::uint8_t>(track.sample_bits)
        : 24u;

    CxSchemaDecodeState schema_state{};
    CxSchemaParseResult initial_schema{};
    std::vector<std::vector<std::int32_t>> stream_buffers;
    std::vector<std::uint32_t> stream_bitdepths;
    std::vector<std::size_t> stream_sample_counts;
    std::vector<lfe::InterpolationState> lfe_interpolation_states;
    std::vector<sasc::Factor2State> sasc_factor2_states;
    std::vector<sasc::Factor4State> sasc_factor4_states;
    std::size_t stream_capacity = 0;
    std::uint32_t samples_per_au = track.duration;
    OutputMapping mapping{};
    std::vector<std::vector<sasc::Step>> sasc_plans;
    std::uint32_t sasc_scratch_stream = UINT32_MAX;
    std::vector<std::uint32_t> awc_stream_parameters;
    wav::Pcm24StreamWriter wav_writer;
    BinauralStreamRenderer binaural_renderer;
    std::vector<std::uint8_t> au_pcm;
    std::vector<std::uint8_t> binaural_pcm;
    std::vector<std::uint8_t> deferred_multichannel_pcm;
    const bool binaural_needs_resample = binaural && track.rate != 48000u;
    bool saw_lossless_awc = false;
    bool saw_transparent_awc = false;

    const auto bed_stream_count = [&](const CxSchemaParseResult& schema) -> std::uint32_t {
        std::uint32_t count = 0;
        for (const auto& pdu : schema.pdus) {
            if (pdu.audio_stream_count)
                count = std::max(count, pdu.first_audio_stream + pdu.audio_stream_count);
        }
        if (!schema.beds.empty()) {
            for (const auto& bed : schema.beds)
                for (const auto& ch : bed.channels)
                    count = std::max(count, ch.audio_stream_index + 1u);
        } else if (schema.bed_channels_decoded) {
            for (const auto& ch : schema.bed_channels)
                count = std::max(count, ch.audio_stream_index + 1u);
        }
        for (const auto& group : schema.object_groups) {
            for (const auto& object : group.objects)
                count = std::max(count, object.audio_stream_index + 1u);
        }
        return count;
    };

    const auto schema_references_stream_range = [](
        const CxSchemaParseResult& schema,std::uint32_t first,std::uint32_t end) {
        bool has_mapping=false;
        if(!schema.beds.empty()){
            for(const auto&bed:schema.beds){
                for(const auto&channel:bed.channels){
                    has_mapping=true;
                    if(channel.audio_stream_index>=first&&channel.audio_stream_index<end)return true;
                }
            }
        }else{
            for(const auto&channel:schema.bed_channels){
                has_mapping=true;
                if(channel.audio_stream_index>=first&&channel.audio_stream_index<end)return true;
            }
        }
        for(const auto&group:schema.object_groups){
            for(const auto&object:group.objects){
                has_mapping=true;
                if(object.audio_stream_index>=first&&object.audio_stream_index<end)return true;
            }
        }
        return !has_mapping;
    };

    const auto ensure_streams = [&](std::uint32_t stream_count) {
        if (stream_count > stream_capacity) {
            stream_capacity = stream_count;
            stream_buffers.resize(stream_count);
            stream_bitdepths.resize(stream_count, 24u);
            stream_sample_counts.resize(stream_count, 0u);
            sasc_factor2_states.resize(stream_count);
            sasc_factor4_states.resize(stream_count);
        }
    };

    const auto apply_sasc_sample_rate = [&] (
        const AuroCxSchemaPduInfo& pdu,
        std::size_t au,
        std::string& apply_error) -> bool {
        if (pdu.sasc_resample_factor == 1u)
            return true;
        if (pdu.sasc_resample_factor != 2u && pdu.sasc_resample_factor != 4u) {
            apply_error = "unsupported SASC sample-rate factor";
            return false;
        }
        const std::size_t level_count = pdu.sasc_channel_sets.size();
        const bool valid_level_count = pdu.sasc_resample_factor == 2u
            ? (level_count == 1u || level_count == 2u)
            : level_count == 2u;
        if (!valid_level_count ||
            pdu.sasc_channel_set_stream_indices.size() != level_count) {
            apply_error = "SASC sample-rate channel-set level mismatch";
            return false;
        }

        for (std::size_t channel = 0;
             channel < pdu.sasc_audio_stream_indices.size(); ++channel) {
            const std::uint32_t base_stream = pdu.sasc_audio_stream_indices[channel];
            if (base_stream >= stream_buffers.size() ||
                base_stream >= stream_sample_counts.size()) {
                apply_error = "SASC base stream index out of range";
                return false;
            }
            const std::size_t base_count = stream_sample_counts[base_stream];
            if (!base_count || base_count > stream_buffers[base_stream].size()) {
                apply_error = "SASC base stream is unavailable at AU " + std::to_string(au);
                return false;
            }

            const auto enhancement = [&] (
                std::size_t level,
                std::size_t expected_count,
                std::vector<std::int32_t>& samples) -> bool {
                samples.assign(expected_count, 0);
                if (channel >= pdu.sasc_channel_sets[level].size() ||
                    channel >= pdu.sasc_channel_set_stream_indices[level].size()) {
                    apply_error = "SASC enhancement channel index out of range";
                    return false;
                }
                if (!pdu.sasc_channel_sets[level][channel])
                    return pdu.sasc_channel_set_stream_indices[level][channel] == UINT32_MAX;
                const std::uint32_t stream =
                    pdu.sasc_channel_set_stream_indices[level][channel];
                if (stream == UINT32_MAX || stream >= stream_buffers.size() ||
                    stream >= stream_sample_counts.size() ||
                    stream_sample_counts[stream] != expected_count ||
                    stream_buffers[stream].size() < expected_count) {
                    apply_error = "SASC enhancement stream geometry mismatch";
                    return false;
                }
                std::copy_n(stream_buffers[stream].begin(), expected_count, samples.begin());
                return true;
            };

            std::vector<std::int32_t> level0;
            if (!enhancement(0u, base_count, level0))
                return false;
            const std::vector<std::int32_t> base(
                stream_buffers[base_stream].begin(),
                stream_buffers[base_stream].begin() + static_cast<std::ptrdiff_t>(base_count));
            std::vector<std::int32_t> reconstructed;
            if (pdu.sasc_resample_factor == 2u) {
                if (!sasc::synthesize_factor2(
                        sasc_factor2_states[base_stream],
                        level0,
                        base,
                        reconstructed,
                        apply_error))
                    return false;
                if (level_count == 2u) {
                    for (auto& sample : reconstructed)
                        sample /= 2;
                }
            } else {
                if (base_count > std::numeric_limits<std::size_t>::max() / 2u) {
                    apply_error = "SASC factor4 stream size overflow";
                    return false;
                }
                std::vector<std::int32_t> level1;
                if (!enhancement(1u, base_count * 2u, level1))
                    return false;
                if (!sasc::synthesize_factor4(
                        sasc_factor4_states[base_stream],
                        level0,
                        level1,
                        base,
                        reconstructed,
                        apply_error))
                    return false;
            }
            if (reconstructed.size() != samples_per_au) {
                apply_error = "SASC reconstructed stream sample count mismatch";
                return false;
            }
            stream_buffers[base_stream] = std::move(reconstructed);
            stream_sample_counts[base_stream] = samples_per_au;
        }
        return true;
    };

    for (std::size_t au = 0; au < track.offsets.size(); ++au) {
        if (progress)
            progress("decode", progress_percent(au, track.offsets.size()));
        const std::uint64_t offset = track.offsets[au];
        const std::uint32_t size = track.sizes[au];
        if (offset + size > mp4.size()) {
            error = "access unit truncated in MP4 payload";
            return false;
        }
        const std::uint8_t* begin = mp4.data() + offset;
        const std::uint8_t* end = begin + size;

        std::vector<std::uint8_t> blob;
        if (!cx_decode_xor_segment1(begin, end, blob, error))
            return false;

        CxSchemaParseResult schema{};
        const bool delta = au > 0;
        if (!cx_parse_schema_blob(blob, schema, &schema_state, delta)) {
            error = delta ? "delta schema parse failed" : "initial schema parse failed";
            return false;
        }
        if (!schema.pdu_types_complete) {
            if (delta) {
                error = "delta schema PDU vector incomplete at AU " + std::to_string(au) +
                        " bit=" + std::to_string(schema.consumed_bits) + "/" +
                        std::to_string(schema.blob_bits);
                return false;
            } else {
                error = "schema PDU vector incomplete at AU " + std::to_string(au) +
                        " bit=" + std::to_string(schema.consumed_bits) + "/" +
                        std::to_string(schema.blob_bits) + " sample_bytes=" +
                        std::to_string(size) + " blob_bytes=" + std::to_string(blob.size()) + " audio_bit=" +
                        std::to_string(schema.audio_bit_offset) + " header_bit=" +
                        std::to_string(schema.config_header_bits) + " programs_bit=" +
                        std::to_string(schema.programs_end_bit) + " pdus=" +
                        std::to_string(schema.pdu_count) + " types=" +
                        std::to_string(schema.pdu_types.size());
                for (const auto type : schema.pdu_types)
                    error += "," + std::to_string(type);
                error += " channels=";
                for (const auto bit : schema.channel_end_bits)
                    error += std::to_string(bit) + ",";
                for (const auto& pdu : schema.pdus)
                    error += " pdu=" + std::to_string(pdu.type) + ":" +
                            std::to_string(pdu.audio_stream_count) + ":" +
                            std::to_string(pdu.header_value) + ":" +
                            std::to_string(pdu.header_flag0) + ":" +
                            std::to_string(pdu.payload_bit_offset) + ":" +
                            std::to_string(pdu.payload_bits);
                return false;
            }
        }
        if (delta) {
            std::uint32_t delta_block_size = 0;
            if (cx_read_config_block_size(blob, true, &schema_state, delta_block_size))
                schema.block_size = delta_block_size;
        }
        const std::uint32_t block_size =
            schema.block_size ? schema.block_size : samples_per_au;
        if (schema.block_size)
            schema_state.block_size = schema.block_size;
        if (au == 0) {
            initial_schema = schema;
            for (const auto& config_pdu : schema.pdus) {
                if (config_pdu.type != 0 || config_pdu.awc_stream_parameters.empty())
                    continue;
                const std::size_t end = static_cast<std::size_t>(config_pdu.first_audio_stream) +
                                        config_pdu.awc_stream_parameters.size();
                if (awc_stream_parameters.size() < end)
                    awc_stream_parameters.resize(end);
                std::copy(config_pdu.awc_stream_parameters.begin(), config_pdu.awc_stream_parameters.end(),
                          awc_stream_parameters.begin() + config_pdu.first_audio_stream);
            }
            mapping = build_output_mapping(schema, declared_layout, has_declared_layout);
            if (!mapping.channels) {
                error = "no output channel mapping";
                return false;
            }
            if (!sasc::build_plans(
                    schema,
                    schema_state.objects,
                    declared_layout,
                    has_declared_layout,
                    sasc_plans,
                    sasc_scratch_stream,
                    error)) {
                error = "SASC plan failed: " + error;
                return false;
            }
            const std::uint32_t max_stream = bed_stream_count(schema);
            std::uint32_t alloc_streams = max_stream;
            if (sasc_scratch_stream != UINT32_MAX)
                alloc_streams = std::max(alloc_streams, sasc_scratch_stream + 1u);
            ensure_streams(alloc_streams);
            for (std::uint32_t s = 0; s < alloc_streams; ++s)
                stream_buffers[s].assign(samples_per_au, 0);
            const std::uint64_t frame_count =
                static_cast<std::uint64_t>(track.offsets.size() - 1u) * samples_per_au;
            if (binaural && !binaural_needs_resample
                && !binaural_renderer.initialize(
                    24u,
                    track.rate,
                    mapping.channels,
                    mapping.channel_id_for_output_channel,
                    room_preset,
                    hrtf_preset,
                    samples_per_au,
                    error))
                return false;
            if (binaural_needs_resample) {
                deferred_multichannel_pcm.reserve(
                    static_cast<std::size_t>(frame_count)
                    * static_cast<std::size_t>(mapping.channels) * 3u);
            } else if (!wav_writer.open(
                    pcm_path,
                    track.rate,
                    binaural ? 2u : mapping.channels,
                    frame_count,
                    error,
                    binaural ? 3u : mapping.channel_mask))
                return false;
            au_pcm.reserve(static_cast<std::size_t>(mapping.channels) * samples_per_au * 3u);
            continue;
        }

        const std::uint64_t blob_bits = static_cast<std::uint64_t>(blob.size()) * 8u;
        if (schema.consumed_bits > blob_bits) {
            error = "schema consumed more bits than blob size";
            return false;
        }

        if (!sasc::build_plans(
                schema,
                schema_state.objects,
                declared_layout,
                has_declared_layout,
                sasc_plans,
                sasc_scratch_stream,
                error)) {
            error = "SASC plan update failed at AU " + std::to_string(au) + ": " + error;
            return false;
        }

        std::uint32_t max_stream = bed_stream_count(schema);
        if (sasc_scratch_stream != UINT32_MAX)
            max_stream = std::max(max_stream, sasc_scratch_stream + 1u);
        ensure_streams(max_stream);
        for (auto& stream : stream_buffers) {
            if (stream.size() != samples_per_au)
                stream.resize(samples_per_au);
            std::fill(stream.begin(), stream.end(), 0);
        }
        std::fill(stream_sample_counts.begin(), stream_sample_counts.end(), 0u);
        bool saw_sasc = false;
        const auto pdu_payload_bounds = [&](const AuroCxSchemaPduInfo& pdu, std::size_t& start, std::size_t& end) -> bool {
            if (!pdu.payload_bits) {
                error = "PDU payload length missing at AU " + std::to_string(au);
                return false;
            }
            start = static_cast<std::size_t>(pdu.payload_bit_offset);
            end = start + static_cast<std::size_t>(pdu.payload_bits);
            if (start > blob_bits || start >= end || end > blob_bits) {
                error = "PDU payload out of bounds at AU " + std::to_string(au) + " offset=" +
                        std::to_string(start);
                return false;
            }
            return true;
        };

        for (std::size_t pdu_index = 0; pdu_index < schema.pdus.size(); ++pdu_index) {
            const auto& pdu = schema.pdus[pdu_index];
            if (pdu.type == 6) {
                saw_sasc = true;
                if (!apply_sasc_sample_rate(pdu, au, error)) {
                    error = "SASC sample-rate decode failed at AU " +
                            std::to_string(au) + ": " + error;
                    return false;
                }
                if (pdu_index >= sasc_plans.size()) {
                    error = "SASC plan index out of range at AU " + std::to_string(au);
                    return false;
                }
                // Processor::run_ (0x492AA0) always calls SCG::decode after
                // the SCG context has been created. Header+60 is consumed by
                // calculate_mode (0x492630); it does not gate frame decoding.
                // Full discrete bed+height WAV uses playback layer 2, so
                // details::decode (0x495460) cancels every cross-step below 2.
                constexpr std::uint32_t kFullDiscreteScgLayer = 2u;
                const std::optional<std::uint32_t> layer_filter =
                    pdu.sasc_channel_bed_decoded
                        ? std::optional<std::uint32_t>(kFullDiscreteScgLayer)
                        : std::nullopt;
                if (!sasc::apply_steps(
                        stream_buffers,
                        stream_bitdepths,
                        sasc_plans[pdu_index],
                        0,
                        samples_per_au,
                        error,
                        layer_filter)) {
                    error = "SASC apply failed at AU " + std::to_string(au) + ": " + error;
                    return false;
                }
                continue;
            }
            if (pdu.type == 2) {
                std::vector<std::uint32_t> selected_lfe_streams;
                const std::uint32_t pdu_stream_end=pdu.first_audio_stream+pdu.audio_stream_count;
                const bool pdu_is_selected=schema_references_stream_range(
                    schema,pdu.first_audio_stream,pdu_stream_end);
                if(!pdu_is_selected)
                    continue;
                for(std::uint32_t s=0;s<(pdu.audio_stream_count?pdu.audio_stream_count:1u);++s)
                    selected_lfe_streams.push_back(pdu.first_audio_stream+s);
                std::size_t payload_start = 0;
                std::size_t payload_end = 0;
                if (!pdu_payload_bounds(pdu, payload_start, payload_end))
                    return false;
                const std::size_t lfe_payload_start = payload_start;
                cx::Bits lfe_bits{blob, lfe_payload_start, payload_end};

                std::vector<std::vector<std::int32_t>> lfe_samples;
                static const std::uint32_t custom_lfe_factors[] = {40, 80, 160, 320, 640};
                std::uint32_t lfe_resample_factor = lfe::default_resample_factor(track.rate);
                if (pdu.header_flag0 && pdu.header_value < 5u)
                    lfe_resample_factor = custom_lfe_factors[pdu.header_value];
                if (!lfe::decode_lfe_payload(
                            lfe_bits,
                            block_size,
                            lfe_resample_factor,
                            pdu.first_audio_stream,
                            static_cast<std::uint32_t>(selected_lfe_streams.size()),
                            lfe_interpolation_states,
                            lfe_samples,
                            error)) {
                    error = "LFE decode failed at AU " + std::to_string(au) +
                            " PDU " + std::to_string(pdu_index) + ": " + error;
                    return false;
                    }
                if (lfe_bits.remaining_bits() != 0) {
                    error = "LFE payload under-read at AU " + std::to_string(au) +
                            " PDU " + std::to_string(pdu_index) +
                            " remaining=" + std::to_string(lfe_bits.remaining_bits());
                    return false;
                }
                for (std::uint32_t s = 0; s < lfe_samples.size(); ++s) {
                    const std::uint32_t stream_index = selected_lfe_streams[s];
                    if (stream_index >= stream_buffers.size())
                        ensure_streams(stream_index + 1);
                    if (stream_buffers[stream_index].empty())
                        stream_buffers[stream_index].resize(samples_per_au, 0);
                    const std::size_t dst = 0;
                    const std::size_t copy_count =
                        std::min(lfe_samples[s].size(), static_cast<std::size_t>(samples_per_au));
                    std::copy(
                        lfe_samples[s].begin(),
                        lfe_samples[s].begin() + static_cast<std::ptrdiff_t>(copy_count),
                        stream_buffers[stream_index].begin() + static_cast<std::ptrdiff_t>(dst));
                    stream_sample_counts[stream_index] = copy_count;
                    stream_bitdepths[stream_index] = 24u;
                }
                continue;
            }
            if (pdu.type != 0) {
                if (pdu.payload_bits) {
                    std::size_t payload_start = 0;
                    std::size_t payload_end = 0;
                    if (!pdu_payload_bounds(pdu, payload_start, payload_end))
                        return false;
                }
                continue;
            }
            if (pdu.header_flag0)
                saw_lossless_awc = true;
            else
                saw_transparent_awc = true;
            if (!pdu.audio_stream_count)
                continue;

            std::vector<std::uint32_t> selected_stream_indices;
            const std::uint32_t pdu_stream_end=pdu.first_audio_stream+pdu.audio_stream_count;
            const bool pdu_is_selected=schema_references_stream_range(
                schema,pdu.first_audio_stream,pdu_stream_end);
            if(!pdu_is_selected)
                continue;
            for(std::uint32_t s=0;s<pdu.audio_stream_count;++s)
                selected_stream_indices.push_back(pdu.first_audio_stream+s);
            const std::uint32_t selected_stream_count=static_cast<std::uint32_t>(selected_stream_indices.size());

            std::size_t payload_start = 0;
            std::size_t payload_end = 0;
            if (!pdu_payload_bounds(pdu, payload_start, payload_end))
                return false;
            cx::Bits awc_bits{blob, payload_start, payload_end};

            if (!pdu.header_flag0) {
                const awc::AwcPayloadConfig transparent_cfg = [&]() -> awc::AwcPayloadConfig {
                    awc::AwcPayloadConfig out{};
                    out.block_size = block_size;
                    if (pdu.awc_payload_config_decoded || !pdu.payload_data.empty())
                        out = awc_config_from_pdu(pdu, error_scale_byte);
                    else {
                        if (pdu_index < schema_state.pdu_templates.size()) {
                            const auto& tmpl = schema_state.pdu_templates[pdu_index];
                            if (tmpl.type == 0 &&
                                tmpl.audio_stream_count == selected_stream_count)
                                out = awc_config_from_pdu(tmpl, error_scale_byte);
                        }
                        if (!out.common_preamble && pdu_index < initial_schema.pdus.size()) {
                            const auto& tmpl = initial_schema.pdus[pdu_index];
                            if (tmpl.type == 0 &&
                                tmpl.audio_stream_count == selected_stream_count)
                                out = awc_config_from_pdu(tmpl, error_scale_byte);
                        }
                    }
                    out.block_size = block_size;
                    out.sample_rate = track.rate;
                    out.frame_divisor = pdu.header_flag1 ? (pdu.header_value ? 4u : 2u) : 1u;
                    out.error_scale_byte = error_scale_byte;
                    if (out.common_preamble && !out.partition_threshold)
                        out.partition_threshold = static_cast<std::uint16_t>(out.common_preamble);
                    return out;
                }();

                std::vector<std::vector<std::int32_t>> group_samples;
                std::vector<std::uint32_t> group_bitdepths;
                if (!awc::decode_transparent_frame(
                            awc_bits,
                            transparent_cfg,
                            selected_stream_count,
                            samples_per_au,
                            group_samples,
                            error,
                            &group_bitdepths)) {
                    error = "transparent AWC decode failed at AU " + std::to_string(au) +
                            " PDU " + std::to_string(pdu_index) + ": " + error;
                    return false;
                    }
                if (awc_bits.remaining_bits() != 0) {
                    error = "transparent AWC payload under-read at AU " + std::to_string(au) +
                            " PDU " + std::to_string(pdu_index) +
                            " remaining=" + std::to_string(awc_bits.remaining_bits());
                    return false;
                }
                for (std::uint32_t s = 0; s < selected_stream_count; ++s) {
                    const std::uint32_t stream_index = selected_stream_indices[s];
                    if (stream_index >= stream_buffers.size())
                        ensure_streams(stream_index + 1);
                    if (stream_buffers[stream_index].empty())
                        stream_buffers[stream_index].resize(samples_per_au, 0);
                    const std::size_t dst = 0;
                    const std::size_t copy_count = group_samples[s].size();
                    if (!transparent_cfg.frame_divisor ||
                        samples_per_au % transparent_cfg.frame_divisor) {
                        error = "transparent AWC stream geometry mismatch";
                        return false;
                    }
                    const std::size_t expected_count =
                        samples_per_au / transparent_cfg.frame_divisor;
                    if (copy_count != expected_count) {
                        error = "transparent AWC stream sample count mismatch";
                        return false;
                    }
                    std::copy(
                        group_samples[s].begin(),
                        group_samples[s].end(),
                        stream_buffers[stream_index].begin() + static_cast<std::ptrdiff_t>(dst));
                    stream_sample_counts[stream_index] = copy_count;
                    if (s < group_bitdepths.size())
                        stream_bitdepths[stream_index] = group_bitdepths[s];
                }
                continue;
            }

            const awc::AwcPayloadConfig cfg = [&]() -> awc::AwcPayloadConfig {
                awc::AwcPayloadConfig out{};
                if (pdu.awc_payload_config_decoded || !pdu.payload_data.empty())
                    out = awc_config_from_pdu(pdu, error_scale_byte);
                else {
                    if (pdu_index < schema_state.pdu_templates.size()) {
                        const auto& tmpl = schema_state.pdu_templates[pdu_index];
                        if (tmpl.type == 0 &&
                            tmpl.audio_stream_count == selected_stream_count)
                            out = awc_config_from_pdu(tmpl, error_scale_byte);
                    }
                    if (!out.common_preamble && pdu_index < initial_schema.pdus.size()) {
                        const auto& tmpl = initial_schema.pdus[pdu_index];
                        if (tmpl.type == 0 &&
                            tmpl.audio_stream_count == selected_stream_count)
                            out = awc_config_from_pdu(tmpl, error_scale_byte);
                    }
                }
                out.block_size = block_size;
                out.sample_rate = track.rate;
                out.frame_divisor = pdu.header_flag1 ? (pdu.header_value ? 4u : 2u) : 1u;
                out.error_scale_byte = error_scale_byte;
                if (out.common_preamble && !out.partition_threshold)
                    out.partition_threshold = static_cast<std::uint16_t>(out.common_preamble);
                return out;
            }();

            std::vector<std::vector<std::int32_t>> group_samples;
            std::vector<std::uint32_t> group_bitdepths;
            cx::Bits lossless_bits{blob, payload_start, payload_end};
            if (!awc::decode_lossless_frame(
                        lossless_bits,
                        cfg,
                        selected_stream_count,
                        samples_per_au,
                        group_samples,
                        error,
                        &group_bitdepths)) {
                error = "lossless AWC decode failed at AU " + std::to_string(au) +
                        " PDU " + std::to_string(pdu_index) + ": " + error;
                return false;
                }
            if (lossless_bits.remaining_bits() != 0) {
                error = "lossless AWC payload under-read at AU " + std::to_string(au) +
                        " PDU " + std::to_string(pdu_index) +
                        " remaining=" + std::to_string(lossless_bits.remaining_bits());
                return false;
            }
            for (std::uint32_t s = 0; s < selected_stream_count; ++s) {
                const std::uint32_t stream_index = selected_stream_indices[s];
                if (stream_index >= stream_buffers.size())
                    ensure_streams(stream_index + 1);
                if (stream_buffers[stream_index].empty())
                    stream_buffers[stream_index].resize(samples_per_au, 0);
                const std::size_t dst = 0;
                const std::size_t copy_count = group_samples[s].size();
                if (!cfg.frame_divisor || samples_per_au % cfg.frame_divisor) {
                    error = "AWC lossless stream geometry mismatch at AU " + std::to_string(au);
                    return false;
                }
                const std::size_t expected_count = samples_per_au / cfg.frame_divisor;
                if (copy_count != expected_count) {
                    error = "AWC lossless stream sample count mismatch at AU " + std::to_string(au);
                    return false;
                }
                std::copy(
                    group_samples[s].begin(),
                    group_samples[s].end(),
                    stream_buffers[stream_index].begin() + static_cast<std::ptrdiff_t>(dst));
                stream_sample_counts[stream_index] = copy_count;
                if (s < group_bitdepths.size())
                    stream_bitdepths[stream_index] = group_bitdepths[s];
            }
        }

        (void)saw_sasc;
        for (const std::uint32_t stream : mapping.stream_for_output_channel) {
            if (stream >= stream_buffers.size() ||
                stream >= stream_sample_counts.size() ||
                stream_buffers[stream].size() < samples_per_au ||
                stream_sample_counts[stream] != samples_per_au) {
                error = "output stream is incomplete at AU " + std::to_string(au) +
                        " stream=" + std::to_string(stream);
                return false;
            }
        }
        au_pcm.clear();
        for (std::size_t frame = 0; frame < samples_per_au; ++frame) {
            for (std::uint32_t out_ch = 0; out_ch < mapping.channels; ++out_ch) {
                const std::uint32_t stream = mapping.stream_for_output_channel[out_ch];
                const std::int32_t sample = stream < stream_buffers.size()
                    ? normalize_pcm24(stream_buffers[stream][frame], stream_bitdepths[stream])
                    : 0;
                append_pcm24(
                    au_pcm,
                    static_cast<std::int32_t>(static_cast<float>(sample) * headroom_gain));
            }
        }
        if (binaural_needs_resample) {
            deferred_multichannel_pcm.insert(
                deferred_multichannel_pcm.end(), au_pcm.begin(), au_pcm.end());
            continue;
        }
        const std::vector<std::uint8_t>* output_pcm = &au_pcm;
        if (binaural) {
            if (!binaural_renderer.process(au_pcm, binaural_pcm, error))
                return false;
            output_pcm = &binaural_pcm;
        }
        if (!wav_writer.write(*output_pcm, error))
            return false;
    }
    if (progress)
        progress("decode", 100);
    // 48 kHz path applies HRTF inline per AU; report the stage once decode finishes.
    if (progress && binaural && !binaural_needs_resample)
        progress("encode binaural", 100);

    if (stream_buffers.empty()) {
        error = "no decoded audio streams";
        return false;
    }

    std::uint32_t xml_rate = track.rate;
    if (binaural_needs_resample) {
        std::vector<std::uint8_t> resampled;
        if (!resample_interleaved_pcm_to_rate(
                deferred_multichannel_pcm,
                24u,
                mapping.channels,
                track.rate,
                48000u,
                resampled,
                error,
                progress))
            return false;
        std::vector<std::uint8_t> stereo;
        if (!render_binaural_from_embedded_ir(
                resampled,
                24u,
                48000u,
                mapping.channels,
                mapping.channel_id_for_output_channel,
                room_preset,
                hrtf_preset,
                stereo,
                error,
                progress))
            return false;
        const std::size_t frame_bytes = 2u * 3u;
        if (!frame_bytes || stereo.size() % frame_bytes != 0u) {
            error = "binaural resample produced incomplete frames";
            remove_temp_wav();
            return false;
        }
        if (!want_flac && progress)
            progress("save wav", -1);
        if (!wav::write_pcm24_le(
                pcm_path,
                48000u,
                2u,
                stereo,
                error,
                3u)) {
            remove_temp_wav();
            return false;
        }
        if (!finalize_output())
            return false;
        xml_rate = 48000u;
    } else if (!wav_writer.close(error)) {
        remove_temp_wav();
        return false;
    } else if (!finalize_output()) {
        return false;
    }
    const char* audio_coding = saw_lossless_awc && saw_transparent_awc
        ? "mixed"
        : saw_lossless_awc
            ? "lossless"
            : saw_transparent_awc
                ? "transparent_near_lossless"
                : "unknown";
    std::string declared_layout_name;
    if (has_declared_layout) {
        if (declared_layout == 0x7FBFu)
            declared_layout_name = "7.1+5H+T (13.1)";
        else if (declared_layout == 0x663Fu)
            declared_layout_name = "5.1+4H (9.1)";
        else if (declared_layout == 0x67BFu)
            declared_layout_name = "7.1+4H (11.1)";
        else if (declared_layout == 0x01BFu)
            declared_layout_name = "7.1";
        else
            declared_layout_name = cx_layout_name(declared_layout);
    }
    if (!write_channel_mapping_xml(
            out_wav,
            path,
            xml_rate,
            track.rate,
            mapping,
            audio_coding,
            binaural,
            track.channels,
            declared_layout_name,
            error))
        return false;
    return true;
}

} // namespace auro3d
