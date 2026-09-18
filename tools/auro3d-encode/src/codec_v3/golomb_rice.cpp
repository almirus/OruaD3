#include "golomb_rice.hpp"

#include <limits>

namespace auro3d::encode {

bool write_golomb_rice(
    LsbBitWriter& writer,
    std::uint32_t value,
    std::uint32_t parameter) {
    if (parameter > 32u)
        return false;
    const std::uint32_t quotient = parameter == 32u ? 0u : value >> parameter;
    // Native writes one bool for every quotient unit, followed by a
    // zero bool terminator. The decoder counts leading ones before that zero.
    for (std::uint32_t bit = 0u; bit < quotient; ++bit) {
        if (!writer.write_one())
            return false;
    }
    if (!writer.write_bits(0u, 1u))
        return false;
    return writer.write_bits(value, parameter);
}

bool write_golomb_rice_block(
    LsbBitWriter& writer,
    const std::vector<std::uint32_t>& values,
    std::uint32_t parameter) {
    if (parameter > 32u)
        return false;
    std::uint64_t expected_bits = 0u;
    if (!golomb_rice_bit_count(values, parameter, expected_bits))
        return false;
    const std::uint64_t start_bits = writer.bit_count();
    for (const std::uint32_t value : values) {
        if (!write_golomb_rice(writer, value, parameter))
            return false;
    }
    return writer.bit_count() >= start_bits
        && writer.bit_count() - start_bits == expected_bits;
}

bool golomb_rice_bit_count(
    const std::vector<std::uint32_t>& values,
    std::uint32_t parameter,
    std::uint64_t& bit_count) {
    bit_count = 0u;
    if (parameter > 32u)
        return false;
    for (const std::uint32_t value : values) {
        const std::uint64_t quotient = parameter == 32u ? 0u : value >> parameter;
        const std::uint64_t add = quotient + 1u + parameter;
        if (bit_count > std::numeric_limits<std::uint64_t>::max() - add)
            return false;
        bit_count += add;
    }
    return true;
}

} // namespace auro3d:encode
