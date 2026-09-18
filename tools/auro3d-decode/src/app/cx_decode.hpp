#pragma once

#include "progress.hpp"

#include <string>
#include <vector>

namespace auro3d {

// True when the file is ISO-BMFF with an a3ds AuroCX audio sample entry.
// Non-MP4 / missing a3ds returns false (not an error).
bool mp4_has_auro_cx_a3ds(const std::string& path);

bool decode_auro_cx_mp4(
    const std::string& path,
    const std::string& out_wav,
    std::string& error,
    float headroom_db = 0.0f,
    unsigned output_bits = 24u,
    unsigned clear_output_lsb = 0u,
    bool binaural = false,
    unsigned room_preset = 0,
    unsigned hrtf_preset = 0,
    const std::string& output_format = "wav",
    const ProgressFn& progress = {},
    std::vector<std::string>* warnings = nullptr,
    bool binaural_hpv2 = false);

} // namespace auro3d
