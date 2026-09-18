// Portable Auro-Matic v3 Engine leaf stages, ported from the native
// libauro.so (Artist Connection 1.21.31). This file is added incrementally and
// is NOT wired into Decoder:decode_next yet. Only the stages that have no
// dependency on still-unported siblings (FrontManager, p3D3D, Engine) live here.

// Native references:
// auro_audio_Smooth_float_*
// auro_matic_v3_DetectSilence_*
// auro_matic_v3_OutputGate_*
// auro_matic_v3_Gain_*
// auro_matic_Silence_*
// Engine member offsets confirmed by Engine_configure disassembly.

namespace {

// --- Engine member offsets (bytes), from auro_matic_v3_Engine_configure ---
constexpr std::size_t kMaticEngineFrontManagerOff = 0x868u; // 2152
constexpr std::size_t kMaticEngineXinNOff = 0xD10u;         // 3344
constexpr std::size_t kMaticEngine3d3dOff = 0x1060u;        // 4192
constexpr std::size_t kMaticEngineGainOff = 0x11F0u;        // 4592
constexpr std::size_t kMaticEngineSilenceOff = 0x1270u;     // 4720
constexpr std::size_t kMaticEngineMaskOff = 0x12F0u;        // 4848

// --- OutputGate (base = Engine + 0) ---
constexpr std::size_t kOutputGateStateOff = 0u;             // u32 state enum
constexpr std::size_t kOutputGateStartedOff = 4u;           // u32 configured flag
constexpr std::size_t kOutputGateDetectSilenceOff = 8u;     // nested DetectSilence
constexpr std::size_t kOutputGateCoeffSlowOff = 0x858u;     // 2136 f32
constexpr std::size_t kOutputGateCoeffFastOff = 0x85Cu;     // 2140 f32
constexpr std::size_t kOutputGateCurrentOff = 0x860u;       // 2144 f32 smooth current
constexpr std::size_t kOutputGateTargetOff = 0x864u;        // 2148 f32 gain target

// --- DetectSilence (base = Engine + 8) ---
constexpr std::size_t kDetectSilenceThreshold0Off = 0u;     // f32
constexpr std::size_t kDetectSilenceThreshold1Off = 4u;     // f32
constexpr std::size_t kDetectSilenceActiveOff = 8u;         // u32 signal-present
constexpr std::size_t kDetectSilenceEntriesOff = 12u;       // 31 entries, stride 68
constexpr std::size_t kDetectSilenceEntryStride = 68u;      // u32 channel + 16 f32 ring
constexpr std::size_t kDetectSilenceEntryCount = 31u;
constexpr std::size_t kDetectSilenceRingSlots = 16u;
constexpr std::size_t kDetectSilenceCountOff = 2120u;       // u32 active entry count
constexpr std::size_t kDetectSilenceRingIndexOff = 2124u;   // u32 ring write index

// --- Gain upmix Silence share the same "channel index list" layout ---
constexpr std::size_t kChannelListCountOff = 124u;          // u32 count
constexpr std::size_t kChannelListMax = 31u;

} // namespace

float auro_audio_Smooth_float_calculate_smoothing(std::uint32_t rate, float seconds) {
    const float samples = static_cast<float>(rate) * seconds;
    if (samples > 0.0f)
        return std::pow(10.0f, -3.0f / samples);
    return 0.0f;
}

void auro_audio_Smooth_float_inst_initialize(float* state, std::uint32_t rate, float seconds) {
    if (!state)
        return;
    const float samples = static_cast<float>(rate) * seconds;
    float coeff = 0.0f;
    if (samples > 0.0f)
        coeff = std::pow(10.0f, -3.0f / samples);
    state[2] = coeff;
}

void auro_audio_Smooth_float_inst_update(float* state, float target) {
    if (state)
        state[1] = target;
}

void auro_audio_Smooth_float_inst_set_current(float* state, float current) {
    if (state)
        state[0] = current;
}

// Native: explicit target/coeff ramp, persistent current in state[0].
void auro_audio_Smooth_float_gains_smooth(
    float* state,
    float* gains,
    std::uint32_t count,
    float target,
    float coeff) {
    if (!state || !gains || count == 0u)
        return;
    const float step = (1.0f - coeff) * target;
    float current = *state;
    for (std::uint32_t i = 0; i < count; ++i) {
        current = current * coeff + step;
        gains[i] = current;
    }
    *state = current;
}

// Native: target/coeff stored in state[1]/state[2], current state[0].
void auro_audio_Smooth_float_inst_gains_smooth(
    float* state,
    float* gains,
    std::uint32_t count) {
    if (!state || !gains || count == 0u)
        return;
    const float coeff = state[2];
    const float step = (1.0f - coeff) * state[1];
    float current = state[0];
    for (std::uint32_t i = 0; i < count; ++i) {
        current = current * coeff + step;
        gains[i] = current;
    }
    state[0] = current;
}

namespace {

void detect_silence_zero_rings(std::uint8_t* base) {
    for (std::size_t c = 0; c < kDetectSilenceEntryCount; ++c) {
        std::memset(
            base + kDetectSilenceEntriesOff + c * kDetectSilenceEntryStride + 4u,
            0,
            kDetectSilenceRingSlots * sizeof(float));
    }
}

bool detect_silence_rate_supported(std::uint32_t rate) {
    switch (rate) {
        case 16000u:
        case 22050u:
        case 24000u:
        case 32000u:
        case 44100u:
        case 48000u:
        case 88200u:
        case 96000u:
        case 176400u:
        case 192000u:
            return true;
        default:
            return false;
    }
}

// Native Configure enumerates mask bits 0..30 in ascending order into the
// per-entry index table; the count lives at +2120.
void channel_list_configure(std::uint8_t* base, std::uint32_t mask) {
    std::uint32_t count = 0u;
    *reinterpret_cast<std::uint32_t*>(base + kChannelListCountOff) = 0u;
    for (std::uint32_t bit = 0u; bit < 31u; ++bit) {
        if ((mask & (1u << bit)) == 0u)
            continue;
        *reinterpret_cast<std::uint32_t*>(base + 4u * count) = bit;
        ++count;
        *reinterpret_cast<std::uint32_t*>(base + kChannelListCountOff) = count;
    }
}

} // namespace

std::int64_t auro_matic_v3_DetectSilence_t_construct(std::uint8_t* base) {
    if (!base)
        return 0;
    detect_silence_zero_rings(base);
    *reinterpret_cast<std::uint64_t*>(base + kDetectSilenceThreshold0Off) =
        0x3A0637BD38077F39ull;
    *reinterpret_cast<std::uint32_t*>(base + kDetectSilenceCountOff) = 0u;
    *reinterpret_cast<std::uint32_t*>(base + kDetectSilenceRingIndexOff) = 0u;
    *reinterpret_cast<std::uint32_t*>(base + kDetectSilenceActiveOff) = 0u;
    return reinterpret_cast<std::int64_t>(base);
}

std::int64_t auro_matic_v3_DetectSilence_configure(
    std::uint8_t* base,
    std::uint32_t rate,
    std::uint32_t watch_mask) {
    if (!base || !detect_silence_rate_supported(rate))
        return 1;
    detect_silence_zero_rings(base);
    *reinterpret_cast<std::uint32_t*>(base + kDetectSilenceActiveOff) = 0u;
    channel_list_configure(base, watch_mask);
    *reinterpret_cast<std::uint32_t*>(base + kDetectSilenceRingIndexOff) = 0u;
    *reinterpret_cast<std::uint32_t*>(base + kDetectSilenceCountOff) = 0u;
    // Rebuild entry count in native order (channel_list_configure wrote it).
    {
        std::uint32_t count = 0u;
        for (std::uint32_t bit = 0u; bit < 31u; ++bit)
            if ((watch_mask & (1u << bit)) != 0u)
                ++count;
        *reinterpret_cast<std::uint32_t*>(base + kDetectSilenceCountOff) = count;
    }
    return 0;
}

std::int64_t auro_matic_v3_DetectSilence_set_thresholds(
    std::uint8_t* base,
    float low_db,
    float high_db) {
    if (!base || high_db < low_db)
        return 1;
    const float low_arg = low_db + low_db;
    float low = 0.0f;
    if (low_arg > -3000.0f)
        low = std::pow(10.0f, low_arg * 0.0500000007f) * 512.0f;
    const float high_arg = high_db + high_db;
    float high = 0.0f;
    if (high_arg > -3000.0f)
        high = std::pow(10.0f, high_arg * 0.0500000007f) * 512.0f;
    *reinterpret_cast<float*>(base + kDetectSilenceThreshold0Off) = low;
    *reinterpret_cast<float*>(base + kDetectSilenceThreshold1Off) = high;
    detect_silence_zero_rings(base);
    *reinterpret_cast<std::uint32_t*>(base + kDetectSilenceRingIndexOff) = 0u;
    *reinterpret_cast<std::uint32_t*>(base + kDetectSilenceActiveOff) = 0u;
    return 0;
}

std::int64_t auro_matic_v3_DetectSilence_reset_audio_state(std::uint8_t* base) {
    if (!base)
        return 0;
    detect_silence_zero_rings(base);
    *reinterpret_cast<std::uint32_t*>(base + kDetectSilenceRingIndexOff) = 0u;
    *reinterpret_cast<std::uint32_t*>(base + kDetectSilenceActiveOff) = 0u;
    return 0;
}

// Native. The native code vectorizes the 16-slot ring reduction for
// the first (count & ~7) channels with a right-associated fold, and uses a
// scalar left-associated fold for the remainder. Both orders are reproduced.
std::int64_t auro_matic_v3_DetectSilence_process(std::uint8_t* base, void** table) {
    if (!base)
        return 0;
    const std::uint32_t count =
        *reinterpret_cast<const std::uint32_t*>(base + kDetectSilenceCountOff);
    const std::uint32_t ring =
        *reinterpret_cast<const std::uint32_t*>(base + kDetectSilenceRingIndexOff);
    *reinterpret_cast<std::uint32_t*>(base + kDetectSilenceRingIndexOff) =
        (ring + 1u) & 0xFu;
    if (count == 0u) {
        *reinterpret_cast<std::uint32_t*>(base + kDetectSilenceActiveOff) = 0u;
        return 0;
    }

    for (std::uint32_t c = 0u; c < count; ++c) {
        auto* entry = base + kDetectSilenceEntriesOff + c * kDetectSilenceEntryStride;
        const std::uint32_t channel =
            *reinterpret_cast<const std::uint32_t*>(entry);
        const auto* plane = static_cast<const float*>(table[channel]);
        if (!plane)
            continue;
        float energy = 0.0f;
        for (std::size_t f = 0u; f < 32u; ++f)
            energy = energy + plane[f] * plane[f];
        reinterpret_cast<float*>(entry + 4u)[ring] = energy;
    }

    const bool primed =
        *reinterpret_cast<const std::uint32_t*>(base + kDetectSilenceActiveOff) != 0u;
    const float threshold = primed
        ? *reinterpret_cast<const float*>(base + kDetectSilenceThreshold1Off)
        : *reinterpret_cast<const float*>(base + kDetectSilenceThreshold0Off);

    const std::uint32_t vectorized = count & ~7u;
    bool all_silent = true;
    for (std::uint32_t c = 0u; c < count; ++c) {
        auto* entry = base + kDetectSilenceEntriesOff + c * kDetectSilenceEntryStride;
        const auto* ring_values = reinterpret_cast<const float*>(entry + 4u);
        float sum = 0.0f;
        if (c < vectorized) {
            for (std::size_t j = 0u; j < kDetectSilenceRingSlots; ++j)
                sum = ring_values[j] + sum;
        } else {
            for (std::size_t j = 0u; j < kDetectSilenceRingSlots; ++j)
                sum = sum + ring_values[j];
        }
        if (!(threshold > sum))
            all_silent = false;
    }

    const std::uint32_t active = all_silent ? 0u : 1u;
    *reinterpret_cast<std::uint32_t*>(base + kDetectSilenceActiveOff) = active;
    return active;
}

std::int64_t auro_matic_v3_OutputGate_t_construct(std::uint8_t* base) {
    if (!base)
        return 0;
    if (auro_matic_v3_DetectSilence_t_construct(base + kOutputGateDetectSilenceOff) == 0)
        return 0;
    *reinterpret_cast<float*>(base + kOutputGateTargetOff) = 1.0f;
    return reinterpret_cast<std::int64_t>(base);
}

std::int64_t auro_matic_v3_OutputGate_configure(
    std::uint8_t* base,
    std::uint32_t rate,
    std::uint32_t watch_mask) {
    if (!base)
        return 1;
    const std::int64_t rc = auro_matic_v3_DetectSilence_configure(
        base + kOutputGateDetectSilenceOff, rate, watch_mask);
    if (rc != 0)
        return rc;
    auro_matic_v3_DetectSilence_reset_audio_state(base + kOutputGateDetectSilenceOff);
    *reinterpret_cast<float*>(base + kOutputGateCoeffSlowOff) =
        auro_audio_Smooth_float_calculate_smoothing(rate, 0.01f);
    *reinterpret_cast<float*>(base + kOutputGateCoeffFastOff) =
        auro_audio_Smooth_float_calculate_smoothing(rate, 0.1f);
    *reinterpret_cast<std::uint32_t*>(base + kOutputGateStartedOff) = 1u;
    *reinterpret_cast<std::uint32_t*>(base + kOutputGateCurrentOff) = 0u;
    return 0;
}

void auro_matic_v3_OutputGate_set_gain(std::uint8_t* base, float gain) {
    if (base)
        *reinterpret_cast<float*>(base + kOutputGateTargetOff) = gain;
}

std::int64_t auro_matic_v3_OutputGate_reset_audio_state(std::uint8_t* base) {
    if (!base)
        return 0;
    *reinterpret_cast<std::uint32_t*>(base + kOutputGateCurrentOff) = 0u;
    *reinterpret_cast<std::uint32_t*>(base + kOutputGateStateOff) = 0u;
    return auro_matic_v3_DetectSilence_reset_audio_state(
        base + kOutputGateDetectSilenceOff);
}

// Native. Writes the 32-sample gain ramp into `gains` (scratch),
// `out_active` = 1 while the gate is fully closed, `out_reset` = 1 on the
// active->closed transition that triggers the Engine-wide state reset.
bool auro_matic_v3_OutputGate_process(
    std::uint8_t* base,
    void** table,
    std::uint32_t* out_active,
    std::uint32_t* out_reset,
    float* gains) {
    auro_matic_v3_DetectSilence_process(base + kOutputGateDetectSilenceOff, table);
    *out_active = 0u;
    *out_reset = 0u;

    const bool detect_active =
        *reinterpret_cast<const std::uint32_t*>(base + kOutputGateDetectSilenceOff
            + kDetectSilenceActiveOff) != 0u;
    const bool started =
        *reinterpret_cast<const std::uint32_t*>(base + kOutputGateStartedOff) != 0u;
    const bool running = detect_active ? started : true;

    std::uint32_t state = *reinterpret_cast<const std::uint32_t*>(base + kOutputGateStateOff);
    const float current = *reinterpret_cast<const float*>(base + kOutputGateCurrentOff);
    const float coeff_slow = *reinterpret_cast<const float*>(base + kOutputGateCoeffSlowOff);
    const float coeff_fast = *reinterpret_cast<const float*>(base + kOutputGateCoeffFastOff);
    const float target_gain = *reinterpret_cast<const float*>(base + kOutputGateTargetOff);

    bool to_label14 = false;
    bool do_smooth = true;
    bool do_reset = false;
    float v15 = 0.0f;
    float v16 = 0.0f;

    switch (state) {
        case 0u:
            // Native OutputGate state 0 opens into state 1; while running the
            // current gain latches to the target immediately (Frida probe of
            // auro_matic_v3_OutputGate_process, 2026-09-15).
            if (running)
                *reinterpret_cast<float*>(base + kOutputGateCurrentOff) = target_gain;
            state = 1u;
            *reinterpret_cast<std::uint32_t*>(base + kOutputGateStateOff) = state;
            v16 = coeff_fast;
            v15 = target_gain;
            break;
        case 1u:
            if (running) {
                state = 2u;
                *reinterpret_cast<std::uint32_t*>(base + kOutputGateStateOff) = state;
                v16 = coeff_slow;
                v15 = 0.0f;
            } else {
                state = 1u;
                *reinterpret_cast<std::uint32_t*>(base + kOutputGateStateOff) = state;
                v16 = coeff_fast;
                v15 = target_gain;
            }
            break;
        case 2u:
            if (running) {
                state = (current > 0.0000099999997f) ? 2u : 0u;
                if (state != 0u) {
                    to_label14 = true;
                } else {
                    do_reset = true;
                    *reinterpret_cast<std::uint32_t*>(base + kOutputGateStateOff) = 0u;
                    v15 = 0.0f;
                    v16 = 0.0f;
                }
            } else {
                state = 3u;
                to_label14 = true;
            }
            break;
        case 3u:
            if (running) {
                state = 2u;
                *reinterpret_cast<std::uint32_t*>(base + kOutputGateStateOff) = state;
                v16 = coeff_slow;
                v15 = 0.0f;
            } else {
                const float delta = std::fabs(current - target_gain);
                const bool differs = delta >= 0.0000099999997f;
                state = (differs ? 2u : 0u) + 1u;
                to_label14 = true;
            }
            break;
        default:
            to_label14 = true;
            break;
    }

    if (to_label14) {
        *reinterpret_cast<std::uint32_t*>(base + kOutputGateStateOff) = state;
        switch (state) {
            case 0u:
                v15 = 0.0f;
                v16 = 0.0f;
                break;
            case 1u:
                v16 = coeff_fast;
                v15 = target_gain;
                break;
            case 2u:
                v16 = coeff_slow;
                v15 = 0.0f;
                break;
            case 3u:
                v16 = coeff_slow;
                v15 = target_gain;
                break;
            default:
                do_smooth = false;
                break;
        }
    }

    if (do_smooth && gains) {
        auro_audio_Smooth_float_gains_smooth(
            reinterpret_cast<float*>(base + kOutputGateCurrentOff),
            gains,
            32u,
            v15,
            v16);
    }
    if (do_reset)
        *out_reset = 1u;

    const bool closed =
        *reinterpret_cast<const std::uint32_t*>(base + kOutputGateStateOff) == 0u;
    *out_active = closed ? 1u : 0u;
    return closed;
}

std::int64_t auro_matic_v3_Gain_t_construct(std::uint8_t* base) {
    if (!base)
        return 0;
    *reinterpret_cast<std::uint32_t*>(base + kChannelListCountOff) = 0u;
    return reinterpret_cast<std::int64_t>(base);
}

std::int64_t auro_matic_v3_Gain_configure(std::uint8_t* base, std::uint32_t mask) {
    if (!base)
        return 1;
    channel_list_configure(base, mask);
    return 0;
}

void auro_matic_v3_Gain_reset_audio_state() {}

std::int64_t auro_matic_v3_Gain_process(
    std::uint8_t* base,
    void** table,
    const float* gains) {
    if (!base || !table || !gains)
        return 0;
    const std::uint32_t count =
        *reinterpret_cast<const std::uint32_t*>(base + kChannelListCountOff);
    for (std::uint32_t k = 0u; k < count; ++k) {
        const std::uint32_t channel =
            *reinterpret_cast<const std::uint32_t*>(base + 4u * k);
        if (channel >= kChannelListMax + 1u)
            continue;
        auto* plane = static_cast<float*>(table[channel]);
        if (!plane)
            continue;
        for (std::size_t f = 0u; f < 32u; ++f)
            plane[f] = gains[f] * plane[f];
    }
    return static_cast<std::int64_t>(count);
}

std::int64_t auro_matic_Silence_t_construct(std::uint8_t* base) {
    if (!base)
        return 0;
    *reinterpret_cast<std::uint32_t*>(base + kChannelListCountOff) = 0u;
    return reinterpret_cast<std::int64_t>(base);
}

std::int64_t auro_matic_Silence_configure(std::uint8_t* base, std::uint32_t mask) {
    if (!base)
        return 1;
    channel_list_configure(base, mask);
    return 0;
}

void auro_matic_Silence_reset_audio_state() {}

void auro_matic_Silence_process(std::uint8_t* base, void** table) {
    if (!base || !table)
        return;
    const std::uint32_t count =
        *reinterpret_cast<const std::uint32_t*>(base + kChannelListCountOff);
    for (std::uint32_t k = 0u; k < count; ++k) {
        const std::uint32_t channel =
            *reinterpret_cast<const std::uint32_t*>(base + 4u * k);
        if (channel >= kChannelListMax + 1u)
            continue;
        auto* plane = static_cast<float*>(table[channel]);
        if (plane)
            std::memset(plane, 0, 32u * sizeof(float));
    }
}

// --- FrontManager (Input at +0, Output at +1176) ---
// Native: auro_matic_v3_FrontManager_*.
// Mode 0 is the decoded-bed -> taller-bed path used by the captured 5.1->5.1.4
// oracle: Input latches the original table planes, Output crossfades the
// processed planes back toward them with the OutputGate gain ramp. Modes 1-3
// (Centergen-driven stereo/surround input) are not ported yet and are rejected
// loudly instead of producing wrong audio.
namespace {

constexpr std::size_t kFrontManagerOutputOff = 1176u;
constexpr std::size_t kFrontInputCentergenModeOff = 84u;
constexpr std::size_t kFrontInputClear0Off = 1048u;
constexpr std::size_t kFrontInputClear1Off = 1056u;
constexpr std::size_t kFrontInputFixedOff = 1072u;   // 16 bytes
constexpr std::size_t kFrontInputDynamicOff = 1088u; // 16 bytes
constexpr std::size_t kFrontInputExtra0Off = 1104u;  // 16 bytes
constexpr std::size_t kFrontInputExtra1Off = 1120u;  // 16 bytes
constexpr std::size_t kFrontInputExtra2Off = 1132u;  // 16 bytes
constexpr std::size_t kFrontInputCallbackOff = 1152u;
constexpr std::size_t kFrontInputCallback2Off = 1160u;
constexpr std::size_t kFrontInputFlagOff = 1168u;    // {latchOut, runCallback}
constexpr std::size_t kFrontInputFlag2Off = 1172u;

constexpr std::size_t kFrontOutputGain0Off = 0u;     // f32
constexpr std::size_t kFrontOutputGain1Off = 4u;     // f32
constexpr std::size_t kFrontOutputModeOff = 8u;      // u32

void copy_plane32(float* dst, const float* src) {
    if (dst && src)
        std::memcpy(dst, src, 32u * sizeof(float));
}

void zero_plane32(float* dst) {
    if (dst)
        std::memset(dst, 0, 32u * sizeof(float));
}

} // namespace

std::int64_t auro_matic_v3_FrontManager_Input_t_construct(std::uint8_t* base) {
    if (!base)
        return 0;
    *reinterpret_cast<std::uint64_t*>(base + kFrontInputCallbackOff) = 0u;
    std::int32_t rate_and_mode[2] = {48000, 65536};
    if (auro_centergen_v3_Processor_t_construct(base, rate_and_mode) == 0u)
        return 0;
    return reinterpret_cast<std::int64_t>(base);
}

std::int64_t auro_matic_v3_FrontManager_Input_configure(
    std::uint8_t* base,
    std::uint32_t rate,
    const std::uint8_t* plan) {
    if (!base || !plan)
        return 1;
    *reinterpret_cast<std::uint64_t*>(base + kFrontInputCallbackOff) = 0u;
    std::memcpy(base + kFrontInputExtra2Off, plan + 64u, 16u);
    std::memcpy(base + kFrontInputExtra1Off, plan + 52u, 16u);
    std::memcpy(base + kFrontInputExtra0Off, plan + 36u, 16u);
    std::memcpy(base + kFrontInputDynamicOff, plan + 20u, 16u);
    std::memcpy(base + kFrontInputFixedOff, plan + 4u, 16u);

    const std::uint32_t mode = *reinterpret_cast<const std::uint32_t*>(plan);
    if (mode == 0u) {
        *reinterpret_cast<std::uint32_t*>(base + kFrontInputFlagOff) = 1u;
        *reinterpret_cast<std::uint32_t*>(base + kFrontInputFlag2Off) = 1u;
        return 0;
    }
    // Modes 1/2/3 require the Centergen front callbacks.
    return 1;
}

std::int64_t auro_matic_v3_FrontManager_Input_reset_audio_state(std::uint8_t* base) {
    if (!base)
        return 0;
    auro_centergen_v3_Processor_reset_audio_state(base);
    const std::uint32_t centergen_mode =
        *reinterpret_cast<const std::uint32_t*>(base + kFrontInputCentergenModeOff);
    if (centergen_mode == 2u) {
        *reinterpret_cast<std::uint64_t*>(base + kFrontInputClear0Off) = 0u;
        *reinterpret_cast<std::uint64_t*>(base + kFrontInputClear0Off + 8u) = 0u;
    } else if (centergen_mode <= 1u) {
        *reinterpret_cast<std::uint32_t*>(base + kFrontInputClear0Off) = 0u;
        *reinterpret_cast<std::uint32_t*>(base + kFrontInputClear1Off) = 0u;
    }
    return static_cast<std::int64_t>(centergen_mode);
}

void auro_matic_v3_FrontManager_Input_process(
    std::uint8_t* base,
    void** table,
    float* a3,
    float* a4,
    float* a5) {
    (void)base;
    if (!table)
        return;
    const std::uint32_t latch_out =
        *reinterpret_cast<const std::uint32_t*>(base + kFrontInputFlag2Off);
    const std::uint32_t run_callback =
        *reinterpret_cast<const std::uint32_t*>(base + kFrontInputFlagOff);
    const std::uint32_t callback =
        *reinterpret_cast<const std::uint32_t*>(base + kFrontInputCallbackOff);

    if (!latch_out) {
        zero_plane32(a5);
        if (!run_callback) {
            zero_plane32(a3);
            zero_plane32(a4);
            return;
        }
    } else {
        copy_plane32(a5, static_cast<const float*>(table[2]));
        if (!run_callback)
            return;
    }
    copy_plane32(a3, static_cast<const float*>(table[0]));
    copy_plane32(a4, static_cast<const float*>(table[1]));
    // Mode 0 has no callback; modes 1-3 are rejected at configure.
    (void)callback;
}

std::int64_t auro_matic_v3_FrontManager_Output_t_construct(std::uint8_t* base) {
    if (!base)
        return 0;
    *reinterpret_cast<std::uint32_t*>(base + kFrontOutputGain0Off) = 0x3F800000u;
    *reinterpret_cast<std::uint32_t*>(base + kFrontOutputGain1Off) = 0x3F800000u;
    *reinterpret_cast<std::uint32_t*>(base + kFrontOutputModeOff) = 0u;
    return reinterpret_cast<std::int64_t>(base);
}

std::int64_t auro_matic_v3_FrontManager_Output_configure(
    std::uint8_t* base,
    std::uint32_t /*rate*/,
    const std::uint8_t* plan) {
    if (!base || !plan)
        return 1;
    *reinterpret_cast<std::uint32_t*>(base + kFrontOutputModeOff) =
        *reinterpret_cast<const std::uint32_t*>(plan + 8u);
    std::memcpy(base + kFrontOutputGain0Off, plan, 8u);
    return 0;
}

void auro_matic_v3_FrontManager_Output_reset_audio_state(std::uint8_t* /*base*/) {}

void auro_matic_v3_FrontManager_Output_process(
    std::uint8_t* base,
    void** table,
    const float* a3,
    const float* a4,
    const float* a5,
    const float* ramp) {
    if (!base || !table)
        return;
    auto* v9 = static_cast<float*>(table[0]);
    auto* v10 = static_cast<float*>(table[1]);
    auto* v8 = static_cast<float*>(table[2]);
    const std::uint32_t mode =
        *reinterpret_cast<const std::uint32_t*>(base + kFrontOutputModeOff);

    if (mode == 1u || mode == 3u || mode == 4u) {
        copy_plane32(v8, a5);
        copy_plane32(v9, a3);
        copy_plane32(v10, a4);
    } else if (mode == 2u) {
        copy_plane32(v9, a3);
        copy_plane32(v10, a4);
        if (v8)
            for (std::size_t i = 0u; i < 32u; ++i)
                v8[i] = (a3[i] + a4[i]) * 0.7071067811865476f;
    }
    // mode 0: keep processed planes; blending below.

    // Gain coefficients equal to 1.0 skip the optional front scaling stage; only
    // that configuration is currently exercised (captured 5.1->5.1.4 plan).
    if (!ramp)
        return;
    if (v8)
        for (std::size_t i = 0u; i < 32u; ++i) {
            const float g = std::fmin(ramp[i], 1.0f);
            v8[i] = g * v8[i] + (1.0f - g) * a5[i];
        }
    if (v9)
        for (std::size_t i = 0u; i < 32u; ++i) {
            const float g = std::fmin(ramp[i], 1.0f);
            v9[i] = g * v9[i] + (1.0f - g) * a3[i];
        }
    if (v10)
        for (std::size_t i = 0u; i < 32u; ++i) {
            const float g = std::fmin(ramp[i], 1.0f);
            v10[i] = g * v10[i] + (1.0f - g) * a4[i];
        }
}

std::int64_t auro_matic_v3_FrontManager_t_construct(std::uint8_t* base) {
    if (!base)
        return 0;
    if (auro_matic_v3_FrontManager_Input_t_construct(base) == 0)
        return 0;
    if (auro_matic_v3_FrontManager_Output_t_construct(
            base + kFrontManagerOutputOff) == 0)
        return 0;
    return reinterpret_cast<std::int64_t>(base);
}

std::int64_t auro_matic_v3_FrontManager_configure(
    std::uint8_t* base,
    std::uint32_t rate,
    const std::uint8_t* plan) {
    if (!base || !plan)
        return 1;
    const std::int64_t rc = auro_matic_v3_FrontManager_Input_configure(base, rate, plan);
    if (rc != 0)
        return rc;
    return auro_matic_v3_FrontManager_Output_configure(
        base + kFrontManagerOutputOff, rate, plan + 88u);
}

std::int64_t auro_matic_v3_FrontManager_reset_audio_state(std::uint8_t* base) {
    if (!base)
        return 0;
    auro_matic_v3_FrontManager_Input_reset_audio_state(base);
    auro_matic_v3_FrontManager_Output_reset_audio_state(base + kFrontManagerOutputOff);
    return 0;
}

void auro_matic_v3_FrontManager_input_process(
    std::uint8_t* base,
    void** table,
    float* a3,
    float* a4,
    float* a5) {
    auro_matic_v3_FrontManager_Input_process(base, table, a3, a4, a5);
}

void auro_matic_v3_FrontManager_output_process(
    std::uint8_t* base,
    void** table,
    const float* a3,
    const float* a4,
    const float* a5,
    const float* ramp) {
    auro_matic_v3_FrontManager_Output_process(
        base + kFrontManagerOutputOff, table, a3, a4, a5, ramp);
}

// --- p3D3D delay/phase-line subsystem ---
// Native: auro_matic_3d3d_*, biquad.
// The line structures and ring write/read/proceed wrap semantics are ported
// instruction-for-instruction from the disassembly.
namespace {

constexpr std::size_t kDelayLineBaseOff = 0u;   // float*
constexpr std::size_t kDelayLineWriteOff = 8u;  // float*
constexpr std::size_t kDelayLineSizeOff = 16u;  // u32
constexpr std::size_t kDelayLineAvailOff = 20u; // u32

constexpr float kMatic3d3dOnes[32] = {
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

bool delay_line_construct(std::uint8_t* line, float* buffer, std::uint32_t size) {
    if (!line)
        return false;
    if (size < 0x20u || (size & 1u) != 0u)
        return false;
    *reinterpret_cast<float**>(line + kDelayLineBaseOff) = buffer;
    *reinterpret_cast<float**>(line + kDelayLineWriteOff) = buffer;
    *reinterpret_cast<std::uint32_t*>(line + kDelayLineSizeOff) = size;
    *reinterpret_cast<std::uint32_t*>(line + kDelayLineAvailOff) = size + 32u;
    return true;
}

void delay_line_set_dirty(std::uint8_t* line) {
    *reinterpret_cast<std::uint32_t*>(line + kDelayLineAvailOff) =
        *reinterpret_cast<const std::uint32_t*>(line + kDelayLineSizeOff) + 32u;
}

void delay_line_clear(std::uint8_t* line) {
    auto* base = *reinterpret_cast<float**>(line + kDelayLineBaseOff);
    *reinterpret_cast<float**>(line + kDelayLineWriteOff) = base;
    const std::uint32_t n =
        (*reinterpret_cast<const std::uint32_t*>(line + kDelayLineSizeOff)
            & 0xFFFFFFFEu) + 32u;
    if (n != 0u)
        std::memset(base, 0, 4u * n);
}

void delay_line_proceed(std::uint8_t* line) {
    auto* base = *reinterpret_cast<float**>(line + kDelayLineBaseOff);
    auto* write = *reinterpret_cast<float**>(line + kDelayLineWriteOff);
    const std::uint32_t size =
        *reinterpret_cast<const std::uint32_t*>(line + kDelayLineSizeOff);
    if (write + 32 >= base + size)
        write = write - size + 32;
    else
        write += 32;
    *reinterpret_cast<float**>(line + kDelayLineWriteOff) = write;
}

void delay_line_read(
    std::uint8_t* line,
    std::uint32_t delay,
    float* out32) {
    const auto* base = *reinterpret_cast<const float**>(line + kDelayLineBaseOff);
    const auto* write = *reinterpret_cast<const float**>(line + kDelayLineWriteOff);
    const std::uint32_t size =
        *reinterpret_cast<const std::uint32_t*>(line + kDelayLineSizeOff);
    const float* src = write - delay;
    if (src < base)
        src += size;
    std::memcpy(out32, src, 32u * sizeof(float));
}

// Native DelayLine_write: write the block at the cursor, wrap the straddle into
// the buffer base, then maintain the forward mirror at cursor+size (kept inside
// the 32-float slack of the native allocation).
void delay_line_write(std::uint8_t* line, const float* in32) {
    auto* base = *reinterpret_cast<float**>(line + kDelayLineBaseOff);
    auto* write = *reinterpret_cast<float**>(line + kDelayLineWriteOff);
    const std::uint32_t size =
        *reinterpret_cast<const std::uint32_t*>(line + kDelayLineSizeOff);
    std::memcpy(write, in32, 32u * sizeof(float));

    const std::int64_t offset =
        static_cast<std::int64_t>(write - base);
    const std::int64_t room = static_cast<std::int64_t>(size) - offset;
    std::uint32_t wrap = static_cast<std::uint32_t>(room > 32 ? 32 : (room < 0 ? 0 : room));
    std::uint32_t wrap_count = (32u - wrap) & 0xFFFFFFFEu;
    if (wrap_count != 0u)
        std::memcpy(base, in32 + wrap, 4u * wrap_count);

    if (offset <= 32) {
        const std::uint32_t mirror = (32u - static_cast<std::uint32_t>(offset)) & 0xFFFFFFFEu;
        if (mirror != 0u)
            std::memcpy(write + size, in32, 4u * mirror);
    }
}

void biquad_process_float_1(float* in, float* out, float* state, const float* coeff) {
    float s0 = state[0];
    float s1 = state[1];
    for (std::size_t i = 0u; i < 32u; ++i) {
        const float x = in[i];
        const float y = coeff[0] * x + s0;
        s0 = coeff[1] * x + s1 - coeff[3] * y;
        s1 = coeff[2] * x - coeff[4] * y;
        out[i] = y;
    }
    state[0] = s0;
    state[1] = s1;
}

void biquad_process_float_2(
    float* in1,
    float* out1,
    float* state1,
    float* in2,
    float* out2,
    float* state2,
    const float* coeff) {
    float a0 = state1[0];
    float a1 = state1[1];
    float b0 = state2[0];
    float b1 = state2[1];
    for (std::size_t i = 0u; i < 32u; ++i) {
        const float x1 = in1[i];
        const float x2 = in2[i];
        const float y1 = coeff[0] * x1 + a0;
        const float y2 = coeff[0] * x2 + b0;
        a0 = coeff[1] * x1 + a1 - coeff[3] * y1;
        b0 = coeff[1] * x2 + b1 - coeff[3] * y2;
        const float c2 = coeff[2];
        const float c4 = coeff[4];
        out1[i] = y1;
        a1 = x1 * c2 - y1 * c4;
        b1 = c2 * x2 - c4 * y2;
        out2[i] = y2;
    }
    state1[0] = a0;
    state2[0] = b0;
    state1[1] = a1;
    state2[1] = b1;
}

} // namespace

void auro_matic_3d3d_Line2_t_construct(std::uint8_t* line, float* buffer) {
    if (!line)
        return;
    delay_line_construct(line + 16u, buffer, 292u);
    delay_line_construct(line + 40u, buffer + 324u, 234u);
    delay_line_set_dirty(line + 16u);
    delay_line_set_dirty(line + 40u);
}

void auro_matic_3d3d_Line3_t_construct(std::uint8_t* line, float* buffer) {
    if (!line)
        return;
    delay_line_construct(line + 16u, buffer, 694u);
    delay_line_construct(line + 40u, buffer + 726u, 738u);
    delay_line_set_dirty(line + 16u);
    delay_line_set_dirty(line + 40u);
}

void auro_matic_3d3d_Line4_t_construct(std::uint8_t* line, float* buffer) {
    if (!line)
        return;
    *reinterpret_cast<std::uint64_t*>(line + 0u) = 0u;
    *reinterpret_cast<std::uint64_t*>(line + 40u) = 0u;
    delay_line_construct(line + 8u, buffer, 176u);
    delay_line_construct(line + 48u, buffer + 208u, 176u);
    delay_line_set_dirty(line + 8u);
    delay_line_set_dirty(line + 48u);
}

void auro_matic_3d3d_Line2_clear(std::uint8_t* line) {
    *reinterpret_cast<std::uint64_t*>(line) = 0u;
    delay_line_clear(line + 16u);
    delay_line_clear(line + 40u);
}

void auro_matic_3d3d_Line3_clear(std::uint8_t* line) {
    *reinterpret_cast<std::uint64_t*>(line) = 0u;
    delay_line_clear(line + 16u);
    delay_line_clear(line + 40u);
}

void auro_matic_3d3d_Line4_clear(std::uint8_t* line) {
    *reinterpret_cast<std::uint64_t*>(line) = 0u;
    delay_line_clear(line + 8u);
    *reinterpret_cast<std::uint64_t*>(line + 40u) = 0u;
    delay_line_clear(line + 48u);
}

void auro_matic_3d3d_Line2_process_ext(
    std::uint8_t* line,
    const float* a2,
    const float* a3,
    const float* a4,
    const float* a5,
    float* a6,
    float* a7,
    const float* coeff,
    const float* gains) {
    delay_line_set_dirty(line + 16u);
    delay_line_set_dirty(line + 40u);
    for (std::size_t i = 0u; i < 32u; ++i) {
        const float g = gains[i] * 0.5f;
        a6[i] = (a2[i] + a3[i]) * g;
        a7[i] = (a4[i] + a5[i]) * g;
    }
    biquad_process_float_2(
        a6,
        a6,
        reinterpret_cast<float*>(line),
        a7,
        a7,
        reinterpret_cast<float*>(line + 8u),
        coeff);
    delay_line_write(line + 16u, a6);
    delay_line_write(line + 40u, a7);
    delay_line_read(line + 40u, 202u, a7);
    delay_line_read(line + 16u, 260u, a6);
    const float c0 = coeff[7];
    const float c1 = coeff[8];
    for (std::size_t i = 0u; i < 32u; ++i)
        a7[i] = a7[i] * c1 + a6[i] * c0;
    delay_line_read(line + 16u, 144u, a6);
    const float c6 = coeff[6];
    for (std::size_t i = 0u; i < 32u; ++i)
        a6[i] *= c6;
    delay_line_proceed(line + 16u);
    delay_line_proceed(line + 40u);
}

void auro_matic_3d3d_Line3_process_ext(
    std::uint8_t* line,
    const float* a2,
    const float* a3,
    float* a4,
    float* a5,
    const float* coeff,
    const float* gains) {
    delay_line_set_dirty(line + 16u);
    delay_line_set_dirty(line + 40u);
    for (std::size_t i = 0u; i < 32u; ++i) {
        const float g = gains[i];
        a4[i] = a2[i] * g;
        a5[i] = g * a3[i];
    }
    biquad_process_float_2(
        a4,
        a4,
        reinterpret_cast<float*>(line),
        a5,
        a5,
        reinterpret_cast<float*>(line + 8u),
        coeff);
    delay_line_write(line + 16u, a4);
    delay_line_write(line + 40u, a5);
    float tmp[32];
    delay_line_read(line + 16u, 576u, a4);
    delay_line_read(line + 16u, 662u, tmp);
    const float c9 = coeff[9];
    const float c10 = coeff[10];
    for (std::size_t i = 0u; i < 32u; ++i)
        a4[i] = tmp[i] * c10 - a4[i] * c9;
    delay_line_read(line + 40u, 576u, a5);
    delay_line_read(line + 40u, 706u, tmp);
    const float c11 = coeff[11];
    const float c12 = coeff[12];
    for (std::size_t i = 0u; i < 32u; ++i)
        a5[i] = tmp[i] * c12 - a5[i] * c11;
    delay_line_proceed(line + 16u);
    delay_line_proceed(line + 40u);
}

void auro_matic_3d3d_Line4_channel_process(
    std::uint8_t* channel,
    const float* in1,
    const float* in2,
    float* out,
    const float* coeff,
    const float* gains) {
    delay_line_set_dirty(channel + 8u);
    for (std::size_t i = 0u; i < 32u; ++i)
        out[i] = (in1[i] + in2[i]) * (gains[i] * 0.5f);
    biquad_process_float_1(out, out, reinterpret_cast<float*>(channel), coeff);
    delay_line_write(channel + 8u, out);
    delay_line_read(channel + 8u, 144u, out);
    const float c13 = coeff[13];
    for (std::size_t i = 0u; i < 32u; ++i)
        out[i] *= c13;
    delay_line_proceed(channel + 8u);
}

void auro_matic_3d3d_Line4_process_ext(
    std::uint8_t* line,
    const float* a2,
    const float* a3,
    const float* a4,
    const float* a5,
    float* a6,
    float* a7,
    const float* coeff,
    const float* gains) {
    auro_matic_3d3d_Line4_channel_process(line, a2, a4, a6, coeff, gains);
    auro_matic_3d3d_Line4_channel_process(line + 40u, a3, a5, a7, coeff, gains);
}

// --- p3D3D container ---
// Native auro_matic_v3_3d3d_*,.
// The captured 5.1->5.1.4 3d3d plan (plan words 36..42) is all zero, which
// registers zero line callbacks, so process only advances the smoothing state.
namespace {

constexpr std::size_t k3d3dLine2Off = 0u;
constexpr std::size_t k3d3dLine3aOff = 64u;
constexpr std::size_t k3d3dLine3bOff = 128u;
constexpr std::size_t k3d3dLine4Off = 192u;
constexpr std::size_t k3d3dCoeffPtrOff = 272u;
constexpr std::size_t k3d3dProcessFnsOff = 280u;
constexpr std::size_t k3d3dCallbackCountOff = 344u;
constexpr std::size_t k3d3dSmoothOff = 352u;
constexpr std::size_t k3d3dBacking0Off = 368u; // Line2
constexpr std::size_t k3d3dBacking1Off = 376u; // Line3a
constexpr std::size_t k3d3dBacking2Off = 384u; // Line3b
constexpr std::size_t k3d3dBacking3Off = 392u; // Line4

constexpr float kMatic3d3dDefaultCoeffs[14] = {
    0.9125f,
    -1.9046f,
    1.0f,
    -1.9046f,
    0.9125f,
    0.7f,
    0.7f,
    0.5f,
    0.8f,
    0.7f,
    0.1f,
    0.7f,
    0.1f,
    0.5f};

} // namespace

std::int64_t auro_matic_v3_3d3d_reset_audio_state(std::uint8_t* base);

std::int64_t auro_matic_v3_3d3d_t_construct(
    std::uint8_t* base,
    const std::uint8_t* memory) {
    if (!base || !memory)
        return 0;
    std::memcpy(base + k3d3dBacking0Off, memory, 16u);
    std::memcpy(base + k3d3dBacking2Off, memory + 16u, 16u);
    if (*reinterpret_cast<const std::uint64_t*>(base + k3d3dBacking0Off) != 0u)
        auro_matic_3d3d_Line2_t_construct(
            base + k3d3dLine2Off,
            reinterpret_cast<float*>(static_cast<std::uintptr_t>(
                *reinterpret_cast<const std::uint64_t*>(base + k3d3dBacking0Off))));
    if (*reinterpret_cast<const std::uint64_t*>(base + k3d3dBacking1Off) != 0u)
        auro_matic_3d3d_Line3_t_construct(
            base + k3d3dLine3aOff,
            reinterpret_cast<float*>(static_cast<std::uintptr_t>(
                *reinterpret_cast<const std::uint64_t*>(base + k3d3dBacking1Off))));
    if (*reinterpret_cast<const std::uint64_t*>(base + k3d3dBacking2Off) != 0u)
        auro_matic_3d3d_Line3_t_construct(
            base + k3d3dLine3bOff,
            reinterpret_cast<float*>(static_cast<std::uintptr_t>(
                *reinterpret_cast<const std::uint64_t*>(base + k3d3dBacking2Off))));
    if (*reinterpret_cast<const std::uint64_t*>(base + k3d3dBacking3Off) != 0u)
        auro_matic_3d3d_Line4_t_construct(
            base + k3d3dLine4Off,
            reinterpret_cast<float*>(static_cast<std::uintptr_t>(
                *reinterpret_cast<const std::uint64_t*>(base + k3d3dBacking3Off))));
    // Smooth_fl32_inst_construct zeroes 12 bytes at +352.
    *reinterpret_cast<std::uint64_t*>(base + k3d3dSmoothOff) = 0u;
    *reinterpret_cast<std::uint32_t*>(base + k3d3dSmoothOff + 8u) = 0u;
    *reinterpret_cast<std::uint32_t*>(base + k3d3dCallbackCountOff) = 0u;
    return reinterpret_cast<std::int64_t>(base);
}

std::int64_t auro_matic_v3_3d3d_configure(
    std::uint8_t* base,
    std::uint32_t rate,
    const std::uint8_t* plan) {
    if (!base || !plan)
        return 1;
    const void* coeff = kMatic3d3dDefaultCoeffs;
    if (*reinterpret_cast<const std::uint64_t*>(plan + 32u) != 0u)
        coeff = *reinterpret_cast<const void* const*>(plan + 32u);
    *reinterpret_cast<const void**>(base + k3d3dCoeffPtrOff) = coeff;
    *reinterpret_cast<std::uint32_t*>(base + k3d3dCallbackCountOff) = 0u;

    const std::uint32_t mode = *reinterpret_cast<const std::uint32_t*>(plan);
    bool plan_uses_lines = mode != 0u;
    for (std::size_t w = 1u; w <= 6u; ++w)
        plan_uses_lines |= *reinterpret_cast<const std::uint32_t*>(plan + 4u * w) != 0u;
    if (plan_uses_lines)
        return 1; // callback-selected line routing not ported yet

    if (rate != 16000u && rate != 22050u && rate != 24000u && rate != 32000u
        && rate != 44100u && rate != 48000u && rate != 88200u && rate != 96000u
        && rate != 176400u && rate != 192000u)
        return 1;
    auro_audio_Smooth_fl32_inst_initialize(
        reinterpret_cast<float*>(base + k3d3dSmoothOff), rate, 0.001f);
    auro_audio_Smooth_fl32_inst_update(
        reinterpret_cast<float*>(base + k3d3dSmoothOff), 1.0f);
    auro_matic_v3_3d3d_reset_audio_state(base);
    return 0;
}

std::int64_t auro_matic_v3_3d3d_reset_audio_state(std::uint8_t* base) {
    if (!base)
        return 0;
    if (*reinterpret_cast<const std::uint64_t*>(base + k3d3dBacking0Off) != 0u)
        auro_matic_3d3d_Line2_clear(base + k3d3dLine2Off);
    if (*reinterpret_cast<const std::uint64_t*>(base + k3d3dBacking1Off) != 0u)
        auro_matic_3d3d_Line3_clear(base + k3d3dLine3aOff);
    if (*reinterpret_cast<const std::uint64_t*>(base + k3d3dBacking2Off) != 0u)
        auro_matic_3d3d_Line3_clear(base + k3d3dLine3bOff);
    if (*reinterpret_cast<const std::uint64_t*>(base + k3d3dBacking3Off) != 0u)
        auro_matic_3d3d_Line4_clear(base + k3d3dLine4Off);
    auro_audio_Smooth_fl32_inst_set_current(
        reinterpret_cast<float*>(base + k3d3dSmoothOff), 0.0f);
    return 0;
}

void auro_matic_v3_3d3d_process(std::uint8_t* base, void** /*table*/) {
    if (!base)
        return;
    float gains[32];
    auro_audio_Smooth_fl32_inst_gains_smooth(
        reinterpret_cast<float*>(base + k3d3dSmoothOff), gains, 32u);
    // No line callbacks are selected by the supported plans; nothing else runs.
}

// --- Auro-Matic v3 Engine container (orchestrates the stages above) ---
// Native auro_matic_v3_Engine_*.

std::int64_t auro_matic_v3_Engine_reset_audio_state(std::uint8_t* base);

// Native is a thunk to OutputGate_set_bypass, which writes
// the started flag at engine+4.
void auro_matic_v3_Engine_set_bypass(std::uint8_t* base, std::int32_t bypass);

std::int64_t auro_matic_v3_Engine_t_construct(
    std::uint8_t* base,
    const std::uint64_t* memory_args) {
    if (!base)
        return 0;
    if (auro_matic_v3_OutputGate_t_construct(base) == 0)
        return 0;
    if (auro_matic_v3_FrontManager_t_construct(base + kMaticEngineFrontManagerOff) == 0)
        return 0;
    if (auro_matic_v3_XinN_fl32_construct(base + kMaticEngineXinNOff, memory_args) == 0)
        return 0;
    if (auro_matic_v3_3d3d_t_construct(
            base + kMaticEngine3d3dOff,
            reinterpret_cast<const std::uint8_t*>(memory_args + 5)) == 0)
        return 0;
    if (auro_matic_v3_Gain_t_construct(base + kMaticEngineGainOff) == 0)
        return 0;
    if (auro_matic_Silence_t_construct(base + kMaticEngineSilenceOff) == 0)
        return 0;
    auro_matic_v3_OutputGate_set_gain(base, 1.0f);
    return reinterpret_cast<std::int64_t>(base);
}

std::int64_t auro_matic_v3_Engine_configure(
    std::uint8_t* base,
    std::uint32_t rate,
    const std::uint8_t* plan) {
    if (!base || !plan)
        return 1;
    const auto plan_u32 = reinterpret_cast<const std::uint32_t*>(plan);
    std::int64_t rc = auro_matic_v3_OutputGate_configure(base, rate, plan_u32[0]);
    if (rc != 0)
        return rc;
    rc = auro_matic_v3_FrontManager_configure(
        base + kMaticEngineFrontManagerOff, rate, plan + 8u);
    if (rc != 0)
        return rc;
    rc = auro_matic_v3_XinN_fl32_configure(
        base + kMaticEngineXinNOff,
        rate,
        reinterpret_cast<const std::uint64_t*>(plan + 112u));
    if (rc != 0)
        return rc;
    rc = auro_matic_v3_3d3d_configure(base + kMaticEngine3d3dOff, rate, plan + 144u);
    if (rc != 0)
        return rc;
    rc = auro_matic_v3_Gain_configure(base + kMaticEngineGainOff, plan_u32[47]);
    if (rc != 0)
        return rc;
    rc = auro_matic_Silence_configure(base + kMaticEngineSilenceOff, plan_u32[46]);
    if (rc != 0)
        return rc;
    *reinterpret_cast<std::uint64_t*>(base + kMaticEngineMaskOff) =
        *reinterpret_cast<const std::uint64_t*>(plan);
    return auro_matic_v3_Engine_reset_audio_state(base);
}

std::int64_t auro_matic_v3_Engine_reset_audio_state(std::uint8_t* base) {
    if (!base)
        return 0;
    auro_matic_v3_OutputGate_reset_audio_state(base);
    auro_matic_v3_FrontManager_reset_audio_state(base + kMaticEngineFrontManagerOff);
    auro_matic_v3_XinN_fl32_reset_audio_state(base + kMaticEngineXinNOff);
    auro_matic_v3_3d3d_reset_audio_state(base + kMaticEngine3d3dOff);
    auro_matic_v3_Gain_reset_audio_state();
    auro_matic_Silence_reset_audio_state();
    return 0;
}

void auro_matic_v3_Engine_process(std::uint8_t* base, void** table) {
    if (!base || !table)
        return;
    std::uint32_t active = 0u;
    std::uint32_t reset = 0u;
    float scratch_gains[32];
    auro_matic_v3_OutputGate_process(base, table, &active, &reset, scratch_gains);

    if (active != 0u) {
        const std::uint32_t input_mask =
            *reinterpret_cast<const std::uint32_t*>(base + kMaticEngineMaskOff);
        const std::uint32_t output_mask =
            *reinterpret_cast<const std::uint32_t*>(base + kMaticEngineMaskOff + 4u);
        const std::uint32_t purge = output_mask & ~input_mask;
        for (std::uint32_t slot = 0u; slot < 32u; ++slot) {
            if ((purge & (1u << slot)) == 0u)
                continue;
            auto* plane = static_cast<float*>(table[slot]);
            if (plane)
                std::memset(plane, 0, 32u * sizeof(float));
        }
    } else {
        float plane_a[32];
        float plane_b[32];
        float plane_c[32];
        auro_matic_v3_FrontManager_input_process(
            base + kMaticEngineFrontManagerOff, table, plane_a, plane_b, plane_c);
        auro_matic_v3_XinN_fl32_process(
            reinterpret_cast<std::uint64_t>(base + kMaticEngineXinNOff), table);
        auro_matic_v3_3d3d_process(base + kMaticEngine3d3dOff, table);
        auro_matic_v3_FrontManager_output_process(
            base + kMaticEngineFrontManagerOff, table, plane_a, plane_b, plane_c,
            scratch_gains);
        auro_matic_v3_Gain_process(base + kMaticEngineGainOff, table, scratch_gains);
        auro_matic_Silence_process(base + kMaticEngineSilenceOff, table);
    }

    if (reset != 0u)
        auro_matic_v3_Engine_reset_audio_state(base);
}

void auro_matic_v3_Engine_set_bypass(std::uint8_t* base, std::int32_t bypass) {
    if (base)
        *reinterpret_cast<std::uint32_t*>(base + kOutputGateStartedOff) =
            static_cast<std::uint32_t>(bypass);
}
