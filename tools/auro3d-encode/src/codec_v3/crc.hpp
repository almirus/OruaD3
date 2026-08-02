#pragma once

#include <cstddef>
#include <cstdint>

namespace auro3d::encode {

/// Codec-v3 CRC word processor. The table and update order are a direct port
/// of auro_codec_v3_decoder_CRC_process @ 0x52ECA0; encoder serialization must
/// feed it the exact same projected word sequence as the native encoder.
class Crc16 {
public:
    void reset();
    void process_words(const std::uint32_t* words, std::size_t word_count);
    std::uint32_t processed_words() const { return position_; }
    std::uint16_t raw() const { return value_; }
    std::uint16_t stored_word() const;

private:
    std::uint16_t value_ = 0;
    std::uint32_t position_ = 0;
};

} // namespace auro3d::encode
