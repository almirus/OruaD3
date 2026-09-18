#include "headphone_source_manager.hpp"
#include "headphone_pca_accumulator.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace auro3d {
namespace {

float bits_to_float(std::uint32_t bits) noexcept {
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

std::vector<float> bits_to_floats(
    const std::vector<std::uint32_t>& bits) {
    std::vector<float> values(bits.size());
    if (!bits.empty())
        std::memcpy(values.data(), bits.data(), bits.size() * sizeof(float));
    return values;
}

void add_separate(const std::array<float, 32>& input,
                  std::array<float, 32>& output) noexcept {
    for (std::size_t sample = 0u; sample < output.size(); ++sample)
        output[sample] += input[sample];
}

float rounded_product(float first, float second) noexcept {
    volatile float product = first * second;
    return product;
}

float rounded_sum(float first, float second) noexcept {
    volatile float sum = first + second;
    return sum;
}

}

bool HeadphoneSourceManager::construct(const HeadphonePcaBank& bank) {
    if (bank.source_manager_configs().size() != 1u
        || bank.late_reverb_core_configs().size() != 1u)
        return false;
    const auto& config = bank.source_manager_configs().front();
    const bool ahp_profile = config.source_count == 11u
        && config.early_reflection_count == 66u;
    const bool am4hp_profile = config.source_count == 9u
        && (config.early_reflection_count == 18u
            || config.early_reflection_count == 30u
            || config.early_reflection_count == 42u
            || config.early_reflection_count == 54u);
    const bool supported_rate = config.sample_rate == 32000u
        || config.sample_rate == 44100u || config.sample_rate == 48000u
        || config.sample_rate == 88200u || config.sample_rate == 96000u;
    if (!supported_rate || (!ahp_profile && !am4hp_profile)
        || config.late_reverb_source_count != 8u
        || config.pca_score_count != 8u
        || config.reflection_enabled != 1u || config.reflection_mode != 0u
        || config.late_reverb_disabled != 0u
        || config.pca_filter_order_index != 9u
        || config.first_pca_count != 8u || config.second_pca_count != 6u
        || config.late_input_gain_f32_bits.size() != config.source_count
        || config.wall_material_enabled.size() != config.source_count
        || config.wall_first_coefficients_f32_bits.size() != config.source_count
        || config.wall_second_coefficients_f32_bits.size() != config.source_count)
        return false;

    std::vector<ExplicitSource> explicit_sources;
    explicit_sources.reserve(bank.explicit_validation_vectors().size());
    for (const auto& native : bank.explicit_validation_vectors()) {
        const std::uint32_t explicit_base = am4hp_profile ? 9u : 11u;
        if (native.slot < explicit_base
            || native.slot >= explicit_base + config.source_count
            || native.mode != 0u
            || native.score_count != config.pca_score_count)
            return false;
        ExplicitSource source;
        source.native_slot = native.slot;
        source.input_slot = native.slot - explicit_base;
        source.first_gains = bits_to_floats(native.first_pca_gain_f32_bits);
        source.second_gains = bits_to_floats(native.second_pca_gain_f32_bits);
        if (!source.path.construct(native.distance_delay_samples,
                                   native.scaled_itd_samples,
                                   bits_to_float(native.distance_gain_f32_bits),
                                   true))
            return false;
        explicit_sources.push_back(std::move(source));
    }
    if (explicit_sources.size() != (am4hp_profile ? 9u : 11u))
        return false;
    std::sort(explicit_sources.begin(), explicit_sources.end(),
              [](const ExplicitSource& first, const ExplicitSource& second) {
                  return first.native_slot < second.native_slot;
              });

    std::vector<EarlySource> early_sources;
    early_sources.reserve(bank.early_reflection_validation_vectors().size());
    for (const auto& native : bank.early_reflection_validation_vectors()) {
        const std::uint32_t early_base = am4hp_profile
            ? config.early_reflection_count : 66u;
        const std::uint32_t early_count = config.early_reflection_count;
        if (native.slot < early_base
            || native.slot >= early_base + early_count || native.mode != 0u
            || native.diffusion_enabled > 1u
            || native.manager_score_count != config.pca_score_count)
            return false;
        EarlySource source;
        source.native_slot = native.slot;
        source.input_slot = (native.slot - early_base) / 6u;
        // The stereo preset-3 room contains Manager sources 0..2 and 5..8.
        // Builder packs six reflections per contained source, so groups from
        // the fourth onward skip the non-contained source records 3 and 4.
        if (config.early_reflection_count == 42u && source.input_slot >= 3u)
            source.input_slot += 2u;
        source.first_gains = bits_to_floats(native.first_pca_gain_f32_bits);
        source.second_gains = bits_to_floats(native.second_pca_gain_f32_bits);
        if (!source.path.construct(native.distance_delay_samples,
                                   native.scaled_itd_samples,
                                   bits_to_float(native.distance_gain_f32_bits),
                                   native.all_pass_delay_samples))
            return false;
        if (am4hp_profile) {
            const float reference_db = config.early_reflection_count >= 42u
                ? -9.0f : -8.0f;
            if (!source.path.set_gain_reference_linear(
                    std::pow(10.0f, reference_db * 0.050000001f)))
                return false;
            // Native source_ER_set_dynamic_parameters always rebuilds owner
            // +116 as the geometry gain multiplied by its linear parameter.
            // The packed vectors contain an already-rounded preset gain, so
            // recover geometry from the retained Q23 distance instead of
            // repeatedly associating that rounded value with a ratio.
            constexpr float kInvQ23 = 0.00000011920929f;
            volatile float meters =
                static_cast<float>(native.distance_q23) * kInvQ23;
            const float geometry_gain = 1.0f /
                (meters > 0.1f ? static_cast<float>(meters) : 0.1f);
            if (!source.path.set_dynamic_geometry_gain(geometry_gain))
                return false;
        }
        if (native.all_pass_delay_samples != 0u
            && !source.path.set_diffusion_coefficient(bits_to_float(
                native.all_pass_coefficient_f32_bits)))
            return false;
        early_sources.push_back(std::move(source));
    }
    if (early_sources.size() != config.early_reflection_count)
        return false;
    std::sort(early_sources.begin(), early_sources.end(),
              [](const EarlySource& first, const EarlySource& second) {
                  return first.native_slot < second.native_slot;
              });

    std::vector<HeadphoneWallMaterialFilter> wall_first_filters(
        config.source_count);
    std::vector<HeadphoneWallMaterialFilter> wall_second_filters(
        config.source_count);
    for (std::size_t source = 0u; source < config.source_count; ++source) {
        if (config.wall_material_enabled[source] != 0u) {
            std::array<float, 5> first{}, second{};
            for (std::size_t coefficient = 0u; coefficient < 5u; ++coefficient) {
                first[coefficient] = bits_to_float(
                    config.wall_first_coefficients_f32_bits[source][coefficient]);
                second[coefficient] = bits_to_float(
                    config.wall_second_coefficients_f32_bits[source][coefficient]);
            }
            wall_first_filters[source].construct(first);
            wall_second_filters[source].construct(second);
        }
    }

    HeadphoneLateReverbCore late_core;
    if (!late_core.construct(bank.late_reverb_core_configs().front()))
        return false;
    std::vector<LateSource> late_sources;
    late_sources.reserve(bank.late_reverb_validation_vectors().size());
    for (const auto& native : bank.late_reverb_validation_vectors()) {
        if (native.slot != late_sources.size()
            || native.manager_score_count != config.pca_score_count)
            return false;
        LateSource source;
        source.first_gains = bits_to_floats(native.first_pca_gain_f32_bits);
        source.second_gains = bits_to_floats(native.second_pca_gain_f32_bits);
        if (!source.path.construct(native.pca_itd_samples, native.score_count))
            return false;
        late_sources.push_back(std::move(source));
    }
    if (late_sources.size() != 8u)
        return false;

    const auto order_it = std::find(bank.filter_orders().begin(),
                                    bank.filter_orders().end(),
                                    static_cast<std::uint8_t>(
                                        config.pca_filter_order_index));
    if (order_it == bank.filter_orders().end())
        return false;
    const auto order_index = static_cast<std::uint32_t>(
        std::distance(bank.filter_orders().begin(), order_it));

    std::vector<HeadphoneWiir> first_filters(config.pca_score_count);
    std::vector<HeadphoneWiir> second_filters(config.pca_score_count);
    for (std::size_t score = 0u; score < config.pca_score_count; ++score) {
        const auto* first = bank.filter(config.sample_rate,
                                        static_cast<std::uint32_t>(score),
                                        0u, order_index);
        const auto* second = bank.filter(config.sample_rate,
                                         static_cast<std::uint32_t>(score),
                                         1u, order_index);
        if (!first || !second
            || !first_filters[score].construct_from_filter(*first)
            || !second_filters[score].construct_from_filter(*second))
            return false;
    }

    source_count_ = config.source_count;
    score_count_ = config.pca_score_count;
    sample_rate_ = config.sample_rate;
    late_input_gains_ = bits_to_floats(config.late_input_gain_f32_bits);
    wall_material_enabled_ = config.wall_material_enabled;
    wall_first_filters_ = std::move(wall_first_filters);
    wall_second_filters_ = std::move(wall_second_filters);
    explicit_sources_ = std::move(explicit_sources);
    early_sources_ = std::move(early_sources);
    late_core_ = std::move(late_core);
    late_sources_ = std::move(late_sources);
    first_filters_ = std::move(first_filters);
    second_filters_ = std::move(second_filters);
    am4hp_profile_ = am4hp_profile;
    if (!rebuild_am4hp_shared_delays())
        return false;
    reset_audio_state();
    return true;
}

bool HeadphoneSourceManager::set_late_feedback_rt60(
    float rt60_seconds) noexcept {
    return late_core_.set_feedback_rt60(rt60_seconds);
}

bool HeadphoneSourceManager::set_early_distance_gain_scale(
    float scale) noexcept {
    for (auto& source : early_sources_) {
        if (!source.path.set_distance_gain_scale(scale))
            return false;
    }
    return true;
}

bool HeadphoneSourceManager::set_early_distance_gain_db(
    float gain_db) noexcept {
    for (auto& source : early_sources_) {
        if (!source.path.set_distance_gain_from_db(gain_db))
            return false;
    }
    return true;
}

bool HeadphoneSourceManager::rebuild_am4hp_shared_delays() {
    am4hp_input_delays_.clear();
    am4hp_wall_input_delays_.clear();
    if (!am4hp_profile_)
        return true;
    std::vector<std::size_t> maximum_delays(source_count_, 0u);
    for (const auto& source : early_sources_) {
        if (source.input_slot >= source_count_)
            return false;
        const std::size_t itd = static_cast<std::size_t>(
            source.path.itd_delay_samples() < 0
                ? -static_cast<std::int64_t>(source.path.itd_delay_samples())
                : source.path.itd_delay_samples());
        maximum_delays[source.input_slot] = std::max(
            maximum_delays[source.input_slot],
            source.path.distance_delay_samples() + itd);
    }
    am4hp_input_delays_.resize(source_count_);
    am4hp_wall_input_delays_.resize(source_count_);
    for (std::size_t source = 0u; source < source_count_; ++source) {
        if (!am4hp_input_delays_[source].construct(maximum_delays[source]))
            return false;
        if (wall_material_enabled_[source] != 0u
            && !am4hp_wall_input_delays_[source].construct(
                maximum_delays[source]))
            return false;
    }
    return true;
}

bool HeadphoneSourceManager::set_late_output_gain_scale(
    float scale) noexcept {
    return late_core_.set_output_gain_scale(scale);
}

bool HeadphoneSourceManager::set_late_dynamic_shelf(
    float damping_coeff, float frequency_hz) noexcept {
    return late_core_.set_dynamic_shelf(damping_coeff, frequency_hz);
}

std::vector<float> HeadphoneSourceManager::debug_early_distance_gains() const {
    std::vector<float> gains;
    gains.reserve(early_sources_.size());
    for (const auto& source : early_sources_)
        gains.push_back(source.path.distance_gain());
    return gains;
}

void HeadphoneSourceManager::reset_audio_state() noexcept {
    for (auto& source : explicit_sources_)
        source.path.reset_audio_state();
    for (auto& source : early_sources_)
        source.path.reset_audio_state();
    for (auto& delay : am4hp_input_delays_)
        delay.reset_audio_state();
    for (auto& delay : am4hp_wall_input_delays_)
        delay.reset_audio_state();
    for (auto& filter : wall_first_filters_)
        filter.reset_audio_state();
    for (auto& filter : wall_second_filters_)
        filter.reset_audio_state();
    late_core_.reset_audio_state();
    for (auto& source : late_sources_)
        source.path.reset_audio_state();
    for (auto& filter : first_filters_)
        filter.reset_audio_state();
    for (auto& filter : second_filters_)
        filter.reset_audio_state();
}

bool HeadphoneSourceManager::process_renderer_input(
    const RendererInput& inputs,
    std::array<float, 32>& first_output,
    std::array<float, 32>& second_output) noexcept {
    if (source_count_ != inputs.size())
        return false;
    std::vector<std::array<float, 32>> copied;
    copied.reserve(inputs.size());
    for (const auto* input : inputs) {
        if (!input)
            return false;
        copied.push_back(*input);
    }
    return process(copied, first_output, second_output);
}

bool HeadphoneSourceManager::process_renderer_descriptor(
    const RendererInputDescriptor& descriptor,
    std::array<float, 32>& first_output,
    std::array<float, 32>& second_output) noexcept {
    if (descriptor.frame_count != 32u
        || descriptor.sample_rate != sample_rate_)
        return false;
    return process_renderer_input(
        descriptor.planes, first_output, second_output);
}

bool HeadphoneSourceManager::process(
    const std::vector<std::array<float, 32>>& inputs,
    std::array<float, 32>& first_output,
    std::array<float, 32>& second_output) noexcept {
    if (inputs.size() != source_count_ || score_count_ == 0u)
        return false;
    // Native adds the filtered Manager contribution to
    // both existing output planes. Renderer_process_float_32_ invokes LFE
    // first, so clearing here would discard the mono LFE seed.
    first_scores_.assign(score_count_, {});
    second_scores_.assign(score_count_, {});
    std::vector<std::array<float, 32>> wall_inputs(inputs.begin(), inputs.end());
    for (std::size_t source = 0u; source < inputs.size(); ++source) {
        if (wall_material_enabled_[source] != 0u) {
            std::array<float, 32> filtered{};
            wall_first_filters_[source].process(inputs[source], filtered);
            wall_inputs[source] = filtered;
        }
        if (am4hp_profile_) {
            // Native adds raw input before WallMaterial, then stores
            // the filtered result in a separate delay for enabled sources.
            am4hp_input_delays_[source].add_buffer(inputs[source]);
            if (wall_material_enabled_[source] != 0u)
                am4hp_wall_input_delays_[source].add_buffer(wall_inputs[source]);
        }
        // Native processes each explicit source immediately after
        // its input/delay preparation, before entering the ER loop.
        for (auto& explicit_source : explicit_sources_) {
            if (explicit_source.input_slot != source)
                continue;
            if (!explicit_source.path.process_with_pca_gains(
                    inputs[explicit_source.input_slot],
                    explicit_source.first_gains, explicit_source.second_gains,
                    first_scores_, second_scores_))
                return false;
            break;
        }
    }
    if (debug_capture_early_scores_) {
        debug_explicit_first_scores_ = first_scores_;
        debug_explicit_second_scores_ = second_scores_;
        debug_early_contributions_.clear();
    }
    for (auto& source : early_sources_) {
        std::array<float, 32> first{}, second{};
        const bool processed = am4hp_profile_
            ? source.path.process_from_shared_delay(
                  wall_material_enabled_[source.input_slot] != 0u
                      ? am4hp_wall_input_delays_[source.input_slot]
                      : am4hp_input_delays_[source.input_slot],
                  first, second)
            : source.path.process(wall_inputs[source.input_slot], first, second);
        if (!processed)
            return false;
        for (std::size_t score = 0u; score < source.first_gains.size(); ++score) {
            HeadphonePcaAccumulator::add(first, source.first_gains[score],
                                          first_scores_[score]);
            HeadphonePcaAccumulator::add(second, source.second_gains[score],
                                          second_scores_[score]);
        }
        if (debug_capture_early_scores_) {
            debug_early_contributions_.push_back({source.native_slot, first, second,
                                                  source.path.debug_pre_gain_first(),
                                                  source.path.debug_pre_gain_second(),
                                                  first_scores_, second_scores_});
        }
    }
    if (debug_capture_early_scores_) {
        debug_early_first_scores_ = first_scores_;
        debug_early_second_scores_ = second_scores_;
    }

    std::array<float, 32> late_input{};
    for (std::size_t source = 0u; source < inputs.size(); ++source) {
        for (std::size_t sample = 0u; sample < late_input.size(); ++sample)
            late_input[sample] = rounded_sum(
                late_input[sample],
                rounded_product(inputs[source][sample],
                                late_input_gains_[source]));
    }
    std::vector<std::array<float, 32>> late_outputs;
    if (!late_core_.process(late_input, late_outputs)
        || late_outputs.size() != late_sources_.size())
        return false;
    for (std::size_t source = 0u; source < late_sources_.size(); ++source) {
        if (!late_sources_[source].path.process(
                late_outputs[source], late_sources_[source].first_gains,
                late_sources_[source].second_gains,
                first_scores_, second_scores_))
            return false;
    }

    for (std::size_t score = 0u; score < score_count_; ++score) {
        std::array<float, 32> first_filtered{}, second_filtered{};
        if (!first_filters_[score].process_pair(
                first_scores_[score], second_scores_[score], first_filtered,
                second_filtered, second_filters_[score]))
            return false;
        add_separate(first_filtered, first_output);
        add_separate(second_filtered, second_output);
    }
    return true;
}

}
