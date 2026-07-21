#include "wav_writer.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace wav {
namespace {

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
    append_chunk("IKEY", metadata.channel_names);
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

void write_fmt_pcm(
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
    out.write("fmt ", 4);
    if (channel_mask) {
        write_le32(out, 40u);
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
        write_le32(out, 16u);
        write_u16(out, 1u);
        write_u16(out, channels);
        write_le32(out, sample_rate);
        write_le32(out, sample_rate * block_align);
        write_u16(out, block_align);
        write_u16(out, bits);
    }
}

bool finalize_container_sizes(
    std::ostream& out,
    bool use_rf64,
    std::uint64_t ds64_payload_pos,
    std::string& error_out) {
    out.flush();
    out.seekp(0, std::ios::end);
    const auto end = out.tellp();
    if (end < 8) {
        error_out = "write failed";
        return false;
    }
    const std::uint64_t riff_size = static_cast<std::uint64_t>(end) - 8ull;
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

} // namespace

bool write_pcm16_le(
    const std::string& path,
    uint32_t sample_rate,
    uint16_t channels,
    const std::vector<std::uint8_t>& interleaved_pcm,
    std::string& error_out,
    std::uint32_t channel_mask,
    const OutputMetadata& metadata) {
    if (channels == 0 || sample_rate == 0) {
        error_out = "invalid channels or sample_rate";
        return false;
    }
    if (interleaved_pcm.size() % (channels * 2) != 0) {
        error_out = "PCM size not aligned to frame";
        return false;
    }

    const std::uint16_t bits = 16;
    const std::uint16_t block_align = static_cast<std::uint16_t>(channels * 2u);
    const std::uint64_t data_size = interleaved_pcm.size();
    const std::uint64_t frame_count = data_size / block_align;
    const bool use_rf64 = data_size > kRiffSafeMaxDataBytes;

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
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
        write_fmt_pcm(out, channels, sample_rate, block_align, bits, channel_mask);
        out.write("data", 4);
        write_le32(out, 0xFFFFFFFFu);
    } else {
        const std::uint32_t fmt_payload = channel_mask ? 40u : 16u;
        const std::uint32_t header_after_riff = 4u + 8u + fmt_payload + 8u;
        out.write("RIFF", 4);
        write_le32(out, header_after_riff + static_cast<std::uint32_t>(data_size));
        out.write("WAVE", 4);
        write_fmt_pcm(out, channels, sample_rate, block_align, bits, channel_mask);
        out.write("data", 4);
        write_le32(out, static_cast<std::uint32_t>(data_size));
    }

    if (!interleaved_pcm.empty())
        out.write(reinterpret_cast<const char*>(interleaved_pcm.data()),
                  static_cast<std::streamsize>(interleaved_pcm.size()));
    if (!write_list_info(out, metadata, error_out))
        return false;
    if (!finalize_container_sizes(out, use_rf64, ds64_pos, error_out))
        return false;
    out.close();
    return !out.fail();
}

bool write_pcm24_le(
    const std::string& path,
    uint32_t sample_rate,
    uint16_t channels,
    const std::vector<std::uint8_t>& interleaved_pcm,
    std::string& error_out,
    std::uint32_t channel_mask,
    const OutputMetadata& metadata) {
    if (channels == 0 || sample_rate == 0) {
        error_out = "invalid channels or sample_rate";
        return false;
    }
    if (interleaved_pcm.size() % (channels * 3) != 0) {
        error_out = "PCM size not aligned to frame";
        return false;
    }

    Pcm24StreamWriter writer;
    writer.set_metadata(metadata);
    const std::uint64_t frame_count =
        interleaved_pcm.size() / (static_cast<std::size_t>(channels) * 3u);
    if (!writer.open(path, sample_rate, channels, frame_count, error_out, channel_mask))
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
    if (!metadata.channel_names.empty())
        command += " -metadata keywords=" + shell_quote(metadata.channel_names);
    command += " -map 0:a:0 -c:a flac " + shell_quote(flac_path);
    if (std::system(command.c_str()) != 0) {
        error_out = "ffmpeg failed to encode FLAC (is ffmpeg available in PATH?)";
        return false;
    }
    return true;
}

bool Pcm24StreamWriter::open(
    const std::string& path,
    std::uint32_t sample_rate,
    std::uint16_t channels,
    std::uint64_t frame_count,
    std::string& error_out,
    std::uint32_t channel_mask) {
    if (!channels || !sample_rate) {
        error_out = "invalid channels or sample_rate";
        return false;
    }
    block_align_ = static_cast<std::uint16_t>(channels * 3u);
    frame_count_ = frame_count;
    expected_bytes_ = frame_count * static_cast<std::uint64_t>(block_align_);
    written_bytes_ = 0;
    use_rf64_ = expected_bytes_ > kRiffSafeMaxDataBytes;

    out_.open(path, std::ios::binary | std::ios::trunc);
    if (!out_) {
        error_out = "cannot open output file";
        return false;
    }

    if (use_rf64_) {
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
        write_fmt_pcm(out_, channels, sample_rate, block_align_, 24, channel_mask);
        out_.write("data", 4);
        write_le32(out_, 0xFFFFFFFFu);
    } else {
        const std::uint32_t fmt_payload = channel_mask ? 40u : 16u;
        const std::uint32_t header_after_riff = 4u + 8u + fmt_payload + 8u; // WAVE + fmt + data hdr
        out_.write("RIFF", 4);
        write_le32(out_, header_after_riff + static_cast<std::uint32_t>(expected_bytes_));
        out_.write("WAVE", 4);
        write_fmt_pcm(out_, channels, sample_rate, block_align_, 24, channel_mask);
        out_.write("data", 4);
        write_le32(out_, static_cast<std::uint32_t>(expected_bytes_));
    }
    if (!out_) {
        error_out = "write failed";
        return false;
    }
    return true;
}

bool Pcm24StreamWriter::write(
    const std::vector<std::uint8_t>& interleaved_pcm,
    std::string& error_out) {
    if (!out_.is_open() || !block_align_ || interleaved_pcm.size() % block_align_ != 0
        || written_bytes_ + interleaved_pcm.size() > expected_bytes_) {
        error_out = "PCM24 stream write is not frame-aligned";
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

bool Pcm24StreamWriter::close(std::string& error_out) {
    if (!out_.is_open())
        return true;
    if (written_bytes_ != expected_bytes_) {
        error_out = "PCM24 stream ended before the declared frame count";
        out_.close();
        return false;
    }
    if (!write_list_info(out_, metadata_, error_out)) {
        out_.close();
        return false;
    }
    if (!finalize_container_sizes(out_, use_rf64_, ds64_chunk_pos_, error_out)) {
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
