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

/// Classic RIFF WAV max payload before RF64 is required (leave headroom for LIST/INFO).
/// Override at compile-time: /DORUA3D_RIFF_SAFE_MAX_DATA_BYTES=N
#ifndef ORUA3D_RIFF_SAFE_MAX_DATA_BYTES
#define ORUA3D_RIFF_SAFE_MAX_DATA_BYTES 0xFFFF0000ull
#endif
inline constexpr std::uint64_t kRiffSafeMaxDataBytes =
    static_cast<std::uint64_t>(ORUA3D_RIFF_SAFE_MAX_DATA_BYTES);

/// Output container for PCM writers.
enum class PcmContainer {
    WavAuto, ///< Classic RIFF, or RF64 when data exceeds kRiffSafeMaxDataBytes.
    W64,     ///< Sony Wave64 (.w64) — no 4 GiB limit; preferred for large multichannel masters.
};

/// WAV/W64 PCM 16-bit little-endian.
bool write_pcm16_le(
    const std::string& path,
    uint32_t sample_rate,
    uint16_t channels,
    const std::vector<std::uint8_t>& interleaved_pcm,
    std::string& error_out,
    std::uint32_t channel_mask = 0,
    const OutputMetadata& metadata = {},
    PcmContainer container = PcmContainer::WavAuto);

bool write_pcm24_le(
    const std::string& path,
    uint32_t sample_rate,
    uint16_t channels,
    const std::vector<std::uint8_t>& interleaved_pcm,
    std::string& error_out,
    std::uint32_t channel_mask = 0,
    const OutputMetadata& metadata = {},
    PcmContainer container = PcmContainer::WavAuto);

/// Encode an existing WAV/RF64/W64 file to FLAC via ffmpeg (PATH).
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
        std::uint32_t channel_mask = 0,
        PcmContainer container = PcmContainer::WavAuto);
    bool write(const std::vector<std::uint8_t>& interleaved_pcm, std::string& error_out);
    bool close(std::string& error_out);

    bool uses_rf64() const { return use_rf64_; }
    bool uses_w64() const { return use_w64_; }

private:
    std::ofstream out_;
    std::uint64_t expected_bytes_ = 0;
    std::uint64_t written_bytes_ = 0;
    std::uint64_t frame_count_ = 0;
    std::uint16_t block_align_ = 0;
    std::uint64_t ds64_chunk_pos_ = 0; // file offset of ds64 payload (after id+size)
    std::uint64_t w64_riff_size_pos_ = 0; // offset of 64-bit riff size field
    bool use_rf64_ = false;
    bool use_w64_ = false;
    OutputMetadata metadata_{};
};

} // namespace wav
