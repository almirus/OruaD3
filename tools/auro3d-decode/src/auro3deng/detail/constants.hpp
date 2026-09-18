#pragma once

namespace auro3deng {

// Возвраты строго по.
constexpr std::int32_t kErrNullProcessor = 2;
constexpr std::int32_t kErrNullIoDesc = 3;
constexpr std::int32_t kErrInField8Mismatch = 122;
constexpr std::int32_t kErrOutField8Mismatch = 123;
constexpr std::int32_t kErrInSizeModulo = 124;
constexpr std::int32_t kErrOutSizeModulo = 125;
constexpr std::int32_t kErrInField4Mismatch = 126;
constexpr std::int32_t kErrOutField4Mismatch = 127;
constexpr std::int32_t kErrInLayoutMismatch = 128;
constexpr std::int32_t kErrOutLayoutMismatch = 129;
constexpr std::int32_t kErrInChannelPtrMissing = 135;
constexpr std::int32_t kErrOutChannelPtrMissing = 136;

// OutputGenerator_process: ключевые поля объекта output generator.
constexpr std::uintptr_t kOgOff_total_samples = 824;
constexpr std::uintptr_t kOgOff_crossfade_block_count = 832;
constexpr std::uintptr_t kOgOff_timeline_cursor = 840;
constexpr std::uintptr_t kOgOff_delay_line_ptr = 848;
constexpr std::uintptr_t kOgOff_frame_deque_ptr = 856;
constexpr std::uintptr_t kOgOff_block_info_ptr = 864;
constexpr std::uintptr_t kOgOff_segment_ctx_ptr = kOgOff_block_info_ptr;
constexpr std::uintptr_t kOgOff_pre_segments_cb = 792; // v6[99]
constexpr std::uintptr_t kOgOff_pre_segments_ctx = 800; // v6[100]
constexpr std::uintptr_t kOgOff_metadata_cb = 808; // v6[101]
constexpr std::uintptr_t kOgOff_metadata_ctx = 816; // v6[102]

// Внутри segment_ctx (v133) из.
constexpr std::uintptr_t kSegCtxOff_ranges_base = 0;      // *v133, stride 24 (start,len,mask,flags)
constexpr std::uintptr_t kSegCtxOff_frame_started = 8;    // v133[1], int[]
constexpr std::uintptr_t kSegCtxOff_frame_ptrs = 16;      // v133[2], ptr[]
constexpr std::uintptr_t kSegCtxOff_count = 24;           // *((DWORD*)v133 + 6)

constexpr std::size_t kSegmentStrideBytes = 24;
constexpr std::uintptr_t kSegRangeOff_start = 0;
constexpr std::uintptr_t kSegRangeOff_len = 8;
constexpr std::uintptr_t kSegRangeOff_mask = 16;
constexpr std::uintptr_t kSegRangeOff_flags = 20;
constexpr std::size_t kMaxSegmentBuildGuard = 4096;
static_assert(kSegmentStrideBytes == 24, "Segment range entries must be 24 bytes.");

// frame/channel layout offsets.
constexpr std::uintptr_t kFrameOff_channel_count_dword = 44;     // v23[11]
constexpr std::uintptr_t kFrameOff_end_dword = 8;                // v23[2]
constexpr std::uintptr_t kFrameOff_channel_slot_base = 48;       // v23[12 + 8*i]
constexpr std::uintptr_t kFrameStride_channel_slot = 32;         // 8 dword per channel
constexpr std::uintptr_t kFrameChSlotOff_channel_index = 0;      // [12 + 8*i]
constexpr std::uintptr_t kFrameChSlotOff_active_flag = 4;        // [13 + 8*i]
constexpr std::uintptr_t kFrameChSlotOff_channel_ptr_qword = 24; // [18 + 8*i]

constexpr std::uintptr_t kFrameChannelOff_ctx_words = 192;       // current: ParseResult + 192
constexpr std::uintptr_t kFrameChannelOff_stream_words = 1920;   // current: ParseResult + 1920
constexpr std::uintptr_t kFrameChannelOff_ctx_count_qword = 3648; // current: written before ctx words
constexpr std::uintptr_t kFrameChannelOff_stream_count_qword = 3656; // current: written before stream words
constexpr std::uintptr_t kFrameChannelOff_ex_scale_idx0 = 0;      // v8[0]
constexpr std::uintptr_t kFrameChannelOff_ex_scale_idx1 = 48;     // v8[12]
constexpr std::uintptr_t kFrameChannelOff_ex_quant_shift = 72;    // v8[18]
constexpr std::uintptr_t kFrameChannelOff_ex_mode = 104;          // v8[26]
constexpr std::uintptr_t kFrameChannelOff_gr_packed_flags = 80;    // GolombRice a2[20]
constexpr std::uintptr_t kFrameChannelOff_gr_base_index = 40;      // GolombRice a2[10]
constexpr std::uintptr_t kFrameChannelOff_gr_bit_width = 44;      // GolombRice a2[11]
constexpr std::uintptr_t kFrameChannelOff_pred_src0_idx = 108;    // v38[27]
constexpr std::uintptr_t kFrameChannelOff_pred_src1_idx = 112;    // v38[28]
constexpr std::uintptr_t kFrameChannelOff_pred_src2_idx = 116;    // v38[29]
constexpr std::uintptr_t kFrameChannelOff_seed0_primary = 16;     // v8[4]
constexpr std::uintptr_t kFrameChannelOff_seed0_secondary = 20;   // v8[5]
constexpr std::uintptr_t kFrameChannelOff_seed1_primary = 24;     // v8[6]
constexpr std::uintptr_t kFrameChannelOff_seed1_secondary = 28;   // v8[7]
constexpr std::uintptr_t kFrameChannelOff_seed2_primary = 32;     // v8[8]
constexpr std::size_t kParseResultStrideBytes = 3672;             // current ParseResultPool stride
constexpr std::uintptr_t kCodecV3FrameOffSlotBase = 48u;
constexpr std::uintptr_t kCodecV3FrameSlotStride = 32u;
constexpr std::uint32_t kCodecV3FrameDequeSlotCopyBytes = 336u;   // FrameDeque_push_back memcpy 0x150
constexpr std::uint32_t kCodecV3FrameDequeCopiedSlotCapacity =
    static_cast<std::uint32_t>((kCodecV3FrameDequeSlotCopyBytes - kCodecV3FrameOffSlotBase) / kCodecV3FrameSlotStride);
constexpr std::uintptr_t kParseResultOff_usage_dword = 3664;      // ParseResult_mark_usage current +3664
constexpr std::uint32_t kCodecV3ChannelCount = 31;
constexpr std::uint32_t kCodecV3ChannelMask = 0x7FFFFFFF;
constexpr std::uint64_t kDelayLineBufferSlotStrideBytes = 256;

constexpr std::uintptr_t kOgOff_gr_state_base = 0;               // a1 + 40*i
constexpr std::uintptr_t kOgStride_gr_state = 40;
constexpr std::uintptr_t kOgOff_ex_state_base = 360;             // a1 + 360 + 48*i
constexpr std::uintptr_t kOgStride_ex_state = 48;

constexpr std::uint32_t kDword273200[8] = {
    0x3F800000u, 0x3F35C28Fu, 0x3F000000u, 0x3EB33333u,
    0x3E800000u, 0x3E3851ECu, 0x3E051EB8u, 0x00000000u,
};
constexpr std::uint32_t kDword289FC0[16] = {
    0u, 0xFFFFFFFFu, 0xFFFFFFFEu, 0xFFFFFFFDu,
    0xFFFFFFFCu, 0xFFFFFFFBu, 0xFFFFFFFAu, 0xFFFFFFF9u,
    0xFFFFFFF8u, 0xFFFFFFF7u, 0xFFFFFFF6u, 0xFFFFFFF5u,
    0xFFFFFFF4u, 0xFFFFFFF3u, 0xFFFFFFF2u, 0xFFFFFFF1u,
};
// GolombRice_get_errors: [k] for remainder bit weights.
constexpr std::uint32_t kDword289CE0[16] = {
    1u, 2u, 4u, 8u, 16u, 32u, 64u, 128u, 256u, 512u, 1024u, 2048u, 4096u, 8192u, 16384u, 32768u,
};
constexpr std::uintptr_t kOgOff_errors_buf = 872;
constexpr std::uintptr_t kOgOff_scratch_base = 880;
constexpr std::uintptr_t kOgOff_metadata_table_base = 0x378;
constexpr std::uintptr_t kOgOff_metadata_latency = 0x378;
constexpr std::uintptr_t kOgOff_metadata_frame_flags = 0x380;
constexpr std::uintptr_t kOgOff_channel_gain_table = 0x384;
constexpr std::uintptr_t kOgOff_metadata_gain_table = 0x47C;
constexpr std::uintptr_t kOgOff_output_status_flag = 1236;

// /; _codec_channel_GolombRice_*.
constexpr std::uintptr_t kGrStateOff_words_ptr_qword = 0;
constexpr std::uintptr_t kGrStateOff_ctx_ptr_qword = 8;
constexpr std::uintptr_t kGrStateOff_bit_index_dword = 16;
constexpr std::uintptr_t kGrStateOff_accum_bits_dword = 20;
constexpr std::uintptr_t kGrStateOff_mode_dword = 24;
constexpr std::uintptr_t kGrStateOff_base_index_dword = 28;
constexpr std::uintptr_t kGrStateOff_k_dword = 32;
constexpr std::uintptr_t kGrStateOff_counter_dword = 36;

// state; _codec_channel_Extrapolate_t_init _initialize _process.
constexpr std::uintptr_t kExStateOff_head_qword = 0;
constexpr std::uintptr_t kExStateOff_mode1_count_dword = 0;
constexpr std::uintptr_t kExStateOff_mode2_count_dword = 4;
constexpr std::uintptr_t kExStateOff_mode2_last_residual_dword = 8;
constexpr std::uintptr_t kExStateOff_mode2_prev_primary_dword = 12;
constexpr std::uintptr_t kExStateOff_mode2_prev_secondary_dword = 16;
constexpr std::uintptr_t kExStateOff_phase_dword = 20;
constexpr std::uintptr_t kExStateOff_mode3_predictor_dword = 24; // a1+24: current predictor term
constexpr std::uintptr_t kExStateOff_mode3_last_a_dword = 28;    // a1+28: latest rotated residual/output
constexpr std::uintptr_t kExStateOff_mode3_last_b_dword = 32;    // a1+32: previous history term for predictor
constexpr std::uintptr_t kExStateOff_mode3_last_c_dword = 36;    // a1+36: previous predictor written to output
constexpr std::uintptr_t kExStateOff_frame_channel_ptr_qword = 40;

constexpr std::size_t kExtrapolateScaleTableSize = 244;
constexpr float kExtrapolateScaleExpStep_1042a1 = 0.011512925465f;
constexpr std::int32_t kPcm24Max_104460 = 0x7FFFFF;
constexpr std::int32_t kPcm24Min_104460 = -8388607;

} // namespace auro3deng
