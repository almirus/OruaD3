#include "pcm_input.hpp"

#include <algorithm>
#include <filesystem>
#include <array>
#include <limits>
#include <utility>

namespace auro3d::encode {

bool Pcm24InputStream::open(
    const std::string& path,
    const WavPcm24Info& info,
    const std::vector<std::uint32_t>& interleaved_channel_ids,
    std::string& error) {
    error.clear();
    if (interleaved_channel_ids.size() != info.channels) {
        error = "channel-order count does not match WAV channel count";
        return false;
    }
    std::array<bool, 31> seen{};
    for (const std::uint32_t id : interleaved_channel_ids) {
        if (id >= seen.size() || seen[id]) {
            error = "channel-order contains an invalid or duplicate codec channel";
            return false;
        }
        seen[id] = true;
    }
    if (in_.is_open())
        in_.close();
    in_.clear();
    in_.open(std::filesystem::u8path(path), std::ios::binary);
    if (!in_) {
        error = "cannot open input WAV";
        return false;
    }
    if (info.data_offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
        error = "WAV data offset out of range";
        return false;
    }
    in_.seekg(static_cast<std::streamoff>(info.data_offset), std::ios::beg);
    if (!in_) {
        error = "cannot seek to WAV data";
        return false;
    }
    info_ = info;
    ids_ = interleaved_channel_ids;
    frames_read_ = 0;
    virtual_frame_count_ = info_.frame_count();
    return true;
}

bool Pcm24InputStream::set_virtual_frame_count(
    std::uint64_t frame_count,
    std::string& error) {
    error.clear();
    if (!in_.is_open() || frame_count < info_.frame_count()) {
        error = "invalid virtual PCM frame count";
        return false;
    }
    virtual_frame_count_ = frame_count;
    return true;
}

std::uint32_t Pcm24InputStream::read(
    std::uint32_t max_frames,
    std::vector<std::vector<std::int32_t>>& codec_planes,
    std::string& error) {
    error.clear();
    codec_planes.assign(31u, {});
    if (max_frames == 0u || frames_read_ == virtual_frame_count_)
        return 0u;
    const std::uint64_t remaining = virtual_frame_count_ - frames_read_;
    const std::uint32_t count = static_cast<std::uint32_t>(
        remaining < max_frames ? remaining : max_frames);
    const std::uint64_t sample_bytes = static_cast<std::uint64_t>(info_.channels) * 3u;
    const std::uint64_t total_bytes = static_cast<std::uint64_t>(count) * sample_bytes;
    if (sample_bytes == 0u || total_bytes > std::numeric_limits<std::size_t>::max()
        || total_bytes > static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max())) {
        error = "WAV PCM read size overflows the host stream type";
        return 0u;
    }
    const std::size_t byte_count = static_cast<std::size_t>(total_bytes);
    std::vector<std::uint8_t> packed(byte_count);
    const std::uint64_t source_remaining =
        frames_read_ < info_.frame_count()
        ? info_.frame_count() - frames_read_
        : 0u;
    const std::uint32_t source_count = static_cast<std::uint32_t>(
        source_remaining < count ? source_remaining : count);
    const std::size_t source_bytes = static_cast<std::size_t>(source_count) *
        static_cast<std::size_t>(info_.channels) * 3u;
    if (source_bytes != 0u)
        in_.read(reinterpret_cast<char*>(packed.data()), static_cast<std::streamsize>(source_bytes));
    if (source_bytes != 0u
        && static_cast<std::size_t>(in_.gcount()) != source_bytes) {
        error = "truncated WAV data";
        return 0u;
    }
    std::fill(packed.begin() + static_cast<std::ptrdiff_t>(source_bytes), packed.end(), 0u);
    for (const std::uint32_t id : ids_)
        codec_planes[id].resize(count);
    for (std::uint32_t frame = 0; frame < count; ++frame) {
        for (std::uint16_t source = 0; source < info_.channels; ++source) {
            const std::size_t offset = (static_cast<std::size_t>(frame) * info_.channels + source) * 3u;
            const std::uint32_t raw = static_cast<std::uint32_t>(packed[offset])
                | (static_cast<std::uint32_t>(packed[offset + 1u]) << 8u)
                | (static_cast<std::uint32_t>(packed[offset + 2u]) << 16u);
            // Sign-extend PCM24 to the int32 sample representation used by codec-v3.
            codec_planes[ids_[source]][frame] = static_cast<std::int32_t>(
                (raw & 0x800000u) != 0u ? (raw | 0xFF000000u) : raw);
        }
    }
    frames_read_ += count;
    return count;
}

PcmUnitAccumulator::PcmUnitAccumulator(
    std::vector<std::uint32_t> active_channel_ids)
    : ids_(std::move(active_channel_ids)), pending_(31u) {}

bool PcmUnitAccumulator::push(
    const std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t frame_count,
    std::string& error) {
    error.clear();
    if (ids_.empty()) {
        error = "invalid codec unit accumulator configuration";
        return false;
    }
    if (frame_count == 0u) {
        error = "codec unit accumulator rejects an empty host block";
        return false;
    }
    if (codec_planes.size() != pending_.size()) {
        error = "invalid codec plane count";
        return false;
    }
    std::vector<bool> active(codec_planes.size(), false);
    for (const std::uint32_t id : ids_) {
        if (id >= codec_planes.size() || active[id]
            || codec_planes[id].size() != frame_count) {
            error = "invalid codec plane length";
            return false;
        }
        active[id] = true;
    }
    for (std::size_t id = 0; id < codec_planes.size(); ++id) {
        if (!active[id] && !codec_planes[id].empty()) {
            error = "inactive codec plane contains samples";
            return false;
        }
    }
    for (const std::uint32_t id : ids_) {
        if (static_cast<std::uint64_t>(frame_count)
                > std::numeric_limits<std::size_t>::max() - pending_[id].size()) {
            error = "codec unit accumulator sample count overflows size_t";
            return false;
        }
        if (pending_[id].size() > std::numeric_limits<std::uint32_t>::max()
            - frame_count) {
            error = "codec unit accumulator sample count exceeds 32-bit native span";
            return false;
        }
    }
    for (const std::uint32_t id : ids_) {
        pending_[id].insert(pending_[id].end(), codec_planes[id].begin(), codec_planes[id].end());
    }
    return true;
}

bool PcmUnitAccumulator::has_frames(std::uint32_t frame_count) const {
    return frame_count != 0u && !ids_.empty()
        && pending_[ids_.front()].size() >= frame_count;
}

bool PcmUnitAccumulator::pop_frames(
    std::uint32_t frame_count,
    std::vector<std::vector<std::int32_t>>& codec_planes,
    std::string& error) {
    error.clear();
    codec_planes.clear();
    if (!has_frames(frame_count)) {
        error = "codec unit accumulator does not contain the requested frame span";
        return false;
    }
    for (const std::uint32_t id : ids_) {
        if (pending_[id].size() < frame_count) {
            error = "codec unit accumulator active planes are inconsistent";
            return false;
        }
    }
    codec_planes.assign(pending_.size(), {});
    for (const std::uint32_t id : ids_) {
        auto& source = pending_[id];
        codec_planes[id].assign(
            source.begin(), source.begin() + frame_count);
        source.erase(source.begin(), source.begin() + frame_count);
    }
    return true;
}

std::uint32_t PcmUnitAccumulator::pending_frames() const {
    return ids_.empty() ? 0u : static_cast<std::uint32_t>(pending_[ids_.front()].size());
}

} // namespace auro3d:encode
