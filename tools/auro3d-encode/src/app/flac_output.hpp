#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Minimal FLAC container writer using verbatim PCM24 subframes. Codec-v3
/// metadata remains in the carrier sample bits; this class only supplies the
/// lossless container and its required frame checksums.
class FlacPcm24Writer {
public:
    FlacPcm24Writer() = default;
    ~FlacPcm24Writer();

    bool open(
        const std::string& path,
        std::uint32_t sample_rate,
        const std::vector<std::uint32_t>& channel_order,
        std::uint64_t frame_count,
        std::string& error);
    bool write(
        const std::vector<std::vector<std::int32_t>>& codec_planes,
        std::uint32_t frame_count,
        std::string& error);
    bool close(std::string& error);

private:
    std::ofstream file_;
    std::vector<std::uint32_t> channel_order_;
    std::uint32_t sample_rate_ = 0;
    std::uint64_t expected_frames_ = 0;
    std::uint64_t written_frames_ = 0;
    std::uint64_t sample_offset_ = 0;
    std::uint32_t minimum_block_size_ = 0xFFFFu;
    std::uint32_t maximum_block_size_ = 0u;
    std::uint32_t minimum_frame_size_ = 0xFFFFFFu;
    std::uint32_t maximum_frame_size_ = 0u;
};

} // namespace auro3d::encode
