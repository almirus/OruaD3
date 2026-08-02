#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Outputs of the per-frame lambda in ComputeQuality @ 0x4F09E0.
struct NativeFrameQuality {
    double mse_db = -144.0;
    double peak_db = -144.0;
    double peak_to_rms_ratio = 1.0;
};

/// Computes native full-scale-normalized MSE and peak-error levels for one
/// PCM24 source/reconstruction pair.
bool measure_native_frame_quality(
    const std::vector<std::int32_t>& original,
    const std::vector<std::int32_t>& reconstructed,
    NativeFrameQuality& out,
    std::string& error);

/// Accumulator stored in ComputeQuality's local `{sum,count,max}` record.
struct NativeQualityAccumulator {
    double squared_rms_sum = 0.0;
    std::uint32_t frame_count = 0;
    double maximum_peak_linear = 0.0;

    bool add(const NativeFrameQuality& frame, std::string& error);
    double error_level_db() const;
};

} // namespace auro3d::encode
