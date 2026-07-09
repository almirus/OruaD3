#include "app_version.hpp"
#include "auro3d_decoder.hpp"
#include "auro3deng_strength.hpp"
#include "wav_writer.hpp"

#include <exception>
#include <filesystem>
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
    bool help_only = false;
    bool version_only = false;
    unsigned sample_rate = 0;
    unsigned channels = 0;
    unsigned block_size = 0;
    unsigned dsp_strength = 12;
    unsigned dsp_output_channels = 0;
    unsigned output_bits = 24;
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
    const std::string& channel_name) {
    std::filesystem::path out_path(output);
    const std::filesystem::path dir = out_path.parent_path();
    const std::string stem = out_path.stem().string();
    std::filesystem::path mono_name = stem + " (" + channel_name + ").wav";
    return dir.empty() ? mono_name : (dir / mono_name);
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
        << "  " << auro3d_decode::kName << " -i <input.wav|input.flac|input.s24le> -o <output.wav> [опции]\n\n"
        << "Опции:\n"
        << "  -i, --input FILE\n"
        << "  -o, --output FILE\n"
        << "  --raw                вход — сырой interleaved s24le (требуются --rate --channels)\n"
        << "  --rate HZ            sample rate для --raw\n"
        << "  --channels N         число каналов для --raw\n"
        << "  --block N            block size в сэмплах; по умолчанию " << auro3d::kDefaultJniBlockSize << "\n"
        << "  --dsp-strength N     сила декодера/рендера (0..15; по умолчанию 12)\n"
        << "  --dsp-output-channels N  output channels; 0/omitted = auto from Auro metadata; no Auro-Matic/XinN fallback, current native path up to "
        << auro3d::kCurrentNativeExportChannelLimit << "\n"
        << "  --output-bits N      WAV PCM depth: 16 or 24; по умолчанию 24\n"
        << "  --mono-tracks        additionally write mono WAV files named <output stem> (FL).wav, etc.\n"
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
        if (a == "-v" || a == "--verbose") {
            opt.verbose = true;
            continue;
        }
        if (a == "--mono-tracks") {
            opt.mono_tracks = true;
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

    if (opt.input.empty() || opt.output.empty()) {
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
    const std::vector<std::uint32_t> output_slots = dec.output_channel_slot_map();
    const bool auromatic_layout =
        auro_meta.found
        && (auro_meta.layout_id & 0x3FE00u) == 0u
        && auro_meta.carrier_layout_id != 0u
        && auro_meta.carrier_layout_id != auro_meta.layout_id
        && auro_meta.output_channels > auro_meta.carrier_channels;
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
    print_decode_mode_channel_list(auromatic_layout ? "auromatic" : "native", output_slots, native_cfg.input_mask, false);
    if (!auromatic_layout)
        std::cerr << " auromatic=none";
    if (auro_meta.found && auro_meta.has_closest_layout_without_mix3) {
        std::cerr << " auromatic_candidate_layout=0x" << std::hex
                  << auro_meta.closest_layout_without_mix3 << std::dec;
    }
    std::cerr << "\n";
    if (opt.verbose) {
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

    const auro3d::DecoderConfig cfg = dec.config();
    const std::uint64_t dsp_clipped = dec.dsp_clipped_samples();
    dec.close();

    if (pcm_all.empty()) {
        std::cerr << "Нет PCM данных.\n";
        return 3;
    }

    std::string err;
    const bool ok = (cfg.bits_per_sample == 24)
        ? wav::write_pcm24_le(opt.output, cfg.sample_rate, cfg.channels, pcm_all, err)
        : wav::write_pcm16_le(opt.output, cfg.sample_rate, cfg.channels, pcm_all, err);
    if (!ok) {
        std::cerr << "WAV: " << err << "\n";
        return 4;
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
                mono_channel_output_path(opt.output, channel_name);
            const std::vector<std::uint8_t> mono_pcm =
                extract_mono_channel_pcm(pcm_all, cfg.channels, ch, bytes_per_sample);
            const bool mono_ok = (cfg.bits_per_sample == 24)
                ? wav::write_pcm24_le(mono_path.string(), cfg.sample_rate, 1, mono_pcm, err)
                : wav::write_pcm16_le(mono_path.string(), cfg.sample_rate, 1, mono_pcm, err);
            if (!mono_ok) {
                std::cerr << "WAV mono " << mono_path.string() << ": " << err << "\n";
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
