#include "channel_bit_writer.hpp"

#include <limits>

namespace auro3d::encode {

bool codec_v3_channel_payload_capacity(
    std::uint32_t bitlines,
    std::uint32_t word_count,
    std::uint64_t& payload_bits) {
    payload_bits = 0;
    // Native Projector templates exist for N=3..16. Channel::initialize's
    // set_mix_bw also rejects headroom below 10 (bit_line above 14).
    if (bitlines < 3u || bitlines > 16u)
        return false;
    for (std::uint32_t word_index = 0; word_index < word_count; ++word_index) {
        const std::uint32_t reserved = word_index < 16u
            ? 3u
            : ((word_index & 15u) == 0u ? 1u : 0u);
        if (bitlines <= reserved)
            continue;
        const std::uint64_t available = bitlines - reserved;
        if (payload_bits > std::numeric_limits<std::uint64_t>::max() - available)
            return false;
        payload_bits += available;
    }
    return true;
}

bool ChannelBitWriter::reset(std::uint32_t bitlines) {
    words_.clear();
    bitlines_ = 0;
    remaining_in_word_ = 0;
    bit_count_ = 0;
    if (bitlines < 3u || bitlines > 16u)
        return false;
    bitlines_ = bitlines;
    return true;
}

std::uint32_t ChannelBitWriter::reserved_bits() const {
    const std::uint32_t word_index = static_cast<std::uint32_t>(words_.size() - 1u);
    if (word_index < 16u)
        return 3u;
    return (word_index & 15u) == 0u ? 1u : 0u;
}

std::uint64_t ChannelBitWriter::payload_capacity_bits() const {
    std::uint64_t capacity = 0u;
    if (!codec_v3_channel_payload_capacity(bitlines_, word_count(), capacity))
        return 0u;
    return capacity;
}

std::uint64_t ChannelBitWriter::remaining_payload_bits() const {
    const std::uint64_t capacity = payload_capacity_bits();
    return bit_count_ >= capacity ? 0u : capacity - bit_count_;
}

bool ChannelBitWriter::begin_word() {
    if (bitlines_ == 0u || words_.size() >= words_.max_size())
        return false;
    words_.push_back(0u);
    const std::uint32_t reserved = reserved_bits();
    // First sixteen words reserve three low bits for the frame CRC/sync
    // geometry. When N<=3 those words carry no payload and must still be
    // allocated so later words become reachable, matching Projector N=3.
    if (bitlines_ <= reserved) {
        remaining_in_word_ = 0u;
        return true;
    }
    remaining_in_word_ = bitlines_;
    return true;
}

bool ChannelBitWriter::write_unsigned(std::uint32_t value, std::uint32_t bit_count) {
    if (bitlines_ == 0u || bit_count > 32u ||
        (bit_count != 32u && (value >> bit_count) != 0u) ||
        bit_count > std::numeric_limits<std::uint64_t>::max() - bit_count_) {
        return false;
    }
    std::uint32_t remaining = bit_count;
    while (remaining != 0u) {
        if (words_.empty()
            || remaining_in_word_ == 0u
            || remaining_in_word_ <= reserved_bits()) {
            if (!begin_word())
                return false;
            if (remaining_in_word_ == 0u
                || remaining_in_word_ <= reserved_bits())
                continue;
        }
        const std::uint32_t reserved = reserved_bits();
        const std::uint32_t capacity = remaining_in_word_ - reserved;
        const std::uint32_t take = remaining < capacity ? remaining : capacity;
        const std::uint32_t shift_from_value = remaining - take;
        const std::uint32_t part = take == 32u
            ? value
            : (value >> shift_from_value) & ((std::uint32_t{1} << take) - 1u);
        words_.back() |= part << (remaining_in_word_ - take);
        remaining_in_word_ -= take;
        remaining -= take;
        bit_count_ += take;
    }
    return true;
}

bool ChannelBitWriter::write_signed(std::int32_t value, std::uint32_t bit_count) {
    if (bit_count == 0u || bit_count > 32u)
        return false;
    if (bit_count != 32u) {
        const std::int64_t minimum = -(std::int64_t{1} << (bit_count - 1u));
        const std::int64_t maximum = (std::int64_t{1} << (bit_count - 1u)) - 1;
        if (value < minimum || value > maximum)
            return false;
    }
    const std::uint32_t raw = static_cast<std::uint32_t>(value);
    const std::uint32_t masked = bit_count == 32u
        ? raw
        : raw & ((std::uint32_t{1} << bit_count) - 1u);
    return write_unsigned(masked, bit_count);
}

} // namespace auro3d::encode
