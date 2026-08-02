#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace auro3d::encode {

/// PCM24 RIFF writer for the completed codec-v3 carrier. The codec metadata
/// itself lives in PCM sample bits; no private RIFF chunk is fabricated.
class WavPcm24Writer {
public:
    WavPcm24Writer() = default;
    ~WavPcm24Writer();

    bool open(
        const std::string& path,
        std::uint32_t sample_rate,
        const std::vector<std::uint32_t>& channel_order,
        std::uint64_t frame_count,
        std::string& error,
        std::uint32_t channel_mask = 0u);
    bool write(
        const std::vector<std::vector<std::int32_t>>& codec_planes,
        std::uint32_t frame_count,
        std::string& error);
    bool close(std::string& error);

private:
    std::ofstream file_;
    std::vector<std::uint32_t> channel_order_;
    std::uint64_t expected_frames_ = 0;
    std::uint64_t written_frames_ = 0;
    std::uint32_t channel_mask_ = 0;
};

} // namespace auro3d::encode
