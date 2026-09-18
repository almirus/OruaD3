#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Inverse of gr_extract_packed_unsigned. Decoder codebooks are a
/// contiguous MSB-first field stream in 32-bit words, unlike the separate
/// LSB-first entropy stream.
bool pack_golomb_codebook(
    const std::vector<std::uint32_t>& values,
    std::uint32_t bit_width,
    std::vector<std::uint32_t>& packed_words);

bool unpack_golomb_codebook(
    const std::vector<std::uint32_t>& packed_words,
    std::uint32_t value_count,
    std::uint32_t bit_width,
    std::vector<std::uint32_t>& values);

/// Signed-residual convenience wrapper: applies the codec-v3 sign-and-
/// magnitude mapping, then packs the resulting fixed-width fields MSB-first.
bool pack_golomb_error_codebook(
    const std::vector<std::int32_t>& values,
    std::uint32_t bit_width,
    std::vector<std::uint32_t>& packed_words,
    std::string& error);

} // namespace auro3d:encode
