#pragma once

#include <cstdint>
#include <cstring>
#include <cstddef>
#include <vector>

// Native auro_a3deng_matic_resample_* factor-2 FIR (tap26), from libauro.so
// auro_fir_coefficients_Bank_get_resample_factor2_tap26 @ 0x529930.

namespace auro3deng {
namespace matic_resample {

inline constexpr std::uint32_t kTapCount = 26u;
inline constexpr std::uint32_t kPhaseTaps = 13u; // kTapCount / 2
inline constexpr std::uint32_t kMaxChannels = 15u;
inline constexpr std::uint32_t kQuantum = 32u;

// Exact float32 table from libauro.so VA 0x2B8C30 (symmetric halfband).
inline constexpr float kFactor2Tap26[kTapCount] = {
    -0.00914958026f, -0.0107425014f, 0.0199617296f, 0.00911884569f,
    -0.0160536394f, -0.0267183613f, 0.0154981352f, 0.0478715375f,
    -0.00221591769f, -0.0843838826f, -0.0397066884f, 0.182912245f,
    0.406624138f, 0.406624138f, 0.182912245f, -0.0397066884f,
    -0.0843838826f, -0.00221591769f, 0.0478715375f, 0.0154981352f,
    -0.0267183613f, -0.0160536394f, 0.00911884569f, 0.0199617296f,
    -0.0107425014f, -0.00914958026f,
};

// Resample object layout at step+64 (Up and Down share the head).
struct ResampleObject {
    std::uint32_t channel_count = 0u;           // +0
    std::uint32_t channel_ids[kMaxChannels]{};  // +4 .. +63
    float* history = nullptr;                   // +64
    std::uint32_t history_words = 0u;           // +72
    std::uint32_t pad1 = 0u;                    // +76
    std::uint64_t tap_count = 0u;               // +80
    const float* coeffs = nullptr;              // +88
};

inline ResampleObject* object_at(std::uint8_t* step) {
    static_assert(offsetof(ResampleObject, history) == 64u, "history @+64");
    static_assert(offsetof(ResampleObject, coeffs) == 88u, "coeffs @+88");
    return reinterpret_cast<ResampleObject*>(step + 64u);
}

inline void pack_channel_ids_from_mask(ResampleObject* obj, std::uint32_t layout) {
    std::uint32_t n = 0u;
    for (std::uint32_t bit = 0u; bit < kMaxChannels; ++bit) {
        if ((layout & (1u << bit)) == 0u)
            continue;
        obj->channel_ids[n++] = bit;
    }
    obj->channel_count = n;
}

// auro_a3deng_matic_resample_Up_init @ 0x56FB10 (factor==2 only).
inline bool init_up(ResampleObject* obj, std::uint32_t layout) {
    if (!obj)
        return false;
    obj->tap_count = kTapCount;
    obj->coeffs = kFactor2Tap26;
    pack_channel_ids_from_mask(obj, layout);
    if (obj->channel_count == 0u)
        return false;
    obj->history_words = kPhaseTaps * obj->channel_count;
    return true;
}

// auro_a3deng_matic_resample_Down_init @ 0x56FFF0 (factor==2 only).
inline bool init_down(ResampleObject* obj, std::uint32_t layout) {
    if (!obj)
        return false;
    obj->tap_count = kTapCount;
    obj->coeffs = kFactor2Tap26;
    pack_channel_ids_from_mask(obj, layout);
    if (obj->channel_count == 0u)
        return false;
    obj->history_words = kTapCount * obj->channel_count;
    return true;
}

inline void clear_up(ResampleObject* obj) {
    if (!obj || !obj->history || obj->channel_count == 0u)
        return;
    std::memset(obj->history, 0, sizeof(float) * kPhaseTaps * obj->channel_count);
    obj->history_words = kPhaseTaps * obj->channel_count;
}

inline void clear_down(ResampleObject* obj) {
    if (!obj || !obj->history || obj->channel_count == 0u)
        return;
    std::memset(obj->history, 0, sizeof(float) * kTapCount * obj->channel_count);
    obj->history_words = kTapCount * obj->channel_count;
}

// Native w32_Up @ 0x59C690 is a causal factor-2 interpolator with a one-output-
// sample phase offset: out[2n+1] uses the even taps and input[n], while
// out[2n] uses the odd taps and input[n-1]. History is oldest-to-newest and is
// replaced by input[19..31]. The caller may alias input and output, so capture
// the complete input window before writing.
inline void w32_up_channel(
    float* hist13,
    const float* input32,
    float* output64,
    const float* coeffs) {
    float window[kPhaseTaps + kQuantum];
    std::memcpy(window, hist13, sizeof(float) * kPhaseTaps);
    std::memcpy(window + kPhaseTaps, input32, sizeof(float) * kQuantum);
    float out[2u * kQuantum];
    auto sample = [&](std::int32_t index) -> float {
        return index < 0
            ? window[static_cast<std::uint32_t>(index + static_cast<std::int32_t>(kPhaseTaps))]
            : window[kPhaseTaps + static_cast<std::uint32_t>(index)];
    };
    for (std::int32_t n = 0; n < static_cast<std::int32_t>(kQuantum); ++n) {
        float even = 0.0f;
        float odd = 0.0f;
        // w32_Up accumulates from the oldest retained sample toward the
        // current sample; preserving that order is required for float32 bit
        // identity with the unrolled native implementation.
        for (std::uint32_t remaining = kPhaseTaps; remaining != 0u; --remaining) {
            const std::uint32_t k = remaining - 1u;
            const auto delay = static_cast<std::int32_t>(k);
            odd += sample(n - delay) * (coeffs[2u * k] + coeffs[2u * k]);
            even += sample(n - 1 - delay)
                * (coeffs[2u * k + 1u] + coeffs[2u * k + 1u]);
        }
        out[2u * static_cast<std::uint32_t>(n)] = even;
        out[2u * static_cast<std::uint32_t>(n) + 1u] = odd;
    }
    // Last 13 input samples: window[32..44] == in[19..31].
    std::memcpy(hist13, window + kQuantum, sizeof(float) * kPhaseTaps);
    std::memcpy(output64, out, sizeof(out));
}

// Native w32_Down: hist(26)+in(64), with a separate compacted output span;
// y[n]=sum_k h[k]*x[2n-k]. Accumulation is oldest-to-newest, matching the
// native SIMD reduction order; history <- input[38..63].
inline void w32_down_channel(
    float* hist26,
    const float* samples64,
    float* output32,
    const float* coeffs) {
    float window[kTapCount + 2u * kQuantum];
    std::memcpy(window, hist26, sizeof(float) * kTapCount);
    std::memcpy(window + kTapCount, samples64, sizeof(float) * 2u * kQuantum);
    float out[kQuantum];
    for (std::uint32_t n = 0u; n < kQuantum; ++n) {
        float acc = 0.0f;
        const std::uint32_t center = kTapCount + 2u * n;
        for (std::uint32_t remaining = kTapCount; remaining != 0u; --remaining) {
            const std::uint32_t k = remaining - 1u;
            acc += window[center - k] * coeffs[k];
        }
        out[n] = acc;
    }
    std::memcpy(hist26, window + 2u * kQuantum, sizeof(float) * kTapCount);
    std::memcpy(output32, out, sizeof(out));
}

inline bool planar_factor2(std::vector<float>& samples, std::size_t& frames,
    std::size_t channels, std::vector<float>& history, bool up) {
    const std::size_t quantum = up ? 32u : 64u;
    const std::size_t taps = up ? kPhaseTaps : kTapCount;
    if (frames % quantum || samples.size() != channels * frames)
        return false;
    if (history.size() != channels * taps)
        history.assign(channels * taps, 0.0f);
    const std::size_t output_frames = up ? frames * 2u : frames / 2u;
    std::vector<float> output(channels * output_frames);
    for (std::size_t ch = 0; ch < channels; ++ch) {
        for (std::size_t frame = 0; frame < frames; frame += quantum) {
            const float* input = samples.data() + ch * frames + frame;
            float* dest = output.data() + ch * output_frames + (up ? frame * 2u : frame / 2u);
            if (up) w32_up_channel(history.data() + ch * taps, input, dest, kFactor2Tap26);
            else w32_down_channel(history.data() + ch * taps, input, dest, kFactor2Tap26);
        }
    }
    samples.swap(output);
    frames = output_frames;
    return true;
}

} // namespace matic_resample
} // namespace auro3deng
