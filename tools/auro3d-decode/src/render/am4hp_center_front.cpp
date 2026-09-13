#include "am4hp_center_front.hpp"

#include "../auro3deng/detail/runtime_api.hpp"

#include <cmath>
#include <cstring>

namespace auro3d {

std::array<std::uint32_t, 9> Am4hpCenterFront::debug_state_bits() const noexcept {
    std::array<std::uint32_t, 9> result{};
    std::memcpy(result.data(), centergen_.data() + 1024u,
                result.size() * sizeof(result[0]));
    return result;
}

std::array<std::uint32_t, 9> Am4hpCenterFront::debug_state_before_bits() const noexcept {
    return state_before_;
}

std::array<std::uint32_t, 27> Am4hpCenterFront::debug_control_bits() const noexcept {
    std::array<std::uint32_t, 27> result{};
    std::memcpy(result.data(), centergen_.data() + 952u,
                result.size() * sizeof(result[0]));
    return result;
}

std::array<std::uint32_t, 27> Am4hpCenterFront::debug_control_before_bits() const noexcept {
    return control_before_;
}

std::array<std::uint32_t, 266> Am4hpCenterFront::debug_processor_bits() const noexcept {
    std::array<std::uint32_t, 266> result{};
    std::memcpy(result.data(), centergen_.data(), result.size() * sizeof(result[0]));
    return result;
}

std::array<float, 4> Am4hpCenterFront::debug_energy_sums() const noexcept {
    std::array<float, 4> result{};
    const auto* ring = reinterpret_cast<const float*>(centergen_.data() + 440u);
    for (std::size_t channel = 0; channel != result.size(); ++channel)
        for (std::size_t index = 0; index != 16u; ++index)
            result[channel] += ring[channel * 32u + index * 2u];
    return result;
}

float Am4hpCenterFront::Biquad::process(float x) noexcept {
    const float y = coefficients[0] * x + z1;
    z1 = coefficients[1] * x + z2 - coefficients[3] * y;
    z2 = coefficients[2] * x - coefficients[4] * y;
    return y;
}

bool Am4hpCenterFront::construct(std::uint32_t sample_rate,
                                 std::uint32_t layout_mask,
                                 float tuning,
                                 const Coefficients& first_coefficients,
                                 const Coefficients& second_coefficients) noexcept {
    if ((sample_rate != 44100u && sample_rate != 48000u) || !std::isfinite(tuning))
        return false;
    for (float v : first_coefficients)
        if (!std::isfinite(v)) return false;
    for (float v : second_coefficients)
        if (!std::isfinite(v)) return false;

    std::int32_t init[2]{static_cast<std::int32_t>(sample_rate), 1};
    if (::auro3deng::auro_centergen_v3_Processor_t_construct(
            centergen_.data(), init) == 0u)
        return false;

    // Exact CenterFront fixed payload from sub_54DB00:
    // uint64_t 1, then uint32_t 524288400 at byte 16.
    std::array<std::uint8_t, 20> fixed{};
    const std::uint64_t fixed_word0 = 1u;
    const std::uint32_t fixed_word4 = 524288400u;
    std::memcpy(fixed.data(), &fixed_word0, sizeof(fixed_word0));
    std::memcpy(fixed.data() + 16u, &fixed_word4, sizeof(fixed_word4));
    (void)::auro3deng::auro_centergen_v3_Processor_set_fixed_parameters(
        centergen_.data(), fixed.data());

    // Exact 56-byte dynamic payload assembled by sub_54DB00.  The four
    // constants are the little-endian words of xmmword_1DCC70 and
    // xmmword_1DBD70 in the selected x86_64 decompilation.
    std::array<std::uint8_t, 56> dynamic{};
    const float dynamic_words0[4]{0.0f, 0.8f, 0.2f, -15.0f};
    const float dynamic_words1[4]{0.0f, 1.0f, 0.05f, 0.5f};
    const float dynamic_words2[2]{9.0f, 0.99f};
    std::memcpy(dynamic.data() + 4u, dynamic_words0, sizeof(dynamic_words0));
    std::memcpy(dynamic.data() + 20u, &tuning, sizeof(tuning));
    std::memcpy(dynamic.data() + 24u, dynamic_words1, sizeof(dynamic_words1));
    std::memcpy(dynamic.data() + 40u, dynamic_words2, sizeof(dynamic_words2));
    (void)::auro3deng::auro_centergen_v3_Processor_set_dynamic_parameters(
        centergen_.data(), dynamic.data());

    first_.coefficients = first_coefficients;
    second_.coefficients = second_coefficients;
    use_external_center_ = (layout_mask & (1u << 2u)) != 0u;
    configured_ = true;
    first_.reset();
    second_.reset();
    last_generated_.fill(0.0f);
    state_before_.fill(0u);
    control_before_.fill(0u);
    return true;
}

bool Am4hpCenterFront::set_gain_db(float gain_db) noexcept {
    if (!configured_ || !std::isfinite(gain_db))
        return false;
    first_gain_ = gain_db - 6.0f > -144.0f
        ? std::pow(10.0f, (gain_db - 6.0f) * 0.050000001f) : 0.0f;
    second_gain_ = gain_db - 7.0f > -144.0f
        ? std::pow(10.0f, (gain_db - 7.0f) * 0.050000001f) : 0.0f;
    return true;
}

void Am4hpCenterFront::reset_audio_state() noexcept {
    (void)::auro3deng::auro_centergen_v3_Processor_reset_audio_state(centergen_.data());
    first_.reset();
    second_.reset();
    last_generated_.fill(0.0f);
    state_before_.fill(0u);
    control_before_.fill(0u);
}

bool Am4hpCenterFront::process(Block& input0,
                               Block& input1,
                               const Block* external_center,
                               Block& scaled_center,
                               Block& filtered) noexcept {
    if (!configured_ || (use_external_center_ && !external_center))
        return false;
    state_before_ = debug_state_bits();
    control_before_ = debug_control_bits();
    // Core has already made the caller-owned work-pair copy.  Native
    // CenterFront mutates this pair in place and Upmixing consumes it next.
    Block generated{};
    if (::auro3deng::auro_centergen_v3_Processor_process(
            centergen_.data(),
            reinterpret_cast<std::uint64_t>(input0.data()),
            reinterpret_cast<std::uint64_t>(input1.data()),
        reinterpret_cast<std::uint8_t*>(generated.data()), 32) != 0)
        return false;
    last_generated_ = generated;
    last_mutated_input0_ = input0;
    last_mutated_input1_ = input1;

    const Block& center = use_external_center_ ? *external_center : generated;
    for (std::size_t i = 0; i != 32u; ++i) {
        scaled_center[i] = center[i] * second_gain_;
        filtered[i] = first_.process(center[i] * first_gain_);
        filtered[i] = second_.process(filtered[i]);
    }
    return true;
}

}
