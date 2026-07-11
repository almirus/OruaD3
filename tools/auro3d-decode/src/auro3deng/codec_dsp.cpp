#include "auro3deng_internal_preamble.hpp"

namespace auro3deng {

bool has_required_channel_ptrs(const ProcessorIOBufferDesc* d, std::int32_t mask) {
    for (int bit = 0; bit < 27; ++bit) {
        if (((mask >> bit) & 1) != 0) {
            if (d->channel_ptr[static_cast<std::size_t>(bit)] == 0)
                return false;
        }
    }
    return true;
}

using PreSegmentsFn = std::int64_t (*)(std::uint64_t ctx, std::uint64_t ranges_base, std::uint64_t started_base);
using MetadataUpdateFn = void (*)(std::uint64_t ctx, std::uint64_t metadata_table);

std::uint64_t safe_min_u64(std::uint64_t a, std::uint64_t b) {
    return (a < b) ? a : b;
}

bool use_started_decode_path_1024a9(const OutputGeneratorSegment& seg) {
    // IDA v131 / started[i]: decode-ветка, отдельно от frame_has_started (timeline).
    return seg.prefer_started_decode_path;
}

bool is_frame_finished_at_cursor_1024a9(std::uint64_t frame_ptr, std::uint64_t timeline_cursor) {
    if (frame_ptr == 0)
        return false;
    const std::int32_t frame_end = *reinterpret_cast<const std::int32_t*>(frame_ptr + kFrameOff_end_dword);
    return frame_end == static_cast<std::int32_t>(timeline_cursor);
}

void channel_parse_result_mark_usage_106d07_partial(std::uint64_t parse_result_ptr, std::uint32_t in_use) {
    // IDA ParseResult_mark_usage @ 0x52F2D0: store usage dword at +3664.
    *reinterpret_cast<std::uint32_t*>(parse_result_ptr + kParseResultOff_usage_dword) = in_use;
}

void channel_parse_result_prepare_new_107220_partial(std::uint64_t parse_result_ptr) {
    // current IDA ParseResultPool_get_new pre-increments cursor, constructs the slot, then marks it in use.
    // IDA 0x107220 tail: construct(parse_result); mark_usage(parse_result, 1).
    channel_parse_result_construct_103810_partial(parse_result_ptr);
    channel_parse_result_mark_usage_106d07_partial(parse_result_ptr, 1u);
}

std::uint64_t parse_result_pool_required_additional_memory_107190_partial(std::uint32_t pool_count) {
    // current IDA ParseResultPool_t_required_additional_memory: align=8, size=3672 * count.
    return static_cast<std::uint64_t>(kParseResultStrideBytes) * static_cast<std::uint64_t>(pool_count);
}

std::uint32_t memory_parse_result_pool_count_106d20_partial(
    std::uint32_t block_samples,
    std::uint32_t channel_count) {
    // IDA 0x106D20: ParseResultPool_t_required_additional_memory(..., 9 * ((a2[0]*a2[7]+255)>>8) + 9)
    return 9u * (((block_samples * channel_count + 255u) >> 8) + 1u);
}

std::uint64_t memory_block_info_required_additional_memory_13d750_partial(std::uint32_t n) {
    return 24ull * static_cast<std::uint64_t>(n) + 4ull * static_cast<std::uint64_t>(n)
        + 8ull * static_cast<std::uint64_t>(n);
}

std::uint32_t memory_block_info_slot_count_106d20_partial(std::uint32_t block_samples) {
    return ((block_samples + 255u) >> 7) | 1u;
}

std::uint64_t memory_accumulator_tail_payload_106d20_partial(std::uint64_t qword_at_a2) {
    const std::uint64_t a = 8ull * (qword_at_a2 & 0x7FFFFFFFULL);
    const std::uint64_t b = (12ull * qword_at_a2) & 0x3FFFFFFFCLL;
    return a + b;
}

std::uint64_t memory_frame_deque_required_additional_memory_13d570_partial(std::uint32_t deque_capacity) {
    return 336ull * static_cast<std::uint64_t>(deque_capacity);
}

std::uint64_t memory_delay_line_payload_sum_1068f0_partial(
    std::uint64_t a2_qword0,
    std::uint64_t a2_qword16,
    std::uint32_t a2_dword44) {
    const std::uint64_t first =
        4ull * a2_qword0 * a2_qword16 * static_cast<std::uint64_t>(a2_dword44);
    const std::uint64_t second = kDelayLineBufferSlotStrideBytes * a2_qword16;
    return first + second;
}

namespace {

CodecV3ParserRuntimeFns1034e0 codec_v3_default_parser_runtime_1034e0();
OutputGeneratorRuntimeFns1024a9 codec_v3_default_output_runtime_1024a9();

// IDA .rodata dword_2732C0 @ 0x2732C0 — маски для BitReader_get_unsigned_bits @ 0x105330.
constexpr std::uint32_t kDword2732C0[33] = {
    0u,         1u,          3u,          7u,          15u,         31u,
    63u,        127u,        255u,        511u,        1023u,       2047u,
    4095u,      8191u,       16383u,      32767u,      65535u,      131071u,
    262143u,    524287u,     1048575u,    2097151u,    4194303u,    8388607u,
    16777215u,  33554431u,   67108863u,   134217727u,  268435455u,  536870911u,
    1073741823u, 2147483647u, 0xFFFFFFFFu,
};

} // namespace

void channel_bit_reader_set_data_105440_partial(std::uint64_t br, std::uint64_t words_ptr, std::int32_t word_count) {
    *reinterpret_cast<std::uint64_t*>(br + 0u) = words_ptr;
    *reinterpret_cast<std::int32_t*>(br + 8u) = word_count;
    *reinterpret_cast<std::uint32_t*>(br + 12u) = 0u;
}

void channel_bit_reader_set_bitlines_1054b0_partial(std::uint64_t br, std::int32_t bitlines) {
    *reinterpret_cast<std::int32_t*>(br + 36u) = bitlines;
    *reinterpret_cast<std::int32_t*>(br + 32u) = bitlines;
}

std::uint32_t channel_bit_reader_get_unsigned_bits_105330_partial(std::uint64_t a1, std::int32_t a2) {
    auto apply_label17 = [a1]() {
        *reinterpret_cast<std::uint32_t*>(a1 + 28u) = 1u;
        *reinterpret_cast<std::uint64_t*>(a1 + 20u) = 0u;
    };
    auto apply_label16 = [a1, a2](std::uint32_t res, std::int32_t v3) {
        *reinterpret_cast<std::int32_t*>(a1 + 20u) = a2 - v3;
        *reinterpret_cast<std::uint32_t*>(a1 + 28u) = 0u;
        *reinterpret_cast<std::uint32_t*>(a1 + 24u) = res;
    };

    std::uint32_t result = *reinterpret_cast<std::uint32_t*>(a1 + 24u);
    std::int32_t v3 = a2 - *reinterpret_cast<std::int32_t*>(a1 + 20u);
    if (a2 == *reinterpret_cast<std::int32_t*>(a1 + 20u)) {
        apply_label17();
        return result;
    }
    std::uint32_t v4 = *reinterpret_cast<std::uint32_t*>(a1 + 16u);
    std::int32_t v5 = 3;
    if (v4 >= 0x10u)
        v5 = static_cast<std::int32_t>((v4 & 0xFu) == 0u);
    if (v3 <= 0) {
        apply_label16(result, v3);
        return result;
    }
    const std::uint32_t v6 = *reinterpret_cast<std::uint32_t*>(a1 + 8u);
    std::uint32_t v7 = *reinterpret_cast<std::uint32_t*>(a1 + 12u);
    if (v7 >= v6) {
        apply_label16(result, v3);
        return result;
    }
    bool exit_partial = false;
    while (true) {
        std::int32_t v8 = *reinterpret_cast<std::int32_t*>(a1 + 32u) - v5;
        if (v3 <= v8)
            v8 = v3;
        const std::uint32_t v9 = result << static_cast<std::uint32_t>(v8);
        const std::int32_t v10 = *reinterpret_cast<std::int32_t*>(a1 + 32u) - v8;
        const auto* words = reinterpret_cast<const std::uint32_t*>(*reinterpret_cast<std::uint64_t*>(a1 + 0u));
        const std::uint32_t v11 =
            kDword2732C0[static_cast<std::size_t>(v8)] & (words[v7] >> static_cast<std::uint32_t>(v10));
        *reinterpret_cast<std::int32_t*>(a1 + 32u) = v10;
        v3 -= v8;
        if (v10 <= v5) {
            ++v4;
            *reinterpret_cast<std::uint32_t*>(a1 + 16u) = v4;
            ++v7;
            *reinterpret_cast<std::uint32_t*>(a1 + 12u) = v7;
            v5 = 3;
            if (v4 >= 0x10u)
                v5 = static_cast<std::int32_t>((v4 & 0xFu) == 0u);
            *reinterpret_cast<std::uint32_t*>(a1 + 32u) = *reinterpret_cast<std::uint32_t*>(a1 + 36u);
        }
        result = v9 | v11;
        if (v3 <= 0)
            break;
        if (v7 >= v6) {
            exit_partial = true;
            break;
        }
    }
    if (exit_partial) {
        apply_label16(result, v3);
        return result;
    }
    if (v3 == 0)
        apply_label17();
    else
        apply_label16(result, v3);
    return result;
}

std::uint32_t channel_bit_reader_get_remaining_nr_bits_105450_partial(std::uint64_t br) {
    return *reinterpret_cast<std::uint32_t*>(br + 20u);
}

std::int64_t channel_bit_reader_get_signed_bits_105460_partial(std::uint64_t br, std::int32_t bit_count) {
    if (bit_count <= 0)
        return 0;
    const std::int64_t result =
        static_cast<std::int64_t>(channel_bit_reader_get_unsigned_bits_105330_partial(br, bit_count));
    const std::uint32_t v3 = (bit_count == 32) ? 0x80000000u : (1u << static_cast<std::uint32_t>(bit_count - 1));
    if (bit_count != 32 && (v3 & static_cast<std::uint32_t>(result)) != 0u)
        return -static_cast<std::int64_t>((~v3) & static_cast<std::uint32_t>(result));
    return result;
}

void channel_bit_reader_reset_105490_partial(std::uint64_t br) {
    std::memset(reinterpret_cast<void*>(br + 16u), 0, 16u);
    std::memset(reinterpret_cast<void*>(br), 0, 16u);
    *reinterpret_cast<std::uint64_t*>(br + 32u) = 0u;
}

void channel_crc_reset_1070d0_partial(std::uint64_t crc_state) {
    *reinterpret_cast<std::uint64_t*>(crc_state) = 0u;
}

namespace {

alignas(16) std::uint16_t g_crc_word_41acf0[256]{};
std::atomic<std::uint8_t> g_crc_table_ready{0};

#if AURO3DENG_CRC_SSE41

// Константы .rodata из IDA (get_bytes @ 0x1C2F60 .. 0x1C3100).
alignas(16) static const std::uint8_t kCrcXmm_1c2f60[16] = {
    0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00};
alignas(16) static const std::uint8_t kCrcXmm_1c2c30[16] = {
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00};
alignas(16) static const std::uint8_t kCrcXmm_1c3090[16] = {
    0x04, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00};
alignas(16) static const std::uint8_t kCrcXmm_1c30a0[16] = {
    0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00, 0x06, 0x00, 0x07, 0x00};
alignas(16) static const std::uint8_t kCrcXmm_1c30b0[16] = {
    0x00, 0xfe, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00};
alignas(16) static const std::uint8_t kCrcXmm_1c30c0[16] = {
    0x21, 0x10, 0x00, 0x00, 0x21, 0x10, 0x00, 0x00, 0x21, 0x10, 0x00, 0x00, 0x21, 0x10, 0x00, 0x00};
alignas(16) static const std::uint8_t kCrcXmm_1c30d0[16] = {
    0x00, 0x01, 0x04, 0x05, 0x08, 0x09, 0x0c, 0x0d, 0x08, 0x09, 0x0c, 0x0d, 0x0c, 0x0d, 0x0e, 0x0f};
alignas(16) static const std::uint8_t kCrcXmm_1c30e0[16] = {
    0x21, 0x10, 0x21, 0x10, 0x21, 0x10, 0x21, 0x10, 0x21, 0x10, 0x21, 0x10, 0x21, 0x10, 0x21, 0x10};
alignas(16) static const std::uint8_t kCrcXmm_1c30f0[16] = {
    0x01, 0x00, 0x03, 0x02, 0x05, 0x04, 0x07, 0x06, 0x09, 0x08, 0x0b, 0x0a, 0x0d, 0x0c, 0x0f, 0x0e};
alignas(16) static const std::uint8_t kCrcXmm_1c3100[16] = {
    0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00};

void crc_table_fill_106f00_sse41() {
    __m128i si128 = _mm_load_si128(reinterpret_cast<const __m128i*>(kCrcXmm_1c3090));
    __m128i v1 = _mm_load_si128(reinterpret_cast<const __m128i*>(kCrcXmm_1c2c30));
    __m128i v2 = _mm_load_si128(reinterpret_cast<const __m128i*>(kCrcXmm_1c30a0));
    __int64 v3 = 0;
    __m128i v4 = _mm_load_si128(reinterpret_cast<const __m128i*>(kCrcXmm_1c30b0));
    __m128i v5 = _mm_load_si128(reinterpret_cast<const __m128i*>(kCrcXmm_1c30c0));
    __m128i v6 = _mm_load_si128(reinterpret_cast<const __m128i*>(kCrcXmm_1c30d0));
    __m128i v7 = _mm_load_si128(reinterpret_cast<const __m128i*>(kCrcXmm_1c30e0));
    __m128i v8 = _mm_load_si128(reinterpret_cast<const __m128i*>(kCrcXmm_1c30f0));
    __m128i v9 = _mm_load_si128(reinterpret_cast<const __m128i*>(kCrcXmm_1c3100));
    __m128i v10 = _mm_load_si128(reinterpret_cast<const __m128i*>(kCrcXmm_1c2f60));
    const __m128i zero = _mm_setzero_si128();
    do {
        __m128i v11 = _mm_cmpgt_epi16(zero, _mm_srai_epi16(_mm_slli_epi16(v2, 8u), 8u));
        __m128i v12 = _mm_cvtepu16_epi32(v11);
        __m128 v13 = _mm_castsi128_ps(_mm_slli_epi32(si128, 9u));
        __m128 v14 = _mm_castsi128_ps(_mm_slli_epi32(v1, 9u));
        __m128i v15 = _mm_unpacklo_epi64(
            _mm_shuffle_epi8(
                _mm_castps_si128(_mm_blendv_ps(
                    v14,
                    _mm_castsi128_ps(_mm_xor_si128(_mm_and_si128(_mm_castps_si128(v14), v4), v5)),
                    _mm_castsi128_ps(_mm_slli_epi32(v12, 0x18u)))),
                v6),
            _mm_shuffle_epi8(
                _mm_castps_si128(_mm_blendv_ps(
                    v13,
                    _mm_castsi128_ps(_mm_xor_si128(_mm_and_si128(_mm_castps_si128(v13), v4), v5)),
                    _mm_castsi128_ps(_mm_slli_epi32(_mm_unpackhi_epi16(v11, v12), 0x18u)))),
                v6));
        __m128i v16 = _mm_cmpgt_epi16(zero, v15);
        __m128i v17 = _mm_add_epi16(v15, v15);
        __m128i v18 = _mm_blendv_epi8(v17, _mm_xor_si128(v17, v7), v16);
        __m128i v19 = _mm_cmpgt_epi16(zero, v18);
        __m128i v20 = _mm_add_epi16(v18, v18);
        __m128i v21 = _mm_blendv_epi8(v20, _mm_xor_si128(v20, v7), v19);
        __m128i v22 = _mm_cmpgt_epi16(zero, v21);
        __m128i v23 = _mm_add_epi16(v21, v21);
        __m128i v24 = _mm_blendv_epi8(v23, _mm_xor_si128(v23, v7), v22);
        __m128i v25 = _mm_cmpgt_epi16(zero, v24);
        __m128i v26 = _mm_add_epi16(v24, v24);
        __m128i v27 = _mm_blendv_epi8(v26, _mm_or_si128(v26, v7), v25);
        __m128i v28 = _mm_cmpgt_epi16(zero, v27);
        __m128i v29 = _mm_add_epi16(v27, v27);
        __m128i v30 = _mm_blendv_epi8(v29, _mm_xor_si128(v29, v7), v28);
        __m128i v31 = _mm_cmpgt_epi16(zero, v30);
        __m128i v32 = _mm_add_epi16(v30, v30);
        __m128i v33 = _mm_blendv_epi8(v32, _mm_xor_si128(v32, v7), v31);
        __m128i v34 = _mm_cmpgt_epi16(zero, v33);
        __m128i v35 = _mm_add_epi16(v33, v33);
        *reinterpret_cast<__m128i*>(reinterpret_cast<std::uint8_t*>(g_crc_word_41acf0) + v3) =
            _mm_shuffle_epi8(_mm_blendv_epi8(v35, _mm_xor_si128(v35, v7), v34), v8);
        v2 = _mm_add_epi16(v2, v9);
        v1 = _mm_add_epi32(v1, v10);
        si128 = _mm_add_epi32(si128, v10);
        v3 += 16;
    } while (v3 != 512);
}

#endif // AURO3DENG_CRC_SSE41

void crc_table_fill_106f00_scalar() {
    for (std::uint32_t i = 0; i != 256u; ++i) {
        std::uint16_t v = static_cast<std::uint16_t>(i << 8u);
        for (std::uint32_t bit = 0; bit != 8u; ++bit) {
            if ((v & 0x8000u) != 0u)
                v = static_cast<std::uint16_t>((v << 1u) ^ 0x1021u);
            else
                v = static_cast<std::uint16_t>(v << 1u);
        }
        g_crc_word_41acf0[i] =
            static_cast<std::uint16_t>((static_cast<std::uint16_t>(v & 0x00FFu) << 8u) | (v >> 8u));
    }
}

void crc_table_ensure_once() {
    if (g_crc_table_ready.load(std::memory_order_acquire) == 2)
        return;
    std::uint8_t expected = 0;
    if (g_crc_table_ready.compare_exchange_strong(expected, 1, std::memory_order_acq_rel)) {
#if AURO3DENG_CRC_SSE41
        crc_table_fill_106f00_sse41();
#else
        crc_table_fill_106f00_scalar();
#endif
        g_crc_table_ready.store(2, std::memory_order_release);
        return;
    }
    while (g_crc_table_ready.load(std::memory_order_acquire) != 2)
        ;
}

} // namespace

void channel_crc_table_init_106f00_partial() {
    crc_table_ensure_once();
}

std::uint64_t channel_crc_process_1070e0_partial(
    std::uint64_t crc_state,
    std::uint64_t words_ptr,
    std::int32_t word_count) {
    // IDA auro_codec_v3_decoder_CRC_process @ 0x52ECA0.
    crc_table_ensure_once();
    auto* a1 = reinterpret_cast<std::uint16_t*>(crc_state);
    std::uint16_t v3 = *a1;
    std::uint32_t result = *reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uint8_t*>(a1) + 4u);
    const auto* words = reinterpret_cast<const std::uint32_t*>(words_ptr);
    if (word_count != 0) {
        for (std::int32_t i = 0; i != word_count; ++i) {
            const std::uint32_t v6 = words[static_cast<std::size_t>(i)];
            const std::uint32_t mask =
                253u + 2u * static_cast<std::uint32_t>(
                    (result + static_cast<std::uint32_t>(i)) >= 0x10u);
            const std::uint16_t v7 = g_crc_word_41acf0[
                ((v6 & mask) ^ static_cast<std::uint8_t>(v3)) & 0xFFu];
            const std::uint32_t v8 = g_crc_word_41acf0[
                static_cast<std::uint8_t>(
                    static_cast<std::uint8_t>(v7)
                    ^ static_cast<std::uint8_t>(v3 >> 8)
                    ^ static_cast<std::uint8_t>((v6 >> 8) & 0xFFu))];
            v3 = static_cast<std::uint16_t>(
                g_crc_word_41acf0[static_cast<std::uint8_t>(
                    static_cast<std::uint8_t>(v8)
                    ^ static_cast<std::uint8_t>(v7 >> 8)
                    ^ static_cast<std::uint8_t>((v6 >> 16) & 0xFFu))]
                ^ static_cast<std::uint16_t>(v8 >> 8));
        }
        result = result + static_cast<std::uint32_t>(word_count);
    }
    *a1 = v3;
    *reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uint8_t*>(a1) + 4u) = result;
    return result;
}

std::int64_t channel_crc_check_107170_partial(
    std::uint64_t crc_state,
    std::int32_t dword_at_plus4_compare,
    std::int16_t expected_crc_word) {
    const auto* base = reinterpret_cast<const std::uint8_t*>(crc_state);
    if (*reinterpret_cast<const std::uint32_t*>(base + 4u)
        != static_cast<std::uint32_t>(dword_at_plus4_compare))
        return 1;
    const std::uint16_t w = *reinterpret_cast<const std::uint16_t*>(base);
    const std::uint16_t rotated =
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(~w) << 8u | static_cast<std::uint16_t>(~w) >> 8u);
    if (rotated == static_cast<std::uint16_t>(static_cast<std::uint16_t>(expected_crc_word)))
        return 1;
    return 0;
}

std::int64_t channel_metadata_combine_info_105d50_partial(std::uint64_t frame_channel_ptr) {
    const std::uint16_t v1 = *reinterpret_cast<const std::uint16_t*>(frame_channel_ptr + 76u);
    if (!(v1 <= 0x1FFu && (v1 < 0x100u || static_cast<std::uint8_t>(v1) <= 0x0Au)))
        return 0;
    const std::uint32_t v4 = *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 80u);
    *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 44u) = static_cast<std::uint8_t>(v4);
    const std::uint32_t v5 = (v4 >> 16) & 0xFFu;
    std::int32_t v6 = 0;
    if (v5 > 4u) {
        if (v5 - 5u > 0xAu) {
            if (v5 - 16u > 0x43u) {
                *reinterpret_cast<std::int32_t*>(frame_channel_ptr + 40u) = -1;
                return 0;
            }
            v6 = static_cast<std::int32_t>(8u * v5 - 64u);
        } else {
            v6 = static_cast<std::int32_t>(4u * v5);
        }
    } else {
        v6 = static_cast<std::int32_t>(2u * v5 + 8u);
    }
    *reinterpret_cast<std::int32_t*>(frame_channel_ptr + 40u) = v6;
    *reinterpret_cast<std::int32_t*>(frame_channel_ptr + 104u) = 0;
    auto map_id = [](std::uint32_t id) -> std::uint32_t { return id == 255u ? 31u : id; };
    auto valid = [](std::uint32_t id) -> bool { return id < kCodecV3ChannelCount; };
    const std::uint32_t id0 =
        map_id((*reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 84u) >> 24) & 0xFFu);
    *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 108u) = id0;
    *reinterpret_cast<std::int32_t*>(frame_channel_ptr + 104u) += valid(id0) ? 1 : 0;
    const std::uint32_t id1 = map_id(*reinterpret_cast<const std::uint8_t*>(frame_channel_ptr + 86u));
    *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 112u) = id1;
    *reinterpret_cast<std::int32_t*>(frame_channel_ptr + 104u) += valid(id1) ? 1 : 0;
    const std::uint32_t id2 = map_id(*reinterpret_cast<const std::uint8_t*>(frame_channel_ptr + 85u));
    *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 116u) = id2;
    *reinterpret_cast<std::int32_t*>(frame_channel_ptr + 104u) += valid(id2) ? 1 : 0;
    return 1;
}

std::int64_t channel_get_carrier_layout_530db0_partial(std::uint32_t* out_layout, std::uint32_t original_layout) {
    if (!out_layout)
        return 0;
    switch (original_layout) {
    case 3u: case 4u:
        *out_layout = 4u; return 1;
    case 7u: case 51u: case 55u: case 71u: case 1587u:
        *out_layout = 3u; return 1;
    case 63u:
        *out_layout = 11u; return 1;
    case 119u: case 439u: case 26167u: case 30263u: case 32311u:
        *out_layout = 55u; return 1;
    case 127u: case 447u: case 1599u: case 26175u: case 30271u: case 32319u:
        *out_layout = 63u; return 1;
    case 2052u: case 6148u:
        *out_layout = 4u; return 1;
    case 26163u:
        *out_layout = 51u; return 1;
    case 26551u: case 30647u: case 32695u:
        *out_layout = 439u; return 1;
    case 26559u: case 30655u: case 32703u: case 1983u:
        *out_layout = 447u; return 1;
    default:
        return 0;
    }
}

std::int64_t channel_parser_construct_104210_partial(
    std::uint64_t parser_base,
    std::uint64_t frame_channel_ptr,
    std::uint64_t frame_meta_16b_ptr,
    std::uint64_t frame_slot_ptr) {
    std::memset(reinterpret_cast<void*>(parser_base), 0, 64u);
    channel_bit_reader_reset_105490_partial(parser_base + 24u);
    channel_crc_reset_1070d0_partial(parser_base + 12u);
    *reinterpret_cast<std::uint32_t*>(parser_base + 8u) = 1u;
    *reinterpret_cast<std::uint16_t*>(frame_channel_ptr + 52u) =
        *reinterpret_cast<const std::uint16_t*>(frame_slot_ptr + 0u);
    const auto* m = reinterpret_cast<const std::uint32_t*>(frame_meta_16b_ptr);
    auto* d = reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 56u);
    // IDA: _mm_shuffle_epi32(loadu(a3), 147) => [3,0,1,2].
    d[0] = m[3];
    d[1] = m[0];
    d[2] = m[1];
    d[3] = m[2];
    const std::uint32_t v = *reinterpret_cast<const std::uint32_t*>(frame_slot_ptr + 8u);
    *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 72u) = v;
    return v;
}

std::uint32_t ida_signed_shift_u32_103840(std::uint32_t value, std::int32_t shift) {
    // IDA channel_Parser_process: `v << (*(_BYTE *)a1 - *((_BYTE *)a1 + 4))` / `v << -(char)rb`.
    // Native is x86 SHL with count = (int8_t)shift; 32-bit SHL uses count & 31.
    // Example: rb=5 → count=-5 → 0xFB & 31 = 27 → left-align 5 bits into MSB.
    const auto count = static_cast<std::uint8_t>(static_cast<std::int8_t>(shift));
    return value << (static_cast<std::uint32_t>(count) & 31u);
}

// libauro3d `0x103840` / libauro `auro_codec_v3_ida::kLibauro_codec_channel_Parser_process`.
std::int64_t channel_parser_process_103840(
    std::uint64_t parser_base,
    std::uint64_t frame_channel_ptr,
    std::uint64_t channel_ptr,
    std::uint32_t word_count,
    std::uint32_t* out_mode) {
    if (out_mode)
        *out_mode = 0u;

    auto* a1 = reinterpret_cast<std::uint32_t*>(parser_base);
    auto* br_ok = reinterpret_cast<std::uint32_t*>(parser_base + 52u);
    const std::uint64_t br = parser_base + 24u;
    // IDA auro_codec_v3_adol_ChannelInputConfig_get_original_layout @ 0x530850.
    // Ops 3/9/10/53/60/61 are only accepted when aliases (a3) != 0.
    auto get_original_layout = [](std::uint32_t op, std::uint32_t* out, bool aliases) -> bool {
        switch (op) {
        case 1u: *out = 55u; return true;
        case 2u: *out = 63u; return true;
        case 8u: *out = 71u; return true;
        case 12u: *out = 51u; return true;
        case 11u: *out = 1587u; return true;
        case 15u: *out = 1599u; return true;
        case 20u: *out = 26163u; return true;
        case 30u: *out = 26175u; return true;
        case 40u: *out = 30271u; return true;
        case 50u: *out = 32319u; return true;
        case 54u: *out = 26559u; return true;
        case 62u: *out = 32703u; return true;
        case 64u: *out = 3u; return true;
        case 66u: *out = 7u; return true;
        case 67u: *out = 119u; return true;
        case 68u: *out = 127u; return true;
        case 69u: *out = 439u; return true;
        case 70u: *out = 447u; return true;
        case 71u: *out = 26167u; return true;
        case 72u: *out = 30263u; return true;
        case 73u: *out = 32311u; return true;
        case 74u: *out = 26551u; return true;
        case 75u: *out = 1983u; return true;
        case 76u: *out = 30647u; return true;
        case 77u: *out = 30655u; return true;
        case 78u: *out = 32695u; return true;
        case 128u: *out = 4u; return true;
        case 129u: *out = 2052u; return true;
        case 130u: *out = 6148u; return true;
        default:
            if (!aliases)
                return false;
            switch (op) {
            case 3u: *out = 55u; return true;
            case 9u: *out = 51u; return true;
            case 10u: *out = 1587u; return true;
            case 53u: *out = 32319u; return true;
            case 60u: *out = 65151u; return true;
            case 61u: *out = 98111u; return true;
            default: return false;
            }
        }
    };

    if (a1[2] == 0u)
        return 0;

    if (a1[2] == 1u) {
        if (*reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 64u) == 0u
            || *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 60u) != 0u) {
            return 0;
        }
        channel_bit_reader_set_bitlines_1054b0_partial(
            br,
            static_cast<std::int32_t>(
                24u - *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 72u)));
        a1[2] = 2u;
    }

    channel_bit_reader_set_data_105440_partial(br, channel_ptr, static_cast<std::int32_t>(word_count));
    // a1[10] == BitReader+16 (words-consumed counter). IDA does not overwrite it with a4.
    if (a1[2] == 2u)
        a1[2] = 3u;

    if (word_count != 0u) {
        while (true) {
            const std::uint32_t state = a1[2];
            switch (state) {
            case 3u: {
                const std::uint16_t u = static_cast<std::uint16_t>(
                    channel_bit_reader_get_unsigned_bits_105330_partial(br, 16));
                if (*br_ok == 0u)
                    break;
                *reinterpret_cast<std::uint16_t*>(frame_channel_ptr + 76u) = u;
                a1[0] = 0u;
                a1[1] = 3u;
                a1[2] = 4u;
                continue;
            }
            case 4u: {
                const std::uint32_t v = channel_bit_reader_get_unsigned_bits_105330_partial(br, 32);
                if (*br_ok == 0u)
                    break;
                *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 80u + 4u * static_cast<std::uint64_t>(a1[0])) = v;
                a1[0] += 1u;
                if (a1[0] >= a1[1])
                    a1[2] = 5u;
                continue;
            }
            case 5u: {
                if (channel_metadata_combine_info_105d50_partial(frame_channel_ptr) == 0)
                    return 0;
                a1[0] = 0u;
                const std::uint32_t mode = *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 104u);
                a1[1] = (mode == 3u) ? 5u : ((mode == 2u) ? 2u : 0u);
                a1[2] = 6u;
                continue;
            }
            case 6u: {
                if (a1[0] != a1[1]) {
                    const std::int32_t sv = static_cast<std::int32_t>(
                        channel_bit_reader_get_signed_bits_105460_partial(br, 32));
                    if (*br_ok == 0u)
                        break;
                    *reinterpret_cast<std::int32_t*>(
                        frame_channel_ptr + 16u + 4u * static_cast<std::uint64_t>(a1[0])) = sv;
                }
                if (*br_ok == 0u)
                    break;
                a1[0] += 1u;
                if (a1[0] >= a1[1]) {
                    a1[0] = 0u;
                    const std::uint32_t v96 = (*reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 80u) >> 8u) & 0xFu;
                    *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 96u) = v96;
                    if (v96 == 0u) {
                        a1[2] = 20u;
                    } else {
                        a1[2] = 7u;
                        a1[1] = 1u;
                    }
                }
                continue;
            }
            case 7u: {
                const std::uint32_t v = channel_bit_reader_get_unsigned_bits_105330_partial(br, 8);
                if (*br_ok == 0u)
                    break;
                a1[0] = 0u;
                a1[1] = 0u;
                if (v == 2u) {
                    a1[2] = 18u;
                    continue;
                }
                if (v != 1u)
                    return 0;
                a1[2] = 8u;
                continue;
            }
            case 8u: {
                // IDA reads signed char opcode; 128..145 == native -128..-112.
                const std::uint32_t op = channel_bit_reader_get_unsigned_bits_105330_partial(br, 8);
                if (*br_ok == 0u)
                    break;
                a1[0] = 0u;
                a1[1] = 0u;
                switch (op) {
                case 14u:
                case 128u: case 129u: case 130u: case 131u: case 132u: case 133u:
                case 140u: case 141u: case 142u: case 143u: case 144u: case 145u:
                case 110u: case 111u: case 112u: case 113u: case 114u: case 115u:
                case 116u: case 117u: case 118u:
                    a1[2] = 16u;
                    continue;
                case 0u:
                    if ((*reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 96u))-- == 1u)
                        a1[2] = 20u;
                    else
                        a1[2] = 7u;
                    continue;
                case 1u: case 3u:
                case 90u: case 91u: case 92u: case 93u: case 94u:
                case 95u: case 96u: case 97u: case 98u:
                    a1[2] = 14u;
                    continue;
                case 2u: case 4u: case 31u:
                case 71u: case 80u: case 81u: case 82u: case 83u:
                case 84u: case 85u: case 86u: case 87u: case 88u:
                    a1[2] = 13u;
                    continue;
                case 30u:
                    a1[2] = 17u;
                    continue;
                case 64u:
                    a1[2] = 9u;
                    continue;
                case 65u:
                    a1[2] = 11u;
                    continue;
                case 70u:
                    a1[2] = 12u;
                    continue;
                case 100u: case 101u: case 102u: case 103u: case 104u: case 105u:
                case 106u: case 107u: case 108u:
                    a1[2] = 15u;
                    continue;
                default:
                    continue;
                }
            }
            case 9u: {
                // IDA: store then check br_ok; on fail exit CRC with state still 9.
                *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 100u) =
                    channel_bit_reader_get_unsigned_bits_105330_partial(br, 8) & 0xFFu;
                if (*br_ok == 0u)
                    break;
                a1[2] = 10u;
                continue;
            }
            case 10u: {
                const std::uint32_t v = channel_bit_reader_get_unsigned_bits_105330_partial(br, 8) & 0xFFu;
                if (*br_ok == 0u)
                    break;
                const std::uint32_t key = *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 100u);
                std::uint32_t* dst = reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 0u);
                if (key == *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 108u)
                    || (dst = reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 4u),
                        key == *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 112u))
                    || (dst = reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 8u),
                        key == *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 116u))) {
                    *dst = v;
                }
                a1[2] = 8u;
                continue;
            }
            case 11u: {
                // IDA: store then check br_ok; on fail exit CRC with state still 11.
                *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 48u) =
                    channel_bit_reader_get_unsigned_bits_105330_partial(br, 8) & 0xFFu;
                if (*br_ok == 0u)
                    break;
                a1[2] = 8u;
                continue;
            }
            case 12u: {
                const std::uint32_t v = channel_bit_reader_get_unsigned_bits_105330_partial(br, 32);
                std::uint32_t carrier_layout = 0u;
                if (channel_get_carrier_layout_530db0_partial(
                        &carrier_layout,
                        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 92u)) == 0) {
                    return 0;
                }
                if (*br_ok == 0u)
                    break;
                std::uint32_t shift_a = 16u;
                std::uint32_t shift_b = 12u;
                std::uint32_t shift_c = 8u;
                std::uint32_t shift_d = 4u;
                std::uint32_t seed = v;
                std::uintptr_t off_a_flag = 120u, off_a_value = 124u;
                std::uintptr_t off_b_flag = 136u, off_b_value = 140u;
                std::uintptr_t off_c_flag = 128u, off_c_value = 132u;
                std::uintptr_t off_d_flag = 152u, off_d_value = 156u;
                std::uintptr_t off_e_flag = 160u, off_e_value = 164u;
                // IDA: compact iff layout<=0x3F && bittest(0x8088000000000818, layout)
                // → 3,4,11,51,55,63 (0x3F==63; bits 51/55/63 are set in the mask).
                const bool compact_carrier =
                    carrier_layout == 3u || carrier_layout == 4u || carrier_layout == 11u
                    || carrier_layout == 51u || carrier_layout == 55u || carrier_layout == 63u;
                if (!compact_carrier) {
                    if (carrier_layout != 439u && carrier_layout != 447u)
                        return 0;
                    *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 120u) = 1u;
                    *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 124u) = kDword289FC0[v & 0xFu];
                    *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 136u) = 1u;
                    *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 140u) = kDword289FC0[(v >> 4u) & 0xFu];
                    seed = v >> 8u;
                    shift_a = 24u;
                    shift_b = 20u;
                    shift_c = 16u;
                    shift_d = 12u;
                    off_a_flag = 128u; off_a_value = 132u;
                    off_b_flag = 152u; off_b_value = 156u;
                    off_c_flag = 160u; off_c_value = 164u;
                    off_d_flag = 176u; off_d_value = 180u;
                    off_e_flag = 184u; off_e_value = 188u;
                }
                *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + off_a_flag) = 1u;
                *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + off_a_value) = kDword289FC0[seed & 0xFu];
                *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + off_b_flag) = 1u;
                *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + off_b_value) =
                    kDword289FC0[(v >> shift_d) & 0xFu];
                *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + off_c_flag) = 1u;
                *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + off_c_value) =
                    kDword289FC0[(v >> shift_c) & 0xFu];
                *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + off_d_flag) = 1u;
                *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + off_d_value) =
                    kDword289FC0[(v >> shift_b) & 0xFu];
                *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + off_e_flag) = 1u;
                *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + off_e_value) =
                    kDword289FC0[(v >> shift_a) & 0xFu];
                *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 144u) = 1u;
                *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 148u) = kDword289FC0[v >> 28u];
                a1[2] = 8u;
                continue;
            }
            case 13u:
                (void)channel_bit_reader_get_unsigned_bits_105330_partial(br, 8);
                if (*br_ok == 0u)
                    break;
                a1[2] = 8u;
                continue;
            case 14u:
                (void)channel_bit_reader_get_unsigned_bits_105330_partial(br, 16);
                if (*br_ok == 0u)
                    break;
                a1[2] = 8u;
                continue;
            case 15u:
                (void)channel_bit_reader_get_unsigned_bits_105330_partial(br, 24);
                if (*br_ok == 0u)
                    break;
                a1[2] = 8u;
                continue;
            case 16u:
                (void)channel_bit_reader_get_unsigned_bits_105330_partial(br, 32);
                if (*br_ok == 0u)
                    break;
                a1[2] = 8u;
                continue;
            case 17u: {
                const std::uint32_t op = channel_bit_reader_get_unsigned_bits_105330_partial(br, 8) & 0xFFu;
                if (*br_ok == 0u)
                    break;
                std::uint32_t layout = 0u;
                if (!get_original_layout(op, &layout, true))
                    return 0;
                *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 92u) = layout;
                a1[2] = 8u;
                continue;
            }
            case 18u: {
                const std::uint32_t v = channel_bit_reader_get_unsigned_bits_105330_partial(br, 24);
                if (*br_ok == 0u)
                    break;
                a1[0] = 0u;
                a1[1] = (v >> 16u) & 0xFFu;
                if (a1[1] != 0u) {
                    a1[2] = 19u;
                } else {
                    if ((*reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 96u))-- == 1u) a1[2] = 20u; else a1[2] = 7u;
                }
                continue;
            }
            case 19u:
                (void)channel_bit_reader_get_unsigned_bits_105330_partial(br, 32);
                if (*br_ok == 0u)
                    break;
                a1[0] += 1u;
                if (a1[0] >= a1[1]) {
                    if ((*reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 96u))-- == 1u) a1[2] = 20u; else a1[2] = 7u;
                }
                continue;
            case 20u: {
                a1[0] = 0u;
                const std::uint32_t base = *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 40u)
                    * *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 44u);
                a1[1] = (*reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 104u) == 3u)
                    ? (2u * base) : base;
                a1[2] = 21u;
                continue;
            }
            case 21u: {
                const std::uint32_t rem = a1[1] - a1[0];
                const std::uint32_t bits = (rem <= 32u) ? rem : 32u;
                const std::uint32_t vv = channel_bit_reader_get_unsigned_bits_105330_partial(br, static_cast<std::int32_t>(bits));
                if (*br_ok == 0u)
                    break;
                std::uint64_t& n = *reinterpret_cast<std::uint64_t*>(
                    frame_channel_ptr + kFrameChannelOff_ctx_count_qword);
                const std::uint64_t idx = n++;
                if (rem <= 32u) {
                    const std::int32_t sh =
                        static_cast<std::int32_t>(a1[0]) - static_cast<std::int32_t>(a1[1]);
                    *reinterpret_cast<std::uint32_t*>(
                        frame_channel_ptr + kFrameChannelOff_ctx_words + 4u * idx) =
                        ida_signed_shift_u32_103840(vv, sh);
                    a1[0] = 0u;
                    a1[1] = 27u;
                    a1[2] = 22u;
                } else {
                    *reinterpret_cast<std::uint32_t*>(
                        frame_channel_ptr + kFrameChannelOff_ctx_words + 4u * idx) = vv;
                    a1[0] += 32u;
                }
                continue;
            }
            case 22u: {
                const std::uint32_t vv = channel_bit_reader_get_unsigned_bits_105330_partial(br, 32);
                if (*br_ok != 0u) {
                    std::uint64_t& n = *reinterpret_cast<std::uint64_t*>(
                        frame_channel_ptr + kFrameChannelOff_stream_count_qword);
                    const std::uint64_t idx = n++;
                    *reinterpret_cast<std::uint32_t*>(
                        frame_channel_ptr + kFrameChannelOff_stream_words + 4u * idx) = vv;
                    continue;
                }
                // IDA @ 0x52F330 case 22: br_ok==0; skip tail when frame+56 > a1[10]
                // (BitReader+16 words-consumed). Tail then LABEL_38 → state 23 → CRC.
                if (*reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 56u) > a1[10])
                    break;
                const std::uint32_t rb = channel_bit_reader_get_remaining_nr_bits_105450_partial(br);
                const std::uint32_t tail = channel_bit_reader_get_unsigned_bits_105330_partial(
                    br,
                    static_cast<std::int32_t>(rb));
                std::uint64_t& n = *reinterpret_cast<std::uint64_t*>(
                    frame_channel_ptr + kFrameChannelOff_stream_count_qword);
                const std::uint64_t idx = n++;
                const std::int32_t sh = -static_cast<std::int32_t>(rb);
                *reinterpret_cast<std::uint32_t*>(
                    frame_channel_ptr + kFrameChannelOff_stream_words + 4u * idx) =
                    ida_signed_shift_u32_103840(tail, sh);
                // LABEL_38: v13=1; with br_ok after successful remaining read → a1[2] = 23.
                if (*br_ok != 0u)
                    a1[2] = 23u;
                break;
            }
            case 23u:
                break;
            default:
                continue;
            }
            break;
        }
    }

    channel_crc_process_1070e0_partial(
        parser_base + 12u,
        channel_ptr,
        static_cast<std::int32_t>(word_count));
    const std::int64_t crc_ck = channel_crc_check_107170_partial(
        parser_base + 12u,
        *reinterpret_cast<const std::int32_t*>(frame_channel_ptr + 56u),
        *reinterpret_cast<const std::int16_t*>(frame_channel_ptr + 52u));
    if (out_mode)
        *out_mode = *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 92u);
    return (crc_ck != 0) ? 1 : 0;
}

bool parse_result_pool_construct_1071b0_partial(
    std::uint64_t pool_base_ptr,
    std::uint64_t storage_base_ptr,
    std::uint32_t pool_count) {
    if (pool_base_ptr == 0u)
        return false;
    // IDA 0x1071B0 layout:
    // [0x00] storage base qword, [0x08] cursor dword, [0x0C] count dword
    *reinterpret_cast<std::uint64_t*>(pool_base_ptr + 0u) = storage_base_ptr;
    *reinterpret_cast<std::uint32_t*>(pool_base_ptr + 8u) = 0u;
    *reinterpret_cast<std::uint32_t*>(pool_base_ptr + 12u) = pool_count;
    std::uint64_t offset = 0u;
    for (std::uint32_t i = 0; i < pool_count; ++i) {
        const std::uint64_t parse_result_ptr = storage_base_ptr + offset;
        channel_parse_result_construct_103810_partial(parse_result_ptr);
        offset += kParseResultStrideBytes;
    }
    return storage_base_ptr != 0u || pool_count == 0u;
}

std::uint64_t parse_result_pool_get_new_107220_partial(std::uint64_t pool_base_ptr) {
    if (pool_base_ptr == 0u)
        return 0u;
    const std::uint64_t storage_base = *reinterpret_cast<const std::uint64_t*>(pool_base_ptr + 0u);
    const std::uint32_t count = *reinterpret_cast<const std::uint32_t*>(pool_base_ptr + 12u);
    if (storage_base == 0u || count == 0u)
        return 0u;
    const std::uint32_t cursor =
        (*reinterpret_cast<const std::uint32_t*>(pool_base_ptr + 8u) + 1u) % count;
    *reinterpret_cast<std::uint32_t*>(pool_base_ptr + 8u) = cursor;
    const std::uint64_t parse_result_ptr =
        storage_base + static_cast<std::uint64_t>(kParseResultStrideBytes) * static_cast<std::uint64_t>(cursor);
    channel_parse_result_prepare_new_107220_partial(parse_result_ptr);
    return parse_result_ptr;
}

void downmix_coefficients_initialize_105d30_partial(std::uint64_t coeff_ptr) {
    if (coeff_ptr == 0u)
        return;
    // IDA 0x105D30: two xmmword constants copied to coeff block (+0, +16).
    auto* dst = reinterpret_cast<std::uint32_t*>(coeff_ptr);
    dst[0] = 0x3F34FDF4u;
    dst[1] = 0x3F34FDF4u;
    dst[2] = 0x3F000000u;
    dst[3] = 0x3F800000u;
    dst[4] = 0x3F189375u;
    dst[5] = 0x3F189375u;
    dst[6] = 0x3F800000u;
    dst[7] = 0x3F800000u;
}

void channel_parse_result_construct_103810_partial(std::uint64_t parse_result_ptr) {
    // current IDA ParseResult_t_construct: zero header 0..191 and tail 3648..3667.
    std::memset(reinterpret_cast<void*>(parse_result_ptr), 0, 192u);
    std::memset(reinterpret_cast<void*>(parse_result_ptr + 3648u), 0, 20u);
}

void frame_construct_parse_results_106cd0_partial(
    std::uint64_t frame_ptr,
    void (*construct_parse_result)(std::uint64_t parse_result_ptr)) {
    if (frame_ptr == 0u || construct_parse_result == nullptr)
        return;
    const std::uint32_t count = *reinterpret_cast<const std::uint32_t*>(frame_ptr + kFrameOff_channel_count_dword);
    // IDA 0x106CD0-style walk: first parse_result ptr at frame+72, slot stride 32.
    std::uintptr_t slot_parse_result_ptr = frame_ptr + kFrameOff_channel_slot_base + kFrameChSlotOff_channel_ptr_qword;
    for (std::uint32_t i = 0; i < count; ++i) {
        const std::uint64_t parse_result_ptr = *reinterpret_cast<const std::uint64_t*>(slot_parse_result_ptr);
        if (parse_result_ptr != 0u)
            construct_parse_result(parse_result_ptr);
        slot_parse_result_ptr += kFrameStride_channel_slot;
    }
}

void frame_construct_106ba0_partial(
    std::uint64_t frame_ptr,
    std::uint64_t start,
    std::uint32_t span,
    std::uint32_t channel_mask,
    std::uint32_t active_mask) {
    if (frame_ptr == 0u)
        return;
    *reinterpret_cast<std::uint64_t*>(frame_ptr + 0u) = start;
    *reinterpret_cast<std::uint64_t*>(frame_ptr + 8u) = start + static_cast<std::uint64_t>(span);
    *reinterpret_cast<std::uint32_t*>(frame_ptr + 16u) = span;
    *reinterpret_cast<std::uint32_t*>(frame_ptr + 24u) = 0u;

    // current IDA Frame_t_construct: clear 9 slot descriptors (offsets 48..328, stride 32).
    for (std::uint32_t i = 0; i < 9u; ++i) {
        const std::uintptr_t slot = frame_ptr + 48u + static_cast<std::uintptr_t>(i) * 32u;
        *reinterpret_cast<std::uint64_t*>(slot + 24u) = 0u;
        *reinterpret_cast<std::uint32_t*>(slot + 0u) = 31u;
        *reinterpret_cast<std::uint32_t*>(slot + 4u) = 0u;
    }

    *reinterpret_cast<std::uint32_t*>(frame_ptr + 20u) = channel_mask;
    *reinterpret_cast<std::uint32_t*>(frame_ptr + 44u) = 0u;

    std::uint32_t slot_idx = 0u;
    std::uint32_t ch = 0u;
    std::uint32_t bit_mask = 1u;
    while (ch < 31u) {
        if ((channel_mask & bit_mask) != 0u) {
            *reinterpret_cast<std::uint32_t*>(frame_ptr + 44u) = slot_idx + 1u;
            const std::uintptr_t slot =
                frame_ptr + 48u + static_cast<std::uintptr_t>(slot_idx) * 32u;
            *reinterpret_cast<std::uint32_t*>(slot + 0u) = ch;
            *reinterpret_cast<std::uint32_t*>(slot + 4u) =
                ((active_mask & bit_mask) != 0u) ? 1u : 0u;
            ++slot_idx;
        }
        ++ch;
        bit_mask <<= 1u;
    }
}

void frame_assign_window_partial(
    std::uint64_t* io_next_frame_start,
    std::uint64_t frame_span,
    std::uint64_t frame_ptr) {
    if (frame_ptr == 0u || io_next_frame_start == nullptr)
        return;
    const std::uint64_t start = *io_next_frame_start;
    const std::uint64_t span = (frame_span != 0u) ? frame_span : 1u;
    const std::uint64_t end = start + span;
    *reinterpret_cast<std::uint64_t*>(frame_ptr + 0u) = start;
    *reinterpret_cast<std::uint64_t*>(frame_ptr + 8u) = end;
    *reinterpret_cast<std::uint32_t*>(frame_ptr + 24u) = 0u;
    *io_next_frame_start = end;
}

void frame_mark_usage_106cd0_partial(
    std::uint64_t frame_ptr,
    std::uint32_t in_use,
    void (*mark_usage)(std::uint64_t parse_result_ptr, std::uint32_t in_use)) {
    if (frame_ptr == 0u || mark_usage == nullptr)
        return;
    const std::uint32_t count = *reinterpret_cast<const std::uint32_t*>(frame_ptr + kFrameOff_channel_count_dword);
    std::uintptr_t slot_parse_result_ptr = frame_ptr + kFrameOff_channel_slot_base + kFrameChSlotOff_channel_ptr_qword;
    for (std::uint32_t i = 0; i < count; ++i) {
        const std::uint64_t parse_result_ptr = *reinterpret_cast<const std::uint64_t*>(slot_parse_result_ptr);
        if (parse_result_ptr != 0u)
            mark_usage(parse_result_ptr, in_use);
        slot_parse_result_ptr += kFrameStride_channel_slot;
    }
}

void frame_mark_as_unused_106cd0_partial(
    std::uint64_t frame_ptr,
    void (*mark_usage)(std::uint64_t parse_result_ptr, std::uint32_t in_use)) {
    if (frame_ptr == 0u || mark_usage == nullptr)
        return;
    const std::uint32_t count = *reinterpret_cast<const std::uint32_t*>(frame_ptr + kFrameOff_channel_count_dword);
    // IDA 0x106CD0: v3 = frame+72 (first parse_result ptr), stride 32 bytes per slot.
    std::uintptr_t slot_parse_result_ptr = frame_ptr + kFrameOff_channel_slot_base + kFrameChSlotOff_channel_ptr_qword;
    for (std::uint32_t i = 0; i < count; ++i) {
        const std::uint64_t parse_result_ptr =
            *reinterpret_cast<const std::uint64_t*>(slot_parse_result_ptr);
        if (parse_result_ptr != 0u)
            mark_usage(parse_result_ptr, 0u);
        slot_parse_result_ptr += kFrameStride_channel_slot;
    }
}

void frame_mark_as_unused_106cd0_default_partial(std::uint64_t frame_ptr) {
    frame_mark_as_unused_106cd0_partial(frame_ptr, channel_parse_result_mark_usage_106d07_partial);
}

std::int64_t parser_frame_mark_as_unused_cb_partial(std::uint64_t frame_ptr) {
    frame_mark_as_unused_106cd0_default_partial(frame_ptr);
    return 0;
}

bool output_generator_plan_is_consistent_1024a9(const OutputGeneratorSegmentPlan& plan) {
    if (plan.segment_count != plan.segments.size())
        return false;
    if (plan.segment_count != 0 && plan.delay_line_buffer == 0)
        return false;
    return true;
}

struct NativeFrameDequeView530240 {
    std::uint64_t head = 0;
    std::uint64_t count = 0;
    std::uint64_t storage = 0;
    std::uint32_t capacity = 0;
};

bool read_native_frame_deque_530240(std::uint64_t frame_deque_ptr, NativeFrameDequeView530240& out) {
    if (frame_deque_ptr == 0u)
        return false;
    const auto* raw = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(frame_deque_ptr));
    NativeFrameDequeView530240 view{};
    view.head = *reinterpret_cast<const std::uint64_t*>(raw + 0u);
    view.count = *reinterpret_cast<const std::uint64_t*>(raw + 8u);
    view.storage = *reinterpret_cast<const std::uint64_t*>(raw + 16u);
    view.capacity = *reinterpret_cast<const std::uint32_t*>(raw + 24u);
    // IDA FrameDeque_t_construct @ 0x530240: only +0..+27 are defined; +28 is not cleared in native.
    if (view.storage == 0u || view.capacity == 0u || view.capacity >= 0x100000u)
        return false;
    out = view;
    return true;
}

std::uint64_t native_frame_deque_slot_index_530240(std::uint64_t logical_index, std::uint32_t capacity) {
    return (logical_index >> 32u) != 0u
        ? logical_index % capacity
        : static_cast<std::uint32_t>(logical_index) % capacity;
}

std::uint64_t native_frame_deque_frame_ptr_530240(const NativeFrameDequeView530240& view, std::uint64_t slot_index) {
    return view.storage + 336ull * slot_index;
}

std::uint64_t frame_deque_find_first_with_end_after_13d570_partial(
    std::uint64_t frame_deque_ptr,
    std::uint64_t sample_pos) {
    NativeFrameDequeView530240 native{};
    if (read_native_frame_deque_530240(frame_deque_ptr, native)) {
        const std::int32_t sample_pos32 = static_cast<std::int32_t>(sample_pos);
        for (std::uint64_t i = 0; i < native.count; ++i) {
            const std::uint64_t idx =
                native_frame_deque_slot_index_530240(native.head + i, native.capacity);
            const std::uint64_t frame_ptr = native_frame_deque_frame_ptr_530240(native, idx);
            const std::int32_t frame_end32 = *reinterpret_cast<const std::int32_t*>(frame_ptr + 8u);
            if ((sample_pos32 - frame_end32) < 0)
                return frame_ptr;
        }
        return 0u;
    }
    auto* dq = reinterpret_cast<FrameDequeState13d570*>(frame_deque_ptr);
    if (!dq || dq->count == 0u || dq->capacity == 0u)
        return 0u;
    const std::int32_t sample_pos32 = static_cast<std::int32_t>(sample_pos);
    for (std::uint64_t i = 0; i < dq->count; ++i) {
        const std::uint32_t idx = static_cast<std::uint32_t>((dq->head + i) % dq->capacity);
        const std::uint64_t frame_ptr = dq->frame_ptrs[idx];
        const std::int32_t frame_end32 = *reinterpret_cast<const std::int32_t*>(frame_ptr + 8u);
        if ((sample_pos32 - frame_end32) < 0)
            return frame_ptr;
    }
    return 0u;
}

std::int64_t frame_deque_push_back_13d5d0_partial(
    std::uint64_t frame_deque_ptr,
    std::uint64_t frame_ptr,
    std::uint64_t copy_bytes) {
    NativeFrameDequeView530240 native{};
    if (read_native_frame_deque_530240(frame_deque_ptr, native)) {
        if (!frame_ptr)
            return 0;
        auto* raw = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(frame_deque_ptr));
        const std::uint64_t idx =
            native_frame_deque_slot_index_530240(native.head + native.count, native.capacity);
        *reinterpret_cast<std::uint64_t*>(raw + 8u) = native.count + 1u;
        std::memcpy(
            reinterpret_cast<void*>(static_cast<std::uintptr_t>(
                native_frame_deque_frame_ptr_530240(native, idx))),
            reinterpret_cast<const void*>(static_cast<std::uintptr_t>(frame_ptr)),
            static_cast<std::size_t>(std::min<std::uint64_t>(copy_bytes, 336u)));
        return 1;
    }
    auto* dq = reinterpret_cast<FrameDequeState13d570*>(frame_deque_ptr);
    if (!dq || !frame_ptr || dq->capacity == 0u)
        return 0;
    const std::uint32_t tail = static_cast<std::uint32_t>((dq->head + dq->count) % dq->capacity);
    const std::uint64_t dst_ptr = dq->slot_ptrs[tail];
    if (!dst_ptr)
        return 0;
    dq->count += 1u;
    std::memcpy(
        reinterpret_cast<void*>(dst_ptr),
        reinterpret_cast<const void*>(frame_ptr),
        static_cast<std::size_t>(copy_bytes));
    dq->frame_ptrs[tail] = dst_ptr;
    return 1;
}

std::uint64_t frame_deque_pop_front_13d670_partial(
    std::uint64_t frame_deque_ptr,
    void (*frame_mark_as_unused)(std::uint64_t frame_ptr)) {
    NativeFrameDequeView530240 native{};
    if (read_native_frame_deque_530240(frame_deque_ptr, native)) {
        if (native.count == 0u)
            return 0u;
        auto* raw = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(frame_deque_ptr));
        const std::uint64_t idx = native_frame_deque_slot_index_530240(native.head, native.capacity);
        const std::uint64_t frame_ptr = native_frame_deque_frame_ptr_530240(native, idx);
        if (frame_mark_as_unused)
            frame_mark_as_unused(frame_ptr);
        const std::uint64_t next_head = native.head + 1u;
        *reinterpret_cast<std::uint64_t*>(raw + 0u) =
            native_frame_deque_slot_index_530240(next_head, native.capacity);
        *reinterpret_cast<std::uint64_t*>(raw + 8u) = native.count - 1u;
        return (next_head >> 32u) != 0u
            ? next_head / native.capacity
            : static_cast<std::uint32_t>(next_head) / native.capacity;
    }
    auto* dq = reinterpret_cast<FrameDequeState13d570*>(frame_deque_ptr);
    if (!dq || dq->count == 0u || dq->capacity == 0u)
        return 0u;
    const std::uint32_t idx = static_cast<std::uint32_t>(dq->head % dq->capacity);
    const std::uint64_t frame_ptr = dq->frame_ptrs[idx];
    if (frame_mark_as_unused && frame_ptr != 0u)
        frame_mark_as_unused(frame_ptr);
    const std::uint64_t next_head = dq->head + 1u;
    dq->head = next_head % dq->capacity;
    dq->count -= 1u;
    return next_head / dq->capacity;
}

std::uint64_t frame_deque_pop_front_keep_frame_partial(std::uint64_t frame_deque_ptr) {
    NativeFrameDequeView530240 native{};
    if (read_native_frame_deque_530240(frame_deque_ptr, native)) {
        if (native.count == 0u)
            return 0u;
        auto* raw = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(frame_deque_ptr));
        const std::uint64_t next_head = native.head + 1u;
        *reinterpret_cast<std::uint64_t*>(raw + 0u) =
            native_frame_deque_slot_index_530240(next_head, native.capacity);
        *reinterpret_cast<std::uint64_t*>(raw + 8u) = native.count - 1u;
        return (next_head >> 32u) != 0u
            ? next_head / native.capacity
            : static_cast<std::uint32_t>(next_head) / native.capacity;
    }
    auto* dq = reinterpret_cast<FrameDequeState13d570*>(frame_deque_ptr);
    if (!dq || dq->count == 0u || dq->capacity == 0u)
        return 0u;
    const std::uint64_t next_head = dq->head + 1u;
    dq->head = next_head % dq->capacity;
    dq->count -= 1u;
    return next_head / dq->capacity;
}

std::uint64_t frame_deque_pop_back_5303f0_partial(
    std::uint64_t frame_deque_ptr,
    void (*frame_mark_as_unused)(std::uint64_t frame_ptr)) {
    NativeFrameDequeView530240 native{};
    if (read_native_frame_deque_530240(frame_deque_ptr, native)) {
        if (native.count == 0u)
            return 0u;
        auto* raw = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(frame_deque_ptr));
        const std::uint64_t idx =
            native_frame_deque_slot_index_530240(native.head + native.count - 1u, native.capacity);
        const std::uint64_t frame_ptr = native_frame_deque_frame_ptr_530240(native, idx);
        if (frame_mark_as_unused)
            frame_mark_as_unused(frame_ptr);
        *reinterpret_cast<std::uint64_t*>(raw + 8u) = native.count - 1u;
        return 0u;
    }
    auto* dq = reinterpret_cast<FrameDequeState13d570*>(frame_deque_ptr);
    if (!dq || dq->count == 0u || dq->capacity == 0u)
        return 0u;
    const std::uint32_t idx = static_cast<std::uint32_t>((dq->head + dq->count - 1u) % dq->capacity);
    const std::uint64_t frame_ptr = dq->frame_ptrs[idx];
    if (frame_mark_as_unused && frame_ptr != 0u)
        frame_mark_as_unused(frame_ptr);
    dq->count -= 1u;
    return 0u;
}

std::uint64_t frame_deque_count_530240_partial(std::uint64_t frame_deque_ptr) {
    NativeFrameDequeView530240 native{};
    if (read_native_frame_deque_530240(frame_deque_ptr, native))
        return native.count;
    const auto* dq = reinterpret_cast<const FrameDequeState13d570*>(frame_deque_ptr);
    if (!dq || dq->capacity == 0u)
        return 0u;
    return dq->count;
}

std::uint64_t frame_deque_frame_at_530240_partial(std::uint64_t frame_deque_ptr, std::uint64_t logical_index) {
    NativeFrameDequeView530240 native{};
    if (read_native_frame_deque_530240(frame_deque_ptr, native)) {
        if (logical_index >= native.count)
            return 0u;
        const std::uint64_t idx =
            native_frame_deque_slot_index_530240(native.head + logical_index, native.capacity);
        return native_frame_deque_frame_ptr_530240(native, idx);
    }
    const auto* dq = reinterpret_cast<const FrameDequeState13d570*>(frame_deque_ptr);
    if (!dq || dq->capacity == 0u || logical_index >= dq->count)
        return 0u;
    const std::uint32_t idx = static_cast<std::uint32_t>((dq->head + logical_index) % dq->capacity);
    return dq->frame_ptrs[idx];
}

std::uint64_t frame_deque_back_530240_partial(std::uint64_t frame_deque_ptr, std::uint64_t* storage_index_out) {
    if (storage_index_out)
        *storage_index_out = 0u;
    NativeFrameDequeView530240 native{};
    if (read_native_frame_deque_530240(frame_deque_ptr, native)) {
        if (native.count == 0u)
            return 0u;
        const std::uint64_t idx =
            native_frame_deque_slot_index_530240(native.head + native.count - 1u, native.capacity);
        if (storage_index_out)
            *storage_index_out = idx;
        return native_frame_deque_frame_ptr_530240(native, idx);
    }
    const auto* dq = reinterpret_cast<const FrameDequeState13d570*>(frame_deque_ptr);
    if (!dq || dq->capacity == 0u || dq->count == 0u)
        return 0u;
    const std::uint64_t idx = (dq->head + dq->count - 1u) % dq->capacity;
    if (storage_index_out)
        *storage_index_out = idx;
    return dq->frame_ptrs[static_cast<std::uint32_t>(idx)];
}

void parser_rebind_frame_parse_results_103610_partial(
    std::uint64_t frame_ptr,
    const ParserRebindContext103610* ctx) {
    if (frame_ptr == 0u || ctx == nullptr || ctx->parse_result_pool_base == 0u)
        return;
    const std::uint32_t channel_count =
        *reinterpret_cast<const std::uint32_t*>(frame_ptr + 44u);
    for (std::uint32_t slot = 0; slot < channel_count; ++slot) {
        const std::uint64_t parse_result_ptr = parse_result_pool_get_new_107220_partial(ctx->parse_result_pool_base);
        const std::uint64_t frame_slot_ptr =
            frame_ptr + 48u + static_cast<std::uint64_t>(slot) * 32u;
        *reinterpret_cast<std::uint64_t*>(frame_slot_ptr + 24u) = parse_result_ptr;

        if (slot >= ctx->channel_parser_count)
            continue;
        const std::uint64_t parser_base = reinterpret_cast<std::uint64_t>(
            ctx->channel_parser_base + static_cast<std::size_t>(slot) * 64u);
        (void)channel_parser_construct_104210_partial(
            parser_base,
            parse_result_ptr,
            frame_ptr + 28u,
            frame_ptr + 56u + static_cast<std::uint64_t>(slot) * 32u);
    }
}

void parser_refresh_payload_partial(
    const CodecV3IoBufferDescEb5a0* input_desc,
    const ParserPayloadRefreshContext* ctx) {
    if (input_desc == nullptr || ctx == nullptr || ctx->frame_deque_ptr == 0u || ctx->block_size == 0u)
        return;
    if (frame_deque_count_530240_partial(ctx->frame_deque_ptr) == 0u)
        return;
    const std::uint64_t active_frame_ptr = frame_deque_frame_at_530240_partial(ctx->frame_deque_ptr, 0u);
    if (active_frame_ptr == 0u)
        return;

    const std::uint32_t slot_count = *reinterpret_cast<const std::uint32_t*>(active_frame_ptr + 44u);
    for (std::uint32_t slot = 0; slot < slot_count; ++slot) {
        const std::uint64_t frame_slot_ptr =
            active_frame_ptr + 48u + static_cast<std::uint64_t>(slot) * 32u;
        if (*reinterpret_cast<const std::uint32_t*>(frame_slot_ptr + 4u) == 0u)
            continue;

        const std::uint32_t channel_index =
            *reinterpret_cast<const std::uint32_t*>(frame_slot_ptr + 0u);
        if (channel_index >= ctx->input_channel_limit || channel_index >= ctx->channel_words_count
            || channel_index >= ctx->channel_ctx_count) {
            continue;
        }
        const std::uint64_t ch_ptr = input_desc->channel_ptr[channel_index];
        if (ch_ptr == 0u)
            continue;

        const std::uint64_t frame_channel_ptr =
            *reinterpret_cast<const std::uint64_t*>(frame_slot_ptr + 24u);
        if (frame_channel_ptr == 0u)
            continue;

        if (ctx->output_generator_base != 0u) {
            auto* gr_state = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(ctx->output_generator_base))
                + 0u + static_cast<std::size_t>(slot) * 40u;
            auto* words = reinterpret_cast<std::uint32_t*>(
                static_cast<std::uintptr_t>(frame_channel_ptr + kFrameChannelOff_stream_words));
            *reinterpret_cast<std::uint64_t*>(gr_state + 0u) =
                reinterpret_cast<std::uint64_t>(words + 1u);
            *reinterpret_cast<std::uint64_t*>(gr_state + 8u) =
                frame_channel_ptr + kFrameChannelOff_ctx_words;
            *reinterpret_cast<std::uint32_t*>(gr_state + 16u) = 31u;
            *reinterpret_cast<std::uint32_t*>(gr_state + 20u) = words[0];
            *reinterpret_cast<std::uint32_t*>(gr_state + 36u) = 0u;
        }
    }
}

std::int64_t parser_ready_frame_push_copy_partial(
    std::uint64_t frame_deque_ptr,
    std::uint64_t frame_ptr,
    std::uint64_t copy_bytes,
    const ParserReadyFrameCopyContext* copy_ctx) {
    if (frame_deque_push_back_13d5d0_partial(frame_deque_ptr, frame_ptr, copy_bytes) == 0)
        return 0;
    if (copy_ctx == nullptr || copy_ctx->ready_parse_result_base == nullptr
        || copy_ctx->ready_parse_result_size == 0u || copy_ctx->copied_slot_capacity == 0u
        || copy_ctx->parse_result_bytes == 0u) {
        return 1;
    }

    std::uint64_t ready_tail = 0u;
    const std::uint64_t ready_frame_ptr = frame_deque_back_530240_partial(frame_deque_ptr, &ready_tail);
    if (ready_frame_ptr == 0u)
        return 1;
    const std::uint32_t ch_count_raw =
        *reinterpret_cast<const std::uint32_t*>(ready_frame_ptr + 44u);
    const std::uint32_t ch_count =
        std::min<std::uint32_t>(ch_count_raw, copy_ctx->copied_slot_capacity);
    *reinterpret_cast<std::uint32_t*>(ready_frame_ptr + 44u) = ch_count;

    const std::size_t ready_slot_base =
        static_cast<std::size_t>(ready_tail) * static_cast<std::size_t>(copy_ctx->copied_slot_capacity);
    for (std::uint32_t si = 0; si < ch_count; ++si) {
        const std::uint64_t slot_ptr =
            ready_frame_ptr + 48u + static_cast<std::uint64_t>(si) * 32u;
        const std::uint64_t src_pr =
            *reinterpret_cast<const std::uint64_t*>(slot_ptr + 24u);
        if (src_pr == 0u)
            continue;
        const std::size_t dst_idx = ready_slot_base + static_cast<std::size_t>(si);
        const std::size_t dst_off = dst_idx * copy_ctx->parse_result_bytes;
        if (dst_off + copy_ctx->parse_result_bytes > copy_ctx->ready_parse_result_size)
            continue;
        std::uint8_t* dst_pr = copy_ctx->ready_parse_result_base + dst_off;
        std::memcpy(dst_pr, reinterpret_cast<const void*>(src_pr), copy_ctx->parse_result_bytes);
        *reinterpret_cast<std::uint64_t*>(slot_ptr + 24u) =
            reinterpret_cast<std::uint64_t>(dst_pr);
    }
    if (std::getenv("AURO3D_DEBUG_FRAMES") != nullptr) {
        static std::atomic<unsigned> debug_count{0};
        const unsigned n = debug_count.fetch_add(1);
        if (n < 8) {
            std::fprintf(
                stderr,
                "ready_frame[%u] start=%u end=%u flags=0x%x slots=%u\n",
                n,
                *reinterpret_cast<const std::uint32_t*>(ready_frame_ptr + 0u),
                *reinterpret_cast<const std::uint32_t*>(ready_frame_ptr + 8u),
                *reinterpret_cast<const std::uint32_t*>(ready_frame_ptr + 24u),
                ch_count);
            for (std::uint32_t si = 0; si < ch_count; ++si) {
                const std::uint64_t slot_ptr =
                    ready_frame_ptr + 48u + static_cast<std::uint64_t>(si) * 32u;
                const std::uint64_t pr =
                    *reinterpret_cast<const std::uint64_t*>(slot_ptr + 24u);
                if (pr == 0u)
                    continue;
                std::fprintf(
                    stderr,
                    "  slot=%u ch=%u active=%u mode=%u pred=%u,%u,%u q=%u cnt=%llu/%llu\n",
                    si,
                    *reinterpret_cast<const std::uint32_t*>(slot_ptr + 0u),
                    *reinterpret_cast<const std::uint32_t*>(slot_ptr + 4u),
                    *reinterpret_cast<const std::uint32_t*>(pr + kFrameChannelOff_ex_mode),
                    *reinterpret_cast<const std::uint32_t*>(pr + kFrameChannelOff_pred_src0_idx),
                    *reinterpret_cast<const std::uint32_t*>(pr + kFrameChannelOff_pred_src1_idx),
                    *reinterpret_cast<const std::uint32_t*>(pr + kFrameChannelOff_pred_src2_idx),
                    *reinterpret_cast<const std::uint32_t*>(pr + kFrameChannelOff_ex_quant_shift),
                    static_cast<unsigned long long>(
                        *reinterpret_cast<const std::uint64_t*>(pr + kFrameChannelOff_ctx_count_qword)),
                    static_cast<unsigned long long>(
                        *reinterpret_cast<const std::uint64_t*>(pr + kFrameChannelOff_stream_count_qword)));
            }
        }
    }
    return 1;
}

thread_local ParserReadyFrameCopyContext g_parser_ready_frame_copy_ctx{};
thread_local bool g_parser_ready_frame_copy_active = false;

std::int64_t parser_frame_deque_push_back_with_optional_copy_530290(
    std::uint64_t frame_deque_ptr,
    std::uint64_t frame_ptr) {
    if (g_parser_ready_frame_copy_active
        && g_parser_ready_frame_copy_ctx.ready_parse_result_base != nullptr
        && g_parser_ready_frame_copy_ctx.ready_parse_result_size != 0u
        && g_parser_ready_frame_copy_ctx.copied_slot_capacity != 0u
        && g_parser_ready_frame_copy_ctx.parse_result_bytes != 0u) {
        return parser_ready_frame_push_copy_partial(
            frame_deque_ptr,
            frame_ptr,
            kCodecV3FrameDequeSlotCopyBytes,
            &g_parser_ready_frame_copy_ctx);
    }
    return frame_deque_push_back_13d5d0_partial(
        frame_deque_ptr, frame_ptr, kCodecV3FrameDequeSlotCopyBytes);
}

void parser_state_sink_notify_partial(ParserStateSinkContext* sink, std::uint32_t state) {
    if (!sink)
        return;
    if (sink->state_ptr)
        *sink->state_ptr = state;
    if (sink->dispatch)
        codec_v3_content_callback_eb870(sink->dispatch, state != 0u ? 1 : 0);
}

std::int64_t decoder_run_parser_1034e0_partial(DecoderParserRunContext1034e0* ctx) {
    const std::size_t parser_slots_bytes = ctx->parser_slots_size;
    const std::size_t parser_blob_size = std::max<std::size_t>(56u + parser_slots_bytes, 652u);
    std::vector<std::uint8_t> parser_blob(parser_blob_size, 0u);
    auto* parser_q = reinterpret_cast<std::uint64_t*>(parser_blob.data());
    parser_q[1] = ctx->timeline_cursor;
    parser_q[2] = ctx->delay_line_ptr;
    parser_q[3] = ctx->block_size;
    parser_q[4] = ctx->frame_deque_ptr;
    parser_q[5] = ctx->ready_frame_deque_ptr;
    parser_q[6] = ctx->parse_result_pool_ptr;
    if (parser_slots_bytes != 0u && ctx->parser_slots_base != nullptr) {
        std::memcpy(parser_blob.data() + 56u, ctx->parser_slots_base, parser_slots_bytes);
    }
    *reinterpret_cast<std::uint32_t*>(parser_blob.data() + 648u) = *ctx->parser_state_inout;

    const std::int64_t rc = codec_v3_parser_process_1034e0(parser_blob.data(), &ctx->runtime_fns);

    if (parser_slots_bytes != 0u && ctx->parser_slots_base != nullptr) {
        std::memcpy(ctx->parser_slots_base, parser_blob.data() + 56u, parser_slots_bytes);
    }
    *ctx->parser_state_inout = *reinterpret_cast<const std::uint32_t*>(parser_blob.data() + 648u);
    ctx->timeline_cursor = parser_q[1];
    return rc;
}

std::int64_t decoder_run_parser_stage_1034e0_partial(DecoderParserStageContext1034e0* ctx) {
    auto update_parser_state = [ctx](std::uint32_t next_state) {
        if (ctx == nullptr || ctx->parser_state_ptr == nullptr)
            return;
        if (*ctx->parser_state_ptr == next_state)
            return;
        *ctx->parser_state_ptr = next_state;
        codec_v3_content_callback_eb870(ctx->dispatch, (next_state != 0u) ? 1 : 0);
    };

    if (ctx == nullptr || ctx->parser_state_ptr == nullptr)
        return 0;
    if (ctx->frame_deque_base == nullptr) {
        update_parser_state(0u);
        return 0;
    }
    if (ctx->ready_frame_deque_base == nullptr || ctx->parse_result_pool_base == nullptr) {
        update_parser_state(0u);
        return 0;
    }

    const auto* og = ctx->output_generator_base;
    const std::uint64_t timeline_cursor0 =
        ctx->timeline_cursor_ptr != nullptr
            ? *ctx->timeline_cursor_ptr
            : ((og != nullptr) ? *reinterpret_cast<const std::uint64_t*>(og + kOgOff_timeline_cursor) : 0u);

    if (std::getenv("AURO3D_DEBUG_FRAMES") != nullptr) {
        static std::atomic<unsigned> debug_count{0};
        const unsigned n = debug_count.fetch_add(1);
        if (n < 16) {
            std::fprintf(
                stderr,
                "parser_stage[%u] cursor=%llu block=%llu dq_count=%llu dq_head=%llu ready_count=%llu dl_abs=%llu dl_slot=%u state=%u\n",
                n,
                static_cast<unsigned long long>(timeline_cursor0),
                static_cast<unsigned long long>(ctx->block_size),
                static_cast<unsigned long long>(frame_deque_count_530240_partial(
                    reinterpret_cast<std::uint64_t>(ctx->frame_deque_base))),
                static_cast<unsigned long long>(*reinterpret_cast<const std::uint64_t*>(ctx->frame_deque_base)),
                static_cast<unsigned long long>(frame_deque_count_530240_partial(
                    reinterpret_cast<std::uint64_t>(ctx->ready_frame_deque_base))),
                ctx->delay_line ? static_cast<unsigned long long>(ctx->delay_line->absolute_cursor) : 0ull,
                ctx->delay_line ? ctx->delay_line->write_slot_index : 0u,
                ctx->parser_state_ptr ? *ctx->parser_state_ptr : 0u);
        }
    }

    ParserStateSinkContext state_sink{};
    state_sink.dispatch = ctx->dispatch;
    state_sink.state_ptr = ctx->parser_state_ptr;

    auto pfn = codec_v3_default_parser_runtime_1034e0();
    if (ctx->runtime_fns.delay_line_get_buffer)
        pfn.delay_line_get_buffer = ctx->runtime_fns.delay_line_get_buffer;
    if (ctx->runtime_fns.frame_deque_find_first_with_end_after)
        pfn.frame_deque_find_first_with_end_after = ctx->runtime_fns.frame_deque_find_first_with_end_after;
    if (ctx->runtime_fns.frame_deque_push_back) {
        pfn.frame_deque_push_back = ctx->runtime_fns.frame_deque_push_back;
    } else if (ctx->ready_parse_result_base != nullptr && ctx->ready_parse_result_size != 0u) {
        g_parser_ready_frame_copy_ctx.ready_parse_result_base = ctx->ready_parse_result_base;
        g_parser_ready_frame_copy_ctx.ready_parse_result_size = ctx->ready_parse_result_size;
        g_parser_ready_frame_copy_ctx.copied_slot_capacity = kCodecV3FrameDequeCopiedSlotCapacity;
        g_parser_ready_frame_copy_ctx.parse_result_bytes = kParseResultStrideBytes;
        g_parser_ready_frame_copy_active = true;
        pfn.frame_deque_push_back = parser_frame_deque_push_back_with_optional_copy_530290;
    }
    if (ctx->runtime_fns.frame_deque_pop_front)
        pfn.frame_deque_pop_front = ctx->runtime_fns.frame_deque_pop_front;
    if (ctx->runtime_fns.frame_mark_as_unused)
        pfn.frame_mark_as_unused = ctx->runtime_fns.frame_mark_as_unused;
    pfn.content_state_callback = [](std::uint64_t sink_ctx, std::uint32_t state) {
        auto* sink = reinterpret_cast<ParserStateSinkContext*>(sink_ctx);
        ::auro3deng::parser_state_sink_notify_partial(sink, state);
    };
    pfn.content_state_ctx = reinterpret_cast<std::uint64_t>(&state_sink);

    DecoderParserRunContext1034e0 parser_ctx{};
    parser_ctx.timeline_cursor = timeline_cursor0;
    parser_ctx.delay_line_ptr = reinterpret_cast<std::uint64_t>(ctx->delay_line);
    parser_ctx.block_size = ctx->block_size;
    parser_ctx.frame_deque_ptr = reinterpret_cast<std::uint64_t>(ctx->frame_deque_base);
    parser_ctx.ready_frame_deque_ptr = reinterpret_cast<std::uint64_t>(ctx->ready_frame_deque_base);
    parser_ctx.parse_result_pool_ptr = reinterpret_cast<std::uint64_t>(ctx->parse_result_pool_base);
    parser_ctx.parser_slots_base = ctx->parser_slots_base;
    parser_ctx.parser_slots_size = ctx->parser_slots_size;
    parser_ctx.parser_state_inout = ctx->parser_state_ptr;
    parser_ctx.runtime_fns = pfn;

    const std::int64_t rc = ::auro3deng::decoder_run_parser_1034e0_partial(&parser_ctx);
    g_parser_ready_frame_copy_active = false;
    if (ctx->timeline_cursor_ptr)
        *ctx->timeline_cursor_ptr = parser_ctx.timeline_cursor;
    update_parser_state(*ctx->parser_state_ptr);
    return rc;
}

std::int64_t decoder_run_output_stage_1024a9_partial(
    const DecoderOutputStageContext1024a9* ctx,
    const void* runtime_fns) {
    if (ctx->ready_frame_deque_base == nullptr)
        return 0;

    DecoderOutputGeneratorRunContext1024a9 run_ctx{};
    run_ctx.output_generator_base = ctx->output_generator_base;
    run_ctx.output_channels_table = ctx->output_table_base;
    run_ctx.output_channels_table_size = ctx->output_table_size;
    run_ctx.output_channel_ptrs_27 = ctx->output_channel_ptrs_27;
    run_ctx.input_channel_ptrs_27 = ctx->input_channel_ptrs_27;
    run_ctx.input_mask = ctx->input_mask;
    run_ctx.delay_line_ptr = reinterpret_cast<std::uint64_t>(ctx->delay_line);
    run_ctx.frame_deque_ptr = reinterpret_cast<std::uint64_t>(ctx->ready_frame_deque_base);
    run_ctx.total_samples = ctx->total_samples;
    run_ctx.produced_output_mask = ctx->produced_output_mask;
    run_ctx.runtime_fns = runtime_fns
        ? OutputGeneratorRuntimeFns1024a9{}
        : codec_v3_default_output_runtime_1024a9();
    if (runtime_fns) {
        const auto& ext = *reinterpret_cast<const OutputGeneratorRuntimeFns1024a9*>(runtime_fns);
        if (ext.delay_line_get_channel) run_ctx.runtime_fns.delay_line_get_channel = ext.delay_line_get_channel;
        if (ext.delay_line_get_buffer) run_ctx.runtime_fns.delay_line_get_buffer = ext.delay_line_get_buffer;
        if (ext.frame_deque_find_first_with_end_after)
            run_ctx.runtime_fns.frame_deque_find_first_with_end_after = ext.frame_deque_find_first_with_end_after;
        if (ext.pre_segments_callback) {
            run_ctx.runtime_fns.pre_segments_callback = ext.pre_segments_callback;
            run_ctx.runtime_fns.pre_segments_ctx = ext.pre_segments_ctx;
        }
        if (ext.metadata_update_callback) {
            run_ctx.runtime_fns.metadata_update_callback = ext.metadata_update_callback;
            run_ctx.runtime_fns.metadata_update_ctx = ext.metadata_update_ctx;
        }
        if (ext.decode_channel_segment) run_ctx.runtime_fns.decode_channel_segment = ext.decode_channel_segment;
        if (ext.golombrice_get_errors) run_ctx.runtime_fns.golombrice_get_errors = ext.golombrice_get_errors;
        if (ext.extrapolate_process) run_ctx.runtime_fns.extrapolate_process = ext.extrapolate_process;
        if (ext.golombrice_initialize) run_ctx.runtime_fns.golombrice_initialize = ext.golombrice_initialize;
        if (ext.extrapolate_initialize) run_ctx.runtime_fns.extrapolate_initialize = ext.extrapolate_initialize;
        if (ext.frame_mark_as_unused) run_ctx.runtime_fns.frame_mark_as_unused = ext.frame_mark_as_unused;
        if (ext.frame_deque_pop_front) run_ctx.runtime_fns.frame_deque_pop_front = ext.frame_deque_pop_front;
    }
    return decoder_run_output_generator_1024a9_partial(&run_ctx);
}

void decoder_init_fake_frame_channels_106ba0_partial(
    const DecoderInitFakeFrameChannelsContext106ba0* ctx) {
    if (!ctx || !ctx->fake_frame || !ctx->fake_frame_next || !ctx->fake_frame_channels_base
        || !ctx->fake_frame_channel_words_base || !ctx->fake_frame_channel_ctx_base)
        return;

    std::uint32_t slot = 0;
    const std::uint32_t input_mask = ctx->input_mask & kCodecV3ChannelMask;
    frame_construct_106ba0_partial(
        reinterpret_cast<std::uint64_t>(ctx->fake_frame),
        0u,
        ctx->block_size,
        input_mask,
        input_mask);
    frame_construct_106ba0_partial(
        reinterpret_cast<std::uint64_t>(ctx->fake_frame_next),
        0u,
        ctx->block_size,
        input_mask,
        input_mask);

    for (std::uint32_t ch = 0; ch < kCodecV3ChannelCount; ++ch) {
        if (((input_mask >> ch) & 1u) == 0)
            continue;
        if (slot >= 9u)
            break;
        auto* frame_slot = ctx->fake_frame + 48u + static_cast<std::size_t>(slot) * 32u;
        auto* frame_channel = ctx->fake_frame_channels_base
            + static_cast<std::size_t>(slot) * ctx->fake_frame_channel_bytes;
        if (ctx->parse_result_pool_base != 0u) {
            const std::uint64_t from_pool = parse_result_pool_get_new_107220_partial(ctx->parse_result_pool_base);
            if (from_pool != 0u)
                frame_channel = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(from_pool));
        }
        std::memset(frame_channel, 0, ctx->fake_frame_channel_bytes);
        *reinterpret_cast<std::uint32_t*>(frame_slot + 0u) = ch;
        const bool is_decode_probe_slot = (slot < ctx->active_decode_probe_slots);
        *reinterpret_cast<std::uint32_t*>(frame_slot + 4u) = is_decode_probe_slot ? 1u : 0u;
        *reinterpret_cast<std::uint64_t*>(frame_slot + 24u) = reinterpret_cast<std::uint64_t>(frame_channel);
        if (is_decode_probe_slot && ctx->output_generator_base != nullptr) {
            auto* gr_state = ctx->output_generator_base + 0u + static_cast<std::size_t>(slot) * 40u;
            auto* ex_state = ctx->output_generator_base + 360u + static_cast<std::size_t>(slot) * 48u;
            auto* words = reinterpret_cast<std::uint32_t*>(
                reinterpret_cast<std::uint64_t>(frame_channel) + kFrameChannelOff_stream_words);
            *reinterpret_cast<std::uint64_t*>(gr_state + 0u) = reinterpret_cast<std::uint64_t>(words + 1);
            *reinterpret_cast<std::uint64_t*>(gr_state + 8u) =
                reinterpret_cast<std::uint64_t>(frame_channel) + kFrameChannelOff_ctx_words;
            *reinterpret_cast<std::uint32_t*>(gr_state + 16u) = 31u;
            *reinterpret_cast<std::uint32_t*>(gr_state + 20u) = words[0];
            *reinterpret_cast<std::uint32_t*>(gr_state + 36u) = 0u;
            *reinterpret_cast<std::uint64_t*>(ex_state + 0u) = 0u;
            *reinterpret_cast<std::uint32_t*>(ex_state + 20u) = 0u;
            *reinterpret_cast<std::uint64_t*>(ex_state + 40u) = reinterpret_cast<std::uint64_t>(frame_channel);
        }
        ++slot;
    }
    *reinterpret_cast<std::uint32_t*>(ctx->fake_frame + 44u) = slot;
    std::memcpy(ctx->fake_frame_next, ctx->fake_frame, 0x150u);
}

void decoder_init_frame_deques_13d5d0_partial(DecoderInitFrameDequesContext13d5d0* ctx) {
    if (!ctx || !ctx->frame_deque || !ctx->ready_frame_deque || !ctx->fake_frame || !ctx->ready_frame)
        return;

    auto* dq = ctx->frame_deque;
    auto* ready_dq = ctx->ready_frame_deque;

    dq->slot_ptrs[0] = reinterpret_cast<std::uint64_t>(ctx->fake_frame);
    dq->slot_ptrs[1] = reinterpret_cast<std::uint64_t>(ctx->fake_frame_next);
    dq->frame_ptrs[0] = 0u;
    dq->frame_ptrs[1] = 0u;
    dq->head = 0u;
    dq->capacity = ctx->deque_capacity;
    dq->count = 0u;
    dq->cursor = 0u;
    dq->frame_span = ctx->frame_span;
    dq->next_frame_start = 0u;

    ready_dq->slot_ptrs[0] = reinterpret_cast<std::uint64_t>(ctx->ready_frame);
    ready_dq->slot_ptrs[1] = reinterpret_cast<std::uint64_t>(ctx->ready_frame_next);
    ready_dq->frame_ptrs[0] = 0u;
    ready_dq->frame_ptrs[1] = 0u;
    ready_dq->head = 0u;
    ready_dq->capacity = ctx->deque_capacity;
    ready_dq->count = 0u;
    ready_dq->cursor = 0u;
    ready_dq->frame_span = ctx->frame_span;
    ready_dq->next_frame_start = 0u;

    (void)ctx;
}

std::int64_t decoder_run_dispatch_eb5a0_partial(DecoderDispatchRunContextEb5a0* ctx) {
    CodecV3IoBufferDescEb5a0 input_desc{};
    CodecV3IoBufferDescEb5a0 output_desc{};
    input_desc.total_samples = ctx->block_size;
    input_desc.sample_rate = ctx->sample_rate;
    input_desc.bits_per_sample = 24u;
    output_desc.total_samples = ctx->block_size;
    output_desc.sample_rate = ctx->sample_rate;
    output_desc.bits_per_sample = 24u;
    for (std::uint32_t ch = 0; ch < kCodecV3ChannelCount; ++ch) {
        input_desc.channel_ptr[ch] = ctx->input_channel_ptrs_27[ch];
        output_desc.channel_ptr[ch] = ctx->output_channel_ptrs_27[ch];
    }

    ctx->dispatch->format_word0 = (ctx->input_mask & kCodecV3ChannelMask);
    ctx->dispatch->sample_rate = ctx->sample_rate;
    ctx->dispatch->block_size = ctx->block_size;
    ctx->dispatch->required_output_mask = ctx->output_mask & kCodecV3ChannelMask;
    ctx->format_detector->blocks_per_call = ctx->block_size / 32u;

    CodecV3PartialRuntimeEb5a0 runtime{};
    runtime.dispatch = ctx->dispatch;
    runtime.format_detector = ctx->format_detector;
    runtime.sync_detector = ctx->sync_detector;
    runtime.delay_line = ctx->delay_line;
    runtime.set_layout = ctx->set_layout;
    runtime.process_block = ctx->process_block;
    runtime.sync_user = ctx->sync_user;

    std::uint32_t io_status = 0;
    return codec_v3_dispatch_eb5a0_partial(
        ctx->dispatch,
        &input_desc,
        &output_desc,
        &io_status,
        codec_v3_process_partial_eb5a0,
        &runtime);
}

std::int64_t decoder_run_step_101800_partial(DecoderStepRunContext101800* ctx) {
    const std::int64_t rc_dispatch = ::auro3deng::decoder_run_dispatch_eb5a0_partial(ctx->dispatch_ctx);
    if (rc_dispatch != 0)
        return rc_dispatch;

    if (ctx->parser_input_desc && ctx->payload_ctx)
        ::auro3deng::parser_refresh_payload_partial(ctx->parser_input_desc, ctx->payload_ctx);
    if (ctx->run_parser_stage)
        ctx->run_parser_stage(ctx->parser_user);
    if (ctx->dispatch_ctx && ctx->dispatch_ctx->output_channel_ptrs_27) {
        const std::uint32_t output_mask = ctx->dispatch_ctx->output_mask & kCodecV3ChannelMask;
        const std::uint64_t sample_count = ctx->dispatch_ctx->block_size;
        for (std::uint32_t ch = 0; ch < kCodecV3ChannelCount; ++ch) {
            if (((output_mask >> ch) & 1u) == 0)
                continue;
            const std::uint64_t dst = ctx->dispatch_ctx->output_channel_ptrs_27[ch];
            if (dst != 0u) {
                std::memset(
                    reinterpret_cast<void*>(static_cast<std::uintptr_t>(dst)),
                    0,
                    static_cast<std::size_t>(sample_count) * sizeof(std::int32_t));
            }
        }
    }
    if (ctx->run_output_stage) {
        (void)ctx->run_output_stage(ctx->output_user);
    }
    // DelayLine_advance is performed in codec_v3_process_partial_eb5a0 after write+FD.
    return 0;
}

void notify_codec_v3_state_change(CodecV3DispatchStateEb5a0* state, std::int64_t kind) {
    if (!state || !state->sink.notify)
        return;
    state->sink.notify(state->sink.user, kind);
}

CodecV3IoBufferDescEb5a0 prepare_codec_v3_desc_eb5a0(
    const CodecV3IoBufferDescEb5a0* src,
    std::uint32_t sample_rate) {
    CodecV3IoBufferDescEb5a0 out{};
    out.total_samples = 64u;
    out.sample_rate = sample_rate;
    out.bits_per_sample = 24u;
    if (src) {
        for (std::size_t i = 0; i < std::size(out.channel_ptr); ++i)
            out.channel_ptr[i] = src->channel_ptr[i];
    }
    return out;
}

bool codec_v3_has_required_channel_ptrs_101800(const CodecV3IoBufferDescEb5a0* desc, std::uint32_t mask) {
    if (!desc)
        return false;
    for (std::uint32_t bit = 0; bit < kCodecV3ChannelCount; ++bit) {
        if (((mask >> bit) & 1u) == 0)
            continue;
        if (desc->channel_ptr[bit] == 0)
            return false;
    }
    return true;
}

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

ProcessorIoExpectDa9ae0 load_expect_da9ae0_from_impl_partial(const std::uint8_t* impl_base) {
    ProcessorIoExpectDa9ae0 ex{};
    if (!impl_base)
        return ex;
    using auro_codec_v3_ida::kAuroDecoderImpl_expect_bytes_unit;
    using auro_codec_v3_ida::kAuroDecoderImpl_off_InputDesc;
    using auro_codec_v3_ida::kAuroDecoderImpl_off_OutputDesc;
    using auro_codec_v3_ida::kAuroDecoderImpl_off_RuntimeInputMask;
    using auro_codec_v3_ida::kAuroDecoderImpl_off_RuntimeOutputMask;
    const auto* in = reinterpret_cast<const ProcessorIOBufferDesc*>(impl_base + kAuroDecoderImpl_off_InputDesc);
    const auto* out = reinterpret_cast<const ProcessorIOBufferDesc*>(impl_base + kAuroDecoderImpl_off_OutputDesc);
    // IDA 0xD9AE0: bytes_unit is processor config (sample-count stride), NOT desc.total_size_bytes.
    // Host IO stores sample count in total_size_bytes; unit=1 → validate modulo 32.
    ex.in_layout = static_cast<std::int32_t>(in->layout_or_kind);
    ex.in_mask = static_cast<std::int32_t>(
        *reinterpret_cast<const std::uint32_t*>(impl_base + kAuroDecoderImpl_off_RuntimeInputMask));
    ex.in_bytes_unit = static_cast<std::int32_t>(kAuroDecoderImpl_expect_bytes_unit);
    ex.in_field4 = in->field_4;
    ex.in_field8_when_layout1 = in->field_8;
    ex.out_layout = static_cast<std::int32_t>(out->layout_or_kind);
    ex.out_mask = static_cast<std::int32_t>(
        *reinterpret_cast<const std::uint32_t*>(impl_base + kAuroDecoderImpl_off_RuntimeOutputMask));
    ex.out_bytes_unit = static_cast<std::int32_t>(kAuroDecoderImpl_expect_bytes_unit);
    ex.out_field4 = out->field_4;
    ex.out_field8_when_layout1 = out->field_8;
    return ex;
}

ProcessorIoExpectDa9ae0 load_expect_da9ae0_from_processor(const std::uint8_t* p) {
    ProcessorIoExpectDa9ae0 ex{};
    ex.in_layout = *reinterpret_cast<const std::int32_t*>(p + kProcOff_InputLayout12);
    ex.in_mask = *reinterpret_cast<const std::int32_t*>(p + kProcOff_InputChannelMask);
    ex.in_bytes_unit = *reinterpret_cast<const std::int32_t*>(p + kProcOff_InputBytesUnit);
    ex.in_field4 = *reinterpret_cast<const std::int32_t*>(p + kProcOff_InputExpectedField4);
    ex.in_field8_when_layout1 = *reinterpret_cast<const std::int32_t*>(p + kProcOff_InputAuxField8);
    ex.out_layout = *reinterpret_cast<const std::int32_t*>(p + kProcOff_OutputLayout12);
    ex.out_mask = *reinterpret_cast<const std::int32_t*>(p + kProcOff_OutputChannelMask);
    ex.out_bytes_unit = *reinterpret_cast<const std::int32_t*>(p + kProcOff_OutputBytesUnit);
    ex.out_field4 = *reinterpret_cast<const std::int32_t*>(p + kProcOff_OutputExpectedField4);
    ex.out_field8_when_layout1 = *reinterpret_cast<const std::int32_t*>(p + kProcOff_OutputAuxField8);
    return ex;
}

const char* ida_callchain_ru() {
    return "Цепочка (x64, IDA):\n"
           "  AuroDecoderImpl::Decode → vtable+168 → auro_a3deng_v3_Processor_process (0xD9AE0)\n"
           "  → auro_a3deng_v3_Analyser_process (0xED500): call [proc+256](proc)\n"
           "  → sub_ED070 (0xED070): call [proc+16](*(void**)(proc+8))\n"
           "  → sub_DA320 (0xDA320): jmp Controller_process_audio(proc+0x140); rsi/rdx — буферы\n"
           "  Controller_process_audio (0xDA870): при dword_25E6E0==0 вызов [ctrl+qword_25E6E8]:\n"
           "    sub_DA6E0 (0xDA6E0) или sub_DA700 (0xDA700) → sub_DB290 (0xDB290, scratch ptr+size)\n"
           "  auro_a3deng_v3_Controller_update_dynamic_parameter @ 0xDAD10 → Decoder_update @ 0xEA200 →\n"
           "    при смене формата sub_EB420 → codec Decoder_t_construct + callbacks.\n"
           "  sub_DB290: Decoder_process / Manager_configure / Manager_process_audio (0xDDD40) / копирование в a3.\n"
           "  Полный перенос sub_DB290+ — отдельный объём (мегабайты состояния и таблиц).\n"
           "\n"
           "Codec v3 (libauro3d.so, ida-pro-mcp):\n"
           "  auro_codec_v3_decoder_CRC_t_init @ 0x106F00 — SSE-таблица 512 байт → unk_41ACF0, флаг byte_41ACE0.\n"
           "  auro_codec_v3_Decoder_t_construct @ 0x101760 вызывает: CRC_t_init → channel_Extrapolate_t_init →\n"
           "  Config_initialize → Memory/FormatDetector/Parser/OutputGenerator construct (см. auro_codec_v3_ida.hpp).\n"
           "  sub_EB420 @ 0xEB420: из auro_a3deng_v3_Decoder_update @ 0xEA200 при (fmt+0xC)==1 && (fmt+0x10)==24;\n"
           "  после успеха — указатель на sub_EB5A0 @ 0xEB5A0; колбэки sub_EB840/EB870/EB8A0.\n"
           "  auro_a3deng_v3_Decoder_reset_audio_state @ 0xEB820 тоже дергает sub_EB420.\n"
           "  xrefs к Decoder_t_construct: sub_EB420 (+ data @ 0xA1C0).\n"
           "\n"
           "Приложение: com.google.android.exoplayer2.ext.auro3d.AuroAudioProcessor + AuroLibrary.\n";
}

void decoder_crc_t_init_106f00() {
    // Literal перенос auro_codec_v3_decoder_CRC_t_init @ 0x106F00.
    // CRC-таблица у нас уже инициализируется лениво через channel_crc_table_init_106f00_partial(),
    // так что конструктор может быть пустым no-op якорем.
}

void decoder_channel_extrapolate_t_init_1042a1() {
    // Literal перенос auro_codec_v3_decoder_channel_Extrapolate_t_init @ 0x1042A1.
    // Полный SSE-пролог заполняет LUT в .bss/.data; для нас важно только то, что это однажды сделанный глобальный init.
    // Таблица уже материализована статически, поэтому дополнительной работы здесь не требуется.
}

// Literal перенос auro_codec_v3_decoder_Config_initialize @ 0x1033C0.
std::int64_t auro_codec_v3_decoder_Config_initialize(std::int64_t a1, std::int64_t a2) {
    const auto v2 = *reinterpret_cast<std::uint64_t*>(a2 + 8);
    std::int64_t result = auro_codec_v3_ida::kDecoderConfigInitErrBlockBits;
    if (v2 >= 0x20 && (v2 & 0x1Fu) == 0) {
        *reinterpret_cast<std::uint64_t*>(a1) = v2;
        *reinterpret_cast<std::uint32_t*>(a1 + 8) = static_cast<std::uint32_t>(v2 >> 5);
        *reinterpret_cast<std::uint32_t*>(a1 + 32) = *reinterpret_cast<std::uint32_t*>(a2);
        const auto v5 = static_cast<std::uint32_t>((v2 + 1023) / v2);
        *reinterpret_cast<std::uint32_t*>(a1 + 24) = 1u;
        *reinterpret_cast<std::uint32_t*>(a1 + 28) = v5 + 1u;
        *reinterpret_cast<std::uint64_t*>(a1 + 16) = static_cast<std::uint32_t>(v5 + 2u);
        const auto v6 = *reinterpret_cast<std::uint32_t*>(a2 + 16);
        *reinterpret_cast<std::uint32_t*>(a1 + 36) = v6;
        *reinterpret_cast<std::uint32_t*>(a1 + 40) = *reinterpret_cast<std::uint32_t*>(a2 + 20);
        *reinterpret_cast<std::uint32_t*>(a1 + 44) =
            auro_channel_Mask_count(v6, a2, static_cast<std::uint32_t>((v2 + 1023) % v2));
        *reinterpret_cast<std::uint32_t*>(a1 + 48) =
            auro_channel_Mask_count(*reinterpret_cast<std::uint32_t*>(a1 + 40), a2, 0);
        *reinterpret_cast<std::uint32_t*>(a1 + 52) = *reinterpret_cast<std::uint32_t*>(a2 + 24);
        const auto in_mask = *reinterpret_cast<std::uint32_t*>(a1 + 36);
        result = auro_codec_v3_ida::kDecoderConfigInitErrInputMaskRange;
        if (static_cast<std::int32_t>(in_mask) >= 0) {
            const auto out_mask = *reinterpret_cast<std::uint32_t*>(a1 + 40);
            result = auro_codec_v3_ida::kDecoderConfigInitErrOutputMaskRange;
            if (static_cast<std::int32_t>(out_mask) >= 0) {
                result = auro_codec_v3_ida::kDecoderConfigInitErrInputMaskNotSubset;
                if ( (in_mask & out_mask) == in_mask ) {
                    *reinterpret_cast<std::uint32_t*>(a1 + 56) =
                        *reinterpret_cast<std::uint32_t*>(a2 + 28);
                    return 0;
                }
            }
        }
    }
    return result;
}

std::uint32_t auro_channel_Mask_count(std::uint32_t mask, std::int64_t, std::uint32_t) {
    // Current IDA semantics: popcount(mask & 0x7fffffff).
    std::uint32_t cnt = 0;
    std::uint32_t m = mask & 0x7FFFFFFFu;
    while (m) {
        cnt += (m & 1u);
        m >>= 1;
    }
    return cnt;
}

std::uint32_t auro_channel_Layout_dimension(std::uint32_t layout) {
    if ((layout & 0xFFEFFFF3u) == 0u)
        return 0u;
    if ((layout & 0x33E3FE00u) != 0u)
        return 3u;
    if ((layout & 0xC0C01F4u) != 0u)
        return 2u;
    return (layout & 3u) != 0u ? 1u : 0u;
}

std::int64_t sub_eb420(
    std::uint64_t* a1,
    std::int64_t a2,
    std::int64_t a3,
    void (__fastcall *a4)(std::uint64_t, std::int64_t),
    std::int64_t a5,
    std::int64_t a6) {
    // Literal перенос sub_EB420 @ 0xEB420 (без инлайна внутренних memory helper'ов).
    *a1 = 0;
    if (reinterpret_cast<std::uint32_t*>(a1)[63030]) {
        reinterpret_cast<std::uint32_t*>(a1)[63030] = 0;
        auto* v6 = reinterpret_cast<std::uint64_t*>(a1[31521]);
        if (v6) {
            a4 = reinterpret_cast<void (__fastcall*)(std::uint64_t, std::int64_t)>(v6[1]);
            if (a4) {
                a2 = 0;
                a4(*v6, 0);
            }
        }
    }
    if (reinterpret_cast<std::uint32_t*>(a1)[63029]) {
        reinterpret_cast<std::uint32_t*>(a1)[63029] = 0;
        auto* v7 = reinterpret_cast<std::uint64_t*>(a1[31521]);
        if (v7) {
            a4 = reinterpret_cast<void (__fastcall*)(std::uint64_t, std::int64_t)>(v7[1]);
            if (a4) {
                a2 = 1;
                a4(*v7, 1);
            }
        }
    }
    if (reinterpret_cast<std::uint32_t*>(a1)[63028]) {
        reinterpret_cast<std::uint32_t*>(a1)[63028] = 0;
        auto* v8 = reinterpret_cast<std::uint64_t*>(a1[31521]);
        if (v8) {
            a4 = reinterpret_cast<void (__fastcall*)(std::uint64_t, std::int64_t)>(v8[1]);
            if (a4) {
                a2 = 1;
                a4(*v8, 1);
            }
        }
    }

    std::uint64_t v10[2]{};
    std::uint64_t v12[2]{};
    std::uint8_t v13[40]{};

    v10[1] = 64;
    reinterpret_cast<std::uint32_t&>(v10[0]) = reinterpret_cast<std::uint32_t*>(a1)[63038];

    auro_memory_block_Accumulator_t_construct(
        v12,
        a2,
        a3,
        reinterpret_cast<void(__fastcall*)(std::uint64_t, std::int64_t)>(a4),
        a5,
        a6,
        static_cast<std::uint32_t>(v10[0]),
        64,
        0x7FFF000001FFLL,
        0);

    auro_codec_v3_Decoder_t_required_additional_memory(v12, v10);

    const std::int64_t result = 1;
    // В оригинале сравнение с глобальным ограничителем; здесь оставляем упрощённую проверку.
    if (v12[0] < 0x3D091u) {
        auro_memory_block_Distributor_t_construct(v13, a1 + 264, "_ZTSNSt6__ndk112system_errorE");
        if (auro_codec_v3_Decoder_t_construct(reinterpret_cast<std::int64_t>(a1 + 1),
                                              reinterpret_cast<std::int64_t>(v13),
                                              reinterpret_cast<std::int64_t>(v10))) {
            *a1 = reinterpret_cast<std::uint64_t>(a1 + 1);
            auro_codec_v3_Decoder_set_sync_callback(a1 + 1, reinterpret_cast<void*>(codec_v3_sync_callback_eb840), a1);
            auro_codec_v3_Decoder_set_content_callback(*a1, reinterpret_cast<void*>(codec_v3_content_callback_eb870), a1);
            auro_codec_v3_Decoder_set_decide_decode_callback(
                *a1,
                reinterpret_cast<void*>(codec_v3_pre_segments_decide_decode_partial),
                a1);
            return 0;
        }
        return 1;
    }
    return result;
}

std::int64_t sub_db290_partial(
    std::uint32_t* a1,
    std::int64_t a2,
    std::uint32_t* a3,
    void* a4) {
    void* v66 = a4;
    // Literal-пролог sub_DB290 @ 0xDB290: проверки размеров/ratio.
    const std::uint32_t v4 = 32u * a1[620960];
    std::uint32_t v5 = 1u;
    if (v4 > 0x80u)
        return v5;

    const std::uint32_t v7 = 32u * a1[620965];
    if (v7 > 0x80u)
        return v5;

    const std::uint32_t v8 = *reinterpret_cast<const std::uint32_t*>(a2);
    if (v7 == 0)
        return v5;

    v5 = 137u;
    const std::uint32_t v9 = (v4 != 0) ? (v8 / v4) : 0u;
    if ((*a3 / v7) != v9)
        return v5;

    // Literal LABEL_6 блок: reconfigure Manager по текущему Decoder latency/state snapshot.
    std::array<std::uint8_t, 0xA0> cfg{};
    auto* cfg_u32 = reinterpret_cast<std::uint32_t*>(cfg.data());
    auto* cfg_u64 = reinterpret_cast<std::uint64_t*>(cfg.data());

    // v47 / v48 / v49 / v50 проекция из sub_DB290 (основные поля, которые реально читаются manager_configure).
    cfg_u32[0] = a1[620861];                   // dword_25E67C
    cfg_u32[1] = a1[620862];                   // dword_25E680
    cfg_u32[2] = a1[620863];                   // dword_25E684
    cfg_u32[3] = a1[620864];                   // dword_25E688
    cfg_u32[4] = a1[620865];                   // dword_25E68C
    cfg_u32[5] = a1[620866];                   // dword_25E690
    cfg_u32[6] = a1[620867];                   // dword_25E694
    cfg_u32[7] = a1[620868];                   // dword_25E698
    cfg_u32[8] = a1[620869];                   // dword_25E69C
    cfg_u32[9] = a1[620870];                   // dword_25E6A0
    cfg_u32[10] = a1[620871];                  // dword_25E6A4
    cfg_u32[11] = a1[620872];                  // dword_25E6A8
    cfg_u64[6] = *reinterpret_cast<const std::uint64_t*>(&a1[620873]); // qword_25E6AC
    cfg_u32[14] = a1[620875];                  // dword_25E6B4
    cfg_u32[15] = a1[620876];                  // dword_25E6B8
    cfg_u32[16] = a1[620877];                  // dword_25E6BC
    cfg_u32[17] = a1[620878];                  // dword_25E6C0

    const auto latency = static_cast<float>(auro_a3deng_v3_Decoder_latency(a1 + 557912));
    auto rc = auro_a3deng_v3_pipeline_Manager_set_initial_latency(a1 + 84, latency);
    if (rc != 0)
        return rc;
    if (a1[620978] != 0u)
        cfg_u32[0] = a1[620978];
    rc = auro_a3deng_v3_pipeline_Manager_configure(a1 + 84, cfg.data(), 1);
    if (rc != 0)
        return rc;

    // Частичный literal LABEL_11: один проход Decoder_process + Manager_configure(initial=0) + Manager_process_audio.
    const std::uint32_t blocks = v9;
    std::uint32_t frame_idx = 0;
    std::uint32_t out_advance = 0;
    while (frame_idx < blocks) {
        std::array<std::uint64_t, 16> in_blob{};
        const std::uint64_t in_base_off = static_cast<std::uint64_t>(frame_idx) * static_cast<std::uint64_t>(v4);
        const std::uint32_t in_mask = a1[620861];
        for (std::uint32_t ch = 0; ch < 14u; ++ch) {
            if (((in_mask >> ch) & 1u) == 0)
                continue;
            in_blob[ch] = *reinterpret_cast<const std::uint64_t*>(a2 + 16ull + 8ull * ch) + in_base_off;
        }

        std::int32_t v44 = 0;
        std::int32_t v43 = 0;
        rc = auro_a3deng_v3_Decoder_process(
            reinterpret_cast<std::int64_t>(a1 + 557912),
            reinterpret_cast<std::int64_t>(in_blob.data()),
            reinterpret_cast<std::int64_t>(&v66),
            reinterpret_cast<std::int64_t>(&v44),
            &v43);
        if (rc != 0)
            return rc;

        a1[620978] = static_cast<std::uint32_t>(v44);
        if (a1[620978] != 0u)
            cfg_u32[0] = a1[620978];
        rc = auro_a3deng_v3_pipeline_Manager_configure(a1 + 84, cfg.data(), 0);
        if (rc != 0)
            return rc;

        const std::int64_t bypass = (v43 != 0 && static_cast<std::uint32_t>(v44) == a1[620978]) ? 1 : 0;
        auro_a3deng_v3_pipeline_step_Upmix_set_bypass(a1 + 2840, bypass);
        rc = auro_a3deng_v3_pipeline_Manager_process_audio(a1 + 84, reinterpret_cast<void**>(&v66));
        if (rc != 0)
            return rc;

        // Literal copy-back tail (LABEL_53..LABEL_76) — без SIMD-оптимизаций, но с теми же guard/return semantics.
        const std::uint32_t out_mask = a1[620866]; // dword_25E690
        const std::size_t sample_count = static_cast<std::size_t>(v7);
        const std::size_t bytes = sample_count * sizeof(std::int32_t);
        auto** io_channels = reinterpret_cast<void**>(&v66);
        const std::size_t dst_off_samples = static_cast<std::size_t>(out_advance);

        for (std::uint32_t ch = 0; ch < 27u; ++ch) {
            if (((out_mask >> ch) & 1u) == 0)
                continue;

            std::int32_t internal_ch = 0;
            if (!auro_a3deng_v3_pipeline_Manager_get_internal_channel_id(a1 + 84, ch, &internal_ch, 0.0))
                return 1;
            const std::uint32_t internal_idx = static_cast<std::uint32_t>(internal_ch);
            const auto* src = reinterpret_cast<const std::int32_t*>(io_channels[internal_idx]);

            auto dst_base_u64 = *reinterpret_cast<std::uint64_t*>(reinterpret_cast<std::uint8_t*>(a3) + 16ull + 8ull * ch);
            auto* dst = reinterpret_cast<std::int32_t*>(dst_base_u64);
            if (!dst) {
                if (a3[3] >= 2u)
                    return 5;
                continue;
            }
            std::memcpy(dst + dst_off_samples, src, bytes);
        }

        out_advance += v7;
        ++frame_idx;
        ++a1[620862];
    }
    return 0;
}

std::int64_t auro_a3deng_v3_Decoder_latency(std::uint32_t* decoder_base) {
    if (!decoder_base)
        return 0;
    const auto decoder_ptr = *reinterpret_cast<const std::uint64_t*>(decoder_base);
    if (decoder_ptr == 0)
        return 0;
    if (*reinterpret_cast<const std::uint32_t*>(decoder_ptr + 2468u) == 0u)
        return 0;
    return static_cast<std::int64_t>(
        *reinterpret_cast<const std::uint64_t*>(decoder_ptr + auro_codec_v3_ida::kDecoder_off_qword_2472));
}

std::int64_t auro_a3deng_v3_Decoder_t_construct(
    std::int64_t decoder_base,
    std::int64_t static_parameters,
    std::int64_t memory_block,
    std::int64_t notify_sink) {
    auto* base = reinterpret_cast<std::uint8_t*>(decoder_base);
    std::memset(base + 8u, 0, 0xA00u);
    *reinterpret_cast<std::uint64_t*>(base + 2488u) = static_cast<std::uint64_t>(memory_block);
    *reinterpret_cast<std::uint64_t*>(base + 2552u) = static_cast<std::uint64_t>(notify_sink);
    *reinterpret_cast<std::uint64_t*>(base) = 0u;
    *reinterpret_cast<std::uint32_t*>(base + 2560u) =
        (*reinterpret_cast<const std::uint32_t*>(static_parameters + 240u) == 1u) ? 1u : 0u;
    return 0;
}

std::int64_t auro_a3deng_v3_Decoder_t_check_static_parameters(std::int64_t static_parameters) {
    const std::uint32_t mode = *reinterpret_cast<const std::uint32_t*>(static_parameters + 8u);
    if (*reinterpret_cast<const std::uint32_t*>(static_parameters + 240u) == 1u)
        return mode > 1u ? 193 : 0;
    return mode != 0u ? 193 : 0;
}

namespace {

void auro_a3deng_v3_decoder_notify_4d91a0(std::uint8_t* base, std::int64_t kind);

void auro_a3deng_v3_copy_or_zero_channels_4d87d0(
    std::uint32_t mask,
    std::uint32_t samples,
    const std::uint64_t* input_channels,
    std::uint64_t* output_channels) {
    const std::size_t bytes = static_cast<std::size_t>(samples) * sizeof(std::uint32_t);
    for (std::uint32_t i = 0; i != 15u; ++i) {
        auto* dst = reinterpret_cast<void*>(output_channels[i]);
        if (((mask >> i) & 1u) == 0u) {
            std::memset(dst, 0, bytes);
            continue;
        }
        const auto* src = reinterpret_cast<const void*>(input_channels[i]);
        std::memmove(dst, src, bytes);
    }
}

} // namespace

std::int64_t sub_4d87d0_partial(
    std::int64_t decoder_base,
    std::int64_t input_channels,
    std::int64_t output_channels,
    std::uint32_t* out_mask) {
    auto* base = reinterpret_cast<std::uint8_t*>(decoder_base);
    const std::uint32_t samples = 32u * *reinterpret_cast<const std::uint32_t*>(base + 2532u);
    if (samples != 0u) {
        auro_a3deng_v3_copy_or_zero_channels_4d87d0(
            *reinterpret_cast<const std::uint32_t*>(base + 2528u),
            samples,
            reinterpret_cast<const std::uint64_t*>(input_channels),
            reinterpret_cast<std::uint64_t*>(output_channels));
    }
    const std::uint32_t mask = *reinterpret_cast<const std::uint32_t*>(base + 2528u);
    *out_mask = mask;
    return mask;
}

std::uint64_t sub_4d88d0_partial(
    std::int64_t decoder_base,
    std::int64_t input_channels,
    std::int64_t output_channels,
    std::uint32_t* out_mask) {
    auto* base = reinterpret_cast<std::uint8_t*>(decoder_base);
    const std::uint32_t mask = *reinterpret_cast<const std::uint32_t*>(base + 2528u);
    const std::uint32_t samples = 32u * *reinterpret_cast<const std::uint32_t*>(base + 2532u);
    if (samples != 0u) {
        auro_a3deng_v3_copy_or_zero_channels_4d87d0(
            mask,
            samples,
            reinterpret_cast<const std::uint64_t*>(input_channels),
            reinterpret_cast<std::uint64_t*>(output_channels));
    }
    *out_mask = mask;
    return mask;
}

std::int64_t auro_a3deng_v3_Decoder_update(std::int64_t decoder_base, std::int64_t update_config) {
    auto* base = reinterpret_cast<std::uint8_t*>(decoder_base);
    const bool same_sample_rate =
        *reinterpret_cast<const std::uint32_t*>(base + 2536u)
        == *reinterpret_cast<const std::uint32_t*>(update_config + 8u);
    *reinterpret_cast<std::uint32_t*>(base + 2544u) =
        *reinterpret_cast<const std::uint32_t*>(update_config + 16u);
    std::memcpy(base + 2528u, reinterpret_cast<const void*>(update_config), 16u);

    if (!same_sample_rate) {
        std::uintptr_t process_fn = 0u;
        if (*reinterpret_cast<const std::uint32_t*>(update_config + 8u) > 0x17700u
            || *reinterpret_cast<const std::uint32_t*>(base + 2560u) != 0u) {
            *reinterpret_cast<std::uint64_t*>(base) = 0u;
            if (*reinterpret_cast<std::uint32_t*>(base + 2504u) != 0u) {
                *reinterpret_cast<std::uint32_t*>(base + 2504u) = 0u;
                auro_a3deng_v3_decoder_notify_4d91a0(base, 0);
            }
            if (*reinterpret_cast<std::uint32_t*>(base + 2500u) != 0u) {
                *reinterpret_cast<std::uint32_t*>(base + 2500u) = 0u;
                auro_a3deng_v3_decoder_notify_4d91a0(base, 1);
            }
            if (*reinterpret_cast<std::uint32_t*>(base + 2496u) != 0u) {
                *reinterpret_cast<std::uint32_t*>(base + 2496u) = 0u;
                auro_a3deng_v3_decoder_notify_4d91a0(base, 1);
            }
            const std::uint32_t mode = *reinterpret_cast<const std::uint32_t*>(update_config + 12u);
            if (mode == 1u) {
                process_fn = reinterpret_cast<std::uintptr_t>(&sub_4d87d0_partial);
            } else {
                if (mode != 0u)
                    return 1;
                process_fn = reinterpret_cast<std::uintptr_t>(&sub_4d88d0_partial);
            }
        } else {
            if (*reinterpret_cast<const std::uint32_t*>(update_config + 12u) != 1u)
                return 1;
            if (*reinterpret_cast<const std::uint32_t*>(update_config + 16u) != 24u)
                return 1;
            if (sub_4d91a0_partial(reinterpret_cast<std::uint64_t*>(base)) != 0)
                return 1;
            process_fn = reinterpret_cast<std::uintptr_t>(&sub_4d9320_partial);
        }
        *reinterpret_cast<std::uintptr_t*>(base + 2520u) = process_fn;
    }

    if (*reinterpret_cast<const std::uint64_t*>(base) == 0u)
        return 0;
    const std::uint32_t blocks = *reinterpret_cast<const std::uint32_t*>(base + 2532u);
    if (blocks == 0u || (blocks & 0x7FFFFFFu) == 2u)
        return 0;
    return 1;
}

std::int64_t auro_a3deng_v3_Decoder_process(
    std::int64_t decoder_base,
    std::int64_t input_channels_blob,
    std::int64_t output_channels_blob,
    std::int64_t out_mask,
    std::int32_t* out_changed) {
    auto* b = reinterpret_cast<std::uint8_t*>(decoder_base);
    if (!b) {
        if (out_changed)
            *out_changed = 0;
        return 0;
    }
    *reinterpret_cast<std::uint32_t*>(b + 2512u) = 0u;
    using ProcFn = std::int64_t (__fastcall *)(std::int64_t, std::int64_t, std::int64_t, std::uint32_t*);
    auto* proc = reinterpret_cast<ProcFn>(*reinterpret_cast<std::uintptr_t*>(b + 2520u));
    if (proc)
        (void)proc(
            decoder_base,
            input_channels_blob,
            output_channels_blob,
            reinterpret_cast<std::uint32_t*>(out_mask));
    const auto rc = static_cast<std::int32_t>(*reinterpret_cast<std::uint32_t*>(b + 2512u));
    if (out_changed)
        *out_changed = rc;
    return rc;
}

namespace {

void auro_a3deng_v3_decoder_notify_4d91a0(std::uint8_t* base, std::int64_t kind) {
    if (!base)
        return;
    const std::uint64_t sink_ptr = *reinterpret_cast<const std::uint64_t*>(base + 2552u);
    if (sink_ptr == 0u)
        return;
    const std::uint64_t fn_ptr = *reinterpret_cast<const std::uint64_t*>(sink_ptr + 8u);
    if (fn_ptr == 0u)
        return;
    using NotifyFn = void (__fastcall *)(std::uint64_t, std::int64_t);
    reinterpret_cast<NotifyFn>(fn_ptr)(*reinterpret_cast<const std::uint64_t*>(sink_ptr), kind);
}

} // namespace

void sub_4d95b0_partial(std::int64_t decoder_base, std::int32_t value) {
    auto* base = reinterpret_cast<std::uint8_t*>(decoder_base);
    if (*reinterpret_cast<std::int32_t*>(base + 2504u) == value)
        return;
    *reinterpret_cast<std::int32_t*>(base + 2504u) = value;
    auro_a3deng_v3_decoder_notify_4d91a0(base, 0);
}

void sub_4d95e0_partial(std::int64_t decoder_base, std::int32_t value) {
    auto* base = reinterpret_cast<std::uint8_t*>(decoder_base);
    if (*reinterpret_cast<std::int32_t*>(base + 2500u) == value)
        return;
    *reinterpret_cast<std::int32_t*>(base + 2500u) = value;
    auro_a3deng_v3_decoder_notify_4d91a0(base, 1);
}

std::int64_t sub_4d9610_partial(
    std::int64_t decoder_base,
    std::int64_t next_state_src,
    std::int64_t decisions,
    std::uint32_t decision_count) {
    auto* base = reinterpret_cast<std::uint8_t*>(decoder_base);
    std::uint32_t result = *reinterpret_cast<const std::uint32_t*>(base + 2496u);
    std::uint32_t v8 = 0u;
    if (decision_count == 0u) {
        *reinterpret_cast<std::uint32_t*>(base + 2512u) = 0u;
        if (result == 0u)
            return result;
    } else {
        std::uint8_t same = 1u;
        auto* decision_words = reinterpret_cast<std::uint32_t*>(decisions);
        for (std::uint32_t i = 0; i != decision_count; ++i) {
            v8 = 0u;
            if (decision_words[i] != 0u) {
                const std::uint32_t allowed = *reinterpret_cast<const std::uint32_t*>(base + 2508u) != 0u ? 1u : 0u;
                decision_words[i] = allowed;
                if (allowed)
                    v8 = *reinterpret_cast<const std::uint32_t*>(next_state_src + 20u);
            }
            same = static_cast<std::uint8_t>(same & (v8 == result));
        }
        result = *reinterpret_cast<const std::uint32_t*>(base + 2496u);
        *reinterpret_cast<std::uint32_t*>(base + 2512u) = static_cast<std::uint32_t>(same ^ 1u);
        if (result == v8)
            return result;
    }
    *reinterpret_cast<std::uint32_t*>(base + 2496u) = v8;
    const std::uint64_t sink_ptr = *reinterpret_cast<const std::uint64_t*>(base + 2552u);
    if (sink_ptr == 0u)
        return sink_ptr;
    const std::uint64_t fn_ptr = *reinterpret_cast<const std::uint64_t*>(sink_ptr + 8u);
    if (fn_ptr == 0u)
        return sink_ptr;
    using NotifyFn = std::int64_t (__fastcall *)(std::uint64_t, std::int64_t);
    return reinterpret_cast<NotifyFn>(fn_ptr)(*reinterpret_cast<const std::uint64_t*>(sink_ptr), 1);
}

std::int64_t sub_4d91a0_partial(std::uint64_t* decoder_base) {
    auto* base = reinterpret_cast<std::uint8_t*>(decoder_base);
    *decoder_base = 0u;

    if (*reinterpret_cast<std::uint32_t*>(base + 2504u) != 0u) {
        *reinterpret_cast<std::uint32_t*>(base + 2504u) = 0u;
        auro_a3deng_v3_decoder_notify_4d91a0(base, 0);
    }
    if (*reinterpret_cast<std::uint32_t*>(base + 2500u) != 0u) {
        *reinterpret_cast<std::uint32_t*>(base + 2500u) = 0u;
        auro_a3deng_v3_decoder_notify_4d91a0(base, 1);
    }
    if (*reinterpret_cast<std::uint32_t*>(base + 2496u) != 0u) {
        *reinterpret_cast<std::uint32_t*>(base + 2496u) = 0u;
        auro_a3deng_v3_decoder_notify_4d91a0(base, 1);
    }

    std::uint8_t init_args[32]{};
    *reinterpret_cast<std::uint32_t*>(init_args + 0u) =
        *reinterpret_cast<const std::uint32_t*>(base + 2536u);
    *reinterpret_cast<std::uint64_t*>(init_args + 8u) = 64u;
    *reinterpret_cast<std::uint64_t*>(init_args + 16u) = 0x7FFF000001FFull;

    std::uint64_t acc[2]{};
    auro_codec_v3_Decoder_t_required_additional_memory(
        acc,
        reinterpret_cast<std::uint64_t*>(init_args));
    if (acc[0] >= 0x3D091u)
        return 1;

    std::uint8_t distributor[40]{};
    auro_memory_block_Distributor_t_construct(
        distributor,
        reinterpret_cast<std::uint64_t*>(*reinterpret_cast<const std::uint64_t*>(base + 2488u)),
        "_ZTSNSt6__ndk112system_errorE");

    if (!auro_codec_v3_Decoder_t_construct(
            reinterpret_cast<std::int64_t>(base + 8u),
            reinterpret_cast<std::int64_t>(distributor),
            reinterpret_cast<std::int64_t>(init_args))) {
        return 1;
    }

    *decoder_base = reinterpret_cast<std::uint64_t>(base + 8u);
    auro_codec_v3_Decoder_set_sync_callback(
        reinterpret_cast<std::uint64_t*>(base + 8u),
        reinterpret_cast<void*>(sub_4d95b0_partial),
        base);
    auro_codec_v3_Decoder_set_content_callback(
        *decoder_base,
        reinterpret_cast<void*>(sub_4d95e0_partial),
        base);
    auro_codec_v3_Decoder_set_decide_decode_callback(
        *decoder_base,
        reinterpret_cast<void*>(codec_v3_pre_segments_decide_decode_partial),
        base);
    return 0;
}

void auro_a3deng_v3_Decoder_allow_decoding(std::int64_t decoder_base, std::int32_t enabled) {
    *reinterpret_cast<std::int32_t*>(reinterpret_cast<std::uint8_t*>(decoder_base) + 2508u) = enabled;
}

std::int64_t auro_a3deng_v3_Decoder_reset_audio_state(std::uint64_t* decoder_base) {
    if (!decoder_base || *decoder_base == 0u)
        return 0;
    return sub_4d91a0_partial(decoder_base) != 0 ? 1 : 0;
}

std::int64_t sub_4d9320_partial(
    std::int64_t decoder_base,
    const void* input_channels,
    std::int64_t output_channels,
    std::uint32_t* io_status) {
    auto* base = reinterpret_cast<std::uint8_t*>(decoder_base);
    CodecV3IoBufferDescEb5a0 input_desc{};
    CodecV3IoBufferDescEb5a0 output_desc{};

    const std::uint32_t block_count = *reinterpret_cast<const std::uint32_t*>(base + 2532u);
    const std::uint32_t total_samples = block_count != 0u ? (32u * block_count) : 64u;
    input_desc.total_samples = total_samples;
    input_desc.sample_rate = *reinterpret_cast<const std::uint32_t*>(base + 2536u);
    input_desc.bits_per_sample = 24u;
    if (input_channels)
        std::memcpy(input_desc.channel_ptr, input_channels, 120u);

    output_desc.total_samples = total_samples;
    output_desc.sample_rate = input_desc.sample_rate;
    output_desc.bits_per_sample = 24u;
    if (output_channels != 0)
        std::memcpy(output_desc.channel_ptr, reinterpret_cast<const void*>(output_channels), 120u);

    std::uint32_t local_status = 0u;
    auto* status = io_status ? io_status : &local_status;
    const std::int64_t result = auro_codec_v3_Decoder_process(
        *reinterpret_cast<const std::uint64_t*>(base),
        &input_desc,
        *reinterpret_cast<const std::uint32_t*>(base + 2528u),
        &output_desc,
        status);
    if (*status == 0u)
        *status = *reinterpret_cast<const std::uint32_t*>(base + 2528u);
    return result;
}

std::int64_t auro_a3deng_v3_parameter_CutoffFrequency_from_int(std::uint32_t value) {
    static constexpr std::uint32_t kThresholds[] = {
        30u, 35u, 45u, 55u, 65u, 75u, 85u, 95u,
        105u, 115u, 125u, 135u, 145u, 155u, 165u, 175u,
        185u, 195u, 205u, 215u, 225u, 235u, 245u, 255u,
        265u, 275u, 285u, 295u, 305u, 315u, 325u, 335u,
        345u, 355u, 365u, 375u, 385u, 395u, 405u, 415u,
        425u, 435u, 445u, 455u, 465u, 475u, 485u, 495u,
        505u,
    };
    static constexpr std::int64_t kValues[] = {
        1, 2, 3, 4, 5, 6, 7, 8,
        9, 0, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 28, 29, 30, 31,
        32, 33, 34, 35, 36, 37, 38, 39,
        40, 41, 42, 43, 44, 45, 46, 47,
        49,
    };
    std::int64_t result = 48;
    for (std::size_t i = 0; i != sizeof(kThresholds) / sizeof(kThresholds[0]); ++i) {
        if (value < kThresholds[i])
            break;
        result = kValues[i];
    }
    return result;
}

std::int64_t auro_a3deng_v3_parameter_CutoffFrequency_to_int(std::uint32_t value) {
    static constexpr std::uint32_t kCutoffFrequencyToInt[48] = {
        120u, 30u, 40u, 50u, 60u, 70u, 80u, 90u,
        100u, 110u, 130u, 140u, 150u, 160u, 170u, 180u,
        190u, 200u, 210u, 220u, 230u, 240u, 250u, 260u,
        270u, 280u, 290u, 300u, 310u, 320u, 330u, 340u,
        350u, 360u, 370u, 380u, 390u, 400u, 410u, 420u,
        430u, 440u, 450u, 460u, 470u, 480u, 490u, 500u,
    };
    if (value <= 0x2Fu)
        return kCutoffFrequencyToInt[value];
    return 0;
}

float auro_a3deng_v3_strength_translate_to_float(std::uint32_t value) {
    static constexpr float kStrengthDb[15] = {
        -30.0f, -27.0f, -21.0f, -18.0f, -15.0f,
        -12.0f, -9.0f, -6.0f, -3.0f, -2.0f,
        -1.0f, 0.0f, 1.0f, 2.0f, 3.0f,
    };
    if (value == 0u)
        return 0.0f;
    if (value > 0xFu)
        return 1.0f;
    return std::pow(10.0f, kStrengthDb[value - 1u] * 0.050000001f);
}

float auro_a3deng_v3_strength_translate_to_dB(std::uint32_t value) {
    static constexpr float kStrengthDb[15] = {
        -30.0f, -27.0f, -21.0f, -18.0f, -15.0f,
        -12.0f, -9.0f, -6.0f, -3.0f, -2.0f,
        -1.0f, 0.0f, 1.0f, 2.0f, 3.0f,
    };
    if (value == 0u)
        return -144.0f;
    if (value <= 0xFu)
        return kStrengthDb[value - 1u];
    return 0.0f;
}

std::int64_t auro_a3deng_v3_strength_check_range(std::uint32_t value) {
    return value < 0x10u ? 0 : 210;
}

std::int64_t auro_a3deng_v3_strength_get_default() {
    return 12;
}

std::int64_t auro_a3deng_v3_pipeline_Manager_set_initial_latency(std::uint32_t* manager_base, float latency) {
    if (!manager_base || latency < 0.0f)
        return 1;
    *reinterpret_cast<float*>(reinterpret_cast<std::uint8_t*>(manager_base) + 480u) = latency;
    return 0;
}

namespace {

constexpr std::uintptr_t kManagerOffParams478078 = ::auro_engine_v4_ida::kManager_off_params;
constexpr std::uintptr_t kManagerOffBypass478078 = ::auro_engine_v4_ida::kManager_off_bypass;
constexpr std::uintptr_t kManagerOffHeadroomDirty478078 = ::auro_engine_v4_ida::kManager_off_headroom_dirty;
constexpr std::uintptr_t kManagerOffHeadroomRecalcDirty478078 = 484u;
constexpr std::uintptr_t kManagerOffStepChain478078 = ::auro_engine_v4_ida::kManager_off_step_chain;
constexpr std::uintptr_t kManagerOffConfigureCb478078 = ::auro_engine_v4_ida::kManager_off_configure_cb;
constexpr std::uintptr_t kManagerParamsBytes478078 = ::auro_engine_v4_ida::kManager_params_bytes;
constexpr std::size_t kManagerIoSilenceBytes478078 = 512u;

struct ManagerStepOffsets478078 {
    std::uintptr_t base;
    std::uintptr_t process_fn;
    std::uintptr_t reset_fn;
    std::uintptr_t active;
};

constexpr ManagerStepOffsets478078 kManagerSteps478078[] = {
    {0u, 16u, 24u, 32u},
    {64u, 80u, 88u, 96u},
    {1800u, 1816u, 1824u, 1832u},
    {3536u, 3552u, 3560u, 3568u},
    {3608u, 3624u, 3632u, 3640u},
    {4464u, 4480u, 4488u, 4496u},
    {5424u, 5440u, 5448u, 5456u},
    {6256u, 6272u, 6280u, 6288u},
    {7952u, 7968u, 7976u, 7984u},
    {13080u, 13096u, 13104u, 13112u},
    {13168u, 13184u, 13192u, 13200u},
    {14000u, 14016u, 14024u, 14032u},
    {14192u, 14208u, 14216u, 14224u},
    {14568u, 14584u, 14592u, 14600u},
    {14656u, 14672u, 14680u, 14688u},
    {14856u, 14872u, 14880u, 14888u},
    {15808u, 15824u, 15832u, 15840u},
    {16184u, 16200u, 16208u, 16216u},
    {16256u, 16272u, 16280u, 16288u},
    {61792u, 61808u, 61816u, 61824u},
};

std::uint32_t manager_parameter_update_type_482a3c_partial(const std::uint32_t* current, const std::uint32_t* next) {
    if (!current || !next)
        return 0u;
    const bool current_has_layout = current[0] != 0u;
    const bool next_has_layout = next[0] != 0u;
    if (!current_has_layout && !next_has_layout)
        return 0u;
    if (!current_has_layout && next_has_layout)
        return 2u;
    if (current_has_layout && !next_has_layout)
        return 3u;
    if (current[2] != next[2] || current[7] != next[7])
        return 2u;

    constexpr std::uint32_t kDynamicCompareWords[] = {
        0u, 5u, 3u, 8u, 10u, 11u, 12u, 15u,
        21u, 22u, 23u, 24u, 25u, 30u, 26u, 27u, 28u, 29u,
    };
    for (std::uint32_t word : kDynamicCompareWords) {
        if (current[word] != next[word])
            return 1u;
    }
    return 0u;
}

void manager_reset_step_chain_477758_partial(std::uint8_t* step_chain) {
    if (!step_chain)
        return;
    for (const auto& step : kManagerSteps478078) {
        const std::uint64_t active = *reinterpret_cast<const std::uint64_t*>(step_chain + step.active);
        if (active == 0u)
            continue;
        using ResetFn = void (*)(std::uint64_t);
        const auto fn = reinterpret_cast<ResetFn>(*reinterpret_cast<const std::uint64_t*>(step_chain + step.reset_fn));
        if (fn)
            fn(reinterpret_cast<std::uint64_t>(step_chain + step.base));
    }
}

std::int64_t manager_process_step_chain_478078_partial(
    std::uint8_t* manager,
    std::uint8_t* step_chain,
    void** io_channels) {
    if (!manager || !step_chain || !io_channels)
        return 1;
    for (const auto& step : kManagerSteps478078) {
        const std::uint64_t active = *reinterpret_cast<const std::uint64_t*>(step_chain + step.active);
        if (active == 0u)
            continue;
        const std::uint64_t fn_u64 = *reinterpret_cast<const std::uint64_t*>(step_chain + step.process_fn);
        if (fn_u64 == 0u)
            continue;
        if (fn_u64 == ::auro_engine_v4_ida::kStepUpmixXinN_process) {
            const std::uint64_t step_base = reinterpret_cast<std::uint64_t>(step_chain + step.base);
            auto* step_bytes = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(step_base));
            std::uint32_t subblock_count = 0u;
            const std::uint8_t* records = nullptr;
            const std::uint64_t block_state = *reinterpret_cast<const std::uint64_t*>(step_bytes + 8u);
            if (block_state != 0u)
                subblock_count = *reinterpret_cast<const std::uint32_t*>(static_cast<std::uintptr_t>(block_state + 4u));
            const std::uint64_t record_owner = *reinterpret_cast<const std::uint64_t*>(step_bytes + 483952u);
            if (record_owner != 0u) {
                const std::uint64_t records_u64 =
                    *reinterpret_cast<const std::uint64_t*>(static_cast<std::uintptr_t>(record_owner + 32u));
                records = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(records_u64));
            }
            const std::int64_t rc = auro_a3deng_v4_pipeline_step_upmix_XinN_process_35b440_partial(
                step_base,
                io_channels,
                subblock_count,
                records,
                nullptr,
                auro_a3deng_v4_pipeline_step_upmix_XinN_reset_audio_state_35b730_partial);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 < 0x10000000u)
            continue;
        using ProcessFn = std::int64_t (*)(std::uint64_t, std::uint64_t, std::uint64_t);
        const auto fn = reinterpret_cast<ProcessFn>(static_cast<std::uintptr_t>(fn_u64));
        const std::int64_t rc = fn(
            reinterpret_cast<std::uint64_t>(step_chain + step.base),
            reinterpret_cast<std::uint64_t>(io_channels),
            reinterpret_cast<std::uint64_t>(manager + kManagerOffParams478078));
        if (rc != 0)
            return rc;
    }
    return 0;
}

void manager_update_headroom_input_gains_478078_partial(std::uint8_t* manager) {
    if (!manager)
        return;
    if (*reinterpret_cast<std::uint32_t*>(manager + kManagerOffHeadroomDirty478078) == 0u)
        return;
    auto* step_chain = reinterpret_cast<std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<std::uint64_t*>(manager + kManagerOffStepChain478078)));
    if (!step_chain) {
        *reinterpret_cast<std::uint32_t*>(manager + kManagerOffHeadroomDirty478078) = 0u;
        return;
    }

    const std::uint32_t output_mask = *reinterpret_cast<const std::uint32_t*>(manager + 136u);
    auto* gains = reinterpret_cast<float*>(manager + 356u);
    auto* headroom_gains = reinterpret_cast<float*>(step_chain + 15872u);
    for (std::uint32_t ch = 0; ch < 15u; ++ch)
        headroom_gains[ch] = ((output_mask >> ch) & 1u) != 0u ? -gains[ch] : -0.0f;

    using RecalcFn = void (*)(std::uint64_t);
    const auto recalc = reinterpret_cast<RecalcFn>(*reinterpret_cast<const std::uint64_t*>(step_chain + 15832u));
    if (recalc)
        recalc(reinterpret_cast<std::uint64_t>(step_chain + 15808u));
    *reinterpret_cast<std::uint32_t*>(manager + kManagerOffHeadroomDirty478078) = 0u;
}

} // namespace

std::int64_t auro_a3deng_v3_pipeline_Manager_configure(
    std::uint32_t* manager_base,
    void* cfg_blob,
    std::int32_t initial) {
    if (!manager_base || !cfg_blob)
        return 1;
    // IDA 0xDD220: ранний guard перед копированием параметров.
    const auto* cfg_u32 = reinterpret_cast<const std::uint32_t*>(cfg_blob);
    if (cfg_u32[1] * cfg_u32[7] != cfg_u32[6] * cfg_u32[2])
        return 202;
    auto* manager = reinterpret_cast<std::uint8_t*>(manager_base);
    const std::uint32_t update_type = manager_parameter_update_type_482a3c_partial(
        reinterpret_cast<const std::uint32_t*>(manager + kManagerOffParams478078),
        cfg_u32);
    std::memcpy(manager + kManagerOffParams478078, cfg_blob, kManagerParamsBytes478078);

    auto* step_chain = reinterpret_cast<std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<std::uint64_t*>(manager + kManagerOffStepChain478078)));
    if (update_type == 3u) {
        *reinterpret_cast<std::uint32_t*>(manager + kManagerOffBypass478078) = 1u;
        manager_reset_step_chain_477758_partial(step_chain);
    } else if (update_type == 2u) {
        *reinterpret_cast<std::uint32_t*>(manager + kManagerOffBypass478078) = 0u;
        if (!initial)
            return 1;
        using ConfigureFn = std::int64_t (*)(std::uint64_t, std::uint32_t);
        const auto fn = reinterpret_cast<ConfigureFn>(*reinterpret_cast<std::uint64_t*>(manager + kManagerOffConfigureCb478078));
        if (fn) {
            const std::int64_t rc = fn(reinterpret_cast<std::uint64_t>(manager), static_cast<std::uint32_t>(initial));
            if (rc != 0)
                return rc;
        }
    } else if (update_type == 1u) {
        *reinterpret_cast<std::uint32_t*>(manager + kManagerOffBypass478078) = 0u;
        using ConfigureFn = std::int64_t (*)(std::uint64_t, std::uint32_t);
        const auto fn = reinterpret_cast<ConfigureFn>(*reinterpret_cast<std::uint64_t*>(manager + kManagerOffConfigureCb478078));
        if (fn) {
            const std::int64_t rc = fn(reinterpret_cast<std::uint64_t>(manager), static_cast<std::uint32_t>(initial));
            if (rc != 0)
                return rc;
        }
    }

    (void)kManagerOffHeadroomRecalcDirty478078;
    return 0;
}

std::int64_t auro_a3deng_v3_pipeline_Manager_process_audio(std::uint32_t* manager_base, void** io_channels) {
    if (!manager_base || !io_channels)
        return 1;
    auto* manager = reinterpret_cast<std::uint8_t*>(manager_base);
    manager_update_headroom_input_gains_478078_partial(manager);

    if (*reinterpret_cast<std::uint32_t*>(manager + kManagerOffBypass478078) != 0u) {
        for (std::size_t i = 0; i < 15; ++i) {
            if (io_channels[i])
                std::memset(io_channels[i], 0, kManagerIoSilenceBytes478078);
        }
        return 0;
    }

    auto* step_chain = reinterpret_cast<std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<std::uint64_t*>(manager + kManagerOffStepChain478078)));
    if (!step_chain)
        return 0;
    return manager_process_step_chain_478078_partial(manager, step_chain, io_channels);
}

std::int64_t auro_matic_XinN_fl32_process_574fe0_partial(std::uint64_t xinn_state, void** channel_span_31) {
    if (xinn_state == 0u)
        return 0;
    using CallbackFn = std::int64_t (*)(std::uint64_t, void**);
    const std::uint64_t fn_u64 = *reinterpret_cast<const std::uint64_t*>(xinn_state + 24u);
    if (fn_u64 == ::auro_engine_v4_ida::kAuroMaticXinN_fl32_process_inplace)
        return auro_matic_XinN_fl32_process_inplace_574e30_partial(xinn_state, channel_span_31);
    if (fn_u64 == ::auro_engine_v4_ida::kAuroMaticXinN_fl32_process_scratch)
        return auro_matic_XinN_fl32_process_scratch_574ee0_partial(xinn_state, channel_span_31);
    const auto fn = reinterpret_cast<CallbackFn>(static_cast<std::uintptr_t>(fn_u64));
    if (fn)
        return fn(xinn_state, channel_span_31);
    return 0;
}

std::int64_t auro_matic_v3_XinN_fl32_process_556440_partial(std::uint64_t xinn_v3_state, void** channel_span_31) {
    if (xinn_v3_state == 0u)
        return 0;
    if (*reinterpret_cast<const std::uint32_t*>(xinn_v3_state + 840u) != 0u)
        return auro_matic_XinN_fl32_process_574fe0_partial(xinn_v3_state, channel_span_31);
    return 0;
}

void auro_audio_Smooth_fl32_inst_initialize_5aab80_partial(float* smooth_state, std::uint32_t sample_rate, float seconds) {
    if (!smooth_state)
        return;
    float coeff = 0.0f;
    const float samples = static_cast<float>(sample_rate) * seconds;
    if (samples > 0.0f)
        coeff = std::pow(10.0f, -3.0f / samples);
    smooth_state[2] = coeff;
}

void auro_audio_Smooth_fl32_inst_update_5aabd0_partial(float* smooth_state, float target) {
    if (smooth_state)
        smooth_state[1] = target;
}

void auro_audio_Smooth_fl32_inst_set_current_5aabe0_partial(float* smooth_state, float current) {
    if (smooth_state)
        smooth_state[0] = current;
}

void auro_audio_Smooth_fl32_inst_gains_smooth_5aae00_partial(
    float* smooth_state,
    float* gains,
    std::uint32_t count) {
    if (!smooth_state || !gains || count == 0u)
        return;
    const float target = smooth_state[1];
    const float coeff = smooth_state[2];
    float current = smooth_state[0];
    for (std::uint32_t i = 0; i < count; ++i) {
        current = (1.0f - coeff) * target + coeff * current;
        gains[i] = current;
    }
    smooth_state[0] = current;
}

void auro_matic_Engine1_fl32_set_total_clear_frames_511698_partial(std::uint8_t* engine1_state, std::uint32_t frames) {
    if (engine1_state)
        *reinterpret_cast<std::uint32_t*>(engine1_state + 64u) = frames;
}

std::int64_t auro_matic_Engine1_fl32_reset_audio_state_511760_partial(std::uint8_t* engine1_state) {
    if (!engine1_state)
        return 0;
    const std::uint32_t delay = *reinterpret_cast<const std::uint32_t*>(engine1_state + 60u);
    *reinterpret_cast<std::uint64_t*>(engine1_state + 52u) = 0u;
    *reinterpret_cast<std::uint64_t*>(engine1_state + 44u) = 0u;
    const std::uint32_t clear_index =
        delay != 0u ? 0u : *reinterpret_cast<const std::uint32_t*>(engine1_state + 64u);
    *reinterpret_cast<std::uint32_t*>(engine1_state + 68u) = clear_index;
    return clear_index;
}

void auro_matic_Engine1_fl32_get_delayed_frame_511784_partial(
    const std::uint8_t* engine1_state,
    std::uint64_t* out_near,
    std::uint64_t* out_far) {
    if (!engine1_state || !out_near || !out_far)
        return;
    const std::uint64_t ring = *reinterpret_cast<const std::uint64_t*>(engine1_state + 8u);
    const std::uint64_t info = *reinterpret_cast<const std::uint64_t*>(engine1_state + 0u);
    const std::uint32_t write_index = 32u * *reinterpret_cast<const std::uint32_t*>(engine1_state + 40u);
    const std::uint32_t latency = info != 0u ? *reinterpret_cast<const std::uint32_t*>(static_cast<std::uintptr_t>(info) + 24u) : 0u;
    const std::uint32_t near_end = latency + 32u;
    const std::uint32_t far_end = latency + 3840u;
    const std::uint32_t near_wrap = write_index >= near_end ? 0u : 7616u;
    const std::uint32_t far_wrap = write_index >= far_end ? 0u : 7616u;
    *out_near = ring + 4ull * static_cast<std::uint64_t>(near_wrap + write_index - near_end);
    *out_far = ring + 4ull * static_cast<std::uint64_t>(far_wrap + write_index - far_end);
}

bool auro_matic_Engine1_fl32_partial_clear_5116ec_partial(std::uint8_t* engine1_state, std::uint32_t* remaining) {
    if (!engine1_state || !remaining)
        return false;
    std::uint32_t delay = *reinterpret_cast<std::uint32_t*>(engine1_state + 60u);
    const std::uint32_t clear = std::min(delay, *remaining);
    if (clear != 0u) {
        const std::uint64_t ring = *reinterpret_cast<const std::uint64_t*>(engine1_state + 8u);
        if (ring != 0u)
            std::memset(reinterpret_cast<void*>(static_cast<std::uintptr_t>(ring) + 4ull * (delay - clear)), 0, 4ull * clear);
    }
    delay -= clear;
    *reinterpret_cast<std::uint32_t*>(engine1_state + 60u) = delay;
    *remaining -= clear;
    return delay == 0u;
}

void engine1_copy32_ring_5130cc_partial(float* ring, std::uint32_t offset, const float* src_32) {
    if (!ring || !src_32)
        return;
    offset %= 7616u;
    float* dst = ring + offset;
    std::memcpy(dst, src_32, 32u * sizeof(float));
    if (offset < 32u)
        std::memcpy(dst + 7616u, src_32, 32u * sizeof(float));
}

void engine1_sum_98_taps_to_vec_5117d8_partial(
    const float* ring,
    std::uint32_t ring_offset,
    const std::uint8_t* entries,
    float* dst_4) {
    if (!dst_4)
        return;
    dst_4[0] = 0.0f;
    dst_4[1] = 0.0f;
    dst_4[2] = 0.0f;
    dst_4[3] = 0.0f;
    if (!ring || !entries)
        return;
    for (std::size_t tap = 0; tap < 98u; ++tap) {
        const auto* e = entries + tap * 8u;
        const std::uint32_t sample_index = *reinterpret_cast<const std::uint32_t*>(e);
        const float gain = *reinterpret_cast<const float*>(e + 4u);
        const std::uint32_t absolute_index = (ring_offset + sample_index) % 7616u;
        const float* x = ring + absolute_index;
        dst_4[0] += x[0] * gain;
        dst_4[1] += x[1] * gain;
        dst_4[2] += x[2] * gain;
        dst_4[3] += x[3] * gain;
    }
}

void auro_matic_Engine1_fl32_process_ext_5117d8_partial(
    std::uint8_t* engine1_state,
    const float* input_a_32,
    const float* input_b_32,
    float* output_0x300,
    const float* gains_32) {
    if (!engine1_state || !input_a_32 || !input_b_32 || !output_0x300 || !gains_32)
        return;

    const std::uint32_t total_clear = *reinterpret_cast<const std::uint32_t*>(engine1_state + 64u);
    std::uint32_t clear_index = *reinterpret_cast<const std::uint32_t*>(engine1_state + 68u);
    auto* ring = reinterpret_cast<float*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<const std::uint64_t*>(engine1_state + 8u)));
    if (!ring)
        return;

    if (total_clear != 0u || clear_index != 0u) {
        if (total_clear != 0u && clear_index < total_clear) {
            const std::uint32_t span = (119u - total_clear) / total_clear;
            const std::uint32_t start = span * clear_index;
            const std::uint32_t count = (clear_index + 1u == total_clear)
                ? (119u - total_clear - span * clear_index)
                : span;
            if (count != 0u) {
                std::memset(ring + 32u * (start + total_clear), 0, 32ull * count * sizeof(float));
                std::memset(ring + 3808u + 32u * start, 0, 32ull * count * sizeof(float));
            }
        } else if (total_clear == 0u && *reinterpret_cast<const std::uint32_t*>(engine1_state + 60u) != 0u) {
            std::memset(ring, 0, 0x7780u);
            clear_index = 0u;
        }
        *reinterpret_cast<std::uint32_t*>(engine1_state + 40u) = clear_index;
    }

    float filtered_a[32];
    float filtered_b[32];
    for (std::size_t i = 0; i < 32u; ++i) {
        filtered_a[i] = input_a_32[i] * gains_32[i];
        filtered_b[i] = input_b_32[i] * gains_32[i];
    }

    const auto* coeff = reinterpret_cast<const float*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<const std::uint64_t*>(engine1_state + 0u)));
    *reinterpret_cast<std::uint32_t*>(engine1_state + 60u) = 7648u;
    if (coeff) {
        float a_z1 = *reinterpret_cast<float*>(engine1_state + 44u);
        float a_z2 = *reinterpret_cast<float*>(engine1_state + 48u);
        float b_z1 = *reinterpret_cast<float*>(engine1_state + 52u);
        float b_z2 = *reinterpret_cast<float*>(engine1_state + 56u);
        for (std::size_t i = 0; i < 32u; ++i) {
            const float xa = filtered_a[i];
            const float xb = filtered_b[i];
            const float ya = a_z1 + coeff[1] * xa;
            const float yb = b_z1 + coeff[1] * xb;
            filtered_a[i] = ya;
            filtered_b[i] = yb;
            a_z1 = (a_z2 + coeff[2] * xa) - coeff[4] * ya;
            b_z1 = (b_z2 + coeff[2] * xb) - coeff[4] * yb;
            a_z2 = coeff[3] * xa - coeff[5] * ya;
            b_z2 = coeff[3] * xb - coeff[5] * yb;
        }
        *reinterpret_cast<float*>(engine1_state + 44u) = a_z1;
        *reinterpret_cast<float*>(engine1_state + 48u) = a_z2;
        *reinterpret_cast<float*>(engine1_state + 52u) = b_z1;
        *reinterpret_cast<float*>(engine1_state + 56u) = b_z2;
    }

    const std::uint32_t write_index = *reinterpret_cast<const std::uint32_t*>(engine1_state + 40u);
    const std::uint32_t ring_a_offset = 32u * write_index;
    engine1_copy32_ring_5130cc_partial(ring, ring_a_offset, filtered_a);
    engine1_copy32_ring_5130cc_partial(ring, ring_a_offset + 3808u, filtered_b);

    std::memset(output_0x300, 0, 0x300u);
    const std::uint32_t current_clear = *reinterpret_cast<const std::uint32_t*>(engine1_state + 68u);
    std::uint32_t next_write_index = write_index;
    if (current_clear >= total_clear) {
        const std::uint32_t group_count = *reinterpret_cast<const std::uint32_t*>(engine1_state + 132u);
        const std::uint32_t history_offset = 32u * write_index;
        for (std::uint32_t group = 0; group < group_count; ++group) {
            const std::uint64_t source_table_u64 =
                *reinterpret_cast<const std::uint64_t*>(engine1_state + 72u + 8u * group);
            if (source_table_u64 == 0u)
                continue;
            const auto* source_table =
                reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(source_table_u64));
            const std::uint32_t output_group = *reinterpret_cast<const std::uint32_t*>(engine1_state + 120u + 4u * group);
            if (output_group >= 3u)
                continue;
            float* out = output_0x300 + static_cast<std::size_t>(output_group) * 64u;
            const auto* entries1 = source_table;
            const auto* entries2 = source_table + 784u;
            for (std::size_t vec = 0; vec < 8u; ++vec)
                engine1_sum_98_taps_to_vec_5117d8_partial(
                    ring,
                    history_offset + static_cast<std::uint32_t>(4u * vec),
                    entries1,
                    out + 4u * vec);
            for (std::size_t vec = 0; vec < 8u; ++vec)
                engine1_sum_98_taps_to_vec_5117d8_partial(
                    ring,
                    history_offset + static_cast<std::uint32_t>(4u * vec),
                    entries2,
                    out + 32u + 4u * vec);
        }
    }

    *reinterpret_cast<std::uint32_t*>(engine1_state + 40u) = (next_write_index + 1u) % 0xEEu;
    *reinterpret_cast<std::uint32_t*>(engine1_state + 68u) = current_clear + 1u;
}

void auro_matic_XinN_Early_fl32_process_mode1_5773e0_partial(
    std::uint8_t* early_state,
    void** channel_span_31,
    float* output_0x300,
    const float* gains_32) {
    if (!early_state || !channel_span_31 || !output_0x300 || !gains_32)
        return;
    const auto* left = static_cast<const float*>(channel_span_31[0]);
    const auto* right = static_cast<const float*>(channel_span_31[1]);
    if (!left || !right)
        return;
    auro_matic_Engine1_fl32_process_ext_5117d8_partial(early_state, left, right, output_0x300, gains_32);
}

void add_scaled_channel_32_5776e0_partial(float* dst, const void* src_raw, float gain) {
    if (!dst || !src_raw)
        return;
    const auto* src = static_cast<const float*>(src_raw);
    for (std::size_t i = 0; i < 32u; ++i)
        dst[i] += src[i] * gain;
}

void auro_matic_XinN_Early_fl32_process_mode2_5776e0_partial(
    std::uint8_t* early_state,
    void** channel_span_31,
    float* output_a_0x300,
    float* output_b_0x300,
    const float* gains_32) {
    if (!early_state || !channel_span_31 || !output_a_0x300 || !output_b_0x300 || !gains_32)
        return;
    const auto* left = static_cast<const float*>(channel_span_31[0]);
    const auto* right = static_cast<const float*>(channel_span_31[1]);
    if (left && right)
        auro_matic_Engine1_fl32_process_ext_5117d8_partial(early_state, left, right, output_a_0x300, gains_32);

    float downmix_a[32]{};
    float downmix_b[32]{};
    const float gain_surround = *reinterpret_cast<const float*>(early_state + 320u);
    const float gain_center = *reinterpret_cast<const float*>(early_state + 324u);
    const float gain_back = *reinterpret_cast<const float*>(early_state + 328u);

    add_scaled_channel_32_5776e0_partial(downmix_a, channel_span_31[4], gain_surround);
    add_scaled_channel_32_5776e0_partial(downmix_b, channel_span_31[5], gain_surround);
    if (*reinterpret_cast<const std::uint32_t*>(early_state + 280u) != 0u) {
        add_scaled_channel_32_5776e0_partial(downmix_a, channel_span_31[6], gain_center);
        add_scaled_channel_32_5776e0_partial(downmix_b, channel_span_31[6], gain_center);
    }
    if (*reinterpret_cast<const std::uint32_t*>(early_state + 276u) != 0u) {
        add_scaled_channel_32_5776e0_partial(downmix_a, channel_span_31[7], gain_back);
        add_scaled_channel_32_5776e0_partial(downmix_b, channel_span_31[8], gain_back);
    }
    auro_matic_Engine1_fl32_process_ext_5117d8_partial(
        early_state + 136u,
        downmix_a,
        downmix_b,
        output_b_0x300,
        gains_32);
}

void auro_matic_XinN_Early_fl32_set_total_clear_frames_5772e0_partial(
    std::uint8_t* early_state,
    std::uint32_t frames) {
    if (!early_state)
        return;
    auro_matic_Engine1_fl32_set_total_clear_frames_511698_partial(early_state, frames);
    if (*reinterpret_cast<const std::uint32_t*>(early_state + 272u) != 0u)
        auro_matic_Engine1_fl32_set_total_clear_frames_511698_partial(early_state + 136u, frames);
}

void auro_matic_XinN_Early_fl32_set_downmix_577e90_partial(
    std::uint8_t* early_state,
    const std::uint8_t* downmix_plan) {
    if (!early_state || !downmix_plan)
        return;
    *reinterpret_cast<std::uint64_t*>(early_state + 284u) =
        *reinterpret_cast<const std::uint64_t*>(downmix_plan + 0u);
    *reinterpret_cast<std::uint32_t*>(early_state + 292u) =
        *reinterpret_cast<const std::uint32_t*>(downmix_plan + 8u);
    *reinterpret_cast<std::uint64_t*>(early_state + 320u) =
        *reinterpret_cast<const std::uint64_t*>(downmix_plan + 0u);
    *reinterpret_cast<float*>(early_state + 328u) =
        *reinterpret_cast<const float*>(downmix_plan + 8u);
}

std::int64_t auro_matic_XinN_Early_fl32_reset_audio_state_577f00_partial(std::uint8_t* early_state) {
    if (!early_state)
        return 0;
    std::int64_t result = auro_matic_Engine1_fl32_reset_audio_state_511760_partial(early_state);
    if (*reinterpret_cast<const std::uint32_t*>(early_state + 272u) != 0u)
        result = auro_matic_Engine1_fl32_reset_audio_state_511760_partial(early_state + 136u);
    return result;
}

bool auro_matic_XinN_Early_fl32_partial_clear_577f40_partial(
    std::uint8_t* early_state,
    std::uint32_t* remaining) {
    if (!early_state || !remaining)
        return false;
    const bool first_done = auro_matic_Engine1_fl32_partial_clear_5116ec_partial(early_state, remaining);
    if (!first_done)
        return false;
    if (*reinterpret_cast<const std::uint32_t*>(early_state + 272u) == 0u)
        return true;
    return auro_matic_Engine1_fl32_partial_clear_5116ec_partial(early_state + 136u, remaining);
}

void auro_matic_XinN_Early_fl32_get_delayed_mode1_577400_partial(
    std::uint8_t* engine1_state,
    float* out_near_32,
    float* out_far_32) {
    if (!engine1_state || !out_near_32 || !out_far_32)
        return;
    std::uint64_t near_ptr = 0;
    std::uint64_t far_ptr = 0;
    auro_matic_Engine1_fl32_get_delayed_frame_511784_partial(engine1_state, &near_ptr, &far_ptr);
    if (near_ptr != 0u)
        std::memcpy(out_near_32, reinterpret_cast<const void*>(static_cast<std::uintptr_t>(near_ptr)), 32u * sizeof(float));
    if (far_ptr != 0u)
        std::memcpy(out_far_32, reinterpret_cast<const void*>(static_cast<std::uintptr_t>(far_ptr)), 32u * sizeof(float));
}

void auro_matic_XinN_Early_fl32_mix_delayed_mode2_577bc0_partial(
    std::uint8_t* early_state,
    float* out_near_32,
    float* out_far_32) {
    if (!early_state || !out_near_32 || !out_far_32)
        return;
    std::uint64_t a_near_ptr = 0;
    std::uint64_t a_far_ptr = 0;
    std::uint64_t b_near_ptr = 0;
    std::uint64_t b_far_ptr = 0;
    auro_matic_Engine1_fl32_get_delayed_frame_511784_partial(early_state, &a_near_ptr, &a_far_ptr);
    auro_matic_Engine1_fl32_get_delayed_frame_511784_partial(early_state + 136u, &b_near_ptr, &b_far_ptr);
    if (a_near_ptr == 0u || a_far_ptr == 0u || b_near_ptr == 0u || b_far_ptr == 0u)
        return;
    const auto* a_near = reinterpret_cast<const float*>(static_cast<std::uintptr_t>(a_near_ptr));
    const auto* a_far = reinterpret_cast<const float*>(static_cast<std::uintptr_t>(a_far_ptr));
    const auto* b_near = reinterpret_cast<const float*>(static_cast<std::uintptr_t>(b_near_ptr));
    const auto* b_far = reinterpret_cast<const float*>(static_cast<std::uintptr_t>(b_far_ptr));
    const float gain_a = *reinterpret_cast<const float*>(early_state + 332u);
    const float gain_b = *reinterpret_cast<const float*>(early_state + 336u);
    for (std::size_t i = 0; i < 32u; ++i) {
        out_near_32[i] = gain_a * a_near[i] + gain_b * b_near[i];
        out_far_32[i] = gain_a * a_far[i] + gain_b * b_far[i];
    }
}

namespace {

constexpr std::uint32_t kEngine2DelayRingFloats = 94640u; // 0x5C6C0 bytes.
constexpr std::uint32_t kEngine2AllocatedFloats = 94704u;  // 0x5C7C0 bytes.
constexpr std::uint32_t kEngine2OutputFloats = 192u;       // 0x300 bytes.
constexpr std::uint32_t kEngine2FrameFloats = 32u;
constexpr std::array<std::uint32_t, 48> kEngine2WriteXarTaps_28af34 = {
    148u,   1396u,  5544u,  5676u,  6924u,  11072u, 11204u, 12452u,
    16600u, 16732u, 17980u, 22128u, 22260u, 23508u, 27656u, 27788u,
    29036u, 33184u, 33316u, 34564u, 38712u, 38844u, 40092u, 44240u,
    44372u, 45620u, 49768u, 49900u, 51148u, 55296u, 55428u, 56676u,
    60824u, 60956u, 62204u, 66352u, 66484u, 67732u, 71880u, 72012u,
    73260u, 77408u, 77540u, 78788u, 82936u, 83068u, 84316u, 88464u,
};
constexpr std::array<std::uint32_t, 6> kEngine2WritePostTaps_28aff4 = {
    89496u, 91560u, 93624u, 90528u, 92592u, 16u,
};
constexpr std::array<float, 34> kEngine2MutedBus_fl32{};

std::uint64_t e2_read_u64(const std::uint8_t* p, std::uintptr_t off) {
    return *reinterpret_cast<const std::uint64_t*>(p + off);
}

std::uint32_t e2_read_u32(const std::uint8_t* p, std::uintptr_t off) {
    return *reinterpret_cast<const std::uint32_t*>(p + off);
}

float e2_read_f32(const std::uint8_t* p, std::uintptr_t off) {
    return *reinterpret_cast<const float*>(p + off);
}

void e2_write_u64(std::uint8_t* p, std::uintptr_t off, std::uint64_t v) {
    *reinterpret_cast<std::uint64_t*>(p + off) = v;
}

void e2_write_u32(std::uint8_t* p, std::uintptr_t off, std::uint32_t v) {
    *reinterpret_cast<std::uint32_t*>(p + off) = v;
}

void e2_write_f32(std::uint8_t* p, std::uintptr_t off, float v) {
    *reinterpret_cast<float*>(p + off) = v;
}

std::uint8_t* e2_ptr(std::uint64_t p) {
    return reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(p));
}

const std::uint8_t* e2_cptr(std::uint64_t p) {
    return reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(p));
}

float* e2_ring_base(std::uint8_t* state) {
    return reinterpret_cast<float*>(static_cast<std::uintptr_t>(e2_read_u64(state, 64u)));
}

std::uint32_t e2_current_index(std::uint8_t* state, float* ring) {
    const std::uint64_t cur_u64 = e2_read_u64(state, 72u);
    if (!ring || cur_u64 == 0u)
        return 0u;
    const auto* cur = reinterpret_cast<const float*>(static_cast<std::uintptr_t>(cur_u64));
    const auto delta = cur - ring;
    if (delta < 0 || static_cast<std::uint64_t>(delta) >= kEngine2DelayRingFloats)
        return 0u;
    return static_cast<std::uint32_t>(delta);
}

void e2_set_current_index(std::uint8_t* state, float* ring, std::uint32_t index) {
    if (!ring)
        return;
    index %= kEngine2DelayRingFloats;
    e2_write_u64(state, 72u, reinterpret_cast<std::uint64_t>(ring + index));
}

std::uint32_t e2_wrap_index(std::uint32_t index) {
    return index % kEngine2DelayRingFloats;
}

void e2_read_ring_frame(const float* ring, std::uint32_t current, std::uint32_t tap, float* dst_32) {
    if (!ring || !dst_32)
        return;
    std::uint32_t idx = e2_wrap_index(current + tap);
    for (std::uint32_t i = 0; i < kEngine2FrameFloats; ++i)
        dst_32[i] = ring[e2_wrap_index(idx + i)];
}

void e2_write_ring_frame(float* ring, std::uint32_t current, std::uint32_t tap, const float* src_32) {
    if (!ring || !src_32)
        return;
    std::uint32_t idx = e2_wrap_index(current + tap);
    for (std::uint32_t i = 0; i < kEngine2FrameFloats; ++i)
        ring[e2_wrap_index(idx + i)] = src_32[i];
}

const std::uint8_t* e2_bus_or_muted(std::uint64_t bus_ptr) {
    if (bus_ptr != 0u)
        return e2_cptr(bus_ptr);
    return reinterpret_cast<const std::uint8_t*>(kEngine2MutedBus_fl32.data());
}

float e2_bus_weight(const std::uint8_t* bus, std::uint32_t index) {
    return e2_read_f32(bus, 4ull * index);
}

std::uint32_t e2_bus_delay(const std::uint8_t* bus) {
    return e2_read_u32(bus, 128u);
}

float e2_bus_post_gain(const std::uint8_t* bus) {
    return e2_read_f32(bus, 132u);
}

void e2_accumulate_frame(float* dst_32, const float* src_32, float gain) {
    if (!dst_32 || !src_32 || gain == 0.0f)
        return;
    for (std::uint32_t i = 0; i < kEngine2FrameFloats; ++i)
        dst_32[i] += src_32[i] * gain;
}

void e2_zero_progressive_clear(std::uint8_t* state, float* ring) {
    const std::uint32_t total = e2_read_u32(state, 92u);
    const std::uint32_t counter = e2_read_u32(state, 96u);
    if (!ring)
        return;
    if ((total | counter) == 0u) {
        std::memset(ring, 0, kEngine2AllocatedFloats * sizeof(float));
        e2_write_u32(state, 80u, 0u);
        e2_set_current_index(state, ring, 0u);
        return;
    }
    if (total == 0u || counter >= total)
        return;
    const std::uint32_t step = 0xB8Fu / total;
    const std::uint32_t start = step * counter;
    const std::uint32_t count = (counter + 1u == total) ? (0xB8Fu - start) : step;
    const std::uint32_t first = std::min<std::uint32_t>(start * kEngine2FrameFloats, kEngine2AllocatedFloats);
    const std::uint32_t avail = kEngine2AllocatedFloats - first;
    const std::uint32_t clear = std::min<std::uint32_t>(count * kEngine2FrameFloats, avail);
    if (clear != 0u)
        std::memset(ring + first, 0, clear * sizeof(float));
}

void e2_read_post_frames(std::uint8_t* state, float* ring, const std::array<std::uint32_t, 3>& delays, float* dst_192) {
    if (!state || !ring || !dst_192)
        return;
    const std::uint32_t current = e2_current_index(state, ring);
    std::uint32_t base = 89480u;
    for (std::uint32_t i = 0; i < 6u; ++i, base += 1032u) {
        const std::uint32_t delay = delays[i >> 1u];
        const std::uint32_t tap = e2_wrap_index(base + kEngine2DelayRingFloats - (delay % kEngine2DelayRingFloats));
        e2_read_ring_frame(ring, current, tap, dst_192 + i * kEngine2FrameFloats);
    }
}

void e2_write_post_frames(std::uint8_t* state, float* ring, const float* src_192) {
    if (!state || !ring || !src_192)
        return;
    const std::uint32_t current = e2_current_index(state, ring);
    for (std::uint32_t i = 0; i < 6u; ++i)
        e2_write_ring_frame(ring, current, kEngine2WritePostTaps_28aff4[i], src_192 + i * kEngine2FrameFloats);
}

void e2_write_xar_frames(std::uint8_t* state, float* ring, std::uint32_t index, const float* a, const float* b, const float* c) {
    if (!state || !ring || index >= 16u)
        return;
    const std::uint32_t current = e2_current_index(state, ring);
    const std::uint32_t tap_base = index * 3u;
    e2_write_ring_frame(ring, current, kEngine2WriteXarTaps_28af34[tap_base + 0u], a);
    e2_write_ring_frame(ring, current, kEngine2WriteXarTaps_28af34[tap_base + 1u], b);
    e2_write_ring_frame(ring, current, kEngine2WriteXarTaps_28af34[tap_base + 2u], c);
}

void e2_reverb_process_57ee40_partial(
    std::uint8_t* state,
    float* ring,
    const std::uint8_t* preset,
    std::uint32_t index,
    const float* cross_64,
    float* out_32) {
    if (!state || !ring || !preset || !cross_64 || !out_32 || index >= 16u) {
        if (out_32)
            std::memset(out_32, 0, kEngine2FrameFloats * sizeof(float));
        return;
    }

    const auto* p = reinterpret_cast<const float*>(preset + 48ull * index);
    const std::uint32_t current = e2_current_index(state, ring);
    const std::uint32_t coeff_tap = e2_read_u32(preset, 768ull + 4ull * index);
    const std::uint32_t input_tap = e2_read_u32(preset, 832ull + 4ull * index);

    float coeff[64]{};
    float ring_in[32]{};
    e2_read_ring_frame(ring, current, coeff_tap, coeff);
    e2_read_ring_frame(ring, current, coeff_tap + 32u, coeff + 32u);
    e2_read_ring_frame(ring, current, input_tap, ring_in);

    float pre[32]{};
    float z = e2_read_f32(state, 4ull * index);
    for (std::uint32_t i = 0; i < kEngine2FrameFloats; ++i) {
        const float first = p[0] * coeff[i + 29u] + 0.0f;
        const float pair = p[0] * coeff[i + 30u] + first;
        z = z * p[1] + pair;
        pre[i] = z;
    }
    e2_write_f32(state, 4ull * index, z);

    const float* feed = cross_64 + ((index < 8u) ? 0u : 32u);
    float out_b[32]{};
    float out_c[32]{};
    for (std::uint32_t i = 0; i < kEngine2FrameFloats; ++i) {
        float filter = pre[i] * p[2];
        filter = coeff[i + 26u] * p[3] + filter;
        filter = coeff[i + 27u] * p[4] + filter;
        filter = coeff[i + 28u] * p[5] + filter;
        filter = coeff[i + 29u] * p[6] + filter;
        filter = coeff[i + 30u] * p[7] + filter;
        filter = coeff[i + 31u] * p[8] + filter;
        filter = coeff[i + 32u] * p[9] + filter;
        const float old = ring_in[i];
        const float delayed = p[10] * old;
        out_32[i] = delayed * p[11] + filter;
        out_b[i] = (feed[i] + filter) + delayed;
        out_c[i] = p[10] * out_b[i] - old;
    }
    e2_write_xar_frames(state, ring, index, out_32, out_b, out_c);
}

void e2_build_cross_input(std::uint8_t* state, const float* route_a, const float* route_b, float* cross_64) {
    if (!state || !route_a || !route_b || !cross_64)
        return;
    const auto* preset = e2_cptr(e2_read_u64(state, 104u));
    const float cross_gain = preset ? e2_read_f32(preset, 960u) : 0.0f;
    for (std::uint32_t i = 0; i < 32u; ++i) {
        cross_64[i] = route_a[i] + route_b[i] * cross_gain;
        cross_64[i + 32u] = route_b[i] + route_a[i] * cross_gain;
    }

    if (!preset)
        return;
    float z0 = e2_read_f32(state, 84u);
    float z1 = e2_read_f32(state, 88u);
    const float c0 = e2_read_f32(preset, 964u);
    const float c1 = e2_read_f32(preset, 968u);
    const float c2 = e2_read_f32(preset, 972u);
    for (std::uint32_t i = 0; i < 32u; ++i) {
        const float x0 = cross_64[i];
        const float x1 = cross_64[i + 32u];
        const float y0 = z0 + c0 * x0;
        const float y1 = z1 + c0 * x1;
        cross_64[i] = y0;
        cross_64[i + 32u] = y1;
        z0 = c1 * x0 - c2 * y0;
        z1 = c1 * x1 - c2 * y1;
    }
    e2_write_f32(state, 84u, z0);
    e2_write_f32(state, 88u, z1);
}

void e2_apply_output_patch_57b0e0(
    std::uint8_t* state,
    float* ring,
    const std::uint8_t* preset,
    const std::array<const std::uint8_t*, 3>& bus,
    float* accum_192) {
    if (!state || !ring || !preset || !accum_192)
        return;
    const std::uint32_t current = e2_current_index(state, ring);
    for (std::uint32_t i = 0; i < 8u; ++i) {
        float frame[32]{};
        e2_read_ring_frame(ring, current, e2_read_u32(preset, 896ull + 32ull + 4ull * i), frame);
        e2_accumulate_frame(accum_192 + 0u, frame, e2_bus_weight(bus[0], 4u * i + 1u));
        e2_accumulate_frame(accum_192 + 32u, frame, e2_bus_weight(bus[1], 4u * i + 1u));
        e2_accumulate_frame(accum_192 + 64u, frame, e2_bus_weight(bus[2], 4u * i + 1u));

        e2_read_ring_frame(ring, current, e2_read_u32(preset, 896ull + 4ull * i), frame);
        e2_accumulate_frame(accum_192 + 96u, frame, e2_bus_weight(bus[0], 4u * i + 2u));
        e2_accumulate_frame(accum_192 + 128u, frame, e2_bus_weight(bus[1], 4u * i + 2u));
        e2_accumulate_frame(accum_192 + 160u, frame, e2_bus_weight(bus[2], 4u * i + 2u));
    }
}

} // namespace

std::int64_t auro_matic_Engine2_fl32_construct_57b000_partial(std::uint64_t engine2_state, std::uint64_t memory) {
    if (engine2_state == 0u || memory == 0u)
        return 0;
    auto* state = e2_ptr(engine2_state);
    e2_write_u64(state, 104u, 0u);
    e2_write_u64(state, 64u, memory + 128u);
    e2_write_u64(state, 72u, memory + 128u);
    e2_write_u32(state, 80u, kEngine2AllocatedFloats);
    return static_cast<std::int64_t>(engine2_state);
}

void auro_matic_Engine2_fl32_set_total_clear_frames_57aff0_partial(
    std::uint64_t engine2_state,
    std::uint32_t frames) {
    if (engine2_state == 0u)
        return;
    e2_write_u32(e2_ptr(engine2_state), 92u, frames);
}

void auro_matic_Engine2_fl32_set_preset_57b040_partial(std::uint64_t engine2_state, std::uint64_t preset) {
    if (engine2_state == 0u)
        return;
    e2_write_u64(e2_ptr(engine2_state), 104u, preset);
}

std::int64_t auro_matic_Engine2_fl32_reset_audio_state_57b050_partial(std::uint64_t engine2_state) {
    if (engine2_state == 0u)
        return 0;
    auto* state = e2_ptr(engine2_state);
    const std::uint32_t remaining = e2_read_u32(state, 80u);
    std::memset(state, 0, 64u);
    e2_write_u32(state, 84u, 0u);
    e2_write_u32(state, 88u, 0u);
    const std::uint32_t clear_index = remaining == 0u ? e2_read_u32(state, 92u) : 0u;
    e2_write_u32(state, 96u, clear_index);
    return clear_index;
}

bool auro_matic_Engine2_fl32_partial_clear_57b080_partial(std::uint64_t engine2_state, std::uint32_t* remaining) {
    if (engine2_state == 0u || !remaining)
        return false;
    auto* state = e2_ptr(engine2_state);
    std::uint32_t pending = e2_read_u32(state, 80u);
    const std::uint32_t clear = std::min(pending, *remaining);
    if (clear != 0u) {
        float* ring = e2_ring_base(state);
        if (ring)
            std::memset(ring + pending - clear, 0, clear * sizeof(float));
    }
    pending -= clear;
    e2_write_u32(state, 80u, pending);
    *remaining -= clear;
    return pending == 0u;
}

void auro_matic_Engine2_fl32_set_output_patch_57c2f0_partial(
    std::uint64_t engine2_state,
    const std::uint32_t* patch_3) {
    if (engine2_state == 0u || !patch_3)
        return;
    auto* state = e2_ptr(engine2_state);
    const std::uint64_t preset = e2_read_u64(state, 104u);
    if (preset == 0u)
        return;
    const auto* preset_p = e2_cptr(preset);
    const std::uint64_t muted = reinterpret_cast<std::uint64_t>(kEngine2MutedBus_fl32.data());
    for (std::uint32_t i = 0; i < 3u; ++i) {
        std::uint64_t bus = muted;
        const std::uint32_t sel = patch_3[i];
        if (sel >= 1u && sel <= 3u)
            bus = e2_read_u64(preset_p, 976ull + 8ull * (sel - 1u));
        if (bus == 0u)
            bus = muted;
        e2_write_u64(state, 112ull + 8ull * i, bus);
    }
}

void* auro_matic_Engine2_fl32_process_57b0e0_partial(
    std::uint64_t engine2_state,
    void* route_a_0x80,
    void* route_b_0x80,
    void* output_0x300) {
    if (engine2_state == 0u || !output_0x300)
        return output_0x300;
    auto* state = e2_ptr(engine2_state);
    auto* output = static_cast<float*>(output_0x300);
    float* ring = e2_ring_base(state);
    const std::uint32_t total_clear = e2_read_u32(state, 92u);
    std::uint32_t counter = e2_read_u32(state, 96u);

    if (!ring || !route_a_0x80 || !route_b_0x80) {
        std::memset(output, 0, kEngine2OutputFloats * sizeof(float));
        e2_write_u32(state, 96u, counter + 1u);
        return output_0x300;
    }

    e2_zero_progressive_clear(state, ring);
    if (counter < total_clear) {
        std::memset(output, 0, kEngine2OutputFloats * sizeof(float));
        e2_write_u32(state, 96u, counter + 1u);
        return output_0x300;
    }

    e2_write_u32(state, 80u, kEngine2AllocatedFloats);
    const auto* preset = e2_cptr(e2_read_u64(state, 104u));
    std::array<const std::uint8_t*, 3> bus = {
        e2_bus_or_muted(e2_read_u64(state, 112u)),
        e2_bus_or_muted(e2_read_u64(state, 120u)),
        e2_bus_or_muted(e2_read_u64(state, 128u)),
    };

    float cross[64]{};
    e2_build_cross_input(
        state,
        static_cast<const float*>(route_a_0x80),
        static_cast<const float*>(route_b_0x80),
        cross);

    float delayed_post[192]{};
    const std::array<std::uint32_t, 3> post_delays = {
        e2_bus_delay(bus[0]),
        e2_bus_delay(bus[1]),
        e2_bus_delay(bus[2]),
    };
    e2_read_post_frames(state, ring, post_delays, delayed_post);
    for (std::uint32_t group = 0; group < 3u; ++group) {
        const float gain = e2_bus_post_gain(bus[group]);
        const std::uint32_t base = group * 64u;
        for (std::uint32_t i = 0; i < 64u; ++i)
            output[base + i] = delayed_post[base + i] * gain;
    }

    float accum[192]{};
    if (preset) {
        for (std::uint32_t i = 0; i < 8u; ++i) {
            float frame[32]{};
            e2_reverb_process_57ee40_partial(state, ring, preset, i, cross, frame);
            e2_accumulate_frame(accum + 0u, frame, e2_bus_weight(bus[0], 4u * i + 0u));
            e2_accumulate_frame(accum + 32u, frame, e2_bus_weight(bus[1], 4u * i + 0u));
            e2_accumulate_frame(accum + 64u, frame, e2_bus_weight(bus[2], 4u * i + 0u));

            e2_reverb_process_57ee40_partial(state, ring, preset, i | 8u, cross, frame);
            e2_accumulate_frame(accum + 96u, frame, e2_bus_weight(bus[0], 4u * i + 3u));
            e2_accumulate_frame(accum + 128u, frame, e2_bus_weight(bus[1], 4u * i + 3u));
            e2_accumulate_frame(accum + 160u, frame, e2_bus_weight(bus[2], 4u * i + 3u));
        }
        e2_apply_output_patch_57b0e0(state, ring, preset, bus, accum);
        e2_write_post_frames(state, ring, accum);
    }

    const std::uint32_t next_index = (e2_current_index(state, ring) + kEngine2FrameFloats) % kEngine2DelayRingFloats;
    e2_set_current_index(state, ring, next_index);
    counter = e2_read_u32(state, 96u);
    e2_write_u32(state, 96u, counter + 1u);
    return output_0x300;
}

std::int64_t auro_matic_XinN_Late_fl32_construct_579920_partial(std::uint32_t* late_state, std::uint64_t memory) {
    if (!late_state)
        return 0;
    late_state[0] = memory != 0u ? 1u : 0u;
    if (memory != 0u)
        return auro_matic_Engine2_fl32_construct_57b000_partial(
            reinterpret_cast<std::uint64_t>(late_state + 2),
            memory);
    return 0;
}

std::int64_t auro_matic_XinN_Late_fl32_reset_audio_state_579940_partial(std::uint32_t* late_state) {
    if (!late_state)
        return 0;
    late_state[37] = 0u;
    if (late_state[0] != 0u)
        return auro_matic_Engine2_fl32_reset_audio_state_57b050_partial(reinterpret_cast<std::uint64_t>(late_state + 2));
    return 0;
}

void auro_matic_XinN_Late_fl32_set_clear_frames_579960_partial(
    std::uint32_t* late_state,
    std::uint32_t early_clear_frames,
    std::uint32_t late_clear_frames) {
    if (!late_state)
        return;
    late_state[36] = early_clear_frames;
    auro_matic_Engine2_fl32_set_total_clear_frames_57aff0_partial(
        reinterpret_cast<std::uint64_t>(late_state + 2),
        late_clear_frames);
}

bool auro_matic_XinN_Late_fl32_partial_clear_579980_partial(std::uint32_t* late_state, std::uint32_t* remaining) {
    if (!late_state)
        return false;
    return late_state[0] == 0u
        || auro_matic_Engine2_fl32_partial_clear_57b080_partial(
            reinterpret_cast<std::uint64_t>(late_state + 2),
            remaining);
}

void auro_matic_XinN_Late_fl32_set_preset_579a30_partial(std::uint32_t* late_state, std::uint64_t preset) {
    if (!late_state || late_state[0] == 0u || preset == 0u)
        return;
    auro_matic_Engine2_fl32_set_preset_57b040_partial(
        reinterpret_cast<std::uint64_t>(late_state + 2),
        preset + 64u);
}

void auro_matic_XinN_Late_fl32_set_output_patch_579a50_partial(std::uint32_t* late_state, const std::uint32_t* patch_3) {
    if (!late_state || late_state[0] == 0u)
        return;
    auro_matic_Engine2_fl32_set_output_patch_57c2f0_partial(
        reinterpret_cast<std::uint64_t>(late_state + 2),
        patch_3);
}

void* auro_matic_XinN_Late_fl32_process_5799a0_partial(
    std::uint32_t* late_state,
    std::uint64_t engine_state,
    void* output_0x300,
    AuroMaticEngine2Fl32ProcessFn engine2_process) {
    if (!late_state || !output_0x300)
        return output_0x300;

    std::array<std::uint8_t, 128> route_a{};
    std::array<std::uint8_t, 128> route_b{};
    void* result = output_0x300;
    if (late_state[0] != 0u && late_state[37] >= late_state[36] && engine_state != 0u && engine2_process) {
        using RoutingFn = void (*)(std::uint64_t, void*, void*);
        const std::uint64_t route_u64 = *reinterpret_cast<const std::uint64_t*>(engine_state + 304u);
        if (route_u64 == 0x577400u) {
            auro_matic_XinN_Early_fl32_get_delayed_mode1_577400_partial(
                reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(engine_state)),
                reinterpret_cast<float*>(route_a.data()),
                reinterpret_cast<float*>(route_b.data()));
        } else if (route_u64 == 0x577BC0u) {
            auro_matic_XinN_Early_fl32_mix_delayed_mode2_577bc0_partial(
                reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(engine_state)),
                reinterpret_cast<float*>(route_a.data()),
                reinterpret_cast<float*>(route_b.data()));
        } else {
            const auto route = reinterpret_cast<RoutingFn>(static_cast<std::uintptr_t>(route_u64));
            if (route)
                route(engine_state, route_a.data(), route_b.data());
        }
        auto* process = engine2_process ? engine2_process : auro_matic_Engine2_fl32_process_57b0e0_partial;
        result = process(
            reinterpret_cast<std::uint64_t>(late_state + 2),
            route_a.data(),
            route_b.data(),
            output_0x300);
    } else {
        std::memset(output_0x300, 0, 0x300u);
    }
    ++late_state[37];
    return result;
}

std::int64_t auro_matic_Engine1_fl32_construct_583d10_partial(std::uint8_t* engine1_state, std::uint64_t memory) {
    if (!engine1_state || memory == 0u)
        return 0;
    *reinterpret_cast<std::uint32_t*>(engine1_state + 132u) = 0u;
    *reinterpret_cast<std::uint32_t*>(engine1_state + 40u) = 0u;
    *reinterpret_cast<std::uint64_t*>(engine1_state + 16u) = memory + 0x7800u;
    *reinterpret_cast<std::uint64_t*>(engine1_state + 0u) = 0u;
    *reinterpret_cast<std::uint64_t*>(engine1_state + 8u) = memory;
    *reinterpret_cast<std::uint32_t*>(engine1_state + 60u) = 7648u;
    *reinterpret_cast<std::uint64_t*>(engine1_state + 24u) = memory + 0x8440u;
    *reinterpret_cast<std::uint64_t*>(engine1_state + 32u) = memory + 0x9080u;
    return reinterpret_cast<std::int64_t>(engine1_state);
}

void engine1_copy_preset_table_584ca0_partial(std::uint64_t dst_u64, std::uint64_t src_u64) {
    if (dst_u64 == 0u || src_u64 == 0u)
        return;
    auto* dst = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(dst_u64));
    const auto* src = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(src_u64));
    for (std::uint32_t off = 0; off != 784u; off += 8u) {
        const std::uint64_t v = *reinterpret_cast<const std::uint64_t*>(src + off);
        *reinterpret_cast<std::uint64_t*>(dst + off) = v;
        *reinterpret_cast<std::uint64_t*>(dst + off + 784u) = v;
        *reinterpret_cast<std::uint32_t*>(dst + off + 784u) = static_cast<std::uint32_t>(v + 7616u);
    }
    for (std::uint32_t off = 0; off != 784u; off += 8u) {
        const std::uint64_t v = *reinterpret_cast<const std::uint64_t*>(src + 784u + off);
        *reinterpret_cast<std::uint64_t*>(dst + 1568u + off) = v;
        *reinterpret_cast<std::uint64_t*>(dst + 2352u + off) = v;
        *reinterpret_cast<std::uint32_t*>(dst + 2352u + off) = static_cast<std::uint32_t>(v + 7616u);
    }
}

void auro_matic_Engine1_fl32_set_preset_584ca0_partial(std::uint8_t* engine1_state, std::uint64_t preset) {
    if (!engine1_state)
        return;
    *reinterpret_cast<std::uint64_t*>(engine1_state + 0u) = preset;
    if (preset == 0u)
        return;
    const auto* p = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(preset));
    for (std::uint32_t table = 0; table < 3u; ++table) {
        const std::uint64_t src = *reinterpret_cast<const std::uint64_t*>(p + 32u + 8u * table);
        const std::uint64_t dst = *reinterpret_cast<const std::uint64_t*>(engine1_state + 16u + 8u * table);
        engine1_copy_preset_table_584ca0_partial(dst, src);
    }
}

std::int64_t auro_matic_Engine1_fl32_set_output_patch_584ac0_partial(std::uint8_t* engine1_state, const std::uint32_t* patch_3) {
    if (!engine1_state || !patch_3)
        return 0;
    *reinterpret_cast<std::uint32_t*>(engine1_state + 132u) = 0u;
    const std::uint64_t preset = *reinterpret_cast<const std::uint64_t*>(engine1_state + 0u);
    if (preset == 0u)
        return 0;
    const auto* p = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(preset));
    std::uint32_t group_count = 0u;
    std::uint64_t result = preset;
    for (std::uint32_t output_group = 0; output_group < 3u; ++output_group) {
        const std::uint32_t selector = patch_3[output_group];
        if (selector < 1u || selector > 3u) {
            if (output_group == 2u)
                result = 0u;
            continue;
        }
        const std::uint32_t table = selector - 1u;
        const std::uint64_t coeff_table = *reinterpret_cast<const std::uint64_t*>(p + 32u + 8u * table);
        if (output_group == 2u)
            result = coeff_table;
        if (coeff_table == 0u)
            continue;
        const std::uint64_t index_table = *reinterpret_cast<const std::uint64_t*>(engine1_state + 16u + 8u * table);
        *reinterpret_cast<std::uint64_t*>(engine1_state + 72u + 8u * group_count) = coeff_table;
        *reinterpret_cast<std::uint64_t*>(engine1_state + 96u + 8u * group_count) = index_table;
        *reinterpret_cast<std::uint32_t*>(engine1_state + 120u + 4u * group_count) = output_group;
        ++group_count;
        *reinterpret_cast<std::uint32_t*>(engine1_state + 132u) = group_count;
    }
    return static_cast<std::int64_t>(result);
}

std::int64_t auro_matic_XinN_Early_fl32_construct_577320_partial(
    std::uint8_t* early_state,
    std::uint64_t memory_a,
    std::uint64_t memory_b) {
    if (!early_state)
        return 0;
    std::int64_t result = auro_matic_Engine1_fl32_construct_583d10_partial(early_state, memory_a);
    *reinterpret_cast<std::uint32_t*>(early_state + 272u) = memory_b != 0u ? 1u : 0u;
    if (memory_b != 0u)
        result = auro_matic_Engine1_fl32_construct_583d10_partial(early_state + 136u, memory_b);
    *reinterpret_cast<std::uint64_t*>(early_state + 276u) = 0u;
    *reinterpret_cast<std::uint64_t*>(early_state + 312u) = 0u;
    return result;
}

std::int64_t auro_matic_XinN_Early_fl32_set_output_patch_577ec0_partial(
    std::uint8_t* early_state,
    const std::uint32_t* primary_patch_3,
    const std::uint32_t* secondary_patch_3) {
    if (!early_state)
        return 0;
    std::int64_t result = auro_matic_Engine1_fl32_set_output_patch_584ac0_partial(early_state, primary_patch_3);
    if (*reinterpret_cast<const std::uint32_t*>(early_state + 272u) != 0u)
        result = auro_matic_Engine1_fl32_set_output_patch_584ac0_partial(early_state + 136u, secondary_patch_3);
    return result;
}

std::uint64_t auro_matic_XinN_Early_fl32_set_preset_577f90_partial(std::uint8_t* early_state, std::uint64_t preset) {
    if (!early_state)
        return 0u;
    *reinterpret_cast<std::uint64_t*>(early_state + 312u) = preset;
    if (preset != 0u) {
        auro_matic_Engine1_fl32_set_preset_584ca0_partial(early_state, preset + 8u);
        if (*reinterpret_cast<const std::uint32_t*>(early_state + 272u) != 0u)
            auro_matic_Engine1_fl32_set_preset_584ca0_partial(early_state + 136u, preset + 8u);
    }
    if (preset != 0u) {
        *reinterpret_cast<std::uint64_t*>(early_state + 332u) =
            *reinterpret_cast<const std::uint64_t*>(static_cast<std::uintptr_t>(preset));
    }
    return preset;
}

void auro_matic_XinN_parameter_Dynamic_t_default_503fbc_partial(std::uint8_t* dynamic_48) {
    if (!dynamic_48)
        return;
    *reinterpret_cast<std::uint64_t*>(dynamic_48 + 0u) = 0x200000001ull;
    *reinterpret_cast<std::uint32_t*>(dynamic_48 + 8u) = 3u;
    *reinterpret_cast<std::uint64_t*>(dynamic_48 + 12u) = 0x200000001ull;
    *reinterpret_cast<std::uint32_t*>(dynamic_48 + 20u) = 3u;
    *reinterpret_cast<std::uint64_t*>(dynamic_48 + 24u) = 0x200000001ull;
    *reinterpret_cast<std::uint32_t*>(dynamic_48 + 32u) = 3u;
    *reinterpret_cast<std::uint64_t*>(dynamic_48 + 36u) = 0x3F3504F33F800000ull;
    *reinterpret_cast<std::uint32_t*>(dynamic_48 + 44u) = 0x3F800000u;
}

void auro_matic_v3_XinN_parameter_Dynamic_t_default_4e9fec_partial(std::uint8_t* dynamic_120) {
    if (!dynamic_120)
        return;
    auro_matic_XinN_parameter_Dynamic_t_default_503fbc_partial(dynamic_120);
    for (std::uint32_t i = 0; i < 4u; ++i)
        *reinterpret_cast<float*>(dynamic_120 + 48u + 4u * i) = 1.0f;
    const float routing_64[4] = {1.0f, 1.0f, 1.0f, 0.1883648931980133f};
    const float routing_80[4] = {1.0f, 0.7079457640647888f, 0.0f, 0.7079457640647888f};
    const float routing_96[4] = {0.1883648931980133f, 1.0f, 1.0f, 0.4216965138912201f};
    std::memcpy(dynamic_120 + 64u, routing_64, sizeof(routing_64));
    std::memcpy(dynamic_120 + 80u, routing_80, sizeof(routing_80));
    std::memcpy(dynamic_120 + 96u, routing_96, sizeof(routing_96));
    *reinterpret_cast<std::uint64_t*>(dynamic_120 + 112u) = 0x3F004DCE3F353BEFull;
}

void auro_matic_v3_XinN_Routing_fl32_construct_557420_partial(std::uint8_t* routing_state) {
    if (!routing_state)
        return;
    *reinterpret_cast<std::uint32_t*>(routing_state + 64u) = 0u;
    *reinterpret_cast<std::uint64_t*>(routing_state + 0u) = 0x557460u;
    *reinterpret_cast<std::uint64_t*>(routing_state + 8u) = 0x5575C0u;
    *reinterpret_cast<std::uint64_t*>(routing_state + 16u) = 0x5570C0u;
    *reinterpret_cast<std::uint64_t*>(routing_state + 24u) = 0x5570E0u;
}

std::uint64_t auro_matic_v3_XinN_Routing_fl32_set_routing_557620_partial(
    std::uint8_t* routing_state,
    const std::uint8_t* routing_72) {
    if (!routing_state || !routing_72)
        return 0u;
    std::memcpy(routing_state + 72u, routing_72, 72u);
    std::memcpy(routing_state + 144u, routing_72, 72u);
    return *reinterpret_cast<const std::uint64_t*>(routing_72 + 64u);
}

std::uint64_t auro_matic_v3_XinN_Routing_fl32_get_routing_557690_partial(
    const std::uint8_t* routing_state,
    std::uint8_t* routing_72) {
    if (!routing_state || !routing_72)
        return 0u;
    const std::uint64_t result = *reinterpret_cast<const std::uint64_t*>(routing_state + 136u);
    std::memcpy(routing_72, routing_state + 72u, 72u);
    return result;
}

std::int64_t auro_matic_v3_XinN_Routing_fl32_configure_557460_partial(
    std::uint8_t* routing_state,
    std::uint32_t mode,
    std::uint32_t output_mask,
    std::uint32_t input_mask) {
    (void)input_mask;
    if (!routing_state)
        return 0;
    if (((~output_mask & 0x180u) == 0u) || ((output_mask & 0x40u) != 0u))
        return 0;

    auto* count = reinterpret_cast<std::uint32_t*>(routing_state + 64u);
    auto* callbacks = reinterpret_cast<std::uint64_t*>(routing_state + 32u);
    *count = 0u;
    std::uint32_t n = 0u;
    const auto push = [&](std::uint64_t fn) {
        callbacks[n] = fn;
        ++n;
        *count = n;
    };

    if (mode == 1u) {
        if ((output_mask & 0x30u) == 0x30u)
            push(0x559810u);
        if ((output_mask & 0x600u) == 0x600u)
            push(0x559AB0u);
        if ((output_mask & 0x6000u) == 0x6000u)
            push(0x559D90u);
        return 1;
    }
    if (mode == 2u) {
        if ((output_mask & 0x600u) == 0x600u)
            push(0x55A070u);
        if ((output_mask & 0x6000u) == 0x6000u)
            push(0x55A4C0u);
        if ((output_mask & 0x800u) != 0u)
            push(0x55A910u);
        if ((output_mask & 0x1000u) != 0u)
            push(0x55AAC0u);
        return 1;
    }
    return 0;
}

namespace {

std::uint32_t xinn_supported_output_mask_4eb380(std::uint32_t mode) {
    if (mode == 2u)
        return 32256u;
    if (mode == 1u)
        return 26160u;
    return 0u;
}

constexpr std::size_t kXinnRuntimePresetSize_35bf40 = 0x428u;
constexpr std::size_t kXinnEngine1SourceTableSize_35c0b0 = 1568u;
constexpr std::size_t kXinnEngine2BusPresetSize_34bdd0 = 136u;
constexpr std::uint64_t kXinnSpreset2in6Base_570460 = 0x73F6B8u;
constexpr std::uint64_t kXinnSpreset5inNBase_570350 = 0x738078u;
constexpr std::uint32_t kXinnSpresetStride_35bf40 = 0x1D90u;
constexpr std::uint32_t kXinnProcessedPresetResourceId = 101u;
constexpr std::size_t kXinnProcessedPresetBytes = 23856u;
constexpr std::size_t kXinnProcessedPresetResourceBytes = 8u * kXinnProcessedPresetBytes;
constexpr std::uint8_t kXinnProcessedPresetMarker = 0xA5u;

#ifndef AURO3DENG_ENABLE_NATIVE_IMAGE_PRESET_POINTERS
#define AURO3DENG_ENABLE_NATIVE_IMAGE_PRESET_POINTERS 0
#endif

bool xinn_runtime_preset_pointer_available_partial(std::uint64_t runtime_preset) {
    if (runtime_preset == 0u)
        return false;
#if AURO3DENG_ENABLE_NATIVE_IMAGE_PRESET_POINTERS
    return true;
#else
    (void)runtime_preset;
    return false;
#endif
}

std::uint64_t xinn_select_native_spreset_from_v4_static_partial(
    bool surround_mode,
    const std::uint8_t* tuning_static) {
    if (!tuning_static)
        return 0u;
    const std::uint32_t room = std::min<std::uint32_t>(
        *reinterpret_cast<const std::uint32_t*>(tuning_static + 4u), 3u);
    const std::uint64_t base = surround_mode ? kXinnSpreset5inNBase_570350 : kXinnSpreset2in6Base_570460;
    return base + static_cast<std::uint64_t>(room) * kXinnSpresetStride_35bf40;
}

const std::uint8_t* xinn_portable_spreset_from_native_va_partial(std::uint64_t spreset_u64) {
    (void)spreset_u64;
    return nullptr;
}

const std::uint8_t* xinn_portable_processed_preset_from_native_va_partial(std::uint64_t spreset_u64) {
    std::uint32_t family = 0u;
    std::uint64_t base = kXinnSpreset2in6Base_570460;
    if (spreset_u64 >= kXinnSpreset2in6Base_570460) {
        family = 0u;
        base = kXinnSpreset2in6Base_570460;
    } else if (spreset_u64 >= kXinnSpreset5inNBase_570350) {
        family = 1u;
        base = kXinnSpreset5inNBase_570350;
    }
    if (spreset_u64 < base)
        return nullptr;
    const std::uint64_t delta = spreset_u64 - base;
    if ((delta % kXinnSpresetStride_35bf40) != 0u)
        return nullptr;
    const std::uint64_t room = delta / kXinnSpresetStride_35bf40;
    if (room >= 4u)
        return nullptr;
#ifdef _WIN32
    static const std::uint8_t* presets = []() -> const std::uint8_t* {
        const HRSRC resource = FindResourceA(
            nullptr,
            MAKEINTRESOURCEA(kXinnProcessedPresetResourceId),
            RT_RCDATA);
        if (!resource || SizeofResource(nullptr, resource) != kXinnProcessedPresetResourceBytes)
            return nullptr;
        const HGLOBAL data = LoadResource(nullptr, resource);
        return data ? static_cast<const std::uint8_t*>(LockResource(data)) : nullptr;
    }();
    return presets ? presets + (family * 4u + room) * kXinnProcessedPresetBytes : nullptr;
#else
    return nullptr;
#endif
}

bool xinn_initialize_processed_preset_from_portable_spreset_partial(
    std::uint8_t* processed_preset,
    std::uint64_t spreset_u64) {
    const std::uint8_t* preset = xinn_portable_processed_preset_from_native_va_partial(spreset_u64);
    if (!processed_preset || !preset)
        return false;

    std::memcpy(processed_preset, preset, kXinnProcessedPresetBytes);
    processed_preset[0] = 1u;
    processed_preset[1] = kXinnProcessedPresetMarker;
    return true;
}

void xinn_engine2_initialize_static_from_spreset_partial(
    std::uint8_t* processed_preset,
    std::uint64_t spreset_u64) {
    if (processed_preset && processed_preset[1] == kXinnProcessedPresetMarker)
        return;
    const std::uint8_t* spreset = xinn_portable_spreset_from_native_va_partial(spreset_u64);
    if (!processed_preset || processed_preset[0] == 0u || !spreset)
        return;

    auto read_u32 = [&](std::uint32_t off) -> std::uint32_t {
        return *reinterpret_cast<const std::uint32_t*>(spreset + off);
    };
    auto write_u32 = [](std::uint8_t* dst, std::uint32_t off, std::uint32_t value) {
        *reinterpret_cast<std::uint32_t*>(dst + off) = value;
    };
    auto write_u64 = [](std::uint8_t* dst, std::uint32_t off, std::uint64_t value) {
        *reinterpret_cast<std::uint64_t*>(dst + off) = value;
    };

    auto* inline_preset = processed_preset + 8u;
    auto* engine2_state = processed_preset + 23320u;
    auto* reverb_preset = inline_preset + 64u;
    *reinterpret_cast<std::uint64_t*>(engine2_state + 0u) =
        reinterpret_cast<std::uint64_t>(reverb_preset);

    const std::uint64_t shared_a = *reinterpret_cast<const std::uint64_t*>(spreset + 6172u);
    const std::uint64_t shared_b = *reinterpret_cast<const std::uint64_t*>(spreset + 6180u);
    for (std::uint32_t i = 0u, selector = 0u; i != 16u; ++i, selector += 2u) {
        const std::uint32_t slot = i < 8u ? selector : selector - 15u;
        const std::uint32_t src = 6204u + 44u * slot;
        auto* dst = reverb_preset + 48u * i;
        write_u32(dst, 0u, read_u32(src + 12u));
        write_u64(dst, 4u, shared_b);
        write_u32(dst, 12u, read_u32(src + 16u));
        write_u32(dst, 16u, read_u32(src + 20u));
        write_u32(dst, 20u, read_u32(src + 24u));
        write_u32(dst, 24u, read_u32(src + 28u));
        write_u32(dst, 28u, read_u32(src + 32u));
        write_u32(dst, 32u, read_u32(src + 36u));
        write_u32(dst, 36u, read_u32(src + 40u));
        write_u64(dst, 40u, shared_a);
    }

    static constexpr std::uint32_t kBusSrc[3][33] = {
        {
            6908u, 6912u, 6972u, 6976u, 6916u, 6920u, 6980u, 6984u,
            6924u, 6928u, 6988u, 6992u, 6932u, 6936u, 6996u, 7000u,
            6940u, 6944u, 7004u, 7008u, 6948u, 6952u, 7012u, 7016u,
            6956u, 6960u, 7020u, 7024u, 6964u, 6968u, 7028u, 7032u,
            7420u,
        },
        {
            7036u, 7040u, 7100u, 7104u, 7044u, 7048u, 7108u, 7112u,
            7052u, 7056u, 7116u, 7120u, 7060u, 7064u, 7124u, 7128u,
            7068u, 7072u, 7132u, 7136u, 7076u, 7080u, 7140u, 7144u,
            7084u, 7088u, 7148u, 7152u, 7092u, 7096u, 7156u, 7160u,
            7424u,
        },
        {
            7164u, 7168u, 7228u, 7232u, 7172u, 7176u, 7236u, 7240u,
            7180u, 7184u, 7244u, 7248u, 7188u, 7192u, 7252u, 7256u,
            7196u, 7200u, 7260u, 7264u, 7204u, 7208u, 7268u, 7272u,
            7212u, 7216u, 7276u, 7280u, 7220u, 7224u, 7284u, 7288u,
            7428u,
        },
    };
    for (std::uint32_t bus = 0u; bus != 3u; ++bus) {
        auto* dst = engine2_state + 8u + kXinnEngine2BusPresetSize_34bdd0 * bus;
        for (std::uint32_t i = 0u; i != 33u; ++i)
            write_u32(dst, 4u * i, read_u32(kBusSrc[bus][i]));
    }

    static constexpr std::uint32_t kTapA[16] = {
        6204u, 6292u, 6380u, 6468u, 6556u, 6644u, 6732u, 6820u,
        6248u, 6336u, 6424u, 6512u, 6600u, 6688u, 6776u, 6864u,
    };
    static constexpr std::uint32_t kTapB[16] = {
        6208u, 6296u, 6384u, 6472u, 6560u, 6648u, 6736u, 6824u,
        6252u, 6340u, 6428u, 6516u, 6604u, 6692u, 6780u, 6868u,
    };
    for (std::uint32_t i = 0u; i != 16u; ++i) {
        write_u32(reverb_preset, 832u + 4u * i, (1380u + 5528u * i) - read_u32(kTapA[i]));
        write_u32(reverb_preset, 768u + 4u * i, (5531u + 5528u * i) - read_u32(kTapB[i]));
    }
    for (std::uint32_t i = 0u; i != 8u; ++i) {
        write_u32(reverb_preset, 896u + 4u * i, (132u + 5528u * i) - read_u32(6192u));
        write_u32(reverb_preset, 928u + 4u * i, (44356u + 5528u * i) - read_u32(6192u));
    }
    write_u32(
        reverb_preset,
        960u,
        *reinterpret_cast<const std::uint32_t*>(processed_preset + 1132u));
}

void xinn_engine1_compute_type5_biquad_partial(std::uint8_t* storage_28, const std::uint8_t* spreset) {
    if (!storage_28 || !spreset)
        return;
    const float frequency = *reinterpret_cast<const float*>(spreset + 8u);
    const float q = *reinterpret_cast<const float*>(spreset + 12u);
    const float gain_db = *reinterpret_cast<const float*>(spreset + 16u);
    if (frequency <= 0.0f || q == 0.0f) {
        std::memset(storage_28 + 4u, 0, 20u);
        return;
    }
    constexpr double kSampleRate = 48000.0;
    constexpr double kTwoPi = 6.283185307179586476925286766559;
    const double omega = static_cast<double>(frequency) * kTwoPi / kSampleRate;
    const double sin_omega = std::sin(omega);
    const double cos_omega = std::cos(omega);
    const double alpha = sin_omega / (static_cast<double>(q) + static_cast<double>(q));
    const double amplitude = std::pow(10.0, static_cast<double>(gain_db) / 40.0);
    const double a0 = 1.0 + alpha / amplitude;
    if (a0 == 0.0) {
        std::memset(storage_28 + 4u, 0, 20u);
        return;
    }

    const float coeffs[5] = {
        static_cast<float>((1.0 + alpha * amplitude) / a0),
        static_cast<float>((cos_omega * -2.0) / a0),
        static_cast<float>((1.0 - alpha * amplitude) / a0),
        static_cast<float>((cos_omega * -2.0) / a0),
        static_cast<float>((1.0 - alpha / amplitude) / a0),
    };
    std::memcpy(storage_28 + 4u, coeffs, sizeof(coeffs));
}

void xinn_apply_output_mask_4eb39c(std::uint8_t* dynamic_48, std::uint32_t mode, std::uint32_t output_mask) {
    if (!dynamic_48)
        return;
    auto* words = reinterpret_cast<std::uint32_t*>(dynamic_48);
    if (mode == 2u) {
        if ((output_mask & 0x1800u) == 0u) {
            words[0] = 0u;
            words[3] = 0u;
            words[6] = 0u;
        }
        if ((output_mask & 0x600u) == 0u) {
            words[1] = 0u;
            words[5] = 0u;
            words[7] = 0u;
        }
        if ((output_mask & 0x6000u) == 0u) {
            words[2] = 0u;
            words[4] = 0u;
            words[8] = 0u;
        }
        return;
    }
    if (mode != 1u)
        return;
    if ((output_mask & 0x30u) == 0u) {
        words[0] = 0u;
        words[6] = 0u;
    }
    if ((output_mask & 0x600u) == 0u) {
        words[1] = 0u;
        words[7] = 0u;
    }
    if ((output_mask & 0x6000u) == 0u) {
        words[2] = 0u;
        words[8] = 0u;
    }
}

float xinn_db_to_linear_35b8a0_partial(float db) {
    return db > -144.0f ? std::pow(10.0f, db * 0.050000001f) : 0.0f;
}

std::uint32_t xinn_u32_bits_35c0b0_partial(const float* word) {
    return *reinterpret_cast<const std::uint32_t*>(word);
}

void xinn_engine1_apply_table_35c0b0_partial(
    float* engine1_state,
    std::uint32_t config_base,
    std::uint32_t output_base,
    const float* gain_set_a,
    const float* gain_set_b,
    const float* inline_gain_xyz) {
    if (!engine1_state || !gain_set_a || !gain_set_b || !inline_gain_xyz)
        return;
    for (std::uint32_t row = 0u, out = output_base; row != 490u; row += 5u, out += 2u) {
        const std::uint32_t gain_index = xinn_u32_bits_35c0b0_partial(engine1_state + config_base + row + 0u);
        if (gain_index >= 3u)
            continue;
        const float* selected =
            xinn_u32_bits_35c0b0_partial(engine1_state + config_base + row + 1u) == 1u ? gain_set_b : gain_set_a;
        float scale = 1.0f;
        if (static_cast<std::uint8_t>(
                xinn_u32_bits_35c0b0_partial(engine1_state + config_base + row + 2u))
            != 0u) {
            scale = inline_gain_xyz[gain_index];
        }
        engine1_state[out] = scale * engine1_state[config_base + row + 3u] * selected[gain_index];
    }
}

void xinn_engine1_apply_35c0b0_partial(float* engine1_state, const float* dynamic_64) {
    if (!engine1_state || !dynamic_64)
        return;

    float gain_set_a[3] = {};
    float gain_set_b[3] = {};
    const float base_a = xinn_db_to_linear_35b8a0_partial(dynamic_64[0]);
    gain_set_a[0] = xinn_db_to_linear_35b8a0_partial(dynamic_64[1]) * base_a;
    gain_set_a[1] = xinn_db_to_linear_35b8a0_partial(dynamic_64[2]) * base_a;
    gain_set_a[2] = xinn_db_to_linear_35b8a0_partial(dynamic_64[3]) * base_a;

    const float base_b = xinn_db_to_linear_35b8a0_partial(dynamic_64[4]);
    gain_set_b[0] = xinn_db_to_linear_35b8a0_partial(dynamic_64[5]) * base_b;
    gain_set_b[1] = xinn_db_to_linear_35b8a0_partial(dynamic_64[6]) * base_b;
    gain_set_b[2] = xinn_db_to_linear_35b8a0_partial(dynamic_64[7]) * base_b;

    const float inline_gain_xyz[3] = {
        *reinterpret_cast<const float*>(reinterpret_cast<const std::uint8_t*>(dynamic_64) + 32u),
        *reinterpret_cast<const float*>(reinterpret_cast<const std::uint8_t*>(dynamic_64) + 36u),
        *reinterpret_cast<const float*>(reinterpret_cast<const std::uint8_t*>(dynamic_64) + 40u),
    };
    xinn_engine1_apply_table_35c0b0_partial(
        engine1_state,
        0u,
        2943u,
        gain_set_a,
        gain_set_b,
        inline_gain_xyz);
    xinn_engine1_apply_table_35c0b0_partial(
        engine1_state,
        490u,
        3139u,
        gain_set_a,
        gain_set_b,
        inline_gain_xyz);
    xinn_engine1_apply_table_35c0b0_partial(
        engine1_state,
        980u,
        3811u,
        gain_set_a,
        gain_set_b,
        inline_gain_xyz);
    xinn_engine1_apply_table_35c0b0_partial(
        engine1_state,
        1470u,
        4007u,
        gain_set_a,
        gain_set_b,
        inline_gain_xyz);
    xinn_engine1_apply_table_35c0b0_partial(
        engine1_state,
        1960u,
        4679u,
        gain_set_a,
        gain_set_b,
        inline_gain_xyz);
    xinn_engine1_apply_table_35c0b0_partial(
        engine1_state,
        2450u,
        4875u,
        gain_set_a,
        gain_set_b,
        inline_gain_xyz);
}

void xinn_preset_tune_late_stereo_35b8a0_partial(std::uint8_t* preset_base, const float* dynamic_96) {
    if (!preset_base || !dynamic_96)
        return;

    std::memcpy(preset_base + 1072u, dynamic_96, 64u);
    const float gain_sum_base = *reinterpret_cast<const float*>(preset_base + 1116u);
    *reinterpret_cast<float*>(preset_base + 23460u) =
        xinn_db_to_linear_35b8a0_partial(*reinterpret_cast<const float*>(preset_base + 1120u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23596u) =
        xinn_db_to_linear_35b8a0_partial(*reinterpret_cast<const float*>(preset_base + 1124u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23732u) =
        xinn_db_to_linear_35b8a0_partial(*reinterpret_cast<const float*>(preset_base + 1128u) + gain_sum_base);

    const std::uint64_t engine2_storage = *reinterpret_cast<const std::uint64_t*>(preset_base + 23320u);
    if (engine2_storage != 0u)
        *reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(engine2_storage) + 960u) =
            *reinterpret_cast<const std::uint32_t*>(preset_base + 1132u);

    *reinterpret_cast<float*>(preset_base + 23784u) = xinn_db_to_linear_35b8a0_partial(dynamic_96[16]);
    *reinterpret_cast<float*>(preset_base + 23788u) = xinn_db_to_linear_35b8a0_partial(dynamic_96[19]);
    *reinterpret_cast<float*>(preset_base + 23792u) = xinn_db_to_linear_35b8a0_partial(dynamic_96[17]);
    *reinterpret_cast<float*>(preset_base + 23796u) = xinn_db_to_linear_35b8a0_partial(dynamic_96[20]);
    *reinterpret_cast<float*>(preset_base + 23800u) = xinn_db_to_linear_35b8a0_partial(dynamic_96[18]);
    *reinterpret_cast<float*>(preset_base + 23804u) = xinn_db_to_linear_35b8a0_partial(dynamic_96[21]);
}

void xinn_preset_tune_late_surround_35bba0_partial(std::uint8_t* preset_base, const float* dynamic_112) {
    if (!preset_base || !dynamic_112)
        return;

    std::memcpy(preset_base + 1072u, dynamic_112, 64u);
    const float gain_sum_base = *reinterpret_cast<const float*>(preset_base + 1116u);
    *reinterpret_cast<float*>(preset_base + 23460u) =
        xinn_db_to_linear_35b8a0_partial(*reinterpret_cast<const float*>(preset_base + 1120u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23596u) =
        xinn_db_to_linear_35b8a0_partial(*reinterpret_cast<const float*>(preset_base + 1124u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23732u) =
        xinn_db_to_linear_35b8a0_partial(*reinterpret_cast<const float*>(preset_base + 1128u) + gain_sum_base);

    const std::uint64_t engine2_storage = *reinterpret_cast<const std::uint64_t*>(preset_base + 23320u);
    if (engine2_storage != 0u)
        *reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(engine2_storage) + 960u) =
            *reinterpret_cast<const std::uint32_t*>(preset_base + 1132u);

    *reinterpret_cast<float*>(preset_base + 23808u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[16]);
    *reinterpret_cast<float*>(preset_base + 23812u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[20]);
    *reinterpret_cast<float*>(preset_base + 23816u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[24]);
    *reinterpret_cast<float*>(preset_base + 23820u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[19]);
    *reinterpret_cast<float*>(preset_base + 23824u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[23]);
    *reinterpret_cast<float*>(preset_base + 23828u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[27]);
    *reinterpret_cast<float*>(preset_base + 23832u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[17]);
    *reinterpret_cast<float*>(preset_base + 23836u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[21]);
    *reinterpret_cast<float*>(preset_base + 23840u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[25]);
    *reinterpret_cast<float*>(preset_base + 23844u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[18]);
    *reinterpret_cast<float*>(preset_base + 23848u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[22]);
    *reinterpret_cast<float*>(preset_base + 23852u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[26]);
}

[[maybe_unused]] void xinn_preset_tune_stereo_35b8a0_partial(std::uint8_t* preset_base, const float* dynamic_96) {
    if (!preset_base || !dynamic_96)
        return;

    std::memcpy(preset_base + 1072u, dynamic_96, 64u);
    xinn_engine1_apply_35c0b0_partial(
        reinterpret_cast<float*>(preset_base + 1136u),
        reinterpret_cast<const float*>(preset_base + 1072u));

    const float gain_sum_base = *reinterpret_cast<const float*>(preset_base + 1116u);
    *reinterpret_cast<float*>(preset_base + 23460u) =
        xinn_db_to_linear_35b8a0_partial(*reinterpret_cast<const float*>(preset_base + 1120u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23596u) =
        xinn_db_to_linear_35b8a0_partial(*reinterpret_cast<const float*>(preset_base + 1124u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23732u) =
        xinn_db_to_linear_35b8a0_partial(*reinterpret_cast<const float*>(preset_base + 1128u) + gain_sum_base);

    const std::uint64_t engine2_storage = *reinterpret_cast<const std::uint64_t*>(preset_base + 23320u);
    if (engine2_storage != 0u)
        *reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(engine2_storage) + 960u) =
            *reinterpret_cast<const std::uint32_t*>(preset_base + 1132u);

    *reinterpret_cast<float*>(preset_base + 23784u) = xinn_db_to_linear_35b8a0_partial(dynamic_96[16]);
    *reinterpret_cast<float*>(preset_base + 23788u) = xinn_db_to_linear_35b8a0_partial(dynamic_96[19]);
    *reinterpret_cast<float*>(preset_base + 23792u) = xinn_db_to_linear_35b8a0_partial(dynamic_96[17]);
    *reinterpret_cast<float*>(preset_base + 23796u) = xinn_db_to_linear_35b8a0_partial(dynamic_96[20]);
    *reinterpret_cast<float*>(preset_base + 23800u) = xinn_db_to_linear_35b8a0_partial(dynamic_96[18]);
    *reinterpret_cast<float*>(preset_base + 23804u) = xinn_db_to_linear_35b8a0_partial(dynamic_96[21]);
}

[[maybe_unused]] void xinn_preset_tune_surround_35bba0_partial(std::uint8_t* preset_base, const float* dynamic_112) {
    if (!preset_base || !dynamic_112)
        return;

    std::memcpy(preset_base + 1072u, dynamic_112, 64u);
    xinn_engine1_apply_35c0b0_partial(
        reinterpret_cast<float*>(preset_base + 1136u),
        reinterpret_cast<const float*>(preset_base + 1072u));

    const float gain_sum_base = *reinterpret_cast<const float*>(preset_base + 1116u);
    *reinterpret_cast<float*>(preset_base + 23460u) =
        xinn_db_to_linear_35b8a0_partial(*reinterpret_cast<const float*>(preset_base + 1120u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23596u) =
        xinn_db_to_linear_35b8a0_partial(*reinterpret_cast<const float*>(preset_base + 1124u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23732u) =
        xinn_db_to_linear_35b8a0_partial(*reinterpret_cast<const float*>(preset_base + 1128u) + gain_sum_base);

    const std::uint64_t engine2_storage = *reinterpret_cast<const std::uint64_t*>(preset_base + 23320u);
    if (engine2_storage != 0u)
        *reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(engine2_storage) + 960u) =
            *reinterpret_cast<const std::uint32_t*>(preset_base + 1132u);

    *reinterpret_cast<float*>(preset_base + 23808u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[16]);
    *reinterpret_cast<float*>(preset_base + 23812u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[20]);
    *reinterpret_cast<float*>(preset_base + 23816u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[24]);
    *reinterpret_cast<float*>(preset_base + 23820u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[19]);
    *reinterpret_cast<float*>(preset_base + 23824u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[23]);
    *reinterpret_cast<float*>(preset_base + 23828u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[27]);
    *reinterpret_cast<float*>(preset_base + 23832u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[17]);
    *reinterpret_cast<float*>(preset_base + 23836u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[21]);
    *reinterpret_cast<float*>(preset_base + 23840u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[25]);
    *reinterpret_cast<float*>(preset_base + 23844u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[18]);
    *reinterpret_cast<float*>(preset_base + 23848u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[22]);
    *reinterpret_cast<float*>(preset_base + 23852u) = xinn_db_to_linear_35b8a0_partial(dynamic_112[26]);
}

std::int64_t xinn_early_initialize_50695c_partial(std::uint8_t* early_state, std::uint32_t mode, std::uint32_t input_mask) {
    if (!early_state)
        return 0;
    *reinterpret_cast<std::uint32_t*>(early_state + 276u) = (input_mask & 0x180u) != 0u ? 1u : 0u;
    *reinterpret_cast<std::uint32_t*>(early_state + 280u) = (input_mask >> 6u) & 1u;
    if (mode == 1u) {
        *reinterpret_cast<std::uint64_t*>(early_state + 296u) = 0x5773E0u;
        *reinterpret_cast<std::uint64_t*>(early_state + 304u) = 0x577400u;
        return 1;
    }
    if (mode == 2u) {
        if (*reinterpret_cast<const std::uint32_t*>(early_state + 272u) == 0u)
            return 0;
        *reinterpret_cast<std::uint64_t*>(early_state + 296u) = 0x5776E0u;
        *reinterpret_cast<std::uint64_t*>(early_state + 304u) = 0x577BC0u;
        return 1;
    }
    return 0;
}

} // namespace

std::int64_t auro_matic_XinN_fl32_construct_574950_partial(
    std::uint8_t* xinn_state,
    std::uint8_t* routing_state,
    const std::uint64_t* memory_args_5) {
    if (!xinn_state || !routing_state || !memory_args_5 || memory_args_5[0] == 0u)
        return 0;
    std::memset(xinn_state, 0, 0x270u);
    *reinterpret_cast<std::uint64_t*>(xinn_state + 552u) = reinterpret_cast<std::uint64_t>(routing_state);
    *reinterpret_cast<std::uint64_t*>(xinn_state + 32u) = memory_args_5[3];
    *reinterpret_cast<std::uint64_t*>(xinn_state + 40u) = memory_args_5[4];
    if (memory_args_5[4] != 0u && memory_args_5[3] < 0x900u)
        return 0;
    (void)auro_matic_XinN_Early_fl32_construct_577320_partial(xinn_state + 48u, memory_args_5[0], memory_args_5[1]);
    auro_matic_XinN_Late_fl32_construct_579920_partial(
        reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
        memory_args_5[2]);
    auro_matic_XinN_parameter_Dynamic_t_default_503fbc_partial(xinn_state + 572u);
    xinn_apply_output_mask_4eb39c(
        xinn_state + 572u,
        *reinterpret_cast<const std::uint32_t*>(xinn_state + 0u),
        *reinterpret_cast<const std::uint32_t*>(xinn_state + 20u));
    auro_matic_XinN_Early_fl32_set_downmix_577e90_partial(xinn_state + 48u, xinn_state + 608u);
    auro_matic_XinN_Early_fl32_set_output_patch_577ec0_partial(
        xinn_state + 48u,
        reinterpret_cast<const std::uint32_t*>(xinn_state + 572u),
        reinterpret_cast<const std::uint32_t*>(xinn_state + 584u));
    auro_matic_XinN_Late_fl32_set_output_patch_579a50_partial(
        reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
        reinterpret_cast<const std::uint32_t*>(xinn_state + 596u));
    return reinterpret_cast<std::int64_t>(xinn_state);
}

void auro_matic_XinN_fl32_set_dynamic_parameters_574a50_partial(
    std::uint8_t* xinn_state,
    const std::uint8_t* dynamic_48) {
    if (!xinn_state || !dynamic_48)
        return;
    std::memcpy(xinn_state + 572u, dynamic_48, 48u);
    xinn_apply_output_mask_4eb39c(
        xinn_state + 572u,
        *reinterpret_cast<const std::uint32_t*>(xinn_state + 0u),
        *reinterpret_cast<const std::uint32_t*>(xinn_state + 20u));
    auro_matic_XinN_Early_fl32_set_downmix_577e90_partial(xinn_state + 48u, xinn_state + 608u);
    auro_matic_XinN_Early_fl32_set_output_patch_577ec0_partial(
        xinn_state + 48u,
        reinterpret_cast<const std::uint32_t*>(xinn_state + 572u),
        reinterpret_cast<const std::uint32_t*>(xinn_state + 584u));
    auro_matic_XinN_Late_fl32_set_output_patch_579a50_partial(
        reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
        reinterpret_cast<const std::uint32_t*>(xinn_state + 596u));
}

void auro_matic_XinN_fl32_get_dynamic_parameters_575080_partial(
    const std::uint8_t* xinn_state,
    std::uint8_t* dynamic_48) {
    if (!xinn_state || !dynamic_48)
        return;
    std::memcpy(dynamic_48, xinn_state + 572u, 48u);
}

std::int64_t auro_matic_XinN_fl32_initialize_574be0_partial(
    std::uint8_t* xinn_state,
    std::uint32_t input_mask,
    std::uint32_t output_mask,
    std::uint64_t preset) {
    if (!xinn_state)
        return 0;
    std::uint32_t mode = ((~input_mask & 0x33u) != 0u)
        ? (((~input_mask & 3u) == 0u) ? 1u : 0u)
        : 2u;
    const std::uint32_t masked_output = output_mask & ~input_mask & ~8u;
    auro_matic_XinN_parameter_Dynamic_t_default_503fbc_partial(xinn_state + 572u);
    if ((masked_output & ~xinn_supported_output_mask_4eb380(mode)) != 0u)
        return 0;
    *reinterpret_cast<std::uint32_t*>(xinn_state + 16u) = input_mask;
    *reinterpret_cast<std::uint32_t*>(xinn_state + 20u) = masked_output;
    *reinterpret_cast<std::uint32_t*>(xinn_state + 0u) = mode;
    *reinterpret_cast<std::uint64_t*>(xinn_state + 24u) = 0u;

    if (preset != 0u) {
        *reinterpret_cast<std::uint64_t*>(xinn_state + 8u) = preset;
        const auto* p = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(preset));
        const std::uint32_t early_blocks = *reinterpret_cast<const std::uint32_t*>(p + 8u) >> 5u;
        const std::uint32_t late_blocks = *reinterpret_cast<const std::uint32_t*>(p + 32u) >> 5u;
        const std::uint32_t shared_blocks = std::min(early_blocks, late_blocks);
        const auto* late_state = reinterpret_cast<const std::uint32_t*>(xinn_state + 392u);
        const std::uint32_t early_clear = late_state[0] != 0u ? shared_blocks : early_blocks;
        auro_matic_XinN_Early_fl32_set_total_clear_frames_5772e0_partial(xinn_state + 48u, early_clear);
        auro_matic_XinN_Late_fl32_set_clear_frames_579960_partial(
            reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
            early_clear,
            late_blocks - early_clear);
        auro_matic_XinN_Early_fl32_set_preset_577f90_partial(xinn_state + 48u, preset);
        auro_matic_XinN_Late_fl32_set_preset_579a30_partial(
            reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
            preset);
        xinn_apply_output_mask_4eb39c(xinn_state + 572u, mode, masked_output);
        auro_matic_XinN_Early_fl32_set_downmix_577e90_partial(xinn_state + 48u, xinn_state + 608u);
        auro_matic_XinN_Early_fl32_set_output_patch_577ec0_partial(
            xinn_state + 48u,
            reinterpret_cast<const std::uint32_t*>(xinn_state + 572u),
            reinterpret_cast<const std::uint32_t*>(xinn_state + 584u));
        auro_matic_XinN_Late_fl32_set_output_patch_579a50_partial(
            reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
            reinterpret_cast<const std::uint32_t*>(xinn_state + 596u));
    }

    std::int64_t ok = xinn_early_initialize_50695c_partial(xinn_state + 48u, mode, input_mask);
    if (!ok)
        return ok;
    std::uint64_t routing_u64 = *reinterpret_cast<const std::uint64_t*>(xinn_state + 552u);
    if (routing_u64 == 0u)
        routing_u64 = reinterpret_cast<std::uint64_t>(xinn_state + 624u);
    if (!auro_matic_v3_XinN_Routing_fl32_configure_557460_partial(
            reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(routing_u64)),
            mode,
            masked_output,
            input_mask))
        return 0;
    *reinterpret_cast<std::uint64_t*>(xinn_state + 24u) =
        *reinterpret_cast<const std::uint64_t*>(xinn_state + 40u) != 0u
            ? 0x574E30u
            : 0x574EE0u;
    auro_audio_Smooth_fl32_inst_initialize_5aab80_partial(reinterpret_cast<float*>(xinn_state + 560u), 48000u, 0.001f);
    auro_matic_XinN_Early_fl32_reset_audio_state_577f00_partial(xinn_state + 48u);
    auro_matic_XinN_Late_fl32_reset_audio_state_579940_partial(reinterpret_cast<std::uint32_t*>(xinn_state + 392u));
    auro_audio_Smooth_fl32_inst_set_current_5aabe0_partial(reinterpret_cast<float*>(xinn_state + 560u), 0.0f);
    auro_audio_Smooth_fl32_inst_update_5aabd0_partial(reinterpret_cast<float*>(xinn_state + 560u), 1.0f);
    xinn_apply_output_mask_4eb39c(xinn_state + 572u, mode, masked_output);
    auro_matic_XinN_Early_fl32_set_downmix_577e90_partial(xinn_state + 48u, xinn_state + 608u);
    auro_matic_XinN_Early_fl32_set_output_patch_577ec0_partial(
        xinn_state + 48u,
        reinterpret_cast<const std::uint32_t*>(xinn_state + 572u),
        reinterpret_cast<const std::uint32_t*>(xinn_state + 584u));
    auro_matic_XinN_Late_fl32_set_output_patch_579a50_partial(
        reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
        reinterpret_cast<const std::uint32_t*>(xinn_state + 596u));
    return 1;
}

std::int64_t auro_matic_v3_XinN_fl32_construct_556180_partial(
    std::uint8_t* xinn_v3_state,
    const std::uint64_t* memory_args_5) {
    if (!xinn_v3_state)
        return 0;
    auro_matic_v3_XinN_Routing_fl32_construct_557420_partial(xinn_v3_state + 624u);
    if (auro_matic_XinN_fl32_construct_574950_partial(xinn_v3_state, xinn_v3_state + 624u, memory_args_5) == 0)
        return 0;
    *reinterpret_cast<std::uint32_t*>(xinn_v3_state + 840u) = 0u;
    std::uint8_t dynamic[120]{};
    auro_matic_v3_XinN_parameter_Dynamic_t_default_4e9fec_partial(dynamic);
    auro_matic_XinN_fl32_set_dynamic_parameters_574a50_partial(xinn_v3_state, dynamic);
    auro_matic_v3_XinN_Routing_fl32_set_routing_557620_partial(xinn_v3_state + 624u, dynamic + 48u);
    return reinterpret_cast<std::int64_t>(xinn_v3_state);
}

std::int64_t auro_matic_v3_XinN_fl32_configure_556330_partial(
    std::uint8_t* xinn_v3_state,
    std::uint32_t sample_rate,
    const std::uint64_t* config_args_4) {
    if (!xinn_v3_state || !config_args_4)
        return 1;
    const auto* cfg = reinterpret_cast<const std::uint8_t*>(config_args_4);
    const std::uint32_t input_mask = *reinterpret_cast<const std::uint32_t*>(cfg + 0u);
    const std::uint32_t output_mask = *reinterpret_cast<const std::uint32_t*>(cfg + 4u);
    const std::uint64_t preset = *reinterpret_cast<const std::uint64_t*>(cfg + 8u);
    const std::uint64_t dynamic = *reinterpret_cast<const std::uint64_t*>(cfg + 16u);
    *reinterpret_cast<std::uint32_t*>(xinn_v3_state + 840u) = input_mask != 0u ? 1u : 0u;
    if (input_mask == 0u) {
        if (output_mask != 0u)
            return 1;
    } else {
        const bool rate_supported = sample_rate == 32000u || sample_rate == 44100u || sample_rate == 48000u;
        if (!rate_supported
            || !auro_matic_XinN_fl32_initialize_574be0_partial(xinn_v3_state, input_mask, output_mask, preset)) {
            return 1;
        }
    }
    if (dynamic != 0u) {
        const auto* dyn = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(dynamic));
        auro_matic_v3_XinN_fl32_set_dynamic_parameters_556200_partial(xinn_v3_state, dyn);
    }
    return 0;
}

void auro_matic_XinN_fl32_set_preset_574af0_partial(std::uint8_t* xinn_state, std::uint64_t preset) {
    if (!xinn_state || preset == 0u)
        return;
    *reinterpret_cast<std::uint64_t*>(xinn_state + 8u) = preset;
    const auto* p = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(preset));
    const std::uint32_t early_blocks = *reinterpret_cast<const std::uint32_t*>(p + 8u) >> 5u;
    const std::uint32_t late_blocks = *reinterpret_cast<const std::uint32_t*>(p + 32u) >> 5u;
    const std::uint32_t shared_blocks = std::min(early_blocks, late_blocks);
    const auto* late_state = reinterpret_cast<const std::uint32_t*>(xinn_state + 392u);
    const std::uint32_t early_clear = late_state[0] != 0u ? shared_blocks : early_blocks;
    auro_matic_XinN_Early_fl32_set_total_clear_frames_5772e0_partial(xinn_state + 48u, early_clear);
    auro_matic_XinN_Late_fl32_set_clear_frames_579960_partial(
        reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
        early_clear,
        late_blocks - early_clear);
    auro_matic_XinN_Early_fl32_set_preset_577f90_partial(xinn_state + 48u, preset);
    auro_matic_XinN_Late_fl32_set_preset_579a30_partial(
        reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
        preset);
    xinn_apply_output_mask_4eb39c(
        xinn_state + 572u,
        *reinterpret_cast<const std::uint32_t*>(xinn_state + 0u),
        *reinterpret_cast<const std::uint32_t*>(xinn_state + 20u));
    auro_matic_XinN_Early_fl32_set_downmix_577e90_partial(xinn_state + 48u, xinn_state + 608u);
    auro_matic_XinN_Early_fl32_set_output_patch_577ec0_partial(
        xinn_state + 48u,
        reinterpret_cast<const std::uint32_t*>(xinn_state + 572u),
        reinterpret_cast<const std::uint32_t*>(xinn_state + 584u));
    auro_matic_XinN_Late_fl32_set_output_patch_579a50_partial(
        reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
        reinterpret_cast<const std::uint32_t*>(xinn_state + 596u));
}

std::int64_t auro_matic_XinN_fl32_reset_audio_state_574f90_partial(std::uint8_t* xinn_state) {
    if (!xinn_state)
        return 0;
    auro_matic_XinN_Early_fl32_reset_audio_state_577f00_partial(xinn_state + 48u);
    auro_matic_XinN_Late_fl32_reset_audio_state_579940_partial(reinterpret_cast<std::uint32_t*>(xinn_state + 392u));
    auro_audio_Smooth_fl32_inst_set_current_5aabe0_partial(reinterpret_cast<float*>(xinn_state + 560u), 0.0f);
    auro_audio_Smooth_fl32_inst_update_5aabd0_partial(reinterpret_cast<float*>(xinn_state + 560u), 1.0f);
    return 0;
}

std::int64_t auro_matic_XinN_fl32_partial_clear_574ff0_partial(
    std::uint8_t* xinn_state,
    std::uint32_t* remaining) {
    if (!xinn_state || !remaining)
        return 0;
    if (auro_matic_XinN_Early_fl32_partial_clear_577f40_partial(xinn_state + 48u, remaining)
        && auro_matic_XinN_Late_fl32_partial_clear_579980_partial(
            reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
            remaining)) {
        (void)auro_matic_XinN_Early_fl32_reset_audio_state_577f00_partial(xinn_state + 48u);
        (void)auro_matic_XinN_Late_fl32_reset_audio_state_579940_partial(
            reinterpret_cast<std::uint32_t*>(xinn_state + 392u));
        auro_audio_Smooth_fl32_inst_set_current_5aabe0_partial(reinterpret_cast<float*>(xinn_state + 560u), 0.0f);
        auro_audio_Smooth_fl32_inst_update_5aabd0_partial(reinterpret_cast<float*>(xinn_state + 560u), 1.0f);
        return 1;
    }
    return 0;
}

void auro_matic_v3_XinN_fl32_set_preset_556250_partial(std::uint8_t* xinn_v3_state, std::uint64_t preset) {
    auro_matic_XinN_fl32_set_preset_574af0_partial(xinn_v3_state, preset);
}

std::int64_t auro_matic_v3_XinN_fl32_reset_audio_state_556240_partial(std::uint8_t* xinn_v3_state) {
    if (!xinn_v3_state || *reinterpret_cast<const std::uint32_t*>(xinn_v3_state + 840u) == 0u)
        return 0;
    return auro_matic_XinN_fl32_reset_audio_state_574f90_partial(xinn_v3_state);
}

std::uint64_t auro_matic_v3_XinN_fl32_set_dynamic_parameters_556200_partial(
    std::uint8_t* xinn_v3_state,
    const std::uint8_t* dynamic_120) {
    if (!xinn_v3_state || !dynamic_120)
        return 0u;
    auro_matic_XinN_fl32_set_dynamic_parameters_574a50_partial(xinn_v3_state, dynamic_120);
    return auro_matic_v3_XinN_Routing_fl32_set_routing_557620_partial(xinn_v3_state + 624u, dynamic_120 + 48u);
}

std::uint64_t auro_matic_v3_XinN_fl32_get_dynamic_parameters_5567f0_partial(
    const std::uint8_t* xinn_v3_state,
    std::uint8_t* dynamic_120) {
    if (!xinn_v3_state || !dynamic_120)
        return 0u;
    auro_matic_XinN_fl32_get_dynamic_parameters_575080_partial(xinn_v3_state, dynamic_120);
    return auro_matic_v3_XinN_Routing_fl32_get_routing_557690_partial(xinn_v3_state + 624u, dynamic_120 + 48u);
}

void auro_matic_v3_XinN_fl32_update_peak_amplitude_556450_partial(
    std::uint8_t* xinn_v3_state,
    float* peak_31) {
    if (!xinn_v3_state || !peak_31 || *reinterpret_cast<const std::uint32_t*>(xinn_v3_state + 840u) == 0u)
        return;

    const std::uint32_t output_mask = *reinterpret_cast<const std::uint32_t*>(xinn_v3_state + 20u);
    float peak = std::max(peak_31[0], peak_31[1]);
    if (*reinterpret_cast<const std::uint32_t*>(xinn_v3_state + 0u) == 2u) {
        float left = *reinterpret_cast<const float*>(xinn_v3_state + 332u) * peak_31[4];
        float right = *reinterpret_cast<const float*>(xinn_v3_state + 332u) * peak_31[5];
        if (*reinterpret_cast<const std::uint32_t*>(xinn_v3_state + 328u) != 0u) {
            const float center = *reinterpret_cast<const float*>(xinn_v3_state + 336u) * peak_31[6];
            left += center;
            right += center;
        }
        if (*reinterpret_cast<const std::uint32_t*>(xinn_v3_state + 324u) != 0u) {
            left += *reinterpret_cast<const float*>(xinn_v3_state + 340u) * peak_31[7];
            right += *reinterpret_cast<const float*>(xinn_v3_state + 340u) * peak_31[8];
        }
        peak = std::max(peak, std::max(left, right));
    }

    for (std::uint32_t bit = 0u; bit < 31u; ++bit) {
        if ((output_mask & (1u << bit)) != 0u)
            peak_31[bit] = peak;
    }
}

std::uint64_t xinn_select_native_runtime_preset_from_v4_static_partial(
    bool surround_mode,
    const std::uint8_t* tuning_static) {
    if (!tuning_static)
        return 0u;

    const std::uint32_t family = *reinterpret_cast<const std::uint32_t*>(tuning_static + 0u);
    const std::uint32_t room = *reinterpret_cast<const std::uint32_t*>(tuning_static + 4u);

    // Family order follows the native preset symbol groups:
    // 0=ss, 1=sb1, 2=sb2, 3=sb3, 4=multichannel.
    // The cached runtime objects expose rooms as:
    // 0=Small, 1=Speech, 2=Movie, 3=Medium, 4=Large.
    // The older AMG3 get_2in6/get_5inN loaders only expose four preset numbers, so
    // prefer the common 4-slot order Small/Medium/Large/Movie and keep Speech as slot 4.
    static constexpr std::uint64_t kStereoRuntimePresets[5][5] = {
        {0x656ED8u, 0x656AB0u, 0x657300u, 0x657728u, 0x657B50u},
        {0x65A908u, 0x65AD30u, 0x65B158u, 0x65B580u, 0x65B9A8u},
        {0x659440u, 0x659868u, 0x659C90u, 0x65A0B8u, 0x65A4E0u},
        {0x6583A0u, 0x6587C8u, 0x657F78u, 0x658BF0u, 0x659018u},
        {0x65C620u, 0x65C1F8u, 0x65BDD0u, 0x65CA48u, 0x65CE70u},
    };
    static constexpr std::uint64_t kSurroundRuntimePresets[5][5] = {
        {0x6502C8u, 0x650B18u, 0x6506F0u, 0x650F40u, 0x651368u},
        {0x654120u, 0x654548u, 0x654970u, 0x654D98u, 0x6551C0u},
        {0x653080u, 0x652C58u, 0x6534A8u, 0x6538D0u, 0x653CF8u},
        {0x651BB8u, 0x651790u, 0x651FE0u, 0x652408u, 0x652830u},
        {0x656260u, 0x6555E8u, 0x655A10u, 0x655E38u, 0x656688u},
    };

    if (family >= 5u || room >= 5u)
        return 0u;
    static constexpr std::uint8_t kRoomRemap[5] = {
        0u, // Small
        3u, // Medium
        4u, // Large
        2u, // Movie
        1u, // Speech
    };
    const std::uint32_t native_room_slot = kRoomRemap[room];
    return surround_mode ? kSurroundRuntimePresets[family][native_room_slot]
                         : kStereoRuntimePresets[family][native_room_slot];
}

void xinn_copy_runtime_preset_into_processed_object_partial(
    std::uint8_t* processed_preset,
    std::uint64_t runtime_preset) {
    if (!processed_preset)
        return;
    const bool preset_pointer_available = xinn_runtime_preset_pointer_available_partial(runtime_preset);
    processed_preset[0] = 0u;
    auto* inline_preset = processed_preset + 8u;
    if (preset_pointer_available) {
#if AURO3DENG_ENABLE_NATIVE_IMAGE_PRESET_POINTERS
        std::memcpy(
            inline_preset,
            reinterpret_cast<const void*>(static_cast<std::uintptr_t>(runtime_preset)),
            kXinnRuntimePresetSize_35bf40);
        processed_preset[0] = 1u;
        return;
#endif
    }
    std::memset(inline_preset, 0, kXinnRuntimePresetSize_35bf40);
}

void xinn_retarget_processed_runtime_preset_backing_partial(std::uint8_t* processed_preset) {
    if (!processed_preset || processed_preset[0] == 0u)
        return;

    auto* inline_preset = processed_preset + 8u;
    auto* engine1_state = processed_preset + 1136u;
    static constexpr std::uint32_t kEngine1TableOffsets[3] = {
        2942u * 4u,
        3810u * 4u,
        4678u * 4u,
    };
    for (std::uint32_t table = 0; table < 3u; ++table) {
        const std::uint64_t src_u64 = *reinterpret_cast<const std::uint64_t*>(inline_preset + 40u + 8u * table);
        auto* dst = engine1_state + kEngine1TableOffsets[table];
#if AURO3DENG_ENABLE_NATIVE_IMAGE_PRESET_POINTERS
        if (src_u64 != 0u) {
            std::memcpy(
                dst,
                reinterpret_cast<const void*>(static_cast<std::uintptr_t>(src_u64)),
                kXinnEngine1SourceTableSize_35c0b0);
        } else {
            std::memset(dst, 0, kXinnEngine1SourceTableSize_35c0b0);
        }
#elif !defined(_WIN32)
        std::memset(dst, 0, kXinnEngine1SourceTableSize_35c0b0);
#endif
        *reinterpret_cast<std::uint64_t*>(inline_preset + 40u + 8u * table) =
            reinterpret_cast<std::uint64_t>(dst);
    }

    *reinterpret_cast<std::uint64_t*>(engine1_state + 11760u) =
        reinterpret_cast<std::uint64_t>(inline_preset + 8u);

    auto* engine2_state = processed_preset + 23320u;
    *reinterpret_cast<std::uint64_t*>(engine2_state) =
        reinterpret_cast<std::uint64_t>(inline_preset + 64u);
    for (std::uint32_t bus = 0; bus < 3u; ++bus) {
        const std::uint64_t src_u64 = *reinterpret_cast<const std::uint64_t*>(inline_preset + 1040u + 8u * bus);
        auto* dst = engine2_state + 8u + kXinnEngine2BusPresetSize_34bdd0 * bus;
#if AURO3DENG_ENABLE_NATIVE_IMAGE_PRESET_POINTERS
        if (src_u64 != 0u) {
            std::memcpy(
                dst,
                reinterpret_cast<const void*>(static_cast<std::uintptr_t>(src_u64)),
                kXinnEngine2BusPresetSize_34bdd0);
        } else {
            std::memset(dst, 0, kXinnEngine2BusPresetSize_34bdd0);
        }
#elif !defined(_WIN32)
        std::memset(dst, 0, kXinnEngine2BusPresetSize_34bdd0);
#endif
        *reinterpret_cast<std::uint64_t*>(inline_preset + 1040u + 8u * bus) =
            reinterpret_cast<std::uint64_t>(dst);
    }
}

bool xinn_engine1_initialize_static_from_spreset_partial(std::uint8_t* processed_preset, std::uint64_t spreset_u64) {
    if (!processed_preset || processed_preset[0] == 0u || spreset_u64 == 0u)
        return false;
    if (processed_preset[1] == kXinnProcessedPresetMarker)
        return true;
    struct Beam {
        std::uint32_t gain_index;
        std::uint32_t selector;
        std::uint32_t side;
        std::uint32_t coefficient;
        std::uint32_t delay;
    };
    static constexpr std::uint32_t kOutputGroup[6] = {0u, 0u, 1u, 1u, 2u, 2u};
    static constexpr std::uint32_t kConfigBases[6] = {0u, 490u, 980u, 1470u, 1960u, 2450u};
    static constexpr std::uint32_t kCoeffBases[6] = {2942u, 3138u, 3810u, 4006u, 4678u, 4874u};
    static constexpr std::uint32_t kLookupBases[6] = {3334u, 3572u, 4202u, 4440u, 5070u, 5308u};

    auto* engine1_words = reinterpret_cast<std::uint32_t*>(processed_preset + 1136u);
    const std::uint8_t* portable_spreset = xinn_portable_spreset_from_native_va_partial(spreset_u64);
    const std::uint32_t* spreset_words = portable_spreset
        ? reinterpret_cast<const std::uint32_t*>(portable_spreset)
        : nullptr;
#if AURO3DENG_ENABLE_NATIVE_IMAGE_PRESET_POINTERS
    if (!spreset_words) {
        spreset_words = reinterpret_cast<const std::uint32_t*>(
            static_cast<std::uintptr_t>(spreset_u64));
    }
#endif
    if (!spreset_words)
        return false;
    auto read_pair = [](const std::uint32_t* words) -> std::uint64_t {
        return static_cast<std::uint64_t>(words[0]) | (static_cast<std::uint64_t>(words[1]) << 32u);
    };
    const std::uint32_t count_a = spreset_words[0];
    const std::uint32_t count_b = spreset_words[1];
    if (count_a > 48u || count_b > 48u)
        return false;

    auto* inline_storage = processed_preset + 16u;
    std::uint32_t min_delay = spreset_words[7u];
    for (std::uint32_t group = 1u; group != 6u; ++group) {
        const std::uint32_t group_delay = *reinterpret_cast<const std::uint32_t*>(
            reinterpret_cast<const std::uint8_t*>(spreset_words) + 28u + 768u * group);
        if (group_delay < min_delay)
            min_delay = group_delay;
    }
    *reinterpret_cast<std::uint32_t*>(inline_storage + 0u) = min_delay;
    *reinterpret_cast<std::uint32_t*>(inline_storage + 24u) = *reinterpret_cast<const std::uint32_t*>(
        reinterpret_cast<const std::uint8_t*>(spreset_words) + 6188u);
    xinn_engine1_compute_type5_biquad_partial(
        inline_storage,
        reinterpret_cast<const std::uint8_t*>(spreset_words));

    for (std::uint32_t group = 0u; group != 6u; ++group) {
        std::array<Beam, 98> rows{};
        const std::uint32_t output_group = kOutputGroup[group];
        const std::uint32_t side = group & 1u;
        const std::uint32_t opposite_side = side ^ 1u;
        const std::uint32_t* group_words = spreset_words + 7u + 192u * group;
        const std::uint32_t* primary = group_words + 96u * side;
        const std::uint32_t* secondary = group_words + 96u * opposite_side;
        std::uint32_t row = 0u;

        auto push_row = [&](std::uint32_t selector, std::uint32_t row_side, std::uint64_t packed, std::uint32_t base) {
            if (row >= rows.size())
                return;
            rows[row++] = Beam{
                output_group,
                selector,
                row_side,
                static_cast<std::uint32_t>(packed >> 32u),
                base - static_cast<std::uint32_t>(packed)};
        };

        push_row(0u, side, read_pair(primary), 7616u);
        for (std::uint32_t i = 1u; i < count_a; ++i)
            push_row(1u, side, read_pair(primary + 2u * i), 7617u);
        for (std::uint32_t i = 0u; i < count_a; ++i)
            push_row(1u, opposite_side, read_pair(secondary + 2u * i), row == 0u ? 7616u : 7617u);
        for (std::uint32_t i = count_a; i < count_b; ++i)
            push_row(1u, 0u, read_pair(group_words + 2u * i), row == 0u ? 7616u : 7617u);

        push_row(0u, opposite_side, read_pair(primary), 3808u);
        for (std::uint32_t i = 1u; i < count_a; ++i)
            push_row(1u, opposite_side, read_pair(primary + 2u * i), 3809u);
        for (std::uint32_t i = 0u; i < count_a; ++i)
            push_row(1u, side, read_pair(secondary + 2u * i), row == 49u ? 3808u : 3809u);
        for (std::uint32_t i = count_a; i < count_b; ++i)
            push_row(1u, 0u, read_pair(group_words + 96u + 2u * i), row == 49u ? 3808u : 3809u);

        if (row != rows.size())
            return false;
        std::sort(rows.begin(), rows.end(), [](const Beam& lhs, const Beam& rhs) {
            if (lhs.delay != rhs.delay)
                return lhs.delay < rhs.delay;
            float lhs_coefficient = 0.0f;
            float rhs_coefficient = 0.0f;
            std::memcpy(&lhs_coefficient, &lhs.coefficient, sizeof(lhs_coefficient));
            std::memcpy(&rhs_coefficient, &rhs.coefficient, sizeof(rhs_coefficient));
            return lhs_coefficient < rhs_coefficient;
        });

        std::uint32_t* cfg = engine1_words + kConfigBases[group];
        for (std::uint32_t i = 0u; i != rows.size(); ++i) {
            cfg[5u * i + 0u] = rows[i].gain_index;
            cfg[5u * i + 1u] = rows[i].selector;
            cfg[5u * i + 2u] = rows[i].side;
            cfg[5u * i + 3u] = rows[i].coefficient;
            cfg[5u * i + 4u] = rows[i].delay;
        }

        for (std::uint32_t i = 0u; i != rows.size(); i += 2u) {
            engine1_words[kCoeffBases[group] + 2u * i + 0u] = rows[i].delay;
            engine1_words[kCoeffBases[group] + 2u * i + 2u] = rows[i + 1u].delay;
        }
        const std::uint32_t max_lookup = engine1_words[kCoeffBases[group]] + 7616u;
        for (std::uint32_t j = 0u; j != 238u; ++j) {
            std::uint32_t threshold = 7616u - 32u * j;
            if (j == 0u)
                threshold = 0u;
            std::uint32_t idx = 0u;
            while (idx < rows.size() && threshold >= engine1_words[kCoeffBases[group] + 2u * idx])
                ++idx;
            if (idx == rows.size() && threshold >= max_lookup)
                return false;
            engine1_words[kLookupBases[group] + j] = idx == rows.size() ? 98u : idx;
        }
    }
    return true;
}

std::uint64_t xinn_inline_runtime_preset_from_processed_object_partial(const std::uint8_t* processed_preset) {
    if (!processed_preset || processed_preset[0] == 0u)
        return 0u;
    return reinterpret_cast<std::uint64_t>(processed_preset + 8u);
}

bool xinn_processed_preset_has_engine1_static_rows_partial(const std::uint8_t* processed_preset) {
    if (!processed_preset || processed_preset[0] == 0u)
        return false;
    const auto* engine1_words = reinterpret_cast<const std::uint32_t*>(processed_preset + 1136u);
    static constexpr std::uint32_t kConfigBases[6] = {
        0u,
        490u,
        980u,
        1470u,
        1960u,
        2450u,
    };
    for (std::uint32_t base : kConfigBases) {
        for (std::uint32_t row = 0u; row != 490u; row += 5u) {
            const std::uint32_t gain_index = engine1_words[base + row + 0u];
            const std::uint32_t selector = engine1_words[base + row + 1u];
            const std::uint32_t inline_gain = engine1_words[base + row + 2u];
            const std::uint32_t coefficient = engine1_words[base + row + 3u];
            if (gain_index < 3u && (selector != 0u || inline_gain != 0u || coefficient != 0u))
                return true;
        }
    }
    return false;
}

void xinn_retune_processed_preset_partial(
    std::uint8_t* processed_preset,
    std::uint32_t mode,
    const std::uint8_t* update_blob) {
    if (!processed_preset || !update_blob || processed_preset[0] == 0u)
        return;
    if (processed_preset[1] == kXinnProcessedPresetMarker)
        return;
    const bool can_apply_engine1 = xinn_processed_preset_has_engine1_static_rows_partial(processed_preset);
    if (mode == 1u && *reinterpret_cast<const std::uint32_t*>(update_blob + 68u) != 0u) {
        if (can_apply_engine1) {
            xinn_preset_tune_stereo_35b8a0_partial(
                processed_preset,
                reinterpret_cast<const float*>(update_blob + 72u));
        } else {
            xinn_preset_tune_late_stereo_35b8a0_partial(
                processed_preset,
                reinterpret_cast<const float*>(update_blob + 72u));
        }
    } else if (mode == 2u && *reinterpret_cast<const std::uint32_t*>(update_blob + 160u) != 0u) {
        if (can_apply_engine1) {
            xinn_preset_tune_surround_35bba0_partial(
                processed_preset,
                reinterpret_cast<const float*>(update_blob + 164u));
        } else {
            xinn_preset_tune_late_surround_35bba0_partial(
                processed_preset,
                reinterpret_cast<const float*>(update_blob + 164u));
        }
    }
}

std::uint32_t xinn_prepare_mode_from_input_mask_portable(std::uint32_t input_mask) {
    const std::uint32_t m = input_mask & 0x7FFFFFFu;
    std::uint32_t mode = ((~m & 0x33u) != 0u)
        ? (((~m & 3u) == 0u) ? 1u : 0u)
        : 2u;
    return mode;
}

void xinn_fill_tuning_static_defaults_portable(
    std::uint8_t* tuning_static,
    bool surround_mode,
    std::uint32_t room_preset) {
    if (!tuning_static)
        return;

    std::array<std::uint8_t, 120u> dynamic{};
    auro_matic_v3_XinN_parameter_Dynamic_t_default_4e9fec_partial(dynamic.data());
    *reinterpret_cast<std::uint32_t*>(tuning_static + 0u) = surround_mode ? 1u : 0u;
    *reinterpret_cast<std::uint32_t*>(tuning_static + 4u) = std::min<std::uint32_t>(room_preset, 4u);
    *reinterpret_cast<std::uint64_t*>(tuning_static + 8u) =
        *reinterpret_cast<const std::uint64_t*>(dynamic.data() + 0u);
    *reinterpret_cast<std::uint32_t*>(tuning_static + 16u) =
        *reinterpret_cast<const std::uint32_t*>(dynamic.data() + 8u);
    *reinterpret_cast<std::uint64_t*>(tuning_static + 20u) =
        *reinterpret_cast<const std::uint64_t*>(dynamic.data() + 12u);
    *reinterpret_cast<std::uint32_t*>(tuning_static + 28u) =
        *reinterpret_cast<const std::uint32_t*>(dynamic.data() + 20u);
    if (!surround_mode)
        return;
    *reinterpret_cast<std::uint64_t*>(tuning_static + 32u) =
        *reinterpret_cast<const std::uint64_t*>(dynamic.data() + 24u);
    *reinterpret_cast<std::uint32_t*>(tuning_static + 40u) =
        *reinterpret_cast<const std::uint32_t*>(dynamic.data() + 32u);
}

void xinn_fill_tuning_dynamic_defaults_portable(std::uint8_t* tuning_dynamic, bool surround_mode) {
    if (!tuning_dynamic)
        return;

    std::array<std::uint8_t, 120u> dynamic{};
    auro_matic_v3_XinN_parameter_Dynamic_t_default_4e9fec_partial(dynamic.data());
    std::memcpy(tuning_dynamic, dynamic.data(), surround_mode ? 112u : 96u);
}

void xinn_write_plan_update_blobs_portable(
    std::uint8_t* plan,
    std::uint8_t* update,
    std::uint32_t input_mask,
    std::uint32_t output_mask,
    std::uint32_t mode,
    std::uint32_t room_preset) {
    if (!plan || !update)
        return;
    if (mode != 1u && mode != 2u) {
        std::memset(plan, 0, kXinnPlanBlobBytesPortable);
        std::memset(update, 0, kXinnUpdateBlobBytesPortable);
        return;
    }
    std::memset(plan, 0, kXinnPlanBlobBytesPortable);
    std::memset(update, 0, kXinnUpdateBlobBytesPortable);
    *reinterpret_cast<std::uint32_t*>(plan + 0u) = input_mask;
    *reinterpret_cast<std::uint32_t*>(plan + 4u) = output_mask;
    *reinterpret_cast<std::uint32_t*>(plan + 8u) = mode;
    if (mode == 1u) {
        xinn_fill_tuning_static_defaults_portable(plan + 12u, false, room_preset);
        plan[44u] = 1u;
        *reinterpret_cast<std::uint32_t*>(update + 68u) = 1u;
        xinn_fill_tuning_dynamic_defaults_portable(update + 72u, false);
    } else {
        xinn_fill_tuning_static_defaults_portable(plan + 48u, true, room_preset);
        plan[92u] = 1u;
        *reinterpret_cast<std::uint32_t*>(update + 160u) = 1u;
        xinn_fill_tuning_dynamic_defaults_portable(update + 164u, true);
    }
}

void xinn_fill_v3_dynamic_from_v4_static_partial(
    std::uint8_t* dynamic_120,
    bool surround_mode,
    const std::uint8_t* tuning_static) {
    if (!dynamic_120 || !tuning_static)
        return;

    auro_matic_v3_XinN_parameter_Dynamic_t_default_4e9fec_partial(dynamic_120);
    if (surround_mode) {
        *reinterpret_cast<std::uint64_t*>(dynamic_120 + 0u) =
            *reinterpret_cast<const std::uint64_t*>(tuning_static + 8u);
        *reinterpret_cast<std::uint32_t*>(dynamic_120 + 8u) =
            *reinterpret_cast<const std::uint32_t*>(tuning_static + 16u);
        *reinterpret_cast<std::uint64_t*>(dynamic_120 + 12u) =
            *reinterpret_cast<const std::uint64_t*>(tuning_static + 20u);
        *reinterpret_cast<std::uint32_t*>(dynamic_120 + 20u) =
            *reinterpret_cast<const std::uint32_t*>(tuning_static + 28u);
        *reinterpret_cast<std::uint64_t*>(dynamic_120 + 24u) =
            *reinterpret_cast<const std::uint64_t*>(tuning_static + 32u);
        *reinterpret_cast<std::uint32_t*>(dynamic_120 + 32u) =
            *reinterpret_cast<const std::uint32_t*>(tuning_static + 40u);
        return;
    }

    *reinterpret_cast<std::uint64_t*>(dynamic_120 + 0u) =
        *reinterpret_cast<const std::uint64_t*>(tuning_static + 8u);
    *reinterpret_cast<std::uint32_t*>(dynamic_120 + 8u) =
        *reinterpret_cast<const std::uint32_t*>(tuning_static + 16u);
    *reinterpret_cast<std::uint64_t*>(dynamic_120 + 24u) =
        *reinterpret_cast<const std::uint64_t*>(tuning_static + 20u);
    *reinterpret_cast<std::uint32_t*>(dynamic_120 + 32u) =
        *reinterpret_cast<const std::uint32_t*>(tuning_static + 28u);
}

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_initialize_35b2c0_partial(
    std::uint64_t step_base,
    const std::uint64_t* memory_3) {
    if (step_base == 0u || !memory_3)
        return 1;
    auto* step = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(step_base));
    const std::uint64_t memory_args[5] = {
        step_base + 0x378u,
        step_base + 0xA038u,
        step_base + 0x13CF8u,
        memory_3[1],
        memory_3[2],
    };
    if (auro_matic_v3_XinN_fl32_construct_556180_partial(step + 40u, memory_args) == 0)
        return 1;
    *reinterpret_cast<std::uint64_t*>(step + 483952u) = memory_3[0] + 88u;
    return 0;
}

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_initialize_355cf8_partial(
    std::uint64_t step_base,
    const std::uint64_t* memory_3) {
    return auro_a3deng_v4_pipeline_step_upmix_XinN_initialize_35b2c0_partial(step_base, memory_3);
}

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_prepare_35b330_partial(
    std::uint64_t step_base,
    const std::uint8_t* plan_blob,
    const std::uint8_t* update_blob) {
    if (step_base == 0u || !plan_blob || !update_blob)
        return 1;

    auto* step = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(step_base));
    const std::uint32_t input_mask = *reinterpret_cast<const std::uint32_t*>(plan_blob + 0u);
    const std::uint32_t output_mask = *reinterpret_cast<const std::uint32_t*>(plan_blob + 4u);
    const std::uint32_t mode = *reinterpret_cast<const std::uint32_t*>(plan_blob + 8u);
    *reinterpret_cast<std::uint32_t*>(step + 460088u) = mode;
    auto* processed_preset = step + 460096u;
    auto* dynamic_120 = step + 483832u;

    std::uint64_t runtime_preset = 0u;
    std::uint64_t spreset = 0u;
    if (mode == 1u) {
        if (plan_blob[44u] == 0u || *reinterpret_cast<const std::uint32_t*>(update_blob + 68u) != 1u)
            return 1;
        runtime_preset = xinn_select_native_runtime_preset_from_v4_static_partial(false, plan_blob + 12u);
        spreset = xinn_select_native_spreset_from_v4_static_partial(false, plan_blob + 12u);
        xinn_fill_v3_dynamic_from_v4_static_partial(dynamic_120, false, plan_blob + 12u);
    } else if (mode == 2u) {
        if (plan_blob[92u] == 0u || *reinterpret_cast<const std::uint32_t*>(update_blob + 160u) != 1u)
            return 1;
        runtime_preset = xinn_select_native_runtime_preset_from_v4_static_partial(true, plan_blob + 48u);
        spreset = xinn_select_native_spreset_from_v4_static_partial(true, plan_blob + 48u);
        xinn_fill_v3_dynamic_from_v4_static_partial(dynamic_120, true, plan_blob + 48u);
    } else {
        auro_matic_v3_XinN_parameter_Dynamic_t_default_4e9fec_partial(dynamic_120);
    }
    xinn_copy_runtime_preset_into_processed_object_partial(processed_preset, runtime_preset);
    if (processed_preset[0] == 0u)
        (void)xinn_initialize_processed_preset_from_portable_spreset_partial(processed_preset, spreset);
    if (processed_preset[0] == 0u)
        return 1;
    xinn_retarget_processed_runtime_preset_backing_partial(processed_preset);
    xinn_engine2_initialize_static_from_spreset_partial(processed_preset, spreset);
    (void)xinn_engine1_initialize_static_from_spreset_partial(processed_preset, spreset);
    xinn_retune_processed_preset_partial(processed_preset, mode, update_blob);
    const std::uint64_t preset = xinn_inline_runtime_preset_from_processed_object_partial(processed_preset);

    const auto* sample_rate_words = reinterpret_cast<const std::uint32_t*>(
        *reinterpret_cast<const std::uint64_t*>(step + 8u));
    const std::uint32_t sample_rate = sample_rate_words ? sample_rate_words[2] : 0u;
    const std::uint64_t config_args[3] = {
        static_cast<std::uint64_t>(input_mask) | (static_cast<std::uint64_t>(output_mask) << 32u),
        preset,
        reinterpret_cast<std::uint64_t>(dynamic_120),
    };
    if (auro_matic_v3_XinN_fl32_configure_556330_partial(step + 40u, sample_rate, config_args) != 0)
        return 1;
    return auro_a3deng_v4_pipeline_step_upmix_XinN_update_35b740_partial(step_base, update_blob);
}

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_reset_audio_state_35b730_partial(std::uint64_t step_base) {
    if (step_base == 0u)
        return 0;
    auto* step = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(step_base));
    return auro_matic_v3_XinN_fl32_reset_audio_state_556240_partial(step + 40u);
}

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_reset_audio_state_35609c_partial(std::uint64_t step_base) {
    return auro_a3deng_v4_pipeline_step_upmix_XinN_reset_audio_state_35b730_partial(step_base);
}

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_update_35b740_partial(
    std::uint64_t step_base,
    const std::uint8_t* update_blob) {
    if (step_base == 0u || !update_blob)
        return 0;
    auto* step = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(step_base));
    const std::uint32_t mode = *reinterpret_cast<const std::uint32_t*>(step + 460088u);
    bool changed = false;
    if (mode == 2u)
        changed = *reinterpret_cast<const std::uint32_t*>(update_blob + 160u) != 0u;
    else if (mode == 1u)
        changed = *reinterpret_cast<const std::uint32_t*>(update_blob + 68u) != 0u;
    if (!changed)
        return 0;

    xinn_retune_processed_preset_partial(step + 460096u, mode, update_blob);
    const std::uint64_t preset = xinn_inline_runtime_preset_from_processed_object_partial(step + 460096u);
    auro_matic_v3_XinN_fl32_set_preset_556250_partial(step + 40u, preset);
    auro_matic_v3_XinN_fl32_set_dynamic_parameters_556200_partial(step + 40u, step + 483832u);
    return 0;
}

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_update_3560a4_partial(
    std::uint64_t step_base,
    const std::uint8_t* update_blob) {
    return auro_a3deng_v4_pipeline_step_upmix_XinN_update_35b740_partial(step_base, update_blob);
}

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_calculate_info_35b7d0_partial(
    std::uint64_t step_base,
    std::uint8_t* info_base) {
    if (step_base != 0u && info_base != nullptr) {
        auto* step = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(step_base));
        auro_matic_v3_XinN_fl32_update_peak_amplitude_556450_partial(
            step + 40u,
            reinterpret_cast<float*>(info_base + 4u));
    }
    return 0;
}

namespace {

void xinn_route_mix_pair(
    void** channel_span_31,
    std::uint32_t left_idx,
    std::uint32_t right_idx,
    const float* src_a_192,
    std::uint32_t src_a_base,
    float gain_a,
    const float* src_b_192,
    std::uint32_t src_b_base,
    float gain_b,
    const float* src_c_192,
    std::uint32_t src_c_base,
    float gain_c) {
    if (!channel_span_31 || left_idx >= ::auro_engine_v4_ida::kChannelCount
        || right_idx >= ::auro_engine_v4_ida::kChannelCount)
        return;
    auto* left = static_cast<float*>(channel_span_31[left_idx]);
    auto* right = static_cast<float*>(channel_span_31[right_idx]);
    if (!left || !right)
        return;
    for (std::uint32_t i = 0; i < 32u; ++i) {
        const float a_l = src_a_192 ? src_a_192[src_a_base + i] : 0.0f;
        const float a_r = src_a_192 ? src_a_192[src_a_base + 32u + i] : 0.0f;
        const float b_l = src_b_192 ? src_b_192[src_b_base + i] : 0.0f;
        const float b_r = src_b_192 ? src_b_192[src_b_base + 32u + i] : 0.0f;
        const float c_l = src_c_192 ? src_c_192[src_c_base + i] : 0.0f;
        const float c_r = src_c_192 ? src_c_192[src_c_base + 32u + i] : 0.0f;
        left[i] = gain_a * a_l + gain_b * b_l + gain_c * c_l;
        right[i] = gain_a * a_r + gain_b * b_r + gain_c * c_r;
    }
}

void xinn_route_mix_mono(
    void** channel_span_31,
    std::uint32_t idx,
    const float* src_a_192,
    std::uint32_t src_a_base,
    float gain_a,
    const float* src_b_192,
    std::uint32_t src_b_base,
    float gain_b,
    const float* src_c_192,
    std::uint32_t src_c_base,
    float gain_c,
    bool sum_c_stereo) {
    if (!channel_span_31 || idx >= ::auro_engine_v4_ida::kChannelCount)
        return;
    auto* dst = static_cast<float*>(channel_span_31[idx]);
    if (!dst)
        return;
    for (std::uint32_t i = 0; i < 32u; ++i) {
        const float a = src_a_192 ? src_a_192[src_a_base + i] : 0.0f;
        const float b = src_b_192 ? src_b_192[src_b_base + i] : 0.0f;
        float c = src_c_192 ? src_c_192[src_c_base + i] : 0.0f;
        if (sum_c_stereo && src_c_192)
            c += src_c_192[src_c_base + 32u + i];
        dst[i] = gain_a * a + gain_b * b + gain_c * c;
    }
}

std::uint64_t xinn_routing_state(std::uint8_t* xinn) {
    std::uint64_t routing = e2_read_u64(xinn, 552u);
    if (routing == 0u)
        routing = reinterpret_cast<std::uint64_t>(xinn + 624u);
    return routing;
}

void xinn_early_process_dispatch_574e30(
    std::uint8_t* early,
    void** channel_span_31,
    float* early_a,
    float* early_b,
    const float* gains,
    std::uint32_t fallback_mode) {
    if (!early)
        return;
    const std::uint64_t process_u64 = e2_read_u64(early, 296u);
    if (process_u64 == 0x5776E0u) {
        auro_matic_XinN_Early_fl32_process_mode2_5776e0_partial(
            early,
            channel_span_31,
            early_a,
            early_b,
            gains);
        return;
    }
    if (process_u64 == 0x5773E0u) {
        auro_matic_XinN_Early_fl32_process_mode1_5773e0_partial(
            early,
            channel_span_31,
            early_a,
            gains);
        return;
    }
    if (fallback_mode == 2u) {
        auro_matic_XinN_Early_fl32_process_mode2_5776e0_partial(
            early,
            channel_span_31,
            early_a,
            early_b,
            gains);
    } else {
        auro_matic_XinN_Early_fl32_process_mode1_5773e0_partial(
            early,
            channel_span_31,
            early_a,
            gains);
    }
}

std::int64_t xinn_process_core_574e30_574ee0(
    std::uint64_t xinn_state,
    void** channel_span_31,
    float* early_a,
    float* early_b,
    float* late) {
    if (xinn_state == 0u || !channel_span_31 || !early_a || !early_b || !late)
        return 0;
    auto* xinn = e2_ptr(xinn_state);
    std::memset(early_a, 0, kEngine2OutputFloats * sizeof(float));
    std::memset(early_b, 0, kEngine2OutputFloats * sizeof(float));
    std::memset(late, 0, kEngine2OutputFloats * sizeof(float));

    float gains[32]{};
    auro_audio_Smooth_fl32_inst_gains_smooth_5aae00_partial(
        reinterpret_cast<float*>(xinn + 560u),
        gains,
        32u);

    const std::uint32_t mode = e2_read_u32(xinn, 0u);
    auto* early = xinn + 48u;
    xinn_early_process_dispatch_574e30(early, channel_span_31, early_a, early_b, gains, mode);

    (void)auro_matic_XinN_Late_fl32_process_5799a0_partial(
        reinterpret_cast<std::uint32_t*>(xinn + 392u),
        reinterpret_cast<std::uint64_t>(early),
        late,
        auro_matic_Engine2_fl32_process_57b0e0_partial);

    return auro_matic_v3_XinN_Routing_fl32_apply_5575c0_partial(
        xinn_routing_state(xinn),
        early_a,
        early_b,
        late,
        channel_span_31,
        mode);
}

} // namespace

std::int64_t auro_matic_v3_XinN_Routing_fl32_apply_5575c0_partial(
    std::uint64_t routing_state,
    const float* early_a_0x300,
    const float* early_b_0x300,
    const float* late_0x300,
    void** channel_span_31,
    std::uint32_t mode) {
    if (routing_state == 0u || !channel_span_31 || !early_a_0x300 || !late_0x300)
        return 0;
    const auto* routing = e2_cptr(routing_state);
    const std::uint32_t configured_count = e2_read_u32(routing, 64u);
    if (configured_count != 0u) {
        const std::uint32_t n = std::min<std::uint32_t>(configured_count, 4u);
        for (std::uint32_t i = 0; i < n; ++i) {
            switch (e2_read_u64(routing, 32u + 8ull * i)) {
            case 0x559810u:
                xinn_route_mix_pair(
                    channel_span_31,
                    4u,
                    5u,
                    early_a_0x300,
                    0u,
                    e2_read_f32(routing, 144u),
                    nullptr,
                    0u,
                    0.0f,
                    late_0x300,
                    0u,
                    e2_read_f32(routing, 148u));
                break;
            case 0x559AB0u:
                xinn_route_mix_pair(
                    channel_span_31,
                    9u,
                    10u,
                    early_a_0x300,
                    64u,
                    e2_read_f32(routing, 152u),
                    nullptr,
                    0u,
                    0.0f,
                    late_0x300,
                    64u,
                    e2_read_f32(routing, 156u));
                break;
            case 0x559D90u:
                xinn_route_mix_pair(
                    channel_span_31,
                    13u,
                    14u,
                    early_a_0x300,
                    128u,
                    e2_read_f32(routing, 160u),
                    nullptr,
                    0u,
                    0.0f,
                    late_0x300,
                    128u,
                    e2_read_f32(routing, 164u));
                break;
            case 0x55A070u:
                xinn_route_mix_pair(
                    channel_span_31,
                    9u,
                    10u,
                    early_a_0x300,
                    64u,
                    e2_read_f32(routing, 168u),
                    early_b_0x300,
                    128u,
                    e2_read_f32(routing, 172u),
                    late_0x300,
                    64u,
                    e2_read_f32(routing, 176u));
                break;
            case 0x55A4C0u:
                xinn_route_mix_pair(
                    channel_span_31,
                    13u,
                    14u,
                    early_a_0x300,
                    128u,
                    e2_read_f32(routing, 192u),
                    early_b_0x300,
                    64u,
                    e2_read_f32(routing, 196u),
                    late_0x300,
                    128u,
                    e2_read_f32(routing, 200u));
                break;
            case 0x55A910u:
                xinn_route_mix_mono(
                    channel_span_31,
                    11u,
                    early_a_0x300,
                    0u,
                    e2_read_f32(routing, 180u),
                    early_b_0x300,
                    0u,
                    e2_read_f32(routing, 184u),
                    late_0x300,
                    0u,
                    e2_read_f32(routing, 188u),
                    false);
                break;
            case 0x55AAC0u:
                xinn_route_mix_mono(
                    channel_span_31,
                    12u,
                    early_a_0x300,
                    32u,
                    e2_read_f32(routing, 204u),
                    early_b_0x300,
                    32u,
                    e2_read_f32(routing, 208u),
                    late_0x300,
                    0u,
                    e2_read_f32(routing, 212u),
                    true);
                break;
            default:
                break;
            }
        }
        return static_cast<std::int64_t>(routing_state);
    }
    if (mode == 2u) {
        xinn_route_mix_pair(
            channel_span_31,
            9u,
            10u,
            early_a_0x300,
            64u,
            e2_read_f32(routing, 168u),
            early_b_0x300,
            128u,
            e2_read_f32(routing, 172u),
            late_0x300,
            64u,
            e2_read_f32(routing, 176u));
        xinn_route_mix_mono(
            channel_span_31,
            11u,
            early_a_0x300,
            0u,
            e2_read_f32(routing, 180u),
            early_b_0x300,
            0u,
            e2_read_f32(routing, 184u),
            late_0x300,
            0u,
            e2_read_f32(routing, 188u),
            false);
        xinn_route_mix_pair(
            channel_span_31,
            13u,
            14u,
            early_a_0x300,
            128u,
            e2_read_f32(routing, 192u),
            early_b_0x300,
            64u,
            e2_read_f32(routing, 196u),
            late_0x300,
            128u,
            e2_read_f32(routing, 200u));
        xinn_route_mix_mono(
            channel_span_31,
            12u,
            early_a_0x300,
            32u,
            e2_read_f32(routing, 204u),
            early_b_0x300,
            32u,
            e2_read_f32(routing, 208u),
            late_0x300,
            0u,
            e2_read_f32(routing, 212u),
            true);
        return static_cast<std::int64_t>(routing_state);
    }

    xinn_route_mix_pair(
        channel_span_31,
        4u,
        5u,
        early_a_0x300,
        0u,
        e2_read_f32(routing, 144u),
        nullptr,
        0u,
        0.0f,
        late_0x300,
        0u,
        e2_read_f32(routing, 148u));
    xinn_route_mix_pair(
        channel_span_31,
        9u,
        10u,
        early_a_0x300,
        64u,
        e2_read_f32(routing, 152u),
        nullptr,
        0u,
        0.0f,
        late_0x300,
        64u,
        e2_read_f32(routing, 156u));
    xinn_route_mix_pair(
        channel_span_31,
        13u,
        14u,
        early_a_0x300,
        128u,
        e2_read_f32(routing, 160u),
        nullptr,
        0u,
        0.0f,
        late_0x300,
        128u,
        e2_read_f32(routing, 164u));
    return static_cast<std::int64_t>(routing_state);
}

std::int64_t auro_matic_XinN_fl32_process_inplace_574e30_partial(std::uint64_t xinn_state, void** channel_span_31) {
    if (xinn_state == 0u)
        return 0;
    auto* xinn = e2_ptr(xinn_state);
    const std::uint64_t scratch_u64 = e2_read_u64(xinn, 40u);
    if (scratch_u64 == 0u)
        return auro_matic_XinN_fl32_process_scratch_574ee0_partial(xinn_state, channel_span_31);
    auto* scratch = reinterpret_cast<float*>(static_cast<std::uintptr_t>(scratch_u64));
    return xinn_process_core_574e30_574ee0(
        xinn_state,
        channel_span_31,
        scratch,
        scratch + kEngine2OutputFloats,
        scratch + 2u * kEngine2OutputFloats);
}

std::int64_t auro_matic_XinN_fl32_process_scratch_574ee0_partial(std::uint64_t xinn_state, void** channel_span_31) {
    std::array<float, kEngine2OutputFloats> early_a{};
    std::array<float, kEngine2OutputFloats> early_b{};
    std::array<float, kEngine2OutputFloats> late{};
    return xinn_process_core_574e30_574ee0(
        xinn_state,
        channel_span_31,
        early_a.data(),
        early_b.data(),
        late.data());
}

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_process_35b440_partial(
    std::uint64_t step_base,
    void** io_channels_31,
    std::uint32_t subblock_count,
    const std::uint8_t* block_records_516,
    AuroMaticV3XinNFl32ProcessFn process_fl32,
    AuroA3dengV4XinNResetFn reset_audio_state) {
    if (step_base == 0u || !io_channels_31)
        return 1;
    auto* process = process_fl32 ? process_fl32 : auro_matic_v3_XinN_fl32_process_556440_partial;
    const auto* step = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(step_base));
    const std::uint64_t sample_rate_words_u64 = *reinterpret_cast<const std::uint64_t*>(step + 8u);
    if (sample_rate_words_u64 != 0u) {
        const auto* sample_rate_words = reinterpret_cast<const std::uint32_t*>(
            static_cast<std::uintptr_t>(sample_rate_words_u64));
        subblock_count = sample_rate_words[1];
    }

    std::array<void*, ::auro_engine_v4_ida::kChannelCount> span{};
    for (std::uint32_t block = 0; block < subblock_count; ++block) {
        const std::uint8_t* record = block_records_516
            ? block_records_516 + static_cast<std::size_t>(block) * 516u
            : nullptr;
        if (record && record[1] != 0u) {
            if (record[0] != 0u && reset_audio_state)
                (void)reset_audio_state(step_base);
            continue;
        }

        const std::uintptr_t byte_offset = static_cast<std::uintptr_t>(block) * 32u * sizeof(float);
        for (std::size_t ch = 0; ch < span.size(); ++ch) {
            auto* base = static_cast<std::uint8_t*>(io_channels_31[ch]);
            span[ch] = base ? base + byte_offset : nullptr;
        }
        (void)process(step_base + 40u, span.data());
    }
    return 0;
}

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_process_355eac_partial(
    std::uint64_t step_base,
    void** io_channels_31,
    std::uint32_t subblock_count,
    const std::uint8_t* block_records_516,
    AuroMaticV3XinNFl32ProcessFn process_fl32,
    AuroA3dengV4XinNResetFn reset_audio_state) {
    return auro_a3deng_v4_pipeline_step_upmix_XinN_process_35b440_partial(
        step_base,
        io_channels_31,
        subblock_count,
        block_records_516,
        process_fl32,
        reset_audio_state);
}

void auro_a3deng_v3_pipeline_step_Upmix_set_bypass(std::uint32_t* upmix_base, std::int64_t bypass) {
    if (!upmix_base)
        return;
    *reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uint8_t*>(upmix_base) + 4u) =
        (bypass != 0) ? 1u : 0u;
}

std::int32_t auro_a3deng_v3_pipeline_Manager_get_internal_channel_id(
    std::uint32_t* manager_base,
    std::uint32_t external_channel,
    std::int32_t* out_internal_channel,
    double /*reserved*/) {
    if (!manager_base || !out_internal_channel || external_channel >= 27u)
        return 1;
    auto* remap = reinterpret_cast<std::uint8_t*>(manager_base) + 88u;
    using RemapFn = std::int64_t (*)(std::uint32_t, std::int64_t);
    auto* fn = *reinterpret_cast<RemapFn*>(remap + 16u);
    if (!fn) {
        *out_internal_channel = static_cast<std::int32_t>(external_channel);
        return 1;
    }
    return static_cast<std::int32_t>(
        fn(external_channel, reinterpret_cast<std::int64_t>(out_internal_channel)));
}

void auro_memory_block_Accumulator_t_construct(
    std::uint64_t* acc,
    std::int64_t /*a2*/,
    std::int64_t /*a3*/,
    void (* /*cb*/)(std::uint64_t, std::int64_t),
    std::int64_t /*a5*/,
    std::int64_t /*a6*/,
    std::uint32_t initial_blocks,
    std::uint32_t align,
    std::uint64_t /*mask*/,
    std::uint64_t /*tail*/) {
    (void)initial_blocks;
    (void)align;
    if (!acc)
        return;
    // IDA 0x13A360: *a1 = xmmword_1C36B0 (нулевое начальное состояние аккумулятора).
    acc[0] = 0;
    acc[1] = 0;
}

void auro_codec_v3_Decoder_t_required_additional_memory(std::uint64_t* acc, std::uint64_t* out_pair) {
    if (!acc || !out_pair)
        return;
    // IDA 0x101730: Config_initialize(tmp, out_pair) -> Memory_t_required_additional_memory(acc, tmp)
    std::uint8_t cfg_raw[64]{};
    const auto rc = auro_codec_v3_decoder_Config_initialize(
        reinterpret_cast<std::int64_t>(cfg_raw),
        reinterpret_cast<std::int64_t>(out_pair));
    if (rc != 0)
        return;

    const auto* cfg_u32 = reinterpret_cast<const std::uint32_t*>(cfg_raw);
    const auto* cfg_u64 = reinterpret_cast<const std::uint64_t*>(cfg_raw);

    const std::uint32_t block_samples =
        cfg_u32[auro_codec_v3_ida::kDecoderConfig_off_block_bits / 4u];
    const std::uint32_t stage1_count =
        cfg_u32[auro_codec_v3_ida::kDecoderConfig_off_stage1_count / 4u];
    const std::uint32_t input_mask_count =
        cfg_u32[auro_codec_v3_ida::kDecoderConfig_off_input_mask_count / 4u];
    const std::uint64_t cfg_qword0 = cfg_u64[0];
    const std::uint64_t cfg_qword16 = cfg_u64[2];

    const std::uint64_t total =
        memory_delay_line_payload_sum_1068f0_partial(cfg_qword0, cfg_qword16, input_mask_count)
        + memory_frame_deque_required_additional_memory_13d570_partial(
            static_cast<std::uint32_t>(((2u * block_samples + 255u) >> 8) + 1u))
        + memory_frame_deque_required_additional_memory_13d570_partial(
            static_cast<std::uint32_t>(((block_samples * stage1_count + 255u) >> 8) + 1u))
        + memory_block_info_required_additional_memory_13d750_partial(
            memory_block_info_slot_count_106d20_partial(block_samples))
        + parse_result_pool_required_additional_memory_107190_partial(
            static_cast<std::uint32_t>(9u * ((block_samples * stage1_count + 255u) >> 8) + 9u))
        + memory_accumulator_tail_payload_106d20_partial(cfg_qword0);

    acc[0] = total;
    acc[1] = 8u;
}

void auro_memory_block_Distributor_t_construct(void* dist, std::uint64_t* base, const char* type_tag) {
    auto* q = reinterpret_cast<std::uint64_t*>(dist);
    if (!q)
        return;
    q[0] = reinterpret_cast<std::uint64_t>(base);
    q[1] = reinterpret_cast<std::uint64_t>(type_tag);
}

std::uint32_t block_info_construct_13d750_partial(std::uint8_t* block_info, std::uint32_t slot_count) {
    if (!block_info)
        return 0u;
    const auto alloc = [](std::uint64_t bytes, std::size_t align) -> std::uint64_t {
        if (bytes == 0u)
            return 0u;
        const std::size_t rounded = static_cast<std::size_t>(
            (bytes + align - 1u) & ~(static_cast<std::uint64_t>(align) - 1u));
        return reinterpret_cast<std::uint64_t>(std::calloc(1u, rounded));
    };

    const std::uint64_t ranges = alloc(24ull * slot_count, 8u);
    *reinterpret_cast<std::uint64_t*>(block_info + 0u) = ranges;
    if (ranges == 0u && slot_count != 0u)
        return 0u;

    const std::uint64_t flags = alloc(4ull * slot_count, 4u);
    *reinterpret_cast<std::uint64_t*>(block_info + 8u) = flags;
    if (flags == 0u && slot_count != 0u)
        return 0u;

    const std::uint64_t frame_ptrs = alloc(8ull * slot_count, 8u);
    *reinterpret_cast<std::uint64_t*>(block_info + 16u) = frame_ptrs;
    if (frame_ptrs == 0u && slot_count != 0u)
        return 0u;

    *reinterpret_cast<std::uint32_t*>(block_info + 24u) = 0u;
    if (ranges == 0u)
        return 0u;
    return static_cast<std::uint32_t>((frame_ptrs != 0u) & (flags != 0u));
}

namespace {

constexpr std::uintptr_t kMemory_off_ready_parse_storage = 168u;

std::uint64_t codec_v3_alloc_zero_52acb0(std::uint64_t bytes, std::size_t align) {
    if (bytes == 0u)
        return 0u;
    const std::size_t rounded = static_cast<std::size_t>((bytes + align - 1u) & ~(static_cast<std::uint64_t>(align) - 1u));
    void* p = std::calloc(1u, rounded);
    return reinterpret_cast<std::uint64_t>(p);
}

bool codec_v3_frame_deque_construct_native_530240(
    std::uint8_t* dq,
    std::uint32_t capacity,
    std::uint64_t frame_span) {
    if (!dq)
        return false;
    std::memset(dq, 0, 32u);
    const std::uint32_t cap = std::max<std::uint32_t>(1u, capacity);
    *reinterpret_cast<std::uint64_t*>(dq + 16u) = codec_v3_alloc_zero_52acb0(frame_span * cap, 8u);
    if (*reinterpret_cast<std::uint64_t*>(dq + 16u) == 0u && cap != 0u)
        return false;
    *reinterpret_cast<std::uint32_t*>(dq + 24u) = cap;
    return true;
}

bool codec_v3_memory_construct_local_5300d0(std::uint8_t* memory_base, const std::uint8_t* cfg) {
    if (!memory_base || !cfg)
        return false;

    const std::uint64_t block_samples = *reinterpret_cast<const std::uint64_t*>(cfg + 0u);
    const std::uint64_t ring_slots = *reinterpret_cast<const std::uint64_t*>(cfg + 16u);
    const std::uint32_t input_mask = *reinterpret_cast<const std::uint32_t*>(cfg + 36u);
    const std::uint32_t input_count = *reinterpret_cast<const std::uint32_t*>(cfg + 44u);
    auto* delay = reinterpret_cast<DelayLineState106b40*>(memory_base);
    std::memset(delay, 0, sizeof(*delay));
    delay->samples_per_block = static_cast<std::uint32_t>(block_samples);
    delay->ring_slot_count = static_cast<std::uint32_t>(ring_slots);
    delay->reserved_8 = codec_v3_alloc_zero_52acb0(4ull * block_samples * ring_slots * input_count, 4u);
    delay->ring_storage_base = codec_v3_alloc_zero_52acb0(kDelayLineBufferSlotStrideBytes * ring_slots, 8u);
    if ((block_samples * ring_slots * input_count) != 0u && delay->reserved_8 == 0u)
        return false;
    if (ring_slots != 0u && delay->ring_storage_base == 0u)
        return false;

    if (delay->ring_storage_base != 0u) {
        std::uint64_t sample_base = delay->reserved_8;
        for (std::uint64_t slot = 0; slot < ring_slots; ++slot) {
            auto* desc = reinterpret_cast<std::uint8_t*>(
                static_cast<std::uintptr_t>(delay->ring_storage_base + slot * kDelayLineBufferSlotStrideBytes));
            *reinterpret_cast<std::uint32_t*>(desc) = 0u;
            for (std::uint32_t ch = 0u; ch != kCodecV3ChannelCount; ++ch) {
                const std::uint64_t ptr = ((input_mask >> ch) & 1u) != 0u ? sample_base : 0u;
                *reinterpret_cast<std::uint64_t*>(desc + 16u + 8u * ch) = ptr;
                if (ptr != 0u)
                    sample_base += 4ull * block_samples;
            }
        }
    }

    const std::uint32_t block_u32 = static_cast<std::uint32_t>(block_samples);
    if (!codec_v3_frame_deque_construct_native_530240(
        memory_base + 40u,
        ((2u * block_u32 + 255u) >> 8u) + 1u,
        336u)) {
        return false;
    }
    if (!codec_v3_frame_deque_construct_native_530240(
        memory_base + 72u,
        ((block_u32 * *reinterpret_cast<const std::uint32_t*>(cfg + 28u) + 255u) >> 8u) + 1u,
        336u)) {
        return false;
    }

    const std::uint32_t block_info_count = memory_block_info_slot_count_106d20_partial(block_u32);
    auto* block_info = memory_base + 104u;
    if (!block_info_construct_13d750_partial(block_info, block_info_count))
        return false;

    const std::uint32_t parse_pool_count =
        memory_parse_result_pool_count_106d20_partial(block_u32, *reinterpret_cast<const std::uint32_t*>(cfg + 28u));
    const std::uint64_t parse_pool_storage =
        codec_v3_alloc_zero_52acb0(parse_result_pool_required_additional_memory_107190_partial(parse_pool_count), 8u);
    if (parse_pool_count != 0u && parse_pool_storage == 0u)
        return false;
    if (!parse_result_pool_construct_1071b0_partial(
            reinterpret_cast<std::uint64_t>(memory_base + 136u),
            parse_pool_storage,
            parse_pool_count)) {
        return false;
    }

    const std::uint64_t word_count = block_samples & 0x7FFFFFFFULL;
    const std::uint64_t channel_words =
        codec_v3_alloc_zero_52acb0(8ull * word_count, 4u);
    *reinterpret_cast<std::uint64_t*>(memory_base + 152u) = channel_words;
    if (word_count != 0u && channel_words == 0u)
        return false;

    const std::uint64_t tail_words =
        codec_v3_alloc_zero_52acb0((12ull * block_samples) & 0x3FFFFFFFCLL, 4u);
    *reinterpret_cast<std::uint64_t*>(memory_base + 160u) = tail_words;
    if ((block_samples != 0u) && tail_words == 0u)
        return false;

    const std::uint32_t stage1_count = *reinterpret_cast<const std::uint32_t*>(cfg + 28u);
    const std::uint32_t ready_cap = ((block_u32 * stage1_count + 255u) >> 8u) + 1u;
    const std::uint64_t ready_parse_bytes =
        static_cast<std::uint64_t>(ready_cap) * 9ull * static_cast<std::uint64_t>(kParseResultStrideBytes);
    const std::uint64_t ready_parse_storage =
        codec_v3_alloc_zero_52acb0(ready_parse_bytes, 8u);
    *reinterpret_cast<std::uint64_t*>(memory_base + kMemory_off_ready_parse_storage) = ready_parse_storage;
    if (ready_parse_bytes != 0u && ready_parse_storage == 0u)
        return false;
    return true;
}

std::int64_t codec_v3_parser_frame_deque_push_back_530290(
    std::uint64_t frame_deque_ptr,
    std::uint64_t frame_ptr) {
    return frame_deque_push_back_13d5d0_partial(
        frame_deque_ptr, frame_ptr, kCodecV3FrameDequeSlotCopyBytes);
}

std::uint64_t codec_v3_parser_frame_deque_pop_front_530380(std::uint64_t frame_deque_ptr) {
    return frame_deque_pop_front_13d670_partial(frame_deque_ptr, frame_mark_as_unused_106cd0_default_partial);
}

CodecV3ParserRuntimeFns1034e0 codec_v3_default_parser_runtime_1034e0() {
    CodecV3ParserRuntimeFns1034e0 fns{};
    fns.delay_line_get_buffer = delay_line_get_buffer_u64_state_partial;
    fns.frame_deque_find_first_with_end_after = frame_deque_find_first_with_end_after_13d570_partial;
    fns.frame_deque_push_back = codec_v3_parser_frame_deque_push_back_530290;
    fns.frame_deque_pop_front = codec_v3_parser_frame_deque_pop_front_530380;
    fns.frame_mark_as_unused = parser_frame_mark_as_unused_cb_partial;
    return fns;
}

std::int64_t codec_v3_parser_process_integrated_impl_1034e0(std::uint8_t* parser_base) {
    if (!parser_base)
        return 0;
    const auto* parser_q = reinterpret_cast<const std::uint64_t*>(parser_base);
    const std::uint64_t memory_base = parser_q[2];
    const std::uint64_t ready_storage =
        memory_base != 0u
            ? *reinterpret_cast<const std::uint64_t*>(memory_base + kMemory_off_ready_parse_storage)
            : 0u;

    bool copy_active = false;
    if (ready_storage != 0u) {
        const auto* ready_dq = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(parser_q[5]));
        const std::uint32_t ready_cap =
            ready_dq ? *reinterpret_cast<const std::uint32_t*>(ready_dq + 24u) : 0u;
        g_parser_ready_frame_copy_ctx.ready_parse_result_base =
            reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(ready_storage));
        g_parser_ready_frame_copy_ctx.ready_parse_result_size =
            static_cast<std::size_t>(ready_cap) * 9u * kParseResultStrideBytes;
        g_parser_ready_frame_copy_ctx.copied_slot_capacity = kCodecV3FrameDequeCopiedSlotCapacity;
        g_parser_ready_frame_copy_ctx.parse_result_bytes = kParseResultStrideBytes;
        g_parser_ready_frame_copy_active = true;
        copy_active = true;
    }

    CodecV3ParserRuntimeFns1034e0 fns = codec_v3_default_parser_runtime_1034e0();
    if (copy_active)
        fns.frame_deque_push_back = parser_frame_deque_push_back_with_optional_copy_530290;

    const std::int64_t rc = codec_v3_parser_process_1034e0(parser_base, &fns);
    if (copy_active)
        g_parser_ready_frame_copy_active = false;
    return rc;
}

std::int64_t codec_v3_parser_process_default_1034e0(std::uint8_t* parser_base) {
    const CodecV3ParserRuntimeFns1034e0 fns = codec_v3_default_parser_runtime_1034e0();
    return codec_v3_parser_process_1034e0(parser_base, &fns);
}

void codec_v3_output_frame_deque_pop_front_530380(std::uint64_t frame_deque_ptr) {
    (void)frame_deque_pop_front_13d670_partial(frame_deque_ptr, frame_mark_as_unused_106cd0_default_partial);
}

std::uint64_t codec_v3_output_delay_line_get_buffer_530530(
    std::uint64_t delay_line_ptr,
    std::uint64_t timeline_cursor,
    std::int64_t* io_state) {
    return delay_line_get_buffer_106b40(
        reinterpret_cast<DelayLineState106b40*>(static_cast<std::uintptr_t>(delay_line_ptr)),
        timeline_cursor,
        io_state);
}

OutputGeneratorRuntimeFns1024a9 codec_v3_default_output_runtime_1024a9() {
    OutputGeneratorRuntimeFns1024a9 fns{};
    fns.delay_line_get_channel = delay_line_get_channel_from_buffer_106ab0;
    fns.delay_line_get_buffer = codec_v3_output_delay_line_get_buffer_530530;
    fns.frame_deque_find_first_with_end_after = frame_deque_find_first_with_end_after_13d570_partial;
    fns.frame_mark_as_unused = frame_mark_as_unused_106cd0_default_partial;
    fns.frame_deque_pop_front = codec_v3_output_frame_deque_pop_front_530380;
    fns.golombrice_get_errors = golombrice_get_errors_104e40;
    fns.extrapolate_process = extrapolate_process_104460_partial;
    fns.golombrice_initialize = golombrice_initialize_104e10;
    fns.extrapolate_initialize = extrapolate_initialize_104440;
    return fns;
}

} // namespace

void format_detector_integrated_set_layout(void* user, std::uint32_t layout_mask) {
    sync_detector_set_layout_105ee0_partial(
        reinterpret_cast<SyncDetectorState105ee0*>(user),
        layout_mask);
}

void format_detector_integrated_process_block(void* user, const std::uint64_t* channel_ptrs_27) {
    sync_detector_process_block_106110_partial(
        reinterpret_cast<SyncDetectorState105ee0*>(user),
        channel_ptrs_27);
}

void parser_t_construct_52ed50_partial(
    std::uint8_t* parser_base,
    const std::uint8_t* decoder_base,
    std::uint8_t* memory_base) {
    if (!parser_base || !decoder_base || !memory_base)
        return;
    std::memset(parser_base, 0, 656u);
    auto* parser_q = reinterpret_cast<std::uint64_t*>(parser_base);
    const auto* delay_line = reinterpret_cast<const DelayLineState106b40*>(memory_base);
    const std::uint32_t stage0_count = *reinterpret_cast<const std::uint32_t*>(
        decoder_base + auro_codec_v3_ida::kDecoderConfig_off_stage0_count);
    *reinterpret_cast<std::uint32_t*>(parser_base) = stage0_count;
    parser_q[1] = delay_line_stream_index_5306c0_partial(delay_line, stage0_count);
    parser_q[2] = reinterpret_cast<std::uint64_t>(memory_base);
    parser_q[3] = *reinterpret_cast<const std::uint64_t*>(decoder_base);
    parser_q[4] = reinterpret_cast<std::uint64_t>(memory_base + 40u);
    parser_q[5] = reinterpret_cast<std::uint64_t>(memory_base + 72u);
    parser_q[6] = reinterpret_cast<std::uint64_t>(memory_base + 136u);
}

std::int64_t codec_v3_parser_process_integrated_1034e0(std::uint8_t* parser_base) {
    return codec_v3_parser_process_integrated_impl_1034e0(parser_base);
}

std::uint32_t auro_codec_v3_Decoder_t_construct(
    std::int64_t decoder_base,
    std::int64_t /*dist*/,
    std::int64_t init_args) {
    return decoder_t_construct_101760(
        reinterpret_cast<std::uint8_t*>(decoder_base),
        reinterpret_cast<const void*>(init_args),
        reinterpret_cast<void*>(decoder_base));
}

void auro_codec_v3_Decoder_set_sync_callback(std::uint64_t* decoder_base, void* fn, void* user) {
    if (!decoder_base)
        return;
    auto* d = reinterpret_cast<std::uint8_t*>(decoder_base);
    auto* region = reinterpret_cast<FormatDetectorRegion1056c0*>(
        d + auro_codec_v3_ida::kDecoder_off_FormatDetector);
    region->tail.sink_notify = reinterpret_cast<void (*)(void*, std::int64_t)>(fn);
    region->tail.sink_user = user;
}

void auro_codec_v3_Decoder_set_content_callback(std::uint64_t decoder_base, void* fn, void* user) {
    if (decoder_base == 0)
        return;
    // IDA 0x101AC0 -> Parser_set_content_callback(a1+568, fn, user)
    auto* d = reinterpret_cast<std::uint8_t*>(decoder_base);
    *reinterpret_cast<std::uint64_t*>(d + 1200u) = reinterpret_cast<std::uint64_t>(fn);   // (a1+568)+632
    *reinterpret_cast<std::uint64_t*>(d + 1208u) = reinterpret_cast<std::uint64_t>(user); // (a1+568)+640
}

void auro_codec_v3_Decoder_set_decide_decode_callback(std::uint64_t decoder_base, void* fn, void* user) {
    if (decoder_base == 0)
        return;
    auto* d = reinterpret_cast<std::uint8_t*>(decoder_base);
    *reinterpret_cast<std::uint64_t*>(
        d + auro_codec_v3_ida::kDecoder_off_OutputGenerator + kOgOff_pre_segments_cb) =
        reinterpret_cast<std::uint64_t>(fn);
    *reinterpret_cast<std::uint64_t*>(
        d + auro_codec_v3_ida::kDecoder_off_OutputGenerator + kOgOff_pre_segments_ctx) =
        reinterpret_cast<std::uint64_t>(user);
}

void auro_codec_v3_Decoder_set_metadata_callback(std::uint64_t decoder_base, void* fn, void* user) {
    if (decoder_base == 0)
        return;
    auto* d = reinterpret_cast<std::uint8_t*>(decoder_base);
    *reinterpret_cast<std::uint64_t*>(
        d + auro_codec_v3_ida::kDecoder_off_OutputGenerator + kOgOff_metadata_cb) =
        reinterpret_cast<std::uint64_t>(fn);
    *reinterpret_cast<std::uint64_t*>(
        d + auro_codec_v3_ida::kDecoder_off_OutputGenerator + kOgOff_metadata_ctx) =
        reinterpret_cast<std::uint64_t>(user);
}

std::uint32_t decoder_t_construct_101760(std::uint8_t* decoder_base, const void* config_init_args, void* user_ctx) {
    // Literal перенос auro_codec_v3_Decoder_t_construct @ 0x101760.
    // a1=decoder_base, a2=Memory args (user_ctx), a3=config_init_args в оригинале; здесь user_ctx передаём дальше.
    if (!decoder_base || !config_init_args)
        return 0;

    decoder_crc_t_init_106f00();
    decoder_channel_extrapolate_t_init_1042a1();

    // Config_initialize(a1, a3)
    auto* cfg = decoder_base;
    const auto* init = static_cast<const std::uint8_t*>(config_init_args);
    const auto rc_cfg = auro_codec_v3_decoder_Config_initialize(
        reinterpret_cast<std::int64_t>(cfg),
        reinterpret_cast<std::int64_t>(init));
    if (rc_cfg != 0)
        return 0;

    if (!codec_v3_memory_construct_local_5300d0(
            decoder_base + auro_codec_v3_ida::kDecoder_off_Memory,
            decoder_base)) {
        return 0;
    }

    auto* fmt_region = reinterpret_cast<FormatDetectorRegion1056c0*>(
        decoder_base + auro_codec_v3_ida::kDecoder_off_FormatDetector);
    format_detector_t_construct_1056c0_partial(
        fmt_region,
        decoder_base,
        decoder_base + auro_codec_v3_ida::kDecoder_off_Memory);

    parser_t_construct_52ed50_partial(
        decoder_base + auro_codec_v3_ida::kDecoder_off_Parser,
        decoder_base,
        decoder_base + auro_codec_v3_ida::kDecoder_off_Memory);

    output_generator_construct_52af20_partial(
        decoder_base + auro_codec_v3_ida::kDecoder_off_OutputGenerator,
        decoder_base,
        reinterpret_cast<std::uint64_t>(decoder_base + auro_codec_v3_ida::kDecoder_off_Memory));

    const std::uint64_t qword0 = *reinterpret_cast<const std::uint64_t*>(decoder_base);
    const std::uint32_t dword28 =
        *reinterpret_cast<const std::uint32_t*>(decoder_base + auro_codec_v3_ida::kDecoder_off_dword_28);
    *reinterpret_cast<std::uint64_t*>(decoder_base + auro_codec_v3_ida::kDecoder_off_qword_2472) =
        static_cast<std::uint64_t>(static_cast<std::uint32_t>(qword0)) * static_cast<std::uint64_t>(dword28);

    (void)user_ctx;
    return 1;
}


std::int64_t sub_ed070(std::uint8_t* processor_base) {
    // Literal 0xED070: return (*(a1+16))(*(a1+8));
    using Inner = std::int64_t (*)(void*);
    auto* inner_arg = *reinterpret_cast<void**>(processor_base + kProcessor_Analyser_ctx_ptr);
    auto* fn = reinterpret_cast<Inner>(*reinterpret_cast<void**>(processor_base + kProcessor_Analyser_inner_fn));
    return fn(inner_arg);
}

std::int64_t sub_da320(std::uint8_t* processor_base, std::uint8_t* buffer_desc_a2, std::uint8_t* buffer_desc_a3) {
    if (!processor_base)
        return 2;
    // Literal 0xDA320: только null-check и tail-call в Controller_process_audio(a1+0x140, a2, a3).
    std::uint8_t* ctrl = processor_base + kProcessor_to_Controller_audio;
    return controller_process_audio_da870(ctrl, buffer_desc_a2, buffer_desc_a3, sizeof(ProcessorIOBufferDesc));
}

std::int64_t analyser_process(std::uint8_t* processor_base, const std::uint32_t* sample_increment) {
    // Literal 0xED500: result = (*(a1+256))(a1); *(_DWORD*)a1 += *a2; return result;
    using Thunk = std::int64_t (*)(std::uint8_t*);
    auto* fn = reinterpret_cast<Thunk>(*reinterpret_cast<void**>(processor_base + kProcessor_Analyser_thunk_fn));
    const std::int64_t result = fn(processor_base);
    *reinterpret_cast<std::uint32_t*>(processor_base) += *sample_increment;
    return result;
}

std::int64_t controller_process_audio_zero_fill(
    std::uint32_t channel_mask,
    std::uint32_t bytes_per_unit,
    std::uint32_t error_flag,
    const void* output_desc_raw,
    std::size_t output_desc_size) {
    (void)output_desc_size;
    const auto* out = reinterpret_cast<const ProcessorIOBufferDesc*>(output_desc_raw);

    if (out->layout_or_kind > 1u)
        return 129;

    const std::uint32_t unit = 32u * bytes_per_unit;
    if (unit == 0)
        return 0;

    const std::size_t n = 4u * static_cast<std::size_t>(unit);
    for (unsigned bit = 0; bit < 27; ++bit) {
        if (((channel_mask >> bit) & 1u) == 0)
            continue;
        const std::uint64_t p = (bit < 27) ? out->channel_ptr[bit] : 0;
        if (p == 0)
            continue;
        std::memset(reinterpret_cast<void*>(static_cast<std::uintptr_t>(p)), 0, n);
    }
    return 0;
}

std::int64_t controller_process_audio_da870(
    std::uint8_t* controller_base,
    const void* input_desc_raw,
    const void* output_desc_raw,
    std::size_t output_desc_size) {
    (void)output_desc_size;
    const auto error_flag = *reinterpret_cast<const std::int32_t*>(controller_base + kController_error_flag);
    if (error_flag == 0) {
        using Fn = std::int64_t (*)(std::uint8_t*, const void*, const void*);
        auto* fn = reinterpret_cast<Fn>(*reinterpret_cast<void**>(controller_base + kController_process_fn));
        const std::int64_t rc = fn(controller_base, input_desc_raw, output_desc_raw);
        return (rc == 0) ? 0 : rc;
    }

    const std::uint32_t mask = *reinterpret_cast<const std::uint32_t*>(controller_base + kController_mask_bits);
    const std::uint32_t unit = *reinterpret_cast<const std::uint32_t*>(controller_base + kController_bytes_unit);
    return controller_process_audio_zero_fill(mask, unit, 1u, output_desc_raw, output_desc_size);
}

std::int32_t processor_process_validate_da9ae0(
    const ProcessorIOBufferDesc* in_desc,
    const ProcessorIOBufferDesc* out_desc,
    const ProcessorIoExpectDa9ae0& ex) {
    if (!in_desc || !out_desc)
        return kErrNullIoDesc;

    if (static_cast<std::int32_t>(in_desc->layout_or_kind) != ex.in_layout)
        return kErrInLayoutMismatch;
    if (!(ex.in_layout == 0 || (ex.in_layout == 1 && static_cast<std::int32_t>(in_desc->field_8) == ex.in_field8_when_layout1)))
        return kErrInField8Mismatch;
    const std::uint32_t in_unit = static_cast<std::uint32_t>(32 * ex.in_bytes_unit);
    if (in_unit != 0 && (in_desc->total_size_bytes % in_unit) != 0)
        return kErrInSizeModulo;
    if (static_cast<std::int32_t>(in_desc->field_4) != ex.in_field4)
        return kErrInField4Mismatch;
    if (!has_required_channel_ptrs(in_desc, ex.in_mask))
        return kErrInChannelPtrMissing;

    if (static_cast<std::int32_t>(out_desc->layout_or_kind) != ex.out_layout)
        return kErrOutLayoutMismatch;
    if (!(ex.out_layout == 0 || (ex.out_layout == 1 && static_cast<std::int32_t>(out_desc->field_8) == ex.out_field8_when_layout1)))
        return kErrOutField8Mismatch;
    const std::uint32_t out_unit = static_cast<std::uint32_t>(32 * ex.out_bytes_unit);
    if (out_unit != 0 && (out_desc->total_size_bytes % out_unit) != 0)
        return kErrOutSizeModulo;
    if (static_cast<std::int32_t>(out_desc->field_4) != ex.out_field4)
        return kErrOutField4Mismatch;
    if (!has_required_channel_ptrs(out_desc, ex.out_mask))
        return kErrOutChannelPtrMissing;
    return 0;
}

std::int64_t processor_process_da9ae0_minimal(
    std::uint8_t* processor_base,
    const ProcessorIOBufferDesc* in_desc,
    const ProcessorIOBufferDesc* out_desc) {
    const ProcessorIoExpectDa9ae0 ex = load_expect_da9ae0_from_processor(processor_base);
    const std::int32_t vrc = processor_process_validate_da9ae0(in_desc, out_desc, ex);
    if (vrc != 0)
        return vrc;

    // 0xD9AE0 использует ratio (*a2 / a2->field_4) в timing-ветке; здесь переносим только инкремент счётчика.
    std::uint32_t sample_increment = 0;
    if (in_desc->field_4 != 0)
        sample_increment = in_desc->total_size_bytes / in_desc->field_4;
    return analyser_process(processor_base, &sample_increment);
}

std::int64_t processor_process_da9ae0_minimal_with_elapsed(
    std::uint8_t* processor_base,
    const ProcessorIOBufferDesc* in_desc,
    const ProcessorIOBufferDesc* out_desc,
    std::int64_t elapsed_ticks) {
    const ProcessorIoExpectDa9ae0 ex = load_expect_da9ae0_from_processor(processor_base);
    const std::int32_t vrc = processor_process_validate_da9ae0(in_desc, out_desc, ex);
    if (vrc != 0)
        return vrc;

    std::uint32_t sample_increment = 0;
    if (in_desc->field_4 != 0)
        sample_increment = in_desc->total_size_bytes / in_desc->field_4;
    const std::int64_t rc = analyser_process(processor_base, &sample_increment);
    processor_update_timing_stats_da9ae0(processor_base, in_desc, elapsed_ticks);
    return rc;
}

void processor_update_timing_stats_da9ae0(
    std::uint8_t* processor_base,
    const ProcessorIOBufferDesc* in_desc,
    std::int64_t elapsed_ticks) {
    if (!processor_base || !in_desc)
        return;
    if (in_desc->field_4 == 0)
        return;

    // IDA 0xD9AE0: v15 = (float)(*a2) / (float)(a2->field_4)
    const float input_ratio = static_cast<float>(in_desc->total_size_bytes) / static_cast<float>(in_desc->field_4);
    *reinterpret_cast<float*>(processor_base + kProcessorPerf_ratio_sum_float) += input_ratio;

    // IDA 0xD9AE0: *qword_25E958 += elapsed
    *reinterpret_cast<std::int64_t*>(processor_base + kProcessorPerf_elapsed_sum_qword) += elapsed_ticks;

    // IDA 0xD9AE0: if (elapsed < 0) special conversion via shifted halves.
    float elapsed_f = 0.0f;
    if (elapsed_ticks < 0) {
        const std::int64_t half = static_cast<std::int64_t>((static_cast<std::uint64_t>(elapsed_ticks) >> 1) | (elapsed_ticks & 1));
        elapsed_f = static_cast<float>(static_cast<int>(half)) + static_cast<float>(static_cast<int>(half));
    } else {
        elapsed_f = static_cast<float>(static_cast<int>(elapsed_ticks));
    }
    const float cur_ratio = elapsed_f / input_ratio;

    float& best_ratio = *reinterpret_cast<float*>(processor_base + kProcessorPerf_best_ratio_float);
    if (best_ratio > cur_ratio) {
        best_ratio = cur_ratio;
        *reinterpret_cast<std::int64_t*>(processor_base + kProcessorPerf_best_elapsed_qword) = elapsed_ticks;
        *reinterpret_cast<float*>(processor_base + kProcessorPerf_best_ratio_input_float) = input_ratio;
    }
}

bool auro_decoder_impl_initialize_partial(
    std::uint8_t* impl_base,
    const AuroDecoderImplInitParams* params) {
    if (!impl_base || !params || params->block_size_samples == 0u)
        return false;

    using auro_codec_v3_ida::kAuroDecoderImpl_off_InputDesc;
    using auro_codec_v3_ida::kAuroDecoderImpl_off_OutputDesc;
    using auro_codec_v3_ida::kAuroDecoderImpl_off_ChannelCount;
    using auro_codec_v3_ida::kAuroDecoderImpl_off_BlockSize;
    using auro_codec_v3_ida::kAuroDecoderImpl_off_RuntimeInputMask;
    using auro_codec_v3_ida::kAuroDecoderImpl_off_RuntimeOutputMask;

    auto* in = reinterpret_cast<ProcessorIOBufferDesc*>(impl_base + kAuroDecoderImpl_off_InputDesc);
    auto* out = reinterpret_cast<ProcessorIOBufferDesc*>(impl_base + kAuroDecoderImpl_off_OutputDesc);
    std::memset(in, 0, sizeof(*in));
    std::memset(out, 0, sizeof(*out));

    // IDA AuroDecoderImpl IO: total_size_bytes = sample count (not byte length).
    // Processor_process validate uses 32 * bytes_unit (expect unit=1) as modulo.
    in->total_size_bytes = params->block_size_samples;
    in->field_4 = static_cast<std::int32_t>(params->sample_rate);
    in->field_8 = 24;
    in->layout_or_kind = 1;
    out->total_size_bytes = params->block_size_samples;
    out->field_4 = static_cast<std::int32_t>(params->sample_rate);
    out->field_8 = 24;
    out->layout_or_kind = 0;

    for (std::size_t ch = 0; ch < 27u; ++ch) {
        in->channel_ptr[ch] = params->input_channel_ptrs[ch];
        out->channel_ptr[ch] = params->output_channel_ptrs[ch];
    }

    *reinterpret_cast<std::uint32_t*>(impl_base + kAuroDecoderImpl_off_RuntimeInputMask) =
        params->input_mask & kCodecV3ChannelMask;
    *reinterpret_cast<std::uint32_t*>(impl_base + kAuroDecoderImpl_off_RuntimeOutputMask) =
        params->output_mask & kCodecV3ChannelMask;
    *reinterpret_cast<std::uint32_t*>(impl_base + kAuroDecoderImpl_off_ChannelCount) =
        auro_channel_Mask_count(params->input_mask & kCodecV3ChannelMask, 0, 0);
    *reinterpret_cast<std::uint32_t*>(impl_base + kAuroDecoderImpl_off_BlockSize) =
        params->block_size_samples;
    return true;
}

std::int32_t auro_decoder_impl_decode_partial(
    std::uint8_t* impl_base,
    std::int64_t (*processor_process)(
        std::uint8_t* processor_base,
        const ProcessorIOBufferDesc* in_desc,
        const ProcessorIOBufferDesc* out_desc)) {
    if (!impl_base)
        return -1;

    using auro_codec_v3_ida::kAuroDecoderImpl_off_InputDesc;
    using auro_codec_v3_ida::kAuroDecoderImpl_off_OutputDesc;
    using auro_codec_v3_ida::kAuroDecoderImpl_off_ProcessorInstance;

    auto* in = reinterpret_cast<const ProcessorIOBufferDesc*>(impl_base + kAuroDecoderImpl_off_InputDesc);
    auto* out = reinterpret_cast<const ProcessorIOBufferDesc*>(impl_base + kAuroDecoderImpl_off_OutputDesc);
    auto* processor_base = impl_base + kAuroDecoderImpl_off_ProcessorInstance;

    const ProcessorIoExpectDa9ae0 ex = load_expect_da9ae0_from_impl_partial(impl_base);
    const std::int32_t vrc = processor_process_validate_da9ae0(in, out, ex);
    if (vrc != 0)
        return vrc;

    const std::int64_t rc = processor_process
        ? processor_process(processor_base, in, out)
        : processor_process_da9ae0_minimal(processor_base, in, out);
    return static_cast<std::int32_t>(rc);
}

void codec_v3_sync_callback_eb840(CodecV3DispatchStateEb5a0* state, int value) {
    if (!state || state->sync_state == static_cast<std::uint32_t>(value))
        return;
    state->sync_state = static_cast<std::uint32_t>(value);
    notify_codec_v3_state_change(state, 0);
}

void codec_v3_content_callback_eb870(CodecV3DispatchStateEb5a0* state, int value) {
    if (!state || state->content_state == static_cast<std::uint32_t>(value))
        return;
    state->content_state = static_cast<std::uint32_t>(value);
    notify_codec_v3_state_change(state, 1);
}

namespace {

constexpr std::uintptr_t kV3DecoderWrapperOffAllowDecoding = 2508u;
constexpr std::uintptr_t kEb5a0BlobOffDecideDecodeGate = 252124u;

std::uint64_t codec_v3_codec_decoder_from_ctx(std::uint64_t ctx) {
    if (ctx == 0u)
        return 0u;
    const auto* blob = reinterpret_cast<const std::uint8_t*>(ctx);
    const std::uint64_t codec = *reinterpret_cast<const std::uint64_t*>(blob);
    if (codec == 0u)
        return 0u;
    return codec;
}

std::uint32_t codec_v3_block_info_segment_count_from_ctx(std::uint64_t ctx) {
    const std::uint64_t codec = codec_v3_codec_decoder_from_ctx(ctx);
    if (codec == 0u)
        return 0u;
    const auto* og = reinterpret_cast<const std::uint8_t*>(
        static_cast<std::uintptr_t>(codec + auro_codec_v3_ida::kDecoder_off_OutputGenerator));
    const std::uint64_t block_info = *reinterpret_cast<const std::uint64_t*>(og + kOgOff_segment_ctx_ptr);
    if (block_info == 0u)
        return 0u;
    return *reinterpret_cast<const std::uint32_t*>(block_info + 24u);
}

std::uint32_t codec_v3_allow_decoding_from_decoder(const std::uint8_t* decoder_base) {
    (void)decoder_base;
    return 1u;
}

std::uint32_t codec_v3_allow_decoding_from_ctx(std::uint64_t ctx) {
    if (ctx == 0u)
        return 1u;
    const auto* blob = reinterpret_cast<const std::uint8_t*>(ctx);
    const std::uint32_t wrapper_gate =
        *reinterpret_cast<const std::uint32_t*>(blob + kV3DecoderWrapperOffAllowDecoding);
    if (wrapper_gate <= 1u)
        return wrapper_gate;
    return *reinterpret_cast<const std::uint32_t*>(blob + kEb5a0BlobOffDecideDecodeGate);
}

struct DownmixToPartial {
    std::uint32_t mask = 0;
    bool ok = false;
};

constexpr std::size_t kDownmixPlanIntCount = 0x280u / sizeof(std::int32_t);

namespace {

struct DownmixPlanMut {
    std::int32_t* plan = nullptr;
    std::uint64_t* row_count = nullptr;
    std::uint32_t* out_mask = nullptr;

    explicit DownmixPlanMut(std::int32_t* p)
        : plan(p)
        , row_count(reinterpret_cast<std::uint64_t*>(p))
        , out_mask(reinterpret_cast<std::uint32_t*>(p + 159)) {}

    bool push_qd(std::uint64_t qword, std::int32_t dword) {
        const std::uint64_t idx = *row_count;
        if (idx > 0x33u)
            return false;
        *row_count = idx + 1u;
        const std::size_t off = 2u + 3u * static_cast<std::size_t>(idx);
        *reinterpret_cast<std::uint64_t*>(plan + off) = qword;
        plan[off + 2] = dword;
        return true;
    }

    bool push_dq(std::int32_t dword0, std::uint64_t qword1) {
        const std::uint64_t idx = *row_count;
        if (idx > 0x33u)
            return false;
        *row_count = idx + 1u;
        auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 8u + 12u * idx;
        *reinterpret_cast<std::int32_t*>(row_base) = dword0;
        *reinterpret_cast<std::uint64_t*>(row_base + 4u) = qword1;
        return true;
    }

    void patch_mask(std::uint32_t or_bits, std::uint32_t and_mask) {
        *out_mask = or_bits | (*out_mask & and_mask);
    }

    bool push_dq_after_two(
        std::uint64_t first_row_idx,
        std::int32_t dword0,
        std::uint64_t qword1,
        std::uint32_t& src_mask) {
        *row_count = first_row_idx + 2u;
        auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 4u * (3u * first_row_idx + 5u);
        *reinterpret_cast<std::int32_t*>(row_base) = dword0;
        *reinterpret_cast<std::uint64_t*>(row_base + 4u) = qword1;
        (void)src_mask;
        return true;
    }
};

constexpr std::uint64_t kDmPair15873 = 0x0000003E00000001uLL;
constexpr std::uint64_t kDmPair14614 = 0x0000003900000016uLL;
constexpr std::uint64_t kDmPair14085 = 0x0000003700000005uLL;
constexpr std::uint64_t kDmPair15382 = 0x0000003C00000016uLL;
constexpr std::uint64_t kDmPair3329 = 0x0000000D00000001uLL;
constexpr std::uint64_t kDmPair2821 = 0x0000000B00000005uLL;
constexpr std::uint64_t kDmPair268 = 0x000000010000000CuLL;
constexpr std::uint64_t kDmPair1802 = 0x000000070000000AuLL;
constexpr std::uint64_t kDmPair1294 = 0x000000050000000EuLL;
constexpr std::uint64_t kDmPair11784 = 0x0000002E00000008uLL;
constexpr std::uint64_t kDmPair11269 = 0x0000002C00000005uLL;
constexpr std::uint64_t kDmPair13569 = 0x0000003500000001uLL;
constexpr std::uint64_t kDmPair18950 = 0x0000004A00000056uLL;
constexpr std::uint64_t kDmPair22790 = 0x0000005900000056uLL;
constexpr std::uint64_t kDmPair17925 = 0x0000004600000005uLL;
constexpr std::uint64_t kDmPair6146 = 0x0000001800000002uLL;
constexpr std::uint64_t kDmPair6917 = 0x0000001B00000005uLL;
constexpr std::uint64_t kDmPair7941 = 0x0000001F00000005uLL;

bool downmix_label101_tgt_layout_rejected(std::uint32_t tgt_layout) {
    // IDA @ 590894: masked tgt equals xmmword_1DC660 in active lanes.
    if ((tgt_layout & 0x8000u) != 0u)
        return false;
    if ((tgt_layout & 0x300000u) != 0x3000000u)
        return false;
    if ((tgt_layout & 0x600000u) != 0x600000u)
        return false;
    return (tgt_layout & 0x6u) == 0u;
}

bool auro_downmix_v1_plan_apply_label274(
    DownmixPlanMut& dp,
    std::uint64_t anchor_row,
    std::int32_t dword0,
    std::uint64_t pair_qword,
    std::uint32_t or_bits,
    std::uint32_t and_mask,
    std::uint32_t& src_mask) {
    auto* row_base = reinterpret_cast<std::uint8_t*>(dp.plan) + 4u * (3u * anchor_row + 5u);
    *reinterpret_cast<std::int32_t*>(row_base) = dword0;
    *reinterpret_cast<std::uint64_t*>(row_base + 4u) = pair_qword;
    src_mask = or_bits | (src_mask & and_mask);
    dp.patch_mask(src_mask, 0xFFFFFFFFu);
    return true;
}

bool auro_downmix_v1_plan_apply_label100_write(
    DownmixPlanMut& dp,
    std::uint64_t anchor_row,
    std::int32_t dword0,
    std::uint64_t pair_qword,
    std::uint32_t or_bits,
    std::uint32_t and_mask,
    std::uint32_t& src_mask) {
    auto* row_base = reinterpret_cast<std::uint8_t*>(dp.plan) + 4u * (3u * anchor_row + 2u);
    *reinterpret_cast<std::int32_t*>(row_base) = dword0;
    *reinterpret_cast<std::uint64_t*>(row_base + 4u) = pair_qword;
    src_mask = or_bits | (src_mask & and_mask);
    dp.patch_mask(src_mask, 0xFFFFFFFFu);
    return true;
}

bool auro_downmix_v1_plan_apply_label100(
    DownmixPlanMut& dp,
    std::uint64_t anchor_row,
    std::int32_t dword0,
    std::uint64_t pair_qword,
    std::uint32_t or_bits,
    std::uint32_t and_mask,
    std::uint32_t& src_mask) {
    *dp.row_count = anchor_row + 1u;
    return auro_downmix_v1_plan_apply_label100_write(
        dp, anchor_row, dword0, pair_qword, or_bits, and_mask, src_mask);
}

constexpr std::uint64_t kDmPair8975 = 0x000000230000000FuLL;
constexpr std::uint64_t kDmPair9486 = 0x000000250000000EuLL;
constexpr std::uint64_t kDmPair12298 = 0x000000300000000AuLL;
constexpr std::uint64_t kDmPair12810 = 0x000000320000000AuLL;
constexpr std::uint64_t kDmPair3869 = 0x0000000F0000001DuLL;
constexpr std::uint64_t kDmPair4107 = 0x000000100000000BuLL;
constexpr std::uint64_t kDmPair4878 = 0x000000130000000EuLL;
constexpr std::uint64_t kDmPair5902 = 0x0000001700000006uLL;
constexpr std::uint64_t kDmPair21249 = 0x0000005300000001uLL;
constexpr std::uint64_t kDmPair22282 = 0x000000570000000AuLL;
constexpr std::uint64_t kDmPair20481 = 0x0000005000000001uLL;
constexpr std::uint64_t kDmPair20738 = 0x0000005100000002uLL;
constexpr std::uint64_t kDmPair20766 = 0x000000510000001EuLL;

bool auro_downmix_v1_plan_finalize_label275_partial(
    std::int32_t* plan,
    std::uint32_t tgt_layout,
    std::uint8_t tgt_class_hint);

bool auro_downmix_v1_plan_finish_label275_partial(
    std::int32_t* plan,
    std::uint32_t tgt_layout,
    std::uint8_t tgt_class_hint,
    bool* engine_complete) {
    if (!auro_downmix_v1_plan_finalize_label275_partial(plan, tgt_layout, tgt_class_hint))
        return false;
    if (engine_complete)
        *engine_complete = true;
    return true;
}

bool auro_downmix_v1_plan_label257_partial(
    DownmixPlanMut& dp,
    std::uint32_t& src_mask,
    std::uint32_t tgt_layout,
    std::uint8_t tgt_class_hint,
    bool* engine_complete) {
  // IDA LABEL_257 @ 591280: shared tail before LABEL_274/LABEL_275.
  if ((~src_mask & 0x30u) != 0u)
    return auro_downmix_v1_plan_finish_label275_partial(
        dp.plan, tgt_layout, tgt_class_hint, engine_complete);

  const std::uint64_t r0 = *dp.row_count;
  if (r0 > 0x33u)
    return false;
  if (!dp.push_qd(4u, 84))
    return false;
  dp.patch_mask(1u, 0xFFFFFFEEu);
  if (r0 == 51u)
    return false;
  if (!dp.push_qd(0x100000005uLL, 85))
    return false;
  dp.patch_mask(2u, 0xFFFFFFDDu);
  if (r0 > 0x31u)
    return false;
  if (!dp.push_qd(0x900000004uLL, 86))
    return false;
  dp.patch_mask(0x200u, 0xFFFFFDEFu);
  if (r0 == 49u)
    return false;
  *dp.row_count = r0 + 4u;
  if (!auro_downmix_v1_plan_apply_label274(
          dp, r0 + 3u, 5, kDmPair22282, 1024u, 0xFFFFFBFFu, src_mask))
    return false;
  return auro_downmix_v1_plan_finish_label275_partial(
      dp.plan, tgt_layout, tgt_class_hint, engine_complete);
}

bool auro_downmix_v1_plan_label209_partial(
    DownmixPlanMut& dp,
    std::uint32_t& src_mask,
    std::uint32_t tgt_layout,
    std::uint32_t inv_tgt,
    std::uint8_t tgt_class_hint,
    bool* engine_complete) {
    if (engine_complete)
        *engine_complete = false;
    const auto finish = [&]() {
        return auro_downmix_v1_plan_finish_label275_partial(
            dp.plan, tgt_layout, tgt_class_hint, engine_complete);
    };
    const std::uint32_t v11 = inv_tgt;
    const std::uint32_t tgt_lo = tgt_layout & 3u;
    const std::uint32_t v147 = tgt_layout & 0x30u;
    const std::uint32_t v73 = tgt_layout & 0x600u;

    if ((tgt_layout & 0x40u) != 0u) {
        const bool v154 = ((~src_mask & 0x180u) != 0u) || ((v11 & 0x180u) == 0u);
        if (v147 == 48u) {
            if (v154)
                return finish();
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(0x400000007uLL, 71))
                return false;
            dp.patch_mask(0x10u, 0xFFFFFF6Fu);
            if (r0 == 51u)
                return false;
            if (!dp.push_qd(0x500000008uLL, 72))
                return false;
            dp.patch_mask(0x20u, 0xFFFFFEDFu);
            if (r0 > 0x31u)
                return false;
            if (!dp.push_qd(0x600000007uLL, 73))
                return false;
            dp.patch_mask(0x40u, 0xFFFFFF3Fu);
            if (r0 == 49u)
                return false;
            *dp.row_count = r0 + 4u;
            if (!auro_downmix_v1_plan_apply_label274(
                    dp, r0 + 3u, 8, kDmPair18950, 64u, 0xFFFFFEBFu, src_mask))
                return false;
        } else {
            if (v154) {
                if ((~src_mask & 0x30u) != 0u)
                    return finish();
            } else {
                const std::uint64_t r0 = *dp.row_count;
                if (r0 > 0x33u)
                    return false;
                if (!dp.push_qd(0x400000007uLL, 69))
                    return false;
                dp.patch_mask(0x10u, 0xFFFFFF6Fu);
                if (r0 == 51u)
                    return false;
                if (!dp.push_qd(0x500000008uLL, 70))
                    return false;
                src_mask = src_mask & 0xFFFFFEDFu | 0x20u;
                dp.patch_mask(src_mask, 0xFFFFFFFFu);
                if ((~src_mask & 0x30u) != 0u)
                    return finish();
            }
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(0x600000004uLL, 88))
                return false;
            dp.patch_mask(0x40u, 0xFFFFFFAFu);
            if (r0 == 51u)
                return false;
            *dp.row_count = r0 + 2u;
            if (!auro_downmix_v1_plan_apply_label274(
                    dp, r0, 5, kDmPair22790, 64u, 0xFFFFFF9Fu, src_mask))
                return false;
        }
        return finish();
    }

    if (v147 == 48u) {
        if ((src_mask & 0x40u) != 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(0x700000006uLL, 77))
                return false;
            dp.patch_mask(0x80u, 0xFFFFFF3Fu);
            if (r0 == 51u)
                return false;
            if (!dp.push_qd(0x800000006uLL, 78))
                return false;
            src_mask = src_mask & 0xFFFFFEBFu | 0x100u;
            dp.patch_mask(src_mask, 0xFFFFFFFFu);
            if ((v11 & 0x180u) == 0u)
                return finish();
        } else if ((v11 & 0x180u) == 0u) {
            return finish();
        }
        if ((src_mask & 0x180u) != 0x180u)
            return finish();
        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        if (!dp.push_qd(0x400000007uLL, 69))
            return false;
        dp.patch_mask(0x10u, 0xFFFFFF6Fu);
        if (r0 == 51u)
            return false;
        *dp.row_count = r0 + 2u;
        if (!auro_downmix_v1_plan_apply_label274(
                dp, r0, 8, kDmPair17925, 32u, 0xFFFFFEDFu, src_mask))
            return false;
        return finish();
    }

    if (v73 != 1536u) {
        if ((v11 & 0x180u) != 0u && (src_mask & 0x180u) == 0x180u) {
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(7u, 75))
                return false;
            dp.patch_mask(1u, 0xFFFFFF7Eu);
            if (r0 == 51u)
                return false;
            if (!dp.push_qd(0x100000008uLL, 76))
                return false;
            src_mask = src_mask & 0xFFFFFEFDu | 2u;
            dp.patch_mask(src_mask, 0xFFFFFFFFu);
        }

        if ((src_mask & 0x40u) != 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (tgt_lo == 3u) {
                if (r0 > 0x33u)
                    return false;
                if (!dp.push_qd(6u, 79))
                    return false;
                dp.patch_mask(1u, 0xFFFFFFBEu);
                if (r0 == 51u)
                    return false;
                *dp.row_count = r0 + 2u;
                if (!auro_downmix_v1_plan_apply_label274(
                        dp, r0, 6, kDmPair20481, 2u, 0xFFFFFFBDu, src_mask))
                    return false;
            } else if (tgt_class_hint != 0u) {
                if (r0 > 0x33u)
                    return false;
                *dp.row_count = r0 + 1u;
                auto* row_base = reinterpret_cast<std::uint8_t*>(dp.plan) + 4u * (3u * r0 + 2u);
                *reinterpret_cast<std::int32_t*>(row_base) = 6;
                *reinterpret_cast<std::uint64_t*>(row_base + 4u) = kDmPair20738;
                src_mask = 4u | (src_mask & 0xFFFFFFBBu);
                dp.patch_mask(src_mask, 0xFFFFFFFFu);
            } else {
                if (r0 > 0x33u)
                    return false;
                *dp.row_count = r0 + 1u;
                auto* row_base = reinterpret_cast<std::uint8_t*>(dp.plan) + 4u * (3u * r0 + 2u);
                *reinterpret_cast<std::int32_t*>(row_base) = 6;
                *reinterpret_cast<std::uint64_t*>(row_base + 4u) = kDmPair20766;
                src_mask = 0x40000000u | (src_mask & 0xBFFFFFBFu);
                dp.patch_mask(src_mask, 0xFFFFFFFFu);
            }
        }

        if ((~src_mask & 0x30u) != 0u)
            return finish();

        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        if (!dp.push_qd(4u, 82))
            return false;
        dp.patch_mask(1u, 0xFFFFFFEEu);
        if (r0 == 51u)
            return false;
        *dp.row_count = r0 + 2u;
        if (!auro_downmix_v1_plan_apply_label274(
                dp, r0, 5, kDmPair21249, 2u, 0xFFFFFFDDu, src_mask))
            return false;
        return finish();
    }

    if ((src_mask & 0x40u) != 0u) {
        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        if (!dp.push_qd(0x700000006uLL, 77))
            return false;
        dp.patch_mask(0x80u, 0xFFFFFF3Fu);
        if (r0 == 51u)
            return false;
        if (!dp.push_qd(0x800000006uLL, 78))
            return false;
        src_mask = src_mask & 0xFFFFFEBFu | 0x100u;
        dp.patch_mask(src_mask, 0xFFFFFFFFu);
        if ((v11 & 0x180u) == 0u)
            return auro_downmix_v1_plan_label257_partial(
                dp, src_mask, tgt_layout, tgt_class_hint, engine_complete);
    } else if ((v11 & 0x180u) == 0u) {
        return auro_downmix_v1_plan_label257_partial(
            dp, src_mask, tgt_layout, tgt_class_hint, engine_complete);
    }

    if ((src_mask & 0x180u) == 0x180u) {
        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        if (!dp.push_qd(0x400000007uLL, 69))
            return false;
        dp.patch_mask(0x10u, 0xFFFFFF6Fu);
        if (r0 == 51u)
            return false;
        if (!dp.push_qd(0x500000008uLL, 70))
            return false;
        src_mask = src_mask & 0xFFFFFEDFu | 0x20u;
        dp.patch_mask(src_mask, 0xFFFFFFFFu);
    }

    return auro_downmix_v1_plan_label257_partial(
        dp, src_mask, tgt_layout, tgt_class_hint, engine_complete);
}

bool auro_downmix_v1_plan_label101_v73_1536_partial(
    DownmixPlanMut& dp,
    const std::int32_t* engine,
    std::uint32_t& src_mask,
    std::uint32_t tgt_layout,
    std::uint32_t inv_tgt) {
    const std::int32_t eng4 = engine ? engine[4] : 0;
    const std::int32_t eng5 = engine ? engine[5] : 0;
    const std::uint32_t v11 = inv_tgt;

    if ((v11 & 0x6000u) != 0u) {
        // IDA: v93 = 1; if ((v11 & 0x30) == 0) v93 = eng4;
        std::uint32_t v93 = 1u;
        if ((v11 & 0x30u) == 0u)
            v93 = static_cast<std::uint32_t>(eng4);
        if (v93 == 0u) {
            if ((tgt_layout & 0x8000u) == 0u && (src_mask & 0x8000u) != 0u) {
                const std::uint64_t r0 = *dp.row_count;
                if (r0 > 0x33u)
                    return false;
                if (!dp.push_qd(0x60000000FuLL, 40))
                    return false;
                src_mask = src_mask & 0xFFFF7FBFu | 0x40u;
                dp.patch_mask(src_mask, 0xFFFFFFFFu);
            }
            if ((v11 & 0x30000u) != 0u && (src_mask & 0x30000u) == 0x30000u) {
                const std::uint64_t r0 = *dp.row_count;
                if (r0 > 0x33u)
                    return false;
                if (!dp.push_qd(0x700000010uLL, 38))
                    return false;
                dp.patch_mask(0x80u, 0xFFFEFF7Fu);
                if (r0 == 51u)
                    return false;
                if (!dp.push_qd(0x800000011uLL, 39))
                    return false;
                src_mask = src_mask & 0xFFFDFEFFu | 0x100u;
                dp.patch_mask(src_mask, 0xFFFFFFFFu);
            }
            if ((~src_mask & 0x6000u) == 0u) {
                const std::uint64_t r0 = *dp.row_count;
                if (eng5 == 1) {
                    if (r0 > 0x33u)
                        return false;
                    if (!dp.push_qd(0x70000000DuLL, 45))
                        return false;
                    dp.patch_mask(0x80u, 0xFFFFDF7Fu);
                    if (r0 == 51u)
                        return false;
                    *dp.row_count = r0 + 2u;
                    if (!auro_downmix_v1_plan_apply_label274(
                            dp, r0, 14, kDmPair11784, 256u, 0xFFFFBF7Fu, src_mask))
                        return false;
                } else {
                    if (r0 > 0x33u)
                        return false;
                    if (!dp.push_qd(0x40000000DuLL, 43))
                        return false;
                    dp.patch_mask(0x10u, 0xFFFFDFEFu);
                    if (r0 == 51u)
                        return false;
                    *dp.row_count = r0 + 2u;
                    if (!auro_downmix_v1_plan_apply_label274(
                            dp, r0, 14, kDmPair11269, 32u, 0xFFFFDFFFu, src_mask))
                        return false;
                }
            }
        } else if (v93 != 1u) {
            return false;
        } else {
            if ((tgt_layout & 0x8000u) == 0u && (src_mask & 0x8000u) != 0u) {
                const std::uint64_t r0 = *dp.row_count;
                if (r0 > 0x33u)
                    return false;
                if (!dp.push_qd(0x100000000FuLL, 41))
                    return false;
                dp.patch_mask(0x10000u, 0xFFFE7FFFu);
                if (r0 == 51u)
                    return false;
                if (!dp.push_qd(0x110000000FuLL, 42))
                    return false;
                src_mask = src_mask & 0xFFFD7FFFu | 0x20000u;
                dp.patch_mask(src_mask, 0xFFFFFFFFu);
            }
            if ((v11 & 0x30000u) != 0u && (src_mask & 0x30000u) == 0x30000u) {
                const std::uint64_t r0 = *dp.row_count;
                if (r0 > 0x33u)
                    return false;
                if (!dp.push_qd(0xD00000010uLL, 36))
                    return false;
                dp.patch_mask(0x2000u, 0xFFFEDFFFu);
                if (r0 == 51u)
                    return false;
                if (!dp.push_qd(0xE00000011uLL, 37))
                    return false;
                src_mask = src_mask & 0xFFFDBFFFu | 0x4000u;
                dp.patch_mask(src_mask, 0xFFFFFFFFu);
            }
            if ((~src_mask & 0x6000u) == 0u) {
                const std::uint64_t r0 = *dp.row_count;
                if (r0 > 0x33u)
                    return false;
                if (!dp.push_qd(0x90000000DuLL, 47))
                    return false;
                dp.patch_mask(0x200u, 0xFFFFDDFFu);
                if (r0 == 51u)
                    return false;
                *dp.row_count = r0 + 2u;
                if (!auro_downmix_v1_plan_apply_label274(
                        dp, r0, 14, kDmPair12298, 1024u, 0xFFFFBBFFu, src_mask))
                    return false;
            }
        }
    } else if ((tgt_layout & 0x8000u) != 0u) {
        if ((v11 & 0x30000u) != 0u && (src_mask & 0x30000u) == 0x30000u) {
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(0xD00000010uLL, 32))
                return false;
            dp.patch_mask(0x2000u, 0xFFFEDFFFu);
            if (r0 == 51u)
                return false;
            if (!dp.push_qd(0xE00000011uLL, 33))
                return false;
            dp.patch_mask(0x4000u, 0xFFFDBFFFu);
            if (r0 > 0x31u)
                return false;
            if (!dp.push_qd(0xF00000010uLL, 34))
                return false;
            dp.patch_mask(0x8000u, 0xFFFE7FFFu);
            if (r0 == 49u)
                return false;
            *dp.row_count = r0 + 4u;
            if (!auro_downmix_v1_plan_apply_label274(
                    dp, r0 + 3u, 17, kDmPair8975, 0x8000u, 0xFFFD7FFFu, src_mask))
                return false;
        }
    } else {
        if ((src_mask & 0x8000u) != 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(0x100000000FuLL, 41))
                return false;
            dp.patch_mask(0x10000u, 0xFFFE7FFFu);
            if (r0 == 51u)
                return false;
            if (!dp.push_qd(0x110000000FuLL, 42))
                return false;
            src_mask = src_mask & 0xFFFD7FFFu | 0x20000u;
            dp.patch_mask(src_mask, 0xFFFFFFFFu);
            if ((v11 & 0x30000u) == 0u)
                goto label_203;
        } else if ((v11 & 0x30000u) == 0u) {
            goto label_203;
        }
        if ((src_mask & 0x30000u) == 0x30000u) {
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(0xD00000010uLL, 36))
                return false;
            dp.patch_mask(0x2000u, 0xFFFEDFFFu);
            if (r0 == 51u)
                return false;
            *dp.row_count = r0 + 2u;
            if (!auro_downmix_v1_plan_apply_label274(
                    dp, r0, 17, kDmPair9486, 0x4000u, 0xFFFD9FFFu, src_mask))
                return false;
        }
    }

label_203:
    if ((tgt_layout & 0x800u) == 0u && (src_mask & 0x800u) != 0u) {
        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        if (!dp.push_qd(0x90000000BuLL, 49))
            return false;
        dp.patch_mask(0x200u, 0xFFFFF5FFu);
        if (r0 == 51u)
            return false;
        *dp.row_count = r0 + 2u;
        if (!auro_downmix_v1_plan_apply_label274(
                dp, r0, 11, kDmPair12810, 1024u, 0xFFFFF3FFu, src_mask))
            return false;
    }
    return true;
}

bool auro_downmix_v1_plan_height_post_label101_partial(
    std::int32_t* plan,
    const std::int32_t* engine,
    std::uint32_t& src_mask,
    std::uint32_t tgt_layout,
    std::uint32_t inv_tgt,
    bool* goto_label101) {
    if (goto_label101)
        *goto_label101 = false;
    DownmixPlanMut dp(plan);
    const std::int32_t eng0 = engine ? engine[0] : 0;

    // IDA @591625: tgt 0x30000000 -> LABEL_100 (before eng0/eng1).
    if ((tgt_layout & 0x30000000u) == 0x30000000u) {
        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        if (!dp.push_qd(0x1C0000000CuLL, 14))
            return false;
        dp.patch_mask(0x10000000u, 0xEFFFEFFFu);
        if (r0 == 51u)
            return false;
        *dp.row_count = r0 + 2u;
        if (!auro_downmix_v1_plan_apply_label100_write(
                dp, r0 + 1u, 12, kDmPair3869, 0x20000000u, 0xDFFEFBFFu, src_mask))
            return false;
        if (goto_label101)
            *goto_label101 = true;
        return true;
    }

    // IDA: eng0 only when (inv_tgt & 0x600) == 0; else fall through to eng1.
    if ((inv_tgt & 0x600u) != 0u)
        return true;

    if ((inv_tgt & 0x6000u) != 0u || eng0 == 2) {
        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        *dp.row_count = r0 + 1u;
        if (!auro_downmix_v1_plan_apply_label100_write(
                dp, r0, 12, kDmPair4107, 2048u, 0xFFFFE7FFu, src_mask))
            return false;
        if (goto_label101)
            *goto_label101 = true;
        return true;
    }

    // eng0==1 without tgt&0x800 falls through to eng0==0 path (IDA @591663).
    if (eng0 == 1 && (tgt_layout & 0x800u) != 0u) {
        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        if (!dp.push_qd(0xB0000000CuLL, 17))
            return false;
        dp.patch_mask(0x800u, 0xFFFFE7FFu);
        if (r0 == 51u)
            return false;
        if (!dp.push_qd(0xD0000000CuLL, 18))
            return false;
        dp.patch_mask(0x2000u, 0xFFFFCFFFu);
        if (r0 > 0x31u)
            return false;
        *dp.row_count = r0 + 3u;
        if (!auro_downmix_v1_plan_apply_label100_write(
                dp, r0 + 2u, 12, kDmPair4878, 0x4000u, 0xFFFFAFFFu, src_mask))
            return false;
        if (goto_label101)
            *goto_label101 = true;
        return true;
    }

    if (eng0 != 0 && eng0 != 1)
        return false;

    const std::uint64_t r0 = *dp.row_count;
    if (r0 > 0x33u)
        return false;
    if (!dp.push_qd(0x90000000CuLL, 20))
        return false;
    dp.patch_mask(0x200u, 0xFFFFEDFFu);
    if (r0 == 51u)
        return false;
    if (!dp.push_qd(0xA0000000CuLL, 21))
        return false;
    dp.patch_mask(0x400u, 0xFFFFEBFFu);
    if (r0 > 0x31u)
        return false;
    if (!dp.push_qd(0xD0000000CuLL, 22))
        return false;
    dp.patch_mask(0x2000u, 0xFFFFCFFFu);
    if (r0 == 49u)
        return false;
    *dp.row_count = r0 + 4u;
    if (!auro_downmix_v1_plan_apply_label100_write(
            dp, r0 + 3u, 12, kDmPair5902, 0x4000u, 0xFFFFAFFFu, src_mask))
        return false;
    if (goto_label101)
        *goto_label101 = true;
    return true;
}

bool auro_downmix_v1_plan_height_tail_eng1_partial(
    std::int32_t* plan,
    const std::int32_t* engine,
    std::uint32_t& src_mask,
    std::uint32_t tgt_layout,
    bool* goto_label101_out) {
    if (goto_label101_out)
        *goto_label101_out = false;
    DownmixPlanMut dp(plan);
    const std::int32_t eng1 = engine ? engine[1] : 0;
    const std::uint32_t inv_tgt = ~tgt_layout;

    if ((inv_tgt & 0x30u) != 0u || eng1 == 2) {
        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        if (!auro_downmix_v1_plan_apply_label100(
                dp, r0, 12, kDmPair6146, 4u, 0xFFFFFEFBu, src_mask))
            return false;
        // IDA LABEL_100 -> LABEL_101.
        if (goto_label101_out)
            *goto_label101_out = true;
        return true;
    }

    // eng1==1 with tgt&4 -> 6917; without bit4 falls through to eng1==0 (IDA @591751).
    if (eng1 == 1 && (tgt_layout & 4u) != 0u) {
        const std::uint64_t r0 = *dp.row_count;
        if (r0 > 0x33u)
            return false;
        if (!dp.push_qd(0x20000000CuLL, 25))
            return false;
        dp.patch_mask(4u, 0xFFFFEFFBu);
        if (r0 == 51u)
            return false;
        if (!dp.push_qd(0x40000000CuLL, 26))
            return false;
        dp.patch_mask(0x10u, 0xFFFFEFEFu);
        if (r0 > 0x31u)
            return false;
        *dp.row_count = r0 + 3u;
        if (!auro_downmix_v1_plan_apply_label100_write(
                dp, r0 + 2u, 12, kDmPair6917, 32u, 0xFFFFEFDFu, src_mask))
            return false;
        if (goto_label101_out)
            *goto_label101_out = true;
        return true;
    }

    if (eng1 != 0 && eng1 != 1)
        return false;

    const std::uint64_t r0 = *dp.row_count;
    if (r0 > 0x33u)
        return false;
    if (!dp.push_qd(12u, 28))
        return false;
    dp.patch_mask(1u, 0xFFFFEFFEu);
    if (r0 == 51u)
        return false;
    if (!dp.push_qd(0x10000000CuLL, 29))
        return false;
    dp.patch_mask(2u, 0xFFFFEFFDu);
    if (r0 > 0x31u)
        return false;
    if (!dp.push_qd(0x40000000CuLL, 30))
        return false;
    dp.patch_mask(0x10u, 0xFFFFEFEFu);
    if (r0 == 49u)
        return false;
    *dp.row_count = r0 + 4u;
    if (!auro_downmix_v1_plan_apply_label100_write(
            dp, r0 + 3u, 12, kDmPair7941, 32u, 0xFFFFEFDFu, src_mask))
        return false;
    if (goto_label101_out)
        *goto_label101_out = true;
    return true;
}

bool auro_downmix_v1_plan_label101_partial(
    std::int32_t* plan,
    const std::int32_t* engine,
    std::uint32_t& src_mask,
    std::uint32_t tgt_layout,
    std::uint32_t inv_tgt,
    std::uint8_t tgt_class_hint,
    bool* engine_complete) {
    DownmixPlanMut dp(plan);
    const std::int32_t eng5 = engine ? engine[5] : 0;

    const std::uint32_t v73 = tgt_layout & 0x600u;
    if (v73 != 1536u) {
        if (downmix_label101_tgt_layout_rejected(tgt_layout))
            return false;

        if ((~src_mask & 0x30000u) == 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(0x700000010uLL, 38))
                return false;
            dp.patch_mask(0x80u, 0xFFFEFF7Fu);
            if (r0 == 51u)
                return false;
            if (!dp.push_qd(0x800000011uLL, 39))
                return false;
            src_mask = src_mask & 0xFFFDFEFFu | 0x100u;
            dp.patch_mask(src_mask, 0xFFFFFFFFu);
        }

        if ((src_mask & 0x8000u) != 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(0x60000000FuLL, 40))
                return false;
            src_mask = src_mask & 0xFFFF7FBFu | 0x40u;
            dp.patch_mask(src_mask, 0xFFFFFFFFu);
        }

        if ((~src_mask & 0x6000u) == 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (eng5 == 1) {
                if (r0 > 0x33u)
                    return false;
                if (!dp.push_qd(0x70000000DuLL, 45))
                    return false;
                dp.patch_mask(0x80u, 0xFFFFDF7Fu);
                if (r0 == 51u)
                    return false;
                *dp.row_count = r0 + 2u;
                if (!auro_downmix_v1_plan_apply_label274(
                        dp, r0, 14, kDmPair11784, 256u, 0xFFFFBF7Fu, src_mask))
                    return false;
            } else {
                if (r0 > 0x33u)
                    return false;
                if (!dp.push_qd(0x40000000DuLL, 43))
                    return false;
                dp.patch_mask(0x10u, 0xFFFFDFEFu);
                if (r0 == 51u)
                    return false;
                *dp.row_count = r0 + 2u;
                if (!auro_downmix_v1_plan_apply_label274(
                        dp, r0, 14, kDmPair11269, 32u, 0xFFFFDFFFu, src_mask))
                    return false;
            }
        }

        if ((src_mask & 0x800u) != 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(0x20000000BuLL, 51))
                return false;
            src_mask = src_mask & 0xFFFFF7FBu | 4u;
            dp.patch_mask(src_mask, 0xFFFFFFFFu);
        }

        if ((~src_mask & 0x600u) == 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (r0 > 0x33u)
                return false;
            if (!dp.push_qd(9u, 52))
                return false;
            dp.patch_mask(1u, 0xFFFFFDFEu);
            if (r0 == 51u)
                return false;
            *dp.row_count = r0 + 2u;
            if (!auro_downmix_v1_plan_apply_label274(
                    dp, r0, 10, kDmPair13569, 2u, 0xFFFFFBFDu, src_mask))
                return false;
        }

        return auro_downmix_v1_plan_label209_partial(
            dp, src_mask, tgt_layout, inv_tgt, tgt_class_hint, engine_complete);
    }

    if (!auro_downmix_v1_plan_label101_v73_1536_partial(
            dp, engine, src_mask, tgt_layout, inv_tgt))
        return false;
    return auro_downmix_v1_plan_label209_partial(
        dp, src_mask, tgt_layout, inv_tgt, tgt_class_hint, engine_complete);
}

bool auro_downmix_v1_plan_apply_label35(
    DownmixPlanMut& dp,
    std::uint64_t anchor_row,
    std::int32_t dword0,
    std::uint64_t pair_qword,
    std::uint32_t or_bits,
    std::uint32_t and_mask,
    std::uint32_t& src_mask) {
    if (!dp.push_dq_after_two(anchor_row, dword0, pair_qword, src_mask))
        return false;
    src_mask = or_bits | (src_mask & and_mask);
    dp.patch_mask(src_mask, 0xFFFFFFFFu);
    return true;
}

bool auro_downmix_v1_plan_apply_label29(
    DownmixPlanMut& dp,
    std::int32_t dword0,
    std::uint64_t pair_qword,
    std::uint32_t or_bits,
    std::uint32_t and_mask,
    std::uint32_t& src_mask) {
    const std::uint64_t rows = *dp.row_count;
    if (rows > 0x33u)
        return false;
    auto* row_base = reinterpret_cast<std::uint8_t*>(dp.plan) + 8u + 12u * rows;
    *reinterpret_cast<std::int32_t*>(row_base) = dword0;
    *reinterpret_cast<std::uint64_t*>(row_base + 4u) = pair_qword;
    src_mask = or_bits | (src_mask & and_mask);
    dp.patch_mask(src_mask, 0xFFFFFFFFu);
    return true;
}

bool auro_downmix_v1_plan_height_preprocess_partial(
    std::int32_t* plan,
    const std::int32_t* engine,
    std::uint32_t src_layout,
    std::uint32_t tgt_layout,
    bool* engine_complete) {
    if (engine_complete)
        *engine_complete = false;
    DownmixPlanMut dp(plan);
    const std::uint32_t inv_tgt = ~tgt_layout;
    std::uint32_t src_mask = *dp.out_mask;
    const std::int32_t eng2 = engine ? engine[2] : 0;
    const std::int32_t eng3 = engine ? engine[3] : 0;

    const std::uint32_t v13 = src_mask & 0x3000000u;
    if ((inv_tgt & 0x600000u) != 0u) {
        if (v13 == 50331648u) {
            const std::uint64_t r0 = *dp.row_count;
            if (!dp.push_qd(0x400000018uLL, 54))
                return false;
            dp.patch_mask(0x10u, 0xFEFFFFEFu);
            if (r0 == 51u)
                return false;
            if (!dp.push_qd(0x500000019uLL, 55))
                return false;
            src_mask = src_mask & 0xFDFFFFDFu | 0x20u;
            dp.patch_mask(src_mask, 0xFFFFFFFFu);
        }
        if ((src_mask & 0x800000u) != 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (!dp.push_qd(0x200000017uLL, 58))
                return false;
            src_mask = src_mask & 0xFF7FFFFBu | 4u;
            dp.patch_mask(src_mask, 0xFFFFFFFFu);
            if (r0 == 51u)
                return false;
        }
        if ((~src_mask & 0x600000u) == 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (!dp.push_qd(21u, 61))
                return false;
            dp.patch_mask(1u, 0xFFDFFFFEu);
            if (r0 == 51u)
                return false;
            *dp.row_count = r0 + 2u;
            if (!auro_downmix_v1_plan_apply_label35(
                    dp, r0, 22, kDmPair15873, 2u, 0xFFBFFFFDu, src_mask))
                return false;
        }
    } else {
        if (v13 == 50331648u && (inv_tgt & 0x3000000u) != 0u) {
            const std::uint64_t r0 = *dp.row_count;
            std::uint64_t pair_qword = 0u;
            std::uint32_t or_bits = 0u;
            std::uint32_t and_mask = 0u;
            if ((inv_tgt & 0x30u) != 0u) {
                if (!dp.push_qd(0x1500000018uLL, 56))
                    return false;
                dp.patch_mask(0x200000u, 0xFEDFFFFFu);
                if (r0 == 51u)
                    return false;
                *dp.row_count = r0 + 2u;
                pair_qword = kDmPair14614;
                or_bits = 0x400000u;
                and_mask = 0xFDC000FFu;
            } else {
                if (!dp.push_qd(0x400000018uLL, 54))
                    return false;
                dp.patch_mask(0x10u, 0xFEFFFFEFu);
                if (r0 == 51u)
                    return false;
                *dp.row_count = r0 + 2u;
                pair_qword = kDmPair14085;
                or_bits = 32u;
                and_mask = 0xFE00000Fu;
            }
            if (!auro_downmix_v1_plan_apply_label29(dp, 25, pair_qword, or_bits, and_mask, src_mask))
                return false;
            if ((tgt_layout & 0x800000u) != 0u)
                goto label_36;
        } else if ((tgt_layout & 0x800000u) != 0u) {
            goto label_36;
        }
        if ((src_mask & 0x800000u) != 0u) {
            const std::uint64_t r0 = *dp.row_count;
            if (!dp.push_qd(0x1500000017uLL, 59))
                return false;
            dp.patch_mask(0x200000u, 0xFF5FFFFFu);
            if (r0 == 51u)
                return false;
            *dp.row_count = r0 + 2u;
            if (!auro_downmix_v1_plan_apply_label35(
                    dp, r0, 23, kDmPair15382, 0x400000u, 0xFF40000Fu, src_mask))
                return false;
        }
    }

label_36:
    // IDA @590733: only when tgt lacks 0x30000000 and src has it.
    if ((tgt_layout & 0x30000000u) != 0x30000000u && (src_mask & 0x30000000u) == 0x30000000u) {
        if ((inv_tgt & 0x600u) != 0u) {
            std::uint32_t v49 = 2u * static_cast<std::uint32_t>((inv_tgt & 0x30u) != 0u);
            if (eng3 != 0)
                v49 = static_cast<std::uint32_t>(eng3);
            if (v49 == 2u) {
                const std::uint64_t r0 = *dp.row_count;
                if (!dp.push_qd(28u, 12))
                    return false;
                dp.patch_mask(1u, 0xEFFFFFFEu);
                if (r0 == 51u)
                    return false;
                *dp.row_count = r0 + 2u;
                if (!auro_downmix_v1_plan_apply_label29(dp, 29, kDmPair3329, 2u, 0xDFFFFFFFu, src_mask))
                    return false;
            } else if (v49 != 0u) {
                return false;
            } else {
                const std::uint64_t r0 = *dp.row_count;
                if (!dp.push_qd(28u, 8))
                    return false;
                dp.patch_mask(1u, 0xEFFFFFFEu);
                if (r0 == 51u)
                    return false;
                if (!dp.push_qd(0x10000001DuLL, 9))
                    return false;
                dp.patch_mask(2u, 0xDFFFFFFDu);
                if (r0 > 0x31u)
                    return false;
                if (!dp.push_qd(0x40000001CuLL, 10))
                    return false;
                dp.patch_mask(0x10u, 0xEFFFFFEFu);
                if (r0 == 49u)
                    return false;
                *dp.row_count = r0 + 4u;
                if (!auro_downmix_v1_plan_apply_label29(dp, 29, kDmPair2821, 32u, 0xDFFFFFC1u, src_mask))
                    return false;
            }
        } else {
            std::uint32_t v31 =
                static_cast<std::uint32_t>((static_cast<std::int32_t>(tgt_layout << 19) >> 31) & 3);
            if (eng2 != 3)
                v31 = static_cast<std::uint32_t>(eng2);
            std::uint32_t v32 = 2u * static_cast<std::uint32_t>((inv_tgt & 0x6000u) != 0u);
            if (v31 != 0u)
                v32 = v31;
            if (v32 == 3u) {
                const std::uint64_t r0 = *dp.row_count;
                if (!dp.push_qd(0xC0000001CuLL, 0))
                    return false;
                dp.patch_mask(0x1000u, 0xEFFFEFFFu);
                if (r0 == 51u)
                    return false;
                *dp.row_count = r0 + 2u;
                if (!auro_downmix_v1_plan_apply_label29(dp, 29, kDmPair268, 4096u, 0xDFFEFBFFu, src_mask))
                    return false;
            } else if (v32 == 2u) {
                const std::uint64_t r0 = *dp.row_count;
                if (!dp.push_qd(0x90000001CuLL, 6))
                    return false;
                dp.patch_mask(0x200u, 0xEFFFFDFFu);
                if (r0 == 51u)
                    return false;
                *dp.row_count = r0 + 2u;
                if (!auro_downmix_v1_plan_apply_label29(dp, 29, kDmPair1802, 1024u, 0xDFFFF7FFu, src_mask))
                    return false;
            } else if (v32 != 0u) {
                return false;
            } else {
                const std::uint64_t r0 = *dp.row_count;
                if (!dp.push_qd(0x90000001CuLL, 2))
                    return false;
                dp.patch_mask(0x200u, 0xEFFFFDFFu);
                if (r0 == 51u)
                    return false;
                if (!dp.push_qd(0xA0000001DuLL, 3))
                    return false;
                dp.patch_mask(0x400u, 0xDFFFFBFFu);
                if (r0 > 0x31u)
                    return false;
                if (!dp.push_qd(0xD0000001CuLL, 4))
                    return false;
                dp.patch_mask(0x2000u, 0xEFFFDFFFu);
                if (r0 == 49u)
                    return false;
                *dp.row_count = r0 + 4u;
                if (!auro_downmix_v1_plan_apply_label29(dp, 29, kDmPair1294, 0x4000u, 0xDFFFBFFFu, src_mask))
                    return false;
            }
        }
    }

    const std::uint8_t tgt_class_hint =
        static_cast<std::uint8_t>(static_cast<std::uint32_t>((tgt_layout & 3u) != 3u) & (tgt_layout >> 2u));
    // IDA @590888: LABEL_101 when tgt has 0x1000 or src lacks it.
    if ((tgt_layout & 0x1000u) != 0u || (src_mask & 0x1000u) == 0u) {
        if (!auro_downmix_v1_plan_label101_partial(
                plan, engine, src_mask, tgt_layout, inv_tgt, tgt_class_hint, engine_complete))
            return false;
        if (engine_complete && *engine_complete)
            return true;
    }

    // IDA eng0: single LABEL_100 -> LABEL_101 (no re-entry).
    {
        bool goto_label101 = false;
        if (!auro_downmix_v1_plan_height_post_label101_partial(
                plan, engine, src_mask, tgt_layout, inv_tgt, &goto_label101))
            return false;
        if (goto_label101) {
            if (!auro_downmix_v1_plan_label101_partial(
                    plan, engine, src_mask, tgt_layout, inv_tgt, tgt_class_hint, engine_complete))
                return false;
            if (engine_complete && *engine_complete)
                return true;
        }
    }

    // IDA eng1: single LABEL_100 -> LABEL_101, then LABEL_275.
    {
        bool goto_tail101 = false;
        if (!auro_downmix_v1_plan_height_tail_eng1_partial(
                plan, engine, src_mask, tgt_layout, &goto_tail101))
            return false;
        if (goto_tail101) {
            if (!auro_downmix_v1_plan_label101_partial(
                    plan, engine, src_mask, tgt_layout, inv_tgt, tgt_class_hint, engine_complete))
                return false;
            if (engine_complete && *engine_complete)
                return true;
        }
    }
    // IDA LABEL_275 @ 591129: sub_5911C0 -> sub_5914E0 -> sub_5917B0.
    return auro_downmix_v1_plan_finalize_label275_partial(plan, tgt_layout, tgt_class_hint);
}

bool auro_downmix_v1_plan_sub_5911c0_partial(std::int32_t* plan, std::uint32_t tgt_layout) {
    auto* row_count = reinterpret_cast<std::uint64_t*>(plan);
    auto* out_mask = reinterpret_cast<std::uint32_t*>(plan + 159);
    const std::uint32_t inv_tgt = ~tgt_layout;
    std::uint32_t mask_word = *out_mask;
    bool result = false;

    if ((~mask_word & 0xC000000u) != 0u || (inv_tgt & 0xC000000u) == 0u) {
        result = true;
        if ((inv_tgt & 0xC0000u) == 0u)
            return result;
    } else {
        std::uint64_t rows = *row_count;
        result = false;
        if ((inv_tgt & 0x30u) != 0u) {
            if (rows > 0x33u)
                return false;
            *row_count = rows + 1u;
            const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
            *reinterpret_cast<std::uint64_t*>(plan + off) = 26u;
            plan[off + 2] = 63;
            *out_mask = (*out_mask & 0xFBFFFFFEu) | 1u;
            if (rows == 51u)
                return false;
            *row_count = rows + 2u;
            auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 20u;
            *reinterpret_cast<std::int32_t*>(row_base) = 27;
            *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x4000000001uLL;
            mask_word = (2u | (*out_mask & 0xF7FFFFFFu));
            *out_mask = mask_word;
            result = true;
            if ((inv_tgt & 0xC0000u) == 0u)
                return result;
        } else {
            if (rows > 0x33u)
                return false;
            *row_count = rows + 1u;
            const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
            *reinterpret_cast<std::uint64_t*>(plan + off) = 26u;
            plan[off + 2] = 65;
            *out_mask = (*out_mask & 0xFBFFFFFEu) | 1u;
            if (rows == 51u)
                return false;
            *row_count = rows + 2u;
            const std::size_t off1 = 2u + 3u * static_cast<std::size_t>(rows + 1u);
            *reinterpret_cast<std::uint64_t*>(plan + off1) = 0x10000001BLL;
            plan[off1 + 2] = 66;
            *out_mask = (*out_mask & 0xF7FFFFFDu) | 2u;
            if (rows > 0x31u)
                return false;
            *row_count = rows + 3u;
            const std::size_t off2 = 2u + 3u * static_cast<std::size_t>(rows + 2u);
            *reinterpret_cast<std::uint64_t*>(plan + off2) = 0x40000001ALL;
            plan[off2 + 2] = 67;
            *out_mask = (*out_mask & 0xFBFFFFEFu) | 0x10u;
            if (rows == 49u)
                return false;
            *row_count = rows + 4u;
            auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 36u;
            *reinterpret_cast<std::int32_t*>(row_base) = 27;
            *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x4400000005uLL;
            mask_word = (32u | (*out_mask & 0xF7FFFFDFu));
            *out_mask = mask_word;
            result = true;
            if ((inv_tgt & 0xC0000u) == 0u)
                return result;
        }
    }

    if ((mask_word & 0xC0000u) != 0xC0000u)
        return result;

    const std::uint64_t rows = *row_count;
    if ((inv_tgt & 7u) != 0u) {
        if (rows > 0x33u)
            return false;
        *row_count = rows + 1u;
        const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
        *reinterpret_cast<std::uint64_t*>(plan + off) = 18u;
        plan[off + 2] = 90;
        *out_mask = (*out_mask & 0xFFFBFFFEu) | 1u;
        if (rows == 51u)
            return false;
        *row_count = rows + 2u;
        auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 20u;
        *reinterpret_cast<std::int32_t*>(row_base) = 19;
        *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x5B00000001uLL;
        *out_mask = (2u | (*out_mask & 0xFFF7FFFDu));
        return true;
    }

    if (rows > 0x33u)
        return false;
    *row_count = rows + 1u;
    {
        const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
        *reinterpret_cast<std::uint64_t*>(plan + off) = 18u;
        plan[off + 2] = 92;
    }
    *out_mask = (*out_mask & 0xFFFBFFFEu) | 1u;
    if (rows == 51u)
        return false;
    *row_count = rows + 2u;
    {
        const std::size_t off1 = 2u + 3u * static_cast<std::size_t>(rows + 1u);
        *reinterpret_cast<std::uint64_t*>(plan + off1) = 0x100000013LL;
        plan[off1 + 2] = 93;
    }
    *out_mask = (*out_mask & 0xFFF7FFFDu) | 2u;
    if (rows > 0x31u)
        return false;
    *row_count = rows + 3u;
    {
        const std::size_t off2 = 2u + 3u * static_cast<std::size_t>(rows + 2u);
        *reinterpret_cast<std::uint64_t*>(plan + off2) = 0x200000012LL;
        plan[off2 + 2] = 94;
    }
    *out_mask = (*out_mask & 0xFFFBFFFBu) | 4u;
    if (rows == 49u)
        return false;
    *row_count = rows + 4u;
    auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 36u;
    *reinterpret_cast<std::int32_t*>(row_base) = 19;
    *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x5F00000002uLL;
    *out_mask = (4u | (*out_mask & 0xFFFBFFFBu));
    return true;
}

bool auro_downmix_v1_plan_sub_5914e0_partial(
    std::int32_t* plan,
    std::uint32_t tgt_layout,
    std::uint8_t tgt_class_hint) {
    (void)tgt_class_hint;
    auto* row_count = reinterpret_cast<std::uint64_t*>(plan);
    auto* out_mask = reinterpret_cast<std::uint32_t*>(plan + 159);
    std::uint32_t mask_word = *out_mask;
    const std::uint32_t inv_tgt = ~tgt_layout;

    if ((inv_tgt & 3u) != 0u) {
        if ((mask_word & 0x100000u) != 0u && (tgt_layout & 0x100000u) == 0u) {
            const std::uint64_t rows = *row_count;
            if (tgt_class_hint == 0u) {
                if (rows >= 0x34u)
                    return false;
                *row_count = rows + 1u;
                const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
                *reinterpret_cast<std::uint64_t*>(plan + off) = 0x1E00000014uLL;
                plan[off + 2] = 109;
                const std::uint32_t prev = *out_mask;
                *out_mask = (*out_mask & 0xBFEFFFFFu) | 0x40000000u;
                if ((((prev & 8u) >> 3u) & static_cast<std::uint32_t>((tgt_layout & 8u) == 0u)) == 0u)
                    return true;
                const std::uint64_t rows2 = *row_count;
                if (rows2 > 0x33u)
                    return false;
                *row_count = rows2 + 1u;
                auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows2 + 8u;
                *reinterpret_cast<std::int32_t*>(row_base) = 3;
                *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6E0000001EuLL;
                *out_mask = (0x40000000u | (*out_mask & 0xBFFFFFF9u));
                return true;
            }
            if (rows > 0x33u)
                return false;
            *row_count = rows + 1u;
            const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
            *reinterpret_cast<std::uint64_t*>(plan + off) = 0x200000014uLL;
            plan[off + 2] = 109;
            mask_word = (*out_mask & 0xFFEFFFFBu) | 4u;
            *out_mask = mask_word;
        }
        if ((tgt_layout & 8u) != 0u || (mask_word & 8u) == 0u)
            return true;
        if (tgt_class_hint != 0u) {
            const std::uint64_t rows = *row_count;
            if (rows > 0x33u)
                return false;
            *row_count = rows + 1u;
            auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 8u;
            *reinterpret_cast<std::int32_t*>(row_base) = 3;
            *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6E00000002uLL;
            *out_mask = (4u | (*out_mask & 0xFFFFFFF3u));
            return true;
        }
        const std::uint64_t rows = *row_count;
        if (rows > 0x33u)
            return false;
        *row_count = rows + 1u;
        auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 8u;
        *reinterpret_cast<std::int32_t*>(row_base) = 3;
        *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6E0000001EuLL;
        *out_mask = (0x40000000u | (*out_mask & 0xBFFFFFF9u));
        return true;
    }

    if ((mask_word & 0x100000u) != 0u) {
        if ((tgt_layout & 0x100000u) != 0u)
            return true;
        const std::uint64_t rows = *row_count;
        if ((tgt_layout & 8u) != 0u) {
            if (rows > 0x33u)
                return false;
            *row_count = rows + 1u;
            auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 8u;
            *reinterpret_cast<std::int32_t*>(row_base) = 20;
            *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6A00000003uLL;
            *out_mask = (8u | (*out_mask & 0xFFEFFFFFu));
            return true;
        }
        if (rows <= 0x33u) {
            *row_count = rows + 1u;
            const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
            *reinterpret_cast<std::uint64_t*>(plan + off) = 20u;
            plan[off + 2] = 107;
            *out_mask = (*out_mask & 0xFFEFFFFEu) | 1u;
            if (rows != 51u) {
                *row_count = rows + 2u;
                auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 20u;
                *reinterpret_cast<std::int32_t*>(row_base) = 3;
                *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6C00000001uLL;
                *out_mask = (2u | (*out_mask & 0xFFFFFFF5u));
                return true;
            }
        }
        return false;
    }

    if (((mask_word & 8u) == 0u) | static_cast<std::uint32_t>((tgt_layout & 8u) >> 3u))
        return true;
    const std::uint64_t rows = *row_count;
    if (rows > 0x33u)
        return false;
    *row_count = rows + 1u;
    const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
    *reinterpret_cast<std::uint64_t*>(plan + off) = 3u;
    plan[off + 2] = 104;
    *out_mask = (*out_mask & 0xFFFFFFF6u) | 1u;
    if (rows == 51u)
        return false;
    *row_count = rows + 2u;
    auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 20u;
    *reinterpret_cast<std::int32_t*>(row_base) = 3;
    *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6900000001uLL;
    *out_mask = (2u | (*out_mask & 0xFFFFFFF5u));
    return true;
}

bool auro_downmix_v1_plan_sub_5917b0_partial(
    std::int32_t* plan,
    std::uint32_t tgt_layout,
    std::uint8_t tgt_class_hint) {
    auto* row_count = reinterpret_cast<std::uint64_t*>(plan);
    auto* out_mask = reinterpret_cast<std::uint32_t*>(plan + 159);
    const std::uint32_t inv_tgt = ~tgt_layout;

    if ((inv_tgt & 3u) != 0u) {
        std::uint32_t mask_word = *out_mask;
        if ((~mask_word & 3u) != 0u) {
            if (tgt_class_hint != 0u) {
                if ((tgt_layout & 0x40000000u) != 0u || (mask_word & 0x40000000u) == 0u)
                    return true;
                const std::uint64_t rows = *row_count;
                if (rows > 0x33u)
                    return false;
                *row_count = rows + 1u;
                auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 8u;
                *reinterpret_cast<std::int32_t*>(row_base) = 30;
                *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6500000002uLL;
                *out_mask = (4u | (*out_mask & 0xBFFFFFFBu));
                return true;
            }
        } else {
            const std::uint64_t rows = *row_count;
            if (tgt_class_hint != 0u) {
                if (rows >= 0x34u)
                    return false;
                *row_count = rows + 1u;
                const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
                *reinterpret_cast<std::uint64_t*>(plan + off) = 0x200000000uLL;
                plan[off + 2] = 102;
                *out_mask = (*out_mask & 0xFFFFFFFAu) | 4u;
                if (rows == 51u)
                    return false;
                *row_count = rows + 2u;
                const std::size_t off1 = 2u + 3u * static_cast<std::size_t>(rows + 1u);
                *reinterpret_cast<std::uint64_t*>(plan + off1) = 0x200000001uLL;
                plan[off1 + 2] = 103;
                mask_word = (*out_mask & 0xFFFFFFF9u) | 4u;
                *out_mask = mask_word;
                if ((tgt_layout & 0x40000000u) != 0u || (mask_word & 0x40000000u) == 0u)
                    return true;
                const std::uint64_t rows2 = *row_count;
                if (rows2 > 0x33u)
                    return false;
                *row_count = rows2 + 1u;
                auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows2 + 8u;
                *reinterpret_cast<std::int32_t*>(row_base) = 30;
                *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6500000002uLL;
                *out_mask = (4u | (*out_mask & 0xBFFFFFFBu));
                return true;
            }
            if (rows >= 0x34u)
                return false;
            *row_count = rows + 1u;
            const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
            *reinterpret_cast<std::uint64_t*>(plan + off) = 0x1E00000000uLL;
            plan[off + 2] = 102;
            *out_mask = (*out_mask & 0xBFFFFFFEu) | 0x40000000u;
            if (rows == 51u)
                return false;
            *row_count = rows + 2u;
            const std::size_t off1 = 2u + 3u * static_cast<std::size_t>(rows + 1u);
            *reinterpret_cast<std::uint64_t*>(plan + off1) = 0x1E00000001uLL;
            plan[off1 + 2] = 103;
            mask_word = (*out_mask & 0xBFFFFFFDu) | 0x40000000u;
            *out_mask = mask_word;
        }
        if ((tgt_layout & 4u) != 0u)
            return true;
        if ((mask_word & 4u) == 0u)
            return true;
        const std::uint64_t rows = *row_count;
        if (rows > 0x33u)
            return false;
        *row_count = rows + 1u;
        auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 8u;
        *reinterpret_cast<std::int32_t*>(row_base) = 2;
        *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x620000001EuLL;
        *out_mask = (0x40000000u | (*out_mask & 0xBFFFFFFBu));
        return true;
    }

    std::uint32_t mask_word = *out_mask;
    if ((tgt_layout & 4u) != 0u) {
        if ((mask_word & 0x40000000u) == 0u || (tgt_layout & 0x40000000u) != 0u)
            return true;
        const std::uint64_t rows = *row_count;
        if (rows > 0x33u)
            return false;
        *row_count = rows + 1u;
        auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 8u;
        *reinterpret_cast<std::int32_t*>(row_base) = 30;
        *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6500000002uLL;
        *out_mask = (4u | (*out_mask & 0xBFFFFFFBu));
        return true;
    }

    if ((mask_word & 4u) != 0u) {
        const std::uint64_t rows = *row_count;
        if (rows > 0x33u)
            return false;
        *row_count = rows + 1u;
        const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
        *reinterpret_cast<std::uint64_t*>(plan + off) = 2u;
        plan[off + 2] = 96;
        *out_mask = (*out_mask & 0xFFFFFFFAu) | 1u;
        if (rows == 51u)
            return false;
        *row_count = rows + 2u;
        const std::size_t off1 = 2u + 3u * static_cast<std::size_t>(rows + 1u);
        *reinterpret_cast<std::uint64_t*>(plan + off1) = 0x100000002uLL;
        plan[off1 + 2] = 97;
        mask_word = (*out_mask & 0xFFFFFFF9u) | 2u;
        *out_mask = mask_word;
        if ((tgt_layout & 0x40000000u) != 0u)
            return true;
    } else if ((tgt_layout & 0x40000000u) != 0u) {
        return true;
    }

    if ((mask_word & 0x40000000u) == 0u)
        return true;
    const std::uint64_t rows = *row_count;
    if (rows > 0x33u)
        return false;
    *row_count = rows + 1u;
    const std::size_t off = 2u + 3u * static_cast<std::size_t>(rows);
    *reinterpret_cast<std::uint64_t*>(plan + off) = 30u;
    plan[off + 2] = 99;
    *out_mask = (*out_mask & 0xBFFFFFFEu) | 1u;
    if (rows == 51u)
        return false;
    *row_count = rows + 2u;
    auto* row_base = reinterpret_cast<std::uint8_t*>(plan) + 12u * rows + 20u;
    *reinterpret_cast<std::int32_t*>(row_base) = 30;
    *reinterpret_cast<std::uint64_t*>(row_base + 4u) = 0x6400000001uLL;
    *out_mask = (2u | (*out_mask & 0xBFFFFFFDu));
    return true;
}

bool auro_downmix_v1_plan_finalize_label275_partial(
    std::int32_t* plan,
    std::uint32_t tgt_layout,
    std::uint8_t tgt_class_hint) {
    return auro_downmix_v1_plan_sub_5911c0_partial(plan, tgt_layout)
        && auro_downmix_v1_plan_sub_5914e0_partial(plan, tgt_layout, tgt_class_hint)
        && auro_downmix_v1_plan_sub_5917b0_partial(plan, tgt_layout, tgt_class_hint);
}

} // namespace

void auro_downmix_v1_engine_t_construct_partial(std::uint8_t* engine_base) {
    if (!engine_base)
        return;
    std::memset(engine_base, 0, 56u);
}

bool auro_downmix_v1_engine_calculate_partial(
    std::int32_t* engine,
    std::int32_t* plan,
    std::uint32_t src_layout,
    std::uint32_t tgt_layout) {
    if (!plan)
        return false;
    std::memset(plan, 0, 0x280u);
    // IDA auro_downmix_v1_Engine_calculate @ 0x58EF80 early exits.
    if (tgt_layout == 0u || src_layout == 0u) {
        plan[158] = static_cast<std::int32_t>(src_layout);
        plan[159] = 0;
        return true;
    }
    if ((~tgt_layout & src_layout) == 0u) {
        plan[158] = static_cast<std::int32_t>(src_layout);
        plan[159] = static_cast<std::int32_t>(src_layout);
        return true;
    }
    const std::uint32_t tgt_lo = tgt_layout & 3u;
    // Height entry @ 0x58EF80: v200 || tgt&0x40000000 || tgt_lo==3.
    const std::uint8_t tgt_class_hint =
        static_cast<std::uint8_t>(static_cast<std::uint32_t>(tgt_lo != 3u) & (tgt_layout >> 2u));
    const bool height_entry =
        tgt_class_hint != 0u || (tgt_layout & 0x40000000u) != 0u || tgt_lo == 3u;
    if (height_entry) {
        std::uint32_t src_dim_layout = src_layout & 0xBFFFFFFBu | 0x40000000u;
        if ((src_layout & 4u) == 0u)
            src_dim_layout = src_layout;
        if ((~src_layout & 3u) == 0u)
            src_dim_layout = src_layout;
        std::uint32_t tgt_dim_layout = tgt_layout & 0xBFFFFFFBu | 0x40000000u;
        if ((tgt_layout & 4u) == 0u)
            tgt_dim_layout = tgt_layout;
        if (tgt_lo == 3u)
            tgt_dim_layout = tgt_layout;
        (void)auro_channel_Layout_dimension(src_dim_layout);
        (void)auro_channel_Layout_dimension(tgt_dim_layout);
        plan[158] = static_cast<std::int32_t>(src_layout);
        plan[159] = static_cast<std::int32_t>(src_layout);
        bool engine_complete = false;
        if (!auro_downmix_v1_plan_height_preprocess_partial(
                plan, engine, src_layout, tgt_layout, &engine_complete))
            return false;
        return static_cast<std::uint32_t>(plan[159]) != 0u;
    }
    return false;
}

DownmixToPartial codec_v3_downmix_to_partial(std::uint32_t layout, std::uint32_t target_layout) {
    DownmixToPartial out{};
    alignas(16) std::uint8_t engine[56]{};
    alignas(16) std::int32_t plan[kDownmixPlanIntCount]{};
    auro_downmix_v1_engine_t_construct_partial(engine);
    if (!auro_downmix_v1_engine_calculate_partial(
            reinterpret_cast<std::int32_t*>(engine),
            plan,
            layout,
            target_layout)) {
        return out;
    }
    const std::uint32_t mask = static_cast<std::uint32_t>(plan[159]);
    if (mask == 0u)
        return out;
    out.mask = mask;
    out.ok = true;
    return out;
}

void codec_v3_layout_pair_from_ctx(std::uint64_t ctx, std::uint32_t* output_layout, std::uint32_t* target_layout) {
    if (output_layout)
        *output_layout = 0u;
    if (target_layout)
        *target_layout = 0u;
    const std::uint64_t codec = codec_v3_codec_decoder_from_ctx(ctx);
    if (codec == 0u)
        return;
    const auto* cfg = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(codec));
    // Match ACV3Decoder::decide_decode_ (a1+88=input/carrier, a1+92=output):
    // fill_started uses downmix(output_layout, target_layout) as the baseline and
    // downmix(frame_flags, target_layout) for the frame — so baseline first arg is
    // Config input_mask, shared second arg is Config output_mask.
    if (output_layout) {
        *output_layout = *reinterpret_cast<const std::uint32_t*>(
            cfg + auro_codec_v3_ida::kDecoderConfig_off_input_mask);
    }
    if (target_layout) {
        *target_layout = *reinterpret_cast<const std::uint32_t*>(
            cfg + auro_codec_v3_ida::kDecoderConfig_off_output_mask);
    }
}

void codec_v3_write_decide_decode_aggregate(std::uint64_t ctx, std::uint32_t aggregate_mask) {
    if (ctx == 0u)
        return;
    auto* blob = reinterpret_cast<std::uint8_t*>(ctx);

    // sub_4d9320 / auro_a3deng_v3 wrapper: state @ +2496, notify via +2552.
    if (*reinterpret_cast<const std::uint64_t*>(blob + 2552u) != 0u) {
        std::uint32_t* state_word = reinterpret_cast<std::uint32_t*>(blob + 2496u);
        if (*state_word != aggregate_mask) {
            *state_word = aggregate_mask;
            sub_4d95e0_partial(static_cast<std::int64_t>(ctx), 1);
        }
        return;
    }

    // sub_EB420 dispatch blob: state @ +252112, last_result @ +252128.
    constexpr std::uintptr_t kEb5a0OffDecideDecodeState = 252112u;
    constexpr std::uintptr_t kEb5a0OffLastResult = 252128u;
    std::uint32_t* state_word = reinterpret_cast<std::uint32_t*>(blob + kEb5a0OffDecideDecodeState);
    if (*state_word == aggregate_mask)
        return;
    *state_word = aggregate_mask;
    *reinterpret_cast<std::uint32_t*>(blob + kEb5a0OffLastResult) = 1u;
    auto* sink_holder = reinterpret_cast<std::uint64_t*>(blob + 31521ull * sizeof(std::uint64_t));
    if (!sink_holder || *sink_holder == 0u)
        return;
    const std::uint64_t fn_ptr = *reinterpret_cast<const std::uint64_t*>(*sink_holder + 8u);
    if (fn_ptr == 0u)
        return;
    using NotifyFn = void (__fastcall *)(std::uint64_t, std::int64_t);
    reinterpret_cast<NotifyFn>(fn_ptr)(*sink_holder, 1);
}

} // namespace

std::uint32_t codec_v3_block_info_segment_count_from_decoder(const std::uint8_t* decoder_base) {
    if (!decoder_base)
        return 0u;
    const auto* og = decoder_base + auro_codec_v3_ida::kDecoder_off_OutputGenerator;
    const std::uint64_t seg_ctx = *reinterpret_cast<const std::uint64_t*>(og + kOgOff_segment_ctx_ptr);
    if (seg_ctx == 0u)
        return 0u;
    return *reinterpret_cast<const std::uint32_t*>(seg_ctx + kSegCtxOff_count);
}

void codec_v3_layout_pair_from_decoder(
    const std::uint8_t* decoder_base,
    std::uint32_t* output_layout,
    std::uint32_t* target_layout) {
    if (output_layout)
        *output_layout = 0u;
    if (target_layout)
        *target_layout = 0u;
    if (!decoder_base)
        return;
    // Same pairing as codec_v3_layout_pair_from_ctx / ACV3Decoder::decide_decode_.
    if (output_layout) {
        *output_layout = *reinterpret_cast<const std::uint32_t*>(
            decoder_base + auro_codec_v3_ida::kDecoderConfig_off_input_mask);
    }
    if (target_layout) {
        *target_layout = *reinterpret_cast<const std::uint32_t*>(
            decoder_base + auro_codec_v3_ida::kDecoderConfig_off_output_mask);
    }
}

namespace {

std::uint32_t codec_v3_decide_decode_fill_started_flags_partial(
    std::uint32_t count,
    std::uint64_t ranges_base,
    std::uint64_t started_base,
    std::uint32_t output_layout,
    std::uint32_t target_layout) {
    const DownmixToPartial output_downmix = codec_v3_downmix_to_partial(output_layout, target_layout);
    if (!output_downmix.ok) {
        for (std::uint32_t i = 0; i < count; ++i) {
            *reinterpret_cast<std::uint32_t*>(started_base + 4ull * static_cast<std::uint64_t>(i)) = 0u;
        }
        return 0u;
    }

    const std::uint32_t output_count = auro_channel_Mask_count(output_downmix.mask, 0, 0);
    std::uint32_t aggregate_mask = 0u;

    for (std::uint32_t i = 0; i < count; ++i) {
        auto* started_word = reinterpret_cast<std::uint32_t*>(
            started_base + 4ull * static_cast<std::uint64_t>(i));
        const auto* slot = reinterpret_cast<const std::uint8_t*>(
            ranges_base + static_cast<std::uintptr_t>(kSegmentStrideBytes) * static_cast<std::uintptr_t>(i));
        const std::uint32_t seg_flags =
            *reinterpret_cast<const std::uint32_t*>(slot + kSegRangeOff_flags);
        if (*started_word == 0u || seg_flags == 0u) {
            *started_word = 0u;
            continue;
        }

        const DownmixToPartial frame_downmix = codec_v3_downmix_to_partial(seg_flags, target_layout);
        if (!frame_downmix.ok) {
            *started_word = 0u;
            continue;
        }

        const std::uint32_t frame_count = auro_channel_Mask_count(frame_downmix.mask, 0, 0);
        const bool prefer_started_decode = frame_count > output_count;
        *started_word = prefer_started_decode ? 1u : 0u;
        if (prefer_started_decode)
            aggregate_mask |= seg_flags;
    }
    return aggregate_mask;
}

} // namespace

std::int64_t codec_v3_pre_segments_decide_decode_partial(
    std::uint64_t ctx,
    std::uint64_t ranges_base,
    std::uint64_t started_base) {
    if (started_base == 0u)
        return 0;
    const std::uint32_t count = codec_v3_block_info_segment_count_from_ctx(ctx);
    if (count == 0u)
        return 0;

    if (codec_v3_allow_decoding_from_ctx(ctx) == 0u) {
        for (std::uint32_t i = 0; i < count; ++i) {
            *reinterpret_cast<std::uint32_t*>(started_base + 4ull * static_cast<std::uint64_t>(i)) = 0u;
        }
        codec_v3_write_decide_decode_aggregate(ctx, 0u);
        return 0;
    }

    std::uint32_t output_layout = 0u;
    std::uint32_t target_layout = 0u;
    codec_v3_layout_pair_from_ctx(ctx, &output_layout, &target_layout);
    const std::uint32_t aggregate_mask = codec_v3_decide_decode_fill_started_flags_partial(
        count,
        ranges_base,
        started_base,
        output_layout,
        target_layout);
    codec_v3_write_decide_decode_aggregate(ctx, aggregate_mask);
    return 0;
}

std::int64_t codec_v3_pre_segments_decide_decode_decoder_partial(
    std::uint64_t decoder_base,
    std::uint64_t ranges_base,
    std::uint64_t started_base) {
    if (decoder_base == 0u || started_base == 0u)
        return 0;
    const auto* dec = reinterpret_cast<const std::uint8_t*>(decoder_base);
    const std::uint32_t count = codec_v3_block_info_segment_count_from_decoder(dec);
    if (count == 0u)
        return 0;

    if (codec_v3_allow_decoding_from_decoder(dec) == 0u) {
        for (std::uint32_t i = 0; i < count; ++i) {
            *reinterpret_cast<std::uint32_t*>(started_base + 4ull * static_cast<std::uint64_t>(i)) = 0u;
        }
        codec_v3_write_decide_decode_aggregate(decoder_base, 0u);
        return 0;
    }

    std::uint32_t output_layout = 0u;
    std::uint32_t target_layout = 0u;
    codec_v3_layout_pair_from_decoder(dec, &output_layout, &target_layout);
    const std::uint32_t aggregate_mask = codec_v3_decide_decode_fill_started_flags_partial(
        count,
        ranges_base,
        started_base,
        output_layout,
        target_layout);
    codec_v3_write_decide_decode_aggregate(
        static_cast<std::uint64_t>(decoder_base), aggregate_mask);
    return 0;
}

std::int64_t codec_v3_pre_segments_decide_decode_layouts_partial(
    std::uint32_t segment_count,
    std::uint64_t ranges_base,
    std::uint64_t started_base,
    std::uint32_t output_layout,
    std::uint32_t target_layout,
    std::uint32_t allow_decoding,
    CodecV3DispatchStateEb5a0* aggregate_dispatch) {
    if (started_base == 0u || segment_count == 0u)
        return 0;

    if (allow_decoding == 0u) {
        for (std::uint32_t i = 0; i < segment_count; ++i) {
            *reinterpret_cast<std::uint32_t*>(started_base + 4ull * static_cast<std::uint64_t>(i)) = 0u;
        }
        if (aggregate_dispatch) {
            if (aggregate_dispatch->decide_decode_state != 0u) {
                aggregate_dispatch->decide_decode_state = 0u;
                aggregate_dispatch->last_result = 1u;
                notify_codec_v3_state_change(aggregate_dispatch, 1);
            } else {
                aggregate_dispatch->last_result = 0u;
            }
        }
        return 0;
    }

    const std::uint32_t aggregate_mask = codec_v3_decide_decode_fill_started_flags_partial(
        segment_count,
        ranges_base,
        started_base,
        output_layout,
        target_layout);
    if (aggregate_dispatch) {
        if (aggregate_dispatch->decide_decode_state != aggregate_mask) {
            aggregate_dispatch->decide_decode_state = aggregate_mask;
            aggregate_dispatch->last_result = 1u;
            notify_codec_v3_state_change(aggregate_dispatch, 1);
        } else {
            aggregate_dispatch->last_result = 0u;
        }
    }
    return 0;
}

void codec_v3_decide_decode_callback_eb8a0(
    CodecV3DispatchStateEb5a0* state,
    std::uint32_t next_state,
    std::uint32_t* decisions,
    int decision_count) {
    if (!state)
        return;
    const std::uint32_t prev_decode_state = state->decide_decode_state;
    std::uint32_t current_decode_state = 0u;

    if (decision_count <= 0) {
        state->last_result = 0u;
        if (prev_decode_state == 0u)
            return;
    } else {
        bool unchanged = true;
        for (int i = 0; i != decision_count; ++i) {
            current_decode_state = 0u;
            if (decisions && decisions[i] != 0u) {
                const bool gate_enabled = state->decide_decode_gate != 0u;
                decisions[i] = gate_enabled ? 1u : 0u;
                if (gate_enabled)
                    current_decode_state = next_state;
            } else if (decisions) {
                decisions[i] = 0u;
            }
            unchanged = unchanged && (current_decode_state == prev_decode_state);
        }
        state->last_result = unchanged ? 0u : 1u;
        if (prev_decode_state == current_decode_state)
            return;
    }

    state->decide_decode_state = current_decode_state;
    notify_codec_v3_state_change(state, 1);
}

std::int64_t codec_v3_dispatch_eb5a0_partial(
    CodecV3DispatchStateEb5a0* state,
    const CodecV3IoBufferDescEb5a0* input_desc,
    const CodecV3IoBufferDescEb5a0* output_desc,
    std::uint32_t* io_status,
    CodecV3ProcessFnEb5a0 process,
    void* process_user) {
    CodecV3IoBufferDescEb5a0 in_local{};
    CodecV3IoBufferDescEb5a0 out_local{};
    const std::uint32_t total_samples = state->block_size != 0u ? state->block_size : 64u;
    in_local.total_samples = total_samples;
    in_local.sample_rate = state->sample_rate;
    in_local.bits_per_sample = 24u;
    out_local.total_samples = total_samples;
    out_local.sample_rate = state->sample_rate;
    out_local.bits_per_sample = 24u;

    for (std::size_t i = 0; i < kCodecV3ChannelCount; ++i) {
        in_local.channel_ptr[i] = input_desc->channel_ptr[i];
        out_local.channel_ptr[i] = output_desc->channel_ptr[i];
    }

    const std::int64_t result =
        process(process_user, &in_local, state->format_word0, &out_local, io_status);
    if (*io_status == 0u)
        *io_status = state->format_word0;
    state->last_result = *io_status;
    return result;
}

std::int64_t codec_v3_process_partial_eb5a0(
    void* user,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t format_word0,
    const CodecV3IoBufferDescEb5a0* output_desc,
    std::uint32_t* io_status) {
    auto* runtime = reinterpret_cast<CodecV3PartialRuntimeEb5a0*>(user);

    const std::uint32_t input_mask = format_word0 & kCodecV3ChannelMask;
    runtime->dispatch->format_word0 = input_mask;
    runtime->dispatch->sample_rate = input_desc->sample_rate;

    if (delay_line_write_buffer_106ab0(runtime->delay_line, input_desc, input_mask) != 0) {
        format_detector_process_1056c0_partial(
            runtime->format_detector,
            input_desc,
            input_mask,
            runtime->set_layout,
            runtime->process_block,
            runtime->sync_user);
        // IDA: FormatDetector owns +308; propagate to dispatch (not the reverse).
        if (runtime->dispatch)
            runtime->dispatch->sync_state = runtime->format_detector->sync_state;
        // Commit the just-written ring slot before Parser/OG.
        // DelayLine_get_buffer always returns write_slot-1 (previous slot); without an
        // early advance the first parse reads an empty slot and CRC-drops the sync frame.
        // Native Decoder_process advances after OG; host commits after write+FD so parser
        // sees the block that FormatDetector just scanned.
        (void)delay_line_advance_106b20(runtime->delay_line);
    }

    *io_status = input_mask;
    return 0;
}

std::int32_t codec_v3_decoder_process_validate_101800(
    const std::uint8_t* decoder_base,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask,
    const CodecV3IoBufferDescEb5a0* output_desc) {
    // current IDA Decoder_process:
    // 3 null input/output, 401 input total_samples mismatch, 386 sample_rate mismatch,
    // 387 bits mismatch, 388 missing input ptr, 389 missing output ptr, 384 disallowed input mask.
    if (!input_desc || !output_desc)
        return 3;
    if (!decoder_base)
        return 401;

    const std::uint64_t expected_total_samples =
        *reinterpret_cast<const std::uint64_t*>(decoder_base + 0u);
    const std::uint32_t expected_sample_rate =
        *reinterpret_cast<const std::uint32_t*>(decoder_base + 32u);
    const std::uint32_t required_output_mask =
        *reinterpret_cast<const std::uint32_t*>(decoder_base + 40u);
    const std::uint32_t allowed_input_mask =
        *reinterpret_cast<const std::uint32_t*>(decoder_base + 36u);

    if (input_desc->total_samples != expected_total_samples)
        return 401;
    if (input_desc->sample_rate != expected_sample_rate)
        return 386;
    if (input_desc->bits_per_sample != 24u)
        return 387;

    if (output_desc->total_samples != input_desc->total_samples)
        return 401;
    if (output_desc->sample_rate != input_desc->sample_rate)
        return 386;
    if (output_desc->bits_per_sample != 24u)
        return 387;

    const std::uint32_t in_mask31 = input_mask & kCodecV3ChannelMask;
    if (!codec_v3_has_required_channel_ptrs_101800(input_desc, in_mask31))
        return 388;
    if (!codec_v3_has_required_channel_ptrs_101800(output_desc, required_output_mask))
        return 389;

    if ((in_mask31 & allowed_input_mask) != in_mask31)
        return 384;
    return 0;
}

std::int64_t codec_v3_decoder_process_101800(
    std::uint8_t* decoder_base,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask,
    CodecV3IoBufferDescEb5a0* output_desc,
    std::uint32_t* io_status,
    const void* output_runtime_fns,
    CodecV3ParserProcessFn101800 parser_process) {
    std::int64_t result = 3;
    if (!input_desc)
        return result;
    if (!output_desc)
        return result;

    const std::uint64_t v9 = input_desc->total_samples;
    result = 401;
    if (!decoder_base || *reinterpret_cast<const std::uint64_t*>(decoder_base + 0u) != v9)
        return result;

    const std::uint32_t v10 = input_desc->sample_rate;
    if (v10 != *reinterpret_cast<const std::uint32_t*>(decoder_base + 32u))
        return 386;
    if (input_desc->bits_per_sample != 24u)
        return 387;
    if (static_cast<std::uint32_t>(v9) != output_desc->total_samples)
        return result;

    result = 386;
    if (output_desc->sample_rate != v10)
        return result;
    result = 387;
    if (output_desc->bits_per_sample != 24u)
        return result;

    std::uint64_t v12 = 0u;
    std::uint32_t v13 = 1u;
    if ((input_mask & 1u) != 0u) {
        if (input_desc->channel_ptr[0] == 0u)
            return 388;
    }
    while (true) {
        const std::uint32_t out_mask = *reinterpret_cast<const std::uint32_t*>(decoder_base + 40u);
        if ((v13 & out_mask) != 0u && output_desc->channel_ptr[v12] == 0u)
            return 389;
        ++v12;
        if (v12 == kCodecV3ChannelCount)
            break;
        v13 = (1u << v12);
        if (((input_mask >> v12) & 1u) != 0u) {
            if (input_desc->channel_ptr[v12] == 0u)
                return 388;
        }
    }

    result = 384;
    if ((input_mask & *reinterpret_cast<const std::uint32_t*>(decoder_base + 36u)) != input_mask)
        return result;

    auto* delay_line_base = decoder_base + auro_codec_v3_ida::kDecoder_off_Memory;
    (void)delay_line_write_buffer_106ab0(
        reinterpret_cast<DelayLineState106b40*>(delay_line_base),
        input_desc,
        input_mask);
    format_detector_process_1056c0_partial(
        &reinterpret_cast<FormatDetectorRegion1056c0*>(
            decoder_base + auro_codec_v3_ida::kDecoder_off_FormatDetector)->tail,
        input_desc,
        input_mask,
        format_detector_integrated_set_layout,
        format_detector_integrated_process_block,
        decoder_base + auro_codec_v3_ida::kDecoder_off_FormatDetector);
    (void)(parser_process ? parser_process : codec_v3_parser_process_integrated_1034e0)(
        decoder_base + auro_codec_v3_ida::kDecoder_off_Parser);

    std::uint64_t v15 = 0u;
    while (true) {
        if (((*reinterpret_cast<const std::uint32_t*>(decoder_base + 40u) >> v15) & 1u) != 0u) {
            const std::uint64_t v17 = *reinterpret_cast<const std::uint64_t*>(decoder_base + 0u);
            if (v17 != 0u) {
                auto* dst = reinterpret_cast<std::int32_t*>(output_desc->channel_ptr[v15]);
                if (dst) {
                    for (std::uint64_t i = 0; i < v17; ++i)
                        dst[i] = 0;
                }
            }
        }
        ++v15;
        if (v15 == kCodecV3ChannelCount)
            break;
    }

    OutputGeneratorRuntimeFns1024a9 fns = codec_v3_default_output_runtime_1024a9();
    const std::uint64_t og_pre_cb = *reinterpret_cast<const std::uint64_t*>(
        decoder_base + auro_codec_v3_ida::kDecoder_off_OutputGenerator + kOgOff_pre_segments_cb);
    if (og_pre_cb == 0u) {
        fns.pre_segments_callback = codec_v3_pre_segments_decide_decode_decoder_partial;
        fns.pre_segments_ctx = reinterpret_cast<std::uint64_t>(decoder_base);
    }
    if (output_runtime_fns) {
        const auto& ext = *reinterpret_cast<const OutputGeneratorRuntimeFns1024a9*>(output_runtime_fns);
        if (ext.delay_line_get_channel) fns.delay_line_get_channel = ext.delay_line_get_channel;
        if (ext.delay_line_get_buffer) fns.delay_line_get_buffer = ext.delay_line_get_buffer;
        if (ext.frame_deque_find_first_with_end_after)
            fns.frame_deque_find_first_with_end_after = ext.frame_deque_find_first_with_end_after;
        if (ext.pre_segments_callback) {
            fns.pre_segments_callback = ext.pre_segments_callback;
            fns.pre_segments_ctx = ext.pre_segments_ctx;
        }
        if (ext.metadata_update_callback) {
            fns.metadata_update_callback = ext.metadata_update_callback;
            fns.metadata_update_ctx = ext.metadata_update_ctx;
        }
        if (ext.decode_channel_segment) fns.decode_channel_segment = ext.decode_channel_segment;
        if (ext.golombrice_get_errors) fns.golombrice_get_errors = ext.golombrice_get_errors;
        if (ext.extrapolate_process) fns.extrapolate_process = ext.extrapolate_process;
        if (ext.golombrice_initialize) fns.golombrice_initialize = ext.golombrice_initialize;
        if (ext.extrapolate_initialize) fns.extrapolate_initialize = ext.extrapolate_initialize;
        if (ext.frame_mark_as_unused) fns.frame_mark_as_unused = ext.frame_mark_as_unused;
        if (ext.frame_deque_pop_front) fns.frame_deque_pop_front = ext.frame_deque_pop_front;
    }

    std::uint32_t local_status = 0u;
    std::uint32_t* status_ptr = io_status ? io_status : &local_status;
    (void)output_generator_process_1024a9_partial(
        decoder_base + auro_codec_v3_ida::kDecoder_off_OutputGenerator,
        reinterpret_cast<std::uint64_t>(output_desc),
        fns,
        status_ptr,
        input_desc,
        input_mask);
    (void)delay_line_advance_106b20(reinterpret_cast<DelayLineState106b40*>(delay_line_base));
    return 0;
}

std::int64_t auro_codec_v3_Decoder_process(
    std::uint64_t decoder_base,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask,
    CodecV3IoBufferDescEb5a0* output_desc,
    std::uint32_t* io_status) {
    return codec_v3_decoder_process_101800(
        reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(decoder_base)),
        input_desc,
        input_mask,
        output_desc,
        io_status,
        nullptr,
        nullptr);
}

// libauro3d `0x1034E0` / libauro `auro_codec_v3_ida::kLibauro_codec_Parser_process`.
std::int64_t codec_v3_parser_process_1034e0(
    std::uint8_t* parser_base,
    const CodecV3ParserRuntimeFns1034e0* runtime_fns) {
    if (!parser_base || !runtime_fns
        || !runtime_fns->delay_line_get_buffer
        || !runtime_fns->frame_deque_find_first_with_end_after
        || !runtime_fns->frame_deque_push_back
        || !runtime_fns->frame_deque_pop_front
        || !runtime_fns->frame_mark_as_unused) {
        return 0;
    }

    auto* a1 = reinterpret_cast<std::uint64_t*>(parser_base);
    std::uint64_t v27 = 0;
    std::uint64_t v31 = runtime_fns->delay_line_get_buffer(a1[2], a1[1], &v27);
    std::uint64_t result = v31;
    std::uint64_t v3 = v27;
    std::uint64_t v4 = a1[3];
    if (std::getenv("AURO3D_DEBUG_FRAMES") != nullptr) {
        static std::atomic<unsigned> debug_count{0};
        const unsigned n = debug_count.fetch_add(1);
        if (n < 16) {
            std::fprintf(
                stderr,
                "parser_process[%u] cursor=%llu block=%llu dl_buf=0x%llx dl_state=%llu dq_count=%llu\n",
                n,
                static_cast<unsigned long long>(a1[1]),
                static_cast<unsigned long long>(v4),
                static_cast<unsigned long long>(v31),
                static_cast<unsigned long long>(v27),
                static_cast<unsigned long long>(frame_deque_count_530240_partial(a1[4])));
        }
    }
    if (v4 <= v27)
        return static_cast<std::int64_t>(result);

    auto* v30 = reinterpret_cast<std::uint32_t*>(a1 + 7);
    auto* v29 = a1;

    while (true) {
        const std::uint64_t v5 = v4 - v3;
        std::uint64_t v6 = runtime_fns->frame_deque_find_first_with_end_after(a1[4], a1[1]);
        std::uint64_t v9 = 0;
        if (std::getenv("AURO3D_DEBUG_FRAMES") != nullptr) {
            static std::atomic<unsigned> debug_count{0};
            const unsigned n = debug_count.fetch_add(1);
            if (n < 24) {
                std::fprintf(
                    stderr,
                    "parser_loop[%u] cursor=%llu remaining=%llu frame=0x%llx",
                    n,
                    static_cast<unsigned long long>(a1[1]),
                    static_cast<unsigned long long>(v5),
                    static_cast<unsigned long long>(v6));
                if (v6 != 0u) {
                    std::fprintf(
                        stderr,
                        " start=%llu end=%llu flags=0x%x slots=%u\n",
                        static_cast<unsigned long long>(*reinterpret_cast<const std::uint64_t*>(v6 + 0u)),
                        static_cast<unsigned long long>(*reinterpret_cast<const std::uint64_t*>(v6 + 8u)),
                        *reinterpret_cast<const std::uint32_t*>(v6 + 24u),
                        *reinterpret_cast<const std::uint32_t*>(v6 + 44u));
                } else {
                    std::fprintf(stderr, "\n");
                }
            }
        }

        if (v6 == 0) {
            v9 = v5;
            a1[1] += v5;
            if (*reinterpret_cast<std::uint32_t*>(parser_base + 648u) != 0u) {
                *reinterpret_cast<std::uint32_t*>(parser_base + 648u) = 0u;
                if (runtime_fns->content_state_callback)
                    runtime_fns->content_state_callback(runtime_fns->content_state_ctx, 0u);
            }
        } else {
            const std::uint64_t frame_start = *reinterpret_cast<const std::uint64_t*>(v6 + 0u);
            const std::uint64_t cursor = a1[1];
            const std::uint64_t v8 = frame_start - cursor;
            const std::uint32_t frame_start_delta32 =
                static_cast<std::uint32_t>(frame_start) - static_cast<std::uint32_t>(cursor);
            if (static_cast<std::int32_t>(frame_start_delta32) > 0) {
                v9 = (v5 < v8) ? v5 : v8;
                a1[1] += v9;
                if (*reinterpret_cast<std::uint32_t*>(parser_base + 648u) != 0u) {
                    *reinterpret_cast<std::uint32_t*>(parser_base + 648u) = 0u;
                    if (runtime_fns->content_state_callback)
                        runtime_fns->content_state_callback(runtime_fns->content_state_ctx, 0u);
                }
            } else {
                v9 = *reinterpret_cast<const std::uint64_t*>(v6 + 8u) - cursor;
                if (v5 < v9)
                    v9 = v5;
                const std::uint64_t v32 = v27;
                std::uint32_t v10 = *reinterpret_cast<const std::uint32_t*>(v6 + 44u);

                // Native frames already own ParseResult objects when parsing
                // starts in the middle of a host block. Host-reconstructed
                // frames can enter here with a negative start delta and null
                // per-channel ParseResult pointers, so bind only missing slots.
                if (v10 != 0u) {
                    std::uint64_t v11 = v6 + 56u;
                    std::uint64_t v12 = reinterpret_cast<std::uint64_t>(v30);
                    std::uint64_t v13 = 0;
                    do {
                        auto* parse_result_slot = reinterpret_cast<std::uint64_t*>(v11 + 16u);
                        if (*parse_result_slot == 0u) {
                            const std::uint64_t v14 =
                                ::auro3deng::parse_result_pool_get_new_107220_partial(a1[6]);
                            *parse_result_slot = v14;
                            ::auro3deng::channel_parser_construct_104210_partial(
                                v12,
                                v14,
                                v6 + 28u,
                                v11);
                        }
                        ++v13;
                        v10 = *reinterpret_cast<const std::uint32_t*>(v6 + 44u);
                        v11 += 32u;
                        v12 += 64u;
                    } while (v13 < v10);
                }

                std::int32_t v18 = 0;
                if (v10 != 0u) {
                    const std::uint64_t v33 = 32ull * static_cast<std::uint64_t>(v10);
                    std::uint64_t v15 = 0;
                    std::uint32_t* v17 = v30;
                    std::int32_t v16 = 1;
                    std::int32_t v19 = 1;

                    while (true) {
                        if (*reinterpret_cast<const std::uint32_t*>(v6 + 52u + v15) != 0u) {
                            const std::uint32_t ch = *reinterpret_cast<const std::uint32_t*>(v6 + 48u + v15);
                            const std::uint64_t frame_channel_ptr =
                                *reinterpret_cast<const std::uint64_t*>(v6 + 72u + v15);
                            const std::uint64_t channel = delay_line_get_channel_from_buffer_106ab0(v31, ch, v32);
                            std::uint32_t mode = 0;
                            const bool ok = ::auro3deng::channel_parser_process_103840(
                                                reinterpret_cast<std::uint64_t>(v17),
                                                frame_channel_ptr,
                                                channel,
                                                static_cast<std::uint32_t>(v9),
                                                &mode)
                                != 0;
                            if (std::getenv("AURO3D_DEBUG_PARSE") != nullptr) {
                                static std::atomic<unsigned> debug_count{0};
                                const unsigned dbg = debug_count.fetch_add(1);
                                if (dbg < 96) {
                                    std::fprintf(
                                        stderr,
                                        "parse_ch[%u] ch=%u ok=%d state=%u header=%04x/%u/%u meta=%08x %08x %08x %08x "
                                        "crc=%04x q=%u flags=0x%x base=%u bw=%u mode=%u pred=%u,%u,%u cnt=%llu/%llu\n",
                                        dbg,
                                        ch,
                                        ok ? 1 : 0,
                                        v17[2],
                                        *reinterpret_cast<const std::uint16_t*>(v6 + 56u + v15),
                                        *reinterpret_cast<const std::uint32_t*>(v6 + 60u + v15),
                                        *reinterpret_cast<const std::uint32_t*>(v6 + 64u + v15),
                                        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 56u),
                                        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 60u),
                                        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 64u),
                                        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 68u),
                                        *reinterpret_cast<const std::uint16_t*>(frame_channel_ptr + 52u),
                                        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 72u),
                                        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 80u),
                                        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 40u),
                                        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 44u),
                                        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 104u),
                                        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 108u),
                                        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 112u),
                                        *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 116u),
                                        static_cast<unsigned long long>(*reinterpret_cast<const std::uint64_t*>(
                                            frame_channel_ptr + kFrameChannelOff_ctx_count_qword)),
                                        static_cast<unsigned long long>(*reinterpret_cast<const std::uint64_t*>(
                                            frame_channel_ptr + kFrameChannelOff_stream_count_qword)));
                                }
                            }
                            const std::int32_t v22 = *reinterpret_cast<const std::int32_t*>(frame_channel_ptr + 92u);
                            if (v18 == 0)
                                v18 = v22;
                            const bool v23 = (v22 == 0);
                            const bool v24 = (v22 == v18);
                            v16 = (v16 != 0 && ok && (v24 || v23)) ? 1 : 0;
                            v19 = (v24 && v19 != 0) ? 1 : 0;
                        }
                        v15 += 32u;
                        v17 += 16u;
                        if (v15 == v33)
                            break;
                    }

                    if (v16 == 0) {
                        if (std::getenv("AURO3D_DEBUG_FRAMES") != nullptr) {
                            std::fprintf(
                                stderr,
                                "parser_drop start=%u end=%u flags=0x%x slots=%u cursor=%llu len=%llu first_state=%u\n",
                                *reinterpret_cast<const std::uint32_t*>(v6 + 0u),
                                *reinterpret_cast<const std::uint32_t*>(v6 + 8u),
                                *reinterpret_cast<const std::uint32_t*>(v6 + 24u),
                                *reinterpret_cast<const std::uint32_t*>(v6 + 44u),
                                static_cast<unsigned long long>(a1[1]),
                                static_cast<unsigned long long>(v9),
                                *reinterpret_cast<const std::uint32_t*>(
                                    *reinterpret_cast<const std::uint64_t*>(v6 + 72u) + 92u));
                        }
                        runtime_fns->frame_mark_as_unused(v6);
                        result = runtime_fns->frame_deque_pop_front(a1[4]);
                        a1[1] += v9;
                        if (*reinterpret_cast<std::uint32_t*>(parser_base + 648u) != 0u) {
                            *reinterpret_cast<std::uint32_t*>(parser_base + 648u) = 0u;
                            if (runtime_fns->content_state_callback)
                                runtime_fns->content_state_callback(runtime_fns->content_state_ctx, 0u);
                        }
                        v3 = v27 + v9;
                        v27 = v3;
                        v4 = a1[3];
                        if (v4 <= v3)
                            return static_cast<std::int64_t>(result);
                        continue;
                    }

                    if (v19 != 0)
                        *reinterpret_cast<std::int32_t*>(v6 + 24u) = v18;
                } else {
                    *reinterpret_cast<std::int32_t*>(v6 + 24u) = 0;
                }

                a1[1] += v9;
                const std::uint32_t v25 = *reinterpret_cast<const std::uint32_t*>(v6 + 24u);
                if (v25 != 0u && v25 != *reinterpret_cast<std::uint32_t*>(parser_base + 648u)) {
                    *reinterpret_cast<std::uint32_t*>(parser_base + 648u) = v25;
                    if (runtime_fns->content_state_callback)
                        runtime_fns->content_state_callback(runtime_fns->content_state_ctx, v25);
                }
                if (*reinterpret_cast<const std::uint32_t*>(v6 + 8u) == static_cast<std::uint32_t>(a1[1])) {
                    result = runtime_fns->frame_deque_push_back(a1[5], v6);
                    result = runtime_fns->frame_deque_pop_front(a1[4]);
                }
            }
        }

        v3 = v27 + v9;
        v27 = v3;
        v4 = a1[3];
        if (v4 <= v3)
            return static_cast<std::int64_t>(result);
    }
}

std::int32_t codec_v3_decoder_process_validate_101800_partial(
    const CodecV3DispatchStateEb5a0* state,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask,
    const CodecV3IoBufferDescEb5a0* output_desc) {
    if (!input_desc || !output_desc)
        return 3;
    if (!state)
        return 401;
    const std::uint32_t expected_total_samples = state->block_size != 0u ? state->block_size : 64u;
    if (input_desc->total_samples != expected_total_samples)
        return 401;
    if (input_desc->sample_rate != state->sample_rate)
        return 386;
    if (input_desc->bits_per_sample != 24u)
        return 387;
    if (output_desc->total_samples != input_desc->total_samples)
        return 401;
    if (output_desc->sample_rate != input_desc->sample_rate)
        return 386;
    if (output_desc->bits_per_sample != 24u)
        return 387;
    if (!codec_v3_has_required_channel_ptrs_101800(input_desc, input_mask & kCodecV3ChannelMask))
        return 388;
    const std::uint32_t required_output_mask =
        state->required_output_mask != 0u ? state->required_output_mask : (input_mask & kCodecV3ChannelMask);
    if (!codec_v3_has_required_channel_ptrs_101800(output_desc, required_output_mask))
        return 389;
    if (((~state->format_word0 & input_mask) & kCodecV3ChannelMask) != 0u)
        return 384;
    return 0;
}

std::uint64_t delay_line_get_buffer_106b40(
    DelayLineState106b40* state,
    std::uint64_t cursor,
    std::int64_t* io_state) {
    if (io_state)
        *io_state = 0;
    if (!state || state->samples_per_block == 0 || state->ring_slot_count == 0 || state->ring_storage_base == 0)
        return 0;

    const std::uint32_t delta32 =
        static_cast<std::uint32_t>(state->absolute_cursor) - static_cast<std::uint32_t>(cursor);
    if (static_cast<std::int32_t>(delta32) <= 0)
        return 0;
    const std::uint64_t delta = delta32;

    const std::uint64_t block_distance =
        (delta - 1ull) / static_cast<std::uint64_t>(state->samples_per_block);
    const std::uint32_t needed_blocks = static_cast<std::uint32_t>(block_distance + 1ull);
    if (needed_blocks >= state->ring_slot_count)
        return 0;

    if (io_state) {
        *io_state = static_cast<std::int64_t>(
            delta - static_cast<std::uint64_t>(needed_blocks) * static_cast<std::uint64_t>(state->samples_per_block));
    }

    std::uint32_t wrap_base = 0;
    if (needed_blocks > state->write_slot_index)
        wrap_base = state->ring_slot_count;
    const std::uint32_t slot = wrap_base + state->write_slot_index - needed_blocks;
    return state->ring_storage_base + kDelayLineBufferSlotStrideBytes * slot;
}

std::uint64_t delay_line_get_buffer_u64_state_partial(
    std::uint64_t delay_line_ptr,
    std::uint64_t cursor,
    std::uint64_t* io_state_u64) {
    std::int64_t io_state = 0;
    const std::uint64_t out = delay_line_get_buffer_106b40(
        reinterpret_cast<DelayLineState106b40*>(delay_line_ptr),
        cursor,
        &io_state);
    if (io_state_u64)
        *io_state_u64 = static_cast<std::uint64_t>(io_state);
    return out;
}

std::uint64_t delay_line_get_channel_from_buffer_106ab0(
    std::uint64_t delay_line_buffer,
    std::uint32_t channel,
    std::uint64_t start) {
    if (delay_line_buffer == 0 || channel >= kCodecV3ChannelCount)
        return 0;
    const auto* slot = reinterpret_cast<const DelayLineBufferSlot106b40*>(delay_line_buffer);
    const std::uint64_t base = slot->channel_ptr[channel];
    return base ? (base + 4ull * start) : 0;
}

std::uint64_t delay_line_stream_index_5306c0_partial(
    const DelayLineState106b40* state,
    std::uint32_t stage_count) {
    if (!state)
        return 0;
    return state->absolute_cursor
        - static_cast<std::uint64_t>(state->samples_per_block)
            * static_cast<std::uint64_t>(stage_count);
}

std::uint64_t delay_line_advance_106b20(DelayLineState106b40* state) {
    if (!state || state->ring_slot_count == 0)
        return 0;
    const std::uint32_t current = state->write_slot_index;
    state->absolute_cursor += state->samples_per_block;
    const std::uint32_t next = (current + 1u) % state->ring_slot_count;
    const std::uint64_t wrapped = (current + 1u) / state->ring_slot_count;
    state->write_slot_index = next;
    return wrapped;
}

std::uint64_t delay_line_write_buffer_106ab0(
    DelayLineState106b40* state,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask) {
    if (!state || !input_desc || state->ring_storage_base == 0 || state->samples_per_block == 0)
        return 0;
    auto* slot = reinterpret_cast<DelayLineBufferSlot106b40*>(
        state->ring_storage_base + kDelayLineBufferSlotStrideBytes * state->write_slot_index);
    slot->channel_mask = input_mask;

    for (std::uint32_t bit = 0; bit < kCodecV3ChannelCount; ++bit) {
        if (((input_mask >> bit) & 1u) == 0)
            continue;
        const std::uint64_t src = input_desc->channel_ptr[bit];
        const std::uint64_t dst = slot->channel_ptr[bit];
        if (!src || !dst)
            continue;
        for (std::uint32_t i = 0; i < state->samples_per_block; ++i) {
            *reinterpret_cast<std::int32_t*>(dst + 4ull * i) =
                *reinterpret_cast<const std::int32_t*>(src + 4ull * i);
        }
    }
    return reinterpret_cast<std::uint64_t>(slot);
}

// Mirrors `auro_codec_v3_decoder_FormatDetector_process` @ 0x52D060 (libauro.so idb).
void sync_detector_t_construct_105ee0_partial(SyncDetectorState105ee0* state) {
    if (!state)
        return;
    std::memset(state, 0, sizeof(*state));
}

void sync_detector_set_callback_105ee0_partial(
    SyncDetectorState105ee0* state,
    void (*notify)(void* ctx, std::int64_t kind, std::uint64_t a, std::uint64_t b),
    void* notify_ctx) {
    if (!state)
        return;
    state->notify = notify;
    state->notify_ctx = notify_ctx;
}

const std::uint32_t* sync_detector_get_common_header_105ee0_partial(const SyncDetectorState105ee0* state) {
    if (!state || state->enabled == 0u || state->state != 1u)
        return nullptr;
    return &state->detect_counter;
}

const SyncDetectorChannelState105ee0* sync_detector_find_channel_header_105ee0_partial(
    const SyncDetectorState105ee0* state,
    std::uint32_t channel) {
    if (!state || state->enabled == 0u || state->state != 1u)
        return nullptr;
    const std::uint32_t n = std::min<std::uint32_t>(
        state->active_channel_count,
        static_cast<std::uint32_t>(std::size(state->channels)));
    for (std::uint32_t i = 0; i < n; ++i) {
        if (state->channels[i].channel == channel)
            return &state->channels[i];
    }
    return nullptr;
}

void format_detector_t_construct_1056c0_partial(
    FormatDetectorRegion1056c0* region,
    std::uint8_t* decoder_base,
    std::uint8_t* memory_base) {
    if (!region || !decoder_base || !memory_base)
        return;
    std::memset(region, 0, sizeof(*region));
    region->tail.frame_deque_ptr =
        reinterpret_cast<std::uint64_t>(memory_base + 40u);
    region->tail.blocks_per_call = *reinterpret_cast<const std::uint32_t*>(
        decoder_base + auro_codec_v3_ida::kDecoderConfig_off_block_words);
    region->tail.allow_low_9bits = *reinterpret_cast<const std::uint32_t*>(
        decoder_base + auro_codec_v3_ida::kDecoderConfig_off_extra_flags);
    const auto* delay_line = reinterpret_cast<const DelayLineState106b40*>(memory_base);
    const std::uint32_t stage0_count = *reinterpret_cast<const std::uint32_t*>(
        decoder_base + auro_codec_v3_ida::kDecoderConfig_off_stage0_count);
    region->tail.processed_samples =
        delay_line_stream_index_5306c0_partial(delay_line, stage0_count);
    sync_detector_t_construct_105ee0_partial(&region->sync);
    sync_detector_set_callback_105ee0_partial(
        &region->sync,
        format_detector_sync_callback_52ced0_partial,
        region);
}

void format_detector_sync_callback_52ced0_partial(
    void* region_raw,
    std::int64_t kind,
    std::uint64_t rel_start,
    std::uint64_t span) {
    auto* region = reinterpret_cast<FormatDetectorRegion1056c0*>(region_raw);
    if (!region)
        return;
    auto& tail = region->tail;

    if (kind == 2) {
        if (tail.frame_deque_ptr != 0u) {
            (void)frame_deque_pop_back_5303f0_partial(
                tail.frame_deque_ptr,
                frame_mark_as_unused_106cd0_default_partial);
        }
        // IDA sub_52CED0 kind==2: clear sync_state only (not expected_frame_end @+328).
        if (tail.sync_state != 0u) {
            tail.sync_state = 0;
            if (tail.sink_notify)
                tail.sink_notify(tail.sink_user, 0);
        }
        return;
    }

    if (kind != 0)
        return;

    const std::int64_t rel = static_cast<std::int64_t>(static_cast<std::int32_t>(rel_start));
    const std::uint64_t frame_start = tail.processed_samples + static_cast<std::uint64_t>(rel);
    const std::uint32_t frame_span = static_cast<std::uint32_t>(span != 0u ? span : kCodecV3FrameDequeSlotCopyBytes);
    const std::uint32_t layout_word = tail.layout;
    const std::uint32_t sub_52ced0_and =
        (tail.allow_low_9bits != 0u)
            ? static_cast<std::uint32_t>(auro_codec_v3_ida::kFormatDetector52ced0_frame_mask_and_when_allow_nonzero)
            : static_cast<std::uint32_t>(static_cast<std::int32_t>(
                auro_codec_v3_ida::kFormatDetector52ced0_frame_mask_and_when_allow_zero));
    const std::uint32_t active_mask = layout_word & sub_52ced0_and;

    // IDA: Frame_t_construct always, then push only when common_header is valid.
    std::array<std::uint8_t, kCodecV3FrameDequeSlotCopyBytes> frame{};
    frame_construct_106ba0_partial(
        reinterpret_cast<std::uint64_t>(frame.data()),
        frame_start,
        frame_span,
        layout_word,
        active_mask);

    const std::uint32_t* common_header =
        sync_detector_get_common_header_105ee0_partial(&region->sync);
    if (!common_header)
        return;
    std::memcpy(frame.data() + 28u, common_header, 16u);

    const std::uint32_t slot_count =
        *reinterpret_cast<const std::uint32_t*>(frame.data() + 44u);
    for (std::uint32_t si = 0; si < slot_count; ++si) {
        auto* slot = frame.data() + kCodecV3FrameOffSlotBase
            + static_cast<std::size_t>(si) * kCodecV3FrameSlotStride;
        if (*reinterpret_cast<const std::uint32_t*>(slot + 4u) == 0u)
            continue;
        const std::uint32_t ch = *reinterpret_cast<const std::uint32_t*>(slot + 0u);
        const auto* header = sync_detector_find_channel_header_105ee0_partial(&region->sync, ch);
        if (!header)
            return;
        // IDA: get_channel_header returns &aligned_word; qword@+0 -> slot+8, dword@+8 -> slot+16.
        *reinterpret_cast<std::uint64_t*>(slot + 8u) =
            (static_cast<std::uint64_t>(header->aligned_bit) << 32)
            | static_cast<std::uint64_t>(header->aligned_word);
        *reinterpret_cast<std::uint32_t*>(slot + 16u) = header->aligned_code;
    }

    if (tail.frame_deque_ptr == 0u)
        return;
    if (frame_deque_push_back_13d5d0_partial(
            tail.frame_deque_ptr,
            reinterpret_cast<std::uint64_t>(frame.data()),
            kCodecV3FrameDequeSlotCopyBytes) == 0)
        return;

    tail.expected_frame_end = frame_start + static_cast<std::uint64_t>(frame_span);
    if (tail.sync_state == 0u) {
        tail.sync_state = 1u;
        if (tail.sink_notify)
            tail.sink_notify(tail.sink_user, 1);
    }
}

void format_detector_process_1056c0_partial(
    FormatDetectorState1056c0* state,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask,
    void (*set_layout)(void* user, std::uint32_t layout_mask),
    void (*process_block)(void* user, const std::uint64_t* channel_ptrs_27),
    void* sync_user) {
    if (!state || !input_desc)
        return;

    if (state->layout != input_mask) {
        // IDA FormatDetector_process: clear sync_state only (not expected_frame_end @+328).
        if (state->sync_state != 0) {
            state->sync_state = 0;
            if (state->sink_notify)
                state->sink_notify(state->sink_user, 0);
        }
        state->layout = input_mask;
        const std::uint32_t v6 = (state->allow_low_9bits != 0u)
            ? static_cast<std::uint32_t>(auro_codec_v3_ida::kFormatDetector52ced0_frame_mask_and_when_allow_nonzero)
            : static_cast<std::uint32_t>(static_cast<std::int32_t>(
                auro_codec_v3_ida::kFormatDetector52ced0_frame_mask_and_when_allow_zero));
        const std::uint32_t layout_mask = (input_mask & kCodecV3ChannelMask) & v6;
        if (set_layout)
            set_layout(sync_user, layout_mask);
    }

    if (state->blocks_per_call == 0)
        return;

    const std::uint64_t sample_span = 32ull * static_cast<std::uint64_t>(state->blocks_per_call);
    for (std::uint64_t sample_off = 0; sample_off < sample_span; sample_off += 32ull) {
        std::uint64_t block_ptrs[kCodecV3ChannelCount]{};
        for (std::uint32_t ch = 0; ch < kCodecV3ChannelCount; ++ch) {
            if (((state->layout >> ch) & 1u) == 0)
                continue;
            const std::uint64_t base = input_desc->channel_ptr[ch];
            if (!base)
                continue;
            block_ptrs[ch] = base + 4ull * sample_off;
        }

        if (process_block)
            process_block(sync_user, block_ptrs);
        state->processed_samples += 32ull;
        // IDA: timeout unlock clears sync_state only; expected_frame_end stays until next push.
        if (state->sync_state != 0u) {
            const auto remaining = static_cast<std::int64_t>(
                state->expected_frame_end - state->processed_samples);
            if (remaining <= -17) {
                state->sync_state = 0;
                if (state->sink_notify)
                    state->sink_notify(state->sink_user, 0);
            }
        }
    }
}

// Mirrors `auro_codec_v3_decoder_SyncDetector_set_layout` @ 0x52C460 (libauro.so idb).
void sync_detector_set_layout_105ee0_partial(SyncDetectorState105ee0* state, std::uint32_t layout_mask) {
    if (!state || state->layout == layout_mask)
        return;

    if (state->state == 1u && state->notify)
        state->notify(state->notify_ctx, 2, 0, 0);

    state->state = 0;
    state->counter_4 = 0;
    state->accum_8 = 0;
    state->accum_c = 0;
    state->word_10 = 0;
    state->reserved_14 = 0;
    state->active_channel_count = 0;
    state->detect_counter = 0;
    state->detect_bit_110 = 0;
    state->detect_bit_114 = 0;
    state->detect_target = 0;
    for (auto& ch : state->channels)
        ch = {};

    state->layout = layout_mask;
    state->enabled = (layout_mask < 0x200u) ? 1u : 0u;
    if (!state->enabled)
        return;

    static constexpr std::uint32_t kOrder[] = {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u};
    std::uint32_t count = 0;
    for (std::uint32_t channel : kOrder) {
        if (((layout_mask >> channel) & 1u) == 0)
            continue;
        if (count >= std::size(state->channels))
            break;
        state->channels[count].channel = channel;
        ++count;
    }
    state->active_channel_count = count;
}

// Mirrors `auro_codec_v3_decoder_SyncDetector_process_block` @ 0x52C680 (libauro.so idb).
void sync_detector_process_block_106110_partial(
    SyncDetectorState105ee0* state,
    const std::uint64_t* channel_ptrs_27) {
    if (!state || !channel_ptrs_27 || state->enabled == 0)
        return;

    auto merge_sync_window16 = [](std::uint32_t old_hist, std::uint32_t new_hist, std::uint32_t sample_idx) {
        const std::uint32_t carry_bits = (sample_idx <= 15u) ? (16u - sample_idx) : 0u;
        const std::uint32_t new_bits = 16u - carry_bits;
        const std::uint32_t old_mask = (carry_bits == 0u) ? 0u : ((1u << carry_bits) - 1u);
        const std::uint32_t new_mask = (new_bits >= 32u) ? 0xFFFFFFFFu : ((1u << new_bits) - 1u);
        const std::uint32_t old_part = (old_hist & old_mask) << new_bits;
        const std::uint32_t new_part =
            (new_bits == 0u) ? 0u : ((new_hist >> (32u - sample_idx)) & new_mask);
        return old_part | new_part;
    };

    std::uint32_t any_lsb[32]{};
    std::uint32_t any_bit2[32]{};
    std::uint32_t hist_bit1[9]{};
    std::uint32_t hist_bit2[9]{};

    const std::uint32_t active_count = std::min<std::uint32_t>(
        state->active_channel_count,
        static_cast<std::uint32_t>(std::size(state->channels)));

    for (std::uint32_t i = 0; i < active_count; ++i) {
        const std::uint32_t channel = state->channels[i].channel;
        const std::uint64_t src = channel < kCodecV3ChannelCount ? channel_ptrs_27[channel] : 0;
        if (!src)
            continue;

        std::uint32_t packed_bit1 = 0;
        std::uint32_t packed_bit2 = 0;
        for (std::uint32_t j = 0; j < 32u; ++j) {
            const std::uint32_t sample =
                static_cast<std::uint32_t>(*reinterpret_cast<const std::int32_t*>(src + 4ull * j));
            const std::uint32_t sample_lsb = (sample & 1u) + 1u;
            const std::uint32_t sample_bit2 = ((sample & 4u) != 0u) ? 2u : 1u;
            any_lsb[j] |= sample_lsb;
            any_bit2[j] |= sample_bit2;
            packed_bit1 = ((sample >> 1) & 1u) + (packed_bit1 << 1);
            packed_bit2 = ((sample & 4u) >> 2) + (packed_bit2 << 1);
        }
        hist_bit1[i] = packed_bit1;
        hist_bit2[i] = packed_bit2;
    }

    for (std::uint32_t j = 0; j < 32u; ++j) {
        const std::uint32_t lsb_flags = any_lsb[j];
        const std::uint32_t bit2_flags = any_bit2[j];

        bool keep_locked = (state->state == 1u);
        if (state->state == 0u) {
            if (lsb_flags == 1u && state->counter_4 >= 16u) {
                const std::uint32_t accum_8 = state->accum_8;
                std::uint32_t detect_target = (((accum_8 >> 4) & 0xFF0u) + 0x10u);
                if (detect_target == 0x3E0u)
                    detect_target = 0x3E8u;

                const bool preamble_ok = (static_cast<std::int8_t>(accum_8 & 0xFFu) >= 0) &&
                    ((state->accum_c & 0xFF00u) == 0xFF00u) &&
                    (detect_target >= 0x100u && detect_target <= 0x400u) &&
                    ((state->accum_c & 0xB0u) == 0xB0u) &&
                    (((accum_8 >> 5) & 1u) != 0u);
                if (preamble_ok) {
                    for (std::uint32_t i = 0; i < active_count; ++i) {
                        auto& ch = state->channels[i];
                        const std::uint32_t merged_bit1 = merge_sync_window16(ch.history_bit1, hist_bit1[i], j);
                        const std::uint32_t merged_bit2 = merge_sync_window16(ch.history_bit2, hist_bit2[i], j);
                        ch.aligned_word = merged_bit1 & 0xFFFFu;
                        ch.aligned_bit = (merged_bit2 >> 6) & 1u;
                        ch.aligned_code = (merged_bit2 & 0xFu) + 10u;
                    }

                    // IDA @ 0x52C680 lock: qword@+268 = 0x100000000 → counter=0, bit_110=1;
                    // dword@+276 = (accum_8>>4)&1; dword@+280 = detect_target.
                    state->detect_counter = 0;
                    state->detect_bit_110 = 1u;
                    state->detect_bit_114 = (accum_8 >> 4u) & 1u;
                    state->detect_target = detect_target;
                    state->word_10 = 16u;
                    state->state = 1u;
                    keep_locked = true;

                    if (state->notify) {
                        const std::int64_t start_pos = static_cast<std::int64_t>(static_cast<std::int32_t>(j)) - 16ll;
                        state->notify(
                            state->notify_ctx,
                            0,
                            static_cast<std::uint64_t>(start_pos),
                            static_cast<std::uint64_t>(detect_target));
                    }
                }
            }
        } else if (state->state == 1u) {
            if (lsb_flags != 1u && (state->word_10 & 0xFu) == 0u && state->word_10 < state->detect_target) {
                if (state->notify)
                    state->notify(state->notify_ctx, 2, j, 0);
                state->state = 0;
                keep_locked = false;
            }
        }

        state->counter_4 = (lsb_flags == 2u) ? (state->counter_4 + 1u) : 0u;
        state->accum_8 = ((state->accum_8 & 0x7FFFu) << 1) | ((bit2_flags == 2u) ? 1u : 0u);
        state->accum_c = ((state->accum_c & 0x7FFFu) << 1) | ((bit2_flags != 3u) ? 1u : 0u);
        ++state->word_10;

        if (keep_locked && state->state == 1u && state->word_10 == state->detect_target) {
            state->state = 0;
            state->counter_4 = 0;
            if (state->notify)
                state->notify(state->notify_ctx, 1, static_cast<std::uint64_t>(j) + 1u, 0);
        }
    }

    for (std::uint32_t i = 0; i < active_count; ++i) {
        state->channels[i].history_bit1 = hist_bit1[i];
        state->channels[i].history_bit2 = hist_bit2[i];
    }
}

OutputGeneratorSegmentPlan output_generator_build_segment_plan_1024a9(
    std::uint8_t* output_generator_base,
    std::uint32_t timeline_cursor,
    std::uint32_t* io_channel_mask_out) {
    OutputGeneratorSegmentPlan plan{};
    if (io_channel_mask_out)
        *io_channel_mask_out = 0;

    auto* base_q = reinterpret_cast<std::uint64_t*>(output_generator_base);
    auto* seg_ctx = reinterpret_cast<std::uint8_t*>(base_q[kOgOff_segment_ctx_ptr / 8]);

    std::int64_t dl_state = 0;
    plan.delay_line_buffer = delay_line_get_buffer_106b40(
        reinterpret_cast<DelayLineState106b40*>(base_q[kOgOff_delay_line_ptr / 8]),
        timeline_cursor,
        &dl_state);
    plan.delay_line_state_offset = dl_state;
    auto* buffer = reinterpret_cast<std::uint32_t*>(plan.delay_line_buffer);
    if (!buffer)
        return plan;

    *reinterpret_cast<std::uint32_t*>(seg_ctx + kSegCtxOff_count) = 0;
    std::uint64_t consumed = 0;
    const std::uint64_t total = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_total_samples);

    std::uint64_t guard = 0;
    while (consumed < total && guard++ < kMaxSegmentBuildGuard) {
        const std::uint32_t seg_idx = *reinterpret_cast<std::uint32_t*>(seg_ctx + kSegCtxOff_count);
        const std::uint64_t seg_start = consumed;
        std::uint64_t seg_len = total - consumed;
        std::uint64_t frame_ptr = 0;
        bool frame_has_started = false;
        std::uint32_t frame_flags = 0;

        const std::uint64_t abs_pos = timeline_cursor + consumed;
        frame_ptr = frame_deque_find_first_with_end_after_13d570_partial(base_q[kOgOff_frame_deque_ptr / 8], abs_pos);

        if (frame_ptr != 0) {
            const std::uint64_t frame_start_u64 = *reinterpret_cast<std::uint64_t*>(frame_ptr + 0);
            const std::int32_t abs_pos32 = static_cast<std::int32_t>(abs_pos);
            frame_has_started = (static_cast<std::int32_t>(frame_start_u64) - abs_pos32 <= 0);
            if (!frame_has_started) {
                const std::uint64_t delta_to_start = frame_start_u64 - abs_pos;
                seg_len = safe_min_u64(seg_len, delta_to_start);
            }
            if (frame_has_started) {
                // IDA 0x1023ba: seg_len = min(remaining, frame_end - abs_pos); pop happens in process.
                const std::uint64_t frame_end_u64 = *reinterpret_cast<std::uint64_t*>(frame_ptr + 8);
                const std::uint64_t delta_to_end = frame_end_u64 - abs_pos;
                seg_len = safe_min_u64(seg_len, delta_to_end);
                frame_flags = *reinterpret_cast<const std::uint32_t*>(frame_ptr + 24);
            }
        }
        // Запись как в v10 + 24*idx: start/len/mask/flags.
        const std::uint64_t ranges_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_ranges_base);
        auto* slot = reinterpret_cast<std::uint8_t*>(ranges_base + kSegmentStrideBytes * seg_idx);
        *reinterpret_cast<std::uint64_t*>(slot + kSegRangeOff_start) = seg_start;
        *reinterpret_cast<std::uint64_t*>(slot + kSegRangeOff_len) = seg_len;
        *reinterpret_cast<std::uint32_t*>(slot + kSegRangeOff_mask) = *buffer;
        *reinterpret_cast<std::uint32_t*>(slot + kSegRangeOff_flags) = frame_flags;

        // IDA 0x102360: store frame_ptr only when timeline-started; prestart keeps nullptr.
        const std::uint64_t stored_frame_ptr = frame_has_started ? frame_ptr : 0u;
        const std::uint64_t frame_ptrs_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_frame_ptrs);
        *reinterpret_cast<std::uint64_t*>(reinterpret_cast<std::uint8_t*>(frame_ptrs_base) + 8ull * seg_idx) = stored_frame_ptr;
        const std::uint64_t started_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_frame_started);
        *reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uint8_t*>(started_base) + 4ull * seg_idx) = frame_has_started ? 1u : 0u;

        OutputGeneratorSegment item{};
        item.start = seg_start;
        item.len = seg_len;
        item.channel_mask = *buffer;
        item.frame_flags = frame_flags;
        item.frame_ptr = stored_frame_ptr;
        item.frame_has_started = frame_has_started;
        item.prefer_started_decode_path = frame_has_started;
        plan.segments.push_back(item);
        if (io_channel_mask_out)
            *io_channel_mask_out |= select_segment_output_mask_1024a9(item);

        consumed += seg_len;
        *reinterpret_cast<std::uint32_t*>(seg_ctx + kSegCtxOff_count) = seg_idx + 1;

        if (seg_len == 0)
            break;
    }

    plan.segment_count = static_cast<std::uint32_t>(plan.segments.size());
    return plan;
}

std::int64_t output_generator_apply_segments_1024a9(
    const OutputGeneratorSegmentPlan& plan,
    const OutputGeneratorApplyCallbacks& cb,
    std::uint32_t* io_channel_mask_out) {
    if (io_channel_mask_out)
        *io_channel_mask_out = 0;

    std::int64_t rc = 0;
    for (const auto& seg : plan.segments) {
        if (io_channel_mask_out)
            *io_channel_mask_out |= select_segment_output_mask_1024a9(seg);
        if (seg.len == 0) {
            rc = warmup_zero_len_channels_1024a9(seg, cb);
            if (rc != 0)
                return rc;
            continue;
        }

        for (int ch = next_set_channel_bit_1024a9(seg.channel_mask, 0);
             ch >= 0;
             ch = next_set_channel_bit_1024a9(seg.channel_mask, ch + 1)) {
            const auto channel = static_cast<std::uint32_t>(ch);
            const std::uint64_t src = cb.get_delay_line_channel(cb.user, channel, seg.start);
            const std::uint64_t out_base = cb.get_output_channel_base(cb.user, channel);
            const std::uint64_t dst = out_base + 4ull * segment_output_start_1024a9(seg, cb);

            if (seg.frame_ptr != 0 && use_started_decode_path_1024a9(seg) && cb.decode_channel_segment) {
                rc = cb.decode_channel_segment(cb.user, &seg, channel, src, dst, seg.len, nullptr);
                if (rc != 0)
                    return rc;
            } else {
                copy_i32_samples(dst, src, seg.len);
            }
        }
    }
    return rc;
}

std::int64_t output_generator_prepare_frame_channels_1024a9(
    const OutputGeneratorSegment& seg,
    std::uint64_t timeline_cursor,
    const OutputGeneratorFrameInitCallbacks& cb) {
    if (seg.frame_ptr == 0 || !should_prepare_frame_init_1024a9(seg.frame_ptr, timeline_cursor))
        return 0;

    const std::uint32_t channel_count = cb.get_frame_channel_count(cb.user, seg.frame_ptr);
    for (std::uint32_t i = 0; i < channel_count; ++i) {
        const std::uint64_t frame_ch = cb.get_frame_channel_ptr(cb.user, seg.frame_ptr, i);
        const std::uint64_t gr_state = cb.get_golombrice_state_ptr(cb.user, i);
        const std::uint64_t ex_state = cb.get_extrapolate_state_ptr(cb.user, i);
        std::uint32_t* words_ptr = cb.get_channel_words_ptr(cb.user, frame_ch);
        const std::uint64_t ctx_ptr = cb.get_channel_ctx_ptr(cb.user, frame_ch);

        // IDA 0x10247B..0x1024CB: both init calls are fire-and-forget, return values are ignored.
        (void)cb.golombrice_initialize(cb.user, gr_state, words_ptr, ctx_ptr);
        (void)cb.extrapolate_initialize(cb.user, ex_state, frame_ch);
    }
    return 0;
}

std::int64_t output_generator_process_segments_1024a9(
    const OutputGeneratorSegmentPlan& plan,
    std::uint64_t timeline_cursor_at_entry,
    const OutputGeneratorFrameInitCallbacks* frame_init_cb,
    const OutputGeneratorApplyCallbacks& apply_cb,
    std::uint32_t* io_channel_mask_out) {
    if (io_channel_mask_out)
        *io_channel_mask_out = 0;

    std::uint64_t timeline_cursor = timeline_cursor_at_entry;
    for (const auto& seg : plan.segments) {
        if (frame_init_cb && seg.frame_ptr != 0
            && should_prepare_frame_init_1024a9(seg.frame_ptr, timeline_cursor)) {
            const std::int64_t rc_init =
                output_generator_prepare_frame_channels_1024a9(seg, timeline_cursor, *frame_init_cb);
            if (rc_init != 0)
                return rc_init;
        }
        if (io_channel_mask_out)
            *io_channel_mask_out |= select_segment_output_mask_1024a9(seg);
        if (seg.len == 0) {
            const std::int64_t rc_warmup = warmup_zero_len_channels_1024a9(seg, apply_cb);
            if (rc_warmup != 0)
                return rc_warmup;
            continue;
        }

        for (int ch = next_set_channel_bit_1024a9(seg.channel_mask, 0);
             ch >= 0;
             ch = next_set_channel_bit_1024a9(seg.channel_mask, ch + 1)) {
            const auto channel = static_cast<std::uint32_t>(ch);
            const std::uint64_t src = apply_cb.get_delay_line_channel(apply_cb.user, channel, seg.start);
            const std::uint64_t out_base = apply_cb.get_output_channel_base(apply_cb.user, channel);
            const std::uint64_t dst = out_base + 4ull * segment_output_start_1024a9(seg, apply_cb);

            if (seg.frame_ptr != 0 && use_started_decode_path_1024a9(seg) && apply_cb.decode_channel_segment) {
                const std::int64_t rc_dec =
                    apply_cb.decode_channel_segment(apply_cb.user, &seg, channel, src, dst, seg.len, nullptr);
                if (rc_dec != 0)
                    return rc_dec;
            } else {
                copy_i32_samples(dst, src, seg.len);
            }
        }
        timeline_cursor += seg.len;
    }
    return 0;
}

std::int64_t output_generator_process_segments_raw_1024a9(
    std::uint8_t* output_generator_base,
    const OutputGeneratorSegmentPlan& plan,
    std::uint64_t output_channels_table_base,
    const OutputGeneratorRuntimeFns1024a9& fns,
    std::uint32_t* io_channel_mask_out,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask) {
    OgRawRuntimeCtx ctx{};
    ctx.og = output_generator_base;
    ctx.out_tbl = output_channels_table_base;
    ctx.delay_line_buffer = plan.delay_line_buffer;
    ctx.delay_line_state_offset = plan.delay_line_state_offset;
    ctx.delay_line_ptr = output_generator_base
        ? *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_delay_line_ptr)
        : 0u;
    ctx.total_samples = output_generator_base
        ? *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_total_samples)
        : 0u;
    ctx.input_desc = input_desc;
    ctx.input_mask = input_mask & kCodecV3ChannelMask;
    ctx.fns = fns;

    OutputGeneratorFrameInitCallbacks ficb{};
    ficb.user = &ctx;
    ficb.get_frame_channel_count = raw_get_frame_channel_count;
    ficb.get_frame_channel_ptr = raw_get_frame_channel_ptr;
    ficb.get_golombrice_state_ptr = raw_get_gr_state_ptr;
    ficb.get_extrapolate_state_ptr = raw_get_ex_state_ptr;
    ficb.get_channel_words_ptr = raw_get_channel_words_ptr;
    ficb.get_channel_ctx_ptr = raw_get_channel_ctx_ptr;
    ficb.golombrice_initialize = raw_gr_initialize;
    ficb.extrapolate_initialize = raw_ex_initialize;

    OutputGeneratorApplyCallbacks acb{};
    acb.user = &ctx;
    acb.copy_input_enabled =
        output_generator_base
        && *reinterpret_cast<const std::uint32_t*>(output_generator_base + 1232u) != 0u
        && input_desc != nullptr;
    acb.input_mask = ctx.input_mask;
    acb.output_block_start = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_timeline_cursor);
    acb.get_delay_line_channel = raw_get_delay_line_channel;
    acb.get_input_channel_base = raw_get_input_channel_base;
    acb.get_output_channel_base = raw_get_output_channel_base;
    acb.decode_channel_segment = raw_decode_channel_segment;

    if (io_channel_mask_out)
        *io_channel_mask_out = 0;

    std::uint64_t timeline_cursor = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_timeline_cursor);
    const std::uint64_t frame_deque_ptr = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_frame_deque_ptr);
    for (const auto& seg : plan.segments) {
        if (should_apply_frame_metadata_1024a9(seg.frame_ptr, timeline_cursor)) {
            if (should_prepare_frame_init_1024a9(seg.frame_ptr, timeline_cursor)) {
                const std::int64_t rc_init =
                    output_generator_prepare_frame_channels_1024a9(seg, timeline_cursor, ficb);
                if (rc_init != 0)
                    return rc_init;
            }
            output_generator_apply_frame_metadata_1024a9(output_generator_base, seg, timeline_cursor, ficb);
            output_generator_dispatch_metadata_update_1024a9(output_generator_base, fns);
        }

        if (seg.len == 0u) {
            if (!(seg.frame_ptr != 0 && use_started_decode_path_1024a9(seg))) {
                const std::int64_t rc_warmup = warmup_zero_len_channels_1024a9(seg, acb);
                if (rc_warmup != 0)
                    return rc_warmup;
            } else {
                (void)process_segment_started_path_1024a9(
                    output_generator_base, output_channels_table_base, seg, acb, nullptr);
            }
            if (io_channel_mask_out) {
                const std::uint32_t external_mask =
                    (seg.frame_ptr != 0 && use_started_decode_path_1024a9(seg))
                        ? frame_output_mask_1024a9(seg)
                        : seg.channel_mask;
                *io_channel_mask_out |= external_mask;
            }
            *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_timeline_cursor) = timeline_cursor;
            if (is_frame_finished_at_cursor_1024a9(seg.frame_ptr, timeline_cursor)) {
                if (seg.frame_ptr != 0 && fns.frame_mark_as_unused)
                    fns.frame_mark_as_unused(seg.frame_ptr);
                if (fns.frame_deque_pop_front)
                    fns.frame_deque_pop_front(frame_deque_ptr);
            }
            continue;
        }

        bool segment_used_copy_input = false;
        std::uint32_t segment_produced_mask = 0u;
        if (seg.frame_ptr != 0 && use_started_decode_path_1024a9(seg)) {
            const std::int64_t rc_started = process_segment_started_path_1024a9(
                output_generator_base, output_channels_table_base, seg, acb, &segment_produced_mask);
            if (rc_started != 0)
                return rc_started;
        } else if (seg.frame_ptr != 0) {
            const std::int64_t rc_prestart = process_segment_prestart_frame_path_1024a9(
                output_generator_base, seg, acb, &segment_produced_mask);
            if (rc_prestart != 0)
                return rc_prestart;
            segment_used_copy_input = acb.copy_input_enabled;
        } else {
            const std::int64_t rc_copy =
                process_segment_copy_only_path_1024a9(seg, acb, &segment_produced_mask);
            if (rc_copy != 0)
                return rc_copy;
            segment_used_copy_input = acb.copy_input_enabled;
        }
        // IDA og+309 (v107): 0 после LABEL_104 copy-input, 1 после decode/delay.
        *reinterpret_cast<std::uint32_t*>(output_generator_base + kOgOff_output_status_flag) =
            segment_used_copy_input ? 0u : 1u;

        const bool started_decode = seg.frame_ptr != 0 && use_started_decode_path_1024a9(seg);
        // Old libauro3d LABEL_85: started decode jumps directly to LABEL_129;
        // zero-missing only runs on the delay-line/copy path.
        if (!started_decode) {
            zero_unproduced_output_channels_1024a9(
                seg,
                acb,
                segment_produced_mask,
                kCodecV3ChannelMask);
        }

        // IDA LABEL_102..153: started (v131) -> *a3 |= frame+24, без zero-missing.
        // non-started -> *a3 |= *buffer (seg.channel_mask) после copy/warmup.
        if (io_channel_mask_out) {
            const std::uint32_t external_mask =
                started_decode
                    ? frame_output_mask_1024a9(seg)
                    : seg.channel_mask;
            *io_channel_mask_out |= external_mask;
            if (std::getenv("AURO3D_DEBUG_FRAMES") != nullptr) {
                static std::atomic<unsigned> debug_count{0};
                const unsigned n = debug_count.fetch_add(1);
                if (n < 32) {
                    std::fprintf(
                        stderr,
                        "og_segment[%u] start=%llu len=%llu chmask=0x%x flags=0x%x started=%u produced=0x%x external=0x%x out=0x%x\n",
                        n,
                        static_cast<unsigned long long>(seg.start),
                        static_cast<unsigned long long>(seg.len),
                        seg.channel_mask,
                        seg.frame_flags,
                        started_decode ? 1u : 0u,
                        segment_produced_mask,
                        external_mask,
                        *io_channel_mask_out);
                }
            }
        }
        timeline_cursor += seg.len;
        *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_timeline_cursor) = timeline_cursor;

        if (is_frame_finished_at_cursor_1024a9(seg.frame_ptr, timeline_cursor)) {
            if (seg.frame_ptr != 0 && fns.frame_mark_as_unused)
                fns.frame_mark_as_unused(seg.frame_ptr);
            if (fns.frame_deque_pop_front)
                fns.frame_deque_pop_front(frame_deque_ptr);
        }
    }

    return 0;
}

OutputGeneratorSegmentPlan output_generator_build_segment_plan_runtime_1024a9(
    std::uint8_t* output_generator_base,
    const OutputGeneratorRuntimeFns1024a9& fns,
    std::uint32_t* io_channel_mask_out) {
    OutputGeneratorSegmentPlan plan{};
    if (io_channel_mask_out)
        *io_channel_mask_out = 0;
    if (!output_generator_base || !fns.delay_line_get_buffer || !fns.frame_deque_find_first_with_end_after)
        return plan;

    auto* seg_ctx = reinterpret_cast<std::uint8_t*>(*reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_segment_ctx_ptr));
    if (!seg_ctx)
        return plan;

    std::int64_t dl_state = 0;
    const std::uint64_t delay_line_ptr = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_delay_line_ptr);
    const std::uint64_t timeline_cursor = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_timeline_cursor);
    auto* buffer = reinterpret_cast<std::uint32_t*>(fns.delay_line_get_buffer(delay_line_ptr, timeline_cursor, &dl_state));
    if (!buffer)
        return plan;
    plan.delay_line_buffer = reinterpret_cast<std::uint64_t>(buffer);
    plan.delay_line_state_offset = dl_state;

    *reinterpret_cast<std::uint32_t*>(seg_ctx + kSegCtxOff_count) = 0;
    std::uint64_t consumed = 0;
    const std::uint64_t total = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_total_samples);
    const std::uint64_t frame_deque_ptr = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_frame_deque_ptr);

    std::uint64_t guard = 0;
    while (consumed < total && guard++ < kMaxSegmentBuildGuard) {
        const std::uint32_t seg_idx = *reinterpret_cast<std::uint32_t*>(seg_ctx + kSegCtxOff_count);
        const std::uint64_t seg_start = consumed;
        std::uint64_t seg_len = total - consumed;
        std::uint64_t frame_ptr = 0;
        bool frame_has_started = false;
        std::uint32_t frame_flags = 0;

        const std::uint64_t abs_pos = timeline_cursor + consumed;
        frame_ptr = fns.frame_deque_find_first_with_end_after(frame_deque_ptr, abs_pos);

        if (frame_ptr != 0) {
            const std::uint64_t frame_start_u64 = *reinterpret_cast<std::uint64_t*>(frame_ptr + 0);
            const std::int32_t abs_pos32 = static_cast<std::int32_t>(abs_pos);
            frame_has_started = (static_cast<std::int32_t>(frame_start_u64) - abs_pos32 <= 0);
            if (!frame_has_started) {
                const std::uint64_t delta_to_start = frame_start_u64 - abs_pos;
                seg_len = safe_min_u64(seg_len, delta_to_start);
            }
            if (frame_has_started) {
                // IDA 0x1023ba: seg_len = min(remaining, frame_end - abs_pos); pop happens in process.
                const std::uint64_t frame_end_u64 = *reinterpret_cast<std::uint64_t*>(frame_ptr + 8);
                const std::uint64_t delta_to_end = frame_end_u64 - abs_pos;
                seg_len = safe_min_u64(seg_len, delta_to_end);
                frame_flags = *reinterpret_cast<const std::uint32_t*>(frame_ptr + 24);
            }
        }
        const std::uint64_t ranges_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_ranges_base);
        auto* slot = reinterpret_cast<std::uint8_t*>(ranges_base + kSegmentStrideBytes * seg_idx);
        *reinterpret_cast<std::uint64_t*>(slot + kSegRangeOff_start) = seg_start;
        *reinterpret_cast<std::uint64_t*>(slot + kSegRangeOff_len) = seg_len;
        *reinterpret_cast<std::uint32_t*>(slot + kSegRangeOff_mask) = *buffer;
        *reinterpret_cast<std::uint32_t*>(slot + kSegRangeOff_flags) = frame_flags;

        // IDA 0x102360: store frame_ptr only when timeline-started; prestart keeps nullptr.
        const std::uint64_t stored_frame_ptr = frame_has_started ? frame_ptr : 0u;
        const std::uint64_t frame_ptrs_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_frame_ptrs);
        *reinterpret_cast<std::uint64_t*>(reinterpret_cast<std::uint8_t*>(frame_ptrs_base) + 8ull * seg_idx) = stored_frame_ptr;
        const std::uint64_t started_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_frame_started);
        *reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uint8_t*>(started_base) + 4ull * seg_idx) = frame_has_started ? 1u : 0u;

        OutputGeneratorSegment item{
            seg_start,
            seg_len,
            *buffer,
            frame_flags,
            stored_frame_ptr,
            frame_has_started,
            frame_has_started};
        plan.segments.push_back(item);
        if (io_channel_mask_out)
            *io_channel_mask_out |= select_segment_output_mask_1024a9(item);
        consumed += seg_len;
        *reinterpret_cast<std::uint32_t*>(seg_ctx + kSegCtxOff_count) = seg_idx + 1;

        if (seg_len == 0)
            break;
    }

    plan.segment_count = static_cast<std::uint32_t>(plan.segments.size());
    return plan;
}

void output_generator_refresh_segment_plan_from_ctx_1024a9(
    OutputGeneratorSegmentPlan& plan,
    std::uint8_t* seg_ctx) {
    if (!seg_ctx)
        return;
    const std::uint64_t ranges_base = *reinterpret_cast<const std::uint64_t*>(seg_ctx + kSegCtxOff_ranges_base);
    if (ranges_base == 0u)
        return;
    const std::uint64_t frame_ptrs_base = *reinterpret_cast<const std::uint64_t*>(seg_ctx + kSegCtxOff_frame_ptrs);
    const std::uint64_t started_base = *reinterpret_cast<const std::uint64_t*>(seg_ctx + kSegCtxOff_frame_started);
    std::uint32_t count = *reinterpret_cast<const std::uint32_t*>(seg_ctx + kSegCtxOff_count);
    if (count > plan.segments.size())
        count = static_cast<std::uint32_t>(plan.segments.size());
    plan.segments.resize(count);
    plan.segment_count = count;
    for (std::uint32_t i = 0; i < count; ++i) {
        auto* slot = reinterpret_cast<const std::uint8_t*>(ranges_base + kSegmentStrideBytes * i);
        OutputGeneratorSegment& seg = plan.segments[i];
        seg.start = *reinterpret_cast<const std::uint64_t*>(slot + kSegRangeOff_start);
        seg.len = *reinterpret_cast<const std::uint64_t*>(slot + kSegRangeOff_len);
        seg.channel_mask = *reinterpret_cast<const std::uint32_t*>(slot + kSegRangeOff_mask);
        seg.frame_flags = *reinterpret_cast<const std::uint32_t*>(slot + kSegRangeOff_flags);
        if (frame_ptrs_base != 0u) {
            seg.frame_ptr = *reinterpret_cast<const std::uint64_t*>(
                reinterpret_cast<const std::uint8_t*>(frame_ptrs_base) + 8ull * i);
        }
        if (started_base != 0u) {
            seg.prefer_started_decode_path =
                *reinterpret_cast<const std::uint32_t*>(
                    reinterpret_cast<const std::uint8_t*>(started_base) + 4ull * i) != 0u;
        }
    }
}

std::int64_t output_generator_process_1024a9(
    std::uint8_t* output_generator_base,
    std::uint64_t output_channels_table_base,
    const OutputGeneratorRuntimeFns1024a9& fns,
    std::uint32_t* external_mask_inout,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask) {
    if (external_mask_inout)
        *external_mask_inout = 0;
    std::int64_t result = 0;

    OutputGeneratorSegmentPlan plan =
        output_generator_build_segment_plan_runtime_1024a9(output_generator_base, fns, nullptr);
    auto* seg_ctx = output_generator_base
        ? reinterpret_cast<std::uint8_t*>(*reinterpret_cast<std::uint64_t*>(
            output_generator_base + kOgOff_segment_ctx_ptr))
        : nullptr;
    if (!output_generator_base)
        return result;
    if (fns.pre_segments_callback) {
        if (seg_ctx) {
            const std::uint64_t ranges_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_ranges_base);
            const std::uint64_t started_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_frame_started);
            result = fns.pre_segments_callback(fns.pre_segments_ctx, ranges_base, started_base);
        }
    } else if (*reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_pre_segments_cb) != 0) {
        if (seg_ctx) {
            const std::uint64_t ranges_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_ranges_base);
            const std::uint64_t started_base = *reinterpret_cast<std::uint64_t*>(seg_ctx + kSegCtxOff_frame_started);
            const auto cb = reinterpret_cast<PreSegmentsFn>(*reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_pre_segments_cb));
            const std::uint64_t ctx = *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_pre_segments_ctx);
            result = cb(ctx, ranges_base, started_base);
        }
    }
    if (result != 0)
        return result;
    output_generator_refresh_segment_plan_from_ctx_1024a9(plan, seg_ctx);
    if (plan.segment_count == 0)
        return result;
    return output_generator_process_segments_raw_1024a9(
        output_generator_base,
        plan,
        output_channels_table_base,
        fns,
        external_mask_inout,
        input_desc,
        input_mask);
}

std::int64_t output_generator_process_1024a9_partial(
    std::uint8_t* output_generator_base,
    std::uint64_t output_channels_table_base,
    const OutputGeneratorRuntimeFns1024a9& fns,
    std::uint32_t* external_mask_inout,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask) {
    return output_generator_process_1024a9(
        output_generator_base,
        output_channels_table_base,
        fns,
        external_mask_inout,
        input_desc,
        input_mask);
}

std::int64_t decoder_run_output_generator_1024a9_partial(
    const DecoderOutputGeneratorRunContext1024a9* ctx) {
    std::memset(ctx->output_channels_table, 0, ctx->output_channels_table_size);
    for (std::uint32_t ch = 0; ch < kCodecV3ChannelCount; ++ch) {
        *reinterpret_cast<std::uint64_t*>(ctx->output_channels_table + 16ull + 8ull * ch) =
            ctx->output_channel_ptrs_27[ch];
    }
    CodecV3IoBufferDescEb5a0 input_desc{};
    input_desc.total_samples = ctx->total_samples;
    for (std::uint32_t ch = 0; ch < kCodecV3ChannelCount; ++ch) {
        input_desc.channel_ptr[ch] = ctx->input_channel_ptrs_27
            ? ctx->input_channel_ptrs_27[ch]
            : 0u;
    }

    *reinterpret_cast<std::uint64_t*>(ctx->output_generator_base + kOgOff_delay_line_ptr) = ctx->delay_line_ptr;
    *reinterpret_cast<std::uint64_t*>(ctx->output_generator_base + kOgOff_total_samples) = ctx->total_samples;
    *reinterpret_cast<std::uint64_t*>(ctx->output_generator_base + kOgOff_frame_deque_ptr) = ctx->frame_deque_ptr;

    std::uint32_t output_mask = 0;
    const std::int64_t rc = output_generator_process_1024a9_partial(
        ctx->output_generator_base,
        reinterpret_cast<std::uint64_t>(ctx->output_channels_table),
        ctx->runtime_fns,
        &output_mask,
        ctx->input_channel_ptrs_27 ? &input_desc : nullptr,
        ctx->input_mask);
    if (ctx->produced_output_mask)
        *ctx->produced_output_mask = output_mask;
    return rc;
}

void output_generator_construct_52af20_partial(
    std::uint8_t* output_generator_base,
    const std::uint8_t* output_config,
    std::uint64_t memory_base) {
    if (!output_generator_base || !output_config)
        return;
    *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_delay_line_ptr) = memory_base;
    *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_frame_deque_ptr) = memory_base + 72u;
    *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_segment_ctx_ptr) = memory_base + 104u;
    std::memcpy(output_generator_base + kOgOff_errors_buf, reinterpret_cast<const void*>(memory_base + 152u), 16u);
    *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_total_samples) =
        *reinterpret_cast<const std::uint64_t*>(output_config + 0u);
    *reinterpret_cast<std::uint32_t*>(output_generator_base + 832u) =
        *reinterpret_cast<const std::uint32_t*>(output_config + 8u);
    *reinterpret_cast<std::uint32_t*>(output_generator_base + 836u) =
        *reinterpret_cast<const std::uint32_t*>(output_config + 28u);
    const auto* delay_line = reinterpret_cast<const DelayLineState106b40*>(memory_base);
    const std::uint32_t latency_blocks =
        *reinterpret_cast<const std::uint32_t*>(output_config + 28u);
    *reinterpret_cast<std::uint64_t*>(output_generator_base + kOgOff_timeline_cursor) =
        delay_line_stream_index_5306c0_partial(delay_line, latency_blocks);
    std::memset(output_generator_base + 888u, 0, 0x158u);
    std::memset(output_generator_base + kOgOff_pre_segments_cb, 0, 32u);
    const std::uint32_t copy_input_flag = *reinterpret_cast<const std::uint32_t*>(output_config + 56u);
    *reinterpret_cast<std::uint32_t*>(output_generator_base + 1232u) = copy_input_flag;
    *reinterpret_cast<std::uint32_t*>(output_generator_base + kOgOff_output_status_flag) =
        copy_input_flag == 0u ? 1u : 0u;
}


} // namespace auro3deng
