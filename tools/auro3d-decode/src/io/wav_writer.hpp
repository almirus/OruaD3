#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace wav {

/// WAV PCM 16-bit little-endian (простейший вариант для проверки пайплайна).
bool write_pcm16_le(
    const std::string& path,
    uint32_t sample_rate,
    uint16_t channels,
    const std::vector<std::uint8_t>& interleaved_pcm,
    std::string& error_out);

bool write_pcm24_le(
    const std::string& path,
    uint32_t sample_rate,
    uint16_t channels,
    const std::vector<std::uint8_t>& interleaved_pcm,
    std::string& error_out);

} // namespace wav
