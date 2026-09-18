#pragma once

#include <array>
#include <cstdint>

namespace auro3d {

// The native AM4HP CenterFront stage processes one 32-sample quantum. The
// Centergen state is kept separately from the two post filters so reset and
// parameter updates have the same lifetime as the native object.
class Am4hpCenterFront {
public:
    using Block = std::array<float, 32>;
    using Coefficients = std::array<float, 5>;

    bool construct(std::uint32_t sample_rate,
                   std::uint32_t layout_mask,
                   float tuning,
                   const Coefficients& first_coefficients,
                   const Coefficients& second_coefficients) noexcept;
    bool set_gain_db(float gain_db) noexcept;
    void reset_audio_state() noexcept;

    // input_pair is the native two-plane Centergen in/out pair and is mutated
    // in place. If
    // external_center is non-null, native bit 2 selects it instead of the
    // generated center. scaled_center is the native a5 output; filtered is
    // the native a4 output after both stateful biquads.
    bool process(Block& input0,
                 Block& input1,
                 const Block* external_center,
                 Block& scaled_center,
                 Block& filtered) noexcept;
    const Block& last_generated() const noexcept { return last_generated_; }
    const Block& last_mutated_input0() const noexcept { return last_mutated_input0_; }
    const Block& last_mutated_input1() const noexcept { return last_mutated_input1_; }
    std::array<std::uint32_t, 9> debug_state_bits() const noexcept;
    std::array<std::uint32_t, 9> debug_state_before_bits() const noexcept;
    std::array<std::uint32_t, 27> debug_control_bits() const noexcept;
    std::array<std::uint32_t, 27> debug_control_before_bits() const noexcept;
    std::array<std::uint32_t, 266> debug_processor_bits() const noexcept;
    std::array<float, 4> debug_energy_sums() const noexcept;

private:
    struct Biquad {
        Coefficients coefficients{};
        float z1 = 0.0f;
        float z2 = 0.0f;
        void reset() noexcept { z1 = 0.0f; z2 = 0.0f; }
        float process(float x) noexcept;
    } first_, second_;
    Block last_generated_{};
    Block last_mutated_input0_{};
    Block last_mutated_input1_{};
    std::array<std::uint32_t, 9> state_before_{};
    std::array<std::uint32_t, 27> control_before_{};

    alignas(16) std::array<std::uint8_t, 0x428u> centergen_{};
    bool use_external_center_ = false;
    float first_gain_ = 1.0f;
    float second_gain_ = 1.0f;
    bool configured_ = false;
};

}
