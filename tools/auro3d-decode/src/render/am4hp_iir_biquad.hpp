#pragma once

#include <array>
#include <cmath>

namespace auro3d {
namespace {

// Native auro_iir_biquad_Config_float64_t get_coeffs cases 7/0xC. Type 7
// (high shelf) and type 12 share this body after construct/update fill A,
// cos, sin, and the stored intermediate at +24.
inline std::array<float, 5> am4hp_iir_shelf_coeffs(
    double cosine, double sine, double a, double intermediate) noexcept {
    const double a_plus = a + 1.0;
    const double a_minus = a - 1.0;
    const double mixed = intermediate * sine;
    const double b0n = (cosine * a_minus + a_plus + mixed) * a;
    const double cos_term = a_plus * cosine;
    const double b1n = a * -2.0 * (a_minus + cos_term);
    const double cos_am = cosine * a_minus;
    const double b2n = (a_plus + cos_am - mixed) * a;
    const double a0_partial = a_plus - cos_am;
    const double a0 = mixed + a0_partial;
    const double a1n = 2.0 * (a_minus - cos_term);
    const double a2n = a0_partial - mixed;
    const double inverse = 1.0 / a0;
    return {static_cast<float>(b0n * inverse),
            static_cast<float>(b1n * inverse),
            static_cast<float>(b2n * inverse),
            static_cast<float>(a1n * inverse),
            static_cast<float>(a2n * inverse)};
}

inline bool am4hp_iir_type7_highshelf(
    float frequency_hz, unsigned sample_rate, float q, float gain_db,
    std::array<float, 5>& coefficients) noexcept {
    if (sample_rate == 0u || !(q > 0.0f) || !(frequency_hz > 0.0f)
        || !std::isfinite(gain_db))
        return false;
    const double omega = static_cast<double>(frequency_hz) * 6.283185307179586
        / static_cast<double>(sample_rate);
    const double a = std::pow(10.0, static_cast<double>(gain_db) / 40.0);
    coefficients = am4hp_iir_shelf_coeffs(
        std::cos(omega), std::sin(omega), a,
        std::sqrt(a) / static_cast<double>(q));
    return true;
}

inline bool am4hp_iir_type12_shelf(
    float frequency_hz, unsigned sample_rate, float q, float gain_db,
    std::array<float, 5>& coefficients) noexcept {
    if (sample_rate == 0u || !(q > 0.0f) || !(frequency_hz > 0.0f)
        || !std::isfinite(gain_db))
        return false;
    const double omega = static_cast<double>(frequency_hz) * 6.283185307179586
        / static_cast<double>(sample_rate);
    // Native Config_float64_t_t_construct rounds the reciprocal to float32
    // before subtracting 1.0f, then stores that rounded value as double.
    // Keeping the reciprocal entirely in double shifts some dynamic shelf
    // coefficients by one ULP.
    const float rounded_r =
        static_cast<float>(1.0 / static_cast<double>(q)) - 1.0f;
    const double r = static_cast<double>(rounded_r);
    const double a = std::pow(10.0, static_cast<double>(gain_db) / 40.0);
    const double intermediate = std::sqrt((a * a + 1.0) * r + a + a);
    coefficients = am4hp_iir_shelf_coeffs(
        std::cos(omega), std::sin(omega), a, intermediate);
    return true;
}

} // namespace
} // namespace auro3d
