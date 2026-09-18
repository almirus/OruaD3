#include "headphone_fgwht.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace auro3d {
namespace {

float rounded_product(float first, float second) noexcept {
    volatile float product = first * second;
    return product;
}

// Android 10's libm uses double-precision range reduction and compact
// sine/cosine kernels for sinf/cosf. The MSVC CRT differs by one ulp for some
// angles, so preserve the constants and instruction ordering observed in the
// emulator's libm.so.
float bionic_kernel_sindf(double x) noexcept {
    // IEEE-754 bits: bfc555545995a603, 3f81107605230bc4,
    // bf2994eb3774cf24.
    constexpr double s1 = -0.166666549437010841;
    constexpr double s2 = 0.00833217814613885360;
    constexpr double s3 = -0.000195172989813857255;
    const double z = x * x;
    const double xz = x * z;
    const double tail_coefficient = s3 * z + s2;
    const double tail = (z * xz) * tail_coefficient;
    const double head = xz * s1 + x;
    return static_cast<float>(tail + head);
}

float bionic_kernel_cosdf(double x) noexcept {
    // IEEE-754 bits: bfdffffffd0c621c, 3fa55553e1068f19,
    // bf56c087e89a359d, 3ef99343027bf8c3.
    constexpr double c0 = -0.499999997251082240;
    constexpr double c1 = 0.0416666233243445164;
    constexpr double c2 = -0.00138867637943760410;
    constexpr double c3 = 0.0000243904507035645425;
    const double z = x * x;
    const double w = z * z;
    const double tail_coefficient = c3 * z + c2;
    const double head = c0 * z + 1.0;
    const double tail = (z * w) * tail_coefficient;
    const double middle = w * c1 + head;
    return static_cast<float>(middle + tail);
}

bool bionic_sincosf_small(float x, float& sine, float& cosine) noexcept {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &x, sizeof(bits));
    const std::uint32_t magnitude = bits & 0x7fffffffu;
    if (magnitude >= 0x7f800000u
        || ((magnitude >> 20u) & 0x7ffu) > 0x42eu)
        return false;

    // IEEE-754 bits: 41645f306dc9c883 and 3ff921fb54442d18.
    constexpr double quadrant_scale = 10680707.4308817443;
    constexpr double pio2 = 1.57079632679489656;
    const std::int32_t scaled = static_cast<std::int32_t>(
        static_cast<double>(x) * quadrant_scale);
    const std::int32_t quadrant = (scaled + 0x00800000) >> 24;
    const double reduced = static_cast<double>(x)
                         - static_cast<double>(quadrant) * pio2;
    const float reduced_sine = bionic_kernel_sindf(reduced);
    const float reduced_cosine = bionic_kernel_cosdf(reduced);
    switch (quadrant & 3) {
    case 0:
        sine = reduced_sine;
        cosine = reduced_cosine;
        break;
    case 1:
        sine = reduced_cosine;
        cosine = -reduced_sine;
        break;
    case 2:
        sine = -reduced_sine;
        cosine = -reduced_cosine;
        break;
    default:
        sine = -reduced_cosine;
        cosine = reduced_sine;
        break;
    }
    return true;
}

void rotate(float& first, float& second, float cosine, float sine) noexcept {
    const float old_first = first;
    const float old_second = second;
    const float first_from_second = rounded_product(old_second, sine);
    const float first_from_first = rounded_product(old_first, cosine);
    const float second_from_second = rounded_product(-old_second, cosine);
    const float second_from_first = rounded_product(old_first, sine);
    first = first_from_second + first_from_first;
    second = second_from_second + second_from_first;
}

bool valid_count(std::size_t count) noexcept {
    return count >= 2u && (count & (count - 1u)) == 0u;
}

}

HeadphoneFgwhtSmoothed::HeadphoneFgwhtSmoothed() noexcept {
    state_[0] = 0.39269909262657166f; // 0x3ec90fdb, pi/8
    state_[1] = state_[0];
    state_[2] = 0.70710676908493042f; // 0x3f3504f3
    state_[3] = state_[2];
}

void HeadphoneFgwhtSmoothed::initialize(float angle) noexcept {
    state_[0] = angle;
    state_[1] = angle;
    if (!bionic_sincosf_small(angle + angle, state_[3], state_[2])) {
        state_[2] = std::numeric_limits<float>::quiet_NaN();
        state_[3] = state_[2];
    }
}

void HeadphoneFgwhtSmoothed::transform_constant(
    std::vector<std::array<float, 32>>& blocks,
    std::size_t offset, std::size_t count,
    float cosine, float sine) noexcept {
    if (count < 2u) return;
    const std::size_t half = count / 2u;
    for (std::size_t pair = 0; pair < half; ++pair)
        for (std::size_t sample = 0; sample < 32u; ++sample)
            rotate(blocks[offset + pair][sample],
                   blocks[offset + half + pair][sample], cosine, sine);
    transform_constant(blocks, offset, half, cosine, sine);
    transform_constant(blocks, offset + half, half, cosine, sine);
}

void HeadphoneFgwhtSmoothed::transform_smoothed(
    std::vector<std::array<float, 32>>& blocks,
    std::size_t offset, std::size_t count,
    const std::array<float, 32>& cosines,
    const std::array<float, 32>& sines) noexcept {
    if (count < 2u) return;
    const std::size_t half = count / 2u;
    for (std::size_t pair = 0; pair < half; ++pair)
        for (std::size_t sample = 0; sample < 32u; ++sample)
            rotate(blocks[offset + pair][sample],
                   blocks[offset + half + pair][sample],
                   cosines[sample], sines[sample]);
    transform_smoothed(blocks, offset, half, cosines, sines);
    transform_smoothed(blocks, offset + half, half, cosines, sines);
}

bool HeadphoneFgwhtSmoothed::process(
    std::vector<std::array<float, 32>>& blocks,
    float target_angle) noexcept {
    if (!valid_count(blocks.size()) || !std::isfinite(target_angle))
        return false;
    const float doubled = target_angle + target_angle;
    float new_cosine = 0.0f, new_sine = 0.0f;
    if (!bionic_sincosf_small(doubled, new_sine, new_cosine))
        return false;
    if (state_[1] == target_angle) {
        transform_constant(blocks, 0u, blocks.size(), new_cosine, new_sine);
        return true;
    }

    std::array<float, 32> cosines{}, sines{};
    const float cosine_step = (new_cosine - state_[2]) * 0.03125f;
    const float sine_step = (new_sine - state_[3]) * 0.03125f;
    float cosine = state_[2];
    float sine = state_[3];
    for (std::size_t sample = 0; sample < 32u; ++sample) {
        cosine += cosine_step;
        sine += sine_step;
        cosines[sample] = cosine;
        sines[sample] = sine;
    }
    state_[1] = target_angle;
    state_[2] = new_cosine;
    state_[3] = new_sine;
    transform_smoothed(blocks, 0u, blocks.size(), cosines, sines);
    return true;
}

}
