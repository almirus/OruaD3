#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "scaler.hpp"

namespace auro3d::encode {

/// downmix_ copies original-layout PCM to its internal frame manager and,
/// when a configured channel scaler differs from 1, divides every sample by
/// that float scaler with truncation toward zero. The mapping from metadata
/// gain index to this float is intentionally outside this primitive.
bool apply_input_rescale(
    std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t original_layout,
    const std::array<float, 31>& scalers,
    std::string& error);

/// Converts explicit per-channel gain values with the native scaler primitive
/// and applies the same truncating rescale. No gain or layout defaults are
/// selected by this convenience wrapper.
bool apply_input_rescale_from_gains(
    std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t original_layout,
    const std::array<float, 31>& gains_db,
    std::string& error);

/// Resolves explicit native scaler indices (including the 255 infinity
/// sentinel) and applies the same rescale contract.
bool apply_input_rescale_from_indices(
    std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t original_layout,
    const std::array<std::uint8_t, 31>& scaler_indices,
    std::string& error);

} // namespace auro3d:encode
