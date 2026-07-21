#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace auro3d {

/// Experimental: when the decoded LFE slot is silent/absent, synthesize it from
/// bed channels (FL/FR/C/LS/RS/LB/RB) via mono sum + 120 Hz low-pass and −10 dB
/// LFE encode gain. This cannot recover an original discrete LFE track.
bool restore_lfe_if_silent(
    std::vector<std::uint8_t>& interleaved_pcm,
    unsigned bits_per_sample,
    unsigned sample_rate,
    unsigned channels,
    const std::vector<std::uint32_t>& output_slots,
    std::string& error,
    bool* applied_out = nullptr);

} // namespace auro3d
