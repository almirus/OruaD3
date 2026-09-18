#pragma once

#include "wav_input.hpp"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Converts the explicitly declared interleaved WAV order into codec-v3 channel
/// ID planes. It does not pad the final host block: native a3deng buffering
/// holds a short remainder until a full UnitBlock is ready, and source-length
/// coverage is owned by plan_codec_v3_unit_blocks rather than host padding.
class Pcm24InputStream {
public:
    bool open(
        const std::string& path,
        const WavPcm24Info& info,
        const std::vector<std::uint32_t>& interleaved_channel_ids,
        std::string& error);
    std::uint32_t read(std::uint32_t max_frames, std::vector<std::vector<std::int32_t>>& codec_planes, std::string& error);
    std::uint64_t frames_read() const { return frames_read_; }
    std::uint64_t frames_total() const { return virtual_frame_count_; }
    bool set_virtual_frame_count(std::uint64_t frame_count, std::string& error);

private:
    std::ifstream in_;
    WavPcm24Info info_{};
    std::vector<std::uint32_t> ids_;
    std::uint64_t frames_read_ = 0;
    std::uint64_t virtual_frame_count_ = 0;
};

/// Reassembles arbitrary host reads into the exact variable-size UnitBlock
/// schedule selected for the source.
class PcmUnitAccumulator {
public:
    explicit PcmUnitAccumulator(
        std::vector<std::uint32_t> active_channel_ids);

    bool push(
        const std::vector<std::vector<std::int32_t>>& codec_planes,
        std::uint32_t frame_count,
        std::string& error);
    bool has_frames(std::uint32_t frame_count) const;
    bool pop_frames(
        std::uint32_t frame_count,
        std::vector<std::vector<std::int32_t>>& codec_planes,
        std::string& error);
    std::uint32_t pending_frames() const;

private:
    std::vector<std::uint32_t> ids_;
    std::vector<std::vector<std::int32_t>> pending_;
};

} // namespace auro3d:encode
