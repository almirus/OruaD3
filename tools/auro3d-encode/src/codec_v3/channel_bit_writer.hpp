#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace auro3d::encode {

/// Exact sum of payload bits visible to channel_bit_reader_get_unsigned_bits
/// across ount codec words. Reserved low bits are excluded.
/// bitlines is the native Projector N BitReader width: equal to the group
/// bit_line (and to 24 - headroom from the channel metadata field).
bool codec_v3_channel_payload_capacity(
    std::uint32_t bitlines,
    std::uint32_t word_count,
    std::uint64_t& payload_bits);

/// Maps group bit_line Channel mux bandwidth onto native BitReader
/// bitlines. Decoder uses 24 - headroom with headroom stored as
/// 24 - bit_line, which simplifies to bit_line itself. Accepted range is
/// the native Projector template span 3..16; Channel:initialize also requires
/// headroom 24 - bit_line >= 10 (bit_line <= 14) via set_mix_bw.
bool codec_v3_channel_bitlines_from_quant_shift(
    std::uint32_t quant_shift,
    std::uint32_t& bitlines);

/// Inverse of channel_bit_reader_get_unsigned_bits. Codec-v3 channel words
/// are consumed MSB-first from their low bitlines bits; initial words reserve
/// three low bits and later words follow the decoder's 16-word reservation
/// cadence. This is distinct from the generic LsbBitWriter used by LDC.
class ChannelBitWriter {
public:
    bool reset(std::uint32_t bitlines);
    bool write_unsigned(std::uint32_t value, std::uint32_t bit_count);
    bool write_signed(std::int32_t value, std::uint32_t bit_count);

    std::uint32_t bitlines() const { return bitlines_; }
    std::uint64_t bit_count() const { return bit_count_; }
    std::uint64_t payload_capacity_bits() const;
    std::uint64_t remaining_payload_bits() const;
    std::uint32_t word_count() const { return static_cast<std::uint32_t>(words_.size()); }
    const std::vector<std::uint32_t>& words() const { return words_; }

private:
    bool begin_word();
    std::uint32_t reserved_bits() const;

    std::vector<std::uint32_t> words_;
    std::uint32_t bitlines_ = 0;
    std::uint32_t remaining_in_word_ = 0;
    std::uint64_t bit_count_ = 0;
};

} // namespace auro3d:encode
