#include "headphone_multi_delay.hpp"

#include <algorithm>

namespace auro3d {

bool HeadphoneMultiDelay::construct(std::size_t delay_samples) {
    // Native: v4 = delay + 30 (or delay - 1 for delay >= 1), then round
    // down to a 32-sample boundary; storage length is v5 + 96 samples.
    const std::size_t rounded_base = delay_samples == 0u
        ? 30u
        : delay_samples - 1u;
    const std::size_t rounded = rounded_base & ~std::size_t(31u);
    capacity_ = rounded + 64u;
    try {
        buffer_.assign(capacity_ + 32u, 0.0f);
    } catch (...) {
        buffer_.clear();
        capacity_ = 0;
        return false;
    }
    configured_delay_ = delay_samples;
    reset_audio_state();
    return true;
}

void HeadphoneMultiDelay::reset_audio_state() noexcept {
    if (buffer_.empty())
        return;
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    write_cursor_ = 32u;
    available_ = 0u;
}

void HeadphoneMultiDelay::add_buffer(const std::array<float, 32>& input) noexcept {
    if (buffer_.empty())
        return;
    std::copy(input.begin(), input.end(), buffer_.begin() + write_cursor_);
    if (write_cursor_ == 0u)
        std::copy(input.begin(), input.end(), buffer_.begin() + capacity_);
    write_cursor_ += 32u;
    if (write_cursor_ == capacity_)
        write_cursor_ = 0u;
    if (available_ < capacity_)
        available_ += 32u;
}

bool HeadphoneMultiDelay::get_delay(std::size_t delay_samples,
                                    std::array<float, 32>& output) const noexcept {
    if (buffer_.empty() || configured_delay_ < delay_samples) {
        output.fill(0.0f);
        return false;
    }
    if (available_ < delay_samples) {
        output.fill(0.0f);
        return true;
    }
    std::size_t position = write_cursor_;
    if (position >= delay_samples + 32u)
        position -= delay_samples;
    else
        position = position + capacity_ - delay_samples;
    const std::size_t source = position - 32u;
    std::copy_n(buffer_.begin() + source, 32u, output.begin());
    return true;
}

bool HeadphoneMultiDelay::get_delay_before_add(int delay_samples,
                                                std::array<float, 32>& output) const noexcept {
    if (delay_samples < 32) {
        output.fill(0.0f);
        return false;
    }
    return get_delay(static_cast<std::size_t>(delay_samples - 32), output);
}

}
