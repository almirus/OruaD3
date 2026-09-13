#include "am4hp_linear_fader.hpp"

namespace auro3d {

bool Am4hpLinearFader::construct(std::size_t length_samples) noexcept {
    reset();
    return set_length_samples(length_samples);
}

bool Am4hpLinearFader::set_length_samples(
    std::size_t length_samples) noexcept {
    if (length_samples == 0u)
        return false;
    const std::size_t rounded = (length_samples + 31u) & ~std::size_t(31u);
    step_ = 1.0f / static_cast<float>(rounded - 1u);
    return true;
}

void Am4hpLinearFader::reset() noexcept {
    current_ = 1.0f;
    direction_ = 0u;
    done_ = true;
}

void Am4hpLinearFader::set_direction(unsigned direction) noexcept {
    const unsigned normalized = direction ? 1u : 0u;
    if (direction_ != normalized) {
        direction_ = normalized;
        done_ = false;
    }
}

unsigned Am4hpLinearFader::get_state() const noexcept {
    if (done_)
        return direction_ ? 1u : 0u;
    // Native CoreProcessor enters the ramp when (state & ~1) == 2:
    // direction 0 maps to state 2 and direction 1 to state 3.
    return direction_ ? 3u : 2u;
}

bool Am4hpLinearFader::next_gains(
    std::array<float, 32>& gains) noexcept {
    if (done_)
        return false;
    const float delta = direction_ ? -step_ : step_;
    gains[0] = current_;
    for (std::size_t i = 1u; i < gains.size(); ++i) {
        current_ += delta;
        gains[i] = current_;
    }
    current_ += delta;
    const float lookahead = direction_ ? (current_ - step_ * 30.0f)
                                       : (current_ + step_ * 30.0f);
    if ((!direction_ && lookahead > 1.0f)
        || (direction_ && lookahead < 0.0f)) {
        done_ = true;
        current_ = direction_ ? 0.0f : 1.0f;
    }
    return true;
}

}
