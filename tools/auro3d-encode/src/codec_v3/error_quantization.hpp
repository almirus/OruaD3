#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Exact inverse of decoder gr_sign_extend_104e40. Widths below 32 are
/// sign-and-magnitude, not two's-complement. Width 32 is copied as int32.
bool pack_golomb_error(
    std::int32_t value,
    std::uint32_t bit_width,
    std::uint32_t& packed);

bool unpack_golomb_error(
    std::uint32_t packed,
    std::uint32_t bit_width,
    std::int32_t& value);

/// Maps a residual table through the decoder's sign-and-magnitude code and
/// reports the first value that cannot fit the selected native width.
bool pack_golomb_errors(
    const std::vector<std::int32_t>& values,
    std::uint32_t bit_width,
    std::vector<std::uint32_t>& packed,
    std::string& error);

} // namespace auro3d:encode
