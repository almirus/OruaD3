#pragma once

#include <array>

namespace auro3d {

// Native WallMaterial_Filter_process is a 32-sample transposed biquad. The
// coefficient order is b0, b1, b2, a1, a2 as exposed by WallMaterial_print.
class HeadphoneWallMaterialFilter {
public:
    void construct(const std::array<float, 5>& coefficients) noexcept;
    void set_coefficients(const std::array<float, 5>& coefficients) noexcept;
    // Native WallMaterial_t_construct derives a type-3 IIR from the two
    // material corner frequencies before installing its five coefficients.
    bool construct_from_wall_material(float high_pass_hz,
                                      float low_pass_hz,
                                      unsigned sample_rate) noexcept;
    void reset_audio_state() noexcept;
    void process(const std::array<float, 32>& input,
                 std::array<float, 32>& output) noexcept;

private:
    std::array<float, 5> coefficients_{};
    float state0_ = 0.0f;
    float state1_ = 0.0f;
};

}
