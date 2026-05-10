#include "wav_writer.hpp"

#include <fstream>

namespace wav {

namespace {

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
#pragma pack(pop)

} // namespace

bool write_pcm16_le(
    const std::string& path,
    uint32_t sample_rate,
    uint16_t channels,
    const std::vector<std::uint8_t>& interleaved_pcm,
    std::string& error_out) {
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
    if (!out) {
        error_out = "write failed";
        return false;
    }
    return true;
}

bool write_pcm24_le(
    const std::string& path,
    uint32_t sample_rate,
    uint16_t channels,
    const std::vector<std::uint8_t>& interleaved_pcm,
    std::string& error_out) {
    if (channels == 0 || sample_rate == 0) {
        error_out = "invalid channels or sample_rate";
        return false;
    }
    if (interleaved_pcm.size() % (channels * 3) != 0) {
        error_out = "PCM size not aligned to frame";
        return false;
    }

    const std::uint32_t data_size = static_cast<std::uint32_t>(interleaved_pcm.size());
    const std::uint16_t bits = 24;
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
    if (!out) {
        error_out = "write failed";
        return false;
    }
    return true;
}

} // namespace wav
