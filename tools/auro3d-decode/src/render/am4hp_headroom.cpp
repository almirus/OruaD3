#include "am4hp_headroom.hpp"

#include <algorithm>
#include <cmath>

namespace auro3d {

bool Am4hpHeadroom::construct(const std::vector<unsigned>& plane_mapping,
                              float gain) noexcept {
    if (plane_mapping.empty() || plane_mapping.size() > 31u
        || !std::isfinite(gain)
        || std::any_of(plane_mapping.begin(), plane_mapping.end(),
                       [](unsigned value) { return value >= 31u; }))
        return false;
    plane_mapping_ = plane_mapping;
    gain_ = gain;
    return true;
}

bool Am4hpHeadroom::process(
    const std::vector<const std::array<float, 32>*>& input,
    const std::vector<std::array<float, 32>*>& output) const noexcept {
    if (plane_mapping_.empty() || input.size() < 31u || output.size() < 31u)
        return false;
    for (std::size_t destination = 0u; destination < plane_mapping_.size();
         ++destination) {
        const unsigned plane_index = plane_mapping_[destination];
        const auto* source = input[plane_index];
        auto* target = output[plane_index];
        if (!source || !target)
            return false;
        for (std::size_t sample = 0u; sample < 32u; ++sample)
            (*target)[sample] = (*source)[sample] * gain_;
    }
    return true;
}

}
