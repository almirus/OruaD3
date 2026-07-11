std::int64_t auro_asc4he_v1_Crossover_initialize_541f60_partial(
    std::uint8_t* a1,
    std::uint32_t a2,
    std::int32_t a3,
    std::int32_t a4) {
    if (!a1)
        return 1;
    std::memset(a1, 0, kAsc4heCrossoverStateBytes);
    auto* const lr500 = auro_iir_w32_LinkwitzRiley_4th_t_construct_partial(a1, a2, 2, a4, 500.0f);
    auto* const lr6000 = auro_iir_w32_LinkwitzRiley_4th_t_construct_partial(a1 + 239u, a2, 2, a4, 6000.0f);
    if (!lr500 || !lr6000)
        return 1;
    *reinterpret_cast<std::uint64_t*>(a1 + 480u) = reinterpret_cast<std::uint64_t>(lr500);
    *reinterpret_cast<std::uint64_t*>(a1 + 488u) = reinterpret_cast<std::uint64_t>(lr6000);
    *reinterpret_cast<std::uint64_t*>(a1 + 496u) = a3 ? 0x542020u : 0x5423B0u;
    (void)auro_iir_w32_LinkwitzRiley_4th_reset_audio_state_partial(lr500);
    (void)auro_iir_w32_LinkwitzRiley_4th_reset_audio_state_partial(lr6000);
    return 0;
}

std::int64_t auro_asc4he_v1_Crossover_reset_audio_state_5428e0_partial(std::uint8_t* a1) {
    if (!a1)
        return 0;
    auto* const lr500 = reinterpret_cast<std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<const std::uint64_t*>(a1 + 480u)));
    auto* const lr6000 = reinterpret_cast<std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<const std::uint64_t*>(a1 + 488u)));
    (void)auro_iir_w32_LinkwitzRiley_4th_reset_audio_state_partial(lr500);
    return auro_iir_w32_LinkwitzRiley_4th_reset_audio_state_partial(lr6000);
}

std::int64_t auro_asc4he_v1_Crossover_process_542910_partial(
    std::uint8_t* a1,
    std::uint64_t* a2,
    std::uint64_t* a3,
    std::uint64_t* a4,
    std::uint64_t* a5) {
    if (!a1 || !a2)
        return 0;
    auto* const in0 = reinterpret_cast<float*>(static_cast<std::uintptr_t>(a2[0]));
    auto* const in1 = reinterpret_cast<float*>(static_cast<std::uintptr_t>(a2[1]));
    if (!in0 || !in1)
        return 0;
    float tmp_low0[32]{};
    float tmp_low1[32]{};
    float tmp_high0[32]{};
    float tmp_high1[32]{};
    float tmp_top0[32]{};
    float tmp_top1[32]{};
    float* const low0 = (a3 && a3[0]) ? reinterpret_cast<float*>(static_cast<std::uintptr_t>(a3[0])) : tmp_low0;
    float* const low1 = (a3 && a3[1]) ? reinterpret_cast<float*>(static_cast<std::uintptr_t>(a3[1])) : tmp_low1;
    if (!low0 || !low1)
        return 0;
    auto* const lr500 = reinterpret_cast<std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<const std::uint64_t*>(a1 + 480u)));
    auto* const lr6000 = reinterpret_cast<std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<const std::uint64_t*>(a1 + 488u)));
    std::uint64_t low_pair[2]{asc4he_u64_ptr(low0), asc4he_u64_ptr(low1)};
    float* const high0 = (a4 && a4[0]) ? reinterpret_cast<float*>(static_cast<std::uintptr_t>(a4[0])) : tmp_high0;
    float* const high1 = (a4 && a4[1]) ? reinterpret_cast<float*>(static_cast<std::uintptr_t>(a4[1])) : tmp_high1;
    float* const top0 = (a5 && a5[0]) ? reinterpret_cast<float*>(static_cast<std::uintptr_t>(a5[0])) : tmp_top0;
    float* const top1 = (a5 && a5[1]) ? reinterpret_cast<float*>(static_cast<std::uintptr_t>(a5[1])) : tmp_top1;
    std::uint64_t high_pair[2]{asc4he_u64_ptr(high0), asc4he_u64_ptr(high1)};
    if (!auro_iir_w32_LinkwitzRiley_4th_process_partial(lr500, a2, low_pair, high_pair))
        return 0;
    const bool split_high_band = *reinterpret_cast<const std::uint64_t*>(a1 + 496u) == 0x542020u;
    if (!split_high_band) {
        if (a5 && a5[0] && a5[1])
            asc4he_zero_pair_32(top0, top1);
        return static_cast<std::int64_t>(a2[0]);
    }
    std::uint64_t high_in_pair[2]{asc4he_u64_ptr(high0), asc4he_u64_ptr(high1)};
    std::uint64_t top_pair[2]{asc4he_u64_ptr(top0), asc4he_u64_ptr(top1)};
    if (!auro_iir_w32_LinkwitzRiley_4th_process_partial(lr6000, high_in_pair, high_pair, top_pair))
        return 0;
    return static_cast<std::int64_t>(a2[0]);
}

std::int64_t auro_asc4he_v1_SideUpCrossover_initialize_5419a0_partial(
    std::uint8_t* a1,
    std::int32_t a2,
    std::uint32_t a3,
    std::uint32_t sample_rate) {
    if (!a1)
        return 1;
    std::memset(a1, 0, kAsc4heSideUpCrossoverStateBytes);
    *reinterpret_cast<std::uint32_t*>(a1 + 0u) = static_cast<std::uint32_t>(a2);
    if (!a2)
        return 1;
    float cutoff_hz = 0.0f;
    std::memcpy(&cutoff_hz, &a3, sizeof(cutoff_hz));
    if (!(cutoff_hz > 0.0f))
        cutoff_hz = 700.0f;
    if (asc4he_butterworth_crossover_float_initialize(a1 + 4u, sample_rate, 1, cutoff_hz) == 0)
        return 0;
    if (asc4he_butterworth_crossover_float_initialize(a1 + 64u, sample_rate, 1, cutoff_hz) == 0)
        return 0;
    return 1;
}

std::int64_t auro_asc4he_v1_SideUpCrossover_reset_audio_state_541a30_partial(std::uint8_t* a1) {
    if (!a1)
        return 0;
    if (*reinterpret_cast<const std::uint32_t*>(a1 + 0u) != 0u) {
        asc4he_butterworth_crossover_float_reset(a1 + 4u);
        asc4he_butterworth_crossover_float_reset(a1 + 64u);
    }
    return 0;
}

std::int64_t auro_asc4he_v1_SideUpCrossover_process_541a60_partial(
    std::uint8_t* a1,
    std::uint64_t* a2,
    std::uint64_t* a3,
    std::uint64_t* a4,
    std::uint64_t* a5) {
    if (!a1 || !a2 || !a4)
        return 0;
    auto* const in0 = reinterpret_cast<float*>(static_cast<std::uintptr_t>(a2[0]));
    auto* const in1 = reinterpret_cast<float*>(static_cast<std::uintptr_t>(a2[1]));
    auto* const low0 = reinterpret_cast<float*>(static_cast<std::uintptr_t>(a4[0]));
    auto* const low1 = reinterpret_cast<float*>(static_cast<std::uintptr_t>(a4[1]));
    if (!in0 || !in1 || !low0 || !low1)
        return 0;
    if (*reinterpret_cast<const std::uint32_t*>(a1 + 0u) == 0u) {
        asc4he_copy_pair_scaled_32(low0, low1, in0, in1, 1.f);
        return 1;
    }
    if (!a3 || !a5)
        return 0;
    auto* const high0 = reinterpret_cast<float*>(static_cast<std::uintptr_t>(a3[0]));
    auto* const high1 = reinterpret_cast<float*>(static_cast<std::uintptr_t>(a3[1]));
    auto* const add0 = reinterpret_cast<float*>(static_cast<std::uintptr_t>(a5[0]));
    auto* const add1 = reinterpret_cast<float*>(static_cast<std::uintptr_t>(a5[1]));
    if (!high0 || !high1 || !add0 || !add1)
        return 0;
    if (!asc4he_butterworth_crossover_float_process_u64(a1 + 4u, a2[0], a3[0], a4[0]))
        return 0;
    if (!asc4he_butterworth_crossover_float_process_u64(a1 + 64u, a2[1], a3[1], a4[1]))
        return 0;
    asc4he_add_pair_32(add0, add1, high0, high1);
    return 1;
}

std::int64_t auro_asc4he_v1_sb_SurroundSatellites_reset_audio_state_542920_partial(std::uint8_t* a1) {
    if (!a1)
        return 0;
    auro_asc4he_v1_ElevationEQ_reset_audio_state_108fc0_partial(a1 + 8u);
    return 0;
}

std::int64_t auro_asc4he_v1_sb_SurroundSatellites_initialize_542930_partial(
    std::uint8_t* a1,
    const std::uint8_t* a2,
    std::uint32_t a3) {
    if (!a1 || !a2)
        return 1;
    *reinterpret_cast<float*>(a1 + 4u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 56u));
    *reinterpret_cast<std::uint32_t*>(a1 + 0u) = 1065353216u;
    return auro_asc4he_v1_ElevationEQ_initialize_108fc0_partial(a1 + 8u, a3);
}

std::int64_t auro_asc4he_v1_sb_SurroundSatellites_process_2_2_542980_partial(
    float* a1,
    std::uint64_t* a2) {
    if (!a1 || !a2)
        return 0;
    auto* const v3 = reinterpret_cast<float*>(static_cast<std::uintptr_t>(a2[6]));
    auto* const v4 = reinterpret_cast<float*>(static_cast<std::uintptr_t>(a2[7]));
    const auto* const v5 = reinterpret_cast<const float*>(static_cast<std::uintptr_t>(a2[15]));
    const auto* const v6 = reinterpret_cast<const float*>(static_cast<std::uintptr_t>(a2[16]));
    auto* const v11 = reinterpret_cast<float*>(static_cast<std::uintptr_t>(a2[23]));
    auto* const v12 = reinterpret_cast<float*>(static_cast<std::uintptr_t>(a2[24]));
    asc4he_scale_pair_32(v3, v4, a1[0]);
    asc4he_copy_pair_scaled_32(v11, v12, v5, v6, a1[1]);
    std::uint64_t pair[2]{
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(v11)),
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(v12)),
    };
    return auro_asc4he_v1_ElevationEQ_process_1090c0_partial(a1 + 2, pair);
}

void auro_asc4he_v1_sb_SurroundSatellites_process_2_0_543190_partial(float* a1, std::uint64_t* a2) {
    if (!a1 || !a2)
        return;
    asc4he_scale_existing_pair_32(a2[6], a2[7], a1[0]);
}

std::int64_t auro_asc4he_v1_sb_HeightSatellites_reset_audio_state_5431b0_partial(std::uint8_t* a1) {
    if (!a1)
        return 0;
    auro_asc4he_v1_Crossover_reset_audio_state_5428e0_partial(a1 + 3368u);
    auro_asc4he_v1_Delay7ms_reset_audio_state_545350_partial(a1);
    auro_asc4he_v1_CrossTalkCompensation_reset_audio_state_546690_partial(a1 + 3876u);
    auro_asc4he_v1_CrossTalk_reset_audio_state_547520_partial(a1 + 3988u);
    return 0;
}

std::int64_t auro_asc4he_v1_sb_HeightSatellites_initialize_5431f0_partial(
    std::uint8_t* a1,
    const std::uint8_t* a2,
    std::uint32_t a3,
    std::uint32_t a4) {
    if (!a1 || !a2)
        return 1;
    std::int64_t result = auro_asc4he_v1_Delay7ms_initialize_545310_partial(a1, a3);
    if (result)
        return result;
    result = auro_asc4he_v1_Crossover_initialize_541f60_partial(a1 + 3368u, a3, static_cast<std::int32_t>(a4), 1);
    if (result)
        return result;
    result = auro_asc4he_v1_CrossTalkCompensation_initialize_5465e0_partial(a1 + 3876u, a2 + 80u, a3);
    if (result)
        return result;
    result = auro_asc4he_v1_CrossTalk_initialize_5474a0_partial(
        a1 + 3988u,
        reinterpret_cast<const float*>(a2 + 132u));
    if (result)
        return result;
    *reinterpret_cast<float*>(a1 + 3360u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 60u));
    *reinterpret_cast<std::uint32_t*>(a1 + 3872u) = 1068813832u;
    return 0;
}

std::int64_t auro_asc4he_v1_sb_HeightSatellites_process_2_0_5432b0_partial(
    std::uint8_t* a1,
    std::uint64_t* a2,
    std::uint8_t*,
    float* a4) {
    if (!a1 || !a2 || !a4)
        return 0;
    auto* const v4 = reinterpret_cast<float*>(static_cast<std::uintptr_t>(a2[11]));
    auto* const v5 = reinterpret_cast<float*>(static_cast<std::uintptr_t>(a2[12]));
    const auto* const v6 = reinterpret_cast<const float*>(static_cast<std::uintptr_t>(a2[15]));
    const auto* const v7 = reinterpret_cast<const float*>(static_cast<std::uintptr_t>(a2[16]));
    float* const lo0 = a4;
    float* const lo1 = a4 + 32;
    float* const hi0 = a4 + 64;
    float* const hi1 = a4 + 96;
    asc4he_copy_pair_scaled_32(lo0, lo1, v6, v7, *reinterpret_cast<float*>(a1 + 3360u));
    std::uint64_t lo_pair[2]{
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(lo0)),
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(lo1)),
    };
    std::uint64_t hi_pair[2]{
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(hi0)),
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(hi1)),
    };
    (void)auro_asc4he_v1_Crossover_process_542910_partial(a1 + 3368u, lo_pair, lo_pair, hi_pair);
    (void)auro_asc4he_v1_CrossTalkCompensation_process_5466a0_partial(
        reinterpret_cast<float*>(a1 + 3876u),
        reinterpret_cast<float**>(hi_pair));
    (void)auro_asc4he_v1_CrossTalk_process_547540_partial(a1 + 3988u, hi_pair);
    asc4he_add_pair_32(lo0, lo1, hi0, hi1);
    (void)auro_asc4he_v1_Delay7ms_process_545360_partial(a1, reinterpret_cast<std::uint8_t*>(lo_pair));
    asc4he_add_pair_32(v4, v5, lo0, lo1);
    return static_cast<std::int64_t>(lo_pair[1]);
}

std::int64_t auro_asc4he_v1_sb_HeightSatellites_process_2_2_543b30_partial(
    std::uint8_t* a1,
    std::uint8_t* a2,
    std::uint8_t* a3,
    float* a4) {
    (void)a3;
    if (!a1 || !a2 || !a4)
        return 0;
    auto* const ext0 = reinterpret_cast<float*>(*reinterpret_cast<std::uint64_t*>(a2 + 144u));
    auto* const ext1 = reinterpret_cast<float*>(*reinterpret_cast<std::uint64_t*>(a2 + 152u));
    auto* const v4 = reinterpret_cast<float*>(*reinterpret_cast<std::uint64_t*>(a2 + 88u));
    auto* const v5 = reinterpret_cast<float*>(*reinterpret_cast<std::uint64_t*>(a2 + 96u));
    const auto* const v6 = reinterpret_cast<const float*>(*reinterpret_cast<std::uint64_t*>(a2 + 120u));
    const auto* const v7 = reinterpret_cast<const float*>(*reinterpret_cast<std::uint64_t*>(a2 + 128u));
    float* const lo0 = a4;
    float* const lo1 = a4 + 32;
    float* const hi0 = a4 + 64;
    float* const hi1 = a4 + 96;
    asc4he_copy_pair_scaled_32(lo0, lo1, v6, v7, *reinterpret_cast<float*>(a1 + 3360u));
    std::uint64_t lo_pair[2]{
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(lo0)),
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(lo1)),
    };
    std::uint64_t hi_pair[2]{
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(hi0)),
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(hi1)),
    };
    (void)auro_asc4he_v1_Crossover_process_542910_partial(a1 + 3368u, lo_pair, lo_pair, hi_pair);
    const float g = *reinterpret_cast<const float*>(a1 + 3872u);
    asc4he_scale_pair_32(ext0, ext1, g);
    asc4he_add_pair_32(ext0, ext1, hi0, hi1);
    (void)auro_asc4he_v1_CrossTalkCompensation_process_5466a0_partial(
        reinterpret_cast<float*>(a1 + 3876u),
        reinterpret_cast<float**>(hi_pair));
    (void)auro_asc4he_v1_CrossTalk_process_547540_partial(a1 + 3988u, hi_pair);
    asc4he_add_pair_32(lo0, lo1, hi0, hi1);
    (void)auro_asc4he_v1_Delay7ms_process_545360_partial(a1, reinterpret_cast<std::uint8_t*>(lo_pair));
    asc4he_add_pair_32(v4, v5, lo0, lo1);
    return static_cast<std::int64_t>(lo_pair[1]);
}

std::int64_t auro_asc4he_v1_CGenXOverBlock_initialize_547cb0_partial(
    std::uint8_t* a1,
    std::uint32_t a2,
    std::uint32_t a3,
    const std::uint8_t* a4,
    const std::uint8_t* a5) {
    if (!a1)
        return 1;
    *reinterpret_cast<std::uint32_t*>(a1 + 0u) = 0u;
    std::memset(a1 + 1636u, 0, 16u);
    *reinterpret_cast<std::uint32_t*>(a1 + 1652u) = 0u;
    std::int64_t result = auro_asc4he_v1_CenterGen_initialize_547a40_partial(a1 + 512u, a2, a4, a5);
    if (result)
        return result;
    result = auro_asc4he_v1_Crossover_initialize_541f60_partial(a1 + 8u, a2, static_cast<std::int32_t>(a3), 1);
    if (result)
        return result;
    return auro_asc4he_v1_CenterCrossOver_initialize_547af0_partial(a1 + 1576u, a2, static_cast<std::int32_t>(a3));
}

std::int64_t auro_asc4he_v1_CGenXOverBlock_get_cgen_dynamic_parameters_547d40_partial(
    const std::uint8_t* a1,
    std::uint8_t* out56) {
    if (!a1)
        return 0;
    return auro_asc4he_v1_CenterGen_get_dynamic_parameters_5479a0_partial(a1 + 512u, out56);
}

std::int64_t auro_asc4he_v1_CGenXOverBlock_set_cgen_dynamic_parameters_547d50_partial(
    std::uint8_t* a1,
    const std::uint8_t* dyn56) {
    if (!a1)
        return 0;
    return auro_asc4he_v1_CenterGen_set_dynamic_parameters_5479d0_partial(a1 + 512u, dyn56);
}

std::int64_t auro_asc4he_v1_CGenXOverBlock_reset_audio_state_547d60_partial(std::uint8_t* a1) {
    if (!a1)
        return 0;
    auro_asc4he_v1_Crossover_reset_audio_state_5428e0_partial(a1 + 8u);
    (void)auro_asc4he_v1_CenterGen_reset_audio_state_6427f0_partial(a1 + 512u);
    auro_asc4he_v1_CenterCrossOver_reset_audio_state_547bb0_partial(a1 + 1576u);
    return 0;
}

float* auro_asc4he_v1_CGenXOverBlock_process_with_external_center_547da0_partial(
    std::uint8_t* a1,
    float** a2,
    float* a3,
    float** a4,
    float** a5,
    std::uint32_t,
    std::uint64_t,
    float* a8) {
    if (!a1 || !a2 || !a2[0] || !a2[1] || !a3 || !a8)
        return nullptr;
    float* const decor0 = a8;
    float* const decor1 = a8 + 32;
    float* const center = a8 + 64;
    asc4he_scale_pair_32(a2[0], a2[1], *reinterpret_cast<float*>(a1 + 0u));
    std::uint64_t io_pair[2]{
        asc4he_u64_ptr(a2[0]),
        asc4he_u64_ptr(a2[1]),
    };
    (void)auro_asc4he_v1_CenterGen_process_547ad0_partial(a1 + 512u, io_pair, reinterpret_cast<std::uint8_t*>(center));
    float* low_pair[2]{
        a4 ? a4[0] : nullptr,
        a4 ? a4[1] : nullptr,
    };
    std::uint64_t low_u64[2]{
        asc4he_u64_ptr(low_pair[0]),
        asc4he_u64_ptr(low_pair[1]),
    };
    std::uint64_t mix_u64[2]{
        asc4he_u64_ptr(a5 ? a5[0] : nullptr),
        asc4he_u64_ptr(a5 ? a5[1] : nullptr),
    };
    std::uint64_t decor_pair[2]{
        asc4he_u64_ptr(decor0),
        asc4he_u64_ptr(decor1),
    };
    (void)auro_asc4he_v1_Crossover_process_542910_partial(a1 + 8u, io_pair, low_u64, mix_u64, decor_pair);
    for (std::uint32_t i = 0; i != 32u; ++i)
        center[i] += a3[i];
    (void)auro_asc4he_v1_CenterCrossOver_process_547bd0_partial(
        a1 + 1576u,
        asc4he_u64_ptr(center),
        asc4he_u64_ptr(center));
    const float v11 = *reinterpret_cast<const float*>(a1 + 1636u);
    const float v12 = *reinterpret_cast<const float*>(a1 + 1640u);
    const float v14 = *reinterpret_cast<const float*>(a1 + 1644u);
    const float v13 = *reinterpret_cast<const float*>(a1 + 1648u);
    for (std::uint32_t i = 0; i != 32u; ++i) {
        a2[0][i] = (a5 && a5[0] ? a5[0][i] : 0.f) * v12 + decor0[i] * v13;
        a2[1][i] = (a5 && a5[1] ? a5[1][i] : 0.f) * v12 + decor1[i] * v13;
        a3[i] = center[i] * v14;
    }
    if (*reinterpret_cast<const std::uint32_t*>(a1 + 1652u) != 0u)
        asc4he_add_scaled_pair_32(a2[0], a2[1], low_pair[0], low_pair[1], v11);
    else if (low_pair[0] && low_pair[1])
        asc4he_scale_pair_32(low_pair[0], low_pair[1], v11);
    return a2[1];
}

float* auro_asc4he_v1_CGenXOverBlock_process_without_external_center_548b00_partial(
    std::uint8_t* a1,
    float** a2,
    float** a3,
    float** a4,
    std::uint64_t,
    float* a6) {
    if (!a1 || !a2 || !a2[0] || !a2[1] || !a6)
        return nullptr;
    float* const decor0 = a6;
    float* const decor1 = a6 + 32;
    float* const center = a6 + 64;
    asc4he_scale_pair_32(a2[0], a2[1], *reinterpret_cast<float*>(a1 + 0u));
    std::uint64_t io_pair[2]{
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(a2[0])),
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(a2[1])),
    };
    (void)auro_asc4he_v1_CenterGen_process_547ad0_partial(a1 + 512u, io_pair, reinterpret_cast<std::uint8_t*>(center));
    std::uint64_t low_pair[2]{
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(a3 ? a3[0] : nullptr)),
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(a3 ? a3[1] : nullptr)),
    };
    std::uint64_t mix_pair[2]{
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(a4 ? a4[0] : nullptr)),
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(a4 ? a4[1] : nullptr)),
    };
    std::uint64_t decor_pair[2]{
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(decor0)),
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(decor1)),
    };
    (void)auro_asc4he_v1_Crossover_process_542910_partial(a1 + 8u, io_pair, low_pair, mix_pair, decor_pair);
    (void)auro_asc4he_v1_CenterCrossOver_process_547bd0_partial(
        a1 + 1576u,
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(center)),
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(center)));
    const float v21 = *reinterpret_cast<const float*>(a1 + 1636u);
    const float v22 = *reinterpret_cast<const float*>(a1 + 1640u);
    const float v24 = *reinterpret_cast<const float*>(a1 + 1644u);
    const float v23 = *reinterpret_cast<const float*>(a1 + 1648u);
    asc4he_mix_center_xover_outputs_32(a2[0], a2[1], a4 ? a4[0] : nullptr, a4 ? a4[1] : nullptr, center, decor0, decor1, v22, v24, v23);
    if (*reinterpret_cast<const std::uint32_t*>(a1 + 1652u) != 0u)
        asc4he_add_scaled_pair_32(a2[0], a2[1], a3 ? a3[0] : nullptr, a3 ? a3[1] : nullptr, v21);
    else if (a3 && a3[0] && a3[1])
        asc4he_scale_pair_32(a3[0], a3[1], v21);
    return a2[1];
}

bool is_median_symmetric_5a6590_partial(std::uint32_t a1) {
    const std::uint32_t v0 =
        ((a1 ^ (a1 >> 1u)) | ((a1 >> 4u) ^ (a1 >> 5u)) | ((a1 >> 7u) ^ (a1 >> 8u))
         | ((a1 >> 9u) ^ (a1 >> 10u)) | ((a1 >> 13u) ^ (a1 >> 14u)) | ((a1 >> 16u) ^ (a1 >> 17u))
         | ((a1 >> 18u) ^ (a1 >> 19u)))
        & 1u;
    return v0 == 0u && ((((a1 >> 22u) ^ (a1 >> 21u)) & 1u) == 0u);
}

std::int64_t auro_asc4he_v1_base_Processor_t_construct_53e630_partial(
    std::uint8_t* a1,
    const std::uint8_t* a2,
    const std::uint8_t* a3) {
    if (!a1 || !a2 || !a3)
        return 0;
    std::memset(a1, 0, kAsc4heBaseProcessorBytes);
    *reinterpret_cast<std::uint32_t*>(a1 + 4644u) = *reinterpret_cast<const std::uint32_t*>(a2 + 8u);
    *reinterpret_cast<std::uint32_t*>(a1 + 2696u) = *reinterpret_cast<const std::uint32_t*>(a2 + 12u);
    *reinterpret_cast<std::uint32_t*>(a1 + 2700u) = *reinterpret_cast<const std::uint32_t*>(a2 + 0u);
    *reinterpret_cast<std::uint32_t*>(a1 + 0u) = *reinterpret_cast<const std::uint32_t*>(a2 + 4u);
    *reinterpret_cast<std::uint32_t*>(a1 + 11368u) = *reinterpret_cast<const std::uint32_t*>(a2 + 16u);
    *reinterpret_cast<std::uint32_t*>(a1 + 4360u) = *reinterpret_cast<const std::uint32_t*>(a2 + 24u);
    *reinterpret_cast<std::uint32_t*>(a1 + 1808u) = *reinterpret_cast<const std::uint32_t*>(a2 + 28u);
    *reinterpret_cast<std::uint64_t*>(a1 + 14080u) = 0x53E730u;
    *reinterpret_cast<std::uint64_t*>(a1 + 14088u) = 0x540FC0u;
    *reinterpret_cast<std::uint64_t*>(a1 + 14096u) = 0x541090u;
    *reinterpret_cast<std::uint32_t*>(a1 + 14072u) = *reinterpret_cast<const std::uint32_t*>(a3 + 12u);
    *reinterpret_cast<std::uint32_t*>(a1 + 14076u) = *reinterpret_cast<const std::uint32_t*>(a3 + 0u);
    const std::uint32_t up0 = *reinterpret_cast<const std::uint32_t*>(a3 + 32u);
    const std::uint32_t up1 = *reinterpret_cast<const std::uint32_t*>(a3 + 40u);
    const std::uint32_t sr0 = *reinterpret_cast<const std::uint32_t*>(a3 + 36u);
    const std::uint32_t sr1 = *reinterpret_cast<const std::uint32_t*>(a3 + 44u);
    if (auro_asc4he_v1_SideUpCrossover_initialize_5419a0_partial(
            a1 + 13792u,
            static_cast<std::int32_t>(up0),
            sr0,
            *reinterpret_cast<const std::uint32_t*>(a2 + 12u)))
        return auro_asc4he_v1_SideUpCrossover_initialize_5419a0_partial(
                   a1 + 13916u,
                   static_cast<std::int32_t>(up1),
                   sr1,
                   *reinterpret_cast<const std::uint32_t*>(a2 + 12u))
            != 0;
    return 0;
}

std::int64_t auro_asc4he_v1_base_Processor_default_process_53e730_partial(
    std::uint8_t* a1,
    std::uint64_t a2,
    std::uint64_t a3,
    std::uint64_t a4) {
    if (!a1 || a2 == 0u || a4 == 0u || a3 < 2432u)
        return 0;
    auto* const table = reinterpret_cast<std::uint64_t*>(static_cast<std::uintptr_t>(a2));
    auto* const work = reinterpret_cast<float*>(static_cast<std::uintptr_t>(a4));
    std::memset(work, 0, 2432u);

    float* front_pair[2]{
        asc4he_float_ptr(table[2]),
        asc4he_float_ptr(table[3]),
    };
    float* const center = asc4he_float_ptr(table[4]);
    float* front_work[2]{work, work + 32};
    float* main_low[2]{work + 64, work + 96};
    float* main_surround[2]{work + 128, work + 160};
    float* height_mix[2]{work + 192, work + 224};
    float* delayed_height[2]{work + 256, work + 288};
    float* side_pair[2]{work + 320, work + 352};
    float* side_hi[2]{work + 384, work + 416};
    float* const cgen_work = work + 512;

    (void)auro_asc4he_v1_CGenXOverBlock_process_with_external_center_547da0_partial(
        a1 + 12136u,
        front_pair,
        center,
        main_low,
        front_work,
        0,
        a3 - 512u,
        work + 128);

    asc4he_copy_pair_32(side_pair[0], side_pair[1], asc4he_float_ptr(table[6]), asc4he_float_ptr(table[7]));
    (void)auro_asc4he_v1_CGenXOverBlock_process_without_external_center_548b00_partial(
        a1 + 2704u,
        side_pair,
        side_hi,
        main_surround,
        a3 - 2048u,
        cgen_work);
    if (*reinterpret_cast<const std::uint32_t*>(a1 + 4360u) == 0u) {
        std::uint64_t main_surround_u64[2]{
            asc4he_u64_ptr(main_surround[0]),
            asc4he_u64_ptr(main_surround[1]),
        };
        (void)auro_asc4he_v1_Decorrelator_process_546110_partial(
            reinterpret_cast<float*>(a1 + 4364u),
            main_surround_u64);
    }

    const std::uint32_t surround_mode = *reinterpret_cast<const std::uint32_t*>(a1 + 4644u);
    if (surround_mode == 1u) {
        asc4he_add_pair_32(side_pair[0], side_pair[1], side_hi[0], side_hi[1]);
        asc4he_copy_pair_32(asc4he_float_ptr(table[6]), asc4he_float_ptr(table[7]), side_pair[0], side_pair[1]);
    } else if (surround_mode != 0u) {
        asc4he_add_pair_32(main_low[0], main_low[1], side_hi[0], side_hi[1]);
        asc4he_add_pair_32(delayed_height[0], delayed_height[1], side_pair[0], side_pair[1]);
    } else {
        asc4he_add_pair_32(side_pair[0], side_pair[1], side_hi[0], side_hi[1]);
    }

    std::uint64_t side_pair_u64[2]{asc4he_u64_ptr(side_pair[0]), asc4he_u64_ptr(side_pair[1])};
    if (surround_mode == 0u) {
        float* sideup_low[2]{work + 448, work + 480};
        float* sideup_high[2]{asc4he_float_ptr(table[9]), asc4he_float_ptr(table[10])};
        std::uint64_t sideup_low_u64[2]{asc4he_u64_ptr(sideup_low[0]), asc4he_u64_ptr(sideup_low[1])};
        std::uint64_t sideup_high_u64[2]{asc4he_u64_ptr(sideup_high[0]), asc4he_u64_ptr(sideup_high[1])};
        std::uint64_t sideup_add_u64[2]{asc4he_u64_ptr(main_low[0]), asc4he_u64_ptr(main_low[1])};
        if (!auro_asc4he_v1_SideUpCrossover_process_541a60_partial(
                a1 + 13792u,
                side_pair_u64,
                sideup_low_u64,
                sideup_high_u64,
                sideup_add_u64))
            return 1;
    }

    asc4he_copy_pair_32(side_pair[0], side_pair[1], asc4he_float_ptr(table[11]), asc4he_float_ptr(table[12]));
    side_pair_u64[0] = asc4he_u64_ptr(side_pair[0]);
    side_pair_u64[1] = asc4he_u64_ptr(side_pair[1]);
    (void)auro_asc4he_v1_ElevationEQ_process_1090c0_partial(reinterpret_cast<float*>(a1 + 1664u), side_pair_u64);
    (void)auro_asc4he_v1_CGenXOverBlock_process_without_external_center_548b00_partial(
        a1 + 8u,
        side_pair,
        side_hi,
        height_mix,
        a3 - 2048u,
        cgen_work);
    if (*reinterpret_cast<const std::uint32_t*>(a1 + 1808u) == 0u) {
        void* vh_pair[2]{height_mix[0], height_mix[1]};
        (void)auro_asc4he_v1_VirtualHeight_process_10cdc0_partial(a1 + 1816u, vh_pair);
    }

    const std::uint32_t height_mode = *reinterpret_cast<const std::uint32_t*>(a1 + 2696u);
    if (height_mode == 0u) {
        if ((*reinterpret_cast<const std::uint32_t*>(a1 + 14076u) - 1u) > 1u || !table[20] || !table[21])
            return 1;
        asc4he_add_pair_32(side_pair[0], side_pair[1], side_hi[0], side_hi[1]);
        float* sideup_in[2]{asc4he_float_ptr(table[20]), asc4he_float_ptr(table[21])};
        float* sideup_hi[2]{work + 448, work + 480};
        std::uint64_t sideup_hi_u64[2]{asc4he_u64_ptr(sideup_hi[0]), asc4he_u64_ptr(sideup_hi[1])};
        std::uint64_t sideup_low_u64[2]{asc4he_u64_ptr(sideup_in[0]), asc4he_u64_ptr(sideup_in[1])};
        std::uint64_t sideup_add_u64[2]{asc4he_u64_ptr(main_low[0]), asc4he_u64_ptr(main_low[1])};
        if (!auro_asc4he_v1_SideUpCrossover_process_541a60_partial(
                a1 + 13916u,
                side_pair_u64,
                sideup_hi_u64,
                sideup_low_u64,
                sideup_add_u64))
            return 1;
    } else if (height_mode == 2u) {
        asc4he_add_pair_32(side_pair[0], side_pair[1], side_hi[0], side_hi[1]);
        if (auto* const mono = asc4he_float_ptr(table[14])) {
            for (std::uint32_t i = 0; i != 32u; ++i)
                mono[i] += side_pair[0][i] + side_pair[1][i];
        }
    } else {
        asc4he_add_pair_32(main_low[0], main_low[1], side_hi[0], side_hi[1]);
        asc4he_add_pair_32(delayed_height[0], delayed_height[1], side_pair[0], side_pair[1]);
    }

    asc4he_copy_pair_scaled_32(
        side_pair[0],
        side_pair[1],
        main_surround[0],
        main_surround[1],
        *reinterpret_cast<const float*>(a1 + 14056u));
    asc4he_add_scaled_pair_32(
        side_pair[0],
        side_pair[1],
        height_mix[0],
        height_mix[1],
        *reinterpret_cast<const float*>(a1 + 14060u));
    std::uint64_t tail_pair_u64[2]{asc4he_u64_ptr(side_pair[0]), asc4he_u64_ptr(side_pair[1])};
    (void)auro_asc4he_v1_Delay7ms_process_545360_partial(a1 + 4648u, reinterpret_cast<std::uint8_t*>(tail_pair_u64));
    asc4he_add_scaled_pair_32(
        side_pair[0],
        side_pair[1],
        front_work[0],
        front_work[1],
        *reinterpret_cast<const float*>(a1 + 14052u));
    (void)auro_asc4he_v1_CrossTalkCompensation_process_5466a0_partial(
        reinterpret_cast<float*>(a1 + 11964u),
        side_pair);
    (void)auro_asc4he_v1_CrossTalk_process_547540_partial(a1 + 11880u, tail_pair_u64);
    asc4he_add_scaled_pair_32(
        front_pair[0],
        front_pair[1],
        side_pair[0],
        side_pair[1],
        *reinterpret_cast<const float*>(a1 + 14048u));

    std::uint64_t delayed_u64[2]{asc4he_u64_ptr(delayed_height[0]), asc4he_u64_ptr(delayed_height[1])};
    (void)auro_asc4he_v1_Delay7ms_process_545360_partial(a1 + 8008u, reinterpret_cast<std::uint8_t*>(delayed_u64));
    asc4he_add_pair_32(delayed_height[0], delayed_height[1], main_low[0], main_low[1]);
    asc4he_add_pair_32(front_pair[0], front_pair[1], delayed_height[0], delayed_height[1]);
    if (*reinterpret_cast<const std::uint32_t*>(a1 + 11368u) != 0u)
        asc4he_add_scaled_mono_to_pair_32(
            front_pair[0],
            front_pair[1],
            center,
            *reinterpret_cast<const float*>(a1 + 14044u));
    return 0;
}

std::int64_t auro_asc4he_v1_base_Processor_default_reset_audio_state_540fc0_partial(std::uint8_t* a1) {
    if (!a1)
        return 0;
    auro_asc4he_v1_CGenXOverBlock_reset_audio_state_547d60_partial(a1 + 2704u);
    if (*reinterpret_cast<const std::uint32_t*>(a1 + 4360u) == 0u)
        auro_asc4he_v1_Decorrelator_reset_audio_state_545a0_partial(a1 + 4364u);
    auro_asc4he_v1_CGenXOverBlock_reset_audio_state_547d60_partial(a1 + 12136u);
    auro_asc4he_v1_CGenXOverBlock_reset_audio_state_547d60_partial(a1 + 8u);
    auro_asc4he_v1_ElevationEQ_reset_audio_state_108fc0_partial(a1 + 1664u);
    if (*reinterpret_cast<const std::uint32_t*>(a1 + 1808u) == 0u)
        auro_asc4he_v1_VirtualHeight_reset_audio_state_10ccf0_partial(a1 + 1816u);
    auro_asc4he_v1_Delay7ms_reset_audio_state_545350_partial(a1 + 4648u);
    auro_asc4he_v1_Delay7ms_reset_audio_state_545350_partial(a1 + 8008u);
    auro_asc4he_v1_Crossover_reset_audio_state_5428e0_partial(a1 + 11376u);
    auro_asc4he_v1_CenterCrossOver_reset_audio_state_547bb0_partial(a1 + 12076u);
    auro_asc4he_v1_CrossTalkCompensation_reset_audio_state_546690_partial(a1 + 11964u);
    auro_asc4he_v1_CrossTalk_reset_audio_state_547520_partial(a1 + 11880u);
    auro_asc4he_v1_SideUpCrossover_reset_audio_state_541a30_partial(a1 + 13792u);
    return auro_asc4he_v1_SideUpCrossover_reset_audio_state_541a30_partial(a1 + 13916u);
}

std::int64_t auro_asc4he_v1_base_Processor_default_initialize_541090_partial(
    std::uint8_t* a1,
    const std::uint8_t* a2,
    std::uint32_t a3) {
    if (!a1 || !a2)
        return 1;
    *reinterpret_cast<std::uint32_t*>(a1 + 14072u) = a3;
    std::uint8_t v38[kAsc4heCenterGenDynamicDefaultsBytes]{};
    std::uint8_t v39[kAsc4heCenterGenFixedDefaultsBytes]{};
    auro_asc4he_v1_CenterGen_get_default_fixed_parameters_5479f0_partial(v39);
    auro_asc4he_v1_CenterGen_get_default_dynamic_parameters_547a10_partial(v38);
    std::int64_t result = auro_asc4he_v1_CGenXOverBlock_initialize_547cb0_partial(
        a1 + 2704u,
        a3,
        *reinterpret_cast<const std::uint32_t*>(a1 + 2700u),
        v39,
        v38);
    if (result)
        return result;
    result = auro_asc4he_v1_CGenXOverBlock_initialize_547cb0_partial(
        a1 + 8u,
        a3,
        *reinterpret_cast<const std::uint32_t*>(a1 + 0u),
        v39,
        v38);
    if (result)
        return result;
    result = auro_asc4he_v1_Crossover_initialize_541f60_partial(a1 + 11376u, a3, 1, 1);
    if (result)
        return result;
    result = auro_asc4he_v1_CrossTalkCompensation_initialize_5465e0_partial(a1 + 11964u, a2 + 80u, a3);
    if (result)
        return result;
    result = auro_asc4he_v1_CrossTalk_initialize_5474a0_partial(
        a1 + 11880u,
        reinterpret_cast<const float*>(a2 + 132u));
    if (result)
        return result;
    result = auro_asc4he_v1_CenterCrossOver_initialize_547af0_partial(a1 + 12076u, a3, 1);
    if (result)
        return result;
    result = auro_asc4he_v1_CGenXOverBlock_initialize_547cb0_partial(
        a1 + 12136u,
        a3,
        1u,
        a2 + 140u,
        a2 + 160u);
    if (result)
        return result;
    *reinterpret_cast<float*>(a1 + 12136u) = 1.f;
    *reinterpret_cast<float*>(a1 + 13772u) = 1.f;
    *reinterpret_cast<float*>(a1 + 13776u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 0u));
    *reinterpret_cast<float*>(a1 + 13780u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 72u));
    *reinterpret_cast<float*>(a1 + 13784u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 76u));
    *reinterpret_cast<std::uint32_t*>(a1 + 13788u) = 1u;
    result = auro_asc4he_v1_ElevationEQ_initialize_108fc0_partial(a1 + 1664u, a3);
    if (result)
        return result;
    if (*reinterpret_cast<const std::uint32_t*>(a1 + 1808u) == 0u) {
        result = auro_asc4he_v1_VirtualHeight_initialize_10ccf0_partial(a1 + 1816u, a3);
        if (result)
            return result;
    }
    if (*reinterpret_cast<const std::uint32_t*>(a1 + 4360u) == 0u) {
        result = auro_asc4he_v1_Decorrelator_initialize_545f10_partial(a1 + 4364u, a3);
        if (result)
            return result;
    }
    result = auro_asc4he_v1_Delay7ms_initialize_545310_partial(a1 + 4648u, a3);
    if (result)
        return result;
    result = auro_asc4he_v1_Delay7ms_initialize_545310_partial(a1 + 8008u, a3);
    if (result)
        return result;
    *reinterpret_cast<float*>(a1 + 2704u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 28u));
    *reinterpret_cast<float*>(a1 + 4340u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 12u));
    *reinterpret_cast<float*>(a1 + 4344u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 16u));
    *reinterpret_cast<float*>(a1 + 4348u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 20u));
    *reinterpret_cast<float*>(a1 + 4352u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 24u));
    *reinterpret_cast<float*>(a1 + 4356u) = 0.f;
    *reinterpret_cast<float*>(a1 + 8u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 52u));
    *reinterpret_cast<float*>(a1 + 1644u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 36u));
    *reinterpret_cast<float*>(a1 + 1648u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 40u));
    *reinterpret_cast<float*>(a1 + 1652u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 44u));
    *reinterpret_cast<float*>(a1 + 1656u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 48u));
    *reinterpret_cast<float*>(a1 + 1660u) = 0.f;
    *reinterpret_cast<float*>(a1 + 14040u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 0u));
    *reinterpret_cast<float*>(a1 + 14052u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 4u));
    *reinterpret_cast<float*>(a1 + 14056u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 8u));
    *reinterpret_cast<float*>(a1 + 14060u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 32u));
    *reinterpret_cast<float*>(a1 + 14048u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 64u));
    *reinterpret_cast<float*>(a1 + 14044u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 68u));
    *reinterpret_cast<float*>(a1 + 14064u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 72u));
    *reinterpret_cast<float*>(a1 + 14068u) = asc4he_db_gain(*reinterpret_cast<const float*>(a2 + 76u));
    return 0;
}

std::int64_t auro_asc4he_v1_sb_Processor_t_is_supported_53e0e0_partial(std::uint32_t a1) {
    std::uint32_t v1 = 0u;
    if ((a1 & 0xFF80F840u) == 0u && is_median_symmetric_5a6590_partial(a1)) {
        const std::uint32_t v3 = a1 & 0x600u;
        if (v3 != 1536u || (a1 & 0x600030u) == 0u) {
            const std::uint32_t v4 = a1 & 0x30u;
            if (v4 != 48u || (a1 & 0x30600u) == 0u) {
                const std::uint32_t v6 = ~a1;
                if (v3 == 1536u || (v6 & 0x30000u) != 0u) {
                    if ((v6 & 3u) == 0u && (v4 == 48u || (v6 & 0x600000u) != 0u))
                        return (a1 & 0x100008u) != 0x100000u;
                }
            }
        }
    }
    return v1;
}

std::int64_t auro_asc4he_v1_sb_Processor_t_get_required_input_layout_53e1a0_partial(
    std::uint32_t a1,
    std::uint32_t* a2) {
    if (!a2)
        return 277;
    if ((a1 & 0xFF80F840u) == 0u && is_median_symmetric_5a6590_partial(a1)) {
        const std::uint32_t v6 = a1 & 0x600u;
        const std::uint32_t v8 = a1 & 0x30u;
        if (v6 != 1536u || (a1 & 0x600030u) == 0u) {
            if ((v8 != 48u || (a1 & 0x30600u) == 0u) && (v6 == 1536u || (~a1 & 0x30000u) != 0u)) {
                if ((a1 & 0x100008u) != 0x100000u && (a1 & 3u) == 3u && (v8 == 48u || (~a1 & 0x600000u) != 0u)) {
                    std::uint32_t v11 = v6 == 1536u ? 26167u : 1591u;
                    if ((a1 & 0x600030u) == 0x600030u)
                        v11 = 26167u;
                    *a2 = (a1 & 0x100008u) | v11;
                    return 0;
                }
            }
        }
    }
    return 277;
}

std::int64_t auro_asc4he_v1_sb_Processor_t_construct_53e2a0_partial(
    std::uint8_t* a1,
    const std::uint8_t* a2) {
    if (!a1 || !a2)
        return 0;
    const std::uint32_t v2 = *reinterpret_cast<const std::uint32_t*>(a2 + 8u);
    alignas(8) std::uint8_t v9[32]{};
    *reinterpret_cast<std::uint32_t*>(v9 + 0u) = ((v2 & 0x180u) == 0x180u) ? 1u : 0u;
    *reinterpret_cast<std::uint32_t*>(v9 + 4u) = ((v2 & 0xC0000u) == 0xC0000u) ? 1u : 0u;
    *reinterpret_cast<std::uint32_t*>(v9 + 8u) = 2u * (((v2 & 0x180u) != 0x180u) ? 1u : 0u);
    *reinterpret_cast<std::uint32_t*>(v9 + 12u) = ((v2 & 0xC0000u) != 0xC0000u) ? 1u : 0u;
    *reinterpret_cast<std::uint32_t*>(v9 + 16u) = (v2 & 4u) == 0u;
    if (!auro_asc4he_v1_base_Processor_t_construct_53e630_partial(a1, v9, a2))
        return 0;
    *reinterpret_cast<std::uint32_t*>(a1 + 18192u) = (~v2 & 0x30000u) == 0u;
    *reinterpret_cast<std::uint64_t*>(a1 + 18176u) = 0u;
    *reinterpret_cast<std::uint64_t*>(a1 + 18184u) = 0u;
    const std::uint32_t v5 = ~v2;
    if ((v5 & 0x30u) == 0u) {
        *reinterpret_cast<std::uint64_t*>(a1 + 18176u) = reinterpret_cast<std::uint64_t>(a1 + 14104u);
        *reinterpret_cast<std::uint64_t*>(a1 + 14088u) = 0x53E410u;
        *reinterpret_cast<std::uint64_t*>(a1 + 14096u) = 0x53E430u;
        *reinterpret_cast<std::uint64_t*>(a1 + 14080u) = (v5 & 0x600000u) == 0u ? 0x53E480u : 0x53E4D0u;
    }
    if ((v5 & 0x600u) == 0u) {
        *reinterpret_cast<std::uint64_t*>(a1 + 18184u) = reinterpret_cast<std::uint64_t>(a1 + 14104u);
        *reinterpret_cast<std::uint64_t*>(a1 + 14088u) = 0x53E520u;
        *reinterpret_cast<std::uint64_t*>(a1 + 14096u) = 0x53E540u;
        *reinterpret_cast<std::uint64_t*>(a1 + 14080u) = (v5 & 0x30000u) == 0u ? 0x53E590u : 0x53E5E0u;
    }
    return 1;
}

std::int64_t auro_asc4he_v1_sb_Processor_surround_reset_53e410_partial(std::uint8_t* a1) {
    auro_asc4he_v1_base_Processor_default_reset_audio_state_540fc0_partial(a1);
    if (!a1)
        return 0;
    return auro_asc4he_v1_sb_SurroundSatellites_reset_audio_state_542920_partial(
        reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(*reinterpret_cast<std::uint64_t*>(a1 + 18176u))));
}

std::int64_t auro_asc4he_v1_sb_Processor_surround_initialize_53e430_partial(
    std::uint8_t* a1,
    const std::uint8_t* a2,
    std::uint32_t a3) {
    const std::int64_t result = auro_asc4he_v1_base_Processor_default_initialize_541090_partial(a1, a2, a3);
    if (result || !a1)
        return result;
    return auro_asc4he_v1_sb_SurroundSatellites_initialize_542930_partial(
        reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(*reinterpret_cast<std::uint64_t*>(a1 + 18176u))),
        a2,
        a3);
}

std::int64_t auro_asc4he_v1_sb_Processor_surround_process_2_2_53e480_partial(
    std::uint8_t* a1,
    std::uint64_t a2,
    std::uint64_t a3,
    std::uint64_t a4) {
    const std::int64_t result = auro_asc4he_v1_base_Processor_default_process_53e730_partial(a1, a2, a3, a4);
    if (!result && a1)
        (void)auro_asc4he_v1_sb_SurroundSatellites_process_2_2_542980_partial(
            reinterpret_cast<float*>(static_cast<std::uintptr_t>(*reinterpret_cast<std::uint64_t*>(a1 + 18176u))),
            reinterpret_cast<std::uint64_t*>(static_cast<std::uintptr_t>(a2)));
    return result;
}

std::int64_t auro_asc4he_v1_sb_Processor_surround_process_2_0_53e4d0_partial(
    std::uint8_t* a1,
    std::uint64_t a2,
    std::uint64_t a3,
    std::uint64_t a4) {
    const std::int64_t result = auro_asc4he_v1_base_Processor_default_process_53e730_partial(a1, a2, a3, a4);
    if (!result && a1)
        auro_asc4he_v1_sb_SurroundSatellites_process_2_0_543190_partial(
            reinterpret_cast<float*>(static_cast<std::uintptr_t>(*reinterpret_cast<std::uint64_t*>(a1 + 18176u))),
            reinterpret_cast<std::uint64_t*>(static_cast<std::uintptr_t>(a2)));
    return result;
}

std::int64_t auro_asc4he_v1_sb_Processor_height_reset_53e520_partial(std::uint8_t* a1) {
    auro_asc4he_v1_base_Processor_default_reset_audio_state_540fc0_partial(a1);
    if (!a1)
        return 0;
    return auro_asc4he_v1_sb_HeightSatellites_reset_audio_state_5431b0_partial(
        reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(*reinterpret_cast<std::uint64_t*>(a1 + 18184u))));
}

std::int64_t auro_asc4he_v1_sb_Processor_height_initialize_53e540_partial(
    std::uint8_t* a1,
    const std::uint8_t* a2,
    std::uint32_t a3) {
    const std::int64_t result = auro_asc4he_v1_base_Processor_default_initialize_541090_partial(a1, a2, a3);
    if (result || !a1)
        return result;
    return auro_asc4he_v1_sb_HeightSatellites_initialize_5431f0_partial(
        reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(*reinterpret_cast<std::uint64_t*>(a1 + 18184u))),
        a2,
        a3,
        *reinterpret_cast<const std::uint32_t*>(a1 + 18192u));
}

std::int64_t auro_asc4he_v1_sb_Processor_height_process_2_2_53e590_partial(
    std::uint8_t* a1,
    std::uint64_t a2,
    std::uint64_t a3,
    std::uint64_t a4) {
    const std::int64_t result = auro_asc4he_v1_base_Processor_default_process_53e730_partial(a1, a2, a3, a4);
    if (!result && a1)
        (void)auro_asc4he_v1_sb_HeightSatellites_process_2_2_543b30_partial(
            reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(*reinterpret_cast<std::uint64_t*>(a1 + 18184u))),
            reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(a2)),
            reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(a3)),
            reinterpret_cast<float*>(static_cast<std::uintptr_t>(a4)));
    return result;
}

std::int64_t auro_asc4he_v1_sb_Processor_height_process_2_0_53e5e0_partial(
    std::uint8_t* a1,
    std::uint64_t a2,
    std::uint64_t a3,
    std::uint64_t a4) {
    const std::int64_t result = auro_asc4he_v1_base_Processor_default_process_53e730_partial(a1, a2, a3, a4);
    if (!result && a1)
        (void)auro_asc4he_v1_sb_HeightSatellites_process_2_0_5432b0_partial(
            reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(*reinterpret_cast<std::uint64_t*>(a1 + 18184u))),
            reinterpret_cast<std::uint64_t*>(static_cast<std::uintptr_t>(a2)),
            reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(a3)),
            reinterpret_cast<float*>(static_cast<std::uintptr_t>(a4)));
    return result;
}

std::int64_t auro_asc4he_v1_ss_Processor_t_is_supported_5445a0_partial(std::uint32_t a1) {
    const std::uint32_t v = a1 & 0xFFEFFFF7u;
    return (v == 4103u || v == 786439u) ? 1 : 0;
}

std::int64_t auro_asc4he_v1_ss_Processor_t_get_required_input_layout_5445c0_partial(
    std::uint32_t a1,
    std::uint32_t* a2) {
    if (!a2)
        return 3;
    const std::uint32_t v = a1 & 0xFFEFFFF7u;
    if (v == 0xC0007u || v == 0x1007u) {
        *a2 = (a1 & 0x100008u) | 0x637u;
        return 0;
    }
    return 279;
}

std::int64_t auro_asc4he_v1_ss_Processor_t_construct_5445f0_partial(std::uint8_t* a1, const std::uint8_t* a2) {
    if (!a1 || !a2)
        return 0;
    alignas(8) std::uint8_t v3[32]{};
    *reinterpret_cast<std::uint64_t*>(v3 + 0u) = 0x100000001ull;
    *reinterpret_cast<std::uint32_t*>(v3 + 8u) = 2u;
    *reinterpret_cast<std::uint32_t*>(v3 + 12u) =
        2u * (((~*reinterpret_cast<const std::uint32_t*>(a2 + 8u)) & 0xC0000u) != 0u);
    *reinterpret_cast<std::uint32_t*>(v3 + 16u) = 0u;
    *reinterpret_cast<std::uint32_t*>(v3 + 20u) = 1u;
    *reinterpret_cast<std::uint32_t*>(v3 + 24u) = 1u;
    *reinterpret_cast<std::uint32_t*>(v3 + 28u) = 1u;
    return auro_asc4he_v1_base_Processor_t_construct_53e630_partial(a1, v3, a2) != 0 ? 1 : 0;
}

std::int64_t auro_asc4he_v1_multichannel_Processor_t_is_supported_544650_partial(std::uint32_t a1) {
    if ((~a1 & 3u) == 0u && is_median_symmetric_5a6590_partial(a1 & 0xFFEFFFF7u)) {
        const bool ok_height = ((~a1 & 0x180u) != 0u) || ((~a1 & 0x30u) == 0u);
        return ok_height && ((a1 & 0xFB7u) == (a1 & 0xFFEFFFF7u)) ? 1 : 0;
    }
    return 0;
}

std::int64_t auro_asc4he_v1_multichannel_Processor_t_get_required_input_layout_5446b0_partial(
    std::uint32_t a1,
    std::uint32_t* a2) {
    if (!a2)
        return 3;
    const std::uint32_t v = a1 & 0xFFEFFFF7u;
    if ((~a1 & 3u) != 0u || !is_median_symmetric_5a6590_partial(v) || ((a1 & 0xFB7u) != v))
        return 278;
    const std::uint32_t v7 = a1 & 0x180u;
    const std::uint32_t v8 = a1 & 0x30u;
    if (v8 != 48u && v7 == 384u)
        return 278;
    std::uint32_t req = v8 == 48u ? 26167u : 1591u;
    if (v7 == 384u)
        req += 384u;
    if ((a1 & 0x800u) != 0u) {
        if ((~a1 & 0x600u) != 0u)
            return 1;
        req |= 0x800u;
    }
    *a2 = req | (a1 & 0x100008u);
    return 0;
}

std::int64_t auro_asc4he_v1_multichannel_HXs_t_construct_544990_partial(std::uint8_t* a1) {
    if (!a1)
        return 0;
    *reinterpret_cast<std::uint64_t*>(a1 + 0u) = 0x545200u;
    *reinterpret_cast<std::uint64_t*>(a1 + 8u) = 0x5449C0u;
    *reinterpret_cast<std::uint64_t*>(a1 + 16u) = 0x545240u;
    return 1;
}

std::int64_t auro_asc4he_v1_multichannel_HXs_process_5449c0_partial(
    std::uint8_t* a1,
    std::uint64_t* a2,
    std::uint64_t a3,
    float* a4) {
    if (!a1 || !a2 || !a4 || a3 < 768u)
        return 0;
    float* const src0 = asc4he_float_ptr(a2[15]);
    float* const src1 = asc4he_float_ptr(a2[16]);
    float* const dst0 = asc4he_float_ptr(a2[6]);
    float* const dst1 = asc4he_float_ptr(a2[7]);
    float* pair[2]{a4, a4 + 32};
    float* hi[2]{a4 + 64, a4 + 96};
    float* low[2]{a4 + 128, a4 + 160};
    float* work = a4 + 192;
    asc4he_copy_pair_32(pair[0], pair[1], src0, src1);
    std::uint64_t pair_u64[2]{asc4he_u64_ptr(pair[0]), asc4he_u64_ptr(pair[1])};
    (void)auro_asc4he_v1_ElevationEQ_process_1090c0_partial(reinterpret_cast<float*>(a1 + 24u), pair_u64);
    (void)auro_asc4he_v1_CGenXOverBlock_process_without_external_center_548b00_partial(
        a1 + 168u,
        pair,
        hi,
        low,
        a3 - 768u,
        work);
    void* vh_pair[2]{low[0], low[1]};
    (void)auro_asc4he_v1_VirtualHeight_process_10cdc0_partial(a1 + 1824u, vh_pair);
    asc4he_add_scaled_pair_32(
        pair[0],
        pair[1],
        low[0],
        low[1],
        *reinterpret_cast<const float*>(a1 + 6068u));
    std::uint64_t delay_pair[2]{asc4he_u64_ptr(pair[0]), asc4he_u64_ptr(pair[1])};
    (void)auro_asc4he_v1_Delay7ms_process_545360_partial(a1 + 2704u, reinterpret_cast<std::uint8_t*>(delay_pair));
    asc4he_add_scaled_pair_32(
        dst0,
        dst1,
        pair[0],
        pair[1],
        *reinterpret_cast<const float*>(a1 + 6064u));
    return 0;
}

std::int64_t auro_asc4he_v1_multichannel_HXs_reset_545200_partial(std::uint8_t* a1) {
    if (!a1)
        return 0;
    auro_asc4he_v1_ElevationEQ_reset_audio_state_108fc0_partial(a1 + 24u);
    auro_asc4he_v1_CGenXOverBlock_reset_audio_state_547d60_partial(a1 + 168u);
    auro_asc4he_v1_VirtualHeight_reset_audio_state_10ccf0_partial(a1 + 1824u);
    return auro_asc4he_v1_Delay7ms_reset_audio_state_545350_partial(a1 + 2704u);
}

std::int64_t auro_asc4he_v1_multichannel_HXs_initialize_545240_partial(
    std::uint8_t* a1,
    const std::uint8_t*,
    std::uint32_t a3) {
    if (!a1)
        return 1;
    std::int64_t result = auro_asc4he_v1_ElevationEQ_initialize_108fc0_partial(a1 + 24u, a3);
    if (result)
        return result;
    std::uint8_t fixed[kAsc4heCenterGenFixedDefaultsBytes]{};
    std::uint8_t dyn[kAsc4heCenterGenDynamicDefaultsBytes]{};
    auro_asc4he_v1_CenterGen_get_default_fixed_parameters_5479f0_partial(fixed);
    auro_asc4he_v1_CenterGen_get_default_dynamic_parameters_547a10_partial(dyn);
    result = auro_asc4he_v1_CGenXOverBlock_initialize_547cb0_partial(a1 + 168u, a3, 1, fixed, dyn);
    if (result)
        return result;
    result = auro_asc4he_v1_VirtualHeight_initialize_10ccf0_partial(a1 + 1824u, a3);
    if (result)
        return result;
    result = auro_asc4he_v1_Delay7ms_initialize_545310_partial(a1 + 2704u, a3);
    if (result)
        return result;
    *reinterpret_cast<std::uint32_t*>(a1 + 168u) = 1065353216u;
    *reinterpret_cast<float*>(a1 + 1804u) = 1.0f;
    *reinterpret_cast<std::uint32_t*>(a1 + 1808u) = 0x3F004DCEu;
    *reinterpret_cast<float*>(a1 + 1812u) = 1.0f;
    *reinterpret_cast<float*>(a1 + 1816u) = 1.0f;
    *reinterpret_cast<std::uint32_t*>(a1 + 1820u) = 1u;
    *reinterpret_cast<std::uint64_t*>(a1 + 6064u) = 0x3FA124783F8F9E4Dull;
    return 0;
}

std::int64_t auro_asc4he_v1_multichannel_Processor_process_544870_partial(
    std::uint8_t* a1,
    std::uint64_t a2,
    std::uint64_t a3,
    std::uint64_t a4) {
    const std::int64_t result = auro_asc4he_v1_base_Processor_default_process_53e730_partial(a1, a2, a3, a4);
    if (result || !a1)
        return result;
    const std::uint32_t count = *reinterpret_cast<const std::uint32_t*>(a1 + 20184u);
    for (std::uint32_t i = 0; i != count; ++i) {
        const std::uint64_t hx = *reinterpret_cast<const std::uint64_t*>(a1 + 20176u + 8u * i);
        if (hx == 0u)
            continue;
        const std::int64_t rc = auro_asc4he_v1_multichannel_HXs_process_5449c0_partial(
            reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(hx)),
            reinterpret_cast<std::uint64_t*>(static_cast<std::uintptr_t>(a2)),
            a3,
            reinterpret_cast<float*>(static_cast<std::uintptr_t>(a4)));
        if (rc)
            return rc;
    }
    return 0;
}

std::int64_t auro_asc4he_v1_multichannel_Processor_reset_5448f0_partial(std::uint8_t* a1) {
    auro_asc4he_v1_base_Processor_default_reset_audio_state_540fc0_partial(a1);
    if (!a1)
        return 0;
    const std::uint32_t count = *reinterpret_cast<const std::uint32_t*>(a1 + 20184u);
    for (std::uint32_t i = 0; i != count; ++i) {
        const std::uint64_t hx = *reinterpret_cast<const std::uint64_t*>(a1 + 20176u + 8u * i);
        auro_asc4he_v1_multichannel_HXs_reset_545200_partial(
            reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(hx)));
    }
    return count;
}

std::int64_t auro_asc4he_v1_multichannel_Processor_initialize_544930_partial(
    std::uint8_t* a1,
    const std::uint8_t* a2,
    std::uint32_t a3) {
    std::int64_t result = auro_asc4he_v1_base_Processor_default_initialize_541090_partial(a1, a2, a3);
    if (result || !a1)
        return result;
    const std::uint32_t count = *reinterpret_cast<const std::uint32_t*>(a1 + 20184u);
    for (std::uint32_t i = 0; i != count; ++i) {
        const std::uint64_t hx = *reinterpret_cast<const std::uint64_t*>(a1 + 20176u + 8u * i);
        result = auro_asc4he_v1_multichannel_HXs_initialize_545240_partial(
            reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(hx)),
            a2,
            a3);
        if (result)
            return result;
    }
    return 0;
}

std::int64_t auro_asc4he_v1_multichannel_Processor_t_construct_544780_partial(
    std::uint8_t* a1,
    const std::uint8_t* a2) {
    if (!a1 || !a2)
        return 0;
    const std::uint32_t v2 = *reinterpret_cast<const std::uint32_t*>(a2 + 8u);
    alignas(8) std::uint8_t v5[32]{};
    *reinterpret_cast<std::uint64_t*>(v5 + 0u) = 0x100000001ull;
    *reinterpret_cast<std::uint32_t*>(v5 + 8u) = ((~v2 & 0x30u) == 0u) ? 1u : 2u;
    *reinterpret_cast<std::uint32_t*>(v5 + 12u) = 1u;
    *reinterpret_cast<std::uint32_t*>(v5 + 16u) = (v2 & 4u) == 0u ? 1u : 0u;
    if (!auro_asc4he_v1_base_Processor_t_construct_53e630_partial(a1, v5, a2))
        return 0;
    *reinterpret_cast<std::uint32_t*>(a1 + 20184u) = 0u;
    if ((~*reinterpret_cast<const std::uint8_t*>(a2 + 8u) & 0x30u) == 0u) {
        if (!auro_asc4he_v1_multichannel_HXs_t_construct_544990_partial(a1 + 14104u))
            return 0;
        *reinterpret_cast<std::uint32_t*>(a1 + 20184u) = 1u;
        *reinterpret_cast<std::uint64_t*>(a1 + 20176u) = reinterpret_cast<std::uint64_t>(a1 + 14104u);
    }
    *reinterpret_cast<std::uint64_t*>(a1 + 14080u) = 0x544870u;
    *reinterpret_cast<std::uint64_t*>(a1 + 14088u) = 0x5448F0u;
    *reinterpret_cast<std::uint64_t*>(a1 + 14096u) = 0x544930u;
    return 1;
}

std::int64_t auro_asc4he_v1_Processor_t_is_supported_53dd00_partial(std::uint32_t a1, std::uint32_t a2) {
    if (a1 == 0u)
        return auro_asc4he_v1_multichannel_Processor_t_is_supported_544650_partial(a2);
    if (a1 == 1u)
        return auro_asc4he_v1_sb_Processor_t_is_supported_53e0e0_partial(a2);
    if (a1 == 2u)
        return auro_asc4he_v1_ss_Processor_t_is_supported_5445a0_partial(a2);
    return 0;
}

std::int64_t auro_asc4he_v1_Processor_t_get_required_input_layout_53dd30_partial(
    std::uint32_t a1,
    std::uint32_t a2,
    std::uint32_t* a3) {
    if (!a3)
        return 3;
    if (a1 == 0u)
        return auro_asc4he_v1_multichannel_Processor_t_get_required_input_layout_5446b0_partial(a2, a3);
    if (a1 == 1u)
        return auro_asc4he_v1_sb_Processor_t_get_required_input_layout_53e1a0_partial(a2, a3);
    if (a1 == 2u)
        return auro_asc4he_v1_ss_Processor_t_get_required_input_layout_5445c0_partial(a2, a3);
    return 274;
}

std::int64_t auro_asc4he_v1_TuningManager_t_get_53d880_partial(
    std::uint8_t* a1,
    std::uint32_t a2,
    std::uint32_t a3) {
    if (!a1)
        return 1;
    std::memset(a1, 0, 240u);
    const auto put32 = [](std::uint8_t* p, std::uint32_t off, std::uint32_t v) {
        *reinterpret_cast<std::uint32_t*>(p + off) = v;
    };
    const auto put64 = [](std::uint8_t* p, std::uint32_t off, std::uint64_t v) {
        *reinterpret_cast<std::uint64_t*>(p + off) = v;
    };
    const auto put128 = [](std::uint8_t* p, std::uint32_t off, std::uint64_t lo, std::uint64_t hi) {
        *reinterpret_cast<std::uint64_t*>(p + off) = lo;
        *reinterpret_cast<std::uint64_t*>(p + off + 8u) = hi;
    };
    auro_asc4he_v1_CenterGen_get_default_fixed_parameters_5479f0_partial(a1 + 140u);
    auro_asc4he_v1_CenterGen_get_default_dynamic_parameters_547a10_partial(a1 + 160u);
    if (a2 == 2u) {
        const bool direct_height = (a3 & 0xC0000u) == 0xC0000u;
        if (direct_height != ((a3 & 0x1000u) == 0u))
            return 1;
        if (direct_height)
            put32(a1, 212u, 0x3F800000u);
        const std::uint64_t v20 = direct_height ? 0xC0C00000C1900000ull : 0xC0A00000C1400000ull;
        const std::uint64_t v21 = direct_height ? 0xC100000040000000ull : 0xC0A0000040C00000ull;
        const std::uint64_t v22 = direct_height ? 0x408000003F000000ull : 0xC0C0000040C00000ull;
        const std::uint64_t v23 = direct_height ? 0x3F00000044BB8000ull : 0x3F33333344898000ull;
        const std::uint64_t v24 = direct_height ? 0x4000000000000000ull : 0x3F8000003F800000ull;
        const std::uint32_t v25 = direct_height ? 0x40000000u : 0u;
        const std::uint32_t v26 = direct_height ? 6u : 5u;
        const std::uint32_t v27 = direct_height ? 0xC0400000u : 0xC0000000u;
        const std::uint32_t v28 = direct_height ? 0xC0800000u : 0xC0200000u;
        const std::uint32_t v29 = direct_height ? 0xC0800000u : 0u;
        put64(a1, 176u, v20);
        put128(a1, 0u, 0xBF800000C1400000ull, 0x3F80000000000000ull);
        put128(a1, 16u, 0xC0C00000C1400000ull, 0x0000000040400000ull);
        put128(a1, 36u, 0xC0C000003F800000ull, 0x40400000C0C00000ull);
        put128(a1, 56u, 0xFF800000FF800000ull, 0xC040000000000000ull);
        put32(a1, 32u, v29);
        put32(a1, 52u, v25);
        put64(a1, 72u, v24);
        put64(a1, 128u, 0x0000000200000000ull);
        put32(a1, 80u, v26);
        put64(a1, 84u, v23);
        put32(a1, 92u, v27);
        put64(a1, 96u, 0x45BB800000000005ull);
        put64(a1, 104u, v22);
        put64(a1, 112u, 0x463B800000000005ull);
        put64(a1, 120u, v21);
        put32(a1, 136u, v28);
        return 0;
    }
    if (a2 == 1u) {
        const std::uint32_t inv = ~a3;
        const bool has_180 = (inv & 0x180u) == 0u;
        const bool has_c0000 = (inv & 0xC0000u) == 0u;
        put64(a1, 176u, 0xC0A00000C1400000ull);
        put64(a1, 0u, 0xBF800000C1400000ull);
        put64(a1, 8u, has_180 ? 0x3F800000C0400000ull : 0u);
        put32(a1, 16u, (inv & 0x180u) != 0u ? 0xC1000000u : 0xC0400000u);
        put64(a1, 20u, 0x40800000C0C00000ull);
        *reinterpret_cast<float*>(a1 + 28u) = ((inv & 0x30u) == 0u) ? -12.0f : 0.0f;
        put64(a1, 32u, has_c0000 ? 0x3F800000C0400000ull : 0x00000000BF800000ull);
        put32(a1, 40u, (inv & 0xC0000u) != 0u ? 0xC1000000u : 0xC0400000u);
        put64(a1, 44u, 0x40800000C0C00000ull);
        *reinterpret_cast<float*>(a1 + 52u) = ((inv & 0x600u) != 0u) ? 0.0f : -12.0f;
        put128(a1, 56u, 0x4040000040400000ull, 0xC040000000000000ull);
        put64(a1, 72u, 0u);
        put32(a1, 80u, 6u);
        put64(a1, 84u, 0x3F00000044898000ull);
        put64(a1, 92u, 0x00000005C0E00000ull);
        put64(a1, 100u, 0x4060000045FA0000ull);
        put64(a1, 108u, 0x00000005C1200000ull);
        put128(a1, 116u, 0x40600000467A0000ull, 0x40400000C1200000ull);
        put32(a1, 136u, 0xC1000000u);
        put32(a1, 132u, 3u);
        return 0;
    }
    if (a2 != 0u)
        return 274;
    const std::uint32_t inv = ~a3;
    const bool missing_600 = (inv & 0x600u) != 0u;
    const bool full_30 = (inv & 0x30u) == 0u;
    put64(a1, 176u, 0xC0A00000C1400000ull);
    put64(a1, 0u, 0xC0C00000C0400000ull);
    put32(a1, 8u, full_30 ? 0xC0A00000u : 0x40000000u);
    put32(a1, 12u, 0x3F800000u);
    if (full_30)
        put128(a1, 16u, 0x3F80000000000000ull, 0xBF8000003F800000ull);
    else
        put128(a1, 16u, 0x00000000C1400000ull, 0x3F80000040000000ull);
    put128(a1, 32u, 0x3F8000003F800000ull, 0x00000000C1100000ull);
    put32(a1, 48u, full_30 ? 0x40000000u : 0x40400000u);
    *reinterpret_cast<float*>(a1 + 52u) = missing_600 ? 1.0f : -8.0f;
    put128(a1, 56u, 0x4040000040400000ull, 0xC040000000000000ull);
    put64(a1, 72u, 0u);
    put32(a1, 80u, 7u);
    put64(a1, 84u, 0x3F00000044960000ull);
    put64(a1, 92u, 0x000000053F800000ull);
    put64(a1, 100u, 0x4020000045960000ull);
    put64(a1, 108u, 0x00000005C0A00000ull);
    put128(a1, 116u, 0x4040000046160000ull, 0x3F800000C0A00000ull);
    put32(a1, 136u, 0xC0400000u);
    put32(a1, 132u, 5u);
    return 0;
}

std::int64_t auro_asc4he_v1_Processor_t_check_static_parameters_53dd90_partial(const std::uint8_t* a1) {
    if (!a1)
        return 3;
    const std::uint32_t type = *reinterpret_cast<const std::uint32_t*>(a1 + 0u);
    const std::uint32_t required = *reinterpret_cast<const std::uint32_t*>(a1 + 4u);
    const std::uint32_t layout = *reinterpret_cast<const std::uint32_t*>(a1 + 8u);
    const std::uint32_t sample_rate = *reinterpret_cast<const std::uint32_t*>(a1 + 12u);
    if (sample_rate != 48000u && sample_rate != 44100u)
        return 275;
    std::uint32_t actual = 0;
    const std::int64_t rc = auro_asc4he_v1_Processor_t_get_required_input_layout_53dd30_partial(type, layout, &actual);
    if (rc)
        return rc;
    if (actual != required)
        return 1;
    if (*reinterpret_cast<const std::uint32_t*>(a1 + 32u) != 0u && *reinterpret_cast<const float*>(a1 + 36u) > 600.f)
        return 281;
    if (*reinterpret_cast<const std::uint32_t*>(a1 + 40u) != 0u && *reinterpret_cast<const float*>(a1 + 44u) > 600.f)
        return 282;
    return 0;
}

std::uint8_t* auro_asc4he_v1_Processor_t_construct_53de70_partial(std::uint8_t* a1, const std::uint8_t* a2) {
    if (!a1 || !a2)
        return nullptr;
    if (auro_asc4he_v1_Processor_t_check_static_parameters_53dd90_partial(a2))
        return nullptr;
    std::memset(a1, 0, 0x4F08u);
    std::memcpy(a1 + 20208u, a2 + 16u, 16u);
    *reinterpret_cast<std::uint64_t*>(a1 + 20224u) =
        *reinterpret_cast<const std::uint64_t*>(a1 + 20208u) >= 2432u ? 0x53E080u : 0x53E0B0u;
    *reinterpret_cast<std::uint32_t*>(a1 + 20200u) = *reinterpret_cast<const std::uint32_t*>(a2 + 12u);
    const std::uint32_t type = *reinterpret_cast<const std::uint32_t*>(a2 + 0u);
    bool ok = false;
    if (type == 0u)
        ok = auro_asc4he_v1_multichannel_Processor_t_construct_544780_partial(a1, a2) != 0;
    else if (type == 1u)
        ok = auro_asc4he_v1_sb_Processor_t_construct_53e2a0_partial(a1, a2) != 0;
    else if (type == 2u)
        ok = auro_asc4he_v1_ss_Processor_t_construct_5445f0_partial(a1, a2) != 0;
    if (!ok)
        return nullptr;
    *reinterpret_cast<std::uint64_t*>(a1 + 20192u) = reinterpret_cast<std::uint64_t>(a1);
    std::uint8_t tuning[240]{};
    const std::uint8_t* params = a2 + 52u;
    if (*reinterpret_cast<const std::uint32_t*>(a2 + 48u) == 0u) {
        if (auro_asc4he_v1_TuningManager_t_get_53d880_partial(
                tuning,
                *reinterpret_cast<const std::uint32_t*>(a2 + 0u),
                *reinterpret_cast<const std::uint32_t*>(a2 + 8u)) != 0) {
            return nullptr;
        }
        params = tuning;
    }
    if (*reinterpret_cast<const std::uint64_t*>(a1 + 14096u) == 0x544930u)
        ok = auro_asc4he_v1_multichannel_Processor_initialize_544930_partial(
                 a1,
                 params,
                 *reinterpret_cast<const std::uint32_t*>(a2 + 12u))
            == 0;
    else if (*reinterpret_cast<const std::uint64_t*>(a1 + 14096u) == 0x53E430u)
        ok = auro_asc4he_v1_sb_Processor_surround_initialize_53e430_partial(
                 a1,
                 params,
                 *reinterpret_cast<const std::uint32_t*>(a2 + 12u))
            == 0;
    else if (*reinterpret_cast<const std::uint64_t*>(a1 + 14096u) == 0x53E540u)
        ok = auro_asc4he_v1_sb_Processor_height_initialize_53e540_partial(
                 a1,
                 params,
                 *reinterpret_cast<const std::uint32_t*>(a2 + 12u))
            == 0;
    else
        ok = auro_asc4he_v1_base_Processor_default_initialize_541090_partial(
                 a1,
                 params,
                 *reinterpret_cast<const std::uint32_t*>(a2 + 12u))
            == 0;
    if (!ok)
        return nullptr;
    (void)auro_asc4he_v1_Processor_reset_audio_state_53e020_partial(a1);
    return a1;
}

std::int64_t auro_asc4he_v1_Processor_t_get_latency_53dfd0_partial() {
    return 0;
}

std::int64_t auro_asc4he_v1_Processor_get_dynamic_parameters_53dfe0_partial(
    const std::uint8_t* a1,
    std::uint8_t* a2) {
    return (a1 == nullptr || a2 == nullptr) ? 1 : 0;
}

std::int64_t auro_asc4he_v1_Processor_set_dynamic_parameters_53e000_partial(
    std::uint8_t* a1,
    const std::uint8_t* a2) {
    return (a1 == nullptr || a2 == nullptr) ? 1 : 0;
}

std::int64_t auro_asc4he_v1_Processor_reset_audio_state_53e020_partial(std::uint8_t* a1) {
    if (!a1)
        return 1;
    auto* const base = reinterpret_cast<std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<const std::uint64_t*>(a1 + 20192u)));
    if (!base)
        return 1;
    const std::uint64_t reset = *reinterpret_cast<const std::uint64_t*>(base + 14088u);
    if (reset == 0x5448F0u)
        return auro_asc4he_v1_multichannel_Processor_reset_5448f0_partial(base);
    if (reset == 0x53E410u)
        return auro_asc4he_v1_sb_Processor_surround_reset_53e410_partial(base);
    if (reset == 0x53E520u)
        return auro_asc4he_v1_sb_Processor_height_reset_53e520_partial(base);
    return auro_asc4he_v1_base_Processor_default_reset_audio_state_540fc0_partial(base);
}

std::int64_t auro_asc4he_v1_Processor_process_53e030_partial(std::uint8_t* a1, std::uint32_t* a2) {
    if (!a1 || !a2)
        return 1;
    if (a2[0] != 32u)
        return 276;
    if (a2[1] != *reinterpret_cast<const std::uint32_t*>(a1 + 20200u))
        return 275;
    alignas(16) std::uint8_t stack_scratch[2432]{};
    std::uint64_t scratch_bytes = *reinterpret_cast<const std::uint64_t*>(a1 + 20208u);
    std::uint64_t scratch = *reinterpret_cast<const std::uint64_t*>(a1 + 20216u);
    if (scratch_bytes < 2432u || scratch == 0u) {
        scratch_bytes = 2432u;
        scratch = reinterpret_cast<std::uint64_t>(stack_scratch);
    }
    auto* const base = reinterpret_cast<std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<const std::uint64_t*>(a1 + 20192u)));
    if (!base)
        return 1;
    const std::uint64_t process = *reinterpret_cast<const std::uint64_t*>(base + 14080u);
    std::int64_t rc = 0;
    const std::uint64_t table = reinterpret_cast<std::uint64_t>(a2);
    if (process == 0x544870u)
        rc = auro_asc4he_v1_multichannel_Processor_process_544870_partial(base, table, scratch_bytes, scratch);
    else if (process == 0x53E480u)
        rc = auro_asc4he_v1_sb_Processor_surround_process_2_2_53e480_partial(base, table, scratch_bytes, scratch);
    else if (process == 0x53E4D0u)
        rc = auro_asc4he_v1_sb_Processor_surround_process_2_0_53e4d0_partial(base, table, scratch_bytes, scratch);
    else if (process == 0x53E590u)
        rc = auro_asc4he_v1_sb_Processor_height_process_2_2_53e590_partial(base, table, scratch_bytes, scratch);
    else if (process == 0x53E5E0u)
        rc = auro_asc4he_v1_sb_Processor_height_process_2_0_53e5e0_partial(base, table, scratch_bytes, scratch);
    else
        rc = auro_asc4he_v1_base_Processor_default_process_53e730_partial(base, table, scratch_bytes, scratch);
    return rc == 0 ? 0 : rc;
}

void auro_asc4he_v1_CenterGen_get_default_fixed_parameters_5479f0_partial(std::uint8_t* a1) {
    if (!a1)
        return;
    std::memset(a1, 0, kAsc4heCenterGenFixedDefaultsBytes);
    *reinterpret_cast<std::uint64_t*>(a1 + 0u) = 1ull;
    *reinterpret_cast<std::uint64_t*>(a1 + 8u) = 0ull;
    *reinterpret_cast<std::uint32_t*>(a1 + 16u) = 393216300u;
}

void auro_asc4he_v1_CenterGen_get_default_dynamic_parameters_547a10_partial(std::uint8_t* a1) {
    if (!a1)
        return;
    std::memset(a1, 0, kAsc4heCenterGenDynamicDefaultsBytes);
    *reinterpret_cast<std::uint32_t*>(a1 + 0u) = 0u;
    *reinterpret_cast<std::uint64_t*>(a1 + 4u) = 0x3F73333300000000ull;
    *reinterpret_cast<std::uint64_t*>(a1 + 12u) = 0xC17000003D4CCCCDull;
    *reinterpret_cast<std::uint64_t*>(a1 + 20u) = 0x00000000C1400000ull;
    *reinterpret_cast<std::uint64_t*>(a1 + 28u) = 0x3C23D70A3F800000ull;
    *reinterpret_cast<std::uint64_t*>(a1 + 36u) = 0x417000003E99999Aull;
    *reinterpret_cast<std::uint64_t*>(a1 + 44u) = 0xC04000003F7D70A4ull;
    *reinterpret_cast<std::uint32_t*>(a1 + 52u) = 0u;
}

