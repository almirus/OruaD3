#include "quality_filter.hpp"

#include <algorithm>

namespace auro3d::encode {

void NativeQualityFilter::update(
    std::uint32_t bit_line,
    float quality) {
    points_.push_back({bit_line, quality});
    std::stable_sort(
        points_.begin(),
        points_.end(),
        [](const NativeQualityFilterPoint& left,
           const NativeQualityFilterPoint& right) {
            return left.bit_line < right.bit_line;
        });
    prune();
}

void NativeQualityFilter::prune() {
    if (points_.size() < 2u)
        return;

    // Native keeps the first maximum on ties (next <= current).
    std::size_t maximum = 0u;
    for (std::size_t index = 1u; index < points_.size(); ++index) {
        if (points_[index].quality > points_[maximum].quality)
            maximum = index;
    }
    if (maximum != 0u && maximum + 1u < points_.size()) {
        const NativeQualityFilterPoint left = points_[maximum - 1u];
        const NativeQualityFilterPoint center = points_[maximum];
        const NativeQualityFilterPoint right = points_[maximum + 1u];
        points_ = {left, center, right};
        return;
    }
    if (points_.size() <= 2u)
        return;
    if (maximum == 0u) {
        points_.resize(2u);
    } else {
        points_.erase(points_.begin(), points_.end() - 2);
    }
}

std::uint32_t NativeQualityFilter::check(
    std::uint32_t bit_line) const {
    if (points_.size() == 3u) {
        return points_.front().bit_line <= bit_line
                && bit_line <= points_.back().bit_line
            ? 0u
            : 2u;
    }
    if (points_.size() != 2u)
        return 0u;

    const NativeQualityFilterPoint& first = points_[0];
    const NativeQualityFilterPoint& second = points_[1];
    if (first.bit_line <= bit_line && bit_line <= second.bit_line)
        return 0u;
    if (bit_line < first.bit_line)
        return second.quality > first.quality ? 2u : 0u;
    return second.quality <= first.quality ? 2u : 0u;
}

} // namespace auro3d:encode
