#include "app_version.hpp"
#include "../../../common/app_build_date.hpp"
#include "cx_probe.hpp"
#include "cx_decode.hpp"
#include "decoder.hpp"
#include "output_layout.hpp"
#include "progress.hpp"
#include "restore_lfe.hpp"
#include "../io/wav_writer.hpp"
#include "../render/binaural_renderer.hpp"
#include "../util/auro3deng_strength.hpp"
#include "../auro3deng/detail/runtime_api.hpp"

#include <array>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <stdexcept>
#include <thread>
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
    bool binaural_reference_ir = false;
    bool restore_lfe = false;
    /// 0 = off; 1..8 = clear that many low PCM bits on export (toward zero).
    unsigned clear_output_lsb = 0;
    bool help_only = false;
    bool version_only = false;
    bool codec_v3_split_self_test = false;
    bool probe = false;
    unsigned sample_rate = 0;
    bool sample_rate_specified = false;
    unsigned channels = 0;
    bool channels_specified = false;
    unsigned block_size = 0;
    bool block_size_specified = false;
    unsigned dsp_strength = 12;
    bool dsp_strength_specified = false;
    auro3d::DspOutputLayoutRequest dsp_output;
    unsigned output_bits = 24;
    std::string output_format;
    float dsp_headroom_db = 0.0f;
    unsigned room_preset = auro3d::kDefaultRoomPreset;
    bool room_preset_specified = false;
    unsigned hrtf_preset = auro3d::kDefaultHrtfPreset;
    bool hrtf_preset_specified = false;
    unsigned virtualizer_mode = auro3d::kDefaultVirtualizerMode;
    bool virtualizer_mode_specified = false;
    unsigned headphone_connected = 1;
    unsigned stereo_device_connected = 1;
};

void configure_console_encoding() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    auro3d::console_style::enable_virtual_terminal();
}

bool console_color() {
    return auro3d::console_style::color_enabled_for_stderr();
}

void print_app_version(std::ostream& out, bool color) {
    auro3d::console_style::paint(
        out, color, auro3d::console_style::dim)
        << auro3d_decode::kVersion;
    auro3d::console_style::paint_reset(out, color) << ' ';
    auro3d::console_style::paint(
        out, color, auro3d::console_style::bright_magenta)
        << '(' << orua3d::build_date_iso() << ')';
    auro3d::console_style::paint_reset(out, color);
}

void print_banner_line_rainbow(const char* line, bool color, bool newline = true) {
    // Rainbow cycle: R Y G C B M
    static constexpr const char* kRainbow[] = {
        "\033[91m",
        "\033[93m",
        "\033[92m",
        "\033[96m",
        "\033[94m",
        "\033[95m",
    };
    if (!color) {
        std::cerr << '\r' << line << "\033[K";
        if (newline)
            std::cerr << '\n';
        else
            std::cerr << std::flush;
        return;
    }
    constexpr std::size_t n = sizeof(kRainbow) / sizeof(kRainbow[0]);
    std::size_t hue = 0;
    std::cerr << '\r';
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(line); *p;) {
        if (*p == ' ') {
            std::cerr << ' ';
            ++p;
            continue;
        }
        // Advance one UTF-8 code point so multi-byte glyphs stay intact.
        const unsigned char* start = p;
        if ((*p & 0x80u) == 0)
            ++p;
        else if ((*p & 0xE0u) == 0xC0u)
            p += 2;
        else if ((*p & 0xF0u) == 0xE0u)
            p += 3;
        else if ((*p & 0xF8u) == 0xF0u)
            p += 4;
        else
            ++p;
        std::cerr << kRainbow[hue % n];
        std::cerr.write(reinterpret_cast<const char*>(start),
            static_cast<std::streamsize>(p - start));
        ++hue;
    }
    std::cerr << auro3d::console_style::kReset << "\033[K";
    if (newline)
        std::cerr << '\n';
    else
        std::cerr << std::flush;
}

void print_banner() {
    // ₃ᴅ → 3D → ³ᴰ → 3D → …
    static constexpr const char* kThreeDFrames[] = {
        "₃ᴅ",
        "3D",
        "³ᴰ",
        "3D",
    };
    constexpr int kLogoLines = 1;
    constexpr int kCycles = 2;
    constexpr auto kFrameDelay = std::chrono::milliseconds(140);

    const bool color = console_color();
    const bool animate = auro3d::console_style::stderr_is_tty();
    auto middle = [](const char* three_d) {
        // Pad so glyph-width changes do not leave leftovers or wrap unevenly.
        std::string line = std::string("))) ORUA:") + three_d + " (((";
        while (line.size() < 20)
            line.push_back(' ');
        return line;
    };
    auto draw_logo = [&](const char* three_d) {
        print_banner_line_rainbow(middle(three_d).c_str(), color);
        std::cerr << std::flush;
    };
    auto draw_version = [&]() {
        print_app_version(std::cerr, color);
        std::cerr << "\033[K\n";
    };

    std::cerr << "\n\n";
    if (!animate) {
        draw_logo("3D");
    } else {
        std::cerr << auro3d::console_style::hide_cursor << std::flush;
        const int frame_count =
            static_cast<int>(sizeof(kThreeDFrames) / sizeof(kThreeDFrames[0]));
        const int total_frames = kCycles * frame_count;
        for (int n = 0; n < total_frames; ++n) {
            if (n > 0)
                std::cerr << "\033[" << kLogoLines << "A";
            draw_logo(kThreeDFrames[n % frame_count]);
            std::this_thread::sleep_for(kFrameDelay);
        }
        std::cerr << "\033[" << kLogoLines << "A";
        draw_logo("3D");
        std::cerr << auro3d::console_style::show_cursor << std::flush;
    }
    std::cerr << '\n';
    draw_version();
    std::cerr << '\n';
}

void print_status(const char* icon_color, const char* icon, const std::string& message) {
    const bool color = console_color();
    auro3d::console_style::paint(std::cerr, color, icon_color) << icon;
    auro3d::console_style::paint_reset(std::cerr, color) << ' ';
    auro3d::console_style::paint(std::cerr, color, auro3d::console_style::white) << message;
    auro3d::console_style::paint_reset(std::cerr, color) << '\n';
}

void print_error(const std::string& message) {
    print_status(auro3d::console_style::red, "✖", message);
}

void print_warning(const std::string& message) {
    print_status(auro3d::console_style::yellow, "!", message);
}

void print_ok(const std::string& message) {
    print_status(auro3d::console_style::bright_green, "✔", message);
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

/// Preferred Microsoft dwChannelMask bit for an Auro/ORUA export slot.
int preferred_wave_bit_for_auro_slot(std::uint32_t slot) {
    switch (slot) {
    case 0: return 0;   // FL  -> FRONT_LEFT
    case 1: return 1;   // FR  -> FRONT_RIGHT
    case 2: return 2;   // C   -> FRONT_CENTER
    case 3: return 3;   // LFE -> LOW_FREQUENCY
    case 7: return 4;   // LB  -> BACK_LEFT
    case 21: return 4;  // BL  -> BACK_LEFT
    case 8: return 5;   // RB  -> BACK_RIGHT
    case 22: return 5;  // BR  -> BACK_RIGHT
    case 18: return 6;  // LC  -> FRONT_LEFT_OF_CENTER
    case 19: return 7;  // RC  -> FRONT_RIGHT_OF_CENTER
    case 6: return 8;   // CS  -> BACK_CENTER
    case 23: return 8;  // BC  -> BACK_CENTER
    case 4: return 9;   // LS  -> SIDE_LEFT
    case 24: return 9;  // BLS -> SIDE_LEFT
    case 5: return 10;  // RS  -> SIDE_RIGHT
    case 25: return 10; // BRS -> SIDE_RIGHT
    case 12: return 11; // T   -> TOP_CENTER
    case 9: return 12;  // HL  -> TOP_FRONT_LEFT
    case 11: return 13; // HC  -> TOP_FRONT_CENTER
    case 10: return 14; // HR  -> TOP_FRONT_RIGHT
    case 13: return 15; // HLS -> TOP_BACK_LEFT
    case 16: return 15; // HLB -> TOP_BACK_LEFT
    case 15: return 16; // HCS -> TOP_BACK_CENTER
    case 14: return 17; // HRS -> TOP_BACK_RIGHT
    case 17: return 17; // HRB -> TOP_BACK_RIGHT
    default: return -1; // LFE2 / OBJ / unknown
    }
}

std::uint32_t wav_channel_mask_from_slots(
    const std::vector<std::uint32_t>& slots,
    unsigned channel_count) {
    if (slots.size() < channel_count)
        return 0u;
    std::uint32_t mask = 0u;
    int previous_bit = -1;
    for (unsigned ch = 0; ch < channel_count; ++ch) {
        const int bit = preferred_wave_bit_for_auro_slot(slots[ch]);
        if (bit < 0)
            return 0u;
        if (bit <= previous_bit)
            return 0u;
        previous_bit = bit;
        mask |= 1u << bit;
    }
    return mask;
}

struct WavStandardPlan {
    std::vector<std::uint32_t> slots;
    std::vector<unsigned> src_index; // dst channel -> source channel
    std::uint32_t channel_mask = 0;
    std::string error;
};

/// Map ORUA slots onto distinct WAVE speaker bits and sort into canonical order.
WavStandardPlan plan_wav_standard_layout(
    const std::vector<std::uint32_t>& slots,
    unsigned channel_count) {
    WavStandardPlan plan;
    if (slots.size() < channel_count || channel_count == 0u) {
        plan.error = "channel slot map is incomplete";
        return plan;
    }

    struct Entry {
        unsigned src = 0;
        std::uint32_t slot = 0;
        int bit = -1;
    };
    std::vector<Entry> entries;
    entries.reserve(channel_count);
    std::uint32_t used_bits = 0;

    for (unsigned ch = 0; ch < channel_count; ++ch) {
        const std::uint32_t slot = slots[ch];
        const int bit = preferred_wave_bit_for_auro_slot(slot);
        if (bit < 0) {
            plan.error = std::string("no WAVE speaker mapping for channel ")
                + auro_slot_name(slot);
            return plan;
        }
        if ((used_bits & (1u << bit)) != 0u) {
            plan.error = std::string("duplicate WAVE speaker mapping for channel ")
                + auro_slot_name(slot);
            return plan;
        }
        used_bits |= 1u << bit;
        entries.push_back(Entry{ch, slot, bit});
    }

    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        return a.bit < b.bit;
    });

    plan.slots.resize(channel_count);
    plan.src_index.resize(channel_count);
    for (unsigned i = 0; i < channel_count; ++i) {
        plan.slots[i] = entries[i].slot;
        plan.src_index[i] = entries[i].src;
        plan.channel_mask |= 1u << entries[i].bit;
    }
    return plan;
}

std::vector<std::uint8_t> remap_interleaved_pcm(
    const std::vector<std::uint8_t>& src,
    unsigned channels,
    unsigned bytes_per_sample,
    const std::vector<unsigned>& src_index) {
    std::vector<std::uint8_t> dst;
    if (channels == 0 || bytes_per_sample == 0 || src_index.size() < channels)
        return dst;
    const std::size_t frame_bytes = static_cast<std::size_t>(channels) * bytes_per_sample;
    if (frame_bytes == 0 || src.size() % frame_bytes != 0)
        return dst;
    const std::size_t frames = src.size() / frame_bytes;
    dst.resize(src.size());
    for (std::size_t f = 0; f < frames; ++f) {
        const std::uint8_t* sframe = src.data() + f * frame_bytes;
        std::uint8_t* dframe = dst.data() + f * frame_bytes;
        for (unsigned dch = 0; dch < channels; ++dch) {
            const unsigned sch = src_index[dch];
            std::memcpy(
                dframe + static_cast<std::size_t>(dch) * bytes_per_sample,
                sframe + static_cast<std::size_t>(sch) * bytes_per_sample,
                bytes_per_sample);
        }
    }
    return dst;
}

/// Zero the low `clear_bits` of each PCM sample (toward zero). Export-only:
/// strips residual Auro sync/ADOL from carrier_passthrough channels.
void clear_interleaved_pcm_lsbs(
    std::vector<std::uint8_t>& pcm,
    unsigned channels,
    unsigned bytes_per_sample,
    unsigned clear_bits) {
    if (clear_bits == 0u || channels == 0u
        || (bytes_per_sample != 2u && bytes_per_sample != 3u)) {
        return;
    }
    const std::size_t frame_bytes =
        static_cast<std::size_t>(channels) * bytes_per_sample;
    if (frame_bytes == 0u || pcm.size() % frame_bytes != 0u)
        return;
    const int step = 1 << static_cast<int>(clear_bits);
    const std::size_t samples = pcm.size() / bytes_per_sample;
    for (std::size_t i = 0; i < samples; ++i) {
        std::uint8_t* p = pcm.data() + i * bytes_per_sample;
        int v = 0;
        if (bytes_per_sample == 3u) {
            v = static_cast<int>(p[0])
                | (static_cast<int>(p[1]) << 8)
                | (static_cast<int>(p[2]) << 16);
            if (v & 0x800000)
                v -= 1 << 24;
            v = (v / step) * step;
            const unsigned u = static_cast<unsigned>(v) & 0xFFFFFFu;
            p[0] = static_cast<std::uint8_t>(u);
            p[1] = static_cast<std::uint8_t>(u >> 8);
            p[2] = static_cast<std::uint8_t>(u >> 16);
        } else {
            v = static_cast<int>(static_cast<std::int16_t>(
                static_cast<unsigned>(p[0]) | (static_cast<unsigned>(p[1]) << 8)));
            v = (v / step) * step;
            const auto s = static_cast<std::int16_t>(v);
            p[0] = static_cast<std::uint8_t>(static_cast<std::uint16_t>(s));
            p[1] = static_cast<std::uint8_t>(static_cast<std::uint16_t>(s) >> 8);
        }
    }
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
    const std::filesystem::path out_path = std::filesystem::u8path(output);
    const std::filesystem::path dir = out_path.parent_path();
    const std::string stem = out_path.stem().u8string();
    const std::filesystem::path mono_name =
        std::filesystem::u8path(stem + " (" + channel_name + ")" + extension);
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
    const std::string ext =
        lowercase(std::filesystem::u8path(opt.output).extension().u8string());
    if (ext == ".flac")
        return "flac";
    if (ext == ".w64")
        return "w64";
    return "wav";
}

void warn_output_extension_mismatch(const Options& opt, const std::string& format) {
    if (opt.output.empty())
        return;
    const std::string extension =
        lowercase(std::filesystem::u8path(opt.output).extension().u8string());
    const std::string expected = "." + format;
    if (!extension.empty() && extension != expected) {
        print_warning(
            "output extension " + extension + " does not match --output-format " + format
            + "; container format follows --output-format");
    }
}

std::string default_output_path(const Options& opt) {
    const std::filesystem::path input = std::filesystem::u8path(opt.input);
    const std::string stem = input.stem().u8string();
    const std::string tag = opt.binaural ? "_decoded_binaural" : "_decoded";
    std::string ext = ".wav";
    if (!opt.output_format.empty()) {
        if (opt.output_format == "flac")
            ext = ".flac";
        else if (opt.output_format == "w64")
            ext = ".w64";
    }
    const std::filesystem::path name = std::filesystem::u8path(stem + tag + ext);
    const std::filesystem::path parent = input.parent_path();
    return (parent.empty() ? name : (parent / name)).u8string();
}

std::string shell_quote(const std::filesystem::path& path) {
    std::string value = path.u8string();
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
        print_error("ffmpeg not found in PATH (or failed to run)");
    if (!ffprobe_ok)
        print_error("ffprobe not found in PATH (or failed to run)");
    print_error("Install FFmpeg and ensure ffmpeg/ffprobe are available in PATH.");
    return false;
}

std::string channel_names_csv(const std::vector<std::uint32_t>& slots) {
    std::string csv;
    for (std::size_t i = 0; i < slots.size(); ++i) {
        if (i)
            csv += ',';
        csv += auro_slot_name(slots[i]);
    }
    return csv;
}

const char* auro_commercial_format_name(std::uint32_t layout) {
    switch (layout) {
    case 1587u: return "Auro 222";          // 4.0+2H
    case 26163u: return "Auro 8.0";         // 4.0+4H
    case 26167u: return "Auro 9.0";         // 5.0+4H
    case 26175u: return "Auro 9.1";         // 5.1+4H
    case 30271u: return "Auro 10.1";        // 5.1+4H+T
    case 26551u: return "Auro 11.0";        // 7.0+4H
    case 32311u: return "Auro 11.0";        // 5.0+5H+T
    case 26559u: return "Auro 11.1";        // 7.1+4H
    case 32319u: return "Auro 11.1";        // 5.1+5H+T
    case 32695u: return "Auro 13.0";        // 7.0+5H+T
    case 32703u: return "Auro 13.1";        // 7.1+5H+T
    default:
        // Layout bits above the horizontal layer start at the height channels.
        return (layout & ~0x1FFu) == 0u ? "Auro 2D" : "Auro 3D";
    }
}

void print_auro_format(std::uint32_t layout, const char* audio_coding = nullptr) {
    if (layout == 0u)
        return;
    const char* layout_name = auro3d::auro_channel_layout_to_string(layout);
    std::cerr << "Auro format: " << auro_commercial_format_name(layout)
              << " | layout: ";
    if (layout_name[0] != '\0')
        std::cerr << layout_name;
    else
        std::cerr << "0x" << std::hex << layout << std::dec;
    if (audio_coding != nullptr && audio_coding[0] != '\0')
        std::cerr << " | audioCoding=" << audio_coding;
    std::cerr << '\n';
}

std::uint32_t auro_cx_stream_layout(const auro3d::AuroCxProbeInfo& info) {
    if (info.has_declared_layout && info.declared_layout != 0u)
        return info.declared_layout;
    std::uint32_t layout = 0u;
    if (info.schema_bed_channels_decoded) {
        for (const auto& channel : info.schema_bed_channels) {
            if (channel.id < 32u)
                layout |= 1u << channel.id;
        }
    }
    return layout;
}

unsigned bit_count(std::uint32_t value) {
    unsigned count = 0u;
    while (value != 0u) {
        value &= value - 1u;
        ++count;
    }
    return count;
}

unsigned auro_cx_output_channel_count(const auro3d::AuroCxProbeInfo& info) {
    const std::uint32_t layout = auro_cx_stream_layout(info);
    if (layout != 0u)
        return bit_count(layout);
    if (info.schema_bed_channels_decoded && !info.schema_bed_channels.empty())
        return static_cast<unsigned>(info.schema_bed_channels.size());
    return info.container_channels;
}

void print_auro_cx_channel_diagram(const auro3d::AuroCxProbeInfo& info) {
    const std::uint32_t layout = auro_cx_stream_layout(info);
    const char* layout_name = auro3d::auro_channel_layout_to_string(layout);
    std::cerr << "channel_diagram AuroCX -> "
              << (layout_name[0] ? layout_name : "custom") << ":\n";
    if (!info.schema_bed_channels_decoded || info.schema_bed_channels.empty()) {
        std::cerr << "  schema channel mapping is unavailable\n";
        return;
    }
    for (const auto& channel : info.schema_bed_channels) {
        const char* name = channel.id < 27u ? auro_slot_name(channel.id) : "?";
        std::cerr << "  audioStream " << channel.audio_stream_index
                  << " -> " << name << " [native AuroCX]\n";
    }
}

void warn_auro_cx_ignored_options(const Options& opt) {
    if (opt.block_size_specified)
        print_warning("--block ignored for AuroCX: block size is carried by the schema");
    if (opt.dsp_strength_specified)
        print_warning("--dsp-strength ignored for AuroCX: AWC/SASC reconstruction has no codec-v3 strength control");
    if (opt.restore_lfe)
        print_warning("--restore-lfe ignored for AuroCX: the streaming AuroCX path preserves its decoded LFE");
    if (opt.mono_tracks)
        print_warning("--mono-tracks ignored for AuroCX: per-channel export is not implemented for this path");
    if (opt.virtualizer_mode_specified)
        print_warning("--virtualizer-mode ignored for AuroCX: output is discrete PCM or explicit --binaural");
    if (!opt.binaural && opt.room_preset_specified)
        print_warning("--room-preset ignored for discrete AuroCX output; use it with --binaural");
    if (!opt.binaural && opt.hrtf_preset_specified)
        print_warning("--hrtf-preset ignored for discrete AuroCX output; use it with --binaural");
    if (opt.dsp_output.specified)
        print_warning("--dsp-output-channels validates AuroCX layout only; AuroCX remapping is not implemented");
}

wav::OutputMetadata make_output_metadata() {
    wav::OutputMetadata meta;
    meta.comment = auro3d_decode::make_decode_comment();
    return meta;
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
    const auro3d::ProgressFn& progress = {},
    const wav::OutputMetadata& metadata = {}) {
    const bool is_w64 = format == "w64";
    const char* save_stage = format == "flac" ? "save flac" : (is_w64 ? "save w64" : "save wav");
    if (format == "wav" || is_w64) {
        if (progress)
            progress(save_stage, -1);
        const wav::PcmContainer container =
            is_w64 ? wav::PcmContainer::W64 : wav::PcmContainer::WavAuto;
        const bool ok = bits == 24
            ? wav::write_pcm24_le(
                  output.u8string(), sample_rate, channels, pcm, error_out, channel_mask, metadata,
                  container)
            : wav::write_pcm16_le(
                  output.u8string(), sample_rate, channels, pcm, error_out, channel_mask, metadata,
                  container);
        if (ok && progress)
            progress(save_stage, 100);
        return ok;
    }

    if (progress)
        progress("encode flac", -1);
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::filesystem::path temp = std::filesystem::temp_directory_path()
        / ("orua3d-decode-" + std::to_string(stamp) + ".wav");
    const bool wav_ok = bits == 24
        ? wav::write_pcm24_le(
              temp.u8string(), sample_rate, channels, pcm, error_out, channel_mask, metadata)
        : wav::write_pcm16_le(
              temp.u8string(), sample_rate, channels, pcm, error_out, channel_mask, metadata);
    if (!wav_ok)
        return false;

    const bool flac_ok =
        wav::encode_wav_to_flac(temp.u8string(), output.u8string(), error_out, metadata);
    std::error_code remove_error;
    std::filesystem::remove(temp, remove_error);
    if (!flac_ok)
        return false;
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
    unsigned source_sample_rate,
    unsigned bits_per_sample,
    unsigned channel_count,
    std::uint32_t source_layout_mask,
    std::uint32_t carrier_layout_mask,
    const std::vector<std::uint32_t>& slots,
    std::uint32_t input_mask,
    std::uint32_t native_mask,
    std::uint32_t auromatic_mask,
    const DematrixRouteMap& dematrix,
    bool binaural,
    bool auro_codec_present,
    std::string& error_out) {
    std::filesystem::path xml_path = audio_path;
    xml_path.replace_extension(".xml");
    std::ofstream out(xml_path, std::ios::binary | std::ios::trunc);
    if (!out) {
        error_out = "cannot open channel mapping XML: " + xml_path.u8string();
        return false;
    }

    const char* output_layout = auro3d::auro_channel_layout_to_string(source_layout_mask);
    const char* carrier_layout = auro3d::auro_channel_layout_to_string(
        carrier_layout_mask != 0u ? carrier_layout_mask : input_mask);
    const std::string output_name = output_layout[0] ? output_layout : "custom";
    const std::string carrier_name = carrier_layout[0] ? carrier_layout : "";
    std::string source_layout = output_name;
    if (!carrier_name.empty() && carrier_name != output_name) {
        // Auro-Codec: carrier embeds a larger discrete layout.
        // Non-encoded Orua-Matic/XinN expansion is an upmix, not embedding.
        if (auro_codec_present)
            source_layout = carrier_name + " embedded " + output_name;
        else
            source_layout = carrier_name + " upmixed " + output_name;
    } else if (!carrier_name.empty()) {
        source_layout = carrier_name;
    }

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<channelMapping audioFile=\""
        << xml_escape(audio_path.filename().u8string())
        << "\" sourceFile=\""
        << xml_escape(source_path.filename().u8string())
        << "\" sampleRate=\"" << sample_rate
        << "\" sourceSampleRate=\"" << (source_sample_rate != 0u ? source_sample_rate : sample_rate)
        << "\" sourceLayout=\""
        << xml_escape(source_layout)
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
    const bool color = console_color();
    auto accent = [&](const char* text) -> std::string {
        if (!color)
            return text;
        return std::string(auro3d::console_style::bright_cyan) + text
            + auro3d::console_style::kReset;
    };
    auto author = [&](const std::string& text) -> std::string {
        if (!color)
            return text;
        return std::string(auro3d::console_style::bright_magenta) + text
            + auro3d::console_style::kReset;
    };
    auto dim = [&](const char* text) -> std::string {
        if (!color)
            return text;
        return std::string(auro3d::console_style::dim) + text
            + auro3d::console_style::kReset;
    };
    auto warn = [&](const char* text) -> std::string {
        if (!color)
            return text;
        return std::string(auro3d::console_style::red) + text
            + auro3d::console_style::kReset;
    };
    std::cerr
        << '\n'
        << accent(auro3d_decode::kName) << ' '
        << author(auro3d_decode::make_author()) << ' '
        << "— ORUA command-line decoder.\n\n"
        << accent("Usage") << ":\n"
        << "  " << auro3d_decode::kName
        << " -i <input.wav|input.flac|input.mkv|input.mp4|input.m2ts|input.dts|input.s24le>"
        << " [-o <output.wav|output.flac|output.w64>] [options]\n"
        << "  " << auro3d_decode::kName << " --probe -i <input>\n"
        << "  " << auro3d_decode::kName << " --channel-diagram -i <input>\n"
        << accent("Options") << ":\n"
        << "  -i, --input FILE\n"
        << "  -o, --output FILE    output path; default: <input>_decoded.wav"
           " or <input>_decoded_binaural.wav with --binaural"
           " (use --output-format flac|w64)\n"
        << "  --output-format FMT  output format: wav (no channel limit), flac ("
        << warn("MAX 8 channel") << "), or w64 (Sony Wave64, no 4 GiB limit)\n"
        << "  --raw                input is raw interleaved s24le (requires --rate and --channels)\n"
        << "  --rate HZ            sample rate for --raw\n"
        << "  --channels N         channel count for --raw\n"
        << "  --block N            codec-v3 host block; default: 832 (ignored for AuroCX)\n"
        << "  --dsp-strength N     codec-v3 decoded/upmix gain (0..15; default: 12)\n"
        << "  --dsp-output-channels LAYOUT  codec-v3 output layout; AuroCX exact-layout check\n"
        << "  --dsp-output-layout LAYOUT    alias of --dsp-output-channels\n"
        << "                       accepts named layout (5.1_4H, 7.1.4), mask, or legacy count\n"
        << "  --output-bits N      output PCM depth for codec-v3/AuroCX: 16 or 24; default: 24\n"
        << "  --clear-output-lsb [N]  clear low N PCM bits on export (default N=4; range 1..8); "
        << warn("WARNING") << "\n"
        << "                       strips residual sync/ADOL from passthrough channels\n"
        << "  --mono-tracks        codec-v3: additionally write one file per output channel\n"
        << "  --channel-diagram    print structural input-to-output channel diagram (no decode; -o not required)\n"
        << "  --probe              print format diagnostics without decoding to a file; -o is not required\n"
        << "  --binaural           render decoded channels to HRTF stereo (force 48 kHz)\n"
        << "  --restore-lfe        codec-v3 only, experimental: if LFE is silent/absent,\n"
        << "                       synthesize LFE from bed channels (mono sum + 120 Hz LPF, −10 dB);\n"
        << "  --dsp-headroom-db X  headroom in dB (0..24; default: 0)\n"
        << "  --room-preset N      binaural/XinN room: 0=HOME,1=CONCERT,2=LOUNGE,3=CINEMA\n"
        << "  --hrtf-preset N      binaural HRTF bank: 0=HPV2 or 2=GENERIC_2\n"
        << "                       GENERIC_1/GENERIC_3 are absent from the bundled IR resource\n"
        << "  --virtualizer-mode N reserved runtime state (0/1); current PCM path ignores it\n"
        // Disabled until headphone/stereo-device state affects the PCM path.
        // << "  --headphone N        headphone connected (0/1; default: 1)\n"
        // << "  --stereo-device N    stereo device connected (0/1; default: 1)\n"
        << "  -v, --verbose\n"
        << "  --version\n"
        << "  -h, --help\n\n"
        << dim("Requires ffmpeg and ffprobe in PATH.") << "\n";
}

bool parse_unsigned_arg(const char* text, unsigned* out, const char* name) {
    if (!text || !out)
        return false;
    try {
        const std::string value(text);
        if (value.empty() || value[0] == '-')
            throw std::invalid_argument("negative or empty");
        std::size_t parsed = 0;
        const unsigned long long number = std::stoull(value, &parsed, 10);
        if (parsed != value.size() || number > std::numeric_limits<unsigned>::max())
            throw std::invalid_argument("trailing characters or overflow");
        *out = static_cast<unsigned>(number);
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
        const std::string value(text);
        std::size_t parsed = 0;
        const float number = std::stof(value, &parsed);
        if (parsed != value.size() || !std::isfinite(number))
            throw std::invalid_argument("trailing characters or non-finite value");
        *out = number;
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
        if (a == "--self-test-codec-v3-split") {
            opt.codec_v3_split_self_test = true;
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
        if (a == "--binaural-reference-ir") {
            opt.binaural = true;
            opt.binaural_reference_ir = true;
            continue;
        }
        if (a == "--clear-output-lsb") {
            opt.clear_output_lsb = 4u;
            if (i + 1 < argc) {
                const char* maybe = argv[i + 1];
                bool all_digits = maybe && *maybe;
                for (const char* p = maybe; all_digits && *p; ++p)
                    all_digits = std::isdigit(static_cast<unsigned char>(*p)) != 0;
                if (all_digits) {
                    unsigned n = 0;
                    if (!parse_unsigned_arg(maybe, &n, "--clear-output-lsb"))
                        return false;
                    if (n < 1u || n > 8u) {
                        std::cerr << "--clear-output-lsb: expected 1..8\n";
                        return false;
                    }
                    opt.clear_output_lsb = n;
                    ++i;
                }
            }
            continue;
        }
        if (a.rfind("--clear-output-lsb=", 0) == 0) {
            unsigned n = 0;
            if (!parse_unsigned_arg(a.c_str() + 19, &n, "--clear-output-lsb"))
                return false;
            if (n < 1u || n > 8u) {
                std::cerr << "--clear-output-lsb: expected 1..8\n";
                return false;
            }
            opt.clear_output_lsb = n;
            continue;
        }
        if (a == "--restore-lfe") {
            opt.restore_lfe = true;
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
            if (opt.output_format != "wav" && opt.output_format != "flac"
                && opt.output_format != "w64") {
                std::cerr << "--output-format: expected wav, flac, or w64\n";
                return false;
            }
            continue;
        }
        if (a == "--rate") {
            const char* v = need("--rate");
            if (!v || !parse_unsigned_arg(v, &opt.sample_rate, "--rate"))
                return false;
            opt.sample_rate_specified = true;
            continue;
        }
        if (a == "--channels") {
            const char* v = need("--channels");
            if (!v || !parse_unsigned_arg(v, &opt.channels, "--channels"))
                return false;
            opt.channels_specified = true;
            continue;
        }
        if (a == "--block") {
            const char* v = need("--block");
            if (!v || !parse_unsigned_arg(v, &opt.block_size, "--block"))
                return false;
            opt.block_size_specified = true;
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
            opt.dsp_strength_specified = true;
            continue;
        }
        if (a == "--dsp-output-channels" || a == "--dsp-output-layout") {
            const char* flag = a.c_str();
            const char* v = need(flag);
            if (!v)
                return false;
            std::string parse_err;
            if (!auro3d::parse_dsp_output_layout_arg(v, opt.dsp_output, parse_err)) {
                std::cerr << flag << ": " << parse_err << "\n";
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
            opt.room_preset_specified = true;
            continue;
        }
        if (a == "--hrtf-preset") {
            const char* v = need("--hrtf-preset");
            if (!v || !parse_unsigned_arg(v, &opt.hrtf_preset, "--hrtf-preset"))
                return false;
            if (opt.hrtf_preset > 3) {
                std::cerr << "--hrtf-preset: expected 0..3\n";
                return false;
            }
            opt.hrtf_preset_specified = true;
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
            opt.virtualizer_mode_specified = true;
            continue;
        }
        // Disabled until headphone/stereo-device state affects the PCM path.
        // if (a == "--headphone") { ... }
        // if (a == "--stereo-device") { ... }

        std::cerr << "Unknown argument: " << a << "\n";
        return false;
    }

    if (opt.input.empty()) {
        std::cerr << "--input is required\n";
        return false;
    }
    if (!opt.probe && !opt.channel_diagram && opt.output.empty())
        opt.output = default_output_path(opt);
    if (opt.raw && (opt.sample_rate == 0 || opt.channels == 0)) {
        std::cerr << "--raw requires --rate and --channels\n";
        return false;
    }
    if (opt.binaural && !opt.binaural_reference_ir
        && (opt.room_preset != 0u || opt.hrtf_preset != 0u)) {
        std::cerr << "Binaural: stateful AHP/AM4HP supports only the captured "
                     "room-0/HPV2 configuration; use --binaural-reference-ir for "
                     "alternate room/HRTF profiles\n";
        return false;
    }
    if (opt.binaural && opt.hrtf_preset != 0u && opt.hrtf_preset != 2u) {
        std::cerr << "--hrtf-preset: bundled binaural IR supports only 0=HPV2 or 2=GENERIC_2\n";
        return false;
    }
    return true;
}

} // namespace

int app_main(int argc, char** argv) {
    configure_console_encoding();

    Options opt{};
    if (!parse_args(argc, argv, opt)) {
        print_banner();
        print_usage();
        return 1;
    }
    if (opt.help_only) {
        print_banner();
        print_usage();
        return 0;
    }
    if (opt.version_only) {
        print_app_version(
            std::cout,
            auro3d::console_style::color_enabled_for_stdout());
        std::cout << '\n';
        return 0;
    }
    if (opt.codec_v3_split_self_test) {
        std::string detail;
        const bool ok = auro3deng::codec_v3_split_equivalence_self_test(detail);
        std::cout << "codec_v3_split_equivalence=" << (ok ? "PASS" : "FAIL")
                  << " " << detail << '\n';
        return ok ? 0 : 5;
    }
    print_banner();
    if (!require_ffmpeg_tools())
        return 1;
    if (!opt.raw && opt.sample_rate_specified)
        print_warning("--rate ignored without --raw");
    if (!opt.raw && opt.channels_specified)
        print_warning("--channels ignored without --raw");

    const bool input_is_auro_cx =
        !opt.raw && auro3d::mp4_has_auro_cx_a3ds(opt.input);
    if ((opt.probe || opt.channel_diagram) && input_is_auro_cx) {
        auro3d::AuroCxProbeInfo info{};
        const bool ok = auro3d::probe_auro_cx_mp4(opt.input, info);
        if (!ok) {
            print_error("OruaCX probe: " + info.error);
            return 2;
        }
        if (opt.probe)
            auro3d::print_auro_cx_probe(info);
        if (opt.channel_diagram)
            print_auro_cx_channel_diagram(info);
        return ok ? 0 : 2;
    }
    if (opt.probe)
        opt.verbose = true;

    // Auto path: MP4 a3ds (AuroCX) wins over classic/native. Do not fall through
    // to auro_native if CX was selected but decode fails.
    if (!opt.probe && input_is_auro_cx) {
        auro3d::AuroCxProbeInfo format_info{};
        if (!auro3d::probe_auro_cx_mp4(opt.input, format_info)) {
            print_error("OruaCX probe: " + format_info.error);
            return 2;
        }
        if (opt.dsp_output.specified) {
            if (!opt.dsp_output.mask_resolved) {
                print_error("AuroCX: could not resolve --dsp-output-channels/--dsp-output-layout");
                return 2;
            }
            const std::uint32_t requested = opt.dsp_output.mask;
            if (!auro3d::auro_cx_output_layout_mask_allowed(requested)) {
                print_error(auro3d::auro_cx_invalid_output_layout_message(requested));
                return 2;
            }
            std::uint32_t stream_layout = 0u;
            if (format_info.has_declared_layout)
                stream_layout = format_info.declared_layout;
            if (stream_layout == 0u && format_info.schema_bed_channels_decoded) {
                for (const auto& ch : format_info.schema_bed_channels) {
                    if (ch.id < 32u)
                        stream_layout |= (1u << ch.id);
                }
            }
            if (stream_layout != 0u && requested != stream_layout) {
                print_error(auro3d::auro_cx_unsupported_remap_message(stream_layout, requested));
                return 2;
            }
        }
        warn_auro_cx_ignored_options(opt);
        if (opt.verbose) {
            auro3d::print_auro_cx_probe(format_info);
            std::cerr << "dsp_headroom_db=" << opt.dsp_headroom_db << "\n";
            std::cerr << "output_bits=" << opt.output_bits
                      << " clear_output_lsb=" << opt.clear_output_lsb << "\n";
            if (opt.dsp_output.specified) {
                std::cerr << "dsp_output_layout_request=" << opt.dsp_output.raw
                          << " mask=0x" << std::hex << opt.dsp_output.mask << std::dec << "\n";
            }
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
        std::size_t awc_pdu_count = 0u;
        const char* audio_coding =
            auro3d::auro_cx_awc_coding(format_info.schema_pdus, awc_pdu_count);
        print_auro_format(auro_cx_stream_layout(format_info), audio_coding);
        auro3d::ProgressReporter progress;
        std::string err;
        std::vector<std::string> decode_warnings;
        const std::string output_format = selected_output_format(opt);
        warn_output_extension_mismatch(opt, output_format);
        const unsigned output_channels = opt.binaural
            ? 2u
            : auro_cx_output_channel_count(format_info);
        if (output_format == "flac" && output_channels > 8u) {
            print_error(
                "FLAC: format supports at most 8 channels; use .wav or .w64 output for "
                + std::to_string(output_channels) + " channels");
            return 4;
        }
        const bool ok = auro3d::decode_auro_cx_mp4(
            opt.input,
            opt.output,
            err,
            opt.dsp_headroom_db,
            opt.output_bits,
            opt.clear_output_lsb,
            opt.binaural,
            opt.room_preset,
            opt.hrtf_preset,
            output_format,
            progress.callback(),
            &decode_warnings);
        progress.finish();
        if (!ok) {
            print_error("OruaCX decode: " + err);
            return 2;
        }
        for (const std::string& warning : decode_warnings)
            print_warning(warning);
        if (opt.verbose)
            print_ok("Done (OruaCX): " + opt.output);
        else
            print_ok("Done: " + opt.output);
        return 0;
    }

    if (opt.virtualizer_mode_specified) {
        print_warning(
            "--virtualizer-mode does not affect the current discrete/binaural PCM path; "
            "runtime state only");
    }
    if (!opt.binaural && opt.hrtf_preset_specified)
        print_warning("--hrtf-preset ignored for discrete codec-v3 output; use it with --binaural");

    auro3d::ProgressReporter progress;
    auro3d::Decoder dec;
    dec.set_progress_callback(progress.callback());
    dec.set_dsp_strength(opt.dsp_strength);
    if (opt.dsp_output.specified && opt.dsp_output.mask_resolved) {
        if (opt.dsp_output.is_legacy_count)
            dec.set_dsp_output_channels(opt.dsp_output.legacy_count);
        else
            dec.set_dsp_output_layout_mask(opt.dsp_output.mask);
    } else {
        dec.set_dsp_output_channels(0);
    }
    dec.set_output_bits(opt.output_bits);
    dec.set_dsp_headroom_db(opt.dsp_headroom_db);
    dec.set_room_preset(opt.room_preset);
    dec.set_hrtf_preset(opt.hrtf_preset);
    dec.set_virtualizer_mode(opt.virtualizer_mode);
    dec.set_binaural(opt.binaural);
    // Disabled until headphone/stereo-device state affects the PCM path.
    // dec.set_output_audio_devices(opt.headphone_connected != 0, opt.stereo_device_connected != 0);
    if (opt.block_size != 0)
        dec.set_block_size(opt.block_size);
    if (opt.raw)
        dec.set_raw_pcm24_params(opt.sample_rate, opt.channels, opt.block_size);

    auro3d::DecodeError e = dec.open(opt.input);
    if (e != auro3d::DecodeError::Ok) {
        progress.finish();
        if (!dec.last_error_detail().empty())
            print_error(dec.last_error_detail());
        else
            print_error(std::string("open: ") + auro3d::decode_error_message(e));
        dec.close();
        return 2;
    }

    const auro3d::DecoderConfig cfg_open = dec.config();
    if (!opt.binaural && opt.room_preset_specified
        && !dec.meta_auromatic_upmix() && !dec.legacy_auromatic_upmix()) {
        print_warning(
            "--room-preset ignored: it affects --binaural or codec-v3 XinN upmix only");
    }
    if (dec.meta_auromatic_upmix() && dec.meta_xinn_rate_decimation() > 1u) {
        const std::uint32_t host_hz = cfg_open.sample_rate;
        const std::uint32_t core_hz = dec.meta_xinn_core_rate() != 0u
            ? dec.meta_xinn_core_rate()
            : host_hz / dec.meta_xinn_rate_decimation();
        print_warning(
            "Orua-Matic/XinN: host "
            + std::to_string(host_hz)
            + " Hz runs on the native factor-2 matic_resample FIR at core "
            + std::to_string(core_hz)
            + " Hz;");
    } else if (dec.legacy_auromatic_upmix() && dec.legacy_auromatic_ffmpeg_downsampled()) {
        print_warning(
            "Orua-Matic/XinN: input "
            + std::to_string(dec.legacy_auromatic_source_rate_hz())
            + " Hz FFmpeg-downsampled to 48000 Hz before XinN (legacy, no metadata)");
    }
    const std::string output_format = selected_output_format(opt);
    warn_output_extension_mismatch(opt, output_format);
    if (!opt.probe && !opt.channel_diagram && !opt.binaural
        && output_format == "flac" && cfg_open.channels > 8u) {
        progress.finish();
        print_error(
            "FLAC: format supports at most 8 channels; use .wav or .w64 output for "
            + std::to_string(cfg_open.channels) + " channels");
        dec.close();
        return 4;
    }
    const auro3d::NativeDecoderConfigState native_cfg = dec.native_config_state();
    const auro3d::NativeRuntimeConfigurationState runtime_cfg = dec.native_runtime_configuration();
    const auro3d::NativeA3dengStaticConfigurationState static_cfg = dec.native_a3deng_static_configuration();
    const auro3d::NativeDynamicParametersState dynamic_cfg = dec.native_dynamic_parameters();
    const auro3d::NativeA3dengRenderState render_cfg = dec.native_a3deng_render_state();
    const auro3d::AuroMetadataInfo auro_meta = dec.auro_metadata();
    if (!opt.probe && !opt.channel_diagram && auro_meta.found)
        print_auro_format(auro_meta.layout_id);
    std::vector<std::uint32_t> output_slots = dec.output_channel_slot_map();
    std::uint32_t native_mask = native_cfg.requested_output_mask & ~native_cfg.input_mask;
    std::uint32_t auromatic_mask = 0u;
    if (!auro_meta.found) {
        auromatic_mask = native_mask;
        native_mask = 0u;
    } else if (native_cfg.requested_output_mask != 0u && auro_meta.layout_id != 0u
               && (native_cfg.requested_output_mask & auro_meta.layout_id) == auro_meta.layout_id
               && native_cfg.requested_output_mask != auro_meta.layout_id) {
        // Post-dematrix compatible upmix: extras beyond metadata layout.
        auromatic_mask = native_cfg.requested_output_mask & ~auro_meta.layout_id;
        native_mask = auro_meta.layout_id & ~native_cfg.input_mask;
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
        std::cerr << "dsp_output_channels_request="
                  << (opt.dsp_output.specified ? opt.dsp_output.raw : "0") << "\n";
        if (opt.dsp_output.mask_resolved) {
            std::cerr << "dsp_output_layout_mask=0x" << std::hex << opt.dsp_output.mask
                      << std::dec << "\n";
        }
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

    if (opt.probe || opt.channel_diagram) {
        if (opt.channel_diagram) {
            print_channel_diagram(
                cfg_open.channels,
                output_slots,
                native_cfg.input_mask,
                native_mask,
                auromatic_mask,
                dematrix_routes);
        }
        progress.finish();
        dec.close();
        return 0;
    }

    auro3d::DecoderConfig cfg = dec.config();
    const std::uint64_t latency_samples = dec.latency_samples();
    const std::uint64_t source_sample_count = dec.source_sample_count();
    const unsigned source_sample_rate = cfg.sample_rate;

    // Keep decoder/export slot order until after restore-lfe / binaural.
    // Discrete output is always remapped to canonical WAVE speaker-bit order.
    WavStandardPlan wav_std_plan;
    std::vector<unsigned> wav_std_src_index;
    std::vector<std::uint32_t> write_slots = output_slots;
    std::uint32_t output_wav_channel_mask =
        wav_channel_mask_from_slots(write_slots, cfg.channels);
    if (!opt.binaural) {
        wav_std_plan = plan_wav_standard_layout(output_slots, cfg.channels);
        if (!wav_std_plan.error.empty()) {
            progress.finish();
            print_error("WAVEFORMATEXTENSIBLE: " + wav_std_plan.error);
            dec.close();
            return 4;
        }
        write_slots = wav_std_plan.slots;
        wav_std_src_index = wav_std_plan.src_index;
        output_wav_channel_mask = wav_std_plan.channel_mask;
        if (opt.verbose) {
            std::cerr << "wave_format_extensible=1 channel_mask=0x" << std::hex << output_wav_channel_mask
                      << std::dec << " order=" << channel_names_csv(write_slots) << "\n";
        }
    }
    const wav::OutputMetadata output_meta = make_output_metadata();

    // Stream PCM24 WAV/RF64/W64 when no post-process needs the full buffer in RAM.
    const bool stream_pcm_out = (output_format == "wav" || output_format == "w64")
        && cfg.bits_per_sample == 24u
        && !opt.binaural
        && !opt.restore_lfe
        && !opt.mono_tracks;

    std::vector<std::uint8_t> pcm_all;
    std::vector<std::uint8_t> chunk;
    progress.update("decode", 0);

    if (stream_pcm_out) {
        wav::PcmStreamWriter wav_out;
        wav_out.set_metadata(output_meta);
        std::string err;
        const wav::PcmContainer container =
            output_format == "w64" ? wav::PcmContainer::W64 : wav::PcmContainer::WavAuto;
        if (!wav_out.open(
                opt.output,
                cfg.sample_rate,
                cfg.channels,
                source_sample_count,
                err,
                output_wav_channel_mask,
                container)) {
            progress.finish();
            print_error(std::string(output_format == "w64" ? "W64: " : "WAV: ") + err);
            dec.close();
            return 4;
        }

        std::uint64_t skip_frames = latency_samples;
        std::uint64_t remain_frames = source_sample_count;
        const std::size_t bytes_per_frame =
            static_cast<std::size_t>(cfg.channels) * 3u;

        while (!dec.exhausted()) {
            e = dec.decode_next(chunk);
            if (e != auro3d::DecodeError::Ok) {
                progress.finish();
                print_error(std::string("decode: ") + auro3d::decode_error_message(e));
                dec.close();
                return 3;
            }
            if (chunk.empty()) {
                progress.update("decode", dec.decode_percent());
                continue;
            }
            if (chunk.size() % bytes_per_frame != 0) {
                progress.finish();
                print_error("decode: PCM frame size mismatch");
                dec.close();
                return 3;
            }
            std::size_t frames = chunk.size() / bytes_per_frame;
            std::size_t frame_off = 0;
            if (skip_frames != 0u) {
                const std::uint64_t skip_now = std::min<std::uint64_t>(skip_frames, frames);
                frame_off = static_cast<std::size_t>(skip_now);
                skip_frames -= skip_now;
                frames -= frame_off;
            }
            if (frames != 0u && remain_frames != 0u) {
                const std::uint64_t take = std::min<std::uint64_t>(remain_frames, frames);
                std::vector<std::uint8_t> piece(
                    chunk.begin() + static_cast<std::ptrdiff_t>(frame_off * bytes_per_frame),
                    chunk.begin()
                        + static_cast<std::ptrdiff_t>((frame_off + static_cast<std::size_t>(take))
                            * bytes_per_frame));
                if (!wav_std_src_index.empty()) {
                    std::vector<std::uint8_t> remapped = remap_interleaved_pcm(
                        piece, cfg.channels, 3u, wav_std_src_index);
                    if (remapped.size() != piece.size()) {
                        progress.finish();
                        print_error("WAVEFORMATEXTENSIBLE: PCM remap failed");
                        dec.close();
                        return 4;
                    }
                    piece.swap(remapped);
                }
                if (opt.clear_output_lsb != 0u)
                    clear_interleaved_pcm_lsbs(piece, cfg.channels, 3u, opt.clear_output_lsb);
                if (!wav_out.write(piece, err)) {
                    progress.finish();
                    print_error(std::string(output_format == "w64" ? "W64: " : "WAV: ") + err);
                    dec.close();
                    return 4;
                }
                remain_frames -= take;
            }
            progress.update("decode", dec.decode_percent());
        }
        progress.done("decode");

        const std::uint64_t dsp_clipped = dec.dsp_clipped_samples();
        dec.close();

        if (skip_frames != 0u || remain_frames != 0u) {
            progress.finish();
            print_error("decode: native latency drain produced insufficient PCM");
            return 3;
        }
        progress.update(output_format == "w64" ? "save w64" : "save wav", -1);
        if (!wav_out.close(err)) {
            progress.finish();
            print_error(std::string(output_format == "w64" ? "W64: " : "WAV: ") + err);
            return 4;
        }
        progress.done(output_format == "w64" ? "save w64" : "save wav");
        if (opt.verbose && wav_out.uses_rf64())
            std::cerr << "wav_container=RF64\n";
        if (opt.verbose && wav_out.uses_w64())
            std::cerr << "wav_container=W64\n";
        if (opt.verbose && opt.clear_output_lsb != 0u)
            std::cerr << "clear_output_lsb=" << opt.clear_output_lsb << "\n";

        if (!write_channel_mapping_xml(
                std::filesystem::u8path(opt.output),
                std::filesystem::u8path(opt.input),
                cfg.sample_rate,
                source_sample_rate,
                cfg.bits_per_sample,
                cfg.channels,
                native_cfg.requested_output_mask,
                auro_meta.found ? auro_meta.carrier_layout_id : native_cfg.input_mask,
                write_slots,
                native_cfg.input_mask,
                native_mask,
                auromatic_mask,
                dematrix_routes,
                opt.binaural,
                auro_meta.found,
                err)) {
            progress.finish();
            print_error("XML: " + err);
            return 4;
        }
        progress.finish();
        if (opt.verbose) {
            std::cerr << "bits_per_sample=" << cfg.bits_per_sample << "\n";
            std::cerr << "dsp_clipped_samples=" << dsp_clipped << "\n";
        }
        print_ok("Done: " + opt.output);
        return 0;
    }

    while (!dec.exhausted()) {
        e = dec.decode_next(chunk);
        if (e != auro3d::DecodeError::Ok) {
            progress.finish();
            print_error(std::string("decode: ") + auro3d::decode_error_message(e));
            dec.close();
            return 3;
        }
        pcm_all.insert(pcm_all.end(), chunk.begin(), chunk.end());
        progress.update("decode", dec.decode_percent());
    }
    progress.done("decode");

    const std::uint64_t dsp_clipped = dec.dsp_clipped_samples();
    dec.close();

    if (latency_samples != 0u) {
        const std::size_t bytes_per_sample = cfg.bits_per_sample == 24u ? 3u : 2u;
        const std::size_t bytes_per_frame = static_cast<std::size_t>(cfg.channels) * bytes_per_sample;
        const std::uint64_t trim_begin_u64 = latency_samples * bytes_per_frame;
        const std::uint64_t trim_size_u64 = source_sample_count * bytes_per_frame;
        if (trim_begin_u64 > pcm_all.size()
            || trim_size_u64 > pcm_all.size() - static_cast<std::size_t>(trim_begin_u64)) {
            progress.finish();
            print_error("decode: native latency drain produced insufficient PCM");
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
        print_error("No PCM data.");
        return 3;
    }

    std::string err;
    if (opt.clear_output_lsb != 0u && !opt.binaural) {
        const std::size_t bytes_per_sample = cfg.bits_per_sample == 24u ? 3u : 2u;
        clear_interleaved_pcm_lsbs(
            pcm_all, cfg.channels, static_cast<unsigned>(bytes_per_sample), opt.clear_output_lsb);
        if (opt.verbose)
            std::cerr << "clear_output_lsb=" << opt.clear_output_lsb << "\n";
    }

    if (opt.restore_lfe) {
        progress.update("restore lfe", -1);
        bool applied = false;
        if (!auro3d::restore_lfe_if_silent(
                pcm_all,
                cfg.bits_per_sample,
                cfg.sample_rate,
                cfg.channels,
                output_slots,
                err,
                &applied)) {
            progress.finish();
            print_error("restore-lfe: " + err);
            return 4;
        }
        progress.done("restore lfe");
        if (opt.verbose) {
            std::cerr << "restore_lfe=" << (applied ? "synthesized" : "skipped_lfe_present")
                      << " (experimental: bed mono + 120 Hz LPF, -10 dB)\n";
        } else if (applied) {
            print_ok("restore-lfe: synthesized experimental LFE from bed channels");
        }
    }

    if (opt.binaural) {
        // The bundled renderer runs at 48 kHz; the stateful AHP/AM4HP path then
        // restores the input rate on its stereo output.
        const std::uint32_t binaural_input_rate = cfg.sample_rate;
        const bool restore_binaural_rate = !opt.binaural_reference_ir
            && binaural_input_rate != 48000u;
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
                print_error("Binaural: " + err);
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
            print_error("Binaural: " + err);
            return 4;
        }
        pcm_all.swap(stereo);
        cfg.channels = 2;
        cfg.channel_mask = 3;
        output_slots = {0u, 1u};
        if (restore_binaural_rate) {
            std::vector<std::uint8_t> restored;
            if (!auro3d::resample_interleaved_pcm_to_rate(
                    pcm_all,
                    cfg.bits_per_sample,
                    cfg.channels,
                    cfg.sample_rate,
                    binaural_input_rate,
                    restored,
                    err,
                    progress.callback())) {
                progress.finish();
                print_error("Binaural: " + err);
                return 4;
            }
            pcm_all.swap(restored);
            cfg.sample_rate = binaural_input_rate;
        }
        if (opt.output_bits == 16u && cfg.bits_per_sample == 24u) {
            std::vector<std::uint8_t> pcm16;
            if (!wav::convert_pcm24_to_pcm16(pcm_all, pcm16, err)) {
                progress.finish();
                print_error("Binaural: " + err);
                return 4;
            }
            pcm_all.swap(pcm16);
            cfg.bits_per_sample = 16u;
        }
        if (opt.clear_output_lsb != 0u) {
            clear_interleaved_pcm_lsbs(
                pcm_all,
                cfg.channels,
                cfg.bits_per_sample / 8u,
                opt.clear_output_lsb);
        }
        if (opt.verbose) {
            std::cerr << "binaural_renderer=original_auro_ahp_ir"
                      << " room_preset=" << opt.room_preset
                      << " hrtf_bank=" << (opt.hrtf_preset == 0 ? "HPv2" : "Generic2") << "\n";
        }
    }

    if (!opt.binaural) {
        if (wav_std_src_index.empty()) {
            wav_std_plan = plan_wav_standard_layout(output_slots, cfg.channels);
            if (!wav_std_plan.error.empty()) {
                progress.finish();
                print_error("WAVEFORMATEXTENSIBLE: " + wav_std_plan.error);
                return 4;
            }
            wav_std_src_index = wav_std_plan.src_index;
            write_slots = wav_std_plan.slots;
            output_wav_channel_mask = wav_std_plan.channel_mask;
        }
        const unsigned bytes_per_sample = cfg.bits_per_sample == 24u ? 3u : 2u;
        std::vector<std::uint8_t> remapped = remap_interleaved_pcm(
            pcm_all, cfg.channels, bytes_per_sample, wav_std_src_index);
        if (remapped.size() != pcm_all.size()) {
            progress.finish();
            print_error("WAVEFORMATEXTENSIBLE: PCM remap failed");
            return 4;
        }
        pcm_all.swap(remapped);
        output_slots = write_slots;
    } else {
        write_slots = output_slots;
        output_wav_channel_mask = wav_channel_mask_from_slots(output_slots, cfg.channels);
    }

    const bool ok = write_audio_file(
        opt.output, output_format, cfg.bits_per_sample, cfg.sample_rate, cfg.channels,
        output_wav_channel_mask, pcm_all, err, progress.callback(),
        make_output_metadata());
    if (!ok) {
        progress.finish();
        print_error(std::string(
                        output_format == "flac" ? "FLAC: "
                            : (output_format == "w64" ? "W64: " : "WAV: "))
            + err);
        return 4;
    }
    if (!write_channel_mapping_xml(
            std::filesystem::u8path(opt.output),
            std::filesystem::u8path(opt.input),
            cfg.sample_rate,
            source_sample_rate,
            cfg.bits_per_sample,
            cfg.channels,
            native_cfg.requested_output_mask,
            auro_meta.found ? auro_meta.carrier_layout_id : native_cfg.input_mask,
            write_slots,
            native_cfg.input_mask,
            native_mask,
            auromatic_mask,
            dematrix_routes,
            opt.binaural,
            auro_meta.found,
            err)) {
        progress.finish();
        print_error("XML: " + err);
        return 4;
    }
    if (opt.mono_tracks) {
        const unsigned bytes_per_sample = cfg.bits_per_sample / 8u;
        for (unsigned ch = 0; ch < cfg.channels; ++ch) {
            std::string channel_name = "ch" + std::to_string(ch);
            if (ch < write_slots.size()) {
                const char* slot_name = auro_slot_name(write_slots[ch]);
                if (slot_name[0] != '?')
                    channel_name = slot_name;
            }
            const std::string mono_ext = output_format == "flac" ? ".flac"
                : (output_format == "w64" ? ".w64" : ".wav");
            const std::filesystem::path mono_path =
                mono_channel_output_path(opt.output, channel_name, mono_ext);
            const std::vector<std::uint8_t> mono_pcm =
                extract_mono_channel_pcm(pcm_all, cfg.channels, ch, bytes_per_sample);
            wav::OutputMetadata mono_meta;
            mono_meta.comment = auro3d_decode::make_decode_comment();
            const bool mono_ok = write_audio_file(
                mono_path, output_format, cfg.bits_per_sample, cfg.sample_rate, 1,
                0u, mono_pcm, err, {}, mono_meta);
            if (!mono_ok) {
                progress.finish();
                print_error(
                    std::string(
                        output_format == "flac" ? "FLAC mono "
                            : (output_format == "w64" ? "W64 mono " : "WAV mono "))
                    + mono_path.u8string() + ": " + err);
                return 4;
            }
        }
    }

    progress.finish();
    if (opt.verbose) {
        std::cerr << "bits_per_sample=" << cfg.bits_per_sample << "\n";
        std::cerr << "dsp_clipped_samples=" << dsp_clipped << "\n";
    }
    print_ok("Done: " + opt.output);
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
