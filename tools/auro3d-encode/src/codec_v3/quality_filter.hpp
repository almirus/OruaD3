#pragma once

#include <cstdint>
#include <vector>

namespace auro3d::encode {

/// One encode:details:Filter:Point: candidate bit line and the negated
/// reconstruction-error level stored by ComputeQuality.
struct NativeQualityFilterPoint {
    std::uint32_t bit_line = 0;
    float quality = 0.0f;
};

/// Scalar representation of the native per-carrier quality search filter.
/// Its state is deliberately independent from PCM analysis so the confirmed
/// update/prune/check control path can be reused once ComputeQuality is
/// connected.
class NativeQualityFilter {
public:
    /// details:Filter:update: insert, sort by bit line, then
    /// retain the maximum-quality point and its immediate neighbours.
    void update(std::uint32_t bit_line, float quality);

    /// encode:Filter:check result: 0 keeps evaluating the
    /// candidate and 2 tells the bit-line search to skip it.
    std::uint32_t check(std::uint32_t bit_line) const;

    void clear() { points_.clear(); }
    const std::vector<NativeQualityFilterPoint>& points() const {
        return points_;
    }

private:
    void prune();

    std::vector<NativeQualityFilterPoint> points_;
};

} // namespace auro3d:encode
