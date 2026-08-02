#include "app_version.hpp"
#include "../../../common/app_build_date.hpp"
#include "wav_input.hpp"
#include "pcm_input.hpp"
#include "../codec_v3/layout.hpp"
#include "../codec_v3/frame_descriptor.hpp"
#include "../codec_v3/encoder_config.hpp"
#include "../codec_v3/encode_groups.hpp"
#include "../codec_v3/encoder_thread_pool.hpp"
#include "../codec_v3/process_groups.hpp"
#include "../codec_v3/prepare_metadata.hpp"
#include "../codec_v3/prepare_mix.hpp"
#include "../codec_v3/channel_frame_plan.hpp"
#include "../codec_v3/unit_encoder.hpp"
#include "carrier_output.hpp"
#include "../../../auro3d-decode/src/app/progress.hpp"

#include <iostream>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

struct Options {
    std::string input;
    std::string output;
    std::string input_layout;
    std::string input_channel_order;
    std::string input_scaler_indices;
    std::string input_gains_db;
    std::string carrier_gains_db;
    std::string output_layout;
    std::uint32_t host_block = 832u;
    std::uint32_t unit_block = 1024u;
    std::uint32_t profile = 2u;
    // Reserve the fixed 112-bit Channel header plus the 48 mux bits occupied
    // by sync/CRC across the first sixteen carrier samples. Mode-dependent
    // seeds and parser records are already present in Group+184/+196.
    std::uint32_t reserve_extra_bits = 160u;
    bool seed_present = false;
    std::uint32_t seed = 0u;
    bool dither_enabled = true;
    bool cts_dmx = false;
    bool validate_only = false;
    bool trace = false;
    bool verbose = false;
    bool version_only = false;
    bool threads_present = false;
    std::uint32_t threads = 0u;
};

struct TemporaryInputWav {
    std::filesystem::path path;

    ~TemporaryInputWav() {
        if (!path.empty()) {
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }
    }
};

void print_app_version(std::ostream& out, bool color) {
    auro3d::console_style::paint(
        out, color, auro3d::console_style::dim)
        << auro3d_encode::kVersion;
    auro3d::console_style::paint_reset(out, color) << ' ';
    auro3d::console_style::paint(
        out, color, auro3d::console_style::bright_magenta)
        << '(' << orua3d::build_date_iso() << ')';
    auro3d::console_style::paint_reset(out, color);
}

void print_banner() {
    const bool color = auro3d::console_style::color_enabled_for_stderr();
    auro3d::console_style::paint(
        std::cerr, color, auro3d::console_style::bright_cyan)
        << "))) ORUA:3D (((";
    auro3d::console_style::paint_reset(std::cerr, color) << '\n';
    auro3d::console_style::paint(
        std::cerr, color, auro3d::console_style::cyan)
        << "codec-v3 encoder";
    auro3d::console_style::paint_reset(std::cerr, color) << ' ';
    print_app_version(std::cerr, color);
    std::cerr << "\n\n";
}

void print_usage() {
    std::cout
        << "Usage: orua3d-encode -i INPUT.wav|INPUT.flac --input-channel-order NAMES [--output OUTPUT.wav|flac]\n"
        << "       orua3d-encode --validate -i INPUT.wav|INPUT.flac --input-channel-order NAMES [--input-layout LAYOUT] [--output-layout LAYOUT]\n"
        << "\n"
        << "Input is multichannel PCM24 RIFF/WAVE or FLAC. FLAC is decoded through\n"
        << "ffmpeg to PCM24 WAV before encoding. The exact original layout is\n"
        << "derived from --input-channel-order, or from WAVEFORMATEXTENSIBLE\n"
        << "when that option is omitted. Optional --input-layout verifies\n"
        << "that scheme; optional --output-layout verifies the automatically\n"
        << "selected physical output layout. The embedded logical carrier layout\n"
        << "is selected independently (for example 7.1_4H -> 5.1).\n"
        << "Without --output, the encoder writes <input-stem>_auro.wav.\n"
        << "Supported layouts include 2.0, 2.1, 5.1, 7.1, 5.1_4H, 7.1_4H,\n"
        << "5.1_4H_1T, 7.1_4H_1T, and 7.1_5H_1T. NAMES is the interleaved-WAV order,\n"
        << "for example FL,FR,C,LFE,LS,RS,HL,HR,HLS,HRS.\n"
        << "--input-scaler-indices I0,I1,... optionally supplies one native\n"
        << "scaler index (0..255) per WAV channel in that same physical order.\n"
        << "--input-gains-db G0,G1,... supplies Dynamic original gains in dB\n"
        << "([-24,0]) per WAV channel; present non-empty tokens become Dynamic\n"
        << "gain records (set_dynamic_params). Empty tokens leave a channel unset.\n"
        << "--carrier-gains-db G0,G1,... supplies Dynamic carrier gains in dB\n"
        << "([-24,0]) per carrier-layout channel order; present tokens become the\n"
        << "second Dynamic gain table (status 396) that feeds secondary downmix.\n"
        << "--profile P selects the native codec-v3 profile (1..5; default 2).\n"
        << "--threads N selects total encoder threads (1..256); default uses\n"
        << "all detected logical processors. --threads 1 is deterministic serial mode.\n"
        << "--reserve-extra-bits N maps to native Encoder::reserve_extra_bits"
           " (default 160).\n"
        << "--seed N supplies Config+144 for deterministic native Rescaler dither.\n"
        << "--no-dither sets native Config+48 to zero.\n"
        << "--cts-dmx enables Config+136 cts_dmx_coeff_limit_ before scalar downmix.\n"
        << "-v, --verbose prints input, layout and output details.\n"
        << "--version prints the encoder version.\n"
        << "--trace prints one carrier/payload summary per complete unit.\n";
}

bool take_value(int& i, int argc, char** argv, std::string& value, const char* option) {
    if (++i >= argc) {
        std::cerr << "missing value after " << option << '\n';
        return false;
    }
    value = argv[i];
    return true;
}

bool supported_output_extension(const std::string& path) {
    const std::size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return false;
    std::string extension = path.substr(dot);
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return extension == ".wav" || extension == ".wave" || extension == ".flac";
}

bool is_flac_input(const std::string& path) {
    const std::filesystem::path input = std::filesystem::u8path(path);
    std::string extension = input.extension().u8string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return extension == ".flac";
}

std::string shell_quote_windows(const std::string& value) {
    std::string quoted = "\"";
    for (const char c : value) {
        if (c == '"')
            quoted += "\\\"";
        else
            quoted += c;
    }
    quoted += '"';
    return quoted;
}

bool decode_flac_to_temporary_wav(
    const std::string& input,
    TemporaryInputWav& temporary,
    std::string& error) {
    error.clear();
    std::error_code ec;
    const std::filesystem::path directory = std::filesystem::temp_directory_path(ec);
    if (ec) {
        error = "cannot locate temporary directory for FLAC input";
        return false;
    }
    const auto nonce = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    temporary.path = directory / ("orua3d_encode_flac_" + std::to_string(nonce) + ".wav");
    const std::string command = "ffmpeg -y -v error -i "
        + shell_quote_windows(input)
        + " -map 0:a:0 -c:a pcm_s24le "
        + shell_quote_windows(temporary.path.u8string());
    if (std::system(command.c_str()) != 0) {
        error = "ffmpeg failed to decode FLAC input (is ffmpeg available in PATH?)";
        return false;
    }
    if (!std::filesystem::exists(temporary.path, ec) || ec) {
        error = "ffmpeg produced no temporary WAV for FLAC input";
        return false;
    }
    return true;
}

std::string default_output_path(const std::string& input, std::uint32_t layout_mask) {
    std::filesystem::path path = std::filesystem::u8path(input);
    path.replace_extension();
    std::string layout = auro3d::encode::layout_label(layout_mask);
    std::replace(layout.begin(), layout.end(), '_', '.');
    path += "_" + layout + "_auro.wav";
    return path.u8string();
}

bool parse_decimal_u32(
    const std::string& text,
    std::uint32_t minimum,
    std::uint32_t maximum,
    std::uint32_t& value) {
    if (text.empty() || text.front() == '-')
        return false;
    std::size_t consumed = 0u;
    unsigned long long parsed = 0u;
    try {
        parsed = std::stoull(text, &consumed, 10);
    } catch (...) {
        return false;
    }
    if (consumed != text.size()
        || parsed < minimum
        || parsed > maximum) {
        return false;
    }
    value = static_cast<std::uint32_t>(parsed);
    return true;
}

bool parse_scaler_indices(
    const std::string& text,
    const std::vector<std::uint32_t>& input_order,
    std::array<std::uint8_t, 31>& indices,
    std::string& error) {
    error.clear();
    indices.fill(0u);
    std::vector<std::uint32_t> values;
    std::istringstream stream(text);
    std::string token;
    while (std::getline(stream, token, ',')) {
        const std::size_t first = token.find_first_not_of(" \t");
        const std::size_t last = token.find_last_not_of(" \t");
        if (first == std::string::npos) {
            error = "input scaler index list contains an empty value";
            return false;
        }
        token = token.substr(first, last - first + 1u);
        std::size_t consumed = 0u;
        unsigned long value = 0u;
        try {
            value = std::stoul(token, &consumed, 10);
        } catch (...) {
            error = "input scaler index is not an integer";
            return false;
        }
        if (consumed != token.size() || value > 255u) {
            error = "input scaler index must be in 0..255";
            return false;
        }
        values.push_back(static_cast<std::uint32_t>(value));
    }
    if (values.size() != input_order.size()) {
        error = "input scaler index count must equal WAV channel count";
        return false;
    }
    for (std::size_t physical = 0u; physical < values.size(); ++physical) {
        const std::uint32_t channel = input_order[physical];
        if (channel >= indices.size()) {
            error = "input scaler channel ID exceeds codec-v3 range";
            return false;
        }
        indices[channel] = static_cast<std::uint8_t>(values[physical]);
    }
    return true;
}

bool parse_input_gains_db(
    const std::string& text,
    const std::vector<std::uint32_t>& input_order,
    auro3d::encode::DynamicParams& params,
    std::string& error) {
    error.clear();
    std::vector<std::string> tokens;
    std::istringstream stream(text);
    std::string token;
    while (std::getline(stream, token, ','))
        tokens.push_back(token);
    if (tokens.size() != input_order.size()) {
        error = "input gain count must equal WAV channel count";
        return false;
    }
    for (std::size_t physical = 0u; physical < tokens.size(); ++physical) {
        std::string item = tokens[physical];
        const std::size_t first = item.find_first_not_of(" \t");
        const std::size_t last = item.find_last_not_of(" \t");
        if (first == std::string::npos)
            continue;
        item = item.substr(first, last - first + 1u);
        std::size_t consumed = 0u;
        float gain = 0.0f;
        try {
            gain = std::stof(item, &consumed);
        } catch (...) {
            error = "input gain is not a floating-point value";
            return false;
        }
        if (consumed != item.size()
            || !std::isfinite(gain)
            || gain < -24.0f
            || gain > 0.0f) {
            error = "input gain must be finite and in [-24,0] dB";
            return false;
        }
        const std::uint32_t channel = input_order[physical];
        if (channel >= params.original_gains.size()) {
            error = "input gain channel ID exceeds codec-v3 range";
            return false;
        }
        params.original_gains[channel].present = true;
        params.original_gains[channel].gain_db = gain;
    }
    return true;
}

bool parse_carrier_gains_db(
    const std::string& text,
    const std::vector<std::uint32_t>& carrier_order,
    auro3d::encode::DynamicParams& params,
    std::string& error) {
    error.clear();
    std::vector<std::string> tokens;
    std::istringstream stream(text);
    std::string token;
    while (std::getline(stream, token, ','))
        tokens.push_back(token);
    if (tokens.size() != carrier_order.size()) {
        error = "carrier gain count must equal carrier channel count";
        return false;
    }
    for (std::size_t physical = 0u; physical < tokens.size(); ++physical) {
        std::string item = tokens[physical];
        const std::size_t first = item.find_first_not_of(" \t");
        const std::size_t last = item.find_last_not_of(" \t");
        if (first == std::string::npos)
            continue;
        item = item.substr(first, last - first + 1u);
        std::size_t consumed = 0u;
        float gain = 0.0f;
        try {
            gain = std::stof(item, &consumed);
        } catch (...) {
            error = "carrier gain is not a floating-point value";
            return false;
        }
        if (consumed != item.size()
            || !std::isfinite(gain)
            || gain < -24.0f
            || gain > 0.0f) {
            error = "carrier gain must be finite and in [-24,0] dB";
            return false;
        }
        const std::uint32_t channel = carrier_order[physical];
        if (channel >= params.carrier_gains.size()) {
            error = "carrier gain channel ID exceeds codec-v3 range";
            return false;
        }
        params.carrier_gains[channel].present = true;
        params.carrier_gains[channel].gain_db = gain;
    }
    return true;
}

std::uint64_t carrier_digest(
    const auro3d::encode::EncodedCarrierUnit& unit,
    const std::vector<std::uint32_t>& channel_order) {
    // FNV-1a over the emitted channel-id/sample order.  This is diagnostic
    // only and deliberately hashes the final post-metadata carrier planes.
    std::uint64_t digest = 1469598103934665603ull;
    for (const std::uint32_t channel : channel_order) {
        digest ^= channel;
        digest *= 1099511628211ull;
        for (const std::int32_t sample : unit.carrier.planes[channel]) {
            const std::uint32_t raw = static_cast<std::uint32_t>(sample);
            for (std::uint32_t byte = 0u; byte < 4u; ++byte) {
                digest ^= (raw >> (byte * 8u)) & 0xFFu;
                digest *= 1099511628211ull;
            }
        }
    }
    return digest;
}

void configure_console_encoding() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    auro3d::console_style::enable_virtual_terminal();
}

bool parse_options(int argc, char** argv, Options& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_banner();
            print_usage();
            return false;
        }
        if (arg == "--input" || arg == "-i") {
            if (!take_value(i, argc, argv, options.input, "--input"))
                return false;
        } else if (arg == "--output") {
            if (!take_value(i, argc, argv, options.output, "--output"))
                return false;
        } else if (arg == "--input-layout") {
            if (!take_value(i, argc, argv, options.input_layout, "--input-layout"))
                return false;
        } else if (arg == "--input-channel-order") {
            if (!take_value(i, argc, argv, options.input_channel_order, "--input-channel-order"))
                return false;
        } else if (arg == "--input-scaler-indices") {
            if (!take_value(
                    i, argc, argv, options.input_scaler_indices,
                    "--input-scaler-indices")) {
                return false;
            }
        } else if (arg == "--input-gains-db") {
            if (!take_value(
                    i, argc, argv, options.input_gains_db,
                    "--input-gains-db")) {
                return false;
            }
        } else if (arg == "--carrier-gains-db") {
            if (!take_value(
                    i, argc, argv, options.carrier_gains_db,
                    "--carrier-gains-db")) {
                return false;
            }
        } else if (arg == "--output-layout") {
            if (!take_value(i, argc, argv, options.output_layout, "--output-layout"))
                return false;
        } else if (arg == "--host-block") {
            std::string value;
            if (!take_value(i, argc, argv, value, "--host-block"))
                return false;
            if (!parse_decimal_u32(
                    value, 1u, 0xFFFFFFFFu,
                    options.host_block)) {
                std::cerr << "--host-block must be a positive integer\n";
                return false;
            }
        } else if (arg == "--unit-block") {
            std::string value;
            if (!take_value(i, argc, argv, value, "--unit-block"))
                return false;
            if (!parse_decimal_u32(
                    value, 1u, 0xFFFFFFFFu,
                    options.unit_block)) {
                std::cerr << "--unit-block must be a positive integer\n";
                return false;
            }
        } else if (arg == "--profile") {
            std::string value;
            if (!take_value(i, argc, argv, value, "--profile"))
                return false;
            if (!parse_decimal_u32(
                    value, 1u, 5u,
                    options.profile)) {
                std::cerr << "--profile must be an integer in 1..5\n";
                return false;
            }
        } else if (arg == "--threads") {
            std::string value;
            if (!take_value(i, argc, argv, value, "--threads"))
                return false;
            if (!parse_decimal_u32(value, 1u, 256u, options.threads)) {
                std::cerr << "--threads must be an integer in 1..256\n";
                return false;
            }
            options.threads_present = true;
        } else if (arg == "--seed") {
            std::string value;
            if (!take_value(i, argc, argv, value, "--seed"))
                return false;
            if (!parse_decimal_u32(
                    value, 0u, 0xFFFFFFFFu,
                    options.seed)) {
                std::cerr << "--seed must be an integer in 0..4294967295\n";
                return false;
            }
            options.seed_present = true;
        } else if (arg == "--reserve-extra-bits") {
            std::string value;
            if (!take_value(
                    i, argc, argv, value,
                    "--reserve-extra-bits")) {
                return false;
            }
            if (!parse_decimal_u32(
                    value, 0u, 0x7FFFFFFFu,
                    options.reserve_extra_bits)) {
                std::cerr << "--reserve-extra-bits must be in 0..2147483647\n";
                return false;
            }
        } else if (arg == "--no-dither") {
            options.dither_enabled = false;
        } else if (arg == "--cts-dmx") {
            options.cts_dmx = true;
        } else if (arg == "--validate") {
            options.validate_only = true;
        } else if (arg == "-v" || arg == "--verbose") {
            options.verbose = true;
        } else if (arg == "--version") {
            options.version_only = true;
        } else if (arg == "--trace") {
            options.trace = true;
        } else {
            std::cerr << "unknown option: " << arg << '\n';
            return false;
        }
    }
    if (!options.version_only && options.input.empty()) {
        std::cerr << "--input is required\n";
        return false;
    }
    if (!options.validate_only && !supported_output_extension(options.output)) {
        if (options.output.empty())
            return true;
        std::cerr << "--output must end in .wav, .wave or .flac\n";
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    configure_console_encoding();
    if (argc <= 1) {
        print_banner();
        print_usage();
        return 0;
    }
    Options options;
    if (!parse_options(argc, argv, options))
        return 1;
    if (options.version_only) {
        print_app_version(
            std::cout,
            auro3d::console_style::color_enabled_for_stdout());
        std::cout << '\n';
        return 0;
    }

    const std::string source_input = options.input;
    TemporaryInputWav temporary_input;
    std::string processing_input = options.input;
    std::string error;
    if (is_flac_input(options.input)
        && !decode_flac_to_temporary_wav(options.input, temporary_input, error)) {
        std::cerr << "input error: " << error << '\n';
        return 2;
    }
    if (!temporary_input.path.empty())
        processing_input = temporary_input.path.u8string();
    auro3d::encode::WavPcm24Info input{};
    if (!auro3d::encode::probe_wav_pcm24(processing_input, input, error)) {
        std::cerr << "input error: " << error << '\n';
        return 2;
    }
    std::uint32_t input_mask = 0;
    std::uint32_t output_mask = 0;
    std::uint32_t carrier_mask = 0;
    std::vector<std::uint32_t> input_order;
    std::uint32_t derived_input_mask = 0u;
    const bool derived_from_wav_mask = options.input_channel_order.empty();
    if (derived_from_wav_mask
        ? !auro3d::encode::derive_layout_from_wav_channel_mask(
            input.channel_mask, derived_input_mask, input_order, error)
        : !auro3d::encode::derive_layout_from_channel_order(
            options.input_channel_order, derived_input_mask,
            input_order, error)) {
        std::cerr << "input channel order error: " << error << '\n';
        return 2;
    }
    input_mask = derived_input_mask;
    if (!options.input_layout.empty()) {
        std::uint32_t declared_input_mask = 0u;
        if (!auro3d::encode::parse_codec_v3_layout(
                options.input_layout, declared_input_mask, error)) {
            std::cerr << "layout error: " << error << '\n';
            return 2;
        }
        if (declared_input_mask != derived_input_mask) {
            std::cerr << "layout error: --input-layout does not match the complete "
                         "--input-channel-order scheme\n";
            return 2;
        }
    }
    // The native encoder accepts original-layout PCM and a separate carrier
    // descriptor. The requested output layout must be exactly the native
    // carrier mapping for the input layout; no arbitrary remapper is implied.
    if (!auro3d::encode::codec_v3_carrier_layout(input_mask, carrier_mask)) {
        std::cerr << "layout error: input layout is not accepted by native codec-v3 carrier mapping\n";
        return 2;
    }
    if (!auro3d::encode::codec_v3_output_layout(
            input_mask, carrier_mask, output_mask)) {
        std::cerr << "layout error: native output layout selection rejected "
                     "the original/carrier pair\n";
        return 2;
    }
    if (!options.validate_only && options.output.empty())
        options.output = default_output_path(options.input, input_mask);
    if (options.verbose) {
        std::cerr << "encode_input path=" << source_input
                  << " sample_rate=" << input.sample_rate
                  << " channels=" << input.channels
                  << " frames=" << input.frame_count()
                  << " host_block=" << options.host_block
                  << " unit_block=" << options.unit_block << '\n';
        std::cerr << "encode_layout input="
                  << auro3d::encode::layout_label(input_mask)
                  << " input_order="
                  << auro3d::encode::format_channel_order(input_order)
                  << " carrier=" << auro3d::encode::layout_label(carrier_mask)
                  << " output=" << auro3d::encode::layout_label(output_mask) << '\n';
    }
    if (!options.output_layout.empty()) {
        std::uint32_t declared_output_mask = 0u;
        if (!auro3d::encode::parse_codec_v3_layout(
                options.output_layout, declared_output_mask, error)) {
            std::cerr << "layout error: " << error << '\n';
            return 2;
        }
        if (declared_output_mask != output_mask) {
            std::cerr << "layout error: --output-layout does not match the native "
                         "physical output layout selected from the input channel scheme\n";
            return 2;
        }
    }
    if (!auro3d::encode::codec_v3_unit_block_size_supported(options.unit_block)) {
        std::cerr << "config error: --unit-block is not supported by native codec-v3\n";
        return 2;
    }
    auro3d::encode::EncoderConfig encoder_config{};
    if (input.channels != auro3d::encode::layout_channel_count(input_mask)) {
        std::cerr << "input error: WAV has " << input.channels
                  << " channels, but the declared channel scheme names "
                  << auro3d::encode::layout_channel_count(input_mask) << '\n';
        return 2;
    }
    if (!auro3d::encode::codec_v3_sample_rate_supported(input.sample_rate)) {
        std::cerr << "input error: sample rate is not supported by native codec-v3\n";
        return 2;
    }
    std::vector<std::uint32_t> unit_schedule;
    if (!auro3d::encode::plan_codec_v3_unit_blocks(
            input.frame_count(), options.unit_block, unit_schedule, error)) {
        std::cerr << "input error: " << error << '\n';
        return 2;
    }
    std::uint64_t scheduled_frames = 0u;
    for (const std::uint32_t block : unit_schedule)
        scheduled_frames += block;
    if (!auro3d::encode::encoder_config_prepare(
        encoder_config, input.sample_rate, input_mask, options.unit_block,
        options.profile, error)) {
        std::cerr << "config error: " << error << '\n';
        return 2;
    }
    encoder_config.dither_seed_present = options.seed_present;
    encoder_config.dither_seed = options.seed;
    if (options.threads_present) {
        encoder_config.thread_workers = {
            1u, options.threads - 1u};
    }
    encoder_config.field_44 = {
        1u, options.dither_enabled ? 1u : 0u};
    if (options.cts_dmx) {
        // Config+136 marker; value at +132 remains the downmix gate (=1).
        encoder_config.field_132.present = 1u;
    }
    encoder_config.reserve_extra_bits =
        options.reserve_extra_bits;
    if (!options.input_scaler_indices.empty()) {
        if (!parse_scaler_indices(
                options.input_scaler_indices,
                input_order,
                encoder_config.input_scaler_indices,
                error)) {
            std::cerr << "input scaler error: " << error << '\n';
            return 2;
        }
        encoder_config.input_scalers_present = true;
    }
    if (!options.input_gains_db.empty()) {
        if (!parse_input_gains_db(
                options.input_gains_db,
                input_order,
                encoder_config.dynamic_params,
                error)) {
            std::cerr << "input gains error: " << error << '\n';
            return 2;
        }
        encoder_config.dynamic_params_present = true;
    }
    std::vector<std::uint32_t> carrier_order_early;
    if (!auro3d::encode::codec_v3_layout_channel_order(
            carrier_mask, carrier_order_early)) {
        std::cerr << "layout error: carrier layout has no channel order\n";
        return 2;
    }
    std::vector<std::uint32_t> output_order;
    if (!auro3d::encode::codec_v3_layout_channel_order(
            output_mask, output_order)) {
        std::cerr << "layout error: output layout has no channel order\n";
        return 2;
    }
    if (!options.carrier_gains_db.empty()) {
        if (!parse_carrier_gains_db(
                options.carrier_gains_db,
                carrier_order_early,
                encoder_config.dynamic_params,
                error)) {
            std::cerr << "carrier gains error: " << error << '\n';
            return 2;
        }
        encoder_config.dynamic_params_present = true;
    }
    std::vector<auro3d::encode::EncodeGroupPlan> encode_groups;
    if (!auro3d::encode::build_encode_group_plan(input_mask, encode_groups, error)) {
        std::cerr << "group error: " << error << '\n';
        return 2;
    }
    std::uint32_t metadata_channel_id = 0u;
    if (!auro3d::encode::codec_v3_metadata_carrier_channel(
            carrier_mask, metadata_channel_id)) {
        std::cerr << "layout error: carrier layout has no native metadata channel\n";
        return 2;
    }
    auro3d::encode::Pcm24InputStream pcm;
    if (!pcm.open(processing_input, input, input_order, error)) {
        std::cerr << "input error: " << error << '\n';
        return 2;
    }
    if (!pcm.set_virtual_frame_count(scheduled_frames, error)) {
        std::cerr << "input error: " << error << '\n';
        return 2;
    }
    std::uint64_t streamed_frames = 0;
    std::uint64_t host_calls = 0;
    std::uint64_t complete_units = 0;
    std::size_t next_scheduled_unit = 0u;
    auro3d::encode::EncoderRuntimeState encoder_state{};
    std::size_t expensive_groups = 0u;
    for (const auro3d::encode::EncodeGroupPlan& group : encode_groups) {
        if (group.sources.arity > 1u)
            ++expensive_groups;
    }
    const std::size_t group_workers = expensive_groups > 1u
        ? std::min<std::size_t>(
            encoder_config.thread_workers.value,
            expensive_groups - 1u)
        : 0u;
    auro3d::encode::EncoderThreadPool thread_pool(group_workers);
    auro3d::encode::PcmUnitAccumulator units(input_order);
    const std::vector<std::uint32_t>& carrier_order = carrier_order_early;
    auro3d::encode::CarrierOutput output;
    if (!options.validate_only
        && !output.open(
            options.output, input.sample_rate, output_order, input.frame_count(),
            output_mask, error)) {
        std::cerr << "output error: " << error << '\n';
        return 2;
    }
    auro3d::ProgressReporter progress;
    progress.update("encode", 0);
    for (;;) {
        std::vector<std::vector<std::int32_t>> planes;
        const std::uint32_t count = pcm.read(options.host_block, planes, error);
        if (!error.empty()) {
            std::cerr << "input error: " << error << '\n';
            return 2;
        }
        if (count == 0u)
            break;
        if (!units.push(planes, count, error)) {
            std::cerr << "input error: " << error << '\n';
            return 2;
        }
        while (next_scheduled_unit < unit_schedule.size()
            && units.has_frames(unit_schedule[next_scheduled_unit])) {
            const std::uint32_t unit_frames =
                unit_schedule[next_scheduled_unit];
            std::vector<std::vector<std::int32_t>> unit_planes;
            if (!units.pop_frames(unit_frames, unit_planes, error)) {
                std::cerr << "codec-v3 accumulator error: " << error << '\n';
                return 2;
            }
            auro3d::encode::EncoderConfig unit_config{};
            if (!auro3d::encode::encoder_config_with_unit_block_size(
                    encoder_config, unit_frames, unit_config, error)) {
                std::cerr << "codec-v3 tail config error: " << error << '\n';
                return 2;
            }
            auro3d::encode::EncodedCarrierUnit encoded_unit{};
            if (!auro3d::encode::encode_v3_scheduled_unit(
                    unit_planes, input_mask, carrier_mask, input.sample_rate,
                    unit_config, encode_groups, metadata_channel_id, encoder_state,
                    encoded_unit, error, &thread_pool)) {
                std::cerr << "codec-v3 unit error: " << error << '\n';
                return 2;
            }
            if (options.trace) {
                std::uint32_t max_shift_attempts = 0u;
                std::uint32_t gvm_1d_groups = 0u;
                std::uint32_t gvm_2d_groups = 0u;
                std::uint64_t quantizer_budget_bits = 0u;
                std::uint64_t quantizer_fixed_bits = 0u;
                std::uint64_t quantizer_used_bits = 0u;
                std::uint64_t golomb_index_bits = 0u;
                std::uint64_t rescaler_bits = 0u;
                std::uint32_t max_gvm_initial_clusters = 0u;
                std::uint32_t gvm_fit_status1_groups = 0u;
                std::uint32_t gvm_fit_status2_groups = 0u;
                std::uint32_t gvm_fit_status3_groups = 0u;
                std::uint32_t gvm_backend_groups = 0u;
                std::uint32_t quantization_backend_groups = 0u;
                std::uint32_t old_fast_gvm_groups = 0u;
                std::uint32_t learned_gvm_groups = 0u;
                std::uint32_t max_gvm_selected_clusters = 0u;
                std::uint32_t max_gvm_learn_attempts = 0u;
                std::uint64_t gvm_learner_points = 0u;
                std::uint32_t deterministic_gvm_groups = 0u;
                std::uint32_t forced_zero_gvm_groups = 0u;
                std::uint32_t max_scaler_index = 0u;
                std::uint32_t max_scaler_attempts = 0u;
                double worst_quality_error_db = -3000.0;
                double worst_quality_peak_db = -144.0;
                for (const auto& group : encoded_unit.metadata_groups) {
                    if (group.gvm_input.dimensions == 1u)
                        ++gvm_1d_groups;
                    else if (group.gvm_input.dimensions == 2u)
                        ++gvm_2d_groups;
                    quantizer_budget_bits += group.quantizer_bit_budget;
                    quantizer_fixed_bits += group.quantizer_fixed_bit_cost;
                    quantizer_used_bits += group.quantizer_used_bits;
                    golomb_index_bits +=
                        group.golomb_index_bit_cost;
                    rescaler_bits += group.rescaler_bit_cost;
                    max_gvm_initial_clusters = std::max(
                        max_gvm_initial_clusters,
                        group.gvm_search.initial_clusters);
                    if (group.gvm_fit_status == 1u)
                        ++gvm_fit_status1_groups;
                    else if (group.gvm_fit_status == 2u)
                        ++gvm_fit_status2_groups;
                    else if (group.gvm_fit_status == 3u)
                        ++gvm_fit_status3_groups;
                    if (group.cluster_backend
                        == auro3d::encode::NativeClusterDeltasBackend::gvm) {
                        ++gvm_backend_groups;
                    } else {
                        ++quantization_backend_groups;
                    }
                    if (group.gvm_learner
                        == auro3d::encode::NativeGvmLearnerImplementation::old_fast) {
                        ++old_fast_gvm_groups;
                    }
                    if (group.gvm_learned)
                        ++learned_gvm_groups;
                    max_gvm_selected_clusters = std::max(
                        max_gvm_selected_clusters,
                        group.gvm_selected_clusters);
                    max_gvm_learn_attempts = std::max(
                        max_gvm_learn_attempts,
                        group.gvm_learn_attempts);
                    gvm_learner_points += group.gvm_learner_points;
                    if (group.gvm_deterministic_seed)
                        ++deterministic_gvm_groups;
                    if (group.gvm_forced_zero_center)
                        ++forced_zero_gvm_groups;
                    max_scaler_index = std::max(
                        max_scaler_index,
                        static_cast<std::uint32_t>(
                            group.scaler_ix));
                    max_scaler_attempts = std::max(
                        max_scaler_attempts,
                        group.scaler_attempts);
                    worst_quality_error_db = std::max(
                        worst_quality_error_db,
                        group.quality_error_db);
                    for (const auto& quality : group.frame_quality) {
                        worst_quality_peak_db = std::max(
                            worst_quality_peak_db,
                            quality.peak_db);
                    }
                    if (group.analysis_arity == 2u) {
                        max_shift_attempts = std::max(
                            max_shift_attempts, group.mix2_shift_attempts);
                    }
                }
                std::int32_t minimum = 0x7FFFFF;
                std::int32_t maximum = -0x800000;
                std::uint64_t serialized_words = 0u;
                std::uint64_t payload_bits = 0u;
                std::uint16_t crc_xor = 0u;
                for (const std::uint32_t channel : carrier_order) {
                    for (const std::int32_t sample : encoded_unit.carrier.planes[channel]) {
                        minimum = std::min(minimum, sample);
                        maximum = std::max(maximum, sample);
                    }
                }
                for (const auto& channel : encoded_unit.channels) {
                    serialized_words += channel.words.size();
                    payload_bits += channel.serialized_bits;
                    crc_xor = static_cast<std::uint16_t>(crc_xor ^ channel.crc_word);
                }
                std::cout << "trace unit=" << complete_units
                          << " frames=" << encoded_unit.carrier.descriptor.frame_count
                          << " metadata_channel=" << encoded_unit.metadata_channel_id
                          << " max_shift_attempts=" << max_shift_attempts
                          << " gvm_1d_groups=" << gvm_1d_groups
                          << " gvm_2d_groups=" << gvm_2d_groups
                          << " quantizer_budget_bits=" << quantizer_budget_bits
                          << " quantizer_fixed_bits=" << quantizer_fixed_bits
                          << " quantizer_used_bits=" << quantizer_used_bits
                          << " golomb_index_bits=" << golomb_index_bits
                          << " rescaler_bits=" << rescaler_bits
                          << " reserve_extra_bits="
                          << unit_config.reserve_extra_bits
                          << " dither="
                          << (unit_config.field_44.value != 0u ? 1u : 0u)
                          << " max_gvm_initial_clusters=" << max_gvm_initial_clusters
                          << " gvm_fit_status1_groups=" << gvm_fit_status1_groups
                          << " gvm_fit_status2_groups=" << gvm_fit_status2_groups
                          << " gvm_fit_status3_groups=" << gvm_fit_status3_groups
                          << " gvm_backend_groups=" << gvm_backend_groups
                          << " quantization_backend_groups="
                          << quantization_backend_groups
                          << " old_fast_gvm_groups=" << old_fast_gvm_groups
                          << " learned_gvm_groups=" << learned_gvm_groups
                          << " max_gvm_selected_clusters="
                          << max_gvm_selected_clusters
                          << " max_gvm_learn_attempts="
                          << max_gvm_learn_attempts
                          << " gvm_learner_points=" << gvm_learner_points
                          << " deterministic_gvm_groups="
                          << deterministic_gvm_groups
                          << " forced_zero_gvm_groups="
                          << forced_zero_gvm_groups
                          << " max_scaler_index="
                          << max_scaler_index
                          << " max_scaler_attempts="
                          << max_scaler_attempts
                          << " worst_quality_error_db="
                          << worst_quality_error_db
                          << " worst_quality_peak_db="
                          << worst_quality_peak_db
                          << " serialized_words=" << serialized_words
                          << " payload_bits=" << payload_bits
                          << " crc_xor=0x" << std::hex << crc_xor << std::dec
                          << " metadata_blocks=" << encoded_unit.metadata.adol_blocks.size()
                          << " carrier_digest=0x" << std::hex
                          << carrier_digest(encoded_unit, carrier_order) << std::dec
                          << " carrier_min=" << minimum
                          << " carrier_max=" << maximum << '\n';
                for (const auto& group : encoded_unit.metadata_groups) {
                    std::cout << "trace_group carrier_ch="
                              << group.carrier_channel_id
                              << " arity=" << group.analysis_arity
                              << " headroom="
                              << static_cast<std::uint32_t>(
                                     group.headroom_bits)
                              << " scaler="
                              << static_cast<std::uint32_t>(group.scaler_ix)
                              << " residual_width="
                              << static_cast<std::uint32_t>(
                                     group.residual_bit_width)
                              << " codebook="
                              << (group.analysis_arity == 3u
                                      ? group.mix3_residuals.size()
                                      : group.residuals.size())
                              << " rice="
                              << (group.analysis_arity == 3u
                                      ? group.mix3_level_pack_mode
                                      : group.level_pack_mode)
                              << " quality_db=" << group.quality_error_db;
                    if (group.analysis_arity == 3u) {
                        std::cout << " seeds="
                                  << group.seed0 << ','
                                  << group.seed1 << ','
                                  << group.seed2 << ','
                                  << group.seed3 << ','
                                  << group.seed4;
                    } else if (group.analysis_arity == 2u) {
                        std::cout << " seeds="
                                  << group.seed0 << ','
                                  << group.seed1;
                    }
                    std::cout << '\n';
                }
            }
            if (options.validate_only
                && !output.validate_unit(encoded_unit, error)) {
                std::cerr << "validation error: " << error << '\n';
                return 2;
            }
            if (!options.validate_only
                && !output.write_unit(encoded_unit, error)) {
                std::cerr << "output error: " << error << '\n';
                return 2;
            }
            ++complete_units;
            ++next_scheduled_unit;
            if (encoder_state.processed_frames < scheduled_frames) {
                progress.update(
                    "encode",
                    auro3d::progress_percent(
                        encoder_state.processed_frames,
                        scheduled_frames));
            }
        }
        streamed_frames += count;
        ++host_calls;
    }
    if (streamed_frames != scheduled_frames) {
        std::cerr << "input error: stream frame count mismatch\n";
        return 2;
    }
    if (next_scheduled_unit != unit_schedule.size()
        || units.pending_frames() != 0u
        || encoder_state.processed_frames != scheduled_frames) {
        std::cerr << "codec-v3 input error: UnitBlock schedule consumed "
                  << next_scheduled_unit << " of " << unit_schedule.size()
                  << " blocks and retained " << units.pending_frames()
                  << " frames at runtime position "
                  << encoder_state.processed_frames << '\n';
        return 2;
    }
    progress.done("encode");

    std::cout << "input=" << source_input << '\n'
              << "sample_rate=" << input.sample_rate << '\n'
              << "frames=" << input.frame_count() << '\n'
              << "host_block=" << options.host_block << " host_calls=" << host_calls << '\n'
              << "unit_block=" << options.unit_block
              << " scheduled_units=" << unit_schedule.size()
              << " complete_units=" << complete_units
              << " pending_frames=" << units.pending_frames() << '\n'
              << "profile=" << options.profile
              << " cluster_backend="
              << (encoder_config.cluster_backend
                      == auro3d::encode::NativeClusterDeltasBackend::gvm
                  ? "gvm"
                  : "quantization")
              << " gvm_mode=" << encoder_config.gvm.mode
              << " gvm_qword=" << (encoder_config.gvm.has_qword_48 ? 1 : 0) << '\n'
              << "input_layout=" << auro3d::encode::layout_label(input_mask) << " (0x" << std::hex << input_mask << std::dec << ")\n"
              << "input_channel_order=" << auro3d::encode::format_channel_order(input_order) << '\n'
              << "input_scalers="
              << (encoder_config.dynamic_params_present
                      ? "dynamic"
                      : (encoder_config.input_scalers_present
                              ? "explicit"
                              : "identity"))
              << '\n'
              << "output_layout=" << auro3d::encode::layout_label(output_mask) << " (0x" << std::hex << output_mask << std::dec << ")\n"
              << "carrier_layout=" << auro3d::encode::layout_label(carrier_mask) << " (0x" << std::hex << carrier_mask << std::dec << ")\n"
              << "auro_layout=" << auro3d::encode::layout_label(input_mask)
              << " carrier=" << auro3d::encode::layout_label(carrier_mask)
              << " container=" << auro3d::encode::layout_label(output_mask)
              << '\n'
              << "encoder_profile=" << encoder_config.profile
              << " bit_line=" << encoder_config.bit_line.low << "/" << encoder_config.bit_line.high
              << " threads=" << (encoder_config.thread_workers.value + 1u)
              << " encode_groups=" << encode_groups.size() << '\n';
    for (const auro3d::encode::EncodeGroupPlan& group : encode_groups) {
        std::cout << "  group carrier_ch=" << group.carrier_channel
                  << " arity=" << group.sources.arity << " sources=";
        for (std::uint32_t i = 0; i < group.sources.arity; ++i) {
            if (i != 0u)
                std::cout << ',';
            std::cout << group.sources.channels[i];
        }
        std::cout << '\n';
    }
    if (options.validate_only) {
        if (options.verbose)
            std::cerr << "Done: validation\n";
        return 0;
    }
    if (!output.close(error)) {
        std::cerr << "output error: " << error << '\n';
        return 2;
    }
    std::cout << "output=" << options.output << '\n';
    if (options.verbose)
        std::cerr << "Done: " << options.output << '\n';
    return 0;
}
