#pragma once

#include "adol_syntax.hpp"
#include "metadata_syntax.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace auro3d::encode {

/// All explicit data carried by one codec-v3 PCM metadata block. No layout,
/// ADOL, or profile value is inferred by this layer.
struct PcmMetadataBlock {
    std::uint32_t mux_m = 0;
    PcmMetadataPrefix prefix;
    std::vector<std::vector<AdolInstruction>> adol_blocks;
};

/// Creates a scanner-valid PCM metadata block: clear mux storage, install sync
/// fields, emit the fixed prefix and every ADOL block, then close the native
/// PCM CRC relation. The carrier PCM has already been prepared by mix output.
bool write_pcm_metadata_block(
    std::vector<std::int32_t>& carrier_samples,
    std::size_t block_start,
    std::size_t block_samples,
    const PcmMetadataBlock& metadata);

/// Checks the scanner CRC closure after a metadata block has been emitted.
bool validate_pcm_metadata_block(
    const std::vector<std::int32_t>& carrier_samples,
    std::size_t block_start,
    std::size_t block_samples,
    std::uint32_t mux_m);

} // namespace auro3d:encode
