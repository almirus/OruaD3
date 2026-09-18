#pragma once

#include "layout_metadata.hpp"
#include "metadata_block.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace auro3d::encode {

/// Explicit optional ADOL values accepted by the standalone metadata
/// factory. No downmix coefficients or gains are inferred from layouts.
struct LayoutMetadataOptions {
    bool loudness_present = false;
    LoudnessMetadata loudness{};
    bool primary_downmix_gain_present = false;
    std::uint32_t primary_downmix_channel = 0;
    std::uint32_t primary_downmix_scaler_index = 0;
    bool secondary_downmix_gains_present = false;
    std::array<float, 31> secondary_downmix_gains_db{};
    bool limit_simple_present = false;
    std::uint32_t limit_simple_scaler_index = 0;
    bool auromatic_present = false;
    std::uint32_t auromatic_profile = 0;
    std::uint32_t auromatic_mode = 0;
    bool opcode_50_present = false;
    std::uint32_t opcode_50_value = 0;
    bool encoder_version_present = false;
    std::uint32_t encoder_version = 0;
    bool opcode_6e_present = false;
    std::uint32_t opcode_6e_value = 0;
};

/// Builds the minimal native-proven layout metadata block: one ADOL block
/// containing opcode 0x1e and the direct channel-input configuration. Every
/// other prefix field stays explicit zero/0xff as required by the scanner.
bool make_layout_metadata_block(
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    std::uint32_t mux_m,
    PcmMetadataBlock& metadata,
    std::string& error);

/// Builds the same layout block and appends explicitly supplied primary and
/// secondary downmix instructions in native scheduler order.
bool make_layout_metadata_block(
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    std::uint32_t mux_m,
    const LayoutMetadataOptions& options,
    PcmMetadataBlock& metadata,
    std::string& error);

} // namespace auro3d:encode
