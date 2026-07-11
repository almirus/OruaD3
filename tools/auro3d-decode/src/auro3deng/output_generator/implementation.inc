int next_set_channel_bit_1024a9(std::uint32_t mask, int from_bit_inclusive) {
    for (int bit = from_bit_inclusive; bit < static_cast<int>(kCodecV3ChannelCount); ++bit) {
        if (((mask >> bit) & 1u) != 0)
            return bit;
    }
    return -1;
}

namespace {

void produced_mask_or_logical_channel_1024a9(std::uint32_t& mask, std::uint32_t channel) {
    if (channel <= 30u)
        mask |= 1u << channel;
}

void produced_mask_or_started_decode_channel_1024a9(
    std::uint32_t& mask,
    std::uint64_t frame_channel_ptr) {
    if (frame_channel_ptr == 0u)
        return;
    produced_mask_or_logical_channel_1024a9(
        mask,
        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + kFrameChannelOff_pred_src0_idx));
    produced_mask_or_logical_channel_1024a9(
        mask,
        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + kFrameChannelOff_pred_src1_idx));
    produced_mask_or_logical_channel_1024a9(
        mask,
        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + kFrameChannelOff_pred_src2_idx));
}

} // namespace

std::uint32_t select_segment_output_mask_1024a9(const OutputGeneratorSegment& seg) {
    // IDA LABEL_102..153: при v131 используется v107=v152 (frame mask), иначе маска delay-line buffer.
    if (seg.frame_ptr != 0 && use_started_decode_path_1024a9(seg))
        return seg.frame_flags;
    return seg.channel_mask;
}

std::uint32_t frame_output_mask_1024a9(const OutputGeneratorSegment& seg) {
    if (seg.frame_ptr == 0)
        return 0u;
    return *reinterpret_cast<const std::uint32_t*>(seg.frame_ptr + 24u);
}

std::int64_t process_segment_copy_only_path_1024a9(
    const OutputGeneratorSegment& seg,
    const OutputGeneratorApplyCallbacks& acb,
    std::uint32_t* produced_mask_out);

void copy_i32_samples(std::uint64_t dst_ptr, std::uint64_t src_ptr, std::uint64_t sample_count) {
    if (!dst_ptr || !src_ptr || sample_count == 0)
        return;
    std::memmove(
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(dst_ptr)),
        reinterpret_cast<const void*>(static_cast<std::uintptr_t>(src_ptr)),
        static_cast<std::size_t>(sample_count) * sizeof(std::int32_t));
}

bool should_use_scalar_copy_1024a9(
    std::uint64_t src_ptr,
    std::uint64_t dst_ptr,
    std::uint64_t sample_count,
    std::uint64_t seg_end) {
    // IDA OutputGenerator_process LABEL_104: scalar when len<0x10, len==0, high dword set, or ptr gap<0x20.
    if (((sample_count - 1ull) >> 32) != 0u || static_cast<std::uint32_t>(sample_count) == 0u || sample_count < 16u)
        return true;
    if (dst_ptr < src_ptr) {
        if ((src_ptr - dst_ptr) < 0x20u)
            return true;
    } else if ((dst_ptr - src_ptr) < 0x20u) {
        return true;
    }
    const std::uint64_t src_end = src_ptr + 4ull * sample_count;
    const std::uint64_t dst_end = dst_ptr + 4ull * seg_end;
    return (dst_ptr < src_end) && (src_ptr < dst_end);
}

void copy_i32_samples_fast_1024a9(std::uint64_t dst_ptr, std::uint64_t src_ptr, std::uint64_t sample_count) {
    if (!dst_ptr || !src_ptr || sample_count == 0u)
        return;
#if AURO3DENG_SSE2_PLATFORM
    auto* dst = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(dst_ptr));
    const auto* src = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(src_ptr));
    std::uint64_t offset_bytes = 0u;
    const std::uint64_t fast_bytes = (sample_count * 4u) & ~31u;
    while (offset_bytes < fast_bytes) {
        const __m128i chunk0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + offset_bytes));
        const __m128i chunk1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + offset_bytes + 16u));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + offset_bytes), chunk0);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + offset_bytes + 16u), chunk1);
        offset_bytes += 32u;
    }
    const std::uint64_t tail_samples = (sample_count * 4u - offset_bytes) / 4u;
    for (std::uint64_t i = 0; i < tail_samples; ++i) {
        *reinterpret_cast<std::int32_t*>(dst + offset_bytes + i * 4u) =
            *reinterpret_cast<const std::int32_t*>(src + offset_bytes + i * 4u);
    }
#else
    std::memcpy(
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(dst_ptr)),
        reinterpret_cast<const void*>(static_cast<std::uintptr_t>(src_ptr)),
        static_cast<std::size_t>(sample_count) * sizeof(std::int32_t));
#endif
}

std::uint64_t segment_output_start_1024a9(
    const OutputGeneratorSegment& seg,
    const OutputGeneratorApplyCallbacks& acb) {
    return (seg.start >= acb.output_block_start) ? (seg.start - acb.output_block_start) : seg.start;
}

void copy_segment_channel_samples_1024a9(
    const OutputGeneratorSegment& seg,
    const OutputGeneratorApplyCallbacks& acb,
    std::uint32_t channel,
    std::uint64_t src_ptr) {
    if (seg.len == 0 || src_ptr == 0)
        return;
    const std::uint64_t out_base = acb.get_output_channel_base(acb.user, channel);
    if (out_base == 0)
        return;
    const std::uint64_t out_start = segment_output_start_1024a9(seg, acb);
    const std::uint64_t dst = out_base + 4ull * out_start;
    const std::uint64_t seg_end = out_start + seg.len;
    if (should_use_scalar_copy_1024a9(src_ptr, dst, seg.len, seg_end)) {
        copy_i32_samples(dst, src_ptr, seg.len);
    } else {
        copy_i32_samples_fast_1024a9(dst, src_ptr, seg.len);
    }
}

void zero_i32_samples_1024a9(std::uint64_t dst_ptr, std::uint64_t sample_count) {
    if (!dst_ptr || sample_count == 0)
        return;
    std::memset(
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(dst_ptr)),
        0,
        static_cast<std::size_t>(sample_count) * sizeof(std::int32_t));
}

// IDA LABEL_144 (host): zero output channels in segment mask but outside produced_mask.
// Native started path (v131) skips this — uses frame+24 mask at LABEL_153 only.
void zero_unproduced_output_channels_1024a9(
    const OutputGeneratorSegment& seg,
    const OutputGeneratorApplyCallbacks& acb,
    std::uint32_t produced_mask,
    std::uint32_t channel_universe_mask) {
    // IDA LABEL_144 @ 0x52B590: runs for started and non-started (after LABEL_103 / LABEL_143).
    // For j in 0..30: if bit j not in produced (v40), zero output channel j when ptr != 0.
    if (seg.len == 0u)
        return;
    const std::uint32_t universe = channel_universe_mask & kCodecV3ChannelMask;
    const std::uint32_t missing = universe & ~produced_mask;
    for (int ch = next_set_channel_bit_1024a9(missing, 0);
         ch >= 0;
         ch = next_set_channel_bit_1024a9(missing, ch + 1)) {
        const std::uint64_t out_base = acb.get_output_channel_base(acb.user, static_cast<std::uint32_t>(ch));
        if (out_base == 0u)
            continue;
        zero_i32_samples_1024a9(out_base + 4ull * segment_output_start_1024a9(seg, acb), seg.len);
    }
}

// IDA: `auro_codec_v3_decoder_OutputGenerator_cross_fade_` @ `auro_engine_v4_ida::kLibauro_codec_OutputGenerator_cross_fade_inner`.
std::int64_t output_generator_cross_fade_52b0b0_partial(
    std::uint8_t* output_generator_base,
    std::uint64_t output_channels_table_base,
    std::uint32_t fade_in_mask,
    std::uint64_t delay_line_buffer,
    std::uint64_t segment_start) {
    if (!output_generator_base || output_channels_table_base == 0)
        return reinterpret_cast<std::int64_t>(output_generator_base);

    const std::uint64_t total_samples =
        *reinterpret_cast<const std::uint64_t*>(output_generator_base + kOgOff_total_samples);
    const std::uint32_t block_count =
        *reinterpret_cast<const std::uint32_t*>(output_generator_base + kOgOff_crossfade_block_count);
    if (total_samples == 0 || block_count == 0)
        return reinterpret_cast<std::int64_t>(output_generator_base);

    const float step = 1.0f / static_cast<float>(total_samples);
    const std::uint32_t delay_mask = delay_line_buffer
        ? (*reinterpret_cast<const std::uint32_t*>(delay_line_buffer) & kCodecV3ChannelMask)
        : 0u;
    const std::uint32_t in_mask = fade_in_mask & kCodecV3ChannelMask;

    for (std::uint32_t block = 0; block < block_count; ++block) {
        const std::uint64_t fade_sample_base = segment_start + static_cast<std::uint64_t>(block) * 32ull;
        float fade_in[32]{};
        float fade_old[32]{};
        for (std::uint32_t i = 0; i < 32u; ++i) {
            const float gain = static_cast<float>(fade_sample_base + i) * step;
            fade_in[i] = gain;
            fade_old[i] = 1.0f - gain;
        }

        for (std::uint32_t ch = 0; ch < kCodecV3ChannelCount; ++ch) {
            const std::uint64_t out_base =
                *reinterpret_cast<const std::uint64_t*>(output_channels_table_base + 16ull + 8ull * ch);
            if (out_base == 0)
                continue;

            auto* out = reinterpret_cast<std::int32_t*>(
                static_cast<std::uintptr_t>(out_base + 4ull * segment_start));
            if (((in_mask >> ch) & 1u) != 0) {
                for (std::uint32_t i = 0; i < 32u; ++i)
                    out[i] = static_cast<std::int32_t>(static_cast<float>(out[i]) * fade_in[i]);
            }

            if (((delay_mask >> ch) & 1u) == 0)
                continue;
            const std::uint64_t old_base = delay_line_get_channel_from_buffer_106ab0(delay_line_buffer, ch, 0);
            if (old_base == 0)
                continue;
            const auto* old = reinterpret_cast<const std::int32_t*>(static_cast<std::uintptr_t>(old_base));
            for (std::uint32_t i = 0; i < 32u; ++i) {
                out[i] = static_cast<std::int32_t>(
                    static_cast<float>(old[i]) * fade_old[i] + static_cast<float>(out[i]));
            }
        }
    }

    return reinterpret_cast<std::int64_t>(output_generator_base);
}

std::int64_t warmup_zero_len_channels_1024a9(
    const OutputGeneratorSegment& seg,
    const OutputGeneratorApplyCallbacks& acb) {
    // IDA LABEL_127..181: при v31==0 вызываются get_channel только для активных битов mask.
    // Порядок вызовов по коду — обход только установленных битов, от меньшего к большему.
    for (int ch = next_set_channel_bit_1024a9(seg.channel_mask, 0);
         ch >= 0;
         ch = next_set_channel_bit_1024a9(seg.channel_mask, ch + 1)) {
        (void)acb.get_delay_line_channel(acb.user, static_cast<std::uint32_t>(ch), seg.start);
    }
    return 0;
}

std::int64_t process_segment_copy_input_path_1024a9(
    const OutputGeneratorSegment& seg,
    const OutputGeneratorApplyCallbacks& acb,
    std::uint32_t* produced_mask_out) {
    // IDA LABEL_104: v128 = v139 & 0x7FFFFFFF.
    const std::uint32_t mask = acb.input_mask & 0x7FFFFFFFu & kCodecV3ChannelMask;
    if (seg.len == 0u) {
        if (produced_mask_out)
            *produced_mask_out |= mask;
        return 0;
    }
    for (int ch = next_set_channel_bit_1024a9(mask, 0);
         ch >= 0;
         ch = next_set_channel_bit_1024a9(mask, ch + 1)) {
        const auto channel = static_cast<std::uint32_t>(ch);
        const std::uint64_t src_base =
            acb.get_input_channel_base ? acb.get_input_channel_base(acb.user, channel) : 0u;
        const std::uint64_t out_base = acb.get_output_channel_base(acb.user, channel);
        if (src_base == 0u || out_base == 0u)
            continue;
        const std::uint64_t dst = out_base + 4ull * segment_output_start_1024a9(seg, acb);
        const std::uint64_t src = src_base + 4ull * seg.start;
        const std::uint64_t seg_end = seg.start + seg.len;
        if (should_use_scalar_copy_1024a9(src, dst, seg.len, seg_end)) {
            copy_i32_samples(dst, src, seg.len);
        } else {
            copy_i32_samples_fast_1024a9(dst, src, seg.len);
        }
        if (produced_mask_out)
            produced_mask_or_logical_channel_1024a9(*produced_mask_out, channel);
    }
    return 0;
}

struct OgRawRuntimeCtx {
    std::uint8_t* og = nullptr;
    std::uint64_t out_tbl = 0;
    std::uint64_t delay_line_buffer = 0;
    std::int64_t delay_line_state_offset = 0;
    std::uint64_t delay_line_ptr = 0;
    std::uint64_t total_samples = 0;
    const CodecV3IoBufferDescEb5a0* input_desc = nullptr;
    std::uint32_t input_mask = 0;
    OutputGeneratorRuntimeFns1024a9 fns{};
    std::array<std::vector<std::int32_t>, kCodecV3ChannelCount> delay_windows{};
};

std::uint32_t raw_get_frame_channel_count(void*, std::uint64_t frame_ptr) {
    return *reinterpret_cast<std::uint32_t*>(frame_ptr + kFrameOff_channel_count_dword);
}

std::uint64_t raw_get_frame_channel_ptr(void*, std::uint64_t frame_ptr, std::uint32_t idx) {
    const std::uintptr_t slot = frame_ptr + kFrameOff_channel_slot_base + static_cast<std::uintptr_t>(idx) * kFrameStride_channel_slot;
    return *reinterpret_cast<std::uint64_t*>(slot + kFrameChSlotOff_channel_ptr_qword);
}

std::uint64_t raw_get_gr_state_ptr(void* user, std::uint32_t idx) {
    auto* c = reinterpret_cast<OgRawRuntimeCtx*>(user);
    return reinterpret_cast<std::uint64_t>(c->og + kOgOff_gr_state_base + static_cast<std::uintptr_t>(idx) * kOgStride_gr_state);
}

std::uint64_t raw_get_ex_state_ptr(void* user, std::uint32_t idx) {
    auto* c = reinterpret_cast<OgRawRuntimeCtx*>(user);
    return reinterpret_cast<std::uint64_t>(c->og + kOgOff_ex_state_base + static_cast<std::uintptr_t>(idx) * kOgStride_ex_state);
}

std::uint32_t* raw_get_channel_words_ptr(void*, std::uint64_t frame_channel_ptr) {
    return reinterpret_cast<std::uint32_t*>(frame_channel_ptr + kFrameChannelOff_stream_words);
}

std::uint64_t raw_get_channel_ctx_ptr(void*, std::uint64_t frame_channel_ptr) {
    return frame_channel_ptr + kFrameChannelOff_ctx_words;
}

std::int64_t golombrice_initialize_104e10(
    std::uint64_t gr_state_ptr,
    std::uint32_t* words_ptr,
    std::uint64_t ctx_ptr) {
    // libauro3d `0x104E10` / libauro `kLibauro_codec_channel_GolombRice_initialize`: zero flags -> ctx -> bit_count=31 -> first word.
    if (!gr_state_ptr || !words_ptr || !ctx_ptr)
        return 0;
    *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_counter_dword) = 0;
    *reinterpret_cast<std::uint64_t*>(gr_state_ptr + kGrStateOff_ctx_ptr_qword) = ctx_ptr;
    *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_bit_index_dword) = 31;
    const std::uint32_t first_word = *words_ptr;
    *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_accum_bits_dword) = first_word;
    *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_counter_dword) = 0;
    *reinterpret_cast<std::uint64_t*>(gr_state_ptr + kGrStateOff_words_ptr_qword) =
        reinterpret_cast<std::uint64_t>(words_ptr + 1);
    return first_word;
}

std::int64_t extrapolate_initialize_104440(std::uint64_t ex_state_ptr, std::uint64_t frame_channel_ptr) {
    // libauro3d `0x104440` / libauro `auro_codec_v3_ida::kLibauro_codec_channel_Extrapolate_initialize`.
    if (!ex_state_ptr || !frame_channel_ptr)
        return 0;
    *reinterpret_cast<std::uint64_t*>(ex_state_ptr + kExStateOff_head_qword) = 0;
    *reinterpret_cast<std::uint32_t*>(ex_state_ptr + kExStateOff_phase_dword) = 0;
    *reinterpret_cast<std::uint64_t*>(ex_state_ptr + kExStateOff_frame_channel_ptr_qword) = frame_channel_ptr;
    return 0;
}

const std::array<float, kExtrapolateScaleTableSize>& extrapolate_scale_table_1042a1() {
    static const std::array<float, kExtrapolateScaleTableSize> table = [] {
        std::array<float, kExtrapolateScaleTableSize> out{};
        for (std::size_t i = 0; i < out.size(); ++i)
            out[i] = std::exp(static_cast<float>(i) * kExtrapolateScaleExpStep_1042a1);

        // IDA 0x104404..0x104422 overwrites these exact slots after the expf loop.
        out[60] = 2.0f;
        out[120] = 4.0f;
        out[180] = 8.0f;
        out[240] = 16.0f;
        return out;
    }();
    return table;
}

std::int32_t clamp_pcm24_104460(std::int32_t value) {
    // IDA mix1/mix2 scalar: clamp to [ -8388608, 0x7FFFFF ] via >= 0x7FFFFF / < -8388607.
    if (value >= 0x7FFFFF)
        return kPcm24Max_104460;
    if (value < -8388607)
        return kPcm24Min_104460;
    return value;
}

std::int32_t scale_shift_clamp_pcm24_104460(std::int32_t value, float scale, std::uint32_t shift) {
    const auto scaled = static_cast<std::int64_t>(
        static_cast<std::int32_t>(static_cast<float>(value) * scale));
    const std::int64_t shifted = shift < 31u ? (scaled << shift) : scaled;
    if (shifted >= kPcm24Max_104460)
        return kPcm24Max_104460;
    if (shifted < kPcm24Min_104460)
        return kPcm24Min_104460;
    return static_cast<std::int32_t>(shifted);
}

std::int32_t div4_trunc0_104460(std::int32_t value) {
    const std::int32_t bias = (value >> 31) & 3;
    return (value + bias) >> 2;
}

// IDA extrapolate_mix3 @ 0x52E5C0: v47 = v39 + 3*v46 + 3; if (3*v46+v39 >= 0) v47 = 3*v46+v39; v49 = v47 >> 2.
std::int32_t div4_mix3_52e5c0(std::int32_t v39, std::int32_t v46) {
    const std::int32_t v48 = 3 * v46 + v39;
    std::int32_t v47 = v39 + 3 * v46 + 3;
    if (v48 >= 0)
        v47 = v48;
    return v47 >> 2;
}

// libauro3d `0x104460` / libauro `auro_codec_v3_ida::kLibauro_codec_channel_Extrapolate_process`.
std::int64_t extrapolate_process_104460_partial(
    std::uint64_t ex_state_ptr,
    std::uint64_t channel_ptr,
    std::uint64_t errors_buf_ptr,
    std::uint64_t src0_ptr,
    std::uint64_t src1_ptr,
    std::uint64_t src2_ptr,
    std::uint64_t sample_count) {
    if (!ex_state_ptr || !channel_ptr)
        return 0;

    const auto* frame_channel =
        reinterpret_cast<const std::uint32_t*>(*reinterpret_cast<const std::uint64_t*>(ex_state_ptr + kExStateOff_frame_channel_ptr_qword));
    if (!frame_channel)
        return 0;

    const std::uint32_t mode = frame_channel[kFrameChannelOff_ex_mode / 4u];
    const auto* src = reinterpret_cast<const std::int32_t*>(channel_ptr);
    if (!src)
        return 0;

    const auto& table = extrapolate_scale_table_1042a1();
    const std::uint32_t quant_shift = frame_channel[kFrameChannelOff_ex_quant_shift / 4u];
    if (quant_shift > 24u)
        return 0;
    const std::uint32_t shift = 24u - quant_shift;
    if (mode == 1u) {
        auto* dst0 = reinterpret_cast<std::int32_t*>(src0_ptr);
        if (!dst0)
            return 0;

        const std::uint32_t scale_index =
            frame_channel[kFrameChannelOff_ex_scale_idx0 / 4u] + frame_channel[kFrameChannelOff_ex_scale_idx1 / 4u];
        if (scale_index >= table.size())
            return 0;

        const std::int32_t mask = static_cast<std::int32_t>(0xFFFFFFFFu << shift);
        const float scale = table[scale_index];
        for (std::uint64_t i = 0; i < sample_count; ++i) {
            const std::int32_t scaled = static_cast<std::int32_t>(static_cast<float>(mask & src[i]) * scale);
            dst0[i] = clamp_pcm24_104460(scaled);
        }

        (void)errors_buf_ptr; // mode==1 path in IDA ignores error buffer.
        *reinterpret_cast<std::uint32_t*>(ex_state_ptr + kExStateOff_mode1_count_dword) += static_cast<std::uint32_t>(sample_count);
        return 0;
    }

    if (mode == 2u) {
        auto* errors = reinterpret_cast<const std::int32_t*>(errors_buf_ptr);
        auto* dst0 = reinterpret_cast<std::int32_t*>(src0_ptr);
        auto* dst1 = reinterpret_cast<std::int32_t*>(src1_ptr);
        if (!errors || !dst0 || !dst1)
            return 0;

        std::uint32_t produced = 0;
        std::uint32_t count = *reinterpret_cast<std::uint32_t*>(ex_state_ptr + kExStateOff_mode2_count_dword);

        if (produced < sample_count && count == 0u) {
            const std::int32_t decoded = src[0] >> shift;
            const std::int32_t accum =
                static_cast<std::int32_t>(frame_channel[kFrameChannelOff_seed0_secondary / 4u] + errors[0]);
            dst1[0] = accum;
            dst0[0] = decoded - accum;
            *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode2_prev_primary_dword) = dst0[0];
            *reinterpret_cast<std::uint32_t*>(ex_state_ptr + kExStateOff_mode2_count_dword) = ++count;
            produced = 1;
        }

        if (produced < sample_count && count == 1u) {
            const std::int32_t decoded = src[produced] >> shift;
            const std::int32_t last_residual =
                static_cast<std::int32_t>(frame_channel[kFrameChannelOff_seed0_primary / 4u]);
            const std::int32_t accum = last_residual + errors[produced];
            dst0[produced] = accum;
            dst1[produced] = decoded - accum;
            *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode2_last_residual_dword) = last_residual;
            *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode2_prev_secondary_dword) = dst1[produced];
            *reinterpret_cast<std::uint32_t*>(ex_state_ptr + kExStateOff_mode2_count_dword) = ++count;
            ++produced;
        }

        if (produced < sample_count) {
            // IDA mix2 @ 0x52E1C0: when (count&1)==0 → predicted=a5/dst0, residual=a6/dst1.
            auto* predicted_dst = ((count & 1u) == 0u) ? dst0 : dst1;
            auto* residual_dst = ((count & 1u) == 0u) ? dst1 : dst0;
            std::int32_t predictor_base =
                *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode2_last_residual_dword);
            std::int32_t prev_predicted =
                *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode2_prev_primary_dword);

            for (std::uint64_t i = produced; i < sample_count; ++i) {
                auto* predicted_cur = predicted_dst;
                const std::int32_t decoded = src[i] >> shift;
                const std::int32_t predicted = 2 * predictor_base - prev_predicted;
                predicted_cur[i] = predicted;
                residual_dst[i] = decoded - predicted;

                prev_predicted =
                    *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode2_prev_secondary_dword);
                *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode2_prev_primary_dword) = prev_predicted;
                predictor_base = residual_dst[i] - errors[i];
                *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode2_last_residual_dword) = predictor_base;
                *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode2_prev_secondary_dword) = predicted_cur[i];
                ++*reinterpret_cast<std::uint32_t*>(ex_state_ptr + kExStateOff_mode2_count_dword);

                predicted_dst = residual_dst;
                residual_dst = predicted_cur;
            }
        }

        const std::uint32_t scale_idx0 =
            frame_channel[kFrameChannelOff_ex_scale_idx0 / 4u] + frame_channel[kFrameChannelOff_ex_scale_idx1 / 4u];
        const std::uint32_t scale_idx1 =
            frame_channel[(kFrameChannelOff_ex_scale_idx0 / 4u) + 1u] + frame_channel[kFrameChannelOff_ex_scale_idx1 / 4u];
        if (scale_idx0 >= table.size() || scale_idx1 >= table.size())
            return 0;

        const float scale0 = table[scale_idx0];
        const float scale1 = table[scale_idx1];
        for (std::uint64_t i = 0; i < sample_count; ++i) {
            dst0[i] = scale_shift_clamp_pcm24_104460(dst0[i], scale0, shift);
            dst1[i] = scale_shift_clamp_pcm24_104460(dst1[i], scale1, shift);
        }
        return 0;
    }

    if (mode == 3u) {
        auto* errors = reinterpret_cast<const std::int32_t*>(errors_buf_ptr);
        auto* dst0 = reinterpret_cast<std::int32_t*>(src0_ptr);
        auto* dst1 = reinterpret_cast<std::int32_t*>(src1_ptr);
        auto* dst2 = reinterpret_cast<std::int32_t*>(src2_ptr);
        if (!errors || !dst0 || !dst1 || !dst2)
            return 0;

        std::uint32_t produced = 0;
        std::uint32_t count = *reinterpret_cast<std::uint32_t*>(ex_state_ptr + kExStateOff_phase_dword);
        std::uint32_t initial_count = count;

        if (produced < sample_count && count == 0u) {
            const std::int32_t decoded = src[0] >> shift;
            dst1[0] = static_cast<std::int32_t>(frame_channel[kFrameChannelOff_seed0_primary / 4u]);
            dst2[0] = static_cast<std::int32_t>(frame_channel[kFrameChannelOff_seed0_secondary / 4u]);
            dst0[0] = decoded - (errors[0] + errors[1]) - dst2[0] - dst1[0];
            *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_last_a_dword) = dst2[0];
            *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_last_c_dword) = dst0[0];
            *reinterpret_cast<std::uint32_t*>(ex_state_ptr + kExStateOff_phase_dword) = ++count;
            ++produced;
        }

        if (produced < sample_count && count == 1u) {
            const std::int32_t decoded = src[produced] >> shift;
            dst2[produced] = static_cast<std::int32_t>(frame_channel[kFrameChannelOff_seed1_secondary / 4u]);
            dst0[produced] = static_cast<std::int32_t>(frame_channel[kFrameChannelOff_seed1_primary / 4u]);
            dst1[produced] = decoded - (errors[2 * produced] + errors[2 * produced + 1]) - dst0[produced] - dst2[produced];
            *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_last_a_dword) = dst0[produced];
            *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_last_b_dword) =
                *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_last_c_dword);
            *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_last_c_dword) = dst1[produced];
            *reinterpret_cast<std::uint32_t*>(ex_state_ptr + kExStateOff_phase_dword) = ++count;
            ++produced;
        }

        if (produced < sample_count && count == 2u) {
            const std::int32_t v39 =
                *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_last_b_dword);
            const std::int32_t v38 =
                *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_last_a_dword);
            const std::int32_t v26 = 4 * v38 - 3 * v39;
            const std::int32_t decoded = src[produced] >> shift;
            dst0[produced] = div4_mix3_52e5c0(v39, v26);
            dst1[produced] = static_cast<std::int32_t>(frame_channel[kFrameChannelOff_seed2_primary / 4u]);
            dst2[produced] =
                decoded - (errors[2 * produced] + errors[2 * produced + 1]) - dst1[produced] - dst0[produced];
            *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_last_a_dword) = dst1[produced];
            *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_last_b_dword) =
                *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_last_c_dword);
            *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_last_c_dword) = dst2[produced];
            *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_predictor_dword) = v26;
            *reinterpret_cast<std::uint32_t*>(ex_state_ptr + kExStateOff_phase_dword) = ++count;
            ++produced;
        }

        // IDA mix3 @ 0x52E5C0 pointer rotate init from count%3:
        // 0: v34=a5/dst0, v35=a7/dst2, v36=a6/dst1
        // 1: v34=a6/dst1, v35=a5/dst0, v36=a7/dst2
        // 2: v34=a7/dst2, v35=a6/dst1, v36=a5/dst0
        auto* v34 = dst0;
        auto* v35 = dst2;
        auto* v36 = dst1;
        const std::uint32_t mod = count % 3u;
        if (mod == 1u) {
            v34 = dst1;
            v35 = dst0;
            v36 = dst2;
        } else if (mod == 2u) {
            v34 = dst2;
            v35 = dst1;
            v36 = dst0;
        }

        if (produced < sample_count) {
            std::int32_t v38 =
                *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_last_a_dword);
            std::int32_t v39 =
                *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_last_b_dword);

            for (std::uint64_t i = produced; i < sample_count; ++i) {
                const std::int32_t decoded = src[i] >> shift;
                const std::int32_t v46 = 4 * v38 - 3 * v39;
                const std::int32_t v49 = div4_mix3_52e5c0(v39, v46);
                const std::int32_t err_pair =
                    errors[2 * i] + errors[2 * i + 1u];
                const std::int32_t a1_1 =
                    *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_predictor_dword);

                v35[i] = decoded - v49 - a1_1 - err_pair;
                v34[i] = a1_1;
                v36[i] = v49;

                const std::int32_t v52 = v35[i];
                const std::int32_t old_a1_4 =
                    *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_last_c_dword);
                const std::int32_t old_a1_1 = a1_1;

                *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_last_a_dword) = v52;
                *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_last_b_dword) = old_a1_4;
                *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_last_c_dword) = old_a1_1;
                *reinterpret_cast<std::int32_t*>(ex_state_ptr + kExStateOff_mode3_predictor_dword) = v46;
                ++*reinterpret_cast<std::uint32_t*>(ex_state_ptr + kExStateOff_phase_dword);

                v38 = v52;
                v39 = old_a1_4;

                auto* v45 = v36;
                v36 = v35;
                v35 = v34;
                v34 = v45;
            }
        }

        if (sample_count != 0u) {
            // IDA residual-add state machine from initial_count%3:
            // 0→a6/a7 (dst1/dst2), 1→a5/a7 (dst0/dst2), 2→a5/a6 (dst0/dst1).
            std::uint32_t phase_mod3 = initial_count % 3u;
            for (std::uint64_t i = 0; i < sample_count; ++i) {
                std::int32_t* first_target = dst1;
                std::int32_t* second_target = dst2;
                if (phase_mod3 == 1u) {
                    first_target = dst0;
                    second_target = dst2;
                } else if (phase_mod3 == 2u) {
                    first_target = dst0;
                    second_target = dst1;
                }

                const std::uint64_t pair_index = 2u * i;
                first_target[i] += errors[pair_index];
                second_target[i] += errors[pair_index + 1u];
                phase_mod3 = (phase_mod3 + 1u) % 3u;
            }
        }

        const std::uint32_t base_scale = frame_channel[kFrameChannelOff_ex_scale_idx1 / 4u];
        const std::uint32_t scale_idx0 = frame_channel[(kFrameChannelOff_ex_scale_idx0 / 4u) + 0u] + base_scale;
        const std::uint32_t scale_idx1 = frame_channel[(kFrameChannelOff_ex_scale_idx0 / 4u) + 1u] + base_scale;
        const std::uint32_t scale_idx2 = frame_channel[(kFrameChannelOff_ex_scale_idx0 / 4u) + 2u] + base_scale;
        if (scale_idx0 >= table.size() || scale_idx1 >= table.size() || scale_idx2 >= table.size())
            return 0;

        const float scale0 = table[scale_idx0];
        const float scale1 = table[scale_idx1];
        const float scale2 = table[scale_idx2];
        for (std::uint64_t i = 0; i < sample_count; ++i) {
            dst0[i] = scale_shift_clamp_pcm24_104460(dst0[i], scale0, shift);
            dst1[i] = scale_shift_clamp_pcm24_104460(dst1[i], scale1, shift);
            dst2[i] = scale_shift_clamp_pcm24_104460(dst2[i], scale2, shift);
        }
        return 0;
    }

    return 0;
}

std::uint32_t gr_read_stream_bit_104e40(std::uint64_t gr_state_ptr) {
    auto* words_ptr = reinterpret_cast<std::uint32_t*>(*reinterpret_cast<std::uint64_t*>(gr_state_ptr + kGrStateOff_words_ptr_qword));
    std::int32_t bit_index = *reinterpret_cast<std::int32_t*>(gr_state_ptr + kGrStateOff_bit_index_dword);
    if (!words_ptr || bit_index < 0 || bit_index > 31)
        return 0;

    const std::uint32_t bit = (words_ptr[0] >> static_cast<std::uint32_t>(bit_index)) & 1u;
    if (bit_index <= 0) {
        *reinterpret_cast<std::uint64_t*>(gr_state_ptr + kGrStateOff_words_ptr_qword) =
            reinterpret_cast<std::uint64_t>(words_ptr + 1);
        *reinterpret_cast<std::int32_t*>(gr_state_ptr + kGrStateOff_bit_index_dword) = 31;
    } else {
        *reinterpret_cast<std::int32_t*>(gr_state_ptr + kGrStateOff_bit_index_dword) = bit_index - 1;
    }
    return bit;
}

void gr_flush_label71_104e40(
    std::uint64_t gr_state_ptr,
    std::uint32_t value_mask_cursor,
    std::uint32_t& accum_bits) {
    // IDA LABEL_71 @ 0x52D8B0: v58 = 2 * v22; do { read bit; v7 = bit + 2*v7; v58 *= 2; } while (v58);
    std::uint32_t flush_scale = value_mask_cursor << 1u;
    while (flush_scale != 0u) {
        accum_bits = gr_read_stream_bit_104e40(gr_state_ptr) + 2u * accum_bits;
        flush_scale *= 2u;
    }
}

std::uint32_t gr_append_bits_104e40(std::uint64_t gr_state_ptr, std::uint32_t accum_bits, std::uint32_t bit_count) {
    for (std::uint32_t i = 0; i < bit_count; ++i)
        accum_bits = gr_read_stream_bit_104e40(gr_state_ptr) + 2u * accum_bits;
    return accum_bits;
}

std::uint32_t gr_pow2_table_104e40(std::uint32_t bit_index) {
    return (bit_index < 31u) ? (1u << bit_index) : 0x80000000u;
}

std::uint32_t gr_extract_packed_unsigned_104e40(
    const std::uint32_t* packed_words,
    std::uint32_t packed_index,
    std::uint32_t bit_width,
    std::uint32_t value_mask) {
    const std::uint32_t word_index = packed_index >> 5;
    const std::uint32_t bit_offset = packed_index & 31u;
    const std::uint32_t first_word = packed_words[word_index];
    const std::int32_t shift = 32 - static_cast<std::int32_t>(bit_offset) - static_cast<std::int32_t>(bit_width);
    if (shift >= 0)
        return value_mask & (first_word >> static_cast<std::uint32_t>(shift));

    const std::uint32_t bits_from_first = 32u - bit_offset;
    const std::uint32_t bits_from_next = bit_width - bits_from_first;
    const std::uint32_t first_mask = (bits_from_first == 32u) ? 0xFFFFFFFFu : ((1u << bits_from_first) - 1u);
    const std::uint32_t next_word = packed_words[word_index + 1];
    return ((first_word & first_mask) << bits_from_next) | (next_word >> (32u - bits_from_next));
}

std::int32_t gr_sign_extend_104e40(std::uint32_t raw_value, std::uint32_t bit_width) {
    if (bit_width == 0u)
        return 0;
    if (bit_width >= 32u)
        return static_cast<std::int32_t>(raw_value);
    const std::uint32_t sign_bit = 1u << (bit_width - 1u);
    const std::uint32_t magnitude_mask = sign_bit - 1u;
    if ((raw_value & sign_bit) == 0u)
        return static_cast<std::int32_t>(raw_value & magnitude_mask);
    return -static_cast<std::int32_t>(raw_value & magnitude_mask);
}

// libauro.so: `auro_codec_v3_decoder_channel_GolombRice_get_errors` @ `auro_engine_v4_ida::kLibauro_codec_channel_GolombRice_get_errors`.
std::int64_t golombrice_get_errors_104e40(
    std::uint64_t gr_state_ptr,
    std::uint64_t frame_channel_ptr,
    std::uint64_t errors_buf_ptr,
    std::uint64_t sample_count) {
    if (!gr_state_ptr || !frame_channel_ptr || !errors_buf_ptr)
        return 0;

    auto* packed_words = reinterpret_cast<const std::uint32_t*>(
        *reinterpret_cast<const std::uint64_t*>(gr_state_ptr + kGrStateOff_ctx_ptr_qword));
    auto* out = reinterpret_cast<std::int32_t*>(errors_buf_ptr);
    if (!packed_words || !out)
        return 0;

    std::uint32_t mode = *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + kFrameChannelOff_ex_mode);
    *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_mode_dword) = mode;
    if (mode < 2u)
        return static_cast<std::int64_t>(sample_count);

    const std::uint32_t flags =
        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + kFrameChannelOff_gr_packed_flags);
    if ((flags & 0x40000000u) == 0)
        *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_k_dword) = (flags >> 24u) & 0xFu;

    const std::uint32_t base_index =
        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + kFrameChannelOff_gr_base_index);
    *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_base_index_dword) = base_index;

    std::uint32_t accum_bits = *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_accum_bits_dword);
    const std::uint32_t bit_width =
        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + kFrameChannelOff_gr_bit_width);
    if (bit_width == 0u)
        return 0;
    const std::uint32_t value_mask = (bit_width >= 32u) ? 0xFFFFFFFFu : (~(0xFFFFFFFFu << bit_width));

    for (std::uint64_t i = 0; i < sample_count; ++i) {
        std::uint32_t unary_scale = gr_pow2_table_104e40(
            *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_k_dword));
        std::uint32_t unary_mask = 0x80000000u;
        std::uint32_t counter = *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_counter_dword);

        // IDA: ((unsigned __int8)(flags>>30) & (counter==0)) == 1 → только bit30, без bit31.
        if ((((flags >> 30u) & 1u) != 0u) && counter == 0u) {
            const std::uint32_t dynamic_k = accum_bits >> 29u;
            *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_k_dword) = dynamic_k;
            unary_scale = gr_pow2_table_104e40(dynamic_k);
            unary_mask = 0x10000000u;
        }

        counter = (counter + 1u > 31u) ? 0u : (counter + 1u);
        *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_counter_dword) = counter;

        std::uint32_t unary_count = 0;
        if ((accum_bits & unary_mask) != 0) {
            unary_mask >>= 1u;
            while (true) {
                if (unary_mask == 1u) {
                    accum_bits = gr_append_bits_104e40(gr_state_ptr, accum_bits, 31u);
                    unary_mask = 0x80000000u;
                }
                ++unary_count;
                if ((accum_bits & unary_mask) == 0)
                    break;
                unary_mask >>= 1u;
            }
        }

        std::uint32_t value_mask_cursor = unary_mask >> 1u;
        if (value_mask_cursor == 1u) {
            accum_bits = gr_append_bits_104e40(gr_state_ptr, accum_bits, 31u);
            value_mask_cursor = 0x80000000u;
        }

        std::uint32_t extra_value = 0;
        const std::uint32_t k_value = *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_k_dword);
        for (std::uint32_t bit = 0; bit < k_value; ++bit) {
            if ((accum_bits & value_mask_cursor) != 0) {
                extra_value += (bit < 16u) ? kDword289CE0[bit] : gr_pow2_table_104e40(bit);
            }
            value_mask_cursor >>= 1u;
            if (value_mask_cursor == 1u) {
                accum_bits = gr_append_bits_104e40(gr_state_ptr, accum_bits, 31u);
                value_mask_cursor = 0x80000000u;
            }
        }

        const std::uint32_t code_index = extra_value + unary_scale * unary_count;
        const std::uint32_t packed_index0 = bit_width * code_index;
        const std::uint32_t raw0 =
            gr_extract_packed_unsigned_104e40(packed_words, packed_index0, bit_width, value_mask);
        *out = gr_sign_extend_104e40(raw0, bit_width);

        if (mode == 3u) {
            const std::uint32_t packed_index1 = bit_width * (base_index + code_index);
            const std::uint32_t raw1 =
                gr_extract_packed_unsigned_104e40(packed_words, packed_index1, bit_width, value_mask);
            out[1] = gr_sign_extend_104e40(raw1, bit_width);
            out += 2;
        } else {
            ++out;
        }

        // IDA LABEL_71 @ 0x52D8B0
        gr_flush_label71_104e40(gr_state_ptr, value_mask_cursor, accum_bits);
    }

    *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_accum_bits_dword) = accum_bits;
    return 0;
}

std::int64_t raw_gr_initialize(void* user, std::uint64_t gr_state_ptr, std::uint32_t* words_ptr, std::uint64_t ctx_ptr) {
    auto* c = reinterpret_cast<OgRawRuntimeCtx*>(user);
    if (c->fns.golombrice_initialize)
        return c->fns.golombrice_initialize(gr_state_ptr, words_ptr, ctx_ptr);
    return golombrice_initialize_104e10(gr_state_ptr, words_ptr, ctx_ptr);
}

std::int64_t raw_ex_initialize(void* user, std::uint64_t ex_state_ptr, std::uint64_t frame_channel_ptr) {
    auto* c = reinterpret_cast<OgRawRuntimeCtx*>(user);
    if (c->fns.extrapolate_initialize)
        return c->fns.extrapolate_initialize(ex_state_ptr, frame_channel_ptr);
    return extrapolate_initialize_104440(ex_state_ptr, frame_channel_ptr);
}

std::uint64_t raw_get_delay_line_channel(void* user, std::uint32_t channel, std::uint64_t start) {
    auto* c = reinterpret_cast<OgRawRuntimeCtx*>(user);
    if (!c->fns.delay_line_get_channel || c->delay_line_buffer == 0)
        return 0;
    const std::int64_t adjusted_start_i64 =
        static_cast<std::int64_t>(start) - c->delay_line_state_offset;
    if (adjusted_start_i64 < 0)
        return 0;
    const auto adjusted_start = static_cast<std::uint64_t>(adjusted_start_i64);
    const auto* state = reinterpret_cast<const DelayLineState106b40*>(
        static_cast<std::uintptr_t>(c->delay_line_ptr));
    if (!state || state->samples_per_block == 0 || adjusted_start >= state->samples_per_block) {
        return c->fns.delay_line_get_channel(c->delay_line_buffer, channel, adjusted_start);
    }
    const std::uint64_t direct = c->fns.delay_line_get_channel(c->delay_line_buffer, channel, adjusted_start);
    if (direct == 0u)
        return 0;
    const std::uint64_t requested = c->total_samples != 0u ? c->total_samples : state->samples_per_block;
    if (adjusted_start + requested <= state->samples_per_block)
        return direct;

    auto& window = c->delay_windows[channel];
    if (window.size() < requested)
        window.assign(static_cast<std::size_t>(requested), 0);
    else
        std::fill(window.begin(), window.begin() + static_cast<std::ptrdiff_t>(requested), 0);

    std::uint64_t copied = 0;
    std::uint64_t slot_ptr = c->delay_line_buffer;
    std::uint64_t slot_start = adjusted_start;
    while (copied < requested && slot_ptr != 0u) {
        const std::uint64_t src = c->fns.delay_line_get_channel(slot_ptr, channel, slot_start);
        const std::uint64_t avail = state->samples_per_block - slot_start;
        const std::uint64_t take = std::min<std::uint64_t>(requested - copied, avail);
        if (src != 0u && take != 0u) {
            std::memcpy(
                window.data() + copied,
                reinterpret_cast<const void*>(static_cast<std::uintptr_t>(src)),
                static_cast<std::size_t>(take) * sizeof(std::int32_t));
        }
        copied += take;
        slot_start = 0;
        const auto* slot = reinterpret_cast<const DelayLineBufferSlot106b40*>(
            static_cast<std::uintptr_t>(slot_ptr));
        const std::uint64_t slot_index =
            (slot_ptr - state->ring_storage_base) / kDelayLineBufferSlotStrideBytes;
        const std::uint64_t next_slot = (slot_index + 1u) % state->ring_slot_count;
        slot_ptr = state->ring_storage_base + next_slot * kDelayLineBufferSlotStrideBytes;
        if (slot == reinterpret_cast<const DelayLineBufferSlot106b40*>(
                static_cast<std::uintptr_t>(slot_ptr))) {
            break;
        }
    }
    return reinterpret_cast<std::uint64_t>(window.data());
}

std::uint64_t raw_get_input_channel_base(void* user, std::uint32_t channel) {
    auto* c = reinterpret_cast<OgRawRuntimeCtx*>(user);
    if (!c || !c->input_desc || channel >= kCodecV3ChannelCount)
        return 0;
    return c->input_desc->channel_ptr[channel];
}

std::uint64_t raw_get_output_channel_base(void* user, std::uint32_t channel) {
    auto* c = reinterpret_cast<OgRawRuntimeCtx*>(user);
    if (channel >= kCodecV3ChannelCount || c->out_tbl == 0)
        return 0;
    return *reinterpret_cast<std::uint64_t*>(c->out_tbl + 8ull * channel + 16ull);
}

bool raw_find_frame_channel_slot(
    const OutputGeneratorSegment* seg,
    std::uint32_t channel,
    std::uint32_t* out_slot_idx,
    std::uint64_t* out_frame_ch_ptr,
    bool* out_slot_active) {
    if (!seg || seg->frame_ptr == 0)
        return false;
    const std::uint32_t n = *reinterpret_cast<const std::uint32_t*>(seg->frame_ptr + kFrameOff_channel_count_dword);
    for (std::uint32_t i = 0; i < n; ++i) {
        const std::uintptr_t slot = seg->frame_ptr + kFrameOff_channel_slot_base + static_cast<std::uintptr_t>(i) * kFrameStride_channel_slot;
        const std::uint32_t out_ch = *reinterpret_cast<const std::uint32_t*>(slot + kFrameChSlotOff_channel_index);
        if (out_ch == channel) {
            if (out_slot_idx)
                *out_slot_idx = i;
            if (out_frame_ch_ptr)
                *out_frame_ch_ptr = *reinterpret_cast<const std::uint64_t*>(slot + kFrameChSlotOff_channel_ptr_qword);
            if (out_slot_active)
                *out_slot_active = (*reinterpret_cast<const std::uint32_t*>(slot + kFrameChSlotOff_active_flag) != 0);
            return true;
        }
    }
    return false;
}

bool should_apply_frame_metadata_1024a9(std::uint64_t frame_ptr, std::uint64_t timeline_cursor) {
    if (frame_ptr == 0)
        return false;
    const std::uint32_t frame_start = *reinterpret_cast<const std::uint32_t*>(frame_ptr + 0);
    // Metadata table update is keyed by frame_start == timeline (channel_count may be 0).
    return frame_start == static_cast<std::uint32_t>(timeline_cursor);
}

bool should_prepare_frame_init_1024a9(std::uint64_t frame_ptr, std::uint64_t timeline_cursor) {
    if (!should_apply_frame_metadata_1024a9(frame_ptr, timeline_cursor))
        return false;
    const std::uint32_t channel_count =
        *reinterpret_cast<const std::uint32_t*>(frame_ptr + kFrameOff_channel_count_dword);
    // IDA 0x102472..0x10247B: GR/Extrapolate init only when channel_count != 0.
    return channel_count != 0;
}

void output_generator_store_channel_gain_1024a9(
    std::uint8_t* output_generator_base,
    std::uint32_t channel,
    std::int32_t coefficient) {
    if (!output_generator_base || channel >= kCodecV3ChannelCount)
        return;
    const std::uintptr_t dst = kOgOff_channel_gain_table + static_cast<std::uintptr_t>(channel) * 8u;
    *reinterpret_cast<std::uint32_t*>(output_generator_base + dst) = 1u;
    *reinterpret_cast<float*>(output_generator_base + dst + 4u) =
        static_cast<float>(coefficient) * -0.1f;
}

void output_generator_apply_frame_metadata_1024a9(
    std::uint8_t* output_generator_base,
    const OutputGeneratorSegment& seg,
    std::uint64_t timeline_cursor,
    const OutputGeneratorFrameInitCallbacks& cb) {
    if (!output_generator_base || seg.frame_ptr == 0)
        return;
    if (!should_apply_frame_metadata_1024a9(seg.frame_ptr, timeline_cursor))
        return;

    const std::uint64_t total_samples =
        *reinterpret_cast<const std::uint64_t*>(output_generator_base + kOgOff_total_samples);
    const std::uint32_t latency_blocks =
        *reinterpret_cast<const std::uint32_t*>(output_generator_base + 836u);
    *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_metadata_latency) =
        *reinterpret_cast<const std::uint64_t*>(seg.frame_ptr + 0u)
        + total_samples * static_cast<std::uint64_t>(latency_blocks);
    *reinterpret_cast<std::uint32_t*>(output_generator_base + kOgOff_metadata_frame_flags) =
        *reinterpret_cast<const std::uint32_t*>(seg.frame_ptr + 16u);

    const std::uint32_t channel_count = cb.get_frame_channel_count(cb.user, seg.frame_ptr);
    for (std::uint32_t slot = 0; slot < channel_count; ++slot) {
        const std::uint64_t frame_ch = cb.get_frame_channel_ptr(cb.user, seg.frame_ptr, slot);
        if (frame_ch == 0)
            continue;

        const auto* fc = reinterpret_cast<const std::uint32_t*>(frame_ch);
        const std::uint32_t mode = fc[kFrameChannelOff_ex_mode / 4u];
        if (mode != 0u) {
            if (mode == 1u) {
                // IDA LABEL_33 @ mode==1: single gain v29[27] / v29[0].
                const std::uint32_t ch = fc[27u];
                if (ch <= 30u) {
                    output_generator_store_channel_gain_1024a9(
                        output_generator_base, ch, static_cast<std::int32_t>(fc[0u]));
                }
            } else {
                // IDA 0x52B888: paired gains for mode 2/3, then optional LABEL_33 tail if mode&1.
                std::uint32_t v31 = 0u;
                for (;;) {
                    const std::uint32_t ch0 = fc[v31 + 27u];
                    if (ch0 <= 30u) {
                        output_generator_store_channel_gain_1024a9(
                            output_generator_base, ch0, static_cast<std::int32_t>(fc[v31]));
                        const std::uint32_t ch1 = fc[v31 + 28u];
                        if (ch1 <= 30u) {
                            output_generator_store_channel_gain_1024a9(
                                output_generator_base, ch1, static_cast<std::int32_t>(fc[v31 + 1u]));
                        }
                    } else {
                        const std::uint32_t ch1 = fc[v31 + 28u];
                        if (ch1 <= 30u) {
                            output_generator_store_channel_gain_1024a9(
                                output_generator_base, ch1, static_cast<std::int32_t>(fc[v31 + 1u]));
                        }
                    }
                    v31 += 2u;
                    if ((mode & 0xFFFFFFFEu) == v31)
                        break;
                }
                if ((mode & 1u) != 0u) {
                    const std::uint32_t ch = fc[v31 + 27u];
                    if (ch <= 30u) {
                        output_generator_store_channel_gain_1024a9(
                            output_generator_base, ch, static_cast<std::int32_t>(fc[v31]));
                    }
                }
            }
        }

        // IDA LABEL_36: v29[30], [32], ... [46] -> og+287..304.
        for (std::uint32_t fi = 30u; fi <= 46u; fi += 2u) {
            if (fc[fi] == 0u)
                continue;
            const std::uintptr_t dst = kOgOff_metadata_gain_table
                + static_cast<std::uintptr_t>((fi - 30u) / 2u) * 8u;
            *reinterpret_cast<std::uint32_t*>(output_generator_base + dst) = 1u;
            *reinterpret_cast<std::uint32_t*>(output_generator_base + dst + 4u) = fc[fi + 1u];
        }
    }
}

void output_generator_dispatch_metadata_update_1024a9(
    std::uint8_t* output_generator_base,
    const OutputGeneratorRuntimeFns1024a9& fns) {
    if (!output_generator_base)
        return;
    const std::uint64_t metadata_table =
        reinterpret_cast<std::uint64_t>(output_generator_base + kOgOff_metadata_table_base);
    if (fns.metadata_update_callback) {
        fns.metadata_update_callback(fns.metadata_update_ctx, metadata_table);
        return;
    }
    const std::uint64_t cb_ptr =
        *reinterpret_cast<const std::uint64_t*>(output_generator_base + kOgOff_metadata_cb);
    if (cb_ptr == 0)
        return;
    const auto cb = reinterpret_cast<MetadataUpdateFn>(cb_ptr);
    const std::uint64_t ctx =
        *reinterpret_cast<const std::uint64_t*>(output_generator_base + kOgOff_metadata_ctx);
    cb(ctx, metadata_table);
}

std::int64_t process_segment_started_path_1024a9(
    std::uint8_t* output_generator_base,
    std::uint64_t output_channels_table_base,
    const OutputGeneratorSegment& seg,
    const OutputGeneratorApplyCallbacks& acb,
    std::uint32_t* produced_mask_out) {
    if (produced_mask_out)
        *produced_mask_out = 0u;
    if (seg.frame_ptr == 0)
        return 0;
    const std::uint32_t frame_channel_count =
        *reinterpret_cast<const std::uint32_t*>(seg.frame_ptr + kFrameOff_channel_count_dword);
    for (std::uint32_t i = 0; i < frame_channel_count; ++i) {
        const std::uintptr_t slot =
            seg.frame_ptr + kFrameOff_channel_slot_base + static_cast<std::uintptr_t>(i) * kFrameStride_channel_slot;
        const std::uint32_t ch = *reinterpret_cast<const std::uint32_t*>(slot + kFrameChSlotOff_channel_index);
        const bool slot_active =
            (*reinterpret_cast<const std::uint32_t*>(slot + kFrameChSlotOff_active_flag) != 0);
        const std::uint64_t frame_ch =
            *reinterpret_cast<const std::uint64_t*>(slot + kFrameChSlotOff_channel_ptr_qword);

        const std::uint64_t src = acb.get_delay_line_channel(acb.user, ch, seg.start);

        if (slot_active && frame_ch != 0u && acb.decode_channel_segment) {
            OutputGeneratorExtrapolateSources ex_src{};
            const std::uint64_t scratch_base = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_scratch_base);
            const std::uint64_t total_samples = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_total_samples);
            ex_src = output_generator_select_extrapolate_sources_1024a9(
                output_channels_table_base,
                frame_ch,
                segment_output_start_1024a9(seg, acb),
                scratch_base,
                total_samples);
            ex_src.frame_slot_index = i;
            // Для decode-пути out_base/dst не нужен: IDA использует channel/errors/src0..2.
            const std::uint64_t dst = 0;
            const std::int64_t rc_dec =
                acb.decode_channel_segment(acb.user, &seg, ch, src, dst, seg.len, &ex_src);
            if (rc_dec != 0)
                return rc_dec;
            if (produced_mask_out)
                produced_mask_or_started_decode_channel_1024a9(*produced_mask_out, frame_ch);
        } else {
            (void)acb.get_delay_line_channel(acb.user, ch, seg.start);
            if (seg.len == 0)
                continue;
            copy_segment_channel_samples_1024a9(seg, acb, ch, src);
            if (produced_mask_out)
                produced_mask_or_logical_channel_1024a9(*produced_mask_out, ch);
        }
    }
    return 0;
}

std::int64_t process_segment_prestart_frame_path_1024a9(
    std::uint8_t* output_generator_base,
    const OutputGeneratorSegment& seg,
    const OutputGeneratorApplyCallbacks& acb,
    std::uint32_t* produced_mask_out) {
    if (produced_mask_out)
        *produced_mask_out = 0u;
    if (seg.frame_ptr == 0)
        return process_segment_copy_only_path_1024a9(seg, acb, produced_mask_out);
    if (seg.len == 0)
        return warmup_zero_len_channels_1024a9(seg, acb);

    // IDA v143: frame есть, started=0 — decode в scratch, затем copy delay-line -> output.
    const std::uint32_t frame_channel_count =
        *reinterpret_cast<const std::uint32_t*>(seg.frame_ptr + kFrameOff_channel_count_dword);
    const std::uint64_t scratch_base =
        *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_scratch_base);
    const std::uint64_t total_samples =
        *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_total_samples);

    for (std::uint32_t i = 0; i < frame_channel_count; ++i) {
        const std::uintptr_t slot =
            seg.frame_ptr + kFrameOff_channel_slot_base + static_cast<std::uintptr_t>(i) * kFrameStride_channel_slot;
        const std::uint32_t ch = *reinterpret_cast<const std::uint32_t*>(slot + kFrameChSlotOff_channel_index);
        const bool slot_active =
            (*reinterpret_cast<const std::uint32_t*>(slot + kFrameChSlotOff_active_flag) != 0);
        const std::uint64_t frame_ch =
            *reinterpret_cast<const std::uint64_t*>(slot + kFrameChSlotOff_channel_ptr_qword);
        const std::uint64_t src = acb.get_delay_line_channel(acb.user, ch, seg.start);

        if (slot_active && frame_ch != 0u && acb.decode_channel_segment) {
            OutputGeneratorExtrapolateSources ex_src{};
            ex_src.src0 = scratch_base + 0ull * 4ull * total_samples;
            ex_src.src1 = scratch_base + 1ull * 4ull * total_samples;
            ex_src.src2 = scratch_base + 2ull * 4ull * total_samples;
            ex_src.frame_slot_index = i;
            const std::int64_t rc_dec =
                acb.decode_channel_segment(acb.user, &seg, ch, src, 0, seg.len, &ex_src);
            if (rc_dec != 0)
                return rc_dec;
        }
        if (seg.len != 0)
            copy_segment_channel_samples_1024a9(seg, acb, ch, src);
        if (produced_mask_out)
            produced_mask_or_logical_channel_1024a9(*produced_mask_out, ch);
    }
    // IDA LABEL_103 -> v143 -> LABEL_104: copy_input идёт после decode+copy delay.
    if (acb.copy_input_enabled) {
        const std::int64_t rc = process_segment_copy_input_path_1024a9(seg, acb, produced_mask_out);
        if (rc != 0)
            return rc;
    }
    return 0;
}

std::int64_t process_segment_copy_only_path_1024a9(
    const OutputGeneratorSegment& seg,
    const OutputGeneratorApplyCallbacks& acb,
    std::uint32_t* produced_mask_out) {
    if (produced_mask_out)
        *produced_mask_out = 0u;
    if (acb.copy_input_enabled)
        return process_segment_copy_input_path_1024a9(seg, acb, produced_mask_out);
    if (seg.len == 0) {
        return warmup_zero_len_channels_1024a9(seg, acb);
    }
    for (int ch = next_set_channel_bit_1024a9(seg.channel_mask, 0);
         ch >= 0;
         ch = next_set_channel_bit_1024a9(seg.channel_mask, ch + 1)) {
        const auto channel = static_cast<std::uint32_t>(ch);
        const std::uint64_t src = acb.get_delay_line_channel(acb.user, channel, seg.start);
        copy_segment_channel_samples_1024a9(seg, acb, channel, src);
        if (produced_mask_out)
            produced_mask_or_logical_channel_1024a9(*produced_mask_out, channel);
    }
    return 0;
}

std::int64_t raw_decode_channel_segment(
    void* user,
    const OutputGeneratorSegment* seg,
    std::uint32_t channel,
    std::uint64_t src_ptr,
    std::uint64_t dst_ptr,
    std::uint64_t sample_count,
    const OutputGeneratorExtrapolateSources* extrap_sources) {
    auto* c = reinterpret_cast<OgRawRuntimeCtx*>(user);
    if (c->fns.decode_channel_segment) {
        return c->fns.decode_channel_segment(c->og, seg, channel, src_ptr, dst_ptr, sample_count, extrap_sources);
    }

    // Более строгий fallback к IDA-порядку: GolombRice_get_errors -> Extrapolate_process.
    std::uint32_t slot_idx = 0;
    std::uint64_t frame_ch = 0;
    bool slot_active = false;
    if (extrap_sources && extrap_sources->frame_slot_index != 0xFFFFFFFFu && seg->frame_ptr != 0) {
        slot_idx = extrap_sources->frame_slot_index;
        const std::uint32_t n = *reinterpret_cast<const std::uint32_t*>(seg->frame_ptr + kFrameOff_channel_count_dword);
        const std::uintptr_t slot =
            seg->frame_ptr + kFrameOff_channel_slot_base + static_cast<std::uintptr_t>(slot_idx) * kFrameStride_channel_slot;
        const std::uint32_t out_ch = *reinterpret_cast<const std::uint32_t*>(slot + kFrameChSlotOff_channel_index);
        slot_active = (*reinterpret_cast<const std::uint32_t*>(slot + kFrameChSlotOff_active_flag) != 0);
        frame_ch = *reinterpret_cast<const std::uint64_t*>(slot + kFrameChSlotOff_channel_ptr_qword);
    } else if (!raw_find_frame_channel_slot(seg, channel, &slot_idx, &frame_ch, &slot_active)) {
        return 0;
    }
    if (frame_ch == 0 || !slot_active)
        return 0;

    const std::uint64_t gr_state =
        reinterpret_cast<std::uint64_t>(c->og + kOgOff_gr_state_base + static_cast<std::uintptr_t>(slot_idx) * kOgStride_gr_state);
    const std::uint64_t ex_state =
        reinterpret_cast<std::uint64_t>(c->og + kOgOff_ex_state_base + static_cast<std::uintptr_t>(slot_idx) * kOgStride_ex_state);
    const std::uint64_t errors_buf = *reinterpret_cast<std::uint64_t*>(c->og + kOgOff_errors_buf);
    if (seg != nullptr && seg->frame_ptr != 0u) {
        static std::unordered_map<std::uint64_t, std::uint32_t> last_frame_start_by_gr_state;
        const std::uint32_t frame_start =
            *reinterpret_cast<const std::uint32_t*>(seg->frame_ptr + 0u);
        auto it = last_frame_start_by_gr_state.find(gr_state);
        if (it == last_frame_start_by_gr_state.end() || it->second != frame_start) {
            last_frame_start_by_gr_state[gr_state] = frame_start;
            auto* words_ptr = reinterpret_cast<std::uint32_t*>(frame_ch + kFrameChannelOff_stream_words);
            const std::uint64_t ctx_ptr = frame_ch + kFrameChannelOff_ctx_words;
            (void)raw_gr_initialize(user, gr_state, words_ptr, ctx_ptr);
            (void)raw_ex_initialize(user, ex_state, frame_ch);
        }
    }

    // IDA 0x1025F0 / 0x10299A: return value is data-flow, not an error code for OutputGenerator_process.
    if (c->fns.golombrice_get_errors) {
        (void)c->fns.golombrice_get_errors(gr_state, frame_ch, errors_buf, sample_count);
    } else {
        (void)golombrice_get_errors_104e40(gr_state, frame_ch, errors_buf, sample_count);
    }

    std::uint64_t s0 = 0, s1 = 0, s2 = 0;
    if (extrap_sources) {
        s0 = extrap_sources->src0;
        s1 = extrap_sources->src1;
        s2 = extrap_sources->src2;
    }
    // IDA 0x102622 / 0x1029D6: native caller ignores the return value.
    if (c->fns.extrapolate_process) {
        (void)c->fns.extrapolate_process(ex_state, src_ptr, errors_buf, s0, s1, s2, sample_count);
    } else {
        (void)extrapolate_process_104460_partial(ex_state, src_ptr, errors_buf, s0, s1, s2, sample_count);
    }
    return 0;
}

