#include "wav_output.hpp"

#include "app_version.hpp"
#include "../codec_v3/frame_descriptor.hpp"

#include <array>
#include <filesystem>
#include <limits>

namespace auro3d::encode {
namespace {

void write_u16(std::ofstream& file, std::uint16_t value) {
    const std::array<char, 2> bytes = {
        static_cast<char>(value & 0xFFu), static_cast<char>(value >> 8u),
    };
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

void write_u32(std::ofstream& file, std::uint32_t value) {
    const std::array<char, 4> bytes = {
        static_cast<char>(value & 0xFFu), static_cast<char>((value >> 8u) & 0xFFu),
        static_cast<char>((value >> 16u) & 0xFFu), static_cast<char>(value >> 24u),
    };
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

std::uint32_t info_chunk_size(const char* id, const std::string& value) {
    const std::uint32_t bytes = static_cast<std::uint32_t>(value.size() + 1u);
    return 8u + bytes + (bytes & 1u);
}

void write_info_chunk(std::ofstream& file, const char* id, const std::string& value) {
    const std::uint32_t bytes = static_cast<std::uint32_t>(value.size() + 1u);
    file.write(id, 4);
    write_u32(file, bytes);
    file.write(value.data(), static_cast<std::streamsize>(value.size()));
    file.put('\0');
    if ((bytes & 1u) != 0u)
        file.put('\0');
}

} // namespace

WavPcm24Writer::~WavPcm24Writer() {
    std::string ignored;
    close(ignored);
}

bool WavPcm24Writer::open(
    const std::string& path,
    std::uint32_t sample_rate,
    const std::vector<std::uint32_t>& channel_order,
    std::uint64_t frame_count,
    std::string& error,
    std::uint32_t channel_mask) {
    error.clear();
    if (file_.is_open() || path.empty() || sample_rate == 0u || channel_order.empty() ||
        channel_order.size() > std::numeric_limits<std::uint16_t>::max()) {
        error = "invalid PCM24 WAV output parameters";
        return false;
    }
    std::array<bool, kCodecV3ChannelCount> seen{};
    for (const std::uint32_t channel_id : channel_order) {
        if (channel_id >= kCodecV3ChannelCount || seen[channel_id]) {
            error = "PCM24 WAV output channel order contains an invalid or duplicate id";
            return false;
        }
        seen[channel_id] = true;
    }
    if (channel_mask != 0u) {
        std::uint32_t mask_count = 0u;
        for (std::uint32_t mask = channel_mask; mask != 0u; mask >>= 1u)
            mask_count += mask & 1u;
        if (mask_count != channel_order.size()) {
            error = "PCM24 WAV channel mask count does not match channel order";
            return false;
        }
    }
    if (frame_count > std::numeric_limits<std::uint64_t>::max()
            / static_cast<std::uint64_t>(channel_order.size())
        || frame_count * channel_order.size()
            > std::numeric_limits<std::uint64_t>::max() / 3u) {
        error = "PCM24 WAV output frame count overflows its byte size";
        return false;
    }
    const std::uint64_t data_bytes = frame_count * channel_order.size() * 3u;
    const std::uint32_t fmt_size = channel_mask == 0u ? 16u : 40u;
    const std::string codec_name = "Auro3D codec-v3";
    const std::string version = auro3d_encode::kVersion;
    const std::string author = auro3d_encode::kAuthor;
    const std::uint32_t list_size = 4u
        + info_chunk_size("INAM", codec_name)
        + info_chunk_size("ICMT", auro3d_encode::make_encode_comment())
        + info_chunk_size("IART", author)
        + info_chunk_size("ICRD", version);
    const std::uint64_t list_bytes = 8u + list_size;
    if (data_bytes > std::numeric_limits<std::uint64_t>::max() - 20u - fmt_size - list_bytes) {
        error = "PCM24 WAV output RIFF size overflows 64-bit arithmetic";
        return false;
    }
    const std::uint64_t riff_size = data_bytes + 20u + fmt_size + list_bytes;
    if (data_bytes > 0xFFFFFFFFull || riff_size > 0xFFFFFFFFull) {
        error = "PCM24 WAV output exceeds RIFF size limit";
        return false;
    }
    if (static_cast<std::uint64_t>(sample_rate)
            > std::numeric_limits<std::uint64_t>::max()
                / static_cast<std::uint64_t>(channel_order.size())
        || static_cast<std::uint64_t>(sample_rate) * channel_order.size()
            > std::numeric_limits<std::uint64_t>::max() / 3u) {
        error = "PCM24 WAV byte rate overflows 64-bit arithmetic";
        return false;
    }
    const std::uint64_t byte_rate = static_cast<std::uint64_t>(sample_rate) * channel_order.size() * 3u;
    if (byte_rate > 0xFFFFFFFFull) {
        error = "PCM24 WAV byte rate exceeds RIFF limit";
        return false;
    }
    file_.open(
        std::filesystem::u8path(path),
        std::ios::binary | std::ios::trunc);
    if (!file_) {
        error = "cannot create WAV output";
        return false;
    }
    file_.write("RIFF", 4);
    write_u32(file_, static_cast<std::uint32_t>(riff_size));
    file_.write("WAVEfmt ", 8);
    write_u32(file_, fmt_size);
    write_u16(file_, channel_mask == 0u ? 1u : 0xFFFEu);
    write_u16(file_, static_cast<std::uint16_t>(channel_order.size()));
    write_u32(file_, sample_rate);
    write_u32(file_, static_cast<std::uint32_t>(byte_rate));
    write_u16(file_, static_cast<std::uint16_t>(channel_order.size() * 3u));
    write_u16(file_, 24u);
    if (channel_mask != 0u) {
        write_u16(file_, 22u);
        write_u16(file_, 24u);
        write_u32(file_, channel_mask);
        file_.write("\x01\x00\x00\x00\x00\x00\x10\x00\x80\x00\x00\xAA\x00\x38\x9B\x71", 16);
    }
    file_.write("LIST", 4);
    write_u32(file_, list_size);
    file_.write("INFO", 4);
    write_info_chunk(file_, "INAM", codec_name);
    write_info_chunk(file_, "ICMT", auro3d_encode::make_encode_comment());
    write_info_chunk(file_, "IART", author);
    write_info_chunk(file_, "ICRD", version);
    file_.write("data", 4);
    write_u32(file_, static_cast<std::uint32_t>(data_bytes));
    if (!file_) {
        error = "cannot write WAV header";
        file_.close();
        return false;
    }
    channel_order_ = channel_order;
    expected_frames_ = frame_count;
    written_frames_ = 0;
    channel_mask_ = channel_mask;
    return true;
}

bool WavPcm24Writer::write(
    const std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t frame_count,
    std::string& error) {
    error.clear();
    if (!file_.is_open() || codec_planes.size() != kCodecV3ChannelCount
        || written_frames_ > expected_frames_
        || frame_count > expected_frames_ - written_frames_) {
        error = "invalid PCM24 WAV write";
        return false;
    }
    for (const std::uint32_t channel_id : channel_order_) {
        if (channel_id >= codec_planes.size() || codec_planes[channel_id].size() < frame_count) {
            error = "carrier channel is missing PCM24 samples";
            return false;
        }
        for (std::uint32_t frame = 0; frame < frame_count; ++frame) {
            const std::int32_t sample = codec_planes[channel_id][frame];
            if (sample < -0x800000 || sample > 0x7FFFFF) {
                error = "carrier sample exceeds signed PCM24 range";
                return false;
            }
        }
    }
    std::array<char, 3> bytes{};
    for (std::uint32_t frame = 0; frame < frame_count; ++frame) {
        for (const std::uint32_t channel_id : channel_order_) {
            const std::uint32_t sample = static_cast<std::uint32_t>(codec_planes[channel_id][frame]);
            bytes[0] = static_cast<char>(sample & 0xFFu);
            bytes[1] = static_cast<char>((sample >> 8u) & 0xFFu);
            bytes[2] = static_cast<char>((sample >> 16u) & 0xFFu);
            file_.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        }
    }
    if (!file_) {
        error = "cannot write PCM24 WAV samples";
        return false;
    }
    written_frames_ += frame_count;
    return true;
}

bool WavPcm24Writer::close(std::string& error) {
    error.clear();
    if (!file_.is_open())
        return true;
    const bool complete = written_frames_ == expected_frames_;
    file_.close();
    if (!complete) {
        error = "PCM24 WAV output ended before its declared frame count";
        return false;
    }
    return true;
}

} // namespace auro3d::encode
