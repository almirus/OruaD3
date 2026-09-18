#include "crc.hpp"

#include <array>
#include <limits>

namespace auro3d::encode {
namespace {

const std::array<std::uint16_t, 256>& crc_table() {
    static const std::array<std::uint16_t, 256> table = [] {
        std::array<std::uint16_t, 256> result{};
        for (std::uint32_t i = 0; i != result.size(); ++i) {
            std::uint16_t value = static_cast<std::uint16_t>(i << 8u);
            for (std::uint32_t bit = 0; bit != 8u; ++bit) {
                value = (value & 0x8000u) != 0u
                    ? static_cast<std::uint16_t>((value << 1u) ^ 0x1021u)
                    : static_cast<std::uint16_t>(value << 1u);
            }
            result[i] = static_cast<std::uint16_t>((value << 8u) | (value >> 8u));
        }
        return result;
    }();
    return table;
}

} // namespace

void Crc16::reset() {
    value_ = 0;
    position_ = 0;
}

void Crc16::process_words(const std::uint32_t* words, std::size_t word_count) {
    if (words == nullptr || word_count == 0u)
        return;
    const auto& table = crc_table();
    for (std::size_t index = 0; index < word_count; ++index) {
        const std::uint32_t word = words[index];
        const std::uint64_t absolute_position =
            static_cast<std::uint64_t>(position_) + index;
        const std::uint32_t mask = 253u + 2u * static_cast<std::uint32_t>(
            absolute_position >= 0x10u);
        const std::uint16_t step0 = table[((word & mask) ^ static_cast<std::uint8_t>(value_)) & 0xFFu];
        const std::uint16_t step1 = table[static_cast<std::uint8_t>(
            static_cast<std::uint8_t>(step0)
            ^ static_cast<std::uint8_t>(value_ >> 8u)
            ^ static_cast<std::uint8_t>(word >> 8u))];
        value_ = static_cast<std::uint16_t>(
            table[static_cast<std::uint8_t>(
                static_cast<std::uint8_t>(step1)
                ^ static_cast<std::uint8_t>(step0 >> 8u)
                ^ static_cast<std::uint8_t>(word >> 16u))]
            ^ static_cast<std::uint16_t>(step1 >> 8u));
    }
    if (word_count > std::numeric_limits<std::uint32_t>::max() - position_) {
        // Native state is uint32. A valid codec frame cannot approach this
        // bound; retaining saturation prevents a wrap from fabricating CRC.
        position_ = std::numeric_limits<std::uint32_t>::max();
    } else {
        position_ += static_cast<std::uint32_t>(word_count);
    }
}

std::uint16_t Crc16::stored_word() const {
    const std::uint16_t inverted = static_cast<std::uint16_t>(~value_);
    return static_cast<std::uint16_t>((inverted << 8u) | (inverted >> 8u));
}

} // namespace auro3d:encode
