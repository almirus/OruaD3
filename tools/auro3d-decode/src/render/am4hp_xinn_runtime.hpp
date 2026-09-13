#pragma once

#include "am4hp_input_presets.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace auro3d {

// Stateful AM4HP Upmixing XinN owner. This is the direct native v3 XinN
// object used by CoreProcessor's Upmixing stage, separate from the
// Manager-facing AuromaticV3UpmixRuntime.
class Am4hpXinnRuntime {
public:
    static constexpr std::uint32_t kStereoOutputMask = 26160u;
    static constexpr std::uint32_t kSurroundOutputMask = 26112u;

    bool construct(std::uint32_t input_mask, std::string& error);
    bool process(void** channel_span_31) noexcept;
    void reset_audio_state() noexcept;

    bool initialized() const noexcept { return initialized_; }
    std::uint32_t input_mask() const noexcept { return input_mask_; }
    std::uint32_t output_mask() const noexcept { return output_mask_; }
    const std::vector<std::uint8_t>& preset_backing() const noexcept {
        return preset_;
    }

private:
    std::vector<std::uint8_t> xinn_state_;
    std::vector<std::uint8_t> arena_;
    std::vector<std::uint8_t> preset_;
    std::uint32_t input_mask_ = 0u;
    std::uint32_t output_mask_ = 0u;
    bool initialized_ = false;
};

}
