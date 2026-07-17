#pragma once

#include "awc_lossless.hpp"
#include "cx_bits.hpp"
#include <cstdint>
#include <string>

namespace auro3d {
namespace awc {

bool decode_transparent_frame(
    cx::Bits& bits,
    const AwcPayloadConfig& config,
    std::uint32_t stream_count,
    std::uint32_t samples_per_access_unit,
    std::vector<std::vector<std::int32_t>>& stream_samples,
    std::string& error,
    std::vector<std::uint32_t>* stream_bitdepths = nullptr);

} // namespace awc
} // namespace auro3d
