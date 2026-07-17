#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
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

class Pcm24StreamWriter {
public:
    bool open(
        const std::string& path,
        std::uint32_t sample_rate,
        std::uint16_t channels,
        std::uint64_t frame_count,
        std::string& error_out,
        std::uint32_t channel_mask = 0);
    bool write(const std::vector<std::uint8_t>& interleaved_pcm, std::string& error_out);
    bool close(std::string& error_out);

private:
    std::ofstream out_;
    std::uint64_t expected_bytes_ = 0;
    std::uint64_t written_bytes_ = 0;
    std::uint16_t block_align_ = 0;
};

} // namespace wav
