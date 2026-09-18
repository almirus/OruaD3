#pragma once

#include <cstdint>
#include <string>

namespace auro3d {

/// ACV3Decoder:get_max_allowed_output_layout.
constexpr std::uint32_t kCodecV3MaxAllowedOutputLayout = 0x7FFFu;

struct DspOutputLayoutRequest {
    bool specified = false;
    bool is_legacy_count = false;
    unsigned legacy_count = 0;
    /// Resolved Auro channel bitmask when known.
    std::uint32_t mask = 0;
    bool mask_resolved = false;
    std::string raw;
};

/// Parse CLI value for --dsp-output-channels --dsp-output-layout.
/// Accepts: 0/omitted-style "0", legacy counts 1..14, hex 0x..., decimal mask >=15,
/// named layouts (5.1_4H, 5.1.4, 7.1_5H_1T, …).
bool parse_dsp_output_layout_arg(
    const std::string& arg,
    DspOutputLayoutRequest& out,
    std::string& error);

/// Reverse of auro_channel_layout_to_string, plus common aliases (5.1.4, 11.1, …).
bool auro_channel_layout_from_string(const std::string& name, std::uint32_t& mask);

/// Legacy N-channel shorthand used by build_native_channel_layout (0 if unknown).
std::uint32_t legacy_dsp_channel_count_to_mask(unsigned channel_count);

/// Codec-v3: mask must fit within kCodecV3MaxAllowedOutputLayout.
bool codec_v3_output_layout_mask_allowed(std::uint32_t mask);

/// True when requested is a known Auro layout that is a strict superset of
/// the codec dematrix stream_layout, and the added slots are fillable by
/// post-dematrix XinN and/or simple bed synthesis (C from FL/FR, silent LFE).
bool is_compatible_post_dematrix_upmix(
    std::uint32_t stream_layout,
    std::uint32_t requested);

/// AuroCX ObjectRenderer:get_api_supported_layout_cicp.
/// Returns the canonical API layout contained in requested, or 0 if none.
std::uint32_t auro_cx_api_supported_layout_mask(std::uint32_t requested);

bool auro_cx_output_layout_mask_allowed(std::uint32_t mask);

std::string format_layout_label(std::uint32_t mask);

/// Path-specific rejection messages (Codec-v3 vs AuroCX).
std::string codec_v3_invalid_output_layout_message(
    std::uint32_t requested_mask,
    std::uint32_t stream_layout_mask = 0,
    unsigned stream_channels = 0);

std::string auro_cx_invalid_output_layout_message(std::uint32_t requested_mask);

std::string auro_cx_unsupported_remap_message(
    std::uint32_t stream_layout_mask,
    std::uint32_t requested_mask);

} // namespace auro3d
