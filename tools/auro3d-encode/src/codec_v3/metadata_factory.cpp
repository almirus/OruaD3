#include "metadata_factory.hpp"

#include "channel_config.hpp"
#include "layout.hpp"
#include "layout_metadata.hpp"

#include <utility>

namespace auro3d::encode {

bool make_layout_metadata_block(
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    std::uint32_t mux_m,
    PcmMetadataBlock& metadata,
    std::string& error) {
    return make_layout_metadata_block(
        original_layout,
        carrier_layout,
        mux_m,
        LayoutMetadataOptions{},
        metadata,
        error);
}

bool make_layout_metadata_block(
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    std::uint32_t mux_m,
    const LayoutMetadataOptions& options,
    PcmMetadataBlock& metadata,
    std::string& error) {
    error.clear();
    metadata = {};
    if (mux_m < 3u || mux_m > 14u) {
        error = "metadata mux parameter is outside the native range";
        return false;
    }
    std::uint32_t expected_carrier = 0;
    std::uint8_t config_id = 0;
    if (!codec_v3_carrier_layout(original_layout, expected_carrier)
        || expected_carrier != carrier_layout
        || !codec_v3_layout_to_channel_config(original_layout, config_id)) {
        error = "layout has no direct native codec-v3 metadata configuration";
        return false;
    }
    std::vector<AdolInstruction> layout_block;
    if (!make_layout_adol_block(original_layout, layout_block, error))
        return false;
    if (options.secondary_downmix_gains_present
        && !append_secondary_downmix_gains_adol(
            layout_block,
            carrier_layout,
            options.secondary_downmix_gains_db,
            error)) {
        return false;
    }
    if (options.loudness_present
        && !append_loudness_adol(
            layout_block, options.loudness, error)) {
        return false;
    }
    if (options.auromatic_present
        && !append_auromatic_adol(
            layout_block,
            options.auromatic_profile,
            options.auromatic_mode,
            error)) {
        return false;
    }
    if (options.opcode_50_present
        && !append_opcode_50_adol(
            layout_block, options.opcode_50_value, error)) {
        return false;
    }
    if (options.primary_downmix_gain_present
        && !append_primary_downmix_gain_adol(
            layout_block,
            options.primary_downmix_channel,
            options.primary_downmix_scaler_index,
            error)) {
        return false;
    }
    if (options.limit_simple_present
        && !append_limit_simple_adol(
            layout_block,
            options.limit_simple_scaler_index,
            error)) {
        return false;
    }
    if (options.opcode_6e_present
        && !append_opcode_6e_adol(
            layout_block, options.opcode_6e_value, error)) {
        return false;
    }
    if (options.encoder_version_present
        && !append_encoder_version_adol(
            layout_block, options.encoder_version, error)) {
        return false;
    }
    if (!validate_adol_block(layout_block, error))
        return false;
    metadata.mux_m = mux_m;
    metadata.prefix.adol_block_count = 1u;
    metadata.prefix.channel_config = {config_id, 0xFFu, 0xFFu, 0xFFu};
    metadata.adol_blocks.push_back(std::move(layout_block));
    return true;
}

} // namespace auro3d:encode
