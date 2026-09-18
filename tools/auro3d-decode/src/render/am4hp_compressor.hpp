#pragma once

#include <array>
#include <cstdint>

namespace auro3d {

// CoreProcessor's float32, linked, 32-sample compressor. This is kept
// separate from NativePeakLimiter: the two native call sites use different
// parameter vectors and therefore have different follower/gain behavior.
class Am4hpCompressor {
public:
    using Block = std::array<float, 32>;

    bool construct(std::uint32_t sample_rate) noexcept;
    void reset() noexcept;
    bool process(Block& left, Block& right) noexcept;

private:
    void update_coefficients() noexcept;
    void follow(const Block& peak, Block& envelope) noexcept;
    void compute_gains(const Block& envelope, Block& gains) noexcept;

    std::uint32_t sample_rate_ = 0;
    float attack_old_ = 0.0f;
    float attack_in_ = 1.0f;
    float release_in_ = 1.0f;
    float release_old_ = 0.0f;
    // Native float32 AM4HP compressor is configured in linked mode: one
    // peak follower and one reduction-gain vector are shared by both planes.
    float envelope_state_ = 0.0f;
    float previous_gain_ = 1.0f;
    float threshold_ = 1.0f;
    float inverse_threshold_ = 1.0f;
    float exponent_ = 0.0f;
    // Captured AM4HP profile scale. The compressor call has a separate
    // parameter; it is not represented by this field.
    float makeup_gain_ = 1.0f;
    bool constructed_ = false;
};

} // namespace auro3d
