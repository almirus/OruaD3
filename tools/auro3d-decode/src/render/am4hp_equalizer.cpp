#include "am4hp_equalizer.hpp"

#include "am4hp_iir_biquad.hpp"

#include <cmath>

namespace auro3d {

namespace {
bool valid_bands(const std::array<Am4hpEqualizer::Band, 3>& bands) noexcept {
    for (const auto& band : bands) {
        if (!std::isfinite(band.gain_db)) return false;
        for (float coefficient : band.coefficients)
            if (!std::isfinite(coefficient)) return false;
    }
    return true;
}
}

bool Am4hpEqualizer::construct(const std::array<Band, 3>& bands,
                               std::size_t channel_count) noexcept {
    if (channel_count == 0u || channel_count > 2u) return false;
    if (!valid_bands(bands)) return false;
    bands_ = bands;
    band_count_ = bands_.size();
    channel_count_ = channel_count;
    reset_audio_state();
    configured_ = true;
    return true;
}

bool Am4hpEqualizer::update(const std::array<Band, 3>& bands) noexcept {
    if (!configured_ || !valid_bands(bands)) return false;
    bands_ = bands;
    return true;
}

bool Am4hpEqualizer::set_band0_gain_db(float gain_db) noexcept {
    if (!configured_ || !std::isfinite(gain_db))
        return false;
    // Native Upmixing EQ band 0 is type 7 / 6000 Hz / Q=1; set_dynamic writes
    // PresetManager v24 into update_non_muting without resetting filter state.
    if (bands_[0].gain_db == gain_db)
        return true;
    std::array<float, 5> coefficients{};
    if (!am4hp_iir_type7_highshelf(6000.0f, 48000u, 1.0f, gain_db, coefficients))
        return false;
    bands_[0].coefficients = coefficients;
    bands_[0].gain_db = gain_db;
    return true;
}

void Am4hpEqualizer::reset_audio_state() noexcept {
    for (auto& band_states : states_)
        for (State& state : band_states) { state.z1 = 0.0f; state.z2 = 0.0f; }
}

bool Am4hpEqualizer::process(const std::array<Block*, 2>& channels) noexcept {
    if (!configured_) return false;
    for (std::size_t channel = 0u; channel < channel_count_; ++channel)
        if (!channels[channel]) return false;
    // Native Equalizer_process_audio walks each channel through the complete
    // cascade before moving to the next channel.  The filter state is
    // channel-local, but preserving this order is required for exact native
    // block behavior and state transitions.
    for (std::size_t channel = 0u; channel < channel_count_; ++channel) {
        for (std::size_t band_index = 0u; band_index < band_count_; ++band_index) {
            const Coefficients& c = bands_[band_index].coefficients;
            State& state = states_[band_index][channel];
            for (float& sample : *channels[channel]) {
                const float output = c[0] * sample + state.z1;
                state.z1 = c[1] * sample + state.z2 - c[3] * output;
                state.z2 = c[2] * sample - c[4] * output;
                sample = output;
            }
        }
    }
    return true;
}

}
