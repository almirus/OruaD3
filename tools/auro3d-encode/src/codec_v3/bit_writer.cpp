#include "bit_writer.hpp"

#include <limits>

namespace auro3d::encode {

bool LsbBitWriter::reserve_bits(std::uint64_t additional_bits) {
    if (additional_bits > std::numeric_limits<std::uint64_t>::max() - bit_count_)
        return false;
    const std::uint64_t total_bits = bit_count_ + additional_bits;
    const std::uint64_t required_bytes = total_bits / 8u + (total_bits % 8u != 0u ? 1u : 0u);
    if (required_bytes > static_cast<std::uint64_t>(bytes_.max_size()))
        return false;
    bytes_.resize(static_cast<std::size_t>(required_bytes), 0u);
    return true;
}

bool LsbBitWriter::write_bits(std::uint64_t value, std::uint32_t bit_count) {
    if (bit_count > 64u || !reserve_bits(bit_count))
        return false;

    // Exact bit direction from Writer<VectorStorage>::write_<unsigned long>
    // at 0x336980: OR low value bits at the current intra-byte bit offset,
    // then shift value right by the number of emitted bits.
    std::uint32_t remaining = bit_count;
    while (remaining != 0u) {
        const std::uint32_t byte_offset = static_cast<std::uint32_t>(bit_count_ & 7u);
        const std::uint32_t available = 8u - byte_offset;
        const std::uint32_t take = remaining < available ? remaining : available;
        const std::uint8_t mask = static_cast<std::uint8_t>((1u << take) - 1u);
        bytes_[static_cast<std::size_t>(bit_count_ >> 3u)] |=
            static_cast<std::uint8_t>((static_cast<std::uint8_t>(value) & mask) << byte_offset);
        value >>= take;
        bit_count_ += take;
        remaining -= take;
    }
    return true;
}

bool LsbBitWriter::write_zeroes(std::uint64_t bit_count) {
    if (!reserve_bits(bit_count))
        return false;
    bit_count_ += bit_count;
    return true;
}

bool LsbBitWriter::write_one() {
    return write_bits(1u, 1u);
}

void LsbBitWriter::clear() {
    bytes_.clear();
    bit_count_ = 0;
}

} // namespace auro3d::encode
