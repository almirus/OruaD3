#pragma once

#include "bit_writer.hpp"

#include <cstdint>
#include <vector>

namespace auro3d::encode {

/// Direct scalar equivalent of
/// bitstream::write_golomb_rice<unsigned int, VectorStorage> at 0x450750.
/// It writes quotient ones followed by zero, then the low `parameter` bits
/// through the native LSB-first storage writer.
bool write_golomb_rice(
    LsbBitWriter& writer,
    std::uint32_t value,
    std::uint32_t parameter);

bool write_golomb_rice_block(
    LsbBitWriter& writer,
    const std::vector<std::uint32_t>& values,
    std::uint32_t parameter);

bool golomb_rice_bit_count(
    const std::vector<std::uint32_t>& values,
    std::uint32_t parameter,
    std::uint64_t& bit_count);

} // namespace auro3d::encode
