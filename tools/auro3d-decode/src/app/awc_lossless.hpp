#pragma once

#include "cx_bits.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d {
namespace awc {

struct AwcPayloadConfig {
    std::uint32_t common_preamble = 0;
    std::vector<std::uint32_t> stream_parameters;
    std::uint32_t block_size = 0;
    std::uint32_t sample_rate = 48000;
    std::uint32_t frame_divisor = 1;
    std::uint8_t error_scale_byte = 1;
    std::uint16_t partition_threshold = 0;
};

struct AwcLosslessDecodeResult {
    bool ok = false;
    std::string error;
    std::vector<std::vector<std::int32_t>> stream_samples;
};

inline unsigned preamble_width(std::uint32_t stream_count) {
    if (stream_count <= 1)
        return 0;
    unsigned bits = 0;
    for (std::uint32_t value = stream_count - 1; value; value >>= 1)
        ++bits;
    return bits;
}

inline std::uint32_t awc_partition_granule_total(
    std::uint32_t block_size,
    std::uint32_t stream_count,
    std::uint32_t preamble) {
    if (!stream_count || block_size <= preamble)
        return 0;
    const std::uint64_t adjusted =
        static_cast<std::uint64_t>(block_size) + stream_count - preamble - 1u;
    return static_cast<std::uint32_t>(adjusted / stream_count);
}

bool parse_bitdepth_mode(cx::Bits& bits, unsigned param_bits, std::uint32_t& mode, std::uint32_t& width);
bool parse_lpc_order(cx::Bits& bits, std::uint32_t& order);
bool parse_angle(cx::Bits& bits, std::uint32_t& angle);

void lpc_lossless_decode(
    std::uint32_t order,
    const std::int32_t* coeffs,
    std::int32_t* output,
    const std::int32_t* errors,
    std::size_t error_count,
    const std::int32_t* history,
    std::size_t history_count,
    bool history_initialized = false);

bool decode_lossless_frame(
    cx::Bits& bits,
    const AwcPayloadConfig& config,
    std::uint32_t stream_count,
    std::uint32_t samples_per_access_unit,
    std::vector<std::vector<std::int32_t>>& stream_samples,
    std::string& error,
    std::vector<std::uint32_t>* stream_bitdepths = nullptr);

} // namespace awc
} // namespace auro3d
