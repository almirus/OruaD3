#pragma once
#include "../app/progress.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace auro3d {
class BinauralStreamRenderer {
public:
    BinauralStreamRenderer();
    ~BinauralStreamRenderer();
    BinauralStreamRenderer(BinauralStreamRenderer&&) noexcept;
    BinauralStreamRenderer& operator=(BinauralStreamRenderer&&) noexcept;
    BinauralStreamRenderer(const BinauralStreamRenderer&) = delete;
    BinauralStreamRenderer& operator=(const BinauralStreamRenderer&) = delete;

    bool initialize(
        unsigned bits_per_sample,
        unsigned sample_rate,
        unsigned channels,
        const std::vector<std::uint32_t>& channel_slots,
        unsigned room_preset,
        unsigned hrtf_preset,
        std::size_t maximum_block_frames,
        std::string& error);
    // Explicit diagnostic/reference renderer. This is deliberately a named
    // entry point so production stateful AHP/AM4HP cannot fall back to the
    // finite bundled IR through a defaulted mode flag.
    bool initialize_reference_ir(
        unsigned bits_per_sample,
        unsigned sample_rate,
        unsigned channels,
        const std::vector<std::uint32_t>& channel_slots,
        unsigned room_preset,
        unsigned hrtf_preset,
        std::size_t maximum_block_frames,
        std::string& error);
    // Stateful AHP/AM4HP follows the native public process contract: every
    // aligned input quantum produces the same number of output frames. The
    // renderer does not trim its reported algorithmic latency and has no
    // tail-producing flush operation. At file EOF, callers may zero-pad the
    // final 32-frame quantum internally, but must trim that padding back to the
    // source frame count rather than append a synthetic reverb tail.
    bool process(
        const std::vector<std::uint8_t>& input_pcm,
        std::vector<std::uint8_t>& stereo_pcm,
        std::string& error);
    void reset_audio_state() noexcept;
    bool set_lfe_dynamic_gain(float gain) noexcept;
    bool set_am4hp_core_preset(unsigned preset, std::string& error);
    // Native v4 AHP Renderer::configure validates the requested layout as a
    // subset of 0x67BF, keeps a fixed full graph, and rebuilds its input maps.
    bool reconfigure(
        unsigned sample_rate,
        const std::vector<std::uint32_t>& channel_slots,
        unsigned room_preset,
        unsigned hrtf_preset,
        bool reset_audio,
        std::string& error);
    bool set_headphone_user_preset(unsigned preset, std::string& error);

private:
    bool initialize_mode(
        unsigned bits_per_sample,
        unsigned sample_rate,
        unsigned channels,
        const std::vector<std::uint32_t>& channel_slots,
        unsigned room_preset,
        unsigned hrtf_preset,
        std::size_t maximum_block_frames,
        std::string& error,
        bool reference_ir);
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

bool render_binaural_from_embedded_ir(
    const std::vector<std::uint8_t>& input_pcm,
    unsigned bits_per_sample,
    unsigned sample_rate,
    unsigned channels,
    const std::vector<std::uint32_t>& channel_slots,
    unsigned room_preset,
    unsigned hrtf_preset,
    std::vector<std::uint8_t>& stereo_pcm,
    std::string& error,
    const ProgressFn& progress = {});
}
