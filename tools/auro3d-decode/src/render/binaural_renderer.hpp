#pragma once
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
    bool process(
        const std::vector<std::uint8_t>& input_pcm,
        std::vector<std::uint8_t>& stereo_pcm,
        std::string& error);

private:
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
    std::string& error);
}
