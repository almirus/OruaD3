#include "input_rescale.hpp"

#include "frame_descriptor.hpp"

#include <cmath>
#include <limits>

namespace auro3d::encode {
namespace {

std::int32_t cvtt_float_to_i32(float value) {
    // x86 CVTTSS2SI returns 0x80000000 for NaN or out-of-range input.
    if (!std::isfinite(value)
        || value >= static_cast<float>(std::numeric_limits<std::int32_t>::max()) + 1.0f
        || value < static_cast<float>(std::numeric_limits<std::int32_t>::min())) {
        return std::numeric_limits<std::int32_t>::min();
    }
    return static_cast<std::int32_t>(value);
}

} // namespace

bool apply_input_rescale(
    std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t original_layout,
    const std::array<float, 31>& scalers,
    std::string& error) {
    error.clear();
    if (codec_planes.size() != kCodecV3ChannelCount
        || scalers.size() != kCodecV3ChannelCount
        || original_layout == 0u) {
        error = "invalid codec plane count for input rescale";
        return false;
    }
    std::size_t expected_samples = 0u;
    bool have_expected_samples = false;
    for (std::uint32_t id = 0; id < scalers.size(); ++id) {
        const bool active = (original_layout & (std::uint32_t{1} << id)) != 0u;
        if (!active) {
            if (!codec_planes[id].empty()) {
                error = "inactive codec plane contains samples during input rescale";
                return false;
            }
            continue;
        }
        if (!have_expected_samples) {
            expected_samples = codec_planes[id].size();
            have_expected_samples = true;
        } else if (codec_planes[id].size() != expected_samples) {
            error = "active codec planes have mismatched sample counts during input rescale";
            return false;
        }
        const float scaler = scalers[id];
        if (std::isnan(scaler) || scaler == 0.0f || scaler < 0.0f) {
            error = "invalid native input scaler";
            return false;
        }
        if (scaler == 1.0f)
            continue;
        for (std::int32_t& sample : codec_planes[id]) {
            // Native uses cvttps_epi32 / scalar float-to-int conversion:
            // truncation toward zero, not rounded fixed-point division.
            sample = cvtt_float_to_i32(static_cast<float>(sample) / scaler);
        }
    }
    return true;
}

bool apply_input_rescale_from_gains(
    std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t original_layout,
    const std::array<float, 31>& gains_db,
    std::string& error) {
    error.clear();
    std::array<float, 31> scalers{};
    for (std::size_t index = 0; index < gains_db.size(); ++index) {
        if (!scaler_from_gain_db(gains_db[index], scalers[index])) {
            error = "invalid native gain value at scaler index "
                + std::to_string(index);
            return false;
        }
    }
    for (std::uint32_t id = 0; id < scalers.size(); ++id) {
        if ((original_layout & (std::uint32_t{1} << id)) != 0u
            && scalers[id] == 0.0f) {
            error = "native gain produced a zero input scaler";
            return false;
        }
    }
    return apply_input_rescale(codec_planes, original_layout, scalers, error);
}

bool apply_input_rescale_from_indices(
    std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t original_layout,
    const std::array<std::uint8_t, 31>& scaler_indices,
    std::string& error) {
    error.clear();
    std::array<float, 31> scalers{};
    for (std::size_t index = 0; index < scaler_indices.size(); ++index) {
        if (!scaler_from_index(scaler_indices[index], scalers[index])) {
            error = "invalid native scaler index at channel "
                + std::to_string(index);
            return false;
        }
    }
    return apply_input_rescale(codec_planes, original_layout, scalers, error);
}

} // namespace auro3d::encode
