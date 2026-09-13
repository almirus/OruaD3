#include "am4hp_upmix_gain.hpp"

#include <algorithm>

namespace auro3d {

bool Am4hpUpmixGain::construct(const std::vector<unsigned>& plane_mapping,
                               const std::vector<float>& gains) noexcept {
    if (plane_mapping.empty() || plane_mapping.size() > 31u
        || gains.size() != plane_mapping.size())
        return false;
    if (std::any_of(plane_mapping.begin(), plane_mapping.end(),
                    [](unsigned value) { return value >= 31u; }))
        return false;
    plane_mapping_ = plane_mapping;
    gains_ = gains;
    return true;
}

void Am4hpUpmixGain::reset_audio_state() noexcept {
    // The captured configuration has no active smoothing state.
}

bool Am4hpUpmixGain::process(
    const std::vector<std::array<float, 32>*>& planes) const noexcept {
    if (plane_mapping_.empty() || gains_.size() != plane_mapping_.size()
        || planes.size() < 31u)
        return false;
    for (std::size_t destination = 0u; destination < plane_mapping_.size();
         ++destination) {
        auto* plane = planes[plane_mapping_[destination]];
        if (!plane)
            return false;
        for (float& sample : *plane)
            sample *= gains_[destination];
    }
    return true;
}

}
