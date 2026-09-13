#pragma once

#include "am4hp_equalizer.hpp"
#include "am4hp_xinn_runtime.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace auro3d {

// AM4HP Upmixing stage owner. XinN and its post-EQ filters are stateful;
// plane selection is supplied by CoreProcessor until the full native output
// descriptor mapping is wired.
class Am4hpUpmixing {
public:
    using Block = Am4hpEqualizer::Block;
    using StereoBlocks = std::array<Block*, 2>;

    bool construct(std::uint32_t input_mask, std::string& error);
    bool process(void** channel_span_31,
                 const StereoBlocks& hl_hr,
                 const StereoBlocks& hls_hrs) noexcept;
    const Block& raw_hl_left() const noexcept { return raw_hl_left_; }
    const Block& raw_hl_right() const noexcept { return raw_hl_right_; }
    const Block& raw_hls_left() const noexcept { return raw_hls_left_; }
    const Block& raw_hls_right() const noexcept { return raw_hls_right_; }
    void reset_audio_state() noexcept;

    bool constructed() const noexcept { return constructed_; }
    bool surround() const noexcept { return surround_; }
    bool set_height_eq_gain_db(float gain_db) noexcept;
    const Am4hpXinnRuntime& xinn() const noexcept { return xinn_; }

private:
    Am4hpXinnRuntime xinn_;
    Am4hpEqualizer hl_hr_equalizer_;
    Am4hpEqualizer hls_hrs_equalizer_;
    bool surround_ = false;
    bool constructed_ = false;
    Block raw_hl_left_{};
    Block raw_hl_right_{};
    Block raw_hls_left_{};
    Block raw_hls_right_{};
};

}
