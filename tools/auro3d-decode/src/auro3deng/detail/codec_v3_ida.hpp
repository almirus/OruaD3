#pragma once

#include <cstddef>
#include <cstdint>

/// Адреса и смещения из libauro3d.so (IDA + user-ida-pro-mcp). База — текущий idb; при другой базе сдвинуть.
namespace auro_codec_v3_ida {

constexpr std::uintptr_t kAuroCodecV3Decoder_CRC_t_init = 0x106F00;
constexpr std::uintptr_t kAuroCodecV3Decoder_t_construct = 0x101760;
constexpr std::uintptr_t kAuroCodecV3Decoder_Config_initialize = 0x1033C0;
constexpr std::uintptr_t kAuroCodecV3Decoder_OutputGenerator_process = 0x102240;
constexpr std::uintptr_t kAuroCodecV3Decoder_channel_Extrapolate_t_init = 0x1042A1;
constexpr std::uintptr_t kAuroCodecV3Decoder_channel_Extrapolate_initialize = 0x104440;
constexpr std::uintptr_t kAuroCodecV3Decoder_channel_Extrapolate_process = 0x104460;
constexpr std::uintptr_t kAuroCodecV3Decoder_channel_GolombRice_initialize = 0x104E10;
constexpr std::uintptr_t kAuroCodecV3Decoder_channel_GolombRice_get_errors = 0x104E40;
constexpr std::uintptr_t kSub_EB420 = 0xEB420;

constexpr std::uintptr_t kAuroA3dengV3Processor_process = 0xD9AE0;
constexpr std::uintptr_t kAuroA3dengV3PipelineManager_process_audio = 0xDDD40;
constexpr std::uintptr_t kAuroA3dengV3PipelineStepUpmix_construct = 0xDB940;
constexpr std::uintptr_t kAuroA3dengV3PipelineStepUpmix_process = 0xDBD20;
constexpr std::uintptr_t kAuroA3dengV3PipelineStepUpmix_set_strength = 0xDBE90;
constexpr std::uintptr_t kAuroMaticV3Engine_construct = 0xE1830;
constexpr std::uintptr_t kAuroMaticV3Engine_process = 0xE1A10;
constexpr std::uintptr_t kAuroMaticV3XinN_configure = 0xF8630;
constexpr std::uintptr_t kAuroMaticV3XinN_process = 0xF86F0;
constexpr std::uintptr_t kAuroMaticV3XinN_default_dynamic = 0x101520;
constexpr std::uintptr_t kAuroMatic3d3dDefaultTable_unk_2CC00C = 0x2CC00C;
constexpr std::uintptr_t kAuroAsc4heElevationEq_initialize = 0x108FC0;
constexpr std::uintptr_t kAuroAsc4heElevationEq_process = 0x1090C0;
constexpr std::uintptr_t kAuroAsc4heVirtualHeight_initialize = 0x10CCF0;
constexpr std::uintptr_t kAuroAsc4heVirtualHeight_process = 0x10CDC0;
constexpr std::uintptr_t kJniAuroAudioProcessor_AuroInitialize = 0xD5360;
constexpr std::uintptr_t kJniAuroAudioProcessor_AuroRelease = 0xD5420;
constexpr std::uintptr_t kJniAuroAudioProcessor_AuroOutputChannelCount = 0xD5450;
constexpr std::uintptr_t kJniAuroAudioProcessor_AuroUpdateAudio = 0xD5460;
constexpr std::uintptr_t kJniAuroAudioProcessor_AuroUpdateConfiguration = 0xD5490;
constexpr std::uintptr_t kJniAuroAudioProcessor_AuroDecode = 0xD54B0;
constexpr std::uintptr_t kAuroDecoderImpl_Initialize = 0xD71E0;
constexpr std::uintptr_t kAuroDecoderImpl_Decode = 0xD7830;
constexpr std::uintptr_t kAuroDecoder_GetOutputChannelCount = 0xD7990; // IDA: returns constant 2 in current APK build
constexpr std::uintptr_t kAuroDecoder_GetChannelCount = 0xD79A0;
constexpr std::uintptr_t kAuroDecoder_GetBlockSize = 0xD79B0;

/// Внутренний layout auro::AuroDecoder::AuroDecoderImpl, подтвержденный по 0xD71E0/0xD79A0/0xD79B0.
constexpr std::uintptr_t kAuroDecoderImpl_off_WorkBuffersBegin = 16;   // std::vector<std::unique_ptr<uint32_t[]>>
constexpr std::uintptr_t kAuroDecoderImpl_off_WorkBuffersEnd = 24;
constexpr std::uintptr_t kAuroDecoderImpl_off_WorkBuffersCap = 32;
constexpr std::uintptr_t kAuroDecoderImpl_off_InputDesc = 40;          // Processor input descriptor, bits=24 at +48
constexpr std::uintptr_t kAuroDecoderImpl_off_OutputDesc = 272;        // Processor output descriptor, first 2 buffer ptrs copied at +288/+296
constexpr std::uintptr_t kAuroDecoderImpl_off_DecodeMutex = 504;       // std::recursive_mutex lock in Decode()
constexpr std::uintptr_t kAuroDecoderImpl_off_RuntimeInputMask = 536;  // partial port: expect for Processor_process
constexpr std::uintptr_t kAuroDecoderImpl_off_RuntimeOutputMask = 540;
constexpr std::uintptr_t kAuroDecoderImpl_off_ChannelCount = 544;      // GetChannelCount()
constexpr std::uintptr_t kAuroDecoderImpl_off_BlockSize = 548;         // GetBlockSize()
constexpr std::uintptr_t kAuroDecoderImpl_off_ProcessorInstance = 552; // create_instance(this+552, static_params)
/// Partial-port expect: Processor_process bytes_unit (native dword_25E7C0/25E7D4).
/// IO desc total_size_bytes holds sample count; validate uses unit = 32 * bytes_unit.
constexpr std::uint32_t kAuroDecoderImpl_expect_bytes_unit = 1u;
constexpr std::size_t kAuroDecoderImplObjectBytes = 560u;

constexpr std::uint32_t kAuroDecoderDecodeRcNeedInit = 2;              // Decode(): processor API missing / not initialized
constexpr std::uint32_t kAuroDecoderDecodeRcOkWhenOutputReady = 1;     // Decode(): normal success path for JNI wrapper
constexpr std::uint32_t kJniAuroDecodeOutputSampleBytes = 4;           // queueInput() uses channelCount * 4096 => float32 * 1024

/// channelMapping @ 0xD79C0: logical input slots inside ProcessorIOBufferDesc::channel_ptr[].
constexpr std::uint32_t kAuroChannelIdLeft = 0;
constexpr std::uint32_t kAuroChannelIdRight = 1;
constexpr std::uint32_t kAuroChannelIdCenter = 2;
constexpr std::uint32_t kAuroChannelIdLfe = 3;
constexpr std::uint32_t kAuroChannelIdLeftSurround = 4;
constexpr std::uint32_t kAuroChannelIdRightSurround = 5;
constexpr std::uint32_t kAuroChannelIdCenterSurround = 6;
constexpr std::uint32_t kAuroChannelIdLeftBack = 7;
constexpr std::uint32_t kAuroChannelIdRightBack = 8;
constexpr std::uint32_t kAuroChannelIdHeightLeft = 9;
constexpr std::uint32_t kAuroChannelIdHeightRight = 10;
constexpr std::uint32_t kAuroChannelIdHeightCenter = 11;
constexpr std::uint32_t kAuroChannelIdTop = 12;
constexpr std::uint32_t kAuroChannelIdHeightLeftSurround = 13;
constexpr std::uint32_t kAuroChannelIdHeightRightSurround = 14;
constexpr std::uint32_t kAuroChannelIdHeightCenterSurround = 15;
constexpr std::uint32_t kAuroChannelIdHeightLeftBack = 16;
constexpr std::uint32_t kAuroChannelIdHeightRightBack = 17;
constexpr std::uint32_t kAuroChannelIdLeftCenter = 18;
constexpr std::uint32_t kAuroChannelIdRightCenter = 19;
constexpr std::uint32_t kAuroChannelIdLfe2 = 20;
constexpr std::uint32_t kAuroChannelIdBottomLeft = 21;
constexpr std::uint32_t kAuroChannelIdBottomRight = 22;
constexpr std::uint32_t kAuroChannelIdBottomCenter = 23;
constexpr std::uint32_t kAuroChannelIdBottomLeftSurround = 24;
constexpr std::uint32_t kAuroChannelIdBottomRightSurround = 25;
constexpr std::uint32_t kAuroChannelIdObject = 26;
constexpr std::uint32_t kAuroChannelIdInvalid = 27;

constexpr std::uint32_t kAuroChMapSlotFrontLeft = 0;
constexpr std::uint32_t kAuroChMapSlotFrontRight = 1;
constexpr std::uint32_t kAuroChMapSlotFrontCenter = 2;
constexpr std::uint32_t kAuroChMapSlotLfe = 3;
constexpr std::uint32_t kAuroChMapSlotSideLeft = 4;
constexpr std::uint32_t kAuroChMapSlotSideRight = 5;
constexpr std::uint32_t kAuroChMapSlotBackCenter = 6;
constexpr std::uint32_t kAuroChMapSlotBackLeft = 7;
constexpr std::uint32_t kAuroChMapSlotBackRight = 8;

constexpr std::uint32_t kAuroChMapMaskStereo = 0x003u;      // FL FR
constexpr std::uint32_t kAuroChMapMask3Ch = 0x00Bu;         // FL FR LFE
constexpr std::uint32_t kAuroChMapMaskQuadSide = 0x033u;    // FL FR SL SR
constexpr std::uint32_t kAuroChMapMask5Ch = 0x03Bu;         // FL FR LFE SL SR
constexpr std::uint32_t kAuroChMapMask5_1 = 0x03Fu;         // FL FR FC LFE SL SR
constexpr std::uint32_t kAuroChMapMask6_1 = 0x07Fu;         // FL FR FC LFE SL SR BC
constexpr std::uint32_t kAuroChMapMask7_1 = 0x1BFu;         // FL FR FC LFE SL SR BL BR
constexpr std::uint32_t kAuroProcessorIoChannelPtrCount = 27; // ProcessorIOBufferDesc::channel_ptr[27]
constexpr std::uint32_t kAuroChannelMappingDefault = 0u;    // sub_31ACE0: numeric channel-id order
constexpr std::uint32_t kAuroChannelMappingBacksBeforeSurrounds = 1u; // sub_31ACE0: xmmword_1DB2B0/1DB2C0 table

/// auro_channel_Layout_dimension @ 0x131BE0:
/// return classes: 1=base-like, 2=surround-layer, 3=height-layer.
/// disasm loads: base-mask=0x100003, surround-mask=0x001F0, height-mask=0x3FE00.
constexpr std::uint32_t kAuroLayoutDimensionBase = 1u;
constexpr std::uint32_t kAuroLayoutDimensionSurround = 2u;
constexpr std::uint32_t kAuroLayoutDimensionHeight = 3u;
constexpr std::uint32_t kAuroChannelMaskBaseLayer = 0x100003u;
constexpr std::uint32_t kAuroChannelMaskSurroundLayer = 0x001F0u;
constexpr std::uint32_t kAuroChannelMaskHeightLayer = 0x3FE00u;

constexpr std::uintptr_t kAuroCodecV3Decoder_set_sync_callback = 0x101AB0;
constexpr std::uintptr_t kSub_EB840_sync_cb = 0xEB840;
constexpr std::uintptr_t kSub_EB870_content_cb = 0xEB870;
constexpr std::uintptr_t kSub_EB8A0_decide_decode_cb = 0xEB8A0;

/// Глобальные данные CRC init: ленивая генерация 512 байт в unk_41ACF0, флаг byte_41ACE0.
constexpr std::uintptr_t kBss_CRC_table_unk_41ACF0 = 0x41ACF0;
constexpr std::uintptr_t kBss_CRC_inited_byte_41ACE0 = 0x41ACE0;

/// Подобъекты внутри auro_codec_v3_Decoder_t (аргумент a1 у construct): из декомпиляции 0x101760.
constexpr std::uintptr_t kDecoder_off_Memory = 64;
constexpr std::uintptr_t kDecoder_off_FormatDetector = 232;

/// `sub_52CED0` @ `0x52CED0` (libauro.so x86_64, IDA): FormatDetector sync callback — FrameDeque push on `a2==0`,
/// pop_back on `a2==2`, optional notify @ `this+288` with `this+296` ctx (`0`/`1`).
constexpr std::uintptr_t kFormatDetector_sub_52CED0 = 0x52CED0;
constexpr std::uintptr_t kFormatDetector52ced0_off_notify_fn = 288;
constexpr std::uintptr_t kFormatDetector52ced0_off_notify_ctx = 296;
constexpr std::uintptr_t kFormatDetector52ced0_off_layout_word = 304;
constexpr std::uintptr_t kFormatDetector52ced0_off_flag_308 = 308;
constexpr std::uintptr_t kFormatDetector52ced0_off_frame_deque = 312;
constexpr std::uintptr_t kFormatDetector52ced0_off_timeline_base_qword = 320;
constexpr std::uintptr_t kFormatDetector52ced0_off_timeline_end_qword = 328;
constexpr std::uintptr_t kFormatDetector52ced0_off_allow_low9bits_dword = 340;
constexpr std::int32_t kFormatDetector52ced0_frame_mask_and_when_allow_nonzero = 0x7FFFFFFF;
constexpr std::int32_t kFormatDetector52ced0_frame_mask_and_when_allow_zero = -385;
constexpr std::uintptr_t kLibauro_codec_FormatDetector_process = 0x52D060;
constexpr std::uintptr_t kLibauro_codec_SyncDetector_set_layout = 0x52C460;
constexpr std::uintptr_t kLibauro_codec_SyncDetector_process_block = 0x52C680;
constexpr std::uintptr_t kLibauro_codec_Parser_process = 0x52EDB0;
constexpr std::uintptr_t kLibauro_codec_channel_Extrapolate_t_init = 0x52DDD0;
constexpr std::uintptr_t kLibauro_codec_channel_Extrapolate_initialize = 0x52DEF0;
constexpr std::uintptr_t kLibauro_codec_channel_Extrapolate_process = 0x52DF10;
constexpr std::uintptr_t kLibauro_codec_channel_Parser_process = 0x52F330;
constexpr std::uintptr_t kDecoder_off_Parser = 576;
constexpr std::uintptr_t kDecoder_off_OutputGenerator = 1232;
constexpr std::uintptr_t kDecoder_off_qword_2472 = 2472;
constexpr std::uintptr_t kDecoder_off_dword_28 = 28;

/// Config_initialize @ 0x1033C0:
/// a2+0=input bytes/unit seed, a2+8=block bits (must be >=32 and aligned by 32),
/// a2+16=input mask, a2+20=output mask, a2+24=extra config dword.
constexpr std::uintptr_t kDecoderConfigInitArg_off_input_bytes_unit = 0;
constexpr std::uintptr_t kDecoderConfigInitArg_off_block_bits = 8;
constexpr std::uintptr_t kDecoderConfigInitArg_off_input_mask = 16;
constexpr std::uintptr_t kDecoderConfigInitArg_off_output_mask = 20;
constexpr std::uintptr_t kDecoderConfigInitArg_off_extra_flags = 24;

/// Config object layout after initialize().
constexpr std::uintptr_t kDecoderConfig_off_block_bits = 0;
constexpr std::uintptr_t kDecoderConfig_off_block_words = 8;
constexpr std::uintptr_t kDecoderConfig_off_buffer_count_qword = 16;
constexpr std::uintptr_t kDecoderConfig_off_stage0_count = 24;
constexpr std::uintptr_t kDecoderConfig_off_stage1_count = 28;
constexpr std::uintptr_t kDecoderConfig_off_input_bytes_unit = 32;
constexpr std::uintptr_t kDecoderConfig_off_input_mask = 36;
constexpr std::uintptr_t kDecoderConfig_off_output_mask = 40;
constexpr std::uintptr_t kDecoderConfig_off_input_mask_count = 44;
constexpr std::uintptr_t kDecoderConfig_off_output_mask_count = 48;
constexpr std::uintptr_t kDecoderConfig_off_extra_flags = 52;

constexpr std::uint32_t kDecoderConfigInitErrBlockBits = 401;
constexpr std::uint32_t kDecoderConfigInitErrInputMaskRange = 402;
constexpr std::uint32_t kDecoderConfigInitErrOutputMaskRange = 403;
constexpr std::uint32_t kDecoderConfigInitErrInputMaskNotSubset = 404;

/// Порядок в auro_codec_v3_Decoder_t_construct: CRC_t_init → channel_Extrapolate_t_init →
/// Config_initialize → Memory_t_construct → FormatDetector_t_construct → Parser_t_construct → OutputGenerator_t_construct.

} // namespace auro_codec_v3_ida

namespace auro_engine_v4_ida {

// Artist Connection 1.21.31, x86_64 libauro.so, package auroenginev4.
// These are IDA image offsets for the current libauro.so analysis target.
constexpr std::uintptr_t kJniA3DENG_AuroInitialize = 0x318C90;
constexpr std::uintptr_t kJniA3DENG_AuroUpdate2 = 0x318D60;
constexpr std::uintptr_t kJniA3DENG_AuroPush = 0x318FE0;
constexpr std::uintptr_t kJniA3DENG_AuroPop = 0x319120;
constexpr std::uintptr_t kA3DENG_construct = 0x319AE0;
constexpr std::uintptr_t kA3DENG_update = 0x319E60;
constexpr std::uintptr_t kA3DENG_push = 0x31AF50;
constexpr std::uintptr_t kA3DENG_pop = 0x31B7A0;
constexpr std::uintptr_t kA3DENG_pop_internal = 0x31B7F0;
constexpr std::uintptr_t kA3DENG_get_output_info = 0x31B4E0;
/// `sub_6319F0` @ `0x6319F0`: stack prep + branch to `sub_6318E0` ([rbx+10]==0) or `sub_631A70`; tail is unwind/abort (IDA).
constexpr std::uintptr_t kA3DENG_sub_6319F0 = 0x6319F0;
constexpr std::uintptr_t kA3DENG_sub_6318E0 = 0x6318E0;
constexpr std::uintptr_t kA3DENG_sub_631A70 = 0x631A70;
/// `A3DENG::pop_` internal (`0x31B7F0`) calls `sub_6319F0` @ `0x31BBC7`.
constexpr std::uintptr_t kA3DENG_pop_internal_call_sub_6319F0 = 0x31BBC7;
constexpr std::uintptr_t kA3DENG_sub_631C00 = 0x631C00;
constexpr std::uintptr_t kA3DENG_sub_631C60 = 0x631C60;
constexpr std::uintptr_t kA3DENG_sub_631D40 = 0x631D40;
constexpr std::uintptr_t kA3DENG_sub_631D50 = 0x631D50;
constexpr std::uintptr_t kA3DENG_sub_635450 = 0x635450;
constexpr std::uintptr_t kA3DENG_audio_block_vtbl_op10 = 0x10;
constexpr std::uintptr_t kA3DENG_audio_block_vtbl_op18 = 0x18;
constexpr std::uintptr_t kA3DENG_audio_block_vtbl_op40 = 0x40;
constexpr std::uintptr_t kA3DENG_audio_block_vtbl_op48 = 0x48;
constexpr std::int32_t kA3DENG_sub_631D50_err_nonempty_queue = -6101; // `0xFFFFE66B`
constexpr std::int32_t kA3DENG_sub_631C60_err = -6094;                // `0xFFFFE672`
/// Same idb (`libauro.so`): embedded `auro_codec_v3_decoder_OutputGenerator_*` (distinct from `libauro3d.so` `0x102240` slot).
constexpr std::uintptr_t kLibauro_codec_OutputGenerator_process = 0x52B590;
constexpr std::uintptr_t kLibauro_codec_OutputGenerator_cross_fade_ = 0x52AFE0;
constexpr std::uintptr_t kLibauro_codec_OutputGenerator_cross_fade_inner = 0x52B0B0;
constexpr std::uintptr_t kLibauro_codec_channel_GolombRice_initialize = 0x52D880;
constexpr std::uintptr_t kLibauro_codec_channel_GolombRice_get_errors = 0x52D8B0;

/// auro::a3deng::v4::android::A3DENG object layout from current x86_64 IDA.
constexpr std::uintptr_t kA3DENG_off_pipeline_audio_block_size = 52;
constexpr std::uintptr_t kA3DENG_off_output_mode = 56;
constexpr std::uintptr_t kA3DENG_off_configured = 48;
constexpr std::uintptr_t kA3DENG_off_settings = 60;
constexpr std::uintptr_t kA3DENG_off_decoder_mode = 64;
constexpr std::uintptr_t kA3DENG_off_output_channel_mask_runtime = 68;
constexpr std::uintptr_t kA3DENG_off_output_sample_type_runtime = 72;
constexpr std::uintptr_t kA3DENG_off_output_bit_depth_runtime = 76;
constexpr std::uintptr_t kA3DENG_off_input_channel_mask_runtime = 80;
constexpr std::uintptr_t kA3DENG_off_input_sample_rate_runtime = 84;
constexpr std::uintptr_t kA3DENG_off_input_sample_type_runtime = 88;
constexpr std::uintptr_t kA3DENG_off_hdmi_channel_mapping_runtime = 92;
constexpr std::uintptr_t kA3DENG_off_hp_user_preset_runtime = 104;
constexpr std::uintptr_t kA3DENG_off_hp_hrtf_preset_runtime = 108;
constexpr std::uintptr_t kA3DENG_off_api = 112;
constexpr std::uintptr_t kA3DENG_off_instance = 120;
constexpr std::uintptr_t kA3DENG_off_mutex = 128;
constexpr std::uintptr_t kA3DENG_off_input_storage_begin = 168;
constexpr std::uintptr_t kA3DENG_off_input_storage_end = 176;
constexpr std::uintptr_t kA3DENG_off_pruned_output_info_storage = 328;
constexpr std::uintptr_t kA3DENG_off_pruned_output_channel_count_for_bytecount = 328;
constexpr std::uintptr_t kA3DENG_off_pruned_output_max_sample = 328;
constexpr std::uintptr_t kA3DENG_off_pruned_output_count = 336;
constexpr std::uintptr_t kA3DENG_off_pruned_output_channel_entries = 352;
constexpr std::uintptr_t kA3DENG_pruned_output_channel_entry_stride = 16;
constexpr std::uintptr_t kA3DENG_off_pruned_output_valid = 728;

/// A3DENG::Settings is copied by AuroUpdate2 and A3DENG::update as 0x34 bytes:
/// incoming Java params -> stack Settings -> object at this+kA3DENG_off_settings.
/// Names below are from A3DENG::update log strings and direct field use.
constexpr std::uintptr_t kA3DENG_settings_size = 0x34;
constexpr std::uintptr_t kA3DENG_settings_off_stereo_device = 0;
constexpr std::uintptr_t kA3DENG_settings_off_headset_connected = 1;
constexpr std::uintptr_t kA3DENG_settings_off_decoder_mode = 4;
constexpr std::uintptr_t kA3DENG_settings_off_output_channel_mask = 8;
constexpr std::uintptr_t kA3DENG_settings_off_output_sample_type = 12;
constexpr std::uintptr_t kA3DENG_settings_off_output_bit_depth = 16;
constexpr std::uintptr_t kA3DENG_settings_off_input_channel_mask = 20;
constexpr std::uintptr_t kA3DENG_settings_off_input_sample_rate = 24;
constexpr std::uintptr_t kA3DENG_settings_off_input_sample_type = 28;
constexpr std::uintptr_t kA3DENG_settings_off_hdmi_channel_mapping = 32;
constexpr std::uintptr_t kA3DENG_settings_off_is_abr = 36; // Java abr_mode_enabled
constexpr std::uintptr_t kA3DENG_settings_off_dynamic_request_flag = 40; // Java virtualization_enabled
constexpr std::uintptr_t kA3DENG_settings_off_dynamic_headphone_flag = 41; // Java listening_mode_auro3d
constexpr std::uintptr_t kA3DENG_settings_off_hp_room = 44; // Java hp_user_preset
constexpr std::uintptr_t kA3DENG_settings_off_hp_hrtf_preset = 48; // Java hp_hrtf_preset

/// A3DENG::get_output_info @ 0x31B4E0 reads this+52 as the constructor
/// pipeline_audio_block_size, keeps bits 0..31 from that dword, and puts API
/// output_block_count (vtable+0xA8) into bits 32..63. A3DENG::pop_ @ 0x31B7F0
/// then uses this+52 as block size for byte-count calculation.
constexpr std::uint32_t kA3DENG_output_info_block_size_low_mask = 0xFFu;
constexpr std::uint32_t kA3DENG_output_info_block_size_high_mask = 0xFFFFFF00u;
constexpr std::uint32_t kA3DENG_api_vtable_off_required_memory = 0x28u;
constexpr std::uint32_t kA3DENG_api_vtable_off_create_instance = 0x30u;
constexpr std::uint32_t kA3DENG_api_vtable_off_destroy_instance = 0x38u;
constexpr std::uint32_t kA3DENG_api_vtable_off_default_static = 0x50u;
constexpr std::uint32_t kA3DENG_api_vtable_off_validate_static = 0x60u;
constexpr std::uint32_t kA3DENG_api_vtable_off_configure = 0x78u;
constexpr std::uint32_t kA3DENG_api_vtable_off_get_dynamic = 0x80u;
constexpr std::uint32_t kA3DENG_api_vtable_off_set_dynamic = 0x88u;
constexpr std::uint32_t kA3DENG_api_vtable_off_get_latency = 0x90u;
constexpr std::uint32_t kA3DENG_api_vtable_off_output_block_count = 0xA8u;
constexpr std::uint32_t kA3DENG_api_vtable_off_push_input = 0xC0u;
constexpr std::uint32_t kA3DENG_api_vtable_off_render_audio = 0xC8u;
constexpr std::uint32_t kA3DENG_api_vtable_off_reset_audio_state = 0xD0u;

/// Live Frida/Kahlo trace, Artist Connection 1.21.31, AURO-3D Demo Compilation.
/// The app's default playback route is stereo, so the observed Pop size is
/// only a runtime sample of that route, not a decoder export-channel limit.
/// JNI AuroPush gets 0x3A80 bytes for the active 6ch s24le track:
/// 832 frames * 6 channels * 3 bytes. JNI AuroPop on the default route returns
/// 0x1A00 bytes: 832 frames * 2 channels * 4-byte float output.
/// JNI `AuroInitialize` / native `A3DENG::A3DENG(this, a2, a3)` constructor arg @ this+52.
constexpr std::uint32_t kA3DENG_constructor_pipeline_block_size = 0x40u;
constexpr std::uint32_t kA3DENG_live_observed_block_frames = 832u;
constexpr std::uint32_t kA3DENG_live_observed_push_bytes_6ch_s24 = 0x3A80u;
constexpr std::uint32_t kA3DENG_live_observed_default_pop_bytes_stereo_f32 = 0x1A00u;
constexpr std::uint32_t kA3DENG_jni_input_sample_bytes_s24 = 3u;
constexpr std::uint32_t kA3DENG_jni_output_sample_bytes_f32 = 4u;

constexpr std::uintptr_t kManager_configure = 0x477758;
constexpr std::uintptr_t kManager_process_audio = 0x478078;
constexpr std::uintptr_t kParameter_get_update_type = 0x482A3C;
constexpr std::uintptr_t kStepUpmixXinN_initialize = 0x35B2C0;
constexpr std::uintptr_t kStepUpmixXinN_prepare = 0x35B330;
constexpr std::uintptr_t kStepUpmixXinN_process = 0x35B440;
constexpr std::uintptr_t kStepUpmixXinN_reset_audio_state = 0x35B730;
constexpr std::uintptr_t kStepUpmixXinN_update = 0x35B740;
constexpr std::uintptr_t kStepUpmixXinN_calculate_info = 0x35B7D0;
constexpr std::uintptr_t kAuroMaticV3XinN_fl32_reset_audio_state = 0x556240;
constexpr std::uintptr_t kAuroMaticV3XinN_fl32_set_preset = 0x556250;
constexpr std::uintptr_t kAuroMaticV3XinN_fl32_set_dynamic_parameters = 0x556200;
constexpr std::uintptr_t kAuroMaticV3XinN_fl32_process = 0x556440;
constexpr std::uintptr_t kAuroMaticXinN_fl32_set_preset = 0x574AF0;
constexpr std::uintptr_t kAuroMaticXinN_fl32_reset_audio_state = 0x574F90;
constexpr std::uintptr_t kAuroMaticXinN_fl32_process = 0x574FE0;
constexpr std::uintptr_t kAuroMaticXinN_Late_fl32_process = 0x5799A0;
constexpr std::uintptr_t kAuroMaticEngine2_fl32_construct = 0x57B000;
constexpr std::uintptr_t kAuroMaticEngine2_fl32_set_total_clear_frames = 0x57AFF0;
constexpr std::uintptr_t kAuroMaticEngine2_fl32_set_preset = 0x57B040;
constexpr std::uintptr_t kAuroMaticEngine2_fl32_reset_audio_state = 0x57B050;
constexpr std::uintptr_t kAuroMaticEngine2_fl32_partial_clear = 0x57B080;
constexpr std::uintptr_t kAuroMaticEngine2_fl32_process = 0x57B0E0;
constexpr std::uintptr_t kAuroMaticEngine2_fl32_set_output_patch = 0x57C2F0;
constexpr std::uintptr_t kAuroMaticEngine2Delay_fl32_read_post_frames = 0x580070;
constexpr std::uintptr_t kAuroMaticEngine2Delay_fl32_write_xar_frame = 0x580230;
constexpr std::uintptr_t kAuroMaticEngine2Delay_fl32_write_post_frame = 0x5809E0;
constexpr std::uintptr_t kAuroMaticEngine2Delay_fl32_increment = 0x580CB0;
constexpr std::uintptr_t kAuroMaticEngine2Reverb_fl32_process = 0x57EE40;
constexpr std::uintptr_t kAuroMaticV3XinNRouting_fl32_construct = 0x557420;
constexpr std::uintptr_t kAuroMaticV3XinNRouting_fl32_apply = 0x5575C0;
constexpr std::uintptr_t kAuroMaticV3XinNRouting_fl32_set_routing = 0x557620;
constexpr std::uintptr_t kAuroMaticXinN_fl32_process_inplace = 0x574E30;
constexpr std::uintptr_t kAuroMaticXinN_fl32_process_scratch = 0x574EE0;

constexpr std::uint32_t kJniBlockSize = 832u;
constexpr std::uint32_t kOutputSampleTypeFloat = 0u;
constexpr std::uint32_t kOutputSampleTypeInt32 = 1u;
constexpr std::uint32_t kOutputBitDepthFloat = 32u;
constexpr std::uint32_t kOutputBitDepthInt24 = 24u;
constexpr std::uint32_t kChannelCount = 31u;
constexpr std::uint32_t kChannelIdLeftWide = 26u;
constexpr std::uint32_t kChannelIdRightWide = 27u;
constexpr std::uint32_t kChannelIdTopLeft = 28u;
constexpr std::uint32_t kChannelIdTopRight = 29u;
constexpr std::uint32_t kChannelIdMono = 30u;

constexpr std::uintptr_t kManager_off_params = 8u;
constexpr std::uintptr_t kManager_off_bypass = 288u;
constexpr std::uintptr_t kManager_off_headroom_dirty = 480u;
constexpr std::uintptr_t kManager_off_step_chain = 848u;
constexpr std::uintptr_t kManager_off_configure_cb = 880u;
constexpr std::size_t kManager_params_bytes = 124u;

} // namespace auro_engine_v4_ida
