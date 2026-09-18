#pragma once

#include <cstddef>
#include <string>

namespace auro3d_decode {

constexpr char kVersion[] = "0.5.11";
constexpr char kName[] = "orua3d-decode";

/// Author handle without a contiguous plaintext literal in the binary.
inline std::string make_author() {
    // " almirus" = bytes[i] XOR (0x37 + 3*i)
    static constexpr unsigned char kEnc[] = {
        0x77, 0x5B, 0x51, 0x2D, 0x2A, 0x34, 0x3C, 0x3F,
    };
    std::string out;
    out.resize(sizeof(kEnc));
    for (std::size_t i = 0; i < sizeof(kEnc); ++i)
        out[i] = static_cast<char>(kEnc[i] ^ static_cast<unsigned char>(0x37 + 3 * i));
    return out;
}

inline std::string make_decode_comment(const std::string& source_audio_coding = {}) {
    std::string comment = std::string("Decoded by ") + kName + " " + kVersion
        + ", " + make_author();
    if (!source_audio_coding.empty())
        comment += ", source audioCoding=" + source_audio_coding;
    return comment;
}

} // namespace auro3d_decode
