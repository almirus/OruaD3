#include "pcm_shift.hpp"

namespace auro3d::encode {

bool pcm_shift_right(
    std::vector<std::int32_t>& samples,
    std::uint32_t shift,
    std::string& error) {
    error.clear();
    // Direct scalar port of auro:pcm:shift_right (PSRAD cases).
    if (shift > 24u) {
        error = "pcm shift_right rejects shifts above 24";
        return false;
    }
    if (shift == 0u)
        return true;
    const std::uint32_t fill = 0xFFFFFFFFu << (32u - shift);
    for (std::int32_t& sample : samples) {
        const std::uint32_t raw = static_cast<std::uint32_t>(sample);
        const std::uint32_t shifted = raw >> shift;
        sample = static_cast<std::int32_t>(
            sample < 0 ? shifted | fill : shifted);
    }
    return true;
}

bool pcm_shift_left(
    std::vector<std::int32_t>& samples,
    std::uint32_t shift,
    std::string& error) {
    error.clear();
    if (shift > 24u) {
        error = "pcm shift_left rejects shifts above 24";
        return false;
    }
    if (shift == 0u)
        return true;
    for (std::int32_t& sample : samples) {
        sample = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(sample) << shift);
    }
    return true;
}

} // namespace auro3d:encode
