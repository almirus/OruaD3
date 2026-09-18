#include "quality_measure.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace auro3d::encode {
namespace {

constexpr double kPcm24Scale = 0.00000011920928955078125;
constexpr double kMseFloorLinear = 3.981071705534969e-15;
constexpr double kPeakFloorLinear = 0.0000000630957344480193;

double level_from_mse(double normalized_mse) {
    return normalized_mse > kMseFloorLinear
        ? std::log10(normalized_mse) * 10.0
        : -144.0;
}

double level_from_peak(double normalized_peak) {
    return normalized_peak > kPeakFloorLinear
        ? std::log10(normalized_peak) * 20.0
        : -144.0;
}

} // namespace

bool measure_native_frame_quality(
    const std::vector<std::int32_t>& original,
    const std::vector<std::int32_t>& reconstructed,
    NativeFrameQuality& out,
    std::string& error) {
    error.clear();
    out = {};
    if (original.empty() || original.size() != reconstructed.size()) {
        error = "native quality measurement requires equal nonempty frames";
        return false;
    }
    double squared_error = 0.0;
    double peak_error = 0.0;
    for (std::size_t index = 0u; index < original.size(); ++index) {
        const double difference =
            static_cast<double>(original[index])
            - static_cast<double>(reconstructed[index]);
        squared_error += difference * difference;
        peak_error = std::max(peak_error, std::abs(difference));
    }
    const double normalized_mse =
        squared_error / static_cast<double>(original.size())
        * kPcm24Scale * kPcm24Scale;
    const double normalized_peak = peak_error * kPcm24Scale;
    out.mse_db = level_from_mse(normalized_mse);
    out.peak_db = level_from_peak(normalized_peak);
    out.peak_to_rms_ratio =
        std::pow(10.0, (out.peak_db - out.mse_db) / 20.0);
    if (!std::isfinite(out.mse_db)
        || !std::isfinite(out.peak_db)
        || !std::isfinite(out.peak_to_rms_ratio)) {
        error = "native quality measurement overflowed";
        return false;
    }
    return true;
}

bool NativeQualityAccumulator::add(
    const NativeFrameQuality& frame,
    std::string& error) {
    error.clear();
    if (!std::isfinite(frame.mse_db)
        || !std::isfinite(frame.peak_db)
        || frame_count == std::numeric_limits<std::uint32_t>::max()) {
        error = "native quality accumulator input is invalid";
        return false;
    }
    const float mse_exponent = static_cast<float>(frame.mse_db * 0.05);
    const float peak_exponent = static_cast<float>(frame.peak_db * 0.05);
    const float rms_linear =
        static_cast<float>(std::pow(10.0, mse_exponent));
    const float peak_linear =
        static_cast<float>(std::pow(10.0, peak_exponent));
    const double rms = static_cast<double>(rms_linear);
    squared_rms_sum += rms * rms;
    ++frame_count;
    maximum_peak_linear = std::max(
        maximum_peak_linear,
        std::abs(static_cast<double>(peak_linear)));
    return true;
}

double NativeQualityAccumulator::error_level_db() const {
    if (frame_count == 0u)
        return -3000.0;
    const double rms = std::sqrt(
        squared_rms_sum / static_cast<double>(frame_count));
    return rms > 0.0
        ? std::max(std::log10(rms) * 20.0, -3000.0)
        : -3000.0;
}

} // namespace auro3d:encode
