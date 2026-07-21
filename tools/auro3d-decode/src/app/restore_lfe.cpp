#include "restore_lfe.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace auro3d {
namespace {

constexpr std::uint32_t kSlotFl = 0;
constexpr std::uint32_t kSlotFr = 1;
constexpr std::uint32_t kSlotC = 2;
constexpr std::uint32_t kSlotLfe = 3;
constexpr std::uint32_t kSlotLs = 4;
constexpr std::uint32_t kSlotRs = 5;
constexpr std::uint32_t kSlotLb = 7;
constexpr std::uint32_t kSlotRb = 8;

// Cinema LFE content is typically low-passed near 120 Hz; encoded LFE is mixed
// ~10 dB down because playback applies +10 dB LFE boost.
constexpr float kLfeCutoffHz = 120.0f;
constexpr float kLfeEncodeGain = 0.316227766f; // -10 dB
// Treat LFE as absent if peak |sample| stays below ~-90 dBFS (24-bit).
constexpr float kSilentPeakFs = 3.0e-5f;

struct Biquad {
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1 = 0, z2 = 0;

    float process(float x) {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};

// RBJ Cookbook Butterworth-like low-pass (Q = 1/√2).
Biquad make_lowpass(float sample_rate, float cutoff_hz) {
    Biquad f;
    const float sr = std::max(sample_rate, 1.0f);
    const float fc = std::min(std::max(cutoff_hz, 1.0f), sr * 0.45f);
    const float w0 = 2.0f * 3.14159265358979323846f * fc / sr;
    const float cos_w0 = std::cos(w0);
    const float sin_w0 = std::sin(w0);
    const float alpha = sin_w0 * static_cast<float>(0.5 * std::sqrt(2.0));
    const float a0 = 1.0f + alpha;
    f.b0 = ((1.0f - cos_w0) * 0.5f) / a0;
    f.b1 = (1.0f - cos_w0) / a0;
    f.b2 = f.b0;
    f.a1 = (-2.0f * cos_w0) / a0;
    f.a2 = (1.0f - alpha) / a0;
    return f;
}

std::int32_t read_pcm_sample(const std::uint8_t* p, unsigned bytes) {
    if (bytes == 2) {
        const std::int16_t v = static_cast<std::int16_t>(p[0] | (p[1] << 8));
        return static_cast<std::int32_t>(v) << 8;
    }
    std::int32_t v = p[0] | (p[1] << 8) | (p[2] << 16);
    if (v & 0x800000)
        v |= ~0xFFFFFF;
    return v;
}

void write_pcm_sample(std::uint8_t* p, unsigned bytes, std::int32_t sample24) {
    if (sample24 > 0x7FFFFF)
        sample24 = 0x7FFFFF;
    if (sample24 < -0x800000)
        sample24 = -0x800000;
    if (bytes == 2) {
        int rounded = (sample24 + (sample24 >= 0 ? 128 : -128)) >> 8;
        if (rounded > 32767)
            rounded = 32767;
        if (rounded < -32768)
            rounded = -32768;
        const auto v = static_cast<std::uint16_t>(static_cast<std::int16_t>(rounded));
        p[0] = static_cast<std::uint8_t>(v & 0xFFu);
        p[1] = static_cast<std::uint8_t>((v >> 8) & 0xFFu);
        return;
    }
    p[0] = static_cast<std::uint8_t>(sample24 & 0xFFu);
    p[1] = static_cast<std::uint8_t>((sample24 >> 8) & 0xFFu);
    p[2] = static_cast<std::uint8_t>((sample24 >> 16) & 0xFFu);
}

bool is_bed_source_slot(std::uint32_t slot) {
    switch (slot) {
    case kSlotFl:
    case kSlotFr:
    case kSlotC:
    case kSlotLs:
    case kSlotRs:
    case kSlotLb:
    case kSlotRb:
        return true;
    default:
        return false;
    }
}

} // namespace

bool restore_lfe_if_silent(
    std::vector<std::uint8_t>& interleaved_pcm,
    unsigned bits_per_sample,
    unsigned sample_rate,
    unsigned channels,
    const std::vector<std::uint32_t>& output_slots,
    std::string& error,
    bool* applied_out) {
    if (applied_out)
        *applied_out = false;
    if (bits_per_sample != 16u && bits_per_sample != 24u) {
        error = "restore-lfe: unsupported bit depth";
        return false;
    }
    if (sample_rate == 0u || channels == 0u || output_slots.size() != channels) {
        error = "restore-lfe: invalid channel layout";
        return false;
    }

    const unsigned bytes = bits_per_sample == 24u ? 3u : 2u;
    const std::size_t frame_bytes = static_cast<std::size_t>(channels) * bytes;
    if (frame_bytes == 0u || interleaved_pcm.size() % frame_bytes != 0u) {
        error = "restore-lfe: PCM size is not frame-aligned";
        return false;
    }

    int lfe_index = -1;
    std::vector<unsigned> bed_indices;
    bed_indices.reserve(channels);
    for (unsigned i = 0; i < channels; ++i) {
        const std::uint32_t slot = output_slots[i];
        if (slot == kSlotLfe)
            lfe_index = static_cast<int>(i);
        else if (is_bed_source_slot(slot))
            bed_indices.push_back(i);
    }
    if (lfe_index < 0) {
        error = "restore-lfe: output has no LFE slot";
        return false;
    }
    if (bed_indices.empty()) {
        error = "restore-lfe: no bed channels available for synthesis";
        return false;
    }

    const std::size_t frames = interleaved_pcm.size() / frame_bytes;
    float peak_lfe = 0.0f;
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const std::uint8_t* p = interleaved_pcm.data()
            + frame * frame_bytes
            + static_cast<std::size_t>(lfe_index) * bytes;
        const float s = static_cast<float>(read_pcm_sample(p, bytes)) / 8388608.0f;
        peak_lfe = std::max(peak_lfe, std::fabs(s));
        if (peak_lfe >= kSilentPeakFs)
            break;
    }
    if (peak_lfe >= kSilentPeakFs) {
        // Reconstructed LFE already present — leave it alone.
        return true;
    }

    Biquad lpf = make_lowpass(static_cast<float>(sample_rate), kLfeCutoffHz);
    const float inv_beds = 1.0f / static_cast<float>(bed_indices.size());
    for (std::size_t frame = 0; frame < frames; ++frame) {
        std::uint8_t* base = interleaved_pcm.data() + frame * frame_bytes;
        float mono = 0.0f;
        for (unsigned idx : bed_indices) {
            const std::uint8_t* p = base + static_cast<std::size_t>(idx) * bytes;
            mono += static_cast<float>(read_pcm_sample(p, bytes)) / 8388608.0f;
        }
        mono *= inv_beds;
        float lfe = lpf.process(mono) * kLfeEncodeGain;
        if (lfe > 1.0f)
            lfe = 1.0f;
        if (lfe < -1.0f)
            lfe = -1.0f;
        const auto sample24 = static_cast<std::int32_t>(std::lround(lfe * 8388607.0f));
        write_pcm_sample(base + static_cast<std::size_t>(lfe_index) * bytes, bytes, sample24);
    }

    if (applied_out)
        *applied_out = true;
    return true;
}

} // namespace auro3d
