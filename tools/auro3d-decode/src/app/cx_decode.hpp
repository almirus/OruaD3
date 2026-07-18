#pragma once

#include <string>

namespace auro3d {

// True when the file is ISO-BMFF with an a3ds AuroCX audio sample entry.
// Non-MP4 / missing a3ds returns false (not an error).
bool mp4_has_auro_cx_a3ds(const std::string& path);

bool decode_auro_cx_mp4(
    const std::string& path,
    const std::string& out_wav,
    std::string& error,
    float headroom_db = 0.0f);

} // namespace auro3d
