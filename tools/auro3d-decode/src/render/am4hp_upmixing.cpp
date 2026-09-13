#include "am4hp_upmixing.hpp"

#include "am4hp_equalizer_profiles.hpp"


namespace auro3d {

bool Am4hpUpmixing::construct(
    std::uint32_t input_mask, std::string& error) {
    if (!xinn_.construct(input_mask, error))
        return false;
    surround_ = input_mask == Am4hpInputPresets::kSurroundInputMask;
    const auto hl_hr_profile = surround_
        ? am4hp_native_surround_eq_hl_hr_48000()
        : am4hp_native_eq_hl_hr_48000();
    const auto hls_hrs_profile = surround_
        ? am4hp_native_surround_eq_hls_hrs_48000()
        : am4hp_native_eq_hls_hrs_48000();
    if (!hl_hr_equalizer_.construct(hl_hr_profile, 2u)) {
        error = "AM4HP HL/HR equalizer construction failed";
        return false;
    }
    if (!hls_hrs_equalizer_.construct(hls_hrs_profile, 2u)) {
        error = "AM4HP HLS/HRS equalizer construction failed";
        return false;
    }
    constructed_ = true;
    return true;
}

bool Am4hpUpmixing::process(
    void** channel_span_31,
    const StereoBlocks& hl_hr,
    const StereoBlocks& hls_hrs) noexcept {
    if (!constructed_)
        return false;
    if (!xinn_.process(channel_span_31)) return false;
    raw_hl_left_ = hl_hr[0] ? *hl_hr[0] : Block{};
    raw_hl_right_ = hl_hr[1] ? *hl_hr[1] : Block{};
    raw_hls_left_ = hls_hrs[0] ? *hls_hrs[0] : Block{};
    raw_hls_right_ = hls_hrs[1] ? *hls_hrs[1] : Block{};
    if (!hl_hr_equalizer_.process(hl_hr))
        return false;
    return hls_hrs_equalizer_.process(hls_hrs);
}

bool Am4hpUpmixing::set_height_eq_gain_db(float gain_db) noexcept {
    if (!constructed_)
        return false;
    return hl_hr_equalizer_.set_band0_gain_db(gain_db)
        && hls_hrs_equalizer_.set_band0_gain_db(gain_db);
}


void Am4hpUpmixing::reset_audio_state() noexcept {
    xinn_.reset_audio_state();
    hl_hr_equalizer_.reset_audio_state();
    hls_hrs_equalizer_.reset_audio_state();
}

}
