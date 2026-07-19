#include "app_version.hpp"
#include "cx_probe.hpp"
#include "cx_decode.hpp"
#include "decoder.hpp"
#include "progress.hpp"
#include "../io/wav_writer.hpp"
#include "../render/binaural_renderer.hpp"
#include "../util/auro3deng_strength.hpp"

#include <array>
#include <chrono>
#include <cstdlib>
#include <cwchar>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

struct Options {
    std::string input;
    std::string output;
    bool raw = false;
    bool verbose = false;
    bool mono_tracks = false;
    bool channel_diagram = false;
    bool binaural = false;
    bool help_only = false;
    bool version_only = false;
    bool probe = false;
    unsigned sample_rate = 0;
    unsigned channels = 0;
    unsigned block_size = 0;
    unsigned dsp_strength = 12;
    unsigned dsp_output_channels = 0;
    unsigned output_bits = 24;
    std::string output_format;
    float dsp_headroom_db = 0.0f;
    unsigned room_preset = auro3d::kDefaultRoomPreset;
    unsigned hrtf_preset = auro3d::kDefaultHrtfPreset;
    unsigned virtualizer_mode = auro3d::kDefaultVirtualizerMode;
    unsigned headphone_connected = 1;
    unsigned stereo_device_connected = 1;
};

void configure_console_encoding() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

#ifdef _WIN32
std::string wide_to_utf8(const wchar_t* value) {
    if (!value || !*value)
        return {};
    const int length = static_cast<int>(std::wcslen(value));
    const int size = WideCharToMultiByte(
        CP_UTF8, 0, value, length, nullptr, 0, nullptr, nullptr);
    if (size <= 0)
        return {};
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(
        CP_UTF8, 0, value, length, result.data(), size, nullptr, nullptr);
    return result;
}
#endif

const char* auro_slot_name(std::uint32_t slot) {
    switch (slot) {
    case 0: return "FL";
    case 1: return "FR";
    case 2: return "C";
    case 3: return "LFE";
    case 4: return "LS";
    case 5: return "RS";
    case 6: return "CS";
    case 7: return "LB";
    case 8: return "RB";
    case 9: return "HL";
    case 10: return "HR";
    case 11: return "HC";
    case 12: return "T";
    case 13: return "HLS";
    case 14: return "HRS";
    case 15: return "HCS";
    case 16: return "HLB";
    case 17: return "HRB";
    case 18: return "LC";
    case 19: return "RC";
    case 20: return "LFE2";
    case 21: return "BL";
    case 22: return "BR";
    case 23: return "BC";
    case 24: return "BLS";
    case 25: return "BRS";
    case 26: return "OBJ";
    default: return "?";
    }
}

const char* native_reconstruction_note(std::uint32_t slot) {
    if (slot >= 9u && slot <= 17u)
        return "codec dematrix height";
    if ((slot >= 4u && slot <= 8u) || (slot >= 21u && slot <= 25u))
        return "codec reconstructed surround";
    if (slot == 2u)
        return "codec reconstructed center";
    if (slot == 3u || slot == 20u)
        return "codec reconstructed LFE";
    return "codec reconstructed channel";
}

std::uint32_t wav_channel_mask_from_slots(
    const std::vector<std::uint32_t>& slots,
    unsigned channel_count) {
    if (slots.size() < channel_count)
        return 0u;
    std::uint32_t mask = 0u;
    int previous_bit = -1;
    for (unsigned ch = 0; ch < channel_count; ++ch) {
        int bit = -1;
        switch (slots[ch]) {
        case 0: bit = 0; break;
        case 1: bit = 1; break;
        case 2: bit = 2; break;
        case 3: bit = 3; break;
        case 7: bit = 4; break;
        case 8: bit = 5; break;
        case 18: bit = 6; break;
        case 19: bit = 7; break;
        case 6: bit = 8; break;
        case 4: bit = 9; break;
        case 5: bit = 10; break;
        case 12: bit = 11; break;
        case 9: bit = 12; break;
        case 11: bit = 13; break;
        case 10: bit = 14; break;
        case 16: bit = 15; break;
        case 15: bit = 16; break;
        case 17: bit = 17; break;
        default: return 0u;
        }
        if (bit <= previous_bit)
            return 0u;
        previous_bit = bit;
        mask |= 1u << bit;
    }
    return mask;
}

// height_slot -> bed carrier used by Auro-Codec dematrix.
struct DematrixPair {
    std::uint32_t height_slot;
    std::uint32_t bed_slot;
};

struct DematrixRouteMap {
    std::uint32_t bed_mask = 0;
    std::array<std::uint32_t, 27> height_to_bed{};
    std::array<std::uint32_t, 27> bed_to_height{};

    DematrixRouteMap() {
        height_to_bed.fill(0xFFFFFFFFu);
        bed_to_height.fill(0xFFFFFFFFu);
    }
};

// Default vertical folds. Native HL/HR from a 7.1 carrier use FL/FR (confirmed
// on Amplitude16 DTS-HD @~23.6s: HL == inFL-outFL exactly while carrier SL is
// silent). Do not assume LS/RS folds for layout 7.1_5H_1T.
DematrixRouteMap build_dematrix_route_map(
    std::uint32_t input_mask,
    std::uint32_t native_mask,
    std::uint32_t layout_id,
    std::uint32_t carrier_layout_id) {
    (void)layout_id;
    (void)carrier_layout_id;
    static constexpr DematrixPair kDefaultPairs[] = {
        {9u, 0u},   // HL <- FL
        {10u, 1u},  // HR <- FR
        {11u, 2u},  // HC <- C
        {12u, 2u},  // T  <- C
        {13u, 4u},  // HLS <- LS
        {14u, 5u},  // HRS <- RS
        {15u, 6u},  // HCS <- CS
        {16u, 7u},  // HLB <- LB
        {17u, 8u},  // HRB <- RB
    };

    DematrixRouteMap map;
    for (const auto& pair : kDefaultPairs) {
        const std::uint32_t height = pair.height_slot;
        const std::uint32_t bed = pair.bed_slot;
        if (height >= 27u || bed >= 27u)
            continue;
        if (((native_mask >> height) & 1u) == 0u)
            continue;
        if (((input_mask >> bed) & 1u) == 0u)
            continue;
        map.height_to_bed[height] = bed;
        map.bed_to_height[bed] = height;
        map.bed_mask |= (1u << bed);
    }
    return map;
}

void print_decode_mode_channel_list(
    const char* label,
    const std::vector<std::uint32_t>& slots,
    std::uint32_t mask,
    bool want_in_mask) {
    std::cerr << label << "=";
    bool any = false;
    for (std::size_t ch = 0; ch < slots.size(); ++ch) {
        const std::uint32_t slot = slots[ch];
        const bool in_mask = slot < 31u && ((mask >> slot) & 1u) != 0u;
        if (in_mask != want_in_mask)
            continue;
        if (any)
            std::cerr << ",";
        std::cerr << "ch" << ch << "(" << auro_slot_name(slot) << ")";
        any = true;
    }
    if (!any)
        std::cerr << "none";
}

std::filesystem::path mono_channel_output_path(
    const std::string& output,
    const std::string& channel_name,
    const std::string& extension) {
    std::filesystem::path out_path(output);
    const std::filesystem::path dir = out_path.parent_path();
    const std::string stem = out_path.stem().string();
    std::filesystem::path mono_name = stem + " (" + channel_name + ")" + extension;
    return dir.empty() ? mono_name : (dir / mono_name);
}

std::string lowercase(std::string value) {
    for (char& c : value) {
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    }
    return value;
}

std::string selected_output_format(const Options& opt) {
    if (!opt.output_format.empty())
        return opt.output_format;
    return lowercase(std::filesystem::path(opt.output).extension().string()) == ".flac"
        ? "flac" : "wav";
}

std::string shell_quote(const std::filesystem::path& path) {
    std::string value = path.string();
    std::string quoted = "\"";
    for (char c : value) {
        if (c == '\"')
            quoted += '\\';
        quoted += c;
    }
    return quoted + "\"";
}

bool tool_runs(const char* name) {
#ifdef _WIN32
    const std::string command = std::string(name) + " -hide_banner -version >nul 2>&1";
#else
    const std::string command = std::string(name) + " -hide_banner -version >/dev/null 2>&1";
#endif
    return std::system(command.c_str()) == 0;
}

bool require_ffmpeg_tools() {
    const bool ffmpeg_ok = tool_runs("ffmpeg");
    const bool ffprobe_ok = tool_runs("ffprobe");
    if (ffmpeg_ok && ffprobe_ok)
        return true;
    if (!ffmpeg_ok)
        std::cerr << "ffmpeg not found in PATH (or failed to run)\n";
    if (!ffprobe_ok)
        std::cerr << "ffprobe not found in PATH (or failed to run)\n";
    std::cerr << "Install FFmpeg and ensure ffmpeg/ffprobe are available in PATH.\n";
    return false;
}

bool write_audio_file(
    const std::filesystem::path& output,
    const std::string& format,
    unsigned bits,
    unsigned sample_rate,
    unsigned channels,
    std::uint32_t channel_mask,
    const std::vector<std::uint8_t>& pcm,
    std::string& error_out,
    const auro3d::ProgressFn& progress = {}) {
    const char* save_stage = format == "flac" ? "save flac" : "save wav";
    if (format == "wav") {
        if (progress)
            progress(save_stage, -1);
        const bool ok = bits == 24
            ? wav::write_pcm24_le(output.string(), sample_rate, channels, pcm, error_out, channel_mask)
            : wav::write_pcm16_le(output.string(), sample_rate, channels, pcm, error_out);
        if (ok && progress)
            progress(save_stage, 100);
        return ok;
    }

    if (progress)
        progress("encode flac", -1);
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::filesystem::path temp = std::filesystem::temp_directory_path()
        / ("auro3d-decode-" + std::to_string(stamp) + ".wav");
    const bool wav_ok = bits == 24
        ? wav::write_pcm24_le(temp.string(), sample_rate, channels, pcm, error_out, channel_mask)
        : wav::write_pcm16_le(temp.string(), sample_rate, channels, pcm, error_out);
    if (!wav_ok)
        return false;

    const std::string command = "ffmpeg -y -v error -i " + shell_quote(temp)
        + " -map 0:a:0 -c:a flac " + shell_quote(output);
    const int status = std::system(command.c_str());
    std::error_code remove_error;
    std::filesystem::remove(temp, remove_error);
    if (status != 0) {
        error_out = "ffmpeg failed to encode FLAC (is ffmpeg available in PATH?)";
        return false;
    }
    if (progress)
        progress("encode flac", 100);
    return true;
}

std::string xml_escape(const std::string& value) {
    std::string escaped;
    for (char c : value) {
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
    unsigned sample_rate,
    unsigned bits_per_sample,
    unsigned channel_count,
    std::uint32_t source_layout_mask,
    const std::vector<std::uint32_t>& slots,
    std::uint32_t input_mask,
    std::uint32_t native_mask,
    std::uint32_t auromatic_mask,
    const DematrixRouteMap& dematrix,
    bool binaural,
    std::string& error_out) {
    std::filesystem::path xml_path = audio_path;
    xml_path.replace_extension(".xml");
    std::ofstream out(xml_path, std::ios::binary | std::ios::trunc);
    if (!out) {
        error_out = "cannot open channel mapping XML: " + xml_path.u8string();
        return false;
    }

    const char* source_layout = auro3d::auro_channel_layout_to_string(source_layout_mask);
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<channelMapping audioFile=\""
        << xml_escape(audio_path.filename().u8string())
        << "\" sourceFile=\""
        << xml_escape(source_path.filename().u8string())
        << "\" sampleRate=\"" << sample_rate
        << "\" sourceLayout=\""
        << xml_escape(source_layout[0] ? source_layout : "custom")
        << "\" sourceLayoutMask=\"0x" << std::hex << source_layout_mask << std::dec
        << "\" bitsPerSample=\"" << bits_per_sample
        << "\" channelCount=\"" << channel_count << "\">\n";
    for (unsigned ch = 0; ch < channel_count; ++ch) {
        const bool known_slot = ch < slots.size() && slots[ch] < 27u;
        const std::uint32_t slot = known_slot ? slots[ch] : 0xFFFFFFFFu;
        const std::uint32_t slot_bit = known_slot ? (1u << slot) : 0u;
        const char* source = "unknown";
        const char* dematrix_from = nullptr;
        const char* dematrix_height = nullptr;
        const char* note = nullptr;
        if (binaural) {
            source = "binaural_renderer";
        } else if ((input_mask & slot_bit) != 0u) {
            if ((dematrix.bed_mask & slot_bit) != 0u) {
                source = "carrier_dematrix";
                const std::uint32_t height = dematrix.bed_to_height[slot];
                if (height < 27u)
                    dematrix_height = auro_slot_name(height);
                note = "bed recovered by dematrix";
            } else {
                source = "carrier_passthrough";
            }
        } else if ((native_mask & slot_bit) != 0u) {
            source = "native_auro";
            const std::uint32_t bed = dematrix.height_to_bed[slot];
            if (bed < 27u)
                dematrix_from = auro_slot_name(bed);
            note = native_reconstruction_note(slot);
        } else if ((auromatic_mask & slot_bit) != 0u) {
            source = "auromatic";
            note = "XinN upmix from decoded bed";
        }
        out << "  <channel index=\"" << ch << "\" number=\"" << (ch + 1u) << "\"";
        if (known_slot) {
            out << " slot=\"" << slot << "\" name=\""
                << auro_slot_name(slot) << "\"";
        } else {
            out << " name=\"ch" << ch << "\"";
        }
        out << " source=\"" << source << "\"";
        if (dematrix_from)
            out << " dematrixFrom=\"" << dematrix_from << "\"";
        if (dematrix_height)
            out << " dematrixHeight=\"" << dematrix_height << "\"";
        if (note)
            out << " note=\"" << note << "\"";
        out << "/>\n";
    }
    out << "</channelMapping>\n";
    if (!out) {
        error_out = "failed to write channel mapping XML: " + xml_path.u8string();
        return false;
    }
    return true;
}

void print_channel_diagram(
    unsigned channels,
    const std::vector<std::uint32_t>& slots,
    std::uint32_t input_mask,
    std::uint32_t native_mask,
    std::uint32_t auromatic_mask,
    const DematrixRouteMap& dematrix) {
    std::uint32_t output_mask = 0u;
    for (unsigned ch = 0; ch < channels && ch < slots.size(); ++ch) {
        if (slots[ch] < 31u)
            output_mask |= 1u << slots[ch];
    }

    const char* input_layout = auro3d::auro_channel_layout_to_string(input_mask);
    const char* output_layout = auro3d::auro_channel_layout_to_string(output_mask);
    std::cerr << "channel_diagram "
              << (input_layout[0] ? input_layout : "custom") << " -> "
              << (output_layout[0] ? output_layout : "custom")
              << ":\n";
    for (unsigned ch = 0; ch < channels; ++ch) {
        const std::uint32_t slot = ch < slots.size() ? slots[ch] : 0xFFFFFFFFu;
        const char* name = slot < 27u ? auro_slot_name(slot) : "?";
        if (slot < 31u && ((input_mask >> slot) & 1u) != 0u) {
            if (slot < 27u && ((dematrix.bed_mask >> slot) & 1u) != 0u) {
                const std::uint32_t height = dematrix.bed_to_height[slot];
                std::cerr << "  carrier " << name << " + codec -> " << name
                          << " [dematrix bed]";
                if (height < 27u)
                    std::cerr << ", " << auro_slot_name(height) << " [native ORUA]";
                std::cerr << "\n";
            } else {
                std::cerr << "  carrier " << name << " -> " << name
                          << " [passthrough]\n";
            }
            continue;
        }

        if (slot < 31u && ((native_mask >> slot) & 1u) != 0u) {
            const std::uint32_t bed =
                (slot < 27u) ? dematrix.height_to_bed[slot] : 0xFFFFFFFFu;
            if (bed < 27u) {
                std::cerr << "  carrier " << auro_slot_name(bed)
                          << " + codec -> " << name << " [native ORUA dematrix]\n";
            } else {
                std::cerr << "  codec dematrix -> " << name << " [native ORUA]\n";
            }
            continue;
        }

        if (slot < 31u && ((auromatic_mask >> slot) & 1u) != 0u) {
            std::cerr << "  decoded bed -> " << name << " [Orua-Matic/XinN]\n";
            continue;
        }

        std::cerr << "  generated -> " << name << " [generated]\n";
    }
}

std::vector<std::uint8_t> extract_mono_channel_pcm(
    const std::vector<std::uint8_t>& interleaved,
    unsigned channels,
    unsigned channel_index,
    unsigned bytes_per_sample) {
    std::vector<std::uint8_t> mono;
    if (channels == 0 || channel_index >= channels || bytes_per_sample == 0)
        return mono;
    const std::size_t frame_bytes = static_cast<std::size_t>(channels) * bytes_per_sample;
    const std::size_t frames = interleaved.size() / frame_bytes;
    mono.resize(frames * bytes_per_sample);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const std::size_t src = frame * frame_bytes
            + static_cast<std::size_t>(channel_index) * bytes_per_sample;
        const std::size_t dst = frame * bytes_per_sample;
        for (unsigned b = 0; b < bytes_per_sample; ++b)
            mono[dst + b] = interleaved[src + b];
    }
    return mono;
}

void print_usage() {
    std::cerr
        << auro3d_decode::kName << " " << auro3d_decode::kVersion << " — ORUA command-line decoder.\n\n"
        << "Usage:\n"
        << "  " << auro3d_decode::kName << " -i <input.wav|input.flac|input.mkv|input.mp4|input.s24le> -o <output.wav|output.flac> [options]\n"
        << "  " << auro3d_decode::kName << " --probe -i <input>   # inspect without -o\n"
        << "Options:\n"
        << "  -i, --input FILE\n"
        << "  -o, --output FILE\n"
        << "  --output-format FMT  output format: wav or flac\n"
        << "  --raw                input is raw interleaved s24le (requires --rate and --channels)\n"
        << "  --rate HZ            sample rate for --raw\n"
        << "  --channels N         channel count for --raw\n"
        << "  --block N            internal block size; default aligns complete ORUA frames (832 fallback)\n"
        << "  --dsp-strength N     decoder/render strength (0..15; default: 12)\n"
        << "  --dsp-output-channels N  output channels; 0/omitted = auto from Orua metadata; native decode with Orua-Matic/XinN height fallback, up to "
        << auro3d::kCurrentNativeExportChannelLimit << "\n"
        << "                           legacy PCM without ORUA metadata: 6=5.1, 10=5.1.4, 12=7.1.4\n"
        << "  --output-bits N      output PCM depth: 16 or 24; default: 24\n"
        << "  --mono-tracks        additionally write mono files named <output stem> (FL).wav/.flac, etc.\n"
        << "  --channel-diagram    print structural input-to-output channel diagram\n"
        << "  --probe              print format diagnostics without decoding to a file; -o is not required\n"
        << "  --binaural           render decoded channels to HRTF stereo (force 48 kHz)\n"
        << "  --dsp-headroom-db X  headroom in dB (0..24; default: 0)\n"
        << "  --room-preset N      room preset ORUA (0=HOME,1=CONCERT,2=LOUNGE,3=CINEMA)\n"
        << "  --hrtf-preset N      HRTF preset (0=HPV2,1=GENERIC_1,2=GENERIC_2,3=GENERIC_3)\n"
        << "  --virtualizer-mode N virtualization mode (0=ENABLED,1=DISABLED)\n"
        // Disabled until headphone/stereo-device state affects the PCM path.
        // << "  --headphone N        headphone connected (0/1; default: 1)\n"
        // << "  --stereo-device N    stereo device connected (0/1; default: 1)\n"
        << "  -v, --verbose\n"
        << "  --version\n"
        << "  -h, --help\n\n"
        << "Requires ffmpeg and ffprobe in PATH.\n";
}

bool parse_unsigned_arg(const char* text, unsigned* out, const char* name) {
    if (!text || !out)
        return false;
    try {
        *out = static_cast<unsigned>(std::stoul(text));
        return true;
    } catch (const std::exception&) {
        std::cerr << "Invalid number for " << name << "\n";
        return false;
    }
}

bool parse_float_arg(const char* text, float* out, const char* name) {
    if (!text || !out)
        return false;
    try {
        *out = std::stof(text);
        return true;
    } catch (const std::exception&) {
        std::cerr << "Invalid number for " << name << "\n";
        return false;
    }
}

bool parse_args(int argc, char** argv, Options& opt) {
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto need = [&](const char* name) -> const char* {
            if (i + 1 >= argc) {
                std::cerr << "Expected a value after " << name << "\n";
                return nullptr;
            }
            return argv[++i];
        };

        if (a == "-h" || a == "--help") {
            opt.help_only = true;
            return true;
        }
        if (a == "--version") {
            opt.version_only = true;
            return true;
        }
        if (a == "--probe") {
            opt.probe = true;
            continue;
        }
        if (a == "-v" || a == "--verbose") {
            opt.verbose = true;
            continue;
        }
        if (a == "--mono-tracks") {
            opt.mono_tracks = true;
            continue;
        }
        if (a == "--channel-diagram") {
            opt.channel_diagram = true;
            continue;
        }
        if (a == "--binaural") {
            opt.binaural = true;
            continue;
        }
        if (a == "--raw") {
            opt.raw = true;
            continue;
        }
        if (a == "-i" || a == "--input") {
            const char* v = need(a.c_str());
            if (!v)
                return false;
            opt.input = v;
            continue;
        }
        if (a == "-o" || a == "--output") {
            const char* v = need(a.c_str());
            if (!v)
                return false;
            opt.output = v;
            continue;
        }
        if (a == "--output-format") {
            const char* v = need("--output-format");
            if (!v)
                return false;
            opt.output_format = lowercase(v);
            if (opt.output_format != "wav" && opt.output_format != "flac") {
                std::cerr << "--output-format: expected wav or flac\n";
                return false;
            }
            continue;
        }
        if (a == "--rate") {
            const char* v = need("--rate");
            if (!v || !parse_unsigned_arg(v, &opt.sample_rate, "--rate"))
                return false;
            continue;
        }
        if (a == "--channels") {
            const char* v = need("--channels");
            if (!v || !parse_unsigned_arg(v, &opt.channels, "--channels"))
                return false;
            continue;
        }
        if (a == "--block") {
            const char* v = need("--block");
            if (!v || !parse_unsigned_arg(v, &opt.block_size, "--block"))
                return false;
            continue;
        }
        if (a == "--dsp-strength") {
            const char* v = need("--dsp-strength");
            if (!v || !parse_unsigned_arg(v, &opt.dsp_strength, "--dsp-strength"))
                return false;
            if (auro3deng::strength_check_range(opt.dsp_strength) != 0) {
                std::cerr << "--dsp-strength: expected range 0..15\n";
                return false;
            }
            continue;
        }
        if (a == "--dsp-output-channels") {
            const char* v = need("--dsp-output-channels");
            if (!v || !parse_unsigned_arg(v, &opt.dsp_output_channels, "--dsp-output-channels"))
                return false;
            if (opt.dsp_output_channels > 64) {
                std::cerr << "--dsp-output-channels: expected range 0..64\n";
                return false;
            }
            continue;
        }
        if (a == "--output-bits") {
            const char* v = need("--output-bits");
            if (!v || !parse_unsigned_arg(v, &opt.output_bits, "--output-bits"))
                return false;
            if (opt.output_bits != 16 && opt.output_bits != 24) {
                std::cerr << "--output-bits: expected 16 or 24\n";
                return false;
            }
            continue;
        }
        if (a == "--dsp-headroom-db") {
            const char* v = need("--dsp-headroom-db");
            if (!v || !parse_float_arg(v, &opt.dsp_headroom_db, "--dsp-headroom-db"))
                return false;
            if (opt.dsp_headroom_db < 0.0f || opt.dsp_headroom_db > 24.0f) {
                std::cerr << "--dsp-headroom-db: expected range 0..24\n";
                return false;
            }
            continue;
        }
        if (a == "--room-preset") {
            const char* v = need("--room-preset");
            if (!v || !parse_unsigned_arg(v, &opt.room_preset, "--room-preset"))
                return false;
            if (opt.room_preset > 3) {
                std::cerr << "--room-preset: expected range 0..3\n";
                return false;
            }
            continue;
        }
        if (a == "--hrtf-preset") {
            const char* v = need("--hrtf-preset");
            if (!v || !parse_unsigned_arg(v, &opt.hrtf_preset, "--hrtf-preset"))
                return false;
            if (opt.hrtf_preset > 3) {
                std::cerr << "--hrtf-preset: expected range 0..3\n";
                return false;
            }
            continue;
        }
        if (a == "--virtualizer-mode") {
            const char* v = need("--virtualizer-mode");
            if (!v || !parse_unsigned_arg(v, &opt.virtualizer_mode, "--virtualizer-mode"))
                return false;
            if (opt.virtualizer_mode > 1) {
                std::cerr << "--virtualizer-mode: expected range 0..1\n";
                return false;
            }
            continue;
        }
        // Disabled until headphone/stereo-device state affects the PCM path.
        // if (a == "--headphone") { ... }
        // if (a == "--stereo-device") { ... }

        std::cerr << "Unknown argument: " << a << "\n";
        return false;
    }

    if (opt.input.empty() || (!opt.probe && opt.output.empty())) {
        std::cerr << "--input and --output are required\n";
        return false;
    }
    if (opt.raw && (opt.sample_rate == 0 || opt.channels == 0)) {
        std::cerr << "--raw requires --rate and --channels\n";
        return false;
    }
    return true;
}

} // namespace

int app_main(int argc, char** argv) {
    configure_console_encoding();

    Options opt{};
    if (!parse_args(argc, argv, opt)) {
        print_usage();
        return 1;
    }
    if (opt.help_only) {
        print_usage();
        return 0;
    }
    if (opt.version_only) {
        std::cout << auro3d_decode::kVersion << '\n';
        return 0;
    }
    if (!require_ffmpeg_tools())
        return 1;
    if (opt.probe && !opt.raw && auro3d::mp4_has_auro_cx_a3ds(opt.input)) {
        auro3d::AuroCxProbeInfo info{};
        const bool ok = auro3d::probe_auro_cx_mp4(opt.input, info);
        auro3d::print_auro_cx_probe(info);
        return ok ? 0 : 2;
    }
    if (opt.probe)
        opt.verbose = true;

    // Auto path: MP4 a3ds (AuroCX) wins over classic/native. Do not fall through
    // to auro_native if CX was selected but decode fails.
    if (!opt.probe && !opt.raw && auro3d::mp4_has_auro_cx_a3ds(opt.input)) {
        if (opt.verbose) {
            auro3d::AuroCxProbeInfo info{};
            if (!auro3d::probe_auro_cx_mp4(opt.input, info)) {
                std::cerr << "OruaCX probe: " << info.error << '\n';
                return 2;
            }
            auro3d::print_auro_cx_probe(info);
            std::cerr << "dsp_headroom_db=" << opt.dsp_headroom_db << "\n";
            std::cerr << "binaural=" << (opt.binaural ? 1 : 0)
                      << " room_preset=" << opt.room_preset
                      << " hrtf_preset=" << opt.hrtf_preset << "\n";
            if (opt.binaural) {
                std::cerr << "binaural_renderer=original_auro_ahp_ir"
                          << " room_preset=" << opt.room_preset
                          << " hrtf_bank=" << (opt.hrtf_preset == 0 ? "HPv2" : "Generic2")
                          << "\n";
            }
        }
        auro3d::ProgressReporter progress;
        std::string err;
        const bool ok = auro3d::decode_auro_cx_mp4(
            opt.input,
            opt.output,
            err,
            opt.dsp_headroom_db,
            opt.binaural,
            opt.room_preset,
            opt.hrtf_preset,
            progress.callback());
        progress.finish();
        if (!ok) {
            std::cerr << "OruaCX decode: " << err << '\n';
            return 2;
        }
        if (opt.verbose)
            std::cerr << "Done (OruaCX): " << opt.output << '\n';
        return 0;
    }

    auro3d::ProgressReporter progress;
    auro3d::Decoder dec;
    dec.set_progress_callback(progress.callback());
    dec.set_dsp_strength(opt.dsp_strength);
    dec.set_dsp_output_channels(opt.dsp_output_channels);
    dec.set_output_bits(opt.output_bits);
    dec.set_dsp_headroom_db(opt.dsp_headroom_db);
    dec.set_room_preset(opt.room_preset);
    dec.set_hrtf_preset(opt.hrtf_preset);
    dec.set_virtualizer_mode(opt.virtualizer_mode);
    // Disabled until headphone/stereo-device state affects the PCM path.
    // dec.set_output_audio_devices(opt.headphone_connected != 0, opt.stereo_device_connected != 0);
    if (opt.block_size != 0)
        dec.set_block_size(opt.block_size);
    if (opt.raw)
        dec.set_raw_pcm24_params(opt.sample_rate, opt.channels, opt.block_size);

    auro3d::DecodeError e = dec.open(opt.input);
    if (e != auro3d::DecodeError::Ok) {
        progress.finish();
        std::cerr << "open: " << auro3d::decode_error_message(e) << "\n";
        dec.close();
        return 2;
    }

    const auro3d::DecoderConfig cfg_open = dec.config();
    const std::string output_format = selected_output_format(opt);
    if (output_format == "flac" && cfg_open.channels > 8u) {
        progress.finish();
        std::cerr << "FLAC: format supports at most 8 channels; use a .wav output for "
                  << cfg_open.channels << " channels\n";
        dec.close();
        return 4;
    }
    const auro3d::NativeDecoderConfigState native_cfg = dec.native_config_state();
    const auro3d::NativeRuntimeConfigurationState runtime_cfg = dec.native_runtime_configuration();
    const auro3d::NativeA3dengStaticConfigurationState static_cfg = dec.native_a3deng_static_configuration();
    const auro3d::NativeDynamicParametersState dynamic_cfg = dec.native_dynamic_parameters();
    const auro3d::NativeA3dengRenderState render_cfg = dec.native_a3deng_render_state();
    const auro3d::AuroMetadataInfo auro_meta = dec.auro_metadata();
    std::vector<std::uint32_t> output_slots = dec.output_channel_slot_map();
    std::uint32_t native_mask = native_cfg.requested_output_mask & ~native_cfg.input_mask;
    std::uint32_t auromatic_mask = 0u;
    if (!auro_meta.found) {
        auromatic_mask = native_mask;
        native_mask = 0u;
    }
    const DematrixRouteMap dematrix_routes = build_dematrix_route_map(
        native_cfg.input_mask,
        native_mask,
        auro_meta.found ? auro_meta.layout_id : 0u,
        auro_meta.found ? auro_meta.carrier_layout_id : 0u);
    if (opt.verbose) {
        const char* native_input_layout = auro3d::auro_channel_layout_to_string(native_cfg.input_mask);
        const char* requested_output_layout = auro3d::auro_channel_layout_to_string(native_cfg.requested_output_mask);
        const char* effective_output_layout = auro3d::auro_channel_layout_to_string(native_cfg.effective_output_mask);
        std::cerr << "decode_channel_modes ";
        std::cerr << "input_layout=" << (native_input_layout[0] ? native_input_layout : "unknown")
                  << " requested_layout=" << (requested_output_layout[0] ? requested_output_layout : "unknown")
                  << " effective_layout=" << (effective_output_layout[0] ? effective_output_layout : "unknown")
                  << " ";
        print_decode_mode_channel_list("carrier_passthrough", output_slots, native_cfg.input_mask, true);
        std::cerr << " ";
        print_decode_mode_channel_list("native", output_slots, native_mask, true);
        std::cerr << " ";
        print_decode_mode_channel_list("auromatic", output_slots, auromatic_mask, true);
        if (auro_meta.found && auro_meta.has_closest_layout_without_mix3) {
            std::cerr << " auromatic_candidate_layout=0x" << std::hex
                      << auro_meta.closest_layout_without_mix3 << std::dec;
        }
        std::cerr << "\n";
        std::cerr << "sample_rate=" << cfg_open.sample_rate
                  << " channels=" << cfg_open.channels
                  << " block=" << cfg_open.block_size << "\n";
        if (cfg_open.channel_mask != 0) {
            std::cerr << "wav_channel_mask=0x" << std::hex << cfg_open.channel_mask << std::dec << "\n";
        }
        if (auro_meta.found) {
            std::cerr << "auro_layout_id=" << auro_meta.layout_id
                      << " layout=" << auro_meta.layout_name
                      << " metadata_channels=" << auro_meta.output_channels
                      << " carrier_layout_id=" << auro_meta.carrier_layout_id
                      << " carrier_layout=" << auro_meta.carrier_layout_name
                      << " carrier_channels=" << auro_meta.carrier_channels
                      << " uses_mix3=" << auro_meta.uses_mix3
                      << " carrier_channel=" << auro_meta.carrier_channel
                      << " sync_sample=" << auro_meta.sync_sample
                      << " metadata_block=" << auro_meta.block_size
                      << " scanned_sync_blocks=" << auro_meta.scanned_sync_blocks
                      << " adol_blocks=" << auro_meta.adol_block_count
                      << " unique_adol_instructions=" << auro_meta.adol_instructions.size() << "\n";
            if (auro_meta.has_closest_layout_without_mix3) {
                std::cerr << "auro_closest_without_mix3=" << auro_meta.closest_layout_without_mix3
                          << " layout=" << auro_meta.closest_layout_without_mix3_name << "\n";
            }
            for (const auto& ins : auro_meta.adol_instructions) {
                std::cerr << "  adol first_sync_block=" << ins.sync_block_index
                          << " block=" << ins.block_index
                          << " tag=" << ins.tag
                          << " opcode=0x" << std::hex << ins.opcode << std::dec
                          << " (" << ins.opcode << ")";
                if (ins.has_value) {
                    std::cerr << " value_bits=" << static_cast<unsigned>(ins.value_bits)
                              << " value=0x" << std::hex << ins.value << std::dec
                              << " (" << ins.value << ")";
                }
                if (ins.has_value2) {
                    std::cerr << " value2_bits=" << static_cast<unsigned>(ins.value2_bits)
                              << " value2=0x" << std::hex << ins.value2 << std::dec
                              << " (" << ins.value2 << ")";
                }
                if (ins.has_decoded_layout)
                    std::cerr << " decoded_layout=0x" << std::hex << ins.decoded_layout << std::dec;
                if (ins.has_carrier_layout)
                    std::cerr << " carrier_layout=0x" << std::hex << ins.carrier_layout << std::dec;
                if (ins.has_primary_downmix_gain)
                    std::cerr << " primary_downmix_ch=" << static_cast<unsigned>(ins.primary_downmix_channel)
                              << " primary_downmix_scaler=" << static_cast<unsigned>(ins.primary_downmix_scaler);
                if (ins.has_limit_simple)
                    std::cerr << " limit_simple_scaler=" << static_cast<unsigned>(ins.limit_simple_scaler);
                if (ins.has_secondary_downmix_gains)
                    std::cerr << " secondary_downmix=0x" << std::hex << ins.secondary_downmix_packed << std::dec;
                if (ins.has_auromatic)
                    std::cerr << " auromatic_profile=" << static_cast<unsigned>(ins.auromatic_profile)
                              << " auromatic_mode=" << static_cast<unsigned>(ins.auromatic_mode);
                if (ins.has_encoder_version)
                    std::cerr << " encoder_version=0x" << std::hex << ins.encoder_version << std::dec;
                if (ins.has_loudness)
                    std::cerr << " loudness_raw=0x" << std::hex << ins.loudness_raw << std::dec;
                std::cerr << " occurrences=" << ins.occurrences << "\n";
            }
        } else {
            std::cerr << "auro_layout_id=0 layout= metadata_channels=0\n";
            std::cerr << "legacy_auromatic=1 discrete_output=1"
                      << " virtualizer_bypassed=1 hrtf_bypassed=1\n";
        }
        std::cerr << "dsp_strength=" << opt.dsp_strength
                  << " gain=" << auro3deng::strength_translate(opt.dsp_strength) << "\n";
        std::cerr << "dsp_output_channels_request=" << opt.dsp_output_channels << "\n";
        std::cerr << "dsp_headroom_db=" << opt.dsp_headroom_db << "\n";
        std::cerr << "native_input_mask=0x" << std::hex << native_cfg.input_mask
                  << " requested_output_mask=0x" << native_cfg.requested_output_mask
                  << " effective_output_mask=0x" << native_cfg.effective_output_mask << std::dec << "\n";
        std::cerr << "native_input_dim=" << native_cfg.input_layout_dimension
                  << " requested_output_dim=" << native_cfg.requested_output_layout_dimension
                  << " effective_output_dim=" << native_cfg.effective_output_layout_dimension << "\n";
        std::cerr << "native_height input=" << native_cfg.input_has_height_layer
                  << " requested=" << native_cfg.requested_output_has_height_layer
                  << " effective=" << native_cfg.effective_output_has_height_layer << "\n";
        std::cerr << "runtime_virtualizer_mode=" << runtime_cfg.virtualizer_mode
                  << " effective_virtualizer_mode=" << runtime_cfg.effective_virtualizer_mode
                  << " room_preset=" << runtime_cfg.room_preset
                  << " hrtf_preset=" << runtime_cfg.hrtf_preset
                  << " target_device=" << runtime_cfg.target_device
                  << " is_abr=" << runtime_cfg.is_abr
                  << " listening_mode=" << runtime_cfg.derived_listening_mode
                  << " output_audio_bits=0x" << std::hex << runtime_cfg.output_audio_configuration_bits << std::dec
                  << "\n";
        std::cerr << "runtime_devices headphone=" << runtime_cfg.headphone_connected
                  << " stereo_device=" << runtime_cfg.stereo_device_connected
                  << " dynamic_request=" << runtime_cfg.dynamic_request_flag
                  << " dynamic_headphone=" << runtime_cfg.dynamic_headphone_flag << "\n";
        std::cerr << "a3deng_static block=" << static_cfg.pipeline_audio_block_size
                  << " output_mode=" << static_cfg.output_mode
                  << " hdmi_carrier=" << static_cfg.hdmi_carrier_valid
                  << " hdmi_quality=" << static_cfg.hdmi_quality
                  << " instance_created=" << static_cfg.instance_created << "\n";
        std::cerr << "a3deng_render input_rate=" << render_cfg.input_sample_rate
                  << " input_mask=0x" << std::hex << render_cfg.input_channel_mask
                  << " output_mask=0x" << render_cfg.output_channel_mask
                  << " pruned_output_mask=0x" << render_cfg.pruned_output_channel_mask << std::dec
                  << " input_block=" << render_cfg.input_block_count
                  << " output_parts=" << render_cfg.output_block_count
                  << " pruned_output_count=" << render_cfg.pruned_output_channel_count
                  << " pruned_valid=" << render_cfg.pruned_output_info_valid
                  << " pruned_max_sample=" << render_cfg.pruned_output_max_sample
                  << " pipeline_block=" << render_cfg.pipeline_audio_block_size
                  << " render_bytes=" << render_cfg.render_bytes_per_block
                  << " max_output_bytes=" << render_cfg.maximum_output_bytecount
                  << " jni_max_output=" << render_cfg.jni_maximum_output_bytecount_return
                  << " pop_return=" << render_cfg.pop_return_bytes
                  << " output_info=0x" << std::hex << render_cfg.output_info_packed
                  << " max_output_info=0x" << render_cfg.maximum_output_bytecount_packed << std::dec << "\n";
        std::cerr << "dynamic_params effective_virtualizer=" << dynamic_cfg.effective_virtualizer_mode
                  << " listening_mode=" << dynamic_cfg.listening_mode
                  << " room_preset=" << dynamic_cfg.room_preset
                  << " hrtf_preset=" << dynamic_cfg.hrtf_preset << "\n";
    }

    if (opt.probe) {
        progress.finish();
        dec.close();
        return 0;
    }

    std::vector<std::uint8_t> pcm_all;
    std::vector<std::uint8_t> chunk;
    progress.update("decode", 0);
    while (!dec.exhausted()) {
        e = dec.decode_next(chunk);
        if (e != auro3d::DecodeError::Ok) {
            progress.finish();
            std::cerr << "decode: " << auro3d::decode_error_message(e) << "\n";
            dec.close();
            return 3;
        }
        pcm_all.insert(pcm_all.end(), chunk.begin(), chunk.end());
        progress.update("decode", dec.decode_percent());
    }
    progress.done("decode");

    auro3d::DecoderConfig cfg = dec.config();
    const std::uint64_t dsp_clipped = dec.dsp_clipped_samples();
    const std::uint64_t latency_samples = dec.latency_samples();
    const std::uint64_t source_sample_count = dec.source_sample_count();
    dec.close();

    if (latency_samples != 0u) {
        const std::size_t bytes_per_sample = cfg.bits_per_sample == 24u ? 3u : 2u;
        const std::size_t bytes_per_frame = static_cast<std::size_t>(cfg.channels) * bytes_per_sample;
        const std::uint64_t trim_begin_u64 = latency_samples * bytes_per_frame;
        const std::uint64_t trim_size_u64 = source_sample_count * bytes_per_frame;
        if (trim_begin_u64 > pcm_all.size()
            || trim_size_u64 > pcm_all.size() - static_cast<std::size_t>(trim_begin_u64)) {
            progress.finish();
            std::cerr << "decode: native latency drain produced insufficient PCM\n";
            return 3;
        }
        const std::size_t trim_begin = static_cast<std::size_t>(trim_begin_u64);
        const std::size_t trim_size = static_cast<std::size_t>(trim_size_u64);
        std::vector<std::uint8_t> latency_compensated(
            pcm_all.begin() + trim_begin,
            pcm_all.begin() + trim_begin + trim_size);
        pcm_all.swap(latency_compensated);
    }

    if (pcm_all.empty()) {
        progress.finish();
        std::cerr << "No PCM data.\n";
        return 3;
    }

    std::string err;
    if (opt.binaural) {
        if (cfg.sample_rate != 48000u) {
            std::vector<std::uint8_t> resampled;
            if (!auro3d::resample_interleaved_pcm_to_rate(
                    pcm_all,
                    cfg.bits_per_sample,
                    cfg.channels,
                    cfg.sample_rate,
                    48000u,
                    resampled,
                    err,
                    progress.callback())) {
                progress.finish();
                std::cerr << "Binaural: " << err << "\n";
                return 4;
            }
            if (opt.verbose) {
                std::cerr << "binaural_resample=" << cfg.sample_rate
                          << "->" << 48000 << " Hz\n";
            }
            pcm_all.swap(resampled);
            cfg.sample_rate = 48000u;
            cfg.bits_per_sample = 24u;
        }
        std::vector<std::uint8_t> stereo;
        if (!auro3d::render_binaural_from_embedded_ir(
                pcm_all, cfg.bits_per_sample, cfg.sample_rate, cfg.channels,
                output_slots, opt.room_preset, opt.hrtf_preset, stereo, err,
                progress.callback())) {
            progress.finish();
            std::cerr << "Binaural: " << err << "\n";
            return 4;
        }
        pcm_all.swap(stereo);
        cfg.channels = 2;
        cfg.channel_mask = 3;
        output_slots = {0u, 1u};
        if (opt.verbose) {
            std::cerr << "binaural_renderer=original_auro_ahp_ir"
                      << " room_preset=" << opt.room_preset
                      << " hrtf_bank=" << (opt.hrtf_preset == 0 ? "HPv2" : "Generic2") << "\n";
        }
    }
    const std::uint32_t output_wav_channel_mask =
        wav_channel_mask_from_slots(output_slots, cfg.channels);
    const bool ok = write_audio_file(
        opt.output, output_format, cfg.bits_per_sample, cfg.sample_rate, cfg.channels,
        output_wav_channel_mask, pcm_all, err, progress.callback());
    if (!ok) {
        progress.finish();
        std::cerr << (output_format == "flac" ? "FLAC: " : "WAV: ") << err << "\n";
        return 4;
    }
    if (!write_channel_mapping_xml(
            std::filesystem::u8path(opt.output),
            std::filesystem::u8path(opt.input),
            cfg.sample_rate,
            cfg.bits_per_sample,
            cfg.channels,
            native_cfg.requested_output_mask,
            output_slots,
            native_cfg.input_mask,
            native_mask,
            auromatic_mask,
            dematrix_routes,
            opt.binaural,
            err)) {
        progress.finish();
        std::cerr << "XML: " << err << "\n";
        return 4;
    }
    if (opt.channel_diagram) {
        print_channel_diagram(
            cfg.channels,
            output_slots,
            native_cfg.input_mask,
            native_mask,
            auromatic_mask,
            dematrix_routes);
    }
    if (opt.mono_tracks) {
        const unsigned bytes_per_sample = cfg.bits_per_sample / 8u;
        for (unsigned ch = 0; ch < cfg.channels; ++ch) {
            std::string channel_name = "ch" + std::to_string(ch);
            if (ch < output_slots.size()) {
                const char* slot_name = auro_slot_name(output_slots[ch]);
                if (slot_name[0] != '?')
                    channel_name = slot_name;
            }
            const std::filesystem::path mono_path =
                mono_channel_output_path(opt.output, channel_name, output_format == "flac" ? ".flac" : ".wav");
            const std::vector<std::uint8_t> mono_pcm =
                extract_mono_channel_pcm(pcm_all, cfg.channels, ch, bytes_per_sample);
            const bool mono_ok = write_audio_file(
                mono_path, output_format, cfg.bits_per_sample, cfg.sample_rate, 1,
                0u, mono_pcm, err);
            if (!mono_ok) {
                progress.finish();
                std::cerr << (output_format == "flac" ? "FLAC mono " : "WAV mono ")
                          << mono_path.string() << ": " << err << "\n";
                return 4;
            }
        }
    }

    progress.finish();
    if (opt.verbose) {
        std::cerr << "bits_per_sample=" << cfg.bits_per_sample << "\n";
        std::cerr << "dsp_clipped_samples=" << dsp_clipped << "\n";
        std::cerr << "Done: " << opt.output << "\n";
    }
    return 0;
}

#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
    std::vector<std::string> utf8_args;
    utf8_args.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i)
        utf8_args.push_back(wide_to_utf8(argv[i]));

    std::vector<char*> narrow_argv;
    narrow_argv.reserve(utf8_args.size());
    for (std::string& arg : utf8_args)
        narrow_argv.push_back(arg.data());
    return app_main(argc, narrow_argv.data());
}
#else
int main(int argc, char** argv) {
    return app_main(argc, argv);
}
#endif
