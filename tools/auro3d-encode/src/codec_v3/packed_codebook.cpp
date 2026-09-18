#include "packed_codebook.hpp"

#include "error_quantization.hpp"

#include <limits>

namespace auro3d::encode {
namespace {

std::uint32_t width_mask(std::uint32_t bit_width) {
    return bit_width == 32u ? 0xFFFFFFFFu : (std::uint32_t{1} << bit_width) - 1u;
}

} // namespace

bool pack_golomb_codebook(
    const std::vector<std::uint32_t>& values,
    std::uint32_t bit_width,
    std::vector<std::uint32_t>& packed_words) {
    packed_words.clear();
    if (bit_width == 0u || bit_width > 32u)
        return false;
    if (values.size() > std::numeric_limits<std::uint64_t>::max() / bit_width)
        return false;
    const std::uint64_t total_bits = static_cast<std::uint64_t>(values.size()) * bit_width;
    if (total_bits > std::numeric_limits<std::uint64_t>::max() - 31u)
        return false;
    const std::uint64_t word_count = (total_bits + 31u) / 32u;
    if (word_count > packed_words.max_size())
        return false;
    packed_words.assign(static_cast<std::size_t>(word_count), 0u);
    const std::uint32_t mask = width_mask(bit_width);
    for (std::uint64_t index = 0; index < values.size(); ++index) {
        const std::uint32_t value = values[static_cast<std::size_t>(index)];
        if ((value & ~mask) != 0u) {
            packed_words.clear();
            return false;
        }
        const std::uint64_t position = index * bit_width;
        const std::size_t word_index = static_cast<std::size_t>(position >> 5u);
        const std::uint32_t offset = static_cast<std::uint32_t>(position & 31u);
        const std::uint32_t bits_in_first = 32u - offset;
        if (bit_width <= bits_in_first) {
            packed_words[word_index] |= value << (bits_in_first - bit_width);
        } else {
            const std::uint32_t bits_in_second = bit_width - bits_in_first;
            if (word_index >= packed_words.size() || word_index + 1u >= packed_words.size()) {
                packed_words.clear();
                return false;
            }
            packed_words[word_index] |= value >> bits_in_second;
            packed_words[word_index + 1u] |=
                (value & width_mask(bits_in_second)) << (32u - bits_in_second);
        }
    }
    return true;
}

bool unpack_golomb_codebook(
    const std::vector<std::uint32_t>& packed_words,
    std::uint32_t value_count,
    std::uint32_t bit_width,
    std::vector<std::uint32_t>& values) {
    values.clear();
    if (bit_width == 0u || bit_width > 32u)
        return false;
    if (static_cast<std::uint64_t>(value_count)
        > std::numeric_limits<std::uint64_t>::max() / bit_width)
        return false;
    const std::uint64_t total_bits = static_cast<std::uint64_t>(value_count) * bit_width;
    if (total_bits > std::numeric_limits<std::uint64_t>::max() - 31u)
        return false;
    const std::uint64_t word_count = (total_bits + 31u) / 32u;
    if (word_count > packed_words.size())
        return false;
    values.resize(value_count);
    const std::uint32_t mask = width_mask(bit_width);
    for (std::uint64_t index = 0; index < value_count; ++index) {
        const std::uint64_t position = index * bit_width;
        const std::size_t word_index = static_cast<std::size_t>(position >> 5u);
        const std::uint32_t offset = static_cast<std::uint32_t>(position & 31u);
        const std::uint32_t bits_in_first = 32u - offset;
        if (bit_width <= bits_in_first) {
            values[static_cast<std::size_t>(index)] =
                (packed_words[word_index] >> (bits_in_first - bit_width)) & mask;
        } else {
            const std::uint32_t bits_in_second = bit_width - bits_in_first;
            if (word_index >= packed_words.size() || word_index + 1u >= packed_words.size())
                return false;
            const std::uint32_t first_mask = width_mask(bits_in_first);
            values[static_cast<std::size_t>(index)] =
                ((packed_words[word_index] & first_mask) << bits_in_second) |
                (packed_words[word_index + 1u] >> (32u - bits_in_second));
        }
    }
    return true;
}

bool pack_golomb_error_codebook(
    const std::vector<std::int32_t>& values,
    std::uint32_t bit_width,
    std::vector<std::uint32_t>& packed_words,
    std::string& error) {
    error.clear();
    std::vector<std::uint32_t> encoded;
    if (!pack_golomb_errors(values, bit_width, encoded, error)
        || !pack_golomb_codebook(encoded, bit_width, packed_words)) {
        if (error.empty())
            error = "could not pack signed residual codebook";
        packed_words.clear();
        return false;
    }
    return true;
}

} // namespace auro3d:encode
