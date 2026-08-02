#include "wav_input.hpp"

#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <vector>

namespace auro3d::encode {
namespace {

std::uint16_t read_le16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0]) | (static_cast<std::uint16_t>(p[1]) << 8u);
}

std::uint32_t read_le32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0])
        | (static_cast<std::uint32_t>(p[1]) << 8u)
        | (static_cast<std::uint32_t>(p[2]) << 16u)
        | (static_cast<std::uint32_t>(p[3]) << 24u);
}

bool read_exact(std::ifstream& in, void* dst, std::size_t count) {
    in.read(static_cast<char*>(dst), static_cast<std::streamsize>(count));
    return static_cast<std::size_t>(in.gcount()) == count;
}

} // namespace

bool probe_wav_pcm24(const std::string& path, WavPcm24Info& info, std::string& error) {
    info = {};
    error.clear();
    std::ifstream in(std::filesystem::u8path(path), std::ios::binary);
    if (!in) {
        error = "cannot open input WAV";
        return false;
    }

    std::array<std::uint8_t, 12> riff{};
    if (!read_exact(in, riff.data(), riff.size())
        || std::memcmp(riff.data(), "RIFF", 4) != 0
        || std::memcmp(riff.data() + 8, "WAVE", 4) != 0) {
        error = "input must be a RIFF/WAVE file";
        return false;
    }
    const std::uint32_t riff_payload_size =
        read_le32(riff.data() + 4);
    if (riff_payload_size < 4u) {
        error = "input WAV has an invalid RIFF payload size";
        return false;
    }

    bool got_fmt = false;
    bool got_data = false;
    while (in && !got_data) {
        std::array<std::uint8_t, 8> header{};
        if (!read_exact(in, header.data(), header.size()))
            break;
        const std::uint32_t size = read_le32(header.data() + 4);
        const std::streamoff payload = in.tellg();
        if (payload < 0) {
            error = "invalid WAV chunk offset";
            return false;
        }
        if (std::memcmp(header.data(), "fmt ", 4) == 0) {
            if (size < 16u || size > 4096u) {
                error = "invalid WAV fmt chunk";
                return false;
            }
            std::vector<std::uint8_t> fmt(size);
            if (!read_exact(in, fmt.data(), fmt.size())) {
                error = "truncated WAV fmt chunk";
                return false;
            }
            if ((size & 1u) != 0u)
                in.seekg(1, std::ios::cur);
            const std::uint16_t format = read_le16(fmt.data());
            info.channels = read_le16(fmt.data() + 2);
            info.sample_rate = read_le32(fmt.data() + 4);
            const std::uint32_t byte_rate = read_le32(fmt.data() + 8);
            const std::uint16_t block_align = read_le16(fmt.data() + 12);
            const std::uint16_t bits = read_le16(fmt.data() + 14);
            if (format == 0xFFFEu && size >= 40u) {
                const std::uint16_t extension_size =
                    read_le16(fmt.data() + 16);
                const std::uint16_t valid_bits =
                    read_le16(fmt.data() + 18);
                if (extension_size < 22u
                    || static_cast<std::uint32_t>(extension_size) + 18u
                        > size
                    || valid_bits != 24u) {
                    error = "WAVEFORMATEXTENSIBLE input must declare 24 valid PCM bits";
                    return false;
                }
                // KSDATAFORMAT_SUBTYPE_PCM, laid out little-endian in WAV.
                static constexpr std::uint8_t kPcmGuid[16] = {
                    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
                    0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71,
                };
                if (std::memcmp(fmt.data() + 24, kPcmGuid, sizeof(kPcmGuid)) != 0) {
                    error = "WAVEFORMATEXTENSIBLE input must use the PCM subformat";
                    return false;
                }
                info.channel_mask = read_le32(fmt.data() + 20);
            } else if (format != 1u) {
                error = "input WAV must use PCM or WAVEFORMATEXTENSIBLE PCM";
                return false;
            }
            const std::uint64_t expected_byte_rate =
                static_cast<std::uint64_t>(info.sample_rate)
                * info.channels * 3u;
            if (info.channels < 2u || info.channels > 31u
                || info.sample_rate == 0u || bits != 24u
                || static_cast<std::uint32_t>(info.channels) * 3u > 0xFFFFu
                || block_align != static_cast<std::uint16_t>(info.channels * 3u)
                || expected_byte_rate > 0xFFFFFFFFu
                || byte_rate != expected_byte_rate) {
                error = "input WAV must be multichannel PCM24 with a valid block alignment";
                return false;
            }
            got_fmt = true;
            continue;
        } else if (std::memcmp(header.data(), "data", 4) == 0) {
            if (!got_fmt) {
                error = "WAV data precedes fmt chunk";
                return false;
            }
            info.data_offset = static_cast<std::uint64_t>(payload);
            info.data_bytes = size;
            if (info.data_offset > std::numeric_limits<std::uint64_t>::max() - info.data_bytes) {
                error = "WAV data range overflows 64-bit offset";
                return false;
            }
            if (info.data_bytes % (static_cast<std::uint64_t>(info.channels) * 3u) != 0u) {
                error = "WAV data size is not an integral PCM24 frame count";
                return false;
            }
            got_data = true;
            break;
        }
        const std::uint64_t skip = static_cast<std::uint64_t>(size) + (size & 1u);
        if (skip > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
            error = "WAV chunk too large";
            return false;
        }
        in.seekg(static_cast<std::streamoff>(skip), std::ios::cur);
    }
    if (!got_fmt || !got_data) {
        error = "WAV requires fmt and data chunks";
        return false;
    }
    std::error_code size_error;
    const std::uintmax_t file_size =
        std::filesystem::file_size(
            std::filesystem::u8path(path), size_error);
    const std::uint64_t riff_end =
        8u + static_cast<std::uint64_t>(riff_payload_size);
    const std::uint64_t data_end =
        info.data_offset + info.data_bytes;
    if (size_error
        || file_size > std::numeric_limits<std::uint64_t>::max()
        || riff_end > static_cast<std::uint64_t>(file_size)
        || data_end > riff_end) {
        error = "WAV RIFF or data range exceeds the physical file";
        return false;
    }
    return true;
}

} // namespace auro3d::encode
