ProcessorIoExpectDa9ae0
load_expect_da9ae0_from_impl(const std::uint8_t* impl_base) {
    ProcessorIoExpectDa9ae0 ex{};
    if (!impl_base)
        return ex;
    using auro_codec_v3::kAuroDecoderImpl_expect_bytes_unit;
    using auro_codec_v3::kAuroDecoderImpl_off_InputDesc;
    using auro_codec_v3::kAuroDecoderImpl_off_OutputDesc;
    using auro_codec_v3::kAuroDecoderImpl_off_RuntimeInputMask;
    using auro_codec_v3::kAuroDecoderImpl_off_RuntimeOutputMask;
    const auto* in = reinterpret_cast<const ProcessorIOBufferDesc*>(impl_base + kAuroDecoderImpl_off_InputDesc);
    const auto* out = reinterpret_cast<const ProcessorIOBufferDesc*>(impl_base + kAuroDecoderImpl_off_OutputDesc);
    // bytes_unit is processor config (sample-count stride), NOT desc.total_size_bytes.
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
    // Literal перенос auro_codec_v3_decoder_CRC_t_init.
    // CRC-таблица у нас уже инициализируется лениво через channel_crc_table_init,
    // так что конструктор может быть пустым no-op якорем.
}

void decoder_channel_extrapolate_t_init_1042a1() {
    // Literal перенос auro_codec_v3_decoder_channel_Extrapolate_t_init.
    // Полный SSE-пролог заполняет LUT в.bss.data; для нас важно только то, что это однажды сделанный глобальный init.
    // Таблица уже материализована статически, поэтому дополнительной работы здесь не требуется.
}

// Literal перенос auro_codec_v3_decoder_Config_initialize.
std::int64_t auro_codec_v3_decoder_Config_initialize(std::int64_t a1, std::int64_t a2) {
    const auto v2 = *reinterpret_cast<std::uint64_t*>(a2 + 8);
    std::int64_t result = auro_codec_v3::kDecoderConfigInitErrBlockBits;
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
        result = auro_codec_v3::kDecoderConfigInitErrInputMaskRange;
        if (static_cast<std::int32_t>(in_mask) >= 0) {
            const auto out_mask = *reinterpret_cast<std::uint32_t*>(a1 + 40);
            result = auro_codec_v3::kDecoderConfigInitErrOutputMaskRange;
            if (static_cast<std::int32_t>(out_mask) >= 0) {
                result = auro_codec_v3::kDecoderConfigInitErrInputMaskNotSubset;
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
    // Current semantics: popcount(mask & 0x7fffffff).
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
    // Literal перенос (без инлайна внутренних memory helper'ов).
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
    // Literal-пролог: проверки размеров/ratio.
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

    // Literal блок: reconfigure Manager по текущему Decoder latency/state snapshot.
    std::array<std::uint8_t, 0xA0> cfg{};
    auto* cfg_u32 = reinterpret_cast<std::uint32_t*>(cfg.data());
    auto* cfg_u64 = reinterpret_cast<std::uint64_t*>(cfg.data());

    // v47 v48 v49 v50 проекция из (основные поля, которые реально читаются manager_configure).
    cfg_u32[0] = a1[620861];
    cfg_u32[1] = a1[620862];
    cfg_u32[2] = a1[620863];
    cfg_u32[3] = a1[620864];
    cfg_u32[4] = a1[620865];
    cfg_u32[5] = a1[620866];
    cfg_u32[6] = a1[620867];
    cfg_u32[7] = a1[620868];
    cfg_u32[8] = a1[620869];
    cfg_u32[9] = a1[620870];
    cfg_u32[10] = a1[620871];
    cfg_u32[11] = a1[620872];
    cfg_u64[6] = *reinterpret_cast<const std::uint64_t*>(&a1[620873]);
    cfg_u32[14] = a1[620875];
    cfg_u32[15] = a1[620876];
    cfg_u32[16] = a1[620877];
    cfg_u32[17] = a1[620878];

    const auto latency = static_cast<float>(auro_a3deng_v3_Decoder_latency(a1 + 557912));
    auto rc = auro_a3deng_v3_pipeline_Manager_set_initial_latency(a1 + 84, latency);
    if (rc != 0)
        return rc;
    if (a1[620978] != 0u)
        cfg_u32[0] = a1[620978];
    rc = auro_a3deng_v3_pipeline_Manager_configure(a1 + 84, cfg.data(), 1);
    if (rc != 0)
        return rc;

    // Частичный literal: один проход Decoder_process + Manager_configure(initial=0) + Manager_process_audio.
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

        // Literal copy-back tail (..) — без SIMD-оптимизаций, но с теми же guard/return semantics.
        const std::uint32_t out_mask = a1[620866];
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
        *reinterpret_cast<const std::uint64_t*>(decoder_ptr + auro_codec_v3::kDecoder_off_qword_2472));
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

constexpr std::uintptr_t kManagerOffParams478078 = ::auro_engine_v4::kManager_off_params;
constexpr std::uintptr_t kManagerOffBypass478078 = ::auro_engine_v4::kManager_off_bypass;
constexpr std::uintptr_t kManagerOffHeadroomDirty478078 = ::auro_engine_v4::kManager_off_headroom_dirty;
constexpr std::uintptr_t kManagerOffHeadroomRecalcDirty478078 = 484u;
constexpr std::uintptr_t kManagerOffStepChain478078 = ::auro_engine_v4::kManager_off_step_chain;
constexpr std::uintptr_t kManagerOffConfigureCb478078 = ::auro_engine_v4::kManager_off_configure_cb;
constexpr std::uintptr_t kManagerParamsBytes478078 = ::auro_engine_v4::kManager_params_bytes;
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

std::uint32_t
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

void
manager_reset_step_chain(std::uint8_t* step_chain) {
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

std::int64_t
manager_process_step_chain(
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
        if (fn_u64 == ::auro_engine_v4::kStepUpmixXinN_process) {
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

void
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
    // ранний guard перед копированием параметров.
    const auto* cfg_u32 = reinterpret_cast<const std::uint32_t*>(cfg_blob);
    if (cfg_u32[1] * cfg_u32[7] != cfg_u32[6] * cfg_u32[2])
        return 202;
    auto* manager = reinterpret_cast<std::uint8_t*>(manager_base);
    const std::uint32_t update_type =
manager_parameter_update_type(
        reinterpret_cast<const std::uint32_t*>(manager + kManagerOffParams478078),
        cfg_u32);
    std::memcpy(manager + kManagerOffParams478078, cfg_blob, kManagerParamsBytes478078);

    auto* step_chain = reinterpret_cast<std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<std::uint64_t*>(manager + kManagerOffStepChain478078)));
    if (update_type == 3u) {
        *reinterpret_cast<std::uint32_t*>(manager + kManagerOffBypass478078) = 1u;

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

    (void)kManagerOffHeadroomRecalcDirty478078;
    return 0;
}

std::int64_t auro_a3deng_v3_pipeline_Manager_process_audio(std::uint32_t* manager_base, void** io_channels) {
    if (!manager_base || !io_channels)
        return 1;
    auto* manager = reinterpret_cast<std::uint8_t*>(manager_base);

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
        return 0;
    return
manager_process_step_chain(manager, step_chain, io_channels);
}

std::int64_t auro_matic_XinN_fl32_process(std::uint64_t xinn_state, void** channel_span_31) {
    if (xinn_state == 0u)
        return 0;
    using CallbackFn = std::int64_t (*)(std::uint64_t, void**);
    const std::uint64_t fn_u64 = *reinterpret_cast<const std::uint64_t*>(xinn_state + 24u);
    if (fn_u64 == ::auro_engine_v4::kAuroMaticXinN_fl32_process_inplace)
        return auro_matic_XinN_fl32_process_inplace(xinn_state, channel_span_31);
    if (fn_u64 == ::auro_engine_v4::kAuroMaticXinN_fl32_process_scratch)
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

void
engine1_copy32_ring(float* ring, std::uint32_t offset, const float* src_32) {
    if (!ring || !src_32)
        return;
    offset %= 7616u;
    float* dst = ring + offset;
    std::memcpy(dst, src_32, 32u * sizeof(float));
    if (offset < 32u)
        std::memcpy(dst + 7616u, src_32, 32u * sizeof(float));
}

void
engine1_sum_98_taps_to_vec(
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

engine1_copy32_ring(ring, ring_a_offset, filtered_a);

engine1_copy32_ring(ring, ring_a_offset + 3808u, filtered_b);

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

engine1_sum_98_taps_to_vec(
                    ring,
                    history_offset + static_cast<std::uint32_t>(4u * vec),
                    entries1,
                    out + 4u * vec);
            for (std::size_t vec = 0; vec < 8u; ++vec)

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

void
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


add_scaled_channel_32(downmix_a, channel_span_31[4], gain_surround);

add_scaled_channel_32(downmix_b, channel_span_31[5], gain_surround);
    if (*reinterpret_cast<const std::uint32_t*>(early_state + 280u) != 0u) {

add_scaled_channel_32(downmix_a, channel_span_31[6], gain_center);

add_scaled_channel_32(downmix_b, channel_span_31[6], gain_center);
    }
    if (*reinterpret_cast<const std::uint32_t*>(early_state + 276u) != 0u) {

add_scaled_channel_32(downmix_a, channel_span_31[7], gain_back);

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

constexpr std::uint32_t kEngine2DelayRingFloats = 94640u; // bytes.
constexpr std::uint32_t kEngine2AllocatedFloats = 94704u;  // bytes.
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

void
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

e2_reverb_process(state, ring, preset, i, cross, frame);
            e2_accumulate_frame(accum + 0u, frame, e2_bus_weight(bus[0], 4u * i + 0u));
            e2_accumulate_frame(accum + 32u, frame, e2_bus_weight(bus[1], 4u * i + 0u));
            e2_accumulate_frame(accum + 64u, frame, e2_bus_weight(bus[2], 4u * i + 0u));


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
    *reinterpret_cast<std::uint32_t*>(engine1_state + 40u) = 0u;
    *reinterpret_cast<std::uint64_t*>(engine1_state + 16u) = memory + 0x7800u;
    *reinterpret_cast<std::uint64_t*>(engine1_state + 0u) = 0u;
    *reinterpret_cast<std::uint64_t*>(engine1_state + 8u) = memory;
    *reinterpret_cast<std::uint32_t*>(engine1_state + 60u) = 7648u;
    *reinterpret_cast<std::uint64_t*>(engine1_state + 24u) = memory + 0x8440u;
    *reinterpret_cast<std::uint64_t*>(engine1_state + 32u) = memory + 0x9080u;
    return reinterpret_cast<std::int64_t>(engine1_state);
}

void
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

bool
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

std::uint64_t
xinn_select_native_spreset_from_v4_static(
    bool surround_mode,
    const std::uint8_t* tuning_static) {
    if (!tuning_static)
        return 0u;
    const std::uint32_t room = std::min<std::uint32_t>(
        *reinterpret_cast<const std::uint32_t*>(tuning_static + 4u), 3u);
    const std::uint64_t base = surround_mode ? kXinnSpreset5inNBase_570350 : kXinnSpreset2in6Base_570460;
    return base + static_cast<std::uint64_t>(room) * kXinnSpresetStride_35bf40;
}

const std::uint8_t*
xinn_portable_spreset_from_native_va(std::uint64_t spreset_u64) {
    (void)spreset_u64;
    return nullptr;
}

const std::uint8_t*
xinn_portable_processed_preset_from_native_va(std::uint64_t spreset_u64) {
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

bool
xinn_initialize_processed_preset_from_portable_spreset(
    std::uint8_t* processed_preset,
    std::uint64_t spreset_u64) {
    const std::uint8_t* preset =
xinn_portable_processed_preset_from_native_va(spreset_u64);
    if (!processed_preset || !preset)
        return false;

    std::memcpy(processed_preset, preset, kXinnProcessedPresetBytes);
    processed_preset[0] = 1u;
    processed_preset[1] = kXinnProcessedPresetMarker;
    return true;
}

void
xinn_engine2_initialize_static_from_spreset(
    std::uint8_t* processed_preset,
    std::uint64_t spreset_u64) {
    if (processed_preset && processed_preset[1] == kXinnProcessedPresetMarker)
        return;
    const std::uint8_t* spreset =
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

void
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

float
xinn_db_to_linear(float db) {
    return db > -144.0f ? std::pow(10.0f, db * 0.050000001f) : 0.0f;
}

std::uint32_t
xinn_u32_bits(const float* word) {
    return *reinterpret_cast<const std::uint32_t*>(word);
}

void
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
        const std::uint32_t gain_index =
xinn_u32_bits(engine1_state + config_base + row + 0u);
        if (gain_index >= 3u)
            continue;
        const float* selected =

xinn_u32_bits(engine1_state + config_base + row + 1u) == 1u ? gain_set_b : gain_set_a;
        float scale = 1.0f;
        if (static_cast<std::uint8_t>(

xinn_u32_bits(engine1_state + config_base + row + 2u))
            != 0u) {
            scale = inline_gain_xyz[gain_index];
        }
        engine1_state[out] = scale * engine1_state[config_base + row + 3u] * selected[gain_index];
    }
}

void
xinn_engine1_apply(float* engine1_state, const float* dynamic_64) {
    if (!engine1_state || !dynamic_64)
        return;

    float gain_set_a[3] = {};
    float gain_set_b[3] = {};
    const float base_a =
xinn_db_to_linear(dynamic_64[0]);
    gain_set_a[0] =
xinn_db_to_linear(dynamic_64[1]) * base_a;
    gain_set_a[1] =
xinn_db_to_linear(dynamic_64[2]) * base_a;
    gain_set_a[2] =
xinn_db_to_linear(dynamic_64[3]) * base_a;

    const float base_b =
xinn_db_to_linear(dynamic_64[4]);
    gain_set_b[0] =
xinn_db_to_linear(dynamic_64[5]) * base_b;
    gain_set_b[1] =
xinn_db_to_linear(dynamic_64[6]) * base_b;
    gain_set_b[2] =
xinn_db_to_linear(dynamic_64[7]) * base_b;

    const float inline_gain_xyz[3] = {
        *reinterpret_cast<const float*>(reinterpret_cast<const std::uint8_t*>(dynamic_64) + 32u),
        *reinterpret_cast<const float*>(reinterpret_cast<const std::uint8_t*>(dynamic_64) + 36u),
        *reinterpret_cast<const float*>(reinterpret_cast<const std::uint8_t*>(dynamic_64) + 40u),
    };

xinn_engine1_apply_table(
        engine1_state,
        0u,
        2943u,
        gain_set_a,
        gain_set_b,
        inline_gain_xyz);

xinn_engine1_apply_table(
        engine1_state,
        490u,
        3139u,
        gain_set_a,
        gain_set_b,
        inline_gain_xyz);

xinn_engine1_apply_table(
        engine1_state,
        980u,
        3811u,
        gain_set_a,
        gain_set_b,
        inline_gain_xyz);

xinn_engine1_apply_table(
        engine1_state,
        1470u,
        4007u,
        gain_set_a,
        gain_set_b,
        inline_gain_xyz);

xinn_engine1_apply_table(
        engine1_state,
        1960u,
        4679u,
        gain_set_a,
        gain_set_b,
        inline_gain_xyz);

xinn_engine1_apply_table(
        engine1_state,
        2450u,
        4875u,
        gain_set_a,
        gain_set_b,
        inline_gain_xyz);
}

void
xinn_preset_tune_late_stereo(std::uint8_t* preset_base, const float* dynamic_96) {
    if (!preset_base || !dynamic_96)
        return;

    std::memcpy(preset_base + 1072u, dynamic_96, 64u);
    const float gain_sum_base = *reinterpret_cast<const float*>(preset_base + 1116u);
    *reinterpret_cast<float*>(preset_base + 23460u) =

xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1120u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23596u) =

xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1124u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23732u) =

xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1128u) + gain_sum_base);

    const std::uint64_t engine2_storage = *reinterpret_cast<const std::uint64_t*>(preset_base + 23320u);
    if (engine2_storage != 0u)
        *reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(engine2_storage) + 960u) =
            *reinterpret_cast<const std::uint32_t*>(preset_base + 1132u);

    *reinterpret_cast<float*>(preset_base + 23784u) =
xinn_db_to_linear(dynamic_96[16]);
    *reinterpret_cast<float*>(preset_base + 23788u) =
xinn_db_to_linear(dynamic_96[19]);
    *reinterpret_cast<float*>(preset_base + 23792u) =
xinn_db_to_linear(dynamic_96[17]);
    *reinterpret_cast<float*>(preset_base + 23796u) =
xinn_db_to_linear(dynamic_96[20]);
    *reinterpret_cast<float*>(preset_base + 23800u) =
xinn_db_to_linear(dynamic_96[18]);
    *reinterpret_cast<float*>(preset_base + 23804u) =
xinn_db_to_linear(dynamic_96[21]);
}

void
xinn_preset_tune_late_surround(std::uint8_t* preset_base, const float* dynamic_112) {
    if (!preset_base || !dynamic_112)
        return;

    std::memcpy(preset_base + 1072u, dynamic_112, 64u);
    const float gain_sum_base = *reinterpret_cast<const float*>(preset_base + 1116u);
    *reinterpret_cast<float*>(preset_base + 23460u) =

xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1120u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23596u) =

xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1124u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23732u) =

xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1128u) + gain_sum_base);

    const std::uint64_t engine2_storage = *reinterpret_cast<const std::uint64_t*>(preset_base + 23320u);
    if (engine2_storage != 0u)
        *reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(engine2_storage) + 960u) =
            *reinterpret_cast<const std::uint32_t*>(preset_base + 1132u);

    *reinterpret_cast<float*>(preset_base + 23808u) =
xinn_db_to_linear(dynamic_112[16]);
    *reinterpret_cast<float*>(preset_base + 23812u) =
xinn_db_to_linear(dynamic_112[20]);
    *reinterpret_cast<float*>(preset_base + 23816u) =
xinn_db_to_linear(dynamic_112[24]);
    *reinterpret_cast<float*>(preset_base + 23820u) =
xinn_db_to_linear(dynamic_112[19]);
    *reinterpret_cast<float*>(preset_base + 23824u) =
xinn_db_to_linear(dynamic_112[23]);
    *reinterpret_cast<float*>(preset_base + 23828u) =
xinn_db_to_linear(dynamic_112[27]);
    *reinterpret_cast<float*>(preset_base + 23832u) =
xinn_db_to_linear(dynamic_112[17]);
    *reinterpret_cast<float*>(preset_base + 23836u) =
xinn_db_to_linear(dynamic_112[21]);
    *reinterpret_cast<float*>(preset_base + 23840u) =
xinn_db_to_linear(dynamic_112[25]);
    *reinterpret_cast<float*>(preset_base + 23844u) =
xinn_db_to_linear(dynamic_112[18]);
    *reinterpret_cast<float*>(preset_base + 23848u) =
xinn_db_to_linear(dynamic_112[22]);
    *reinterpret_cast<float*>(preset_base + 23852u) =
xinn_db_to_linear(dynamic_112[26]);
}

[[maybe_unused]] void
xinn_preset_tune_stereo(std::uint8_t* preset_base, const float* dynamic_96) {
    if (!preset_base || !dynamic_96)
        return;

    std::memcpy(preset_base + 1072u, dynamic_96, 64u);

xinn_engine1_apply(
        reinterpret_cast<float*>(preset_base + 1136u),
        reinterpret_cast<const float*>(preset_base + 1072u));

    const float gain_sum_base = *reinterpret_cast<const float*>(preset_base + 1116u);
    *reinterpret_cast<float*>(preset_base + 23460u) =

xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1120u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23596u) =

xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1124u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23732u) =

xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1128u) + gain_sum_base);

    const std::uint64_t engine2_storage = *reinterpret_cast<const std::uint64_t*>(preset_base + 23320u);
    if (engine2_storage != 0u)
        *reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(engine2_storage) + 960u) =
            *reinterpret_cast<const std::uint32_t*>(preset_base + 1132u);

    *reinterpret_cast<float*>(preset_base + 23784u) =
xinn_db_to_linear(dynamic_96[16]);
    *reinterpret_cast<float*>(preset_base + 23788u) =
xinn_db_to_linear(dynamic_96[19]);
    *reinterpret_cast<float*>(preset_base + 23792u) =
xinn_db_to_linear(dynamic_96[17]);
    *reinterpret_cast<float*>(preset_base + 23796u) =
xinn_db_to_linear(dynamic_96[20]);
    *reinterpret_cast<float*>(preset_base + 23800u) =
xinn_db_to_linear(dynamic_96[18]);
    *reinterpret_cast<float*>(preset_base + 23804u) =
xinn_db_to_linear(dynamic_96[21]);
}

[[maybe_unused]] void
xinn_preset_tune_surround(std::uint8_t* preset_base, const float* dynamic_112) {
    if (!preset_base || !dynamic_112)
        return;

    std::memcpy(preset_base + 1072u, dynamic_112, 64u);

xinn_engine1_apply(
        reinterpret_cast<float*>(preset_base + 1136u),
        reinterpret_cast<const float*>(preset_base + 1072u));

    const float gain_sum_base = *reinterpret_cast<const float*>(preset_base + 1116u);
    *reinterpret_cast<float*>(preset_base + 23460u) =

xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1120u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23596u) =

xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1124u) + gain_sum_base);
    *reinterpret_cast<float*>(preset_base + 23732u) =

xinn_db_to_linear(*reinterpret_cast<const float*>(preset_base + 1128u) + gain_sum_base);

    const std::uint64_t engine2_storage = *reinterpret_cast<const std::uint64_t*>(preset_base + 23320u);
    if (engine2_storage != 0u)
        *reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(engine2_storage) + 960u) =
            *reinterpret_cast<const std::uint32_t*>(preset_base + 1132u);

    *reinterpret_cast<float*>(preset_base + 23808u) =
xinn_db_to_linear(dynamic_112[16]);
    *reinterpret_cast<float*>(preset_base + 23812u) =
xinn_db_to_linear(dynamic_112[20]);
    *reinterpret_cast<float*>(preset_base + 23816u) =
xinn_db_to_linear(dynamic_112[24]);
    *reinterpret_cast<float*>(preset_base + 23820u) =
xinn_db_to_linear(dynamic_112[19]);
    *reinterpret_cast<float*>(preset_base + 23824u) =
xinn_db_to_linear(dynamic_112[23]);
    *reinterpret_cast<float*>(preset_base + 23828u) =
xinn_db_to_linear(dynamic_112[27]);
    *reinterpret_cast<float*>(preset_base + 23832u) =
xinn_db_to_linear(dynamic_112[17]);
    *reinterpret_cast<float*>(preset_base + 23836u) =
xinn_db_to_linear(dynamic_112[21]);
    *reinterpret_cast<float*>(preset_base + 23840u) =
xinn_db_to_linear(dynamic_112[25]);
    *reinterpret_cast<float*>(preset_base + 23844u) =
xinn_db_to_linear(dynamic_112[18]);
    *reinterpret_cast<float*>(preset_base + 23848u) =
xinn_db_to_linear(dynamic_112[22]);
    *reinterpret_cast<float*>(preset_base + 23852u) =
xinn_db_to_linear(dynamic_112[26]);
}

std::int64_t
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

    std::int64_t ok =
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
    if (!xinn_v3_state)
        return 1;
    *reinterpret_cast<std::uint32_t*>(xinn_v3_state + 840u) =
        input_mask != 0u ? 1u : 0u;
    if (input_mask == 0u)
        return output_mask != 0u ? 1 : 0;
    // Native accepts only 32/44.1/48 kHz here; higher rates
    // are rejected because the AM4HP Upmixing graph has no differential
    // coverage for them.
    if (sample_rate != 32000u && sample_rate != 44100u && sample_rate != 48000u)
        return 1;
    return auro_matic_XinN_fl32_initialize(
               xinn_v3_state, input_mask, output_mask, preset)
        ? 0
        : 1;
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

std::uint64_t
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

void
xinn_copy_runtime_preset_into_processed_object(
    std::uint8_t* processed_preset,
    std::uint64_t runtime_preset) {
    if (!processed_preset)
        return;
    const bool preset_pointer_available =
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

void
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

bool
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
    const std::uint8_t* portable_spreset =
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

std::uint64_t
xinn_inline_runtime_preset_from_processed_object(const std::uint8_t* processed_preset) {
    if (!processed_preset || processed_preset[0] == 0u)
        return 0u;
    return reinterpret_cast<std::uint64_t>(processed_preset + 8u);
}

bool
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

void
xinn_retune_processed_preset(
    std::uint8_t* processed_preset,
    std::uint32_t mode,
    const std::uint8_t* update_blob) {
    if (!processed_preset || !update_blob || processed_preset[0] == 0u)
        return;
    if (processed_preset[1] == kXinnProcessedPresetMarker)
        return;
    const bool can_apply_engine1 =
xinn_processed_preset_has_engine1_static_rows(processed_preset);
    if (mode == 1u && *reinterpret_cast<const std::uint32_t*>(update_blob + 68u) != 0u) {
        if (can_apply_engine1) {

xinn_preset_tune_stereo(
                processed_preset,
                reinterpret_cast<const float*>(update_blob + 72u));
        } else {

xinn_preset_tune_late_stereo(
                processed_preset,
                reinterpret_cast<const float*>(update_blob + 72u));
        }
    } else if (mode == 2u && *reinterpret_cast<const std::uint32_t*>(update_blob + 160u) != 0u) {
        if (can_apply_engine1) {

xinn_preset_tune_surround(
                processed_preset,
                reinterpret_cast<const float*>(update_blob + 164u));
        } else {

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

    std::array<std::uint8_t, 120u> dynamic{};
    auro_matic_v3_XinN_parameter_Dynamic_t_default(dynamic.data());
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
    auro_matic_v3_XinN_parameter_Dynamic_t_default(dynamic.data());
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

void
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
        runtime_preset =
xinn_select_native_runtime_preset_from_v4_static(false, plan_blob + 12u);
        spreset =
xinn_select_native_spreset_from_v4_static(false, plan_blob + 12u);

xinn_fill_v3_dynamic_from_v4_static(dynamic_120, false, plan_blob + 12u);
    } else if (mode == 2u) {
        if (plan_blob[92u] == 0u || *reinterpret_cast<const std::uint32_t*>(update_blob + 160u) != 1u)
            return 1;
        runtime_preset =
xinn_select_native_runtime_preset_from_v4_static(true, plan_blob + 48u);
        spreset =
xinn_select_native_spreset_from_v4_static(true, plan_blob + 48u);

xinn_fill_v3_dynamic_from_v4_static(dynamic_120, true, plan_blob + 48u);
    } else {
        auro_matic_v3_XinN_parameter_Dynamic_t_default(dynamic_120);
    }

xinn_copy_runtime_preset_into_processed_object(processed_preset, runtime_preset);
    if (processed_preset[0] == 0u)
        (void)
xinn_initialize_processed_preset_from_portable_spreset(processed_preset, spreset);
    if (processed_preset[0] == 0u)
        return 1;

xinn_retarget_processed_runtime_preset_backing(processed_preset);

xinn_engine2_initialize_static_from_spreset(processed_preset, spreset);
    (void)
xinn_engine1_initialize_static_from_spreset(processed_preset, spreset);

xinn_retune_processed_preset(processed_preset, mode, update_blob);
    const std::uint64_t preset =
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


xinn_retune_processed_preset(step + 460096u, mode, update_blob);
    const std::uint64_t preset =
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
    if (!channel_span_31 || left_idx >= ::auro_engine_v4::kChannelCount
        || right_idx >= ::auro_engine_v4::kChannelCount)
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
    if (!channel_span_31 || idx >= ::auro_engine_v4::kChannelCount)
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

    return auro_matic_v3_XinN_Routing_fl32_apply(
        xinn_routing_state(xinn),
        early_a,
        early_b,
        late,
        channel_span_31,
        mode);
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

    std::array<void*, ::auro_engine_v4::kChannelCount> span{};
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
    // *a1 = (нулевое начальное состояние аккумулятора).
    acc[0] = 0;
    acc[1] = 0;
}

void auro_codec_v3_Decoder_t_required_additional_memory(std::uint64_t* acc, std::uint64_t* out_pair) {
    if (!acc || !out_pair)
        return;
    // Config_initialize(tmp, out_pair) -> Memory_t_required_additional_memory(acc, tmp)
    std::uint8_t cfg_raw[64]{};
    const auto rc = auro_codec_v3_decoder_Config_initialize(
        reinterpret_cast<std::int64_t>(cfg_raw),
        reinterpret_cast<std::int64_t>(out_pair));
    if (rc != 0)
        return;

    const auto* cfg_u32 = reinterpret_cast<const std::uint32_t*>(cfg_raw);
    const auto* cfg_u64 = reinterpret_cast<const std::uint64_t*>(cfg_raw);

    const std::uint32_t block_samples =
        cfg_u32[auro_codec_v3::kDecoderConfig_off_block_bits / 4u];
    const std::uint32_t stage1_count =
        cfg_u32[auro_codec_v3::kDecoderConfig_off_stage1_count / 4u];
    const std::uint32_t input_mask_count =
        cfg_u32[auro_codec_v3::kDecoderConfig_off_input_mask_count / 4u];
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
        decoder_base + auro_codec_v3::kDecoderConfig_off_stage0_count);
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
        d + auro_codec_v3::kDecoder_off_FormatDetector);
    region->tail.sink_notify = reinterpret_cast<void (*)(void*, std::int64_t)>(fn);
    region->tail.sink_user = user;
}

void auro_codec_v3_Decoder_set_content_callback(std::uint64_t decoder_base, void* fn, void* user) {
    if (decoder_base == 0)
        return;
    // Parser_set_content_callback(a1+568, fn, user)
    auto* d = reinterpret_cast<std::uint8_t*>(decoder_base);
    *reinterpret_cast<std::uint64_t*>(d + 1200u) = reinterpret_cast<std::uint64_t>(fn);   // (a1+568)+632
    *reinterpret_cast<std::uint64_t*>(d + 1208u) = reinterpret_cast<std::uint64_t>(user); // (a1+568)+640
}

void auro_codec_v3_Decoder_set_decide_decode_callback(std::uint64_t decoder_base, void* fn, void* user) {
    if (decoder_base == 0)
        return;
    auto* d = reinterpret_cast<std::uint8_t*>(decoder_base);
    *reinterpret_cast<std::uint64_t*>(
        d + auro_codec_v3::kDecoder_off_OutputGenerator + kOgOff_pre_segments_cb) =
        reinterpret_cast<std::uint64_t>(fn);
    *reinterpret_cast<std::uint64_t*>(
        d + auro_codec_v3::kDecoder_off_OutputGenerator + kOgOff_pre_segments_ctx) =
        reinterpret_cast<std::uint64_t>(user);
}

void auro_codec_v3_Decoder_set_metadata_callback(std::uint64_t decoder_base, void* fn, void* user) {
    if (decoder_base == 0)
        return;
    auto* d = reinterpret_cast<std::uint8_t*>(decoder_base);
    *reinterpret_cast<std::uint64_t*>(
        d + auro_codec_v3::kDecoder_off_OutputGenerator + kOgOff_metadata_cb) =
        reinterpret_cast<std::uint64_t>(fn);
    *reinterpret_cast<std::uint64_t*>(
        d + auro_codec_v3::kDecoder_off_OutputGenerator + kOgOff_metadata_ctx) =
        reinterpret_cast<std::uint64_t>(user);
}

std::uint32_t decoder_t_construct_101760(std::uint8_t* decoder_base, const void* config_init_args, void* user_ctx) {
    // Literal перенос auro_codec_v3_Decoder_t_construct.
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
            decoder_base + auro_codec_v3::kDecoder_off_Memory,
            decoder_base)) {
        return 0;
    }

    auto* fmt_region = reinterpret_cast<FormatDetectorRegion1056c0*>(
        decoder_base + auro_codec_v3::kDecoder_off_FormatDetector);
    format_detector_t_construct(
        fmt_region,
        decoder_base,
        decoder_base + auro_codec_v3::kDecoder_off_Memory);

    parser_t_construct(
        decoder_base + auro_codec_v3::kDecoder_off_Parser,
        decoder_base,
        decoder_base + auro_codec_v3::kDecoder_off_Memory);

    output_generator_construct(
        decoder_base + auro_codec_v3::kDecoder_off_OutputGenerator,
        decoder_base,
        reinterpret_cast<std::uint64_t>(decoder_base + auro_codec_v3::kDecoder_off_Memory));

    const std::uint64_t qword0 = *reinterpret_cast<const std::uint64_t*>(decoder_base);
    const std::uint32_t dword28 =
        *reinterpret_cast<const std::uint32_t*>(decoder_base + auro_codec_v3::kDecoder_off_dword_28);
    *reinterpret_cast<std::uint64_t*>(decoder_base + auro_codec_v3::kDecoder_off_qword_2472) =
        static_cast<std::uint64_t>(static_cast<std::uint32_t>(qword0)) * static_cast<std::uint64_t>(dword28);

    (void)user_ctx;
    return 1;
}


std::int64_t process_analyser_audio(std::uint8_t* processor_base) {
    // Literal: return (*(a1+16))(*(a1+8));
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
    // Literal: только null-check и tail-call в Controller_process_audio(a1+0x140, a2, a3).
    std::uint8_t* ctrl = processor_base + kProcessor_to_Controller_audio;
    return controller_process_audio_da870(
        ctrl,
        input_buffer_desc,
        output_buffer_desc,
        sizeof(ProcessorIOBufferDesc));
}

std::int64_t analyser_process(std::uint8_t* processor_base, const std::uint32_t* sample_increment) {
    // Literal: result = (*(a1+256))(a1); *(_DWORD*)a1 += *a2; return result;
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

    // использует ratio (*a2 a2->field_4) в timing-ветке; здесь переносим только инкремент счётчика.
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

    // v15 = (float)(*a2) (float)(a2->field_4)
    const float input_ratio = static_cast<float>(in_desc->total_size_bytes) / static_cast<float>(in_desc->field_4);
    *reinterpret_cast<float*>(processor_base + kProcessorPerf_ratio_sum_float) += input_ratio;

    // * += elapsed
    *reinterpret_cast<std::int64_t*>(processor_base + kProcessorPerf_elapsed_sum_qword) += elapsed_ticks;

    // if (elapsed < 0) special conversion via shifted halves.
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

    using auro_codec_v3::kAuroDecoderImpl_off_InputDesc;
    using auro_codec_v3::kAuroDecoderImpl_off_OutputDesc;
    using auro_codec_v3::kAuroDecoderImpl_off_ChannelCount;
    using auro_codec_v3::kAuroDecoderImpl_off_BlockSize;
    using auro_codec_v3::kAuroDecoderImpl_off_RuntimeInputMask;
    using auro_codec_v3::kAuroDecoderImpl_off_RuntimeOutputMask;

    auto* in = reinterpret_cast<ProcessorIOBufferDesc*>(impl_base + kAuroDecoderImpl_off_InputDesc);
    auto* out = reinterpret_cast<ProcessorIOBufferDesc*>(impl_base + kAuroDecoderImpl_off_OutputDesc);
    std::memset(in, 0, sizeof(*in));
    std::memset(out, 0, sizeof(*out));

    // AuroDecoderImpl IO: total_size_bytes = sample count (not byte length).
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

    using auro_codec_v3::kAuroDecoderImpl_off_InputDesc;
    using auro_codec_v3::kAuroDecoderImpl_off_OutputDesc;
    using auro_codec_v3::kAuroDecoderImpl_off_ProcessorInstance;

    auto* in = reinterpret_cast<const ProcessorIOBufferDesc*>(impl_base + kAuroDecoderImpl_off_InputDesc);
    auto* out = reinterpret_cast<const ProcessorIOBufferDesc*>(impl_base + kAuroDecoderImpl_off_OutputDesc);
    auto* processor_base = impl_base + kAuroDecoderImpl_off_ProcessorInstance;

    const ProcessorIoExpectDa9ae0 ex =
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

