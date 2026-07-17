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

void channel_parse_result_mark_usage(std::uint64_t parse_result_ptr, std::uint32_t in_use) {
    // IDA ParseResult_mark_usage @ 0x52F2D0: store usage dword at +3664.
    *reinterpret_cast<std::uint32_t*>(parse_result_ptr + kParseResultOff_usage_dword) = in_use;
}

void channel_parse_result_prepare_new(std::uint64_t parse_result_ptr) {
    // current IDA ParseResultPool_get_new pre-increments cursor, constructs the slot, then marks it in use.
    // IDA 0x107220 tail: construct(parse_result); mark_usage(parse_result, 1).
    channel_parse_result_construct(parse_result_ptr);
    channel_parse_result_mark_usage(parse_result_ptr, 1u);
}

std::uint64_t parse_result_pool_required_additional_memory(std::uint32_t pool_count) {
    // current IDA ParseResultPool_t_required_additional_memory: align=8, size=3672 * count.
    return static_cast<std::uint64_t>(kParseResultStrideBytes) * static_cast<std::uint64_t>(pool_count);
}

std::uint32_t memory_parse_result_pool_count(
    std::uint32_t block_samples,
    std::uint32_t channel_count) {
    // IDA 0x106D20: ParseResultPool_t_required_additional_memory(..., 9 * ((a2[0]*a2[7]+255)>>8) + 9)
    return 9u * (((block_samples * channel_count + 255u) >> 8) + 1u);
}

std::uint64_t memory_block_info_required_additional_memory(std::uint32_t n) {
    return 24ull * static_cast<std::uint64_t>(n) + 4ull * static_cast<std::uint64_t>(n)
        + 8ull * static_cast<std::uint64_t>(n);
}

std::uint32_t memory_block_info_slot_count(std::uint32_t block_samples) {
    return ((block_samples + 255u) >> 7) | 1u;
}

std::uint64_t memory_accumulator_tail_payload(std::uint64_t qword_at_a2) {
    const std::uint64_t a = 8ull * (qword_at_a2 & 0x7FFFFFFFULL);
    const std::uint64_t b = (12ull * qword_at_a2) & 0x3FFFFFFFCLL;
    return a + b;
}

std::uint64_t memory_frame_deque_required_additional_memory(std::uint32_t deque_capacity) {
    return 336ull * static_cast<std::uint64_t>(deque_capacity);
}

std::uint64_t memory_delay_line_payload_sum(
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

void channel_bit_reader_set_data(std::uint64_t br, std::uint64_t words_ptr, std::int32_t word_count) {
    *reinterpret_cast<std::uint64_t*>(br + 0u) = words_ptr;
    *reinterpret_cast<std::int32_t*>(br + 8u) = word_count;
    *reinterpret_cast<std::uint32_t*>(br + 12u) = 0u;
}

void channel_bit_reader_set_bitlines(std::uint64_t br, std::int32_t bitlines) {
    *reinterpret_cast<std::int32_t*>(br + 36u) = bitlines;
    *reinterpret_cast<std::int32_t*>(br + 32u) = bitlines;
}

std::uint32_t channel_bit_reader_get_unsigned_bits(std::uint64_t a1, std::int32_t a2) {
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
    bool exit = false;
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
            exit = true;
            break;
        }
    }
    if (exit) {
        apply_label16(result, v3);
        return result;
    }
    if (v3 == 0)
        apply_label17();
    else
        apply_label16(result, v3);
    return result;
}

std::uint32_t channel_bit_reader_get_remaining_nr_bits(std::uint64_t br) {
    return *reinterpret_cast<std::uint32_t*>(br + 20u);
}

std::int64_t channel_bit_reader_get_signed_bits(std::uint64_t br, std::int32_t bit_count) {
    if (bit_count <= 0)
        return 0;
    const std::int64_t result =
        static_cast<std::int64_t>(channel_bit_reader_get_unsigned_bits(br, bit_count));
    const std::uint32_t v3 = (bit_count == 32) ? 0x80000000u : (1u << static_cast<std::uint32_t>(bit_count - 1));
    if (bit_count != 32 && (v3 & static_cast<std::uint32_t>(result)) != 0u)
        return -static_cast<std::int64_t>((~v3) & static_cast<std::uint32_t>(result));
    return result;
}

void channel_bit_reader_reset(std::uint64_t br) {
    std::memset(reinterpret_cast<void*>(br + 16u), 0, 16u);
    std::memset(reinterpret_cast<void*>(br), 0, 16u);
    *reinterpret_cast<std::uint64_t*>(br + 32u) = 0u;
}

void channel_crc_reset(std::uint64_t crc_state) {
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

void channel_crc_table_init() {
    crc_table_ensure_once();
}

std::uint64_t channel_crc_process(
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

std::int64_t channel_crc_check(
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

std::int64_t channel_metadata_combine_info(std::uint64_t frame_channel_ptr) {
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

std::int64_t /* Decompiled name: channel_get_carrier_layout */
channel_get_carrier_layout(std::uint32_t* out_layout, std::uint32_t original_layout) {
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

std::int64_t channel_parser_construct(
    std::uint64_t parser_base,
    std::uint64_t frame_channel_ptr,
    std::uint64_t frame_meta_16b_ptr,
    std::uint64_t frame_slot_ptr) {
    std::memset(reinterpret_cast<void*>(parser_base), 0, 64u);
    channel_bit_reader_reset(parser_base + 24u);
    channel_crc_reset(parser_base + 12u);
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
        channel_bit_reader_set_bitlines(
            br,
            static_cast<std::int32_t>(
                24u - *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 72u)));
        a1[2] = 2u;
    }

    channel_bit_reader_set_data(br, channel_ptr, static_cast<std::int32_t>(word_count));
    // a1[10] == BitReader+16 (words-consumed counter). IDA does not overwrite it with a4.
    if (a1[2] == 2u)
        a1[2] = 3u;

    if (word_count != 0u) {
        while (true) {
            const std::uint32_t state = a1[2];
            switch (state) {
            case 3u: {
                const std::uint16_t u = static_cast<std::uint16_t>(
                    channel_bit_reader_get_unsigned_bits(br, 16));
                if (*br_ok == 0u)
                    break;
                *reinterpret_cast<std::uint16_t*>(frame_channel_ptr + 76u) = u;
                a1[0] = 0u;
                a1[1] = 3u;
                a1[2] = 4u;
                continue;
            }
            case 4u: {
                const std::uint32_t v = channel_bit_reader_get_unsigned_bits(br, 32);
                if (*br_ok == 0u)
                    break;
                *reinterpret_cast<std::uint32_t*>(frame_channel_ptr + 80u + 4u * static_cast<std::uint64_t>(a1[0])) = v;
                a1[0] += 1u;
                if (a1[0] >= a1[1])
                    a1[2] = 5u;
                continue;
            }
            case 5u: {
                if (channel_metadata_combine_info(frame_channel_ptr) == 0)
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
                        channel_bit_reader_get_signed_bits(br, 32));
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
                const std::uint32_t v = channel_bit_reader_get_unsigned_bits(br, 8);
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
                const std::uint32_t op = channel_bit_reader_get_unsigned_bits(br, 8);
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
                    channel_bit_reader_get_unsigned_bits(br, 8) & 0xFFu;
                if (*br_ok == 0u)
                    break;
                a1[2] = 10u;
                continue;
            }
            case 10u: {
                const std::uint32_t v = channel_bit_reader_get_unsigned_bits(br, 8) & 0xFFu;
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
                    channel_bit_reader_get_unsigned_bits(br, 8) & 0xFFu;
                if (*br_ok == 0u)
                    break;
                a1[2] = 8u;
                continue;
            }
            case 12u: {
                const std::uint32_t v = channel_bit_reader_get_unsigned_bits(br, 32);
                std::uint32_t carrier_layout = 0u;
                if (/* Decompiled name: channel_get_carrier_layout */
channel_get_carrier_layout(
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
                (void)channel_bit_reader_get_unsigned_bits(br, 8);
                if (*br_ok == 0u)
                    break;
                a1[2] = 8u;
                continue;
            case 14u:
                (void)channel_bit_reader_get_unsigned_bits(br, 16);
                if (*br_ok == 0u)
                    break;
                a1[2] = 8u;
                continue;
            case 15u:
                (void)channel_bit_reader_get_unsigned_bits(br, 24);
                if (*br_ok == 0u)
                    break;
                a1[2] = 8u;
                continue;
            case 16u:
                (void)channel_bit_reader_get_unsigned_bits(br, 32);
                if (*br_ok == 0u)
                    break;
                a1[2] = 8u;
                continue;
            case 17u: {
                const std::uint32_t op = channel_bit_reader_get_unsigned_bits(br, 8) & 0xFFu;
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
                const std::uint32_t v = channel_bit_reader_get_unsigned_bits(br, 24);
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
                (void)channel_bit_reader_get_unsigned_bits(br, 32);
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
                const std::uint32_t vv = channel_bit_reader_get_unsigned_bits(br, static_cast<std::int32_t>(bits));
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
                const std::uint32_t vv = channel_bit_reader_get_unsigned_bits(br, 32);
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
                const std::uint32_t rb = channel_bit_reader_get_remaining_nr_bits(br);
                const std::uint32_t tail = channel_bit_reader_get_unsigned_bits(
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

    channel_crc_process(
        parser_base + 12u,
        channel_ptr,
        static_cast<std::int32_t>(word_count));
    const std::int64_t crc_ck = channel_crc_check(
        parser_base + 12u,
        *reinterpret_cast<const std::int32_t*>(frame_channel_ptr + 56u),
        *reinterpret_cast<const std::int16_t*>(frame_channel_ptr + 52u));
    if (out_mode)
        *out_mode = *reinterpret_cast<const std::uint32_t*>(frame_channel_ptr + 92u);
    return (crc_ck != 0) ? 1 : 0;
}

bool parse_result_pool_construct(
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
        channel_parse_result_construct(parse_result_ptr);
        offset += kParseResultStrideBytes;
    }
    return storage_base_ptr != 0u || pool_count == 0u;
}

std::uint64_t parse_result_pool_get_new(std::uint64_t pool_base_ptr) {
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
    channel_parse_result_prepare_new(parse_result_ptr);
    return parse_result_ptr;
}

void downmix_coefficients_initialize(std::uint64_t coeff_ptr) {
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

void channel_parse_result_construct(std::uint64_t parse_result_ptr) {
    // current IDA ParseResult_t_construct: zero header 0..191 and tail 3648..3667.
    std::memset(reinterpret_cast<void*>(parse_result_ptr), 0, 192u);
    std::memset(reinterpret_cast<void*>(parse_result_ptr + 3648u), 0, 20u);
}

void frame_construct_parse_results(
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

void frame_construct(
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

void frame_assign_window(
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

void frame_mark_usage(
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

void frame_mark_as_unused(
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

void frame_mark_as_unused_106cd0_default(std::uint64_t frame_ptr) {
    frame_mark_as_unused(frame_ptr, channel_parse_result_mark_usage);
}

std::int64_t parser_frame_mark_as_unused_cb(std::uint64_t frame_ptr) {
    frame_mark_as_unused_106cd0_default(frame_ptr);
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

std::uint64_t frame_deque_find_first_with_end_after(
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

std::int64_t frame_deque_push_back(
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

std::uint64_t frame_deque_pop_front(
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

std::uint64_t frame_deque_pop_front_keep_frame(std::uint64_t frame_deque_ptr) {
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

std::uint64_t frame_deque_pop_back(
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

std::uint64_t /* Decompiled name: frame_deque_count */
frame_deque_count(std::uint64_t frame_deque_ptr) {
    NativeFrameDequeView530240 native{};
    if (read_native_frame_deque_530240(frame_deque_ptr, native))
        return native.count;
    const auto* dq = reinterpret_cast<const FrameDequeState13d570*>(frame_deque_ptr);
    if (!dq || dq->capacity == 0u)
        return 0u;
    return dq->count;
}

std::uint64_t /* Decompiled name: frame_deque_frame_at */
frame_deque_frame_at(std::uint64_t frame_deque_ptr, std::uint64_t logical_index) {
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

std::uint64_t /* Decompiled name: frame_deque_back */
frame_deque_back(std::uint64_t frame_deque_ptr, std::uint64_t* storage_index_out) {
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

void parser_rebind_frame_parse_results(
    std::uint64_t frame_ptr,
    const ParserRebindContext103610* ctx) {
    if (frame_ptr == 0u || ctx == nullptr || ctx->parse_result_pool_base == 0u)
        return;
    const std::uint32_t channel_count =
        *reinterpret_cast<const std::uint32_t*>(frame_ptr + 44u);
    for (std::uint32_t slot = 0; slot < channel_count; ++slot) {
        const std::uint64_t parse_result_ptr = parse_result_pool_get_new(ctx->parse_result_pool_base);
        const std::uint64_t frame_slot_ptr =
            frame_ptr + 48u + static_cast<std::uint64_t>(slot) * 32u;
        *reinterpret_cast<std::uint64_t*>(frame_slot_ptr + 24u) = parse_result_ptr;

        if (slot >= ctx->channel_parser_count)
            continue;
        const std::uint64_t parser_base = reinterpret_cast<std::uint64_t>(
            ctx->channel_parser_base + static_cast<std::size_t>(slot) * 64u);
        (void)channel_parser_construct(
            parser_base,
            parse_result_ptr,
            frame_ptr + 28u,
            frame_ptr + 56u + static_cast<std::uint64_t>(slot) * 32u);
    }
}

void parser_refresh_payload(
    const CodecV3IoBufferDescEb5a0* input_desc,
    const ParserPayloadRefreshContext* ctx) {
    if (input_desc == nullptr || ctx == nullptr || ctx->frame_deque_ptr == 0u || ctx->block_size == 0u)
        return;
    if (/* Decompiled name: frame_deque_count */
frame_deque_count(ctx->frame_deque_ptr) == 0u)
        return;
    const std::uint64_t active_frame_ptr = /* Decompiled name: frame_deque_frame_at */
frame_deque_frame_at(ctx->frame_deque_ptr, 0u);
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

std::int64_t parser_ready_frame_push_copy(
    std::uint64_t frame_deque_ptr,
    std::uint64_t frame_ptr,
    std::uint64_t copy_bytes,
    const ParserReadyFrameCopyContext* copy_ctx) {
    if (frame_deque_push_back(frame_deque_ptr, frame_ptr, copy_bytes) == 0)
        return 0;
    if (copy_ctx == nullptr || copy_ctx->ready_parse_result_base == nullptr
        || copy_ctx->ready_parse_result_size == 0u || copy_ctx->copied_slot_capacity == 0u
        || copy_ctx->parse_result_bytes == 0u) {
        return 1;
    }

    std::uint64_t ready_tail = 0u;
    const std::uint64_t ready_frame_ptr = /* Decompiled name: frame_deque_back */
frame_deque_back(frame_deque_ptr, &ready_tail);
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
        return parser_ready_frame_push_copy(
            frame_deque_ptr,
            frame_ptr,
            kCodecV3FrameDequeSlotCopyBytes,
            &g_parser_ready_frame_copy_ctx);
    }
    return frame_deque_push_back(
        frame_deque_ptr, frame_ptr, kCodecV3FrameDequeSlotCopyBytes);
}

void parser_state_sink_notify(ParserStateSinkContext* sink, std::uint32_t state) {
    if (!sink)
        return;
    if (sink->state_ptr)
        *sink->state_ptr = state;
    if (sink->dispatch)
        codec_v3_content_callback_eb870(sink->dispatch, state != 0u ? 1 : 0);
}

std::int64_t decoder_run_parser(DecoderParserRunContext1034e0* ctx) {
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

std::int64_t decoder_run_parser_stage(DecoderParserStageContext1034e0* ctx) {
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
        ::auro3deng::parser_state_sink_notify(sink, state);
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

    const std::int64_t rc = ::auro3deng::decoder_run_parser(&parser_ctx);
    g_parser_ready_frame_copy_active = false;
    if (ctx->timeline_cursor_ptr)
        *ctx->timeline_cursor_ptr = parser_ctx.timeline_cursor;
    update_parser_state(*ctx->parser_state_ptr);
    return rc;
}

std::int64_t decoder_run_output_stage(
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
    return decoder_run_output_generator(&run_ctx);
}

void decoder_init_fake_frame_channels(
    const DecoderInitFakeFrameChannelsContext106ba0* ctx) {
    if (!ctx || !ctx->fake_frame || !ctx->fake_frame_next || !ctx->fake_frame_channels_base
        || !ctx->fake_frame_channel_words_base || !ctx->fake_frame_channel_ctx_base)
        return;

    std::uint32_t slot = 0;
    const std::uint32_t input_mask = ctx->input_mask & kCodecV3ChannelMask;
    frame_construct(
        reinterpret_cast<std::uint64_t>(ctx->fake_frame),
        0u,
        ctx->block_size,
        input_mask,
        input_mask);
    frame_construct(
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
            const std::uint64_t from_pool = parse_result_pool_get_new(ctx->parse_result_pool_base);
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

void decoder_init_frame_deques(DecoderInitFrameDequesContext13d5d0* ctx) {
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

std::int64_t decoder_run_dispatch(DecoderDispatchRunContextEb5a0* ctx) {
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
    return codec_v3_dispatch(
        ctx->dispatch,
        &input_desc,
        &output_desc,
        &io_status,
        codec_v3_process_partial_eb5a0,
        &runtime);
}

std::int64_t decoder_run_step(DecoderStepRunContext101800* ctx) {
    const std::int64_t rc_dispatch = ::auro3deng::decoder_run_dispatch(ctx->dispatch_ctx);
    if (rc_dispatch != 0)
        return rc_dispatch;

    if (ctx->parser_input_desc && ctx->payload_ctx)
        ::auro3deng::parser_refresh_payload(ctx->parser_input_desc, ctx->payload_ctx);
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

