#include "app_version.hpp"
#include "cx_probe.hpp"
#include "cx_decode.hpp"
#include "decoder.hpp"
#include "../io/wav_writer.hpp"
#include "../render/binaural_renderer.hpp"
#include "../util/auro3deng_strength.hpp"

#include <chrono>
#include <cstdlib>
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
    float dsp_headroom_db = 6.0f;
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

bool write_audio_file(
    const std::filesystem::path& output,
    const std::string& format,
    unsigned bits,
    unsigned sample_rate,
    unsigned channels,
    const std::vector<std::uint8_t>& pcm,
    std::string& error_out) {
    if (format == "wav") {
        return bits == 24
            ? wav::write_pcm24_le(output.string(), sample_rate, channels, pcm, error_out)
            : wav::write_pcm16_le(output.string(), sample_rate, channels, pcm, error_out);
    }

    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::filesystem::path temp = std::filesystem::temp_directory_path()
        / ("auro3d-decode-" + std::to_string(stamp) + ".wav");
    const bool wav_ok = bits == 24
        ? wav::write_pcm24_le(temp.string(), sample_rate, channels, pcm, error_out)
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
    unsigned sample_rate,
    unsigned bits_per_sample,
    unsigned channel_count,
    const std::vector<std::uint32_t>& slots,
    std::uint32_t input_mask,
    std::uint32_t native_mask,
    std::uint32_t auromatic_mask,
    bool binaural,
    std::string& error_out) {
    std::filesystem::path xml_path = audio_path;
    xml_path.replace_extension(".xml");
    std::ofstream out(xml_path, std::ios::binary | std::ios::trunc);
    if (!out) {
        error_out = "cannot open channel mapping XML: " + xml_path.string();
        return false;
    }

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<channelMapping audioFile=\""
        << xml_escape(audio_path.filename().string())
        << "\" sampleRate=\"" << sample_rate
        << "\" bitsPerSample=\"" << bits_per_sample
        << "\" channelCount=\"" << channel_count << "\">\n";
    for (unsigned ch = 0; ch < channel_count; ++ch) {
        const bool known_slot = ch < slots.size() && slots[ch] < 27u;
        const std::uint32_t slot_bit = known_slot ? (1u << slots[ch]) : 0u;
        const char* source = "unknown";
        if (binaural)
            source = "binaural_renderer";
        else if ((input_mask & slot_bit) != 0u)
            source = "carrier_passthrough";
        else if ((native_mask & slot_bit) != 0u)
            source = "native_auro";
        else if ((auromatic_mask & slot_bit) != 0u)
            source = "auromatic";
        out << "  <channel index=\"" << ch << "\" number=\"" << (ch + 1u) << "\"";
        if (known_slot) {
            out << " slot=\"" << slots[ch] << "\" name=\""
                << auro_slot_name(slots[ch]) << "\"";
        } else {
            out << " name=\"ch" << ch << "\"";
        }
        out << " source=\"" << source << "\"/>\n";
    }
    out << "</channelMapping>\n";
    if (!out) {
        error_out = "failed to write channel mapping XML: " + xml_path.string();
        return false;
    }
    return true;
}

void print_channel_diagram(
    unsigned channels,
    const std::vector<std::uint32_t>& slots,
    std::uint32_t input_mask,
    std::uint32_t native_mask,
    std::uint32_t auromatic_mask) {
    std::vector<unsigned> source_channels;
    std::uint32_t output_mask = 0u;
    for (unsigned ch = 0; ch < channels && ch < slots.size(); ++ch) {
        if (slots[ch] < 31u)
            output_mask |= 1u << slots[ch];
        if (slots[ch] < 31u && ((input_mask >> slots[ch]) & 1u) != 0u)
            source_channels.push_back(ch);
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
            std::cerr << "  carrier " << name << " -> " << name
                      << " [native bed reconstruction]\n";
            continue;
        }

        const char* mode = slot < 31u && ((auromatic_mask >> slot) & 1u) != 0u
            ? "Auro-Matic" : (slot < 31u && ((native_mask >> slot) & 1u) != 0u ? "native AURO" : "generated");
        std::cerr << "  ";
        for (std::size_t i = 0; i < source_channels.size(); ++i) {
            if (i != 0u)
                std::cerr << "+";
            std::cerr << auro_slot_name(slots[source_channels[i]]);
        }
        if (source_channels.empty())
            std::cerr << "carrier";
        std::cerr << (std::string(mode) == "Auro-Matic" ? " -> " : " + codec data -> ")
                  << name << " [" << mode << "]\n";
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
        << auro3d_decode::kName << " " << auro3d_decode::kVersion << " — консольный декодер AURO на базе RE libauro.so / libauro3d.so.\n\n"
        << "Использование:\n"
        << "  " << auro3d_decode::kName << " -i <input.wav|input.flac|input.mkv|input.mp4|input.s24le> -o <output.wav|output.flac> [опции]\n"
        << "  " << auro3d_decode::kName << " --probe -i <input>   # диагностика без -o\n"
        << "  Путь декодирования/probe выбирается автоматически: MP4 с a3ds (AuroCX) → CX;\n"
        << "  иначе → classic auro_native (WAV/codec-v3 и т.п.).\n\n"
        << "Опции:\n"
        << "  -i, --input FILE\n"
        << "  -o, --output FILE\n"
        << "  --output-format FMT  output format: wav or flac; default is inferred from .flac extension\n"
        << "  --raw                вход — сырой interleaved s24le (требуются --rate --channels)\n"
        << "  --rate HZ            sample rate для --raw\n"
        << "  --channels N         число каналов для --raw\n"
        << "  --block N            block size в сэмплах; по умолчанию " << auro3d::kDefaultJniBlockSize << "\n"
        << "  --dsp-strength N     сила декодера/рендера (0..15; по умолчанию 12)\n"
        << "  --dsp-output-channels N  output channels; 0/omitted = auto from Auro metadata; native decode with Auro-Matic/XinN height fallback, up to "
        << auro3d::kCurrentNativeExportChannelLimit << "\n"
        << "                           legacy PCM without AURO metadata: 6=5.1, 10=5.1.4, 12=7.1.4\n"
        << "  --output-bits N      output PCM depth: 16 or 24; по умолчанию 24\n"
        << "  --mono-tracks        additionally write mono files named <output stem> (FL).wav/.flac, etc.\n"
        << "  --channel-diagram    print structural input-to-output channel diagram\n"
        << "  --probe              print format diagnostics without decoding to a file; -o is not required\n"
        << "                       MP4 a3ds (AuroCX) → schema/container probe; else → classic/native open info\n"
        << "  --binaural           render decoded channels to HRTF stereo (48 kHz)\n"
        << "  --dsp-headroom-db X  headroom в dB (0..24; по умолчанию 6)\n"
        << "  --room-preset N      room preset AURO (0=HOME,1=CONCERT,2=LOUNGE,3=CINEMA)\n"
        << "  --hrtf-preset N      HRTF preset (0=HPV2,1=GENERIC_1,2=GENERIC_2,3=GENERIC_3)\n"
        << "  --virtualizer-mode N virtualization mode (0=ENABLED,1=DISABLED)\n"
        << "  --headphone N        headphone connected (0/1; по умолчанию 1)\n"
        << "  --stereo-device N    stereo device connected (0/1; по умолчанию 1)\n"
        << "  -v, --verbose\n"
        << "  --version\n"
        << "  -h, --help\n";
}

bool parse_unsigned_arg(const char* text, unsigned* out, const char* name) {
    if (!text || !out)
        return false;
    try {
        *out = static_cast<unsigned>(std::stoul(text));
        return true;
    } catch (const std::exception&) {
        std::cerr << "Некорректное число для " << name << "\n";
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
        std::cerr << "Некорректное число для " << name << "\n";
        return false;
    }
}

bool parse_args(int argc, char** argv, Options& opt) {
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto need = [&](const char* name) -> const char* {
            if (i + 1 >= argc) {
                std::cerr << "Ожидается значение после " << name << "\n";
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
                std::cerr << "--output-format: допустимо wav или flac\n";
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
                std::cerr << "--dsp-strength: диапазон 0..15\n";
                return false;
            }
            continue;
        }
        if (a == "--dsp-output-channels") {
            const char* v = need("--dsp-output-channels");
            if (!v || !parse_unsigned_arg(v, &opt.dsp_output_channels, "--dsp-output-channels"))
                return false;
            if (opt.dsp_output_channels > 64) {
                std::cerr << "--dsp-output-channels: диапазон 0..64\n";
                return false;
            }
            continue;
        }
        if (a == "--output-bits") {
            const char* v = need("--output-bits");
            if (!v || !parse_unsigned_arg(v, &opt.output_bits, "--output-bits"))
                return false;
            if (opt.output_bits != 16 && opt.output_bits != 24) {
                std::cerr << "--output-bits: допустимо 16 или 24\n";
                return false;
            }
            continue;
        }
        if (a == "--dsp-headroom-db") {
            const char* v = need("--dsp-headroom-db");
            if (!v || !parse_float_arg(v, &opt.dsp_headroom_db, "--dsp-headroom-db"))
                return false;
            if (opt.dsp_headroom_db < 0.0f || opt.dsp_headroom_db > 24.0f) {
                std::cerr << "--dsp-headroom-db: диапазон 0..24\n";
                return false;
            }
            continue;
        }
        if (a == "--room-preset") {
            const char* v = need("--room-preset");
            if (!v || !parse_unsigned_arg(v, &opt.room_preset, "--room-preset"))
                return false;
            if (opt.room_preset > 3) {
                std::cerr << "--room-preset: диапазон 0..3\n";
                return false;
            }
            continue;
        }
        if (a == "--hrtf-preset") {
            const char* v = need("--hrtf-preset");
            if (!v || !parse_unsigned_arg(v, &opt.hrtf_preset, "--hrtf-preset"))
                return false;
            if (opt.hrtf_preset > 3) {
                std::cerr << "--hrtf-preset: диапазон 0..3\n";
                return false;
            }
            continue;
        }
        if (a == "--virtualizer-mode") {
            const char* v = need("--virtualizer-mode");
            if (!v || !parse_unsigned_arg(v, &opt.virtualizer_mode, "--virtualizer-mode"))
                return false;
            if (opt.virtualizer_mode > 1) {
                std::cerr << "--virtualizer-mode: диапазон 0..1\n";
                return false;
            }
            continue;
        }
        if (a == "--headphone") {
            const char* v = need("--headphone");
            if (!v || !parse_unsigned_arg(v, &opt.headphone_connected, "--headphone"))
                return false;
            if (opt.headphone_connected > 1) {
                std::cerr << "--headphone: диапазон 0..1\n";
                return false;
            }
            continue;
        }
        if (a == "--stereo-device") {
            const char* v = need("--stereo-device");
            if (!v || !parse_unsigned_arg(v, &opt.stereo_device_connected, "--stereo-device"))
                return false;
            if (opt.stereo_device_connected > 1) {
                std::cerr << "--stereo-device: диапазон 0..1\n";
                return false;
            }
            continue;
        }

        std::cerr << "Неизвестный аргумент: " << a << "\n";
        return false;
    }

    if (opt.input.empty() || (!opt.probe && opt.output.empty())) {
        std::cerr << "Нужны --input и --output\n";
        return false;
    }
    if (opt.raw && (opt.sample_rate == 0 || opt.channels == 0)) {
        std::cerr << "Для --raw нужны --rate и --channels\n";
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
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
        std::string err;
        const bool ok = auro3d::decode_auro_cx_mp4(opt.input, opt.output, err);
        if (!ok) {
            std::cerr << "AuroCX decode: " << err << '\n';
            return 2;
        }
        if (opt.verbose)
            std::cerr << "Готово (AuroCX): " << opt.output << '\n';
        return 0;
    }

    auro3d::Decoder dec;
    dec.set_dsp_strength(opt.dsp_strength);
    dec.set_dsp_output_channels(opt.dsp_output_channels);
    dec.set_output_bits(opt.output_bits);
    dec.set_dsp_headroom_db(opt.dsp_headroom_db);
    dec.set_room_preset(opt.room_preset);
    dec.set_hrtf_preset(opt.hrtf_preset);
    dec.set_virtualizer_mode(opt.virtualizer_mode);
    dec.set_output_audio_devices(opt.headphone_connected != 0, opt.stereo_device_connected != 0);
    if (opt.block_size != 0)
        dec.set_block_size(opt.block_size);
    if (opt.raw)
        dec.set_raw_pcm24_params(opt.sample_rate, opt.channels, opt.block_size);

    auro3d::DecodeError e = dec.open(opt.input);
    if (e != auro3d::DecodeError::Ok) {
        std::cerr << "open: " << auro3d::decode_error_message(e) << "\n";
        dec.close();
        return 2;
    }

    const auro3d::DecoderConfig cfg_open = dec.config();
    const auro3d::NativeDecoderConfigState native_cfg = dec.native_config_state();
    const auro3d::NativeRuntimeConfigurationState runtime_cfg = dec.native_runtime_configuration();
    const auro3d::NativeA3dengStaticConfigurationState static_cfg = dec.native_a3deng_static_configuration();
    const auro3d::NativeDynamicParametersState dynamic_cfg = dec.native_dynamic_parameters();
    const auro3d::NativeA3dengRenderState render_cfg = dec.native_a3deng_render_state();
    const auro3d::AuroMetadataInfo auro_meta = dec.auro_metadata();
    std::vector<std::uint32_t> output_slots = dec.output_channel_slot_map();
    constexpr std::uint32_t kLayout7_1_5H_1T = 0x7FBFu;
    constexpr std::uint32_t kCarrier7_1 = 0x01BFu;
    constexpr std::uint32_t kDirect7_1_2H = 0x07BFu;
    std::uint32_t native_mask = native_cfg.requested_output_mask & ~native_cfg.input_mask;
    std::uint32_t auromatic_mask = 0u;
    if (!auro_meta.found) {
        auromatic_mask = native_mask;
        native_mask = 0u;
    } else if (auro_meta.found
        && auro_meta.layout_id == kLayout7_1_5H_1T
        && auro_meta.carrier_layout_id == kCarrier7_1) {
        native_mask &= kDirect7_1_2H;
        auromatic_mask = native_cfg.requested_output_mask
            & ~native_cfg.input_mask
            & ~native_mask;
    }
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
        dec.close();
        return 0;
    }

    std::vector<std::uint8_t> pcm_all;
    std::vector<std::uint8_t> chunk;
    while (!dec.exhausted()) {
        e = dec.decode_next(chunk);
        if (e != auro3d::DecodeError::Ok) {
            std::cerr << "decode: " << auro3d::decode_error_message(e) << "\n";
            dec.close();
            return 3;
        }
        pcm_all.insert(pcm_all.end(), chunk.begin(), chunk.end());
    }

    auro3d::DecoderConfig cfg = dec.config();
    const std::uint64_t dsp_clipped = dec.dsp_clipped_samples();
    dec.close();

    if (pcm_all.empty()) {
        std::cerr << "Нет PCM данных.\n";
        return 3;
    }

    std::string err;
    if (opt.binaural) {
        std::vector<std::uint8_t> stereo;
        if (!auro3d::render_binaural_from_embedded_ir(
                pcm_all, cfg.bits_per_sample, cfg.sample_rate, cfg.channels,
                output_slots, opt.room_preset, opt.hrtf_preset, stereo, err)) {
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
    const std::string output_format = selected_output_format(opt);
    const bool ok = write_audio_file(
        opt.output, output_format, cfg.bits_per_sample, cfg.sample_rate, cfg.channels, pcm_all, err);
    if (!ok) {
        std::cerr << (output_format == "flac" ? "FLAC: " : "WAV: ") << err << "\n";
        return 4;
    }
    if (!write_channel_mapping_xml(
            opt.output,
            cfg.sample_rate,
            cfg.bits_per_sample,
            cfg.channels,
            output_slots,
            native_cfg.input_mask,
            native_mask,
            auromatic_mask,
            opt.binaural,
            err)) {
        std::cerr << "XML: " << err << "\n";
        return 4;
    }
    if (opt.channel_diagram) {
        print_channel_diagram(
            cfg.channels,
            output_slots,
            native_cfg.input_mask,
            native_mask,
            auromatic_mask);
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
                mono_path, output_format, cfg.bits_per_sample, cfg.sample_rate, 1, mono_pcm, err);
            if (!mono_ok) {
                std::cerr << (output_format == "flac" ? "FLAC mono " : "WAV mono ")
                          << mono_path.string() << ": " << err << "\n";
                return 4;
            }
        }
    }

    if (opt.verbose) {
        std::cerr << "bits_per_sample=" << cfg.bits_per_sample << "\n";
        std::cerr << "dsp_clipped_samples=" << dsp_clipped << "\n";
        std::cerr << "Готово: " << opt.output << "\n";
    }
    return 0;
}
