#pragma once

#include <cstdint>
#include <string>

namespace auro3d::encode {

struct WavPcm24Info {
    std::uint32_t sample_rate = 0;
    std::uint16_t channels = 0;
    std::uint32_t channel_mask = 0;
    std::uint64_t data_offset = 0;
    std::uint64_t data_bytes = 0;

    std::uint64_t frame_count() const {
        return channels == 0 ? 0 : data_bytes / (static_cast<std::uint64_t>(channels) * 3u);
    }
};

bool probe_wav_pcm24(const std::string& path, WavPcm24Info& info, std::string& error);

} // namespace auro3d:encode
