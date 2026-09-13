#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace auro3d {

// Native auro_headphones_v2_MultiDelay operates on 32-sample float blocks.
// The extra mirrored block is part of its ring-buffer addressing contract.
class HeadphoneMultiDelay {
public:
    bool construct(std::size_t delay_samples);
    void reset_audio_state() noexcept;
    void add_buffer(const std::array<float, 32>& input) noexcept;
    bool get_delay(std::size_t delay_samples,
                   std::array<float, 32>& output) const noexcept;
    bool get_delay_before_add(int delay_samples,
                              std::array<float, 32>& output) const noexcept;

    std::size_t configured_delay() const noexcept { return configured_delay_; }

private:
    std::vector<float> buffer_;
    std::size_t configured_delay_ = 0;
    std::size_t capacity_ = 0;
    std::size_t write_cursor_ = 32;
    std::size_t available_ = 0;
};

}
