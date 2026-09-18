#pragma once

#include "pcm_metadata.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Fixed prefix consumed by extract_metadata_from_a3d_block_at before its ADOL
/// block list. Values remain explicit until Encoder:prepare_metadata_unit_block_
/// is fully ported; this writer never chooses defaults on the caller's behalf.
struct PcmMetadataPrefix {
    std::uint8_t field_8_0 = 0;
    std::array<bool, 4> flags_1_0{};
    std::uint8_t field_4_0 = 0;
    std::array<std::uint8_t, 2> fields_8_1{};
    std::array<bool, 2> flags_1_1{};
    std::uint8_t field_2_0 = 0;
    std::uint8_t field_4_1 = 0;
    std::uint8_t field_8_2 = 0;
    std::uint8_t adol_block_count = 0;
    std::uint8_t field_8_3 = 0;
    std::array<std::uint8_t, 4> channel_config{};
    std::array<std::uint8_t, 4> reserved{};
    std::vector<std::uint32_t> extension_words;
};

/// Validates prefix field widths and the extension-word cardinality without
/// consuming the metadata writer.
bool validate_pcm_metadata_prefix(
    const PcmMetadataPrefix& prefix,
    std::string& error);

/// Writes the exact field order parsed by extract_metadata_from_a3d_block_at.
/// ADOL tag/instruction serialization follows this prefix in a later stage.
bool write_pcm_metadata_prefix(PcmMetadataFalseWriter& writer, const PcmMetadataPrefix& prefix);

} // namespace auro3d:encode
