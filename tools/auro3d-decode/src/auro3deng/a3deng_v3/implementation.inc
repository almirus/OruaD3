ProcessorIoExpectDa9ae0 /* Decompiled name: load_expect_da9ae0_from_impl */
load_expect_da9ae0_from_impl(const std::uint8_t* impl_base) {
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

void decoder_crc_t_init_106f00() {
    // Literal перенос auro_codec_v3_decoder_CRC_t_init @ 0x106F00.
    // CRC-таблица у нас уже инициализируется лениво через channel_crc_table_init(),
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

std::int64_t construct_codec_v3_decoder_runtime(
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
                reinterpret_cast<void*>(codec_v3_pre_segments_decide_decode),
                a1);
            return 0;
        }
        return 1;
    }
    return result;
}

std::int64_t auro_a3deng_v3_process_decoder_manager(
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

std::int64_t auro_a3deng_v3_copy_input_to_output_channels(
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

std::uint64_t auro_a3deng_v3_copy_channels_and_return_mask(
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
                process_fn = reinterpret_cast<std::uintptr_t>(&auro_a3deng_v3_copy_input_to_output_channels);
            } else {
                if (mode != 0u)
                    return 1;
                process_fn = reinterpret_cast<std::uintptr_t>(&auro_a3deng_v3_copy_channels_and_return_mask);
            }
        } else {
            if (*reinterpret_cast<const std::uint32_t*>(update_config + 12u) != 1u)
                return 1;
            if (*reinterpret_cast<const std::uint32_t*>(update_config + 16u) != 24u)
                return 1;
            if (auro_a3deng_v3_reset_decoder_runtime_state(reinterpret_cast<std::uint64_t*>(base)) != 0)
                return 1;
            process_fn = reinterpret_cast<std::uintptr_t>(&auro_a3deng_v3_process_codec_block);
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

void auro_a3deng_v3_set_input_state_and_notify(std::int64_t decoder_base, std::int32_t value) {
    auto* base = reinterpret_cast<std::uint8_t*>(decoder_base);
    if (*reinterpret_cast<std::int32_t*>(base + 2504u) == value)
        return;
    *reinterpret_cast<std::int32_t*>(base + 2504u) = value;
    auro_a3deng_v3_decoder_notify_4d91a0(base, 0);
}

void auro_a3deng_v3_set_decode_state_and_notify(std::int64_t decoder_base, std::int32_t value) {
    auto* base = reinterpret_cast<std::uint8_t*>(decoder_base);
    if (*reinterpret_cast<std::int32_t*>(base + 2500u) == value)
        return;
    *reinterpret_cast<std::int32_t*>(base + 2500u) = value;
    auro_a3deng_v3_decoder_notify_4d91a0(base, 1);
}

std::int64_t auro_a3deng_v3_apply_decode_decisions(
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

std::int64_t auro_a3deng_v3_reset_decoder_runtime_state(std::uint64_t* decoder_base) {
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
        reinterpret_cast<void*>(auro_a3deng_v3_set_input_state_and_notify),
        base);
    auro_codec_v3_Decoder_set_content_callback(
        *decoder_base,
        reinterpret_cast<void*>(auro_a3deng_v3_set_decode_state_and_notify),
        base);
    auro_codec_v3_Decoder_set_decide_decode_callback(
        *decoder_base,
        reinterpret_cast<void*>(codec_v3_pre_segments_decide_decode),
        base);
    return 0;
}

void auro_a3deng_v3_Decoder_allow_decoding(std::int64_t decoder_base, std::int32_t enabled) {
    *reinterpret_cast<std::int32_t*>(reinterpret_cast<std::uint8_t*>(decoder_base) + 2508u) = enabled;
}

std::int64_t auro_a3deng_v3_Decoder_reset_audio_state(std::uint64_t* decoder_base) {
    if (!decoder_base || *decoder_base == 0u)
        return 0;
    return auro_a3deng_v3_reset_decoder_runtime_state(decoder_base) != 0 ? 1 : 0;
}

std::int64_t auro_a3deng_v3_process_codec_block(
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
    // Native Manager uses this slot as the step's bound block-state pointer.
    // It is the activation predicate until Manager_configure binds a block.
    std::uintptr_t block_state;
};

void manager_construct_downsample_step(std::uint8_t* step);
void manager_construct_upsample_step(std::uint8_t* step);
void manager_construct_downmixer_step(std::uint8_t* step);
void manager_construct_bass_management_step(std::uint8_t* step);
void manager_reset_bass_management_step(std::uint8_t* step);
std::int64_t manager_configure_bass_management_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry,
    std::uint32_t sample_rate);
std::int64_t manager_update_peak_bass_management(
    std::uint8_t* step,
    float* peak_31);
std::int64_t manager_process_bass_management(
    std::uint8_t* step,
    void** io_channels);
void manager_construct_silence_step(std::uint8_t* step);
std::int64_t manager_configure_silence_step(
    std::uint8_t* step,
    const std::uint8_t* block_state);
std::int64_t manager_process_silence_step(
    std::uint8_t* step,
    void** io_channels);
void manager_reset_silence_step(std::uint8_t* step);
void manager_construct_channel_test_step(std::uint8_t* step);
std::int64_t manager_configure_channel_test_step(
    std::uint8_t* step,
    const std::uint8_t* plan_entry);
std::int64_t manager_process_channel_test_step(
    std::uint8_t* step,
    void** io_channels);
std::int64_t manager_update_peak_channel_test_step(
    std::uint8_t* step,
    float* peak_31);
void manager_reset_channel_test_step(std::uint8_t* step);
void manager_construct_scene_step(std::uint8_t* step);
std::int64_t manager_configure_scene_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry);
std::int64_t manager_process_scene_step(
    std::uint8_t* step,
    void** io_channels);
std::int64_t manager_update_peak_scene_step(
    std::uint8_t* step,
    float* peak_31);
void manager_reset_scene_step(std::uint8_t* step);
void manager_construct_legacy_step(std::uint8_t* step);
std::int64_t manager_configure_legacy_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry,
    std::uint32_t input_layout);
std::int64_t manager_process_legacy_step(
    std::uint8_t* step,
    void** io_channels);
std::int64_t manager_update_peak_legacy_step(
    std::uint8_t* step,
    float* peak_31);
void manager_reset_legacy_step(std::uint8_t* step);
void manager_construct_delay_step(std::uint8_t* step);
std::int64_t manager_configure_delay_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry,
    std::uint32_t input_layout);
std::int64_t manager_process_delay_step(
    std::uint8_t* step,
    void** io_channels);
void manager_reset_delay_step(std::uint8_t* step);
void manager_construct_alt3d_store_step(std::uint8_t* step);
std::int64_t manager_configure_alt3d_store_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry);
std::int64_t manager_process_alt3d_store_step(
    std::uint8_t* step,
    void** io_channels);
void manager_reset_alt3d_store_step(std::uint8_t* step);
std::int64_t manager_configure_alt3d_mix_step(
    std::uint8_t* step,
    std::uint8_t* store_step,
    const std::uint32_t* plan_entry);
std::int64_t manager_process_alt3d_mix_step(
    std::uint8_t* step,
    void** io_channels);
void manager_reset_alt3d_mix_step(std::uint8_t* step);
std::int64_t manager_configure_downmixer_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry,
    bool first_time,
    std::uint8_t* block_state);
std::int64_t manager_process_downmixer(std::uint8_t* step, void** io_channels);
std::int64_t manager_update_peak_downmixer(
    std::uint8_t* step,
    float* peak_31);
std::int64_t manager_update_peak_alt3d_store(
    std::uint8_t* step,
    float* peak_31);
std::int64_t manager_update_peak_alt3d_mix(
    std::uint8_t* step,
    float* peak_31);

// The common activation ABI recovered from sub_4CD6C0.  Plan entries are
// sparse and each active step receives the predecessor's metadata before its
// typed configure callback is called.  Keep this helper limited to Upmix
// until the other step callback signatures are ported; silently invoking them
// through one generic function would corrupt their state.
std::int64_t manager_link_upmix_plan_entry(
    std::uint8_t* step,
    std::uint8_t* predecessor_metadata,
    const std::uint8_t* plan_entry) {
    if (!step || !predecessor_metadata || !plan_entry)
        return 1;
    *reinterpret_cast<std::uint64_t*>(step + 32u) =
        reinterpret_cast<std::uint64_t>(predecessor_metadata);
    std::memcpy(step + 40u, predecessor_metadata, 16u);
    *reinterpret_cast<std::uint32_t*>(step + 56u) =
        *reinterpret_cast<const std::uint32_t*>(predecessor_metadata + 16u);
    return auro_a3deng_v3_pipeline_Upmix_configure(
        step, predecessor_metadata, plan_entry);
}

constexpr ManagerStepOffsets478078 kManagerSteps478078[] = {
    {0u, 16u, 24u, 32u},
    {64u, 80u, 88u, 96u},
    {1800u, 1816u, 1824u, 1832u},
    {3536u, 3552u, 3560u, 3568u},
    {3608u, 3624u, 3632u, 3640u},
    {4456u, 4472u, 4480u, 4488u},
    {5416u, 5432u, 5440u, 5448u},
    {6248u, 6264u, 6272u, 6280u},
    {7944u, 7960u, 7968u, 7976u},
    {13072u, 13088u, 13096u, 13104u},
    {13160u, 13176u, 13184u, 13192u},
    {13992u, 14008u, 14016u, 14024u},
    {14184u, 14200u, 14208u, 14216u},
    {14560u, 14576u, 14584u, 14592u},
    {14648u, 14664u, 14672u, 14680u},
    {14848u, 14864u, 14872u, 14880u},
    {15800u, 15816u, 15824u, 15832u},
    {16176u, 16192u, 16200u, 16208u},
    {16248u, 16264u, 16272u, 16280u},
    {61784u, 61800u, 61808u, 61816u},
};

void manager_construct_confirmed_simple_step_headers(
    std::uint8_t* step_chain,
    std::uint8_t* manager) {
    if (!step_chain || !manager)
        return;
    const auto set_slots = [&](std::size_t base, std::uint64_t configure,
                               std::uint64_t update, std::uint64_t process,
                               std::uint64_t reset) {
        auto* step = step_chain + base;
        *reinterpret_cast<std::uint64_t*>(step + 0u) = configure;
        *reinterpret_cast<std::uint64_t*>(step + 8u) = update;
        *reinterpret_cast<std::uint64_t*>(step + 16u) = process;
        *reinterpret_cast<std::uint64_t*>(step + 24u) = reset;
    };
    // These callback tables are direct transcriptions of the current
    // Manager_t_construct call sites; their block-state fields remain zero
    // until the corresponding native configure branch is ported.
    // auro_a3deng_v3_pipeline_step_SampleConvertor_t_construct @ 0x4CF810:
    // [0]=configure, [1]=update, [2]=0, [3]=nullsub.
    set_slots(0u,
              ::auro_engine_v4_ida::kStepSampleConvertor_configure,
              ::auro_engine_v4_ida::kStepSampleConvertor_update,
              0u,
              0u);
    set_slots(61784u,
              ::auro_engine_v4_ida::kStepSampleConvertor_configure,
              ::auro_engine_v4_ida::kStepSampleConvertor_update,
              0u,
              0u);
    set_slots(6248u,
              ::auro_engine_v4_ida::kStepDelay_configure,
              ::auro_engine_v4_ida::kStepDelay_update,
              ::auro_engine_v4_ida::kStepDelay_process,
              ::auro_engine_v4_ida::kStepDelay_reset);
    manager_construct_delay_step(step_chain + 6248u);
    set_slots(64u,
              ::auro_engine_v4_ida::kStepDownSample_configure,
              ::auro_engine_v4_ida::kStepDownSample_update,
              ::auro_engine_v4_ida::kStepDownSample_process,
              ::auro_engine_v4_ida::kStepDownSample_reset);
    manager_construct_downsample_step(step_chain + 64u);
    set_slots(1800u,
              ::auro_engine_v4_ida::kStepDownSample_configure,
              ::auro_engine_v4_ida::kStepDownSample_update,
              ::auro_engine_v4_ida::kStepDownSample_process,
              ::auro_engine_v4_ida::kStepDownSample_reset);
    manager_construct_downsample_step(step_chain + 1800u);
    set_slots(3536u,
              ::auro_engine_v4_ida::kStepLegacy_configure,
              ::auro_engine_v4_ida::kStepLegacy_update,
              ::auro_engine_v4_ida::kStepLegacy_process,
              ::auro_engine_v4_ida::kStepLegacy_reset);
    manager_construct_legacy_step(step_chain + 3536u);
    set_slots(3608u,
              ::auro_engine_v4_ida::kStepBassManagement_configure,
              ::auro_engine_v4_ida::kStepBassManagement_update,
              ::auro_engine_v4_ida::kStepBassManagement_process,
              ::auro_engine_v4_ida::kStepBassManagement_reset);
    manager_construct_bass_management_step(step_chain + 3608u);
    set_slots(4456u,
              ::auro_engine_v4_ida::kStepAlt3DStore_configure,
              ::auro_engine_v4_ida::kStepAlt3DStore_update,
              ::auro_engine_v4_ida::kStepAlt3DStore_process,
              ::auro_engine_v4_ida::kStepAlt3DStore_reset);
    manager_construct_alt3d_store_step(step_chain + 4456u);
    set_slots(5416u,
              ::auro_engine_v4_ida::kStepDownmix_configure,
              ::auro_engine_v4_ida::kStepDownmix_update,
              ::auro_engine_v4_ida::kStepDownmix_process,
              ::auro_engine_v4_ida::kStepDownmix_reset);
    manager_construct_downmixer_step(step_chain + 5416u);
    set_slots(14848u,
              ::auro_engine_v4_ida::kStepUpSample_configure,
              ::auro_engine_v4_ida::kStepUpSample_update,
              ::auro_engine_v4_ida::kStepUpSample_process,
              ::auro_engine_v4_ida::kStepUpSample_reset);
    manager_construct_upsample_step(step_chain + 14848u);
    set_slots(13072u,
              ::auro_engine_v4_ida::kStepAlt3DMix_configure,
              ::auro_engine_v4_ida::kStepAlt3DMix_update,
              ::auro_engine_v4_ida::kStepAlt3DMix_process,
              0u);
    // alt3D_Mix_t_construct stores its linked Store callback at +64. The
    // Manager configure path replaces this with the Store state pointer.
    *reinterpret_cast<std::uint64_t*>(step_chain + 13072u + 64u) = 0u;
    set_slots(13160u,
              ::auro_engine_v4_ida::kStepDownmix_configure,
              ::auro_engine_v4_ida::kStepDownmix_update,
              ::auro_engine_v4_ida::kStepDownmix_process,
              ::auro_engine_v4_ida::kStepDownmix_reset);
    manager_construct_downmixer_step(step_chain + 13160u);
    set_slots(13992u,
              ::auro_engine_v4_ida::kStepSilence_configure,
              ::auro_engine_v4_ida::kStepSilence_update,
              ::auro_engine_v4_ida::kStepSilence_process,
              ::auro_engine_v4_ida::kStepSilence_reset);
    manager_construct_silence_step(step_chain + 13992u);
    set_slots(14184u,
              ::auro_engine_v4_ida::kStepScene_configure,
              ::auro_engine_v4_ida::kStepScene_update,
              ::auro_engine_v4_ida::kStepScene_process,
              ::auro_engine_v4_ida::kStepScene_reset);
    manager_construct_scene_step(step_chain + 14184u);
    // Manager_t_construct passes manager+136 as Scene's remap context
    // (libauro.so.c:4D2670).  The remaining Scene constructor arguments are
    // native static tables/arena pointers and are intentionally not guessed.
    *reinterpret_cast<std::uint64_t*>(step_chain + 14184u + 368u) =
        reinterpret_cast<std::uint64_t>(manager + 136u);
    set_slots(14560u,
              ::auro_engine_v4_ida::kStepHeadphones_configure,
              ::auro_engine_v4_ida::kStepHeadphones_update,
              ::auro_engine_v4_ida::kStepHeadphones_process,
              ::auro_engine_v4_ida::kStepHeadphones_reset);
    // Headphones_t_construct leaves this mode flag at zero unless native
    // mode 3 allocates the separate AHP/AM4HP backing object.
    *reinterpret_cast<std::uint32_t*>(step_chain + 14560u + 64u) = 0u;
    // Headroom_t_construct @ 0x4D13E0. This owning helper has no separate
    // initial-headroom argument; its native default is zero dB here.
    auto* headroom = step_chain + 15800u;
    (void)auro_a3deng_v3_Headroom_construct_object(headroom, 0.0f);
    set_slots(15800u,
              ::auro_engine_v4_ida::kStepHeadroom_configure,
              ::auro_engine_v4_ida::kStepHeadroom_update,
              ::auro_engine_v4_ida::kStepHeadroom_process,
              0u);
    // Gate_t_construct clears exactly its 44-byte state before installing
    // the callback table; reproduce that order rather than only zeroing the
    // surrounding chain.
    (void)auro_a3deng_v4_Gate_construct_portable(step_chain + 16176u);
    set_slots(16176u,
              ::auro_engine_v4_ida::kStepGate_configure,
              ::auro_engine_v4_ida::kStepGate_update,
              ::auro_engine_v4_ida::kStepGate_process_current,
              ::auro_engine_v4_ida::kStepGate_reset);
    set_slots(16248u,
              ::auro_engine_v4_ida::kStepPeakLimiter_configure,
              ::auro_engine_v4_ida::kStepPeakLimiter_update,
              ::auro_engine_v4_ida::kStepPeakLimiter_process,
              ::auro_engine_v4_ida::kStepPeakLimiter_reset);
    set_slots(14648u,
              ::auro_engine_v4_ida::kStepChannelTest_configure,
              ::auro_engine_v4_ida::kStepChannelTest_update,
              ::auro_engine_v4_ida::kStepChannelTest_process,
              ::auro_engine_v4_ida::kStepChannelTest_reset);
    manager_construct_channel_test_step(step_chain + 14648u);
}

std::uint32_t /* Decompiled name: manager_parameter_update_type */
manager_parameter_update_type(const std::uint32_t* current, const std::uint32_t* next) {
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

// auro_matic_v3_DetectSilence @ 0x555190/0x642D70.  The native detector
// keeps one 16-entry energy history per selected input channel.  The scalar
// form below follows the same ring/index/threshold transition; SIMD is only
// an implementation detail of the native body.
void auromatic_detect_silence_construct(std::uint8_t* detector) {
    if (!detector)
        return;
    std::memset(detector, 0, 2128u);
    *reinterpret_cast<std::uint64_t*>(detector) = 0x3A0637BD38077F39ull;
}

void auromatic_detect_silence_reset(std::uint8_t* detector) {
    if (!detector)
        return;
    for (std::uint32_t channel = 0u; channel < 31u; ++channel)
        std::memset(detector + 16u + 68u * channel, 0, 16u * sizeof(float));
    *reinterpret_cast<std::uint32_t*>(detector + 2124u) = 0u;
    *reinterpret_cast<std::uint32_t*>(detector + 8u) = 0u;
}

void auromatic_detect_silence_configure(
    std::uint8_t* detector,
    std::uint32_t sample_rate,
    std::uint32_t channel_mask) {
    if (!detector)
        return;
    if (sample_rate != 16000u && sample_rate != 22050u && sample_rate != 24000u
        && sample_rate != 32000u && sample_rate != 44100u && sample_rate != 48000u
        && sample_rate != 88200u && sample_rate != 96000u && sample_rate != 176400u
        && sample_rate != 192000u)
        return;
    auromatic_detect_silence_reset(detector);
    std::uint32_t count = 0u;
    for (std::uint32_t id = 0; id < 31u; ++id) {
        if ((channel_mask & (1u << id)) != 0u) {
            *reinterpret_cast<std::uint32_t*>(detector + 12u + 68u * count) = id;
            ++count;
        }
    }
    *reinterpret_cast<std::uint32_t*>(detector + 2120u) = count;
}

bool auromatic_detect_silence_process(
    std::uint8_t* detector,
    void** channel_span) {
    if (!detector || !channel_span)
        return false;
    const std::uint32_t count = *reinterpret_cast<const std::uint32_t*>(detector + 2120u);
    if (count == 0u) {
        *reinterpret_cast<std::uint32_t*>(detector + 8u) = 0u;
        return false;
    }
    const std::uint32_t ring = *reinterpret_cast<const std::uint32_t*>(detector + 2124u) & 15u;
    for (std::uint32_t n = 0; n < count; ++n) {
        const std::uint32_t id = *reinterpret_cast<const std::uint32_t*>(
            detector + 12u + 68u * n);
        const auto* samples = static_cast<const float*>(channel_span[id]);
        float energy = 0.0f;
        if (samples) {
            for (std::uint32_t i = 0; i < 32u; ++i)
                energy += samples[i] * samples[i];
        }
        reinterpret_cast<float*>(detector + 16u + 68u * n)[ring] = energy;
    }
    *reinterpret_cast<std::uint32_t*>(detector + 2124u) = (ring + 1u) & 15u;
    const float threshold = *reinterpret_cast<const float*>(
        detector + (*reinterpret_cast<const std::uint32_t*>(detector + 8u) == 0u ? 0u : 4u));
    bool active = false;
    for (std::uint32_t n = 0; n < count; ++n) {
        float history_sum = 0.0f;
        const auto* history = reinterpret_cast<const float*>(detector + 16u + 68u * n);
        for (std::uint32_t i = 0; i < 16u; ++i)
            history_sum += history[i];
        if (history_sum >= threshold) {
            active = true;
            break;
        }
    }
    *reinterpret_cast<std::uint32_t*>(detector + 8u) = active ? 1u : 0u;
    return active;
}

void auromatic_gate_smooth(
    float* current,
    float* gains,
    float target,
    float coefficient) {
    if (!current || !gains)
        return;
    float value = *current;
    for (std::uint32_t i = 0; i < 32u; ++i) {
        value = coefficient * value + (1.0f - coefficient) * target;
        gains[i] = value;
    }
    *current = value;
}

bool auromatic_output_gate_process(
    std::uint8_t* gate,
    void** channel_span,
    float* gains,
    std::uint32_t* output_silent,
    std::uint32_t* transition_silent) {
    if (!gate || !channel_span || !gains)
        return false;
    (void)auromatic_detect_silence_process(gate + 8u, channel_span);
    if (output_silent)
        *output_silent = 0u;
    if (transition_silent)
        *transition_silent = 0u;
    const bool bypass = *reinterpret_cast<const std::uint32_t*>(gate + 4u) != 0u;
    const bool detected_active = *reinterpret_cast<const std::uint32_t*>(gate + 16u) != 0u;
    const bool native_bypass = detected_active ? bypass : true;
    std::uint32_t state = *reinterpret_cast<const std::uint32_t*>(gate);
    const float target = *reinterpret_cast<const float*>(gate + 2148u);
    float* current = reinterpret_cast<float*>(gate + 2144u);
    float output = 0.0f;
    float smoothing = 0.0f;
    bool smooth = true;

    switch (state) {
    case 0u:
        if (!native_bypass) {
            state = 1u;
            *current = target;
            output = target;
            smoothing = *reinterpret_cast<const float*>(gate + 2140u);
        } else {
            state = 0u;
            output = 0.0f;
            smoothing = 0.0f;
        }
        break;
    case 1u:
        if (native_bypass) {
            state = 2u;
            output = 0.0f;
            smoothing = *reinterpret_cast<const float*>(gate + 2136u);
        } else {
            state = 1u;
            output = target;
            smoothing = *reinterpret_cast<const float*>(gate + 2140u);
        }
        break;
    case 2u:
        if (native_bypass) {
            if (*current > 0.0000099999997f) {
                state = 2u;
                output = 0.0f;
                smoothing = *reinterpret_cast<const float*>(gate + 2136u);
            } else {
                state = 0u;
                if (transition_silent)
                    *transition_silent = 1u;
                *current = 0.0f;
                output = 0.0f;
                smoothing = 0.0f;
                smooth = false;
            }
        } else {
            state = 3u;
            output = target;
            smoothing = *reinterpret_cast<const float*>(gate + 2136u);
        }
        break;
    case 3u:
        if (!native_bypass) {
            const bool changed = std::fabs(*current - target) >= 0.0000099999997f;
            state = changed ? 3u : 1u;
            output = target;
            smoothing = *reinterpret_cast<const float*>(
                gate + (changed ? 2136u : 2140u));
        } else {
            state = 2u;
            output = 0.0f;
            smoothing = *reinterpret_cast<const float*>(gate + 2136u);
        }
        break;
    default:
        state = 0u;
        output = 0.0f;
        smoothing = 0.0f;
        smooth = false;
        break;
    }
    *reinterpret_cast<std::uint32_t*>(gate) = state;
    if (state == 0u && output_silent)
        *output_silent = 1u;
    if (smooth)
        auromatic_gate_smooth(current, gains, output, smoothing);
    else
        std::fill(gains, gains + 32u, output);
    return state == 0u;
}

// Native upmix::Input::process @ 0x35AAA0.  OutputGate writes the per-sample
// gain vector into record+4; the following Gain step applies it to every
// selected channel.  The two leading record bytes are the native output
// flags and are deliberately written before the next stage sees the record.
std::int64_t manager_process_upmix_input(
    std::uint8_t* step,
    void** io_channels) {
    if (!step || !io_channels)
        return 1;
    const std::uint64_t block_state = *reinterpret_cast<const std::uint64_t*>(step + 8u);
    if (!block_state)
        return 0;
    const std::uint32_t subblocks = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(block_state) + 4u);
    const std::uint64_t owner = *reinterpret_cast<const std::uint64_t*>(step + 2192u);
    if (!owner)
        return 0;
    auto* records = reinterpret_cast<std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<const std::uint64_t*>(owner + 32u)));
    if (!records)
        return 0;

    // gate+16 is DetectSilence's dynamic active flag (detector+8), not a
    // configuration marker. Native Input::process never reconfigures here;
    // doing so on silence would erase the detector history.
    auto* gate = step + 36u;

    std::array<void*, ::auro_engine_v4_ida::kChannelCount> span{};
    for (std::uint32_t block = 0; block < subblocks; ++block) {
        const std::size_t offset = static_cast<std::size_t>(block) * 32u;
        for (std::size_t ch = 0; ch < span.size(); ++ch) {
            auto* base = static_cast<std::uint8_t*>(io_channels[ch]);
            span[ch] = base ? base + offset * sizeof(float) : nullptr;
        }
        auto* record = const_cast<std::uint8_t*>(records + static_cast<std::size_t>(block) * 516u);
        std::uint32_t output_silent = 0u;
        std::uint32_t transition_silent = 0u;
        (void)auromatic_output_gate_process(
            gate,
            span.data(),
            reinterpret_cast<float*>(record + 4u),
            &output_silent,
            &transition_silent);
        record[0u] = output_silent != 0u ? 1u : 0u;
        record[1u] = transition_silent != 0u ? 1u : 0u;
    }
    return 0;
}

// Native upmix::Gain::process @ 0x35CE60.  The v4 step receives the
// subblock gain records produced by the pipeline, not a single gain for the
// whole host call.  Keep the record cursor and the 32-sample granularity
// identical to the decompilation.
std::int64_t manager_process_upmix_gain(
    std::uint8_t* step,
    void** io_channels) {
    if (!step || !io_channels)
        return 1;
    const std::uint64_t block_state = *reinterpret_cast<const std::uint64_t*>(step + 8u);
    if (!block_state)
        return 0;
    const std::uint32_t subblocks = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(block_state) + 4u);
    const std::uint32_t channel_count = *reinterpret_cast<const std::uint32_t*>(step + 40u);
    const auto* channel_ids = reinterpret_cast<const std::uint32_t*>(step + 48u);
    const std::uint64_t owner = *reinterpret_cast<const std::uint64_t*>(step + 176u);
    if (!owner)
        return 0;
    auto* records = reinterpret_cast<std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<const std::uint64_t*>(owner + 32u)));
    if (!records)
        return 0;

    for (std::uint32_t block = 0; block < subblocks; ++block) {
        auto* record = records + static_cast<std::size_t>(block) * 516u;
        if (record[1u] != 0u)
            continue;
        const auto* gains = reinterpret_cast<const float*>(record + 4u);
        for (std::uint32_t ch = 0; ch < channel_count && ch < 31u; ++ch) {
            const std::uint32_t id = channel_ids[ch];
            auto* samples = static_cast<float*>(io_channels[id]);
            if (!samples)
                continue;
            for (std::uint32_t i = 0; i < 32u; ++i)
                samples[block * 32u + i] *= gains[i];
        }
    }
    return 0;
}

// Native upmix::Silence::process @ 0x35D210.  Layouts are channel masks;
// the decompiled begin()/iteration is therefore equivalent to visiting set
// channel bits in ascending channel-id order.
std::int64_t manager_process_upmix_silence(
    std::uint8_t* step,
    void** io_channels) {
    if (!step || !io_channels)
        return 1;
    const std::uint64_t block_state = *reinterpret_cast<const std::uint64_t*>(step + 8u);
    if (!block_state)
        return 0;
    const std::uint32_t subblocks = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(block_state) + 4u);
    const std::uint32_t input_layout = *reinterpret_cast<const std::uint32_t*>(step + 36u);
    const std::uint32_t output_layout = *reinterpret_cast<const std::uint32_t*>(step + 40u);
    const std::uint64_t owner = *reinterpret_cast<const std::uint64_t*>(step + 48u);
    if (!owner)
        return 0;
    const auto* records = reinterpret_cast<const std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<const std::uint64_t*>(owner + 32u)));
    if (!records)
        return 0;

    for (std::uint32_t block = 0; block < subblocks; ++block) {
        const auto* record = records + static_cast<std::size_t>(block) * 516u;
        const std::uint32_t layout = record[1u] != 0u ? output_layout : input_layout;
        for (std::uint32_t id = 0; id < 31u; ++id) {
            if ((layout & (1u << id)) == 0u)
                continue;
            auto* samples = static_cast<float*>(io_channels[id]);
            if (samples)
                std::memset(samples + block * 32u, 0, 32u * sizeof(float));
        }
    }
    return 0;
}

// Native front_manager::Input::process @ 0x35AF60.  Its selected v3
// FrontManager callback consumes the first three entries of the 31-channel
// span as L/R/center.  The native wrapper still builds the complete span;
// doing the same here keeps channel addressing explicit and leaves the
// remaining entries available to future front-manager variants.
std::int64_t manager_process_upmix_front_manager_input(
    std::uint8_t* step,
    void** io_channels) {
    if (!step || !io_channels)
        return 1;
    const std::uint64_t block_state = *reinterpret_cast<const std::uint64_t*>(step + 8u);
    if (!block_state)
        return 0;
    const std::uint32_t subblocks = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(block_state) + 4u);
    const std::uint64_t owner = *reinterpret_cast<const std::uint64_t*>(step + 1216u);
    if (!owner)
        return 0;

    // FrontManager_t_construct calls Input_t_construct before any prepare or
    // process.  The portable manager can be entered from a restored step
    // chain, so perform the same construction at the first dispatch if the
    // embedded Centergen vtable is still null.  The native constructor passes
    // {48000, 1} (0x10000BB80) to Processor_t_construct.
    auto* processor = step + 40u;
    if (*reinterpret_cast<const std::uint64_t*>(processor) == 0u) {
        std::int32_t init_args[2] = {48000, 1};
        if (!auro_centergen_v3_Processor_t_construct(processor, init_args))
            return 1;
        *reinterpret_cast<std::uint64_t*>(step + 1192u) = 0u;
        *reinterpret_cast<std::uint64_t*>(processor + 1152u) = 0x5531A0u;
        *reinterpret_cast<std::uint64_t*>(processor + 1168u) = 1u;
    }
    auto* records = reinterpret_cast<std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<const std::uint64_t*>(owner + 32u)));
    if (!records)
        return 0;
    for (std::uint32_t block = 0; block < subblocks; ++block) {
        auto* record = records + static_cast<std::size_t>(block) * 516u;
        if (record[1u] != 0u) {
            if (record[0u] != 0u)
                (void)auro_centergen_v3_Processor_reset_audio_state(processor);
            continue;
        }
        const auto* left = static_cast<const float*>(io_channels[0]);
        const auto* right = static_cast<const float*>(io_channels[1]);
        if (!left || !right)
            continue;
        const std::size_t offset = static_cast<std::size_t>(block) * 32u;
        auto* record_left = reinterpret_cast<float*>(record + 132u);
        auto* record_right = reinterpret_cast<float*>(record + 260u);
        auto* record_center = reinterpret_cast<float*>(record + 388u);
        std::memcpy(record_left, left + offset, 32u * sizeof(float));
        std::memcpy(record_right, right + offset, 32u * sizeof(float));
        std::array<void*, ::auro_engine_v4_ida::kChannelCount> input_channels{};
        input_channels[0] = const_cast<float*>(left + offset);
        input_channels[1] = const_cast<float*>(right + offset);
        input_channels[2] = io_channels[2]
            ? static_cast<float*>(io_channels[2]) + offset
            : nullptr;
        if (auro_matic_v3_FrontManager_Input_process(
                processor,
                input_channels.data(),
                record_left,
                record_right,
                record_center) != 0)
            return 1;
    }
    return 0;
}

// Native front_manager::Output::process @ 0x35CA90 and its v3 callback.
// The callback operates on the three front channels and four 32-sample
// vectors carried by each 516-byte block record: the source vectors at
// +132/+260/+388 and the gain vector at +4.  Keep the native alias-safe
// interpolation order (the native SIMD path has an equivalent scalar path).
std::int64_t manager_process_upmix_front_manager_output(
    std::uint8_t* step,
    void** io_channels) {
    if (!step || !io_channels)
        return 1;
    const std::uint64_t block_state = *reinterpret_cast<const std::uint64_t*>(step + 8u);
    if (!block_state)
        return 0;
    const std::uint32_t subblocks = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(block_state) + 4u);
    const std::uint64_t owner = *reinterpret_cast<const std::uint64_t*>(step + 48u);
    if (!owner)
        return 0;

    // FrontManager_Output_t_construct initializes the two channel scales to
    // 1.0 and the mode to zero.  Its state is only eight bytes, and a zero
    // mode is the native pre-configuration value; initialize it here before
    // the first process when a restored chain has not run the constructor.
    if (*reinterpret_cast<const std::uint32_t*>(step + 44u) == 0u
        && *reinterpret_cast<const std::uint32_t*>(step + 36u) == 0u
        && *reinterpret_cast<const std::uint32_t*>(step + 40u) == 0u) {
        *reinterpret_cast<float*>(step + 36u) = 1.0f;
        *reinterpret_cast<float*>(step + 40u) = 1.0f;
    }
    const auto* records = reinterpret_cast<const std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<const std::uint64_t*>(owner + 32u)));
    if (!records)
        return 0;

    const std::uint32_t mode = *reinterpret_cast<const std::uint32_t*>(step + 44u);
    const float channel2_scale = *reinterpret_cast<const float*>(step + 36u);
    const float channel01_scale = *reinterpret_cast<const float*>(step + 40u);
    for (std::uint32_t block = 0; block < subblocks; ++block) {
        const auto* record = records + static_cast<std::size_t>(block) * 516u;
        if (record[1u] != 0u) {
            if (record[0u] != 0u)
                continue;
            continue;
        }
        auto* left = static_cast<float*>(io_channels[0]);
        auto* right = static_cast<float*>(io_channels[1]);
        auto* center = static_cast<float*>(io_channels[2]);
        if (!left || !right || !center)
            continue;
        const auto* source_left = reinterpret_cast<const float*>(record + 132u);
        const auto* source_right = reinterpret_cast<const float*>(record + 260u);
        const auto* source_center = reinterpret_cast<const float*>(record + 388u);
        const auto* gains = reinterpret_cast<const float*>(record + 4u);
        const std::size_t offset = static_cast<std::size_t>(block) * 32u;
        for (std::uint32_t i = 0; i < 32u; ++i) {
            float out_left = source_left[i];
            float out_right = source_right[i];
            float out_center = source_center[i];
            if (mode == 2u)
                out_center = (source_left[i] + source_right[i]) * 0.7071067811865476f;
            else if (mode != 1u && mode != 3u && mode != 4u)
                continue;
            out_center *= channel2_scale;
            out_left *= channel01_scale;
            out_right *= channel01_scale;
            const float gain = std::min(gains[i], 1.0f);
            center[offset + i] = gain * out_center + (1.0f - gain) * source_center[i];
            left[offset + i] = gain * out_left + (1.0f - gain) * source_left[i];
            right[offset + i] = gain * out_right + (1.0f - gain) * source_right[i];
        }
    }
    return 0;
}

// Auro-Matic 3D3D DelayLine primitives.  The ARM decompilation contains the
// same implementation contract as the v3 object: a base pointer, a moving
// cursor, an even sample capacity, and the number of samples still marked for
// partial clearing at offsets 0/8/16/20.  Keep these operations independent
// from the codec-v3 DelayLineState: 3D3D uses 32-sample Float32 vectors and
// advances by 32 samples per call.
struct Auromatic3d3dDelayLine {
    float* base = nullptr;
    float* cursor = nullptr;
    std::uint32_t capacity = 0u;
    std::uint32_t clear_remaining = 0u;
};

bool auromatic_3d3d_delayline_construct(
    Auromatic3d3dDelayLine* line,
    float* storage,
    std::uint32_t capacity) {
    if (!line || !storage || capacity < 32u || (capacity & 1u) != 0u)
        return false;
    line->base = storage;
    line->cursor = storage;
    line->capacity = capacity;
    line->clear_remaining = capacity + 32u;
    return true;
}

void auromatic_3d3d_delayline_set_dirty(Auromatic3d3dDelayLine* line) {
    if (line)
        line->clear_remaining = line->capacity + 32u;
}

void auromatic_3d3d_delayline_clear(Auromatic3d3dDelayLine* line) {
    if (!line || !line->base)
        return;
    line->cursor = line->base;
    std::memset(line->base, 0, static_cast<std::size_t>(line->capacity + 32u) * sizeof(float));
}

bool auromatic_3d3d_delayline_partial_clear(
    Auromatic3d3dDelayLine* line,
    std::uint32_t* remaining) {
    if (!line || !line->base || !remaining)
        return false;
    const std::uint32_t count = std::min(line->clear_remaining, *remaining);
    if (count != 0u) {
        std::memset(
            line->base + line->clear_remaining - count,
            0,
            static_cast<std::size_t>(count) * sizeof(float));
    }
    line->cursor = line->base;
    line->clear_remaining -= count;
    *remaining -= count;
    return line->clear_remaining == 0u;
}

void auromatic_3d3d_delayline_proceed(Auromatic3d3dDelayLine* line) {
    if (!line || !line->base || line->capacity == 0u)
        return;
    line->cursor += 32u;
    if (line->cursor >= line->base + line->capacity)
        line->cursor -= line->capacity;
}

void auromatic_3d3d_delayline_read(
    const Auromatic3d3dDelayLine* line,
    std::uint32_t delay,
    float* destination) {
    if (!line || !line->base || !destination || line->capacity == 0u)
        return;
    std::ptrdiff_t index = line->cursor - line->base - static_cast<std::ptrdiff_t>(delay);
    index %= static_cast<std::ptrdiff_t>(line->capacity);
    if (index < 0)
        index += line->capacity;
    for (std::uint32_t i = 0u; i < 32u; ++i)
        destination[i] = line->base[(static_cast<std::uint32_t>(index) + i) % line->capacity];
}

void auromatic_3d3d_delayline_write(
    Auromatic3d3dDelayLine* line,
    const float* source) {
    if (!line || !line->base || !source || line->capacity == 0u)
        return;
    const std::uint32_t index = static_cast<std::uint32_t>(line->cursor - line->base);
    for (std::uint32_t i = 0u; i < 32u; ++i)
        line->base[(index + i) % line->capacity] = source[i];
}

// Scalar form of auro_iir_biquad_w32_Process_float_2 from the ARM
// decompilation.  The two interleaved state pairs are deliberately kept
// separate: native Line2/Line3 use the first pair for the first vector and
// the second pair for the second vector.
void auromatic_3d3d_biquad_pair(
    float* first,
    float* second,
    float* first_state,
    float* second_state,
    const float* coefficients) {
    if (!first || !second || !first_state || !second_state || !coefficients)
        return;
    float first_s0 = first_state[0];
    float first_s1 = first_state[1];
    float second_s0 = second_state[0];
    float second_s1 = second_state[1];
    for (std::uint32_t i = 0u; i < 32u; ++i) {
        const float first_in = first[i];
        const float second_in = second[i];
        const float first_out = first_s0 + first_in * coefficients[0];
        const float second_out = second_s0 + second_in * coefficients[0];
        first[i] = first_out;
        second[i] = second_out;
        first_s0 = first_s1 + first_in * coefficients[1] - coefficients[3] * first_out;
        second_s0 = second_s1 + second_in * coefficients[1] - coefficients[3] * second_out;
        first_s1 = first_in * coefficients[2] - coefficients[4] * first_out;
        second_s1 = second_in * coefficients[2] - coefficients[4] * second_out;
    }
    first_state[0] = first_s0;
    first_state[1] = first_s1;
    second_state[0] = second_s0;
    second_state[1] = second_s1;
}

// Scalar form of auro_iir_biquad_w32_Process_float_1.  This is the exact
// recurrence used by each Line4 channel.
void auromatic_3d3d_biquad_single(
    float* samples,
    float* state,
    const float* coefficients) {
    if (!samples || !state || !coefficients)
        return;
    float s0 = state[0];
    float s1 = state[1];
    for (std::uint32_t i = 0u; i < 32u; ++i) {
        const float input = samples[i];
        const float output = s0 + input * coefficients[0];
        samples[i] = output;
        s0 = s1 + input * coefficients[1] - coefficients[3] * output;
        s1 = input * coefficients[2] - coefficients[4] * output;
    }
    state[0] = s0;
    state[1] = s1;
}

struct Auromatic3d3dLine4Channel {
    float filter_state[2]{};
    Auromatic3d3dDelayLine delay;
    std::uint8_t reserved[8]{};
};

void auromatic_3d3d_line4_channel_process_ext(
    Auromatic3d3dLine4Channel* channel,
    const float* first,
    const float* second,
    const float* gains,
    const float* coefficients,
    float* output) {
    if (!channel || !first || !second || !gains || !coefficients || !output)
        return;
    auromatic_3d3d_delayline_set_dirty(&channel->delay);
    float work[32]{};
    for (std::uint32_t i = 0u; i < 32u; ++i)
        work[i] = (first[i] + second[i]) * (gains[i] * 0.5f);
    auromatic_3d3d_biquad_single(work, channel->filter_state, coefficients);
    auromatic_3d3d_delayline_write(&channel->delay, work);
    auromatic_3d3d_delayline_read(&channel->delay, 144u, work);
    for (std::uint32_t i = 0u; i < 32u; ++i)
        output[i] = work[i] * coefficients[13u];
    auromatic_3d3d_delayline_proceed(&channel->delay);
}

struct Auromatic3d3dLine3 {
    float filter_state[4]{};
    Auromatic3d3dDelayLine first_delay;
    Auromatic3d3dDelayLine second_delay;
};

static_assert(sizeof(Auromatic3d3dDelayLine) == 24u);
static_assert(offsetof(Auromatic3d3dLine3, first_delay) == 16u);
static_assert(offsetof(Auromatic3d3dLine3, second_delay) == 40u);
static_assert(sizeof(Auromatic3d3dLine3) == 64u);

// Line2 shares the state shape with Line3, but has its own recurrence and
// native delay capacities (292 and 234 samples, versus 694 and 738).
using Auromatic3d3dLine2 = Auromatic3d3dLine3;

bool auromatic_3d3d_line2_construct(
    Auromatic3d3dLine2* line,
    float* first_storage,
    float* second_storage) {
    if (!line)
        return false;
    return auromatic_3d3d_delayline_construct(&line->first_delay, first_storage, 292u)
        && auromatic_3d3d_delayline_construct(&line->second_delay, second_storage, 234u);
}

bool auromatic_3d3d_line3_construct(
    Auromatic3d3dLine3* line,
    float* first_storage,
    float* second_storage) {
    if (!line)
        return false;
    return auromatic_3d3d_delayline_construct(&line->first_delay, first_storage, 694u)
        && auromatic_3d3d_delayline_construct(&line->second_delay, second_storage, 738u);
}

void auromatic_3d3d_line2_process_ext(
    Auromatic3d3dLine3* line,
    const float* first_a,
    const float* first_b,
    const float* first_output_input,
    const float* second_output_input,
    float* first_output,
    float* second_output,
    const float* coefficients,
    const float* gains) {
    if (!line || !first_a || !first_b || !first_output_input
        || !second_output_input || !first_output || !second_output
        || !coefficients || !gains)
        return;
    auromatic_3d3d_delayline_set_dirty(&line->first_delay);
    auromatic_3d3d_delayline_set_dirty(&line->second_delay);
    float first_work[32]{};
    float second_work[32]{};
    for (std::uint32_t i = 0u; i < 32u; ++i) {
        const float half_gain = gains[i] * 0.5f;
        first_work[i] = (first_a[i] + first_b[i]) * half_gain;
        second_work[i] = (second_output_input[i] + first_output_input[i]) * half_gain;
    }
    auromatic_3d3d_biquad_pair(
        first_work,
        second_work,
        line->filter_state,
        line->filter_state + 2u,
        coefficients);
    auromatic_3d3d_delayline_write(&line->first_delay, first_work);
    auromatic_3d3d_delayline_write(&line->second_delay, second_work);
    float first_mix_tap[32]{};
    float second_mix_tap[32]{};
    auromatic_3d3d_delayline_read(&line->second_delay, 202u, second_mix_tap);
    auromatic_3d3d_delayline_read(&line->first_delay, 260u, first_mix_tap);
    const float first_gain = coefficients[7u];
    const float second_gain = coefficients[8u];
    for (std::uint32_t i = 0u; i < 32u; ++i)
        second_output[i] = second_mix_tap[i] * second_gain
            + first_mix_tap[i] * first_gain;
    auromatic_3d3d_delayline_read(&line->first_delay, 144u, first_output);
    for (std::uint32_t i = 0u; i < 32u; ++i)
        first_output[i] *= coefficients[6u];
    auromatic_3d3d_delayline_proceed(&line->first_delay);
    auromatic_3d3d_delayline_proceed(&line->second_delay);
}

// Line3 has a different native recurrence from Line2: its two input buses
// are multiplied independently, and the delayed difference taps are 576/662
// and 576/706. It must not share Line2's 202/260 reconstruction.
void auromatic_3d3d_line3_process_ext(
    Auromatic3d3dLine3* line,
    const float* source_a,
    const float* source_b,
    float* first_output,
    float* second_output,
    const float* coefficients,
    const float* gains) {
    if (!line || !source_a || !source_b
        || !coefficients || !gains || !first_output || !second_output)
        return;
    auromatic_3d3d_delayline_set_dirty(&line->first_delay);
    auromatic_3d3d_delayline_set_dirty(&line->second_delay);
    float first_work[32]{};
    float second_work[32]{};
    for (std::uint32_t i = 0u; i < 32u; ++i) {
        first_work[i] = source_a[i] * gains[i];
        second_work[i] = source_b[i] * gains[i];
    }
    auromatic_3d3d_biquad_pair(
        first_work,
        second_work,
        line->filter_state,
        line->filter_state + 2u,
        coefficients);
    auromatic_3d3d_delayline_write(&line->first_delay, first_work);
    auromatic_3d3d_delayline_write(&line->second_delay, second_work);
    float first_short[32]{};
    float first_long[32]{};
    float second_short[32]{};
    float second_long[32]{};
    auromatic_3d3d_delayline_read(&line->first_delay, 576u, first_short);
    auromatic_3d3d_delayline_read(&line->first_delay, 662u, first_long);
    auromatic_3d3d_delayline_read(&line->second_delay, 576u, second_short);
    auromatic_3d3d_delayline_read(&line->second_delay, 706u, second_long);
    for (std::uint32_t i = 0u; i < 32u; ++i) {
        first_output[i] = first_long[i] * coefficients[10u]
            - first_short[i] * coefficients[9u];
        second_output[i] = second_long[i] * coefficients[12u]
            - second_short[i] * coefficients[11u];
    }
    auromatic_3d3d_delayline_proceed(&line->first_delay);
    auromatic_3d3d_delayline_proceed(&line->second_delay);
}

struct Auromatic3d3dLine4 {
    Auromatic3d3dLine4Channel first;
    Auromatic3d3dLine4Channel second;
};

static_assert(offsetof(Auromatic3d3dLine4Channel, delay) == 8u);
static_assert(sizeof(Auromatic3d3dLine4Channel) == 40u);
static_assert(sizeof(Auromatic3d3dLine4) == 80u);

bool auromatic_3d3d_line4_construct(
    Auromatic3d3dLine4* line,
    float* storage) {
    if (!line || !storage)
        return false;
    return auromatic_3d3d_delayline_construct(&line->first.delay, storage, 176u)
        && auromatic_3d3d_delayline_construct(
            &line->second.delay, storage + 208u, 176u);
}

bool auromatic_3d3d_line2_partial_clear(
    Auromatic3d3dLine2* line,
    std::uint32_t* remaining) {
    if (!line || !remaining)
        return false;
    std::memset(line->filter_state, 0, sizeof(line->filter_state));
    const bool first_done = auromatic_3d3d_delayline_partial_clear(
        &line->first_delay, remaining);
    const bool second_done = auromatic_3d3d_delayline_partial_clear(
        &line->second_delay, remaining);
    return first_done && second_done;
}

bool auromatic_3d3d_line3_partial_clear(
    Auromatic3d3dLine3* line,
    std::uint32_t* remaining) {
    return auromatic_3d3d_line2_partial_clear(line, remaining);
}

bool auromatic_3d3d_line4_partial_clear(
    Auromatic3d3dLine4* line,
    std::uint32_t* remaining) {
    if (!line || !remaining)
        return false;
    std::memset(line->first.filter_state, 0, sizeof(line->first.filter_state));
    std::memset(line->second.filter_state, 0, sizeof(line->second.filter_state));
    const bool first_done = auromatic_3d3d_delayline_partial_clear(
        &line->first.delay, remaining);
    const bool second_done = auromatic_3d3d_delayline_partial_clear(
        &line->second.delay, remaining);
    return first_done && second_done;
}

void auromatic_3d3d_line2_peak_amplitude(
    float* first_peak,
    float* second_peak,
    const float* coefficients,
    float first_gain_a,
    float first_gain_b,
    float second_gain_a,
    float second_gain_b) {
    if (!first_peak || !second_peak || !coefficients)
        return;
    const float first = (first_gain_a + first_gain_b) * 0.5f;
    *first_peak = coefficients[6u] * first;
    *second_peak = coefficients[7u] * first
        + coefficients[8u] * ((second_gain_a + second_gain_b) * 0.5f);
}

void auromatic_3d3d_line3_peak_amplitude(
    float* first_peak,
    float* second_peak,
    const float* coefficients,
    float first_gain,
    float second_gain) {
    if (!first_peak || !second_peak || !coefficients)
        return;
    *first_peak = std::fabs(coefficients[10u] - coefficients[9u]) * first_gain;
    *second_peak = std::fabs(coefficients[12u] - coefficients[11u]) * second_gain;
}

void auromatic_3d3d_line4_peak_amplitude(
    float* first_peak,
    float* second_peak,
    const float* coefficients,
    float first_a,
    float first_b,
    float second_a,
    float second_b) {
    if (!first_peak || !second_peak || !coefficients)
        return;
    const float first = (first_a + second_a) * 0.5f;
    const float second = (first_b + second_b) * 0.5f;
    *first_peak = first * coefficients[13u];
    *second_peak = second * coefficients[13u];
}

void auromatic_3d3d_line4_process_ext(
    Auromatic3d3dLine4* line,
    const float* first_a,
    const float* first_b,
    const float* second_a,
    const float* second_b,
    const float* gains,
    const float* coefficients,
    float* first_output,
    float* second_output) {
    if (!line || !first_a || !first_b || !second_a || !second_b || !gains
        || !coefficients || !first_output || !second_output)
        return;
    auromatic_3d3d_line4_channel_process_ext(
        &line->first,
        first_a,
        second_a,
        gains,
        coefficients,
        first_output);
    auromatic_3d3d_line4_channel_process_ext(
        &line->second,
        first_b,
        second_b,
        gains,
        coefficients,
        second_output);
}

bool manager_construct_upmix_3d3d(std::uint8_t* step);

// Native p3D3D::initialize passes four in-object plan buffers to the
// auro_matic state constructor.  Keep this boundary explicit: the plan
// pointers are storage owned by the step, not newly invented DSP memory.
bool manager_initialize_upmix_3d3d(std::uint8_t* step) {
    if (!step)
        return false;
    if (!manager_construct_upmix_3d3d(step))
        return false;
    auto* state = step + 40u;
    auto** line2_plan = reinterpret_cast<float**>(step + 448u);
    auto** line3a_plan = reinterpret_cast<float**>(step + 2808u);
    auto** line3b_plan = reinterpret_cast<float**>(step + 8792u);
    auto** line4_plan = reinterpret_cast<float**>(step + 14776u);

    auto set_plan = [state](std::size_t offset, float* plan) {
        *reinterpret_cast<float**>(state + offset) = plan;
    };
    if (!*reinterpret_cast<float**>(state + 368u) && *line2_plan)
        set_plan(368u, *line2_plan);
    if (!*reinterpret_cast<float**>(state + 376u) && *line3a_plan)
        set_plan(376u, *line3a_plan);
    if (!*reinterpret_cast<float**>(state + 384u) && *line3b_plan)
        set_plan(384u, *line3b_plan);
    if (!*reinterpret_cast<float**>(state + 392u) && *line4_plan)
        set_plan(392u, *line4_plan);

    bool ok = true;
    if (*reinterpret_cast<float**>(state + 368u)) {
        auto* line = reinterpret_cast<Auromatic3d3dLine2*>(state);
        if (!line->first_delay.base || !line->second_delay.base) {
            const auto plan = *reinterpret_cast<float**>(state + 368u);
            ok = auromatic_3d3d_line2_construct(line, plan, plan + 324u) && ok;
        }
    }
    if (*reinterpret_cast<float**>(state + 376u)) {
        auto* line = reinterpret_cast<Auromatic3d3dLine3*>(state + 64u);
        if (!line->first_delay.base || !line->second_delay.base) {
            const auto plan = *reinterpret_cast<float**>(state + 376u);
            ok = auromatic_3d3d_line3_construct(line, plan, plan + 726u) && ok;
        }
    }
    if (*reinterpret_cast<float**>(state + 384u)) {
        auto* line = reinterpret_cast<Auromatic3d3dLine3*>(state + 128u);
        if (!line->first_delay.base || !line->second_delay.base) {
            const auto plan = *reinterpret_cast<float**>(state + 384u);
            ok = auromatic_3d3d_line3_construct(line, plan, plan + 726u) && ok;
        }
    }
    if (*reinterpret_cast<float**>(state + 392u)) {
        auto* line = reinterpret_cast<Auromatic3d3dLine4*>(state + 192u);
        if (!line->first.delay.base || !line->second.delay.base)
            ok = auromatic_3d3d_line4_construct(
                line, *reinterpret_cast<float**>(state + 392u)) && ok;
    }
    return ok;
}

void manager_reset_upmix_3d3d(std::uint8_t* step) {
    if (!step)
        return;
    auto* state = step + 40u;
    if (*reinterpret_cast<const std::uint32_t*>(state + 344u) == 0u)
        return;
    if (*reinterpret_cast<float**>(state + 368u)) {
        auto* line = reinterpret_cast<Auromatic3d3dLine2*>(state);
        std::memset(line->filter_state, 0, sizeof(line->filter_state));
        auromatic_3d3d_delayline_clear(&line->first_delay);
        auromatic_3d3d_delayline_clear(&line->second_delay);
    }
    if (*reinterpret_cast<float**>(state + 376u)) {
        auto* line = reinterpret_cast<Auromatic3d3dLine3*>(state + 64u);
        std::memset(line->filter_state, 0, sizeof(line->filter_state));
        auromatic_3d3d_delayline_clear(&line->first_delay);
        auromatic_3d3d_delayline_clear(&line->second_delay);
    }
    if (*reinterpret_cast<float**>(state + 384u)) {
        auto* line = reinterpret_cast<Auromatic3d3dLine3*>(state + 128u);
        std::memset(line->filter_state, 0, sizeof(line->filter_state));
        auromatic_3d3d_delayline_clear(&line->first_delay);
        auromatic_3d3d_delayline_clear(&line->second_delay);
    }
    if (*reinterpret_cast<float**>(state + 392u)) {
        auto* line = reinterpret_cast<Auromatic3d3dLine4*>(state + 192u);
        std::memset(line->first.filter_state, 0, sizeof(line->first.filter_state));
        std::memset(line->second.filter_state, 0, sizeof(line->second.filter_state));
        auromatic_3d3d_delayline_clear(&line->first.delay);
        auromatic_3d3d_delayline_clear(&line->second.delay);
    }
}

// In auro_matic_v3_3d3d_configure the native rate predicate accepts the ten
// codec rates below.  A second native predicate is applied while installing
// Line2/Line3/Line4 callbacks: those callback paths are only valid when the
// rate scale is exactly 1.0 (16/22.05/24/88.2/96/176.4/192 kHz therefore may
// use the empty mode-0 configuration, but must reject callback modes).
bool auromatic_3d3d_configurable_rate(std::uint32_t sample_rate) {
    return sample_rate == 16000u || sample_rate == 22050u
        || sample_rate == 24000u || sample_rate == 32000u
        || sample_rate == 44100u || sample_rate == 48000u
        || sample_rate == 88200u || sample_rate == 96000u
        || sample_rate == 176400u || sample_rate == 192000u;
}

bool auromatic_3d3d_callback_rate(std::uint32_t sample_rate) {
    return sample_rate == 32000u || sample_rate == 44100u
        || sample_rate == 48000u;
}

constexpr std::uint32_t kAuromatic3d3dConfigured = 0x33443344u;
constexpr std::uint32_t kAuromatic3d3dConstructed = 0x33443355u;

// Native configure stores a second callback table at +312.  Its entries are
// not the audio-process entry points at +280: each process callback has a
// separate peak-amplitude callback in the decompilation.
std::uint64_t auromatic_3d3d_peak_callback(std::uint64_t process_callback) {
    switch (process_callback) {
    case 0x551330u: return 0x5513A0u;
    case 0x5513D0u: return 0x551440u;
    case 0x551480u: return 0x5514C0u;
    case 0x5514F0u: return 0x551540u;
    case 0x551580u: return 0x5515F0u;
    case 0x551630u: return 0x551680u;
    case 0x5516B0u: return 0x5516F0u;
    case 0x551720u: return 0x551760u;
    case 0x551790u: return 0x5517D0u;
    case 0x551800u: return 0x551B20u;
    default: return 0u;
    }
}

bool manager_construct_upmix_3d3d(std::uint8_t* step) {
    if (!step)
        return false;
    auto* state = step + 40u;
    // The four plan pointers are installed by p3D3D::initialize at the same
    // offsets used by native (step+448/+2808/+8792/+14776).  Preserve them
    // while constructing the 400-byte embedded p3D3D object.
    constexpr std::size_t kPlanOffsets[] = {368u, 376u, 384u, 392u};
    float* plans[4]{};
    for (std::size_t i = 0; i < 4u; ++i)
        plans[i] = *reinterpret_cast<float**>(state + kPlanOffsets[i]);
    if (*reinterpret_cast<const std::uint32_t*>(state + 400u)
        == kAuromatic3d3dConstructed)
        return true;
    std::memset(state, 0, 400u);
    for (std::size_t i = 0; i < 4u; ++i)
        *reinterpret_cast<float**>(state + kPlanOffsets[i]) = plans[i];
    if (plans[0]
        && !auromatic_3d3d_line2_construct(
            reinterpret_cast<Auromatic3d3dLine2*>(state),
            plans[0],
            plans[0] + 324u))
        return false;
    if (plans[1]
        && !auromatic_3d3d_line3_construct(
            reinterpret_cast<Auromatic3d3dLine3*>(state + 64u),
            plans[1],
            plans[1] + 726u))
        return false;
    if (plans[2]
        && !auromatic_3d3d_line3_construct(
            reinterpret_cast<Auromatic3d3dLine3*>(state + 128u),
            plans[2],
            plans[2] + 726u))
        return false;
    if (plans[3]
        && !auromatic_3d3d_line4_construct(
            reinterpret_cast<Auromatic3d3dLine4*>(state + 192u), plans[3]))
        return false;
    *reinterpret_cast<std::uint32_t*>(state + 400u) = kAuromatic3d3dConstructed;
    return true;
}

// Native fallback at libauro.so:0x5E4DE8.  It is the 14-float p3D3D
// coefficient block used when Config[4] is null.
constexpr std::array<float, 14u> kAuromatic3d3dDefaultCoefficients{
    0.912501514f, -1.904478431f, 1.0f, -1.904478431f,
    0.912501514f, 0.473151237f, 0.699842f, 0.5011872f,
    0.8035261f, 0.5011872f, 0.100000001f, 0.5011872f,
    0.100000001f, 0.316227764f};

std::int64_t manager_configure_upmix_3d3d(
    std::uint8_t* step,
    std::uint32_t sample_rate,
    const std::uint32_t* config) {
    if (!step || !config || !auromatic_3d3d_configurable_rate(sample_rate))
        return 1;
    if (!manager_construct_upmix_3d3d(step))
        return 1;
    auto* state = step + 40u;
    *reinterpret_cast<std::uint32_t*>(state + 344u) = 0u;
    const auto config_ptr = *reinterpret_cast<float* const*>(
        reinterpret_cast<const std::uint8_t*>(config) + 32u);
    *reinterpret_cast<float**>(state + 272u) = config_ptr
        ? config_ptr
        : const_cast<float*>(kAuromatic3d3dDefaultCoefficients.data());
    auto* coefficients = *reinterpret_cast<float**>(state + 272u);

    const std::uint32_t mode = config[0];
    const bool callback_rate = auromatic_3d3d_callback_rate(sample_rate);
    if ((mode == 1u || mode == 2u)
        && *reinterpret_cast<float**>(state + 368u)) {
        if (!coefficients || !callback_rate)
            return 1;
        // Native mode 2 is valid only when the third flag is clear.
        if (mode == 2u && config[2] != 0u)
            return 1;
        const bool has_secondary = config[1] != 0u;
        const auto callback = mode == 1u
            // Native 0x550FA0 initializes Line2/Line3 and replaces the
            // base callback only when flag[1] is clear.
            ? (has_secondary ? 0x551330u : 0x5513D0u)
            : (has_secondary ? 0x551480u : 0x5514F0u);
        *reinterpret_cast<std::uint64_t*>(state + 280u) = callback;
        *reinterpret_cast<std::uint32_t*>(state + 344u) = 1u;
    } else {
        if (mode != 0u)
            // Native mode 1/2 requires the Line2/Line3 state at +368.
            return 1;
        // Native mode 0 starts with no callback unless flag 1 is set.
        // Additional flags are appended below in the same order as native.
        std::uint32_t callback_count = 0u;
        if (config[1] != 0u) {
            if (!callback_rate || !*reinterpret_cast<float**>(state + 368u))
                return 1;
            *reinterpret_cast<std::uint64_t*>(state + 280u + 8u * callback_count++) = 0x551580u;
        }
        if (config[6] != 0u) {
            if (!callback_rate || !*reinterpret_cast<float**>(state + 392u))
                return 1;
            *reinterpret_cast<std::uint64_t*>(state + 280u + 8u * callback_count++) = 0x551630u;
        }
        if (config[2] != 0u) {
            if (!callback_rate || !*reinterpret_cast<float**>(state + 376u))
                return 1;
            *reinterpret_cast<std::uint64_t*>(state + 280u + 8u * callback_count++) = 0x5516B0u;
        }
        if (config[3] != 0u) {
            // Native rejects Line3 callback 3 when the later flag[5]
            // callback is requested as well (libauro(1).so.c:500026).
            if (!callback_rate || config[5] != 0u
                || !*reinterpret_cast<float**>(state + 384u))
                return 1;
            *reinterpret_cast<std::uint64_t*>(state + 280u + 8u * callback_count++) = 0x551720u;
        }
        if (config[4] != 0u) {
            if (!callback_rate || !*reinterpret_cast<float**>(state + 384u)
                || callback_count > 3u)
                return 1;
            *reinterpret_cast<std::uint64_t*>(state + 280u + 8u * callback_count++) = 0x551790u;
        }
        if (config[5] != 0u) {
            if (!callback_rate || !*reinterpret_cast<float**>(state + 384u)
                || callback_count > 3u)
                return 1;
            *reinterpret_cast<std::uint64_t*>(state + 280u + 8u * callback_count++) = 0x551800u;
        }
        if (callback_count > 4u)
            return 1;
        *reinterpret_cast<std::uint32_t*>(state + 344u) = callback_count;
    }

    // Native appends the same optional Line4/Line3 callbacks after the
    // mode-1/2 base callback.  These branches were previously unreachable in
    // the portable mode-1/2 path.
    std::uint32_t callback_count =
        *reinterpret_cast<const std::uint32_t*>(state + 344u);
    if (mode == 1u || mode == 2u) {
        if (config[6] != 0u) {
            if (!callback_rate || !*reinterpret_cast<float**>(state + 392u))
                return 1;
            *reinterpret_cast<std::uint64_t*>(state + 280u + 8u * callback_count++) = 0x551630u;
        }
        if (config[2] != 0u) {
            if (!callback_rate || !*reinterpret_cast<float**>(state + 376u))
                return 1;
            *reinterpret_cast<std::uint64_t*>(state + 280u + 8u * callback_count++) = 0x5516B0u;
        }
        if (config[3] != 0u) {
            if (!callback_rate || config[5] != 0u
                || !*reinterpret_cast<float**>(state + 384u))
                return 1;
            *reinterpret_cast<std::uint64_t*>(state + 280u + 8u * callback_count++) = 0x551720u;
        }
        if (config[4] != 0u) {
            if (!callback_rate || !*reinterpret_cast<float**>(state + 384u)
                || callback_count > 3u)
                return 1;
            *reinterpret_cast<std::uint64_t*>(state + 280u + 8u * callback_count++) = 0x551790u;
        }
        if (config[5] != 0u) {
            if (!callback_rate || !*reinterpret_cast<float**>(state + 384u)
                || callback_count > 3u)
                return 1;
            *reinterpret_cast<std::uint64_t*>(state + 280u + 8u * callback_count++) = 0x551800u;
        }
        if (callback_count > 4u)
            return 1;
        *reinterpret_cast<std::uint32_t*>(state + 344u) = callback_count;
    }

    if (mode != 0u || callback_count != 0u) {
        for (std::uint32_t i = 0u; i < callback_count; ++i)
            *reinterpret_cast<std::uint64_t*>(state + 312u + 8u * i) =
                auromatic_3d3d_peak_callback(*reinterpret_cast<const std::uint64_t*>(
                    state + 280u + 8u * i));
    }
    auro_audio_Smooth_fl32_inst_initialize(
        reinterpret_cast<float*>(state + 352u), sample_rate, 0.001f);
    auro_audio_Smooth_fl32_inst_update(
        reinterpret_cast<float*>(state + 352u), 1.0f);
    manager_reset_upmix_3d3d(step);
    *reinterpret_cast<std::uint32_t*>(state + 348u) = kAuromatic3d3dConfigured;
    return 0;
}

// p3D3D stores callback addresses in its state table.  They are native
// addresses in the decompilation, but are identifiers here: invoking the
// addresses would call into an unrelated process image.  Keep the mapping
// explicit while the callback bodies are ported one by one.
std::int64_t manager_process_upmix_3d3d(
    std::uint8_t* step,
    void** io_channels) {
    if (!step || !io_channels || !manager_initialize_upmix_3d3d(step))
        return 1;
    auto* state = step + 40u;
    auto* coefficients = *reinterpret_cast<float**>(state + 272u);
    const std::uint32_t callback_count =
        *reinterpret_cast<const std::uint32_t*>(state + 344u);
    if (!coefficients || callback_count == 0u)
        return 0;

    float gains[32]{};
    auro_audio_Smooth_fl32_inst_gains_smooth(
        reinterpret_cast<float*>(state + 352u), gains, 32u);
    float zero[32]{};
    for (std::uint32_t callback_index = 0u;
         callback_index < callback_count;
         ++callback_index) {
        const auto callback = *reinterpret_cast<const std::uint64_t*>(
            state + 280u + 8u * callback_index);
        if (callback == 0x551330u) {
            auto* first_output = static_cast<float*>(io_channels[11u]);
            auto* second_output = static_cast<float*>(io_channels[12u]);
            if (!first_output || !second_output || !io_channels[9u] || !io_channels[10u])
                return 1;
            auromatic_3d3d_line2_process_ext(
                reinterpret_cast<Auromatic3d3dLine2*>(state),
                static_cast<const float*>(io_channels[9u]),
                static_cast<const float*>(io_channels[10u]),
                zero,
                zero,
                first_output,
                second_output,
                coefficients,
                gains);
            continue;
        }
        if (callback == 0x5513D0u) {
            if (!io_channels[9u] || !io_channels[10u] || !io_channels[12u])
                return 1;
            float first_output[32]{};
            float zero[32]{};
            auromatic_3d3d_line2_process_ext(
                reinterpret_cast<Auromatic3d3dLine2*>(state),
                static_cast<const float*>(io_channels[9u]),
                static_cast<const float*>(io_channels[10u]),
                zero,
                zero,
                first_output,
                static_cast<float*>(io_channels[12u]),
                coefficients,
                gains);
            continue;
        }
        if (callback == 0x551480u) {
            auto* first_output = static_cast<float*>(io_channels[11u]);
            auto* second_output = static_cast<float*>(io_channels[12u]);
            if (!first_output || !second_output || !io_channels[9u] || !io_channels[10u]
                || !io_channels[13u] || !io_channels[14u])
                return 1;
            auromatic_3d3d_line2_process_ext(
                reinterpret_cast<Auromatic3d3dLine2*>(state),
                static_cast<const float*>(io_channels[9u]),
                static_cast<const float*>(io_channels[10u]),
                static_cast<float*>(io_channels[13u]),
                static_cast<const float*>(io_channels[14u]),
                first_output,
                second_output,
                coefficients,
                gains);
            continue;
        }
        if (callback == 0x5514F0u) {
            if (!io_channels[9u] || !io_channels[10u] || !io_channels[12u]
                || !io_channels[13u] || !io_channels[14u])
                return 1;
            float first_output[32]{};
            float second_output[32]{};
            auromatic_3d3d_line2_process_ext(
                reinterpret_cast<Auromatic3d3dLine2*>(state),
                static_cast<const float*>(io_channels[9u]),
                static_cast<const float*>(io_channels[10u]),
                static_cast<const float*>(io_channels[13u]),
                static_cast<const float*>(io_channels[14u]),
                first_output,
                static_cast<float*>(io_channels[12u]),
                coefficients,
                gains);
            continue;
        }
        if (callback == 0x551580u) {
            if (!io_channels[9u] || !io_channels[10u] || !io_channels[11u])
                return 1;
            float second_output[32]{};
            auromatic_3d3d_line2_process_ext(
                reinterpret_cast<Auromatic3d3dLine2*>(state),
                static_cast<const float*>(io_channels[9u]),
                static_cast<const float*>(io_channels[10u]),
                zero,
                zero,
                static_cast<float*>(io_channels[11u]),
                second_output,
                coefficients,
                gains);
            continue;
        }
        if (callback == 0x551630u) {
            if (!io_channels[9u] || !io_channels[10u] || !io_channels[13u]
                || !io_channels[14u] || !io_channels[28u] || !io_channels[29u])
                return 1;
            auromatic_3d3d_line4_process_ext(
                reinterpret_cast<Auromatic3d3dLine4*>(state + 192u),
                static_cast<const float*>(io_channels[9u]),
                static_cast<const float*>(io_channels[10u]),
                static_cast<const float*>(io_channels[13u]),
                static_cast<const float*>(io_channels[14u]),
                gains,
                coefficients,
                static_cast<float*>(io_channels[28u]),
                static_cast<float*>(io_channels[29u]));
            continue;
        }
        if (callback == 0x5516B0u) {
            if (!io_channels[9u] || !io_channels[10u] || !io_channels[13u]
                || !io_channels[14u])
                return 1;
            auromatic_3d3d_line3_process_ext(
                reinterpret_cast<Auromatic3d3dLine3*>(state + 64u),
                static_cast<const float*>(io_channels[9u]),
                static_cast<const float*>(io_channels[10u]),
                static_cast<float*>(io_channels[13u]),
                static_cast<float*>(io_channels[14u]),
                coefficients,
                gains);
            continue;
        }
        if (callback == 0x551790u) {
            if (!io_channels[7u] || !io_channels[8u] || !io_channels[13u]
                || !io_channels[14u])
                return 1;
            auromatic_3d3d_line3_process_ext(
                reinterpret_cast<Auromatic3d3dLine3*>(state + 128u),
                static_cast<const float*>(io_channels[13u]),
                static_cast<const float*>(io_channels[14u]),
                static_cast<float*>(io_channels[7u]),
                static_cast<float*>(io_channels[8u]),
                coefficients,
                gains);
            continue;
        }
        if (callback == 0x551800u) {
            if (!io_channels[4u] || !io_channels[5u] || !io_channels[6u])
                return 1;
            float first_output[32u]{};
            float second_output[32u]{};
            auromatic_3d3d_line3_process_ext(
                reinterpret_cast<Auromatic3d3dLine3*>(state + 128u),
                static_cast<const float*>(io_channels[4u]),
                static_cast<const float*>(io_channels[5u]),
                first_output,
                second_output,
                coefficients,
                gains);
            auto* mono_top = static_cast<float*>(io_channels[6u]);
            for (std::uint32_t i = 0u; i < 32u; ++i)
                mono_top[i] = (first_output[i] + second_output[i]) * 0.5f;
            continue;
        }
        if (callback == 0x551720u) {
            if (!io_channels[4u] || !io_channels[5u] || !io_channels[7u]
                || !io_channels[8u])
                return 1;
            auromatic_3d3d_line3_process_ext(
                reinterpret_cast<Auromatic3d3dLine3*>(state + 128u),
                static_cast<const float*>(io_channels[4u]),
                static_cast<const float*>(io_channels[5u]),
                static_cast<float*>(io_channels[7u]),
                static_cast<float*>(io_channels[8u]),
                coefficients,
                gains);
            continue;
        }
        return 1;
    }
    return 0;
}

bool manager_partial_clear_upmix_3d3d(
    std::uint8_t* step,
    std::uint32_t* remaining) {
    if (!step || !remaining)
        return false;
    auto* state = step + 40u;
    bool complete = true;
    if (*reinterpret_cast<float**>(state + 368u))
        complete = auromatic_3d3d_line2_partial_clear(
            reinterpret_cast<Auromatic3d3dLine2*>(state), remaining) && complete;
    if (*reinterpret_cast<float**>(state + 376u))
        complete = auromatic_3d3d_line3_partial_clear(
            reinterpret_cast<Auromatic3d3dLine3*>(state + 64u), remaining) && complete;
    if (*reinterpret_cast<float**>(state + 384u))
        complete = auromatic_3d3d_line3_partial_clear(
            reinterpret_cast<Auromatic3d3dLine3*>(state + 128u), remaining) && complete;
    if (*reinterpret_cast<float**>(state + 392u))
        complete = auromatic_3d3d_line4_partial_clear(
            reinterpret_cast<Auromatic3d3dLine4*>(state + 192u), remaining) && complete;
    return complete;
}

// Native auro_matic_v3_3d3d_update_peak_amplitude @ 0x551BF0 walks the
// peak-callback table independently of the audio-process table.
void manager_update_upmix_3d3d_peak(
    std::uint8_t* step,
    std::uint8_t* info_base) {
    if (!step || !info_base)
        return;
    auto* state = step + 40u;
    const auto* coefficients = reinterpret_cast<const float*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<const std::uint64_t*>(state + 272u)));
    const std::uint32_t count = *reinterpret_cast<const std::uint32_t*>(state + 344u);
    if (!coefficients)
        return;
    // The native peak callbacks receive the original info record as a2.
    // Their byte offsets (a2+36, a2+44, ...) and indexed accesses (a2[28],
    // a2[29]) therefore start at info_base itself.  Do not use the +4 offset
    // of the amplitude subarray here: that would shift every peak channel.
    auto at = [&](std::uint32_t byte_offset) {
        return reinterpret_cast<float*>(info_base + byte_offset);
    };
    auto* values = reinterpret_cast<float*>(info_base);

    for (std::uint32_t i = 0u; i < count && i < 4u; ++i) {
        // Native p3D3D keeps a separate peak-callback table at +312. Store
        // semantic callback identifiers there during configure and dispatch
        // that table independently from the audio-process table.
        const std::uint64_t callback =
            *reinterpret_cast<const std::uint64_t*>(state + 312u + 8u * i);
        if (callback == 0x5513A0u || callback == 0x551440u
            || callback == 0x5514C0u || callback == 0x551540u
            || callback == 0x5515F0u) {
            const float a = *at(36u);
            const float b = *at(40u);
            const float c = (callback == 0x5514C0u || callback == 0x551540u)
                ? *at(52u) : 0.0f;
            const float d = (callback == 0x5514C0u || callback == 0x551540u)
                ? *at(56u) : 0.0f;
            float first = 0.0f;
            float second = 0.0f;
            auromatic_3d3d_line2_peak_amplitude(
                &first, &second, coefficients, a, b, c, d);
            if (callback != 0x551440u && callback != 0x551540u)
                *at(44u) = first;
            if (callback != 0x5515F0u)
                *at(48u) = second;
        } else if (callback == 0x551680u) {
            auromatic_3d3d_line4_peak_amplitude(
                values + 28u, values + 29u, coefficients,
                values[9u], values[10u], values[13u], values[14u]);
        } else if (callback == 0x5516F0u) {
            auromatic_3d3d_line3_peak_amplitude(
                at(52u), at(56u), coefficients, *at(36u), *at(40u));
        } else if (callback == 0x551760u || callback == 0x5517D0u) {
            const std::uint32_t base = callback == 0x551760u ? 16u : 52u;
            auromatic_3d3d_line3_peak_amplitude(
                at(28u), at(32u), coefficients, *at(base), *at(base + 4u));
        } else if (callback == 0x551B20u) {
            float first = 0.0f;
            float second = 0.0f;
            auromatic_3d3d_line3_peak_amplitude(
                &first, &second, coefficients,
                *at(16u), *at(20u));
            *at(6u) = (first + second) * 0.5f;
        }
    }
}

// Native Engine_configure calls these two small state configurators directly
// after the DSP stages. Keep the channel enumeration in one place so a
// reconfigure cannot leave the step's cached channel list stale.
void manager_configure_upmix_gain_silence(
    std::uint8_t* step_chain,
    const std::uint32_t* manager_params) {
    if (!step_chain || !manager_params)
        return;
    for (const auto& step : kManagerSteps478078) {
        const auto process_fn = *reinterpret_cast<const std::uint64_t*>(
            step_chain + step.process_fn);
        auto* state = step_chain + step.base;
        if (process_fn == ::auro_engine_v4_ida::kStepUpmixGain_process) {
            const std::uint32_t mask = manager_params[47u];
            std::uint32_t count = 0u;
            for (std::uint32_t id = 0; id < 31u; ++id) {
                if ((mask & (1u << id)) == 0u)
                    continue;
                if (count < 31u)
                    *reinterpret_cast<std::uint32_t*>(state + 48u + count * 4u) = id;
                ++count;
            }
            *reinterpret_cast<std::uint32_t*>(state + 40u) =
                std::min(count, 31u);
        } else if (process_fn == ::auro_engine_v4_ida::kStepUpmixSilence_process) {
            // Silence::prepare normally derives the two masks from the
            // current block layout.  Preserve the configured native silence
            // mask here; the per-block prepare path may replace +40.
            *reinterpret_cast<std::uint32_t*>(state + 36u) = manager_params[46u];
        }
    }
}

std::int64_t manager_calculate_info_step_chain(
    std::uint8_t* step_chain,
    std::uint8_t* info_base) {
    if (!step_chain || !info_base)
        return 1;
    for (const auto& step : kManagerSteps478078) {
        if (*reinterpret_cast<const std::uint64_t*>(step_chain + step.block_state) == 0u)
            continue;
        const auto process_fn = *reinterpret_cast<const std::uint64_t*>(
            step_chain + step.process_fn);
        if (process_fn == ::auro_engine_v4_ida::kStepDownSample_process) {
            // Native sub_4D23D0: DownSample::update(*peak) = (*peak + 13) / 2.
            auto* peak = reinterpret_cast<float*>(info_base + 4u);
            *peak = (*peak + 13.0f) * 0.5f;
        } else if (process_fn == ::auro_engine_v4_ida::kStepUpSample_process) {
            // Native sub_4CF7F0: UpSample::update(*peak) = 2 * *peak + 13.
            auto* peak = reinterpret_cast<float*>(info_base + 4u);
            *peak = *peak + *peak + 13.0f;
        } else if (process_fn == ::auro_engine_v4_ida::kStepDelay_process) {
            // Native sub_4D5E20: Delay::update(*peak) adds the configured
            // sample delay stored at step+1688.
            auto* peak = reinterpret_cast<float*>(info_base + 4u);
            *peak += static_cast<float>(*reinterpret_cast<const std::int32_t*>(
                step_chain + step.base + 1688u));
        } else if (process_fn == ::auro_engine_v4_ida::kStepDownmix_process) {
            (void)manager_update_peak_downmixer(
                step_chain + step.base,
                reinterpret_cast<float*>(info_base + 4u));
        } else if (process_fn == ::auro_engine_v4_ida::kStepAlt3DStore_process) {
            (void)manager_update_peak_alt3d_store(
                step_chain + step.base,
                reinterpret_cast<float*>(info_base + 4u));
        } else if (process_fn == ::auro_engine_v4_ida::kStepAlt3DMix_process) {
            (void)manager_update_peak_alt3d_mix(
                step_chain + step.base,
                reinterpret_cast<float*>(info_base + 4u));
        } else if (process_fn == ::auro_engine_v4_ida::kStepUpmix3D3D_process)
            manager_update_upmix_3d3d_peak(step_chain + step.base, info_base);
        if (process_fn == ::auro_engine_v4_ida::kStepBassManagement_process)
            (void)manager_update_peak_bass_management(
                step_chain + step.base, reinterpret_cast<float*>(info_base + 4u));
        if (process_fn == ::auro_engine_v4_ida::kStepLegacy_process)
            (void)manager_update_peak_legacy_step(
                step_chain + step.base, reinterpret_cast<float*>(info_base + 4u));
        if (process_fn == ::auro_engine_v4_ida::kStepChannelTest_process)
            (void)manager_update_peak_channel_test_step(
                step_chain + step.base, reinterpret_cast<float*>(info_base + 4u));
        if (process_fn == ::auro_engine_v4_ida::kStepScene_process)
            (void)manager_update_peak_scene_step(
                step_chain + step.base, reinterpret_cast<float*>(info_base + 4u));
    }
    return 0;
}

// sub_4CC230: refresh the Manager's 15-channel gain table after plan
// activation. The native routine seeds the info record from the layout,
// lets active step peak callbacks replace the values, then stores the
// logarithmic table and active-channel mask used by the next update.
std::int64_t manager_refresh_info_native(std::uint8_t* manager) {
    if (!manager)
        return 1;
    auto* step_chain = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(
        *reinterpret_cast<const std::uint64_t*>(manager + kManagerOffStepChain478078)));
    if (!step_chain)
        return 1;
    std::array<std::uint8_t, 64u> info{};
    auto* amplitudes = reinterpret_cast<float*>(info.data() + 4u);
    const auto layout = *reinterpret_cast<const std::uint32_t*>(manager + 8u);
    for (std::uint32_t id = 0u; id < 15u; ++id)
        amplitudes[id] = (layout & (1u << id)) != 0u ? 1.0f : 0.0f;
    if (manager_calculate_info_step_chain(step_chain, info.data()) != 0)
        return 1;
    std::uint32_t active_mask = 0u;
    bool gain_table_changed = false;
    for (std::uint32_t id = 0u; id < 15u; ++id) {
        const float amplitude = amplitudes[id];
        if (amplitude > 0.0f)
            active_mask |= 1u << id;
        const float clamped = std::max(amplitude, 1.0f);
        const float db = std::max(std::log10(clamped) * 20.0f, -144.0f);
        auto* cached_db = reinterpret_cast<float*>(manager + 496u) + id;
        if (std::fabs(*cached_db - db) > 0.0000099999997f) {
            *cached_db = db;
            gain_table_changed = true;
        }
    }
    *reinterpret_cast<std::uint32_t*>(manager + 556u) = active_mask;
    auto* listener = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(
        *reinterpret_cast<const std::uint64_t*>(manager + 856u)));
    if (gain_table_changed && listener
        && *reinterpret_cast<const std::uint64_t*>(listener + 8u) != 0u) {
        using NotifyFn = void (*)(std::uint64_t, std::int64_t);
        const auto notify = reinterpret_cast<NotifyFn>(
            static_cast<std::uintptr_t>(
                *reinterpret_cast<const std::uint64_t*>(listener + 8u)));
        notify(reinterpret_cast<std::uint64_t>(listener), 2);
    }
    const float current_headroom =
        *reinterpret_cast<const float*>(manager + 560u);
    if (current_headroom != *reinterpret_cast<const float*>(manager + 492u)) {
        *reinterpret_cast<float*>(manager + 492u) = current_headroom;
        if (listener && *reinterpret_cast<const std::uint64_t*>(listener + 8u) != 0u) {
            using NotifyFn = void (*)(std::uint64_t, std::int64_t);
            const auto notify = reinterpret_cast<NotifyFn>(
                static_cast<std::uintptr_t>(
                    *reinterpret_cast<const std::uint64_t*>(listener + 8u)));
            notify(reinterpret_cast<std::uint64_t>(listener), 4);
        }
    }
    *reinterpret_cast<std::uint32_t*>(manager + kManagerOffHeadroomRecalcDirty478078) = 0u;
    return 0;
}

bool manager_partial_clear_step_chain(
    std::uint8_t* step_chain,
    std::uint32_t* remaining) {
    if (!step_chain || !remaining)
        return false;
    // The native XinN step owns the fl32 partial-clear entry point at its
    // embedded state (+40).  Unlike the ordinary Engine wrapper, XinN must
    // clear Early, Late, and its smoothing state together, and only resets
    // them after all pending samples have been emitted.
    extern std::int64_t auro_matic_XinN_fl32_partial_clear(
        std::uint8_t* xinn_state,
        std::uint32_t* remaining);
    bool complete = true;
    for (const auto& step : kManagerSteps478078) {
        if (*reinterpret_cast<const std::uint64_t*>(step_chain + step.block_state) == 0u)
            continue;
        const auto process_fn = *reinterpret_cast<const std::uint64_t*>(
            step_chain + step.process_fn);
        if (process_fn == ::auro_engine_v4_ida::kStepUpmixXinN_process) {
            complete = auro_matic_XinN_fl32_partial_clear(
                step_chain + step.base + 40u, remaining) != 0
                && complete;
        } else if (process_fn == ::auro_engine_v4_ida::kStepUpmix3D3D_process) {
            complete = manager_partial_clear_upmix_3d3d(
                step_chain + step.base, remaining) && complete;
        }
    }
    return complete;
}

std::int64_t manager_process_peak_limiter(std::uint8_t* step, void** io_channels);
std::int64_t manager_configure_peak_limiter_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry,
    bool first_time,
    std::uint8_t* block_state);
void manager_reset_peak_limiter(std::uint8_t* step);
std::int64_t manager_process_sample_convertor(std::uint8_t* step, void** io_channels);
std::int64_t manager_configure_sample_convertor_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry,
    bool first_time,
    std::uint8_t* block_state);
std::int64_t manager_configure_downsample_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry,
    bool first_time,
    std::uint8_t* block_state);
std::int64_t manager_configure_upsample_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry,
    bool first_time,
    std::uint8_t* block_state);
std::int64_t manager_process_downsample(std::uint8_t* step, void** io_channels);
std::int64_t manager_process_upsample(std::uint8_t* step, void** io_channels);
void manager_construct_downsample_step(std::uint8_t* step);
void manager_construct_upsample_step(std::uint8_t* step);

void /* Decompiled name: manager_reset_step_chain */
manager_reset_step_chain(std::uint8_t* step_chain) {
    if (!step_chain)
        return;
    for (const auto& step : kManagerSteps478078) {
        const std::uint64_t block_state = *reinterpret_cast<const std::uint64_t*>(step_chain + step.block_state);
        if (block_state == 0u)
            continue;
        const auto process_fn = *reinterpret_cast<const std::uint64_t*>(step_chain + step.process_fn);
        if (process_fn == ::auro_codec_v3_ida::kAuroA3dengV3PipelineStepUpmix_process) {
            auto* engine = step_chain + step.base + 264u;
            AuromaticV3EngineObjectParts parts{};
            parts.output_gate = engine;
            parts.front_manager = engine + 2152u;
            parts.xinn = engine + 3344u;
            parts.p3d3d = engine + 4192u;
            parts.gain = engine + 4592u;
            parts.silence = engine + 4720u;
            (void)auro_matic_v3_Engine_reset_audio_state_objects(parts);
            continue;
        }
        if (process_fn == ::auro_engine_v4_ida::kStepUpmix3D3D_process) {
            manager_reset_upmix_3d3d(step_chain + step.base);
            continue;
        }
        if (process_fn == ::auro_engine_v4_ida::kStepBassManagement_process) {
            manager_reset_bass_management_step(step_chain + step.base);
            continue;
        }
        if (process_fn == ::auro_engine_v4_ida::kStepLegacy_process) {
            manager_reset_legacy_step(step_chain + step.base);
            continue;
        }
        if (process_fn == ::auro_engine_v4_ida::kStepDelay_process) {
            manager_reset_delay_step(step_chain + step.base);
            continue;
        }
        if (process_fn == ::auro_engine_v4_ida::kStepAlt3DStore_process) {
            manager_reset_alt3d_store_step(step_chain + step.base);
            continue;
        }
        if (process_fn == ::auro_engine_v4_ida::kStepAlt3DMix_process) {
            manager_reset_alt3d_mix_step(step_chain + step.base);
            continue;
        }
        if (process_fn == ::auro_engine_v4_ida::kStepSilence_process) {
            manager_reset_silence_step(step_chain + step.base);
            continue;
        }
        if (process_fn == ::auro_engine_v4_ida::kStepChannelTest_process) {
            manager_reset_channel_test_step(step_chain + step.base);
            continue;
        }
        if (process_fn == ::auro_engine_v4_ida::kStepScene_process) {
            manager_reset_scene_step(step_chain + step.base);
            continue;
        }
        if (process_fn == ::auro_engine_v4_ida::kStepGate_process
            || process_fn == ::auro_engine_v4_ida::kStepGate_process_current) {
            // Gate::reset_audio_state @ 0x4D1EB0 writes unity to +64/+68.
            *reinterpret_cast<std::uint64_t*>(step_chain + step.base + 64u) =
                0x3F8000003F800000ull;
            continue;
        }
        if (process_fn == ::auro_engine_v4_ida::kStepPeakLimiter_process) {
            manager_reset_peak_limiter(step_chain + step.base);
            continue;
        }
        if (process_fn == ::auro_engine_v4_ida::kStepDownSample_process) {
            matic_resample::clear_down(matic_resample::object_at(
                step_chain + step.base));
            continue;
        }
        if (process_fn == ::auro_engine_v4_ida::kStepUpSample_process) {
            matic_resample::clear_up(matic_resample::object_at(
                step_chain + step.base));
            continue;
        }
        if (process_fn == ::auro_engine_v4_ida::kStepDownmix_process) {
            (void)auro_matic_Silence_reset_audio_state_object(
                step_chain + step.base + 40u + 632u);
            continue;
        }
        using ResetFn = void (*)(std::uint64_t);
        const auto fn = reinterpret_cast<ResetFn>(*reinterpret_cast<const std::uint64_t*>(step_chain + step.reset_fn));
        if (fn)
            fn(reinterpret_cast<std::uint64_t>(step_chain + step.base));
    }
}

void manager_invalidate_upmix_3d3d_config(std::uint8_t* step_chain) {
    if (!step_chain)
        return;
    for (const auto& step : kManagerSteps478078) {
        if (*reinterpret_cast<const std::uint64_t*>(step_chain + step.process_fn)
            == ::auro_engine_v4_ida::kStepUpmix3D3D_process) {
            *reinterpret_cast<std::uint32_t*>(
                step_chain + step.base + 40u + 348u) = 0u;
        }
    }
}

// Native pipeline Upmix::process @ 0x4CF4B0 invokes the Engine once per
// fixed 32-sample span.  The object is embedded at +264 in the outer step;
// keep the adapter here so a populated native manager chain does not fall
// through to an incompatible generic callback ABI.
std::int64_t manager_process_upmix_outer(
    std::uint8_t* step,
    void** io_channels) {
    if (!step || !io_channels)
        return 1;
    const std::uint64_t block_state = *reinterpret_cast<const std::uint64_t*>(step + 32u);
    if (!block_state)
        return 0;
    const std::uint32_t subblocks = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(block_state) + 4u);
    auto* engine = step + 264u;
    // Upmix_t_process receives the manager-level bypass decision through its
    // own +5124 field and forwards it to Engine_set_bypass before iterating
    // the 32-sample blocks.  The weak native gain updater has no portable
    // counterpart, but bypass is local state and must not be dropped.
    auro_matic_v3_OutputGate_set_bypass(
        engine,
        *reinterpret_cast<const std::uint32_t*>(step + 5124u));
    AuromaticV3EngineObjectParts parts{};
    parts.output_gate = engine;
    parts.front_manager = engine + 2152u;
    parts.xinn = engine + 3344u;
    parts.p3d3d = engine + 4192u;
    parts.gain = engine + 4592u;
    parts.silence = engine + 4720u;
    parts.output_masks = reinterpret_cast<std::uint32_t*>(engine + 4848u);
    for (std::uint32_t block = 0u; block < subblocks; ++block) {
        std::array<float*, 31u> channels{};
        const std::size_t offset = static_cast<std::size_t>(block) * 32u;
        for (std::size_t ch = 0u; ch < 31u; ++ch) {
            auto* base = static_cast<float*>(io_channels[ch]);
            channels[ch] = base ? base + offset : nullptr;
        }
        if (auro_matic_v3_Engine_process_objects(parts, channels.data()) != 0)
            return 1;
    }
    return 0;
}

std::int64_t manager_process_gate(std::uint8_t* gate, void** io_channels);
std::int64_t manager_process_headroom(std::uint8_t* headroom, void** io_channels);

std::int64_t /* Decompiled name: manager_process_step_chain */
manager_process_step_chain(
    std::uint8_t* manager,
    std::uint8_t* step_chain,
    void** io_channels) {
    if (!manager || !step_chain || !io_channels)
        return 1;
    const bool v3_engine_upmix_active =
        *reinterpret_cast<const std::uint64_t*>(step_chain + 7944u + 16u)
            == ::auro_codec_v3_ida::kAuroA3dengV3PipelineStepUpmix_process;
    for (const auto& step : kManagerSteps478078) {
        const std::uint64_t block_state = *reinterpret_cast<const std::uint64_t*>(step_chain + step.block_state);
        if (block_state == 0u)
            continue;
        const std::uint64_t fn_u64 = *reinterpret_cast<const std::uint64_t*>(step_chain + step.process_fn);
        if (fn_u64 == 0u)
            continue;
        // The v3 Upmix outer step owns FrontManager, XinN, p3D3D, Gain, and
        // Silence in one Engine_process call. The similarly named legacy
        // chain entries are an alternative v4 graph; running them afterwards
        // applies the newly configured Gain/Silence lanes a second time.
        if (v3_engine_upmix_active
            && (fn_u64 == ::auro_engine_v4_ida::kStepUpmix3D3D_process
                || fn_u64 == ::auro_engine_v4_ida::kStepUpmixXinN_process
                || fn_u64 == ::auro_engine_v4_ida::kStepUpmixInput_process
                || fn_u64 == ::auro_engine_v4_ida::kStepUpmixGain_process
                || fn_u64 == ::auro_engine_v4_ida::kStepUpmixSilence_process
                || fn_u64 == ::auro_engine_v4_ida::kStepUpmixFrontManagerInput_process
                || fn_u64 == ::auro_engine_v4_ida::kStepUpmixFrontManagerOutput_process))
            continue;
        if (fn_u64 == ::auro_codec_v3_ida::kAuroA3dengV3PipelineStepUpmix_process) {
            const auto rc = manager_process_upmix_outer(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepUpmix3D3D_process) {
            auto* step_bytes = step_chain + step.base;
            auto* state = step_bytes + 40u;
            if (*reinterpret_cast<const std::uint32_t*>(state + 348u)
                != kAuromatic3d3dConfigured) {
                const auto block_state = *reinterpret_cast<const std::uint64_t*>(step_bytes + 8u);
                const auto sample_rate = block_state == 0u
                    ? 0u
                    : *reinterpret_cast<const std::uint32_t*>(
                        static_cast<std::uintptr_t>(block_state + 8u));
                const auto configure_rc = manager_configure_upmix_3d3d(
                    step_bytes,
                    sample_rate,
                    reinterpret_cast<const std::uint32_t*>(manager + kManagerOffParams478078));
                if (configure_rc != 0)
                    return configure_rc;
            }
            const auto rc = manager_process_upmix_3d3d(step_bytes, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
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
            const std::int64_t rc = auro_a3deng_v4_pipeline_step_upmix_XinN_process_impl(
                step_base,
                io_channels,
                subblock_count,
                records,
                nullptr,
                auro_a3deng_v4_pipeline_step_upmix_XinN_reset_audio_state_impl);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepUpmixInput_process) {
            const auto rc = manager_process_upmix_input(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepUpmixGain_process) {
            const auto rc = manager_process_upmix_gain(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepUpmixSilence_process) {
            const auto rc = manager_process_upmix_silence(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepUpmixFrontManagerInput_process) {
            const auto rc = manager_process_upmix_front_manager_input(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepUpmixFrontManagerOutput_process) {
            const auto rc = manager_process_upmix_front_manager_output(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepHeadroom_process
            || fn_u64 == ::auro_engine_v4_ida::kStepHeadroom_process_direct
            || fn_u64 == ::auro_engine_v4_ida::kStepHeadroom_process_smooth) {
            const auto rc = manager_process_headroom(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepGate_process
            || fn_u64 == ::auro_engine_v4_ida::kStepGate_process_current) {
            const auto rc = manager_process_gate(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepPeakLimiter_process) {
            const auto rc = manager_process_peak_limiter(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepSampleConvertor_process_float_clamp
            || fn_u64 == ::auro_engine_v4_ida::kStepSampleConvertor_process_nop) {
            const auto rc = manager_process_sample_convertor(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepDownSample_process) {
            const auto rc = manager_process_downsample(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepUpSample_process) {
            const auto rc = manager_process_upsample(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepBassManagement_process) {
            const auto rc = manager_process_bass_management(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepLegacy_process) {
            const auto rc = manager_process_legacy_step(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepDelay_process) {
            const auto rc = manager_process_delay_step(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepAlt3DStore_process) {
            const auto rc = manager_process_alt3d_store_step(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepAlt3DMix_process) {
            const auto rc = manager_process_alt3d_mix_step(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepSilence_process) {
            const auto rc = manager_process_silence_step(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepChannelTest_process) {
            const auto rc = manager_process_channel_test_step(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepScene_process) {
            const auto rc = manager_process_scene_step(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepDownmix_process) {
            const auto rc = manager_process_downmixer(
                step_chain + step.base, io_channels);
            if (rc != 0)
                return rc;
            continue;
        }
        if (fn_u64 == ::auro_engine_v4_ida::kStepHeadphones_process)
            // The native AHP/AM4HP processor is not portable yet. Do not
            // silently discard an active Headphones stage.
            return 1;
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

void /* Decompiled name: manager_update_headroom_input_gains */
manager_update_headroom_input_gains(std::uint8_t* manager) {
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
    // Native dB inputs for pipeline Headroom start at step+64 (15 channels).
    auto* headroom_gains = reinterpret_cast<float*>(step_chain + 15800u + 64u);
    for (std::uint32_t ch = 0; ch < 15u; ++ch)
        headroom_gains[ch] = ((output_mask >> ch) & 1u) != 0u ? -gains[ch] : -0.0f;

    (void)auro_a3deng_v3_Headroom_recalculate_gains(step_chain + 15800u);
    *reinterpret_cast<std::uint32_t*>(manager + kManagerOffHeadroomDirty478078) = 0u;
}

// Native pipeline::step::Gate::process @ 0x4D1BE0. Mask/blocks live in the
// linked metadata at +40/+44; ramp endpoints are at +64/+68. The SIMD body
// advances the local current value but does not write it back.
std::int64_t manager_process_gate(
    std::uint8_t* gate,
    void** io_channels) {
    if (!gate || !io_channels)
        return 1;
    const std::uint32_t mask =
        *reinterpret_cast<const std::uint32_t*>(gate + 40u);
    const std::uint32_t blocks =
        *reinterpret_cast<const std::uint32_t*>(gate + 44u);
    float current = *reinterpret_cast<const float*>(gate + 64u);
    const float target = *reinterpret_cast<const float*>(gate + 68u);
    if (current == 1.0f && target == 1.0f)
        return 0;
    if (blocks == 0u)
        return 0;

    const float block_delta = (target - current) /
        static_cast<float>(blocks);
    for (std::uint32_t block = 0u; block < blocks; ++block) {
        const float block_start = current;
        current += block_delta;
        const float sample_delta = (current - block_start) * 0.03125f;
        const std::size_t sample_offset = static_cast<std::size_t>(block) * 32u;
        for (std::uint32_t id = 0u; id < 31u; ++id) {
            if ((mask & (1u << id)) == 0u || io_channels[id] == nullptr)
                continue;
            auto* samples = static_cast<float*>(io_channels[id]) + sample_offset;
            float sample_gain = block_start + sample_delta;
            for (std::uint32_t i = 0u; i < 32u; ++i) {
                samples[i] *= sample_gain;
                sample_gain += sample_delta;
            }
        }
    }
    return 0;
}

// Native Headroom::process_direct @ 0x4D1470. Per-channel linear gains start at
// +124; mask/blocks are the metadata dwords at +40/+44.
std::int64_t manager_process_headroom(
    std::uint8_t* headroom,
    void** io_channels) {
    if (!headroom || !io_channels)
        return 1;
    const std::uint32_t mask =
        *reinterpret_cast<const std::uint32_t*>(headroom + 40u);
    const std::uint32_t blocks =
        *reinterpret_cast<const std::uint32_t*>(headroom + 44u);
    const std::uint32_t sample_count = blocks * 32u;
    if (sample_count == 0u)
        return 0;
    for (std::uint32_t id = 0u; id < 15u; ++id) {
        if ((mask & (1u << id)) == 0u || io_channels[id] == nullptr)
            continue;
        const float gain =
            *reinterpret_cast<const float*>(headroom + 124u + id * 4u);
        auto* samples = static_cast<float*>(io_channels[id]);
        for (std::uint32_t i = 0u; i < sample_count; ++i)
            samples[i] *= gain;
    }
    return 0;
}

// PeakLimiter::configure @ 0x4D4FD0 + process @ 0x4D5330. Reuses the linked
// float32 NativePeakLimiter (prepare coeffs from 0x360950) embedded at +128.
constexpr std::uint32_t kPeakLimiterEmbedMagic = 0x504C4632u; // 'PLF2'

struct PeakLimiterEmbed {
    std::uint32_t magic = 0u;
    std::uint32_t channel_count = 0u;
    auro3d::NativePeakLimiter limiter{};
};

PeakLimiterEmbed* peak_limiter_embed(std::uint8_t* step) {
    return reinterpret_cast<PeakLimiterEmbed*>(step + 128u);
}

std::int64_t manager_configure_peak_limiter_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry,
    bool first_time,
    std::uint8_t* block_state) {
    (void)first_time;
    (void)plan_entry;
    (void)block_state;
    if (!step)
        return 1;
    const auto* bound = reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(
            *reinterpret_cast<const std::uint64_t*>(step + 32u)));
    if (!bound || bound[3] != 0u)
        return 1;
    const std::uint32_t layout = bound[0];
    std::uint32_t count = 0u;
    *reinterpret_cast<std::uint32_t*>(step + 124u) = 0u;
    for (std::uint32_t bit = 0u; bit < 15u; ++bit) {
        if ((layout & (1u << bit)) == 0u)
            continue;
        *reinterpret_cast<std::uint32_t*>(step + 64u + 4u * count) = bit;
        ++count;
        *reinterpret_cast<std::uint32_t*>(step + 124u) = count;
    }
    constexpr std::uint32_t kForcedChannels = 1u;
    const std::uint32_t memsize =
        ((kForcedChannels == 0u) + kForcedChannels) * 256u
        + 8u * kForcedChannels
        + 8u * ((kForcedChannels == 0u) + kForcedChannels) + 117u;
    if (memsize > 0x1064u)
        return 1;
    auto* embed = peak_limiter_embed(step);
    if (embed->magic == kPeakLimiterEmbedMagic)
        embed->~PeakLimiterEmbed();
    new (embed) PeakLimiterEmbed{};
    embed->magic = kPeakLimiterEmbedMagic;
    embed->channel_count = count;
    embed->limiter.set_sample_rate(bound[2]);
    embed->limiter.reset();
    std::memcpy(step + 40u, bound, 16u);
    *reinterpret_cast<std::uint32_t*>(step + 56u) =
        *reinterpret_cast<const std::uint32_t*>(
            reinterpret_cast<const std::uint8_t*>(bound) + 16u);
    return 0;
}

std::int64_t manager_process_peak_limiter(
    std::uint8_t* step,
    void** io_channels) {
    if (!step || !io_channels)
        return 1;
    auto* embed = peak_limiter_embed(step);
    if (embed->magic != kPeakLimiterEmbedMagic)
        return 1;
    const auto* pred = reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(
            *reinterpret_cast<const std::uint64_t*>(step + 32u)));
    if (!pred)
        return 1;
    const std::uint32_t sample_count = pred[1] * 32u;
    if (sample_count == 0u)
        return 0;
    const std::uint32_t count = *reinterpret_cast<const std::uint32_t*>(step + 124u);
    if (count == 0u)
        return 0;
    std::array<float*, 15u> gathered{};
    std::uint32_t gathered_count = 0u;
    for (std::uint32_t i = 0u; i < count && gathered_count < gathered.size(); ++i) {
        const std::uint32_t id =
            *reinterpret_cast<const std::uint32_t*>(step + 64u + 4u * i);
        if (id >= 31u || io_channels[id] == nullptr)
            continue;
        gathered[gathered_count++] = static_cast<float*>(io_channels[id]);
    }
    if (gathered_count == 0u)
        return 0;
    // Float32 Manager path: no PCM24 ceiling (that is export-only).
    embed->limiter.process_planar(
        gathered.data(), gathered_count, sample_count, false);
    return 0;
}

void manager_reset_peak_limiter(std::uint8_t* step) {
    if (!step)
        return;
    auto* embed = peak_limiter_embed(step);
    if (embed->magic == kPeakLimiterEmbedMagic)
        embed->limiter.reset();
}

// SampleConvertor::configure @ 0x4CF840. For the Float32 Manager path the
// active plan installs float±1 clamp (0x4CF930) when output_bytes_unit != 0.
std::uint32_t sample_convertor_bitdepth_for_type(std::uint32_t sample_type) {
    return sample_type == 1u ? 24u : 0u;
}

std::int64_t manager_configure_sample_convertor_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry,
    bool first_time,
    std::uint8_t* block_state) {
    (void)first_time;
    (void)block_state;
    if (!step || !plan_entry)
        return 1;
    const auto* bound = reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(
            *reinterpret_cast<const std::uint64_t*>(step + 32u)));
    if (!bound)
        return 1;
    const std::uint32_t pred_type = bound[3];
    if (pred_type != plan_entry[1])
        return 1;
    std::uint64_t process_fn = 0u;
    std::uint32_t out_type = 0u;
    if (pred_type == 1u) {
        if (bound[4] != 24u)
            return 1;
        const std::uint32_t converter = plan_entry[2];
        const std::uint32_t bytes_unit = plan_entry[3];
        if (converter != 1u)
            return 1;
        // PCM24 predecessor stays PCM24.  Native selects 0x4D0110 for a
        // plain int32->float conversion or 0x4D04F0 for the clamped form;
        // those callbacks are not the Float32->PCM24 output callbacks below.
        process_fn = bytes_unit == 0u
            ? 0x4D0110u
            : 0x4D04F0u;
        out_type = 1u;
        *reinterpret_cast<std::uint64_t*>(step + 16u) = process_fn;
        *reinterpret_cast<std::uint32_t*>(step + 52u) = out_type;
        *reinterpret_cast<std::uint32_t*>(step + 56u) =
            sample_convertor_bitdepth_for_type(out_type);
        return 0;
    }
    if (pred_type != 0u)
        return 1;
    const std::uint32_t converter = plan_entry[2];
    const std::uint32_t bytes_unit = plan_entry[3];
    if (converter == 1u) {
        process_fn = bytes_unit == 0u
            ? ::auro_engine_v4_ida::kStepSampleConvertor_process_pcm24
            : ::auro_engine_v4_ida::kStepSampleConvertor_process_pcm24_sat;
        out_type = 1u;
        *reinterpret_cast<std::uint64_t*>(step + 16u) = process_fn;
        *reinterpret_cast<std::uint32_t*>(step + 52u) = out_type;
        *reinterpret_cast<std::uint32_t*>(step + 56u) =
            sample_convertor_bitdepth_for_type(out_type);
        return 0;
    }
    if (converter != 0u)
        return 1;
    // converter==0: float stay. bytes_unit==0 → nop; else ±1 clamp.
    process_fn = bytes_unit == 0u
        ? ::auro_engine_v4_ida::kStepSampleConvertor_process_nop
        : ::auro_engine_v4_ida::kStepSampleConvertor_process_float_clamp;
    out_type = 0u;
    *reinterpret_cast<std::uint64_t*>(step + 16u) = process_fn;
    *reinterpret_cast<std::uint32_t*>(step + 52u) = out_type;
    *reinterpret_cast<std::uint32_t*>(step + 56u) =
        sample_convertor_bitdepth_for_type(out_type);
    return 0;
}

std::int64_t manager_process_sample_convertor(
    std::uint8_t* step,
    void** io_channels) {
    if (!step || !io_channels)
        return 1;
    const std::uint64_t process_fn =
        *reinterpret_cast<const std::uint64_t*>(step + 16u);
    if (process_fn == 0u
        || process_fn == ::auro_engine_v4_ida::kStepSampleConvertor_process_nop)
        return 0;
    if (process_fn == ::auro_engine_v4_ida::kStepSampleConvertor_process_pcm24
        || process_fn == ::auro_engine_v4_ida::kStepSampleConvertor_process_pcm24_sat) {
        const std::uint32_t mask =
            *reinterpret_cast<const std::uint32_t*>(step + 40u);
        const std::uint32_t blocks =
            *reinterpret_cast<const std::uint32_t*>(step + 44u);
        const std::size_t sample_count = static_cast<std::size_t>(blocks) * 32u;
        for (std::uint32_t id = 0u; id < 15u; ++id) {
            if ((mask & (1u << id)) == 0u || io_channels[id] == nullptr)
                continue;
            auto* samples = static_cast<float*>(io_channels[id]);
            auto* pcm = reinterpret_cast<std::int32_t*>(samples);
            for (std::size_t i = 0u; i < sample_count; ++i) {
                if (process_fn == ::auro_engine_v4_ida::kStepSampleConvertor_process_pcm24_sat)
                    pcm[i] = ::auro3d::native_float_to_pcm24(samples[i]);
                else {
                    const float bounded = std::min(
                        255.0f, std::max(-255.0f, samples[i]));
                    pcm[i] = static_cast<std::int32_t>(std::trunc(
                        static_cast<double>(bounded * 8388608.0f)));
                }
            }
        }
        return 0;
    }
    if (process_fn != ::auro_engine_v4_ida::kStepSampleConvertor_process_float_clamp)
        return 1;
    // sub_4CF930: in-place clamp of float channels to [-1, 1].
    const std::uint32_t mask =
        *reinterpret_cast<const std::uint32_t*>(step + 40u);
    const std::uint32_t blocks =
        *reinterpret_cast<const std::uint32_t*>(step + 44u);
    const std::uint32_t sample_count = blocks * 32u;
    if (sample_count == 0u)
        return 0;
    for (std::uint32_t id = 0u; id < 15u; ++id) {
        if ((mask & (1u << id)) == 0u || io_channels[id] == nullptr)
            continue;
        auto* samples = static_cast<float*>(io_channels[id]);
        for (std::uint32_t i = 0u; i < sample_count; ++i) {
            const float v = samples[i];
            samples[i] = v < -1.0f ? -1.0f : (v > 1.0f ? 1.0f : v);
        }
    }
    return 0;
}

// DownSample / UpSample (plan[4]/[5]/[601]). Factor-2 matic FIR.
void manager_construct_downsample_step(std::uint8_t* step) {
    if (!step)
        return;
    auto* obj = matic_resample::object_at(step);
    *obj = matic_resample::ResampleObject{};
    // Native DownSample_t_construct: history buffer at step+160.
    obj->history = reinterpret_cast<float*>(step + 160u);
}

void manager_construct_upsample_step(std::uint8_t* step) {
    if (!step)
        return;
    auto* obj = matic_resample::object_at(step);
    *obj = matic_resample::ResampleObject{};
    obj->history = reinterpret_cast<float*>(step + 160u);
}

std::int64_t manager_configure_downsample_step(
    std::uint8_t* step,
    const std::uint32_t* /*plan_entry*/,
    bool /*first_time*/,
    std::uint8_t* /*block_state*/) {
    // sub_4D22E0
    if (!step)
        return 1;
    const auto* bound = reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(
            *reinterpret_cast<const std::uint64_t*>(step + 32u)));
    if (!bound)
        return 1;
    auto* obj = matic_resample::object_at(step);
    if (!obj->history)
        manager_construct_downsample_step(step);
    if (!matic_resample::init_down(obj, bound[0]))
        return 1;
    matic_resample::clear_down(obj);
    *reinterpret_cast<std::uint32_t*>(step + 1728u) = bound[0];
    *reinterpret_cast<std::uint64_t*>(step + 1720u) =
        *reinterpret_cast<const std::uint64_t*>(bound + 1u);
    // blocks must be even; then halve blocks and rate at +44.
    const std::uint32_t blocks = bound[1];
    if ((blocks & 1u) != 0u)
        return 1;
    *reinterpret_cast<std::uint32_t*>(step + 44u) = blocks >> 1u;
    *reinterpret_cast<std::uint32_t*>(step + 48u) = bound[2] >> 1u;
    *reinterpret_cast<std::uint64_t*>(step + 16u) =
        ::auro_engine_v4_ida::kStepDownSample_process;
    return 0;
}

std::int64_t manager_configure_upsample_step(
    std::uint8_t* step,
    const std::uint32_t* /*plan_entry*/,
    bool /*first_time*/,
    std::uint8_t* /*block_state*/) {
    // sub_4CF700
    if (!step)
        return 1;
    const auto* bound = reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(
            *reinterpret_cast<const std::uint64_t*>(step + 32u)));
    if (!bound)
        return 1;
    auto* obj = matic_resample::object_at(step);
    if (!obj->history)
        manager_construct_upsample_step(step);
    if (!matic_resample::init_up(obj, bound[0]))
        return 1;
    matic_resample::clear_up(obj);
    *reinterpret_cast<std::uint32_t*>(step + 948u) = bound[0];
    *reinterpret_cast<std::uint64_t*>(step + 940u) =
        *reinterpret_cast<const std::uint64_t*>(bound + 1u);
    // blocks must be 1; then double blocks and rate at +44.
    if (bound[1] != 1u)
        return 1;
    *reinterpret_cast<std::uint32_t*>(step + 44u) = 2u;
    *reinterpret_cast<std::uint32_t*>(step + 48u) = bound[2] << 1u;
    *reinterpret_cast<std::uint64_t*>(step + 16u) =
        ::auro_engine_v4_ida::kStepUpSample_process;
    return 0;
}

std::int64_t manager_process_downsample(std::uint8_t* step, void** io_channels) {
    if (!step || !io_channels)
        return 1;
    auto* obj = matic_resample::object_at(step);
    if (!obj->history || !obj->coeffs || obj->channel_count == 0u)
        return 1;
    const std::uint32_t blocks =
        *reinterpret_cast<const std::uint32_t*>(step + 44u);
    // Process consumes 2*blocks input quanta before configure halved metadata.
    // After configure, +44 already holds the output block count; recover input
    // as 2x for this call by reading the pre-halve copy at +1720.
    const std::uint32_t in_blocks =
        *reinterpret_cast<const std::uint32_t*>(step + 1720u);
    if (in_blocks < 2u || (in_blocks & 1u) != 0u)
        return 1;
    const std::uint32_t pairs = in_blocks >> 1u;
    for (std::uint32_t ch = 0u; ch < obj->channel_count; ++ch) {
        const std::uint32_t id = obj->channel_ids[ch];
        if (id >= 31u || io_channels[id] == nullptr)
            return 1;
        auto* samples = static_cast<float*>(io_channels[id]);
        float* hist = obj->history + static_cast<std::size_t>(ch) * matic_resample::kTapCount;
        for (std::uint32_t p = 0u; p < pairs; ++p) {
            matic_resample::w32_down_channel(
                hist,
                samples + static_cast<std::size_t>(p) * 64u,
                samples + static_cast<std::size_t>(p) * 32u,
                obj->coeffs);
        }
    }
    (void)blocks;
    return 0;
}

std::int64_t manager_process_upsample(std::uint8_t* step, void** io_channels) {
    if (!step || !io_channels)
        return 1;
    auto* obj = matic_resample::object_at(step);
    if (!obj->history || !obj->coeffs || obj->channel_count == 0u)
        return 1;
    // Pre-double copy at +940 holds input blocks (==1).
    const std::uint32_t in_blocks =
        *reinterpret_cast<const std::uint32_t*>(step + 940u);
    if (in_blocks != 1u)
        return 1;
    for (std::uint32_t ch = 0u; ch < obj->channel_count; ++ch) {
        const std::uint32_t id = obj->channel_ids[ch];
        if (id >= 31u || io_channels[id] == nullptr)
            return 1;
        auto* samples = static_cast<float*>(io_channels[id]);
        float* hist = obj->history + static_cast<std::size_t>(ch) * matic_resample::kPhaseTaps;
        matic_resample::w32_up_channel(hist, samples, samples, obj->coeffs);
    }
    return 0;
}

// auro_remix_v1_Downmixer_configure/process @ 0x48166C/0x4816A4.
// The native object is a 0x278-byte row table followed by Silence at +632.
void manager_construct_downmixer_step(std::uint8_t* step) {
    if (!step)
        return;
    std::memset(step + 40u, 0, 0x278u + 632u + 128u);
    (void)auro_matic_Silence_construct_object(step + 40u + 632u);
}

std::int64_t manager_configure_downmixer_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry,
    bool /*first_time*/,
    std::uint8_t* block_state) {
    if (!step || !plan_entry)
        return 1;
    manager_construct_downmixer_step(step);
    auto* object = step + 40u;
    std::memcpy(object, plan_entry + 2u, 0x278u);
    // sub_4D1F30 stores the predecessor channel mask at step+824 for the
    // peak callback.  The portable block record exposes the same mask at
    // offset zero.
    *reinterpret_cast<std::uint32_t*>(step + 824u) = block_state
        ? *reinterpret_cast<const std::uint32_t*>(block_state)
        : 0u;
    return auro_matic_Silence_configure_object(
        object + 632u, plan_entry[0u] & ~plan_entry[1u]);
}

// Native auro_remix_v1_Downmixer_update_peak_amplitude, called by
// sub_4D2120.  The route rows are source, destination, gain at object+8;
// the active bits implement the native max-vs-accumulate distinction.
std::int64_t manager_update_peak_downmixer(
    std::uint8_t* step,
    float* peak_31) {
    if (!step || !peak_31)
        return 1;
    const auto* object = step + 40u;
    const std::uint64_t row_count =
        *reinterpret_cast<const std::uint64_t*>(object);
    if (row_count > 0x34u)
        return 1;
    std::uint32_t active =
        *reinterpret_cast<const std::uint32_t*>(step + 824u) & 0x7FFFu;
    for (std::uint64_t row = 0u; row < row_count; ++row) {
        const auto* row_base = object + 8u + static_cast<std::size_t>(row) * 12u;
        const auto source = *reinterpret_cast<const std::uint32_t*>(row_base + 0u);
        const auto destination = *reinterpret_cast<const std::uint32_t*>(row_base + 4u);
        const float gain = *reinterpret_cast<const float*>(row_base + 8u);
        if (source >= 31u || destination >= 31u)
            return 1;
        const bool source_active = (active & (1u << source)) != 0u;
        const float source_peak = peak_31[source];
        const float destination_peak = peak_31[destination];
        if (source_active) {
            peak_31[destination] = source == destination
                ? gain * destination_peak
                : source_peak * gain + destination_peak;
        } else {
            peak_31[destination] = std::max(destination_peak, source_peak);
        }
        if (source_active || (active & (1u << destination)) != 0u)
            active |= 1u << destination;
    }
    return 0;
}

std::int64_t manager_process_downmixer(std::uint8_t* step, void** io_channels) {
    if (!step || !io_channels)
        return 1;
    const auto* object = step + 40u;
    const std::uint64_t row_count =
        *reinterpret_cast<const std::uint64_t*>(object);
    if (row_count > 0x34u)
        return 1;
    const auto block_state = *reinterpret_cast<const std::uint64_t*>(step + 8u);
    if (!block_state)
        return 0;
    const std::uint32_t blocks = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(block_state) + 4u);
    for (std::uint32_t block = 0u; block < blocks; ++block) {
        const std::size_t sample_offset = static_cast<std::size_t>(block) * 32u;
        for (std::uint64_t row = 0u; row < row_count; ++row) {
            const auto* row_base = object + 8u + static_cast<std::size_t>(row) * 12u;
            const std::uint32_t source =
                *reinterpret_cast<const std::uint32_t*>(row_base + 0u);
            const std::uint32_t destination =
                *reinterpret_cast<const std::uint32_t*>(row_base + 4u);
            const float gain = *reinterpret_cast<const float*>(row_base + 8u);
            if (source >= 31u || destination >= 31u || !io_channels[destination])
                return 1;
            auto* dst = static_cast<float*>(io_channels[destination]) + sample_offset;
            if (source == destination) {
                for (std::uint32_t i = 0u; i < 32u; ++i)
                    dst[i] *= gain;
                continue;
            }
            if (!io_channels[source])
                return 1;
            const auto* src = static_cast<const float*>(io_channels[source]) + sample_offset;
            for (std::uint32_t i = 0u; i < 32u; ++i)
                dst[i] += src[i] * gain;
        }
        std::array<float*, 31u> silence_io{};
        for (std::uint32_t id = 0u; id < 31u; ++id)
            silence_io[id] = io_channels[id]
                ? static_cast<float*>(io_channels[id]) + sample_offset
                : nullptr;
        const auto silence_rc = auro_matic_Silence_process_object(
            object + 632u, silence_io.data(), true);
        if (silence_rc != 0)
            return silence_rc;
    }
    return 0;
}

// pipeline::step::Silence wrapper @ 0x4D4DD0/0x4D4E00.  The native wrapper
// binds auro_matic_Silence at step+64, configures it from block_state+8, and
// applies the 32-sample operation once per block.
void manager_construct_silence_step(std::uint8_t* step) {
    if (!step)
        return;
    std::memset(step + 64u, 0, 128u);
    (void)auro_matic_Silence_construct_object(step + 64u);
}

std::int64_t manager_configure_silence_step(
    std::uint8_t* step,
    const std::uint8_t* block_state) {
    if (!step || !block_state)
        return 1;
    return auro_matic_Silence_configure_object(
        step + 64u,
        *reinterpret_cast<const std::uint32_t*>(block_state + 8u));
}

std::int64_t manager_process_silence_step(
    std::uint8_t* step,
    void** io_channels) {
    if (!step || !io_channels)
        return 1;
    const auto block_state = *reinterpret_cast<const std::uint64_t*>(step + 8u);
    if (!block_state)
        return 0;
    const std::uint32_t blocks = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(block_state) + 4u);
    std::array<float*, 31u> silence_io{};
    for (std::uint32_t id = 0u; id < 31u; ++id)
        silence_io[id] = static_cast<float*>(io_channels[id]);
    for (std::uint32_t block = 0u; block < blocks; ++block) {
        std::array<float*, 31u> block_io = silence_io;
        for (float*& channel : block_io)
            if (channel)
                channel += static_cast<std::size_t>(block) * 32u;
        const auto rc = auro_matic_Silence_process_object(
            step + 64u, block_io.data(), true);
        if (rc != 0)
            return rc;
    }
    return 0;
}

void manager_reset_silence_step(std::uint8_t* step) {
    if (step)
        (void)auro_matic_Silence_reset_audio_state_object(step + 64u);
}

// ChannelTest wrapper @ 0x4D6080..0x4D6400.  Native uses the embedded
// Silence object at +68 and, when its predicate is true, replaces logical
// channel 7 with (channel 8 + channel 9) / sqrt(2) before applying Silence.
void manager_construct_channel_test_step(std::uint8_t* step) {
    if (!step)
        return;
    *reinterpret_cast<std::uint32_t*>(step + 64u) = 0u;
    std::memset(step + 68u, 0, 128u);
    (void)auro_matic_Silence_construct_object(step + 68u);
}

std::int64_t manager_configure_channel_test_step(
    std::uint8_t* step,
    const std::uint8_t* plan_entry) {
    if (!step || !plan_entry)
        return 1;
    const std::uint32_t source_layout =
        *reinterpret_cast<const std::uint32_t*>(plan_entry + 4u);
    const std::uint32_t target_layout =
        *reinterpret_cast<const std::uint32_t*>(plan_entry + 8u);
    std::uint32_t silence_mask = source_layout ^ target_layout;
    const bool enabled =
        ((~source_layout & 0x180u) == 0u)
        && ((target_layout & 0x40u) != 0u);
    if (enabled) {
        if ((~target_layout & 0x180u) == 0u)
            return 1;
        silence_mask &= ~0x40u;
    }
    *reinterpret_cast<std::uint32_t*>(step + 64u) = enabled ? 1u : 0u;
    const auto rc = auro_matic_Silence_configure_object(
        step + 68u, silence_mask);
    if (rc != 0)
        return rc;
    *reinterpret_cast<std::uint32_t*>(step + 40u) = target_layout;
    return 0;
}

std::int64_t manager_process_channel_test_step(
    std::uint8_t* step,
    void** io_channels) {
    if (!step || !io_channels)
        return 1;
    const auto block_state =
        *reinterpret_cast<const std::uint64_t*>(step + 32u);
    if (!block_state)
        return 0;
    const std::uint32_t blocks = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(block_state) + 4u);
    const bool enabled =
        *reinterpret_cast<const std::uint32_t*>(step + 64u) != 0u;
    constexpr float kInvSqrt2 = 0.7071067811865475244f;
    for (std::uint32_t block = 0u; block < blocks; ++block) {
        std::array<float*, 31u> block_io{};
        for (std::uint32_t id = 0u; id < 31u; ++id) {
            auto* base = static_cast<float*>(io_channels[id]);
            block_io[id] = base ? base + static_cast<std::size_t>(block) * 32u : nullptr;
        }
        if (enabled && block_io[6u] && block_io[7u] && block_io[8u]) {
            for (std::uint32_t i = 0u; i < 32u; ++i)
                block_io[6u][i] =
                    (block_io[7u][i] + block_io[8u][i]) * kInvSqrt2;
        }
        const auto rc = auro_matic_Silence_process_object(
            step + 68u, block_io.data(), true);
        if (rc != 0)
            return rc;
    }
    return 0;
}

std::int64_t manager_update_peak_channel_test_step(
    std::uint8_t* step,
    float* peak_31) {
    if (!step || !peak_31)
        return 1;
    if (*reinterpret_cast<const std::uint32_t*>(step + 64u) != 0u)
        peak_31[6u] = (peak_31[7u] + peak_31[8u]) *
            0.7071067811865475244f;
    // auro_matic_Silence_update_peak_amplitude @ 0x572430 clears the
    // configured logical channel IDs.  The portable Silence object keeps
    // the same count/list at +124/+0.
    const auto* silence = step + 68u;
    const std::uint32_t count =
        *reinterpret_cast<const std::uint32_t*>(silence + 124u);
    for (std::uint32_t i = 0u; i < count; ++i) {
        const std::uint32_t id =
            *reinterpret_cast<const std::uint32_t*>(silence + i * 4u);
        if (id < 31u)
            peak_31[id] = 0.0f;
    }
    return 0;
}

void manager_reset_channel_test_step(std::uint8_t* /*step*/) {
    // Native nullsub_48: ChannelTest does not reset the embedded Silence
    // state through its step reset callback.
}

// Scene direct callbacks @ 0x4D31E0 / 0x4D3440.  The native direct
// configurator selects these callbacks when plan[589] is one.
void manager_construct_scene_step(std::uint8_t* step) {
    if (!step)
        return;
    // Scene_t_construct @ 0x4D2670 clears a1+64..a1+335.  The outer
    // callback/metadata prefix is initialized by Manager_t_construct and is
    // deliberately left outside this embedded-object clear.
    std::memset(step + 64u, 0, 0x110u);
}

std::int64_t manager_configure_scene_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry) {
    // Native Scene::configure is called only for an active plan record, and
    // selects its direct callbacks from record[1] (mode), not record[0].
    if (!step || !plan_entry || plan_entry[1u] != 1u)
        return 1;
    // Scene::configure writes a1+356/a1+360.  In the step view these are
    // step+396/step+400 (the embedded Scene object begins at step+40).
    *reinterpret_cast<std::uint32_t*>(step + 396u) = plan_entry[1u];
    *reinterpret_cast<std::uint32_t*>(step + 400u) = plan_entry[2u];
    return 0;
}

std::int64_t manager_process_scene_step(
    std::uint8_t* step,
    void** io_channels) {
    if (!step || !io_channels)
        return 1;
    const auto predecessor = *reinterpret_cast<const std::uint64_t*>(step + 32u);
    if (!predecessor)
        return 1;
    const auto previous_layout = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(predecessor));
    const auto current_layout = *reinterpret_cast<const std::uint32_t*>(step + 40u);
    const auto clear_mask = current_layout & ~previous_layout;
    const auto blocks = *reinterpret_cast<const std::uint32_t*>(step + 44u);
    const auto samples = static_cast<std::size_t>(blocks) * 32u;
    for (std::uint32_t id = 0u; id < 15u; ++id) {
        if ((clear_mask & (1u << id)) != 0u && io_channels[id])
            std::memset(io_channels[id], 0, samples * sizeof(float));
    }
    return 0;
}

std::int64_t manager_update_peak_scene_step(
    std::uint8_t* step,
    float* peak_31) {
    if (!step || !peak_31)
        return 1;
    const auto predecessor = *reinterpret_cast<const std::uint64_t*>(step + 32u);
    if (!predecessor)
        return 1;
    const auto layout = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(predecessor));
    // Scene::update @ 0x4D3440.  The native SIMD code multiplies the
    // incoming peak table by these exact layout-bit masks; it does not
    // replace every absent channel with zero independently.  The duplicated
    // bits in the first two groups are intentional (they are the native
    // packed-mask expansion).
    const auto bit = [layout](std::uint32_t n) -> float {
        return (layout >> n) & 1u ? 1.0f : 0.0f;
    };
    peak_31[0u] *= bit(0u);
    peak_31[1u] *= bit(1u);
    peak_31[2u] *= bit(2u);
    peak_31[3u] *= bit(3u);
    peak_31[4u] *= bit(4u);
    peak_31[5u] *= bit(4u);
    peak_31[6u] *= bit(7u);
    peak_31[7u] *= bit(5u);
    peak_31[8u] *= bit(8u);
    peak_31[9u] *= bit(8u);
    peak_31[10u] *= bit(11u);
    peak_31[11u] *= bit(9u);
    peak_31[12u] *= bit(12u);
    peak_31[13u] *= bit(13u);
    peak_31[14u] *= bit(14u);
    return 0;
}

void manager_reset_scene_step(std::uint8_t* step) {
    (void)step;
    // Native Scene reset callback is nullsub_45 @ 0x4D3570.
}

// Legacy wrapper/remixer @ 0x4D5E40..0x4D6060 and
// auro_remix_v1_Legacy_process @ 0x4DD7D0.  The plan entry is the native
// three-word Legacy record: word 0 is the original layout, word 1 the
// remapped layout, and word 2 is the active flag.
void manager_construct_legacy_step(std::uint8_t* step) {
    if (!step)
        return;
    std::memset(step + 64u, 0, 4u);
}

std::int64_t manager_configure_legacy_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry,
    std::uint32_t input_layout) {
    if (!step || !plan_entry)
        return 1;
    *reinterpret_cast<std::uint32_t*>(step + 64u) = plan_entry[2u];
    const std::uint32_t remapped =
        (*reinterpret_cast<const std::uint32_t*>(step + 64u) == 0u)
            ? input_layout
            : ((input_layout & 0xFFFFFF8Fu) | 0x30u);
    *reinterpret_cast<std::uint32_t*>(step + 40u) = remapped;
    return 0;
}

std::int64_t manager_process_legacy_step(
    std::uint8_t* step,
    void** io_channels) {
    if (!step || !io_channels)
        return 1;
    if (*reinterpret_cast<const std::uint32_t*>(step + 64u) == 0u)
        return 0;
    const auto block_state = *reinterpret_cast<const std::uint64_t*>(step + 8u);
    if (!block_state)
        return 0;
    const std::uint32_t blocks = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(block_state) + 4u);
    constexpr float kInvSqrt2 = 0.7071067811865476f;
    for (std::uint32_t block = 0u; block < blocks; ++block) {
        auto* left = static_cast<float*>(io_channels[4u]);
        auto* right = static_cast<float*>(io_channels[5u]);
        auto* source = static_cast<float*>(io_channels[6u]);
        if (!left || !right || !source)
            continue;
        left += static_cast<std::size_t>(block) * 32u;
        right += static_cast<std::size_t>(block) * 32u;
        source += static_cast<std::size_t>(block) * 32u;
        const bool overlap =
            (source < left + 32u && left < source + 32u)
            || (source < right + 32u && right < source + 32u)
            || (right < left + 32u && left < right + 32u);
        if (!overlap) {
            for (std::uint32_t i = 0u; i < 32u; ++i) {
                const float value = source[i] * kInvSqrt2;
                left[i] = value;
                right[i] = value;
                source[i] = 0.0f;
            }
        } else {
            std::array<float, 32u> copy{};
            std::memcpy(copy.data(), source, sizeof(copy));
            for (std::uint32_t i = 0u; i < 32u; ++i) {
                const float value = copy[i] * kInvSqrt2;
                left[i] = value;
                right[i] = value;
                source[i] = 0.0f;
            }
        }
    }
    return 0;
}

std::int64_t manager_update_peak_legacy_step(
    std::uint8_t* step,
    float* peak_31) {
    if (!step || !peak_31)
        return 1;
    if (*reinterpret_cast<const std::uint32_t*>(step + 64u) != 0u) {
        const float value = peak_31[6u] * 0.7071067811865476f;
        peak_31[4u] = value;
        peak_31[5u] = value;
        peak_31[6u] = 0.0f;
    }
    return 0;
}

void manager_reset_legacy_step(std::uint8_t* step) {
    if (step)
        *reinterpret_cast<std::uint32_t*>(step + 64u) = 0u;
}

// Delay wrapper @ 0x4D5420..0x4D5E20.  The native object stores its selected
// channel list at object+1560, the count at +1620, the delay in samples at
// +1624, and one 26-sample history record (104 bytes) per channel at +0.
void manager_construct_delay_step(std::uint8_t* step) {
    if (step)
        std::memset(step + 64u, 0, 1692u);
}

std::int64_t manager_configure_delay_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry,
    std::uint32_t input_layout) {
    if (!step || !plan_entry || plan_entry[1u] > 26u)
        return 1;
    auto* object = step + 64u;
    const std::uint32_t delay = plan_entry[1u];
    *reinterpret_cast<std::uint32_t*>(object + 1624u) = delay;
    *reinterpret_cast<std::uint32_t*>(object + 1620u) = 0u;
    for (std::uint32_t id = 0u; id < 15u; ++id) {
        if ((input_layout & (1u << id)) == 0u)
            continue;
        auto count = *reinterpret_cast<std::uint32_t*>(object + 1620u);
        *reinterpret_cast<std::uint32_t*>(object + 1560u + count * 4u) = id;
        *reinterpret_cast<std::uint32_t*>(object + 1620u) = ++count;
    }
    *reinterpret_cast<std::uint32_t*>(object + 1624u) = delay;
    return 0;
}

std::int64_t manager_process_delay_step(
    std::uint8_t* step,
    void** io_channels) {
    if (!step || !io_channels)
        return 1;
    auto* object = step + 64u;
    const std::uint32_t count = *reinterpret_cast<const std::uint32_t*>(object + 1620u);
    const std::uint32_t delay = *reinterpret_cast<const std::uint32_t*>(object + 1624u);
    const auto block_state = *reinterpret_cast<const std::uint64_t*>(step + 8u);
    if (!block_state || count == 0u || delay == 0u)
        return 0;
    const std::uint32_t blocks = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(block_state) + 4u);
    for (std::uint32_t n = 0u; n < count; ++n) {
        const std::uint32_t id = *reinterpret_cast<const std::uint32_t*>(object + 1560u + n * 4u);
        if (id >= 31u || !io_channels[id])
            continue;
        auto* samples = static_cast<float*>(io_channels[id]);
        auto* history = reinterpret_cast<float*>(object + id * 104u);
        for (std::uint32_t block = 0u; block < blocks; ++block) {
            float* current = samples + static_cast<std::size_t>(block) * 32u;
            std::array<float, 32u> input{};
            std::memcpy(input.data(), current, sizeof(input));
            for (std::uint32_t i = 0u; i < 32u; ++i) {
                const float delayed = history[0u];
                for (std::uint32_t j = 1u; j < delay; ++j)
                    history[j - 1u] = history[j];
                history[delay - 1u] = input[i];
                current[i] = delayed;
            }
        }
    }
    return 0;
}

void manager_reset_delay_step(std::uint8_t* step) {
    if (step)
        std::memset(step + 64u, 0, 1624u);
}

struct ManagerAlt3dStoreState {
    std::array<std::uint8_t, 1200u> downmix_step{};
    std::vector<float> samples;
    std::array<std::uint32_t, 31u> channels{};
    std::uint32_t channel_count = 0u;
    std::array<std::uint32_t, 6u> route_ids{};
    std::array<float, 6u> route_peaks{};
    std::uint32_t route_count = 0u;
};

// Native sub_4D4230 first delegates peak propagation to the embedded
// Downmixer object before updating the Alt3D buffer bookkeeping.  The latter
// route table is separate state; do not approximate it from channel IDs.
std::int64_t manager_update_peak_alt3d_store(
    std::uint8_t* step,
    float* peak_31) {
    if (!step || !peak_31)
        return 1;
    auto* state = reinterpret_cast<ManagerAlt3dStoreState*>(static_cast<std::uintptr_t>(
        *reinterpret_cast<const std::uint64_t*>(step + 64u)));
    if (!state)
        return 0;
    const auto rc = manager_update_peak_downmixer(
        state->downmix_step.data(), peak_31);
    if (rc != 0)
        return rc;
    // Native sub_4D4230 copies the post-Downmixer peaks into the Buffer
    // route records, then runs Silence_update_peak_amplitude.  The Buffer
    // initializer installs identity routes for the only accepted mask bits,
    // so the route source is also the channel cleared from the public peak
    // table.  Keep the saved values for the following Alt3D_Mix update.
    for (std::uint32_t i = 0u; i < state->route_count; ++i) {
        const auto id = state->route_ids[i];
        if (id >= 31u)
            return 1;
        state->route_peaks[i] = peak_31[id];
        peak_31[id] = 0.0f;
    }
    return 0;
}

std::int64_t manager_update_peak_alt3d_mix(
    std::uint8_t* step,
    float* peak_31) {
    if (!step || !peak_31)
        return 1;
    auto* state = reinterpret_cast<ManagerAlt3dStoreState*>(static_cast<std::uintptr_t>(
        *reinterpret_cast<const std::uint64_t*>(step + 64u)));
    if (!state)
        return 0;
    const float gain_a = *reinterpret_cast<const float*>(step + 76u);
    const float gain_b = *reinterpret_cast<const float*>(step + 80u);
    // Native sub_4D4540 applies the saved Buffer peak and the current peak
    // to each route destination.  Routes are identity routes by construction.
    for (std::uint32_t i = 0u; i < state->route_count; ++i) {
        const auto id = state->route_ids[i];
        if (id >= 31u)
            return 1;
        peak_31[id] = state->route_peaks[i] * gain_a + peak_31[id] * gain_b;
    }
    return 0;
}

void manager_construct_alt3d_store_step(std::uint8_t* step) {
    if (step)
        *reinterpret_cast<std::uint64_t*>(step + 64u) = 0u;
}

std::int64_t manager_configure_alt3d_store_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry) {
    if (!step || !plan_entry)
        return 1;
    manager_reset_alt3d_store_step(step);
    auto* state = new (std::nothrow) ManagerAlt3dStoreState();
    if (!state)
        return 1;
    *reinterpret_cast<std::uint64_t*>(step + 64u) =
        reinterpret_cast<std::uint64_t>(state);
    *reinterpret_cast<std::uint32_t*>(step + 40u) = plan_entry[3u];
    const auto downmix_rc = manager_configure_downmixer_step(
        state->downmix_step.data(), plan_entry + 6u, true, nullptr);
    if (downmix_rc != 0) {
        manager_reset_alt3d_store_step(step);
        return downmix_rc;
    }
    // auro_a3deng_v3_pipeline_step_alt3D_Buffer_initialize accepts only the
    // six height bits 0x200..0x4000 and installs one identity route per set
    // bit, in ascending channel order.  Preserve that native route table;
    // arbitrary channel-mask iteration would also store non-height buses.
    const std::uint32_t mask = plan_entry[4u];
    if ((mask & ~0x7E00u) != 0u) {
        manager_reset_alt3d_store_step(step);
        return 1;
    }
    for (std::uint32_t id = 9u; id <= 14u; ++id) {
        if ((mask & (1u << id)) == 0u)
            continue;
        state->route_ids[state->route_count++] = id;
    }
    state->channels = {};
    for (std::uint32_t i = 0u; i < state->route_count; ++i)
        state->channels[state->channel_count++] = state->route_ids[i];
    return 0;
}

std::int64_t manager_process_alt3d_store_step(
    std::uint8_t* step,
    void** io_channels) {
    if (!step || !io_channels)
        return 1;
    auto* state = reinterpret_cast<ManagerAlt3dStoreState*>(static_cast<std::uintptr_t>(
        *reinterpret_cast<const std::uint64_t*>(step + 64u)));
    if (!state)
        return 0;
    *reinterpret_cast<std::uint64_t*>(state->downmix_step.data() + 8u) =
        *reinterpret_cast<const std::uint64_t*>(step + 8u);
    const auto block_state = *reinterpret_cast<const std::uint64_t*>(step + 8u);
    if (!block_state)
        return 0;
    const std::uint32_t blocks = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(block_state) + 4u);
    constexpr std::size_t kSamplesPerBlock = 31u * 32u;
    if (blocks > (std::numeric_limits<std::size_t>::max() / kSamplesPerBlock))
        return 1;
    state->samples.assign(static_cast<std::size_t>(blocks) * kSamplesPerBlock, 0.0f);
    // Native Alt3D Store calls its embedded Downmixer once with the complete
    // block chain; the wrapper itself then iterates blocks to store and clear
    // the selected channels.  Do not pass a one-block view to a Downmixer
    // that already consumes the linked block count.
    const auto downmix_rc = manager_process_downmixer(
        state->downmix_step.data(), io_channels);
    if (downmix_rc != 0)
        return downmix_rc;
    for (std::uint32_t block = 0u; block < blocks; ++block) {
        std::array<float*, 31u> block_io{};
        for (std::uint32_t id = 0u; id < 31u; ++id)
            block_io[id] = io_channels[id]
                ? static_cast<float*>(io_channels[id]) + static_cast<std::size_t>(block) * 32u
                : nullptr;
        for (std::uint32_t n = 0u; n < state->channel_count; ++n) {
            const auto id = state->channels[n];
            if (id < 31u && block_io[id])
                std::memcpy(state->samples.data()
                                + static_cast<std::size_t>(block) * kSamplesPerBlock
                                + static_cast<std::size_t>(id) * 32u,
                            block_io[id], 32u * sizeof(float));
        }
        for (std::uint32_t n = 0u; n < state->channel_count; ++n) {
            const auto id = state->channels[n];
            if (id < 31u && block_io[id])
                std::memset(block_io[id], 0, 32u * sizeof(float));
        }
    }
    return 0;
}

void manager_reset_alt3d_store_step(std::uint8_t* step) {
    if (!step)
        return;
    auto* state = reinterpret_cast<ManagerAlt3dStoreState*>(static_cast<std::uintptr_t>(
        *reinterpret_cast<const std::uint64_t*>(step + 64u)));
    delete state;
    *reinterpret_cast<std::uint64_t*>(step + 64u) = 0u;
}

std::int64_t manager_configure_alt3d_mix_step(
    std::uint8_t* step,
    std::uint8_t* store_step,
    const std::uint32_t* plan_entry) {
    if (!step || !store_step || !plan_entry)
        return 1;
    *reinterpret_cast<std::uint64_t*>(step + 64u) =
        *reinterpret_cast<const std::uint64_t*>(store_step + 64u);
    *reinterpret_cast<std::uint64_t*>(step + 72u) =
        *reinterpret_cast<const std::uint64_t*>(plan_entry + 1u);
    *reinterpret_cast<std::uint32_t*>(step + 80u) = plan_entry[3u];
    *reinterpret_cast<std::uint32_t*>(step + 40u) = plan_entry[3u];
    return 0;
}

std::int64_t manager_process_alt3d_mix_step(
    std::uint8_t* step,
    void** io_channels) {
    if (!step || !io_channels)
        return 1;
    auto* state = reinterpret_cast<ManagerAlt3dStoreState*>(static_cast<std::uintptr_t>(
        *reinterpret_cast<const std::uint64_t*>(step + 64u)));
    if (!state)
        return 0;
    const auto block_state = *reinterpret_cast<const std::uint64_t*>(step + 8u);
    if (!block_state)
        return 0;
    const std::uint32_t blocks = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(block_state) + 4u);
    const float gain_a = *reinterpret_cast<const float*>(step + 76u);
    const float gain_b = *reinterpret_cast<const float*>(step + 80u);
    constexpr std::size_t kSamplesPerBlock = 31u * 32u;
    if (blocks > (std::numeric_limits<std::size_t>::max() / kSamplesPerBlock)
        || state->samples.size() < static_cast<std::size_t>(blocks) * kSamplesPerBlock)
        return 1;
    for (std::uint32_t block = 0u; block < blocks; ++block) {
        for (std::uint32_t n = 0u; n < state->channel_count; ++n) {
            const auto id = state->channels[n];
            if (id >= 31u || !io_channels[id])
                continue;
            auto* dst = static_cast<float*>(io_channels[id]) + static_cast<std::size_t>(block) * 32u;
            const auto* src = state->samples.data()
                + static_cast<std::size_t>(block) * kSamplesPerBlock
                + static_cast<std::size_t>(id) * 32u;
            for (std::uint32_t i = 0u; i < 32u; ++i)
                dst[i] = dst[i] * gain_b + src[i] * gain_a;
        }
    }
    return 0;
}

void manager_reset_alt3d_mix_step(std::uint8_t* step) {
    if (step)
        *reinterpret_cast<std::uint64_t*>(step + 64u) = 0u;
}

// BassManager_t_construct/reset/configure/update_peak @ 0x480108,
// 0x4801E4, 0x480300 and 0x480FEC.  The object is embedded after the Manager
// step header.  Audio activation is intentionally separate: the two native
// IIR callback paths still require their exact coefficient/process port.
void manager_construct_bass_management_step(std::uint8_t* step) {
    if (!step)
        return;
    std::memset(step + 64u, 0, 880u);
    *reinterpret_cast<float*>(step + 64u + 764u) = 1.0f;
    *reinterpret_cast<float*>(step + 64u + 772u) = 1.0f;
}

void manager_reset_bass_management_step(std::uint8_t* step) {
    if (!step)
        return;
    auto* state = step + 64u;
    // Native reset clears the per-route biquad state and the 31-channel
    // accumulator rows, while preserving configured gains and route IDs.
    std::memset(state + 4u, 0, 624u);
    // The native reset clears the LFE filter history at +668, but the
    // coefficient records at +628/+688 are configuration state and survive.
    std::memset(state + 668u, 0, 20u);
}

std::int64_t manager_configure_bass_management_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry,
    std::uint32_t sample_rate) {
    if (!step || !plan_entry || sample_rate == 0u)
        return 1;
    auto* state = step + 64u;
    const std::uint32_t source_layout = plan_entry[0u];
    const std::uint32_t target_layout = plan_entry[1u];
    const std::uint32_t changed_layout = target_layout & ~source_layout;
    if (((~target_layout & source_layout) != 0u)
        || (changed_layout & 0xFFEFFFF7u) != 0u
        || (target_layout & 8u) == 0u)
        return 1;
    const std::uint32_t manager_mode = changed_layout != 0u
        ? (source_layout & 8u) == 0u ? 3u : 2u
        : 0u;
    *reinterpret_cast<std::uint32_t*>(state + 744u) = manager_mode;
    *reinterpret_cast<std::uint32_t*>(state + 748u) = 0u;
    const auto db_to_linear = [](float db) {
        return db > -144.0f ? std::pow(10.0f, db * 0.05f) : 0.0f;
    };
    *reinterpret_cast<float*>(state + 764u) = db_to_linear(
        *reinterpret_cast<const float*>(plan_entry + 2u));
    *reinterpret_cast<float*>(state + 772u) = db_to_linear(
        *reinterpret_cast<const float*>(plan_entry + 3u));
    *reinterpret_cast<std::uint32_t*>(state + 768u) = source_layout & 0x100008u;
    *reinterpret_cast<std::uint32_t*>(state + 776u) = target_layout & 0x100008u;
    const float filter_q = 0.70710677f;
    std::array<std::uint8_t, 16u> filter_parameters{};
    *reinterpret_cast<float*>(filter_parameters.data() + 8u) = filter_q;
    *reinterpret_cast<float*>(filter_parameters.data() + 12u) = 0.0f;
    const std::uint32_t filter_enabled = plan_entry[4u];
    if (filter_enabled != 0u) {
        const auto filter_word = *reinterpret_cast<const std::uint64_t*>(plan_entry + 5u);
        *reinterpret_cast<std::uint32_t*>(filter_parameters.data()) =
            static_cast<std::uint32_t>(filter_word & 0xFFFFFFFFu);
        *reinterpret_cast<float*>(filter_parameters.data() + 4u) =
            static_cast<float>(filter_word >> 32u);
        if (auro_iir_biquad_parameter_Config_float32_t_compute(
                filter_parameters.data(), sample_rate, state + 716u) != 0)
            return 1;
        *reinterpret_cast<std::uint64_t*>(state + 736u) = 1u;
        const auto route_word = *reinterpret_cast<const std::uint64_t*>(plan_entry + 7u);
        *reinterpret_cast<std::uint32_t*>(state + 0u) =
            static_cast<std::uint32_t>(route_word & 0xFFFFFFFFu);
        *reinterpret_cast<std::uint32_t*>(state + 4u) =
            static_cast<std::uint32_t>(route_word >> 32u);
        if (*reinterpret_cast<const std::uint32_t*>(state + 0u) != 0u) {
            *reinterpret_cast<std::uint32_t*>(filter_parameters.data()) =
                *reinterpret_cast<const std::uint32_t*>(state + 0u) == 1u ? 16u : 2u;
            *reinterpret_cast<float*>(filter_parameters.data() + 4u) =
                *reinterpret_cast<const float*>(state + 4u);
            if (auro_iir_biquad_parameter_Config_float32_t_compute(
                    filter_parameters.data(), sample_rate, state + 632u) != 0)
                return 1;
            *reinterpret_cast<std::uint64_t*>(state + 656u) = 1u;
        }
    }
    std::uint32_t count = 0u;
    const std::uint32_t route_mask = source_layout & 0xFFEFFFF7u;
    for (std::uint32_t id = 0u; id < 31u; ++id) {
        if ((route_mask & (1u << id)) == 0u)
            continue;
        auto* route = state + 8u + 20u * count;
        *reinterpret_cast<std::uint32_t*>(route) = id;
        ++count;
    }
    *reinterpret_cast<std::uint32_t*>(state + 628u) = count;
    *reinterpret_cast<std::uint32_t*>(state + 712u) =
        (target_layout & 8u) != 0u ? 0u : 1u;
    if ((target_layout & 0x100000u) != 0u) {
        *reinterpret_cast<std::uint32_t*>(state + 672u) = 20u;
        *reinterpret_cast<std::uint32_t*>(state + 712u) = 2u;
    }
    return 0;
}

void bass_process_section_32(
    float* samples,
    float* filter_state,
    const float* coefficients,
    std::uint32_t sections) {
    if (!samples || !filter_state || !coefficients || sections == 0u)
        return;
    for (std::uint32_t section = 0u; section < sections; ++section) {
        float s0 = filter_state[section * 2u + 0u];
        float s1 = filter_state[section * 2u + 1u];
        for (std::uint32_t i = 0u; i < 32u; ++i) {
            const float input = samples[i];
            const float output = s0 + input * coefficients[0u];
            s0 = s1 + input * coefficients[1u] - coefficients[3u] * output;
            s1 = input * coefficients[2u] - coefficients[4u] * output;
            samples[i] = output;
        }
        filter_state[section * 2u + 0u] = s0;
        filter_state[section * 2u + 1u] = s1;
    }
}

std::int64_t manager_process_bass_management(
    std::uint8_t* step,
    void** io_channels) {
    if (!step || !io_channels)
        return 1;
    auto* state = step + 64u;
    auto* lfe = static_cast<float*>(io_channels[3u]);
    if (!lfe)
        return 1;
    const std::uint32_t input_mask =
        *reinterpret_cast<const std::uint32_t*>(state + 768u);
    const std::uint32_t mode =
        *reinterpret_cast<const std::uint32_t*>(state + 744u);
    if (mode == 3u) {
        std::memset(lfe, 0, 32u * sizeof(float));
        return 0;
    }
    const float lfe_gain = *reinterpret_cast<const float*>(state + 764u);
    if ((input_mask & 8u) != 0u) {
        for (std::uint32_t i = 0u; i < 32u; ++i)
            lfe[i] *= lfe_gain;
    } else {
        std::memset(lfe, 0, 32u * sizeof(float));
    }
    const std::uint32_t count =
        *reinterpret_cast<const std::uint32_t*>(state + 628u);
    std::array<float, 32u> accumulated{};
    for (std::uint32_t i = 0u; i < count; ++i) {
        const std::uint32_t id = *reinterpret_cast<const std::uint32_t*>(
            state + 8u + 20u * i);
        if (id >= 31u || !io_channels[id])
            return 1;
        const auto* source = static_cast<const float*>(io_channels[id]);
        for (std::uint32_t sample = 0u; sample < 32u; ++sample)
            accumulated[sample] += source[sample];
    }
    const auto lfe_callback =
        *reinterpret_cast<const std::uint64_t*>(state + 736u);
    if (lfe_callback != 0u) {
        const std::uint32_t sections = lfe_callback == 1u ? 1u : 2u;
        bass_process_section_32(
            accumulated.data(), reinterpret_cast<float*>(state + 696u),
            reinterpret_cast<const float*>(state + 716u), sections);
    }
    for (std::uint32_t sample = 0u; sample < 32u; ++sample)
        lfe[sample] += accumulated[sample];

    const auto route_callback =
        *reinterpret_cast<const std::uint64_t*>(state + 656u);
    if (route_callback != 0u) {
        const std::uint32_t sections = route_callback == 1u ? 1u : 2u;
        const auto* coefficients = reinterpret_cast<const float*>(state + 632u);
        for (std::uint32_t i = 0u; i < count; ++i) {
            const std::uint32_t id = *reinterpret_cast<const std::uint32_t*>(
                state + 8u + 20u * i);
            bass_process_section_32(
                static_cast<float*>(io_channels[id]),
                reinterpret_cast<float*>(state + 12u + 20u * i),
                coefficients, sections);
        }
    }
    const float output_gain = *reinterpret_cast<const float*>(state + 772u);
    const std::uint32_t output_mask =
        *reinterpret_cast<const std::uint32_t*>(state + 776u);
    for (std::uint32_t sample = 0u; sample < 32u; ++sample)
        if ((output_mask & 8u) != 0u)
            lfe[sample] *= output_gain;
    return 0;
}

std::int64_t manager_update_peak_bass_management(
    std::uint8_t* step,
    float* peak_31) {
    if (!step || !peak_31)
        return 1;
    const auto* state = step + 64u;
    const std::uint32_t input_mask =
        *reinterpret_cast<const std::uint32_t*>(state + 768u);
    float lfe_peak = peak_31[3u];
    if ((input_mask & 8u) != 0u)
        lfe_peak *= *reinterpret_cast<const float*>(state + 764u);
    const std::uint32_t mode =
        *reinterpret_cast<const std::uint32_t*>(state + 744u);
    if ((input_mask & 0x100000u) != 0u && mode == 1u) {
        lfe_peak *= 0.7071067811865476f;
        peak_31[20u] = lfe_peak;
    }
    if (mode == 3u) {
        peak_31[3u] = 0.0f;
        peak_31[20u] = 0.0f;
    } else if (mode == 2u) {
        peak_31[3u] = 0.0f;
    }
    if (*reinterpret_cast<const std::uint32_t*>(state + 748u) == 0u)
        return 0;
    const std::uint32_t mask_a =
        *reinterpret_cast<const std::uint32_t*>(state + 752u);
    const std::uint32_t mask_b =
        *reinterpret_cast<const std::uint32_t*>(state + 756u);
    const std::uint32_t mask_c =
        *reinterpret_cast<const std::uint32_t*>(state + 760u);
    for (std::uint32_t id = 0u; id < 31u; ++id) {
        const std::uint32_t bit = 1u << id;
        if ((mask_a & bit) != 0u)
            peak_31[id] += peak_31[id];
        else if ((mask_b & bit) != 0u)
            peak_31[id] += peak_31[20u];
        else if ((mask_c & bit) != 0u)
            peak_31[3u] += peak_31[id] * 0.7071067811865476f;
    }
    const std::uint32_t output_mask =
        *reinterpret_cast<const std::uint32_t*>(state + 776u);
    if ((output_mask & 8u) != 0u)
        peak_31[3u] *= *reinterpret_cast<const float*>(state + 772u);
    if ((output_mask & 0x100000u) != 0u)
        peak_31[20u] *= *reinterpret_cast<const float*>(state + 772u);
    return 0;
}

} // namespace

// Native auro_matic_v3_OutputGate lifecycle @ 0x553F20/0x553FA0/0x553FB0.
std::int64_t auro_matic_v3_OutputGate_construct_portable(std::uint8_t* gate) {
    if (!gate)
        return 0;
    auromatic_detect_silence_construct(gate + 8u);
    *reinterpret_cast<float*>(gate + 2148u) = 1.0f;
    return reinterpret_cast<std::int64_t>(gate);
}

std::int64_t auro_matic_v3_OutputGate_configure(
    std::uint8_t* gate,
    std::uint32_t sample_rate,
    std::uint32_t channel_mask) {
    if (!gate || sample_rate == 0u)
        return 1;
    auromatic_detect_silence_construct(gate + 8u);
    auromatic_detect_silence_configure(gate + 8u, sample_rate, channel_mask);
    auromatic_detect_silence_reset(gate + 8u);
    const float rate = static_cast<float>(sample_rate);
    *reinterpret_cast<float*>(gate + 2136u) = std::pow(10.0f, -3.0f / (rate * 0.01f));
    *reinterpret_cast<float*>(gate + 2140u) = std::pow(10.0f, -3.0f / (rate * 0.1f));
    *reinterpret_cast<std::uint32_t*>(gate + 4u) = 1u;
    *reinterpret_cast<std::uint32_t*>(gate + 2144u) = 0u;
    *reinterpret_cast<float*>(gate + 2148u) = 1.0f;
    return 0;
}

void auro_matic_v3_OutputGate_set_bypass(std::uint8_t* gate, std::int64_t bypass) {
    if (gate)
        *reinterpret_cast<std::uint32_t*>(gate + 4u) = bypass != 0 ? 1u : 0u;
}

std::int64_t auro_matic_v3_OutputGate_reset_audio_state(std::uint8_t* gate) {
    if (!gate)
        return 1;
    *reinterpret_cast<std::uint32_t*>(gate) = 0u;
    *reinterpret_cast<std::uint32_t*>(gate + 2144u) = 0u;
    auromatic_detect_silence_reset(gate + 8u);
    return 0;
}

bool auro_matic_v3_OutputGate_process(
    std::uint8_t* gate,
    void** channel_span,
    float* gains,
    std::uint32_t* output_silent,
    std::uint32_t* transition_silent) {
    return auromatic_output_gate_process(
        gate, channel_span, gains, output_silent, transition_silent);
}

std::int64_t auro_a3deng_v4_Gate_construct_portable(std::uint8_t* gate) {
    if (!gate)
        return 0;
    // Pipeline Gate_t_construct only installs the callback table; gains are
    // written by configure/reset at +64. Clear the step header and endpoints.
    std::memset(gate, 0, 72u);
    *reinterpret_cast<std::uint64_t*>(gate + 64u) =
        0x3F8000003F800000ull;
    return reinterpret_cast<std::int64_t>(gate);
}

std::int64_t auro_a3deng_v4_Gate_reset_audio_state(std::uint8_t* gate) {
    if (!gate)
        return 1;
    *reinterpret_cast<std::uint64_t*>(gate + 64u) =
        0x3F8000003F800000ull;
    return 0;
}

std::int64_t auro_a3deng_v4_Gate_process(
    std::uint8_t* gate,
    void** io_channels) {
    return manager_process_gate(gate, io_channels);
}

bool auromatic_gate_self_test(std::string& detail) {
    std::array<std::uint8_t, 80u> gate{};
    std::array<float, 64u> selected{};
    std::array<float, 64u> unselected{};
    selected.fill(1.0f);
    unselected.fill(1.0f);
    void* channels[31u]{};
    channels[0u] = selected.data();
    channels[1u] = unselected.data();

    if (!auro_a3deng_v4_Gate_construct_portable(gate.data())) {
        detail = "construct failed";
        return false;
    }
    // Pipeline Gate process @ 0x4D1BE0: mask/blocks at +40/+44, gains at +64/+68.
    *reinterpret_cast<std::uint32_t*>(gate.data() + 40u) = 0x1u;
    *reinterpret_cast<std::uint32_t*>(gate.data() + 44u) = 2u;
    *reinterpret_cast<float*>(gate.data() + 64u) = 0.0f;
    *reinterpret_cast<float*>(gate.data() + 68u) = 1.0f;
    if (auro_a3deng_v4_Gate_process(gate.data(), channels) != 0) {
        detail = "process failed";
        return false;
    }
    const auto close = [](float actual, float expected) {
        return std::fabs(actual - expected) <= 1.0e-6f;
    };
    if (!close(selected[0u], 0.015625f)
        || !close(selected[31u], 0.5f)
        || !close(selected[32u], 0.515625f)
        || !close(selected[63u], 1.0f)
        || !close(unselected[0u], 1.0f)) {
        detail = "two-block ramp or mask mismatch";
        return false;
    }
    if (auro_a3deng_v4_Gate_reset_audio_state(gate.data()) != 0
        || *reinterpret_cast<const std::uint64_t*>(gate.data() + 64u)
            != 0x3F8000003F800000ull) {
        detail = "reset mismatch";
        return false;
    }
    detail = "two-block 32-sample ramp/reset PASS";
    return true;
}

bool auromatic_xinn_tuning_self_test(std::string& detail) {
    std::array<std::uint8_t, 120u> block{};
    const auto i32 = [&](std::size_t offset) {
        return *reinterpret_cast<const std::int32_t*>(block.data() + offset);
    };
    const auto f32 = [&](std::size_t offset) {
        return *reinterpret_cast<const float*>(block.data() + offset);
    };
    const auto close = [](float actual, float expected) {
        return std::fabs(actual - expected) <= 1.0e-7f;
    };
    auro_matic_v3_XinN_parameter_Dynamic_set_stereo_Xs(block.data(), 11, 12, .11f, .12f);
    auro_matic_v3_XinN_parameter_Dynamic_set_stereo_HX(block.data(), 21, 22, .21f, .22f);
    auro_matic_v3_XinN_parameter_Dynamic_set_stereo_HXs(block.data(), 31, 32, .31f, .32f);
    auro_matic_v3_XinN_parameter_Dynamic_set_surround_HX(block.data(), 41, 42, 43, .41f, .42f, .43f);
    auro_matic_v3_XinN_parameter_Dynamic_set_surround_HXs(block.data(), 51, 52, 53, .51f, .52f, .53f);
    auro_matic_v3_XinN_parameter_Dynamic_set_surround_HC_T(
        block.data(), 61, 62, 63, .61f, .62f, .63f, .64f, .65f, .66f);
    if (i32(0u) != 61 || i32(4u) != 41 || i32(8u) != 51
        || i32(12u) != 62 || i32(16u) != 52 || i32(20u) != 42
        || i32(24u) != 63 || i32(28u) != 43 || i32(32u) != 53
        || !close(f32(48u), .11f) || !close(f32(52u), .12f)
        || !close(f32(56u), .21f) || !close(f32(60u), .22f)
        || !close(f32(64u), .31f) || !close(f32(68u), .32f)
        || !close(f32(72u), .41f) || !close(f32(76u), .42f)
        || !close(f32(80u), .43f) || !close(f32(84u), .61f)
        || !close(f32(88u), .63f) || !close(f32(92u), .65f)
        || !close(f32(96u), .51f) || !close(f32(100u), .52f)
        || !close(f32(104u), .53f) || !close(f32(108u), .62f)
        || !close(f32(112u), .64f) || !close(f32(116u), .66f)) {
        detail = "setter offsets mismatch";
        return false;
    }
    auro_matic_v3_XinN_parameter_Dynamic_mute_stereo_Xs(block.data());
    auro_matic_v3_XinN_parameter_Dynamic_mute_stereo_HX(block.data());
    auro_matic_v3_XinN_parameter_Dynamic_mute_stereo_HXs(block.data());
    auro_matic_v3_XinN_parameter_Dynamic_mute_surround_HX(block.data());
    auro_matic_v3_XinN_parameter_Dynamic_mute_surround_HXs(block.data());
    auro_matic_v3_XinN_parameter_Dynamic_mute_surround_HC_T(block.data());
    if (i32(0u) != 0 || i32(4u) != 0 || i32(8u) != 0
        || i32(12u) != 0 || i32(16u) != 0 || i32(20u) != 0
        || i32(24u) != 0 || i32(28u) != 0 || i32(32u) != 0
        || *reinterpret_cast<const std::uint64_t*>(block.data() + 48u) != 0u
        || *reinterpret_cast<const std::uint64_t*>(block.data() + 56u) != 0u
        || *reinterpret_cast<const std::uint64_t*>(block.data() + 64u) != 0u
        || *reinterpret_cast<const std::uint64_t*>(block.data() + 72u) != 0u
        || *reinterpret_cast<const std::uint64_t*>(block.data() + 96u) != 0u
        || *reinterpret_cast<const std::uint64_t*>(block.data() + 108u) != 0u) {
        detail = "mute offsets mismatch";
        return false;
    }
    detail = "all stereo/surround setter and mute offsets PASS";
    return true;
}

bool auromatic_front_stage_self_test(std::string& detail) {
    std::array<std::uint8_t, 16u> front{};
    std::array<std::uint8_t, 128u> gain{};
    std::array<std::uint8_t, 128u> silence{};
    std::array<float, 32u> left{};
    std::array<float, 32u> right{};
    std::array<float, 32u> center{};
    std::array<float, 32u> source_left{};
    std::array<float, 32u> source_right{};
    std::array<float, 32u> source_center{};
    std::array<float, 32u> gains{};
    left.fill(2.0f);
    right.fill(4.0f);
    center.fill(6.0f);
    source_left.fill(10.0f);
    source_right.fill(20.0f);
    source_center.fill(30.0f);
    gains.fill(0.5f);
    float* outputs[31u]{};
    outputs[0u] = left.data();
    outputs[1u] = right.data();
    outputs[2u] = center.data();

    if (!auro_matic_v3_FrontManager_Output_construct_object(front.data())) {
        detail = "FrontManager Output construct failed";
        return false;
    }
    *reinterpret_cast<float*>(front.data() + 0u) = 2.0f;
    *reinterpret_cast<float*>(front.data() + 4u) = 0.5f;
    *reinterpret_cast<std::uint32_t*>(front.data() + 8u) = 1u;
    if (auro_matic_v3_FrontManager_Output_process_object(
            front.data(), outputs, source_left.data(), source_right.data(),
            source_center.data(), gains.data()) != 0
        || std::fabs(left[0u] - 7.5f) > 1.0e-6f
        || std::fabs(right[0u] - 15.0f) > 1.0e-6f
        || std::fabs(center[0u] - 45.0f) > 1.0e-6f) {
        detail = "FrontManager Output routing/scaling mismatch";
        return false;
    }
    if (!auro_matic_v3_Gain_construct_object(gain.data())
        || auro_matic_v3_Gain_configure_object(gain.data(), 0x5u) != 0
        || auro_matic_v3_Gain_process_object(gain.data(), outputs, gains.data()) != 0
        || std::fabs(left[0u] - 3.75f) > 1.0e-6f
        || std::fabs(center[0u] - 22.5f) > 1.0e-6f
        || std::fabs(right[0u] - 15.0f) > 1.0e-6f) {
        detail = "Gain channel-mask mismatch count="
            + std::to_string(*reinterpret_cast<const std::uint32_t*>(gain.data() + 124u))
            + " values=" + std::to_string(left[0u]) + ","
            + std::to_string(center[0u]);
        return false;
    }
    if (!auro_matic_Silence_construct_object(silence.data())
        || auro_matic_Silence_configure_object(silence.data(), 0x5u) != 0
        || auro_matic_Silence_process_object(silence.data(), outputs, true) != 0
        || left[0u] != 0.0f || center[0u] != 0.0f
        || right[0u] != 15.0f) {
        detail = "Silence channel-mask mismatch";
        return false;
    }
    detail = "FrontManager Output, Gain, and Silence routing PASS";
    return true;
}

// Native auro_matic_v3_Gain_t_construct/configure @ 0x550B80/0x550BB0.
std::int64_t auro_matic_v3_Gain_construct_object(std::uint8_t* gain) {
    if (!gain)
        return 0;
    // Native constructor writes only the channel-list count.
    *reinterpret_cast<std::uint32_t*>(gain + 124u) = 0u;
    return reinterpret_cast<std::int64_t>(gain);
}

std::int64_t auro_matic_v3_Gain_configure_object(
    std::uint8_t* gain,
    std::uint32_t channel_mask) {
    if (!gain)
        return 1;
    std::uint32_t count = 0u;
    for (std::uint32_t id = 0u; id < 31u; ++id) {
        if ((channel_mask & (1u << id)) == 0u)
            continue;
        if (count < 31u)
            *reinterpret_cast<std::uint32_t*>(gain + count * 4u) = id;
        ++count;
    }
    *reinterpret_cast<std::uint32_t*>(gain + 124u) = std::min(count, 31u);
    return 0;
}

// Native upmix::Gain::prepare @ 0x35CDF0.  This writes the step-local
// channel list consumed by Gain::process; it is intentionally separate from
// the compact object-level Gain::configure above.
std::int64_t auro_matic_v3_Gain_prepare_step(
    std::uint8_t* step,
    std::uint32_t channel_mask) {
    if (!step)
        return 1;
    std::uint32_t count = 0u;
    for (std::uint32_t id = 0u; id < 31u; ++id) {
        if ((channel_mask & (1u << id)) == 0u)
            continue;
        if (count >= 31u)
            return 1;
        *reinterpret_cast<std::uint32_t*>(step + 48u + count * 4u) = id;
        ++count;
    }
    *reinterpret_cast<std::uint64_t*>(step + 40u) = count;
    return 0;
}

std::int64_t auro_matic_v3_Gain_reset_audio_state_object(std::uint8_t* gain) {
    return gain ? 0 : 1;
}

// Native auro_matic_Silence_t_construct/configure/reset use the same compact
// channel-list ABI as Gain.  Reset is audio-state-only and preserves config.
std::int64_t auro_matic_Silence_construct_object(std::uint8_t* silence) {
    if (!silence)
        return 0;
    std::memset(silence, 0, 128u);
    return reinterpret_cast<std::int64_t>(silence);
}

std::int64_t auro_matic_Silence_configure_object(
    std::uint8_t* silence,
    std::uint32_t channel_mask) {
    if (!silence)
        return 1;
    std::uint32_t count = 0u;
    for (std::uint32_t id = 0u; id < 31u; ++id) {
        if ((channel_mask & (1u << id)) == 0u)
            continue;
        if (count < 31u)
            *reinterpret_cast<std::uint32_t*>(silence + count * 4u) = id;
        ++count;
    }
    *reinterpret_cast<std::uint32_t*>(silence + 124u) = std::min(count, 31u);
    return 0;
}

// Native upmix::Silence::prepare @ 0x35D1F0.  The input layout comes from
// the current block state, while the output layout is supplied by the
// configurator.
std::int64_t auro_matic_Silence_prepare_step(
    std::uint8_t* step,
    std::uint32_t output_layout) {
    if (!step)
        return 1;
    const std::uint64_t block_state =
        *reinterpret_cast<const std::uint64_t*>(step + 8u);
    if (!block_state)
        return 1;
    const std::uint32_t input_layout =
        *reinterpret_cast<const std::uint32_t*>(
            static_cast<std::uintptr_t>(block_state));
    *reinterpret_cast<std::uint32_t*>(step + 36u) = output_layout;
    *reinterpret_cast<std::uint32_t*>(step + 16u) = input_layout & ~output_layout;
    return 0;
}

std::int64_t auro_matic_Silence_reset_audio_state_object(std::uint8_t* silence) {
    return silence ? 0 : 1;
}

std::int64_t auro_matic_v3_Gain_process_object(
    const std::uint8_t* gain,
    float* const* io_channels,
    const float* gains) {
    if (!gain || !io_channels || !gains)
        return 1;
    bool unity = true;
    for (std::uint32_t i = 0u; i < 32u; ++i) {
        if (gains[i] != 1.0f) {
            unity = false;
            break;
        }
    }
    if (unity)
        return 0;
    const std::uint32_t count = std::min(
        *reinterpret_cast<const std::uint32_t*>(gain + 124u), 31u);
    for (std::uint32_t n = 0u; n < count; ++n) {
        const std::uint32_t id = *reinterpret_cast<const std::uint32_t*>(gain + n * 4u);
        if (id >= 31u || !io_channels[id])
            continue;
        for (std::uint32_t i = 0u; i < 32u; ++i)
            io_channels[id][i] *= gains[i];
    }
    return 0;
}

std::int64_t auro_matic_Silence_process_object(
    const std::uint8_t* silence,
    float* const* io_channels,
    bool /* transition_silent */) {
    if (!silence || !io_channels)
        return 1;
    const std::uint32_t count = std::min(
        *reinterpret_cast<const std::uint32_t*>(silence + 124u), 31u);
    for (std::uint32_t n = 0u; n < count; ++n) {
        const std::uint32_t id = *reinterpret_cast<const std::uint32_t*>(silence + n * 4u);
        if (id < 31u && io_channels[id])
            std::memset(io_channels[id], 0, 32u * sizeof(float));
    }
    return 0;
}

// Native auro_matic_v3_3d3d_t_construct/reset_audio_state @
// 0x550E50/0x550F20.  The four plan pointers are part of the object at
// offsets +368/+376/+384/+392; this is the object layout, not the pipeline
// step wrapper whose embedded object starts at step +40.
std::int64_t auro_matic_v3_3d3d_construct_object(
    std::uint8_t* state,
    const std::uint64_t* line_plans_4) {
    if (!state || !line_plans_4)
        return 0;
    std::memset(state, 0, 400u);
    for (std::size_t i = 0; i < 4u; ++i)
        *reinterpret_cast<std::uint64_t*>(state + 368u + i * 8u) = line_plans_4[i];
    if (line_plans_4[0u]
        && !auromatic_3d3d_line2_construct(
            reinterpret_cast<Auromatic3d3dLine2*>(state),
            reinterpret_cast<float*>(static_cast<std::uintptr_t>(line_plans_4[0u])),
            reinterpret_cast<float*>(static_cast<std::uintptr_t>(line_plans_4[0u])) + 324u))
        return 0;
    if (line_plans_4[1u]
        && !auromatic_3d3d_line3_construct(
            reinterpret_cast<Auromatic3d3dLine3*>(state + 64u),
            reinterpret_cast<float*>(static_cast<std::uintptr_t>(line_plans_4[1u])),
            reinterpret_cast<float*>(static_cast<std::uintptr_t>(line_plans_4[1u])) + 726u))
        return 0;
    if (line_plans_4[2u]
        && !auromatic_3d3d_line3_construct(
            reinterpret_cast<Auromatic3d3dLine3*>(state + 128u),
            reinterpret_cast<float*>(static_cast<std::uintptr_t>(line_plans_4[2u])),
            reinterpret_cast<float*>(static_cast<std::uintptr_t>(line_plans_4[2u])) + 726u))
        return 0;
    if (line_plans_4[3u]
        && !auromatic_3d3d_line4_construct(
            reinterpret_cast<Auromatic3d3dLine4*>(state + 192u),
            reinterpret_cast<float*>(static_cast<std::uintptr_t>(line_plans_4[3u]))))
        return 0;
    *reinterpret_cast<std::uint32_t*>(state + 344u) = 0u;
    return reinterpret_cast<std::int64_t>(state);
}

std::int64_t auro_matic_v3_3d3d_reset_audio_state_object(std::uint8_t* state) {
    if (!state)
        return 1;
    // Native auro_matic_v3_3d3d_reset_audio_state @ 0x550F20 does not
    // consult a constructed flag here. +344 belongs to the smoothing object,
    // so treating a zero value as "not constructed" leaves the four delay
    // graphs dirty after a reset and only becomes audible when their delayed
    // samples emerge many blocks later.
    if (*reinterpret_cast<std::uint64_t*>(state + 368u)) {
        auto* line = reinterpret_cast<Auromatic3d3dLine2*>(state);
        std::memset(line->filter_state, 0, sizeof(line->filter_state));
        auromatic_3d3d_delayline_clear(&line->first_delay);
        auromatic_3d3d_delayline_clear(&line->second_delay);
    }
    if (*reinterpret_cast<std::uint64_t*>(state + 376u)) {
        auto* line = reinterpret_cast<Auromatic3d3dLine3*>(state + 64u);
        std::memset(line->filter_state, 0, sizeof(line->filter_state));
        auromatic_3d3d_delayline_clear(&line->first_delay);
        auromatic_3d3d_delayline_clear(&line->second_delay);
    }
    if (*reinterpret_cast<std::uint64_t*>(state + 384u)) {
        auto* line = reinterpret_cast<Auromatic3d3dLine3*>(state + 128u);
        std::memset(line->filter_state, 0, sizeof(line->filter_state));
        auromatic_3d3d_delayline_clear(&line->first_delay);
        auromatic_3d3d_delayline_clear(&line->second_delay);
    }
    if (*reinterpret_cast<std::uint64_t*>(state + 392u)) {
        auto* line = reinterpret_cast<Auromatic3d3dLine4*>(state + 192u);
        std::memset(line->first.filter_state, 0, sizeof(line->first.filter_state));
        std::memset(line->second.filter_state, 0, sizeof(line->second.filter_state));
        auromatic_3d3d_delayline_clear(&line->first.delay);
        auromatic_3d3d_delayline_clear(&line->second.delay);
    }
    // auro_audio_Smooth_fl32_inst_set_current(state + 352, 0.0f).
    *reinterpret_cast<float*>(state + 352u) = 0.0f;
    return 0;
}

std::int64_t auro_matic_v3_3d3d_process_object(
    std::uint8_t* state,
    void** channel_span_31) {
    if (!state || !channel_span_31)
        return 1;
    // manager_process_upmix_3d3d already contains the native callback
    // selection and all Line2/3/4 routing. Present it with the v4 step view
    // it expects, while preserving the object state after the call.
    std::array<std::uint8_t, 14784u> step_view{};
    std::memcpy(step_view.data() + 40u, state, 400u);
    *reinterpret_cast<std::uint32_t*>(step_view.data() + 40u + 400u) =
        kAuromatic3d3dConstructed;
    constexpr std::size_t state_plan_offsets[] = {368u, 376u, 384u, 392u};
    constexpr std::size_t step_plan_offsets[] = {448u, 2808u, 8792u, 14776u};
    for (std::size_t i = 0u; i < 4u; ++i) {
        *reinterpret_cast<float**>(step_view.data() + step_plan_offsets[i]) =
            *reinterpret_cast<float**>(state + state_plan_offsets[i]);
    }
    const auto rc = manager_process_upmix_3d3d(step_view.data(), channel_span_31);
    std::memcpy(state, step_view.data() + 40u, 400u);
    return rc;
}

std::int64_t auro_matic_v3_Engine_construct_objects(
    const AuromaticV3EngineObjectParts& parts) {
    if (!parts.output_gate || !parts.front_manager || !parts.xinn
        || !parts.p3d3d || !parts.gain || !parts.silence
        || !parts.xinn_memory_args || !parts.p3d3d_line_plans)
        return 0;
    if (!auro_matic_v3_OutputGate_construct_portable(parts.output_gate))
        return 0;
    if (!auro_matic_v3_FrontManager_construct_portable(parts.front_manager))
        return 0;
    if (!auro_matic_v3_XinN_fl32_construct(
            parts.xinn, parts.xinn_memory_args))
        return 0;
    if (!auro_matic_v3_3d3d_construct_object(
            parts.p3d3d, parts.p3d3d_line_plans))
        return 0;
    // Confirmed by auro_matic_v3_Engine_t_construct in
    // decopiled/libauro(1).so.c: after all six members are constructed the
    // native body sets OutputGate gain to exactly 1.0.  The compact Gain and
    // Silence constructors have no additional state visible in this port;
    // clear their object storage before configure fills the channel lists.
    if (!auro_matic_v3_Gain_construct_object(parts.gain)
        || !auro_matic_Silence_construct_object(parts.silence))
        return 0;
    *reinterpret_cast<float*>(parts.output_gate + 2148u) = 1.0f;
    return reinterpret_cast<std::int64_t>(parts.output_gate);
}

// Exact constructor bodies from libauro.so.c.  RemapOutput_t_construct uses
// the identity callback for both directions; the callback itself is the
// decompiled sub_4CE0F0 and is also restored by set_output_layout for input
// layouts 0 and 3.
bool manager_remap_output_identity_callback(
    std::uint32_t channel_id,
    std::uint32_t* mapped_channel_id) {
    if (!mapped_channel_id)
        return false;
    *mapped_channel_id = channel_id;
    return channel_id < 0xFu;
}

void manager_construct_remap_output(std::uint8_t* remap) {
    if (!remap)
        return;
    *reinterpret_cast<std::uint64_t*>(remap + 0u) = 0u;
    *reinterpret_cast<bool (**)(std::uint32_t, std::uint32_t*)>(
        remap + 8u) = &manager_remap_output_identity_callback;
    *reinterpret_cast<bool (**)(std::uint32_t, std::uint32_t*)>(
        remap + 16u) = &manager_remap_output_identity_callback;
    *reinterpret_cast<std::uint32_t*>(remap + 24u) = 0u;
}

void manager_construct_preset_manager(std::uint8_t* preset_manager) {
    if (preset_manager)
        std::memset(preset_manager, 0, 15u * sizeof(std::uint64_t) * 2u);
}

std::int64_t auro_a3deng_v3_pipeline_Manager_construct_upmix_objects(
    std::uint8_t* manager,
    std::uint8_t* step_chain,
    const std::uint64_t* xinn_memory_args,
    const std::uint64_t* p3d3d_line_plans) {
    if (!manager || !step_chain || !xinn_memory_args || !p3d3d_line_plans)
        return 1;

    // Manager_t_construct @ 0x4CB9B0 clears the manager header before it
    // installs the shared chain.  These are the constructor defaults visible
    // in the current decompilation; keeping them here is important because
    // Manager_process_audio consults +480 even when every optional step is
    // inactive.
    std::memset(manager, 0, 0x348u + 8u);
    *reinterpret_cast<std::uint64_t*>(manager + 848u) =
        reinterpret_cast<std::uint64_t>(step_chain);
    *reinterpret_cast<std::uint64_t*>(manager + 480u) =
        0x100000001ull;
    constexpr std::uint32_t kInitialHeadroom = 0x3F350481u;
    *reinterpret_cast<std::uint32_t*>(manager + 288u) = 1u;
    for (std::size_t offset : {296u, 312u, 328u, 344u})
        *reinterpret_cast<std::uint32_t*>(manager + offset) = kInitialHeadroom;
    *reinterpret_cast<std::uint32_t*>(manager + 352u) = 1060439169u;
    *reinterpret_cast<std::uint32_t*>(manager + 492u) = 0u;
    *reinterpret_cast<std::uint32_t*>(manager + 560u) = 0u;

    // Manager_t_construct @ 0x4CB9B0 constructs these owning objects before
    // any plan is configured.  Their bodies are fully visible in the
    // decompile, so initialize them here even though the portable runtime
    // currently activates only the confirmed Upmix step.
    manager_construct_remap_output(manager + 136u);
    manager_construct_preset_manager(manager + 608u);

    // The caller owns the complete chain.  The remaining step bodies stay
    // zero until their native constructors are ported; that is the same
    // pre-Manager_configure state, not a guessed processing fallback.
    *reinterpret_cast<std::uint64_t*>(manager + kManagerOffStepChain478078) =
        reinterpret_cast<std::uint64_t>(step_chain);
    *reinterpret_cast<std::uint32_t*>(manager + kManagerOffBypass478078) = 0u;
    manager_construct_confirmed_simple_step_headers(step_chain, manager);
    auto* step = step_chain + 7944u;
    std::memset(step, 0, 5200u);
    *reinterpret_cast<std::uint64_t*>(step + 0u) =
        ::auro_codec_v3_ida::kAuroA3dengV3PipelineStepUpmix_configure;
    *reinterpret_cast<std::uint64_t*>(step + 8u) =
        ::auro_codec_v3_ida::kAuroA3dengV3PipelineStepUpmix_update;
    *reinterpret_cast<std::uint64_t*>(step + 16u) =
        ::auro_codec_v3_ida::kAuroA3dengV3PipelineStepUpmix_process;
    *reinterpret_cast<std::uint64_t*>(step + 24u) =
        ::auro_codec_v3_ida::kAuroA3dengV3PipelineStepUpmix_reset;
    auto* engine = step + 264u;
    AuromaticV3EngineObjectParts parts{};
    parts.output_gate = engine;
    parts.front_manager = engine + 2152u;
    parts.xinn = engine + 3344u;
    parts.p3d3d = engine + 4192u;
    parts.gain = engine + 4592u;
    parts.silence = engine + 4720u;
    parts.xinn_memory_args = xinn_memory_args;
    parts.p3d3d_line_plans = p3d3d_line_plans;
    parts.output_masks = reinterpret_cast<std::uint32_t*>(engine + 4848u);
    if (!auro_matic_v3_Engine_construct_objects(parts))
        return 1;
    return 0;
}

std::int64_t auro_a3deng_v3_pipeline_Manager_construct_upmix_native(
    std::uint8_t* manager,
    std::uint8_t* step_chain,
    std::uint64_t storage,
    std::uint64_t engine_arg3,
    std::uint64_t engine_arg4) {
    if (!manager || !step_chain || storage == 0u)
        return 1;
    const std::uint64_t xinn_memory_args[5u] = {
        storage,
        storage + 40128u,
        storage + 80256u,
        engine_arg3,
        engine_arg4,
    };
    const std::uint64_t p3d3d_line_plans[4u] = {
        storage + 459200u,
        storage + 461560u,
        storage + 467544u,
        0u,
    };
    return auro_a3deng_v3_pipeline_Manager_construct_upmix_objects(
        manager, step_chain, xinn_memory_args, p3d3d_line_plans);
}

std::int64_t auro_a3deng_v3_pipeline_Manager_bind_upmix_block_state(
    std::uint32_t* manager_base,
    std::uint64_t block_state) {
    if (!manager_base || block_state == 0u)
        return 1;
    auto* manager = reinterpret_cast<std::uint8_t*>(manager_base);
    auto* step_chain = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(
        *reinterpret_cast<std::uint64_t*>(manager + kManagerOffStepChain478078)));
    if (!step_chain)
        return 1;
    *reinterpret_cast<std::uint64_t*>(step_chain + 7944u + 32u) = block_state;
    return 0;
}

std::int64_t auro_a3deng_v3_pipeline_Manager_configure_upmix_objects(
    std::uint32_t* manager_base,
    std::uint32_t sample_rate,
    const std::uint32_t* engine_params) {
    if (!manager_base || !engine_params || sample_rate == 0u)
        return 1;
    auto* manager = reinterpret_cast<std::uint8_t*>(manager_base);
    auto* step_chain = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(
        *reinterpret_cast<std::uint64_t*>(manager + kManagerOffStepChain478078)));
    if (!step_chain)
        return 1;
    auto* engine = step_chain + 7944u + 264u;
    AuromaticV3EngineObjectParts parts{};
    parts.output_gate = engine;
    parts.front_manager = engine + 2152u;
    parts.xinn = engine + 3344u;
    parts.p3d3d = engine + 4192u;
    parts.gain = engine + 4592u;
    parts.silence = engine + 4720u;
    parts.output_masks = reinterpret_cast<std::uint32_t*>(engine + 4848u);
    const auto rc = auro_matic_v3_Engine_configure_objects(
        parts, sample_rate, engine_params);
    return rc;
}

std::int64_t auro_a3deng_v3_pipeline_Upmix_configure(
    std::uint8_t* step,
    const std::uint8_t* block_state,
    const std::uint8_t* config_blob) {
    if (!step || !block_state || !config_blob)
        return 1;
    // Upmix::configure @ 0x4CF440 only accepts an initial, non-reentrant
    // configuration. These guards are deliberately read from the same two
    // objects as native: block_state+12 and step+52.
    if (*reinterpret_cast<const std::uint32_t*>(block_state + 12u) != 0u
        || *reinterpret_cast<const std::uint32_t*>(step + 52u) != 0u)
        return 1;
    std::memcpy(step + 64u, config_blob + 8u, 0xC8u);
    auto* engine = step + 264u;
    AuromaticV3EngineObjectParts parts{};
    parts.output_gate = engine;
    parts.front_manager = engine + 2152u;
    parts.xinn = engine + 3344u;
    parts.p3d3d = engine + 4192u;
    parts.gain = engine + 4592u;
    parts.silence = engine + 4720u;
    parts.output_masks = reinterpret_cast<std::uint32_t*>(engine + 4848u);
    const auto sample_rate =
        *reinterpret_cast<const std::uint32_t*>(block_state + 8u);
    const auto rc = auro_matic_v3_Engine_configure_objects(
        parts, sample_rate, reinterpret_cast<const std::uint32_t*>(step + 64u));
    if (rc != 0)
        return rc;
    *reinterpret_cast<std::uint32_t*>(step + 40u) =
        *reinterpret_cast<const std::uint32_t*>(step + 68u);
    return 0;
}

std::int64_t auro_matic_v3_Engine_configure_objects(
    const AuromaticV3EngineObjectParts& parts,
    std::uint32_t sample_rate,
    const std::uint32_t* engine_params) {
    if (!parts.output_gate || !parts.front_manager || !parts.xinn
        || !parts.p3d3d || !parts.gain || !parts.silence
        || !engine_params || sample_rate == 0u)
        return 1;
    if (auro_matic_v3_OutputGate_configure(
            parts.output_gate, sample_rate, engine_params[0u]) != 0)
        return 1;
    if (auro_matic_v3_FrontManager_configure_portable(
            parts.front_manager,
            sample_rate,
            reinterpret_cast<const std::uint8_t*>(engine_params + 2u)) != 0)
        return 1;
    if (auro_matic_v3_XinN_fl32_configure(
            reinterpret_cast<std::uint8_t*>(parts.xinn),
            sample_rate,
            reinterpret_cast<const std::uint64_t*>(engine_params + 28u)) != 0)
        return 1;
    // Reuse the already ported object layout/configuration logic.  The native
    // p3D3D configurator is a step-shaped entry point, but its embedded object
    // starts at +40 and is byte-for-byte the confirmed Engine object state.
    std::array<std::uint8_t, 512u> step_view{};
    std::memcpy(step_view.data() + 40u, parts.p3d3d, 400u);
    *reinterpret_cast<std::uint32_t*>(step_view.data() + 40u + 400u) =
        kAuromatic3d3dConstructed;
    if (manager_configure_upmix_3d3d(
            step_view.data(), sample_rate, engine_params + 36u) != 0)
        return 1;
    std::memcpy(parts.p3d3d, step_view.data() + 40u, 400u);
    if (auro_matic_v3_Gain_configure_object(
            parts.gain, engine_params[47u]) != 0)
        return 1;
    if (auro_matic_Silence_configure_object(
            parts.silence, engine_params[46u]) != 0)
        return 1;
    if (parts.output_masks != nullptr) {
        parts.output_masks[0u] = engine_params[0u];
        parts.output_masks[1u] = engine_params[1u];
    }
    return auro_matic_v3_Engine_reset_audio_state_objects(parts) == 0 ? 0 : 1;
}

std::int64_t auro_matic_v3_Engine_reset_audio_state_objects(
    const AuromaticV3EngineObjectParts& parts) {
    if (!parts.output_gate || !parts.front_manager || !parts.xinn
        || !parts.p3d3d || !parts.gain || !parts.silence)
        return 1;
    (void)auro_matic_v3_OutputGate_reset_audio_state(parts.output_gate);
    (void)auro_centergen_v3_Processor_reset_audio_state(parts.front_manager);
    // FrontManager::Input_reset_audio_state (0x552F20) adds a fade-in
    // reset after the underlying Centergen reset. Do not change Centergen's
    // standalone reset, which is also used by the headphone Core.
    const auto front_format = *reinterpret_cast<const std::uint32_t*>(
        parts.front_manager + 84u);
    if (front_format == 2u) {
        std::memset(parts.front_manager + 1048u, 0, 16u);
    } else if (front_format <= 1u) {
        std::memset(parts.front_manager + 1048u, 0, 4u);
        std::memset(parts.front_manager + 1056u, 0, 4u);
    }
    (void)auro_matic_v3_XinN_fl32_reset_audio_state(parts.xinn);
    (void)auro_matic_v3_3d3d_reset_audio_state_object(parts.p3d3d);
    (void)auro_matic_v3_Gain_reset_audio_state_object(parts.gain);
    (void)auro_matic_Silence_reset_audio_state_object(parts.silence);
    return 0;
}

std::int64_t auro_matic_v3_Engine_update_xinn_preset(
    const AuromaticV3EngineObjectParts& parts,
    std::uint64_t preset) {
    if (!parts.xinn)
        return 1;
    auro_matic_v3_XinN_fl32_set_preset(parts.xinn, preset);
    return 0;
}

std::int64_t auro_matic_v3_Engine_update_xinn_parameters(
    const AuromaticV3EngineObjectParts& parts,
    const std::uint8_t* dynamic_120) {
    if (!parts.xinn || !dynamic_120)
        return 1;
    return static_cast<std::int64_t>(
        auro_matic_v3_XinN_fl32_set_dynamic_parameters(
            parts.xinn, dynamic_120));
}

std::int64_t auro_matic_v3_Engine_update_front_manager(
    const AuromaticV3EngineObjectParts& parts,
    const std::uint8_t* dynamic_56) {
    if (!parts.front_manager || !dynamic_56)
        return 1;
    auro_centergen_v3_Processor_set_dynamic_parameters(
        parts.front_manager, dynamic_56);
    return 0;
}

// Native auro_matic_v3_Engine_update_peak_amplitude @ 0x550980.  This is a
// metadata path: it updates the supplied 31-channel peak record in the same
// stage order as the Engine and never touches audio samples.
std::int64_t auro_matic_v3_Engine_update_peak_amplitude_objects(
    const AuromaticV3EngineObjectParts& parts,
    float* peak_31) {
    if (!parts.front_manager || !parts.xinn || !parts.p3d3d
        || !parts.silence || !peak_31)
        return 1;

    float front_left = peak_31[0u];
    float front_right = peak_31[1u];
    float front_center = peak_31[2u];
    auro_centergen_v3_Processor_t_update_peak_amplitude(
        &front_left, &front_right, &front_center);

    auro_matic_v3_XinN_fl32_update_peak_amplitude(
        parts.xinn, peak_31);

    // The p3D3D peak helper uses the v4 step view, while the Engine owns the
    // same 400-byte object directly at +4192.  Keep the adapter byte-for-byte
    // so callback table and coefficient offsets remain native.
    std::array<std::uint8_t, 512u> step_view{};
    std::array<std::uint8_t, 128u> info{};
    std::memcpy(step_view.data() + 40u, parts.p3d3d, 400u);
    std::memcpy(info.data() + 4u, peak_31, 31u * sizeof(float));
    *reinterpret_cast<std::uint32_t*>(step_view.data() + 440u) =
        kAuromatic3d3dConstructed;
    manager_update_upmix_3d3d_peak(step_view.data(), info.data());
    std::memcpy(parts.p3d3d, step_view.data() + 40u, 400u);
    std::memcpy(peak_31, info.data() + 4u, 31u * sizeof(float));

    (void)auro_matic_v3_FrontManager_Output_update_peak_amplitude(
        parts.front_manager + 1176u,
        peak_31,
        front_left,
        front_right,
        front_center);

    const std::uint32_t count = std::min(
        *reinterpret_cast<const std::uint32_t*>(parts.silence + 124u), 31u);
    for (std::uint32_t i = 0u; i < count; ++i) {
        const std::uint32_t id =
            *reinterpret_cast<const std::uint32_t*>(parts.silence + i * 4u);
        if (id < 31u)
            peak_31[id] = 0.0f;
    }
    return 0;
}

std::int64_t auro_matic_v3_Engine_process_objects(
    const AuromaticV3EngineObjectParts& parts,
    float* const* io_channels_31,
    AuromaticV3EngineStageTrace* trace) {
    if (!parts.output_gate || !parts.front_manager || !parts.xinn
        || !parts.p3d3d || !parts.gain || !parts.silence
        || !io_channels_31)
        return 1;

    // Engine::process @ 0x5500F0: OutputGate, front input, XinN, p3D3D,
    // front output, Gain, Silence.  The object adapter supplies the same
    // three front-manager record vectors explicitly for this one subblock.
    std::array<float, 32u> front_left{};
    std::array<float, 32u> front_right{};
    std::array<float, 32u> front_center{};
    std::array<float*, 31u> channels{};
    std::array<void*, 31u> spans{};
    for (std::size_t i = 0u; i < 31u; ++i) {
        channels[i] = io_channels_31[i];
        spans[i] = io_channels_31[i];
    }
    std::array<float, 32u> output_gate_gains{};
    std::uint32_t output_silent = 0u;
    std::uint32_t transition_silent = 0u;
    // OutputGate's boolean result is the native output-silent state, not an
    // error code. A closed gate uses Engine_process's early-return path.
    (void)auro_matic_v3_OutputGate_process(
        parts.output_gate, spans.data(), output_gate_gains.data(),
        &output_silent, &transition_silent);
    const auto snapshot = [&](auto& target) {
        if (!trace) return;
        for (std::size_t slot = 0; slot != 31u; ++slot)
            if (channels[slot])
                std::memcpy(target[slot].data(), channels[slot], 32u * sizeof(float));
    };
    if (trace) snapshot(trace->after_output_gate);

    // Engine::process clears only (+4852) & ~(+4848) on an output-silent
    // subblock, skips all DSP stages, and resets only on transition.
    if (output_silent != 0u && parts.output_masks != nullptr) {
        const std::uint32_t clear_mask =
            parts.output_masks[1u] & ~parts.output_masks[0u];
        for (std::uint32_t id = 0u; id < 31u; ++id) {
            if ((clear_mask & (1u << id)) != 0u && channels[id])
                std::memset(channels[id], 0, 32u * sizeof(float));
        }
        if (transition_silent != 0u)
            return auro_matic_v3_Engine_reset_audio_state_objects(parts) == 0 ? 0 : 1;
        return 0;
    }
    void* front_input[31u]{};
    for (std::size_t i = 0u; i < 31u; ++i)
        front_input[i] = io_channels_31[i];
    if (auro_matic_v3_FrontManager_Input_process(
            parts.front_manager,
            front_input,
            front_left.data(), front_right.data(), front_center.data()) != 0)
        return 1;
    if (trace) snapshot(trace->after_front_input);

    if (auro_matic_v3_XinN_fl32_process(
            reinterpret_cast<std::uint64_t>(parts.xinn),
            spans.data()) != 0)
        return 1;
    if (trace) snapshot(trace->after_xinn);
    if (trace) {
        const auto scratch_u64 = *reinterpret_cast<const std::uint64_t*>(parts.xinn + 40u);
        if (scratch_u64 != 0u) {
            const auto* scratch = reinterpret_cast<const float*>(
                static_cast<std::uintptr_t>(scratch_u64));
            std::memcpy(trace->xinn_early_a.data(), scratch, 192u * sizeof(float));
            std::memcpy(trace->xinn_early_b.data(), scratch + 192u, 192u * sizeof(float));
            std::memcpy(trace->xinn_late.data(), scratch + 384u, 192u * sizeof(float));
        }
    }
    if (auro_matic_v3_3d3d_process_object(
            parts.p3d3d,
            spans.data()) != 0)
        return 1;
    if (trace) snapshot(trace->after_3d3d);
    if (auro_matic_v3_FrontManager_Output_process_object(
            parts.front_manager + 1176u,
            channels.data(),
            front_left.data(), front_right.data(), front_center.data(),
            output_gate_gains.data()) != 0)
        return 1;
    if (trace) snapshot(trace->after_front_output);
    if (trace)
        std::memcpy(trace->gain_curve.data(), output_gate_gains.data(),
                    output_gate_gains.size() * sizeof(float));
    if (auro_matic_v3_Gain_process_object(
            parts.gain, channels.data(), output_gate_gains.data()) != 0)
        return 1;
    if (trace) snapshot(trace->after_gain);
    if (auro_matic_Silence_process_object(
            parts.silence, channels.data(), false) != 0)
        return 1;
    if (transition_silent != 0u)
        return auro_matic_v3_Engine_reset_audio_state_objects(parts) == 0 ? 0 : 1;
    return 0;
}

// Native Upmix::update @ 0x4CF660 forwards the Manager info record at +4 to
// Engine_update_peak_amplitude.  Keep the outer-step adapter explicit so the
// Manager lifecycle does not silently drop peak metadata.
std::int64_t auro_a3deng_v3_pipeline_Upmix_update_peak_amplitude(
    std::uint8_t* step,
    float* peak_31) {
    if (!step || !peak_31)
        return 1;
    auto* engine = step + 264u;
    AuromaticV3EngineObjectParts parts{};
    parts.output_gate = engine;
    parts.front_manager = engine + 2152u;
    parts.xinn = engine + 3344u;
    parts.p3d3d = engine + 4192u;
    parts.gain = engine + 4592u;
    parts.silence = engine + 4720u;
    return auro_matic_v3_Engine_update_peak_amplitude_objects(parts, peak_31);
}

std::int64_t AuromaticV3UpmixRuntime::construct(
    std::uint64_t engine_arg3,
    std::uint64_t engine_arg4) {
    manager_.assign(kManagerBytes, 0u);
    step_chain_.assign(kStepChainBytes, 0u);
    upmix_arena_.assign(kUpmixArenaBytes, 0u);
    block_state_.assign(kBlockStateBytes, 0u);
    xinn_scratch_.fill(0u);
    constructed_ = false;

    // Native Upmix owner supplies the 0x900-byte XinN work area. Preserve
    // explicitly supplied ABI arguments, but make the owning default follow
    // the normal in-place callback rather than the no-scratch fallback.
    if (engine_arg3 == 0u && engine_arg4 == 0u) {
        engine_arg3 = xinn_scratch_.size();
        engine_arg4 = reinterpret_cast<std::uint64_t>(xinn_scratch_.data());
    }

    const auto rc = auro_a3deng_v3_pipeline_Manager_construct_upmix_native(
        manager_.data(),
        step_chain_.data(),
        reinterpret_cast<std::uint64_t>(upmix_arena_.data()),
        engine_arg3,
        engine_arg4);
    if (rc != 0)
        return rc;
    // The outer native step reads the subblock count at block_state+4.  The
    // direct object process below remains the public one-subblock API, but
    // binding this state also makes Manager_process_audio usable by callers.
    *reinterpret_cast<std::uint32_t*>(block_state_.data() + 4u) = 1u;
    if (auro_a3deng_v3_pipeline_Manager_bind_upmix_block_state(
            reinterpret_cast<std::uint32_t*>(manager_.data()),
            reinterpret_cast<std::uint64_t>(block_state_.data())) != 0)
        return 1;
    auto* manager = manager_.data();
    *reinterpret_cast<std::uint64_t*>(manager + ::auro_engine_v4_ida::kManager_off_configure_cb) =
        reinterpret_cast<std::uint64_t>(&auro_a3deng_v3_pipeline_Manager_configure_plan_entry);
    auto* creator = reinterpret_cast<std::uint32_t*>(manager + 568u);
    creator[0] = 1u;
    creator[2] = 4u;
    creator[3] = 1u;
    creator[4] = 0u;
    (void)auro_a3deng_v3_Headroom_construct_object(step_chain_.data() + 15800u, 0.0f);
    constructed_ = true;
    return 0;
}

std::int64_t AuromaticV3UpmixRuntime::bind_block_state(
    std::uint64_t block_state) {
    if (!constructed_)
        return 1;
    return auro_a3deng_v3_pipeline_Manager_bind_upmix_block_state(
        reinterpret_cast<std::uint32_t*>(manager_.data()), block_state);
}

std::int64_t AuromaticV3UpmixRuntime::configure(
    std::uint32_t sample_rate,
    const std::uint32_t* engine_params) {
    if (!constructed_)
        return 1;
    // Upmix::configure @ 0x4CF440 obtains the rate from its bound native
    // block record at +8. Keep that record authoritative even though the
    // object-level configure seam also receives the rate explicitly.
    if (block_state_.size() < 12u || sample_rate == 0u)
        return 1;
    *reinterpret_cast<std::uint32_t*>(block_state_.data() + 8u) = sample_rate;
    std::array<std::uint8_t, 0xD0u> upmix_config{};
    std::memcpy(upmix_config.data() + 8u, engine_params, 0xC8u);
    return manager_link_upmix_plan_entry(
        step_chain_.data() + 7944u,
        block_state_.data(),
        upmix_config.data());
}

std::int64_t AuromaticV3UpmixRuntime::reset_audio_state() {
    if (!constructed_)
        return 1;
    auto* engine = step_chain_.data() + 7944u + 264u;
    AuromaticV3EngineObjectParts parts{};
    parts.output_gate = engine;
    parts.front_manager = engine + 2152u;
    parts.xinn = engine + 3344u;
    parts.p3d3d = engine + 4192u;
    parts.gain = engine + 4592u;
    parts.silence = engine + 4720u;
    return auro_matic_v3_Engine_reset_audio_state_objects(parts);
}

std::int64_t AuromaticV3UpmixRuntime::set_bypass(std::int64_t bypass) {
    if (!constructed_)
        return 1;
    // Current native auro_a3deng_v3_pipeline_step_Upmix_set_bypass
    // @ 0x4CF690 stores the flag in the outer step at +5124.  The older
    // similarly named helper below targets a different ABI and writes +4.
    *reinterpret_cast<std::uint32_t*>(
        step_chain_.data() + 7944u + 5124u) = bypass != 0 ? 1u : 0u;
    return 0;
}

std::int64_t AuromaticV3UpmixRuntime::update_xinn_preset(
    std::uint64_t preset) {
    if (!constructed_)
        return 1;
    auto* engine = step_chain_.data() + 7944u + 264u;
    AuromaticV3EngineObjectParts parts{};
    parts.xinn = engine + 3344u;
    return auro_matic_v3_Engine_update_xinn_preset(parts, preset);
}

std::int64_t AuromaticV3UpmixRuntime::update_xinn_parameters(
    const std::uint8_t* dynamic_120) {
    if (!constructed_ || !dynamic_120)
        return 1;
    auto* engine = step_chain_.data() + 7944u + 264u;
    AuromaticV3EngineObjectParts parts{};
    parts.xinn = engine + 3344u;
    return auro_matic_v3_Engine_update_xinn_parameters(parts, dynamic_120);
}

std::int64_t AuromaticV3UpmixRuntime::update_front_manager(
    const std::uint8_t* dynamic_56) {
    if (!constructed_ || !dynamic_56)
        return 1;
    auto* engine = step_chain_.data() + 7944u + 264u;
    AuromaticV3EngineObjectParts parts{};
    parts.front_manager = engine + 2152u;
    return auro_matic_v3_Engine_update_front_manager(parts, dynamic_56);
}

std::int64_t AuromaticV3UpmixRuntime::update_peak_amplitude(float* peak_31) {
    if (!constructed_)
        return 1;
    return auro_a3deng_v3_pipeline_Upmix_update_peak_amplitude(
        step_chain_.data() + 7944u, peak_31);
}

std::int64_t AuromaticV3UpmixRuntime::process(float* const* io_channels_31) {
    if (!constructed_ || !io_channels_31)
        return 1;
    std::array<void*, ::auro_engine_v4_ida::kChannelCount> manager_channels{};
    for (std::size_t i = 0u; i < manager_channels.size(); ++i)
        manager_channels[i] = io_channels_31[i];
    // The owning public API is intentionally the same Manager -> Upmix
    // boundary used by the native pipeline.  The bound block state contains
    // one 32-sample subblock for this adapter.
    return process_manager(manager_channels.data());
}

std::int64_t AuromaticV3UpmixRuntime::process_manager(void** io_channels_31) {
    if (!constructed_ || !io_channels_31)
        return 1;
    return auro_a3deng_v3_pipeline_Manager_process_audio(
        reinterpret_cast<std::uint32_t*>(manager_.data()), io_channels_31);
}

std::uint32_t auro_matic_v3_Configurator_get_downmix_layout(
    std::uint32_t input_mask,
    std::uint32_t output_mask) {
    // Literal transcription of 0x4E8950.  Keep the masks unsigned: the
    // native function operates on 32-bit words and its complement tests are
    // part of the layout contract.
    const std::uint32_t masked_output = output_mask & ~0xF7u;
    std::uint32_t dimension = 0u;
    if (masked_output != 0u) {
        const std::uint32_t output_class = output_mask < 0x200u ? 2u : 3u;
        dimension = output_mask >= 0x10u ? output_class : 1u;
    }
    const std::uint32_t missing = output_mask & ~input_mask;
    const bool not_stereo = dimension != 1u;
    const std::uint32_t base = (missing & 0x1000u) != 0u ? 24591u : 15u;
    const bool extended_surround =
        dimension == 3u
        && (input_mask & ~0xF7u) != 0u
        && input_mask > 0x1FFu;
    const std::uint32_t height_or_surround =
        !extended_surround ? (not_stereo ? 1u : 0u)
                           : ((missing & 0x1000u) >> 12u);
    const std::uint32_t front_additions = height_or_surround != 0u ? 48u : 0u;
    const std::uint32_t rear_additions =
        ((~missing & 0x30u) == 0u) ? 3584u : 0u;
    return ((base | rear_additions | front_additions) & input_mask) | output_mask;
}

std::uint32_t /* Decompiled name: auro_a3deng_v3_get_direct_firing_speakers */
auro_a3deng_v3_get_direct_firing_speakers(
    std::uint32_t layout,
    std::uint32_t mode) {
    // Literal switch from 0x426319.  The native function deliberately keeps
    // only the front bed for mode 1 and stereo for mode 2.
    switch (mode) {
    case 0u:
    case 3u:
        return layout;
    case 1u:
        return layout & 0x10063Fu;
    case 2u:
        return layout & 0xFu;
    default:
        return 0u;
    }
}

std::int64_t /* Decompiled name: Scene::get_remix_layout */
auro_a3deng_v3_scene_get_remix_layout(
    std::uint32_t* remix_layout,
    std::uint32_t* pass_through,
    std::uint32_t* processor_layout,
    std::int32_t processor_type,
    std::uint32_t source_layout,
    std::uint32_t target_layout,
    std::uint32_t mode) {
    if (!remix_layout || !pass_through || !processor_layout)
        return 1;
    // Literal control flow from 0x4D28E0.  The first condition is the
    // native automatic pass-through decision for the full-scene processor.
    if (mode == 0u && processor_type == 3 && *processor_layout == 0u) {
        *processor_layout =
            (((~source_layout & 0x6000u) != 0u
                || (~target_layout & 0x6000u) == 0u)
             && ((~source_layout & 0x600u) != 0u
                || (~target_layout & 0x600u) == 0u)
             && ((~source_layout & 0x30u) != 0u
                || (~target_layout & 0x30u) == 0u)) ? 1u : 0u;
    }
    if (!auro_asc4he_v1_Processor_t_is_supported(mode, target_layout)) {
        *processor_layout = 1u;
        *remix_layout = auro_a3deng_v3_get_direct_firing_speakers(
            target_layout, mode);
    } else if (*processor_layout == 1u) {
        *remix_layout = auro_a3deng_v3_get_direct_firing_speakers(
            target_layout, mode);
    } else if (*processor_layout != 0u
        || auro_asc4he_v1_Processor_t_get_required_input_layout(
            mode, target_layout, remix_layout) != 0) {
        return mode < 3u ? static_cast<std::int64_t>(mode + 196u) : 203;
    }

    bool native_pass = true;
    if (mode - 1u >= 2u) {
        if (mode != 0u)
            return 203;
        native_pass = *processor_layout == 0u;
    }
    *pass_through = native_pass ? 1u : 0u;
    return 0;
}

std::int64_t auro_a3deng_v3_Headroom_construct_object(
    std::uint8_t* state,
    float initial_headroom_db) {
    if (!state)
        return 0;
    // Headroom_t_construct @ 0x4D13E0 does not wipe the callback table. Only
    // store the initial dB and clear the smoothed-process flag; the owning
    // Manager installs configure/process slots separately.
    *reinterpret_cast<float*>(state + 368u) = initial_headroom_db;
    *reinterpret_cast<std::uint32_t*>(state + 364u) = 0u;
    // Zero dB inputs yield unity linear gains after recalculate.
    std::memset(state + 64u, 0, 15u * 4u);
    (void)auro_a3deng_v3_Headroom_recalculate_gains(state);
    return reinterpret_cast<std::int64_t>(state);
}

std::int64_t auro_a3deng_v3_Headroom_recalculate_gains(
    std::uint8_t* state) {
    if (!state)
        return 1;
    // pipeline_step_Headroom_recalculate_gains @ 0x4D0A30: dB at +64, linear
    // at +124 for 15 channels. Smoothed gains are a separate process path.
    for (std::uint32_t id = 0u; id < 15u; ++id) {
        const float db = *reinterpret_cast<const float*>(state + 64u + id * 4u);
        const float gain = db > -144.0f ? std::pow(10.0f, db * 0.05f) : 0.0f;
        *reinterpret_cast<float*>(state + 124u + id * 4u) = gain;
    }
    return 0;
}

std::int64_t auro_a3deng_v3_Headroom_process_object(
    std::uint8_t* state,
    float* const* io_channels) {
    if (!state || !io_channels)
        return 1;
    // Same contract as Headroom::process_direct @ 0x4D1470.
    const std::uint32_t mask =
        *reinterpret_cast<const std::uint32_t*>(state + 40u);
    const std::uint32_t blocks =
        *reinterpret_cast<const std::uint32_t*>(state + 44u);
    const std::uint32_t sample_count = blocks * 32u;
    if (sample_count == 0u)
        return 0;
    for (std::uint32_t id = 0u; id < 15u; ++id) {
        if ((mask & (1u << id)) == 0u || io_channels[id] == nullptr)
            continue;
        const float gain =
            *reinterpret_cast<const float*>(state + 124u + id * 4u);
        auto* samples = io_channels[id];
        for (std::uint32_t i = 0u; i < sample_count; ++i)
            samples[i] *= gain;
    }
    return 0;
}

bool auromatic_layout_transform_self_test(std::string& detail) {
    constexpr std::uint32_t layout = 0x7FBFu;
    if (auro_a3deng_v3_get_direct_firing_speakers(layout, 0u) != layout
        || auro_a3deng_v3_get_direct_firing_speakers(layout, 1u) != 0x63Fu
        || auro_a3deng_v3_get_direct_firing_speakers(layout, 2u) != 0xFu
        || auro_a3deng_v3_get_direct_firing_speakers(layout, 3u) != layout
        || auro_a3deng_v3_get_direct_firing_speakers(layout, 4u) != 0u) {
        detail = "direct-firing speaker mask mismatch";
        return false;
    }
    struct Case {
        std::uint32_t input;
        std::uint32_t output;
        std::uint32_t expected;
    };
    constexpr Case cases[] = {
        {0x01BFu, 0x7FBFu, 0x7FBFu},
        // Native does not preserve every input bit for this downmix class.
        {0x01FFu, 0x0003u, 0x003Fu},
        {0x003Fu, 0x7FBFu, 0x7FBFu},
        {0x7FBFu, 0x7FBFu, 0x7FBFu},
        {0x0007u, 0x003Fu, 0x003Fu},
    };
    for (const auto& test : cases) {
        const std::uint32_t actual =
            auro_matic_v3_Configurator_get_downmix_layout(
                test.input, test.output);
        if (actual != test.expected) {
            detail = "input=0x" + std::to_string(test.input)
                + " output=0x" + std::to_string(test.output)
                + " expected=0x" + std::to_string(test.expected)
                + " actual=0x" + std::to_string(actual);
            return false;
        }
    }
    std::array<std::uint8_t, 64u> remap{};
    if (!auro_a3deng_v3_RemapOutput_set_output_layout(
            remap.data(), 1u, 0x30600u)
        || *reinterpret_cast<const std::uint32_t*>(remap.data()) != 0x630u
        || *reinterpret_cast<const std::uint32_t*>(remap.data() + 4u)
            != 0x30600u) {
        detail = "RemapOutput mode-1 table mismatch";
        return false;
    }
    remap.fill(0u);
    if (!auro_a3deng_v3_RemapOutput_set_output_layout(
            remap.data(), 2u, 0x3070Fu)
        || *reinterpret_cast<const std::uint32_t*>(remap.data()) != 0xC000Fu) {
        detail = "RemapOutput mode-2 table mismatch";
        return false;
    }
    detail = "native mask cases PASS";
    return true;
}

std::int64_t auro_matic_v3_Engine_process_portable(
    const AuromaticV3EngineStages& stages,
    void** io_channels_31) {
    if (!io_channels_31 || !stages.input || !stages.front_manager_input
        || !stages.xinn || !stages.p3d3d || !stages.front_manager_output
        || !stages.gain || !stages.silence)
        return 1;
    if (manager_process_upmix_input(stages.input, io_channels_31) != 0)
        return 1;
    if (manager_process_upmix_front_manager_input(
            stages.front_manager_input, io_channels_31) != 0)
        return 1;

    const std::uint64_t block_state =
        *reinterpret_cast<const std::uint64_t*>(stages.xinn + 8u);
    if (!block_state)
        return 1;
    const std::uint32_t subblocks = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(block_state) + 4u);
    const std::uint64_t owner =
        *reinterpret_cast<const std::uint64_t*>(stages.xinn + 483952u);
    const std::uint8_t* records = nullptr;
    if (owner != 0u)
        records = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(
            *reinterpret_cast<const std::uint64_t*>(
                static_cast<std::uintptr_t>(owner) + 32u)));
    if (auro_a3deng_v4_pipeline_step_upmix_XinN_process_impl(
            reinterpret_cast<std::uint64_t>(stages.xinn),
            io_channels_31,
            subblocks,
            records,
            nullptr,
            auro_a3deng_v4_pipeline_step_upmix_XinN_reset_audio_state_impl) != 0)
        return 1;
    if (manager_process_upmix_3d3d(stages.p3d3d, io_channels_31) != 0)
        return 1;
    if (manager_process_upmix_front_manager_output(
            stages.front_manager_output, io_channels_31) != 0)
        return 1;
    if (manager_process_upmix_gain(stages.gain, io_channels_31) != 0)
        return 1;
    if (manager_process_upmix_silence(stages.silence, io_channels_31) != 0)
        return 1;

    // Engine::process resets all stateful stages after the Input record marks
    // a silent transition.  The reset is deliberately performed after the
    // complete stage sequence, matching native 0x5500F0.
    bool transition_silent = false;
    const std::uint64_t input_owner =
        *reinterpret_cast<const std::uint64_t*>(stages.input + 2192u);
    if (input_owner != 0u) {
        const auto* records = reinterpret_cast<const std::uint8_t*>(
            static_cast<std::uintptr_t>(*reinterpret_cast<const std::uint64_t*>(
                static_cast<std::uintptr_t>(input_owner) + 32u)));
        const std::uint64_t input_block_state =
            *reinterpret_cast<const std::uint64_t*>(stages.input + 8u);
        const std::uint32_t subblocks = input_block_state == 0u
            ? 0u
            : *reinterpret_cast<const std::uint32_t*>(
                static_cast<std::uintptr_t>(input_block_state) + 4u);
        if (records != nullptr) {
            for (std::uint32_t block = 0u; block < subblocks; ++block) {
                if (records[static_cast<std::size_t>(block) * 516u + 1u] != 0u) {
                    transition_silent = true;
                    break;
                }
            }
        }
    }
    if (transition_silent) {
        (void)auro_matic_v3_OutputGate_reset_audio_state(stages.input + 36u);
        (void)auro_centergen_v3_Processor_reset_audio_state(
            stages.front_manager_input + 40u);
        (void)auro_a3deng_v4_pipeline_step_upmix_XinN_reset_audio_state_impl(
            reinterpret_cast<std::uint64_t>(stages.xinn));
        manager_reset_upmix_3d3d(stages.p3d3d);
    }
    return 0;
}

std::int64_t auro_matic_v3_Engine_configure_portable(
    const AuromaticV3EngineStages& stages,
    std::uint32_t sample_rate,
    const std::uint32_t* engine_params) {
    if (!stages.input || !stages.front_manager_input || !stages.xinn
        || !stages.p3d3d || !stages.front_manager_output || !stages.gain
        || !stages.silence || !engine_params)
        return 1;

    // This is the exact native Engine_configure order.  The v4 step wrapper
    // stores OutputGate in the Input step at +36, while FrontManager keeps
    // Input and Output as separate embedded states in this portable view.
    if (auro_matic_v3_OutputGate_configure(
            stages.input + 36u, sample_rate, engine_params[0u]) != 0)
        return 1;
    if (auro_matic_v3_FrontManager_Input_configure(
            stages.front_manager_input + 40u,
            sample_rate,
            reinterpret_cast<const std::uint8_t*>(engine_params + 2u)) != 0)
        return 1;
    if (auro_matic_v3_FrontManager_Output_configure(
            stages.front_manager_output + 36u,
            sample_rate,
            reinterpret_cast<const std::uint8_t*>(engine_params + 2u)) != 0)
        return 1;
    if (auro_matic_v3_XinN_fl32_configure(
            stages.xinn + 40u,
            sample_rate,
            reinterpret_cast<const std::uint64_t*>(engine_params + 7u)) != 0)
        return 1;
    if (manager_configure_upmix_3d3d(
            stages.p3d3d,
            sample_rate,
            engine_params + 9u) != 0)
        return 1;

    if (auro_matic_v3_Gain_prepare_step(stages.gain, engine_params[47u]) != 0)
        return 1;
    *reinterpret_cast<std::uint32_t*>(stages.silence + 36u) = engine_params[46u];
    *reinterpret_cast<std::uint32_t*>(stages.silence + 40u) = engine_params[0u];

    (void)auro_matic_v3_OutputGate_reset_audio_state(stages.input + 36u);
    (void)auro_centergen_v3_Processor_reset_audio_state(
        stages.front_manager_input + 40u);
    (void)auro_a3deng_v4_pipeline_step_upmix_XinN_reset_audio_state_impl(
        reinterpret_cast<std::uint64_t>(stages.xinn));
    manager_reset_upmix_3d3d(stages.p3d3d);
    return 0;
}

std::int64_t auro_a3deng_v3_pipeline_plan_MetaInfo_t_construct(
    std::uint8_t* meta_info,
    const std::uint8_t* config) {
    if (!meta_info || !config)
        return 1;
    // Literal transcription of 0x4D6870.  The two pointer fields are
    // intentionally retained as addresses: downstream Plan_Creator helpers
    // dereference the source records rather than receiving copied values.
    std::memset(meta_info, 0, 88u);
    *reinterpret_cast<std::uint64_t*>(meta_info + 0u) =
        reinterpret_cast<std::uint64_t>(config);
    *reinterpret_cast<std::uint64_t*>(meta_info + 8u) =
        reinterpret_cast<std::uint64_t>(config + 20u);
    *reinterpret_cast<std::uint32_t*>(meta_info + 40u) =
        *reinterpret_cast<const std::uint32_t*>(config + 60u);
    std::memcpy(meta_info + 16u, config + 40u, 16u);
    *reinterpret_cast<std::uint64_t*>(meta_info + 32u) = 0x100000000ull;
    std::memcpy(meta_info + 44u, config + 84u, 16u);
    std::memcpy(meta_info + 60u, config + 100u, 16u);
    *reinterpret_cast<std::uint64_t*>(meta_info + 76u) =
        *reinterpret_cast<const std::uint64_t*>(config + 116u);
    auto first_rate = *reinterpret_cast<const std::uint32_t*>(config + 8u);
    const auto second_rate = *reinterpret_cast<const std::uint32_t*>(config + 28u);
    if ((first_rate >> 9u) >= 0x177u)
        first_rate = 192000u;
    *reinterpret_cast<std::uint32_t*>(meta_info + 84u) =
        std::min(first_rate, second_rate);
    return 0;
}

std::int64_t auro_a3deng_v3_pipeline_Plan_configure_sample_convertors(
    std::uint8_t* plan,
    const std::uint8_t* meta_info,
    std::uint32_t output_bytes_unit,
    AuromaticV3PlanAbi abi) {
    if (!plan || !meta_info)
        return 1;
    const auto input_record =
        *reinterpret_cast<const std::uint64_t*>(meta_info + 0u);
    const auto output_record =
        *reinterpret_cast<const std::uint64_t*>(meta_info + 8u);
    if (input_record == 0u || output_record == 0u)
        return 1;
    const auto input_converter = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(input_record) + 12u);
    const auto output_converter = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(output_record) + 12u);
    *reinterpret_cast<std::uint32_t*>(plan + 0u) = input_converter != 0u ? 1u : 0u;
    *reinterpret_cast<std::uint32_t*>(plan + 4u) = input_converter;
    *reinterpret_cast<std::uint64_t*>(plan + 8u) = 0u;
    if (abi == AuromaticV3PlanAbi::Libauro1) {
        *reinterpret_cast<std::uint32_t*>(plan + 2556u) =
            (output_converter | output_bytes_unit) != 0u ? 1u : 0u;
        *reinterpret_cast<std::uint32_t*>(plan + 2560u) = 0u;
        *reinterpret_cast<std::uint32_t*>(plan + 2564u) = output_converter;
        *reinterpret_cast<std::uint32_t*>(plan + 2568u) = output_bytes_unit;
    } else {
        *reinterpret_cast<std::uint32_t*>(plan + 2420u) =
            (output_converter | output_bytes_unit) != 0u ? 1u : 0u;
        *reinterpret_cast<std::uint32_t*>(plan + 2424u) = 0u;
        *reinterpret_cast<std::uint32_t*>(plan + 2428u) = output_converter;
        *reinterpret_cast<std::uint32_t*>(plan + 2432u) = output_bytes_unit;
    }
    return 0;
}

std::int64_t auro_a3deng_v3_pipeline_Plan_configure_resamplers(
    std::uint8_t* plan,
    const std::uint8_t* meta_info,
    AuromaticV3PlanAbi abi) {
    if (!plan || !meta_info)
        return 1;
    const auto input_record =
        *reinterpret_cast<const std::uint64_t*>(meta_info + 0u);
    const auto output_record =
        *reinterpret_cast<const std::uint64_t*>(meta_info + 8u);
    const auto plan_rate =
        *reinterpret_cast<const std::uint32_t*>(meta_info + 84u);
    if (input_record == 0u || output_record == 0u || plan_rate == 0u)
        return 1;
    const auto input_rate = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(input_record) + 8u);
    const auto output_rate = *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(output_record) + 8u);
    const auto resampler_mode_offset = abi == AuromaticV3PlanAbi::Libauro1
        ? 2540u : 2404u;
    const auto marker_offset = abi == AuromaticV3PlanAbi::Libauro1
        ? 1584u : 1448u;
    const auto input_ratio = input_rate / plan_rate;
    std::uint8_t downsample = 0u;
    if (input_ratio == 1u) {
        *reinterpret_cast<std::uint64_t*>(plan + 16u) = 0u;
        if (plan_rate != input_rate)
            return 204;
    } else if (input_ratio == 2u) {
        downsample = 1u;
        *reinterpret_cast<std::uint64_t*>(plan + 16u) = 1u;
        if (2u * plan_rate != input_rate)
            return 204;
    } else if (input_ratio == 4u) {
        downsample = 1u;
        *reinterpret_cast<std::uint64_t*>(plan + 16u) = 0x100000001ull;
        if (4u * plan_rate != input_rate)
            return 204;
    } else {
        return 204;
    }
    const auto output_ratio = output_rate / plan_rate;
    if (output_ratio == 1u) {
        *reinterpret_cast<std::uint32_t*>(plan + resampler_mode_offset) = 0u;
        if (plan_rate != output_rate)
            return 204;
    } else if (output_ratio == 2u) {
        *reinterpret_cast<std::uint32_t*>(plan + resampler_mode_offset) = 1u;
        if (2u * plan_rate != output_rate)
            return 204;
    } else {
        return 204;
    }
    if ((input_rate == 88200u || input_rate == 96000u)
        && input_rate == output_rate && downsample == 0u)
        *reinterpret_cast<std::uint64_t*>(plan + marker_offset) = 0x1A00000001ull;
    return 0;
}

std::int64_t auro_remix_v1_Configurator_t_construct(std::uint8_t* state) {
    if (!state)
        return 0;
    // auro_remix_v1_Configurator_t_construct @ 0x4DB8C0 first constructs the
    // embedded auro_matic_v3_Configurator_t at a1+40.  Its constructor is
    // deliberately reproduced here before the wrapper-owned fields are set;
    // leaving this subobject zeroed changes the callback ABI used by the
    // native make_plan path.
    std::memset(state, 0, 248u);
    auto* matic = state + 40u;
    std::memset(matic, 0, 160u);
    *reinterpret_cast<std::uint64_t*>(matic + 0u) =
        reinterpret_cast<std::uint64_t>(matic);
    *reinterpret_cast<std::uint64_t*>(matic + 8u) = 0x54F890u;
    *reinterpret_cast<std::uint64_t*>(matic + 16u) = 0x54F8E0u;
    *reinterpret_cast<std::uint64_t*>(matic + 24u) = 0x54F830u;
    *reinterpret_cast<std::uint64_t*>(state + 0u) =
        reinterpret_cast<std::uint64_t>(state);
    *reinterpret_cast<std::uint64_t*>(state + 8u) =
        reinterpret_cast<std::uint64_t>(state + 40u);
    *reinterpret_cast<std::uint64_t*>(state + 24u) = 0x4DB940u;
    *reinterpret_cast<std::uint64_t*>(state + 32u) = 0x4DB970u;
    *reinterpret_cast<std::uint64_t*>(state + 204u) = 0x3F8000003F800000ull;
    *reinterpret_cast<std::uint64_t*>(state + 212u) = 1u;
    return reinterpret_cast<std::int64_t>(state);
}

void auro_remix_v1_Configurator_set_listening_mode(
    std::uint8_t* state,
    std::uint32_t mode) {
    if (state)
        *reinterpret_cast<std::uint32_t*>(state + 16u) = mode;
}

std::int64_t auro_remix_v1_Configurator_set_device(
    std::uint8_t* state,
    std::uint32_t device) {
    if (!state)
        return 0;
    *reinterpret_cast<std::uint32_t*>(state + 20u) = device;
    return 1;
}

void auro_remix_v1_Configurator_configure_alt_3D(
    std::uint8_t* state,
    const std::uint8_t* config) {
    if (!state || !config)
        return;
    *reinterpret_cast<std::uint64_t*>(state + 200u) =
        *reinterpret_cast<const std::uint64_t*>(config + 0u);
    *reinterpret_cast<std::uint32_t*>(state + 208u) =
        *reinterpret_cast<const std::uint32_t*>(config + 8u);
}

void auro_remix_v1_Configurator_configure_bass_management(
    std::uint8_t* state,
    const std::uint8_t* config) {
    if (!state || !config)
        return;
    std::memcpy(state + 212u, config + 0u, 16u);
    *reinterpret_cast<std::uint64_t*>(state + 228u) =
        *reinterpret_cast<const std::uint64_t*>(config + 16u);
}

std::int64_t auro_matic_v3_Plan_calculate_downmix(
    std::int32_t* plan,
    std::uint32_t source_layout,
    std::uint32_t target_layout,
    std::uint32_t* output_layout) {
    if (!plan || !output_layout || source_layout == 0u || target_layout == 0u)
        return 1;
    alignas(16) std::array<std::uint8_t, 56u> engine{};
    auro_matic_v3_downmix_engine_t_construct(engine.data());
    if (!auro_matic_v3_downmix_engine_calculate(
            reinterpret_cast<std::int32_t*>(engine.data()),
            plan, source_layout, target_layout))
        return 1;
    *output_layout = static_cast<std::uint32_t>(plan[159]);
    return *output_layout != 0u ? 0 : 1;
}

std::int64_t auro_remix_v1_Configurator_compute_downmix_plan(
    std::uint8_t* result_plan,
    std::uint32_t source_layout,
    std::uint32_t target_layout,
    std::uint32_t* output_changed) {
    // auro_remix_v1_Configurator_t_compute_downmix @ 0x4DB450.
    if (!result_plan || !output_changed || source_layout == 0u || target_layout == 0u)
        return 1;
    std::memset(result_plan, 0, 648u);
    alignas(16) std::array<std::uint8_t, 56u> engine{};
    alignas(16) std::array<std::int32_t, 0x280u / sizeof(std::int32_t)> indexed{};
    auro_matic_v3_downmix_engine_t_construct(engine.data());
    if (!auro_matic_v3_downmix_engine_calculate(
            reinterpret_cast<std::int32_t*>(engine.data()), indexed.data(),
            source_layout, target_layout))
        return 1;
    const std::uint32_t calculated_layout = static_cast<std::uint32_t>(indexed[159]);
    if (!auro_matic_v3_downmix_plan_construct(
            indexed.data(), reinterpret_cast<std::int32_t*>(result_plan + 8u),
            auro_downmix_v1_gain_table_default()))
        return 1;
    *reinterpret_cast<std::uint32_t*>(result_plan + 0u) = source_layout;
    *reinterpret_cast<std::uint32_t*>(result_plan + 4u) = calculated_layout;
    *output_changed = calculated_layout != source_layout ? 1u : 0u;
    return 0;
}

std::int64_t auro_remix_v1_Configurator_plan_alt_3D_store(
    std::uint8_t* downmix_engine,
    std::uint8_t* result_plan,
    std::uint32_t source_layout,
    std::uint32_t mode_flags,
    const std::uint8_t* config,
    std::uint32_t* output_layout,
    std::uint32_t* active) {
    // auro_remix_v1_Configurator_t_plan_alt_3D_store @ 0x4DB4F0.
    if (!downmix_engine || !result_plan || !config || !output_layout || !active)
        return 1;
    std::memset(result_plan, 0, 664u);
    *reinterpret_cast<std::uint32_t*>(result_plan + 0u) = source_layout;
    *active = 0u;
    const std::uint32_t height_flags = mode_flags & 0x3FE00u;
    if (height_flags == 0u || *reinterpret_cast<const std::uint32_t*>(config) == 0u) {
        *output_layout = source_layout;
        *reinterpret_cast<std::uint32_t*>(result_plan + 4u) = source_layout;
        return 0;
    }
    const std::uint32_t target_layout =
        (source_layout & 0x1C01FFu) | height_flags;
    std::array<std::int32_t, 0x280u / sizeof(std::int32_t)> indexed{};
    auro_matic_v3_downmix_engine_t_construct(downmix_engine);
    if (!auro_matic_v3_downmix_engine_calculate(
            reinterpret_cast<std::int32_t*>(downmix_engine), indexed.data(),
            source_layout, target_layout)
        || !auro_matic_v3_downmix_plan_construct(
            indexed.data(), reinterpret_cast<std::int32_t*>(result_plan + 24u),
            auro_downmix_v1_gain_table_default()))
        return 1;
    const std::uint32_t calculated_layout = static_cast<std::uint32_t>(indexed[159]);
    *reinterpret_cast<std::uint32_t*>(result_plan + 8u) = calculated_layout & 0x3FE00u;
    *reinterpret_cast<std::uint32_t*>(result_plan + 16u) = source_layout;
    *reinterpret_cast<std::uint32_t*>(result_plan + 20u) = calculated_layout;
    *active = ((calculated_layout ^ source_layout)
        | (calculated_layout & 0x3FE00u)) != 0u ? 1u : 0u;
    const std::uint32_t final_layout = calculated_layout & 0xFFFC01FFu;
    *output_layout = final_layout;
    *reinterpret_cast<std::uint32_t*>(result_plan + 4u) = final_layout;
    return 0;
}

bool auro_remix_v1_project_output_layout(
    std::uint32_t* projected_layout,
    std::uint32_t input_layout,
    std::uint32_t legacy_layout,
    std::uint32_t device,
    std::uint32_t mode) {
    // auro_remix_v1_project_output_layout @ 0x4DDA40.
    if (!projected_layout || device >= 32u)
        return false;
    std::uint32_t allowed = 0u;
    switch (mode) {
    case 0u:
        allowed = 0x7FFFFFFFu;
        if (device > 6u)
            return false;
        break;
    case 1u:
        allowed = 0x1FFu;
        if (device > 6u)
            return false;
        break;
    case 2u:
        allowed = 0xBu;
        if (device > 6u)
            return false;
        break;
    case 3u:
        if (legacy_layout == 4u) {
            const std::uint64_t native_mask_expr =
                (static_cast<std::uint64_t>(input_layout & 4u)
                    | 0x400000000ull) - 1ull;
            allowed = static_cast<std::uint32_t>(native_mask_expr >> 32u);
        } else {
            allowed = legacy_layout | 3u;
            if (device > 6u)
                return false;
        }
        break;
    default:
        return false;
    }
    static constexpr std::uint32_t kDeviceGroupA = 0x29u;
    static constexpr std::uint32_t kDeviceGroupB = 0x46u;
    const std::uint32_t bit = 1u << device;
    if ((kDeviceGroupA & bit) == 0u) {
        if ((kDeviceGroupB & bit) == 0u)
            return false;
        allowed |= 8u;
    }
    *projected_layout = input_layout & allowed;
    return true;
}

std::int64_t auro_remix_v1_plan_Legacy_t_construct(
    std::uint32_t* plan,
    std::uint32_t layout) {
    if (!plan)
        return 1;
    plan[0u] = layout;
    std::uint32_t remapped = layout;
    if ((layout & 0x40u) != 0u) {
        plan[2u] = ((~layout & 0x30u) != 0u) ? 1u : 0u;
        if (plan[2u] != 0u)
            remapped = (layout & 0xFFFFFF8Fu) | 0x30u;
    } else {
        plan[2u] = 0u;
    }
    plan[1u] = remapped;
    return 0;
}

std::uint32_t auro_remix_v1_plan_LegacyFlags_remap_layout(
    const std::uint32_t* flags,
    std::uint32_t layout) {
    if (!flags || *flags == 0u)
        return layout;
    return (layout & 0xFFFFFF8Fu) | 0x30u;
}

bool auro_remix_v1_plan_Legacy_active(const std::uint32_t* plan) {
    return plan != nullptr && plan[1u] != plan[0u];
}

std::int64_t auro_remix_v1_Configurator_plan_bass_management(
    std::uint8_t* state,
    std::uint32_t* output_layout,
    std::uint32_t* output_flag,
    std::uint32_t input_layout,
    std::uint32_t mode_flags,
    const std::uint8_t* config,
    std::uint32_t device) {
    if (!state || !output_layout || !output_flag || !config)
        return 1;
    *reinterpret_cast<std::uint32_t*>(state + 36u) = 0u;
    *reinterpret_cast<std::uint32_t*>(state + 0u) = input_layout;
    *reinterpret_cast<std::uint64_t*>(state + 8u) = 0u;
    *reinterpret_cast<std::uint64_t*>(state + 20u) =
        *reinterpret_cast<const std::uint64_t*>(config + 8u);
    *reinterpret_cast<std::uint64_t*>(state + 28u) =
        *reinterpret_cast<const std::uint64_t*>(config + 16u);
    std::uint32_t additions = 0u;
    if ((mode_flags & 8u) != 0u) {
        if (device > 6u)
            return 1;
        if ((0x29u & (1u << device)) != 0u) {
            if (*reinterpret_cast<const std::uint32_t*>(config) == 0u)
                return 227;
        } else if ((0x46u & (1u << device)) != 0u) {
            if (*reinterpret_cast<const std::uint32_t*>(config) != 0u) {
                *output_flag = 1u;
                *reinterpret_cast<std::uint32_t*>(state + 16u) = 1u;
                if (*reinterpret_cast<const std::uint32_t*>(config + 4u) != 0u)
                    *reinterpret_cast<std::uint32_t*>(state + 8u) = 1092616192u;
            }
        } else {
            return 1;
        }
        const bool special = (mode_flags & 0x100000u) != 0u
            && (input_layout & 0x100000u) == 0u;
        additions = special ? ((~input_layout & 8u) | 0x100000u)
                            : (~input_layout & 8u);
        if ((!special && (input_layout & 8u) == 0u) || special)
            *output_flag = 1u;
        if (device == 2u && input_layout < 0x200u)
            *reinterpret_cast<std::uint32_t*>(state + 12u) =
                input_layout < 0x10u ? 1065353216u : 1073741824u;
    }
    *output_layout = input_layout | additions;
    *reinterpret_cast<std::uint32_t*>(state + 36u) = *output_layout;
    *reinterpret_cast<std::uint32_t*>(state + 4u) = *output_layout;
    return 0;
}

std::int64_t auro_remix_v1_Configurator_plan_alt_3D_mix(
    std::uint32_t required_layout,
    std::uint8_t* plan,
    std::uint32_t* output_layout,
    std::uint32_t* active,
    std::uint32_t source_layout,
    std::uint32_t enabled_layout,
    const std::uint8_t* alt_3d_config) {
    if (!plan || !output_layout || !active || !alt_3d_config)
        return 1;
    if ((source_layout & required_layout & enabled_layout) != required_layout)
        return 1;
    *reinterpret_cast<std::uint32_t*>(plan + 0u) = required_layout;
    *reinterpret_cast<std::uint64_t*>(plan + 4u) =
        *reinterpret_cast<const std::uint64_t*>(alt_3d_config + 4u);
    *active = required_layout != 0u ? 1u : 0u;
    *output_layout = source_layout;
    return 0;
}

std::int64_t build_engine_params_for_upmix_plan(
    std::uint32_t* engine_params,
    std::uint32_t source_layout,
    std::uint32_t target_layout,
    std::uint32_t sample_rate,
    bool surround_mode,
    std::uint32_t room_preset,
    std::uint8_t* processed_preset_out,
    std::uint8_t* dynamic_parameters_out,
    std::uint32_t preset_family);

#include "plan_creator.inc"

std::int64_t auro_a3deng_v3_pipeline_Manager_configure(
    std::uint32_t* manager_base,
    void* cfg_blob,
    std::int32_t initial) {
    if (!manager_base || !cfg_blob)
        return 1;
    // Native 0x4CBEE0: ранний guard перед копированием параметров.
    const auto* cfg_u32 = reinterpret_cast<const std::uint32_t*>(cfg_blob);
    if (cfg_u32[1] * cfg_u32[7] != cfg_u32[6] * cfg_u32[2])
        return 202;
    auto* manager = reinterpret_cast<std::uint8_t*>(manager_base);
    const std::uint32_t update_type = /* Decompiled name: manager_parameter_update_type */
manager_parameter_update_type(
        reinterpret_cast<const std::uint32_t*>(manager + kManagerOffParams478078),
        cfg_u32);
    std::memcpy(manager + kManagerOffParams478078, cfg_blob, kManagerParamsBytes478078);

    auto* step_chain = reinterpret_cast<std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<std::uint64_t*>(manager + kManagerOffStepChain478078)));
    if ((update_type == 1u || update_type == 2u) && !step_chain)
        return 1;
    if (step_chain)
        manager_configure_upmix_gain_silence(
            step_chain,
            reinterpret_cast<const std::uint32_t*>(manager + kManagerOffParams478078));
    if (update_type == 1u || update_type == 2u)
        manager_invalidate_upmix_3d3d_config(step_chain);
    if (update_type == 3u) {
        *reinterpret_cast<std::uint32_t*>(manager + kManagerOffBypass478078) = 1u;
        /* Decompiled name: manager_reset_step_chain */
manager_reset_step_chain(step_chain);
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

    // Native sub_4CDxxx refreshes the cached channel gains after an initial
    // configure, but only when the recalculation-dirty field is set.
    if (initial != 0
        && *reinterpret_cast<const std::uint32_t*>(
            manager + kManagerOffHeadroomRecalcDirty478078) != 0u) {
        const std::int64_t rc = manager_refresh_info_native(manager);
        if (rc != 0)
            return rc;
    }
    return 0;
}

std::int64_t auro_a3deng_v3_pipeline_Manager_process_audio(std::uint32_t* manager_base, void** io_channels) {
    if (!manager_base || !io_channels)
        return 1;
    auto* manager = reinterpret_cast<std::uint8_t*>(manager_base);
    /* Decompiled name: manager_update_headroom_input_gains */
manager_update_headroom_input_gains(manager);

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
        return 1;
    return /* Decompiled name: manager_process_step_chain */
manager_process_step_chain(manager, step_chain, io_channels);
}

std::int64_t auro_a3deng_v3_pipeline_Manager_calculate_info(
    std::uint32_t* manager_base,
    std::uint8_t* info_base) {
    if (!manager_base || !info_base)
        return 1;
    auto* manager = reinterpret_cast<std::uint8_t*>(manager_base);
    auto* step_chain = reinterpret_cast<std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<std::uint64_t*>(
            manager + kManagerOffStepChain478078)));
    if (!step_chain)
        return 1;
    return manager_calculate_info_step_chain(step_chain, info_base);
}

bool auro_a3deng_v3_RemapOutput_set_output_layout(
    std::uint8_t* remap,
    std::uint32_t input_layout,
    std::uint32_t output_layout) {
    if (!remap)
        return false;
    *reinterpret_cast<std::uint32_t*>(remap + 4u) = output_layout;
    *reinterpret_cast<std::uint32_t*>(remap + 0u) = 0u;
    *reinterpret_cast<std::uint32_t*>(remap + 24u) = 0u;
    *reinterpret_cast<std::uint32_t*>(remap + 28u) = 0u;
    *reinterpret_cast<std::uint32_t*>(remap + 32u) = 0u;
    *reinterpret_cast<std::uint32_t*>(remap + 40u) = 0u;

        const std::uint32_t* external_to_internal = nullptr;
    std::size_t mapping_count = 0u;
    if (input_layout == 0u || input_layout == 3u) {
        // sub_4CE0F0: identity mapping, accepted for channel ids < 15.
        for (std::uint32_t id = 0u; id < 31u; ++id) {
            if ((output_layout & (1u << id)) != 0u && id < 15u)
                *reinterpret_cast<std::uint32_t*>(remap) |= 1u << id;
        }
        return true;
    }
    if (input_layout == 1u) {
        if ((output_layout & 0x600030u) != 0u
            && (output_layout & 0x30600u) != 0u)
            return false;
        static constexpr std::uint32_t kMapA[] = {
            0u, 1u, 2u, 3u, 16u, 17u, 0u, 7u,
            8u, 9u, 10u, 0u, 0u, 18u, 19u,
        };
        static constexpr std::uint32_t kMapB[] = {
            0u, 1u, 2u, 3u, 0u, 0u, 0u, 7u, 8u, 9u,
            10u, 0u, 0u, 0u, 0u, 0u, 4u, 5u, 13u, 14u,
        };
        static constexpr std::uint32_t kMapC[] = {
            0u, 1u, 2u, 3u, 4u, 5u, 0u, 7u,
            8u, 18u, 19u, 0u, 0u, 21u, 22u,
        };
        if ((output_layout & 0x30600u) != 0u) {
            // The native loop uses the +16 callback (kMapB) to calculate the
            // internal mask. kMapA remains the external-id callback and is
            // intentionally not used for this direction.
            external_to_internal = kMapB;
            mapping_count = 20u;
            *reinterpret_cast<std::uint32_t*>(remap + 40u) = 17u;
        } else {
            external_to_internal = kMapC;
            mapping_count = 15u;
            (void)kMapA;
        }
    } else if (input_layout == 2u) {
        static constexpr std::uint32_t kMap[] = {
            0u, 1u, 2u, 3u, 0u, 0u, 0u, 0u,
            0u, 18u, 19u, 0u, 12u,
        };
        if ((output_layout & 0xC0000u) == 0xC0000u) {
            *reinterpret_cast<std::uint64_t*>(remap + 24u) =
                0x1200000002ull;
            *reinterpret_cast<std::uint32_t*>(remap + 32u) = 19u;
        }
        external_to_internal = kMap;
        mapping_count = 13u;
    } else {
        return false;
    }
    for (std::uint32_t id = 0u; id < mapping_count; ++id) {
        if ((output_layout & (1u << id)) == 0u)
            continue;
        std::uint32_t mapped = external_to_internal[id];
        if (mapped < 31u)
            *reinterpret_cast<std::uint32_t*>(remap) |= 1u << mapped;
    }
    return true;
}

std::int64_t auro_matic_XinN_fl32_process(std::uint64_t xinn_state, void** channel_span_31) {
    if (xinn_state == 0u)
        return 0;
    using CallbackFn = std::int64_t (*)(std::uint64_t, void**);
    const std::uint64_t fn_u64 = *reinterpret_cast<const std::uint64_t*>(xinn_state + 24u);
    if (fn_u64 == ::auro_engine_v4_ida::kAuroMaticXinN_fl32_process_inplace)
        return auro_matic_XinN_fl32_process_inplace(xinn_state, channel_span_31);
    if (fn_u64 == ::auro_engine_v4_ida::kAuroMaticXinN_fl32_process_scratch)
        return auro_matic_XinN_fl32_process_scratch(xinn_state, channel_span_31);
    const auto fn = reinterpret_cast<CallbackFn>(static_cast<std::uintptr_t>(fn_u64));
    if (fn)
        return fn(xinn_state, channel_span_31);
    return 0;
}

std::int64_t auro_matic_v3_XinN_fl32_process(std::uint64_t xinn_v3_state, void** channel_span_31) {
    if (xinn_v3_state == 0u)
        return 0;
    if (*reinterpret_cast<const std::uint32_t*>(xinn_v3_state + 840u) != 0u)
        return auro_matic_XinN_fl32_process(xinn_v3_state, channel_span_31);
    return 0;
}

void auro_audio_Smooth_fl32_inst_initialize(float* smooth_state, std::uint32_t sample_rate, float seconds) {
    if (!smooth_state)
        return;
    float coeff = 0.0f;
    const float samples = static_cast<float>(sample_rate) * seconds;
    if (samples > 0.0f)
        coeff = std::pow(10.0f, -3.0f / samples);
    smooth_state[2] = coeff;
}

void auro_audio_Smooth_fl32_inst_update(float* smooth_state, float target) {
    if (smooth_state)
        smooth_state[1] = target;
}

void auro_audio_Smooth_fl32_inst_set_current(float* smooth_state, float current) {
    if (smooth_state)
        smooth_state[0] = current;
}

void auro_audio_Smooth_fl32_inst_gains_smooth(
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

void auro_matic_Engine1_fl32_set_total_clear_frames(std::uint8_t* engine1_state, std::uint32_t frames) {
    if (engine1_state)
        *reinterpret_cast<std::uint32_t*>(engine1_state + 64u) = frames;
}

std::int64_t auro_matic_Engine1_fl32_reset_audio_state(std::uint8_t* engine1_state) {
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

void auro_matic_Engine1_fl32_get_delayed_frame(
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

bool auro_matic_Engine1_fl32_partial_clear(std::uint8_t* engine1_state, std::uint32_t* remaining) {
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

void /* Decompiled name: engine1_copy32_ring */
engine1_copy32_ring(float* ring, std::uint32_t offset, const float* src_32) {
    if (!ring || !src_32)
        return;
    offset %= 7616u;
    float* dst = ring + offset;
    std::memcpy(dst, src_32, 32u * sizeof(float));
    if (offset < 32u)
        std::memcpy(dst + 7616u, src_32, 32u * sizeof(float));
}

void /* Decompiled name: engine1_sum_98_taps_to_vec */
engine1_sum_98_taps_to_vec(
    const float* ring,
    std::int32_t ring_offset,
    const std::uint8_t* entries,
    float* dst_4) {
    if (!dst_4)
        return;
    if (!ring || !entries) {
        dst_4[0] = 0.0f;
        dst_4[1] = 0.0f;
        dst_4[2] = 0.0f;
        dst_4[3] = 0.0f;
        return;
    }
    __m128 accumulator = _mm_setzero_ps();
    auto ring_position = [ring_offset](std::uint32_t tap_index) {
        constexpr std::int64_t kRingSamples = 7616;
        std::int64_t position = static_cast<std::int64_t>(ring_offset)
            + static_cast<std::int64_t>(tap_index);
        position %= kRingSamples;
        if (position < 0)
            position += kRingSamples;
        return static_cast<std::size_t>(position);
    };
    for (std::size_t tap = 0; tap < 98u; tap += 2u) {
        const auto* first = entries + tap * 8u;
        const auto* second = first + 8u;
        const std::uint32_t first_index =
            *reinterpret_cast<const std::uint32_t*>(first);
        const std::uint32_t second_index =
            *reinterpret_cast<const std::uint32_t*>(second);
        const float first_gain = *reinterpret_cast<const float*>(first + 4u);
        const float second_gain = *reinterpret_cast<const float*>(second + 4u);
        // The backing store contains a 7616-sample circular delay line plus a
        // 32-sample mirror used by SIMD reads that cross its end.  Preset tap
        // indices are logical circular positions; applying them as raw offsets
        // can address beyond Engine1's 0x7800-byte ring allocation.
        const __m128 first_value = _mm_loadu_ps(ring + ring_position(first_index));
        const __m128 second_value = _mm_loadu_ps(ring + ring_position(second_index));
        const __m128 first_product =
            _mm_mul_ps(_mm_set1_ps(first_gain), first_value);
        const __m128 second_product =
            _mm_mul_ps(_mm_set1_ps(second_gain), second_value);
        // Native 0x583E50 evaluates second_product +
        // (first_product + accumulator).
        accumulator = _mm_add_ps(
            second_product, _mm_add_ps(first_product, accumulator));
    }
    _mm_storeu_ps(dst_4, accumulator);
}

void auro_matic_Engine1_fl32_process_ext(
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
                std::memset(ring + 3808u + 32u * (start + total_clear), 0,
                            32ull * count * sizeof(float));
            }
            *reinterpret_cast<std::uint32_t*>(engine1_state + 40u) = clear_index;
        } else if (total_clear == 0u && *reinterpret_cast<const std::uint32_t*>(engine1_state + 60u) != 0u) {
            std::memset(ring, 0, 0x7780u);
            clear_index = 0u;
            *reinterpret_cast<std::uint32_t*>(engine1_state + 40u) = clear_index;
        }
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
    /* Decompiled name: engine1_copy32_ring */
engine1_copy32_ring(ring, ring_a_offset, filtered_a);
    /* Decompiled name: engine1_copy32_ring */
engine1_copy32_ring(ring, ring_a_offset + 3808u, filtered_b);

    std::memset(output_0x300, 0, 0x300u);
    const std::uint32_t current_clear = *reinterpret_cast<const std::uint32_t*>(engine1_state + 68u);
    std::uint32_t next_write_index = write_index;
    if (current_clear >= total_clear) {
        const std::uint32_t group_count = *reinterpret_cast<const std::uint32_t*>(engine1_state + 132u);
        const std::int32_t history_offset = write_index == 0u
            ? 0
            : static_cast<std::int32_t>(32u * write_index) - 7616;
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
            const auto* index_table = reinterpret_cast<const std::uint8_t*>(
                static_cast<std::uintptr_t>(
                    *reinterpret_cast<const std::uint64_t*>(
                        engine1_state + 96u + 8u * group)));
            if (!index_table)
                continue;
            const std::uint32_t first_row = *reinterpret_cast<const std::uint32_t*>(
                source_table + 1568u + 4u * write_index);
            const std::uint32_t second_row = *reinterpret_cast<const std::uint32_t*>(
                source_table + 2520u + 4u * write_index);
            const auto* entries1 = index_table + 8u * first_row;
            const auto* entries2 = index_table + 1568u + 8u * second_row;
            for (std::size_t vec = 0; vec < 8u; ++vec)
                /* Decompiled name: engine1_sum_98_taps_to_vec */
engine1_sum_98_taps_to_vec(
                    ring,
                    history_offset + static_cast<std::uint32_t>(4u * vec),
                    entries1,
                    out + 4u * vec);
            for (std::size_t vec = 0; vec < 8u; ++vec)
                /* Decompiled name: engine1_sum_98_taps_to_vec */
engine1_sum_98_taps_to_vec(
                    ring,
                    history_offset + static_cast<std::uint32_t>(4u * vec),
                    entries2,
                    out + 32u + 4u * vec);
        }
    }

    *reinterpret_cast<std::uint32_t*>(engine1_state + 40u) = (next_write_index + 1u) % 0xEEu;
    *reinterpret_cast<std::uint32_t*>(engine1_state + 68u) = current_clear + 1u;
}

void auro_matic_XinN_Early_fl32_process_mode1(
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
    auro_matic_Engine1_fl32_process_ext(early_state, left, right, output_0x300, gains_32);
}

void /* Decompiled name: add_scaled_channel_32 */
add_scaled_channel_32(float* dst, const void* src_raw, float gain) {
    if (!dst || !src_raw)
        return;
    const auto* src = static_cast<const float*>(src_raw);
    for (std::size_t i = 0; i < 32u; ++i)
        dst[i] += src[i] * gain;
}

void auro_matic_XinN_Early_fl32_process_mode2(
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
        auro_matic_Engine1_fl32_process_ext(early_state, left, right, output_a_0x300, gains_32);

    float downmix_a[32]{};
    float downmix_b[32]{};
    const float gain_surround = *reinterpret_cast<const float*>(early_state + 320u);
    const float gain_center = *reinterpret_cast<const float*>(early_state + 324u);
    const float gain_back = *reinterpret_cast<const float*>(early_state + 328u);

    /* Decompiled name: add_scaled_channel_32 */
add_scaled_channel_32(downmix_a, channel_span_31[4], gain_surround);
    /* Decompiled name: add_scaled_channel_32 */
add_scaled_channel_32(downmix_b, channel_span_31[5], gain_surround);
    if (*reinterpret_cast<const std::uint32_t*>(early_state + 280u) != 0u) {
        /* Decompiled name: add_scaled_channel_32 */
add_scaled_channel_32(downmix_a, channel_span_31[6], gain_center);
        /* Decompiled name: add_scaled_channel_32 */
add_scaled_channel_32(downmix_b, channel_span_31[6], gain_center);
    }
    if (*reinterpret_cast<const std::uint32_t*>(early_state + 276u) != 0u) {
        /* Decompiled name: add_scaled_channel_32 */
add_scaled_channel_32(downmix_a, channel_span_31[7], gain_back);
        /* Decompiled name: add_scaled_channel_32 */
add_scaled_channel_32(downmix_b, channel_span_31[8], gain_back);
    }
    auro_matic_Engine1_fl32_process_ext(
        early_state + 136u,
        downmix_a,
        downmix_b,
        output_b_0x300,
        gains_32);
}

void auro_matic_XinN_Early_fl32_set_total_clear_frames(
    std::uint8_t* early_state,
    std::uint32_t frames) {
    if (!early_state)
        return;
    auro_matic_Engine1_fl32_set_total_clear_frames(early_state, frames);
    if (*reinterpret_cast<const std::uint32_t*>(early_state + 272u) != 0u)
        auro_matic_Engine1_fl32_set_total_clear_frames(early_state + 136u, frames);
}

void auro_matic_XinN_Early_fl32_set_downmix(
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

std::int64_t auro_matic_XinN_Early_fl32_reset_audio_state(std::uint8_t* early_state) {
    if (!early_state)
        return 0;
    std::int64_t result = auro_matic_Engine1_fl32_reset_audio_state(early_state);
    if (*reinterpret_cast<const std::uint32_t*>(early_state + 272u) != 0u)
        result = auro_matic_Engine1_fl32_reset_audio_state(early_state + 136u);
    return result;
}

bool auro_matic_XinN_Early_fl32_partial_clear(
    std::uint8_t* early_state,
    std::uint32_t* remaining) {
    if (!early_state || !remaining)
        return false;
    const bool first_done = auro_matic_Engine1_fl32_partial_clear(early_state, remaining);
    if (!first_done)
        return false;
    if (*reinterpret_cast<const std::uint32_t*>(early_state + 272u) == 0u)
        return true;
    return auro_matic_Engine1_fl32_partial_clear(early_state + 136u, remaining);
}

void auro_matic_XinN_Early_fl32_get_delayed_mode1(
    std::uint8_t* engine1_state,
    float* out_near_32,
    float* out_far_32) {
    if (!engine1_state || !out_near_32 || !out_far_32)
        return;
    std::uint64_t near_ptr = 0;
    std::uint64_t far_ptr = 0;
    auro_matic_Engine1_fl32_get_delayed_frame(engine1_state, &near_ptr, &far_ptr);
    if (near_ptr != 0u)
        std::memcpy(out_near_32, reinterpret_cast<const void*>(static_cast<std::uintptr_t>(near_ptr)), 32u * sizeof(float));
    if (far_ptr != 0u)
        std::memcpy(out_far_32, reinterpret_cast<const void*>(static_cast<std::uintptr_t>(far_ptr)), 32u * sizeof(float));
}

void auro_matic_XinN_Early_fl32_mix_delayed_mode2(
    std::uint8_t* early_state,
    float* out_near_32,
    float* out_far_32) {
    if (!early_state || !out_near_32 || !out_far_32)
        return;
    std::uint64_t a_near_ptr = 0;
    std::uint64_t a_far_ptr = 0;
    std::uint64_t b_near_ptr = 0;
    std::uint64_t b_far_ptr = 0;
    auro_matic_Engine1_fl32_get_delayed_frame(early_state, &a_near_ptr, &a_far_ptr);
    auro_matic_Engine1_fl32_get_delayed_frame(early_state + 136u, &b_near_ptr, &b_far_ptr);
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

void /* Decompiled name: e2_reverb_process */
e2_reverb_process(
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

std::int64_t auro_matic_Engine2_fl32_construct(std::uint64_t engine2_state, std::uint64_t memory) {
    if (engine2_state == 0u || memory == 0u)
        return 0;
    auto* state = e2_ptr(engine2_state);
    e2_write_u64(state, 104u, 0u);
    e2_write_u64(state, 64u, memory + 128u);
    e2_write_u64(state, 72u, memory + 128u);
    e2_write_u32(state, 80u, kEngine2AllocatedFloats);
    return static_cast<std::int64_t>(engine2_state);
}

void auro_matic_Engine2_fl32_set_total_clear_frames(
    std::uint64_t engine2_state,
    std::uint32_t frames) {
    if (engine2_state == 0u)
        return;
    e2_write_u32(e2_ptr(engine2_state), 92u, frames);
}

void auro_matic_Engine2_fl32_set_preset(std::uint64_t engine2_state, std::uint64_t preset) {
    if (engine2_state == 0u)
        return;
    e2_write_u64(e2_ptr(engine2_state), 104u, preset);
}

std::int64_t auro_matic_Engine2_fl32_reset_audio_state(std::uint64_t engine2_state) {
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

bool auro_matic_Engine2_fl32_partial_clear(std::uint64_t engine2_state, std::uint32_t* remaining) {
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

void auro_matic_Engine2_fl32_set_output_patch(
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

void* auro_matic_Engine2_fl32_process(
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
            /* Decompiled name: e2_reverb_process */
e2_reverb_process(state, ring, preset, i, cross, frame);
            e2_accumulate_frame(accum + 0u, frame, e2_bus_weight(bus[0], 4u * i + 0u));
            e2_accumulate_frame(accum + 32u, frame, e2_bus_weight(bus[1], 4u * i + 0u));
            e2_accumulate_frame(accum + 64u, frame, e2_bus_weight(bus[2], 4u * i + 0u));

            /* Decompiled name: e2_reverb_process */
e2_reverb_process(state, ring, preset, i | 8u, cross, frame);
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

std::int64_t auro_matic_XinN_Late_fl32_construct(std::uint32_t* late_state, std::uint64_t memory) {
    if (!late_state)
        return 0;
    late_state[0] = memory != 0u ? 1u : 0u;
    if (memory != 0u)
        return auro_matic_Engine2_fl32_construct(
            reinterpret_cast<std::uint64_t>(late_state + 2),
            memory);
    return 0;
}

std::int64_t auro_matic_XinN_Late_fl32_reset_audio_state(std::uint32_t* late_state) {
    if (!late_state)
        return 0;
    late_state[37] = 0u;
    if (late_state[0] != 0u)
        return auro_matic_Engine2_fl32_reset_audio_state(reinterpret_cast<std::uint64_t>(late_state + 2));
    return 0;
}

void auro_matic_XinN_Late_fl32_set_clear_frames(
    std::uint32_t* late_state,
    std::uint32_t early_clear_frames,
    std::uint32_t late_clear_frames) {
    if (!late_state)
        return;
    late_state[36] = early_clear_frames;
    auro_matic_Engine2_fl32_set_total_clear_frames(
        reinterpret_cast<std::uint64_t>(late_state + 2),
        late_clear_frames);
}

bool auro_matic_XinN_Late_fl32_partial_clear(std::uint32_t* late_state, std::uint32_t* remaining) {
    if (!late_state)
        return false;
    return late_state[0] == 0u
        || auro_matic_Engine2_fl32_partial_clear(
            reinterpret_cast<std::uint64_t>(late_state + 2),
            remaining);
}

void auro_matic_XinN_Late_fl32_set_preset(std::uint32_t* late_state, std::uint64_t preset) {
    if (!late_state || late_state[0] == 0u || preset == 0u)
        return;
    auro_matic_Engine2_fl32_set_preset(
        reinterpret_cast<std::uint64_t>(late_state + 2),
        preset + 64u);
}

void auro_matic_XinN_Late_fl32_set_output_patch(std::uint32_t* late_state, const std::uint32_t* patch_3) {
    if (!late_state || late_state[0] == 0u)
        return;
    auro_matic_Engine2_fl32_set_output_patch(
        reinterpret_cast<std::uint64_t>(late_state + 2),
        patch_3);
}

void* auro_matic_XinN_Late_fl32_process(
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
            auro_matic_XinN_Early_fl32_get_delayed_mode1(
                reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(engine_state)),
                reinterpret_cast<float*>(route_a.data()),
                reinterpret_cast<float*>(route_b.data()));
        } else if (route_u64 == 0x577BC0u) {
            auro_matic_XinN_Early_fl32_mix_delayed_mode2(
                reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(engine_state)),
                reinterpret_cast<float*>(route_a.data()),
                reinterpret_cast<float*>(route_b.data()));
        } else {
            const auto route = reinterpret_cast<RoutingFn>(static_cast<std::uintptr_t>(route_u64));
            if (route)
                route(engine_state, route_a.data(), route_b.data());
        }
        auto* process = engine2_process ? engine2_process : auro_matic_Engine2_fl32_process;
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

std::int64_t auro_matic_Engine1_fl32_construct(std::uint8_t* engine1_state, std::uint64_t memory) {
    if (!engine1_state || memory == 0u)
        return 0;
    *reinterpret_cast<std::uint32_t*>(engine1_state + 132u) = 0u;
    *reinterpret_cast<std::uint32_t*>(engine1_state + 28u) = 0u;
    *reinterpret_cast<std::uint64_t*>(engine1_state + 16u) = memory + 0x7800u;
    *reinterpret_cast<std::uint64_t*>(engine1_state + 0u) = 0u;
    *reinterpret_cast<std::uint64_t*>(engine1_state + 8u) = memory;
    *reinterpret_cast<std::uint32_t*>(engine1_state + 60u) = 7648u;
    *reinterpret_cast<std::uint64_t*>(engine1_state + 24u) = memory + 0x8440u;
    *reinterpret_cast<std::uint64_t*>(engine1_state + 32u) = memory + 0x9080u;
    return reinterpret_cast<std::int64_t>(engine1_state);
}

void /* Decompiled name: engine1_copy_preset_table */
engine1_copy_preset_table(std::uint64_t dst_u64, std::uint64_t src_u64) {
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

void auro_matic_Engine1_fl32_set_preset(std::uint8_t* engine1_state, std::uint64_t preset) {
    if (!engine1_state)
        return;
    *reinterpret_cast<std::uint64_t*>(engine1_state + 0u) = preset;
    if (preset == 0u)
        return;
    const auto* p = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(preset));
    for (std::uint32_t table = 0; table < 3u; ++table) {
        const std::uint64_t src = *reinterpret_cast<const std::uint64_t*>(p + 32u + 8u * table);
        const std::uint64_t dst = *reinterpret_cast<const std::uint64_t*>(engine1_state + 16u + 8u * table);
        /* Decompiled name: engine1_copy_preset_table */
engine1_copy_preset_table(dst, src);
    }
}

std::int64_t auro_matic_Engine1_fl32_set_output_patch(std::uint8_t* engine1_state, const std::uint32_t* patch_3) {
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

std::int64_t auro_matic_XinN_Early_fl32_construct(
    std::uint8_t* early_state,
    std::uint64_t memory_a,
    std::uint64_t memory_b) {
    if (!early_state)
        return 0;
    std::int64_t result = auro_matic_Engine1_fl32_construct(early_state, memory_a);
    *reinterpret_cast<std::uint32_t*>(early_state + 272u) = memory_b != 0u ? 1u : 0u;
    if (memory_b != 0u)
        result = auro_matic_Engine1_fl32_construct(early_state + 136u, memory_b);
    *reinterpret_cast<std::uint64_t*>(early_state + 276u) = 0u;
    *reinterpret_cast<std::uint64_t*>(early_state + 312u) = 0u;
    return result;
}

std::int64_t auro_matic_XinN_Early_fl32_set_output_patch(
    std::uint8_t* early_state,
    const std::uint32_t* primary_patch_3,
    const std::uint32_t* secondary_patch_3) {
    if (!early_state)
        return 0;
    std::int64_t result = auro_matic_Engine1_fl32_set_output_patch(early_state, primary_patch_3);
    if (*reinterpret_cast<const std::uint32_t*>(early_state + 272u) != 0u)
        result = auro_matic_Engine1_fl32_set_output_patch(early_state + 136u, secondary_patch_3);
    return result;
}

std::uint64_t auro_matic_XinN_Early_fl32_set_preset(std::uint8_t* early_state, std::uint64_t preset) {
    if (!early_state)
        return 0u;
    *reinterpret_cast<std::uint64_t*>(early_state + 312u) = preset;
    if (preset != 0u) {
        auro_matic_Engine1_fl32_set_preset(early_state, preset + 8u);
        if (*reinterpret_cast<const std::uint32_t*>(early_state + 272u) != 0u)
            auro_matic_Engine1_fl32_set_preset(early_state + 136u, preset + 8u);
    }
    if (preset != 0u) {
        *reinterpret_cast<std::uint64_t*>(early_state + 332u) =
            *reinterpret_cast<const std::uint64_t*>(static_cast<std::uintptr_t>(preset));
    }
    return preset;
}

void auro_matic_XinN_parameter_Dynamic_t_default(std::uint8_t* dynamic_48) {
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

void auro_matic_v3_XinN_parameter_Dynamic_t_default(std::uint8_t* dynamic_120) {
    if (!dynamic_120)
        return;
    auro_matic_XinN_parameter_Dynamic_t_default(dynamic_120);
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

void auro_matic_v3_XinN_parameter_Dynamic_set_stereo_Xs(
    std::uint8_t* dynamic,
    std::int32_t input_id,
    std::int32_t output_id,
    float gain_a,
    float gain_b) {
    if (!dynamic)
        return;
    *reinterpret_cast<std::int32_t*>(dynamic + 0u) = input_id;
    *reinterpret_cast<std::int32_t*>(dynamic + 24u) = output_id;
    *reinterpret_cast<float*>(dynamic + 48u) = gain_a;
    *reinterpret_cast<float*>(dynamic + 52u) = gain_b;
}

void auro_matic_v3_XinN_parameter_Dynamic_set_stereo_HX(
    std::uint8_t* dynamic,
    std::int32_t input_id,
    std::int32_t output_id,
    float gain_a,
    float gain_b) {
    if (!dynamic)
        return;
    *reinterpret_cast<std::int32_t*>(dynamic + 4u) = input_id;
    *reinterpret_cast<std::int32_t*>(dynamic + 28u) = output_id;
    *reinterpret_cast<float*>(dynamic + 56u) = gain_a;
    *reinterpret_cast<float*>(dynamic + 60u) = gain_b;
}

void auro_matic_v3_XinN_parameter_Dynamic_set_stereo_HXs(
    std::uint8_t* dynamic,
    std::int32_t input_id,
    std::int32_t output_id,
    float gain_a,
    float gain_b) {
    if (!dynamic)
        return;
    *reinterpret_cast<std::int32_t*>(dynamic + 8u) = input_id;
    *reinterpret_cast<std::int32_t*>(dynamic + 32u) = output_id;
    *reinterpret_cast<float*>(dynamic + 64u) = gain_a;
    *reinterpret_cast<float*>(dynamic + 68u) = gain_b;
}

void auro_matic_v3_XinN_parameter_Dynamic_set_surround_HX(
    std::uint8_t* dynamic,
    std::int32_t input_a,
    std::int32_t input_b,
    std::int32_t output_id,
    float gain_a,
    float gain_b,
    float gain_c) {
    if (!dynamic)
        return;
    *reinterpret_cast<std::int32_t*>(dynamic + 4u) = input_a;
    *reinterpret_cast<std::int32_t*>(dynamic + 20u) = input_b;
    *reinterpret_cast<std::int32_t*>(dynamic + 28u) = output_id;
    *reinterpret_cast<float*>(dynamic + 72u) = gain_a;
    *reinterpret_cast<float*>(dynamic + 76u) = gain_b;
    *reinterpret_cast<float*>(dynamic + 80u) = gain_c;
}

void auro_matic_v3_XinN_parameter_Dynamic_set_surround_HXs(
    std::uint8_t* dynamic,
    std::int32_t input_a,
    std::int32_t input_b,
    std::int32_t output_id,
    float gain_a,
    float gain_b,
    float gain_c) {
    if (!dynamic)
        return;
    *reinterpret_cast<std::int32_t*>(dynamic + 8u) = input_a;
    *reinterpret_cast<std::int32_t*>(dynamic + 16u) = input_b;
    *reinterpret_cast<std::int32_t*>(dynamic + 32u) = output_id;
    *reinterpret_cast<float*>(dynamic + 96u) = gain_a;
    *reinterpret_cast<float*>(dynamic + 100u) = gain_b;
    *reinterpret_cast<float*>(dynamic + 104u) = gain_c;
}

void auro_matic_v3_XinN_parameter_Dynamic_set_surround_HC_T(
    std::uint8_t* dynamic,
    std::int32_t input_a,
    std::int32_t input_b,
    std::int32_t output_id,
    float gain_a,
    float gain_b,
    float gain_c,
    float gain_d,
    float gain_e,
    float gain_f) {
    if (!dynamic)
        return;
    *reinterpret_cast<std::int32_t*>(dynamic + 0u) = input_a;
    *reinterpret_cast<std::int32_t*>(dynamic + 12u) = input_b;
    *reinterpret_cast<std::int32_t*>(dynamic + 24u) = output_id;
    *reinterpret_cast<float*>(dynamic + 84u) = gain_a;
    *reinterpret_cast<float*>(dynamic + 88u) = gain_c;
    *reinterpret_cast<float*>(dynamic + 92u) = gain_e;
    *reinterpret_cast<float*>(dynamic + 108u) = gain_b;
    *reinterpret_cast<float*>(dynamic + 112u) = gain_d;
    *reinterpret_cast<float*>(dynamic + 116u) = gain_f;
}

void auro_matic_v3_XinN_parameter_Dynamic_mute_stereo_Xs(std::uint8_t* dynamic) {
    if (!dynamic)
        return;
    *reinterpret_cast<std::int32_t*>(dynamic + 0u) = 0;
    *reinterpret_cast<std::int32_t*>(dynamic + 24u) = 0;
    *reinterpret_cast<std::uint64_t*>(dynamic + 48u) = 0u;
}

void auro_matic_v3_XinN_parameter_Dynamic_mute_stereo_HX(std::uint8_t* dynamic) {
    if (!dynamic)
        return;
    *reinterpret_cast<std::int32_t*>(dynamic + 4u) = 0;
    *reinterpret_cast<std::int32_t*>(dynamic + 28u) = 0;
    *reinterpret_cast<std::uint64_t*>(dynamic + 56u) = 0u;
}

void auro_matic_v3_XinN_parameter_Dynamic_mute_stereo_HXs(std::uint8_t* dynamic) {
    if (!dynamic)
        return;
    *reinterpret_cast<std::int32_t*>(dynamic + 8u) = 0;
    *reinterpret_cast<std::int32_t*>(dynamic + 32u) = 0;
    *reinterpret_cast<std::uint64_t*>(dynamic + 64u) = 0u;
}

void auro_matic_v3_XinN_parameter_Dynamic_mute_surround_HX(std::uint8_t* dynamic) {
    if (!dynamic)
        return;
    *reinterpret_cast<std::int32_t*>(dynamic + 4u) = 0;
    *reinterpret_cast<std::int32_t*>(dynamic + 20u) = 0;
    *reinterpret_cast<std::int32_t*>(dynamic + 28u) = 0;
    *reinterpret_cast<std::uint64_t*>(dynamic + 72u) = 0u;
    *reinterpret_cast<std::uint32_t*>(dynamic + 80u) = 0u;
}

void auro_matic_v3_XinN_parameter_Dynamic_mute_surround_HXs(std::uint8_t* dynamic) {
    if (!dynamic)
        return;
    *reinterpret_cast<std::int32_t*>(dynamic + 8u) = 0;
    *reinterpret_cast<std::int32_t*>(dynamic + 16u) = 0;
    *reinterpret_cast<std::int32_t*>(dynamic + 32u) = 0;
    *reinterpret_cast<std::uint64_t*>(dynamic + 96u) = 0u;
    *reinterpret_cast<std::uint32_t*>(dynamic + 104u) = 0u;
}

void auro_matic_v3_XinN_parameter_Dynamic_mute_surround_HC_T(std::uint8_t* dynamic) {
    if (!dynamic)
        return;
    *reinterpret_cast<std::int32_t*>(dynamic + 0u) = 0;
    *reinterpret_cast<std::int32_t*>(dynamic + 12u) = 0;
    *reinterpret_cast<std::int32_t*>(dynamic + 24u) = 0;
    *reinterpret_cast<std::uint32_t*>(dynamic + 92u) = 0u;
    *reinterpret_cast<std::uint64_t*>(dynamic + 84u) = 0u;
    *reinterpret_cast<std::uint64_t*>(dynamic + 108u) = 0u;
}

void auro_matic_v3_XinN_Routing_fl32_construct(std::uint8_t* routing_state) {
    if (!routing_state)
        return;
    *reinterpret_cast<std::uint32_t*>(routing_state + 64u) = 0u;
    *reinterpret_cast<std::uint64_t*>(routing_state + 0u) = 0x557460u;
    *reinterpret_cast<std::uint64_t*>(routing_state + 8u) = 0x5575C0u;
    *reinterpret_cast<std::uint64_t*>(routing_state + 16u) = 0x5570C0u;
    *reinterpret_cast<std::uint64_t*>(routing_state + 24u) = 0x5570E0u;
}

std::uint64_t auro_matic_v3_XinN_Routing_fl32_set_routing(
    std::uint8_t* routing_state,
    const std::uint8_t* routing_72) {
    if (!routing_state || !routing_72)
        return 0u;
    std::memcpy(routing_state + 72u, routing_72, 72u);
    std::memcpy(routing_state + 144u, routing_72, 72u);
    return *reinterpret_cast<const std::uint64_t*>(routing_72 + 64u);
}

std::uint64_t auro_matic_v3_XinN_Routing_fl32_get_routing(
    const std::uint8_t* routing_state,
    std::uint8_t* routing_72) {
    if (!routing_state || !routing_72)
        return 0u;
    const std::uint64_t result = *reinterpret_cast<const std::uint64_t*>(routing_state + 136u);
    std::memcpy(routing_72, routing_state + 72u, 72u);
    return result;
}

std::int64_t auro_matic_v3_XinN_Routing_fl32_configure(
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
constexpr std::size_t kXinnEngine1SourceTableSize_35c0b0 = 3472u;
constexpr std::size_t kXinnEngine2BusPresetSize_34bdd0 = 136u;
constexpr std::uint64_t kXinnSpreset2in6Base_570460 = 0x73F6B8u;
constexpr std::uint64_t kXinnSpreset5inNBase_570350 = 0x738078u;
constexpr std::uint32_t kXinnSpresetStride_35bf40 = 0x1D90u;
constexpr std::uint32_t kXinnProcessedPresetResourceId = 101u;
constexpr std::size_t kXinnProcessedPresetBytes = 23856u;
constexpr std::size_t kXinnProcessedPresetResourceBytes = 50u * kXinnProcessedPresetBytes;
constexpr std::uint8_t kXinnProcessedPresetMarker = 0xA5u;

#ifndef AURO3DENG_ENABLE_NATIVE_IMAGE_PRESET_POINTERS
#define AURO3DENG_ENABLE_NATIVE_IMAGE_PRESET_POINTERS 0
#endif

bool /* Decompiled name: xinn_runtime_preset_pointer_available */
xinn_runtime_preset_pointer_available(std::uint64_t runtime_preset) {
    if (runtime_preset == 0u)
        return false;
#if AURO3DENG_ENABLE_NATIVE_IMAGE_PRESET_POINTERS
    return true;
#else
    (void)runtime_preset;
    return false;
#endif
}

std::uint64_t xinn_select_native_spreset_from_v4_static(
    std::uint32_t family,
    bool surround_mode,
    const std::uint8_t* tuning_static);

std::uint64_t /* Decompiled name: xinn_select_native_spreset_from_v4_static */
xinn_select_native_spreset_from_v4_static(
    bool surround_mode,
    const std::uint8_t* tuning_static) {
    return xinn_select_native_spreset_from_v4_static(
        4u, surround_mode, tuning_static);
}

std::uint64_t /* Decompiled name: PresetManager family selector */
xinn_select_native_spreset_from_v4_static(
    std::uint32_t family,
    bool surround_mode,
    const std::uint8_t* tuning_static) {
    if (!tuning_static)
        return 0u;
    if (family >= 5u)
        return 0u;
    const std::uint32_t room = std::min<std::uint32_t>(
        *reinterpret_cast<const std::uint32_t*>(tuning_static + 4u), 4u);

    // Native PresetManager_configure uses the room order
    // Small, Medium, Large, Movie, Speech (libauro(1).so.c:376380+).
    // The extracted SS/SB resource groups are stored as
    // Small, Speech, Movie, Medium, Large, so the address table remains
    // explicit instead of relying on a non-native contiguous stride.
    static constexpr std::uint64_t kNativePresetAddresses[5][2][5] = {
        {
            {0x5C9438u, 0x5C9C88u, 0x5CA0B0u, 0x5C9860u, 0x5C9010u},
            {0x5C2C50u, 0x5C34A0u, 0x5C38C8u, 0x5C3078u, 0x5C2828u},
        },
        {
            {0x5CD290u, 0x5CDAE0u, 0x5CDF08u, 0x5CD6B8u, 0x5CCE68u},
            {0x5C6AA8u, 0x5C72F8u, 0x5C7720u, 0x5C6ED0u, 0x5C6680u},
        },
        {
            {0x5CBDC8u, 0x5CC618u, 0x5CCA40u, 0x5CC1F0u, 0x5CB9A0u},
            {0x5C55E0u, 0x5C5E30u, 0x5C6258u, 0x5C5A08u, 0x5C51B8u},
        },
        {
            {0x5CA900u, 0x5CB150u, 0x5CB578u, 0x5CAD28u, 0x5CA4D8u},
            {0x5C4118u, 0x5C4968u, 0x5C4D90u, 0x5C4540u, 0x5C3CF0u},
        },
        {
            {0x5CE758u, 0x5CEFA8u, 0x5CF3D0u, 0x5CEB80u, 0x5CE330u},
            {0x5C7F70u, 0x5C87C0u, 0x5C8BE8u, 0x5C8398u, 0x5C7B48u},
        },
    };
    return kNativePresetAddresses[family][surround_mode ? 1u : 0u][room];
}

const std::uint8_t* /* Decompiled name: xinn_portable_spreset_from_native_va */
xinn_portable_spreset_from_native_va(std::uint64_t spreset_u64) {
    (void)spreset_u64;
    return nullptr;
}

const std::uint8_t* /* Decompiled name: xinn_portable_processed_preset_from_native_va */
xinn_portable_processed_preset_from_native_va(std::uint64_t spreset_u64) {
    static constexpr std::uint64_t kNativePresetAddresses[5][2][5] = {
        {{0x5C9438u, 0x5C9C88u, 0x5CA0B0u, 0x5C9860u, 0x5C9010u},
         {0x5C2C50u, 0x5C34A0u, 0x5C38C8u, 0x5C3078u, 0x5C2828u}},
        {{0x5CD290u, 0x5CDAE0u, 0x5CDF08u, 0x5CD6B8u, 0x5CCE68u},
         {0x5C6AA8u, 0x5C72F8u, 0x5C7720u, 0x5C6ED0u, 0x5C6680u}},
        {{0x5CBDC8u, 0x5CC618u, 0x5CCA40u, 0x5CC1F0u, 0x5CB9A0u},
         {0x5C55E0u, 0x5C5E30u, 0x5C6258u, 0x5C5A08u, 0x5C51B8u}},
        {{0x5CA900u, 0x5CB150u, 0x5CB578u, 0x5CAD28u, 0x5CA4D8u},
         {0x5C4118u, 0x5C4968u, 0x5C4D90u, 0x5C4540u, 0x5C3CF0u}},
        {{0x5CE758u, 0x5CEFA8u, 0x5CF3D0u, 0x5CEB80u, 0x5CE330u},
         {0x5C7F70u, 0x5C87C0u, 0x5C8BE8u, 0x5C8398u, 0x5C7B48u}},
    };
    std::uint32_t family = 0u;
    std::uint32_t variant = 0u;
    std::uint32_t room = 0u;
    bool found = false;
    for (std::uint32_t f = 0u; f < 5u && !found; ++f) {
        for (std::uint32_t v = 0u; v < 2u && !found; ++v) {
            for (std::uint32_t r = 0u; r < 5u; ++r) {
                if (kNativePresetAddresses[f][v][r] == spreset_u64) {
                    family = f;
                    variant = v;
                    room = r;
                    found = true;
                    break;
                }
            }
        }
    }
    if (!found)
        return nullptr;
    // Resource order: multichannel stereo/surround first, then each SS/SB
    // family as stereo/surround blocks, each in extracted image order
    // Small, Speech, Movie, Medium, Large.
    static constexpr std::uint32_t kResourceRoom[5] = {0u, 3u, 4u, 2u, 1u};
    const std::uint32_t family_block = family == 4u ? 0u : 10u + family * 10u;
    const std::uint32_t resource_index = family_block + variant * 5u
        + kResourceRoom[room];
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
    return presets ? presets + resource_index * kXinnProcessedPresetBytes : nullptr;
#else
    return nullptr;
#endif
}

bool /* Decompiled name: xinn_initialize_processed_preset_from_portable_spreset */
xinn_initialize_processed_preset_from_portable_spreset(
    std::uint8_t* processed_preset,
    std::uint64_t spreset_u64) {
    const std::uint8_t* preset = /* Decompiled name: xinn_portable_processed_preset_from_native_va */
xinn_portable_processed_preset_from_native_va(spreset_u64);
    if (!processed_preset || !preset)
        return false;

    std::memcpy(processed_preset, preset, kXinnProcessedPresetBytes);
    processed_preset[0] = 1u;
    processed_preset[1] = kXinnProcessedPresetMarker;
    return true;
}

void /* Decompiled name: xinn_engine2_initialize_static_from_spreset */
xinn_engine2_initialize_static_from_spreset(
    std::uint8_t* processed_preset,
    std::uint64_t spreset_u64) {
    if (processed_preset && processed_preset[1] == kXinnProcessedPresetMarker)
        return;
    const std::uint8_t* spreset = /* Decompiled name: xinn_portable_spreset_from_native_va */
xinn_portable_spreset_from_native_va(spreset_u64);
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

void /* Decompiled name: xinn_engine1_compute_type5_biquad */
xinn_engine1_compute_type5_biquad(std::uint8_t* storage_28, const std::uint8_t* spreset) {
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

float /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(float db) {
    return db > -144.0f ? std::pow(10.0f, db * 0.050000001f) : 0.0f;
}

std::uint32_t /* Decompiled name: xinn_u32_bits */
xinn_u32_bits(const float* word) {
    return *reinterpret_cast<const std::uint32_t*>(word);
}

void /* Decompiled name: xinn_engine1_apply_table */
xinn_engine1_apply_table(
    float* engine1_state,
    std::uint32_t config_base,
    std::uint32_t output_base,
    const float* gain_set_a,
    const float* gain_set_b,
    const float* inline_gain_xyz) {
    if (!engine1_state || !gain_set_a || !gain_set_b || !inline_gain_xyz)
        return;
    for (std::uint32_t row = 0u, out = output_base; row != 490u; row += 5u, out += 2u) {
        const std::uint32_t gain_index = /* Decompiled name: xinn_u32_bits */
xinn_u32_bits(engine1_state + config_base + row + 0u);
        if (gain_index >= 3u)
            continue;
        const float* selected =
            /* Decompiled name: xinn_u32_bits */
xinn_u32_bits(engine1_state + config_base + row + 1u) == 1u ? gain_set_b : gain_set_a;
        float scale = 1.0f;
        if (static_cast<std::uint8_t>(
                /* Decompiled name: xinn_u32_bits */
xinn_u32_bits(engine1_state + config_base + row + 2u))
            != 0u) {
            scale = inline_gain_xyz[gain_index];
        }
        engine1_state[out] = scale * engine1_state[config_base + row + 3u] * selected[gain_index];
    }
}

void /* Decompiled name: xinn_engine1_apply */
xinn_engine1_apply(float* engine1_state, const float* dynamic_64) {
    if (!engine1_state || !dynamic_64)
        return;

    float gain_set_a[3] = {};
    float gain_set_b[3] = {};
    const float base_a = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_64[0]);
    gain_set_a[0] = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_64[1]) * base_a;
    gain_set_a[1] = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_64[2]) * base_a;
    gain_set_a[2] = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_64[3]) * base_a;

    const float base_b = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_64[4]);
    gain_set_b[0] = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_64[5]) * base_b;
    gain_set_b[1] = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_64[6]) * base_b;
    gain_set_b[2] = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_64[7]) * base_b;

    const float inline_gain_xyz[3] = {
        *reinterpret_cast<const float*>(reinterpret_cast<const std::uint8_t*>(dynamic_64) + 32u),
        *reinterpret_cast<const float*>(reinterpret_cast<const std::uint8_t*>(dynamic_64) + 36u),
        *reinterpret_cast<const float*>(reinterpret_cast<const std::uint8_t*>(dynamic_64) + 40u),
    };
    /* Decompiled name: xinn_engine1_apply_table */
xinn_engine1_apply_table(
        engine1_state,
        0u,
        2943u,
        gain_set_a,
        gain_set_b,
        inline_gain_xyz);
    /* Decompiled name: xinn_engine1_apply_table */
xinn_engine1_apply_table(
        engine1_state,
        490u,
        3139u,
        gain_set_a,
        gain_set_b,
        inline_gain_xyz);
    /* Decompiled name: xinn_engine1_apply_table */
xinn_engine1_apply_table(
        engine1_state,
        980u,
        3811u,
        gain_set_a,
        gain_set_b,
        inline_gain_xyz);
    /* Decompiled name: xinn_engine1_apply_table */
xinn_engine1_apply_table(
        engine1_state,
        1470u,
        4007u,
        gain_set_a,
        gain_set_b,
        inline_gain_xyz);
    /* Decompiled name: xinn_engine1_apply_table */
xinn_engine1_apply_table(
        engine1_state,
        1960u,
        4679u,
        gain_set_a,
        gain_set_b,
        inline_gain_xyz);
    /* Decompiled name: xinn_engine1_apply_table */
xinn_engine1_apply_table(
        engine1_state,
        2450u,
        4875u,
        gain_set_a,
        gain_set_b,
        inline_gain_xyz);
}

void /* Decompiled name: xinn_preset_tune_late_stereo */
xinn_preset_tune_late_stereo(std::uint8_t* preset_base, const float* dynamic_96) {
    if (!preset_base || !dynamic_96)
        return;

    std::memcpy(preset_base + 1072u, dynamic_96, 64u);
    const float gain_sum_base = *reinterpret_cast<const float*>(preset_base + 1116u);
    *reinterpret_cast<float*>(preset_base + 23460u) =
        /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1120u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23596u) =
        /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1124u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23732u) =
        /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1128u) + gain_sum_base);

    const std::uint64_t engine2_storage = *reinterpret_cast<const std::uint64_t*>(preset_base + 23320u);
    if (engine2_storage != 0u)
        *reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(engine2_storage) + 960u) =
            *reinterpret_cast<const std::uint32_t*>(preset_base + 1132u);

    *reinterpret_cast<float*>(preset_base + 23784u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_96[16]);
    *reinterpret_cast<float*>(preset_base + 23788u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_96[19]);
    *reinterpret_cast<float*>(preset_base + 23792u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_96[17]);
    *reinterpret_cast<float*>(preset_base + 23796u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_96[20]);
    *reinterpret_cast<float*>(preset_base + 23800u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_96[18]);
    *reinterpret_cast<float*>(preset_base + 23804u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_96[21]);
}

void /* Decompiled name: xinn_preset_tune_late_surround */
xinn_preset_tune_late_surround(std::uint8_t* preset_base, const float* dynamic_112) {
    if (!preset_base || !dynamic_112)
        return;

    std::memcpy(preset_base + 1072u, dynamic_112, 64u);
    const float gain_sum_base = *reinterpret_cast<const float*>(preset_base + 1116u);
    *reinterpret_cast<float*>(preset_base + 23460u) =
        /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1120u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23596u) =
        /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1124u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23732u) =
        /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1128u) + gain_sum_base);

    const std::uint64_t engine2_storage = *reinterpret_cast<const std::uint64_t*>(preset_base + 23320u);
    if (engine2_storage != 0u)
        *reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(engine2_storage) + 960u) =
            *reinterpret_cast<const std::uint32_t*>(preset_base + 1132u);

    *reinterpret_cast<float*>(preset_base + 23808u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[16]);
    *reinterpret_cast<float*>(preset_base + 23812u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[20]);
    *reinterpret_cast<float*>(preset_base + 23816u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[24]);
    *reinterpret_cast<float*>(preset_base + 23820u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[19]);
    *reinterpret_cast<float*>(preset_base + 23824u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[23]);
    *reinterpret_cast<float*>(preset_base + 23828u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[27]);
    *reinterpret_cast<float*>(preset_base + 23832u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[17]);
    *reinterpret_cast<float*>(preset_base + 23836u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[21]);
    *reinterpret_cast<float*>(preset_base + 23840u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[25]);
    *reinterpret_cast<float*>(preset_base + 23844u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[18]);
    *reinterpret_cast<float*>(preset_base + 23848u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[22]);
    *reinterpret_cast<float*>(preset_base + 23852u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[26]);
}

[[maybe_unused]] void /* Decompiled name: xinn_preset_tune_stereo */
xinn_preset_tune_stereo(std::uint8_t* preset_base, const float* dynamic_96) {
    if (!preset_base || !dynamic_96)
        return;

    std::memcpy(preset_base + 1072u, dynamic_96, 64u);
    /* Decompiled name: xinn_engine1_apply */
xinn_engine1_apply(
        reinterpret_cast<float*>(preset_base + 1136u),
        reinterpret_cast<const float*>(preset_base + 1072u));

    const float gain_sum_base = *reinterpret_cast<const float*>(preset_base + 1116u);
    *reinterpret_cast<float*>(preset_base + 23460u) =
        /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1120u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23596u) =
        /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1124u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23732u) =
        /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1128u) + gain_sum_base);

    const std::uint64_t engine2_storage = *reinterpret_cast<const std::uint64_t*>(preset_base + 23320u);
    if (engine2_storage != 0u)
        *reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(engine2_storage) + 960u) =
            *reinterpret_cast<const std::uint32_t*>(preset_base + 1132u);

    *reinterpret_cast<float*>(preset_base + 23784u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_96[16]);
    *reinterpret_cast<float*>(preset_base + 23788u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_96[19]);
    *reinterpret_cast<float*>(preset_base + 23792u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_96[17]);
    *reinterpret_cast<float*>(preset_base + 23796u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_96[20]);
    *reinterpret_cast<float*>(preset_base + 23800u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_96[18]);
    *reinterpret_cast<float*>(preset_base + 23804u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_96[21]);
}

[[maybe_unused]] void /* Decompiled name: xinn_preset_tune_surround */
xinn_preset_tune_surround(std::uint8_t* preset_base, const float* dynamic_112) {
    if (!preset_base || !dynamic_112)
        return;

    std::memcpy(preset_base + 1072u, dynamic_112, 64u);
    /* Decompiled name: xinn_engine1_apply */
xinn_engine1_apply(
        reinterpret_cast<float*>(preset_base + 1136u),
        reinterpret_cast<const float*>(preset_base + 1072u));

    const float gain_sum_base = *reinterpret_cast<const float*>(preset_base + 1116u);
    *reinterpret_cast<float*>(preset_base + 23460u) =
        /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1120u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23596u) =
        /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1124u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23732u) =
        /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1128u) + gain_sum_base);

    const std::uint64_t engine2_storage = *reinterpret_cast<const std::uint64_t*>(preset_base + 23320u);
    if (engine2_storage != 0u)
        *reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(engine2_storage) + 960u) =
            *reinterpret_cast<const std::uint32_t*>(preset_base + 1132u);

    *reinterpret_cast<float*>(preset_base + 23808u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[16]);
    *reinterpret_cast<float*>(preset_base + 23812u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[20]);
    *reinterpret_cast<float*>(preset_base + 23816u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[24]);
    *reinterpret_cast<float*>(preset_base + 23820u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[19]);
    *reinterpret_cast<float*>(preset_base + 23824u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[23]);
    *reinterpret_cast<float*>(preset_base + 23828u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[27]);
    *reinterpret_cast<float*>(preset_base + 23832u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[17]);
    *reinterpret_cast<float*>(preset_base + 23836u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[21]);
    *reinterpret_cast<float*>(preset_base + 23840u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[25]);
    *reinterpret_cast<float*>(preset_base + 23844u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[18]);
    *reinterpret_cast<float*>(preset_base + 23848u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[22]);
    *reinterpret_cast<float*>(preset_base + 23852u) = /* Decompiled name: xinn_db_to_linear */
xinn_db_to_linear(dynamic_112[26]);
}

std::int64_t /* Decompiled name: xinn_early_initialize */
xinn_early_initialize(std::uint8_t* early_state, std::uint32_t mode, std::uint32_t input_mask) {
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

std::int64_t auro_matic_XinN_fl32_construct(
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
    (void)auro_matic_XinN_Early_fl32_construct(xinn_state + 48u, memory_args_5[0], memory_args_5[1]);
    auro_matic_XinN_Late_fl32_construct(
        reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
        memory_args_5[2]);
    auro_matic_XinN_parameter_Dynamic_t_default(xinn_state + 572u);
    xinn_apply_output_mask_4eb39c(
        xinn_state + 572u,
        *reinterpret_cast<const std::uint32_t*>(xinn_state + 0u),
        *reinterpret_cast<const std::uint32_t*>(xinn_state + 20u));
    auro_matic_XinN_Early_fl32_set_downmix(xinn_state + 48u, xinn_state + 608u);
    auro_matic_XinN_Early_fl32_set_output_patch(
        xinn_state + 48u,
        reinterpret_cast<const std::uint32_t*>(xinn_state + 572u),
        reinterpret_cast<const std::uint32_t*>(xinn_state + 584u));
    auro_matic_XinN_Late_fl32_set_output_patch(
        reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
        reinterpret_cast<const std::uint32_t*>(xinn_state + 596u));
    return reinterpret_cast<std::int64_t>(xinn_state);
}

void auro_matic_XinN_fl32_set_dynamic_parameters(
    std::uint8_t* xinn_state,
    const std::uint8_t* dynamic_48) {
    if (!xinn_state || !dynamic_48)
        return;
    std::memcpy(xinn_state + 572u, dynamic_48, 48u);
    xinn_apply_output_mask_4eb39c(
        xinn_state + 572u,
        *reinterpret_cast<const std::uint32_t*>(xinn_state + 0u),
        *reinterpret_cast<const std::uint32_t*>(xinn_state + 20u));
    auro_matic_XinN_Early_fl32_set_downmix(xinn_state + 48u, xinn_state + 608u);
    auro_matic_XinN_Early_fl32_set_output_patch(
        xinn_state + 48u,
        reinterpret_cast<const std::uint32_t*>(xinn_state + 572u),
        reinterpret_cast<const std::uint32_t*>(xinn_state + 584u));
    auro_matic_XinN_Late_fl32_set_output_patch(
        reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
        reinterpret_cast<const std::uint32_t*>(xinn_state + 596u));
}

void auro_matic_XinN_fl32_get_dynamic_parameters(
    const std::uint8_t* xinn_state,
    std::uint8_t* dynamic_48) {
    if (!xinn_state || !dynamic_48)
        return;
    std::memcpy(dynamic_48, xinn_state + 572u, 48u);
}

std::int64_t auro_matic_XinN_fl32_initialize(
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
    auro_matic_XinN_parameter_Dynamic_t_default(xinn_state + 572u);
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
        auro_matic_XinN_Early_fl32_set_total_clear_frames(xinn_state + 48u, early_clear);
        auro_matic_XinN_Late_fl32_set_clear_frames(
            reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
            early_clear,
            late_blocks - early_clear);
        auro_matic_XinN_Early_fl32_set_preset(xinn_state + 48u, preset);
        auro_matic_XinN_Late_fl32_set_preset(
            reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
            preset);
        xinn_apply_output_mask_4eb39c(xinn_state + 572u, mode, masked_output);
        auro_matic_XinN_Early_fl32_set_downmix(xinn_state + 48u, xinn_state + 608u);
        auro_matic_XinN_Early_fl32_set_output_patch(
            xinn_state + 48u,
            reinterpret_cast<const std::uint32_t*>(xinn_state + 572u),
            reinterpret_cast<const std::uint32_t*>(xinn_state + 584u));
        auro_matic_XinN_Late_fl32_set_output_patch(
            reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
            reinterpret_cast<const std::uint32_t*>(xinn_state + 596u));
    }

    std::int64_t ok = /* Decompiled name: xinn_early_initialize */
xinn_early_initialize(xinn_state + 48u, mode, input_mask);
    if (!ok)
        return ok;
    std::uint64_t routing_u64 = *reinterpret_cast<const std::uint64_t*>(xinn_state + 552u);
    if (routing_u64 == 0u)
        routing_u64 = reinterpret_cast<std::uint64_t>(xinn_state + 624u);
    if (!auro_matic_v3_XinN_Routing_fl32_configure(
            reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(routing_u64)),
            mode,
            masked_output,
            input_mask))
        return 0;
    *reinterpret_cast<std::uint64_t*>(xinn_state + 24u) =
        *reinterpret_cast<const std::uint64_t*>(xinn_state + 40u) != 0u
            ? 0x574E30u
            : 0x574EE0u;
    auro_audio_Smooth_fl32_inst_initialize(reinterpret_cast<float*>(xinn_state + 560u), 48000u, 0.001f);
    auro_matic_XinN_Early_fl32_reset_audio_state(xinn_state + 48u);
    auro_matic_XinN_Late_fl32_reset_audio_state(reinterpret_cast<std::uint32_t*>(xinn_state + 392u));
    auro_audio_Smooth_fl32_inst_set_current(reinterpret_cast<float*>(xinn_state + 560u), 0.0f);
    auro_audio_Smooth_fl32_inst_update(reinterpret_cast<float*>(xinn_state + 560u), 1.0f);
    xinn_apply_output_mask_4eb39c(xinn_state + 572u, mode, masked_output);
    auro_matic_XinN_Early_fl32_set_downmix(xinn_state + 48u, xinn_state + 608u);
    auro_matic_XinN_Early_fl32_set_output_patch(
        xinn_state + 48u,
        reinterpret_cast<const std::uint32_t*>(xinn_state + 572u),
        reinterpret_cast<const std::uint32_t*>(xinn_state + 584u));
    auro_matic_XinN_Late_fl32_set_output_patch(
        reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
        reinterpret_cast<const std::uint32_t*>(xinn_state + 596u));
    return 1;
}

std::int64_t auro_matic_v3_XinN_fl32_construct(
    std::uint8_t* xinn_v3_state,
    const std::uint64_t* memory_args_5) {
    if (!xinn_v3_state)
        return 0;
    auro_matic_v3_XinN_Routing_fl32_construct(xinn_v3_state + 624u);
    if (auro_matic_XinN_fl32_construct(xinn_v3_state, xinn_v3_state + 624u, memory_args_5) == 0)
        return 0;
    *reinterpret_cast<std::uint32_t*>(xinn_v3_state + 840u) = 0u;
    std::uint8_t dynamic[120]{};
    auro_matic_v3_XinN_parameter_Dynamic_t_default(dynamic);
    auro_matic_XinN_fl32_set_dynamic_parameters(xinn_v3_state, dynamic);
    auro_matic_v3_XinN_Routing_fl32_set_routing(xinn_v3_state + 624u, dynamic + 48u);
    return reinterpret_cast<std::int64_t>(xinn_v3_state);
}

std::int64_t auro_matic_v3_XinN_fl32_initialize(
    std::uint8_t* xinn_v3_state,
    std::uint32_t input_mask,
    std::uint32_t output_mask,
    std::uint32_t sample_rate,
    std::uint64_t preset) {
    // This is the direct AM4HP call at native 0x54DD0F.  Its constructor
    // supplies 48000 explicitly, so do not broaden the portable contract to
    // rates that have no AM4HP differential coverage.
    if (!xinn_v3_state || sample_rate != 48000u || preset == 0u)
        return 1;
    if (!auro_matic_XinN_fl32_initialize(
            xinn_v3_state, input_mask, output_mask, preset))
        return 1;
    *reinterpret_cast<std::uint32_t*>(xinn_v3_state + 840u) = 1u;
    return 0;
}

std::int64_t auro_matic_v3_XinN_fl32_configure(
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
            || !auro_matic_XinN_fl32_initialize(xinn_v3_state, input_mask, output_mask, preset)) {
            return 1;
        }
    }
    if (dynamic != 0u) {
        const auto* dyn = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(dynamic));
        auro_matic_v3_XinN_fl32_set_dynamic_parameters(xinn_v3_state, dyn);
    }
    return 0;
}

void auro_matic_XinN_fl32_set_preset(std::uint8_t* xinn_state, std::uint64_t preset) {
    if (!xinn_state || preset == 0u)
        return;
    *reinterpret_cast<std::uint64_t*>(xinn_state + 8u) = preset;
    const auto* p = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(preset));
    const std::uint32_t early_blocks = *reinterpret_cast<const std::uint32_t*>(p + 8u) >> 5u;
    const std::uint32_t late_blocks = *reinterpret_cast<const std::uint32_t*>(p + 32u) >> 5u;
    const std::uint32_t shared_blocks = std::min(early_blocks, late_blocks);
    const auto* late_state = reinterpret_cast<const std::uint32_t*>(xinn_state + 392u);
    const std::uint32_t early_clear = late_state[0] != 0u ? shared_blocks : early_blocks;
    auro_matic_XinN_Early_fl32_set_total_clear_frames(xinn_state + 48u, early_clear);
    auro_matic_XinN_Late_fl32_set_clear_frames(
        reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
        early_clear,
        late_blocks - early_clear);
    auro_matic_XinN_Early_fl32_set_preset(xinn_state + 48u, preset);
    auro_matic_XinN_Late_fl32_set_preset(
        reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
        preset);
    xinn_apply_output_mask_4eb39c(
        xinn_state + 572u,
        *reinterpret_cast<const std::uint32_t*>(xinn_state + 0u),
        *reinterpret_cast<const std::uint32_t*>(xinn_state + 20u));
    auro_matic_XinN_Early_fl32_set_downmix(xinn_state + 48u, xinn_state + 608u);
    auro_matic_XinN_Early_fl32_set_output_patch(
        xinn_state + 48u,
        reinterpret_cast<const std::uint32_t*>(xinn_state + 572u),
        reinterpret_cast<const std::uint32_t*>(xinn_state + 584u));
    auro_matic_XinN_Late_fl32_set_output_patch(
        reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
        reinterpret_cast<const std::uint32_t*>(xinn_state + 596u));
}

std::int64_t auro_matic_XinN_fl32_reset_audio_state(std::uint8_t* xinn_state) {
    if (!xinn_state)
        return 0;
    auro_matic_XinN_Early_fl32_reset_audio_state(xinn_state + 48u);
    auro_matic_XinN_Late_fl32_reset_audio_state(reinterpret_cast<std::uint32_t*>(xinn_state + 392u));
    auro_audio_Smooth_fl32_inst_set_current(reinterpret_cast<float*>(xinn_state + 560u), 0.0f);
    auro_audio_Smooth_fl32_inst_update(reinterpret_cast<float*>(xinn_state + 560u), 1.0f);
    return 0;
}

std::int64_t auro_matic_XinN_fl32_partial_clear(
    std::uint8_t* xinn_state,
    std::uint32_t* remaining) {
    if (!xinn_state || !remaining)
        return 0;
    if (auro_matic_XinN_Early_fl32_partial_clear(xinn_state + 48u, remaining)
        && auro_matic_XinN_Late_fl32_partial_clear(
            reinterpret_cast<std::uint32_t*>(xinn_state + 392u),
            remaining)) {
        (void)auro_matic_XinN_Early_fl32_reset_audio_state(xinn_state + 48u);
        (void)auro_matic_XinN_Late_fl32_reset_audio_state(
            reinterpret_cast<std::uint32_t*>(xinn_state + 392u));
        auro_audio_Smooth_fl32_inst_set_current(reinterpret_cast<float*>(xinn_state + 560u), 0.0f);
        auro_audio_Smooth_fl32_inst_update(reinterpret_cast<float*>(xinn_state + 560u), 1.0f);
        return 1;
    }
    return 0;
}

void auro_matic_v3_XinN_fl32_set_preset(std::uint8_t* xinn_v3_state, std::uint64_t preset) {
    auro_matic_XinN_fl32_set_preset(xinn_v3_state, preset);
}

std::int64_t auro_matic_v3_XinN_fl32_reset_audio_state(std::uint8_t* xinn_v3_state) {
    if (!xinn_v3_state || *reinterpret_cast<const std::uint32_t*>(xinn_v3_state + 840u) == 0u)
        return 0;
    return auro_matic_XinN_fl32_reset_audio_state(xinn_v3_state);
}

std::uint64_t auro_matic_v3_XinN_fl32_set_dynamic_parameters(
    std::uint8_t* xinn_v3_state,
    const std::uint8_t* dynamic_120) {
    if (!xinn_v3_state || !dynamic_120)
        return 0u;
    auro_matic_XinN_fl32_set_dynamic_parameters(xinn_v3_state, dynamic_120);
    return auro_matic_v3_XinN_Routing_fl32_set_routing(xinn_v3_state + 624u, dynamic_120 + 48u);
}

std::uint64_t auro_matic_v3_XinN_fl32_get_dynamic_parameters(
    const std::uint8_t* xinn_v3_state,
    std::uint8_t* dynamic_120) {
    if (!xinn_v3_state || !dynamic_120)
        return 0u;
    auro_matic_XinN_fl32_get_dynamic_parameters(xinn_v3_state, dynamic_120);
    return auro_matic_v3_XinN_Routing_fl32_get_routing(xinn_v3_state + 624u, dynamic_120 + 48u);
}

void auro_matic_v3_XinN_fl32_update_peak_amplitude(
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

std::uint64_t /* Decompiled name: xinn_select_native_runtime_preset_from_v4_static */
xinn_select_native_runtime_preset_from_v4_static(
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

void /* Decompiled name: xinn_copy_runtime_preset_into_processed_object */
xinn_copy_runtime_preset_into_processed_object(
    std::uint8_t* processed_preset,
    std::uint64_t runtime_preset) {
    if (!processed_preset)
        return;
    const bool preset_pointer_available = /* Decompiled name: xinn_runtime_preset_pointer_available */
xinn_runtime_preset_pointer_available(runtime_preset);
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

void /* Decompiled name: xinn_retarget_processed_runtime_preset_backing */
xinn_retarget_processed_runtime_preset_backing(std::uint8_t* processed_preset) {
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
#if AURO3DENG_ENABLE_NATIVE_IMAGE_PRESET_POINTERS
        *reinterpret_cast<std::uint64_t*>(inline_preset + 40u + 8u * table) =
            src_u64 != 0u ? src_u64 : reinterpret_cast<std::uint64_t>(dst);
#else
        *reinterpret_cast<std::uint64_t*>(inline_preset + 40u + 8u * table) =
            reinterpret_cast<std::uint64_t>(dst);
#endif
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

bool /* Decompiled name: xinn_engine1_initialize_static_from_spreset */
xinn_engine1_initialize_static_from_spreset(std::uint8_t* processed_preset, std::uint64_t spreset_u64) {
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
    const std::uint8_t* portable_spreset = /* Decompiled name: xinn_portable_spreset_from_native_va */
xinn_portable_spreset_from_native_va(spreset_u64);
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
    /* Decompiled name: xinn_engine1_compute_type5_biquad */
xinn_engine1_compute_type5_biquad(
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

std::uint64_t /* Decompiled name: xinn_inline_runtime_preset_from_processed_object */
xinn_inline_runtime_preset_from_processed_object(const std::uint8_t* processed_preset) {
    if (!processed_preset || processed_preset[0] == 0u)
        return 0u;
    return reinterpret_cast<std::uint64_t>(processed_preset + 8u);
}

bool /* Decompiled name: xinn_processed_preset_has_engine1_static_rows */
xinn_processed_preset_has_engine1_static_rows(const std::uint8_t* processed_preset) {
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

void /* Decompiled name: xinn_retune_processed_preset */
xinn_retune_processed_preset(
    std::uint8_t* processed_preset,
    std::uint32_t mode,
    const std::uint8_t* update_blob) {
    if (!processed_preset || !update_blob || processed_preset[0] == 0u)
        return;
    if (processed_preset[1] == kXinnProcessedPresetMarker)
        return;
    const bool can_apply_engine1 = /* Decompiled name: xinn_processed_preset_has_engine1_static_rows */
xinn_processed_preset_has_engine1_static_rows(processed_preset);
    if (mode == 1u && *reinterpret_cast<const std::uint32_t*>(update_blob + 68u) != 0u) {
        if (can_apply_engine1) {
            /* Decompiled name: xinn_preset_tune_stereo */
xinn_preset_tune_stereo(
                processed_preset,
                reinterpret_cast<const float*>(update_blob + 72u));
        } else {
            /* Decompiled name: xinn_preset_tune_late_stereo */
xinn_preset_tune_late_stereo(
                processed_preset,
                reinterpret_cast<const float*>(update_blob + 72u));
        }
    } else if (mode == 2u && *reinterpret_cast<const std::uint32_t*>(update_blob + 160u) != 0u) {
        if (can_apply_engine1) {
            /* Decompiled name: xinn_preset_tune_surround */
xinn_preset_tune_surround(
                processed_preset,
                reinterpret_cast<const float*>(update_blob + 164u));
        } else {
            /* Decompiled name: xinn_preset_tune_late_surround */
xinn_preset_tune_late_surround(
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

    *reinterpret_cast<std::uint32_t*>(tuning_static + 0u) = surround_mode ? 1u : 0u;
    *reinterpret_cast<std::uint32_t*>(tuning_static + 4u) = std::min<std::uint32_t>(room_preset, 4u);
    // Artist Connection 1.21.31 PresetManager_configure returns these
    // routing selectors for its multichannel preset.  The v3 generic Dynamic
    // default is not the v4 PresetManager tuning: in particular, stereo HX_s
    // and Late group C are disabled.
    static constexpr std::uint32_t kStereoSelectors[6] = {
        1u, 2u, 0u, 1u, 2u, 0u,
    };
    static constexpr std::uint32_t kSurroundSelectors[9] = {
        0u, 2u, 3u, 0u, 0u, 3u, 0u, 2u, 3u,
    };
    const auto* selectors = surround_mode ? kSurroundSelectors : kStereoSelectors;
    std::memcpy(tuning_static + 8u, selectors, 3u * sizeof(std::uint32_t));
    std::memcpy(tuning_static + 20u, selectors + 3u, 3u * sizeof(std::uint32_t));
    if (surround_mode)
        std::memcpy(tuning_static + 32u, selectors + 6u, 3u * sizeof(std::uint32_t));
}

void xinn_fill_tuning_dynamic_defaults_portable(std::uint8_t* tuning_dynamic, bool surround_mode) {
    if (!tuning_dynamic)
        return;

    // Exact uint32 image returned by the same native PresetManager.  Keeping
    // this separate from auro_matic_v3_XinN_parameter_Dynamic_t_default is
    // intentional: the latter describes the lower-level v3 object only.
    static constexpr std::uint32_t kStereoWords[24] = {
        1u, 2u, 0u, 1u, 2u, 3u, 1u, 2u, 0u,
        0x3F800000u, 0x3F3504F3u, 0x3F800000u,
        0x3F800000u, 0x3F800000u, 0x3F800000u, 0x3F800000u,
        0u, 0u, 0x3F800000u, 0x3E40E2BAu,
        0x3F800000u, 0x3F353BEFu, 0u, 0x3F353BEFu,
    };
    static constexpr std::uint32_t kSurroundWords[28] = {
        0u, 2u, 3u, 0u, 0u, 3u, 0u, 2u, 3u,
        0x3F800000u, 0x3F3504F3u, 0x3F800000u,
        0x3F800000u, 0x3F800000u, 0x3F800000u, 0x3F800000u,
        0x3F800000u, 0x3F800000u, 0x3F800000u, 0x3E40E2BAu,
        0x3F800000u, 0u, 0u, 0u,
        0x3F353BEFu, 0u, 0x3F8F9E4Du, 0u,
    };
    std::memcpy(
        tuning_dynamic,
        surround_mode ? kSurroundWords : kStereoWords,
        surround_mode ? sizeof(kSurroundWords) : sizeof(kStereoWords));
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

void /* Decompiled name: xinn_fill_v3_dynamic_from_v4_static */
xinn_fill_v3_dynamic_from_v4_static(
    std::uint8_t* dynamic_120,
    bool surround_mode,
    const std::uint8_t* tuning_static) {
    if (!dynamic_120 || !tuning_static)
        return;

    auro_matic_v3_XinN_parameter_Dynamic_t_default(dynamic_120);
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

std::int64_t build_engine_params_for_upmix_plan(
    std::uint32_t* engine_params,
    std::uint32_t source_layout,
    std::uint32_t target_layout,
    std::uint32_t sample_rate,
    bool surround_mode,
    std::uint32_t room_preset,
    std::uint8_t* processed_preset_out,
    std::uint8_t* dynamic_parameters_out,
    std::uint32_t preset_family) {
    if (!engine_params)
        return 1;
    std::array<std::uint8_t, 48u> tuning_static{};
    xinn_fill_tuning_static_defaults_portable(
        tuning_static.data(), surround_mode, room_preset);
    const std::uint64_t spreset = xinn_select_native_spreset_from_v4_static(
        preset_family, surround_mode, tuning_static.data());
    if (spreset == 0u)
        return 1;
    static thread_local std::array<std::uint8_t, 23856u> tls_processed{};
    static thread_local std::array<std::uint8_t, 120u> tls_dynamic{};
    auto* processed_preset =
        processed_preset_out != nullptr ? processed_preset_out : tls_processed.data();
    auto* dynamic_parameters =
        dynamic_parameters_out != nullptr ? dynamic_parameters_out : tls_dynamic.data();
    if (!xinn_initialize_processed_preset_from_portable_spreset(
            processed_preset, spreset))
        return 1;
    xinn_retarget_processed_runtime_preset_backing(processed_preset);
    xinn_fill_v3_dynamic_from_v4_static(
        dynamic_parameters, surround_mode, tuning_static.data());
    if (surround_mode) {
        // PresetManager's surround Dynamic record replaces the generic v3
        // routing gains as well as its selectors.  In particular the rear
        // height Early-A gain is 0.70794576, not the generic 0.18836489.
        // The final two v3 words retain their constructor defaults.
        xinn_fill_tuning_dynamic_defaults_portable(
            dynamic_parameters, true);
    }
    // auro_matic_v3_Configurator_process @ 0x54F9A0 keeps the input layout
    // in word 0 and the actually produced layout in word 1.  For a stereo
    // source requesting C, its native FrontManager branch is mode 1: the
    // Centergen fixed/dynamic payload occupies words 3..21 and Output mode 1
    // copies the generated center from the FrontManager scratch lane.
    std::uint32_t produced_layout = source_layout;
    const bool generate_center =
        (source_layout & 0x3u) == 0x3u
        && (source_layout & 0x4u) == 0u
        && (target_layout & 0x4u) != 0u;
    // Configurator_process writes the two FrontManager output scales before
    // it chooses a center-generation branch (libauro.so.c:545945). Existing
    // front channels must therefore retain unity scaling as well.
    engine_params[24u] = 0x3F800000u;
    engine_params[25u] = 0x3F800000u;
    auto* front_config = reinterpret_cast<std::uint8_t*>(engine_params + 2u);
    // PresetManager supplies the Centergen fixed/dynamic payload even when
    // FrontManager mode 0 leaves center generation inactive.  Engine_configure
    // still validates and stores that payload for later reconfiguration.
    auto* center_fixed = front_config + 4u;
    std::memset(center_fixed, 0, 20u);
    *reinterpret_cast<std::uint64_t*>(center_fixed + 0u) = 1u;
    *reinterpret_cast<std::uint32_t*>(center_fixed + 16u) = 393216300u;
    auto* center_dynamic = front_config + 24u;
    std::memset(center_dynamic, 0, 56u);
    *reinterpret_cast<std::uint32_t*>(center_dynamic + 0u) = 1u;
    *reinterpret_cast<std::uint64_t*>(center_dynamic + 4u) =
        0x3F73333300000000ull;
    *reinterpret_cast<std::uint64_t*>(center_dynamic + 12u) =
        0xC14000003D4CCCCDull;
    *reinterpret_cast<std::uint64_t*>(center_dynamic + 20u) =
        0x00000000C1400000ull;
    *reinterpret_cast<std::uint64_t*>(center_dynamic + 28u) =
        0x3C23D70A3F800000ull;
    *reinterpret_cast<std::uint64_t*>(center_dynamic + 36u) =
        0x417000003E99999Aull;
    *reinterpret_cast<std::uint32_t*>(center_dynamic + 44u) =
        0x3F7D70A4u;
    if (preset_family == 4u) {
        // Native multichannel PresetManager uses room-specific Centergen
        // getters (0x4DB110..0x4DB290), not the SS/SB scene payload.
        // Small/Large share one record; Movie/Speech share another.
        const bool small_or_large = room_preset == 0u || room_preset == 2u;
        const std::uint32_t words[14] = {
            small_or_large ? 1u : 0u,
            0u, 0x3F733333u, 0x3D4CCCCDu, 0xC1100000u,
            room_preset >= 3u ? 0xC1400000u : 0xC0C00000u,
            0u, 0x3F800000u, 0x3CF5C28Fu, 0x3DCCCCCDu,
            0x41900000u, 0x3F7D70A4u,
            small_or_large ? 0xC0D00000u : 0xC0400000u, 0u
        };
        std::memcpy(center_dynamic, words, sizeof(words));
    }
    if (generate_center) {
        engine_params[2u] = 1u;
        // Family 4 leaves PresetManager +116 clear, so the native generated
        // center branch retains the processed channel planes.
        engine_params[26u] = preset_family == 4u ? 0u : 1u;
        produced_layout |= 0x4u;
    }

    const std::uint32_t xinn_additions =
        target_layout & ~produced_layout & (surround_mode ? 0x7E00u : 0x6630u);
    produced_layout |= xinn_additions;

    // Configurator_process then configures p3D3D from the channels still
    // missing after XinN.  Besides Top/HC, this stage can synthesize the
    // side pair from LS/RS, the rear-height pair from HL/HR, or BC from
    // LS/RS.  The masks and flags below mirror 0x54FC14..0x54FDC9; in
    // particular the side pair is not part of XinN's 0x6630 output mask.
    const std::uint32_t missing_after_xinn = target_layout & ~produced_layout;
    std::uint32_t p3d3d_additions = 0u;
    if ((missing_after_xinn & 0x1000u) != 0u) {
        if ((produced_layout & 0x600u) != 0x600u)
            return 1;
        engine_params[36u] = (produced_layout & 0x6000u) == 0x6000u ? 2u : 1u;
        p3d3d_additions |= 0x1000u;
    }
    if ((missing_after_xinn & 0x800u) != 0u) {
        if ((produced_layout & 0x600u) != 0x600u)
            return 1;
        engine_params[37u] = 1u;
        p3d3d_additions |= 0x800u;
    }
    if ((missing_after_xinn & 0x6000u) == 0x6000u) {
        if ((produced_layout & 0x600u) != 0x600u)
            return 1;
        engine_params[38u] = 1u;
        p3d3d_additions |= 0x6000u;
    }
    if ((missing_after_xinn & 0x180u) == 0x180u) {
        if ((produced_layout & 0x30u) != 0x30u)
            return 1;
        engine_params[39u] = 1u;
        p3d3d_additions |= 0x180u;
    }
    if ((missing_after_xinn & 0x40u) != 0u) {
        if ((produced_layout & 0x30u) != 0x30u
            || engine_params[39u] != 0u)
            return 1;
        engine_params[41u] = 1u;
        p3d3d_additions |= 0x40u;
    }
    produced_layout |= p3d3d_additions;

    engine_params[0u] = source_layout;
    engine_params[1u] = produced_layout;
    // XinN surround mode consumes the bed without C/LFE. FrontManager keeps
    // those channels in the shared planes while XinN receives the native
    // 0x1F3-filtered input descriptor (0x33 for 5.1, 0x1B3 for 7.1).
    engine_params[28u] = surround_mode ? (source_layout & 0x1F3u) : source_layout;
    engine_params[29u] = xinn_additions;
    const auto preset = xinn_inline_runtime_preset_from_processed_object(
        processed_preset);
    const auto dynamic = reinterpret_cast<std::uint64_t>(dynamic_parameters);
    engine_params[30u] = static_cast<std::uint32_t>(preset);
    engine_params[31u] = static_cast<std::uint32_t>(preset >> 32u);
    engine_params[32u] = static_cast<std::uint32_t>(dynamic);
    engine_params[33u] = static_cast<std::uint32_t>(dynamic >> 32u);
    // Words 46/47 are Silence/Gain masks. Configurator_process adds every
    // channel created by XinN or p3D3D to Gain (libauro.so.c:546120,546142).
    engine_params[46u] = 0u;
    engine_params[47u] = (xinn_additions | p3d3d_additions) & ~source_layout;
    engine_params[48u] = engine_params[1u] != source_layout ? 1u : 0u;
    // Configurator_process builds a rate-independent plan. The actual sample
    // rate is supplied later by Engine_configure to each active processor.
    (void)sample_rate;
    return 0;
}

std::int64_t build_engine_params_for_upmix_plan(
    std::uint32_t* engine_params,
    std::uint32_t source_layout,
    std::uint32_t target_layout,
    std::uint32_t sample_rate,
    bool surround_mode,
    std::uint32_t room_preset) {
    return build_engine_params_for_upmix_plan(
        engine_params,
        source_layout,
        target_layout,
        sample_rate,
        surround_mode,
        room_preset,
        nullptr,
        nullptr,
        4u);
}

std::int64_t build_engine_params_for_upmix_plan_family(
    std::uint32_t* engine_params,
    std::uint32_t source_layout,
    std::uint32_t target_layout,
    std::uint32_t sample_rate,
    bool surround_mode,
    std::uint32_t room_preset,
    std::uint32_t preset_family) {
    return build_engine_params_for_upmix_plan(
        engine_params,
        source_layout,
        target_layout,
        sample_rate,
        surround_mode,
        room_preset,
        nullptr,
        nullptr,
        preset_family);
}

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_initialize_impl(
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
    if (auro_matic_v3_XinN_fl32_construct(step + 40u, memory_args) == 0)
        return 1;
    *reinterpret_cast<std::uint64_t*>(step + 483952u) = memory_3[0] + 88u;
    return 0;
}

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_initialize(
    std::uint64_t step_base,
    const std::uint64_t* memory_3) {
    return auro_a3deng_v4_pipeline_step_upmix_XinN_initialize_impl(step_base, memory_3);
}

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_prepare(
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
        runtime_preset = /* Decompiled name: xinn_select_native_runtime_preset_from_v4_static */
xinn_select_native_runtime_preset_from_v4_static(false, plan_blob + 12u);
        spreset = /* Decompiled name: xinn_select_native_spreset_from_v4_static */
xinn_select_native_spreset_from_v4_static(false, plan_blob + 12u);
    } else if (mode == 2u) {
        if (plan_blob[92u] == 0u || *reinterpret_cast<const std::uint32_t*>(update_blob + 160u) != 1u)
            return 1;
        runtime_preset = /* Decompiled name: xinn_select_native_runtime_preset_from_v4_static */
xinn_select_native_runtime_preset_from_v4_static(true, plan_blob + 48u);
        spreset = /* Decompiled name: xinn_select_native_spreset_from_v4_static */
xinn_select_native_spreset_from_v4_static(true, plan_blob + 48u);
    }
    /* Decompiled name: xinn_copy_runtime_preset_into_processed_object */
xinn_copy_runtime_preset_into_processed_object(processed_preset, runtime_preset);
    if (processed_preset[0] == 0u)
        (void)/* Decompiled name: xinn_initialize_processed_preset_from_portable_spreset */
xinn_initialize_processed_preset_from_portable_spreset(processed_preset, spreset);
    if (processed_preset[0] == 0u)
        return 1;
    /* Decompiled name: xinn_retarget_processed_runtime_preset_backing */
xinn_retarget_processed_runtime_preset_backing(processed_preset);
    /* Decompiled name: xinn_engine2_initialize_static_from_spreset */
xinn_engine2_initialize_static_from_spreset(processed_preset, spreset);
    (void)/* Decompiled name: xinn_engine1_initialize_static_from_spreset */
xinn_engine1_initialize_static_from_spreset(processed_preset, spreset);
    /* Decompiled name: xinn_retune_processed_preset */
xinn_retune_processed_preset(processed_preset, mode, update_blob);
    // Native live object packs Dynamic at processed_preset+23736 (== step+483832).
    // The portable 23856-byte resource image overlaps that slot, so write
    // Dynamic after every preset image touch.
    if (mode == 1u) {
        /* Decompiled name: xinn_fill_v3_dynamic_from_v4_static */
xinn_fill_v3_dynamic_from_v4_static(dynamic_120, false, plan_blob + 12u);
    } else if (mode == 2u) {
        /* Decompiled name: xinn_fill_v3_dynamic_from_v4_static */
xinn_fill_v3_dynamic_from_v4_static(dynamic_120, true, plan_blob + 48u);
    } else {
        auro_matic_v3_XinN_parameter_Dynamic_t_default(dynamic_120);
    }
    const std::uint64_t preset = /* Decompiled name: xinn_inline_runtime_preset_from_processed_object */
xinn_inline_runtime_preset_from_processed_object(processed_preset);

    const auto* sample_rate_words = reinterpret_cast<const std::uint32_t*>(
        *reinterpret_cast<const std::uint64_t*>(step + 8u));
    const std::uint32_t sample_rate = sample_rate_words ? sample_rate_words[2] : 0u;
    const std::uint64_t config_args[3] = {
        static_cast<std::uint64_t>(input_mask) | (static_cast<std::uint64_t>(output_mask) << 32u),
        preset,
        reinterpret_cast<std::uint64_t>(dynamic_120),
    };
    if (auro_matic_v3_XinN_fl32_configure(step + 40u, sample_rate, config_args) != 0)
        return 1;
    return auro_a3deng_v4_pipeline_step_upmix_XinN_update_impl(step_base, update_blob);
}

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_reset_audio_state_impl(std::uint64_t step_base) {
    if (step_base == 0u)
        return 0;
    auto* step = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(step_base));
    return auro_matic_v3_XinN_fl32_reset_audio_state(step + 40u);
}

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_reset_audio_state(std::uint64_t step_base) {
    return auro_a3deng_v4_pipeline_step_upmix_XinN_reset_audio_state_impl(step_base);
}

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_update_impl(
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

    /* Decompiled name: xinn_retune_processed_preset */
xinn_retune_processed_preset(step + 460096u, mode, update_blob);
    const std::uint64_t preset = /* Decompiled name: xinn_inline_runtime_preset_from_processed_object */
xinn_inline_runtime_preset_from_processed_object(step + 460096u);
    auro_matic_v3_XinN_fl32_set_preset(step + 40u, preset);
    auro_matic_v3_XinN_fl32_set_dynamic_parameters(step + 40u, step + 483832u);
    return 0;
}

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_update(
    std::uint64_t step_base,
    const std::uint8_t* update_blob) {
    return auro_a3deng_v4_pipeline_step_upmix_XinN_update_impl(step_base, update_blob);
}

std::int64_t AuromaticV3UpmixRuntime::configure_via_manager(
    std::uint32_t sample_rate,
    std::uint32_t input_mask,
    std::uint32_t output_mask,
    bool surround_mode,
    std::uint32_t room_preset,
    std::uint32_t preset_family) {
    if (!constructed_ || sample_rate == 0u)
        return 1;
    if (block_state_.size() < 12u)
        return 1;
    *reinterpret_cast<std::uint32_t*>(block_state_.data() + 8u) = sample_rate;
    *reinterpret_cast<std::uint32_t*>(block_state_.data() + 12u) = 0u;
    auto* manager = manager_.data();
    // Build the configure blob out-of-place. Manager_configure compares the
    // live manager params against this blob to decide update_type; writing the
    // new values into manager+8 first collapses that to a no-op.
    std::array<std::uint8_t, ::auro_engine_v4_ida::kManager_params_bytes> cfg{};
    auto* params = reinterpret_cast<std::uint32_t*>(cfg.data());
    // Manager params[1]/[6] are 32-sample subblock counts (native process
    // quantum is at most 0x80 samples / 4 subblocks). The owning public API
    // feeds one subblock per process() call.
    constexpr std::uint32_t kSubblockCount = 1u;
    params[0] = input_mask;
    params[1] = kSubblockCount;
    params[2] = sample_rate;
    params[5] = output_mask;
    params[6] = kSubblockCount;
    params[7] = sample_rate;
    // plan_MetaInfo_t_construct exposes config+44 as MetaInfo+20 (native
    // PresetManager room/a3) and config+48 as MetaInfo+24 (native preset kind,
    // not the portable five-entry family index).  Native uses kind 1 for
    // soundbar, 2 for SS, and 0/3 for multichannel; passing the portable
    // family index here changes the PresetManager branch (notably family 0
    // SS would otherwise be interpreted as multichannel).
    params[11] = std::min<std::uint32_t>(room_preset, 4u);
    params[12] = preset_family == 0u
        ? 2u
        : (preset_family <= 3u ? 1u : 3u);
    params[15] = surround_mode ? 6u : 0u;
    params[25] = output_mask;
    *reinterpret_cast<std::uint64_t*>(manager + 0u) =
        reinterpret_cast<std::uint64_t>(manager + ::auro_engine_v4_ida::kManager_off_params);
    *reinterpret_cast<std::uint32_t*>(manager + 24u) = sample_rate;
    *reinterpret_cast<std::uint32_t*>(manager + 28u) = output_mask;
    const auto rc = auro_a3deng_v3_pipeline_Manager_configure(
        reinterpret_cast<std::uint32_t*>(manager),
        cfg.data(),
        1);
    if (rc != 0)
        return rc;
    // Activation leaves step+32 linked to the manager params record (native
    // predecessor metadata). Re-bind the owning one-subblock block_state so
    // the public process() API still sees subblock_count=1 at +4.
    if (auro_a3deng_v3_pipeline_Manager_bind_upmix_block_state(
            reinterpret_cast<std::uint32_t*>(manager),
            reinterpret_cast<std::uint64_t>(block_state_.data())) != 0)
        return 2;
    auto* upmix = step_chain_.data() + 7944u;
    if (*reinterpret_cast<const std::uint32_t*>(upmix + 40u) == 0u
        && *reinterpret_cast<const std::uint32_t*>(upmix + 68u) == 0u)
        return 3;
    // Re-bind XinN preset/dynamic pointers into runtime-owned storage. The plan
    // path configures Engine with thread_local backing that is valid for the
    // configure call; process needs member lifetime matching the owning runtime.
    processed_preset_.assign(23856u, 0u);
    dynamic_parameters_.assign(120u, 0u);
    engine_parameters_.assign(64u, 0u);
    if (build_engine_params_for_upmix_plan(
            engine_parameters_.data(),
            input_mask,
            output_mask,
            sample_rate,
            surround_mode,
            room_preset,
            processed_preset_.data(),
            dynamic_parameters_.data(),
            preset_family) != 0)
        return 1;
    *reinterpret_cast<std::uint32_t*>(block_state_.data() + 12u) = 0u;
    *reinterpret_cast<std::uint32_t*>(upmix + 52u) = 0u;
    return configure(sample_rate, engine_parameters_.data());
}

std::int64_t AuromaticV3UpmixRuntime::configure_default(
    std::uint32_t sample_rate,
    std::uint32_t input_mask,
    std::uint32_t output_mask,
    bool surround_mode,
    std::uint32_t room_preset,
    std::uint32_t preset_family) {
    if (!constructed_ || sample_rate == 0u)
        return 1;
    // This owning runtime represents the discrete native Upmix object, not
    // the unrelated converter/downmix steps in the full Manager plan. Keep
    // its preset and Dynamic backing alive for the complete process lifetime
    // and configure the Engine through the same 0xC8-byte plan record.
    processed_preset_.assign(23856u, 0u);
    dynamic_parameters_.assign(120u, 0u);
    engine_parameters_.assign(64u, 0u);
    if (build_engine_params_for_upmix_plan(
            engine_parameters_.data(), input_mask, output_mask, sample_rate,
            surround_mode, room_preset, processed_preset_.data(),
            dynamic_parameters_.data(), preset_family) != 0)
        return 1;
    return configure(sample_rate, engine_parameters_.data());
}

bool auromatic_runtime_configure_self_test(std::string& detail) {
    std::array<std::uint8_t, 128u> plan_config{};
    *reinterpret_cast<std::uint32_t*>(plan_config.data() + 8u) = 96000u;
    *reinterpret_cast<std::uint32_t*>(plan_config.data() + 28u) = 48000u;
    *reinterpret_cast<std::uint32_t*>(plan_config.data() + 60u) = 7u;
    *reinterpret_cast<std::uint32_t*>(plan_config.data() + 100u) = 11u;
    std::array<std::uint8_t, 88u> meta_info{};
    if (auro_a3deng_v3_pipeline_plan_MetaInfo_t_construct(
            meta_info.data(), plan_config.data()) != 0
        || *reinterpret_cast<const std::uint64_t*>(meta_info.data())
            != reinterpret_cast<std::uint64_t>(plan_config.data())
        || *reinterpret_cast<const std::uint64_t*>(meta_info.data() + 8u)
            != reinterpret_cast<std::uint64_t>(plan_config.data() + 20u)
        || *reinterpret_cast<const std::uint32_t*>(meta_info.data() + 40u) != 7u
        || *reinterpret_cast<const std::uint32_t*>(meta_info.data() + 60u) != 11u
        || *reinterpret_cast<const std::uint32_t*>(meta_info.data() + 84u) != 48000u) {
        detail = "Plan MetaInfo construction mismatch";
        return false;
    }
    std::array<std::uint8_t, 248u> remix_configurator{};
    std::array<std::uint8_t, 24u> remix_config{};
    *reinterpret_cast<std::uint64_t*>(remix_config.data() + 0u) = 0x1122334455667788ull;
    *reinterpret_cast<std::uint32_t*>(remix_config.data() + 8u) = 9u;
    *reinterpret_cast<std::uint64_t*>(remix_config.data() + 16u) = 0x8877665544332211ull;
    if (auro_remix_v1_Configurator_t_construct(remix_configurator.data()) == 0
        || *reinterpret_cast<const std::uint64_t*>(remix_configurator.data())
            != reinterpret_cast<std::uint64_t>(remix_configurator.data())) {
        detail = "Remix Configurator construction mismatch";
        return false;
    }
    auro_remix_v1_Configurator_set_listening_mode(
        remix_configurator.data(), 6u);
    auro_remix_v1_Configurator_configure_alt_3D(
        remix_configurator.data(), remix_config.data());
    auro_remix_v1_Configurator_configure_bass_management(
        remix_configurator.data(), remix_config.data());
    if (*reinterpret_cast<const std::uint32_t*>(remix_configurator.data() + 16u) != 6u
        || *reinterpret_cast<const std::uint64_t*>(remix_configurator.data() + 200u)
            != 0x1122334455667788ull
        || *reinterpret_cast<const std::uint32_t*>(remix_configurator.data() + 208u) != 9u
        || *reinterpret_cast<const std::uint64_t*>(remix_configurator.data() + 228u)
            != 0x8877665544332211ull) {
        detail = "Remix Configurator field layout mismatch";
        return false;
    }
    std::array<std::uint32_t, 3u> legacy_plan{};
    if (auro_remix_v1_plan_Legacy_t_construct(
            legacy_plan.data(), 0x40u) != 0
        || legacy_plan[0u] != 0x40u
        || legacy_plan[1u] != 0x30u
        || legacy_plan[2u] != 1u
        || !auro_remix_v1_plan_Legacy_active(legacy_plan.data())
        || auro_remix_v1_plan_LegacyFlags_remap_layout(
            legacy_plan.data() + 2u, 0x7FBFu) != 0x7FBFu) {
        detail = "Legacy plan layout mismatch";
        return false;
    }
    std::uint32_t projected_layout = 0u;
    if (!auro_remix_v1_project_output_layout(
            &projected_layout, 0x7FBFu, 0u, 1u, 0u)
        || projected_layout != 0x7FBFu
        || auro_remix_v1_project_output_layout(
            &projected_layout, 0x7FBFu, 0u, 4u, 0u)) {
        detail = "Remixer output-layout projection mismatch";
        return false;
    }
    std::array<std::uint8_t, 40u> bass_state{};
    std::array<std::uint8_t, 24u> bass_config{};
    *reinterpret_cast<std::uint32_t*>(bass_config.data()) = 1u;
    *reinterpret_cast<std::uint32_t*>(bass_config.data() + 4u) = 1u;
    std::uint32_t bass_layout = 0u;
    std::uint32_t bass_active = 0u;
    if (auro_remix_v1_Configurator_plan_bass_management(
            bass_state.data(), &bass_layout, &bass_active, 0u, 8u,
            bass_config.data(), 2u) != 0
        || bass_layout != 8u || bass_active != 1u
        || *reinterpret_cast<const std::uint32_t*>(bass_state.data() + 12u)
            != 1065353216u) {
        detail = "Bass-management plan layout mismatch";
        return false;
    }
    std::array<std::uint8_t, 16u> alt3d_config{};
    *reinterpret_cast<std::uint64_t*>(alt3d_config.data() + 4u) =
        0xAABBCCDDEEFF0011ull;
    std::array<std::uint8_t, 16u> alt3d_plan{};
    std::uint32_t alt3d_layout = 0u;
    std::uint32_t alt3d_active = 0u;
    if (auro_remix_v1_Configurator_plan_alt_3D_mix(
            8u, alt3d_plan.data(), &alt3d_layout, &alt3d_active,
            8u, 8u, alt3d_config.data()) != 0
        || *reinterpret_cast<const std::uint32_t*>(alt3d_plan.data()) != 8u
        || *reinterpret_cast<const std::uint64_t*>(alt3d_plan.data() + 4u)
            != 0xAABBCCDDEEFF0011ull
        || alt3d_layout != 8u || alt3d_active != 1u) {
        detail = "Alt-3D mix plan layout mismatch";
        return false;
    }
    *reinterpret_cast<std::uint32_t*>(plan_config.data() + 12u) = 3u;
    *reinterpret_cast<std::uint32_t*>(plan_config.data() + 32u) = 5u;
    std::array<std::uint8_t, 2600u> plan{};
    if (auro_a3deng_v3_pipeline_Plan_configure_sample_convertors(
            plan.data(), meta_info.data(), 4u) != 0
        || *reinterpret_cast<const std::uint32_t*>(plan.data()) != 1u
        || *reinterpret_cast<const std::uint32_t*>(plan.data() + 4u) != 3u
        || *reinterpret_cast<const std::uint32_t*>(plan.data() + 2420u) != 1u
        || *reinterpret_cast<const std::uint32_t*>(plan.data() + 2428u) != 5u
        || *reinterpret_cast<const std::uint32_t*>(plan.data() + 2432u) != 4u
        || auro_a3deng_v3_pipeline_Plan_configure_resamplers(
            plan.data(), meta_info.data()) != 0
        || *reinterpret_cast<const std::uint64_t*>(plan.data() + 16u) != 1u
        || *reinterpret_cast<const std::uint32_t*>(plan.data() + 2404u) != 0u) {
        detail = "current Plan conversion/rate configuration mismatch";
        return false;
    }
    plan.fill(0u);
    if (auro_a3deng_v3_pipeline_Plan_configure_sample_convertors(
            plan.data(), meta_info.data(), 4u, AuromaticV3PlanAbi::Libauro1) != 0
        || *reinterpret_cast<const std::uint32_t*>(plan.data() + 2556u) != 1u
        || *reinterpret_cast<const std::uint32_t*>(plan.data() + 2564u) != 5u
        || *reinterpret_cast<const std::uint32_t*>(plan.data() + 2568u) != 4u
        || auro_a3deng_v3_pipeline_Plan_configure_resamplers(
            plan.data(), meta_info.data(), AuromaticV3PlanAbi::Libauro1) != 0
        || *reinterpret_cast<const std::uint64_t*>(plan.data() + 16u) != 1u
        || *reinterpret_cast<const std::uint32_t*>(plan.data() + 2540u) != 0u) {
        detail = "libauro(1) Plan conversion/rate configuration mismatch";
        return false;
    }
    auto check_resampler_rates = [&](std::uint32_t input_rate,
                                     std::uint32_t core_rate,
                                     std::uint32_t output_rate,
                                     std::uint64_t expected_down,
                                     std::uint32_t expected_up) {
        *reinterpret_cast<std::uint32_t*>(plan_config.data() + 8u) = input_rate;
        *reinterpret_cast<std::uint32_t*>(plan_config.data() + 28u) = output_rate;
        *reinterpret_cast<std::uint32_t*>(meta_info.data() + 84u) = core_rate;
        plan.fill(0u);
        return auro_a3deng_v3_pipeline_Plan_configure_resamplers(
                   plan.data(), meta_info.data()) == 0
            && *reinterpret_cast<const std::uint64_t*>(plan.data() + 16u)
                == expected_down
            && *reinterpret_cast<const std::uint32_t*>(plan.data() + 2404u)
                == expected_up;
    };
    if (!check_resampler_rates(48000u, 48000u, 48000u, 0u, 0u)
        || !check_resampler_rates(88200u, 44100u, 88200u, 1u, 1u)
        || !check_resampler_rates(96000u, 48000u, 96000u, 1u, 1u)) {
        detail = "required-rate input/core/output resampler plan mismatch";
        return false;
    }
    // Restore the original fixture fields used by the remaining checks.
    *reinterpret_cast<std::uint32_t*>(plan_config.data() + 8u) = 96000u;
    *reinterpret_cast<std::uint32_t*>(plan_config.data() + 28u) = 48000u;
    *reinterpret_cast<std::uint32_t*>(meta_info.data() + 84u) = 48000u;
    plan.fill(0u);
    if (auro_a3deng_v3_pipeline_Plan_configure_sample_convertors(
            plan.data(), meta_info.data(), 4u, AuromaticV3PlanAbi::Libauro1) != 0
        || auro_a3deng_v3_pipeline_Plan_configure_resamplers(
            plan.data(), meta_info.data(), AuromaticV3PlanAbi::Libauro1) != 0) {
        detail = "failed to restore plan fixture after required-rate matrix";
        return false;
    }
    std::array<std::int32_t, 0x280u / sizeof(std::int32_t)> downmix_plan{};
    std::uint32_t downmix_output = 0u;
    if (auro_matic_v3_Plan_calculate_downmix(
            downmix_plan.data(), 0x7FBFu, 0x003Fu, &downmix_output) != 0
        || downmix_output == 0u
        || *reinterpret_cast<const std::uint64_t*>(downmix_plan.data()) == 0u) {
        detail = "shared Auro-Matic downmix plan calculation failed";
        return false;
    }
    std::array<std::int32_t, 0x280u / sizeof(std::int32_t)> indexed_plan{};
    std::array<std::int32_t, 0x280u / sizeof(std::int32_t)> expanded_plan{};
    std::array<float, 1u> indexed_gains{0.5f};
    *reinterpret_cast<std::uint64_t*>(indexed_plan.data()) = 1u;
    indexed_plan[2u] = 0;
    indexed_plan[3u] = 1;
    indexed_plan[4u] = 0;
    indexed_plan[158u] = 0x3;
    indexed_plan[159u] = 0x3;
    if (!auro_matic_v3_downmix_plan_construct(
            indexed_plan.data(), expanded_plan.data(), indexed_gains.data())
        || *reinterpret_cast<const std::uint64_t*>(expanded_plan.data()) != 3u
        || expanded_plan[2u] != 0 || expanded_plan[3u] != 0
        || *reinterpret_cast<const float*>(expanded_plan.data() + 4u) != 1.0f
        || expanded_plan[5u] != 0 || expanded_plan[6u] != 1
        || *reinterpret_cast<const float*>(expanded_plan.data() + 7u) != 0.5f
        || expanded_plan[8u] != 1 || expanded_plan[9u] != 1
        || *reinterpret_cast<const float*>(expanded_plan.data() + 10u) != 1.0f) {
        detail = "indexed Auro-Matic downmix plan expansion mismatch";
        return false;
    }
    std::array<std::uint8_t, 648u> remixer_downmix{};
    std::uint32_t remixer_downmix_changed = 0u;
    if (auro_remix_v1_Configurator_compute_downmix_plan(
            remixer_downmix.data(), 0x7FBFu, 0x003Fu,
            &remixer_downmix_changed) != 0
        || *reinterpret_cast<const std::uint32_t*>(remixer_downmix.data()) != 0x7FBFu
        || *reinterpret_cast<const std::uint32_t*>(remixer_downmix.data() + 4u) == 0u
        || *reinterpret_cast<const std::uint64_t*>(remixer_downmix.data() + 8u) == 0u
        || remixer_downmix_changed == 0u) {
        detail = "default-gain Auro-Matic downmix plan mismatch";
        return false;
    }
    std::array<std::uint8_t, 56u> alt_store_engine{};
    std::array<std::uint8_t, 664u> alt_store_plan{};
    std::array<std::uint8_t, 4u> alt_store_config{1u, 0u, 0u, 0u};
    std::uint32_t alt_store_layout = 0u;
    std::uint32_t alt_store_active = 0u;
    if (auro_remix_v1_Configurator_plan_alt_3D_store(
            alt_store_engine.data(), alt_store_plan.data(), 0x7FBFu, 0x20000u,
            alt_store_config.data(), &alt_store_layout, &alt_store_active) != 0
        || *reinterpret_cast<const std::uint32_t*>(alt_store_plan.data()) != 0x7FBFu
        || *reinterpret_cast<const std::uint32_t*>(alt_store_plan.data() + 16u) != 0x7FBFu
        || *reinterpret_cast<const std::uint64_t*>(alt_store_plan.data() + 24u) == 0u
        || alt_store_layout == 0u || alt_store_active == 0u) {
        detail = "Alt-3D store plan mismatch";
        return false;
    }
    AuromaticV3UpmixRuntime runtime;
    if (runtime.construct() != 0) {
        detail = "owning Upmix construct failed";
        return false;
    }
    const auto* upmix_step = runtime.step_chain_storage() + 7944u;
    if (*reinterpret_cast<const std::uint64_t*>(upmix_step + 0u)
            != ::auro_codec_v3_ida::kAuroA3dengV3PipelineStepUpmix_configure
        || *reinterpret_cast<const std::uint64_t*>(upmix_step + 8u)
            != ::auro_codec_v3_ida::kAuroA3dengV3PipelineStepUpmix_update
        || *reinterpret_cast<const std::uint64_t*>(upmix_step + 16u)
            != ::auro_codec_v3_ida::kAuroA3dengV3PipelineStepUpmix_process
        || *reinterpret_cast<const std::uint64_t*>(upmix_step + 24u)
            != ::auro_codec_v3_ida::kAuroA3dengV3PipelineStepUpmix_reset) {
        detail = "Upmix lifecycle callback table mismatch";
        return false;
    }
    if (runtime.reset_audio_state() != 0) {
        detail = "owning Upmix reset failed";
        return false;
    }
    if (runtime.configure_default(48000u, 0x3u, 0x663Fu, false, 0u) != 0) {
        detail = "default Engine configure failed";
        return false;
    }
    // The native PresetManager family is part of the owning runtime contract;
    // exercise every SS/SB/multichannel family, both XinN variants, and all
    // five native room slots through the same Manager activation path.
    for (std::uint32_t family = 0u; family < 5u; ++family) {
        for (std::uint32_t surround = 0u; surround < 2u; ++surround) {
            for (std::uint32_t room = 0u; room < 5u; ++room) {
                if (runtime.configure_default(
                        48000u, 0x3u, 0x663Fu, surround != 0u, room, family) != 0) {
                    detail = "owning runtime preset family/room configure failed family="
                        + std::to_string(family) + " surround="
                        + std::to_string(surround) + " room="
                        + std::to_string(room);
                    return false;
                }
            }
        }
    }
    if (runtime.configure_default(48000u, 0x3u, 0x663Fu, false, 0u, 4u) != 0) {
        detail = "owning runtime default reconfigure failed";
        return false;
    }
    auto* runtime_upmix_step = runtime.step_chain_storage() + 7944u;
    if (runtime.set_bypass(1) != 0
        || *reinterpret_cast<const std::uint32_t*>(runtime_upmix_step + 5124u) != 1u
        || runtime.set_bypass(0) != 0
        || *reinterpret_cast<const std::uint32_t*>(runtime_upmix_step + 5124u) != 0u) {
        detail = "Upmix step bypass field mismatch";
        return false;
    }
    std::array<std::uint8_t, 120u> dynamic_120{};
    std::array<std::uint8_t, 56u> dynamic_56{};
    if (runtime.update_xinn_parameters(dynamic_120.data()) != 0
        || runtime.update_front_manager(dynamic_56.data()) != 0
        || runtime.configure_default(48000u, 0x3u, 0x663Fu, false, 0u) != 0) {
        detail = "Engine dynamic update callback failed";
        return false;
    }
    std::array<float, ::auro_engine_v4_ida::kChannelCount> peaks{};
    peaks[0u] = 1.0f;
    peaks[1u] = 0.5f;
    if (runtime.update_peak_amplitude(peaks.data()) != 0) {
        detail = "Upmix peak update failed";
        return false;
    }
    std::array<std::array<float, 32u>, ::auro_engine_v4_ida::kChannelCount> direct_planes{};
    std::array<float*, ::auro_engine_v4_ida::kChannelCount> direct_channels{};
    for (std::size_t i = 0u; i < direct_channels.size(); ++i)
        direct_channels[i] = direct_planes[i].data();
    direct_planes[0u].fill(0.25f);
    direct_planes[1u].fill(0.125f);
    auto* direct_engine = runtime.step_chain_storage() + 7944u + 264u;
    AuromaticV3EngineObjectParts direct_parts{};
    direct_parts.output_gate = direct_engine;
    direct_parts.front_manager = direct_engine + 2152u;
    direct_parts.xinn = direct_engine + 3344u;
    direct_parts.p3d3d = direct_engine + 4192u;
    direct_parts.gain = direct_engine + 4592u;
    direct_parts.silence = direct_engine + 4720u;
    direct_parts.output_masks = reinterpret_cast<std::uint32_t*>(direct_engine + 4848u);
    {
        std::array<float, 32u> front_left{};
        std::array<float, 32u> front_right{};
        std::array<float, 32u> front_center{};
        std::array<void*, ::auro_engine_v4_ida::kChannelCount> front_input{};
        front_input[0u] = direct_planes[0u].data();
        front_input[1u] = direct_planes[1u].data();
        front_input[2u] = direct_planes[2u].data();
        for (std::uint32_t block = 0u; block < 256u; ++block) {
            direct_planes[0u].fill(0.25f);
            direct_planes[1u].fill(0.125f);
            if (auro_matic_v3_FrontManager_Input_process(
                    direct_parts.front_manager, front_input.data(),
                    front_left.data(), front_right.data(), front_center.data()) != 0) {
                detail = "configured FrontManager input process failed";
                return false;
            }
        }
        if (std::all_of(direct_planes[2u].begin(), direct_planes[2u].end(),
                [](float sample) { return sample == 0.0f; })) {
            detail = "configured Centergen produced a silent center mode="
                + std::to_string(*reinterpret_cast<const std::uint32_t*>(
                    direct_parts.front_manager + 84u))
                + " callback=" + std::to_string(
                    *reinterpret_cast<const std::uint64_t*>(
                        direct_parts.front_manager + 1152u))
                + " config_mode=" + std::to_string(
                    *reinterpret_cast<const std::uint32_t*>(
                        runtime.step_chain_storage() + 7944u + 64u + 8u))
                + " center_gain=" + std::to_string(
                    *reinterpret_cast<const float*>(
                        direct_parts.front_manager + 192u));
            return false;
        }
        (void)auro_centergen_v3_Processor_reset_audio_state(direct_parts.front_manager);
    }
    // Keep a non-zero front pair here. Native Engine_process consumes exactly
    // one 32-float lane per channel; a zero-only smoke test does not exercise
    // XinN's delay, preset, or routing storage.
    AuromaticV3UpmixRuntime direct_runtime;
    if (direct_runtime.construct() != 0
        || direct_runtime.configure_default(48000u, 0x3u, 0x663Fu, false, 0u) != 0) {
        detail = "fresh direct runtime setup failed";
        return false;
    }
    direct_engine = direct_runtime.step_chain_storage() + 7944u + 264u;
    direct_parts.output_gate = direct_engine;
    direct_parts.front_manager = direct_engine + 2152u;
    direct_parts.xinn = direct_engine + 3344u;
    direct_parts.p3d3d = direct_engine + 4192u;
    direct_parts.gain = direct_engine + 4592u;
    direct_parts.silence = direct_engine + 4720u;
    direct_parts.output_masks = reinterpret_cast<std::uint32_t*>(direct_engine + 4848u);
    // Upmix::process writes its current bypass decision before every Engine
    // call.  OutputGate_configure starts this field set, so a direct Engine
    // equivalence call must explicitly select the active (non-bypass) path.
    auro_matic_v3_OutputGate_set_bypass(direct_engine, 0);
    direct_planes[0u].fill(0.25f);
    direct_planes[1u].fill(0.125f);
    const auto direct_rc = auro_matic_v3_Engine_process_objects(
        direct_parts, direct_channels.data());
    if (direct_rc != 0) {
        detail = "owning Engine process failed rc=" + std::to_string(direct_rc);
        return false;
    }
    AuromaticV3UpmixRuntime manager_runtime;
    if (manager_runtime.construct() != 0
        || manager_runtime.configure_default(48000u, 0x3u, 0x663Fu, false, 0u) != 0) {
        detail = "second owning runtime setup failed";
        return false;
    }
    std::array<std::array<float, 32u>, ::auro_engine_v4_ida::kChannelCount> manager_planes{};
    std::array<void*, ::auro_engine_v4_ida::kChannelCount> manager_channels{};
    manager_planes[0u].fill(0.25f);
    manager_planes[1u].fill(0.125f);
    for (std::size_t i = 0u; i < manager_channels.size(); ++i)
        manager_channels[i] = manager_planes[i].data();
    // Compare Engine_process with the native Upmix outer adapter only.  A
    // complete Manager run additionally executes configured downstream
    // BassManagement/Headroom/Gate/Limiter steps, so it is intentionally not
    // an Engine PCM-equivalence boundary.
    const auto manager_rc = manager_process_upmix_outer(
        manager_runtime.step_chain_storage() + 7944u, manager_channels.data());
    if (manager_rc != 0) {
        detail = "Manager-facing Upmix outer process failed rc="
            + std::to_string(manager_rc);
        return false;
    }
    float max_abs_difference = 0.0f;
    std::size_t worst_channel = 0u;
    std::size_t worst_sample = 0u;
    for (std::size_t channel = 0u; channel < direct_planes.size(); ++channel) {
        for (std::size_t sample = 0u; sample < direct_planes[channel].size(); ++sample) {
            const float direct = direct_planes[channel][sample];
            const float managed = manager_planes[channel][sample];
            if (!std::isfinite(direct) || !std::isfinite(managed)) {
                detail = "direct Engine or Manager Upmix produced non-finite PCM";
                return false;
            }
            const float difference = std::fabs(direct - managed);
            if (difference > max_abs_difference) {
                max_abs_difference = difference;
                worst_channel = channel;
                worst_sample = sample;
            }
        }
    }
    if (max_abs_difference > 1.0e-6f) {
        detail = "direct Engine and Manager Upmix outer exceed PCM tolerance max_abs="
            + std::to_string(max_abs_difference)
            + " channel=" + std::to_string(worst_channel)
            + " sample=" + std::to_string(worst_sample)
            + " direct=" + std::to_string(direct_planes[worst_channel][worst_sample])
            + " manager=" + std::to_string(manager_planes[worst_channel][worst_sample]);
        return false;
    }
    // Exercise the owning Manager path for every native PresetManager family,
    // stereo/surround variant, and room slot.  The plan-only matrix above
    // cannot catch a bad processed-preset resource binding or an activation
    // failure in the actual Manager/Upmix object graph.
    for (std::uint32_t family = 0u; family < 5u; ++family) {
        for (std::uint32_t surround = 0u; surround < 2u; ++surround) {
            for (std::uint32_t room = 0u; room < 5u; ++room) {
                AuromaticV3UpmixRuntime matrix_runtime;
                if (matrix_runtime.construct() != 0
                    || matrix_runtime.configure_default(
                        48000u, 0x3u, 0x663Fu, surround != 0u, room, family) != 0) {
                    detail = "PresetManager runtime matrix failed family="
                        + std::to_string(family) + " surround="
                        + std::to_string(surround) + " room="
                        + std::to_string(room);
                    return false;
                }
                std::array<std::array<float, 32u>, 31u> matrix_planes{};
                std::array<float*, 31u> matrix_ptrs{};
                for (std::uint32_t channel = 0u; channel < 31u; ++channel)
                    matrix_ptrs[channel] = matrix_planes[channel].data();
                matrix_planes[0u][0u] = 0.25f;
                matrix_planes[1u][0u] = -0.125f;
                if (matrix_runtime.process(matrix_ptrs.data()) != 0) {
                    detail = "PresetManager runtime matrix process failed family="
                        + std::to_string(family) + " surround="
                        + std::to_string(surround) + " room="
                        + std::to_string(room);
                    return false;
                }
                for (const auto& plane : matrix_planes) {
                    for (const float sample : plane) {
                        if (!std::isfinite(sample)) {
                            detail = "PresetManager runtime matrix produced non-finite PCM family="
                                + std::to_string(family) + " surround="
                                + std::to_string(surround) + " room="
                                + std::to_string(room);
                            return false;
                        }
                    }
                }
            }
        }
    }
    detail = "owning Upmix construct/default Engine configure PASS";
    return true;
}

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_calculate_info(
    std::uint64_t step_base,
    std::uint8_t* info_base) {
    if (step_base != 0u && info_base != nullptr) {
        auto* step = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(step_base));
        auro_matic_v3_XinN_fl32_update_peak_amplitude(
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
        auro_matic_XinN_Early_fl32_process_mode2(
            early,
            channel_span_31,
            early_a,
            early_b,
            gains);
        return;
    }
    if (process_u64 == 0x5773E0u) {
        auro_matic_XinN_Early_fl32_process_mode1(
            early,
            channel_span_31,
            early_a,
            gains);
        return;
    }
    if (fallback_mode == 2u) {
        auro_matic_XinN_Early_fl32_process_mode2(
            early,
            channel_span_31,
            early_a,
            early_b,
            gains);
    } else {
        auro_matic_XinN_Early_fl32_process_mode1(
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
    auro_audio_Smooth_fl32_inst_gains_smooth(
        reinterpret_cast<float*>(xinn + 560u),
        gains,
        32u);

    const std::uint32_t mode = e2_read_u32(xinn, 0u);
    auto* early = xinn + 48u;
    xinn_early_process_dispatch_574e30(early, channel_span_31, early_a, early_b, gains, mode);

    (void)auro_matic_XinN_Late_fl32_process(
        reinterpret_cast<std::uint32_t*>(xinn + 392u),
        reinterpret_cast<std::uint64_t>(early),
        late,
        auro_matic_Engine2_fl32_process);

    // Routing_apply returns the native routing object address as its helper
    // result.  XinN::process itself is a status-returning callback, so the
    // Engine must not interpret that non-zero helper value as an error.
    (void)auro_matic_v3_XinN_Routing_fl32_apply(
        xinn_routing_state(xinn),
        early_a,
        early_b,
        late,
        channel_span_31,
        mode);
    return 0;
}

} // namespace

std::int64_t auro_matic_v3_XinN_Routing_fl32_apply(
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

std::int64_t auro_matic_XinN_fl32_process_inplace(std::uint64_t xinn_state, void** channel_span_31) {
    if (xinn_state == 0u)
        return 0;
    auto* xinn = e2_ptr(xinn_state);
    const std::uint64_t scratch_u64 = e2_read_u64(xinn, 40u);
    if (scratch_u64 == 0u)
        return auro_matic_XinN_fl32_process_scratch(xinn_state, channel_span_31);
    auto* scratch = reinterpret_cast<float*>(static_cast<std::uintptr_t>(scratch_u64));
    return xinn_process_core_574e30_574ee0(
        xinn_state,
        channel_span_31,
        scratch,
        scratch + kEngine2OutputFloats,
        scratch + 2u * kEngine2OutputFloats);
}

std::int64_t auro_matic_XinN_fl32_process_scratch(std::uint64_t xinn_state, void** channel_span_31) {
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

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_process_impl(
    std::uint64_t step_base,
    void** io_channels_31,
    std::uint32_t subblock_count,
    const std::uint8_t* block_records_516,
    AuroMaticV3XinNFl32ProcessFn process_fl32,
    AuroA3dengV4XinNResetFn reset_audio_state) {
    if (step_base == 0u || !io_channels_31)
        return 1;
    auto* process = process_fl32 ? process_fl32 : auro_matic_v3_XinN_fl32_process;
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

std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_process(
    std::uint64_t step_base,
    void** io_channels_31,
    std::uint32_t subblock_count,
    const std::uint8_t* block_records_516,
    AuroMaticV3XinNFl32ProcessFn process_fl32,
    AuroA3dengV4XinNResetFn reset_audio_state) {
    return auro_a3deng_v4_pipeline_step_upmix_XinN_process_impl(
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
        memory_delay_line_payload_sum(cfg_qword0, cfg_qword16, input_mask_count)
        + memory_frame_deque_required_additional_memory(
            static_cast<std::uint32_t>(((2u * block_samples + 255u) >> 8) + 1u))
        + memory_frame_deque_required_additional_memory(
            static_cast<std::uint32_t>(((block_samples * stage1_count + 255u) >> 8) + 1u))
        + memory_block_info_required_additional_memory(
            memory_block_info_slot_count(block_samples))
        + parse_result_pool_required_additional_memory(
            static_cast<std::uint32_t>(9u * ((block_samples * stage1_count + 255u) >> 8) + 9u))
        + memory_accumulator_tail_payload(cfg_qword0);

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

std::uint32_t block_info_construct(std::uint8_t* block_info, std::uint32_t slot_count) {
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

    const std::uint32_t block_info_count = memory_block_info_slot_count(block_u32);
    auto* block_info = memory_base + 104u;
    if (!block_info_construct(block_info, block_info_count))
        return false;

    const std::uint32_t parse_pool_count =
        memory_parse_result_pool_count(block_u32, *reinterpret_cast<const std::uint32_t*>(cfg + 28u));
    const std::uint64_t parse_pool_storage =
        codec_v3_alloc_zero_52acb0(parse_result_pool_required_additional_memory(parse_pool_count), 8u);
    if (parse_pool_count != 0u && parse_pool_storage == 0u)
        return false;
    if (!parse_result_pool_construct(
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
    return frame_deque_push_back(
        frame_deque_ptr, frame_ptr, kCodecV3FrameDequeSlotCopyBytes);
}

std::uint64_t codec_v3_parser_frame_deque_pop_front_530380(std::uint64_t frame_deque_ptr) {
    return frame_deque_pop_front(frame_deque_ptr, frame_mark_as_unused_106cd0_default);
}

CodecV3ParserRuntimeFns1034e0 codec_v3_default_parser_runtime_1034e0() {
    CodecV3ParserRuntimeFns1034e0 fns{};
    fns.delay_line_get_buffer = delay_line_get_buffer_u64_state;
    fns.frame_deque_find_first_with_end_after = frame_deque_find_first_with_end_after;
    fns.frame_deque_push_back = codec_v3_parser_frame_deque_push_back_530290;
    fns.frame_deque_pop_front = codec_v3_parser_frame_deque_pop_front_530380;
    fns.frame_mark_as_unused = parser_frame_mark_as_unused_cb;
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
    (void)frame_deque_pop_front(frame_deque_ptr, frame_mark_as_unused_106cd0_default);
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
    fns.frame_deque_find_first_with_end_after = frame_deque_find_first_with_end_after;
    fns.frame_mark_as_unused = frame_mark_as_unused_106cd0_default;
    fns.frame_deque_pop_front = codec_v3_output_frame_deque_pop_front_530380;
    fns.golombrice_get_errors = golombrice_get_errors_104e40;
    fns.extrapolate_process = extrapolate_process;
    fns.golombrice_initialize = golombrice_initialize_104e10;
    fns.extrapolate_initialize = extrapolate_initialize_104440;
    return fns;
}

} // namespace

void format_detector_integrated_set_layout(void* user, std::uint32_t layout_mask) {
    sync_detector_set_layout(
        reinterpret_cast<SyncDetectorState105ee0*>(user),
        layout_mask);
}

void format_detector_integrated_process_block(void* user, const std::uint64_t* channel_ptrs_27) {
    sync_detector_process_block(
        reinterpret_cast<SyncDetectorState105ee0*>(user),
        channel_ptrs_27);
}

void parser_t_construct(
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
    parser_q[1] = delay_line_stream_index(delay_line, stage0_count);
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
    format_detector_t_construct(
        fmt_region,
        decoder_base,
        decoder_base + auro_codec_v3_ida::kDecoder_off_Memory);

    parser_t_construct(
        decoder_base + auro_codec_v3_ida::kDecoder_off_Parser,
        decoder_base,
        decoder_base + auro_codec_v3_ida::kDecoder_off_Memory);

    output_generator_construct(
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


std::int64_t process_analyser_audio(std::uint8_t* processor_base) {
    // Literal 0xED070: return (*(a1+16))(*(a1+8));
    using Inner = std::int64_t (*)(void*);
    auto* inner_arg = *reinterpret_cast<void**>(processor_base + kProcessor_Analyser_ctx_ptr);
    auto* fn = reinterpret_cast<Inner>(*reinterpret_cast<void**>(processor_base + kProcessor_Analyser_inner_fn));
    return fn(inner_arg);
}

std::int64_t forward_analyser_to_controller(
    std::uint8_t* processor_base,
    std::uint8_t* input_buffer_desc,
    std::uint8_t* output_buffer_desc) {
    if (!processor_base)
        return 2;
    // Literal 0xDA320: только null-check и tail-call в Controller_process_audio(a1+0x140, a2, a3).
    std::uint8_t* ctrl = processor_base + kProcessor_to_Controller_audio;
    return controller_process_audio_da870(
        ctrl,
        input_buffer_desc,
        output_buffer_desc,
        sizeof(ProcessorIOBufferDesc));
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
    // Native unit-block size 1000 is explicitly supported; waive the generic
    // A3DENG modulo-32 IO check for that host size.
    const bool in_unit_block_1000 = in_desc->total_size_bytes == 1000u;
    if (in_unit != 0 && !in_unit_block_1000 && (in_desc->total_size_bytes % in_unit) != 0)
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
    const bool out_unit_block_1000 = out_desc->total_size_bytes == 1000u;
    if (out_unit != 0 && !out_unit_block_1000 && (out_desc->total_size_bytes % out_unit) != 0)
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

bool auro_decoder_impl_initialize(
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

std::int32_t auro_decoder_impl_decode(
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

    const ProcessorIoExpectDa9ae0 ex = /* Decompiled name: load_expect_da9ae0_from_impl */
load_expect_da9ae0_from_impl(impl_base);
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

