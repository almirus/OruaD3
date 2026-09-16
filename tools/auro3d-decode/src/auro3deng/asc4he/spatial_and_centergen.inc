static std::int64_t centergen_iir_float64_coeffs_to48(
    std::uint8_t* param_stack12,
    std::uint32_t sr_hz,
    std::uint8_t* out48) noexcept {
    (void)param_stack12;
    (void)sr_hz;
    if (!out48)
        return 0;
    std::memset(out48, 0, 48u);
    return 0;
}

static std::int64_t centergen_iir_fixed32_coeffs_to48(
    std::uint8_t* param_stack12,
    std::uint32_t sr_hz,
    std::uint8_t* out48) noexcept {
    (void)param_stack12;
    (void)sr_hz;
    if (!out48)
        return 0;
    std::memset(out48, 0, 48u);
    return 0;
}

std::int64_t auro_asc4he_v1_blocked_Delay_initialize(
    std::uint8_t* a1,
    std::int32_t a2,
    std::uint8_t* a3,
    std::uint32_t a4,
    std::int32_t a5) {
    if (!a1 || !a3)
        return 1;
    const std::uint32_t a2u = static_cast<std::uint32_t>(a2);
    const std::uint32_t a5u = static_cast<std::uint32_t>(a5);
    const std::uint32_t v6 = (((48000u * a2u + 999999u) / 1000000u) + 31u) & 0xFFFFFFE0u;
    const std::uint32_t v7 = 2u * v6 + 128u;
    if (v7 > a4)
        return 1;
    *reinterpret_cast<std::uint32_t*>(a1 + 24u) = v6 + 32u;
    const std::uint32_t v8 = a2u * a5u + 999999u;
    const std::uint32_t div = v8 / 1000000u;
    *reinterpret_cast<std::uint32_t*>(a1 + 20u) = div;
    if (div > v6)
        return 1;
    *reinterpret_cast<std::uint64_t*>(a1 + 0u) = reinterpret_cast<std::uint64_t>(a3);
    *reinterpret_cast<std::uint64_t*>(a1 + 8u) =
        reinterpret_cast<std::uint64_t>(a3 + static_cast<std::size_t>(2ull * static_cast<std::uint64_t>(v7)));
    const std::uint32_t v9 =
        (4u * ((48000u * a2u + 999999u) / 1000000u) + 124u) & 0xFFFFFF80u;
    const std::uintptr_t a3p = reinterpret_cast<std::uintptr_t>(a3);
    const std::uintptr_t v10 = static_cast<std::uintptr_t>(v9) + a3p + 272u;
    const std::uint64_t v11 = static_cast<std::uint64_t>(v9) + 128u;
    for (std::uint64_t v12 = 0ull; v12 != v11; v12 += 32ull) {
        std::memset(reinterpret_cast<void*>(a3p + static_cast<std::uintptr_t>(v12)), 0, 16u);
        std::memset(reinterpret_cast<void*>(a3p + static_cast<std::uintptr_t>(v12) + 16u), 0, 16u);
        const std::uintptr_t p2 = v10 + static_cast<std::uintptr_t>(v12) - 16u;
        std::memset(reinterpret_cast<void*>(p2), 0, 16u);
        std::memset(reinterpret_cast<void*>(p2 + 16u), 0, 16u);
    }
    *reinterpret_cast<std::uint32_t*>(a1 + 16u) = 0u;
    return 0;
}

std::int64_t auro_asc4he_v1_blocked_Delay_reset_audio_state(std::uint8_t* a1) {
    if (!a1)
        return 0;
    const std::uint32_t v1 = *reinterpret_cast<std::uint32_t*>(a1 + 24u);
    const std::uint32_t v2 = v1 & 0xFFFFFFE0u;
    const std::uint64_t v3 = *reinterpret_cast<std::uint64_t*>(a1 + 0u);
    const std::uint64_t v4 = *reinterpret_cast<std::uint64_t*>(a1 + 8u);
    if (v2 != 0u && v3 != 0u && v4 != 0u) {
        const std::uint64_t span = v4 - v3;
        if (span >= 32ull) {
            std::int64_t v7 = 0;
            do {
                const std::uintptr_t b0 = static_cast<std::uintptr_t>(v3 + static_cast<std::uint64_t>(4ull * static_cast<std::uint64_t>(v7)));
                std::memset(reinterpret_cast<void*>(b0), 0, 16u);
                std::memset(reinterpret_cast<void*>(b0 + 16u), 0, 16u);
                const std::uintptr_t b1 = static_cast<std::uintptr_t>(v4 + static_cast<std::uint64_t>(4ull * static_cast<std::uint64_t>(v7)));
                std::memset(reinterpret_cast<void*>(b1), 0, 16u);
                std::memset(reinterpret_cast<void*>(b1 + 16u), 0, 16u);
                v7 += 8;
            } while (static_cast<std::uint32_t>(v7) != v2);
        } else {
            const std::uint64_t v5 = (4ull * static_cast<std::uint64_t>(v1)) & 0xFFFFFFFFFFFFFF80ull;
            std::uint64_t v6 = 0;
            do {
                std::memset(reinterpret_cast<void*>(static_cast<std::uintptr_t>(v3 + v6)), 0, 16u);
                std::memset(reinterpret_cast<void*>(static_cast<std::uintptr_t>(v4 + v6)), 0, 16u);
                v6 += 16ull;
            } while (v5 != v6);
        }
    }
    *reinterpret_cast<std::uint32_t*>(a1 + 16u) = 0u;
    return 0;
}

std::int64_t auro_asc4he_v1_blocked_Delay_process(std::uint8_t* a1, std::uint8_t* a2_pair) {
    if (!a1 || !a2_pair)
        return 1;
    auto** v4p = reinterpret_cast<std::uint32_t**>(a2_pair + 0u);
    auto** v6p = reinterpret_cast<std::uint32_t**>(a2_pair + 8u);
    if (!v4p || !v6p || !*v4p || !*v6p)
        return 1;
    std::uint32_t* const v4 = *v4p;
    std::uint32_t* const v6 = *v6p;
    const std::uint32_t v2 = *reinterpret_cast<const std::uint32_t*>(a1 + 16u);
    const std::uint64_t v3 = *reinterpret_cast<const std::uint64_t*>(a1 + 0u);
    const std::uint64_t v5 = *reinterpret_cast<const std::uint64_t*>(a1 + 8u);
    if (v3 == 0u || v5 == 0u)
        return 1;
    const std::uint64_t base = static_cast<std::uint64_t>(v2) * 4u;
    for (unsigned i = 0u; i < 32u; ++i) {
        *reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(v3 + base + 4ull * i)) = v4[i];
        *reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(v5 + base + 4ull * i)) = v6[i];
    }
    if (v2 == 0u) {
        const std::uint32_t v7 = *reinterpret_cast<const std::uint32_t*>(a1 + 24u);
        const std::uint64_t base7 = static_cast<std::uint64_t>(v7) * 4u;
        for (unsigned i = 0u; i < 32u; ++i) {
            *reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(v3 + base7 + 4ull * i)) = v4[i];
            *reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(v5 + base7 + 4ull * i)) = v6[i];
        }
    }
    const std::uint32_t v8 = *reinterpret_cast<const std::uint32_t*>(a1 + 16u);
    const std::uint32_t v20 = *reinterpret_cast<const std::uint32_t*>(a1 + 20u);
    const std::uint32_t v24lim = *reinterpret_cast<const std::uint32_t*>(a1 + 24u);
    const std::uint32_t v10 = (v8 < v20) ? v24lim : 0u;
    const std::uint32_t v11 = v10 + v8 - v20;
    std::uint32_t* const v12 = *reinterpret_cast<std::uint32_t**>(a2_pair);
    const std::uint64_t v13 = *reinterpret_cast<const std::uint64_t*>(a1 + 0u);
    for (unsigned i = 0u; i < 32u; ++i)
        v12[i] = *reinterpret_cast<const std::uint32_t*>(
            static_cast<std::uintptr_t>(v13 + static_cast<std::uint64_t>(v11) * 4ull + 4ull * i));
    std::uint32_t* const v14 = *reinterpret_cast<std::uint32_t**>(a2_pair + 8u);
    const std::uint64_t v15 = *reinterpret_cast<const std::uint64_t*>(a1 + 8u);
    for (unsigned i = 0u; i < 32u; ++i)
        v14[i] = *reinterpret_cast<const std::uint32_t*>(
            static_cast<std::uintptr_t>(v15 + static_cast<std::uint64_t>(v11) * 4ull + 4ull * i));
    std::uint32_t v9 = 0u;
    if (v8 + 32u >= v24lim)
        v9 = v24lim;
    const std::uint32_t result = v8 - v9 + 32u;
    *reinterpret_cast<std::uint32_t*>(a1 + 16u) = result;
    return static_cast<std::int64_t>(static_cast<std::int32_t>(result));
}

std::int64_t auro_asc4he_v1_Delay7ms_initialize(std::uint8_t* a1, std::uint32_t a2) {
    if (!a1)
        return 1;
    const std::int32_t v2 = static_cast<std::int32_t>(
        auro_asc4he_v1_blocked_Delay_initialize(a1 + 3328u, 7000, a1, 832, a2));
    if (v2 == 0)
        return 0;
    return static_cast<std::int64_t>(static_cast<std::uint32_t>(v2 == 0) + static_cast<std::uint32_t>(v2));
}

std::int64_t auro_asc4he_v1_Delay7ms_reset_audio_state(std::uint8_t* a1) {
    return auro_asc4he_v1_blocked_Delay_reset_audio_state(a1 ? a1 + 3328u : nullptr);
}

std::int64_t auro_asc4he_v1_Delay7ms_process(std::uint8_t* a1, std::uint8_t* a2_pair) {
    return auro_asc4he_v1_blocked_Delay_process(a1 ? a1 + 3328u : nullptr, a2_pair);
}

std::int64_t auro_asc4he_v1_Delay10ms_initialize(
    std::uint8_t* a1,
    std::int32_t a2,
    std::uint32_t a3) {
    if (!a1)
        return 1;
    const std::int32_t v3 = static_cast<std::int32_t>(
        auro_asc4he_v1_blocked_Delay_initialize(a1 + 4352u, a2, a1, 1088, a3));
    if (v3 == 0)
        return 0;
    return static_cast<std::int64_t>(static_cast<std::uint32_t>(v3 == 0) + static_cast<std::uint32_t>(v3));
}

std::int64_t auro_asc4he_v1_Delay10ms_reset_audio_state(std::uint8_t* a1) {
    return auro_asc4he_v1_blocked_Delay_reset_audio_state(a1 ? a1 + 4352u : nullptr);
}

std::int64_t auro_asc4he_v1_Delay10ms_process(std::uint8_t* a1, std::uint8_t* a2_pair) {
    return auro_asc4he_v1_blocked_Delay_process(a1 ? a1 + 4352u : nullptr, a2_pair);
}

std::int64_t auro_asc4he_v1_Decorrelator_initialize(std::uint8_t* state, std::uint64_t cfg_ptr) {
    if (!state)
        return 1;
    std::uint8_t v3[16]{};
    static constexpr struct {
        int kind;
        std::uint64_t q;
        std::uint32_t off;
    } kSteps[] = {
        {9, 0x4040000044480000ull, 0u},
        {9, 0x40400000447A0000ull, 28u},
        {9, 0x40400000449C4000ull, 56u},
        {9, 0x4040000044C80000ull, 84u},
        {9, 0x4040000044FA0000ull, 112u},
        {9, 0x40400000451C4000ull, 140u},
        {9, 0x404000004544E000ull, 168u},
        {9, 0x40400000457A0000ull, 196u},
        {9, 0x40400000459C4000ull, 224u},
        {9, 0x4040000045C4E000ull, 252u},
    };
    for (const auto& s : kSteps) {
        *reinterpret_cast<std::int32_t*>(v3 + 0u) = s.kind;
        *reinterpret_cast<std::uint64_t*>(v3 + 4u) = s.q;
        *reinterpret_cast<std::int32_t*>(v3 + 12u) = 0;
        (void)auro_iir_biquad_parameter_Config_float32_t_compute(v3, cfg_ptr, state + s.off);
    }
    for (std::uint32_t off = 0u; off != 280u; off += 28u)
        *reinterpret_cast<std::uint64_t*>(state + off + 20u) = 0u;
    return 0;
}

void auro_asc4he_v1_Decorrelator_reset_audio_state(std::uint8_t* a1) {
    if (!a1)
        return;
    for (std::uint32_t off = 0u; off != 280u; off += 28u)
        *reinterpret_cast<std::uint64_t*>(a1 + off + 20u) = 0u;
}

namespace {

void asc4he_decorrelator_pass(
    float* coeffs_state,
    std::uintptr_t buf_bytes,
    int coeff_index_base) noexcept {
    float z0 = coeffs_state[coeff_index_base + 5];
    float z1 = coeffs_state[coeff_index_base + 6];
    const float* c = coeffs_state + coeff_index_base;
    for (std::int32_t i = 0; i != 32; ++i) {
        const float v = *reinterpret_cast<float*>(buf_bytes + static_cast<std::uintptr_t>(4u * static_cast<std::uint32_t>(i)));
        const float out = c[0] * v + z0;
        z0 = (c[1] * v + z1) - (c[3] * out);
        z1 = (v * c[2]) - (c[4] * out);
        *reinterpret_cast<float*>(buf_bytes + static_cast<std::uintptr_t>(4u * static_cast<std::uint32_t>(i))) = out;
    }
    coeffs_state[coeff_index_base + 5] = z0;
    coeffs_state[coeff_index_base + 6] = z1;
}

void asc4he_elevation_eq_section(
    float* a1,
    std::uintptr_t result_buf,
    std::uintptr_t alt_buf,
    int base) noexcept {
    float v4 = a1[base + 0];
    float v5 = a1[base + 1];
    float v6 = a1[base + 2];
    float v7 = a1[base + 3];
    const float* c = a1 + base;
    for (std::int32_t i = 0; i != 32; ++i) {
        const float v9 = *reinterpret_cast<float*>(result_buf + static_cast<std::uintptr_t>(4u * static_cast<std::uint32_t>(i)));
        const float v10 = *reinterpret_cast<float*>(alt_buf + static_cast<std::uintptr_t>(4u * static_cast<std::uint32_t>(i)));
        const float v11 = c[4];
        const float v12 = v11 * v10 + v6;
        const float v13 = v11 * v9 + v4;
        v4 = (c[5] * v9 + v5) - (c[7] * v13);
        v6 = (c[5] * v10 + v7) - (c[7] * v12);
        const float v14 = c[6];
        const float v15 = c[8];
        *reinterpret_cast<float*>(result_buf + static_cast<std::uintptr_t>(4u * static_cast<std::uint32_t>(i))) = v13;
        v5 = (v9 * v14) - (v13 * v15);
        v7 = (v14 * v10) - (v15 * v12);
        *reinterpret_cast<float*>(alt_buf + static_cast<std::uintptr_t>(4u * static_cast<std::uint32_t>(i))) = v12;
    }
    a1[base + 0] = v4;
    a1[base + 1] = v5;
    a1[base + 2] = v6;
    a1[base + 3] = v7;
}

} // namespace

std::int64_t auro_asc4he_v1_CrossTalkCompensation_initialize(
    std::uint8_t* a1,
    const std::uint8_t* a2,
    std::uint32_t a3) {
    if (!a1 || !a2)
        return 1;
    std::memset(a1, 0, kAsc4heCrossTalkCompensationStateBytes);
    (void)auro_iir_biquad_parameter_Config_float32_t_compute(const_cast<std::uint8_t*>(a2), a3, a1 + 16u);
    (void)auro_iir_biquad_parameter_Config_float32_t_compute(const_cast<std::uint8_t*>(a2 + 16u), a3, a1 + 52u);
    (void)auro_iir_biquad_parameter_Config_float32_t_compute(const_cast<std::uint8_t*>(a2 + 32u), a3, a1 + 88u);
    const float v4 = *reinterpret_cast<const float*>(a2 + 48u);
    float v5 = 0.0f;
    if (v4 > -144.0f)
        v5 = std::pow(10.0f, v4 * 0.050000001f);
    *reinterpret_cast<float*>(a1 + 108u) = v5;
    std::memset(a1 + 0u, 0, 16u);
    std::memset(a1 + 36u, 0, 16u);
    std::memset(a1 + 72u, 0, 16u);
    return 0;
}

void auro_asc4he_v1_CrossTalkCompensation_reset_audio_state(std::uint8_t* a1) {
    if (!a1)
        return;
    std::memset(a1 + 0u, 0, 16u);
    std::memset(a1 + 36u, 0, 16u);
    std::memset(a1 + 72u, 0, 16u);
}

float* auro_asc4he_v1_CrossTalkCompensation_process(float* a1, float** a2) {
    if (!a1 || !a2 || !a2[0] || !a2[1])
        return nullptr;
    float* const v2 = a2[0];
    float* const result = a2[1];
    asc4he_elevation_eq_section(a1, reinterpret_cast<std::uintptr_t>(v2), reinterpret_cast<std::uintptr_t>(result), 0);
    asc4he_elevation_eq_section(a1, reinterpret_cast<std::uintptr_t>(v2), reinterpret_cast<std::uintptr_t>(result), 9);
    asc4he_elevation_eq_section(a1, reinterpret_cast<std::uintptr_t>(v2), reinterpret_cast<std::uintptr_t>(result), 18);
    const float v40 = a1[27];
    for (std::uint32_t i = 0; i != 32u; ++i) {
        v2[i] *= v40;
        result[i] *= v40;
    }
    return result;
}

std::int64_t auro_asc4he_v1_Decorrelator_process(float* block, std::uint64_t* io_pair) {
    if (!block || !io_pair)
        return 0;
    const std::uintptr_t v2 = static_cast<std::uintptr_t>(io_pair[0]);
    const std::uintptr_t result = static_cast<std::uintptr_t>(io_pair[1]);
    if (v2 == 0u || result == 0u)
        return 0;
    asc4he_decorrelator_pass(block, v2, 0);
    asc4he_decorrelator_pass(block, result, 7);
    asc4he_decorrelator_pass(block, v2, 14);
    asc4he_decorrelator_pass(block, result, 21);
    asc4he_decorrelator_pass(block, v2, 28);
    asc4he_decorrelator_pass(block, result, 35);
    asc4he_decorrelator_pass(block, v2, 42);
    asc4he_decorrelator_pass(block, result, 49);
    asc4he_decorrelator_pass(block, v2, 56);
    asc4he_decorrelator_pass(block, result, 63);
    return static_cast<std::int64_t>(result);
}

std::int64_t auro_asc4he_v1_ElevationEQ_initialize(std::uint8_t* a1, std::uint64_t a2) {
    if (!a1)
        return 1;
    std::memset(a1, 0, kAsc4heElevationEqStateBytes);
    std::uint8_t v3[16]{};
    *reinterpret_cast<std::int32_t*>(v3 + 0u) = 5;
    *reinterpret_cast<std::uint64_t*>(v3 + 4u) = 0x4040000043E10000ull;
    *reinterpret_cast<std::int32_t*>(v3 + 12u) = 0;
    (void)auro_iir_biquad_parameter_Config_float32_t_compute(v3, a2, a1 + 16u);
    *reinterpret_cast<std::int32_t*>(v3 + 0u) = 5;
    *reinterpret_cast<std::uint64_t*>(v3 + 4u) = 0x40A00000442A0000ull;
    *reinterpret_cast<std::int32_t*>(v3 + 12u) = -1061158912;
    (void)auro_iir_biquad_parameter_Config_float32_t_compute(v3, a2, a1 + 52u);
    *reinterpret_cast<std::int32_t*>(v3 + 0u) = 5;
    *reinterpret_cast<std::uint64_t*>(v3 + 4u) = 0x4040000045FA0000ull;
    *reinterpret_cast<std::int32_t*>(v3 + 12u) = 1086324736;
    (void)auro_iir_biquad_parameter_Config_float32_t_compute(v3, a2, a1 + 124u);
    *reinterpret_cast<std::int32_t*>(v3 + 0u) = 5;
    *reinterpret_cast<std::uint64_t*>(v3 + 4u) = 0x40A0000046147000ull;
    *reinterpret_cast<std::int32_t*>(v3 + 12u) = -1069547520;
    (void)auro_iir_biquad_parameter_Config_float32_t_compute(v3, a2, a1 + 88u);
    std::memset(a1 + 0u, 0, 16u);
    std::memset(a1 + 36u, 0, 16u);
    std::memset(a1 + 72u, 0, 16u);
    std::memset(a1 + 108u, 0, 16u);
    return 0;
}

void auro_asc4he_v1_ElevationEQ_reset_audio_state(std::uint8_t* state) {
    if (!state)
        return;
    std::memset(state + 0u, 0, 16u);
    std::memset(state + 36u, 0, 16u);
    std::memset(state + 72u, 0, 16u);
    std::memset(state + 108u, 0, 16u);
}

std::int64_t auro_asc4he_v1_ElevationEQ_process(float* block, std::uint64_t* io_pair) {
    if (!block || !io_pair)
        return 0;
    const std::uintptr_t result = static_cast<std::uintptr_t>(io_pair[0]);
    const std::uintptr_t v3 = static_cast<std::uintptr_t>(io_pair[1]);
    if (result == 0u || v3 == 0u)
        return 0;
    asc4he_elevation_eq_section(block, result, v3, 0);
    asc4he_elevation_eq_section(block, result, v3, 9);
    asc4he_elevation_eq_section(block, result, v3, 18);
    asc4he_elevation_eq_section(block, result, v3, 27);
    return static_cast<std::int64_t>(result);
}

std::int64_t auro_asc4he_v1_VirtualHeight_initialize(std::uint8_t* a1, std::uint64_t a2) {
    if (!a1)
        return 1;
    std::memset(a1, 0, kAsc4heVirtualHeightStateBytes);
    std::uint8_t v3[16]{};
    *reinterpret_cast<std::int32_t*>(v3 + 0u) = 5;
    *reinterpret_cast<std::uint64_t*>(v3 + 4u) = 0x3ECCCCCD45674000ULL;
    *reinterpret_cast<std::int32_t*>(v3 + 12u) = 0x40000000;
    (void)auro_iir_biquad_parameter_Config_float32_t_compute(v3, a2, a1 + 16u);
    *reinterpret_cast<std::int32_t*>(v3 + 0u) = 7;
    *reinterpret_cast<std::uint64_t*>(v3 + 4u) = 0x3F00000045566000ULL;
    *reinterpret_cast<std::int32_t*>(v3 + 12u) = -1054867456;
    (void)auro_iir_biquad_parameter_Config_float32_t_compute(v3, a2, a1 + 52u);
    constexpr std::uint32_t kBufOff = 72u;
    constexpr std::uint32_t kDelayOff = 840u;
    const std::int64_t dr = auro_asc4he_v1_blocked_Delay_initialize(
        a1 + kDelayOff,
        416,
        a1 + kBufOff,
        192,
        static_cast<std::int32_t>(static_cast<std::uint32_t>(a2)));
    if (dr != 0)
        return dr;
    *reinterpret_cast<std::uint32_t*>(a1 + 872u) = 1063528709u;
    std::memset(a1, 0, 16u);
    std::memset(a1 + 36u, 0, 16u);
    auro_asc4he_v1_blocked_Delay_reset_audio_state(a1 + kDelayOff);
    return 0;
}

std::int64_t auro_asc4he_v1_VirtualHeight_reset_audio_state(std::uint8_t* a1) {
    if (!a1)
        return 0;
    std::memset(a1, 0, 16u);
    std::memset(a1 + 36u, 0, 16u);
    return auro_asc4he_v1_blocked_Delay_reset_audio_state(a1 + 840u);
}

namespace {

inline void asc4he_vh_first_biquad_scalar(std::uint8_t* a1, float* ch0, float* ch1) noexcept {
    float v36 = *reinterpret_cast<float*>(a1 + 0u);
    float v37 = *reinterpret_cast<float*>(a1 + 4u);
    float v38 = *reinterpret_cast<float*>(a1 + 8u);
    float v39 = *reinterpret_cast<float*>(a1 + 12u);
    for (std::int32_t i = 0; i != 32; ++i) {
        const float v41 = ch0[i];
        const float v42 = ch1[i];
        const float v43 = *reinterpret_cast<float*>(a1 + 16u);
        const float v44 = v43 * v42 + v38;
        const float v45 = v43 * v41 + v36;
        v36 = *reinterpret_cast<float*>(a1 + 20u) * v41 + v37 - *reinterpret_cast<float*>(a1 + 28u) * v45;
        v38 = *reinterpret_cast<float*>(a1 + 20u) * v42 + v39 - *reinterpret_cast<float*>(a1 + 28u) * v44;
        const float v46 = *reinterpret_cast<float*>(a1 + 24u);
        const float v47 = *reinterpret_cast<float*>(a1 + 32u);
        ch0[i] = v45;
        v37 = v41 * v46 - v45 * v47;
        v39 = v46 * v42 - v47 * v44;
        ch1[i] = v44;
    }
    *reinterpret_cast<float*>(a1 + 0u) = v36;
    *reinterpret_cast<float*>(a1 + 8u) = v38;
    *reinterpret_cast<float*>(a1 + 4u) = v37;
    *reinterpret_cast<float*>(a1 + 12u) = v39;
}

inline void asc4he_vh_second_biquad_scalar(std::uint8_t* a1, float* w75f, float* w66f) noexcept {
    float v48 = *reinterpret_cast<float*>(a1 + 36u);
    float v49 = *reinterpret_cast<float*>(a1 + 40u);
    float v50 = *reinterpret_cast<float*>(a1 + 44u);
    float v51 = *reinterpret_cast<float*>(a1 + 48u);
    const float v52 = *reinterpret_cast<float*>(a1 + 52u);
    const float v53 = *reinterpret_cast<float*>(a1 + 56u);
    const float v54 = -*reinterpret_cast<float*>(a1 + 64u);
    const float v55 = *reinterpret_cast<float*>(a1 + 60u);
    const float v56 = -*reinterpret_cast<float*>(a1 + 68u);
    for (std::int32_t j = 0; j != 32; ++j) {
        const float v58 = w75f[j];
        const float v59 = w66f[j];
        const float v60 = v52 * v58 + v48;
        const float v61 = v52 * v59 + v50;
        v48 = v54 * v60 + (v53 * v58 + v49);
        v50 = v54 * v61 + (v53 * v59 + v51);
        w75f[j] = v60;
        v49 = v58 * v55 + v60 * v56;
        w66f[j] = v61;
        v51 = v59 * v55 + v61 * v56;
    }
    *reinterpret_cast<float*>(a1 + 36u) = v48;
    *reinterpret_cast<float*>(a1 + 44u) = v50;
    *reinterpret_cast<float*>(a1 + 40u) = v49;
    *reinterpret_cast<float*>(a1 + 48u) = v51;
}

inline std::int64_t asc4he_vh_process_full_scalar(std::uint8_t* a1, void** a2) noexcept {
    auto* const ch0 = static_cast<float*>(a2[0]);
    auto* const ch1 = static_cast<float*>(a2[1]);
    if (!ch0 || !ch1)
        return 0;
    alignas(16) float w75[32];
    alignas(16) float w66[32];
    std::memcpy(w75, ch0, sizeof(w75));
    std::memcpy(w66, ch1, sizeof(w66));
    asc4he_vh_first_biquad_scalar(a1, ch0, ch1);
    asc4he_vh_second_biquad_scalar(a1, w75, w66);
    alignas(8) std::uint8_t pair[16]{};
    *reinterpret_cast<std::uint64_t*>(pair + 0u) = reinterpret_cast<std::uint64_t>(w75);
    *reinterpret_cast<std::uint64_t*>(pair + 8u) = reinterpret_cast<std::uint64_t>(w66);
    (void)auro_asc4he_v1_blocked_Delay_process(a1 + 840u, pair);
    const float g = *reinterpret_cast<const float*>(a1 + 872u);
    for (std::int32_t k = 0; k != 32; k += 2) {
        ch0[k] += w66[k] * g;
        ch1[k] += w75[k] * g;
        ch0[k + 1] += w66[k + 1] * g;
        ch1[k + 1] += w75[k + 1] * g;
    }
    return reinterpret_cast<std::int64_t>(a2[0]);
}

} // namespace

#if AURO3DENG_CRC_SSE41
namespace {

union alignas(16) M128u {
    __m128 m;
    std::int32_t i[4];
    std::uint32_t u[4];
    float f[4];
};

inline void asc4he_vh_pack_block(const __m128& v2b, const __m128& v3b, __m128* out75, __m128* out66) noexcept {
    M128u v2u;
    M128u v3u;
    v2u.m = v2b;
    v3u.m = v3b;
    M128u o75{};
    o75.i[0] = v2u.i[0];
    o75.i[1] = v2u.i[1];
    const std::uint64_t pr =
        (static_cast<std::uint64_t>(static_cast<std::uint32_t>(v2u.i[3])) << 32u)
        | static_cast<std::uint32_t>(v2u.i[2]);
    std::memcpy(&o75.i[2], &pr, sizeof(pr));
    __m128 v6 = _mm_castsi128_ps(_mm_cvtsi32_si128(static_cast<int>(v3u.u[0])));
    __m128 inserted =
        _mm_insert_ps(v6, _mm_castsi128_ps(_mm_cvtsi32_si128(static_cast<int>(v3u.u[1]))), 16);
    inserted = _mm_insert_ps(
        inserted, _mm_castsi128_ps(_mm_cvtsi32_si128(static_cast<int>(v3u.u[2]))), 32);
    *out66 = _mm_insert_ps(
        inserted, _mm_castsi128_ps(_mm_cvtsi32_si128(static_cast<int>(v3u.u[3]))), 48);
    *out75 = o75.m;
}

inline void asc4he_vh_first_biquad_on_channels(std::uint8_t* a1, float* ch0, float* ch1) noexcept {
    float v36 = *reinterpret_cast<float*>(a1 + 0u);
    float v37 = *reinterpret_cast<float*>(a1 + 4u);
    float v38 = *reinterpret_cast<float*>(a1 + 8u);
    float v39 = *reinterpret_cast<float*>(a1 + 12u);
    for (std::int32_t i = 0; i != 32; ++i) {
        const float v41 = ch0[i];
        const float v42 = ch1[i];
        const float v43 = *reinterpret_cast<float*>(a1 + 16u);
        const float v44 = v43 * v42 + v38;
        const float v45 = v43 * v41 + v36;
        v36 = *reinterpret_cast<float*>(a1 + 20u) * v41 + v37 - *reinterpret_cast<float*>(a1 + 28u) * v45;
        v38 = *reinterpret_cast<float*>(a1 + 20u) * v42 + v39 - *reinterpret_cast<float*>(a1 + 28u) * v44;
        const float v46 = *reinterpret_cast<float*>(a1 + 24u);
        const float v47 = *reinterpret_cast<float*>(a1 + 32u);
        ch0[i] = v45;
        v37 = v41 * v46 - v45 * v47;
        v39 = v46 * v42 - v47 * v44;
        ch1[i] = v44;
    }
    *reinterpret_cast<float*>(a1 + 0u) = v36;
    *reinterpret_cast<float*>(a1 + 8u) = v38;
    *reinterpret_cast<float*>(a1 + 4u) = v37;
    *reinterpret_cast<float*>(a1 + 12u) = v39;
}

inline void asc4he_vh_second_biquad_on_stack(std::uint8_t* a1, float* w75f, float* w66f) noexcept {
    float v48 = *reinterpret_cast<float*>(a1 + 36u);
    float v49 = *reinterpret_cast<float*>(a1 + 40u);
    float v50 = *reinterpret_cast<float*>(a1 + 44u);
    float v51 = *reinterpret_cast<float*>(a1 + 48u);
    const float v52 = *reinterpret_cast<float*>(a1 + 52u);
    const float v53 = *reinterpret_cast<float*>(a1 + 56u);
    const float v54 = -*reinterpret_cast<float*>(a1 + 64u);
    const float v55 = *reinterpret_cast<float*>(a1 + 60u);
    const float v56 = -*reinterpret_cast<float*>(a1 + 68u);
    for (std::int32_t j = 0; j != 32; ++j) {
        const float v58 = w75f[j];
        const float v59 = w66f[j];
        const float v60 = v52 * v58 + v48;
        const float v61 = v52 * v59 + v50;
        v48 = v54 * v60 + (v53 * v58 + v49);
        v50 = v54 * v61 + (v53 * v59 + v51);
        w75f[j] = v60;
        v49 = v58 * v55 + v60 * v56;
        w66f[j] = v61;
        v51 = v59 * v55 + v61 * v56;
    }
    *reinterpret_cast<float*>(a1 + 36u) = v48;
    *reinterpret_cast<float*>(a1 + 44u) = v50;
    *reinterpret_cast<float*>(a1 + 40u) = v49;
    *reinterpret_cast<float*>(a1 + 48u) = v51;
}

inline std::int64_t asc4he_vh_process_full_sse41(std::uint8_t* a1, void** a2) noexcept {
    auto* const v2 = reinterpret_cast<__m128*>(a2[0]);
    auto* const v3 = reinterpret_cast<__m128*>(a2[1]);
    if (!v2 || !v3)
        return 0;
    alignas(16) __m128 w75[8];
    alignas(16) __m128 w66[8];
    for (int b = 0; b < 8; ++b)
        asc4he_vh_pack_block(v2[b], v3[b], &w75[b], &w66[b]);
    float* const ch0f = reinterpret_cast<float*>(v2);
    float* const ch1f = reinterpret_cast<float*>(v3);
    asc4he_vh_first_biquad_on_channels(a1, ch0f, ch1f);
    float* const w75f = reinterpret_cast<float*>(w75);
    float* const w66f = reinterpret_cast<float*>(w66);
    asc4he_vh_second_biquad_on_stack(a1, w75f, w66f);
    std::uint32_t* hold0 = reinterpret_cast<std::uint32_t*>(w75);
    std::uint32_t* hold1 = reinterpret_cast<std::uint32_t*>(w66);
    alignas(8) std::uint8_t pair[16]{};
    *reinterpret_cast<std::uint64_t*>(pair + 0u) = reinterpret_cast<std::uint64_t>(hold0);
    *reinterpret_cast<std::uint64_t*>(pair + 8u) = reinterpret_cast<std::uint64_t>(hold1);
    (void)auro_asc4he_v1_blocked_Delay_process(a1 + 840u, pair);
    const float g = *reinterpret_cast<const float*>(a1 + 872u);
    for (std::int32_t k = 0; k != 32; k += 2) {
        ch0f[k] += w66f[k] * g;
        ch1f[k] += w75f[k] * g;
        ch0f[k + 1] += w66f[k + 1] * g;
        ch1f[k + 1] += w75f[k + 1] * g;
    }
    return reinterpret_cast<std::int64_t>(a2[0]);
}

} // namespace
#endif

std::int64_t auro_asc4he_v1_VirtualHeight_process(std::uint8_t* state, void** channel_ptrs) {
    if (!state || !channel_ptrs)
        return 0;
#if AURO3DENG_CRC_SSE41
    return asc4he_vh_process_full_sse41(state, channel_ptrs);
#else
    return asc4he_vh_process_full_scalar(state, channel_ptrs);
#endif
}

std::int64_t auro_asc4he_v1_CrossTalk_initialize(std::uint8_t* a1, const float* a2) {
    if (!a1 || !a2)
        return 1;
    std::memset(a1 + 0u, 0, 84u);
    const float v2 = *a2;
    *reinterpret_cast<float*>(a1 + 12u) = v2;
    const std::uint32_t lo = *reinterpret_cast<const std::uint32_t*>(&v2);
    if (lo - 1u > 7u)
        return 1;
    *reinterpret_cast<std::uint32_t*>(a1 + 8u) = 7u;
    *reinterpret_cast<std::uint32_t*>(a1 + 4u) = static_cast<std::uint8_t>(lo) & 7u;
    float v5 = 0.f;
    const float v4 = a2[1];
    if (v4 > -144.f)
        v5 = std::pow(10.f, v4 * 0.05f);
    *reinterpret_cast<float*>(a1 + 16u) = v5;
    return 0;
}

void auro_asc4he_v1_CrossTalk_reset_audio_state(std::uint8_t* a1) {
    if (!a1)
        return;
    std::memset(a1 + 20u, 0, 64u);
}

std::int64_t auro_asc4he_v1_CrossTalk_process(std::uint8_t* a1, std::uint64_t* io_pair) {
    if (!a1 || !io_pair)
        return 0;
    const std::uintptr_t result = static_cast<std::uintptr_t>(io_pair[0]);
    const std::uintptr_t v3 = static_cast<std::uintptr_t>(io_pair[1]);
    if (result == 0u || v3 == 0u)
        return 0;
    std::uint32_t v4_0 = *reinterpret_cast<const std::uint32_t*>(a1 + 0u);
    std::uint32_t v4_1 = *reinterpret_cast<const std::uint32_t*>(a1 + 4u);
    const float v5 = *reinterpret_cast<const float*>(a1 + 16u);
    const std::uint32_t v6 = *reinterpret_cast<const std::uint32_t*>(a1 + 8u);
    for (std::uint32_t i = 0; i != 32u; ++i) {
        const std::uint32_t v8 = v4_0;
        float v9 = *reinterpret_cast<float*>(result + 4u * static_cast<std::uint32_t>(i))
            + *reinterpret_cast<float*>(a1 + 4ull * static_cast<std::uint64_t>(v8) + 52u);
        float v10 = *reinterpret_cast<float*>(v3 + 4u * static_cast<std::uint32_t>(i))
            + *reinterpret_cast<float*>(a1 + 4ull * static_cast<std::uint64_t>(v8) + 20u);
        *reinterpret_cast<float*>(result + 4u * static_cast<std::uint32_t>(i)) = v9;
        *reinterpret_cast<float*>(v3 + 4u * static_cast<std::uint32_t>(i)) = v10;
        const std::uint32_t epi32 = v4_1;
        *reinterpret_cast<float*>(a1 + 4ull * static_cast<std::uint64_t>(epi32) + 20u) = -v9 * v5;
        *reinterpret_cast<float*>(a1 + 4ull * static_cast<std::uint64_t>(epi32) + 52u) = -v10 * v5;
        v4_0 = (v4_0 + 1u) & v6;
        v4_1 = (v4_1 + 1u) & v6;
    }
    const std::uint64_t lo64 = static_cast<std::uint64_t>(v4_0) | (static_cast<std::uint64_t>(v4_1) << 32u);
    *reinterpret_cast<std::uint64_t*>(a1) = lo64;
    return static_cast<std::int64_t>(result);
}

void auro_centergen_v3_default_fixed_params(std::uint8_t* a1) {
    if (!a1)
        return;
    std::memset(a1, 0, kAsc4heCenterGenFixedDefaultsBytes);
    *reinterpret_cast<std::uint64_t*>(a1 + 0u) = 1ull;
    *reinterpret_cast<std::uint64_t*>(a1 + 8u) = 0ull;
    *reinterpret_cast<std::uint32_t*>(a1 + 16u) = 524288200u;
}

void auro_centergen_v3_default_dynamic_params(std::uint8_t* a1) {
    if (!a1)
        return;
    std::memset(a1, 0, kAsc4heCenterGenDynamicDefaultsBytes);
    *reinterpret_cast<std::uint32_t*>(a1 + 0u) = 0u;
    // libauro xmmword_1DC9F0 / 1DD660 / 1DCF70 @ 0x58C8F0
    *reinterpret_cast<std::uint64_t*>(a1 + 4u) = 0x3F59999A40C00000ull;
    *reinterpret_cast<std::uint64_t*>(a1 + 12u) = 0xC17000003D4CCCCDull;
    *reinterpret_cast<std::uint64_t*>(a1 + 20u) = 0x00000000C1200000ull;
    *reinterpret_cast<std::uint64_t*>(a1 + 28u) = 0x3DA3D70A3F800000ull;
    *reinterpret_cast<std::uint64_t*>(a1 + 36u) = 0x41A000003E99999Aull;
    *reinterpret_cast<std::uint64_t*>(a1 + 44u) = 0xC09000003F7D70A4ull;
    *reinterpret_cast<std::uint32_t*>(a1 + 52u) = 0u;
}

std::int64_t auro_centergen_v3_Processor_set_fixed_parameters(std::uint8_t* a1, const std::uint8_t* a2) {
    if (!a1 || !a2)
        return 0;
    *reinterpret_cast<std::uint32_t*>(a1 + 16u) = *reinterpret_cast<const std::uint32_t*>(a2 + 16u);
    std::memcpy(a1, a2, 16u);
    std::memcpy(a1 + 92u, a2, 16u);
    alignas(8) std::uint8_t v5stack[16]{};
    *reinterpret_cast<std::uint32_t*>(v5stack + 12u) = 0u;
    *reinterpret_cast<std::uint32_t*>(v5stack + 0u) = 2u;
    *reinterpret_cast<float*>(v5stack + 4u) =
        static_cast<float>(*reinterpret_cast<const std::uint16_t*>(a1 + 16u));
    *reinterpret_cast<std::uint32_t*>(v5stack + 8u) = 1065353216u;
    const std::uint32_t mode = *reinterpret_cast<std::uint32_t*>(a1 + 84u);
    const std::uint32_t sr = *reinterpret_cast<const std::uint32_t*>(a1 + 80u);
    if (mode == 1u) {
        (void)auro_iir_biquad_parameter_Config_float32_t_compute(v5stack, sr, a1 + 256u);
    } else if (mode == 2u) {
        (void)centergen_iir_float64_coeffs_to48(v5stack, sr, a1 + 256u);
    } else if (mode == 0u) {
        (void)centergen_iir_fixed32_coeffs_to48(v5stack, sr, a1 + 256u);
    }
    *reinterpret_cast<std::uint32_t*>(v5stack + 12u) = 0u;
    *reinterpret_cast<std::uint32_t*>(v5stack + 0u) = 1u;
    *reinterpret_cast<float*>(v5stack + 4u) =
        static_cast<float>(*reinterpret_cast<const std::uint16_t*>(a1 + 18u));
    *reinterpret_cast<std::uint32_t*>(v5stack + 8u) = 1065353216u;
    if (mode == 1u) {
        (void)auro_iir_biquad_parameter_Config_float32_t_compute(v5stack, sr, a1 + 304u);
    } else if (mode == 2u) {
        (void)centergen_iir_float64_coeffs_to48(v5stack, sr, a1 + 304u);
    } else if (mode == 0u) {
        (void)centergen_iir_fixed32_coeffs_to48(v5stack, sr, a1 + 304u);
    }
    std::memset(a1 + 344u, 0, 0x2C0u);
    const std::uint32_t m = *reinterpret_cast<std::uint32_t*>(a1 + 84u);
    *reinterpret_cast<std::uint32_t*>(a1 + 344u) = m;
    *reinterpret_cast<std::uint32_t*>(a1 + 392u) = m;
    *reinterpret_cast<std::uint32_t*>(a1 + 368u) = m;
    *reinterpret_cast<std::uint32_t*>(a1 + 416u) = m;
    std::memcpy(a1 + 1048u, a1 + 192u, 16u);
    return static_cast<std::int64_t>(m);
}

void auro_centergen_v3_Processor_set_dynamic_parameters(std::uint8_t* a1, const std::uint8_t* a2) {
    if (!a1 || !a2)
        return;
    auro_centergen_v3_Processor_set_dynamic_parameters_5871d0_body(a1, a2);
}

std::uint64_t auro_centergen_v3_Processor_t_construct(std::uint8_t* a1, std::int32_t* a2) {
    if (!a1 || !a2)
        return 0u;
    std::uint8_t* const p = a1;
    std::memset(p, 0, kCentergenV3ProcessorBytes);
    const std::int32_t mode = a2[1];
    const std::uint32_t mode_u = static_cast<std::uint32_t>(mode);
    *reinterpret_cast<std::uint32_t*>(p + 84u) = mode_u;
    *reinterpret_cast<std::uint32_t*>(p + 248u) = mode_u;
    *reinterpret_cast<std::uint32_t*>(p + 296u) = mode_u;
    *reinterpret_cast<std::uint32_t*>(p + 344u) = mode_u;
    *reinterpret_cast<std::uint32_t*>(p + 392u) = mode_u;
    *reinterpret_cast<std::uint32_t*>(p + 368u) = mode_u;
    *reinterpret_cast<std::uint32_t*>(p + 416u) = mode_u;
    std::int32_t v4 = a2[0];
    *reinterpret_cast<std::uint32_t*>(p + 80u) = static_cast<std::uint32_t>(v4);
    switch (mode) {
    case 2:
        *reinterpret_cast<std::uint64_t*>(p + 208u) = 0x3F60000000000000ull;
        v4 = a2[0];
        break;
    case 1:
        *reinterpret_cast<std::uint32_t*>(p + 208u) = 989855744u;
        v4 = a2[0];
        break;
    case 0:
        *reinterpret_cast<std::uint32_t*>(p + 208u) = 0x4000u;
        v4 = a2[0];
        break;
    default:
        break;
    }
    if (v4 == 44100) {
        *reinterpret_cast<std::uint32_t*>(p + 88u) = 32u;
        *reinterpret_cast<std::uint32_t*>(p + 80u) = 48000u;
    } else if (v4 == 48000) {
        *reinterpret_cast<std::uint32_t*>(p + 88u) = 32u;
    } else {
        return 0u;
    }
    alignas(16) std::uint8_t tmp[64]{};
    auro_centergen_v3_default_fixed_params(tmp);
    (void)auro_centergen_v3_Processor_set_fixed_parameters(p, tmp);
    auro_centergen_v3_default_dynamic_params(tmp);
    auro_centergen_v3_Processor_set_dynamic_parameters(p, tmp);
    std::memcpy(p + 1048u, p + 192u, 16u);
    return reinterpret_cast<std::uint64_t>(p);
}

std::int64_t auro_asc4he_v1_CenterGen_initialize(
    std::uint8_t* a1,
    std::int32_t a2,
    const std::uint8_t* a3,
    const std::uint8_t* a4) {
    std::int32_t v7[2];
    v7[0] = a2;
    v7[1] = 1;
    if (auro_centergen_v3_Processor_t_construct(a1, v7) == 0u)
        return 1;
    (void)auro_centergen_v3_Processor_set_fixed_parameters(a1, a3);
    auro_centergen_v3_Processor_set_dynamic_parameters(a1, a4);
    return 0;
}

std::int64_t auro_centergen_v3_Processor_reset_audio_state(std::uint8_t* a1) {
    if (!a1)
        return 0;
    std::memset(a1 + 344u, 0, 0x2C0u);
    const std::uint32_t m = *reinterpret_cast<std::uint32_t*>(a1 + 84u);
    *reinterpret_cast<std::uint32_t*>(a1 + 344u) = m;
    *reinterpret_cast<std::uint32_t*>(a1 + 392u) = m;
    *reinterpret_cast<std::uint32_t*>(a1 + 368u) = m;
    *reinterpret_cast<std::uint32_t*>(a1 + 416u) = m;
    std::memcpy(a1 + 1048u, a1 + 192u, 16u);
    return static_cast<std::int64_t>(m);
}

std::int64_t auro_asc4he_v1_CenterGen_reset_audio_state(std::uint8_t* a1) {
    return auro_centergen_v3_Processor_reset_audio_state(a1);
}

std::int64_t auro_centergen_v3_Processor_get_fixed_parameters(const std::uint8_t* a1, std::uint8_t* a2) {
    if (!a1 || !a2)
        return 0;
    const std::uint32_t r = *reinterpret_cast<const std::uint32_t*>(a1 + 16u);
    *reinterpret_cast<std::uint32_t*>(a2 + 16u) = r;
    std::memcpy(a2, a1, 16u);
    return static_cast<std::int64_t>(r);
}

std::int64_t auro_centergen_v3_Processor_get_dynamic_parameters(const std::uint8_t* a1, std::uint8_t* a2) {
    if (!a1 || !a2)
        return 0;
    const std::uint64_t r = *reinterpret_cast<const std::uint64_t*>(a1 + 68u);
    *reinterpret_cast<std::uint64_t*>(a2 + 48u) = r;
    std::memcpy(a2 + 0u, a1 + 20u, 16u);
    std::memcpy(a2 + 16u, a1 + 36u, 16u);
    std::memcpy(a2 + 32u, a1 + 52u, 16u);
    return static_cast<std::int64_t>(r);
}

std::int64_t auro_asc4he_v1_CenterGen_get_fixed_parameters(
    const std::uint8_t* proc,
    std::uint8_t* out20) {
    return auro_centergen_v3_Processor_get_fixed_parameters(proc, out20);
}

std::int64_t auro_asc4he_v1_CenterGen_set_dynamic_parameters(
    std::uint8_t* proc,
    const std::uint8_t* dyn56) {
    auro_centergen_v3_Processor_set_dynamic_parameters(proc, dyn56);
    return 0;
}

std::int64_t auro_asc4he_v1_CenterGen_get_dynamic_parameters(
    const std::uint8_t* proc,
    std::uint8_t* out56) {
    return auro_centergen_v3_Processor_get_dynamic_parameters(proc, out56);
}

namespace {

float cg_read_f32(const std::uint8_t* p, std::uintptr_t off) {
    return *reinterpret_cast<const float*>(p + off);
}

std::uint32_t cg_read_u32(const std::uint8_t* p, std::uintptr_t off) {
    return *reinterpret_cast<const std::uint32_t*>(p + off);
}

std::int32_t cg_read_i32(const std::uint8_t* p, std::uintptr_t off) {
    return *reinterpret_cast<const std::int32_t*>(p + off);
}

void cg_write_f32(std::uint8_t* p, std::uintptr_t off, float v) {
    *reinterpret_cast<float*>(p + off) = v;
}

void cg_write_u32(std::uint8_t* p, std::uintptr_t off, std::uint32_t v) {
    *reinterpret_cast<std::uint32_t*>(p + off) = v;
}

void cg_biquad32_float(std::uint8_t* proc, float* x, std::uintptr_t coeff_off, std::uintptr_t state_off) {
    const float b0 = cg_read_f32(proc, coeff_off + 0u);
    const float b1 = cg_read_f32(proc, coeff_off + 4u);
    const float b2 = cg_read_f32(proc, coeff_off + 8u);
    const float a1 = cg_read_f32(proc, coeff_off + 12u);
    const float a2 = cg_read_f32(proc, coeff_off + 16u);
    float s0 = cg_read_f32(proc, state_off + 0u);
    float s1 = cg_read_f32(proc, state_off + 4u);
    for (std::uint32_t i = 0; i != 32u; i += 2u) {
        const float x0 = x[i];
        const float y0 = b0 * x0 + s0;
        x[i] = y0;
        const float t0 = x0 * b2 - y0 * a2;
        const float x1 = x[i + 1u];
        const float y1 = b0 * x1 + (b1 * x0 + s1 - a1 * y0);
        x[i + 1u] = y1;
        s0 = b1 * x1 + t0 - a1 * y1;
        s1 = x1 * b2 - y1 * a2;
    }
    cg_write_f32(proc, state_off + 0u, s0);
    cg_write_f32(proc, state_off + 4u, s1);
}

float cg_smooth_peak(float peak, float now, float coef) {
    const float count = 1.0f + static_cast<float>(coef <= 0.0f ? 0 : static_cast<int>(coef - 1.0f));
    return (peak * (count - 1.0f) + now) / count;
}

float cg_clamp01_positive(float v) {
    if (!(v > 0.0f))
        return 0.0f;
    return std::min(1.0f, v);
}

std::int32_t /* Decompiled name: auro_centergen_v3_Processor_float32_t_process */
auro_centergen_v3_Processor_float32_t_process(
    std::uint8_t* proc,
    float* a2,
    float* a3,
    float* a4,
    std::int32_t a5) {
    if (cg_read_i32(proc, 88u) != a5 || a5 != 32)
        return 1;
    const float v6 = cg_read_f32(proc, 184u);
    const float in_gain = cg_read_f32(proc, 112u);
    for (std::uint32_t i = 0; i != 32u; ++i) {
        a2[i] *= in_gain;
        a3[i] *= in_gain;
    }
    float v244[32];
    float v245[32];
    std::memcpy(v244, a2, sizeof(v244));
    std::memcpy(v245, a3, sizeof(v245));
    const bool v22 = cg_read_u32(proc, 92u) != 0u;
    if (v22 || cg_read_u32(proc, 96u) != 0u || cg_read_u32(proc, 100u) != 0u) {
        cg_biquad32_float(proc, v244, 304u, 400u);
        cg_biquad32_float(proc, v244, 256u, 352u);
        cg_biquad32_float(proc, v245, 304u, 424u);
        cg_biquad32_float(proc, v245, 256u, 376u);
    }
    float v253[32];
    float v254[32];
    const float* v72 = v22 ? v244 : a2;
    const float* v73 = v22 ? v245 : a3;
    for (std::uint32_t i = 0; i != 32u; ++i) {
        v253[i] = v72[i] + v73[i];
        v254[i] = v72[i] - v73[i];
    }
    const float energy_gain = cg_read_f32(proc, 208u);
    float v255[4]{};
    float* ring = reinterpret_cast<float*>(proc + 440u);
    const std::uint32_t ring_index = cg_read_u32(proc, 952u);
    const float* energy_src[4] = {v72, v73, v253, v254};
    for (std::uint32_t n = 0; n != 4u; ++n) {
        float acc = 0.0f;
        for (std::uint32_t i = 0; i != 32u; ++i)
            acc += energy_src[n][i] * energy_src[n][i] * energy_gain;
        ring[32u * n + ring_index] = acc;
        for (std::uint32_t i = 0; i != 16u; ++i)
            v255[n] += ring[32u * n + 2u * i];
    }
    cg_write_u32(proc, 952u, ring_index == 15u ? 0u : ring_index + 1u);
    float p0 = cg_read_f32(proc, 960u);
    float p1 = cg_read_f32(proc, 968u);
    float p2 = cg_read_f32(proc, 976u);
    float p3 = cg_read_f32(proc, 984u);
    const float peak_ratio = cg_read_f32(proc, 176u);
    std::uintptr_t smooth01 = 168u;
    if (v255[0] <= peak_ratio * p0 && p0 <= peak_ratio * v255[0]) {
        smooth01 = 168u;
        if (v255[1] <= peak_ratio * p1)
            smooth01 = (p1 <= peak_ratio * v255[1]) ? 172u : 168u;
    }
    const float c01 = static_cast<float>(cg_read_i32(proc, smooth01));
    p0 = cg_smooth_peak(p0, v255[0], c01);
    p1 = cg_smooth_peak(p1, v255[1], c01);
    std::uintptr_t smooth23 = 168u;
    if (v255[2] <= peak_ratio * p2 && p2 <= peak_ratio * v255[2]) {
        smooth23 = 168u;
        if (v255[3] <= peak_ratio * p3)
            smooth23 = (p3 <= peak_ratio * v255[3]) ? 172u : 168u;
    }
    const float c23 = static_cast<float>(cg_read_i32(proc, smooth23));
    p2 = cg_smooth_peak(p2, v255[2], c23);
    p3 = cg_smooth_peak(p3, v255[3], c23);
    cg_write_f32(proc, 960u, p0);
    cg_write_f32(proc, 968u, p1);
    cg_write_f32(proc, 976u, p2);
    cg_write_f32(proc, 984u, p3);
    const float lr = (p0 - p1) / (p0 + p1 + 0.00000011920929f);
    const float sr = (p2 - p3) / (p2 + p3 + 0.00000011920929f);
    const float ctrl0_raw = (std::fabs(lr) - cg_read_f32(proc, 120u)) / cg_read_f32(proc, 128u);
    const float ctrl1_raw = (cg_clamp01_positive(sr) - cg_read_f32(proc, 152u)) / cg_read_f32(proc, 160u);
    const float ctrl0 = (ctrl0_raw > 0.0f) ? 1.0f - std::min(1.0f, ctrl0_raw) : 1.0f;
    const float ctrl1 = (ctrl1_raw > 0.0f) ? std::min(1.0f, ctrl1_raw) : 1.0f;
    const bool dynamic_center = cg_read_u32(proc, 104u) != 0u;
    float v207 = cg_read_f32(proc, 136u);
    float mix0 = 0.0f;
    float mix1 = 0.0f;
    bool use_mix = cg_read_u32(proc, 108u) == 0u;
    if (dynamic_center) {
        const float base = cg_read_f32(proc, 144u);
        const float dist0 = std::max((lr - cg_read_f32(proc, 120u)) / cg_read_f32(proc, 128u), 0.0f);
        const float dist1 = std::max((-lr - cg_read_f32(proc, 120u)) / cg_read_f32(proc, 128u), 0.0f);
        const float t0 = ctrl1 * dist0;
        const float t1 = ctrl1 * dist1;
        const float u0 = (1.0f - ctrl1) * dist0 + (1.0f - dist0 - ctrl1 + t0) + t0;
        const float u1 = (1.0f - ctrl1) * dist1 + (1.0f - dist1 - ctrl1 + t1) + t1;
        mix0 = std::max(0.707000017f * t0 + u0, base);
        mix1 = std::max(0.707000017f * t1 + u1, base);
        v207 = std::max(0.293000013f * t1 + (ctrl0 * ctrl1 + 0.293000013f * t0), v207);
    } else if (ctrl0 == 0.0f || ctrl1 == 0.0f) {
        use_mix = cg_read_u32(proc, 108u) == 0u;
    } else {
        v207 = std::max(ctrl0 * ctrl1, v207);
        const float q0 = p0 + 0.00000011920929f;
        const float q1 = p1 + 0.00000011920929f;
        const float ratio0 = q0 / q1;
        const float ratio1 = q1 / q0;
        const float v225 = (1.0f - cg_read_f32(proc, 144u)) * ctrl0;
        mix0 = v225 * ratio0 * ctrl1;
        mix1 = (v225 / ratio0) * ctrl1;
    }
    if (cg_read_u32(proc, 108u) != 0u)
        v207 = 0.5f;
    const float* v227 = cg_read_u32(proc, 96u) ? v245 : a3;
    const float* v228 = cg_read_u32(proc, 96u) ? v244 : a2;
    const float* v21 = cg_read_u32(proc, 100u) ? v245 : a3;
    const float* v71 = cg_read_u32(proc, 100u) ? v244 : a2;
    float v229 = cg_read_f32(proc, 1024u);
    const float v230 = v207 * (1.0f - v6);
    float ins0 = cg_read_f32(proc, 1032u);
    float ins1 = cg_read_f32(proc, 1040u);
    const float add0 = (1.0f - v6) * (use_mix ? mix0 : 0.0f);
    const float add1 = (1.0f - v6) * (use_mix ? mix1 : 0.0f);
    for (std::uint32_t i = 0; i != 32u; ++i) {
        v229 = v229 * v6 + v230;
        ins0 = std::min(1.0f, ins0 * v6 + add0);
        ins1 = std::min(1.0f, ins1 * v6 + add1);
        if (!(ins0 > 0.0f))
            ins0 = 0.0f;
        if (!(ins1 > 0.0f))
            ins1 = 0.0f;
        a4[i] = (v228[i] + v227[i]) * v229;
        if (dynamic_center) {
            a2[i] = v71[i] * ins0;
            a3[i] = v21[i] * ins1;
        } else {
            const float side = ins1 * v71[i];
            a2[i] = a2[i] - v21[i] * ins0;
            a3[i] = a3[i] - side;
        }
    }
    cg_write_f32(proc, 1024u, v229);
    cg_write_f32(proc, 1032u, ins0);
    cg_write_f32(proc, 1040u, ins1);
    float v239 = cg_read_f32(proc, 1048u);
    float v240 = cg_read_f32(proc, 1056u);
    const float v241 = cg_read_f32(proc, 200u) * (1.0f - v6);
    const float v242 = (1.0f - v6) * cg_read_f32(proc, 192u);
    for (std::uint32_t i = 0; i != 32u; ++i) {
        v240 = v240 * v6 + v241;
        a2[i] *= v240;
        a3[i] *= v240;
        v239 = v239 * v6 + v242;
        a4[i] *= v239;
    }
    cg_write_f32(proc, 1056u, v240);
    cg_write_f32(proc, 1048u, v239);
    return 0;
}

} // namespace

std::int64_t auro_centergen_v3_Processor_process(
    std::uint8_t* proc,
    std::uint64_t io_pair_q0,
    std::uint64_t io_pair_q1,
    std::uint8_t* block_or_side_ctx,
    std::int32_t frame_count) {
    if (!proc || io_pair_q0 == 0u || io_pair_q1 == 0u || !block_or_side_ctx)
        return 0;
    const std::int32_t mode = cg_read_i32(proc, 84u);
    if (mode == 2)
        return 0;
    if (mode == 1) {
        const std::int32_t r = /* Decompiled name: auro_centergen_v3_Processor_float32_t_process */
auro_centergen_v3_Processor_float32_t_process(
            proc,
            reinterpret_cast<float*>(static_cast<std::uintptr_t>(io_pair_q0)),
            reinterpret_cast<float*>(static_cast<std::uintptr_t>(io_pair_q1)),
            reinterpret_cast<float*>(block_or_side_ctx),
            frame_count);
        return r ? 1 : 0;
    }
    if (mode == 0)
        return 0;
    return 0;
}

std::int64_t auro_asc4he_v1_CenterGen_process(
    std::uint8_t* proc,
    std::uint64_t* io_pair,
    std::uint8_t* block_or_side_ctx) {
    if (!proc || !io_pair || !block_or_side_ctx)
        return 0;
    return auro_centergen_v3_Processor_process(
        proc, io_pair[0], io_pair[1], block_or_side_ctx, 32);
}

std::int64_t auro_asc4he_v1_CenterCrossOver_initialize(
    std::uint8_t* a1,
    std::uint64_t sample_rate,
    std::int32_t a3) {
    if (!a1)
        return 1;
    std::memset(a1, 0, kAsc4heCenterCrossOverStateBytes);
    *reinterpret_cast<std::uint32_t*>(a1 + 56u) = static_cast<std::uint32_t>(a3);
    alignas(8) std::uint8_t v6[16]{};
    *reinterpret_cast<std::int32_t*>(v6 + 0u) = 9;
    *reinterpret_cast<std::uint64_t*>(v6 + 4u) = 0x3F33333343FA0000ull;
    *reinterpret_cast<std::int32_t*>(v6 + 12u) = 0;
    (void)auro_iir_biquad_parameter_Config_float32_t_compute(v6, sample_rate, a1 + 16u);
    if (*reinterpret_cast<const std::uint32_t*>(a1 + 56u) != 0u) {
        *reinterpret_cast<std::int32_t*>(v6 + 0u) = 9;
        *reinterpret_cast<std::uint64_t*>(v6 + 4u) = 0x3F33333345BB8000ull;
        *reinterpret_cast<std::int32_t*>(v6 + 12u) = 0;
        (void)auro_iir_biquad_parameter_Config_float32_t_compute(v6, sample_rate, a1 + 36u);
        *reinterpret_cast<std::uint64_t*>(a1 + 0u) = 0u;
        *reinterpret_cast<std::uint64_t*>(a1 + 8u) = 0u;
    } else {
        *reinterpret_cast<std::uint64_t*>(a1 + 0u) = 0u;
    }
    return 0;
}

void auro_asc4he_v1_CenterCrossOver_reset_audio_state(std::uint8_t* a1) {
    if (!a1)
        return;
    *reinterpret_cast<std::uint64_t*>(a1 + 0u) = 0u;
    if (*reinterpret_cast<const std::uint32_t*>(a1 + 56u) != 0u)
        *reinterpret_cast<std::uint64_t*>(a1 + 8u) = 0u;
}

std::int64_t auro_asc4he_v1_CenterCrossOver_process(
    std::uint8_t* a1,
    std::uint64_t a2,
    std::uint64_t a3) {
    if (!a1 || a2 == 0u || a3 == 0u)
        return 0;
    auto* out = reinterpret_cast<float*>(static_cast<std::uintptr_t>(a2));
    const auto* in = reinterpret_cast<const float*>(static_cast<std::uintptr_t>(a3));
    float v3 = *reinterpret_cast<float*>(a1 + 0u);
    float v4 = *reinterpret_cast<float*>(a1 + 4u);
    for (std::int64_t result = 0; result != 32; ++result) {
        const float v6 = in[result];
        const float v7 = *reinterpret_cast<const float*>(a1 + 16u) * v6 + v3;
        v3 = (*reinterpret_cast<const float*>(a1 + 20u) * v6 + v4)
            - (*reinterpret_cast<const float*>(a1 + 28u) * v7);
        v4 = (v6 * *reinterpret_cast<const float*>(a1 + 24u))
            - (*reinterpret_cast<const float*>(a1 + 32u) * v7);
        out[result] = v7;
    }
    *reinterpret_cast<float*>(a1 + 0u) = v3;
    *reinterpret_cast<float*>(a1 + 4u) = v4;
    if (*reinterpret_cast<const std::uint32_t*>(a1 + 56u) != 0u) {
        float v8 = *reinterpret_cast<float*>(a1 + 8u);
        float v9 = *reinterpret_cast<float*>(a1 + 12u);
        for (std::int64_t result = 0; result != 32; ++result) {
            const float v10 = out[result];
            const float v11 = *reinterpret_cast<const float*>(a1 + 36u) * v10 + v8;
            v8 = (*reinterpret_cast<const float*>(a1 + 40u) * v10 + v9)
                - (*reinterpret_cast<const float*>(a1 + 48u) * v11);
            v9 = (v10 * *reinterpret_cast<const float*>(a1 + 44u))
                - (*reinterpret_cast<const float*>(a1 + 52u) * v11);
            out[result] = v11;
        }
        *reinterpret_cast<float*>(a1 + 8u) = v8;
        *reinterpret_cast<float*>(a1 + 12u) = v9;
    }
    return 32;
}

namespace {

inline float asc4he_db_gain(float db) noexcept {
    return db > -144.f ? std::pow(10.f, db * 0.05f) : 0.f;
}

inline void asc4he_scale_pair_32(float* a1, float* a2, float a3) noexcept {
    if (!a1 || !a2)
        return;
    for (std::uint32_t i = 0; i != 32u; ++i) {
        a1[i] *= a3;
        a2[i] *= a3;
    }
}

inline void asc4he_copy_pair_scaled_32(float* dst0, float* dst1, const float* src0, const float* src1, float gain)
    noexcept {
    if (!dst0 || !dst1 || !src0 || !src1)
        return;
    for (std::uint32_t i = 0; i != 32u; ++i) {
        dst0[i] = src0[i] * gain;
        dst1[i] = src1[i] * gain;
    }
}

inline void asc4he_add_pair_32(float* dst0, float* dst1, const float* src0, const float* src1) noexcept {
    if (!dst0 || !dst1 || !src0 || !src1)
        return;
    for (std::uint32_t i = 0; i != 32u; ++i) {
        dst0[i] += src0[i];
        dst1[i] += src1[i];
    }
}

inline void asc4he_add_mono_to_pair_32(float* dst0, float* dst1, const float* mono) noexcept {
    if (!dst0 || !dst1 || !mono)
        return;
    for (std::uint32_t i = 0; i != 32u; ++i) {
        dst0[i] += mono[i];
        dst1[i] += mono[i];
    }
}

inline void asc4he_add_scaled_mono_to_pair_32(float* dst0, float* dst1, const float* mono, float gain) noexcept {
    if (!dst0 || !dst1 || !mono)
        return;
    for (std::uint32_t i = 0; i != 32u; ++i) {
        dst0[i] += mono[i] * gain;
        dst1[i] += mono[i] * gain;
    }
}

inline void asc4he_mix_center_xover_outputs_32(
    float* dst0,
    float* dst1,
    const float* src0,
    const float* src1,
    const float* center,
    const float* decor0,
    const float* decor1,
    float src_gain,
    float center_gain,
    float decor_gain) noexcept {
    if (!dst0 || !dst1 || !src0 || !src1 || !center || !decor0 || !decor1)
        return;
    for (std::uint32_t i = 0; i != 32u; ++i) {
        dst0[i] = src0[i] * src_gain + center[i] * center_gain + decor0[i] * decor_gain;
        dst1[i] = src1[i] * src_gain + center[i] * center_gain + decor1[i] * decor_gain;
    }
}

inline void asc4he_add_scaled_pair_32(float* dst0, float* dst1, const float* src0, const float* src1, float gain)
    noexcept {
    if (!dst0 || !dst1 || !src0 || !src1)
        return;
    for (std::uint32_t i = 0; i != 32u; ++i) {
        dst0[i] += src0[i] * gain;
        dst1[i] += src1[i] * gain;
    }
}

inline void asc4he_scale_existing_pair_32(std::uint64_t pair0, std::uint64_t pair1, float gain) noexcept {
    auto* const ch0 = reinterpret_cast<float*>(static_cast<std::uintptr_t>(pair0));
    auto* const ch1 = reinterpret_cast<float*>(static_cast<std::uintptr_t>(pair1));
    asc4he_scale_pair_32(ch0, ch1, gain);
}

inline float* asc4he_float_ptr(std::uint64_t ptr) noexcept {
    return reinterpret_cast<float*>(static_cast<std::uintptr_t>(ptr));
}

inline std::uint64_t asc4he_u64_ptr(const float* ptr) noexcept {
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(ptr));
}

inline void asc4he_copy_pair_32(float* dst0, float* dst1, const float* src0, const float* src1) noexcept {
    asc4he_copy_pair_scaled_32(dst0, dst1, src0, src1, 1.f);
}

inline void asc4he_zero_pair_32(float* dst0, float* dst1) noexcept {
    if (!dst0 || !dst1)
        return;
    for (std::uint32_t i = 0; i != 32u; ++i) {
        dst0[i] = 0.0f;
        dst1[i] = 0.0f;
    }
}

inline std::int64_t asc4he_butterworth_crossover_float_initialize(
    std::uint8_t* state,
    std::uint32_t sample_rate,
    std::int32_t invert_high_band,
    float cutoff_hz) noexcept {
    if (!state || sample_rate == 0u)
        return 0;
    std::memset(state, 0, 60u);
    alignas(8) std::uint8_t cfg[16]{};
    *reinterpret_cast<std::int32_t*>(cfg + 0u) = 1;
    *reinterpret_cast<float*>(cfg + 4u) = cutoff_hz;
    *reinterpret_cast<float*>(cfg + 8u) = 0.70710677f;
    if (auro_iir_biquad_parameter_Config_float32_t_compute(cfg, sample_rate, state + 8u) == 0)
        return 0;
    *reinterpret_cast<std::int32_t*>(cfg + 0u) = 2;
    if (auro_iir_biquad_parameter_Config_float32_t_compute(cfg, sample_rate, state + 36u) == 0)
        return 0;
    *reinterpret_cast<std::uint32_t*>(state + 56u) = invert_high_band ? 1u : 0u;
    return 1;
}

inline std::int64_t asc4he_butterworth_crossover_float_process(
    std::uint8_t* state,
    const float* input,
    float* low,
    float* high) noexcept {
    if (!state || !input || !low || !high)
        return 0;
    const auto in_addr = reinterpret_cast<std::uintptr_t>(input);
    const auto low_addr = reinterpret_cast<std::uintptr_t>(low);
    if (in_addr >= low_addr) {
        if (in_addr <= low_addr || low_addr + 124u >= in_addr)
            return 0;
    } else if (in_addr + 124u >= low_addr) {
        return 0;
    }
    float z1 = *reinterpret_cast<float*>(state + 0u);
    float z2 = *reinterpret_cast<float*>(state + 4u);
    const float* c = reinterpret_cast<const float*>(state + 8u);
    for (std::uint32_t i = 0; i != 32u; ++i) {
        const float x = input[i];
        const float y = c[0] * x + z1;
        z1 = c[1] * x + z2 - c[3] * y;
        z2 = c[2] * x - c[4] * y;
        low[i] = y;
    }
    *reinterpret_cast<float*>(state + 0u) = z1;
    *reinterpret_cast<float*>(state + 4u) = z2;
    z1 = *reinterpret_cast<float*>(state + 28u);
    z2 = *reinterpret_cast<float*>(state + 32u);
    c = reinterpret_cast<const float*>(state + 36u);
    for (std::uint32_t i = 0; i != 32u; ++i) {
        const float x = input[i];
        const float y = c[0] * x + z1;
        z1 = c[1] * x + z2 - c[3] * y;
        z2 = c[2] * x - c[4] * y;
        high[i] = y;
    }
    *reinterpret_cast<float*>(state + 28u) = z1;
    *reinterpret_cast<float*>(state + 32u) = z2;
    if (*reinterpret_cast<const std::uint32_t*>(state + 56u) != 0u) {
        for (std::uint32_t i = 0; i != 32u; ++i)
            high[i] = -high[i];
    }
    return 1;
}

inline std::int64_t asc4he_butterworth_crossover_float_process_u64(
    std::uint8_t* state,
    std::uint64_t in,
    std::uint64_t low,
    std::uint64_t high) noexcept {
    return asc4he_butterworth_crossover_float_process(
        state,
        reinterpret_cast<const float*>(static_cast<std::uintptr_t>(in)),
        reinterpret_cast<float*>(static_cast<std::uintptr_t>(low)),
        reinterpret_cast<float*>(static_cast<std::uintptr_t>(high)));
}

inline void asc4he_butterworth_crossover_float_reset(std::uint8_t* state) noexcept {
    if (!state)
        return;
    *reinterpret_cast<std::uint64_t*>(state + 0u) = 0u;
    *reinterpret_cast<std::uint64_t*>(state + 28u) = 0u;
}

inline float asc4he_biquad_df2t_process_sample(const float* c, float* z, float x) noexcept {
    const float y = c[0] * x + z[0];
    z[0] = c[1] * x + z[1] - c[3] * y;
    z[1] = c[2] * x - c[4] * y;
    return y;
}

inline std::uint8_t* auro_iir_w32_LinkwitzRiley_4th_t_construct(
    std::uint8_t* a1,
    std::uint32_t a2,
    std::int32_t a3,
    std::int32_t a4,
    float a5) noexcept {
    if (!a1)
        return nullptr;
    const auto raw = reinterpret_cast<std::uintptr_t>(a1);
    auto* const state = reinterpret_cast<std::uint8_t*>(raw + ((8u - (raw & 7u)) & 7u));
    auto* const history_base = state + 104u;
    const auto hist_raw = reinterpret_cast<std::uintptr_t>(history_base);
    auto* const history = reinterpret_cast<std::uint8_t*>(hist_raw + ((8u - (hist_raw & 7u)) & 7u));

    *reinterpret_cast<std::uint32_t*>(state + 0u) = static_cast<std::uint32_t>(a4);
    alignas(8) std::uint8_t cfg[16]{};
    *reinterpret_cast<std::int32_t*>(cfg + 0u) = 1;
    *reinterpret_cast<float*>(cfg + 4u) = a5;
    *reinterpret_cast<float*>(cfg + 8u) = 0.70710677f;
    if (a4 == 1 || a4 == 2) {
        (void)auro_iir_biquad_parameter_Config_float32_t_compute(cfg, a2, state + 8u);
        *reinterpret_cast<std::int32_t*>(cfg + 0u) = 2;
        (void)auro_iir_biquad_parameter_Config_float32_t_compute(cfg, a2, state + 48u);
    } else {
        (void)auro_iir_biquad_parameter_Config_float32_t_compute(cfg, a2, state + 8u);
        *reinterpret_cast<std::int32_t*>(cfg + 0u) = 2;
        (void)auro_iir_biquad_parameter_Config_float32_t_compute(cfg, a2, state + 48u);
    }
    *reinterpret_cast<std::uint32_t*>(state + 88u) = static_cast<std::uint32_t>(a3);
    *reinterpret_cast<std::uint64_t*>(state + 96u) = a3 ? reinterpret_cast<std::uint64_t>(history) : 0u;
    return state;
}

inline std::int64_t auro_iir_w32_LinkwitzRiley_4th_reset_audio_state(std::uint8_t* a1) noexcept {
    if (!a1)
        return 0;
    const std::uint32_t channels = *reinterpret_cast<const std::uint32_t*>(a1 + 88u);
    auto* const history = reinterpret_cast<std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<const std::uint64_t*>(a1 + 96u)));
    if (!channels || !history)
        return channels;
    for (std::uint32_t ch = 0; ch != channels; ++ch) {
        auto* const h = history + ch * 64u;
        *reinterpret_cast<std::uint64_t*>(h + 0u) = 0u;
        *reinterpret_cast<std::uint64_t*>(h + 16u) = 0u;
        *reinterpret_cast<std::uint64_t*>(h + 32u) = 0u;
        *reinterpret_cast<std::uint64_t*>(h + 48u) = 0u;
    }
    return channels;
}

inline std::uint64_t auro_iir_w32_LinkwitzRiley_4th_process(
    std::uint8_t* a1,
    std::uint64_t* a2,
    std::uint64_t* a3,
    std::uint64_t* a4) noexcept {
    if (!a1 || !a2)
        return 0u;
    auto* const history = reinterpret_cast<std::uint8_t*>(
        static_cast<std::uintptr_t>(*reinterpret_cast<const std::uint64_t*>(a1 + 96u)));
    if (!history)
        return 0u;
    const float* coeff_low = reinterpret_cast<const float*>(a1 + 8u);
    const float* coeff_high = reinterpret_cast<const float*>(a1 + 48u);
    const std::uint32_t channels = *reinterpret_cast<const std::uint32_t*>(a1 + 88u);
    for (std::uint32_t ch = 0; ch != channels; ++ch) {
        const auto* const in = reinterpret_cast<const float*>(static_cast<std::uintptr_t>(a2[ch]));
        auto* const high = (a4 && a4[ch]) ? reinterpret_cast<float*>(static_cast<std::uintptr_t>(a4[ch])) : nullptr;
        if (!in)
            return 0u;
        auto* const h = reinterpret_cast<float*>(history + ch * 64u);
        for (std::uint32_t i = 0; i != 32u; ++i) {
            const float x = in[i];
            const float high1 = asc4he_biquad_df2t_process_sample(coeff_high, h + 8u, x);
            const float high2 = asc4he_biquad_df2t_process_sample(coeff_high, h + 12u, high1);
            if (high)
                high[i] = high2;
        }
    }
    for (std::uint32_t ch = 0; ch != channels; ++ch) {
        const auto* const in = reinterpret_cast<const float*>(static_cast<std::uintptr_t>(a2[ch]));
        auto* const low = (a3 && a3[ch]) ? reinterpret_cast<float*>(static_cast<std::uintptr_t>(a3[ch])) : nullptr;
        if (!in)
            return 0u;
        auto* const h = reinterpret_cast<float*>(history + ch * 64u);
        for (std::uint32_t i = 0; i != 32u; ++i) {
            const float x = in[i];
            const float low1 = asc4he_biquad_df2t_process_sample(coeff_low, h + 0u, x);
            const float low2 = asc4he_biquad_df2t_process_sample(coeff_low, h + 4u, low1);
            if (low)
                low[i] = low2;
        }
    }
    return a2[0];
}

} // namespace

