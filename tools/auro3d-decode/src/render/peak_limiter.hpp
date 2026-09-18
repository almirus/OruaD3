#pragma once

#include <cstddef>
#include <cstdint>

namespace auro3d {

// Native A3DENG PeakLimiter: auro_compressor_v1 Processor, linked, float32.
// PeakLimiter:prepare libauro installs
// {attack=0, release=0.15, ratio=50, knee=0 dB, threshold=-0.5 dB}.
// Linked max is taken first, PeakFollower updates the envelope
// , then the 32-sample look-ahead gain computer (with
// a3[3]==1) writes one gain lane that is applied to every channel.
class NativePeakLimiter {
public:
    void reset();
    void set_sample_rate(std::uint32_t sample_rate);

    // Planar float, `channel_count` pointers of `frames` samples each.
    // Channels with a null pointer are skipped for both detect and apply.
    // `apply_pcm24_ceiling` is an export-path aid; the Float32 Manager pipeline
    // leaves it false to match native HDMI processing.
    void process_planar(
        float* const* channels,
        std::uint32_t channel_count,
        std::size_t frames,
        bool apply_pcm24_ceiling = true);

    // Two time-domain streams (AHP left/right).
    void process_stereo(double* left, double* right, std::size_t frames);

private:
    static constexpr std::size_t kBlock = 32;

    void ensure_coefficients();
    void follow_block(const float* peak, float* envelope, std::size_t count);
    void compute_gains(const float* envelope, float* gains, std::size_t count);
    void limit_gains_to_pcm24_ceiling(const float* peak, float* gains, std::size_t count);
    void process_count(
        float* const* channels,
        std::uint32_t channel_count,
        std::size_t frames,
        bool apply_pcm24_ceiling);

    std::uint32_t sample_rate_ = 0;
    float attack_old_ = 0.0f;
    float release_old_ = 0.0f;
    float attack_in_ = 1.0f;
    float release_in_ = 1.0f;
    float envelope_ = 0.0f;
    float prev_gain_ = 1.0f;
    float threshold_ = 1.0f;
    float inv_threshold_ = 1.0f;
    float exponent_ = 0.0f;
};

} // namespace auro3d
