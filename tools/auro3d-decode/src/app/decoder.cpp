#include "decoder.hpp"

#include "../auro3deng/detail/codec_v3_ida.hpp"
#include "../auro3deng/detail/runtime_api.hpp"
#include "../render/java_auro_decode_pcm.hpp"
#include "../util/auro3deng_strength.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>

#ifndef AURO3DENG_ENABLE_NATIVE_IMAGE_PRESET_POINTERS
#define AURO3DENG_ENABLE_NATIVE_IMAGE_PRESET_POINTERS 0
#endif

namespace {

constexpr std::uint32_t kProcessorDescSampleBitsS24 = 24u;
constexpr std::uint32_t kProcessorDescInputLayout = 1u;
constexpr std::uint32_t kProcessorDescOutputLayout = 0u;
constexpr std::uint32_t kInvalidChannelSlot = 0xFFFFFFFFu;
constexpr std::uint32_t kCodecV3ChannelCount = 31u;
constexpr std::uint32_t kCodecV3ChannelMask = 0x7FFFFFFFu;
// Native codec-v3 decode is preferred. XinN fills only requested height slots
// that the carrier/native decoder did not provide.
constexpr bool kEnableAuroMaticXinNUpmix = true;
constexpr bool kEnableSyntheticHeightFallback = false;
constexpr std::size_t kCodecV3OgStateBytes = 2048u;
constexpr std::size_t kCodecV3OgOutTableBytes = 16u + kCodecV3ChannelCount * sizeof(std::uint64_t);
constexpr std::size_t kCodecV3OgSegCtxBytes = 32u;
constexpr std::uintptr_t kCodecV3OgOffPreSegmentsCb = 792u;
constexpr std::uintptr_t kCodecV3OgOffPreSegmentsCtx = 800u;
constexpr std::uintptr_t kCodecV3OgOffTotalSamples = 824u;
constexpr std::uintptr_t kCodecV3OgOffTimelineCursor = 840u;
constexpr std::uintptr_t kCodecV3OgOffDelayLinePtr = 848u;
constexpr std::uintptr_t kCodecV3OgOffFrameDequePtr = 856u;
constexpr std::uintptr_t kCodecV3OgOffBlockInfoPtr = 864u;
constexpr std::uintptr_t kCodecV3OgOffSegmentCtxPtr = kCodecV3OgOffBlockInfoPtr;
constexpr std::uintptr_t kCodecV3SegCtxOffRangesBase = 0u;
constexpr std::uintptr_t kCodecV3SegCtxOffFrameStarted = 8u;
constexpr std::uintptr_t kCodecV3SegCtxOffFramePtrs = 16u;
constexpr std::uintptr_t kCodecV3SegCtxOffCount = 24u;
constexpr std::size_t kCodecV3SegmentStrideBytes = 24u;
constexpr std::uintptr_t kCodecV3FrameOffStart = 0u;
constexpr std::uintptr_t kCodecV3FrameOffEnd = 8u;
constexpr std::uintptr_t kCodecV3FrameOffFlags = 24u;
constexpr std::uintptr_t kCodecV3FrameOffChannelCount = 44u;
constexpr std::uintptr_t kCodecV3FrameOffSlotBase = 48u;
constexpr std::uintptr_t kCodecV3FrameSlotStride = 32u;
constexpr std::uint32_t kCodecV3FrameSlotCapacity = 9u;
constexpr std::uintptr_t kCodecV3FrameSlotOffChannelIndex = 0u;
constexpr std::uintptr_t kCodecV3FrameSlotOffActiveFlag = 4u;
constexpr std::uintptr_t kCodecV3FrameSlotOffChannelPtr = 24u;
constexpr std::size_t kCodecV3FakeFrameBytes = kCodecV3FrameOffSlotBase + kCodecV3FrameSlotCapacity * kCodecV3FrameSlotStride;
constexpr std::size_t kCodecV3FrameDequeSlotCopyBytes = 0x150u; // IDA 0x13d5d0 memcpy size
static_assert(
    (kCodecV3FrameDequeSlotCopyBytes - kCodecV3FrameOffSlotBase) % kCodecV3FrameSlotStride == 0u,
    "FrameDeque memcpy size must align to full slot stride.");
constexpr std::uint32_t kCodecV3FrameDequeCopiedSlotCapacity =
    (kCodecV3FrameDequeSlotCopyBytes > kCodecV3FrameOffSlotBase)
    ? static_cast<std::uint32_t>(
        (kCodecV3FrameDequeSlotCopyBytes - kCodecV3FrameOffSlotBase) / kCodecV3FrameSlotStride)
    : 0u;
static_assert(kCodecV3FrameDequeCopiedSlotCapacity == 9u, "IDA 0x13d5d0 memcpy(0x150) must cover 9 slots.");
static_assert(kCodecV3FrameDequeCopiedSlotCapacity == kCodecV3FrameSlotCapacity, "Copied slot capacity must fit frame slot count.");
constexpr std::size_t kCodecV3FakeFrameChannelBytes = 4096u;
constexpr std::size_t kNativeXinnStepStateBytes = 484064u;
constexpr std::size_t kNativeXinnPlanBlobBytes = 96u;
constexpr std::size_t kNativeXinnUpdateBlobBytes = 276u;
constexpr std::uintptr_t kNativeXinnStepOffBlockInfoPtr = 483952u;
constexpr std::uintptr_t kNativeXinnBlockInfoOffRecords = 32u;

constexpr std::uintptr_t kCodecV3FrameChannelOffCtxPtr = 192u;
constexpr std::uintptr_t kCodecV3FrameChannelOffWordsPtr = 1920u;
constexpr std::uintptr_t kCodecV3FrameChannelOffScaleIdx0 = 0u;
constexpr std::uintptr_t kCodecV3FrameChannelOffScaleIdx1 = 48u;
constexpr std::uintptr_t kCodecV3FrameChannelOffQuantShift = 72u;
constexpr std::uintptr_t kCodecV3FrameChannelOffFlags = 80u;      // v8[20]
constexpr std::uintptr_t kCodecV3FrameChannelOffBaseIndex = 40u;  // v8[10]
constexpr std::uintptr_t kCodecV3FrameChannelOffBitWidth = 44u;   // v8[11]
constexpr std::uintptr_t kCodecV3FrameChannelOffSeed0Primary = 16u;
constexpr std::uintptr_t kCodecV3FrameChannelOffSeed0Secondary = 20u;
constexpr std::uintptr_t kCodecV3FrameChannelOffSeed1Primary = 24u;
constexpr std::uintptr_t kCodecV3FrameChannelOffSeed1Secondary = 28u;
constexpr std::uintptr_t kCodecV3FrameChannelOffSeed2Primary = 32u;
constexpr std::uintptr_t kCodecV3FrameChannelOffMode = 104u;
constexpr std::uintptr_t kCodecV3FrameChannelOffParserState = 92u;
constexpr std::uintptr_t kCodecV3OgOffErrorsBuf = 872u;
constexpr std::uintptr_t kCodecV3OgOffScratchBase = 880u;
constexpr std::uintptr_t kCodecV3OgOffCopyInputFlag = 1232u;
constexpr std::uintptr_t kCodecV3OgOffOutputStatusFlag = 1236u;
constexpr std::uintptr_t kCodecV3OgOffGrStateBase = 0u;
constexpr std::uintptr_t kCodecV3OgGrStateStride = 40u;
constexpr std::uintptr_t kCodecV3OgOffExStateBase = 360u;
constexpr std::uintptr_t kCodecV3OgExStateStride = 48u;
constexpr std::uintptr_t kCodecV3GrOffWordsPtr = 0u;
constexpr std::uintptr_t kCodecV3GrOffCtxPtr = 8u;
constexpr std::uintptr_t kCodecV3GrOffBitIndex = 16u;
constexpr std::uintptr_t kCodecV3GrOffAccumBits = 20u;
constexpr std::uintptr_t kCodecV3GrOffCounter = 36u;
constexpr std::uintptr_t kCodecV3ExOffHead = 0u;
constexpr std::uintptr_t kCodecV3ExOffPhase = 20u;
constexpr std::uintptr_t kCodecV3ExOffFrameChannelPtr = 40u;
constexpr std::size_t kCodecV3ParseResultBytes = 3672u;      // current IDA ParseResult stride
constexpr std::size_t kCodecV3ParseResultPoolStateBytes = 16u; // [base qword][cursor dword][count dword]
constexpr std::size_t kCodecV3NativeFrameDequeBytes = 32u;
/// IDA channel_Parser_t: BitReader @ a1+24, CRC @ a1+12, состояние в *(_DWORD*)(a1+8) == a1_u32[2].
constexpr std::size_t kCodecV3ChannelParserBytes = 64u;

std::uint32_t mask_count_27(std::uint32_t mask) {
    mask &= kCodecV3ChannelMask;
    std::uint32_t count = 0;
    while (mask != 0) {
        count += (mask & 1u);
        mask >>= 1;
    }
    return count;
}

std::uint32_t codec_v3_input_frame_deque_capacity(std::uint32_t block_size) {
    return ((2u * block_size + 255u) >> 8u) + 1u;
}

std::uint32_t codec_v3_ready_frame_deque_capacity(std::uint32_t block_size, std::uint32_t pipeline_stage_count) {
    return ((block_size * std::max<std::uint32_t>(1u, pipeline_stage_count) + 255u) >> 8u) + 1u;
}

void codec_v3_native_frame_deque_init(
    std::vector<std::uint8_t>& deque_storage,
    std::vector<std::uint8_t>& frame_storage,
    std::uint32_t capacity) {
    const std::uint32_t cap = std::max<std::uint32_t>(1u, capacity);
    deque_storage.assign(kCodecV3NativeFrameDequeBytes, 0u);
    frame_storage.assign(static_cast<std::size_t>(cap) * kCodecV3FrameDequeSlotCopyBytes, 0u);
    auto* dq = deque_storage.data();
    *reinterpret_cast<std::uint64_t*>(dq + 0u) = 0u;
    *reinterpret_cast<std::uint64_t*>(dq + 8u) = 0u;
    *reinterpret_cast<std::uint64_t*>(dq + 16u) =
        reinterpret_cast<std::uint64_t>(frame_storage.data());
    *reinterpret_cast<std::uint32_t*>(dq + 24u) = cap;
}

std::uint32_t layout_dimension_27(std::uint32_t mask) {
    mask &= kCodecV3ChannelMask;
    std::uint32_t result = 0;
    for (std::uint32_t bit = 0; bit < kCodecV3ChannelCount; ++bit) {
        if (((mask >> bit) & 1u) == 0)
            continue;
        if (bit > 0x14u)
            continue;
        if (((auro_codec_v3_ida::kAuroChannelMaskHeightLayer >> bit) & 1u) != 0) {
            if (result < auro_codec_v3_ida::kAuroLayoutDimensionHeight)
                result = auro_codec_v3_ida::kAuroLayoutDimensionHeight;
            continue;
        }
        if (((auro_codec_v3_ida::kAuroChannelMaskSurroundLayer >> bit) & 1u) != 0) {
            if (result < auro_codec_v3_ida::kAuroLayoutDimensionSurround)
                result = auro_codec_v3_ida::kAuroLayoutDimensionSurround;
            continue;
        }
        if (((auro_codec_v3_ida::kAuroChannelMaskBaseLayer >> bit) & 1u) != 0) {
            if (result < auro_codec_v3_ida::kAuroLayoutDimensionBase)
                result = auro_codec_v3_ida::kAuroLayoutDimensionBase;
        }
    }
    return result;
}

struct NativeA3dengOutputInfoModel {
    std::uint32_t output_block_count = 0;
    std::uint32_t pipeline_audio_block_size = 0;

    std::uint64_t packed() const {
        return (static_cast<std::uint64_t>(output_block_count) << 32)
            | (pipeline_audio_block_size & auro_engine_v4_ida::kA3DENG_output_info_block_size_high_mask)
            | (pipeline_audio_block_size & auro_engine_v4_ida::kA3DENG_output_info_block_size_low_mask);
    }

    bool render_available() const {
        return (packed() & auro_engine_v4_ida::kA3DENG_output_info_block_size_low_mask) != 0
            && output_block_count != 0;
    }
};

std::uint32_t derive_a3deng_output_sample_rate(std::uint32_t input_sample_rate, std::uint32_t decoder_mode) {
    if (input_sample_rate == 0)
        return decoder_mode == 2u ? 96000u : 48000u;

    if (decoder_mode == 1u) {
        std::uint32_t rate = input_sample_rate;
        while (rate > 48000u)
            rate >>= 1;
        return rate;
    }

    if (decoder_mode == 2u) {
        std::uint32_t rate = input_sample_rate;
        while (rate > 96000u)
            rate >>= 1;
        while (rate < 48001u)
            rate *= 2u;
        return rate;
    }

    return input_sample_rate;
}

std::uint32_t derive_a3deng_push_block_count(
    std::uint32_t constructor_block_count,
    std::uint32_t input_sample_rate,
    std::uint32_t decoder_mode,
    std::uint32_t output_mode) {
    if (input_sample_rate == 0)
        return 0;

    std::uint32_t normalized_rate = input_sample_rate;
    const std::uint32_t rate_mode = decoder_mode == 2u ? 0u : output_mode;
    if (rate_mode == 1u) {
        while (normalized_rate > 48000u)
            normalized_rate >>= 1;
    } else if (rate_mode == 2u) {
        while (normalized_rate > 96000u)
            normalized_rate >>= 1;
        while (normalized_rate < 48001u)
            normalized_rate *= 2u;
    }

    std::uint32_t block_count = constructor_block_count;
    while (normalized_rate != input_sample_rate && normalized_rate != 0u) {
        const bool normalized_above_input = normalized_rate > input_sample_rate;
        normalized_rate = normalized_above_input ? (normalized_rate >> 1) : (normalized_rate << 1);
        block_count = normalized_above_input ? (block_count >> 1) : (block_count << 1);
    }
    return normalized_rate == input_sample_rate ? block_count : 0u;
}

std::size_t a3deng_pop_part_byte_count(
    const NativeA3dengOutputInfoModel& output_info,
    std::uint32_t output_channel_mask,
    std::uint32_t output_sample_type,
    std::uint32_t output_sample_bits) {
    if (!output_info.render_available())
        return 0;
    if (output_sample_type > auro_engine_v4_ida::kOutputSampleTypeInt32)
        return 0;
    const std::uint32_t native_container_bits =
        output_sample_type == auro_engine_v4_ida::kOutputSampleTypeFloat ? 32u : output_sample_bits;
    if (native_container_bits == 0u || (native_container_bits & 7u) != 0u)
        return 0;

    const std::uint32_t channel_count =
        auro3deng::auro_channel_Mask_count(output_channel_mask & 0x7FFFFFFu, 0, 0);
    return (static_cast<std::size_t>(output_info.pipeline_audio_block_size)
            * channel_count
            * native_container_bits) >> 3;
}

std::size_t a3deng_pop_total_byte_count(
    const NativeA3dengOutputInfoModel& output_info,
    std::uint32_t output_channel_mask,
    std::uint32_t output_sample_type,
    std::uint32_t output_sample_bits) {
    return a3deng_pop_part_byte_count(
        output_info,
        output_channel_mask,
        output_sample_type,
        output_sample_bits) * output_info.output_block_count;
}

std::uint64_t a3deng_maximum_output_bytecount_packed(
    const NativeA3dengOutputInfoModel& output_info,
    std::uint32_t output_channel_mask,
    std::uint32_t output_sample_bits) {
    if (!output_info.render_available())
        return 0;
    const std::uint32_t channel_count =
        auro3deng::auro_channel_Mask_count(output_channel_mask & 0x7FFFFFFu, 0, 0);
    const std::uint64_t byte_count =
        ((static_cast<std::uint64_t>(output_sample_bits) >> 3)
         * output_info.pipeline_audio_block_size
         * output_info.output_block_count
         * channel_count);
    return 0x100000000ULL | (byte_count & 0xFFFFFFFFULL);
}

std::size_t a3deng_push_part_byte_count(
    std::uint32_t input_block_count,
    std::uint32_t input_channel_mask,
    std::uint32_t input_sample_bits) {
    const std::uint32_t channel_count =
        auro3deng::auro_channel_Mask_count(input_channel_mask & 0x7FFFFFFu, 0, 0);
    return (static_cast<std::size_t>(input_block_count) * channel_count * input_sample_bits) >> 3;
}

std::uint32_t derive_listening_mode_aurodeco(
    std::uint32_t virtualizer_mode,
    bool headphone_connected,
    bool stereo_device_connected) {
    // AuroDecoderImpl::UpdateConfiguration / Initialize:
    // v9 = 2 * (virtualizer_mode != 0);
    // if (!virtualizer_mode && !headphone_connected)
    //     v9 = 2 * (stereo_device_connected == 0);
    std::uint32_t listening_mode = 2u * static_cast<std::uint32_t>(virtualizer_mode != 0);
    if (virtualizer_mode == 0u && !headphone_connected)
        listening_mode = 2u * static_cast<std::uint32_t>(!stereo_device_connected);
    return listening_mode;
}

std::uint32_t derive_effective_virtualizer_mode_aurodeco(
    std::uint32_t requested_virtualizer_mode,
    std::uint16_t output_audio_configuration_bits) {
    // AuroDecoderImpl::Initialize / UpdateConfiguration:
    // if ((bits & 0x101) == 0) effective virtualizer becomes 1 ("Disabled"),
    // otherwise it keeps the requested mode.
    if ((output_audio_configuration_bits & 0x101u) == 0)
        return 1u;
    return requested_virtualizer_mode;
}

std::uint32_t derive_effective_virtualizer_mode_a3deng(
    bool dynamic_request_flag,
    bool stereo_device_connected) {
    // A3DENG::update dynamic params:
    // actual_virtualization_mode = !stereo_device || !dynamic_request_flag.
    return (!stereo_device_connected || !dynamic_request_flag) ? 1u : 0u;
}

std::uint32_t derive_listening_mode_a3deng(
    bool stereo_device_connected,
    bool headphone_dynamic_enabled) {
    // A3DENG::update dynamic params:
    // listening_mode = 3 * (!stereo_device || !dynamic_headphone_flag).
    return (!stereo_device_connected || !headphone_dynamic_enabled) ? 3u : 0u;
}

std::uint32_t derive_target_device_a3deng(
    std::uint32_t decoder_mode,
    bool stereo_device_connected,
    bool headphone_connected) {
    // A3DENG::update config path:
    // format-detector mode keeps target_device=0; non-stereo routes to 5;
    // stereo routes to 1 or 3 depending on headset_connected.
    if (decoder_mode == 2u)
        return 0u;
    if (!stereo_device_connected)
        return 5u;
    return 2u * static_cast<std::uint32_t>(headphone_connected) + 1u;
}

void format_detector_sink_notify_1056c0_bridge(void* user, std::int64_t kind) {
    auto* dispatch = reinterpret_cast<auro3deng::CodecV3DispatchStateEb5a0*>(user);
    if (!dispatch)
        return;
    // IDA FormatDetector sink: 0 = unlock, 1 = lock (sub_52CED0).
    auro3deng::codec_v3_sync_callback_eb840(dispatch, static_cast<int>(kind));
}

void sync_detector_notify_105ee0_bridge(void* ctx, std::int64_t kind, std::uint64_t, std::uint64_t) {
    auto* dispatch = reinterpret_cast<auro3deng::CodecV3DispatchStateEb5a0*>(ctx);
    if (!dispatch || kind == 1)
        return;
    auro3deng::codec_v3_sync_callback_eb840(dispatch, kind == 0 ? 1 : 0);
}

void codec_v3_frame_deque_pop_front_keep_frame_13d670_bridge(std::uint64_t frame_deque_ptr) {
    // Path where mark_as_unused is done by caller (e.g. IDA 0x103063/0x10306F).
    (void)auro3deng::frame_deque_pop_front_keep_frame_partial(frame_deque_ptr);
}

struct CodecV3ParserBridgeCtx {
    std::uint8_t* ready_parse_result_base = nullptr;
    std::size_t ready_parse_result_size = 0;
};

struct ParserRebindBridgeCtx {
    std::uint64_t parse_result_pool_base = 0;
    std::uint32_t* channel_words_base = nullptr;
    std::size_t channel_words_count = 0;
    std::uint8_t* channel_ctx_base = nullptr;
    std::size_t channel_ctx_count = 0;
    std::uint8_t* channel_parser_base = nullptr;
    std::size_t channel_parser_count = 0;
};

struct DecoderStepBridgeCtx {
    auro3deng::CodecV3DispatchStateEb5a0* dispatch = nullptr;
    auro3deng::DelayLineState106b40* delay_line = nullptr;
    std::uint8_t* output_generator_base = nullptr;
    std::uint8_t* output_table_base = nullptr;
    std::size_t output_table_size = 0;
    const std::uint64_t* output_channel_ptrs_27 = nullptr;
    const std::uint64_t* input_channel_ptrs_27 = nullptr;
    std::uint32_t input_mask = 0;
    std::uint8_t* fake_frame_deque_base = nullptr;
    std::uint8_t* ready_frame_deque_base = nullptr;
    std::uint8_t* parse_result_pool_base = nullptr;
    std::uint8_t* parser_slots_base = nullptr;
    std::size_t parser_slots_size = 0;
    std::uint8_t* ready_parse_result_base = nullptr;
    std::size_t ready_parse_result_size = 0;
    std::uint64_t block_size = 0;
    std::uint64_t* parser_timeline_cursor_ptr = nullptr;
    std::uint64_t* og_timeline_cursor_ptr = nullptr;
    std::uint64_t output_timeline_delay = 0;
    std::uint32_t* parser_state_ptr = nullptr;
    std::uint32_t* produced_output_mask = nullptr;
};

struct HostOgDecideDecodeCtx {
    auro3deng::CodecV3DispatchStateEb5a0* dispatch = nullptr;
    std::uint8_t* output_generator_base = nullptr;
    std::uint32_t output_layout = 0;
    std::uint32_t target_layout = 0;
};

std::int64_t host_og_pre_segments_decide_decode_bridge(
    std::uint64_t ctx,
    std::uint64_t ranges_base,
    std::uint64_t started_base) {
    auto* host = reinterpret_cast<HostOgDecideDecodeCtx*>(ctx);
    if (!host || !host->output_generator_base || started_base == 0u || ranges_base == 0u)
        return 0;
    const std::uint64_t seg_ctx_ptr = *reinterpret_cast<const std::uint64_t*>(
        host->output_generator_base + kCodecV3OgOffSegmentCtxPtr);
    if (seg_ctx_ptr == 0u)
        return 0;
    const std::uint32_t count = *reinterpret_cast<const std::uint32_t*>(
        reinterpret_cast<const std::uint8_t*>(seg_ctx_ptr) + kCodecV3SegCtxOffCount);
    const std::uint32_t allow =
        (host->dispatch == nullptr || host->dispatch->decide_decode_gate != 0u) ? 1u : 0u;
    return auro3deng::codec_v3_pre_segments_decide_decode_layouts_partial(
        count,
        ranges_base,
        started_base,
        host->output_layout,
        host->target_layout,
        allow,
        host->dispatch);
}

thread_local CodecV3ParserBridgeCtx* g_codec_v3_parser_bridge_ctx = nullptr;

struct SyncDetectorNotifyBridgeCtx {
    auro3deng::CodecV3DispatchStateEb5a0* dispatch = nullptr;
    auro3deng::FormatDetectorState1056c0* format_detector = nullptr;
    auro3deng::SyncDetectorState105ee0* sync_detector = nullptr;
    std::uint64_t frame_deque_ptr = 0;
};

void sync_detector_notify_frame_builder_105530_bridge(
    void* raw_ctx,
    std::int64_t kind,
    std::uint64_t a,
    std::uint64_t b) {
    auto* ctx = reinterpret_cast<SyncDetectorNotifyBridgeCtx*>(raw_ctx);
    if (!ctx || !ctx->dispatch)
        return;

    if (kind == 2 && ctx->format_detector && ctx->frame_deque_ptr != 0u) {
        (void)auro3deng::frame_deque_pop_back_5303f0_partial(
            ctx->frame_deque_ptr,
            auro3deng::frame_mark_as_unused_106cd0_default_partial);
        // IDA sub_52CED0/sub_105530 kind==2: clear sync_state only (not expected_frame_end).
        if (ctx->format_detector->sync_state != 0u) {
            ctx->format_detector->sync_state = 0;
            auro3deng::codec_v3_sync_callback_eb840(ctx->dispatch, 0);
        }
        return;
    }

    if (kind == 0 && ctx->format_detector && ctx->sync_detector && ctx->frame_deque_ptr != 0u) {
        std::array<std::uint8_t, kCodecV3FrameDequeSlotCopyBytes> frame{};
        // IDA sub_105530: v5 = processed_samples + (int)a3 — signed add with uint64 wrap.
        const std::int32_t rel_start = static_cast<std::int32_t>(a);
        const std::uint64_t frame_start =
            ctx->format_detector->processed_samples + static_cast<std::uint64_t>(static_cast<std::int64_t>(rel_start));
        const std::uint32_t span = static_cast<std::uint32_t>(b != 0u ? b : kCodecV3FrameDequeSlotCopyBytes);
        const std::uint32_t layout_word = ctx->format_detector->layout & kCodecV3ChannelMask;
        const std::uint32_t sub_52ced0_and =
            (ctx->format_detector->allow_low_9bits != 0u)
                ? static_cast<std::uint32_t>(auro_codec_v3_ida::kFormatDetector52ced0_frame_mask_and_when_allow_nonzero)
                : static_cast<std::uint32_t>(static_cast<std::int32_t>(
                    auro_codec_v3_ida::kFormatDetector52ced0_frame_mask_and_when_allow_zero));
        const std::uint32_t active_mask = layout_word & sub_52ced0_and;

        auro3deng::frame_construct_106ba0_partial(
            reinterpret_cast<std::uint64_t>(frame.data()),
            frame_start,
            span,
            layout_word,
            active_mask);

        // IDA: construct always, then push only when common_header is valid.
        const std::uint32_t* common_header =
            auro3deng::sync_detector_get_common_header_105ee0_partial(ctx->sync_detector);
        if (!common_header)
            return;

        std::memcpy(frame.data() + 28u, common_header, 16u);

        const std::uint32_t slot_count = *reinterpret_cast<const std::uint32_t*>(
            frame.data() + kCodecV3FrameOffChannelCount);
        for (std::uint32_t si = 0; si < slot_count; ++si) {
            auto* slot = frame.data() + kCodecV3FrameOffSlotBase
                + static_cast<std::size_t>(si) * kCodecV3FrameSlotStride;
            if (*reinterpret_cast<const std::uint32_t*>(slot + kCodecV3FrameSlotOffActiveFlag) == 0u)
                continue;
            const std::uint32_t ch =
                *reinterpret_cast<const std::uint32_t*>(slot + kCodecV3FrameSlotOffChannelIndex);
            const auto* header =
                auro3deng::sync_detector_find_channel_header_105ee0_partial(ctx->sync_detector, ch);
            if (!header)
                return;
            // IDA: get_channel_header returns &aligned_word; qword@+0 -> slot+8, dword@+8 -> slot+16.
            *reinterpret_cast<std::uint64_t*>(slot + 8u) =
                (static_cast<std::uint64_t>(header->aligned_bit) << 32)
                | static_cast<std::uint64_t>(header->aligned_word);
            *reinterpret_cast<std::uint32_t*>(slot + 16u) = header->aligned_code;
        }

        const std::int64_t push_rc = auro3deng::frame_deque_push_back_13d5d0_partial(
            ctx->frame_deque_ptr,
            reinterpret_cast<std::uint64_t>(frame.data()),
            kCodecV3FrameDequeSlotCopyBytes);
        if (push_rc == 0)
            return;
        ctx->format_detector->expected_frame_end =
            frame_start + static_cast<std::uint64_t>(span);
        const bool notify_sync = ctx->format_detector->sync_state == 0u;
        if (notify_sync)
            ctx->format_detector->sync_state = 1u;
        if (notify_sync)
            auro3deng::codec_v3_sync_callback_eb840(ctx->dispatch, 1);
        return;
    }

    if (kind != 1)
        auro3deng::codec_v3_sync_callback_eb840(ctx->dispatch, kind == 0 ? 1 : 0);
}

std::uint64_t codec_v3_delay_line_get_buffer_bridge(
    std::uint64_t delay_line_ptr,
    std::uint64_t timeline_cursor,
    std::int64_t* io_state);

std::int64_t parser_ready_frame_push_copy_13d5d0_bridge(std::uint64_t frame_deque_ptr, std::uint64_t frame_ptr) {
    auto* ctx = g_codec_v3_parser_bridge_ctx;
    auro3deng::ParserReadyFrameCopyContext copy_ctx{};
    if (ctx) {
        copy_ctx.ready_parse_result_base = ctx->ready_parse_result_base;
        copy_ctx.ready_parse_result_size = ctx->ready_parse_result_size;
    }
    copy_ctx.copied_slot_capacity = kCodecV3FrameDequeCopiedSlotCapacity;
    copy_ctx.parse_result_bytes = kCodecV3ParseResultBytes;
    return auro3deng::parser_ready_frame_push_copy_partial(
        frame_deque_ptr,
        frame_ptr,
        kCodecV3FrameDequeSlotCopyBytes,
        &copy_ctx);
}

std::uint64_t frame_deque_pop_front_13d670_bridge(std::uint64_t frame_deque_ptr) {
    (void)auro3deng::frame_deque_pop_front_13d670_partial(
        frame_deque_ptr,
        auro3deng::frame_mark_as_unused_106cd0_default_partial);
    return 0;
}

void parser_state_sink_notify_bridge(std::uint64_t ctx, std::uint32_t state) {
    auto* sink = reinterpret_cast<auro3deng::ParserStateSinkContext*>(ctx);
    auro3deng::parser_state_sink_notify_partial(sink, state);
}

void parser_rebind_frame_parse_results_103610_bridge(void* user, std::uint64_t frame_ptr) {
    auto* ctx = reinterpret_cast<ParserRebindBridgeCtx*>(user);
    if (!ctx)
        return;
    auro3deng::ParserRebindContext103610 rebind_ctx{};
    rebind_ctx.parse_result_pool_base = ctx->parse_result_pool_base;
    rebind_ctx.channel_words_base = ctx->channel_words_base;
    rebind_ctx.channel_words_count = ctx->channel_words_count;
    rebind_ctx.channel_ctx_base = ctx->channel_ctx_base;
    rebind_ctx.channel_ctx_count = ctx->channel_ctx_count;
    rebind_ctx.channel_parser_base = ctx->channel_parser_base;
    rebind_ctx.channel_parser_count = ctx->channel_parser_count;
    auro3deng::parser_rebind_frame_parse_results_103610_partial(frame_ptr, &rebind_ctx);
}

void run_parser_stage_1034e0_bridge(void* user) {
    auto* step = reinterpret_cast<DecoderStepBridgeCtx*>(user);
    if (!step)
        return;
    CodecV3ParserBridgeCtx parser_bridge_ctx{};
    parser_bridge_ctx.ready_parse_result_base = step->ready_parse_result_base;
    parser_bridge_ctx.ready_parse_result_size = step->ready_parse_result_size;

    auro3deng::DecoderParserStageContext1034e0 ctx{};
    ctx.dispatch = step->dispatch;
    ctx.delay_line = step->delay_line;
    ctx.output_generator_base = step->output_generator_base;
    ctx.frame_deque_base = step->fake_frame_deque_base;
    ctx.ready_frame_deque_base = step->ready_frame_deque_base;
    ctx.parse_result_pool_base = step->parse_result_pool_base;
    ctx.parser_slots_base = step->parser_slots_base;
    ctx.parser_slots_size = step->parser_slots_size;
    ctx.ready_parse_result_base = step->ready_parse_result_base;
    ctx.ready_parse_result_size = step->ready_parse_result_size;
    ctx.block_size = step->block_size;
    ctx.timeline_cursor_ptr = step->parser_timeline_cursor_ptr;
    ctx.parser_state_ptr = step->parser_state_ptr;
    ctx.runtime_fns.delay_line_get_buffer = auro3deng::delay_line_get_buffer_u64_state_partial;
    ctx.runtime_fns.frame_deque_find_first_with_end_after = auro3deng::frame_deque_find_first_with_end_after_13d570_partial;
    ctx.runtime_fns.frame_deque_push_back = parser_ready_frame_push_copy_13d5d0_bridge;
    ctx.runtime_fns.frame_deque_pop_front = frame_deque_pop_front_13d670_bridge;
    ctx.runtime_fns.frame_mark_as_unused = auro3deng::parser_frame_mark_as_unused_cb_partial;

    g_codec_v3_parser_bridge_ctx = &parser_bridge_ctx;
    (void)auro3deng::decoder_run_parser_stage_1034e0_partial(&ctx);
    g_codec_v3_parser_bridge_ctx = nullptr;
}

std::int64_t run_output_stage_1024a9_bridge(void* user) {
    auto* step = reinterpret_cast<DecoderStepBridgeCtx*>(user);
    if (!step || !step->output_generator_base || !step->output_table_base || !step->output_channel_ptrs_27)
        return 0;
    // IDA keeps OG timeline independent of parser (stage1 vs stage0 latency).
    // Seed OG object from host OG cursor; do not overwrite from parser cursor.
    if (step->og_timeline_cursor_ptr) {
        if (step->parser_timeline_cursor_ptr && step->output_timeline_delay != 0u) {
            const std::uint64_t parser_cursor = *step->parser_timeline_cursor_ptr;
            *step->og_timeline_cursor_ptr =
                parser_cursor > step->output_timeline_delay
                    ? (parser_cursor - step->output_timeline_delay)
                    : 0u;
        }
        *reinterpret_cast<std::uint64_t*>(
            step->output_generator_base + kCodecV3OgOffTimelineCursor) =
            *step->og_timeline_cursor_ptr;
    }

    auro3deng::OutputGeneratorRuntimeFns1024a9 fns{};
    fns.delay_line_get_channel = auro3deng::delay_line_get_channel_from_buffer_106ab0;
    fns.delay_line_get_buffer = codec_v3_delay_line_get_buffer_bridge;
    fns.frame_deque_find_first_with_end_after = auro3deng::frame_deque_find_first_with_end_after_13d570_partial;
    fns.frame_mark_as_unused = auro3deng::frame_mark_as_unused_106cd0_default_partial;
    fns.frame_deque_pop_front = codec_v3_frame_deque_pop_front_keep_frame_13d670_bridge;
    auro3deng::DecoderOutputStageContext1024a9 ctx{};
    ctx.delay_line = step->delay_line;
    ctx.output_generator_base = step->output_generator_base;
    ctx.output_table_base = step->output_table_base;
    ctx.output_table_size = step->output_table_size;
    ctx.output_channel_ptrs_27 = step->output_channel_ptrs_27;
    ctx.input_channel_ptrs_27 = step->input_channel_ptrs_27;
    ctx.input_mask = step->input_mask;
    ctx.ready_frame_deque_base = step->ready_frame_deque_base;
    ctx.total_samples = step->block_size;
    ctx.produced_output_mask = step->produced_output_mask;
    const std::int64_t rc = auro3deng::decoder_run_output_stage_1024a9_partial(&ctx, &fns);
    if (step->og_timeline_cursor_ptr) {
        *step->og_timeline_cursor_ptr = *reinterpret_cast<const std::uint64_t*>(
            step->output_generator_base + kCodecV3OgOffTimelineCursor);
    }
    return rc;
}

std::uint64_t codec_v3_delay_line_get_buffer_bridge(
    std::uint64_t delay_line_ptr,
    std::uint64_t timeline_cursor,
    std::int64_t* io_state) {
    auto* state = reinterpret_cast<auro3deng::DelayLineState106b40*>(delay_line_ptr);
    return auro3deng::delay_line_get_buffer_106b40(state, timeline_cursor, io_state);
}

void rebuild_native_dynamic_parameters_state_from_runtime(
    const auro3d::NativeRuntimeConfigurationState& runtime,
    auro3d::NativeDynamicParametersState* dynamic_state) {
    if (dynamic_state == nullptr)
        return;
    dynamic_state->effective_virtualizer_mode = runtime.effective_virtualizer_mode;
    dynamic_state->listening_mode = runtime.derived_listening_mode;
    dynamic_state->room_preset = runtime.room_preset;
    dynamic_state->hrtf_preset = runtime.hrtf_preset;
}

bool native_dynamic_parameters_equal(
    const auro3d::NativeDynamicParametersState& lhs,
    const auro3d::NativeDynamicParametersState& rhs) {
    return lhs.effective_virtualizer_mode == rhs.effective_virtualizer_mode
        && lhs.listening_mode == rhs.listening_mode
        && lhs.room_preset == rhs.room_preset
        && lhs.hrtf_preset == rhs.hrtf_preset;
}

inline std::int16_t i32_sample_to_s16(std::int32_t v, float gain, std::uint64_t* clip_counter) {
    float x = static_cast<float>(v) * gain * (1.0f / 256.0f);
    if (x > 32767.0f) {
        x = 32767.0f;
        if (clip_counter)
            *clip_counter += 1;
    } else if (x < -32768.0f) {
        x = -32768.0f;
        if (clip_counter)
            *clip_counter += 1;
    }
    return static_cast<std::int16_t>(x);
}

inline std::int32_t i32_sample_to_s24(std::int32_t v, float gain, std::uint64_t* clip_counter) {
    float x = static_cast<float>(v) * gain;
    if (x > 8388607.0f) {
        x = 8388607.0f;
        if (clip_counter)
            *clip_counter += 1;
    } else if (x < -8388608.0f) {
        x = -8388608.0f;
        if (clip_counter)
            *clip_counter += 1;
    }
    return static_cast<std::int32_t>(x);
}

struct NativeChannelLayoutPlan {
    std::uint32_t mask = 0;
    unsigned slot_count = 0;
    std::array<std::uint32_t, auro3d::kCurrentNativeExportChannelLimit> slots{};
};

void push_layout_slot(NativeChannelLayoutPlan& plan, std::uint32_t slot) {
    if (slot >= 27u || plan.slot_count >= plan.slots.size())
        return;
    if (((plan.mask >> slot) & 1u) != 0)
        return;
    plan.slots[plan.slot_count++] = slot;
    plan.mask |= (1u << slot);
}

void push_layout_slots_from_mask(
    NativeChannelLayoutPlan& plan,
    std::uint32_t mask,
    unsigned channel_count,
    std::uint32_t channel_mapping) {
    // IDA sub_31ACE0: mapping 1 uses the static table initialized from
    // xmmword_1DB2B0/1DB2C0; other mappings iterate channel ids 0..30.
    static constexpr std::uint32_t kBacksBeforeSurroundsOrder[] = {
        auro_codec_v3_ida::kAuroChMapSlotFrontLeft,
        auro_codec_v3_ida::kAuroChMapSlotFrontRight,
        auro_codec_v3_ida::kAuroChMapSlotFrontCenter,
        auro_codec_v3_ida::kAuroChMapSlotLfe,
        auro_codec_v3_ida::kAuroChMapSlotBackLeft,
        auro_codec_v3_ida::kAuroChMapSlotBackRight,
        auro_codec_v3_ida::kAuroChMapSlotSideLeft,
        auro_codec_v3_ida::kAuroChMapSlotSideRight,
    };

    if (channel_mapping == auro_codec_v3_ida::kAuroChannelMappingBacksBeforeSurrounds) {
        for (std::uint32_t slot : kBacksBeforeSurroundsOrder) {
            if (plan.slot_count >= channel_count)
                return;
            if (((mask >> slot) & 1u) != 0)
                push_layout_slot(plan, slot);
        }
        return;
    }

    for (std::uint32_t slot = 0; slot < auro_codec_v3_ida::kAuroProcessorIoChannelPtrCount
         && plan.slot_count < channel_count; ++slot) {
        if (((mask >> slot) & 1u) != 0)
            push_layout_slot(plan, slot);
    }
}

NativeChannelLayoutPlan build_native_channel_layout(unsigned channel_count) {
    NativeChannelLayoutPlan plan{};
    switch (channel_count) {
    case 0:
        break;
    case 1:
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotFrontLeft);
        break;
    case 2:
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotFrontLeft);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotFrontRight);
        break;
    case 3:
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotFrontLeft);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotFrontRight);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotLfe);
        break;
    case 4:
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotFrontLeft);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotFrontRight);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotSideLeft);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotSideRight);
        break;
    case 5:
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotFrontLeft);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotFrontRight);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotLfe);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotSideLeft);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotSideRight);
        break;
    case 6:
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotFrontLeft);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotFrontRight);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotFrontCenter);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotLfe);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotSideLeft);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotSideRight);
        break;
    case 7:
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotFrontLeft);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotFrontRight);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotFrontCenter);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotLfe);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotSideLeft);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotSideRight);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotBackCenter);
        break;
    case 9:
        push_layout_slots_from_mask(
            plan,
            26167u,
            channel_count,
            auro_codec_v3_ida::kAuroChannelMappingDefault);
        break;
    case 10:
        push_layout_slots_from_mask(
            plan,
            26175u,
            channel_count,
            auro_codec_v3_ida::kAuroChannelMappingDefault);
        break;
    case 11:
        push_layout_slots_from_mask(
            plan,
            30271u,
            channel_count,
            auro_codec_v3_ida::kAuroChannelMappingDefault);
        break;
    case 12:
        push_layout_slots_from_mask(
            plan,
            26559u,
            channel_count,
            auro_codec_v3_ida::kAuroChannelMappingDefault);
        break;
    case 13:
        push_layout_slots_from_mask(
            plan,
            30655u,
            channel_count,
            auro_codec_v3_ida::kAuroChannelMappingDefault);
        break;
    case 14:
        push_layout_slots_from_mask(
            plan,
            32703u,
            channel_count,
            auro_codec_v3_ida::kAuroChannelMappingDefault);
        break;
    default:
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotFrontLeft);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotFrontRight);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotFrontCenter);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotLfe);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotSideLeft);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotSideRight);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotBackLeft);
        push_layout_slot(plan, auro_codec_v3_ida::kAuroChMapSlotBackRight);
        break;
    }

    push_layout_slots_from_mask(
        plan,
        0x7FFFFFFu,
        channel_count,
        auro_codec_v3_ida::kAuroChannelMappingDefault);
    return plan;
}

NativeChannelLayoutPlan build_native_channel_layout_from_mask(std::uint32_t mask, unsigned channel_count) {
    NativeChannelLayoutPlan plan{};
    push_layout_slots_from_mask(
        plan,
        mask & 0x7FFFFFFu,
        channel_count,
        auro_codec_v3_ida::kAuroChannelMappingDefault);
    return plan;
}

bool wav_speaker_bit_to_auro_slot(unsigned bit, std::uint32_t& slot) {
    using namespace auro_codec_v3_ida;
    switch (bit) {
    case 0:  slot = kAuroChMapSlotFrontLeft; return true;
    case 1:  slot = kAuroChMapSlotFrontRight; return true;
    case 2:  slot = kAuroChMapSlotFrontCenter; return true;
    case 3:  slot = kAuroChMapSlotLfe; return true;
    case 4:  slot = kAuroChMapSlotBackLeft; return true;
    case 5:  slot = kAuroChMapSlotBackRight; return true;
    case 6:  slot = kAuroChannelIdLeftCenter; return true;
    case 7:  slot = kAuroChannelIdRightCenter; return true;
    case 8:  slot = kAuroChMapSlotBackCenter; return true;
    case 9:  slot = kAuroChMapSlotSideLeft; return true;
    case 10: slot = kAuroChMapSlotSideRight; return true;
    case 11: slot = kAuroChannelIdTop; return true;
    case 12: slot = kAuroChannelIdHeightLeft; return true;
    case 13: slot = kAuroChannelIdHeightCenter; return true;
    case 14: slot = kAuroChannelIdHeightRight; return true;
    case 15: slot = kAuroChannelIdHeightLeftBack; return true;
    case 16: slot = kAuroChannelIdHeightCenterSurround; return true;
    case 17: slot = kAuroChannelIdHeightRightBack; return true;
    default:
        return false;
    }
}

NativeChannelLayoutPlan build_native_channel_layout_from_wav_mask(
    std::uint32_t wav_channel_mask,
    unsigned channel_count) {
    NativeChannelLayoutPlan plan{};
    if (wav_channel_mask == 0)
        return plan;

    for (unsigned bit = 0; bit < 32u && plan.slot_count < channel_count; ++bit) {
        if (((wav_channel_mask >> bit) & 1u) == 0)
            continue;
        std::uint32_t slot = 0;
        if (!wav_speaker_bit_to_auro_slot(bit, slot))
            continue;
        push_layout_slot(plan, slot);
    }
    if (plan.slot_count != channel_count)
        return {};
    return plan;
}

NativeChannelLayoutPlan build_native_input_channel_layout(
    unsigned channel_count,
    std::uint32_t wav_channel_mask) {
    NativeChannelLayoutPlan plan = build_native_channel_layout_from_wav_mask(wav_channel_mask, channel_count);
    if (plan.slot_count == channel_count)
        return plan;
    return build_native_channel_layout(channel_count);
}

NativeChannelLayoutPlan build_native_input_channel_layout(
    unsigned channel_count,
    std::uint32_t wav_channel_mask,
    const auro3d::AuroMetadataInfo& metadata) {
    // Input WAV is the carrier (or carrier-sized) PCM container. Height slots are
    // produced by codec-v3 decode into the requested output layout, not present as
    // already-decoded PCM planes in the file.
    if (metadata.found
        && metadata.carrier_layout_id != 0u
        && metadata.carrier_channels == channel_count) {
        return build_native_channel_layout_from_mask(metadata.carrier_layout_id, channel_count);
    }
    if (metadata.found
        && metadata.layout_id != 0u
        && metadata.output_channels == channel_count) {
        return build_native_channel_layout_from_mask(metadata.layout_id, channel_count);
    }
    for (const auto& ins : metadata.adol_instructions) {
        if (!ins.has_carrier_layout)
            continue;
        if (mask_count_27(ins.carrier_layout) != channel_count)
            continue;
        return build_native_channel_layout_from_mask(ins.carrier_layout, channel_count);
    }
    return build_native_input_channel_layout(channel_count, wav_channel_mask);
}

NativeChannelLayoutPlan build_requested_native_channel_layout(
    unsigned channel_count,
    const auro3d::AuroMetadataInfo& metadata) {
    constexpr std::uint32_t kAuroLayout7_1_5H_1T = 0x7FBFu;
    constexpr std::uint32_t kAuroCarrier7_1 = 0x01BFu;
    constexpr std::uint32_t kAuroDirect7_1_2H = 0x07BFu;
    if (metadata.found
        && metadata.layout_id == kAuroLayout7_1_5H_1T
        && metadata.carrier_layout_id == kAuroCarrier7_1
        && channel_count == mask_count_27(kAuroDirect7_1_2H)) {
        return build_native_channel_layout_from_mask(kAuroDirect7_1_2H, channel_count);
    }
    if (metadata.found && metadata.layout_id != 0 && metadata.output_channels == channel_count)
        return build_native_channel_layout_from_mask(metadata.layout_id, channel_count);
    return build_native_channel_layout(channel_count);
}

struct A3DSyncInfo {
    unsigned block_size = 0;
    unsigned m = 0;
};

struct MuxIteratorFalse {
    const std::vector<std::int32_t>* samples = nullptr;
    std::size_t base = 0;
    unsigned m = 0;
    std::uint64_t pos = 0;
    unsigned param = 0;
};

struct MuxIteratorTrue {
    const std::vector<std::int32_t>* samples = nullptr;
    std::size_t base = 0;
    unsigned m = 0;
    std::uint64_t pos = 0;
};

struct AdolInstruction {
    std::uint32_t block_index = 0;
    std::uint32_t tag = 0;
    unsigned opcode = 0;
    bool has_value = false;
    std::uint32_t value = 0;
    unsigned value_bits = 0;
    bool has_value2 = false;
    std::uint32_t value2 = 0;
    unsigned value2_bits = 0;
};

std::uint8_t pcm24_byte(std::int32_t x, unsigned n) {
    return static_cast<std::uint8_t>((static_cast<std::uint32_t>(x) >> (8u * n)) & 0xFFu);
}

std::uint16_t crc16_ccitt_update(std::uint16_t crc, std::uint8_t b) {
    crc = static_cast<std::uint16_t>(crc ^ (static_cast<std::uint16_t>(b) << 8u));
    for (unsigned i = 0; i < 8u; ++i) {
        if ((crc & 0x8000u) != 0)
            crc = static_cast<std::uint16_t>((crc << 1u) ^ 0x1021u);
        else
            crc = static_cast<std::uint16_t>(crc << 1u);
    }
    return crc;
}

std::uint16_t compute_crc16_ccitt_over_pcm24_at(
    const std::vector<std::int32_t>& samples,
    std::size_t start,
    std::size_t length) {
    std::uint16_t crc = 0;
    const std::size_t first = std::min<std::size_t>(length, 16u);
    for (std::size_t i = 0; i < first; ++i) {
        const std::int32_t w = samples[start + i];
        crc = crc16_ccitt_update(crc, static_cast<std::uint8_t>(pcm24_byte(w, 0) & 0xFDu));
        crc = crc16_ccitt_update(crc, pcm24_byte(w, 1));
        crc = crc16_ccitt_update(crc, pcm24_byte(w, 2));
    }
    for (std::size_t i = first; i < length; ++i) {
        const std::int32_t w = samples[start + i];
        crc = crc16_ccitt_update(crc, pcm24_byte(w, 0));
        crc = crc16_ccitt_update(crc, pcm24_byte(w, 1));
        crc = crc16_ccitt_update(crc, pcm24_byte(w, 2));
    }
    return static_cast<std::uint16_t>(~crc);
}

unsigned mux_bit_from_pos_true(const MuxIteratorTrue& it) {
    const std::uint64_t v = it.pos;
    const std::uint64_t m = it.m;
    std::uint64_t idx = 0;
    std::uint64_t rem = 0;
    std::uint32_t mask = 0;
    if (v > 0x2Full) {
        if (v >= 16ull * m) {
            idx = v / m;
            rem = v % m;
        } else {
            const std::uint64_t x = v - 48ull;
            idx = x / (m - 3ull);
            rem = x % (m - 3ull);
        }
        mask = 1u << static_cast<unsigned>(m - 1ull - rem);
    } else if (v >= 0x10ull) {
        if (v <= 0x1Full) {
            idx = v - 16ull;
            mask = 2u;
        } else {
            idx = v - 32ull;
            mask = 4u;
        }
    } else {
        idx = v;
        mask = 1u;
    }
    if (!it.samples || it.base + idx >= it.samples->size())
        return 0;
    return ((static_cast<std::uint32_t>((*it.samples)[it.base + static_cast<std::size_t>(idx)]) & mask) != 0) ? 1u : 0u;
}

unsigned mux_bit_from_pos_false(const MuxIteratorFalse& it) {
    const std::uint64_t v = it.pos;
    const std::uint64_t m = it.m;
    const std::uint64_t p = it.param;
    std::uint64_t idx = 0;
    std::uint64_t rem = 0;
    std::uint32_t mask = 0;
    if (v > 0x2Full) {
        if (v >= 16ull * m) {
            const std::uint64_t x = (v - m) / p + v;
            idx = x / m;
            rem = x % m;
        } else {
            const std::uint64_t x = v - 48ull;
            idx = x / (m - 3ull);
            rem = x % (m - 3ull);
        }
        mask = 1u << static_cast<unsigned>(m - 1ull - rem);
    } else if (v >= 0x10ull) {
        if (v <= 0x1Full) {
            idx = v - 16ull;
            mask = 2u;
        } else {
            idx = v - 32ull;
            mask = 4u;
        }
    } else {
        idx = v;
        mask = 1u;
    }
    if (!it.samples || it.base + idx >= it.samples->size())
        return 0;
    return ((static_cast<std::uint32_t>((*it.samples)[it.base + static_cast<std::size_t>(idx)]) & mask) != 0) ? 1u : 0u;
}

bool read_unsigned_true(unsigned nbits, MuxIteratorTrue& it, std::uint64_t& bits_left, std::uint64_t& out) {
    if (bits_left < nbits)
        return false;
    bits_left -= nbits;
    out = 0;
    for (unsigned i = 0; i < nbits; ++i) {
        out = (out << 1u) | mux_bit_from_pos_true(it);
        ++it.pos;
    }
    return true;
}

bool read_unsigned_false(unsigned nbits, MuxIteratorFalse& it, std::uint64_t& bits_left, std::uint32_t& out) {
    if (bits_left < nbits)
        return false;
    bits_left -= nbits;
    out = 0;
    for (unsigned i = 0; i < nbits; ++i) {
        out = (out << 1u) | mux_bit_from_pos_false(it);
        ++it.pos;
    }
    return true;
}

bool read_bool_false(MuxIteratorFalse& it, std::uint64_t& bits_left) {
    std::uint32_t v = 0;
    return read_unsigned_false(1u, it, bits_left, v);
}

bool parse_sync_pcm24_at(
    const std::vector<std::int32_t>& samples,
    std::size_t start,
    std::size_t avail_len,
    bool strict_len,
    A3DSyncInfo& out) {
    out = {};
    if (avail_len < 16u || start + avail_len > samples.size())
        return false;
    for (unsigned i = 0; i < 16u; ++i) {
        if ((static_cast<std::uint32_t>(samples[start + i]) & 1u) == 0)
            return false;
    }
    const std::uint32_t s0 = static_cast<std::uint32_t>(samples[start + 0]);
    const std::uint32_t s1 = static_cast<std::uint32_t>(samples[start + 1]);
    const std::uint32_t s2 = static_cast<std::uint32_t>(samples[start + 2]);
    const std::uint32_t s3 = static_cast<std::uint32_t>(samples[start + 3]);
    const std::uint32_t s4 = static_cast<std::uint32_t>(samples[start + 4]);
    const std::uint32_t s5 = static_cast<std::uint32_t>(samples[start + 5]);
    const std::uint32_t s6 = static_cast<std::uint32_t>(samples[start + 6]);
    const std::uint32_t s7 = static_cast<std::uint32_t>(samples[start + 7]);
    const std::uint32_t a = (s0 & 4u) | ((s1 >> 1u) & 3u);
    const std::uint32_t b = ((4u * a) & 0xF8u) | (s2 & 4u) | ((s3 >> 1u) & 3u);
    const std::uint32_t c = ((4u * b) & 0x78u) | (s4 & 4u) | ((s5 >> 1u) & 2u) | (((s6 & 4u) != 0) ? 1u : 0u);
    const std::uint32_t block_raw = ((4u * s7) & 0x10u) | (32u * c);
    const unsigned block_size = (block_raw == 0x3D0u) ? 1000u : static_cast<unsigned>(block_raw + 16u);

    MuxIteratorTrue it{&samples, start, 3u, 44u};
    std::uint64_t bits_left = 8u;
    std::uint64_t v30 = 0;
    if (!read_unsigned_true(4u, it, bits_left, v30) || (v30 + 13u) >= 0x19u)
        return false;
    const unsigned m = 14u - static_cast<unsigned>(v30);
    if (m < 3u)
        return false;
    if (strict_len) {
        if (avail_len != block_size)
            return false;
    } else if (avail_len < block_size) {
        return false;
    }

    const std::uint64_t v19 = 16ull * m;
    const std::uint64_t v20 = (m <= 3u) ? 3u : m;
    const std::uint64_t v21 = static_cast<std::uint64_t>(m) * block_size;
    std::uint64_t v24 = 17ull * m - 1ull;
    if (v24 < v21) {
        const std::uint64_t v25 = v20 - 3ull;
        while (v24 < v21) {
            std::uint64_t idx = 0;
            std::uint64_t rem = 0;
            std::uint32_t mask = 0;
            if (v24 > 0x2Full) {
                if (v24 >= 16ull * v20) {
                    idx = v24 / v20;
                    rem = v24 % v20;
                } else {
                    idx = (v24 - 48ull) / v25;
                    rem = (v24 - 48ull) % v25;
                }
                mask = 1u << static_cast<unsigned>(v20 - 1ull - rem);
            } else if (v24 >= 0x10ull) {
                if (v24 <= 0x1Full) {
                    idx = v24 - 16ull;
                    mask = 2u;
                } else {
                    idx = v24 - 32ull;
                    mask = 4u;
                }
            } else {
                idx = v24;
                mask = 1u;
            }
            if (start + idx >= samples.size())
                return false;
            if ((static_cast<std::uint32_t>(samples[start + static_cast<std::size_t>(idx)]) & mask) != 0)
                return false;
            v24 += v19;
        }
    }
    out.block_size = block_size;
    out.m = m;
    return true;
}

std::uint16_t compute_v25_from_first16_at(const std::vector<std::int32_t>& samples, std::size_t start) {
    std::uint16_t v15[32]{};
    for (unsigned i = 0; i < 16u; ++i) {
        const std::uint32_t w = static_cast<std::uint32_t>(samples[start + i]);
        v15[2u * i] = static_cast<std::uint16_t>(w & 0xFFFFu);
        v15[2u * i + 1u] = static_cast<std::uint16_t>((w >> 16u) & 0xFFFFu);
    }
    const std::uint16_t v18 = v15[4];
    const std::uint16_t v19 = v15[8];
    const std::uint32_t v21 = v15[6] & 0xFFu;
    const std::uint32_t v22 = (((2u * v15[0]) & 0xFFFFFFFCu) | (v15[2] & 3u)) >> 1u;
    const auto b4 = [&v15](unsigned idx) -> std::uint32_t {
        return (2u * (v15[idx] & 0xFFu)) & 4u;
    };
    std::uint32_t a = (((2u * v18) & 4u) | (8u * (v22 & 3u)) | (v21 & 3u)) >> 1u;
    a &= 0xFu;
    const std::uint32_t b = (((2u * v19) & 4u) | (8u * a) | (v15[10] & 3u)) >> 1u;
    const std::uint32_t c = (b4(12) | (8u * b) | (v15[14] & 3u)) >> 1u;
    const std::uint32_t d = (b4(16) | (8u * c) | (v15[18] & 3u)) >> 1u;
    const std::uint32_t e = (b4(20) | (8u * d) | (v15[22] & 3u)) >> 1u;
    const std::uint32_t f = b4(24) | (8u * e) | (v15[26] & 3u);
    const std::uint32_t v23 = 2u * f;
    return static_cast<std::uint16_t>((v23 & 0xFFFFFFFCu) | (v15[28] & 2u) | ((v15[30] >> 1u) & 1u));
}

bool auro_adol_channel_input_config_get_original_layout(unsigned cfg_id, std::uint32_t& out, bool aliases) {
    switch (cfg_id) {
    case 1: out = 55u; return true;
    case 2: out = 63u; return true;
    case 8: out = 71u; return true;
    case 11: out = 1587u; return true;
    case 12: out = 51u; return true;
    case 15: out = 1599u; return true;
    case 20: out = 26163u; return true;
    case 30: out = 26175u; return true;
    case 40: out = 30271u; return true;
    case 50: out = 32319u; return true;
    case 54: out = 26559u; return true;
    case 62: out = 32703u; return true;
    case 64: out = 3u; return true;
    case 66: out = 7u; return true;
    case 67: out = 119u; return true;
    case 68: out = 127u; return true;
    case 69: out = 439u; return true;
    case 70: out = 447u; return true;
    case 71: out = 26167u; return true;
    case 72: out = 30263u; return true;
    case 73: out = 32311u; return true;
    case 74: out = 26551u; return true;
    case 75: out = 1983u; return true;
    case 76: out = 30647u; return true;
    case 77: out = 30655u; return true;
    case 78: out = 32695u; return true;
    case 128: out = 4u; return true;
    case 129: out = 2052u; return true;
    case 130: out = 6148u; return true;
    default:
        if (!aliases)
            return false;
        switch (cfg_id) {
        case 3: out = 55u; return true;
        case 9: out = 51u; return true;
        case 10: out = 1587u; return true;
        case 53: out = 32319u; return true;
        case 60: out = 65151u; return true;
        case 61: out = 98111u; return true;
        default: return false;
        }
    }
}

std::uint32_t auro_adol_channel_input_config_get_original_layout(unsigned cfg_id) {
    std::uint32_t out = 0;
    return auro_adol_channel_input_config_get_original_layout(cfg_id, out, true) ? out : 0u;
}

bool auro_adol_channel_input_config_get_carrier_layout(unsigned cfg_id, std::uint32_t& out) {
    switch (cfg_id) {
    case 1:
    case 8:
    case 11:
    case 12:
    case 66:
        out = 3u;
        return true;
    case 2:
        out = 11u;
        return true;
    case 15:
    case 30:
    case 40:
    case 50:
    case 68:
    case 70:
        out = 63u;
        return true;
    case 20:
        out = 51u;
        return true;
    case 54:
    case 62:
    case 75:
    case 77:
        out = 447u;
        return true;
    case 64:
    case 128:
    case 129:
    case 130:
        out = 4u;
        return true;
    case 67:
    case 69:
    case 71:
    case 72:
    case 73:
        out = 55u;
        return true;
    case 74:
    case 76:
    case 78:
        out = 439u;
        return true;
    default:
        return false;
    }
}

bool auro_codec_v3_is_sample_rate_supported(std::uint32_t sample_rate) {
    return sample_rate == 44100u || sample_rate == 48000u
        || sample_rate == 88200u || sample_rate == 96000u;
}

bool auro_codec_v3_is_unit_block_size_supported(std::uint32_t block_size) {
    if (block_size == 1000u)
        return true;
    if (block_size == 992u)
        return false;
    const std::uint32_t delta = block_size - 1025u;
    if (delta >= 0xFFFFFCFFu)
        return (block_size & 0xFu) == 0u;
    return false;
}

bool auro_codec_v3_get_carrier_layout(std::uint32_t original_layout, std::uint32_t& out) {
    switch (original_layout) {
    case 3u:
    case 4u:
    case 2052u:
    case 6148u:
        out = 4u;
        return true;
    case 7u:
    case 51u:
    case 55u:
    case 71u:
    case 1587u:
        out = 3u;
        return true;
    case 63u:
        out = 11u;
        return true;
    case 119u:
    case 439u:
    case 26167u:
    case 30263u:
    case 32311u:
        out = 55u;
        return true;
    case 127u:
    case 447u:
    case 1599u:
    case 26175u:
    case 30271u:
    case 32319u:
        out = 63u;
        return true;
    case 26163u:
        out = 51u;
        return true;
    case 26551u:
    case 30647u:
    case 32695u:
        out = 439u;
        return true;
    case 1983u:
    case 26559u:
    case 30655u:
    case 32703u:
        out = 447u;
        return true;
    default:
        return false;
    }
}

bool auro_codec_v3_uses_mix3(std::uint32_t layout) {
    switch (layout) {
    case 55u:
    case 63u:
    case 71u:
    case 1587u:
    case 6148u:
    case 30263u:
    case 30271u:
    case 30647u:
    case 30655u:
    case 32311u:
    case 32319u:
    case 32695u:
    case 32703u:
        return true;
    default:
        return false;
    }
}

bool auro_codec_v3_get_closest_layout_without_mix3(std::uint32_t layout, std::uint32_t& out) {
    switch (layout) {
    case 55u:
    case 71u:
    case 1587u:
        out = 51u;
        return true;
    case 63u:
        return false;
    case 6148u:
        out = 2052u;
        return true;
    case 30263u:
    case 32311u:
        out = 26167u;
        return true;
    case 30271u:
    case 32319u:
        out = 26175u;
        return true;
    case 30647u:
    case 32695u:
        out = 26551u;
        return true;
    case 30655u:
    case 32703u:
        out = 26559u;
        return true;
    default:
        out = layout;
        return true;
    }
}

const char* auro_channel_layout_to_string(std::uint32_t layout) {
    switch (layout) {
    case 3u: return "2.0";
    case 4u: return "1.0";
    case 7u: return "3.0";
    case 8u: return "0.1";
    case 11u: return "2.1";
    case 12u: return "1.1";
    case 15u: return "3.1";
    case 51u: return "4.0";
    case 55u: return "5.0";
    case 59u: return "4.1";
    case 63u: return "5.1";
    case 71u: return "LCRS";
    case 119u: return "6.0";
    case 127u: return "6.1";
    case 435u: return "7.0_no_C";
    case 439u: return "7.0";
    case 443u: return "7.1_no_C";
    case 447u: return "7.1";
    case 1539u: return "2.0_2H";
    case 1543u: return "3.0_2H";
    case 1547u: return "2.1_2H";
    case 1551u: return "3.1_2H";
    case 1587u: return "4.0_2H";
    case 1591u: return "5.0_2H";
    case 1595u: return "4.1_2H";
    case 1599u: return "5.1_2H";
    case 1971u: return "7.0_2H_no_C";
    case 1975u: return "7.0_2H";
    case 1979u: return "7.1_2H_no_C";
    case 1983u: return "7.1_2H";
    case 3591u: return "3.0_3H";
    case 3599u: return "3.1_3H";
    case 26163u: return "4.0_4H";
    case 26167u: return "5.0_4H";
    case 26171u: return "4.1_4H";
    case 26175u: return "5.1_4H";
    case 26547u: return "7.0_4H_no_C";
    case 26551u: return "7.0_4H";
    case 26555u: return "7.1_4H_no_C";
    case 26559u: return "7.1_4H";
    case 28211u: return "4.0_5H";
    case 28215u: return "5.0_5H";
    case 28219u: return "4.1_5H";
    case 28223u: return "5.1_5H";
    case 28595u: return "7.0_5H_no_C";
    case 28599u: return "7.0_5H";
    case 28603u: return "7.1_5H_no_C";
    case 28607u: return "7.1_5H";
    case 30259u: return "4.0_4H_1T";
    case 30263u: return "5.0_4H_1T";
    case 30267u: return "4.1_4H_1T";
    case 30271u: return "5.1_4H_1T";
    case 30643u: return "7.0_4H_1T_no_C";
    case 30647u: return "7.0_4H_1T";
    case 30651u: return "7.1_4H_1T_no_C";
    case 30655u: return "7.1_4H_1T";
    case 32307u: return "4.0_5H_1T";
    case 32311u: return "5.0_5H_1T";
    case 32315u: return "4.1_5H_1T";
    case 32319u: return "5.1_5H_1T";
    case 32691u: return "7.0_5H_1T_no_C";
    case 32695u: return "7.0_5H_1T";
    case 32699u: return "7.1_5H_1T_no_C";
    case 32703u: return "7.1_5H_1T";
    case 805332543u: return "5.1_4H_2T";
    case 805332927u: return "7.1_4H_2T";
    case 805334591u: return "5.1_5H_2T";
    case 805334975u: return "7.1_5H_2T";
    case 1006659519u: return "9.1_4H_2T";
    case 1006661567u: return "9.1_5H_2T";
    case 0xFFFFFFu: return "NHK_22.2";
    case 15728631u: return "NHK_22.0";
    case 56649216u: return "Cube";
    case 2052u: return "TestMix2.0";
    case 6148u: return "TestMix3.0";
    default: return "";
    }
}

unsigned popcount_u32(std::uint32_t v) {
    unsigned n = 0;
    while (v != 0) {
        n += static_cast<unsigned>(v & 1u);
        v >>= 1u;
    }
    return n;
}

void update_auro_metadata_layout_fields(auro3d::AuroMetadataInfo& info) {
    if (info.layout_id == 0u)
        return;
    info.layout_name = auro_channel_layout_to_string(info.layout_id);
    info.output_channels = popcount_u32(info.layout_id);
    info.carrier_layout_id = 0u;
    info.carrier_layout_name.clear();
    info.carrier_channels = 0u;
    info.uses_mix3 = auro_codec_v3_uses_mix3(info.layout_id);
    info.has_closest_layout_without_mix3 = false;
    info.closest_layout_without_mix3 = 0u;
    info.closest_layout_without_mix3_name.clear();
    std::uint32_t carrier_layout = 0;
    if (auro_codec_v3_get_carrier_layout(info.layout_id, carrier_layout)) {
        info.carrier_layout_id = carrier_layout;
        info.carrier_layout_name = auro_channel_layout_to_string(carrier_layout);
        info.carrier_channels = popcount_u32(carrier_layout);
    }
    std::uint32_t closest_without_mix3 = 0;
    if (info.uses_mix3
        && auro_codec_v3_get_closest_layout_without_mix3(info.layout_id, closest_without_mix3)) {
        info.has_closest_layout_without_mix3 = true;
        info.closest_layout_without_mix3 = closest_without_mix3;
        info.closest_layout_without_mix3_name = auro_channel_layout_to_string(closest_without_mix3);
    }
}

void decode_adol_semantics(auro3d::AuroAdolInstructionInfo& dst) {
    switch (dst.opcode) {
    case 0x1Eu:
        if (dst.has_value) {
            dst.decoded_layout = auro_adol_channel_input_config_get_original_layout(dst.value);
            dst.has_decoded_layout = dst.decoded_layout != 0u;
            dst.has_carrier_layout =
                auro_adol_channel_input_config_get_carrier_layout(dst.value, dst.carrier_layout);
        }
        break;
    case 0x40u:
        if (dst.has_value && dst.has_value2 && dst.value <= 0x1Eu) {
            dst.has_primary_downmix_gain = true;
            dst.primary_downmix_channel = static_cast<std::uint8_t>(dst.value);
            dst.primary_downmix_scaler = static_cast<std::uint8_t>(dst.value2);
        }
        break;
    case 0x41u:
        if (dst.has_value) {
            dst.has_limit_simple = true;
            dst.limit_simple_scaler = static_cast<std::uint8_t>(dst.value);
        }
        break;
    case 0x46u:
        if (dst.has_value) {
            dst.has_secondary_downmix_gains = true;
            dst.secondary_downmix_packed = dst.value;
        }
        break;
    case 0x47u:
        if (dst.has_value) {
            dst.has_auromatic = true;
            dst.auromatic_profile = static_cast<std::uint8_t>((dst.value >> 4u) & 0x0Fu);
            dst.auromatic_mode = static_cast<std::uint8_t>(dst.value & 0x0Fu);
        }
        break;
    case 0x64u:
        if (dst.has_value) {
            dst.has_encoder_version = true;
            dst.encoder_version = dst.value & 0x00FFFFFFu;
        }
        break;
    default:
        if (dst.opcode >= 0x80u && dst.opcode <= 0x85u && dst.has_value) {
            dst.has_loudness = true;
            dst.loudness_raw = dst.value;
        }
        break;
    }
}

bool adol_parse(
    MuxIteratorFalse& it,
    std::uint64_t& bits_left,
    std::uint32_t block_index,
    std::uint32_t tag,
    std::vector<AdolInstruction>& out) {
    for (;;) {
        std::uint32_t opcode = 0;
        if (!read_unsigned_false(8u, it, bits_left, opcode))
            return false;
        AdolInstruction ins{};
        ins.block_index = block_index;
        ins.tag = tag;
        ins.opcode = opcode;
        auto read_value = [&](unsigned bits) -> bool {
            std::uint32_t v = 0;
            if (!read_unsigned_false(bits, it, bits_left, v))
                return false;
            ins.has_value = true;
            ins.value = v;
            ins.value_bits = bits;
            return true;
        };
        auto read_value2 = [&](unsigned bits) -> bool {
            std::uint32_t v = 0;
            if (!read_unsigned_false(bits, it, bits_left, v))
                return false;
            ins.has_value2 = true;
            ins.value2 = v;
            ins.value2_bits = bits;
            return true;
        };
        if (opcode == 1u || opcode == 3u || (opcode >= 0x5Au && opcode <= 0x62u)) {
            if (!read_value(16u))
                return false;
        } else if (opcode == 2u || opcode == 4u || opcode == 0x1Eu || opcode == 0x1Fu
            || opcode == 0x41u || opcode == 0x47u || (opcode >= 0x50u && opcode <= 0x58u)) {
            if (!read_value(8u))
                return false;
        } else if (opcode == 0x0Eu || opcode == 0x46u || (opcode >= 0x6Eu && opcode <= 0x76u)
            || (opcode >= 0x80u && opcode <= 0x85u) || (opcode >= 0x8Cu && opcode <= 0x90u)) {
            if (!read_value(32u))
                return false;
        } else if (opcode == 0x40u) {
            if (!read_value(8u) || !read_value2(8u))
                return false;
        } else if (opcode >= 0x64u && opcode <= 0x6Cu) {
            if (!read_value(24u))
                return false;
        } else if (opcode != 0u) {
            return false;
        }
        out.push_back(ins);
        if (opcode == 0u)
            return true;
    }
}

bool extract_metadata_from_a3d_block_at(
    const std::vector<std::int32_t>& samples,
    std::size_t start,
    unsigned block_size,
    std::uint32_t sync_block_index,
    auro3d::AuroMetadataInfo& info) {
    info = {};
    A3DSyncInfo sync{};
    if (!parse_sync_pcm24_at(samples, start, block_size, true, sync))
        return false;
    const std::uint16_t crc = compute_crc16_ccitt_over_pcm24_at(samples, start, block_size);
    const std::uint16_t expected = compute_v25_from_first16_at(samples, start);
    if (crc != expected)
        return false;
    const std::uint64_t bits_total =
        static_cast<std::uint64_t>(block_size) * sync.m - ((block_size - 1u) / 16u);
    if (bits_total <= 32u)
        return false;
    std::uint64_t bits_left = bits_total - 32u;
    MuxIteratorFalse it{&samples, start, sync.m, 32u, 16u * sync.m - 1u};
    std::uint32_t tmp = 0;
    if (!read_unsigned_false(8u, it, bits_left, tmp))
        return false;
    for (unsigned i = 0; i < 4u; ++i) {
        if (!read_bool_false(it, bits_left))
            return false;
    }
    if (!read_unsigned_false(4u, it, bits_left, tmp))
        return false;
    for (unsigned i = 0; i < 2u; ++i) {
        if (!read_unsigned_false(8u, it, bits_left, tmp))
            return false;
    }
    for (unsigned i = 0; i < 2u; ++i) {
        if (!read_bool_false(it, bits_left))
            return false;
    }
    if (!read_unsigned_false(2u, it, bits_left, tmp)
        || !read_unsigned_false(4u, it, bits_left, tmp)
        || !read_unsigned_false(8u, it, bits_left, tmp)) {
        return false;
    }
    std::uint32_t adol_blocks = 0;
    if (!read_unsigned_false(8u, it, bits_left, adol_blocks)
        || !read_unsigned_false(8u, it, bits_left, tmp)) {
        return false;
    }
    info.adol_block_count = adol_blocks;
    unsigned count = 0;
    for (unsigned i = 0; i < 4u; ++i) {
        if (!read_unsigned_false(8u, it, bits_left, tmp))
            return false;
        if (tmp != 255u)
            ++count;
    }
    for (unsigned i = 0; i < 4u; ++i) {
        if (!read_unsigned_false(8u, it, bits_left, tmp))
            return false;
    }
    unsigned vec_sz = 0;
    if (count == 2u)
        vec_sz = 2u;
    else if (count == 3u)
        vec_sz = 5u;
    else if (count > 3u)
        return false;
    for (unsigned i = 0; i < vec_sz; ++i) {
        if (!read_unsigned_false(32u, it, bits_left, tmp))
            return false;
    }

    std::vector<AdolInstruction> instructions;
    for (std::uint32_t b = 0; b < adol_blocks; ++b) {
        std::uint32_t tag = 0;
        if (!read_unsigned_false(8u, it, bits_left, tag) || tag != 1u)
            return false;
        if (!adol_parse(it, bits_left, b, tag, instructions))
            return false;
    }
    for (const auto& ins : instructions) {
        auro3d::AuroAdolInstructionInfo dst{};
        dst.sync_block_index = sync_block_index;
        dst.block_index = ins.block_index;
        dst.tag = ins.tag;
        dst.opcode = static_cast<std::uint32_t>(ins.opcode);
        dst.has_value = ins.has_value;
        dst.value = ins.value;
        dst.value_bits = static_cast<std::uint8_t>(ins.value_bits);
        dst.has_value2 = ins.has_value2;
        dst.value2 = ins.value2;
        dst.value2_bits = static_cast<std::uint8_t>(ins.value2_bits);
        decode_adol_semantics(dst);
        info.adol_instructions.push_back(dst);
        if (dst.has_decoded_layout) {
            info.layout_id = dst.decoded_layout;
            update_auro_metadata_layout_fields(info);
        }
    }
    return info.layout_id != 0;
}

std::int32_t decode_pcm24_sample(const std::uint8_t* p) {
    std::int32_t v = static_cast<std::int32_t>(p[0])
        | (static_cast<std::int32_t>(p[1]) << 8)
        | (static_cast<std::int32_t>(p[2]) << 16);
    if ((v & 0x800000) != 0)
        v |= ~0xFFFFFF;
    return v;
}

std::int32_t clamp_i32_to_pcm24(std::int64_t v) {
    if (v > 0x7FFFFFll)
        return 0x7FFFFF;
    if (v < -0x800000ll)
        return -0x800000;
    return static_cast<std::int32_t>(v);
}

auro3d::AuroMetadataInfo scan_auro_metadata_pcm24(
    const std::vector<std::uint8_t>& file_bytes,
    std::size_t pcm_begin,
    std::size_t pcm_length,
    unsigned channels) {
    auro3d::AuroMetadataInfo info{};
    if (channels == 0)
        return info;
    const std::size_t frame_bytes = static_cast<std::size_t>(channels) * 3u;
    if (frame_bytes == 0 || pcm_length < frame_bytes || pcm_length % frame_bytes != 0)
        return info;
    const std::size_t frames = pcm_length / frame_bytes;
    if (frames < 16u || pcm_begin + pcm_length > file_bytes.size())
        return info;

    auto merge_instruction = [](auro3d::AuroMetadataInfo& dst, const auro3d::AuroAdolInstructionInfo& src) {
        for (auto& existing : dst.adol_instructions) {
            if (existing.tag == src.tag
                && existing.opcode == src.opcode
                && existing.has_value == src.has_value
                && existing.value == src.value
                && existing.value_bits == src.value_bits
                && existing.has_value2 == src.has_value2
                && existing.value2 == src.value2
                && existing.value2_bits == src.value2_bits) {
                ++existing.occurrences;
                return;
            }
        }
        dst.adol_instructions.push_back(src);
    };

    for (unsigned ch = 0; ch < channels; ++ch) {
        std::vector<std::int32_t> samples(frames);
        const std::uint8_t* base = file_bytes.data() + pcm_begin + static_cast<std::size_t>(ch) * 3u;
        for (std::size_t s = 0; s < frames; ++s)
            samples[s] = decode_pcm24_sample(base + s * frame_bytes);

        unsigned run_ones = 0;
        std::uint32_t sync_block_index = 0;
        for (std::size_t s = 0; s < frames; ++s) {
            if ((static_cast<std::uint32_t>(samples[s]) & 1u) != 0)
                ++run_ones;
            else {
                run_ones = 0;
                continue;
            }
            if (run_ones < 16u)
                continue;
            const std::size_t start = s - 15u;
            A3DSyncInfo sync{};
            if (!parse_sync_pcm24_at(samples, start, frames - start, false, sync))
                continue;
            if (start + sync.block_size > frames)
                continue;
            auro3d::AuroMetadataInfo block_info{};
            if (!extract_metadata_from_a3d_block_at(samples, start, sync.block_size, sync_block_index, block_info))
                continue;
            if (!info.found) {
                info.found = true;
                info.layout_id = block_info.layout_id;
                info.layout_name = block_info.layout_name;
                info.output_channels = block_info.output_channels;
                info.carrier_layout_id = block_info.carrier_layout_id;
                info.carrier_layout_name = block_info.carrier_layout_name;
                info.carrier_channels = block_info.carrier_channels;
                info.uses_mix3 = block_info.uses_mix3;
                info.has_closest_layout_without_mix3 = block_info.has_closest_layout_without_mix3;
                info.closest_layout_without_mix3 = block_info.closest_layout_without_mix3;
                info.closest_layout_without_mix3_name = block_info.closest_layout_without_mix3_name;
                info.carrier_channel = ch;
                info.sync_sample = static_cast<std::uint64_t>(start);
                info.block_size = sync.block_size;
            }
            info.adol_block_count += block_info.adol_block_count;
            ++info.scanned_sync_blocks;
            for (const auto& ins : block_info.adol_instructions)
                merge_instruction(info, ins);
            ++sync_block_index;
            s = start + sync.block_size - 1u;
            run_ones = 0;
        }
        if (info.found)
            return info;
    }
    return info;
}

bool try_parse_wav_s24le(
    const std::uint8_t* p,
    std::size_t n,
    std::size_t& pcm_begin,
    std::size_t& pcm_length,
    std::uint16_t& channels,
    std::uint32_t& sample_rate,
    std::uint32_t& channel_mask,
    std::string& err) {
    channel_mask = 0;
    if (n < 12 || std::memcmp(p, "RIFF", 4) != 0 || std::memcmp(p + 8, "WAVE", 4) != 0) {
        err = "not RIFF/WAVE";
        return false;
    }
    bool fmt_ok = false;
    std::size_t pos = 12;
    while (pos + 8 <= n) {
        const char* id = reinterpret_cast<const char*>(p + pos);
        const std::uint32_t csz = static_cast<std::uint32_t>(p[pos + 4]) | (static_cast<std::uint32_t>(p[pos + 5]) << 8)
            | (static_cast<std::uint32_t>(p[pos + 6]) << 16) | (static_cast<std::uint32_t>(p[pos + 7]) << 24);
        pos += 8;
        if (pos + csz > n) {
            err = "truncated chunk";
            return false;
        }
        if (std::memcmp(id, "fmt ", 4) == 0) {
            if (csz < 16) {
                err = "fmt too small";
                return false;
            }
            const std::uint16_t audio_format = static_cast<std::uint16_t>(p[pos]) | (static_cast<std::uint16_t>(p[pos + 1]) << 8);
            channels = static_cast<std::uint16_t>(p[pos + 2]) | (static_cast<std::uint16_t>(p[pos + 3]) << 8);
            sample_rate = static_cast<std::uint32_t>(p[pos + 4]) | (static_cast<std::uint32_t>(p[pos + 5]) << 8)
                | (static_cast<std::uint32_t>(p[pos + 6]) << 16) | (static_cast<std::uint32_t>(p[pos + 7]) << 24);
            const std::uint16_t bits = static_cast<std::uint16_t>(p[pos + 14]) | (static_cast<std::uint16_t>(p[pos + 15]) << 8);
            const bool is_pcm_s24 = (audio_format == 1 && bits == 24);
            bool is_extensible_pcm_s24 = false;
            if (audio_format == 0xFFFEu && csz >= 40) {
                const std::uint16_t cb_size =
                    static_cast<std::uint16_t>(p[pos + 16]) | (static_cast<std::uint16_t>(p[pos + 17]) << 8);
                const std::uint16_t valid_bits =
                    static_cast<std::uint16_t>(p[pos + 18]) | (static_cast<std::uint16_t>(p[pos + 19]) << 8);
                const std::uint32_t extensible_channel_mask =
                    static_cast<std::uint32_t>(p[pos + 20])
                    | (static_cast<std::uint32_t>(p[pos + 21]) << 8)
                    | (static_cast<std::uint32_t>(p[pos + 22]) << 16)
                    | (static_cast<std::uint32_t>(p[pos + 23]) << 24);
                static constexpr std::uint8_t kKsdSubTypePcmTail[14] = {
                    0x00, 0x00, 0x00, 0x00,
                    0x10, 0x00, 0x80, 0x00,
                    0x00, 0xAA, 0x00, 0x38,
                    0x9B, 0x71,
                };
                const bool subtype_pcm = p[pos + 24] == 0x01 && p[pos + 25] == 0x00
                    && std::memcmp(p + pos + 26, kKsdSubTypePcmTail, sizeof(kKsdSubTypePcmTail)) == 0;
                is_extensible_pcm_s24 = (cb_size >= 22u && valid_bits == 24u && bits == 24u && subtype_pcm);
                if (is_extensible_pcm_s24)
                    channel_mask = extensible_channel_mask;
            }
            fmt_ok = is_pcm_s24 || is_extensible_pcm_s24;
            if (!fmt_ok) {
                err = "WAV must be PCM 24-bit or WAVEFORMATEXTENSIBLE PCM 24-bit";
                return false;
            }
        } else if (std::memcmp(id, "data", 4) == 0) {
            if (!fmt_ok) {
                err = "fmt chunk missing or invalid";
                return false;
            }
            pcm_begin = pos;
            pcm_length = csz;
            return true;
        }
        pos += csz + (csz & 1u);
    }
    err = "no data chunk";
    return false;
}

bool read_file_bytes(const std::string& path, std::vector<std::uint8_t>& bytes, std::string& err) {
    err.clear();
    bytes.clear();
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        err = "cannot open file";
        return false;
    }
    in.seekg(0, std::ios::end);
    const auto sz = in.tellg();
    if (sz < 0) {
        err = "file size";
        return false;
    }
    in.seekg(0, std::ios::beg);
    bytes.resize(static_cast<std::size_t>(sz));
    if (sz > 0 && !in.read(reinterpret_cast<char*>(bytes.data()), sz)) {
        err = "read error";
        return false;
    }
    return true;
}

bool read_file_prefix(
    const std::string& path,
    std::size_t maximum_bytes,
    std::vector<std::uint8_t>& bytes,
    std::string& err) {
    err.clear();
    bytes.clear();
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        err = "cannot open file";
        return false;
    }
    bytes.resize(maximum_bytes);
    in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(maximum_bytes));
    const std::streamsize count = in.gcount();
    if (count < 0 || (in.bad() && count == 0)) {
        err = "read error";
        bytes.clear();
        return false;
    }
    bytes.resize(static_cast<std::size_t>(count));
    return true;
}

bool is_flac_stream(const std::vector<std::uint8_t>& bytes) {
    return bytes.size() >= 4 && std::memcmp(bytes.data(), "fLaC", 4) == 0;
}

bool is_matroska_stream(const std::vector<std::uint8_t>& bytes) {
    static constexpr std::uint8_t kEbmlHeader[] = {0x1A, 0x45, 0xDF, 0xA3};
    return bytes.size() >= sizeof(kEbmlHeader)
        && std::memcmp(bytes.data(), kEbmlHeader, sizeof(kEbmlHeader)) == 0;
}

bool is_iso_base_media_stream(const std::vector<std::uint8_t>& bytes) {
    return bytes.size() >= 12 && std::memcmp(bytes.data() + 4, "ftyp", 4) == 0;
}

bool is_ffmpeg_audio_input(const std::vector<std::uint8_t>& bytes) {
    return is_flac_stream(bytes) || is_matroska_stream(bytes) || is_iso_base_media_stream(bytes);
}

std::string shell_quote_path(const std::string& path) {
    std::string q = "\"";
    for (char c : path) {
        if (c == '"')
            q += "\\\"";
        else
            q += c;
    }
    q += "\"";
    return q;
}

std::string temp_wav_path() {
    const char* tmp = std::getenv("TEMP");
    if (!tmp || !*tmp)
        tmp = std::getenv("TMP");
    if (!tmp || !*tmp)
        tmp = ".";
    std::string dir = tmp;
    const char last = dir.empty() ? '\0' : dir.back();
    if (last != '\\' && last != '/')
#ifdef _WIN32
        dir += "\\";
#else
        dir += "/";
#endif
    const auto ticks = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::ostringstream name;
    name << dir << "auro3d_decode_audio_" << ticks << "_" << std::rand() << ".wav";
    return name.str();
}

bool run_command_capture_stdout(const std::string& command, std::string& output) {
    output.clear();
#ifdef _WIN32
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif
    if (!pipe)
        return false;

    std::array<char, 4096> buffer{};
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe))
        output.append(buffer.data());

#ifdef _WIN32
    return _pclose(pipe) == 0;
#else
    return pclose(pipe) == 0;
#endif
}

std::string ascii_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

unsigned parse_probe_unsigned(const std::string& value) {
    if (value.empty() || value == "N/A")
        return 0u;
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(value.c_str(), &end, 10);
    return end && *end == '\0' ? static_cast<unsigned>(parsed) : 0u;
}

bool find_supported_audio_stream(const std::string& path, unsigned& stream_index, std::string& err) {
    const std::string command =
        "ffprobe -v error -select_streams a "
        "-show_entries stream=index,codec_name,profile,bits_per_sample,bits_per_raw_sample "
        "-of compact=p=0:nk=0 " + shell_quote_path(path);
    std::string probe_output;
    if (!run_command_capture_stdout(command, probe_output)) {
        err = "ffprobe failed to inspect input audio streams";
        return false;
    }

    std::istringstream lines(probe_output);
    std::string line;
    while (std::getline(lines, line)) {
        unsigned index = std::numeric_limits<unsigned>::max();
        unsigned bits_per_sample = 0u;
        unsigned bits_per_raw_sample = 0u;
        std::string codec;
        std::string profile;

        std::istringstream fields(line);
        std::string field;
        while (std::getline(fields, field, '|')) {
            const std::size_t equals = field.find('=');
            if (equals == std::string::npos)
                continue;
            const std::string key = field.substr(0, equals);
            const std::string value = field.substr(equals + 1);
            if (key == "index")
                index = parse_probe_unsigned(value);
            else if (key == "codec_name")
                codec = ascii_lower(value);
            else if (key == "profile")
                profile = ascii_lower(value);
            else if (key == "bits_per_sample")
                bits_per_sample = parse_probe_unsigned(value);
            else if (key == "bits_per_raw_sample")
                bits_per_raw_sample = parse_probe_unsigned(value);
        }

        const unsigned bit_depth = bits_per_raw_sample != 0u
            ? bits_per_raw_sample
            : bits_per_sample;
        const bool flac24 = codec == "flac" && bit_depth == 24u;
        const bool dts_hd_ma = codec == "dts"
            && profile.find("dts-hd ma") != std::string::npos
            && (bit_depth == 0u || bit_depth == 24u);
        if (index != std::numeric_limits<unsigned>::max() && (flac24 || dts_hd_ma)) {
            stream_index = index;
            return true;
        }
    }

    err = "no 24-bit FLAC or DTS-HD MA audio stream found";
    return false;
}

bool decode_supported_audio_to_pcm24_wav_bytes(
    const std::string& path,
    std::vector<std::uint8_t>& wav_bytes,
    std::string& err) {
    err.clear();
    wav_bytes.clear();
    unsigned stream_index = 0u;
    if (!find_supported_audio_stream(path, stream_index, err))
        return false;

    const std::string tmp_wav = temp_wav_path();
    const std::string cmd =
        "ffmpeg -y -v error -i " + shell_quote_path(path)
        + " -map 0:" + std::to_string(stream_index)
        + " -c:a pcm_s24le -f wav " + shell_quote_path(tmp_wav);
    const int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::remove(tmp_wav.c_str());
        err = "ffmpeg failed to decode selected audio stream to PCM24 WAV";
        return false;
    }
    const bool ok = read_file_bytes(tmp_wav, wav_bytes, err);
    std::remove(tmp_wav.c_str());
    if (!ok)
        return false;
    if (wav_bytes.size() < 12 || std::memcmp(wav_bytes.data(), "RIFF", 4) != 0) {
        err = "ffmpeg did not produce WAV";
        return false;
    }
    return true;
}

} // namespace

namespace auro3d {

const char* auro_channel_layout_to_string(std::uint32_t layout) {
    return ::auro_channel_layout_to_string(layout);
}

const char* decode_error_message(DecodeError e) {
    switch (e) {
    case DecodeError::Ok: return "ok";
    case DecodeError::InitFailed: return "decoder not initialized";
    case DecodeError::BadInput: return "invalid or unsupported input";
    case DecodeError::IoError: return "I/O error";
    case DecodeError::NotImplemented:
        return "unsupported AURO layout or legacy input needs --dsp-output-channels 6, 8, 10, or 12";
    default: return "unknown error";
    }
}

void Decoder::set_raw_pcm24_params(uint32_t sample_rate_hz, unsigned channel_count, unsigned block_size) {
    sample_rate_ = sample_rate_hz;
    channel_count_ = channel_count;
    block_size_ = block_size != 0 ? block_size : kDefaultJniBlockSize;
    raw_forced_ = true;
}

void Decoder::set_dsp_headroom_db(float db) {
    if (db < 0.0f)
        db = 0.0f;
    if (db > 24.0f)
        db = 24.0f;
    dsp_headroom_gain_ = std::pow(10.0f, -db / 20.0f);
}

void Decoder::set_room_preset(unsigned preset) {
    room_preset_ = preset;
    if (opened_)
        (void)apply_native_dynamic_parameters_update();
}

void Decoder::set_hrtf_preset(unsigned preset) {
    hrtf_preset_ = preset;
    if (opened_)
        (void)apply_native_dynamic_parameters_update();
}

void Decoder::set_virtualizer_mode(unsigned mode) {
    virtualizer_mode_ = mode;
    if (opened_)
        (void)apply_native_dynamic_parameters_update();
}

void Decoder::set_output_audio_devices(bool headphone_connected, bool stereo_device_connected) {
    headphone_connected_ = headphone_connected;
    stereo_device_connected_ = stereo_device_connected;
    if (opened_)
        (void)apply_native_dynamic_parameters_update();
}

std::int32_t* Decoder::native_work_buffer(unsigned index) {
    if (block_size_ == 0 || index >= native_work_buffer_count_)
        return nullptr;
    return native_work_buffer_storage_.data() + static_cast<std::size_t>(index) * block_size_;
}

const std::int32_t* Decoder::native_work_buffer(unsigned index) const {
    if (block_size_ == 0 || index >= native_work_buffer_count_)
        return nullptr;
    return native_work_buffer_storage_.data() + static_cast<std::size_t>(index) * block_size_;
}

std::int32_t* Decoder::native_input_buffer(unsigned index) {
    if (block_size_ == 0 || index >= native_input_buffer_count_)
        return nullptr;
    return native_input_buffer_storage_.data() + static_cast<std::size_t>(index) * block_size_;
}

const std::int32_t* Decoder::native_input_buffer(unsigned index) const {
    if (block_size_ == 0 || index >= native_input_buffer_count_)
        return nullptr;
    return native_input_buffer_storage_.data() + static_cast<std::size_t>(index) * block_size_;
}

void Decoder::apply_native_input_channel_mapping() {
    input_channel_mask_ = 0;
    for (auto& ptr : input_desc_.channel_ptr)
        ptr = 0;

    const NativeChannelLayoutPlan layout =
        build_native_input_channel_layout(channel_count_, input_wav_channel_mask_, auro_metadata_);
    for (unsigned work_index = 0; work_index < layout.slot_count; ++work_index) {
        const std::uint32_t logical_slot = layout.slots[work_index];
        if (logical_slot >= auro_codec_v3_ida::kAuroProcessorIoChannelPtrCount || work_index >= channel_count_)
            continue;
        input_desc_.channel_ptr[logical_slot] =
            reinterpret_cast<std::uint64_t>(native_input_buffer(work_index));
        input_channel_mask_ |= (1u << logical_slot);
    }
}

void Decoder::apply_native_output_channel_mapping() {
    requested_output_channel_mask_ = 0;
    output_channel_mask_ = 0;
    output_channel_slot_map_.clear();
    for (auto& ptr : output_desc_.channel_ptr)
        ptr = 0;

    const NativeChannelLayoutPlan requested_layout =
        build_requested_native_channel_layout(dsp_output_channels_, auro_metadata_);
    output_channel_slot_map_.assign(dsp_output_channels_, kInvalidChannelSlot);
    for (unsigned out_idx = 0; out_idx < requested_layout.slot_count; ++out_idx) {
        const std::uint32_t logical_slot = requested_layout.slots[out_idx];
        output_channel_slot_map_[out_idx] = logical_slot;
        if (logical_slot >= 27u)
            continue;
        requested_output_channel_mask_ |= (1u << logical_slot);
    }

    const std::uint32_t effective_output_mask = (requested_output_channel_mask_ | input_channel_mask_) & 0x7FFFFFFu;

    unsigned buffer_idx = 0;
    for (std::uint32_t logical_slot = 0; logical_slot < auro_codec_v3_ida::kAuroProcessorIoChannelPtrCount; ++logical_slot) {
        if (((effective_output_mask >> logical_slot) & 1u) == 0)
            continue;
        if (buffer_idx >= native_work_buffer_count_)
            break;
        output_desc_.channel_ptr[logical_slot] =
            reinterpret_cast<std::uint64_t>(native_work_buffer(buffer_idx++));
        output_channel_mask_ |= (1u << logical_slot);
    }
}

void Decoder::rebuild_native_io_descriptors() {
    std::memset(&input_desc_, 0, sizeof(input_desc_));
    std::memset(&output_desc_, 0, sizeof(output_desc_));

    input_desc_.total_size_bytes = block_size_;
    input_desc_.field_4 = sample_rate_;
    input_desc_.field_8 = kProcessorDescSampleBitsS24;
    input_desc_.layout_or_kind = kProcessorDescInputLayout;
    apply_native_input_channel_mapping();

    output_desc_.total_size_bytes = block_size_;
    output_desc_.field_4 = sample_rate_;
    output_desc_.field_8 = kProcessorDescSampleBitsS24;
    output_desc_.layout_or_kind = kProcessorDescOutputLayout;
    apply_native_output_channel_mapping();
    rebuild_auro_decoder_impl_state();
}

void Decoder::rebuild_auro_decoder_impl_state() {
    using auro_codec_v3_ida::kAuroDecoderImplObjectBytes;
    using auro_codec_v3_ida::kAuroDecoderImpl_off_ProcessorInstance;
    if (block_size_ == 0u || sample_rate_ == 0u) {
        auro_decoder_impl_blob_.clear();
        return;
    }
    if (auro_decoder_impl_blob_.size() != kAuroDecoderImplObjectBytes)
        auro_decoder_impl_blob_.assign(kAuroDecoderImplObjectBytes, 0u);

    auro3deng::AuroDecoderImplInitParams params{};
    params.sample_rate = sample_rate_;
    params.block_size_samples = static_cast<std::uint32_t>(block_size_);
    params.input_mask = native_config_state_.input_mask & kCodecV3ChannelMask;
    params.output_mask = native_config_state_.effective_output_mask & kCodecV3ChannelMask;
    for (std::uint32_t ch = 0; ch < auro_codec_v3_ida::kAuroProcessorIoChannelPtrCount; ++ch) {
        params.input_channel_ptrs[ch] = input_desc_.channel_ptr[ch];
        params.output_channel_ptrs[ch] = output_desc_.channel_ptr[ch];
    }
    if (!auro3deng::auro_decoder_impl_initialize_partial(auro_decoder_impl_blob_.data(), &params)) {
        auro_decoder_impl_blob_.clear();
        return;
    }
    *reinterpret_cast<Decoder**>(
        auro_decoder_impl_blob_.data() + kAuroDecoderImpl_off_ProcessorInstance) = this;
}

std::int64_t Decoder::codec_v3_processor_process_bridge(
    std::uint8_t* processor_base,
    const auro3deng::ProcessorIOBufferDesc* in_desc,
    const auro3deng::ProcessorIOBufferDesc* out_desc) {
    (void)in_desc;
    (void)out_desc;
    if (!processor_base)
        return static_cast<std::int64_t>(auro3deng::kProcessorProcessErrNull);
    auto* self = *reinterpret_cast<Decoder**>(processor_base);
    if (!self)
        return static_cast<std::int64_t>(auro3deng::kProcessorProcessErrNull);
    self->run_codec_v3_partial_step();
    return 0;
}

void Decoder::rebuild_native_config_state() {
    native_config_state_ = {};
    if (block_size_ == 0)
        return;

    const std::uint64_t block_samples = static_cast<std::uint64_t>(block_size_);
    native_config_state_.block_bits = block_samples;
    native_config_state_.block_words = static_cast<std::uint32_t>(block_samples >> 5);
    native_config_state_.stage0_count = 1u;
    const std::uint32_t div = static_cast<std::uint32_t>((block_samples + 1023u) / block_samples);
    native_config_state_.stage1_count = div + 1u;
    native_config_state_.buffer_count = static_cast<std::uint64_t>(div + 2u);
    native_config_state_.input_bytes_unit = auro_codec_v3_ida::kAuroDecoderImpl_expect_bytes_unit;

    native_config_state_.input_mask = input_channel_mask_ & 0x7FFFFFFu;
    native_config_state_.requested_output_mask = requested_output_channel_mask_ & 0x7FFFFFFu;
    native_config_state_.effective_output_mask = output_channel_mask_ & 0x7FFFFFFu;

    native_config_state_.input_layout_dimension = layout_dimension_27(native_config_state_.input_mask);
    native_config_state_.requested_output_layout_dimension =
        layout_dimension_27(native_config_state_.requested_output_mask);
    native_config_state_.effective_output_layout_dimension =
        layout_dimension_27(native_config_state_.effective_output_mask);
    native_config_state_.input_has_height_layer =
        (native_config_state_.input_mask & auro_codec_v3_ida::kAuroChannelMaskHeightLayer) != 0;
    native_config_state_.requested_output_has_height_layer =
        (native_config_state_.requested_output_mask & auro_codec_v3_ida::kAuroChannelMaskHeightLayer) != 0;
    native_config_state_.effective_output_has_height_layer =
        (native_config_state_.effective_output_mask & auro_codec_v3_ida::kAuroChannelMaskHeightLayer) != 0;
    native_config_state_.input_mask_count = mask_count_27(native_config_state_.input_mask);
    native_config_state_.requested_output_mask_count = mask_count_27(native_config_state_.requested_output_mask);
    native_config_state_.effective_output_mask_count = mask_count_27(native_config_state_.effective_output_mask);
    native_config_state_.extra_flags = 0u;
    native_config_state_.input_is_subset_of_effective_output =
        ((native_config_state_.input_mask & native_config_state_.effective_output_mask)
         == native_config_state_.input_mask);
}

void Decoder::rebuild_native_a3deng_static_configuration_state() {
    native_a3deng_static_configuration_ = {};
    native_a3deng_static_configuration_.pipeline_audio_block_size = block_size_;
    native_a3deng_static_configuration_.smoothing_s = 0.0f;
    native_a3deng_static_configuration_.hdmi_carrier_valid = true;
    native_a3deng_static_configuration_.hdmi_allow_2_0 = true;
    native_a3deng_static_configuration_.hdmi_allow_2_1 = true;
    native_a3deng_static_configuration_.hdmi_allow_5_1 = true;
    native_a3deng_static_configuration_.hdmi_allow_7_1 = true;
    native_a3deng_static_configuration_.hdmi_quality = 3u;
    native_a3deng_static_configuration_.output_mode = a3deng_output_mode_;
    native_a3deng_static_configuration_.api_available = true;
    native_a3deng_static_configuration_.instance_created = block_size_ != 0u;
}

void Decoder::rebuild_native_runtime_configuration_state() {
    native_runtime_configuration_ = {};
    constexpr std::uint32_t decoder_mode = 2u;

    native_runtime_configuration_.auro_update_is_stereo_device = stereo_device_connected_;
    native_runtime_configuration_.auro_update_headset_connected = headphone_connected_;
    native_runtime_configuration_.auro_update_decoder_mode = decoder_mode;
    native_runtime_configuration_.auro_update_output_sample_type = auro_engine_v4_ida::kOutputSampleTypeInt32;
    native_runtime_configuration_.auro_update_output_bit_depth = auro_engine_v4_ida::kOutputBitDepthInt24;
    native_runtime_configuration_.auro_update_output_layout_mask = requested_output_channel_mask_ & 0x7FFFFFFu;
    native_runtime_configuration_.auro_update_pcm_input_layout_mask = input_channel_mask_ & 0x7FFFFFFu;
    native_runtime_configuration_.auro_update_pcm_input_sample_rate = sample_rate_;
    native_runtime_configuration_.auro_update_pcm_input_sample_type = auro_engine_v4_ida::kOutputSampleTypeInt32;
    native_runtime_configuration_.auro_update_channels_backs_before_surrounds = false;
    native_runtime_configuration_.auro_update_virtualization_enabled = virtualizer_mode_ == 0u;
    native_runtime_configuration_.auro_update_listening_mode_auro3d = listening_mode_auro3d_;
    native_runtime_configuration_.auro_update_abr_mode_enabled =
        decoder_mode != 2u && !stereo_device_connected_;
    native_runtime_configuration_.auro_update_hp_user_preset = room_preset_;
    native_runtime_configuration_.auro_update_hp_hrtf_preset = hrtf_preset_;

    native_runtime_configuration_.virtualizer_mode = virtualizer_mode_;
    native_runtime_configuration_.room_preset = room_preset_;
    native_runtime_configuration_.hrtf_preset = hrtf_preset_;
    native_runtime_configuration_.headphone_connected = headphone_connected_;
    native_runtime_configuration_.stereo_device_connected = stereo_device_connected_;
    native_runtime_configuration_.dynamic_request_flag =
        native_runtime_configuration_.virtualizer_mode == 0u;
    native_runtime_configuration_.dynamic_headphone_flag =
        listening_mode_auro3d_;
    native_runtime_configuration_.target_device = derive_target_device_a3deng(
        decoder_mode,
        native_runtime_configuration_.stereo_device_connected,
        native_runtime_configuration_.headphone_connected);
    native_runtime_configuration_.is_abr =
        decoder_mode != 2u && !native_runtime_configuration_.stereo_device_connected;
    native_runtime_configuration_.output_audio_configuration_bits =
        static_cast<std::uint16_t>((native_runtime_configuration_.stereo_device_connected ? 0x100u : 0u)
                                   | (native_runtime_configuration_.headphone_connected ? 0x001u : 0u));
    native_runtime_configuration_.effective_virtualizer_mode = derive_effective_virtualizer_mode_a3deng(
        native_runtime_configuration_.dynamic_request_flag,
        native_runtime_configuration_.stereo_device_connected);
    native_runtime_configuration_.derived_listening_mode = derive_listening_mode_a3deng(
        native_runtime_configuration_.stereo_device_connected,
        native_runtime_configuration_.dynamic_headphone_flag);
}

void Decoder::rebuild_native_dynamic_parameters_state() {
    native_dynamic_parameters_ = {};
    rebuild_native_dynamic_parameters_state_from_runtime(
        native_runtime_configuration_,
        &native_dynamic_parameters_);
}

void Decoder::rebuild_native_a3deng_render_state() {
    native_a3deng_render_state_ = {};
    native_a3deng_render_state_.decoder_mode = 2u;
    native_a3deng_render_state_.input_sample_rate = sample_rate_;
    native_a3deng_render_state_.input_sample_type = auro_engine_v4_ida::kOutputSampleTypeInt32;
    native_a3deng_render_state_.input_channel_mask = input_channel_mask_ & 0x7FFFFFFu;
    native_a3deng_render_state_.input_channel_count =
        auro3deng::auro_channel_Mask_count(native_a3deng_render_state_.input_channel_mask, 0, 0);
    native_a3deng_render_state_.input_block_count = derive_a3deng_push_block_count(
        auro_engine_v4_ida::kA3DENG_constructor_pipeline_block_size,
        native_a3deng_render_state_.input_sample_rate,
        native_a3deng_render_state_.decoder_mode,
        a3deng_output_mode_);
    native_a3deng_render_state_.input_bytes_per_block = a3deng_push_part_byte_count(
        native_a3deng_render_state_.input_block_count,
        native_a3deng_render_state_.input_channel_mask,
        auro_engine_v4_ida::kOutputBitDepthInt24);
    native_a3deng_render_state_.output_sample_rate =
        derive_a3deng_output_sample_rate(sample_rate_, native_a3deng_render_state_.decoder_mode);
    native_a3deng_render_state_.output_channel_mask = requested_output_channel_mask_ & 0x7FFFFFFu;
    native_a3deng_render_state_.output_channel_count =
        auro3deng::auro_channel_Mask_count(native_a3deng_render_state_.output_channel_mask, 0, 0);
    native_a3deng_render_state_.output_sample_type = auro_engine_v4_ida::kOutputSampleTypeInt32;
    native_a3deng_render_state_.output_bit_depth = auro_engine_v4_ida::kOutputBitDepthInt24;
    native_a3deng_render_state_.output_block_count = 0u;
    native_a3deng_render_state_.pipeline_audio_block_size =
        auro_engine_v4_ida::kA3DENG_constructor_pipeline_block_size;
    native_a3deng_render_state_.pruned_output_info_valid = false;
    native_a3deng_render_state_.pruned_output_channel_mask =
        native_a3deng_render_state_.pruned_output_info_valid
            ? native_a3deng_render_state_.output_channel_mask
            : 0u;
    native_a3deng_render_state_.pruned_output_channel_count =
        auro3deng::auro_channel_Mask_count(
            native_a3deng_render_state_.pruned_output_channel_mask,
            0,
            0);
    native_a3deng_render_state_.pruned_output_max_sample =
        native_a3deng_render_state_.pruned_output_info_valid
            ? native_a3deng_render_state_.pipeline_audio_block_size
            : 0u;

    const NativeA3dengOutputInfoModel output_info{
        native_a3deng_render_state_.output_block_count,
        native_a3deng_render_state_.pipeline_audio_block_size,
    };
    native_a3deng_render_state_.output_info_packed = output_info.packed();
    native_a3deng_render_state_.render_available = output_info.render_available();
    native_a3deng_render_state_.render_bytes_per_block = a3deng_pop_total_byte_count(
        output_info,
        native_a3deng_render_state_.output_channel_mask,
        native_a3deng_render_state_.output_sample_type,
        native_a3deng_render_state_.output_bit_depth);
    native_a3deng_render_state_.maximum_output_bytecount_packed =
        a3deng_maximum_output_bytecount_packed(
            output_info,
            native_a3deng_render_state_.output_channel_mask,
            native_a3deng_render_state_.output_bit_depth);
    native_a3deng_render_state_.maximum_output_bytecount =
        static_cast<std::size_t>(native_a3deng_render_state_.maximum_output_bytecount_packed & 0xFFFFFFFFULL);
    native_a3deng_render_state_.jni_maximum_output_bytecount_return =
        (native_a3deng_render_state_.maximum_output_bytecount_packed & 0xFF00000000ULL) == 0
            ? -1
            : static_cast<std::int64_t>(native_a3deng_render_state_.maximum_output_bytecount);
    native_a3deng_render_state_.pop_return_bytes =
        native_a3deng_render_state_.render_available
            ? static_cast<std::int64_t>(native_a3deng_render_state_.render_bytes_per_block)
            : -1;

    if (!a3deng_partial_blob_constructed_ || a3deng_partial_blob_.empty())
        return;

    std::uint8_t* a3deng_base = a3deng_partial_blob_.data();
    native_a3deng_render_state_.input_block_count =
        auro3deng::auro_a3deng_v4_android_A3DENG_input_block_size_31b280_partial(a3deng_base);
    native_a3deng_render_state_.input_bytes_per_block = a3deng_push_part_byte_count(
        native_a3deng_render_state_.input_block_count,
        native_a3deng_render_state_.input_channel_mask,
        auro_engine_v4_ida::kOutputBitDepthInt24);
    native_a3deng_render_state_.output_sample_rate =
        auro3deng::auro_a3deng_v4_android_A3DENG_output_sample_rate_319920_partial(a3deng_base);
    native_a3deng_render_state_.output_channel_mask =
        auro3deng::auro_a3deng_v4_android_A3DENG_get_output_layout_31b330_partial(a3deng_base);
    native_a3deng_render_state_.output_channel_count =
        auro3deng::auro_a3deng_v4_android_A3DENG_get_output_channel_count_31b400_partial(a3deng_base);
    native_a3deng_render_state_.output_info_packed =
        auro3deng::auro_a3deng_v4_android_A3DENG_get_output_info_31b4e0_partial(a3deng_base);
    native_a3deng_render_state_.output_block_count =
        static_cast<std::uint32_t>(native_a3deng_render_state_.output_info_packed >> 32u);
    native_a3deng_render_state_.pipeline_audio_block_size =
        static_cast<std::uint32_t>(native_a3deng_render_state_.output_info_packed & 0xFFFFFFFFu);
    native_a3deng_render_state_.pruned_output_info_valid =
        auro3deng::a3deng_output_info_valid_31b4e0(native_a3deng_render_state_.output_info_packed);
    native_a3deng_render_state_.pruned_output_channel_mask =
        native_a3deng_render_state_.pruned_output_info_valid
            ? native_a3deng_render_state_.output_channel_mask
            : 0u;
    native_a3deng_render_state_.pruned_output_channel_count =
        auro3deng::auro_channel_Mask_count(native_a3deng_render_state_.pruned_output_channel_mask, 0, 0);
    native_a3deng_render_state_.pruned_output_max_sample =
        native_a3deng_render_state_.pruned_output_info_valid
            ? native_a3deng_render_state_.pipeline_audio_block_size
            : 0u;
    const NativeA3dengOutputInfoModel partial_output_info{
        native_a3deng_render_state_.output_block_count,
        native_a3deng_render_state_.pipeline_audio_block_size,
    };
    native_a3deng_render_state_.render_available = partial_output_info.render_available();
    native_a3deng_render_state_.render_bytes_per_block = a3deng_pop_total_byte_count(
        partial_output_info,
        native_a3deng_render_state_.output_channel_mask,
        native_a3deng_render_state_.output_sample_type,
        native_a3deng_render_state_.output_bit_depth);
    native_a3deng_render_state_.maximum_output_bytecount_packed =
        auro3deng::auro_a3deng_v4_android_A3DENG_get_maximum_output_bytecount_31b5a0_partial(a3deng_base);
    native_a3deng_render_state_.maximum_output_bytecount =
        static_cast<std::size_t>(native_a3deng_render_state_.maximum_output_bytecount_packed & 0xFFFFFFFFULL);
    native_a3deng_render_state_.jni_maximum_output_bytecount_return =
        (native_a3deng_render_state_.maximum_output_bytecount_packed & 0xFF00000000ULL) == 0
            ? -1
            : static_cast<std::int64_t>(native_a3deng_render_state_.maximum_output_bytecount);
    native_a3deng_render_state_.pop_return_bytes =
        native_a3deng_render_state_.render_available
            ? static_cast<std::int64_t>(native_a3deng_render_state_.render_bytes_per_block)
            : -1;
}

void Decoder::rebuild_a3deng_partial_blob() {
    constexpr std::size_t kA3dengObjectBytes = 0x2E0u;
    if (block_size_ == 0u) {
        if (a3deng_partial_blob_.size() == kA3dengObjectBytes) {
            auro3deng::auro_a3deng_v4_android_A3DENG_destroy_319ae0_partial(a3deng_partial_blob_.data());
        }
        a3deng_partial_blob_.clear();
        a3deng_partial_blob_constructed_ = false;
        a3deng_partial_blob_block_size_ = 0u;
        a3deng_partial_blob_output_mode_ = 0u;
        return;
    }
    if (a3deng_partial_blob_.size() != kA3dengObjectBytes)
        a3deng_partial_blob_.assign(kA3dengObjectBytes, 0u);
    const std::uint32_t bs = static_cast<std::uint32_t>(block_size_);
    const bool layout_changed =
        !a3deng_partial_blob_constructed_
        || a3deng_partial_blob_output_mode_ != a3deng_output_mode_;
    if (layout_changed) {
        if (a3deng_partial_blob_constructed_)
            auro3deng::auro_a3deng_v4_android_A3DENG_destroy_319ae0_partial(a3deng_partial_blob_.data());
        auro3deng::auro_a3deng_v4_android_A3DENG_construct_319ae0_partial(
            a3deng_partial_blob_.data(),
            auro_engine_v4_ida::kA3DENG_constructor_pipeline_block_size,
            a3deng_output_mode_);
        a3deng_partial_blob_constructed_ = true;
        a3deng_partial_blob_block_size_ = bs;
        a3deng_partial_blob_output_mode_ = a3deng_output_mode_;
    }
    const auto& r = native_runtime_configuration_;
    auro3deng::A3DENGSettingsFields318d60 f{};
    f.is_stereo_device = r.auro_update_is_stereo_device;
    f.headset_connected = r.auro_update_headset_connected;
    f.decoder_mode = r.auro_update_decoder_mode;
    f.output_layout_mask = r.auro_update_output_layout_mask;
    f.output_sample_type = r.auro_update_output_sample_type;
    f.output_bit_depth = r.auro_update_output_bit_depth;
    f.pcm_input_layout_mask = r.auro_update_pcm_input_layout_mask;
    f.pcm_input_sample_rate = r.auro_update_pcm_input_sample_rate;
    f.pcm_input_sample_type = r.auro_update_pcm_input_sample_type;
    f.channels_backs_before_surrounds = r.auro_update_channels_backs_before_surrounds;
    f.abr_mode_enabled = r.auro_update_abr_mode_enabled;
    f.virtualization_enabled = r.auro_update_virtualization_enabled;
    f.listening_mode_auro3d = r.auro_update_listening_mode_auro3d;
    f.hp_user_preset = r.auro_update_hp_user_preset;
    f.hp_hrtf_preset = r.auro_update_hp_hrtf_preset;
    (void)auro3deng::auro_a3deng_v4_android_A3DENG_AuroUpdate2_318d60_partial(a3deng_partial_blob_.data(), f);
    auro3deng::auro_a3deng_v4_android_A3DENG_set_codec_v3_pop_render_hook_partial(
        a3deng_partial_blob_.data(),
        &Decoder::a3deng_codec_v3_pop_render_trampoline,
        this);
}

std::uint64_t Decoder::a3deng_codec_v3_pop_render_trampoline(
    void* user,
    std::uint8_t* a3deng_base,
    std::uint8_t* output_bytes,
    std::uint32_t frames,
    std::uint32_t output_mask,
    std::uint32_t input_mask) {
    if (!user)
        return 0u;
    return static_cast<Decoder*>(user)->render_codec_v3_a3deng_pop(
        a3deng_base,
        output_bytes,
        frames,
        output_mask,
        input_mask);
}

std::uint64_t Decoder::render_codec_v3_a3deng_pop(
    std::uint8_t* a3deng_base,
    std::uint8_t* output_bytes,
    std::uint32_t frames,
    std::uint32_t output_mask,
    std::uint32_t input_mask) {
    if (!opened_ || !a3deng_base || !output_bytes || frames == 0u)
        return 0u;
    if (frames != static_cast<std::uint32_t>(block_size_))
        return 0u;

    if (!auro3deng::auro_a3deng_v4_android_A3DENG_consume_interleaved_input_to_planar_i32_partial(
            a3deng_base,
            frames,
            input_mask,
            input_desc_.channel_ptr,
            static_cast<std::uint32_t>(block_size_))) {
        return 0u;
    }

    const std::size_t plane_bytes = static_cast<std::size_t>(block_size_) * sizeof(std::int32_t);
    const std::uint32_t requested_mask = output_mask & kCodecV3ChannelMask;
    for (std::uint32_t slot = 0; slot < auro_codec_v3_ida::kAuroProcessorIoChannelPtrCount; ++slot) {
        if ((requested_mask & (1u << slot)) == 0u)
            continue;
        if (output_desc_.channel_ptr[slot] == 0u)
            continue;
        std::memset(
            reinterpret_cast<void*>(static_cast<std::uintptr_t>(output_desc_.channel_ptr[slot])),
            0,
            plane_bytes);
    }

    run_codec_v3_partial_step();

    std::uint32_t produced_mask = codec_v3_dispatch_.produced_output_mask & kCodecV3ChannelMask;
    const std::uint32_t passthrough_mask = requested_mask & input_mask & kCodecV3ChannelMask & ~produced_mask;
    for (std::uint32_t slot = 0; slot < auro_codec_v3_ida::kAuroProcessorIoChannelPtrCount; ++slot) {
        if ((passthrough_mask & (1u << slot)) == 0u)
            continue;
        if (input_desc_.channel_ptr[slot] == 0u || output_desc_.channel_ptr[slot] == 0u)
            continue;
        std::memcpy(
            reinterpret_cast<void*>(static_cast<std::uintptr_t>(output_desc_.channel_ptr[slot])),
            reinterpret_cast<const void*>(static_cast<std::uintptr_t>(input_desc_.channel_ptr[slot])),
            plane_bytes);
    }

    const std::uint32_t output_sample_type = *reinterpret_cast<const std::uint32_t*>(
        a3deng_base + auro_engine_v4_ida::kA3DENG_off_output_sample_type_runtime);
    return auro3deng::a3deng_write_pruned_interleaved_from_planar_i32_partial(
        a3deng_base,
        output_bytes,
        frames,
        output_desc_.channel_ptr,
        output_sample_type);
}

bool Decoder::apply_native_dynamic_parameters_update() {
    const NativeDynamicParametersState previous_dynamic = native_dynamic_parameters_;
    rebuild_native_a3deng_static_configuration_state();
    rebuild_native_runtime_configuration_state();
    rebuild_native_dynamic_parameters_state();
    rebuild_a3deng_partial_blob();
    rebuild_native_a3deng_render_state();
    native_xinn_partial_ready_ = false;
    native_asc4he_partial_ready_ = false;
    return !native_dynamic_parameters_equal(previous_dynamic, native_dynamic_parameters_);
}

void Decoder::rebuild_native_xinn_partial_state() {
    native_xinn_partial_ready_ = false;
    native_xinn_partial_input_mask_ = 0u;
    native_xinn_partial_output_mask_ = 0u;
    native_xinn_partial_mode_ = 0u;
    native_xinn_step_state_.clear();
    native_xinn_sample_rate_words_.clear();
    native_xinn_float_span_storage_.clear();
    native_xinn_process_scratch_storage_.clear();
    native_xinn_block_records_storage_.clear();

    if (!kEnableAuroMaticXinNUpmix)
        return;
    if (block_size_ == 0)
        return;
    if ((block_size_ % 32u) != 0u)
        return;
    if (!legacy_auromatic_upmix_ && !native_config_state_.requested_output_has_height_layer)
        return;

    const std::uint32_t input_mask = native_config_state_.input_mask & 0x7FFFFFFu;
    std::uint32_t mode = auro3deng::xinn_prepare_mode_from_input_mask_portable(input_mask);
    const std::uint32_t requested_output_mask =
        native_config_state_.effective_output_mask & 0x7FFFFFFu;
    const std::uint32_t xinn_supported_additions = mode == 1u
        ? 0x6630u
        : (mode == 2u ? 0x7E00u : 0u);
    const std::uint32_t output_mask = legacy_auromatic_upmix_
        ? (input_mask | (requested_output_mask & xinn_supported_additions))
        : requested_output_mask;

    native_xinn_step_state_.assign(kNativeXinnStepStateBytes, 0u);
    native_xinn_sample_rate_words_.assign(4u, 0u);
    native_xinn_sample_rate_words_[1] = block_size_ / 32u;
    native_xinn_sample_rate_words_[2] = sample_rate_;
    native_xinn_float_span_storage_.assign(
        static_cast<std::size_t>(auro_engine_v4_ida::kChannelCount) * block_size_,
        0.0f);
    native_xinn_process_scratch_storage_.assign(0x900u / sizeof(float), 0.0f);
    native_xinn_block_records_storage_.assign(
        static_cast<std::size_t>(block_size_ / 32u) * 516u,
        0u);

    auto* step = native_xinn_step_state_.data();
    *reinterpret_cast<std::uint64_t*>(step + 8u) =
        reinterpret_cast<std::uint64_t>(native_xinn_sample_rate_words_.data());
    const std::uint64_t memory_args[3] = {
        reinterpret_cast<std::uint64_t>(step),
        0x900u,
        reinterpret_cast<std::uint64_t>(native_xinn_process_scratch_storage_.data()),
    };
    if (auro3deng::auro_a3deng_v4_pipeline_step_upmix_XinN_initialize_35b2c0_partial(
            reinterpret_cast<std::uint64_t>(step), memory_args) != 0) {
        return;
    }
    const std::uint64_t block_info = *reinterpret_cast<const std::uint64_t*>(
        step + kNativeXinnStepOffBlockInfoPtr);
    if (block_info != 0u) {
        *reinterpret_cast<std::uint64_t*>(
            static_cast<std::uintptr_t>(block_info + kNativeXinnBlockInfoOffRecords)) =
            reinterpret_cast<std::uint64_t>(native_xinn_block_records_storage_.data());
    }

    std::array<std::uint8_t, kNativeXinnPlanBlobBytes> plan{};
    std::array<std::uint8_t, kNativeXinnUpdateBlobBytes> update{};
    auro3deng::xinn_write_plan_update_blobs_portable(
        plan.data(),
        update.data(),
        input_mask,
        output_mask,
        mode,
        native_dynamic_parameters_.room_preset);
    std::int64_t prc = auro3deng::auro_a3deng_v4_pipeline_step_upmix_XinN_prepare_35b330_partial(
        reinterpret_cast<std::uint64_t>(step),
        plan.data(),
        update.data());
    if (prc != 0)
        return;

    native_xinn_partial_input_mask_ = input_mask;
    native_xinn_partial_output_mask_ = requested_output_mask;
    native_xinn_partial_mode_ = mode;
    native_xinn_partial_ready_ = true;
}

void Decoder::rebuild_native_asc4he_partial_state() {
    native_asc4he_partial_ready_ = false;
    native_asc4he_partial_input_mask_ = 0u;
    native_asc4he_partial_output_mask_ = 0u;
    native_asc4he_processor_state_.clear();
    native_asc4he_static_params_.clear();
    native_asc4he_float_span_storage_.clear();
    native_asc4he_channel_table_.clear();

    if (!kEnableSyntheticHeightFallback)
        return;
    if (block_size_ == 0u || (block_size_ % 32u) != 0u)
        return;
    if (!native_config_state_.requested_output_has_height_layer)
        return;
    const std::uint32_t output_mask = native_config_state_.effective_output_mask & 0x7FFFFFFu;
    const std::uint32_t input_mask = native_config_state_.input_mask & 0x7FFFFFFu;
    std::uint32_t required_input = 0u;
    if (auro3deng::auro_asc4he_v1_Processor_t_get_required_input_layout_53dd30_partial(
            0u,
            output_mask,
            &required_input) != 0) {
        return;
    }

    native_asc4he_processor_state_.assign(0x4F08u, 0u);
    native_asc4he_static_params_.assign(52u + 240u, 0u);
    auto* p = native_asc4he_static_params_.data();
    *reinterpret_cast<std::uint32_t*>(p + 0u) = 0u;
    *reinterpret_cast<std::uint32_t*>(p + 4u) = required_input;
    *reinterpret_cast<std::uint32_t*>(p + 8u) = output_mask;
    *reinterpret_cast<std::uint32_t*>(p + 12u) = sample_rate_;
    *reinterpret_cast<std::uint32_t*>(p + 48u) = 0u;
    if (!auro3deng::auro_asc4he_v1_Processor_t_construct_53de70_partial(
            native_asc4he_processor_state_.data(),
            p)) {
        native_asc4he_processor_state_.clear();
        native_asc4he_static_params_.clear();
        return;
    }

    native_asc4he_float_span_storage_.assign(
        static_cast<std::size_t>(auro_engine_v4_ida::kChannelCount) * block_size_,
        0.0f);
    native_asc4he_channel_table_.assign(2u + auro_engine_v4_ida::kChannelCount, 0u);
    native_asc4he_partial_input_mask_ = input_mask;
    native_asc4he_partial_output_mask_ = output_mask;
    native_asc4he_partial_ready_ = true;
}

void Decoder::rebuild_codec_v3_partial_state() {
    codec_v3_dispatch_ = {};
    codec_v3_requested_layout_ever_satisfied_ = false;
    codec_v3_dispatch_.format_word0 = native_config_state_.input_mask & kCodecV3ChannelMask;
    codec_v3_dispatch_.sample_rate = sample_rate_;
    codec_v3_dispatch_.block_size = static_cast<std::uint32_t>(block_size_);
    codec_v3_dispatch_.required_output_mask =
        native_config_state_.effective_output_mask & kCodecV3ChannelMask;
    // Host path: allow decide_decode unless explicitly gated off.
    codec_v3_dispatch_.decide_decode_gate = 1u;

    codec_v3_format_detector_ = {};
    codec_v3_format_detector_.blocks_per_call = block_size_ / 32u;
    codec_v3_format_detector_.allow_low_9bits = native_config_state_.extra_flags;
    codec_v3_format_detector_.sink_user = &codec_v3_dispatch_;
    codec_v3_format_detector_.sink_notify = format_detector_sink_notify_1056c0_bridge;

    codec_v3_sync_detector_ = {};
    codec_v3_sync_detector_.notify_ctx = &codec_v3_dispatch_;
    codec_v3_sync_detector_.notify = sync_detector_notify_105ee0_bridge;

    codec_v3_delay_line_ = {};
    codec_v3_delay_line_.samples_per_block = block_size_;
    // With host early DelayLine_advance after write, timelines start at 0 (not
    // stream_index(stage0) which underflows when absolute_cursor is still 0).
    codec_v3_format_detector_.processed_samples = 0;

    const std::uint32_t slot_count =
        std::max<std::uint32_t>(2u, static_cast<std::uint32_t>(native_config_state_.buffer_count));
    const std::uint32_t delay_line_input_count =
        std::max<std::uint32_t>(1u, native_config_state_.input_mask_count);
    const std::uint32_t delay_line_input_mask = native_config_state_.input_mask & kCodecV3ChannelMask;
    codec_v3_delay_line_slots_.assign(slot_count, {});
    codec_v3_delay_line_storage_.assign(
        static_cast<std::size_t>(slot_count) * delay_line_input_count * static_cast<std::size_t>(block_size_),
        0);

    std::size_t sample_offset = 0;
    for (std::uint32_t slot = 0; slot < slot_count; ++slot) {
        auto& dl_slot = codec_v3_delay_line_slots_[slot];
        for (std::uint32_t ch = 0; ch < kCodecV3ChannelCount; ++ch) {
            if (((delay_line_input_mask >> ch) & 1u) == 0u) {
                dl_slot.channel_ptr[ch] = 0u;
                continue;
            }
            dl_slot.channel_ptr[ch] = reinterpret_cast<std::uint64_t>(
                codec_v3_delay_line_storage_.data() + sample_offset);
            sample_offset += static_cast<std::size_t>(block_size_);
        }
    }
    codec_v3_delay_line_.ring_storage_base =
        reinterpret_cast<std::uint64_t>(codec_v3_delay_line_slots_.data());
    codec_v3_delay_line_.ring_slot_count = slot_count;
    rebuild_codec_v3_output_generator_state();
    // IDA FormatDetector+312: input FrameDeque (parser ingest), not ready deque.
    codec_v3_format_detector_.frame_deque_ptr = codec_v3_fake_frame_deque_storage_.empty()
        ? 0u
        : reinterpret_cast<std::uint64_t>(codec_v3_fake_frame_deque_storage_.data());
}

void Decoder::rebuild_codec_v3_output_generator_state() {
    codec_v3_output_generator_state_.assign(kCodecV3OgStateBytes, 0);
    codec_v3_output_table_storage_.assign(kCodecV3OgOutTableBytes, 0);
    codec_v3_segment_ctx_storage_.assign(kCodecV3OgSegCtxBytes, 0);
    const std::uint32_t codec_v3_segment_capacity = auro3deng::memory_block_info_slot_count_106d20_partial(
        block_size_ != 0u ? static_cast<std::uint32_t>(block_size_) : 1u);
    codec_v3_segment_ranges_storage_.assign(
        static_cast<std::size_t>(codec_v3_segment_capacity) * kCodecV3SegmentStrideBytes, 0);
    codec_v3_segment_started_.assign(static_cast<std::size_t>(codec_v3_segment_capacity), 0u);
    codec_v3_segment_frame_ptrs_.assign(static_cast<std::size_t>(codec_v3_segment_capacity), 0u);
    const std::uint32_t input_deque_capacity = codec_v3_input_frame_deque_capacity(
        block_size_ != 0u ? static_cast<std::uint32_t>(block_size_) : 1u);
    const std::uint32_t ready_deque_capacity = codec_v3_ready_frame_deque_capacity(
        block_size_ != 0u ? static_cast<std::uint32_t>(block_size_) : 1u,
        std::max<std::uint32_t>(1u, native_config_state_.stage1_count));
    codec_v3_native_frame_deque_init(
        codec_v3_fake_frame_deque_storage_,
        codec_v3_fake_frame_deque_frame_storage_,
        input_deque_capacity);
    codec_v3_native_frame_deque_init(
        codec_v3_ready_frame_deque_storage_,
        codec_v3_ready_frame_deque_frame_storage_,
        ready_deque_capacity);
    codec_v3_fake_frame_channels_storage_.assign(kCodecV3ChannelCount * kCodecV3FakeFrameChannelBytes, 0);
    codec_v3_ready_frame_storage_.assign(kCodecV3FakeFrameBytes, 0);
    codec_v3_ready_frame_storage_next_.assign(kCodecV3FakeFrameBytes, 0);
    codec_v3_fake_frame_channel_words_.assign(kCodecV3ChannelCount * 8u, 0u);
    codec_v3_fake_frame_channel_ctx_storage_.assign(kCodecV3ChannelCount * 64u, 0);
    codec_v3_fake_parse_result_pool_state_.assign(kCodecV3ParseResultPoolStateBytes, 0);
    const std::uint32_t parse_pool_stage_count =
        std::max<std::uint32_t>(1u, native_config_state_.stage1_count);
    const std::uint32_t parse_pool_count =
        std::max<std::uint32_t>(1u, auro3deng::memory_parse_result_pool_count_106d20_partial(
            block_size_,
            parse_pool_stage_count));
    codec_v3_active_decode_probe_slots_ = std::min<std::uint32_t>(kCodecV3ChannelCount, parse_pool_count);
    const std::size_t parse_pool_bytes = static_cast<std::size_t>(
        auro3deng::parse_result_pool_required_additional_memory_107190_partial(
            parse_pool_count));
    codec_v3_fake_parse_result_pool_storage_.assign(parse_pool_bytes, 0);
    codec_v3_ready_parse_result_storage_.assign(
        static_cast<std::size_t>(ready_deque_capacity * kCodecV3FrameDequeCopiedSlotCapacity) * kCodecV3ParseResultBytes,
        0u);
    codec_v3_channel_parser_storage_.assign(kCodecV3ChannelCount * kCodecV3ChannelParserBytes, 0u);
    codec_v3_output_errors_storage_.assign(2u * block_size_, 0);
    codec_v3_output_scratch_storage_.assign(3u * block_size_, 0);
    // Host early-advance path: parser/OG cursors start at 0 and track committed
    // samples. Keep them as separate host fields (do not force equal each step).
    const std::uint32_t og_latency_blocks =
        std::max<std::uint32_t>(1u, native_config_state_.stage1_count);
    codec_v3_parser_timeline_cursor_ = 0;
    codec_v3_og_timeline_cursor_ = 0;

    auto* og = codec_v3_output_generator_state_.data();
    auto* seg_ctx = codec_v3_segment_ctx_storage_.data();
    codec_v3_fake_frame_storage_.assign(kCodecV3FakeFrameBytes, 0);
    codec_v3_fake_frame_storage_next_.assign(kCodecV3FakeFrameBytes, 0);
    auto* fake_frame = codec_v3_fake_frame_storage_.data();
    auto* fake_frame_next = codec_v3_fake_frame_storage_next_.data();
    if (!codec_v3_fake_parse_result_pool_state_.empty() && !codec_v3_fake_parse_result_pool_storage_.empty()) {
        (void)auro3deng::parse_result_pool_construct_1071b0_partial(
            reinterpret_cast<std::uint64_t>(codec_v3_fake_parse_result_pool_state_.data()),
            reinterpret_cast<std::uint64_t>(codec_v3_fake_parse_result_pool_storage_.data()),
            parse_pool_count);
    }

    *reinterpret_cast<std::uint64_t*>(og + kCodecV3OgOffPreSegmentsCb) = 0;
    *reinterpret_cast<std::uint64_t*>(og + kCodecV3OgOffPreSegmentsCtx) = 0;
    *reinterpret_cast<std::uint64_t*>(og + kCodecV3OgOffTotalSamples) = block_size_;
    *reinterpret_cast<std::uint32_t*>(og + 836u) = og_latency_blocks;
    *reinterpret_cast<std::uint64_t*>(og + kCodecV3OgOffTimelineCursor) = codec_v3_og_timeline_cursor_;
    *reinterpret_cast<std::uint64_t*>(og + kCodecV3OgOffDelayLinePtr) =
        reinterpret_cast<std::uint64_t>(&codec_v3_delay_line_);
    *reinterpret_cast<std::uint64_t*>(og + kCodecV3OgOffFrameDequePtr) =
        reinterpret_cast<std::uint64_t>(codec_v3_ready_frame_deque_storage_.data());
    *reinterpret_cast<std::uint64_t*>(og + kCodecV3OgOffSegmentCtxPtr) =
        reinterpret_cast<std::uint64_t>(seg_ctx);
    *reinterpret_cast<std::uint64_t*>(og + kCodecV3OgOffErrorsBuf) =
        reinterpret_cast<std::uint64_t>(codec_v3_output_errors_storage_.data());
    *reinterpret_cast<std::uint64_t*>(og + kCodecV3OgOffScratchBase) =
        reinterpret_cast<std::uint64_t>(codec_v3_output_scratch_storage_.data());
    *reinterpret_cast<std::uint32_t*>(og + kCodecV3OgOffCopyInputFlag) = 1u;
    *reinterpret_cast<std::uint32_t*>(og + kCodecV3OgOffOutputStatusFlag) = 0u;

    *reinterpret_cast<std::uint64_t*>(seg_ctx + kCodecV3SegCtxOffRangesBase) =
        reinterpret_cast<std::uint64_t>(codec_v3_segment_ranges_storage_.data());
    *reinterpret_cast<std::uint64_t*>(seg_ctx + kCodecV3SegCtxOffFrameStarted) =
        reinterpret_cast<std::uint64_t>(codec_v3_segment_started_.data());
    *reinterpret_cast<std::uint64_t*>(seg_ctx + kCodecV3SegCtxOffFramePtrs) =
        reinterpret_cast<std::uint64_t>(codec_v3_segment_frame_ptrs_.data());
    *reinterpret_cast<std::uint32_t*>(seg_ctx + kCodecV3SegCtxOffCount) = 0u;

    auro3deng::DecoderInitFakeFrameChannelsContext106ba0 init_ctx{};
    init_ctx.output_generator_base = og;
    init_ctx.fake_frame = fake_frame;
    init_ctx.fake_frame_next = fake_frame_next;
    init_ctx.fake_frame_channels_base = codec_v3_fake_frame_channels_storage_.data();
    init_ctx.fake_frame_channel_bytes = kCodecV3FakeFrameChannelBytes;
    init_ctx.fake_frame_channel_words_base = codec_v3_fake_frame_channel_words_.data();
    init_ctx.fake_frame_channel_ctx_base = codec_v3_fake_frame_channel_ctx_storage_.data();
    init_ctx.parse_result_pool_base = codec_v3_fake_parse_result_pool_state_.empty()
        ? 0u
        : reinterpret_cast<std::uint64_t>(codec_v3_fake_parse_result_pool_state_.data());
    init_ctx.input_mask = native_config_state_.input_mask;
    init_ctx.block_size = static_cast<std::uint32_t>(block_size_);
    init_ctx.active_decode_probe_slots = codec_v3_active_decode_probe_slots_;
    auro3deng::decoder_init_fake_frame_channels_106ba0_partial(&init_ctx);
    rebuild_auro_decoder_impl_state();
}

void Decoder::parser_rebind_frame_parse_results_103610(std::uint64_t frame_ptr) {
    auro3deng::ParserRebindContext103610 ctx{};
    ctx.parse_result_pool_base = reinterpret_cast<std::uint64_t>(codec_v3_fake_parse_result_pool_state_.data());
    ctx.channel_words_base = codec_v3_fake_frame_channel_words_.data();
    ctx.channel_words_count = codec_v3_fake_frame_channel_words_.size() / 8u;
    ctx.channel_ctx_base = codec_v3_fake_frame_channel_ctx_storage_.data();
    ctx.channel_ctx_count = codec_v3_fake_frame_channel_ctx_storage_.size() / 64u;
    ctx.channel_parser_base = codec_v3_channel_parser_storage_.data();
    ctx.channel_parser_count = codec_v3_channel_parser_storage_.size() / kCodecV3ChannelParserBytes;
    auro3deng::parser_rebind_frame_parse_results_103610_partial(frame_ptr, &ctx);
}

bool Decoder::run_native_xinn_partial_step(std::uint32_t copy_back_mask) {
    if (!kEnableAuroMaticXinNUpmix)
        return false;
    if (block_size_ == 0)
        return false;
    if (!legacy_auromatic_upmix_ && !native_config_state_.requested_output_has_height_layer)
        return false;
    if (!native_xinn_partial_ready_
        || native_xinn_partial_input_mask_ != (native_config_state_.input_mask & 0x7FFFFFFu)
        || native_xinn_partial_output_mask_ != (native_config_state_.effective_output_mask & 0x7FFFFFFu)) {
        rebuild_native_xinn_partial_state();
    }
    if (!native_xinn_partial_ready_)
        return false;

    std::array<void*, auro_engine_v4_ida::kChannelCount> span{};
    const std::size_t plane_stride = static_cast<std::size_t>(block_size_);
    for (std::uint32_t slot = 0; slot < auro_engine_v4_ida::kChannelCount; ++slot) {
        float* dst = native_xinn_float_span_storage_.data() + static_cast<std::size_t>(slot) * plane_stride;
        span[slot] = dst;
        std::fill(dst, dst + plane_stride, 0.0f);
        if (slot >= auro_codec_v3_ida::kAuroProcessorIoChannelPtrCount || output_desc_.channel_ptr[slot] == 0u)
            continue;
        const auto* src = reinterpret_cast<const std::int32_t*>(
            static_cast<std::uintptr_t>(output_desc_.channel_ptr[slot]));
        if (!src)
            continue;
        for (unsigned s = 0; s < block_size_; ++s)
            dst[s] = static_cast<float>(src[s]) * (1.0f / 8388608.0f);
    }

    const std::uint32_t subblocks = static_cast<std::uint32_t>(block_size_ / 32u);
    if (subblocks == 0u || (block_size_ % 32u) != 0u)
        return false;
    const std::int64_t rc = auro3deng::auro_a3deng_v4_pipeline_step_upmix_XinN_process_35b440_partial(
        reinterpret_cast<std::uint64_t>(native_xinn_step_state_.data()),
        span.data(),
        subblocks,
        nullptr,
        nullptr,
        auro3deng::auro_a3deng_v4_pipeline_step_upmix_XinN_reset_audio_state_35b730_partial);
    if (rc != 0) {
        native_xinn_partial_ready_ = false;
        return false;
    }

    copy_back_mask &= native_config_state_.effective_output_mask & 0x7FFFFFFu;
    for (std::uint32_t slot = 0; slot < auro_codec_v3_ida::kAuroProcessorIoChannelPtrCount; ++slot) {
        if ((copy_back_mask & (1u << slot)) == 0u)
            continue;
        if (output_desc_.channel_ptr[slot] == 0u)
            continue;
        auto* dst = reinterpret_cast<std::int32_t*>(
            static_cast<std::uintptr_t>(output_desc_.channel_ptr[slot]));
        const float* src = native_xinn_float_span_storage_.data() + static_cast<std::size_t>(slot) * plane_stride;
        if (!dst)
            continue;
        for (unsigned s = 0; s < block_size_; ++s)
            dst[s] = clamp_i32_to_pcm24(
                static_cast<std::int64_t>(std::lrintf(src[s] * 8388608.0f)));
    }
    return true;
}

bool Decoder::run_native_asc4he_partial_step(std::uint32_t copy_back_mask) {
    if (!kEnableSyntheticHeightFallback)
        return false;
    if (block_size_ == 0u || (block_size_ % 32u) != 0u)
        return false;
    if (!native_config_state_.requested_output_has_height_layer)
        return false;
    if (!native_asc4he_partial_ready_
        || native_asc4he_partial_input_mask_ != (native_config_state_.input_mask & 0x7FFFFFFu)
        || native_asc4he_partial_output_mask_ != (native_config_state_.effective_output_mask & 0x7FFFFFFu)) {
        rebuild_native_asc4he_partial_state();
    }
    if (!native_asc4he_partial_ready_)
        return false;

    const std::size_t plane_stride = static_cast<std::size_t>(block_size_);
    for (std::uint32_t slot = 0; slot < auro_engine_v4_ida::kChannelCount; ++slot) {
        float* dst = native_asc4he_float_span_storage_.data() + static_cast<std::size_t>(slot) * plane_stride;
        std::fill(dst, dst + plane_stride, 0.0f);
        if (slot >= auro_codec_v3_ida::kAuroProcessorIoChannelPtrCount)
            continue;
        std::uint64_t src_ptr = output_desc_.channel_ptr[slot];
        if (src_ptr == 0u && ((native_config_state_.input_mask >> slot) & 1u) != 0u)
            src_ptr = input_desc_.channel_ptr[slot];
        if (src_ptr == 0u)
            continue;
        const auto* src = reinterpret_cast<const std::int32_t*>(
            static_cast<std::uintptr_t>(src_ptr));
        if (!src)
            continue;
        for (unsigned s = 0; s < block_size_; ++s)
            dst[s] = static_cast<float>(src[s]);
    }

    native_asc4he_channel_table_[0] =
        (static_cast<std::uint64_t>(sample_rate_) << 32u) | 32u;
    native_asc4he_channel_table_[1] = 0u;
    const std::uint32_t subblocks = static_cast<std::uint32_t>(block_size_ / 32u);
    for (std::uint32_t b = 0; b != subblocks; ++b) {
        const std::size_t block_offset = static_cast<std::size_t>(b) * 32u;
        for (std::uint32_t slot = 0; slot < auro_engine_v4_ida::kChannelCount; ++slot) {
            float* p = native_asc4he_float_span_storage_.data()
                + static_cast<std::size_t>(slot) * plane_stride
                + block_offset;
            native_asc4he_channel_table_[2u + slot] =
                static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(p));
        }
        const std::int64_t rc = auro3deng::auro_asc4he_v1_Processor_process_53e030_partial(
            native_asc4he_processor_state_.data(),
            reinterpret_cast<std::uint32_t*>(native_asc4he_channel_table_.data()));
        if (rc != 0) {
            native_asc4he_partial_ready_ = false;
            return false;
        }
    }

    copy_back_mask &= native_config_state_.effective_output_mask & 0x7FFFFFFu;
    for (std::uint32_t slot = 0; slot < auro_codec_v3_ida::kAuroProcessorIoChannelPtrCount; ++slot) {
        if ((copy_back_mask & (1u << slot)) == 0u)
            continue;
        if (output_desc_.channel_ptr[slot] == 0u)
            continue;
        auto* dst = reinterpret_cast<std::int32_t*>(
            static_cast<std::uintptr_t>(output_desc_.channel_ptr[slot]));
        const float* src = native_asc4he_float_span_storage_.data() + static_cast<std::size_t>(slot) * plane_stride;
        if (!dst)
            continue;
        for (unsigned s = 0; s < block_size_; ++s)
            dst[s] = clamp_i32_to_pcm24(static_cast<std::int64_t>(std::lrintf(src[s])));
    }
    return true;
}

void Decoder::run_codec_v3_partial_step() {
    if (block_size_ == 0 || codec_v3_delay_line_.ring_storage_base == 0)
        return;

    const std::uint32_t input_mask = native_config_state_.input_mask & kCodecV3ChannelMask;
    std::uint64_t codec_v3_input_channel_ptrs[kCodecV3ChannelCount]{};
    std::uint64_t codec_v3_output_channel_ptrs[kCodecV3ChannelCount]{};
    for (std::uint32_t ch = 0; ch < auro_codec_v3_ida::kAuroProcessorIoChannelPtrCount; ++ch) {
        codec_v3_input_channel_ptrs[ch] = input_desc_.channel_ptr[ch];
        codec_v3_output_channel_ptrs[ch] = output_desc_.channel_ptr[ch];
    }
    // SyncDetector ORs LSBs across every non-null channel. Permanent silent pads
    // (e.g. zero LFE in 2.1/3ch mix3 WAV) must be null, not a zeroed buffer.
    // Also drop those bits from the FormatDetector/SyncDetector layout so frames
    // are not built with empty LFE slots that later CRC-fail in the parser.
    std::uint32_t live_input_mask = input_mask;
    for (std::uint32_t ch = 0; ch < auro_codec_v3_ida::kAuroProcessorIoChannelPtrCount; ++ch) {
        if (((live_input_mask >> ch) & 1u) == 0u)
            continue;
        const std::uint64_t ptr = codec_v3_input_channel_ptrs[ch];
        if (ptr == 0u) {
            live_input_mask &= ~(1u << ch);
            continue;
        }
        const auto* samples = reinterpret_cast<const std::int32_t*>(static_cast<std::uintptr_t>(ptr));
        bool any_nonzero = false;
        for (unsigned s = 0; s < block_size_; ++s) {
            if (samples[s] != 0) {
                any_nonzero = true;
                break;
            }
        }
        if (!any_nonzero) {
            codec_v3_input_channel_ptrs[ch] = 0u;
            live_input_mask &= ~(1u << ch);
        }
    }

    auro3deng::DecoderDispatchRunContextEb5a0 dispatch_ctx{};
    dispatch_ctx.dispatch = &codec_v3_dispatch_;
    dispatch_ctx.format_detector = &codec_v3_format_detector_;
    dispatch_ctx.sync_detector = &codec_v3_sync_detector_;
    dispatch_ctx.delay_line = &codec_v3_delay_line_;
    dispatch_ctx.sample_rate = sample_rate_;
    dispatch_ctx.block_size = static_cast<std::uint32_t>(block_size_);
    dispatch_ctx.input_mask = live_input_mask;
    dispatch_ctx.output_mask = native_config_state_.effective_output_mask & kCodecV3ChannelMask;
    dispatch_ctx.input_channel_ptrs_27 = codec_v3_input_channel_ptrs;
    dispatch_ctx.output_channel_ptrs_27 = codec_v3_output_channel_ptrs;
    dispatch_ctx.set_layout = reinterpret_cast<void (*)(void*, std::uint32_t)>(auro3deng::sync_detector_set_layout_105ee0_partial);
    dispatch_ctx.process_block = reinterpret_cast<void (*)(void*, const std::uint64_t*)>(auro3deng::sync_detector_process_block_106110_partial);
    dispatch_ctx.sync_user = &codec_v3_sync_detector_;

    SyncDetectorNotifyBridgeCtx sync_notify_ctx{};
    sync_notify_ctx.dispatch = &codec_v3_dispatch_;
    sync_notify_ctx.format_detector = &codec_v3_format_detector_;
    sync_notify_ctx.sync_detector = &codec_v3_sync_detector_;
    sync_notify_ctx.frame_deque_ptr = codec_v3_fake_frame_deque_storage_.empty()
        ? 0u
        : reinterpret_cast<std::uint64_t>(codec_v3_fake_frame_deque_storage_.data());
    codec_v3_sync_detector_.notify_ctx = &sync_notify_ctx;
    codec_v3_sync_detector_.notify = sync_detector_notify_frame_builder_105530_bridge;
    codec_v3_dispatch_.produced_output_mask = 0u;

    auro3deng::CodecV3IoBufferDescEb5a0 input_desc{};
    input_desc.total_samples = block_size_;
    input_desc.sample_rate = sample_rate_;
    input_desc.bits_per_sample = kProcessorDescSampleBitsS24;
    for (std::uint32_t ch = 0; ch < kCodecV3ChannelCount; ++ch)
        input_desc.channel_ptr[ch] = codec_v3_input_channel_ptrs[ch];

    auro3deng::ParserPayloadRefreshContext payload_ctx{};
    payload_ctx.frame_deque_ptr = codec_v3_fake_frame_deque_storage_.empty()
        ? 0u
        : reinterpret_cast<std::uint64_t>(codec_v3_fake_frame_deque_storage_.data());
    payload_ctx.output_generator_base = codec_v3_output_generator_state_.empty()
        ? 0u
        : reinterpret_cast<std::uint64_t>(codec_v3_output_generator_state_.data());
    payload_ctx.input_channel_limit = kCodecV3ChannelCount;
    payload_ctx.block_size = static_cast<std::uint32_t>(block_size_);
    payload_ctx.channel_words_base = codec_v3_fake_frame_channel_words_.data();
    payload_ctx.channel_words_count = codec_v3_fake_frame_channel_words_.size() / 8u;
    payload_ctx.channel_ctx_base = codec_v3_fake_frame_channel_ctx_storage_.data();
    payload_ctx.channel_ctx_count = codec_v3_fake_frame_channel_ctx_storage_.size() / 64u;

    DecoderStepBridgeCtx step_bridge{};
    step_bridge.dispatch = &codec_v3_dispatch_;
    step_bridge.delay_line = &codec_v3_delay_line_;
    step_bridge.output_generator_base = codec_v3_output_generator_state_.empty()
        ? nullptr
        : codec_v3_output_generator_state_.data();
    step_bridge.output_table_base = codec_v3_output_table_storage_.data();
    step_bridge.output_table_size = codec_v3_output_table_storage_.size();
    step_bridge.output_channel_ptrs_27 = codec_v3_output_channel_ptrs;
    step_bridge.input_channel_ptrs_27 = codec_v3_input_channel_ptrs;
    step_bridge.input_mask = input_mask;
    step_bridge.fake_frame_deque_base = codec_v3_fake_frame_deque_storage_.empty()
        ? nullptr
        : codec_v3_fake_frame_deque_storage_.data();
    step_bridge.ready_frame_deque_base = codec_v3_ready_frame_deque_storage_.empty()
        ? nullptr
        : codec_v3_ready_frame_deque_storage_.data();
    step_bridge.parse_result_pool_base = codec_v3_fake_parse_result_pool_state_.empty()
        ? nullptr
        : codec_v3_fake_parse_result_pool_state_.data();
    step_bridge.parser_slots_base = codec_v3_channel_parser_storage_.data();
    step_bridge.parser_slots_size = codec_v3_channel_parser_storage_.size();
    step_bridge.ready_parse_result_base = codec_v3_ready_parse_result_storage_.data();
    step_bridge.ready_parse_result_size = codec_v3_ready_parse_result_storage_.size();
    step_bridge.block_size = static_cast<std::uint64_t>(block_size_);
    step_bridge.parser_timeline_cursor_ptr = &codec_v3_parser_timeline_cursor_;
    step_bridge.og_timeline_cursor_ptr = &codec_v3_og_timeline_cursor_;
    const std::uint64_t sync_offset =
        (auro_metadata_.found && block_size_ != 0u)
            ? (auro_metadata_.sync_sample % block_size_)
            : 0u;
    step_bridge.output_timeline_delay =
        sync_offset != 0u
            ? (2u * static_cast<std::uint64_t>(block_size_) - sync_offset)
            : static_cast<std::uint64_t>(block_size_);
    step_bridge.parser_state_ptr = &codec_v3_parser_state_;
    step_bridge.produced_output_mask = &codec_v3_dispatch_.produced_output_mask;

    auro3deng::DecoderStepRunContext101800 step_ctx{};
    step_ctx.dispatch_ctx = &dispatch_ctx;
    step_ctx.parser_input_desc = &input_desc;
    step_ctx.payload_ctx = &payload_ctx;
    step_ctx.delay_line_ptr = reinterpret_cast<std::uint64_t>(&codec_v3_delay_line_);
    step_ctx.run_parser_stage = run_parser_stage_1034e0_bridge;
    step_ctx.parser_user = &step_bridge;
    step_ctx.run_output_stage = run_output_stage_1024a9_bridge;
    step_ctx.output_user = &step_bridge;
    (void)auro3deng::decoder_run_step_101800_partial(&step_ctx);
}

bool Decoder::validate_native_processor_io_model() const {
    auro3deng::ProcessorIoExpectDa9ae0 ex{};
    ex.in_layout = static_cast<std::int32_t>(kProcessorDescInputLayout);
    ex.in_mask = static_cast<std::int32_t>(native_config_state_.input_mask);
    ex.in_bytes_unit = static_cast<std::int32_t>(native_config_state_.input_bytes_unit);
    ex.in_field4 = static_cast<std::int32_t>(sample_rate_);
    ex.in_field8_when_layout1 = static_cast<std::int32_t>(kProcessorDescSampleBitsS24);

    ex.out_layout = static_cast<std::int32_t>(kProcessorDescOutputLayout);
    ex.out_mask = static_cast<std::int32_t>(native_config_state_.effective_output_mask);
    ex.out_bytes_unit = static_cast<std::int32_t>(native_config_state_.input_bytes_unit);
    ex.out_field4 = static_cast<std::int32_t>(sample_rate_);
    ex.out_field8_when_layout1 = static_cast<std::int32_t>(kProcessorDescSampleBitsS24);

    if (!native_config_state_.input_is_subset_of_effective_output)
        return false;
    return auro3deng::processor_process_validate_da9ae0(&input_desc_, &output_desc_, ex) == 0;
}

bool probe_wav_pcm_s24le(
    const std::string& path,
    std::size_t& pcm_byte_offset,
    std::size_t& pcm_byte_length,
    std::uint32_t& sample_rate,
    std::uint16_t& channels,
    std::string& err) {
    err.clear();
    pcm_byte_offset = 0;
    pcm_byte_length = 0;
    sample_rate = 0;
    channels = 0;
    std::uint32_t channel_mask = 0;

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        err = "cannot open file";
        return false;
    }

    std::vector<std::uint8_t> buf;
    constexpr std::size_t kStep = 262144;
    for (;;) {
        const std::size_t old = buf.size();
        buf.resize(old + kStep);
        in.read(reinterpret_cast<char*>(buf.data() + old), static_cast<std::streamsize>(kStep));
        const std::streamsize got = in.gcount();
        buf.resize(old + static_cast<std::size_t>(got));
        if (try_parse_wav_s24le(buf.data(), buf.size(), pcm_byte_offset, pcm_byte_length, channels, sample_rate, channel_mask, err))
            return true;
        if (got == 0)
            break;
        if (err == "no data chunk" || err == "truncated chunk")
            continue;
        if (err == "not RIFF/WAVE" && buf.size() < 12)
            continue;
        return false;
    }
    if (err.empty())
        err = "WAV: no data chunk";
    return false;
}

bool resolve_s24le_interleaved_pcm_region(
    const std::string& path,
    bool raw,
    std::uint32_t raw_sample_rate,
    unsigned raw_channels,
    std::size_t& pcm_byte_offset,
    std::size_t& pcm_byte_length,
    std::uint32_t& sample_rate_hz,
    unsigned& channel_count,
    std::string& err) {
    err.clear();
    pcm_byte_offset = 0;
    pcm_byte_length = 0;
    sample_rate_hz = 0;
    channel_count = 0;

    std::uint16_t ch16 = 0;
    if (!raw) {
        if (!probe_wav_pcm_s24le(path, pcm_byte_offset, pcm_byte_length, sample_rate_hz, ch16, err))
            return false;
    } else {
        if (raw_channels == 0 || raw_sample_rate == 0) {
            err = "raw stream needs --rate and --channels";
            return false;
        }
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            err = "cannot open file";
            return false;
        }
        in.seekg(0, std::ios::end);
        const auto sz = in.tellg();
        if (sz < 0) {
            err = "file size";
            return false;
        }
        pcm_byte_offset = 0;
        pcm_byte_length = static_cast<std::size_t>(sz);
        sample_rate_hz = raw_sample_rate;
        ch16 = static_cast<std::uint16_t>(raw_channels);
    }

    channel_count = static_cast<unsigned>(ch16);
    if (channel_count == 0) {
        err = "invalid channel count";
        return false;
    }
    const std::uint64_t unit_u64 = static_cast<std::uint64_t>(channel_count) * 3u;
    if (unit_u64 > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        err = "invalid channel count";
        return false;
    }
    const std::size_t unit = static_cast<std::size_t>(unit_u64);
    if (pcm_byte_length % unit != 0) {
        err = "PCM byte length not multiple of 3*channels";
        return false;
    }
    return true;
}

bool read_pcm24_interleaved_frames_i32(
    const std::string& path,
    std::size_t pcm_byte_offset,
    std::size_t pcm_byte_length,
    unsigned channels,
    std::uint64_t first_frame,
    std::size_t frame_count,
    std::vector<std::int32_t>& interleaved_out,
    std::string& err) {
    err.clear();
    interleaved_out.clear();
    if (channels == 0 || frame_count == 0) {
        err = "invalid frame read args";
        return false;
    }
    const std::uint64_t unit_u64 = static_cast<std::uint64_t>(channels) * 3u;
    const std::uint64_t need_u64 = static_cast<std::uint64_t>(frame_count) * unit_u64;
    const std::uint64_t start_u64 = static_cast<std::uint64_t>(pcm_byte_offset) + first_frame * unit_u64;
    const std::uint64_t region_end = static_cast<std::uint64_t>(pcm_byte_offset) + static_cast<std::uint64_t>(pcm_byte_length);
    if (start_u64 + need_u64 > region_end || start_u64 + need_u64 < start_u64) {
        err = "read past PCM region";
        return false;
    }
    const std::size_t max_sz = std::numeric_limits<std::size_t>::max();
    if (need_u64 > static_cast<std::uint64_t>(max_sz)) {
        err = "window too large";
        return false;
    }
    const std::size_t need = static_cast<std::size_t>(need_u64);

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        err = "cannot open file";
        return false;
    }
    in.seekg(0, std::ios::end);
    const auto fsize = in.tellg();
    if (fsize < 0 || static_cast<std::uint64_t>(fsize) < start_u64 + need_u64) {
        err = "file too small for requested window";
        return false;
    }
    in.seekg(static_cast<std::streamoff>(start_u64));
    std::vector<std::uint8_t> rawb(need);
    if (!in.read(reinterpret_cast<char*>(rawb.data()), static_cast<std::streamsize>(need))) {
        err = "read error";
        return false;
    }

    interleaved_out.resize(frame_count * static_cast<std::size_t>(channels));
    const std::uint8_t* p = rawb.data();
    for (std::size_t i = 0; i < frame_count * static_cast<std::size_t>(channels); ++i) {
        const int b0 = p[0];
        const int b1 = p[1];
        const int b2 = p[2];
        p += 3;
        int v = b0 | (b1 << 8) | (b2 << 16);
        if (v & 0x800000)
            v |= ~0xFFFFFF;
        interleaved_out[i] = v;
    }
    return true;
}

bool load_all_pcm_s24le_interleaved_i32(
    const std::string& path,
    bool raw,
    uint32_t raw_sample_rate,
    unsigned raw_channels,
    std::vector<std::int32_t>& interleaved_out,
    DecoderConfig& cfg_out,
    std::string& err) {
    err.clear();
    interleaved_out.clear();
    cfg_out = {};

    std::vector<std::uint8_t> prefix;
    if (!read_file_prefix(path, 12u, prefix, err))
        return false;
    if (raw && is_ffmpeg_audio_input(prefix)) {
        err = "compressed/container input cannot be used with --raw";
        return false;
    }

    std::vector<std::uint8_t> file_bytes;
    if (!raw && is_ffmpeg_audio_input(prefix)) {
        if (!decode_supported_audio_to_pcm24_wav_bytes(path, file_bytes, err))
            return false;
    } else {
        if (!read_file_bytes(path, file_bytes, err))
            return false;
    }

    std::size_t pcm_b = 0;
    std::size_t pcm_len = 0;
    std::uint16_t ch = 0;
    std::uint32_t rate = 0;
    std::uint32_t channel_mask = 0;

    const bool riff = file_bytes.size() >= 12 && std::memcmp(file_bytes.data(), "RIFF", 4) == 0;

    if (!raw && riff && try_parse_wav_s24le(file_bytes.data(), file_bytes.size(), pcm_b, pcm_len, ch, rate, channel_mask, err)) {
        // ok
    } else if (raw) {
        if (raw_channels == 0 || raw_sample_rate == 0) {
            err = "raw mode needs channel count and sample rate";
            return false;
        }
        pcm_b = 0;
        pcm_len = file_bytes.size();
        ch = static_cast<std::uint16_t>(raw_channels);
        rate = raw_sample_rate;
    } else {
        if (err.empty())
            err = "need WAV/FLAC PCM24 or --raw with --rate/--channels";
        return false;
    }

    const unsigned ch_n = static_cast<unsigned>(ch);
    const std::size_t unit = static_cast<std::size_t>(ch_n) * 3u;
    if (unit == 0 || pcm_len % unit != 0) {
        err = "PCM byte length not multiple of 3*channels";
        return false;
    }

    const std::size_t frames = pcm_len / unit;
    interleaved_out.resize(frames * ch_n);
    const std::uint8_t* p = file_bytes.data() + pcm_b;
    for (std::size_t i = 0; i < frames * ch_n; ++i) {
        const int b0 = p[0];
        const int b1 = p[1];
        const int b2 = p[2];
        p += 3;
        int v = b0 | (b1 << 8) | (b2 << 16);
        if (v & 0x800000)
            v -= 1 << 24;
        interleaved_out[i] = v;
    }

    cfg_out.sample_rate = rate;
    cfg_out.channels = ch;
    cfg_out.bits_per_sample = 24;
    cfg_out.channel_mask = channel_mask;
    cfg_out.block_size = 0;
    return true;
}

DecodeError Decoder::open(const std::string& path) {
    // IDA 0x101760: Decoder construct starts with CRC_t_init.
    auro3deng::decoder_crc_t_init_106f00();
    file_bytes_.clear();
    pcm_begin_ = 0;
    pcm_length_ = 0;
    read_pos_ = 0;
    opened_ = false;
    planar_scratch_.clear();
    native_xinn_step_state_.clear();
    native_xinn_sample_rate_words_.clear();
    native_xinn_float_span_storage_.clear();
    native_xinn_process_scratch_storage_.clear();
    native_xinn_block_records_storage_.clear();
    native_xinn_partial_ready_ = false;
    native_xinn_partial_input_mask_ = 0u;
    native_xinn_partial_output_mask_ = 0u;
    native_xinn_partial_mode_ = 0u;
    native_asc4he_processor_state_.clear();
    native_asc4he_static_params_.clear();
    native_asc4he_float_span_storage_.clear();
    native_asc4he_channel_table_.clear();
    native_asc4he_partial_ready_ = false;
    native_asc4he_partial_input_mask_ = 0u;
    native_asc4he_partial_output_mask_ = 0u;
    dsp_output_channels_ = 0;
    dsp_clipped_samples_ = 0;
    input_wav_channel_mask_ = 0;
    auro_metadata_ = {};
    legacy_auromatic_upmix_ = false;

    std::string input_err;
    std::vector<std::uint8_t> prefix;
    if (!read_file_prefix(path, 12u, prefix, input_err))
        return DecodeError::IoError;
    if (raw_forced_ && is_ffmpeg_audio_input(prefix))
        return DecodeError::BadInput;
    if (!raw_forced_ && is_ffmpeg_audio_input(prefix)) {
        if (!decode_supported_audio_to_pcm24_wav_bytes(path, file_bytes_, input_err))
            return DecodeError::BadInput;
    } else {
        if (!read_file_bytes(path, file_bytes_, input_err))
            return DecodeError::IoError;
    }
    std::string wav_err;
    std::size_t pcm_b = 0;
    std::size_t pcm_len = 0;
    uint16_t wav_ch = 0;
    uint32_t wav_rate = 0;
    uint32_t wav_channel_mask = 0;

    const bool riff = file_bytes_.size() >= 12 && std::memcmp(file_bytes_.data(), "RIFF", 4) == 0;

    if (!raw_forced_ && riff
        && try_parse_wav_s24le(
            file_bytes_.data(), file_bytes_.size(), pcm_b, pcm_len, wav_ch, wav_rate, wav_channel_mask, wav_err)) {
        pcm_begin_ = pcm_b;
        pcm_length_ = pcm_len;
        sample_rate_ = wav_rate;
        channel_count_ = wav_ch;
        input_wav_channel_mask_ = wav_channel_mask;
        // APK/JADX auroenginev4 and live Frida/Kahlo on libauro.so:
        // AuroInitialize uses 832-frame blocks; AuroPush carries
        // 832 * channels * 3 bytes for s24le input, not 1024-frame legacy blocks.
        block_size_ = block_request_ != 0 ? block_request_ : kDefaultJniBlockSize;
    } else if (raw_forced_) {
        pcm_begin_ = 0;
        pcm_length_ = file_bytes_.size();
        block_size_ = block_request_ != 0 ? block_request_ : kDefaultJniBlockSize;
        if (sample_rate_ == 0 || channel_count_ == 0 || block_size_ == 0)
            return DecodeError::BadInput;
        input_wav_channel_mask_ = 0;
    } else {
        return DecodeError::BadInput;
    }

    const std::size_t sample_frame_b = static_cast<std::size_t>(channel_count_) * 3u;
    if (sample_frame_b == 0 || pcm_length_ % sample_frame_b != 0)
        return DecodeError::BadInput;

    read_pos_ = pcm_begin_;
    opened_ = true;
    // По умолчанию (как JNI путь из libauro3d.so) используем stereo.
    // При явном запросе разрешаем multichannel export через native slot layout.
    auro_metadata_ = scan_auro_metadata_pcm24(file_bytes_, pcm_begin_, pcm_length_, channel_count_);
    if (!auro_metadata_.found) {
        const bool supported_legacy_target =
            (dsp_output_channels_req_ == 6u && channel_count_ >= 2u && channel_count_ <= 3u)
            || (dsp_output_channels_req_ == 10u && channel_count_ >= 2u && channel_count_ <= 6u)
            || (dsp_output_channels_req_ == 12u && channel_count_ == 8u);
        if (!supported_legacy_target || dsp_output_channels_req_ <= channel_count_) {
            opened_ = false;
            return DecodeError::NotImplemented;
        }
        legacy_auromatic_upmix_ = true;
    }
    // The metadata block size is the sync/instruction interval, not the host
    // processing block size. The native Processor requires the configured host
    // block to remain a multiple of 32 (JNI uses 832 samples).
    // WAV channel_count is the container/layout width (often includes height slots).
    // carrier_channels is the encoded subset; mismatch is expected for height decode.
    const std::size_t frame_b = static_cast<std::size_t>(block_size_) * sample_frame_b;
    if (frame_b == 0) {
        opened_ = false;
        return DecodeError::BadInput;
    }
    if (!auro_codec_v3_is_sample_rate_supported(sample_rate_)
        || !auro_codec_v3_is_unit_block_size_supported(block_size_)) {
        opened_ = false;
        return DecodeError::BadInput;
    }
    constexpr std::uint32_t kAuroLayout7_1_5H_1T = 0x7FBFu;
    constexpr std::uint32_t kAuroCarrier7_1 = 0x01BFu;
    constexpr std::uint32_t kAuroDirect7_1_2H = 0x07BFu;
    const bool direct_7_1_2h_output =
        auro_metadata_.found
        &&
        auro_metadata_.layout_id == kAuroLayout7_1_5H_1T
        && auro_metadata_.carrier_layout_id == kAuroCarrier7_1;
    const unsigned native_direct_output_channels = legacy_auromatic_upmix_
        ? dsp_output_channels_req_
        : (direct_7_1_2h_output
            ? mask_count_27(kAuroDirect7_1_2H)
            : auro_metadata_.output_channels);
    const unsigned auto_output_channels = legacy_auromatic_upmix_
        ? dsp_output_channels_req_
        : (kEnableAuroMaticXinNUpmix
            ? auro_metadata_.output_channels
            : native_direct_output_channels);

    dsp_output_channels_ = dsp_output_channels_req_;
    const bool explicit_layout_supported =
        dsp_output_channels_ == native_direct_output_channels
        || (kEnableAuroMaticXinNUpmix && dsp_output_channels_ == auto_output_channels);
    if (dsp_output_channels_ != 0 && !explicit_layout_supported) {
        opened_ = false;
        return DecodeError::NotImplemented;
    }
    if (dsp_output_channels_ == 0
        && auto_output_channels > 0
        && auto_output_channels <= kCurrentNativeExportChannelLimit) {
        dsp_output_channels_ = auto_output_channels;
    }
    if (dsp_output_channels_ == 0) {
        opened_ = false;
        return DecodeError::BadInput;
    }
    if (channel_count_ > kCurrentNativeExportChannelLimit) {
        opened_ = false;
        return DecodeError::NotImplemented;
    }
    if (dsp_output_channels_ > kCurrentNativeExportChannelLimit) {
        opened_ = false;
        return DecodeError::NotImplemented;
    }
    planar_scratch_.resize(static_cast<std::size_t>(channel_count_) * block_size_);
    const NativeChannelLayoutPlan input_layout =
        build_native_input_channel_layout(channel_count_, input_wav_channel_mask_, auro_metadata_);
    const NativeChannelLayoutPlan requested_layout =
        build_requested_native_channel_layout(dsp_output_channels_, auro_metadata_);
    if (input_layout.slot_count != channel_count_ || requested_layout.slot_count != dsp_output_channels_) {
        opened_ = false;
        return DecodeError::NotImplemented;
    }
    native_input_buffer_count_ = channel_count_;
    native_work_buffer_count_ = mask_count_27((input_layout.mask | requested_layout.mask) & 0x7FFFFFFu);
    native_input_buffer_storage_.assign(static_cast<std::size_t>(native_input_buffer_count_) * block_size_, 0);
    native_work_buffer_storage_.assign(static_cast<std::size_t>(native_work_buffer_count_) * block_size_, 0);
    rebuild_native_io_descriptors();
    rebuild_native_config_state();
    native_runtime_configuration_ = {};
    native_a3deng_static_configuration_ = {};
    native_dynamic_parameters_ = {};
    (void)apply_native_dynamic_parameters_update();
    if (!legacy_auromatic_upmix_)
        rebuild_codec_v3_partial_state();
    if (!validate_native_processor_io_model()) {
        opened_ = false;
        return DecodeError::BadInput;
    }
    return DecodeError::Ok;
}

DecodeError Decoder::decode_next(std::vector<std::uint8_t>& pcm_out) {
    pcm_out.clear();
    if (!opened_)
        return DecodeError::InitFailed;

    const std::size_t sample_frame_b = static_cast<std::size_t>(channel_count_) * 3u;
    const std::size_t frame_b = static_cast<std::size_t>(block_size_) * sample_frame_b;
    const std::size_t cur = read_pos_ - pcm_begin_;
    if (cur >= pcm_length_)
        return DecodeError::Ok;
    const std::size_t remaining_b = pcm_length_ - cur;
    const std::size_t valid_frames = std::min<std::size_t>(block_size_, remaining_b / sample_frame_b);
    if (valid_frames == 0) {
        read_pos_ = pcm_begin_ + pcm_length_;
        return DecodeError::Ok;
    }

    const std::uint8_t* frame = file_bytes_.data() + read_pos_;
    if (valid_frames == block_size_) {
        unpack_exoplayer_s24le_interleaved_to_planar_i32(frame, channel_count_, block_size_, planar_scratch_);
    } else {
        planar_scratch_.assign(static_cast<std::size_t>(channel_count_) * block_size_, 0);
        for (std::size_t s = 0; s < valid_frames; ++s) {
            const std::uint8_t* sample = frame + s * sample_frame_b;
            for (unsigned ch = 0; ch < channel_count_; ++ch) {
                const std::uint8_t* p = sample + static_cast<std::size_t>(ch) * 3u;
                int v = static_cast<int>(p[0]) | (static_cast<int>(p[1]) << 8) | (static_cast<int>(p[2]) << 16);
                if ((v & 0x800000) != 0)
                    v -= 0x1000000;
                planar_scratch_[static_cast<std::size_t>(ch) * block_size_ + s] = v;
            }
        }
    }

    const std::size_t plane_bytes = static_cast<std::size_t>(block_size_) * sizeof(std::int32_t);
    for (unsigned ch = 0; ch < channel_count_; ++ch) {
        std::int32_t* dst_plane = native_input_buffer(ch);
        const std::int32_t* src_plane = planar_scratch_.data() + static_cast<std::size_t>(ch) * block_size_;
        if (dst_plane)
            std::memcpy(dst_plane, src_plane, plane_bytes);
    }
    for (unsigned ch = channel_count_; ch < native_input_buffer_count_; ++ch) {
        std::int32_t* dst_plane = native_input_buffer(ch);
        if (dst_plane)
            std::memset(dst_plane, 0, plane_bytes);
    }
    for (unsigned ch = 0; ch < native_work_buffer_count_; ++ch) {
        std::int32_t* dst_plane = native_work_buffer(ch);
        if (dst_plane)
            std::memset(dst_plane, 0, plane_bytes);
    }
    // AuroDecoderImpl::Decode → Processor_process → codec-v3 partial step.
    if (legacy_auromatic_upmix_) {
        for (std::uint32_t slot = 0; slot < auro_codec_v3_ida::kAuroProcessorIoChannelPtrCount; ++slot) {
            if ((native_config_state_.input_mask & (1u << slot)) == 0u)
                continue;
            if (input_desc_.channel_ptr[slot] == 0u || output_desc_.channel_ptr[slot] == 0u)
                continue;
            std::memcpy(
                reinterpret_cast<void*>(static_cast<std::uintptr_t>(output_desc_.channel_ptr[slot])),
                reinterpret_cast<const void*>(static_cast<std::uintptr_t>(input_desc_.channel_ptr[slot])),
                plane_bytes);
        }
    } else if (auro_decoder_impl_blob_.size() == auro_codec_v3_ida::kAuroDecoderImplObjectBytes) {
        const std::int32_t decode_rc = auro3deng::auro_decoder_impl_decode_partial(
            auro_decoder_impl_blob_.data(),
            &Decoder::codec_v3_processor_process_bridge);
        if (decode_rc != 0)
            return DecodeError::BadInput;
    } else {
        run_codec_v3_partial_step();
    }
    std::uint32_t produced_mask = legacy_auromatic_upmix_
        ? (native_config_state_.input_mask & kCodecV3ChannelMask)
        : (codec_v3_dispatch_.produced_output_mask & kCodecV3ChannelMask);
    const std::uint32_t requested_mask = native_config_state_.requested_output_mask & kCodecV3ChannelMask;
    const std::uint32_t input_mask = native_config_state_.input_mask & kCodecV3ChannelMask;
    constexpr std::uint32_t kHeightMask = auro_codec_v3_ida::kAuroChannelMaskHeightLayer;
    const std::uint32_t req_height = native_config_state_.requested_output_mask & kHeightMask;
    const bool height_satisfied_by_input =
        req_height != 0u && (req_height & ~native_config_state_.input_mask) == 0u;
    constexpr std::uint32_t kAuroLayout7_1_5H_1T = 0x7FBFu;
    constexpr std::uint32_t kAuroCarrier7_1 = 0x01BFu;
    constexpr std::uint32_t kAuroDirect7_1_2H = 0x07BFu;
    // For 7.1_5H_1T the codec declares the complete frame layout in its
    // produced mask, but the native export path only emits the direct HL/HR
    // pair. The remaining height slots belong to the XinN renderer.
    const bool native_direct_7_1_2h =
        auro_metadata_.layout_id == kAuroLayout7_1_5H_1T
        && auro_metadata_.carrier_layout_id == kAuroCarrier7_1;
    const std::uint32_t native_height_mask = native_direct_7_1_2h
        ? (kAuroDirect7_1_2H & kHeightMask)
        : (produced_mask & kHeightMask);
    const std::uint32_t missing_height_mask =
        req_height & ~(native_config_state_.input_mask | native_height_mask) & kCodecV3ChannelMask;
    const std::uint32_t missing_requested_mask =
        requested_mask & ~(native_config_state_.input_mask | produced_mask) & kCodecV3ChannelMask;
    const std::uint32_t xinn_request_mask = legacy_auromatic_upmix_
        ? missing_requested_mask
        : missing_height_mask;
    if (xinn_request_mask != 0u && run_native_xinn_partial_step(xinn_request_mask))
        produced_mask |= xinn_request_mask;
    else if (missing_height_mask != 0u && run_native_asc4he_partial_step(missing_height_mask))
        produced_mask |= missing_height_mask;
    if (legacy_auromatic_upmix_) {
        constexpr std::uint32_t kFrontLeft = auro_codec_v3_ida::kAuroChMapSlotFrontLeft;
        constexpr std::uint32_t kFrontRight = auro_codec_v3_ida::kAuroChMapSlotFrontRight;
        constexpr std::uint32_t kFrontCenter = auro_codec_v3_ida::kAuroChMapSlotFrontCenter;
        constexpr std::uint32_t kLfe = auro_codec_v3_ida::kAuroChMapSlotLfe;
        if ((missing_requested_mask & (1u << kFrontCenter)) != 0u
            && output_desc_.channel_ptr[kFrontCenter] != 0u
            && output_desc_.channel_ptr[kFrontLeft] != 0u
            && output_desc_.channel_ptr[kFrontRight] != 0u) {
            auto* center = reinterpret_cast<std::int32_t*>(
                static_cast<std::uintptr_t>(output_desc_.channel_ptr[kFrontCenter]));
            const auto* left = reinterpret_cast<const std::int32_t*>(
                static_cast<std::uintptr_t>(output_desc_.channel_ptr[kFrontLeft]));
            const auto* right = reinterpret_cast<const std::int32_t*>(
                static_cast<std::uintptr_t>(output_desc_.channel_ptr[kFrontRight]));
            for (unsigned s = 0; s < block_size_; ++s)
                center[s] = static_cast<std::int32_t>(
                    (static_cast<std::int64_t>(left[s]) + static_cast<std::int64_t>(right[s])) / 2);
            produced_mask |= 1u << kFrontCenter;
        }
        // Auro-Matic does not synthesize an LFE feed; bass management is a
        // separate downstream stage in the original engine. Keep it silent.
        if ((missing_requested_mask & (1u << kLfe)) != 0u)
            produced_mask |= 1u << kLfe;
    }
    const std::uint32_t satisfied_mask = produced_mask | input_mask;
    const bool requested_ok = (requested_mask & ~satisfied_mask) == 0u;
    const std::uint32_t codec_v3_or_fallback_height = produced_mask & kHeightMask;
    const bool height_satisfied_after_fallback =
        req_height != 0u && (req_height & ~codec_v3_or_fallback_height) == 0u;
    const bool native_height_ok = !native_config_state_.requested_output_has_height_layer
        || height_satisfied_by_input
        || height_satisfied_after_fallback;
    if (requested_ok && native_height_ok)
        codec_v3_requested_layout_ever_satisfied_ = true;
    // Sync/parser/OG need DelayLine latency (stage1) and may lock mid-stream
    // (sync_sample != 0). Do not fail the first warm-up blocks; fail once past
    // latency if the requested layout (incl. height) was never produced.
    const std::uint64_t warmup_samples =
        static_cast<std::uint64_t>(block_size_)
        * static_cast<std::uint64_t>(std::max<std::uint32_t>(2u, native_config_state_.stage1_count + 1u));
    const bool past_warmup = legacy_auromatic_upmix_
        ? read_pos_ > pcm_begin_ + warmup_samples * sample_frame_b
        : codec_v3_delay_line_.absolute_cursor >= warmup_samples;
    if (past_warmup && !codec_v3_requested_layout_ever_satisfied_)
        return DecodeError::NotImplemented;
    // По умолчанию stereo, но при явном запросе рендерим все выходные слоты.
    const float gain = auro3deng::strength_translate(static_cast<std::uint32_t>(dsp_strength_)) * dsp_headroom_gain_;
    const unsigned out_ch = dsp_output_channels_ ? dsp_output_channels_ : channel_count_;
    if (out_ch == 0 || output_channel_slot_map_.size() < out_ch)
        return DecodeError::BadInput;
    for (unsigned ch = 0; ch < out_ch; ++ch) {
        const std::uint32_t logical_slot = output_channel_slot_map_[ch];
        if (logical_slot >= auro_codec_v3_ida::kAuroProcessorIoChannelPtrCount
            || output_desc_.channel_ptr[logical_slot] == 0) {
            return DecodeError::BadInput;
        }
    }

    const unsigned bytes_per_sample = (output_bits_ == 24u) ? 3u : 2u;
    std::vector<float> output_channel_gain(out_ch, gain);
    for (unsigned ch = 0; ch < out_ch; ++ch) {
        const std::uint32_t logical_slot = output_channel_slot_map_[ch];
        if ((native_config_state_.input_mask & (1u << logical_slot)) != 0u)
            output_channel_gain[ch] = 1.0f;
    }
    pcm_out.resize(static_cast<std::size_t>(block_size_) * out_ch * bytes_per_sample);
    std::uint8_t* dst = pcm_out.data();
    for (unsigned s = 0; s < block_size_; ++s) {
        for (unsigned ch = 0; ch < out_ch; ++ch) {
            const std::uint32_t logical_slot = output_channel_slot_map_[ch];
            const auto* src_plane = reinterpret_cast<const std::int32_t*>(
                static_cast<std::uintptr_t>(output_desc_.channel_ptr[logical_slot]));
            const std::int32_t v = src_plane[s];
            const float channel_gain = output_channel_gain[ch];
            if (output_bits_ == 24u) {
                const std::int32_t o = i32_sample_to_s24(v, channel_gain, &dsp_clipped_samples_);
                *dst++ = static_cast<std::uint8_t>(o & 0xFF);
                *dst++ = static_cast<std::uint8_t>((static_cast<std::uint32_t>(o) >> 8) & 0xFF);
                *dst++ = static_cast<std::uint8_t>((static_cast<std::uint32_t>(o) >> 16) & 0xFF);
            } else {
                const std::int16_t o = i32_sample_to_s16(v, channel_gain, &dsp_clipped_samples_);
                *dst++ = static_cast<std::uint8_t>(o & 0xFF);
                *dst++ = static_cast<std::uint8_t>((static_cast<std::uint16_t>(o) >> 8) & 0xFF);
            }
        }
    }
    read_pos_ += valid_frames * sample_frame_b;
    return DecodeError::Ok;
}

bool Decoder::exhausted() const {
    if (!opened_)
        return true;
    const std::size_t cur = read_pos_ - pcm_begin_;
    return cur >= pcm_length_;
}

DecoderConfig Decoder::config() const {
    DecoderConfig c;
    c.sample_rate = sample_rate_;
    c.channels = static_cast<std::uint16_t>(dsp_output_channels_ ? dsp_output_channels_ : channel_count_);
    c.block_size = block_size_;
    c.bits_per_sample = static_cast<std::uint16_t>(output_bits_);
    c.channel_mask = input_wav_channel_mask_;
    return c;
}

void Decoder::close() {
    file_bytes_.clear();
    pcm_begin_ = 0;
    pcm_length_ = 0;
    read_pos_ = 0;
    sample_rate_ = 0;
    channel_count_ = 0;
    block_size_ = 0;
    dsp_output_channels_ = 0;
    output_bits_ = 24;
    dsp_clipped_samples_ = 0;
    input_wav_channel_mask_ = 0;
    opened_ = false;
    planar_scratch_.clear();
    native_input_buffer_storage_.clear();
    native_work_buffer_storage_.clear();
    native_xinn_step_state_.clear();
    native_xinn_sample_rate_words_.clear();
    native_xinn_float_span_storage_.clear();
    native_xinn_process_scratch_storage_.clear();
    native_xinn_block_records_storage_.clear();
    native_xinn_partial_ready_ = false;
    native_xinn_partial_input_mask_ = 0u;
    native_xinn_partial_output_mask_ = 0u;
    native_xinn_partial_mode_ = 0u;
    native_asc4he_processor_state_.clear();
    native_asc4he_static_params_.clear();
    native_asc4he_float_span_storage_.clear();
    native_asc4he_channel_table_.clear();
    native_asc4he_partial_ready_ = false;
    native_asc4he_partial_input_mask_ = 0u;
    native_asc4he_partial_output_mask_ = 0u;
    output_channel_slot_map_.clear();
    native_work_buffer_count_ = 0;
    native_input_buffer_count_ = 0;
    input_channel_mask_ = 0;
    requested_output_channel_mask_ = 0;
    output_channel_mask_ = 0;
    auro_metadata_ = {};
    legacy_auromatic_upmix_ = false;
    native_config_state_ = {};
    native_runtime_configuration_ = {};
    native_a3deng_static_configuration_ = {};
    native_dynamic_parameters_ = {};
    native_a3deng_render_state_ = {};
    auro_decoder_impl_blob_.clear();
    std::memset(&input_desc_, 0, sizeof(input_desc_));
    std::memset(&output_desc_, 0, sizeof(output_desc_));
    codec_v3_dispatch_ = {};
    codec_v3_format_detector_ = {};
    codec_v3_sync_detector_ = {};
    codec_v3_delay_line_ = {};
    codec_v3_delay_line_slots_.clear();
    codec_v3_delay_line_storage_.clear();
    codec_v3_output_generator_state_.clear();
    codec_v3_output_table_storage_.clear();
    codec_v3_segment_ctx_storage_.clear();
    codec_v3_segment_ranges_storage_.clear();
    codec_v3_segment_started_.clear();
    codec_v3_segment_frame_ptrs_.clear();
    codec_v3_fake_frame_deque_storage_.clear();
    codec_v3_ready_frame_deque_storage_.clear();
    codec_v3_fake_frame_deque_frame_storage_.clear();
    codec_v3_ready_frame_deque_frame_storage_.clear();
    codec_v3_fake_frame_storage_.clear();
    codec_v3_fake_frame_storage_next_.clear();
    codec_v3_ready_frame_storage_.clear();
    codec_v3_ready_frame_storage_next_.clear();
    codec_v3_fake_frame_channels_storage_.clear();
    codec_v3_fake_frame_channel_words_.clear();
    codec_v3_fake_frame_channel_ctx_storage_.clear();
    codec_v3_fake_parse_result_pool_state_.clear();
    codec_v3_fake_parse_result_pool_storage_.clear();
    codec_v3_ready_parse_result_storage_.clear();
    codec_v3_channel_parser_storage_.clear();
    codec_v3_output_errors_storage_.clear();
    codec_v3_output_scratch_storage_.clear();
    codec_v3_parser_timeline_cursor_ = 0;
    codec_v3_og_timeline_cursor_ = 0;
    codec_v3_parser_state_ = 0;
    codec_v3_requested_layout_ever_satisfied_ = false;
    rebuild_a3deng_partial_blob();
}

} // namespace auro3d
