#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d {
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
