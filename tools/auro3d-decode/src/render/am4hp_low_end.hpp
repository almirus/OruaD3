#pragma once

#include <array>
#include <cstdint>

namespace auro3d {

class Am4hpLowEnd {
public:
    using Block = std::array<float, 32>;
    using Coefficients = std::array<float, 5>;

    // Coefficients use the native order b0,b1,b2,a1,a2. The flag bits are
    // the native CoreProcessor LowEnd configuration bits.
    bool construct(const Coefficients& mid_coefficients,
                   const Coefficients& final_coefficients,
                   std::uint32_t flags) noexcept;
    void reset_audio_state() noexcept;
    std::array<std::uint32_t, 8> debug_state_bits() const noexcept;

    // planes are indexed by the native 31-plane descriptor; this stage reads
    // planes 2..7, mutates filtered source planes like native, and produces
    // the two accumulated output planes.
    bool process(const std::array<Block*, 8>& planes,
                 Block& first_output,
                 Block& second_output) noexcept;

private:
    struct Biquad {
        Coefficients c{};
        float z1 = 0.0f;
        float z2 = 0.0f;

        void reset() noexcept { z1 = 0.0f; z2 = 0.0f; }
        float process(float input) noexcept;
    };

    bool configured_ = false;
    std::uint32_t flags_ = 0u;
    Biquad mid_first_, mid_second_, mid_center_, mid_height_first_, mid_height_second_;
    Biquad final_first_, final_second_;
};

}
