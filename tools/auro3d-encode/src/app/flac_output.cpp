#include "flac_output.hpp"

#include "app_version.hpp"
#include "../codec_v3/frame_descriptor.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <limits>

namespace auro3d::encode {
namespace {

void append_be16(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value >> 8u));
    out.push_back(static_cast<std::uint8_t>(value));
}

void append_be24(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value >> 16u));
    out.push_back(static_cast<std::uint8_t>(value >> 8u));
    out.push_back(static_cast<std::uint8_t>(value));
}

void append_le32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value));
    out.push_back(static_cast<std::uint8_t>(value >> 8u));
    out.push_back(static_cast<std::uint8_t>(value >> 16u));
    out.push_back(static_cast<std::uint8_t>(value >> 24u));
}

void write_be16(std::ofstream& file, std::uint32_t value) {
    const std::array<char, 2> bytes = {
        static_cast<char>(value >> 8u),
        static_cast<char>(value),
    };
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

void write_be24(std::ofstream& file, std::uint32_t value) {
    const std::array<char, 3> bytes = {
        static_cast<char>(value >> 16u),
        static_cast<char>(value >> 8u),
        static_cast<char>(value),
    };
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

void append_utf8_uint(std::vector<std::uint8_t>& out, std::uint64_t value) {
    if (value < 0x80u) {
        out.push_back(static_cast<std::uint8_t>(value));
    } else if (value < 0x800u) {
        out.push_back(static_cast<std::uint8_t>(0xC0u | (value >> 6u)));
        out.push_back(static_cast<std::uint8_t>(0x80u | (value & 0x3Fu)));
    } else if (value < 0x10000u) {
        out.push_back(static_cast<std::uint8_t>(0xE0u | (value >> 12u)));
        out.push_back(static_cast<std::uint8_t>(0x80u | ((value >> 6u) & 0x3Fu)));
        out.push_back(static_cast<std::uint8_t>(0x80u | (value & 0x3Fu)));
    } else if (value < 0x200000u) {
        out.push_back(static_cast<std::uint8_t>(0xF0u | (value >> 18u)));
        out.push_back(static_cast<std::uint8_t>(0x80u | ((value >> 12u) & 0x3Fu)));
        out.push_back(static_cast<std::uint8_t>(0x80u | ((value >> 6u) & 0x3Fu)));
        out.push_back(static_cast<std::uint8_t>(0x80u | (value & 0x3Fu)));
    } else if (value < 0x4000000u) {
        out.push_back(static_cast<std::uint8_t>(0xF8u | (value >> 24u)));
        out.push_back(static_cast<std::uint8_t>(0x80u | ((value >> 18u) & 0x3Fu)));
        out.push_back(static_cast<std::uint8_t>(0x80u | ((value >> 12u) & 0x3Fu)));
        out.push_back(static_cast<std::uint8_t>(0x80u | ((value >> 6u) & 0x3Fu)));
        out.push_back(static_cast<std::uint8_t>(0x80u | (value & 0x3Fu)));
    } else if (value < 0x80000000u) {
        out.push_back(static_cast<std::uint8_t>(0xFCu | (value >> 30u)));
        out.push_back(static_cast<std::uint8_t>(0x80u | ((value >> 24u) & 0x3Fu)));
        out.push_back(static_cast<std::uint8_t>(0x80u | ((value >> 18u) & 0x3Fu)));
        out.push_back(static_cast<std::uint8_t>(0x80u | ((value >> 12u) & 0x3Fu)));
        out.push_back(static_cast<std::uint8_t>(0x80u | ((value >> 6u) & 0x3Fu)));
        out.push_back(static_cast<std::uint8_t>(0x80u | (value & 0x3Fu)));
    } else {
        out.push_back(0xFEu);
        out.push_back(static_cast<std::uint8_t>(0x80u | ((value >> 30u) & 0x3Fu)));
        out.push_back(static_cast<std::uint8_t>(0x80u | ((value >> 24u) & 0x3Fu)));
        out.push_back(static_cast<std::uint8_t>(0x80u | ((value >> 18u) & 0x3Fu)));
        out.push_back(static_cast<std::uint8_t>(0x80u | ((value >> 12u) & 0x3Fu)));
        out.push_back(static_cast<std::uint8_t>(0x80u | ((value >> 6u) & 0x3Fu)));
        out.push_back(static_cast<std::uint8_t>(0x80u | (value & 0x3Fu)));
    }
}

std::uint8_t crc8(const std::vector<std::uint8_t>& bytes) {
    std::uint8_t crc = 0;
    for (const std::uint8_t byte : bytes) {
        crc ^= byte;
        for (unsigned bit = 0; bit != 8u; ++bit)
            crc = (crc & 0x80u) != 0u ? static_cast<std::uint8_t>((crc << 1u) ^ 0x07u)
                                      : static_cast<std::uint8_t>(crc << 1u);
    }
    return crc;
}

std::uint16_t crc16(const std::vector<std::uint8_t>& bytes) {
    std::uint16_t crc = 0;
    for (const std::uint8_t byte : bytes) {
        crc ^= static_cast<std::uint16_t>(byte) << 8u;
        for (unsigned bit = 0; bit != 8u; ++bit)
            crc = (crc & 0x8000u) != 0u ? static_cast<std::uint16_t>((crc << 1u) ^ 0x8005u)
                                        : static_cast<std::uint16_t>(crc << 1u);
    }
    return crc;
}

void append_bits(std::vector<std::uint8_t>& bytes, std::uint8_t& bit_offset, std::uint32_t value, unsigned count) {
    for (unsigned bit = count; bit != 0u; --bit) {
        if (bit_offset == 0u)
            bytes.push_back(0u);
        if (((value >> (bit - 1u)) & 1u) != 0u)
            bytes.back() = static_cast<std::uint8_t>(bytes.back() | (1u << (7u - bit_offset)));
        bit_offset = static_cast<std::uint8_t>((bit_offset + 1u) & 7u);
    }
}

} // namespace

FlacPcm24Writer::~FlacPcm24Writer() {
    std::string ignored;
    close(ignored);
}

bool FlacPcm24Writer::open(
    const std::string& path,
    std::uint32_t sample_rate,
    const std::vector<std::uint32_t>& channel_order,
    std::uint64_t frame_count,
    std::string& error) {
    error.clear();
    if (file_.is_open() || path.empty() || sample_rate == 0u || sample_rate > 0xFFFFFu
        || channel_order.empty()
        || channel_order.size() > 8u || frame_count > 0xFFFFFFFFFull) {
        error = "invalid FLAC PCM24 output parameters";
        return false;
    }
    std::array<bool, kCodecV3ChannelCount> seen{};
    for (const std::uint32_t channel_id : channel_order) {
        if (channel_id >= kCodecV3ChannelCount || seen[channel_id]) {
            error = "FLAC output channel order contains an invalid or duplicate id";
            return false;
        }
        seen[channel_id] = true;
    }
    file_.open(
        std::filesystem::u8path(path),
        std::ios::binary | std::ios::trunc);
    if (!file_) {
        error = "cannot create FLAC output";
        return false;
    }
    file_.write("fLaC", 4);
    std::vector<std::uint8_t> streaminfo;
    streaminfo.reserve(34u);
    // Patched with actual values after the last frame has been written.
    append_be16(streaminfo, 0u);
    append_be16(streaminfo, 0u);
    append_be24(streaminfo, 0u);
    append_be24(streaminfo, 0u);
    const std::uint64_t packed = (static_cast<std::uint64_t>(sample_rate) << 44u)
        | (static_cast<std::uint64_t>(channel_order.size() - 1u) << 41u)
        | (23ull << 36u) | frame_count;
    streaminfo.push_back(static_cast<std::uint8_t>(packed >> 56u));
    streaminfo.push_back(static_cast<std::uint8_t>(packed >> 48u));
    streaminfo.push_back(static_cast<std::uint8_t>(packed >> 40u));
    streaminfo.push_back(static_cast<std::uint8_t>(packed >> 32u));
    streaminfo.push_back(static_cast<std::uint8_t>(packed >> 24u));
    streaminfo.push_back(static_cast<std::uint8_t>(packed >> 16u));
    streaminfo.push_back(static_cast<std::uint8_t>(packed >> 8u));
    streaminfo.push_back(static_cast<std::uint8_t>(packed));
    streaminfo.insert(streaminfo.end(), 16u, 0u);
    // STREAMINFO is followed by a last-block Vorbis Comment containing the
    // encoder identity. The streaminfo offset remains fixed at byte 8 for
    // the final seek-back patch below.
    file_.put(static_cast<char>(0x00u));
    file_.put(0u);
    file_.put(0u);
    file_.put(34u);
    file_.write(reinterpret_cast<const char*>(streaminfo.data()), static_cast<std::streamsize>(streaminfo.size()));
    std::vector<std::uint8_t> comments;
    const std::string vendor = auro3d_encode::kName;
    append_le32(comments, static_cast<std::uint32_t>(vendor.size()));
    comments.insert(comments.end(), vendor.begin(), vendor.end());
    append_le32(comments, 3u);
    const std::string fields[] = {
        "TITLE=Auro3D codec-v3",
        "VERSION=" + std::string(auro3d_encode::kVersion),
        "AUTHOR=" + std::string(auro3d_encode::kAuthor),
    };
    for (const std::string& field : fields) {
        append_le32(comments, static_cast<std::uint32_t>(field.size()));
        comments.insert(comments.end(), field.begin(), field.end());
    }
    if (comments.size() > 0xFFFFFFu) {
        error = "FLAC encoder metadata is too large";
        file_.close();
        return false;
    }
    const std::uint32_t comment_size = static_cast<std::uint32_t>(comments.size());
    file_.put(static_cast<char>(0x84u));
    file_.put(static_cast<char>(comment_size >> 16u));
    file_.put(static_cast<char>(comment_size >> 8u));
    file_.put(static_cast<char>(comment_size));
    file_.write(reinterpret_cast<const char*>(comments.data()), static_cast<std::streamsize>(comments.size()));
    if (!file_) {
        error = "cannot write FLAC STREAMINFO";
        file_.close();
        return false;
    }
    channel_order_ = channel_order;
    sample_rate_ = sample_rate;
    expected_frames_ = frame_count;
    written_frames_ = 0u;
    sample_offset_ = 0u;
    minimum_block_size_ = 0xFFFFu;
    maximum_block_size_ = 0u;
    minimum_frame_size_ = 0xFFFFFFu;
    maximum_frame_size_ = 0u;
    return true;
}

bool FlacPcm24Writer::write(
    const std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t frame_count,
    std::string& error) {
    error.clear();
    if (!file_.is_open() || frame_count == 0u || written_frames_ > expected_frames_
        || frame_count > expected_frames_ - written_frames_
        || sample_offset_ > std::numeric_limits<std::uint64_t>::max() - frame_count) {
        error = "invalid FLAC PCM24 write size";
        return false;
    }
    // CarrierOutput supplies one complete unit per call. `sample_offset_`
    // tracks the file position only; each supplied plane is unit-local.
    for (const std::uint32_t id : channel_order_) {
        if (id >= codec_planes.size() || codec_planes[id].size() < frame_count) {
            error = "carrier channel is missing PCM24 samples";
            return false;
        }
        for (std::uint32_t frame = 0u; frame < frame_count; ++frame) {
            const std::int32_t sample = codec_planes[id][frame];
            if (sample < -0x800000 || sample > 0x7FFFFF) {
                error = "carrier sample exceeds signed PCM24 range";
                return false;
            }
        }
    }
    std::uint32_t offset = 0u;
    while (offset < frame_count) {
        const std::uint32_t chunk = std::min<std::uint32_t>(256u, frame_count - offset);
        const std::uint64_t channel_bytes = static_cast<std::uint64_t>(channel_order_.size())
            * chunk * 3u;
        constexpr std::uint64_t kFrameOverhead = 32u;
        if (channel_bytes > std::numeric_limits<std::size_t>::max() - kFrameOverhead
            || channel_bytes + kFrameOverhead
                > static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max())) {
            error = "FLAC frame size exceeds stream storage limits";
            return false;
        }
        std::vector<std::uint8_t> frame;
        frame.reserve(static_cast<std::size_t>(channel_bytes + kFrameOverhead));
        frame.push_back(0xFFu);
        // Variable-block stream: supported codec units include 1000 samples,
        // so a short 232-sample FLAC frame can occur before later full frames.
        frame.push_back(0xF9u);
        frame.push_back(0x60u);
        // FLAC sample-size code 6 is the native 24-bit subframe width.
        frame.push_back(static_cast<std::uint8_t>(((channel_order_.size() - 1u) << 4u) | 6u));
        append_utf8_uint(frame, sample_offset_);
        frame.push_back(static_cast<std::uint8_t>(chunk - 1u));
        frame.push_back(crc8(frame));
        std::uint8_t bit_offset = 0u;
        for (const std::uint32_t id : channel_order_) {
            // Subframe header: zero padding, verbatim type (000001), no
            // wasted bits => 00000010 in MSB-first FLAC bit order.
            append_bits(frame, bit_offset, 2u, 8u);
            for (std::uint32_t sample = 0; sample < chunk; ++sample) {
                const std::uint32_t index = offset + sample;
                append_bits(frame, bit_offset, static_cast<std::uint32_t>(codec_planes[id][index]), 24u);
            }
        }
        if (bit_offset != 0u) {
            error = "internal FLAC bit writer did not end on a byte boundary";
            return false;
        }
        const std::uint16_t checksum = crc16(frame);
        frame.push_back(static_cast<std::uint8_t>(checksum >> 8u));
        frame.push_back(static_cast<std::uint8_t>(checksum));
        if (frame.size() > 0xFFFFFFu) {
            error = "FLAC frame exceeds STREAMINFO 24-bit size storage";
            return false;
        }
        file_.write(reinterpret_cast<const char*>(frame.data()), static_cast<std::streamsize>(frame.size()));
        if (!file_) {
            error = "cannot write FLAC frame";
            return false;
        }
        minimum_block_size_ =
            std::min(minimum_block_size_, chunk);
        maximum_block_size_ =
            std::max(maximum_block_size_, chunk);
        minimum_frame_size_ =
            std::min(
                minimum_frame_size_,
                static_cast<std::uint32_t>(frame.size()));
        maximum_frame_size_ =
            std::max(
                maximum_frame_size_,
                static_cast<std::uint32_t>(frame.size()));
        offset += chunk;
        written_frames_ += chunk;
        sample_offset_ += chunk;
    }
    return true;
}

bool FlacPcm24Writer::close(std::string& error) {
    error.clear();
    if (!file_.is_open())
        return true;
    const bool complete = written_frames_ == expected_frames_;
    if (complete
        && (minimum_block_size_ == 0xFFFFu
            || maximum_block_size_ == 0u
            || minimum_frame_size_ == 0xFFFFFFu
            || maximum_frame_size_ == 0u)) {
        file_.close();
        error = "FLAC output contains no complete audio frame";
        return false;
    }
    if (complete) {
        // STREAMINFO payload begins at file byte 8. Its first ten bytes are
        // min/max block size and min/max encoded frame size.
        file_.seekp(8, std::ios::beg);
        if (!file_) {
            file_.close();
            error = "cannot seek to FLAC STREAMINFO";
            return false;
        }
        write_be16(file_, minimum_block_size_);
        write_be16(file_, maximum_block_size_);
        write_be24(file_, minimum_frame_size_);
        write_be24(file_, maximum_frame_size_);
        if (!file_) {
            file_.close();
            error = "cannot finalize FLAC STREAMINFO";
            return false;
        }
    }
    file_.close();
    if (!complete) {
        error = "FLAC output ended before its declared frame count";
        return false;
    }
    return true;
}

} // namespace auro3d:encode
