#include "am4hp_low_end.hpp"

#include <cmath>
#include <cstring>

namespace auro3d {

float Am4hpLowEnd::Biquad::process(float input) noexcept {
    const float output = c[0] * input + z1;
    z1 = c[1] * input + z2 - c[3] * output;
    z2 = c[2] * input - c[4] * output;
    return output;
}

bool Am4hpLowEnd::construct(const Coefficients& mid_coefficients,
                            const Coefficients& final_coefficients,
                            std::uint32_t flags) noexcept {
    for (float value : mid_coefficients)
        if (!std::isfinite(value)) return false;
    for (float value : final_coefficients)
        if (!std::isfinite(value)) return false;
    flags_ = flags;
    mid_first_.c = mid_coefficients;
    mid_second_.c = mid_coefficients;
    mid_center_.c = mid_coefficients;
    mid_height_first_.c = mid_coefficients;
    mid_height_second_.c = mid_coefficients;
    final_first_.c = final_coefficients;
    final_second_.c = final_coefficients;
    reset_audio_state();
    configured_ = true;
    return true;
}

void Am4hpLowEnd::reset_audio_state() noexcept {
    mid_first_.reset();
    mid_second_.reset();
    mid_center_.reset();
    mid_height_first_.reset();
    mid_height_second_.reset();
    final_first_.reset();
    final_second_.reset();
}

std::array<std::uint32_t, 8> Am4hpLowEnd::debug_state_bits() const noexcept {
    std::array<std::uint32_t, 8> result{};
    const float values[8]{final_first_.z1, final_first_.z2,
                          final_second_.z1, final_second_.z2,
                          mid_first_.z1, mid_first_.z2,
                          mid_second_.z1, mid_second_.z2};
    std::memcpy(result.data(), values, sizeof(values));
    return result;
}

bool Am4hpLowEnd::process(const std::array<Block*, 8>& planes,
                          Block& first_output,
                          Block& second_output) noexcept {
    if (!configured_ || !planes[2] || !planes[3])
        return false;

    first_output = *planes[2];
    second_output = *planes[3];
    for (std::size_t sample = 0u; sample < 32u; ++sample) {
        (*planes[2])[sample] = -mid_first_.process((*planes[2])[sample]);
        (*planes[3])[sample] = -mid_second_.process((*planes[3])[sample]);
    }

    if ((flags_ & 4u) != 0u) {
        if (!planes[4]) return false;
        for (std::size_t sample = 0u; sample < 32u; ++sample) {
            // Native adds the dry center first, then filters and sign-inverts
            // the descriptor plane in place for the following CenterFront.
            const float center = (*planes[4])[sample];
            // The non-aliasing SIMD branch widens pairs to float64 for this
            // fixed gain and narrows the product back to float32.
            const float contribution = static_cast<float>(
                static_cast<double>(center) * 0.5011872336272722);
            first_output[sample] += contribution;
            second_output[sample] += contribution;
            (*planes[4])[sample] = -mid_center_.process(center);
        }
    }

    if ((flags_ & 0x30u) == 0x30u) {
        if (!planes[6] || !planes[7]) return false;
        for (std::size_t sample = 0u; sample < 32u; ++sample) {
            first_output[sample] += (*planes[6])[sample];
            second_output[sample] += (*planes[7])[sample];
            (*planes[6])[sample] = -mid_height_first_.process((*planes[6])[sample]);
            (*planes[7])[sample] = -mid_height_second_.process((*planes[7])[sample]);
        }
    }

    for (std::size_t sample = 0u; sample < 32u; ++sample) {
        first_output[sample] = final_first_.process(first_output[sample]);
        second_output[sample] = final_second_.process(second_output[sample]);
    }

    if ((flags_ & 8u) != 0u) {
        if (!planes[5]) return false;
        for (std::size_t sample = 0u; sample < 32u; ++sample) {
            first_output[sample] += (*planes[5])[sample] * 0.7079457843841379f;
            second_output[sample] += (*planes[5])[sample] * 0.7079457843841379f;
        }
    }
    return true;
}

}
