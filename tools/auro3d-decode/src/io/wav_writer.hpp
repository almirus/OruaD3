#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace wav {

/// Embedded output tags written into WAV LIST/INFO and FLAC vorbis comments.
struct OutputMetadata {
    /// e.g. "Decoded by orua3d-decode 0.4.144, author @almirus"
    std::string comment;
    /// Channel names in file order, comma-separated: "FL,FR,C,LFE,LS,RS"
    std::string channel_names;

    bool empty() const { return comment.empty() && channel_names.empty(); }
};

/// WAV PCM 16-bit little-endian (простейший вариант для проверки пайплайна).
bool write_pcm16_le(
    const std::string& path,
    uint32_t sample_rate,
    uint16_t channels,
    const std::vector<std::uint8_t>& interleaved_pcm,
    std::string& error_out,
    const OutputMetadata& metadata = {});

bool write_pcm24_le(
    const std::string& path,
    uint32_t sample_rate,
    uint16_t channels,
    const std::vector<std::uint8_t>& interleaved_pcm,
    std::string& error_out,
    std::uint32_t channel_mask = 0,
    const OutputMetadata& metadata = {});

/// Encode an existing WAV file to FLAC via ffmpeg (PATH).
bool encode_wav_to_flac(
    const std::string& wav_path,
    const std::string& flac_path,
    std::string& error_out,
    const OutputMetadata& metadata = {});

class Pcm24StreamWriter {
public:
    void set_metadata(OutputMetadata metadata) { metadata_ = std::move(metadata); }

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
    OutputMetadata metadata_{};
};

} // namespace wav
