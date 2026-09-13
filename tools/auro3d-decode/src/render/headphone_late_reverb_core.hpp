#pragma once

#include "headphone_fgwht.hpp"
#include "headphone_late_reverb_band.hpp"
#include "headphone_multi_delay.hpp"
#include "headphone_pca_bank.hpp"
#include "headphone_wall_material_filter.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace auro3d {

// Portable HPv2 LateReverb core recovered from sub_56DF40. Both the legacy
// AHP mode-2/16-band and AM4HP mode-1/8-band configurations use this native
// processing body; the configuration selects the band count.
class HeadphoneLateReverbCore {
public:
    bool construct(const HeadphoneLateReverbCoreConfig& config);
    void reset_audio_state() noexcept;
    bool process(const std::array<float, 32>& input,
                 std::vector<std::array<float, 32>>& outputs) noexcept;

    std::size_t output_count() const noexcept { return 8u; }
    std::size_t band_count() const noexcept { return bands_.size(); }
    bool set_feedback_rt60(float rt60_seconds) noexcept;
    bool set_output_gain_scale(float scale) noexcept;
    bool set_dynamic_shelf(float damping_coeff, float frequency_hz) noexcept;

private:
    float next_target_angle() noexcept;

    unsigned sample_rate_ = 0u;
    bool input_filter_enabled_ = false;
    float constructed_output_gain_ = 0.0f;
    float output_gain_ = 0.0f;
    float modulation_depth_ = 0.0f;
    float modulation_rate_ = 0.0f;
    float modulation_phase_ = 0.0f;
    HeadphoneWallMaterialFilter input_filter_;
    HeadphoneMultiDelay input_delay_;
    std::size_t input_delay_samples_ = 0u;
    HeadphoneWallMaterialFilter damping_correction_;
    std::vector<HeadphoneLateReverbBand> bands_;
    std::vector<float> feedback_gains_;
    HeadphoneFgwhtSmoothed fgwht_;
    // Captured banks already store comb gains for the preset RT60. Seed from
    // input delay in construct so the first matching set_feedback_rt60 is a
    // no-op. Native Late set_dynamic never rebuilds allpass stages.
    float last_feedback_rt60_ = 0.800000011920929f;
    float last_shelf_coeff_ = 0.800000011920929f;
    float last_shelf_hz_ = 8000.0f;
};

}
