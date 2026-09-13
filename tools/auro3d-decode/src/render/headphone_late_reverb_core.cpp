#include "headphone_late_reverb_core.hpp"

#include "am4hp_iir_biquad.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

namespace auro3d {
namespace {

float bits_to_float(std::uint32_t bits) noexcept {
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

template <std::size_t Count>
std::array<float, Count> bits_to_floats(
    const std::array<std::uint32_t, Count>& bits) noexcept {
    std::array<float, Count> values{};
    std::memcpy(values.data(), bits.data(), sizeof(values));
    return values;
}

float rounded_product(float first, float second) noexcept {
    volatile float product = first * second;
    return product;
}

float add_separate(float accumulator, float value) noexcept {
    volatile float sum = accumulator + value;
    return sum;
}

bool late_core_trace_enabled() noexcept {
    static const bool enabled = [] {
        const char* path = std::getenv("ORUA_AM4HP_LATE_CORE_TRACE_PATH");
        return path && *path;
    }();
    return enabled;
}

void write_late_core_trace(
    const std::array<float, 32>& input,
    const std::array<float, 32>& corrected_before_normalization,
    const std::array<float, 32>& corrected,
    const std::vector<std::array<float, 32>>& raw_delayed_blocks,
    const std::vector<std::array<float, 32>>& delayed_blocks,
    const std::vector<std::array<float, 32>>& band_input_blocks,
    const std::vector<std::array<float, 32>>& retained_blocks,
    const std::vector<std::array<float, 32>>& outputs) {
    static std::ofstream trace;
    static std::uint32_t call_index = 0u;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        const char* path = std::getenv("ORUA_AM4HP_LATE_CORE_TRACE_PATH");
        if (path && *path)
            trace.open(path, std::ios::binary | std::ios::trunc);
    }
    if (!trace)
        return;
    trace.write(reinterpret_cast<const char*>(&call_index), sizeof(call_index));
    trace.write(reinterpret_cast<const char*>(input.data()), sizeof(input));
    trace.write(reinterpret_cast<const char*>(corrected_before_normalization.data()),
                sizeof(corrected_before_normalization));
    trace.write(reinterpret_cast<const char*>(corrected.data()), sizeof(corrected));
    for (const auto& block : raw_delayed_blocks)
        trace.write(reinterpret_cast<const char*>(block.data()), sizeof(block));
    for (const auto& block : delayed_blocks)
        trace.write(reinterpret_cast<const char*>(block.data()), sizeof(block));
    for (const auto& block : band_input_blocks)
        trace.write(reinterpret_cast<const char*>(block.data()), sizeof(block));
    for (const auto& block : retained_blocks)
        trace.write(reinterpret_cast<const char*>(block.data()), sizeof(block));
    for (const auto& output : outputs)
        trace.write(reinterpret_cast<const char*>(output.data()), sizeof(output));
    ++call_index;
}

}

bool HeadphoneLateReverbCore::construct(
    const HeadphoneLateReverbCoreConfig& config) {
    if (config.sample_rate == 0u || (config.mode != 1u && config.mode != 2u)
        || config.output_layout != 3u || config.all_pass_count != 4u
        || (config.bands.size() != 8u && config.bands.size() != 16u)
        || !std::isfinite(bits_to_float(config.output_gain_f32_bits)))
        return false;

    HeadphoneMultiDelay input_delay;
    if (!input_delay.construct(config.input_delay_samples))
        return false;

    std::vector<HeadphoneLateReverbBand> bands;
    std::vector<float> feedback_gains;
    bands.reserve(config.bands.size());
    feedback_gains.reserve(config.bands.size());
    const auto damping = bits_to_floats(config.band_damping_filter_f32_bits);
    for (std::size_t index = 0u; index < config.bands.size(); ++index) {
        const auto& native = config.bands[index];
        if (native.slot != index
            || native.all_passes.size() != config.all_pass_count)
            return false;
        std::vector<std::size_t> delays;
        std::vector<float> coefficients;
        delays.reserve(native.all_passes.size());
        coefficients.reserve(native.all_passes.size());
        for (const auto& all_pass : native.all_passes) {
            delays.push_back(all_pass.delay_samples);
            coefficients.push_back(bits_to_float(
                all_pass.coefficient_f32_bits));
        }
        HeadphoneLateReverbBand band;
        if (!band.construct(native.delay_samples, delays, damping)
            || !band.set_allpass_coefficients(coefficients))
            return false;
        bands.push_back(std::move(band));
        feedback_gains.push_back(bits_to_float(
            native.feedback_gain_f32_bits));
    }

    sample_rate_ = config.sample_rate;
    input_filter_enabled_ = config.input_filter_enabled != 0u;
    constructed_output_gain_ = bits_to_float(config.output_gain_f32_bits);
    output_gain_ = constructed_output_gain_;
    modulation_depth_ = bits_to_float(config.modulation_depth_f32_bits);
    modulation_rate_ = bits_to_float(config.modulation_rate_f32_bits);
    modulation_phase_ = bits_to_float(config.modulation_phase_f32_bits);
    std::array<float, 5> input_filter_coefficients{};
    std::memcpy(input_filter_coefficients.data(),
                config.input_filter_f32_bits.data(),
                sizeof(input_filter_coefficients));
    input_filter_.construct(input_filter_coefficients);
    input_delay_ = std::move(input_delay);
    input_delay_samples_ = config.input_delay_samples;
    damping_correction_.construct(bits_to_floats(
        config.damping_correction_filter_f32_bits));
    bands_ = std::move(bands);
    feedback_gains_ = std::move(feedback_gains);
    fgwht_.set_state(bits_to_floats(config.fgwht_state_f32_bits));
    // Native LateReverb_set_dynamic writes comb feedback from +52 (RT60) and
    // does not rebuild allpass coefficients. Seed the last-applied RT60 from
    // the captured input delay (0.04 s → 1920, 0.05 s → 2400 at 48 kHz) so a
    // freshly swapped preset-3 bank is not rewritten on the first quantum.
    switch (config.input_delay_samples) {
    case 480u:  last_feedback_rt60_ = 0.300000011920929f; break;
    case 960u:  last_feedback_rt60_ = 0.5f; break;
    case 2400u: last_feedback_rt60_ = 1.0f; break;
    default:    last_feedback_rt60_ = 0.800000011920929f; break;
    }
    last_shelf_coeff_ = bits_to_float(config.damping_f32_bits);
    switch (config.input_delay_samples) {
    case 480u:  last_shelf_hz_ = 5000.0f; break;
    case 960u:  last_shelf_hz_ = 6000.0f; break;
    case 2400u: last_shelf_hz_ = 9000.0f; break;
    default:    last_shelf_hz_ = 8000.0f; break;
    }
    reset_audio_state();
    return true;
}

bool HeadphoneLateReverbCore::set_feedback_rt60(float rt60_seconds) noexcept {
    if (sample_rate_ == 0u || !(rt60_seconds > 0.0f)
        || !std::isfinite(rt60_seconds)
        || feedback_gains_.size() != bands_.size())
        return false;
    if (last_feedback_rt60_ == rt60_seconds)
        return true;
    const float rate = static_cast<float>(sample_rate_);
    for (std::size_t band = 0u; band != bands_.size(); ++band) {
        const float delay = static_cast<float>(
            static_cast<int>(bands_[band].feedback_delay_samples()));
        const float exponent =
            (delay * -3.0f) / (rate * rt60_seconds);
        // Native `LateReverb_set_dynamic_parameters` uses libm `pow` (double)
        // on that float32 exponent and only stores the comb gain.
        feedback_gains_[band] =
            static_cast<float>(std::pow(10.0, static_cast<double>(exponent)));
    }
    last_feedback_rt60_ = rt60_seconds;
    return true;
}

bool HeadphoneLateReverbCore::set_output_gain_scale(float scale) noexcept {
    if (!std::isfinite(scale))
        return false;
    output_gain_ = constructed_output_gain_ * scale;
    return true;
}

bool HeadphoneLateReverbCore::set_dynamic_shelf(
    float damping_coeff, float frequency_hz) noexcept {
    if (sample_rate_ == 0u || bands_.empty()
        || !std::isfinite(damping_coeff) || !(damping_coeff > 0.0f)
        || !std::isfinite(frequency_hz) || !(frequency_hz > 0.0f))
        return false;
    if (last_shelf_coeff_ == damping_coeff && last_shelf_hz_ == frequency_hz)
        return true;
    // Native LateReverb_set_dynamic_parameters uses type 12, Q=0.9, and
    // gain = coeff*-30 on the shared band damper / coeff*15 on +116.
    constexpr float kNativeType12Q = 0.8999999761581421f;
    std::array<float, 5> correction{};
    std::array<float, 5> band_damping{};
    if (!am4hp_iir_type12_shelf(frequency_hz, sample_rate_, kNativeType12Q,
                                damping_coeff * 15.0f, correction)
        || !am4hp_iir_type12_shelf(frequency_hz, sample_rate_, kNativeType12Q,
                                   damping_coeff * -30.0f, band_damping))
        return false;
    damping_correction_.set_coefficients(correction);
    for (auto& band : bands_)
        band.set_damping_coefficients(band_damping);
    last_shelf_coeff_ = damping_coeff;
    last_shelf_hz_ = frequency_hz;
    return true;
}

void HeadphoneLateReverbCore::reset_audio_state() noexcept {
    input_filter_.reset_audio_state();
    input_delay_.reset_audio_state();
    damping_correction_.reset_audio_state();
    for (auto& band : bands_)
        band.reset_audio_state();
    modulation_phase_ = 0.0f;
}

float HeadphoneLateReverbCore::next_target_angle() noexcept {
    constexpr double kPhaseStepScale = 201.0619298297468;
    constexpr double kHalfPi = 1.5707963267948966;
    constexpr double kPi = 3.1415926535897932;
    constexpr double kThreeHalfPi = 4.7123889803846899;
    constexpr double kTwoPi = 6.2831853071795865;
    constexpr float kBaseAngle = 0.39269909262657166f;
    double phase = static_cast<double>(modulation_rate_) * kPhaseStepScale
                 / static_cast<double>(sample_rate_)
                 + static_cast<double>(modulation_phase_);
    if (phase > kTwoPi)
        phase -= kTwoPi;
    modulation_phase_ = static_cast<float>(phase);
    const double quadrant = phase / kHalfPi;
    double triangle = quadrant;
    if (phase >= kHalfPi)
        triangle = phase >= kThreeHalfPi ? quadrant - 4.0 : 2.0 - quadrant;
    return kBaseAngle + modulation_depth_ * static_cast<float>(triangle);
}

bool HeadphoneLateReverbCore::process(
    const std::array<float, 32>& input,
    std::vector<std::array<float, 32>>& outputs) noexcept {
    if ((bands_.size() != 8u && bands_.size() != 16u)
        || feedback_gains_.size() != bands_.size())
        return false;
    outputs.assign(8u, {});
    const bool trace_enabled = late_core_trace_enabled();
    std::vector<std::array<float, 32>> blocks(bands_.size());
    std::vector<std::array<float, 32>> raw_delayed_blocks;
    std::vector<std::array<float, 32>> delayed_blocks;
    std::vector<std::array<float, 32>> band_input_blocks;
    std::vector<std::array<float, 32>> retained_blocks;
    if (trace_enabled) {
        raw_delayed_blocks.resize(bands_.size());
        delayed_blocks.resize(bands_.size());
        band_input_blocks.resize(bands_.size());
        retained_blocks.resize(bands_.size());
    }

    std::array<float, 32> filtered = input;
    if (input_filter_enabled_)
        input_filter_.process(input, filtered);
    input_delay_.add_buffer(filtered);
    std::array<float, 32> delayed{};
    if (!input_delay_.get_delay(input_delay_samples_, delayed))
        return false;
    std::array<float, 32> corrected{};
    damping_correction_.process(delayed, corrected);
    std::array<float, 32> corrected_before_normalization{};
    if (trace_enabled)
        corrected_before_normalization = corrected;
    // Native sub_56DF40 normalizes the corrected input by 1/sqrt(v7), where
    // v7 is the selected band count.  The legacy 16-band case is 0.25; AM4HP
    // uses the distinct 8-band coefficient 0x3eb504f3.
    const float normalization = bands_.size() == 8u
        ? bits_to_float(0x3eb504f3u) : 0.25f;
    for (std::size_t sample = 0u; sample < corrected.size(); ++sample)
        corrected[sample] = rounded_product(corrected[sample], normalization);

    for (std::size_t band = 0u; band < bands_.size(); ++band) {
        if (!bands_[band].mix_delayed_feedback(feedback_gains_[band],
                                               blocks[band],
                                               trace_enabled
                                                   ? &raw_delayed_blocks[band]
                                                   : nullptr))
            return false;
        // Native layout 3 uses qword_1DB0D0[3] == 7 as a binary-tree
        // routing mask, so the band index is masked rather than truncated
        // to its low three bits.
        auto& output = outputs[7u & band];
        for (std::size_t sample = 0u; sample < output.size(); ++sample)
            output[sample] = add_separate(output[sample], blocks[band][sample]);
    }
    if (trace_enabled)
        delayed_blocks = blocks;

    if (!fgwht_.process(blocks, next_target_angle()))
        return false;
    for (std::size_t band = 0u; band < bands_.size(); ++band) {
        for (std::size_t sample = 0u; sample < corrected.size(); ++sample)
            blocks[band][sample] = add_separate(blocks[band][sample],
                                                 corrected[sample]);
        if (trace_enabled)
            band_input_blocks[band] = blocks[band];
        std::array<float, 32> retained{};
        if (!bands_[band].process(blocks[band], retained))
            return false;
        if (trace_enabled)
            retained_blocks[band] = retained;
    }
    for (auto& output : outputs)
        for (float& sample : output)
            sample = rounded_product(sample, output_gain_);
    if (trace_enabled)
        write_late_core_trace(input, corrected_before_normalization, corrected,
                              raw_delayed_blocks, delayed_blocks,
                              band_input_blocks, retained_blocks, outputs);
    return true;
}

}
