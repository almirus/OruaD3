#pragma once
#include "../app/progress.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace auro3d {
// Native stateful AHP HPV2 headphone renderer (headphone_pca_bank +
// headphone_source_manager + peak_limiter). This is the multichannel AHP
// graph, not the direct-stereo AM4HP Core. Input physical slots follow mask
// 0x67BF with slot 3 as LFE; HC (11) and Top (12) are folded onto the captured
// height HRTFs. Captured rates are 32/44.1/48/88.2/96 kHz, room 0 HPV2 only.
class AhpBinauralRenderer {
public:
    AhpBinauralRenderer();
    ~AhpBinauralRenderer();
    AhpBinauralRenderer(AhpBinauralRenderer&&) noexcept;
    AhpBinauralRenderer& operator=(AhpBinauralRenderer&&) noexcept;
    AhpBinauralRenderer(const AhpBinauralRenderer&) = delete;
    AhpBinauralRenderer& operator=(const AhpBinauralRenderer&) = delete;

    bool initialize(
        unsigned bits_per_sample,
        unsigned sample_rate,
        unsigned channels,
        const std::vector<std::uint32_t>& channel_slots,
        unsigned room_preset,
        unsigned hrtf_preset,
        std::size_t maximum_block_frames,
        std::string& error);
    // Stateful AHP follows the native public process contract: every aligned
    // input quantum produces the same number of output frames. The renderer
    // does not trim its algorithmic latency and has no tail-producing flush.
    bool process(
        const std::vector<std::uint8_t>& input_pcm,
        std::vector<std::uint8_t>& stereo_pcm,
        std::string& error);
    // AM4HP Core listening preset (0..3). Valid only for the AM4HP core path
    // (direct stereo 2.0 or dimensional 5.0.2H at 48 kHz); the multichannel
    // AHP graph has no Core preset.
    bool set_am4hp_core_preset(unsigned preset, std::string& error);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

bool render_binaural_ahp(
    const std::vector<std::uint8_t>& input_pcm,
    unsigned bits_per_sample,
    unsigned sample_rate,
    unsigned channels,
    const std::vector<std::uint32_t>& channel_slots,
    unsigned room_preset,
    unsigned hrtf_preset,
    std::vector<std::uint8_t>& stereo_pcm,
    std::string& error,
    unsigned am4hp_core_preset,
    const ProgressFn& progress = {});
}
