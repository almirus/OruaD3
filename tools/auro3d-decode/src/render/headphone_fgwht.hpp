#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace auro3d {

// Stateful frequency-generalized Walsh-Hadamard transform used by HPv2
// LateReverb. Coefficients transition linearly over one 32-sample quantum.
class HeadphoneFgwhtSmoothed {
public:
    HeadphoneFgwhtSmoothed() noexcept;

    void initialize(float angle) noexcept;
    void set_state(const std::array<float, 4>& state) noexcept { state_ = state; }
    const std::array<float, 4>& state() const noexcept { return state_; }

    bool process(std::vector<std::array<float, 32>>& blocks,
                 float target_angle) noexcept;

private:
    static void transform_constant(
        std::vector<std::array<float, 32>>& blocks,
        std::size_t offset, std::size_t count,
        float cosine, float sine) noexcept;
    static void transform_smoothed(
        std::vector<std::array<float, 32>>& blocks,
        std::size_t offset, std::size_t count,
        const std::array<float, 32>& cosines,
        const std::array<float, 32>& sines) noexcept;

    std::array<float, 4> state_{};
};

}
