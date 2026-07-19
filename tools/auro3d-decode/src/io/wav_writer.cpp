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

bool write_list_info(std::ostream& out, const OutputMetadata& metadata, std::string& error_out) {
    if (metadata.empty())
        return true;
    // LIST size = 4 ("INFO") + sum of padded subchunks; computed after building payload.
    std::string payload;
    {
        // Build into a temporary stream buffer via string assembly.
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
        // comment -> ICMT, channel list -> IKEY (keywords)
        append_chunk("ICMT", metadata.comment);
        append_chunk("IKEY", metadata.channel_names);
        payload.assign(buf.begin(), buf.end());
    }
    if (payload.empty())
        return true;

    out.write("LIST", 4);
    write_le32(out, static_cast<std::uint32_t>(4u + payload.size()));
    out.write("INFO", 4);
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    if (!out) {
        error_out = "failed to write WAV LIST/INFO metadata";
        return false;
    }
    return true;
}

bool patch_riff_size(std::fstream& file, std::string& error_out) {
    file.seekp(0, std::ios::end);
    const auto end = file.tellp();
    if (end < 8) {
        error_out = "WAV file too small to patch RIFF size";
        return false;
    }
    const auto riff_size = static_cast<std::uint32_t>(static_cast<std::uint64_t>(end) - 8ull);
    file.seekp(4, std::ios::beg);
    write_le32(file, riff_size);
    if (!file) {
        error_out = "failed to patch WAV RIFF size";
        return false;
    }
    return true;
}

#pragma pack(push, 1)
struct WavHeader {
    char riff[4];       // "RIFF"
    std::uint32_t size; // file size - 8
    char wave[4];       // "WAVE"
    char fmt_[4];       // "fmt "
    std::uint32_t fmt_size;
    std::uint16_t audio_format; // 1 = PCM
    std::uint16_t num_channels;
    std::uint32_t sample_rate;
    std::uint32_t byte_rate;
    std::uint16_t block_align;
    std::uint16_t bits_per_sample;
    char data[4]; // "data"
    std::uint32_t data_size;
};

struct WavExtensibleHeader {
    char riff[4];
    std::uint32_t size;
    char wave[4];
    char fmt_[4];
    std::uint32_t fmt_size;
    std::uint16_t audio_format;
    std::uint16_t num_channels;
    std::uint32_t sample_rate;
    std::uint32_t byte_rate;
    std::uint16_t block_align;
    std::uint16_t bits_per_sample;
    std::uint16_t extension_size;
    std::uint16_t valid_bits_per_sample;
    std::uint32_t channel_mask;
    std::uint8_t sub_format[16];
    char data[4];
    std::uint32_t data_size;
};
#pragma pack(pop)

static_assert(sizeof(WavHeader) == 44, "unexpected PCM WAV header size");
static_assert(sizeof(WavExtensibleHeader) == 68, "unexpected extensible WAV header size");

} // namespace

bool write_pcm16_le(
    const std::string& path,
    uint32_t sample_rate,
    uint16_t channels,
    const std::vector<std::uint8_t>& interleaved_pcm,
    std::string& error_out,
    const OutputMetadata& metadata) {
    if (channels == 0 || sample_rate == 0) {
        error_out = "invalid channels or sample_rate";
        return false;
    }
    if (interleaved_pcm.size() % (channels * 2) != 0) {
        error_out = "PCM size not aligned to frame";
        return false;
    }

    const std::uint32_t data_size = static_cast<std::uint32_t>(interleaved_pcm.size());
    const std::uint16_t bits = 16;
    const std::uint16_t block_align = static_cast<std::uint16_t>(channels * (bits / 8));
    const std::uint32_t byte_rate = sample_rate * block_align;

    WavHeader h{};
    h.riff[0] = 'R';
    h.riff[1] = 'I';
    h.riff[2] = 'F';
    h.riff[3] = 'F';
    h.size = 36 + data_size;
    h.wave[0] = 'W';
    h.wave[1] = 'A';
    h.wave[2] = 'V';
    h.wave[3] = 'E';
    h.fmt_[0] = 'f';
    h.fmt_[1] = 'm';
    h.fmt_[2] = 't';
    h.fmt_[3] = ' ';
    h.fmt_size = 16;
    h.audio_format = 1;
    h.num_channels = channels;
    h.sample_rate = sample_rate;
    h.byte_rate = byte_rate;
    h.block_align = block_align;
    h.bits_per_sample = bits;
    h.data[0] = 'd';
    h.data[1] = 'a';
    h.data[2] = 't';
    h.data[3] = 'a';
    h.data_size = data_size;

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        error_out = "cannot open output file";
        return false;
    }
    out.write(reinterpret_cast<const char*>(&h), sizeof(h));
    if (!interleaved_pcm.empty())
        out.write(reinterpret_cast<const char*>(interleaved_pcm.data()),
                  static_cast<std::streamsize>(interleaved_pcm.size()));
    if (!write_list_info(out, metadata, error_out))
        return false;
    if (!out) {
        error_out = "write failed";
        return false;
    }
    out.close();
    if (!metadata.empty()) {
        std::fstream patch(path, std::ios::binary | std::ios::in | std::ios::out);
        if (!patch || !patch_riff_size(patch, error_out))
            return false;
    }
    return true;
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
    const std::uint64_t frame_count = interleaved_pcm.size() /
        (static_cast<std::size_t>(channels) * 3u);
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
    expected_bytes_ = frame_count * block_align_;
    written_bytes_ = 0;
    const std::uint32_t riff_overhead = channel_mask ? 60u : 36u;
    if (expected_bytes_ > 0xFFFFFFFFull - riff_overhead) {
        error_out = "PCM24 output exceeds RIFF size limit";
        return false;
    }

    out_.open(path, std::ios::binary | std::ios::trunc);
    if (!out_) {
        error_out = "cannot open output file";
        return false;
    }
    if (channel_mask) {
        WavExtensibleHeader h{};
        h.riff[0] = 'R'; h.riff[1] = 'I'; h.riff[2] = 'F'; h.riff[3] = 'F';
        h.size = riff_overhead + static_cast<std::uint32_t>(expected_bytes_);
        h.wave[0] = 'W'; h.wave[1] = 'A'; h.wave[2] = 'V'; h.wave[3] = 'E';
        h.fmt_[0] = 'f'; h.fmt_[1] = 'm'; h.fmt_[2] = 't'; h.fmt_[3] = ' ';
        h.fmt_size = 40;
        h.audio_format = 0xFFFEu;
        h.num_channels = channels;
        h.sample_rate = sample_rate;
        h.byte_rate = sample_rate * block_align_;
        h.block_align = block_align_;
        h.bits_per_sample = 24;
        h.extension_size = 22;
        h.valid_bits_per_sample = 24;
        h.channel_mask = channel_mask;
        const std::uint8_t pcm_guid[16] = {
            0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
            0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71
        };
        std::copy(std::begin(pcm_guid), std::end(pcm_guid), h.sub_format);
        h.data[0] = 'd'; h.data[1] = 'a'; h.data[2] = 't'; h.data[3] = 'a';
        h.data_size = static_cast<std::uint32_t>(expected_bytes_);
        out_.write(reinterpret_cast<const char*>(&h), sizeof(h));
    } else {
        WavHeader h{};
        h.riff[0] = 'R'; h.riff[1] = 'I'; h.riff[2] = 'F'; h.riff[3] = 'F';
        h.size = riff_overhead + static_cast<std::uint32_t>(expected_bytes_);
        h.wave[0] = 'W'; h.wave[1] = 'A'; h.wave[2] = 'V'; h.wave[3] = 'E';
        h.fmt_[0] = 'f'; h.fmt_[1] = 'm'; h.fmt_[2] = 't'; h.fmt_[3] = ' ';
        h.fmt_size = 16;
        h.audio_format = 1;
        h.num_channels = channels;
        h.sample_rate = sample_rate;
        h.byte_rate = sample_rate * block_align_;
        h.block_align = block_align_;
        h.bits_per_sample = 24;
        h.data[0] = 'd'; h.data[1] = 'a'; h.data[2] = 't'; h.data[3] = 'a';
        h.data_size = static_cast<std::uint32_t>(expected_bytes_);
        out_.write(reinterpret_cast<const char*>(&h), sizeof(h));
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
    if (!out_.is_open() || !block_align_ || interleaved_pcm.size() % block_align_ != 0 ||
        written_bytes_ + interleaved_pcm.size() > expected_bytes_) {
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
    out_.flush();
    // Patch RIFF chunk size now that LIST/INFO may have been appended.
    out_.seekp(0, std::ios::end);
    const auto end = out_.tellp();
    if (end >= 8) {
        const auto riff_size = static_cast<std::uint32_t>(static_cast<std::uint64_t>(end) - 8ull);
        out_.seekp(4, std::ios::beg);
        write_le32(out_, riff_size);
    }
    out_.close();
    if (out_.fail()) {
        error_out = "write failed";
        return false;
    }
    return true;
}

} // namespace wav
