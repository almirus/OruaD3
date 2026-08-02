#include "wav_writer.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace wav {
namespace {

#ifdef _WIN32
std::wstring utf8_to_wide(const std::string& value) {
    if (value.empty())
        return {};
    const int size = MultiByteToWideChar(
        CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (size <= 0)
        return {};
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    if (MultiByteToWideChar(
            CP_UTF8, 0, value.data(), static_cast<int>(value.size()), wide.data(), size)
        <= 0) {
        return {};
    }
    return wide;
}

bool open_binary_trunc(std::ofstream& out, const std::string& utf8_path) {
    const std::wstring wide = utf8_to_wide(utf8_path);
    if (wide.empty() && !utf8_path.empty())
        return false;
    out.open(wide.c_str(), std::ios::binary | std::ios::trunc);
    return static_cast<bool>(out);
}
#else
bool open_binary_trunc(std::ofstream& out, const std::string& utf8_path) {
    out.open(utf8_path, std::ios::binary | std::ios::trunc);
    return static_cast<bool>(out);
}
#endif

std::string shell_quote(const std::string& path) {
    std::string quoted = "\"";
    for (char c : path) {
        if (c == '\"')
            quoted += '\\';
        quoted += c;
    }
    return quoted + "\"";
}

void write_le32(std::ostream& out, std::uint32_t value) {
    const unsigned char b[4] = {
        static_cast<unsigned char>(value & 0xFFu),
        static_cast<unsigned char>((value >> 8) & 0xFFu),
        static_cast<unsigned char>((value >> 16) & 0xFFu),
        static_cast<unsigned char>((value >> 24) & 0xFFu),
    };
    out.write(reinterpret_cast<const char*>(b), 4);
}

void write_le64(std::ostream& out, std::uint64_t value) {
    write_le32(out, static_cast<std::uint32_t>(value & 0xFFFFFFFFu));
    write_le32(out, static_cast<std::uint32_t>((value >> 32) & 0xFFFFFFFFu));
}

void write_u16(std::ostream& out, std::uint16_t value) {
    const unsigned char b[2] = {
        static_cast<unsigned char>(value & 0xFFu),
        static_cast<unsigned char>((value >> 8) & 0xFFu),
    };
    out.write(reinterpret_cast<const char*>(b), 2);
}

// Sony Wave64 GUIDs (little-endian on disk; first 4 bytes spell the FOURCC).
constexpr char kW64GuidRiff[16] = {
    'r', 'i', 'f', 'f',
    static_cast<char>(0x2E), static_cast<char>(0x91), static_cast<char>(0xCF), static_cast<char>(0x11),
    static_cast<char>(0xA5), static_cast<char>(0xD6), static_cast<char>(0x28), static_cast<char>(0xDB),
    static_cast<char>(0x04), static_cast<char>(0xC1), static_cast<char>(0x00), static_cast<char>(0x00),
};
constexpr char kW64GuidWave[16] = {
    'w', 'a', 'v', 'e',
    static_cast<char>(0xF3), static_cast<char>(0xAC), static_cast<char>(0xD3), static_cast<char>(0x11),
    static_cast<char>(0x8C), static_cast<char>(0xD1), static_cast<char>(0x00), static_cast<char>(0xC0),
    static_cast<char>(0x4F), static_cast<char>(0x8E), static_cast<char>(0xDB), static_cast<char>(0x8A),
};
constexpr char kW64GuidFmt[16] = {
    'f', 'm', 't', ' ',
    static_cast<char>(0xF3), static_cast<char>(0xAC), static_cast<char>(0xD3), static_cast<char>(0x11),
    static_cast<char>(0x8C), static_cast<char>(0xD1), static_cast<char>(0x00), static_cast<char>(0xC0),
    static_cast<char>(0x4F), static_cast<char>(0x8E), static_cast<char>(0xDB), static_cast<char>(0x8A),
};
constexpr char kW64GuidData[16] = {
    'd', 'a', 't', 'a',
    static_cast<char>(0xF3), static_cast<char>(0xAC), static_cast<char>(0xD3), static_cast<char>(0x11),
    static_cast<char>(0x8C), static_cast<char>(0xD1), static_cast<char>(0x00), static_cast<char>(0xC0),
    static_cast<char>(0x4F), static_cast<char>(0x8E), static_cast<char>(0xDB), static_cast<char>(0x8A),
};

std::uint64_t w64_pad8(std::uint64_t n) {
    return (n + 7ull) & ~7ull;
}

void write_w64_chunk_header(std::ostream& out, const char guid[16], std::uint64_t payload_bytes) {
    // Size includes the 24-byte GUID+size header; padding is NOT included.
    out.write(guid, 16);
    write_le64(out, 24ull + payload_bytes);
}

bool write_list_info(std::ostream& out, const OutputMetadata& metadata, std::string& error_out) {
    if (metadata.empty())
        return true;
    std::vector<char> buf;
    auto append_chunk = [&](const char id[4], const std::string& text) {
        if (text.empty())
            return;
        const std::uint32_t raw_size = static_cast<std::uint32_t>(text.size() + 1u);
        const std::size_t pad = (raw_size & 1u) ? 1u : 0u;
        const std::size_t old = buf.size();
        buf.resize(old + 8u + raw_size + pad);
        char* p = buf.data() + old;
        p[0] = id[0]; p[1] = id[1]; p[2] = id[2]; p[3] = id[3];
        p[4] = static_cast<char>(raw_size & 0xFF);
        p[5] = static_cast<char>((raw_size >> 8) & 0xFF);
        p[6] = static_cast<char>((raw_size >> 16) & 0xFF);
        p[7] = static_cast<char>((raw_size >> 24) & 0xFF);
        std::copy(text.begin(), text.end(), p + 8);
        p[8 + text.size()] = '\0';
        if (pad)
            p[8 + raw_size] = '\0';
    };
    append_chunk("ICMT", metadata.comment);
    if (buf.empty())
        return true;

    out.write("LIST", 4);
    write_le32(out, static_cast<std::uint32_t>(4u + buf.size()));
    out.write("INFO", 4);
    out.write(buf.data(), static_cast<std::streamsize>(buf.size()));
    if (!out) {
        error_out = "failed to write WAV LIST/INFO metadata";
        return false;
    }
    return true;
}

void write_fmt_pcm_payload(
    std::ostream& out,
    std::uint16_t channels,
    std::uint32_t sample_rate,
    std::uint16_t block_align,
    std::uint16_t bits,
    std::uint32_t channel_mask) {
    static constexpr std::uint8_t kPcmGuid[16] = {
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
        0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71
    };
    if (channel_mask) {
        write_u16(out, 0xFFFEu);
        write_u16(out, channels);
        write_le32(out, sample_rate);
        write_le32(out, sample_rate * block_align);
        write_u16(out, block_align);
        write_u16(out, bits);
        write_u16(out, 22u);
        write_u16(out, bits);
        write_le32(out, channel_mask);
        out.write(reinterpret_cast<const char*>(kPcmGuid), 16);
    } else {
        write_u16(out, 1u);
        write_u16(out, channels);
        write_le32(out, sample_rate);
        write_le32(out, sample_rate * block_align);
        write_u16(out, block_align);
        write_u16(out, bits);
    }
}

void write_fmt_pcm_riff(
    std::ostream& out,
    std::uint16_t channels,
    std::uint32_t sample_rate,
    std::uint16_t block_align,
    std::uint16_t bits,
    std::uint32_t channel_mask) {
    const std::uint32_t fmt_payload = channel_mask ? 40u : 16u;
    out.write("fmt ", 4);
    write_le32(out, fmt_payload);
    write_fmt_pcm_payload(out, channels, sample_rate, block_align, bits, channel_mask);
}

void write_fmt_pcm_w64(
    std::ostream& out,
    std::uint16_t channels,
    std::uint32_t sample_rate,
    std::uint16_t block_align,
    std::uint16_t bits,
    std::uint32_t channel_mask) {
    const std::uint64_t fmt_payload = channel_mask ? 40ull : 16ull;
    write_w64_chunk_header(out, kW64GuidFmt, fmt_payload);
    write_fmt_pcm_payload(out, channels, sample_rate, block_align, bits, channel_mask);
    const std::uint64_t padded = w64_pad8(fmt_payload);
    for (std::uint64_t i = fmt_payload; i < padded; ++i)
        out.put('\0');
}

bool finalize_container_sizes(
    std::ostream& out,
    bool use_rf64,
    bool use_w64,
    std::uint64_t ds64_payload_pos,
    std::uint64_t w64_riff_size_pos,
    std::string& error_out) {
    out.flush();
    out.seekp(0, std::ios::end);
    const auto end = out.tellp();
    if (end < 8) {
        error_out = "write failed";
        return false;
    }
    const std::uint64_t file_size = static_cast<std::uint64_t>(end);
    if (use_w64) {
        // Sony Wave64: riff size field is the total file size (includes header).
        out.seekp(static_cast<std::streamoff>(w64_riff_size_pos));
        write_le64(out, file_size);
        return static_cast<bool>(out);
    }
    const std::uint64_t riff_size = file_size - 8ull;
    if (use_rf64) {
        out.seekp(static_cast<std::streamoff>(ds64_payload_pos));
        write_le64(out, riff_size);
        return static_cast<bool>(out);
    }
    if (riff_size > 0xFFFFFFFFull) {
        error_out = "PCM output exceeds RIFF size limit";
        return false;
    }
    out.seekp(4, std::ios::beg);
    write_le32(out, static_cast<std::uint32_t>(riff_size));
    return static_cast<bool>(out);
}

bool write_pcm_riff_or_rf64(
    const std::string& path,
    uint32_t sample_rate,
    uint16_t channels,
    uint16_t bits,
    const std::vector<std::uint8_t>& interleaved_pcm,
    std::string& error_out,
    std::uint32_t channel_mask,
    const OutputMetadata& metadata) {
    const std::uint16_t bytes_per_sample = static_cast<std::uint16_t>(bits / 8u);
    const std::uint16_t block_align = static_cast<std::uint16_t>(channels * bytes_per_sample);
    const std::uint64_t data_size = interleaved_pcm.size();
    const std::uint64_t frame_count = data_size / block_align;
    const bool use_rf64 = data_size > kRiffSafeMaxDataBytes;

    std::ofstream out;
    if (!open_binary_trunc(out, path)) {
        error_out = "cannot open output file";
        return false;
    }

    std::uint64_t ds64_pos = 0;
    if (use_rf64) {
        out.write("RF64", 4);
        write_le32(out, 0xFFFFFFFFu);
        out.write("WAVE", 4);
        out.write("ds64", 4);
        write_le32(out, 28u);
        ds64_pos = static_cast<std::uint64_t>(out.tellp());
        write_le64(out, 0);
        write_le64(out, data_size);
        write_le64(out, frame_count);
        write_le32(out, 0);
        write_fmt_pcm_riff(out, channels, sample_rate, block_align, bits, channel_mask);
        out.write("data", 4);
        write_le32(out, 0xFFFFFFFFu);
    } else {
        const std::uint32_t fmt_payload = channel_mask ? 40u : 16u;
        const std::uint32_t header_after_riff = 4u + 8u + fmt_payload + 8u;
        out.write("RIFF", 4);
        write_le32(out, header_after_riff + static_cast<std::uint32_t>(data_size));
        out.write("WAVE", 4);
        write_fmt_pcm_riff(out, channels, sample_rate, block_align, bits, channel_mask);
        out.write("data", 4);
        write_le32(out, static_cast<std::uint32_t>(data_size));
    }

    if (!interleaved_pcm.empty())
        out.write(reinterpret_cast<const char*>(interleaved_pcm.data()),
                  static_cast<std::streamsize>(interleaved_pcm.size()));
    if (!write_list_info(out, metadata, error_out))
        return false;
    if (!finalize_container_sizes(out, use_rf64, false, ds64_pos, 0, error_out))
        return false;
    out.close();
    return !out.fail();
}

bool write_pcm_w64(
    const std::string& path,
    uint32_t sample_rate,
    uint16_t channels,
    uint16_t bits,
    const std::vector<std::uint8_t>& interleaved_pcm,
    std::string& error_out,
    std::uint32_t channel_mask) {
    const std::uint16_t bytes_per_sample = static_cast<std::uint16_t>(bits / 8u);
    const std::uint16_t block_align = static_cast<std::uint16_t>(channels * bytes_per_sample);
    const std::uint64_t data_size = interleaved_pcm.size();

    std::ofstream out;
    if (!open_binary_trunc(out, path)) {
        error_out = "cannot open output file";
        return false;
    }

    // riff GUID + size(placeholder) + wave GUID
    out.write(kW64GuidRiff, 16);
    const std::uint64_t riff_size_pos = static_cast<std::uint64_t>(out.tellp());
    write_le64(out, 0);
    out.write(kW64GuidWave, 16);

    write_fmt_pcm_w64(out, channels, sample_rate, block_align, bits, channel_mask);

    write_w64_chunk_header(out, kW64GuidData, data_size);
    if (!interleaved_pcm.empty())
        out.write(reinterpret_cast<const char*>(interleaved_pcm.data()),
                  static_cast<std::streamsize>(interleaved_pcm.size()));
    const std::uint64_t data_padded = w64_pad8(data_size);
    for (std::uint64_t i = data_size; i < data_padded; ++i)
        out.put('\0');

    // No LIST/INFO in W64 path (editors care about PCM; RF64/WAV keep tags).
    if (!finalize_container_sizes(out, false, true, 0, riff_size_pos, error_out))
        return false;
    out.close();
    return !out.fail();
}

} // namespace

bool write_pcm16_le(
    const std::string& path,
    uint32_t sample_rate,
    uint16_t channels,
    const std::vector<std::uint8_t>& interleaved_pcm,
    std::string& error_out,
    std::uint32_t channel_mask,
    const OutputMetadata& metadata,
    PcmContainer container) {
    if (channels == 0 || sample_rate == 0) {
        error_out = "invalid channels or sample_rate";
        return false;
    }
    if (interleaved_pcm.size() % (channels * 2) != 0) {
        error_out = "PCM size not aligned to frame";
        return false;
    }
    if (container == PcmContainer::W64)
        return write_pcm_w64(path, sample_rate, channels, 16, interleaved_pcm, error_out, channel_mask);
    return write_pcm_riff_or_rf64(
        path, sample_rate, channels, 16, interleaved_pcm, error_out, channel_mask, metadata);
}

bool write_pcm24_le(
    const std::string& path,
    uint32_t sample_rate,
    uint16_t channels,
    const std::vector<std::uint8_t>& interleaved_pcm,
    std::string& error_out,
    std::uint32_t channel_mask,
    const OutputMetadata& metadata,
    PcmContainer container) {
    if (channels == 0 || sample_rate == 0) {
        error_out = "invalid channels or sample_rate";
        return false;
    }
    if (interleaved_pcm.size() % (channels * 3) != 0) {
        error_out = "PCM size not aligned to frame";
        return false;
    }

    PcmStreamWriter writer;
    writer.set_metadata(metadata);
    const std::uint64_t frame_count =
        interleaved_pcm.size() / (static_cast<std::size_t>(channels) * 3u);
    if (!writer.open(path, sample_rate, channels, frame_count, error_out, channel_mask, container))
        return false;
    if (!writer.write(interleaved_pcm, error_out))
        return false;
    return writer.close(error_out);
}

bool encode_wav_to_flac(
    const std::string& wav_path,
    const std::string& flac_path,
    std::string& error_out,
    const OutputMetadata& metadata) {
    std::string command = "ffmpeg -y -v error -i " + shell_quote(wav_path);
    if (!metadata.comment.empty())
        command += " -metadata comment=" + shell_quote(metadata.comment);
    command += " -map 0:a:0 -c:a flac -f flac " + shell_quote(flac_path);
    if (std::system(command.c_str()) != 0) {
        std::error_code remove_error;
        std::filesystem::remove(std::filesystem::u8path(flac_path), remove_error);
        error_out = "ffmpeg failed to encode FLAC (is ffmpeg available in PATH?)";
        return false;
    }
    return true;
}

bool convert_pcm24_to_pcm16(
    const std::vector<std::uint8_t>& pcm24,
    std::vector<std::uint8_t>& pcm16,
    std::string& error_out) {
    if (pcm24.size() % 3u != 0u) {
        error_out = "PCM24 byte count is not sample-aligned";
        return false;
    }
    pcm16.clear();
    pcm16.reserve((pcm24.size() / 3u) * 2u);
    for (std::size_t offset = 0; offset < pcm24.size(); offset += 3u) {
        std::int32_t sample = static_cast<std::int32_t>(pcm24[offset])
            | (static_cast<std::int32_t>(pcm24[offset + 1u]) << 8)
            | (static_cast<std::int32_t>(pcm24[offset + 2u]) << 16);
        if ((sample & 0x800000) != 0)
            sample |= ~0xFFFFFF;
        const std::int16_t sample16 = static_cast<std::int16_t>(sample / 256);
        const std::uint16_t packed = static_cast<std::uint16_t>(sample16);
        pcm16.push_back(static_cast<std::uint8_t>(packed));
        pcm16.push_back(static_cast<std::uint8_t>(packed >> 8));
    }
    return true;
}

bool PcmStreamWriter::open(
    const std::string& path,
    std::uint32_t sample_rate,
    std::uint16_t channels,
    std::uint64_t frame_count,
    std::string& error_out,
    std::uint32_t channel_mask,
    PcmContainer container,
    unsigned bits_per_sample) {
    if (!channels || !sample_rate || (bits_per_sample != 16u && bits_per_sample != 24u)) {
        error_out = "invalid channels, sample_rate, or PCM bit depth";
        return false;
    }
    bits_per_sample_ = bits_per_sample;
    block_align_ = static_cast<std::uint16_t>(channels * (bits_per_sample_ / 8u));
    frame_count_ = frame_count;
    expected_bytes_ = frame_count * static_cast<std::uint64_t>(block_align_);
    written_bytes_ = 0;
    use_w64_ = container == PcmContainer::W64;
    use_rf64_ = !use_w64_ && expected_bytes_ > kRiffSafeMaxDataBytes;
    ds64_chunk_pos_ = 0;
    w64_riff_size_pos_ = 0;

    out_.close();
    if (!open_binary_trunc(out_, path)) {
        error_out = "cannot open output file";
        return false;
    }

    if (use_w64_) {
        out_.write(kW64GuidRiff, 16);
        w64_riff_size_pos_ = static_cast<std::uint64_t>(out_.tellp());
        write_le64(out_, 0);
        out_.write(kW64GuidWave, 16);
        write_fmt_pcm_w64(out_, channels, sample_rate, block_align_, bits_per_sample_, channel_mask);
        write_w64_chunk_header(out_, kW64GuidData, expected_bytes_);
    } else if (use_rf64_) {
        out_.write("RF64", 4);
        write_le32(out_, 0xFFFFFFFFu);
        out_.write("WAVE", 4);
        out_.write("ds64", 4);
        write_le32(out_, 28u);
        ds64_chunk_pos_ = static_cast<std::uint64_t>(out_.tellp());
        write_le64(out_, 0);
        write_le64(out_, expected_bytes_);
        write_le64(out_, frame_count_);
        write_le32(out_, 0);
        write_fmt_pcm_riff(out_, channels, sample_rate, block_align_, bits_per_sample_, channel_mask);
        out_.write("data", 4);
        write_le32(out_, 0xFFFFFFFFu);
    } else {
        const std::uint32_t fmt_payload = channel_mask ? 40u : 16u;
        const std::uint32_t header_after_riff = 4u + 8u + fmt_payload + 8u;
        out_.write("RIFF", 4);
        write_le32(out_, header_after_riff + static_cast<std::uint32_t>(expected_bytes_));
        out_.write("WAVE", 4);
        write_fmt_pcm_riff(out_, channels, sample_rate, block_align_, bits_per_sample_, channel_mask);
        out_.write("data", 4);
        write_le32(out_, static_cast<std::uint32_t>(expected_bytes_));
    }
    if (!out_) {
        error_out = "write failed";
        return false;
    }
    return true;
}

bool PcmStreamWriter::write(
    const std::vector<std::uint8_t>& interleaved_pcm,
    std::string& error_out) {
    if (!out_.is_open() || !block_align_ || interleaved_pcm.size() % block_align_ != 0
        || written_bytes_ + interleaved_pcm.size() > expected_bytes_) {
        error_out = "PCM stream write is not frame-aligned";
        return false;
    }
    if (!interleaved_pcm.empty())
        out_.write(reinterpret_cast<const char*>(interleaved_pcm.data()),
                   static_cast<std::streamsize>(interleaved_pcm.size()));
    if (!out_) {
        error_out = "write failed";
        return false;
    }
    written_bytes_ += interleaved_pcm.size();
    return true;
}

bool PcmStreamWriter::close(std::string& error_out) {
    if (!out_.is_open())
        return true;
    if (written_bytes_ != expected_bytes_) {
        error_out = "PCM stream ended before the declared frame count";
        out_.close();
        return false;
    }
    if (use_w64_) {
        const std::uint64_t padded = w64_pad8(expected_bytes_);
        for (std::uint64_t i = expected_bytes_; i < padded; ++i)
            out_.put('\0');
    } else if (!write_list_info(out_, metadata_, error_out)) {
        out_.close();
        return false;
    }
    if (!finalize_container_sizes(
            out_, use_rf64_, use_w64_, ds64_chunk_pos_, w64_riff_size_pos_, error_out)) {
        out_.close();
        return false;
    }
    out_.close();
    if (out_.fail()) {
        error_out = "write failed";
        return false;
    }
    return true;
}

} // namespace wav
