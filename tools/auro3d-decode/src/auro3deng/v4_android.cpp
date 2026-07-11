#include "auro3deng_internal_preamble.hpp"

namespace auro3deng {

namespace {

struct A3DENGPartialQueue {
    std::vector<std::uint8_t> input;
    std::vector<std::uint8_t> native_heap;
    std::uint32_t pushed_bytes = 0;
    std::uint32_t rendered_blocks = 0;
    std::uint32_t target_device = 0;
    std::uint32_t output_sample_rate = 0;
    std::uint32_t effective_virtualization_mode = 1;
    std::uint32_t effective_listening_mode = 3;
    std::uint64_t debug_push_count = 0;
    std::uint64_t debug_pop_count = 0;
    bool is_abr = false;
    A3dengCodecV3PopRenderFn codec_v3_pop_render = nullptr;
    void* codec_v3_pop_render_user = nullptr;
};

constexpr std::uintptr_t kA3DENG_cfg_off_decoder_mode = 0x000u;
constexpr std::uintptr_t kA3DENG_cfg_off_input_sample_rate = 0x008u;
constexpr std::uintptr_t kA3DENG_cfg_off_input_channels = 0x010u;
constexpr std::uintptr_t kA3DENG_cfg_off_output_sample_rate = 0x198u;
constexpr std::uintptr_t kA3DENG_cfg_off_target_device = 0x1A0u;
constexpr std::uintptr_t kA3DENG_cfg_off_output_channels = 0x1A8u;
constexpr std::uintptr_t kA3DENG_cfg_off_output_audio_configuration = 0x330u;
constexpr std::uintptr_t kA3DENG_cfg_off_is_abr = 0x334u;
constexpr std::size_t kA3DENG_cfg_size = 0x340u;

constexpr std::uintptr_t kA3DENG_dyn_off_actual_virtualization = 0x000u;
constexpr std::uintptr_t kA3DENG_dyn_off_preset = 0x004u;
constexpr std::uintptr_t kA3DENG_dyn_off_listening_mode = 0x008u;
constexpr std::uintptr_t kA3DENG_dyn_off_strength = 0x00Cu;
constexpr std::uintptr_t kA3DENG_dyn_off_direct = 0x010u;
constexpr std::uintptr_t kA3DENG_dyn_off_alt_3d = 0x014u;
constexpr std::uintptr_t kA3DENG_dyn_off_hp_room = 0x01Cu;
constexpr std::uintptr_t kA3DENG_dyn_off_hp_head_size = 0x020u;
constexpr std::uintptr_t kA3DENG_dyn_off_hp_hrtf_preset = 0x024u;
constexpr std::size_t kA3DENG_dyn_size = 0x70u;

struct A3DENGSyntheticApi31bbe0 {
    std::uint32_t version_major = 4u;
    std::uint32_t version_minor = 0u;
    std::uint32_t version_patch = 14u;
    std::uint32_t version_beta = 0x7FFFFFFFu;
    const char* version_tag = "9d106532";
};

std::unordered_map<std::uint8_t*, A3DENGPartialQueue>& a3deng_partial_queues_319ae0() {
    static std::unordered_map<std::uint8_t*, A3DENGPartialQueue> queues;
    return queues;
}

std::unordered_map<std::uint8_t*, std::string>& a3deng_debug_dirs_31bde0() {
    static std::unordered_map<std::uint8_t*, std::string> paths;
    return paths;
}

bool a3deng_path_is_directory(const char* path) {
    if (!path || !path[0])
        return false;
#ifdef _WIN32
    struct _stat64 st{};
    if (_stat64(path, &st) != 0)
        return false;
    return (st.st_mode & _S_IFDIR) != 0;
#else
    struct stat st{};
    if (stat(path, &st) != 0)
        return false;
    return S_ISDIR(st.st_mode);
#endif
}

bool a3deng_mkdir_one(const std::string& path) {
    if (path.empty())
        return false;
    if (a3deng_path_is_directory(path.c_str()))
        return true;
#ifdef _WIN32
    if (_mkdir(path.c_str()) == 0)
        return true;
#else
    if (mkdir(path.c_str(), 0755) == 0)
        return true;
#endif
    return a3deng_path_is_directory(path.c_str());
}

bool a3deng_mkdir_p(std::string path) {
    for (char& c : path) {
        if (c == '\\')
            c = '/';
    }
    while (!path.empty() && path.back() == '/')
        path.pop_back();
    if (path.empty())
        return false;
    std::size_t pos = 0;
    if (path.size() >= 2u && path[1] == ':') {
        const std::size_t slash = path.find('/', 2u);
        pos = slash == std::string::npos ? path.size() : slash + 1u;
    } else if (path[0] == '/') {
        pos = 1u;
    }
    for (;;) {
        const std::size_t slash = path.find('/', pos);
        const std::string part = slash == std::string::npos ? path : path.substr(0, slash);
        if (!part.empty() && part != "." && !a3deng_path_is_directory(part.c_str())) {
            if (!a3deng_mkdir_one(part))
                return false;
        }
        if (slash == std::string::npos)
            break;
        pos = slash + 1u;
    }
    return a3deng_path_is_directory(path.c_str());
}

std::string a3deng_join_path(const std::string& dir, const char* filename) {
    if (!filename)
        return dir;
    std::string out = dir;
    if (!out.empty() && out.back() != '/' && out.back() != '\\')
        out.push_back('/');
    out += filename;
    return out;
}

void a3deng_debug_log_buffer_31bf70(
    std::uint8_t* a3deng_base,
    const char* prefix,
    const std::uint8_t* data,
    std::uint32_t byte_count) {
    if (!a3deng_base || !prefix || !data || byte_count == 0u)
        return;
    const auto dir_it = a3deng_debug_dirs_31bde0().find(a3deng_base);
    if (dir_it == a3deng_debug_dirs_31bde0().end())
        return;
    auto& q = a3deng_partial_queues_319ae0()[a3deng_base];
    std::uint64_t& counter =
        std::strcmp(prefix, "pop") == 0 ? q.debug_pop_count : q.debug_push_count;
    ++counter;
    char filename[32];
    std::snprintf(filename, sizeof(filename), "%s_%04llu.bin", prefix,
        static_cast<unsigned long long>(counter));
    const std::string out_path = a3deng_join_path(dir_it->second, filename);
    FILE* f = std::fopen(out_path.c_str(), "wb");
    if (!f)
        return;
    (void)std::fwrite(data, 1u, byte_count, f);
    std::fclose(f);
}

std::uint64_t a3deng_global_api_319ae0() {
    static A3DENGSyntheticApi31bbe0 api;
    return reinterpret_cast<std::uint64_t>(&api);
}

std::uint32_t a3deng_read_u32_319e60(const std::uint8_t* p, std::uintptr_t off) {
    return *reinterpret_cast<const std::uint32_t*>(p + off);
}

void a3deng_write_u32_319e60(std::uint8_t* p, std::uintptr_t off, std::uint32_t v) {
    *reinterpret_cast<std::uint32_t*>(p + off) = v;
}

std::uint64_t a3deng_native_api_fn_319e60(
    std::uint64_t api,
    std::uint32_t vtable_offset) {
    if (api == 0u || api == a3deng_global_api_319ae0())
        return 0u;
    return *reinterpret_cast<const std::uint64_t*>(
        static_cast<std::uintptr_t>(api + vtable_offset));
}

struct A3DENGRateBlock_31af50 {
    std::uint32_t sample_rate = 0u;
    std::uint32_t frames = 0u;
};

std::uint32_t a3deng_target_rate_31af50(
    std::uint32_t input_sample_rate,
    std::uint32_t output_mode,
    std::uint32_t decoder_mode) {
    if (decoder_mode == 2u)
        output_mode = 0u;
    return auro_a3deng_v4_android_A3DENG_output_sample_rate_3198d0_partial(input_sample_rate, output_mode);
}

A3DENGRateBlock_31af50 a3deng_rate_block_31af50(
    std::uint32_t input_sample_rate,
    std::uint32_t output_mode,
    std::uint32_t decoder_mode,
    std::uint32_t block_frames) {
    if (input_sample_rate == 0u)
        return {0u, 0u};

    const std::uint32_t target_rate =
        a3deng_target_rate_31af50(input_sample_rate, output_mode, decoder_mode);
    std::uint32_t frames = block_frames;
    if (target_rate != input_sample_rate) {
        std::uint32_t rate = target_rate;
        while (rate != 0u) {
            std::uint32_t next_rate = rate * 2u;
            const std::uint32_t half_frames = frames >> 1u;
            frames *= 2u;
            if (rate > input_sample_rate) {
                next_rate = rate >> 1u;
                frames = half_frames;
            }
            rate = next_rate;
            if (rate == input_sample_rate)
                return {target_rate, frames};
        }
        frames = 0u;
    }
    return {target_rate, frames};
}

std::uint32_t a3deng_input_block_size_31b280(
    const std::uint8_t* a3deng_base) {
    if (!a3deng_base)
        return 0u;
    const std::uint32_t input_sample_rate =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_input_sample_rate_runtime);
    if (input_sample_rate == 0u)
        return 0u;
    const std::uint32_t block_size =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_pipeline_audio_block_size);
    const std::uint32_t decoder_mode =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_decoder_mode);
    std::uint32_t output_mode = 0u;
    if (decoder_mode != 2u)
        output_mode = a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_output_mode);
    if (output_mode == 0u)
        return block_size;

    std::uint32_t adjusted_rate = input_sample_rate;
    if (output_mode == 1u) {
        std::uint32_t next = input_sample_rate;
        do {
            adjusted_rate = next;
            next >>= 1u;
        } while (adjusted_rate > 48000u);
    } else if (output_mode == 2u) {
        std::uint32_t next = input_sample_rate;
        do {
            adjusted_rate = next;
            next >>= 1u;
        } while (adjusted_rate > 96000u);
        do {
            next = adjusted_rate;
            adjusted_rate *= 2u;
        } while (next < 48001u);
        adjusted_rate = next;
    } else {
        return block_size;
    }

    std::uint32_t frames = block_size;
    if (adjusted_rate == input_sample_rate)
        return frames;
    while (adjusted_rate != 0u) {
        const std::uint32_t half_frames = frames >> 1u;
        frames *= 2u;
        std::uint32_t next_rate = 2u * adjusted_rate;
        if (adjusted_rate > input_sample_rate) {
            next_rate = adjusted_rate >> 1u;
            frames = half_frames;
        }
        adjusted_rate = next_rate;
        if (next_rate == input_sample_rate)
            return frames;
    }
    return 0u;
}

std::uint32_t a3deng_output_block_count_31b4e0(std::uint8_t* a3deng_base) {
    const std::uint64_t api = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_api);
    const std::uint64_t instance = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_instance);
    if (api != 0u && instance != 0u && api != a3deng_global_api_319ae0()) {
        using OutputBlockCountFn = std::uint32_t (*)(std::uint64_t);
        const auto fn = reinterpret_cast<OutputBlockCountFn>(
            *reinterpret_cast<const std::uint64_t*>(
                static_cast<std::uintptr_t>(api + auro_engine_v4_ida::kA3DENG_api_vtable_off_output_block_count)));
        if (fn)
            return fn(instance);
    }
    const auto it = a3deng_partial_queues_319ae0().find(a3deng_base);
    return it == a3deng_partial_queues_319ae0().end() ? 0u : it->second.rendered_blocks;
}

std::uint32_t a3deng_current_output_sample_rate_31b700(std::uint8_t* a3deng_base) {
    const auto it = a3deng_partial_queues_319ae0().find(a3deng_base);
    if (it != a3deng_partial_queues_319ae0().end() && it->second.output_sample_rate != 0u)
        return it->second.output_sample_rate;
    return auro_a3deng_v4_android_A3DENG_output_sample_rate_319920_partial(a3deng_base);
}

std::uint32_t a3deng_output_sample_bytes_31b7f0(std::uint32_t output_sample_type) {
    if (output_sample_type == 0u)
        return 4u;
    if (output_sample_type == 1u)
        return 3u;
    return 0u;
}

std::uint32_t a3deng_output_sample_bits_31b7f0(std::uint32_t output_sample_type) {
    if (output_sample_type == 0u)
        return 32u;
    if (output_sample_type == 1u)
        return 24u;
    return 0u;
}

std::uint32_t a3deng_input_sample_bytes_31af50(std::uint32_t input_sample_type) {
    if (input_sample_type == 0u)
        return 4u;
    if (input_sample_type == 1u)
        return 3u;
    if (input_sample_type == 2u)
        return 2u;
    return 0u;
}

std::uint32_t a3deng_input_sample_bits_31af50(std::uint32_t decoder_mode, std::uint32_t input_sample_type) {
    if (decoder_mode == 4u || input_sample_type == 0u)
        return 32u;
    if (input_sample_type == 1u)
        return 24u;
    if (input_sample_type == 2u)
        return 16u;
    return 0u;
}

std::vector<std::uint32_t> a3deng_mask_channel_order_31ace0(std::uint32_t channel_mask, std::uint32_t hdmi_mapping) {
    alignas(16) std::uint8_t layout[0x188]{};
    auro_a3deng_v4_android_channel_layout_31ace0_partial(layout, channel_mask, hdmi_mapping);
    const std::uint64_t count64 = *reinterpret_cast<const std::uint64_t*>(layout);
    const std::uint32_t count = static_cast<std::uint32_t>(std::min<std::uint64_t>(count64, 24u));
    std::vector<std::uint32_t> channels;
    channels.reserve(count);
    for (std::uint32_t i = 0u; i != count; ++i) {
        const std::uint32_t channel =
            *reinterpret_cast<const std::uint32_t*>(layout + 16u + static_cast<std::uintptr_t>(i) * 16u);
        if (channel < 31u)
            channels.push_back(channel);
    }
    return channels;
}

std::int32_t a3deng_find_channel_index_31ace0(
    const std::vector<std::uint32_t>& channels,
    std::uint32_t channel) {
    for (std::uint32_t i = 0u; i != channels.size(); ++i) {
        if (channels[i] == channel)
            return static_cast<std::int32_t>(i);
    }
    return -1;
}

constexpr std::size_t kA3DENGAudioBlockWordsBytes_31af50 = 0xF0u;
constexpr std::size_t kA3DENGAudioBlockOutputBytes_31b7f0 = 0x100u;
constexpr std::uintptr_t kA3DENGAudioBlockOffSampleBits_31af50 = 0u;
constexpr std::uintptr_t kA3DENGAudioBlockOffSampleRate_31af50 = 32u;
constexpr std::uintptr_t kA3DENGAudioBlockOffBlockFrames_31af50 = 36u;
constexpr std::uintptr_t kA3DENGAudioBlockOffChannelCount_31af50 = 40u;
constexpr std::uintptr_t kA3DENGAudioBlockOffReserved_31af50 = 44u;
constexpr std::uintptr_t kA3DENGAudioBlockOffData_31af50 = 48u;
constexpr std::uintptr_t kA3DENGAudioBlockOffValid_31b7f0 = 0xF0u;
constexpr std::size_t kA3DENGStaticParamsBytes_31a790 = 0x68u;
constexpr std::uintptr_t kA3DENGStaticOffRendererCount_31a790 = 0u;
constexpr std::uintptr_t kA3DENGStaticOffPipelineBlockSize_31a790 = 4u;
constexpr std::uintptr_t kA3DENGStaticOffDisableLimiter_31a790 = 8u;
constexpr std::uintptr_t kA3DENGStaticOffSmoothing_31a790 = 16u;
constexpr std::uintptr_t kA3DENGStaticOffHdmiCarrierValid_31a790 = 20u;
constexpr std::uintptr_t kA3DENGStaticOffHdmiAllow20_31a790 = 24u;
constexpr std::uintptr_t kA3DENGStaticOffHdmiAllow21_31a790 = 28u;
constexpr std::uintptr_t kA3DENGStaticOffHdmiAllow40_31a790 = 32u;
constexpr std::uintptr_t kA3DENGStaticOffHdmiAllow50_31a790 = 36u;
constexpr std::uintptr_t kA3DENGStaticOffHdmiAllow51_31a790 = 40u;
constexpr std::uintptr_t kA3DENGStaticOffHdmiAllow70_31a790 = 44u;
constexpr std::uintptr_t kA3DENGStaticOffHdmiAllow71_31a790 = 48u;
constexpr std::uintptr_t kA3DENGStaticOffHdmiDisableDownmixLimiter_31a790 = 52u;
constexpr std::uintptr_t kA3DENGStaticOffHdmiDisableMix3_31a790 = 56u;
constexpr std::uintptr_t kA3DENGStaticOffHdmiQuality_31a790 = 60u;
constexpr std::uintptr_t kA3DENGStaticOffDiagnosticsMode_31a790 = 64u;

void a3deng_static_params_apply_android_defaults_31a790(
    std::uint8_t* static_params,
    std::uint32_t pipeline_audio_block_size) {
    *reinterpret_cast<std::uint32_t*>(static_params + kA3DENGStaticOffRendererCount_31a790) = 0u;
    *reinterpret_cast<std::uint64_t*>(static_params + kA3DENGStaticOffPipelineBlockSize_31a790) =
        pipeline_audio_block_size;
    *reinterpret_cast<std::uint32_t*>(static_params + kA3DENGStaticOffSmoothing_31a790) = 0u;
    *reinterpret_cast<std::uint64_t*>(static_params + kA3DENGStaticOffHdmiCarrierValid_31a790) = 0x100000001ull;
    *reinterpret_cast<std::uint32_t*>(static_params + kA3DENGStaticOffHdmiAllow21_31a790) = 1u;
    *reinterpret_cast<std::uint32_t*>(static_params + kA3DENGStaticOffHdmiAllow51_31a790) = 1u;
    *reinterpret_cast<std::uint32_t*>(static_params + kA3DENGStaticOffHdmiAllow71_31a790) = 1u;
    *reinterpret_cast<std::uint32_t*>(static_params + kA3DENGStaticOffHdmiQuality_31a790) = 3u;
}

bool a3deng_make_push_audio_block_31af50(
    std::uint8_t* audio_block,
    std::size_t audio_block_bytes,
    std::uint32_t decoder_mode,
    std::uint32_t input_sample_type,
    std::uint32_t input_sample_rate,
    std::uint32_t input_block_frames_or_bytes,
    std::uint32_t input_channel_count,
    const std::uint8_t* input_bytes) {
    if (!audio_block || audio_block_bytes < kA3DENGAudioBlockWordsBytes_31af50)
        return false;
    std::memset(audio_block, 0, audio_block_bytes);
    const std::uint32_t sample_bits = a3deng_input_sample_bits_31af50(decoder_mode, input_sample_type);
    if (sample_bits == 0u)
        return false;
    *reinterpret_cast<std::uint64_t*>(audio_block + kA3DENGAudioBlockOffSampleBits_31af50) =
        static_cast<std::uint64_t>(sample_bits);
    if (decoder_mode != 4u)
        *reinterpret_cast<std::uint32_t*>(audio_block + kA3DENGAudioBlockOffSampleRate_31af50) = input_sample_rate;
    *reinterpret_cast<std::uint32_t*>(audio_block + kA3DENGAudioBlockOffBlockFrames_31af50) =
        input_block_frames_or_bytes;
    *reinterpret_cast<std::uint32_t*>(audio_block + kA3DENGAudioBlockOffChannelCount_31af50) =
        decoder_mode == 4u ? 1u : input_channel_count;
    *reinterpret_cast<std::uint32_t*>(audio_block + kA3DENGAudioBlockOffReserved_31af50) = 0u;
    *reinterpret_cast<std::uint64_t*>(audio_block + kA3DENGAudioBlockOffData_31af50) =
        reinterpret_cast<std::uint64_t>(input_bytes);
    return true;
}

bool a3deng_make_pop_audio_block_31b7f0(
    std::uint8_t* audio_block,
    std::size_t audio_block_bytes,
    std::uint32_t output_sample_type,
    std::uint32_t output_sample_rate,
    std::uint32_t output_block_frames,
    std::uint32_t output_channel_count,
    std::uint8_t* output_bytes) {
    if (!audio_block || audio_block_bytes < kA3DENGAudioBlockOutputBytes_31b7f0)
        return false;
    std::memset(audio_block, 0, audio_block_bytes);
    const std::uint32_t sample_bits = a3deng_output_sample_bits_31b7f0(output_sample_type);
    if (sample_bits == 0u)
        return false;
    *reinterpret_cast<std::uint64_t*>(audio_block + kA3DENGAudioBlockOffSampleBits_31af50) =
        static_cast<std::uint64_t>(sample_bits);
    *reinterpret_cast<std::uint32_t*>(audio_block + kA3DENGAudioBlockOffSampleRate_31af50) = output_sample_rate;
    *reinterpret_cast<std::uint32_t*>(audio_block + kA3DENGAudioBlockOffBlockFrames_31af50) = output_block_frames;
    *reinterpret_cast<std::uint32_t*>(audio_block + kA3DENGAudioBlockOffChannelCount_31af50) = output_channel_count;
    *reinterpret_cast<std::uint32_t*>(audio_block + kA3DENGAudioBlockOffReserved_31af50) = 0u;
    *reinterpret_cast<std::uint64_t*>(audio_block + kA3DENGAudioBlockOffData_31af50) =
        reinterpret_cast<std::uint64_t>(output_bytes);
    audio_block[kA3DENGAudioBlockOffValid_31b7f0] = 1u;
    return true;
}

bool a3deng_resize_native_memory_31a790(
    std::uint8_t* a3deng_base,
    std::uint32_t required_bytes) {
    if (!a3deng_base)
        return false;
    auto& q = a3deng_partial_queues_319ae0()[a3deng_base];
    if (required_bytes == 0u) {
        q.native_heap.clear();
    } else if (q.native_heap.size() != required_bytes) {
        q.native_heap.assign(required_bytes, 0u);
    } else {
        std::fill(q.native_heap.begin(), q.native_heap.end(), 0u);
    }
    const std::uint64_t begin = q.native_heap.empty()
        ? 0u
        : reinterpret_cast<std::uint64_t>(q.native_heap.data());
    *reinterpret_cast<std::uint64_t*>(a3deng_base + auro_engine_v4_ida::kA3DENG_off_input_storage_begin) = begin;
    *reinterpret_cast<std::uint64_t*>(a3deng_base + auro_engine_v4_ida::kA3DENG_off_input_storage_end) =
        begin + q.native_heap.size();
    return required_bytes == 0u || begin != 0u;
}

bool a3deng_native_create_instance_31a790(
    std::uint8_t* a3deng_base,
    std::uint64_t api,
    const std::uint8_t* static_params) {
    using RequiredMemoryFn = std::uint32_t (*)(const void*);
    using CreateFn = std::uint64_t (*)(void*, const void*);
    const auto required = reinterpret_cast<RequiredMemoryFn>(
        a3deng_native_api_fn_319e60(api, auro_engine_v4_ida::kA3DENG_api_vtable_off_required_memory));
    const auto create = reinterpret_cast<CreateFn>(
        a3deng_native_api_fn_319e60(api, auro_engine_v4_ida::kA3DENG_api_vtable_off_create_instance));
    if (!required || !create)
        return false;
    const std::uint32_t required_bytes = required(static_params);
    if (!a3deng_resize_native_memory_31a790(a3deng_base, required_bytes))
        return false;
    auto& q = a3deng_partial_queues_319ae0()[a3deng_base];
    const std::uint64_t instance = create(q.native_heap.data(), static_params);
    *reinterpret_cast<std::uint64_t*>(a3deng_base + auro_engine_v4_ida::kA3DENG_off_instance) = instance;
    if (instance == 0u) {
        q.native_heap.clear();
        *reinterpret_cast<std::uint64_t*>(a3deng_base + auro_engine_v4_ida::kA3DENG_off_input_storage_begin) = 0u;
        *reinterpret_cast<std::uint64_t*>(a3deng_base + auro_engine_v4_ida::kA3DENG_off_input_storage_end) = 0u;
    }
    return instance != 0u;
}

std::uint32_t a3deng_settings_config_target_device_3198b0(
    const std::uint8_t* settings_0x34) {
    if (!settings_0x34)
        return 0u;
    const std::uint32_t decoder_mode =
        a3deng_read_u32_319e60(settings_0x34, auro_engine_v4_ida::kA3DENG_settings_off_decoder_mode);
    if (decoder_mode == 2u)
        return 0u;
    if (settings_0x34[auro_engine_v4_ida::kA3DENG_settings_off_stereo_device] == 0u)
        return 5u;
    return 2u * static_cast<std::uint32_t>(
        settings_0x34[auro_engine_v4_ida::kA3DENG_settings_off_headset_connected] != 0u) + 1u;
}

bool a3deng_settings_config_equals_3199c0(
    const std::uint8_t* lhs_0x34,
    const std::uint8_t* rhs_0x34) {
    if (!lhs_0x34 || !rhs_0x34)
        return false;
    return lhs_0x34[auro_engine_v4_ida::kA3DENG_settings_off_stereo_device]
        == rhs_0x34[auro_engine_v4_ida::kA3DENG_settings_off_stereo_device]
        && lhs_0x34[auro_engine_v4_ida::kA3DENG_settings_off_headset_connected]
            == rhs_0x34[auro_engine_v4_ida::kA3DENG_settings_off_headset_connected]
        && *reinterpret_cast<const std::uint32_t*>(lhs_0x34 + auro_engine_v4_ida::kA3DENG_settings_off_output_sample_type)
            == *reinterpret_cast<const std::uint32_t*>(rhs_0x34 + auro_engine_v4_ida::kA3DENG_settings_off_output_sample_type)
        && *reinterpret_cast<const std::uint32_t*>(lhs_0x34 + auro_engine_v4_ida::kA3DENG_settings_off_output_bit_depth)
            == *reinterpret_cast<const std::uint32_t*>(rhs_0x34 + auro_engine_v4_ida::kA3DENG_settings_off_output_bit_depth)
        && *reinterpret_cast<const std::uint32_t*>(lhs_0x34 + auro_engine_v4_ida::kA3DENG_settings_off_output_channel_mask)
            == *reinterpret_cast<const std::uint32_t*>(rhs_0x34 + auro_engine_v4_ida::kA3DENG_settings_off_output_channel_mask)
        && *reinterpret_cast<const std::uint32_t*>(lhs_0x34 + auro_engine_v4_ida::kA3DENG_settings_off_decoder_mode)
            == *reinterpret_cast<const std::uint32_t*>(rhs_0x34 + auro_engine_v4_ida::kA3DENG_settings_off_decoder_mode)
        && *reinterpret_cast<const std::uint32_t*>(lhs_0x34 + auro_engine_v4_ida::kA3DENG_settings_off_input_channel_mask)
            == *reinterpret_cast<const std::uint32_t*>(rhs_0x34 + auro_engine_v4_ida::kA3DENG_settings_off_input_channel_mask)
        && *reinterpret_cast<const std::uint32_t*>(lhs_0x34 + auro_engine_v4_ida::kA3DENG_settings_off_input_sample_rate)
            == *reinterpret_cast<const std::uint32_t*>(rhs_0x34 + auro_engine_v4_ida::kA3DENG_settings_off_input_sample_rate)
        && lhs_0x34[auro_engine_v4_ida::kA3DENG_settings_off_is_abr]
            == rhs_0x34[auro_engine_v4_ida::kA3DENG_settings_off_is_abr];
}

std::uint32_t a3deng_settings_virtualization_mode_319980(
    const std::uint8_t* settings_0x34) {
    if (!settings_0x34)
        return 1u;
    return (settings_0x34[auro_engine_v4_ida::kA3DENG_settings_off_stereo_device] == 0u)
        | static_cast<std::uint32_t>(
            settings_0x34[auro_engine_v4_ida::kA3DENG_settings_off_dynamic_request_flag] == 0u);
}

std::uint32_t a3deng_settings_listening_mode_3199a0(
    const std::uint8_t* settings_0x34) {
    if (!settings_0x34)
        return 3u;
    return 3u * static_cast<std::uint32_t>(
        settings_0x34[auro_engine_v4_ida::kA3DENG_settings_off_stereo_device] == 0u
        || settings_0x34[auro_engine_v4_ida::kA3DENG_settings_off_dynamic_headphone_flag] == 0u);
}

bool a3deng_native_configure_319e60(
    std::uint8_t* a3deng_base,
    const std::uint8_t* settings_0x34) {
    const std::uint64_t api = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_api);
    const std::uint64_t instance = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_instance);
    using ConfigureFn = std::uint32_t (*)(std::uint64_t, const void*);
    const auto configure = reinterpret_cast<ConfigureFn>(
        a3deng_native_api_fn_319e60(api, auro_engine_v4_ida::kA3DENG_api_vtable_off_configure));
    if (!configure)
        return true;

    alignas(16) std::array<std::uint8_t, kA3DENG_cfg_size> cfg{};
    const std::uint32_t decoder_mode =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_decoder_mode);
    *reinterpret_cast<std::uint32_t*>(cfg.data() + kA3DENG_cfg_off_decoder_mode) = decoder_mode;
    if (decoder_mode != 4u) {
        *reinterpret_cast<std::uint64_t*>(cfg.data() + kA3DENG_cfg_off_input_sample_rate) =
            a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_input_sample_rate_runtime);
        auro_a3deng_v4_android_channel_layout_31ace0_partial(
            cfg.data() + kA3DENG_cfg_off_input_channels,
            a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_input_channel_mask_runtime),
            a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_hdmi_channel_mapping_runtime));
    }
    *reinterpret_cast<std::uint64_t*>(cfg.data() + kA3DENG_cfg_off_output_sample_rate) =
        auro_a3deng_v4_android_A3DENG_output_sample_rate_319920_partial(a3deng_base);
    *reinterpret_cast<std::uint32_t*>(cfg.data() + kA3DENG_cfg_off_target_device) =
        auro_a3deng_v4_android_A3DENG_Settings_Config_target_device_3198b0_partial(settings_0x34);
    auro_a3deng_v4_android_channel_layout_31ace0_partial(
        cfg.data() + kA3DENG_cfg_off_output_channels,
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_output_channel_mask_runtime),
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_hdmi_channel_mapping_runtime));
    if (decoder_mode != 2u
        && settings_0x34[auro_engine_v4_ida::kA3DENG_settings_off_stereo_device] == 0u) {
        *reinterpret_cast<std::uint32_t*>(cfg.data() + kA3DENG_cfg_off_output_audio_configuration) = 1u;
        *reinterpret_cast<std::uint32_t*>(cfg.data() + kA3DENG_cfg_off_is_abr) =
            settings_0x34[auro_engine_v4_ida::kA3DENG_settings_off_is_abr];
    }
    return configure(instance, cfg.data()) == 0u;
}

bool a3deng_native_set_dynamic_319e60(
    std::uint8_t* a3deng_base,
    const std::uint8_t* settings_0x34) {
    const std::uint64_t api = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_api);
    const std::uint64_t instance = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_instance);
    using DynamicFn = std::uint32_t (*)(std::uint64_t, void*);
    const auto get_dynamic = reinterpret_cast<DynamicFn>(
        a3deng_native_api_fn_319e60(api, auro_engine_v4_ida::kA3DENG_api_vtable_off_get_dynamic));
    const auto set_dynamic = reinterpret_cast<DynamicFn>(
        a3deng_native_api_fn_319e60(api, auro_engine_v4_ida::kA3DENG_api_vtable_off_set_dynamic));
    if (!set_dynamic)
        return true;

    alignas(16) std::array<std::uint8_t, kA3DENG_dyn_size> dyn{};
    if (get_dynamic && get_dynamic(instance, dyn.data()) != 0u)
        return false;

    const bool stereo =
        settings_0x34[auro_engine_v4_ida::kA3DENG_settings_off_stereo_device] != 0u;
    const bool requested_virtual =
        settings_0x34[auro_engine_v4_ida::kA3DENG_settings_off_dynamic_request_flag] != 0u;
    const bool headphone_auro =
        settings_0x34[auro_engine_v4_ida::kA3DENG_settings_off_dynamic_headphone_flag] != 0u;
    *reinterpret_cast<std::uint32_t*>(dyn.data() + kA3DENG_dyn_off_actual_virtualization) =
        static_cast<std::uint32_t>(!stereo || !requested_virtual);
    *reinterpret_cast<std::uint32_t*>(dyn.data() + kA3DENG_dyn_off_preset) = 1u;
    *reinterpret_cast<std::uint32_t*>(dyn.data() + kA3DENG_dyn_off_listening_mode) =
        3u * static_cast<std::uint32_t>(!stereo || !headphone_auro);
    *reinterpret_cast<std::uint32_t*>(dyn.data() + kA3DENG_dyn_off_strength) = 12u;
    *reinterpret_cast<std::uint32_t*>(dyn.data() + kA3DENG_dyn_off_direct) = 0u;
    *reinterpret_cast<std::uint32_t*>(dyn.data() + kA3DENG_dyn_off_alt_3d) = 0u;
    *reinterpret_cast<float*>(dyn.data() + kA3DENG_dyn_off_hp_head_size) = 1.0f;
    *reinterpret_cast<std::uint32_t*>(dyn.data() + kA3DENG_dyn_off_hp_room) =
        a3deng_read_u32_319e60(settings_0x34, auro_engine_v4_ida::kA3DENG_settings_off_hp_room);
    *reinterpret_cast<std::uint32_t*>(dyn.data() + kA3DENG_dyn_off_hp_hrtf_preset) =
        a3deng_read_u32_319e60(settings_0x34, auro_engine_v4_ida::kA3DENG_settings_off_hp_hrtf_preset);
    return set_dynamic(instance, dyn.data()) == 0u;
}

void a3deng_channel_layout_add_31ace0(
    std::uint8_t* layout_0x188,
    std::uint64_t& count,
    std::uint32_t channel) {
    constexpr std::uint64_t kMaxNativeEntries = 24u;
    if (channel >= 31u || count >= kMaxNativeEntries)
        return;
    const std::uintptr_t slot = 8u + static_cast<std::uintptr_t>(count) * 16u;
    *reinterpret_cast<std::uint64_t*>(layout_0x188 + slot) = count;
    *reinterpret_cast<std::uint32_t*>(layout_0x188 + slot + 8u) = channel;
    *reinterpret_cast<std::uint64_t*>(layout_0x188) = ++count;
}

void a3deng_pruned_output_clear_31b7f0(std::uint8_t* a3deng_base) {
    std::memset(a3deng_base + auro_engine_v4_ida::kA3DENG_off_pruned_output_info_storage, 0, 0x190u);
    a3deng_base[auro_engine_v4_ida::kA3DENG_off_pruned_output_valid] = 1u;
}

void a3deng_pruned_output_add_pair_31b7f0(
    std::uint8_t* a3deng_base,
    std::uint64_t& count,
    std::uint32_t stream_index,
    std::uint32_t channel) {
    if (!a3deng_base || count >= 24u || channel >= 31u)
        return;
    const std::uintptr_t pair_base =
        auro_engine_v4_ida::kA3DENG_off_pruned_output_count + 8u
        + static_cast<std::uintptr_t>(count) * auro_engine_v4_ida::kA3DENG_pruned_output_channel_entry_stride;
    *reinterpret_cast<std::uint64_t*>(a3deng_base + pair_base) = stream_index;
    *reinterpret_cast<std::uint32_t*>(a3deng_base + pair_base + 8u) = channel;
    *reinterpret_cast<std::uint64_t*>(a3deng_base + auro_engine_v4_ida::kA3DENG_off_pruned_output_count) =
        ++count;
    const std::uint64_t required_streams = static_cast<std::uint64_t>(stream_index) + 1u;
    auto* max_streams = reinterpret_cast<std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_pruned_output_max_sample);
    if (*max_streams < required_streams)
        *max_streams = required_streams;
}

std::uint32_t a3deng_pruned_output_count_31b330(const std::uint8_t* a3deng_base) {
    if (!a3deng_base || a3deng_base[auro_engine_v4_ida::kA3DENG_off_pruned_output_valid] == 0u)
        return 0u;
    const auto count = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_pruned_output_count);
    return static_cast<std::uint32_t>(std::min<std::uint64_t>(count, 24u));
}

std::uint32_t a3deng_pruned_output_channel_at_31b330(
    const std::uint8_t* a3deng_base,
    std::uint32_t idx) {
    if (!a3deng_base || idx >= 24u)
        return 31u;
    const std::uintptr_t entry =
        auro_engine_v4_ida::kA3DENG_off_pruned_output_channel_entries
        + static_cast<std::uintptr_t>(idx) * auro_engine_v4_ida::kA3DENG_pruned_output_channel_entry_stride;
    return *reinterpret_cast<const std::uint32_t*>(a3deng_base + entry);
}

void a3deng_mirror_pruned_output_31b7f0(
    std::uint8_t* a3deng_base,
    std::uint32_t output_mask,
    std::uint32_t fallback_channel_count) {
    a3deng_pruned_output_clear_31b7f0(a3deng_base);
    std::uint64_t count = 0u;
    for (std::uint32_t channel = 0u; channel != 31u && count < 24u; ++channel) {
        if (((output_mask >> channel) & 1u) == 0u)
            continue;
        a3deng_pruned_output_add_pair_31b7f0(
            a3deng_base,
            count,
            static_cast<std::uint32_t>(count),
            channel);
    }
    if (count == 0u && fallback_channel_count != 0u)
        *reinterpret_cast<std::uint64_t*>(a3deng_base + auro_engine_v4_ida::kA3DENG_off_pruned_output_max_sample) =
            fallback_channel_count;
}

std::uint32_t a3deng_pruned_output_mask_31b330(std::uint8_t* a3deng_base) {
    if (a3deng_base[auro_engine_v4_ida::kA3DENG_off_pruned_output_valid] == 0u)
        return a3deng_read_u32_319e60(
            a3deng_base,
            auro_engine_v4_ida::kA3DENG_off_output_channel_mask_runtime);
    std::uint32_t mask = 0u;
    const std::uint32_t count = a3deng_pruned_output_count_31b330(a3deng_base);
    std::uint32_t idx = 0u;
    const std::uint32_t bulk = count & ~3u;
    for (; idx != bulk; idx += 4u) {
        const std::uint32_t ch0 = a3deng_pruned_output_channel_at_31b330(a3deng_base, idx + 0u);
        const std::uint32_t ch1 = a3deng_pruned_output_channel_at_31b330(a3deng_base, idx + 1u);
        const std::uint32_t ch2 = a3deng_pruned_output_channel_at_31b330(a3deng_base, idx + 2u);
        const std::uint32_t ch3 = a3deng_pruned_output_channel_at_31b330(a3deng_base, idx + 3u);
        if (ch0 < 31u) mask |= 1u << ch0;
        if (ch1 < 31u) mask |= 1u << ch1;
        if (ch2 < 31u) mask |= 1u << ch2;
        if (ch3 < 31u) mask |= 1u << ch3;
    }
    for (; idx != count; ++idx) {
        const std::uint32_t channel = a3deng_pruned_output_channel_at_31b330(a3deng_base, idx);
        if (channel < 31u)
            mask |= 1u << channel;
    }
    return mask;
}

std::uint32_t a3deng_pruned_output_channel_count_for_pop_31b7f0(std::uint8_t* a3deng_base) {
    const auto streams = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_pruned_output_max_sample);
    return static_cast<std::uint32_t>(std::min<std::uint64_t>(streams, 0xFFFFFFFFull));
}

bool a3deng_render_audio_native_31b7f0(
    std::uint8_t* a3deng_base,
    std::uint8_t*& output_bytes,
    std::int32_t& remaining_output_byte_count) {
    const std::uint64_t api = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_api);
    const std::uint64_t instance = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_instance);
    if (api == 0u || instance == 0u || api == a3deng_global_api_319ae0())
        return false;
    using RenderAudioFn = std::uint32_t (*)(std::uint64_t, void*, char*);
    const auto fn = reinterpret_cast<RenderAudioFn>(
        *reinterpret_cast<const std::uint64_t*>(
            static_cast<std::uintptr_t>(api + auro_engine_v4_ida::kA3DENG_api_vtable_off_render_audio)));
    if (!fn)
        return false;
    const std::uint64_t info = auro_a3deng_v4_android_A3DENG_get_output_info_31b4e0_partial(a3deng_base);
    const std::uint32_t block_count = static_cast<std::uint32_t>(info >> 32u);
    if (block_count == 0u)
        return false;
    const std::uint32_t decoder_mode =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_decoder_mode);
    const std::uint32_t output_mode = decoder_mode == 2u
        ? 0u
        : a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_output_mode);
    std::uint32_t sample_rate =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_input_sample_rate_runtime);
    if (sample_rate != 0u) {
        if (output_mode == 1u) {
            while (sample_rate > 48000u)
                sample_rate >>= 1u;
        } else if (output_mode == 2u) {
            std::uint32_t prev = sample_rate;
            while (prev > 96000u) {
                sample_rate = prev >> 1u;
                prev = sample_rate;
            }
            while (sample_rate < 48001u) {
                sample_rate = prev * 2u;
                prev = sample_rate;
            }
        }
    } else {
        sample_rate = output_mode == 2u ? 96000u : 48000u;
    }
    const std::uint32_t sample_type =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_output_sample_type_runtime);
    const std::uint32_t bit_depth =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_output_bit_depth_runtime);
    const std::uint32_t sample_bits = a3deng_output_sample_bits_31b7f0(sample_type);
    if (sample_bits == 0u || sample_bits != bit_depth)
        return false;
    const std::uint32_t block_size =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_pipeline_audio_block_size);
    const std::uint32_t channels = auro_channel_Mask_count(
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_output_channel_mask_runtime), 0, 0);
    alignas(16) std::uint8_t audio_block[0x110]{};
    if (!a3deng_make_pop_audio_block_31b7f0(
            audio_block,
            sizeof(audio_block),
            sample_type,
            sample_rate,
            block_size,
            channels,
            output_bytes)) {
        return false;
    }
    for (std::uint32_t part = 0u; part != block_count; ++part) {
        std::memset(a3deng_base + auro_engine_v4_ida::kA3DENG_off_pruned_output_info_storage, 0, 0x190u);
        a3deng_base[auro_engine_v4_ida::kA3DENG_off_pruned_output_valid] = 1u;
        const std::uint32_t rc = fn(
            instance,
            audio_block,
            reinterpret_cast<char*>(a3deng_base + auro_engine_v4_ida::kA3DENG_off_pruned_output_info_storage));
        if (rc != 0u)
            return false;
        const std::uint32_t pruned_channels =
            a3deng_pruned_output_channel_count_for_pop_31b7f0(a3deng_base);
        const std::uint64_t part_bytes =
            (static_cast<std::uint64_t>(sample_bits) * static_cast<std::uint64_t>(block_size * pruned_channels)) >> 3u;
        if (part_bytes > static_cast<std::uint64_t>(remaining_output_byte_count))
            return false;
        output_bytes += part_bytes;
        remaining_output_byte_count -= static_cast<std::int32_t>(part_bytes);
        *reinterpret_cast<std::uint64_t*>(audio_block + kA3DENGAudioBlockOffData_31af50) =
            reinterpret_cast<std::uint64_t>(output_bytes);
    }
    return true;
}

std::uint64_t a3deng_render_pcm_passthrough_31b7f0(
    std::uint8_t* a3deng_base,
    std::uint8_t* output_bytes,
    std::uint32_t frames,
    std::uint32_t output_mask,
    std::uint32_t input_mask) {
    auto& q = a3deng_partial_queues_319ae0()[a3deng_base];
    if (q.input.empty())
        return 0u;
    const std::uint32_t input_sample_type =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_input_sample_type_runtime);
    const std::uint32_t output_sample_type =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_output_sample_type_runtime);
    const std::uint32_t input_sample_bytes = a3deng_input_sample_bytes_31af50(input_sample_type);
    const std::uint32_t output_sample_bytes = a3deng_output_sample_bytes_31b7f0(output_sample_type);
    if (input_sample_bytes == 0u || output_sample_bytes == 0u || input_sample_bytes != output_sample_bytes)
        return 0u;

    const std::uint32_t hdmi_mapping =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_hdmi_channel_mapping_runtime);
    const auto input_channels = a3deng_mask_channel_order_31ace0(input_mask, hdmi_mapping);
    const auto output_channels = a3deng_mask_channel_order_31ace0(output_mask, hdmi_mapping);
    if (input_channels.empty() || output_channels.empty())
        return 0u;

    const std::uint64_t input_frame_bytes =
        static_cast<std::uint64_t>(input_channels.size()) * input_sample_bytes;
    const std::uint64_t output_frame_bytes =
        static_cast<std::uint64_t>(output_channels.size()) * output_sample_bytes;
    const std::uint32_t available_frames =
        static_cast<std::uint32_t>(q.input.size() / input_frame_bytes);
    const std::uint32_t frames_to_copy = std::min(frames, available_frames);
    if (frames_to_copy == 0u)
        return 0u;

    for (std::uint32_t frame = 0u; frame != frames_to_copy; ++frame) {
        const std::uint64_t input_frame = static_cast<std::uint64_t>(frame) * input_frame_bytes;
        const std::uint64_t output_frame = static_cast<std::uint64_t>(frame) * output_frame_bytes;
        for (std::uint32_t out_idx = 0u; out_idx != output_channels.size(); ++out_idx) {
            const std::int32_t in_idx = a3deng_find_channel_index_31ace0(input_channels, output_channels[out_idx]);
            if (in_idx < 0)
                continue;
            const std::uint64_t input_off =
                input_frame + static_cast<std::uint64_t>(in_idx) * input_sample_bytes;
            const std::uint64_t output_off =
                output_frame + static_cast<std::uint64_t>(out_idx) * output_sample_bytes;
            std::memcpy(output_bytes + output_off, q.input.data() + input_off, output_sample_bytes);
        }
    }

    const std::uint64_t consumed = static_cast<std::uint64_t>(frames_to_copy) * input_frame_bytes;
    q.input.erase(q.input.begin(), q.input.begin() + static_cast<std::ptrdiff_t>(consumed));
    return static_cast<std::uint64_t>(frames_to_copy) * output_frame_bytes;
}

bool a3deng_push_audio_native_31af50(
    std::uint8_t* a3deng_base,
    const std::uint8_t* input_bytes,
    std::uint32_t input_byte_count) {
    const std::uint64_t api = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_api);
    const std::uint64_t instance = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_instance);
    if (api == 0u || instance == 0u || api == a3deng_global_api_319ae0())
        return false;
    using PushInputFn = std::uint32_t (*)(std::uint64_t, const void*);
    const auto fn = reinterpret_cast<PushInputFn>(
        *reinterpret_cast<const std::uint64_t*>(
            static_cast<std::uintptr_t>(api + auro_engine_v4_ida::kA3DENG_api_vtable_off_push_input)));
    if (!fn)
        return false;

    const std::uint32_t decoder_mode =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_decoder_mode);
    const std::uint32_t input_sample_type =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_input_sample_type_runtime);
    const std::uint32_t input_sample_rate =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_input_sample_rate_runtime);
    const std::uint32_t input_mask =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_input_channel_mask_runtime);
    const std::uint32_t input_block_frames = decoder_mode == 4u
        ? input_byte_count
        : auro_a3deng_v4_android_A3DENG_input_block_size_31b280_partial(a3deng_base);
    const std::uint32_t channels = decoder_mode == 4u
        ? 1u
        : auro_channel_Mask_count(input_mask, 0, 0);

    alignas(16) std::uint8_t audio_block[0xF0]{};
    if (!a3deng_make_push_audio_block_31af50(
            audio_block,
            sizeof(audio_block),
            decoder_mode,
            input_sample_type,
            input_sample_rate,
            input_block_frames,
            channels,
            input_bytes)) {
        return false;
    }
    return fn(instance, audio_block) == 0u;
}

bool a3deng_has_api_and_instance_31b4e0(const std::uint8_t* a3deng_base) {
    if (!a3deng_base)
        return false;
    const auto api = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_api);
    const auto instance = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_instance);
    return api != 0u && instance != 0u;
}

std::int64_t auro_a3deng_v4_android_A3DENG_valid_31ae50_partial(
    const std::uint8_t* a3deng_base) {
    return a3deng_has_api_and_instance_31b4e0(a3deng_base) ? 1 : 0;
}

struct A3DENGSettingsCompare_319a00 {
    bool config_changed = false;
    bool dynamic_changed = false;
};

A3DENGSettingsCompare_319a00 a3deng_settings_compare_319a00(
    const std::uint8_t* lhs_0x34,
    const std::uint8_t* rhs_0x34) {
    A3DENGSettingsCompare_319a00 out{};
    if (!lhs_0x34 || !rhs_0x34) {
        out.config_changed = true;
        out.dynamic_changed = true;
        return out;
    }

    out.config_changed =
        !a3deng_settings_config_equals_3199c0(lhs_0x34, rhs_0x34);

    const bool lhs_stereo = lhs_0x34[auro_engine_v4_ida::kA3DENG_settings_off_stereo_device] != 0u;
    const bool rhs_stereo = rhs_0x34[auro_engine_v4_ida::kA3DENG_settings_off_stereo_device] != 0u;
    const bool lhs_virtual = lhs_0x34[auro_engine_v4_ida::kA3DENG_settings_off_dynamic_request_flag] != 0u;
    const bool rhs_virtual = rhs_0x34[auro_engine_v4_ida::kA3DENG_settings_off_dynamic_request_flag] != 0u;
    const bool lhs_listen = lhs_0x34[auro_engine_v4_ida::kA3DENG_settings_off_dynamic_headphone_flag] != 0u;
    const bool rhs_listen = rhs_0x34[auro_engine_v4_ida::kA3DENG_settings_off_dynamic_headphone_flag] != 0u;

    const bool lhs_actual_virtual = !lhs_stereo || !lhs_virtual;
    const bool rhs_actual_virtual = !rhs_stereo || !rhs_virtual;
    const bool lhs_actual_listen = lhs_stereo && lhs_listen;
    const bool rhs_actual_listen = rhs_stereo && rhs_listen;

    out.dynamic_changed =
        lhs_actual_virtual != rhs_actual_virtual
        || lhs_actual_listen != rhs_actual_listen
        || *reinterpret_cast<const std::uint32_t*>(lhs_0x34 + auro_engine_v4_ida::kA3DENG_settings_off_hp_room)
            != *reinterpret_cast<const std::uint32_t*>(rhs_0x34 + auro_engine_v4_ida::kA3DENG_settings_off_hp_room)
        || *reinterpret_cast<const std::uint32_t*>(lhs_0x34 + auro_engine_v4_ida::kA3DENG_settings_off_hp_hrtf_preset)
            != *reinterpret_cast<const std::uint32_t*>(rhs_0x34 + auro_engine_v4_ida::kA3DENG_settings_off_hp_hrtf_preset);

    return out;
}

} // namespace

std::uint32_t auro_a3deng_v4_android_A3DENG_input_block_size_31b280_partial(
    const std::uint8_t* a3deng_base) {
    return a3deng_input_block_size_31b280(a3deng_base);
}

std::uint32_t auro_a3deng_v4_android_A3DENG_Settings_Config_target_device_3198b0_partial(
    const std::uint8_t* settings_0x34) {
    return a3deng_settings_config_target_device_3198b0(settings_0x34);
}

bool auro_a3deng_v4_android_A3DENG_Settings_Config_equals_3199c0_partial(
    const std::uint8_t* lhs_0x34,
    const std::uint8_t* rhs_0x34) {
    return a3deng_settings_config_equals_3199c0(lhs_0x34, rhs_0x34);
}

std::uint32_t auro_a3deng_v4_android_A3DENG_Settings_virtualization_mode_319980_partial(
    const std::uint8_t* settings_0x34) {
    return a3deng_settings_virtualization_mode_319980(settings_0x34);
}

std::uint32_t auro_a3deng_v4_android_A3DENG_Settings_listening_mode_3199a0_partial(
    const std::uint8_t* settings_0x34) {
    return a3deng_settings_listening_mode_3199a0(settings_0x34);
}

std::uint8_t* auro_a3deng_v4_android_A3DENG_construct_319ae0_partial(
    std::uint8_t* a3deng_base,
    std::uint32_t pipeline_audio_block_size,
    std::uint32_t output_mode) {
    if (!a3deng_base)
        return nullptr;
    std::memset(a3deng_base, 0, 0x2E0u);
    a3deng_base[96u] = 1u;
    a3deng_write_u32_319e60(
        a3deng_base,
        auro_engine_v4_ida::kA3DENG_off_pipeline_audio_block_size,
        pipeline_audio_block_size);
    a3deng_write_u32_319e60(
        a3deng_base,
        auro_engine_v4_ida::kA3DENG_off_output_mode,
        output_mode);
    *reinterpret_cast<std::uint64_t*>(a3deng_base + auro_engine_v4_ida::kA3DENG_off_api) =
        a3deng_global_api_319ae0();
    *reinterpret_cast<std::uint64_t*>(a3deng_base + auro_engine_v4_ida::kA3DENG_off_instance) = 0u;
    a3deng_partial_queues_319ae0()[a3deng_base] = {};
    return a3deng_base;
}

std::uint8_t* auro_a3deng_v4_android_A3DENG_AuroInitialize_318c90_partial(
    std::uint32_t pipeline_audio_block_size,
    std::uint32_t output_mode) {
    auto* a3deng_base = new std::uint8_t[0x2E0u];
    if (!auro_a3deng_v4_android_A3DENG_construct_319ae0_partial(
            a3deng_base,
            pipeline_audio_block_size,
            output_mode)) {
        delete[] a3deng_base;
        return nullptr;
    }
    return a3deng_base;
}

void auro_a3deng_v4_android_A3DENG_destroy_319ae0_partial(std::uint8_t* a3deng_base) {
    if (!a3deng_base)
        return;
    auro_a3deng_v4_android_A3DENG_destroy_instance_319dd0_partial(a3deng_base);
    auto& map = a3deng_partial_queues_319ae0();
    map.erase(a3deng_base);
    a3deng_debug_dirs_31bde0().erase(a3deng_base);
    std::memset(a3deng_base, 0, 0x2E0u);
}

bool auro_a3deng_v4_android_A3DENG_get_version_31bbe0_partial(
    const std::uint8_t* a3deng_base,
    A3DENGVersionFields31bbe0* out) {
    if (!a3deng_base || !out)
        return false;
    const auto api_ptr = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_api);
    if (api_ptr == 0u)
        return false;
    const auto* api = reinterpret_cast<const A3DENGSyntheticApi31bbe0*>(api_ptr);
    out->major = api->version_major;
    out->minor = api->version_minor;
    out->patch = api->version_patch;
    out->beta = api->version_beta;
    out->tag = api->version_tag;
    return true;
}

bool auro_a3deng_v4_android_A3DENG_destroy_instance_319dd0_partial(
    std::uint8_t* a3deng_base) {
    if (!a3deng_base)
        return false;
    const auto api = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_api);
    if (api == 0u)
        return false;
    const auto instance = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_instance);
    if (instance != 0u && api != a3deng_global_api_319ae0()) {
        using DestroyFn = void (*)(std::uint64_t);
        const auto fn = reinterpret_cast<DestroyFn>(
            a3deng_native_api_fn_319e60(
                api,
                auro_engine_v4_ida::kA3DENG_api_vtable_off_destroy_instance));
        if (fn)
            fn(instance);
    }
    *reinterpret_cast<std::uint64_t*>(a3deng_base + auro_engine_v4_ida::kA3DENG_off_instance) = 0u;
    auto& q = a3deng_partial_queues_319ae0()[a3deng_base];
    q.input.clear();
    q.native_heap.clear();
    q.pushed_bytes = 0u;
    q.rendered_blocks = 0u;
    q.debug_push_count = 0u;
    q.debug_pop_count = 0u;
    q.codec_v3_pop_render = nullptr;
    q.codec_v3_pop_render_user = nullptr;
    a3deng_base[auro_engine_v4_ida::kA3DENG_off_pruned_output_valid] = 0u;
    return true;
}

bool auro_a3deng_v4_android_A3DENG_create_instance_31a790_partial(
    std::uint8_t* a3deng_base,
    std::uint32_t decoder_mode) {
    if (!a3deng_base)
        return false;
    const auto api = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_api);
    if (api == 0u)
        return false;

    if (api != a3deng_global_api_319ae0()) {
        using DefaultStaticFn = std::uint32_t (*)(void*);
        using ValidateStaticFn = std::uint32_t (*)(const void*);
        const auto defaults = reinterpret_cast<DefaultStaticFn>(
            a3deng_native_api_fn_319e60(
                api,
                auro_engine_v4_ida::kA3DENG_api_vtable_off_default_static));
        const auto validate = reinterpret_cast<ValidateStaticFn>(
            a3deng_native_api_fn_319e60(
                api,
                auro_engine_v4_ida::kA3DENG_api_vtable_off_validate_static));
        if (!defaults || !validate)
            return false;

        alignas(16) std::uint8_t static_params[kA3DENGStaticParamsBytes_31a790]{};
        if (defaults(static_params) != 0u)
            return false;
        a3deng_static_params_apply_android_defaults_31a790(
            static_params,
            a3deng_read_u32_319e60(
                a3deng_base,
                auro_engine_v4_ida::kA3DENG_off_pipeline_audio_block_size));
        if (validate(static_params) != 0u)
            return false;

        const bool created = a3deng_native_create_instance_31a790(a3deng_base, api, static_params);
        a3deng_write_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_decoder_mode, decoder_mode);
        return created;
    }

    *reinterpret_cast<std::uint64_t*>(a3deng_base + auro_engine_v4_ida::kA3DENG_off_input_storage_begin) =
        reinterpret_cast<std::uint64_t>(a3deng_base + auro_engine_v4_ida::kA3DENG_off_pruned_output_info_storage);
    *reinterpret_cast<std::uint64_t*>(a3deng_base + auro_engine_v4_ida::kA3DENG_off_input_storage_end) =
        reinterpret_cast<std::uint64_t>(a3deng_base + auro_engine_v4_ida::kA3DENG_off_pruned_output_info_storage);
    *reinterpret_cast<std::uint64_t*>(a3deng_base + auro_engine_v4_ida::kA3DENG_off_instance) =
        reinterpret_cast<std::uint64_t>(a3deng_base + auro_engine_v4_ida::kA3DENG_off_input_storage_begin);
    a3deng_write_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_decoder_mode, decoder_mode);
    return true;
}

std::uint8_t* auro_a3deng_v4_android_A3DENG_settings_319e50_partial(
    std::uint8_t* a3deng_base) {
    return a3deng_base ? a3deng_base + auro_engine_v4_ida::kA3DENG_off_settings : nullptr;
}

const std::uint8_t* auro_a3deng_v4_android_A3DENG_settings_319e50_partial(
    const std::uint8_t* a3deng_base) {
    return a3deng_base ? a3deng_base + auro_engine_v4_ida::kA3DENG_off_settings : nullptr;
}

std::uint32_t auro_a3deng_v4_android_A3DENG_output_sample_rate_3198d0_partial(
    std::uint32_t input_sample_rate,
    std::uint32_t output_mode) {
    if (input_sample_rate == 0u)
        return output_mode == 2u ? 96000u : 48000u;
    if (output_mode == 1u) {
        std::uint32_t rate = input_sample_rate;
        std::uint32_t prev = rate;
        do {
            prev = rate;
            rate >>= 1u;
        } while (prev > 48000u);
        return prev;
    }
    if (output_mode == 2u) {
        std::uint32_t rate = input_sample_rate;
        std::uint32_t prev = rate;
        do {
            prev = rate;
            rate >>= 1u;
        } while (prev > 96000u);
        do {
            rate = prev;
            prev *= 2u;
        } while (rate < 48001u);
        return rate;
    }
    return input_sample_rate;
}

std::uint32_t auro_a3deng_v4_android_A3DENG_output_sample_rate_319920_partial(
    const std::uint8_t* a3deng_base) {
    if (!a3deng_base)
        return 48000u;
    const std::uint32_t input_sample_rate =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_input_sample_rate_runtime);
    std::uint32_t output_mode = 0u;
    if (a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_decoder_mode) != 2u)
        output_mode = a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_output_mode);
    return auro_a3deng_v4_android_A3DENG_output_sample_rate_3198d0_partial(input_sample_rate, output_mode);
}

std::string auro_a3deng_v4_android_A3DENG_AuroVersion_3188e0_partial() {
    alignas(8) std::uint8_t tmp[0x2E0u];
    auro_a3deng_v4_android_A3DENG_construct_319ae0_partial(tmp, 0x40u, 1u);
    A3DENGVersionFields31bbe0 version{};
    if (!auro_a3deng_v4_android_A3DENG_get_version_31bbe0_partial(tmp, &version)) {
        auro_a3deng_v4_android_A3DENG_destroy_319ae0_partial(tmp);
        return {};
    }

    char buf[96];
    if (version.beta != 0x7FFFFFFFu) {
        std::snprintf(
            buf,
            sizeof(buf),
            "Version: %u.%u.%ubeta%u",
            version.major,
            version.minor,
            version.patch,
            version.beta);
    } else {
        std::snprintf(
            buf,
            sizeof(buf),
            "Version: %u.%u.%u",
            version.major,
            version.minor,
            version.patch);
    }
    std::string out(buf);
    if (version.tag && *version.tag) {
        out.push_back('-');
        out += version.tag;
    }
    auro_a3deng_v4_android_A3DENG_destroy_319ae0_partial(tmp);
    return out;
}

bool auro_a3deng_v4_android_A3DENG_AuroRelease_318d14_partial(std::uint8_t* a3deng_base) {
    if (!a3deng_base)
        return false;
    auro_a3deng_v4_android_A3DENG_destroy_319ae0_partial(a3deng_base);
    delete[] a3deng_base;
    return true;
}

std::int64_t auro_a3deng_v4_android_A3DENG_AuroIsValid_318cd0_partial(
    std::uint8_t* a3deng_base) {
    if (!a3deng_base)
        return 0;
    return auro_a3deng_v4_android_A3DENG_valid_31ae50_partial(a3deng_base);
}

std::int64_t auro_a3deng_v4_android_A3DENG_AuroReset_318fa0_partial(
    std::uint8_t* a3deng_base) {
    if (!a3deng_base)
        return 0;
    return auro_a3deng_v4_android_A3DENG_reset_31ae90_partial(a3deng_base) ? 1 : 0;
}

std::int64_t auro_a3deng_v4_android_A3DENG_AuroPush_318fe0_partial(
    std::uint8_t* a3deng_base,
    const std::uint8_t* input_bytes,
    std::int32_t input_byte_count) {
    if (!a3deng_base || !input_bytes)
        return 0;
    return auro_a3deng_v4_android_A3DENG_push_31af50_partial(
        a3deng_base,
        input_bytes,
        static_cast<std::uint32_t>(input_byte_count));
}

std::int64_t auro_a3deng_v4_android_A3DENG_AuroGetLatencyUs_319050_partial(
    std::uint8_t* a3deng_base) {
    if (!a3deng_base)
        return 0;
    return auro_a3deng_v4_android_A3DENG_get_latency_nr_samples_31b5f0_partial(a3deng_base);
}

std::int64_t auro_a3deng_v4_android_A3DENG_AuroGetOutputChannelCount_319090_partial(
    std::uint8_t* a3deng_base) {
    if (!a3deng_base)
        return 0;
    return auro_a3deng_v4_android_A3DENG_get_output_channel_count_31b400_partial(a3deng_base);
}

std::int64_t auro_a3deng_v4_android_A3DENG_AuroPop_319120_partial(
    std::uint8_t* a3deng_base,
    std::uint8_t* output_bytes,
    std::int32_t output_byte_count) {
    if (!a3deng_base)
        return 0;
    if (!output_bytes || output_byte_count <= 0)
        return -1;
    return auro_a3deng_v4_android_A3DENG_pop_31b7a0_partial(
        a3deng_base,
        output_bytes,
        output_byte_count);
}

std::int64_t auro_a3deng_v4_android_A3DENG_AuroGetMaximumOutputBytecount_319040_partial(
    std::uint8_t* a3deng_base) {
    if (!a3deng_base)
        return 0;
    const std::uint64_t packed =
        auro_a3deng_v4_android_A3DENG_get_maximum_output_bytecount_31b5a0_partial(a3deng_base);
    if ((packed & 0xFF00000000ull) == 0u)
        return -1;
    return static_cast<std::int64_t>(packed & 0xFFFFFFFFull);
}

std::int64_t auro_a3deng_v4_android_A3DENG_AuroInputBlockSize_3191d0_partial(
    std::uint8_t* a3deng_base) {
    if (!a3deng_base)
        return 0;
    return static_cast<std::int64_t>(
        auro_a3deng_v4_android_A3DENG_input_block_size_31b280_partial(a3deng_base));
}

std::int64_t auro_a3deng_v4_android_A3DENG_AuroResetAudioState_319210_partial(
    std::uint8_t* a3deng_base) {
    if (!a3deng_base)
        return 0;
    return auro_a3deng_v4_android_A3DENG_reset_audio_state_31bc00_partial(a3deng_base) ? 1 : 0;
}

bool auro_a3deng_v4_android_A3DENG_AuroSetDebugPath_319260_partial(
    std::uint8_t* a3deng_base,
    const char* path_utf8,
    const char* tag_utf8) {
    if (!a3deng_base)
        return false;
    if (!path_utf8 || !tag_utf8)
        return false;
    (void)auro_a3deng_v4_android_A3DENG_set_debug_path_31bde0_partial(
        a3deng_base,
        path_utf8,
        tag_utf8);
    return true;
}

void auro_a3deng_v4_android_A3DENG_settings_pack_like_jni_318d60(
    std::uint8_t* out_0x34,
    const A3DENGSettingsFields318d60& f) {
    if (!out_0x34)
        return;
    std::memset(out_0x34, 0, auro_engine_v4_ida::kA3DENG_settings_size);
    out_0x34[auro_engine_v4_ida::kA3DENG_settings_off_stereo_device] =
        f.is_stereo_device ? 1u : 0u;
    out_0x34[auro_engine_v4_ida::kA3DENG_settings_off_headset_connected] =
        f.headset_connected ? 1u : 0u;
    a3deng_write_u32_319e60(
        out_0x34,
        auro_engine_v4_ida::kA3DENG_settings_off_decoder_mode,
        f.decoder_mode);
    a3deng_write_u32_319e60(
        out_0x34,
        auro_engine_v4_ida::kA3DENG_settings_off_output_channel_mask,
        f.output_layout_mask);
    a3deng_write_u32_319e60(
        out_0x34,
        auro_engine_v4_ida::kA3DENG_settings_off_output_sample_type,
        f.output_sample_type);
    a3deng_write_u32_319e60(
        out_0x34,
        auro_engine_v4_ida::kA3DENG_settings_off_output_bit_depth,
        f.output_bit_depth);
    a3deng_write_u32_319e60(
        out_0x34,
        auro_engine_v4_ida::kA3DENG_settings_off_input_channel_mask,
        f.pcm_input_layout_mask);
    a3deng_write_u32_319e60(
        out_0x34,
        auro_engine_v4_ida::kA3DENG_settings_off_input_sample_rate,
        f.pcm_input_sample_rate);
    a3deng_write_u32_319e60(
        out_0x34,
        auro_engine_v4_ida::kA3DENG_settings_off_input_sample_type,
        f.pcm_input_sample_type);
    const std::uint32_t hdmi = f.channels_backs_before_surrounds ? 1u : 0u;
    a3deng_write_u32_319e60(
        out_0x34,
        auro_engine_v4_ida::kA3DENG_settings_off_hdmi_channel_mapping,
        hdmi);
    out_0x34[auro_engine_v4_ida::kA3DENG_settings_off_is_abr] = f.abr_mode_enabled ? 1u : 0u;
    out_0x34[auro_engine_v4_ida::kA3DENG_settings_off_dynamic_request_flag] =
        f.virtualization_enabled ? 1u : 0u;
    out_0x34[auro_engine_v4_ida::kA3DENG_settings_off_dynamic_headphone_flag] =
        f.listening_mode_auro3d ? 1u : 0u;
    a3deng_write_u32_319e60(
        out_0x34,
        auro_engine_v4_ida::kA3DENG_settings_off_hp_room,
        f.hp_user_preset);
    a3deng_write_u32_319e60(
        out_0x34,
        auro_engine_v4_ida::kA3DENG_settings_off_hp_hrtf_preset,
        f.hp_hrtf_preset);
}

std::int64_t auro_a3deng_v4_android_A3DENG_AuroUpdate2_318d60_partial(
    std::uint8_t* a3deng_base,
    const A3DENGSettingsFields318d60& f) {
    if (!a3deng_base)
        return 0;
    alignas(8) std::uint8_t settings[0x34];
    auro_a3deng_v4_android_A3DENG_settings_pack_like_jni_318d60(settings, f);
    return auro_a3deng_v4_android_A3DENG_update_319e60_partial(a3deng_base, settings) ? 1 : 0;
}

bool auro_a3deng_v4_android_A3DENG_update_319e60_partial(
    std::uint8_t* a3deng_base,
    const std::uint8_t* settings_0x34) {
    if (!a3deng_base || !settings_0x34)
        return false;

    const std::uint32_t decoder_mode =
        a3deng_read_u32_319e60(settings_0x34, auro_engine_v4_ida::kA3DENG_settings_off_decoder_mode);
    const std::uint32_t output_mask =
        a3deng_read_u32_319e60(settings_0x34, auro_engine_v4_ida::kA3DENG_settings_off_output_channel_mask);
    const std::uint32_t output_sample_type =
        a3deng_read_u32_319e60(settings_0x34, auro_engine_v4_ida::kA3DENG_settings_off_output_sample_type);
    const std::uint32_t output_bit_depth =
        a3deng_read_u32_319e60(settings_0x34, auro_engine_v4_ida::kA3DENG_settings_off_output_bit_depth);
    const std::uint32_t input_mask =
        a3deng_read_u32_319e60(settings_0x34, auro_engine_v4_ida::kA3DENG_settings_off_input_channel_mask);
    const std::uint32_t input_sample_rate =
        a3deng_read_u32_319e60(settings_0x34, auro_engine_v4_ida::kA3DENG_settings_off_input_sample_rate);
    const std::uint32_t input_sample_type =
        a3deng_read_u32_319e60(settings_0x34, auro_engine_v4_ida::kA3DENG_settings_off_input_sample_type);
    const std::uint32_t hdmi_mapping =
        a3deng_read_u32_319e60(settings_0x34, auro_engine_v4_ida::kA3DENG_settings_off_hdmi_channel_mapping);

    if (!(decoder_mode == 0u || decoder_mode == 1u || decoder_mode == 2u || decoder_mode == 4u))
        return false;
    const std::uint32_t native_output_bits = a3deng_output_sample_bits_31b7f0(output_sample_type);
    if (native_output_bits == 0u || native_output_bits != output_bit_depth)
        return false;

    if (*reinterpret_cast<const std::uint64_t*>(a3deng_base + auro_engine_v4_ida::kA3DENG_off_instance) == 0u) {
        if (!auro_a3deng_v4_android_A3DENG_create_instance_31a790_partial(a3deng_base, decoder_mode))
            return false;
    }

    const bool was_configured = a3deng_base[auro_engine_v4_ida::kA3DENG_off_configured] != 0u;
    auto cmp = a3deng_settings_compare_319a00(
        settings_0x34,
        auro_a3deng_v4_android_A3DENG_settings_319e50_partial(a3deng_base));
    if (!was_configured)
        cmp = {true, true};
    std::memcpy(auro_a3deng_v4_android_A3DENG_settings_319e50_partial(a3deng_base), settings_0x34, 0x34u);
    a3deng_base[auro_engine_v4_ida::kA3DENG_off_configured] = 1u;
    if (!cmp.config_changed && !cmp.dynamic_changed)
        return true;

    a3deng_write_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_decoder_mode, decoder_mode);
    a3deng_write_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_output_channel_mask_runtime, output_mask);
    a3deng_write_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_output_sample_type_runtime, output_sample_type);
    a3deng_write_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_output_bit_depth_runtime, output_bit_depth);
    a3deng_write_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_input_channel_mask_runtime, input_mask);
    a3deng_write_u32_319e60(
        a3deng_base,
        auro_engine_v4_ida::kA3DENG_off_input_sample_rate_runtime,
        input_sample_rate);
    a3deng_write_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_input_sample_type_runtime, input_sample_type);
    a3deng_write_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_hdmi_channel_mapping_runtime, hdmi_mapping);
    a3deng_write_u32_319e60(
        a3deng_base,
        auro_engine_v4_ida::kA3DENG_off_hp_user_preset_runtime,
        a3deng_read_u32_319e60(settings_0x34, auro_engine_v4_ida::kA3DENG_settings_off_hp_room));
    a3deng_write_u32_319e60(
        a3deng_base,
        auro_engine_v4_ida::kA3DENG_off_hp_hrtf_preset_runtime,
        a3deng_read_u32_319e60(settings_0x34, auro_engine_v4_ida::kA3DENG_settings_off_hp_hrtf_preset));
    auto& q = a3deng_partial_queues_319ae0()[a3deng_base];
    q.target_device =
        auro_a3deng_v4_android_A3DENG_Settings_Config_target_device_3198b0_partial(settings_0x34);
    q.output_sample_rate = auro_a3deng_v4_android_A3DENG_output_sample_rate_319920_partial(a3deng_base);
    q.effective_virtualization_mode =
        auro_a3deng_v4_android_A3DENG_Settings_virtualization_mode_319980_partial(settings_0x34);
    q.effective_listening_mode =
        auro_a3deng_v4_android_A3DENG_Settings_listening_mode_3199a0_partial(settings_0x34);
    q.is_abr = settings_0x34[auro_engine_v4_ida::kA3DENG_settings_off_is_abr] != 0u;
    if (cmp.config_changed && !a3deng_native_configure_319e60(a3deng_base, settings_0x34))
        return false;
    if (cmp.dynamic_changed && !a3deng_native_set_dynamic_319e60(a3deng_base, settings_0x34))
        return false;
    return true;
}

void auro_a3deng_v4_android_channel_layout_31ace0_partial(
    std::uint8_t* layout_0x188,
    std::uint32_t channel_mask,
    std::uint32_t hdmi_channel_mapping) {
    if (!layout_0x188)
        return;
    std::memset(layout_0x188, 0, 0x188u);

    std::uint64_t count = 0u;
    if (hdmi_channel_mapping == 1u) {
        constexpr std::array<std::uint32_t, 8> kHdmiOrder_1db2b0 = {
            0u, 1u, 2u, 3u, 7u, 8u, 4u, 5u,
        };
        for (std::uint32_t channel : kHdmiOrder_1db2b0) {
            if (((channel_mask >> channel) & 1u) != 0u)
                a3deng_channel_layout_add_31ace0(layout_0x188, count, channel);
        }
        return;
    }

    for (std::uint32_t channel = 0u; channel != 31u; ++channel) {
        if (((channel_mask >> channel) & 1u) != 0u)
            a3deng_channel_layout_add_31ace0(layout_0x188, count, channel);
    }
}

bool a3deng_output_info_valid_31b4e0(std::uint64_t output_info) {
    return (output_info & auro_engine_v4_ida::kA3DENG_output_info_block_size_low_mask) != 0u
        && static_cast<std::uint32_t>(output_info >> 32u) != 0u;
}

std::uint64_t auro_a3deng_v4_android_A3DENG_get_output_info_31b4e0_partial(
    std::uint8_t* a3deng_base) {
    if (!a3deng_has_api_and_instance_31b4e0(a3deng_base))
        return 0u;
    // IDA @ 0x31B4E0: (output_block_count << 32) | (this+52 & 0xFFFFFF00) | (uint8_t)this+52.
    const std::uint32_t pipeline_field =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_pipeline_audio_block_size);
    return (static_cast<std::uint64_t>(a3deng_output_block_count_31b4e0(a3deng_base)) << 32u)
        | static_cast<std::uint64_t>(pipeline_field & auro_engine_v4_ida::kA3DENG_output_info_block_size_high_mask)
        | static_cast<std::uint64_t>(static_cast<std::uint8_t>(pipeline_field));
}

std::uint64_t auro_a3deng_v4_android_A3DENG_get_maximum_output_bytecount_31b5a0_partial(
    std::uint8_t* a3deng_base) {
    if (!a3deng_base)
        return 0u;
    const std::uint64_t info = auro_a3deng_v4_android_A3DENG_get_output_info_31b4e0_partial(a3deng_base);
    const std::uint32_t block_size = static_cast<std::uint32_t>(info);
    const std::uint32_t block_count = static_cast<std::uint32_t>(info >> 32u);
    if (!a3deng_output_info_valid_31b4e0(info))
        return 0u;
    const std::uint32_t output_bit_depth =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_output_bit_depth_runtime);
    const std::uint32_t sample_bits = a3deng_output_sample_bits_31b7f0(
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_output_sample_type_runtime));
    if (sample_bits == 0u || sample_bits != output_bit_depth)
        return 0u;
    const std::uint32_t output_mask =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_output_channel_mask_runtime);
    const std::uint32_t channel_count = auro_channel_Mask_count(output_mask, 0, 0);
    const std::uint64_t bytes =
        static_cast<std::uint64_t>(sample_bits >> 3u) * block_size * block_count * channel_count;
    return 0x100000000ull | (bytes & 0xFFFFFFFFull);
}

bool auro_a3deng_v4_android_A3DENG_reset_31ae90_partial(std::uint8_t* a3deng_base) {
    if (!a3deng_has_api_and_instance_31b4e0(a3deng_base))
        return false;
    a3deng_base[auro_engine_v4_ida::kA3DENG_off_pruned_output_valid] = 0u;
    return true;
}

std::int64_t auro_a3deng_v4_android_A3DENG_get_latency_nr_samples_31b5f0_partial(
    std::uint8_t* a3deng_base) {
    if (!a3deng_has_api_and_instance_31b4e0(a3deng_base))
        return -1;
    const std::uint64_t api = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_api);
    const std::uint64_t instance = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_instance);
    using GetLatencyFn = std::uint32_t (*)(std::uint64_t, std::uint64_t*);
    const auto get_latency = reinterpret_cast<GetLatencyFn>(
        a3deng_native_api_fn_319e60(api, auro_engine_v4_ida::kA3DENG_api_vtable_off_get_latency));
    if (get_latency) {
        std::uint64_t latency = 0u;
        if (get_latency(instance, &latency) != 0u)
            return -1;
        return static_cast<std::int64_t>(latency);
    }
    return 0;
}

std::int64_t auro_a3deng_v4_android_A3DENG_get_latency_us_31b700_partial(
    std::uint8_t* a3deng_base) {
    const std::int64_t latency_samples =
        auro_a3deng_v4_android_A3DENG_get_latency_nr_samples_31b5f0_partial(a3deng_base);
    if (latency_samples == -1)
        return -1;
    const std::uint32_t output_sample_rate = a3deng_current_output_sample_rate_31b700(a3deng_base);
    if (output_sample_rate == 0u)
        return -1;
    const float latency_us =
        (static_cast<float>(static_cast<std::int32_t>(latency_samples))
         / static_cast<float>(static_cast<std::int32_t>(output_sample_rate)))
        * 1000000.0f;
    return static_cast<std::uint32_t>(static_cast<std::int32_t>(latency_us));
}

std::int64_t auro_a3deng_v4_android_A3DENG_push_31af50_partial(
    std::uint8_t* a3deng_base,
    const std::uint8_t* input_bytes,
    std::uint32_t input_byte_count) {
    if (!a3deng_has_api_and_instance_31b4e0(a3deng_base) || !input_bytes)
        return 0;
    if (a3deng_push_audio_native_31af50(a3deng_base, input_bytes, input_byte_count)) {
        a3deng_debug_log_buffer_31bf70(a3deng_base, "push", input_bytes, input_byte_count);
        return 1;
    }
    const std::uint32_t input_mask =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_input_channel_mask_runtime);
    const std::uint32_t decoder_mode =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_decoder_mode);
    const std::uint32_t input_sample_type =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_input_sample_type_runtime);
    if (decoder_mode == 4u) {
        auto& q = a3deng_partial_queues_319ae0()[a3deng_base];
        a3deng_debug_log_buffer_31bf70(a3deng_base, "push", input_bytes, input_byte_count);
        q.input.insert(q.input.end(), input_bytes, input_bytes + input_byte_count);
        q.pushed_bytes += input_byte_count;
        q.rendered_blocks = input_byte_count == 0u ? 0u : 1u;
        return 1;
    }
    const std::uint32_t input_sample_bytes = a3deng_input_sample_bytes_31af50(input_sample_type);
    if (input_sample_bytes == 0u)
        return 0;
    const std::uint32_t input_block_frames =
        auro_a3deng_v4_android_A3DENG_input_block_size_31b280_partial(a3deng_base);
    const std::uint32_t channel_count = std::max<std::uint32_t>(1u, auro_channel_Mask_count(input_mask, 0, 0));
    const std::uint64_t bytes_per_block =
        static_cast<std::uint64_t>(input_block_frames) * channel_count * input_sample_bytes;
    auto& q = a3deng_partial_queues_319ae0()[a3deng_base];
    a3deng_debug_log_buffer_31bf70(a3deng_base, "push", input_bytes, input_byte_count);
    q.input.insert(q.input.end(), input_bytes, input_bytes + input_byte_count);
    q.pushed_bytes += input_byte_count;
    q.rendered_blocks = bytes_per_block == 0u ? 0u : static_cast<std::uint32_t>(q.input.size() / bytes_per_block);
    return 1;
}

bool auro_a3deng_v4_android_A3DENG_reset_audio_state_31bc00_partial(std::uint8_t* a3deng_base) {
    if (!a3deng_has_api_and_instance_31b4e0(a3deng_base))
        return false;
    bool ok = true;
    const std::uint64_t api = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_api);
    const std::uint64_t instance = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_instance);
    using ResetAudioStateFn = std::uint32_t (*)(std::uint64_t);
    const auto reset_audio_state = reinterpret_cast<ResetAudioStateFn>(
        a3deng_native_api_fn_319e60(api, auro_engine_v4_ida::kA3DENG_api_vtable_off_reset_audio_state));
    if (reset_audio_state)
        ok = reset_audio_state(instance) == 0u;
    if (!ok)
        return false;
    auto& q = a3deng_partial_queues_319ae0()[a3deng_base];
    q.input.clear();
    q.rendered_blocks = 0u;
    q.pushed_bytes = 0u;
    a3deng_base[auro_engine_v4_ida::kA3DENG_off_pruned_output_valid] = 0u;
    std::memset(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_pruned_output_info_storage,
        0,
        0x190u);
    return ok;
}

bool auro_a3deng_v4_android_A3DENG_set_debug_path_31bde0_partial(
    std::uint8_t* a3deng_base,
    const char* path_utf8,
    const char* tag_utf8) {
    if (!a3deng_base)
        return false;
    const auto api = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_api);
    if (api == 0u)
        return false;
    if (!path_utf8 || !tag_utf8)
        return false;

    std::string dir(path_utf8);
    if (!dir.empty() && dir.back() != '/' && dir.back() != '\\')
        dir.push_back('/');
    dir += "debug_dump";
    if (dir.empty())
        return false;
    if (!a3deng_mkdir_p(dir))
        return false;
    a3deng_debug_dirs_31bde0()[a3deng_base] = std::move(dir);
    return true;
}

bool auro_a3deng_v4_android_A3DENG_pop_internal_31b7f0_partial(
    std::uint8_t* a3deng_base,
    std::uint8_t*& output_bytes,
    std::int32_t& remaining_output_byte_count) {
    if (!a3deng_has_api_and_instance_31b4e0(a3deng_base) || !output_bytes || remaining_output_byte_count <= 0)
        return false;
    if (a3deng_render_audio_native_31b7f0(a3deng_base, output_bytes, remaining_output_byte_count))
        return true;
    const std::uint64_t info = auro_a3deng_v4_android_A3DENG_get_output_info_31b4e0_partial(a3deng_base);
    const std::uint32_t block_count = static_cast<std::uint32_t>(info >> 32u);
    if (!a3deng_output_info_valid_31b4e0(info))
        return false;

    const std::uint32_t pipeline_block =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_pipeline_audio_block_size);
    const std::uint32_t input_block =
        auro_a3deng_v4_android_A3DENG_input_block_size_31b280_partial(a3deng_base);
    const std::uint32_t output_mask =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_output_channel_mask_runtime);
    const std::uint32_t input_mask =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_input_channel_mask_runtime);
    const std::uint32_t requested_output_channels = auro_channel_Mask_count(output_mask, 0, 0);
    const std::uint32_t output_sample_type =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_output_sample_type_runtime);
    const std::uint32_t bytes_per_sample = a3deng_output_sample_bytes_31b7f0(output_sample_type);
    if (bytes_per_sample == 0u)
        return false;
    const std::uint32_t decoder_mode =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_decoder_mode);
    const std::uint64_t api = *reinterpret_cast<const std::uint64_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_api);
    auto& q = a3deng_partial_queues_319ae0()[a3deng_base];
    const bool synthetic_codec_pop =
        decoder_mode == 2u
        && q.codec_v3_pop_render != nullptr
        && (api == 0u || api == a3deng_global_api_319ae0());
    const std::uint32_t render_block =
        synthetic_codec_pop && input_block != 0u ? input_block : pipeline_block;

    for (std::uint32_t part = 0u; part != block_count; ++part) {
        a3deng_mirror_pruned_output_31b7f0(a3deng_base, output_mask, requested_output_channels);
        const std::uint32_t pruned_channels =
            a3deng_pruned_output_channel_count_for_pop_31b7f0(a3deng_base);
        const std::uint64_t part_bytes_u64 =
            static_cast<std::uint64_t>(render_block) * pruned_channels * bytes_per_sample;
        if (part_bytes_u64 > static_cast<std::uint64_t>(remaining_output_byte_count))
            return false;
        if (part_bytes_u64 > 0u) {
            std::memset(output_bytes, 0, static_cast<std::size_t>(part_bytes_u64));
            if (decoder_mode != 4u) {
                std::uint64_t rendered = 0u;
                if (synthetic_codec_pop) {
                    rendered = q.codec_v3_pop_render(
                        q.codec_v3_pop_render_user,
                        a3deng_base,
                        output_bytes,
                        render_block,
                        output_mask,
                        input_mask);
                    if (rendered == 0u)
                        return false;
                }
                if (!synthetic_codec_pop && rendered == 0u) {
                    rendered = a3deng_render_pcm_passthrough_31b7f0(
                        a3deng_base,
                        output_bytes,
                        render_block,
                        output_mask,
                        input_mask);
                }
                (void)rendered;
            }
            a3deng_debug_log_buffer_31bf70(
                a3deng_base,
                "pop",
                output_bytes,
                static_cast<std::uint32_t>(part_bytes_u64));
            output_bytes += part_bytes_u64;
            remaining_output_byte_count -= static_cast<std::int32_t>(part_bytes_u64);
        }
    }
    q.input.clear();
    q.rendered_blocks = 0u;
    return true;
}

std::int64_t auro_a3deng_v4_android_A3DENG_pop_31b7a0_partial(
    std::uint8_t* a3deng_base,
    std::uint8_t* output_bytes,
    std::int32_t output_byte_count) {
    std::uint8_t* output_cursor = output_bytes;
    std::int32_t remaining = output_byte_count;
    if (!auro_a3deng_v4_android_A3DENG_pop_internal_31b7f0_partial(a3deng_base, output_cursor, remaining))
        return -1;
    return static_cast<std::int64_t>(output_byte_count - remaining);
}

std::uint32_t auro_a3deng_v4_android_A3DENG_get_output_layout_31b330_partial(
    std::uint8_t* a3deng_base) {
    if (!a3deng_base)
        return 0u;
    return a3deng_pruned_output_mask_31b330(a3deng_base);
}

std::uint32_t auro_a3deng_v4_android_A3DENG_get_output_channel_count_31b400_partial(
    std::uint8_t* a3deng_base) {
    if (!a3deng_base)
        return 0u;
    return auro_channel_Mask_count(auro_a3deng_v4_android_A3DENG_get_output_layout_31b330_partial(a3deng_base), 0, 0);
}

void auro_a3deng_v4_android_A3DENG_set_codec_v3_pop_render_hook_partial(
    std::uint8_t* a3deng_base,
    A3dengCodecV3PopRenderFn fn,
    void* user) {
    if (!a3deng_base)
        return;
    auto& q = a3deng_partial_queues_319ae0()[a3deng_base];
    q.codec_v3_pop_render = fn;
    q.codec_v3_pop_render_user = user;
}

const std::uint8_t* auro_a3deng_v4_android_A3DENG_partial_queue_input_data_partial(
    std::uint8_t* a3deng_base) {
    if (!a3deng_base)
        return nullptr;
    const auto it = a3deng_partial_queues_319ae0().find(a3deng_base);
    if (it == a3deng_partial_queues_319ae0().end() || it->second.input.empty())
        return nullptr;
    return it->second.input.data();
}

std::size_t auro_a3deng_v4_android_A3DENG_partial_queue_input_size_partial(
    std::uint8_t* a3deng_base) {
    if (!a3deng_base)
        return 0u;
    const auto it = a3deng_partial_queues_319ae0().find(a3deng_base);
    return it == a3deng_partial_queues_319ae0().end() ? 0u : it->second.input.size();
}

static std::int32_t a3deng_read_interleaved_sample_i32(
    const std::uint8_t* sample_bytes,
    std::uint32_t sample_bytes_count) {
    if (sample_bytes_count == 4u) {
        std::int32_t v = 0;
        std::memcpy(&v, sample_bytes, sizeof(v));
        return v;
    }
    if (sample_bytes_count == 3u) {
        int v = static_cast<int>(sample_bytes[0])
            | (static_cast<int>(sample_bytes[1]) << 8)
            | (static_cast<int>(sample_bytes[2]) << 16);
        if ((v & 0x800000) != 0)
            v -= 0x1000000;
        return static_cast<std::int32_t>(v);
    }
    if (sample_bytes_count == 2u) {
        std::int16_t v = 0;
        std::memcpy(&v, sample_bytes, sizeof(v));
        return static_cast<std::int32_t>(v);
    }
    return 0;
}

static void a3deng_write_interleaved_sample_i32(
    std::uint8_t* sample_bytes,
    std::uint32_t sample_bytes_count,
    std::int32_t v) {
    if (sample_bytes_count == 4u) {
        std::memcpy(sample_bytes, &v, sizeof(v));
        return;
    }
    if (sample_bytes_count == 3u) {
        sample_bytes[0] = static_cast<std::uint8_t>(v & 0xFF);
        sample_bytes[1] = static_cast<std::uint8_t>((static_cast<std::uint32_t>(v) >> 8) & 0xFF);
        sample_bytes[2] = static_cast<std::uint8_t>((static_cast<std::uint32_t>(v) >> 16) & 0xFF);
        return;
    }
    if (sample_bytes_count == 2u) {
        const std::int16_t s = static_cast<std::int16_t>(v);
        std::memcpy(sample_bytes, &s, sizeof(s));
    }
}

bool auro_a3deng_v4_android_A3DENG_consume_interleaved_input_to_planar_i32_partial(
    std::uint8_t* a3deng_base,
    std::uint32_t frames,
    std::uint32_t input_mask,
    const std::uint64_t* out_channel_ptrs_27,
    std::uint32_t plane_stride_samples) {
    if (!a3deng_base || !out_channel_ptrs_27 || frames == 0u || plane_stride_samples == 0u)
        return false;
    auto& q = a3deng_partial_queues_319ae0()[a3deng_base];
    if (q.input.empty())
        return false;

    const std::uint32_t input_sample_type =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_input_sample_type_runtime);
    const std::uint32_t input_sample_bytes = a3deng_input_sample_bytes_31af50(input_sample_type);
    if (input_sample_bytes == 0u)
        return false;

    const std::uint32_t hdmi_mapping =
        a3deng_read_u32_319e60(a3deng_base, auro_engine_v4_ida::kA3DENG_off_hdmi_channel_mapping_runtime);
    const auto input_channels = a3deng_mask_channel_order_31ace0(input_mask, hdmi_mapping);
    if (input_channels.empty())
        return false;

    const std::uint64_t input_frame_bytes =
        static_cast<std::uint64_t>(input_channels.size()) * input_sample_bytes;
    const std::uint32_t available_frames =
        static_cast<std::uint32_t>(q.input.size() / input_frame_bytes);
    const std::uint32_t frames_to_copy = std::min(frames, available_frames);
    if (frames_to_copy == 0u)
        return false;

    for (std::uint32_t slot = 0u; slot != 27u; ++slot) {
        if (out_channel_ptrs_27[slot] == 0u)
            continue;
        auto* plane = reinterpret_cast<std::int32_t*>(static_cast<std::uintptr_t>(out_channel_ptrs_27[slot]));
        std::memset(plane, 0, static_cast<std::size_t>(plane_stride_samples) * sizeof(std::int32_t));
    }

    for (std::uint32_t frame = 0u; frame != frames_to_copy; ++frame) {
        const std::uint64_t input_frame = static_cast<std::uint64_t>(frame) * input_frame_bytes;
        for (std::uint32_t in_idx = 0u; in_idx != input_channels.size(); ++in_idx) {
            const std::uint32_t channel = input_channels[in_idx];
            if (channel >= 27u || out_channel_ptrs_27[channel] == 0u)
                continue;
            const std::uint64_t input_off =
                input_frame + static_cast<std::uint64_t>(in_idx) * input_sample_bytes;
            auto* plane = reinterpret_cast<std::int32_t*>(
                static_cast<std::uintptr_t>(out_channel_ptrs_27[channel]));
            plane[frame] = a3deng_read_interleaved_sample_i32(q.input.data() + input_off, input_sample_bytes);
        }
    }

    const std::uint64_t consumed = static_cast<std::uint64_t>(frames_to_copy) * input_frame_bytes;
    q.input.erase(q.input.begin(), q.input.begin() + static_cast<std::ptrdiff_t>(consumed));
    return frames_to_copy == frames;
}

std::uint64_t a3deng_write_pruned_interleaved_from_planar_i32_partial(
    std::uint8_t* a3deng_base,
    std::uint8_t* output_bytes,
    std::uint32_t frames,
    const std::uint64_t* channel_ptrs_27,
    std::uint32_t output_sample_type) {
    if (!a3deng_base || !output_bytes || !channel_ptrs_27 || frames == 0u)
        return 0u;
    const std::uint32_t output_sample_bytes = a3deng_output_sample_bytes_31b7f0(output_sample_type);
    if (output_sample_bytes == 0u)
        return 0u;
    const std::uint32_t pruned_channels = a3deng_pruned_output_channel_count_for_pop_31b7f0(a3deng_base);
    if (pruned_channels == 0u)
        return 0u;

    const std::uint64_t output_frame_bytes =
        static_cast<std::uint64_t>(pruned_channels) * output_sample_bytes;
    for (std::uint32_t frame = 0u; frame != frames; ++frame) {
        const std::uint64_t output_frame = static_cast<std::uint64_t>(frame) * output_frame_bytes;
        for (std::uint32_t out_idx = 0u; out_idx != pruned_channels; ++out_idx) {
            const std::uint32_t channel = a3deng_pruned_output_channel_at_31b330(a3deng_base, out_idx);
            std::int32_t sample = 0;
            if (channel < 27u && channel_ptrs_27[channel] != 0u) {
                const auto* plane = reinterpret_cast<const std::int32_t*>(
                    static_cast<std::uintptr_t>(channel_ptrs_27[channel]));
                sample = plane[frame];
            }
            const std::uint64_t output_off =
                output_frame + static_cast<std::uint64_t>(out_idx) * output_sample_bytes;
            a3deng_write_interleaved_sample_i32(
                output_bytes + output_off,
                output_sample_bytes,
                sample);
        }
    }
    return static_cast<std::uint64_t>(frames) * output_frame_bytes;
}

OutputGeneratorExtrapolateSources output_generator_select_extrapolate_sources_1024a9(
    std::uint64_t output_channels_table_base,
    std::uint64_t frame_channel_ptr,
    std::uint64_t segment_start,
    std::uint64_t scratch_base,
    std::uint64_t total_samples) {
    OutputGeneratorExtrapolateSources out{};
    if (!frame_channel_ptr)
        return out;

    const auto select_ptr = [output_channels_table_base, segment_start](std::uint32_t ch_idx) -> std::uint64_t {
        if (ch_idx >= kCodecV3ChannelCount || output_channels_table_base == 0)
            return 0;
        const std::uint64_t ch_base = *reinterpret_cast<const std::uint64_t*>(output_channels_table_base + 8ull * ch_idx + 16ull);
        return ch_base ? (ch_base + 4ull * segment_start) : 0;
    };

    const std::uint32_t i0 = *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + kFrameChannelOff_pred_src0_idx);
    const std::uint32_t i1 = *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + kFrameChannelOff_pred_src1_idx);
    const std::uint32_t i2 = *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + kFrameChannelOff_pred_src2_idx);

    out.src0 = select_ptr(i0);
    out.src1 = select_ptr(i1);
    out.src2 = select_ptr(i2);

    // IDA fallback: scratch + N*total_samples (в int32 -> 4*offset по байтам на уровне вызова Extrapolate_process уже учтено).
    if (!out.src0)
        out.src0 = scratch_base + 0ull * 4ull * total_samples;
    if (!out.src1)
        out.src1 = scratch_base + 1ull * 4ull * total_samples;
    if (!out.src2)
        out.src2 = scratch_base + 2ull * 4ull * total_samples;
    return out;
}

void a3deng_u32_vector_assign_sub_31cb30_partial(
    std::uint8_t* vector_base24,
    const void* src_bytes,
    std::size_t uint32_element_count) {
    if (!vector_base24)
        return;
    auto** begin = reinterpret_cast<std::uint32_t**>(vector_base24);
    auto** end = reinterpret_cast<std::uint32_t**>(vector_base24 + 8u);
    auto** cap_end = reinterpret_cast<std::uint32_t**>(vector_base24 + 16u);
    if (*begin) {
        delete[] *begin;
        *begin = nullptr;
        *end = nullptr;
        *cap_end = nullptr;
    }
    if (uint32_element_count == 0u || !src_bytes)
        return;
    auto* p = new std::uint32_t[uint32_element_count];
    std::memcpy(p, src_bytes, uint32_element_count * sizeof(std::uint32_t));
    *begin = p;
    *end = p + uint32_element_count;
    *cap_end = p + uint32_element_count;
}

std::uint32_t a3deng_channel_mask_slots_hdmi_back_before_surround_partial(
    std::uint32_t* out_slots,
    std::uint32_t out_cap,
    std::uint32_t channel_mask) {
    if (!out_slots || out_cap == 0u)
        return 0u;
    static constexpr std::array<std::uint32_t, 8> kOrder = {0u, 1u, 2u, 3u, 7u, 8u, 4u, 5u};
    std::uint32_t n = 0u;
    for (std::uint32_t ch : kOrder) {
        if (((channel_mask >> ch) & 1u) == 0u)
            continue;
        if (n >= out_cap)
            break;
        out_slots[n++] = ch;
    }
    return n;
}

std::int64_t auro_iir_biquad_parameter_Config_float32_t_compute_partial(
    std::uint8_t* param_stack12,
    std::uint64_t cfg_ptr,
    std::uint8_t* coeff_state_out) {
    (void)param_stack12;
    (void)cfg_ptr;
    if (coeff_state_out)
        std::memset(coeff_state_out, 0, 48u);
    return 0;
}

} // namespace auro3deng
