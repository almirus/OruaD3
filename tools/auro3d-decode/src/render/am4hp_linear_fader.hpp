#pragma once

#include <array>
#include <cstddef>

namespace auro3d {

// Stateful 32-sample gain ramp used by the native AM4HP LinearFader.
class Am4hpLinearFader {
public:
    bool construct(std::size_t length_samples) noexcept;
    bool set_length_samples(std::size_t length_samples) noexcept;
    void reset() noexcept;
    void set_direction(unsigned direction) noexcept;
    bool fading_done() const noexcept { return done_; }
    unsigned get_state() const noexcept;
    bool next_gains(std::array<float, 32>& gains) noexcept;

private:
    float current_ = 1.0f;
    float step_ = 0.0f;
    unsigned direction_ = 0u;
    bool done_ = true;
};

}
