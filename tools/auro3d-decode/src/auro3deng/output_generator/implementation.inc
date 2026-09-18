int next_set_channel_bit(std::uint32_t mask, int from_bit_inclusive) {
    for (int bit = from_bit_inclusive; bit < static_cast<int>(kCodecV3ChannelCount); ++bit) {
        if (((mask >> bit) & 1u) != 0)
            return bit;
    }
    return -1;
}

namespace {

void produced_mask_or_logical_channel(std::uint32_t& mask, std::uint32_t channel) {
    if (channel <= 30u)
        mask |= 1u << channel;
}

void produced_mask_or_started_decode_channel(
    std::uint32_t& mask,
    std::uint64_t frame_channel_ptr) {
    if (frame_channel_ptr == 0u)
        return;
    produced_mask_or_logical_channel(
        mask,
        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + kFrameChannelOff_pred_src0_idx));
    produced_mask_or_logical_channel(
        mask,
        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + kFrameChannelOff_pred_src1_idx));
    produced_mask_or_logical_channel(
        mask,
        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + kFrameChannelOff_pred_src2_idx));
}

} // namespace

std::uint32_t select_segment_output_mask(const OutputGeneratorSegment& seg) {
    // 153: при v131 используется v107=v152 (frame mask), иначе маска delay-line buffer.
    if (seg.frame_ptr != 0 && use_started_decode_path(seg))
        return seg.frame_flags;
    return seg.channel_mask;
}

std::uint32_t frame_output_mask(const OutputGeneratorSegment& seg) {
    if (seg.frame_ptr == 0)
        return 0u;
    return *reinterpret_cast<const std::uint32_t*>(seg.frame_ptr + 24u);
}

std::int64_t process_segment_copy_only_path(
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

bool should_use_scalar_copy(
    std::uint64_t src_ptr,
    std::uint64_t dst_ptr,
    std::uint64_t sample_count,
    std::uint64_t seg_end) {
    // OutputGenerator_process: scalar when len<0x10, len==0, high dword set, or ptr gap<0x20.
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

void copy_i32_samples_fast(std::uint64_t dst_ptr, std::uint64_t src_ptr, std::uint64_t sample_count) {
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

std::uint64_t segment_output_start(
    const OutputGeneratorSegment& seg,
    const OutputGeneratorApplyCallbacks& acb) {
    return (seg.start >= acb.output_block_start) ? (seg.start - acb.output_block_start) : seg.start;
}

void copy_segment_channel_samples(
    const OutputGeneratorSegment& seg,
    const OutputGeneratorApplyCallbacks& acb,
    std::uint32_t channel,
    std::uint64_t src_ptr) {
    if (seg.len == 0 || src_ptr == 0)
        return;
    const std::uint64_t out_base = acb.get_output_channel_base(acb.user, channel);
    if (out_base == 0)
        return;
    const std::uint64_t out_start = segment_output_start(seg, acb);
    const std::uint64_t dst = out_base + 4ull * out_start;
    const std::uint64_t seg_end = out_start + seg.len;
    if (should_use_scalar_copy(src_ptr, dst, seg.len, seg_end)) {
        copy_i32_samples(dst, src_ptr, seg.len);
    } else {
        copy_i32_samples_fast(dst, src_ptr, seg.len);
    }
}

void zero_i32_samples(std::uint64_t dst_ptr, std::uint64_t sample_count) {
    if (!dst_ptr || sample_count == 0)
        return;
    std::memset(
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(dst_ptr)),
        0,
        static_cast<std::size_t>(sample_count) * sizeof(std::int32_t));
}

// (host): zero output channels in segment mask but outside produced_mask.
// Native started path (v131) skips this — uses frame+24 mask at only.
void zero_unproduced_output_channels(
    const OutputGeneratorSegment& seg,
    const OutputGeneratorApplyCallbacks& acb,
    std::uint32_t produced_mask,
    std::uint32_t channel_universe_mask) {
    // runs for started and non-started (after).
    // For j in 0..30: if bit j not in produced (v40), zero output channel j when ptr != 0.
    if (seg.len == 0u)
        return;
    const std::uint32_t universe = channel_universe_mask & kCodecV3ChannelMask;
    const std::uint32_t missing = universe & ~produced_mask;
    for (int ch = next_set_channel_bit(missing, 0);
         ch >= 0;
         ch = next_set_channel_bit(missing, ch + 1)) {
        const std::uint64_t out_base = acb.get_output_channel_base(acb.user, static_cast<std::uint32_t>(ch));
        if (out_base == 0u)
            continue;
        zero_i32_samples(out_base + 4ull * segment_output_start(seg, acb), seg.len);
    }
}

// auro_codec_v3_decoder_OutputGenerator_cross_fade_ _codec_OutputGenerator_cross_fade_inner.
std::int64_t output_generator_cross_fade(
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
            const std::uint64_t old_base = delay_line_get_channel_from_buffer(delay_line_buffer, ch, 0);
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

std::int64_t warmup_zero_len_channels(
    const OutputGeneratorSegment& seg,
    const OutputGeneratorApplyCallbacks& acb) {
    // 181: при v31==0 вызываются get_channel только для активных битов mask.
    // Порядок вызовов по коду — обход только установленных битов, от меньшего к большему.
    for (int ch = next_set_channel_bit(seg.channel_mask, 0);
         ch >= 0;
         ch = next_set_channel_bit(seg.channel_mask, ch + 1)) {
        (void)acb.get_delay_line_channel(acb.user, static_cast<std::uint32_t>(ch), seg.start);
    }
    return 0;
}

std::int64_t process_segment_copy_input_path(
    const OutputGeneratorSegment& seg,
    const OutputGeneratorApplyCallbacks& acb,
    std::uint32_t* produced_mask_out) {
    // v128 = v139 & 0x7FFFFFFF.
    const std::uint32_t mask = acb.input_mask & 0x7FFFFFFFu & kCodecV3ChannelMask;
    if (seg.len == 0u) {
        if (produced_mask_out)
            *produced_mask_out |= mask;
        return 0;
    }
    for (int ch = next_set_channel_bit(mask, 0);
         ch >= 0;
         ch = next_set_channel_bit(mask, ch + 1)) {
        const auto channel = static_cast<std::uint32_t>(ch);
        const std::uint64_t src_base =
            acb.get_input_channel_base ? acb.get_input_channel_base(acb.user, channel) : 0u;
        const std::uint64_t out_base = acb.get_output_channel_base(acb.user, channel);
        if (src_base == 0u || out_base == 0u)
            continue;
        const std::uint64_t dst = out_base + 4ull * segment_output_start(seg, acb);
        const std::uint64_t src = src_base + 4ull * seg.start;
        const std::uint64_t seg_end = seg.start + seg.len;
        if (should_use_scalar_copy(src, dst, seg.len, seg_end)) {
            copy_i32_samples(dst, src, seg.len);
        } else {
            copy_i32_samples_fast(dst, src, seg.len);
        }
        if (produced_mask_out)
            produced_mask_or_logical_channel(*produced_mask_out, channel);
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

std::int64_t golombrice_initialize(
    std::uint64_t gr_state_ptr,
    std::uint32_t* words_ptr,
    std::uint64_t ctx_ptr) {
    // _codec_channel_GolombRice_initialize: zero flags -> ctx -> bit_count=31 -> first word.
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

std::int64_t extrapolate_initialize(std::uint64_t ex_state_ptr, std::uint64_t frame_channel_ptr) {
    // auro_codec_v3:_codec_channel_Extrapolate_initialize.
    if (!ex_state_ptr || !frame_channel_ptr)
        return 0;
    *reinterpret_cast<std::uint64_t*>(ex_state_ptr + kExStateOff_head_qword) = 0;
    *reinterpret_cast<std::uint32_t*>(ex_state_ptr + kExStateOff_phase_dword) = 0;
    *reinterpret_cast<std::uint64_t*>(ex_state_ptr + kExStateOff_frame_channel_ptr_qword) = frame_channel_ptr;
    return 0;
}

const std::array<float, kExtrapolateScaleTableSize>& extrapolate_scale_table() {
    static const std::array<float, kExtrapolateScaleTableSize> table = [] {
        std::array<float, kExtrapolateScaleTableSize> out{};
        for (std::size_t i = 0; i < out.size(); ++i)
            out[i] = std::exp(static_cast<float>(i) * kExtrapolateScaleExpStep);

        // overwrites these exact slots after the expf loop.
        out[60] = 2.0f;
        out[120] = 4.0f;
        out[180] = 8.0f;
        out[240] = 16.0f;
        return out;
    }();
    return table;
}

std::int32_t clamp_pcm24_impl(std::int32_t value) {
    // mix1/mix2 scalar: clamp to [ -8388608, ] via >= < -8388607.
    if (value >= 0x7FFFFF)
        return kPcm24Max;
    if (value < -8388607)
        return kPcm24Min;
    return value;
}

std::int32_t scale_shift_clamp_pcm24_impl(std::int32_t value, float scale, std::uint32_t shift) {
    // / uses CVTTSS2SI followed by a 32-bit SHL. The
    // shift therefore wraps in 32 bits before the signed PCM24 clamp; widening
    // it to int64 turns wrapped reconstruction values into long full-scale
    // plateaus.
    const std::int32_t scaled =
        static_cast<std::int32_t>(static_cast<float>(value) * scale);
    const std::int32_t shifted = shift < 32u
        ? static_cast<std::int32_t>(static_cast<std::uint32_t>(scaled) << shift)
        : 0;
    if (shifted >= kPcm24Max)
        return kPcm24Max;
    if (shifted < kPcm24Min)
        return kPcm24Min;
    return static_cast<std::int32_t>(shifted);
}

std::int32_t div4_trunc0(std::int32_t value) {
    const std::int32_t bias = (value >> 31) & 3;
    return (value + bias) >> 2;
}

// extrapolate_mix3: v47 = v39 + 3*v46 + 3; if (3*v46+v39 >= 0) v47 = 3*v46+v39; v49 = v47 >> 2.
std::int32_t div4_mix3(std::int32_t v39, std::int32_t v46) {
    const std::int32_t v48 = 3 * v46 + v39;
    std::int32_t v47 = v39 + 3 * v46 + 3;
    if (v48 >= 0)
        v47 = v48;
    return v47 >> 2;
}

// auro_codec_v3:_codec_channel_Extrapolate_process.
std::int64_t
extrapolate_process(
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

    const auto& table = extrapolate_scale_table();
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
            dst0[i] = clamp_pcm24_impl(scaled);
        }

        (void)errors_buf_ptr; // mode==1 path in ignores error buffer.
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
            // mix2: when (count&1)==0 → predicted=a5/dst0, residual=a6/dst1.
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
            dst0[i] = scale_shift_clamp_pcm24_impl(dst0[i], scale0, shift);
            dst1[i] = scale_shift_clamp_pcm24_impl(dst1[i], scale1, shift);
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
            dst0[produced] = div4_mix3(v39, v26);
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

        // mix3 pointer rotate init from count%3:
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
                const std::int32_t v49 = div4_mix3(v39, v46);
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
            // residual-add state machine from initial_count%3:
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
            dst0[i] = scale_shift_clamp_pcm24_impl(dst0[i], scale0, shift);
            dst1[i] = scale_shift_clamp_pcm24_impl(dst1[i], scale1, shift);
            dst2[i] = scale_shift_clamp_pcm24_impl(dst2[i], scale2, shift);
        }
        return 0;
    }

    return 0;
}

std::uint32_t gr_read_stream_bit(std::uint64_t gr_state_ptr) {
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

void gr_flush_label71(
    std::uint64_t gr_state_ptr,
    std::uint32_t value_mask_cursor,
    std::uint32_t& accum_bits) {
    // v58 = 2 * v22; do { read bit; v7 = bit + 2*v7; v58 *= 2; } while (v58);
    std::uint32_t flush_scale = value_mask_cursor << 1u;
    while (flush_scale != 0u) {
        accum_bits = gr_read_stream_bit(gr_state_ptr) + 2u * accum_bits;
        flush_scale *= 2u;
    }
}

std::uint32_t gr_append_bits(std::uint64_t gr_state_ptr, std::uint32_t accum_bits, std::uint32_t bit_count) {
    for (std::uint32_t i = 0; i < bit_count; ++i)
        accum_bits = gr_read_stream_bit(gr_state_ptr) + 2u * accum_bits;
    return accum_bits;
}

std::uint32_t gr_pow2_table(std::uint32_t bit_index) {
    return (bit_index < 31u) ? (1u << bit_index) : 0x80000000u;
}

std::uint32_t gr_extract_packed_unsigned(
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

std::int32_t gr_sign_extend(std::uint32_t raw_value, std::uint32_t bit_width) {
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

// auro_codec_v3_decoder_channel_GolombRice_get_errors _codec_channel_GolombRice_get_errors.
std::int64_t golombrice_get_errors(
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
        std::uint32_t unary_scale = gr_pow2_table(
            *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_k_dword));
        std::uint32_t unary_mask = 0x80000000u;
        std::uint32_t counter = *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_counter_dword);

        // ((unsigned __int8)(flags>>30) & (counter==0)) == 1 → только bit30, без bit31.
        if ((((flags >> 30u) & 1u) != 0u) && counter == 0u) {
            const std::uint32_t dynamic_k = accum_bits >> 29u;
            *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_k_dword) = dynamic_k;
            unary_scale = gr_pow2_table(dynamic_k);
            unary_mask = 0x10000000u;
        }

        counter = (counter + 1u > 31u) ? 0u : (counter + 1u);
        *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_counter_dword) = counter;

        std::uint32_t unary_count = 0;
        if ((accum_bits & unary_mask) != 0) {
            unary_mask >>= 1u;
            while (true) {
                if (unary_mask == 1u) {
                    accum_bits = gr_append_bits(gr_state_ptr, accum_bits, 31u);
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
            accum_bits = gr_append_bits(gr_state_ptr, accum_bits, 31u);
            value_mask_cursor = 0x80000000u;
        }

        std::uint32_t extra_value = 0;
        const std::uint32_t k_value = *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_k_dword);
        for (std::uint32_t bit = 0; bit < k_value; ++bit) {
            if ((accum_bits & value_mask_cursor) != 0) {
                extra_value += (bit < 16u) ? kDword289CE0[bit] : gr_pow2_table(bit);
            }
            value_mask_cursor >>= 1u;
            if (value_mask_cursor == 1u) {
                accum_bits = gr_append_bits(gr_state_ptr, accum_bits, 31u);
                value_mask_cursor = 0x80000000u;
            }
        }

        const std::uint32_t code_index = extra_value + unary_scale * unary_count;
        const std::uint32_t packed_index0 = bit_width * code_index;
        const std::uint32_t raw0 =
            gr_extract_packed_unsigned(packed_words, packed_index0, bit_width, value_mask);
        *out = gr_sign_extend(raw0, bit_width);

        if (mode == 3u) {
            const std::uint32_t packed_index1 = bit_width * (base_index + code_index);
            const std::uint32_t raw1 =
                gr_extract_packed_unsigned(packed_words, packed_index1, bit_width, value_mask);
            out[1] = gr_sign_extend(raw1, bit_width);
            out += 2;
        } else {
            ++out;
        }


        gr_flush_label71(gr_state_ptr, value_mask_cursor, accum_bits);
    }

    *reinterpret_cast<std::uint32_t*>(gr_state_ptr + kGrStateOff_accum_bits_dword) = accum_bits;
    return 0;
}

std::int64_t raw_gr_initialize(void* user, std::uint64_t gr_state_ptr, std::uint32_t* words_ptr, std::uint64_t ctx_ptr) {
    auto* c = reinterpret_cast<OgRawRuntimeCtx*>(user);
    if (c->fns.golombrice_initialize)
        return c->fns.golombrice_initialize(gr_state_ptr, words_ptr, ctx_ptr);
    return golombrice_initialize(gr_state_ptr, words_ptr, ctx_ptr);
}

std::int64_t raw_ex_initialize(void* user, std::uint64_t ex_state_ptr, std::uint64_t frame_channel_ptr) {
    auto* c = reinterpret_cast<OgRawRuntimeCtx*>(user);
    if (c->fns.extrapolate_initialize)
        return c->fns.extrapolate_initialize(ex_state_ptr, frame_channel_ptr);
    return extrapolate_initialize(ex_state_ptr, frame_channel_ptr);
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
    if (!state || state->samples_per_block == 0 || adjusted_start >= state->samples_per_block)
        return 0;
    return c->fns.delay_line_get_channel(c->delay_line_buffer, channel, adjusted_start);
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

bool should_apply_frame_metadata(std::uint64_t frame_ptr, std::uint64_t timeline_cursor) {
    if (frame_ptr == 0)
        return false;
    const std::uint32_t frame_start = *reinterpret_cast<const std::uint32_t*>(frame_ptr + 0);
    // Metadata table update is keyed by frame_start == timeline (channel_count may be 0).
    return frame_start == static_cast<std::uint32_t>(timeline_cursor);
}

bool should_prepare_frame_init(std::uint64_t frame_ptr, std::uint64_t timeline_cursor) {
    if (!should_apply_frame_metadata(frame_ptr, timeline_cursor))
        return false;
    const std::uint32_t channel_count =
        *reinterpret_cast<const std::uint32_t*>(frame_ptr + kFrameOff_channel_count_dword);
    // GR/Extrapolate init only when channel_count != 0.
    return channel_count != 0;
}

void output_generator_store_channel_gain(
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

void output_generator_apply_frame_metadata(
    std::uint8_t* output_generator_base,
    const OutputGeneratorSegment& seg,
    std::uint64_t timeline_cursor,
    const OutputGeneratorFrameInitCallbacks& cb) {
    if (!output_generator_base || seg.frame_ptr == 0)
        return;
    if (!should_apply_frame_metadata(seg.frame_ptr, timeline_cursor))
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
                // mode==1: single gain v29[27] v29[0].
                const std::uint32_t ch = fc[27u];
                if (ch <= 30u) {
                    output_generator_store_channel_gain(
                        output_generator_base, ch, static_cast<std::int32_t>(fc[0u]));
                }
            } else {
                // paired gains for mode 2/3, then optional tail if mode&1.
                std::uint32_t v31 = 0u;
                for (;;) {
                    const std::uint32_t ch0 = fc[v31 + 27u];
                    if (ch0 <= 30u) {
                        output_generator_store_channel_gain(
                            output_generator_base, ch0, static_cast<std::int32_t>(fc[v31]));
                        const std::uint32_t ch1 = fc[v31 + 28u];
                        if (ch1 <= 30u) {
                            output_generator_store_channel_gain(
                                output_generator_base, ch1, static_cast<std::int32_t>(fc[v31 + 1u]));
                        }
                    } else {
                        const std::uint32_t ch1 = fc[v31 + 28u];
                        if (ch1 <= 30u) {
                            output_generator_store_channel_gain(
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
                        output_generator_store_channel_gain(
                            output_generator_base, ch, static_cast<std::int32_t>(fc[v31]));
                    }
                }
            }
        }

        // v29[30], [32], [46] -> og+287..304.
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

void output_generator_dispatch_metadata_update(
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

std::int64_t process_segment_started_path(
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
            ex_src = output_generator_select_extrapolate_sources(
                output_channels_table_base,
                frame_ch,
                segment_output_start(seg, acb),
                scratch_base,
                total_samples);
            ex_src.frame_slot_index = i;
            // Для decode-пути out_base/dst не нужен: использует channel/errors/src0..2.
            const std::uint64_t dst = 0;
            const std::int64_t rc_dec =
                acb.decode_channel_segment(acb.user, &seg, ch, src, dst, seg.len, &ex_src);
            if (rc_dec != 0)
                return rc_dec;
            if (produced_mask_out)
                produced_mask_or_started_decode_channel(*produced_mask_out, frame_ch);
        } else {
            (void)acb.get_delay_line_channel(acb.user, ch, seg.start);
            if (seg.len == 0)
                continue;
            copy_segment_channel_samples(seg, acb, ch, src);
            if (produced_mask_out)
                produced_mask_or_logical_channel(*produced_mask_out, ch);
        }
    }
    return 0;
}

std::int64_t process_segment_prestart_frame_path(
    std::uint8_t* output_generator_base,
    const OutputGeneratorSegment& seg,
    const OutputGeneratorApplyCallbacks& acb,
    std::uint32_t* produced_mask_out) {
    if (produced_mask_out)
        *produced_mask_out = 0u;
    if (seg.frame_ptr == 0)
        return process_segment_copy_only_path(seg, acb, produced_mask_out);
    if (seg.len == 0)
        return warmup_zero_len_channels(seg, acb);

    // v143: frame есть, started=0 — decode в scratch, затем copy delay-line -> output.
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
            copy_segment_channel_samples(seg, acb, ch, src);
        if (produced_mask_out)
            produced_mask_or_logical_channel(*produced_mask_out, ch);
    }
    // v143 ->: copy_input идёт после decode+copy delay.
    if (acb.copy_input_enabled) {
        const std::int64_t rc = process_segment_copy_input_path(seg, acb, produced_mask_out);
        if (rc != 0)
            return rc;
    }
    return 0;
}

std::int64_t process_segment_copy_only_path(
    const OutputGeneratorSegment& seg,
    const OutputGeneratorApplyCallbacks& acb,
    std::uint32_t* produced_mask_out) {
    if (produced_mask_out)
        *produced_mask_out = 0u;
    if (acb.copy_input_enabled)
        return process_segment_copy_input_path(seg, acb, produced_mask_out);
    if (seg.len == 0) {
        return warmup_zero_len_channels(seg, acb);
    }
    for (int ch = next_set_channel_bit(seg.channel_mask, 0);
         ch >= 0;
         ch = next_set_channel_bit(seg.channel_mask, ch + 1)) {
        const auto channel = static_cast<std::uint32_t>(ch);
        const std::uint64_t src = acb.get_delay_line_channel(acb.user, channel, seg.start);
        copy_segment_channel_samples(seg, acb, channel, src);
        if (produced_mask_out)
            produced_mask_or_logical_channel(*produced_mask_out, channel);
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

    // Более строгий fallback к -порядку: GolombRice_get_errors -> Extrapolate_process.
    std::uint32_t slot_idx = 0;
    std::uint64_t frame_ch = 0;
    bool slot_active = false;
    if (extrap_sources && extrap_sources->frame_slot_index != 0xFFFFFFFFu && seg->frame_ptr != 0) {
        slot_idx = extrap_sources->frame_slot_index;
        const std::uintptr_t slot =
            seg->frame_ptr + kFrameOff_channel_slot_base + static_cast<std::uintptr_t>(slot_idx) * kFrameStride_channel_slot;
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
    // /: return value is data-flow, not an error code for OutputGenerator_process.
    if (c->fns.golombrice_get_errors) {
        (void)c->fns.golombrice_get_errors(gr_state, frame_ch, errors_buf, sample_count);
    } else {
        (void)golombrice_get_errors(gr_state, frame_ch, errors_buf, sample_count);
    }
    std::uint64_t s0 = 0, s1 = 0, s2 = 0;
    if (extrap_sources) {
        s0 = extrap_sources->src0;
        s1 = extrap_sources->src1;
        s2 = extrap_sources->src2;
    }
    // /: native caller ignores the return value.
    if (c->fns.extrapolate_process) {
        (void)c->fns.extrapolate_process(ex_state, src_ptr, errors_buf, s0, s1, s2, sample_count);
    } else {
        (void)
extrapolate_process(ex_state, src_ptr, errors_buf, s0, s1, s2, sample_count);
    }
    return 0;
}

bool codec_v3_split_equivalence_self_test(std::string& detail) {
    constexpr std::size_t kSamples = 1000u;
    constexpr std::size_t kGrStateBytes = kOgStride_gr_state;
    constexpr std::size_t kExStateBytes = kOgStride_ex_state;

    const std::array<std::vector<std::size_t>, 6> split_cases{{
        {1u, kSamples - 1u},
        {2u, 3u, kSamples - 5u},
        {640u, 360u},
        {472u, 528u},
        {776u, 224u},
        {168u, 304u, 360u, 168u},
    }};

    for (const std::uint32_t mode : {2u, 3u}) {
        std::array<std::uint32_t, 48> frame_channel{};
        frame_channel[kFrameChannelOff_ex_mode / 4u] = mode;
        frame_channel[kFrameChannelOff_ex_quant_shift / 4u] = 17u;
        frame_channel[kFrameChannelOff_ex_scale_idx0 / 4u + 0u] = 0u;
        frame_channel[kFrameChannelOff_ex_scale_idx0 / 4u + 1u] = 0u;
        frame_channel[kFrameChannelOff_ex_scale_idx0 / 4u + 2u] = 0u;
        frame_channel[kFrameChannelOff_ex_scale_idx1 / 4u] = 0u;
        frame_channel[kFrameChannelOff_seed0_primary / 4u] = 630u;
        frame_channel[kFrameChannelOff_seed0_secondary / 4u] = 0u;
        frame_channel[kFrameChannelOff_seed1_primary / 4u] = 2268u;
        frame_channel[kFrameChannelOff_seed1_secondary / 4u] = static_cast<std::uint32_t>(-1);
        frame_channel[kFrameChannelOff_seed2_primary / 4u] = 491u;
        frame_channel[kFrameChannelOff_gr_packed_flags / 4u] = 0u;
        frame_channel[kFrameChannelOff_gr_base_index / 4u] = 1u;
        frame_channel[kFrameChannelOff_gr_bit_width / 4u] = 1u;

        std::vector<std::uint32_t> gr_words(4096u);
        std::vector<std::uint32_t> gr_context(4096u);
        for (std::size_t i = 0; i < gr_words.size(); ++i) {
            gr_words[i] = (i & 1u) != 0u ? 0x55555555u : 0xAAAAAAAAu;
            gr_context[i] = static_cast<std::uint32_t>(0x9E3779B9u * (i + 1u));
        }
        std::vector<std::int32_t> carrier(kSamples);
        for (std::size_t i = 0; i < carrier.size(); ++i) {
            const std::int32_t saw = static_cast<std::int32_t>((i * 7919u + 1237u) % 200001u) - 100000;
            carrier[i] = (saw + static_cast<std::int32_t>((i % 7u) * 13u)) * 64;
        }

        const std::size_t error_count = kSamples * (mode == 3u ? 2u : 1u);
        std::vector<std::int32_t> whole_errors(error_count, 0);
        std::vector<std::int32_t> extrapolate_errors(error_count, 0);
        for (std::size_t i = 0; i < error_count; ++i)
            extrapolate_errors[i] = static_cast<std::int32_t>((i * 37u + 11u) % 187u) - 93;
        std::vector<std::int32_t> whole0(kSamples, 0);
        std::vector<std::int32_t> whole1(kSamples, 0);
        std::vector<std::int32_t> whole2(kSamples, 0);
        alignas(8) std::array<std::uint8_t, kGrStateBytes> whole_gr{};
        alignas(8) std::array<std::uint8_t, kExStateBytes> whole_ex{};
        (void)golombrice_initialize(
            reinterpret_cast<std::uint64_t>(whole_gr.data()),
            gr_words.data(),
            reinterpret_cast<std::uint64_t>(gr_context.data()));
        (void)extrapolate_initialize(
            reinterpret_cast<std::uint64_t>(whole_ex.data()),
            reinterpret_cast<std::uint64_t>(frame_channel.data()));
        (void)golombrice_get_errors(
            reinterpret_cast<std::uint64_t>(whole_gr.data()),
            reinterpret_cast<std::uint64_t>(frame_channel.data()),
            reinterpret_cast<std::uint64_t>(whole_errors.data()),
            kSamples);
        (void)extrapolate_process(
            reinterpret_cast<std::uint64_t>(whole_ex.data()),
            reinterpret_cast<std::uint64_t>(carrier.data()),
            reinterpret_cast<std::uint64_t>(extrapolate_errors.data()),
            reinterpret_cast<std::uint64_t>(whole0.data()),
            reinterpret_cast<std::uint64_t>(whole1.data()),
            reinterpret_cast<std::uint64_t>(whole2.data()),
            kSamples);

        for (std::size_t split_index = 0; split_index < split_cases.size(); ++split_index) {
            std::vector<std::int32_t> split_errors(error_count, 0);
            std::vector<std::int32_t> split0(kSamples, 0);
            std::vector<std::int32_t> split1(kSamples, 0);
            std::vector<std::int32_t> split2(kSamples, 0);
            alignas(8) std::array<std::uint8_t, kGrStateBytes> split_gr{};
            alignas(8) std::array<std::uint8_t, kExStateBytes> split_ex{};
            (void)golombrice_initialize(
                reinterpret_cast<std::uint64_t>(split_gr.data()),
                gr_words.data(),
                reinterpret_cast<std::uint64_t>(gr_context.data()));
            (void)extrapolate_initialize(
                reinterpret_cast<std::uint64_t>(split_ex.data()),
                reinterpret_cast<std::uint64_t>(frame_channel.data()));

            std::size_t offset = 0u;
            for (const std::size_t count : split_cases[split_index]) {
                const std::size_t error_offset = offset * (mode == 3u ? 2u : 1u);
                (void)golombrice_get_errors(
                    reinterpret_cast<std::uint64_t>(split_gr.data()),
                    reinterpret_cast<std::uint64_t>(frame_channel.data()),
                    reinterpret_cast<std::uint64_t>(split_errors.data() + error_offset),
                    count);
                (void)extrapolate_process(
                    reinterpret_cast<std::uint64_t>(split_ex.data()),
                    reinterpret_cast<std::uint64_t>(carrier.data() + offset),
                    reinterpret_cast<std::uint64_t>(extrapolate_errors.data() + error_offset),
                    reinterpret_cast<std::uint64_t>(split0.data() + offset),
                    reinterpret_cast<std::uint64_t>(split1.data() + offset),
                    reinterpret_cast<std::uint64_t>(split2.data() + offset),
                    count);
                offset += count;
            }

            const bool pcm_equal = split_errors == whole_errors
                && split0 == whole0
                && split1 == whole1
                && (mode != 3u || split2 == whole2);
            const bool gr_equal = std::memcmp(split_gr.data(), whole_gr.data(), kGrStateBytes) == 0;
            const bool ex_equal = std::memcmp(split_ex.data(), whole_ex.data(), kExStateBytes) == 0;
            if (!pcm_equal || !gr_equal || !ex_equal || offset != kSamples) {
                detail = "mode=" + std::to_string(mode)
                    + " split_case=" + std::to_string(split_index)
                    + " pcm=" + std::to_string(pcm_equal ? 1 : 0)
                    + " gr_state=" + std::to_string(gr_equal ? 1 : 0)
                    + " ex_state=" + std::to_string(ex_equal ? 1 : 0);
                return false;
            }
        }
    }

    detail = "mode2/mode3 PCM and GR/Extrapolate state are exactly equal";
    return true;
}

