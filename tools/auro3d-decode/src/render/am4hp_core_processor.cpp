#include "am4hp_core_processor.hpp"

#include "am4hp_center_front_profiles.hpp"
#include "am4hp_low_end_profiles.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace auro3d {

bool Am4hpCoreProcessor::construct(std::uint32_t sample_rate,
                                    std::uint32_t layout_mask,
                                    float output_gain,
                                    std::string* error) noexcept {
    auto fail = [error](const char* message) noexcept {
        if (error) *error = message;
        return false;
    };
    // These are the two native Core layouts currently covered end-to-end:
    // direct stereo and the 5.0 bed selected from 5.0.2H PCM.
    if (sample_rate != 48000u || (layout_mask != 3u && layout_mask != 0x37u))
        return fail("AM4HP Core currently requires native 48-kHz layout 0x3 or 0x37");
    if (!std::isfinite(output_gain) || output_gain < 0.0f)
        return fail("AM4HP Core output gain is invalid");
    std::string stage_error;
    // Native stereo AM4HP static headroom is -10 dB (0.31622777 linear), as
    // confirmed by the Core+1392 object and the live stage34 capture.
    std::vector<unsigned> headroom_planes;
    for (unsigned slot = 0u; slot != 6u; ++slot)
        if ((layout_mask & (1u << slot)) != 0u)
            headroom_planes.push_back(slot);
    const auto low_end_coefficients = am4hp_native_low_end_48000_coefficients();
    if (!headroom_.construct(headroom_planes, 0.31622776601683794f)
        || !low_end_.construct(low_end_coefficients.first,
                               low_end_coefficients.second, layout_mask))
        return fail("AM4HP Core headroom/low-end construction failed");
    if (!construct_am4hp_native_center_front_48000(center_front_, layout_mask))
        return fail("AM4HP Core center-front construction failed");
    const bool surround = (layout_mask & 0x30u) == 0x30u;
    if (surround && !construct_am4hp_native_center_surround_48000(center_surround_))
        return fail("AM4HP Core center-surround construction failed");
    if (!upmixing_.construct(surround ? 0x33u : 3u, stage_error))
        return fail("AM4HP Core upmixing construction failed");
    const std::vector<unsigned> generated_planes = surround
        ? std::vector<unsigned>{9u, 10u, 13u, 14u}
        : std::vector<unsigned>{4u, 5u, 9u, 10u, 13u, 14u};
    if (!upmix_gain_.construct(generated_planes,
                               std::vector<float>(generated_planes.size(), 1.0f)))
        return fail("AM4HP Core upmix-gain construction failed");
    if (!linear_fader_.construct(2400u)
        || !compressor_.construct(sample_rate))
        return fail("AM4HP Core fader/compressor construction failed");
    if (!dynamic_state_.initialize(am4hp_native_dynamic_48000_default()))
        return fail("AM4HP Core dynamic-state construction failed");

    sample_rate_ = sample_rate;
    layout_mask_ = layout_mask;
    output_gain_ = output_gain;
    configured_ = true;
    constructed_ = true;
    applied_preset_ = dynamic_state_.parameters().preset;
    pending_preset_ = applied_preset_;
    ready_preset_ = applied_preset_;
    reset_audio_state();
    return true;
}

void Am4hpCoreProcessor::reset_dynamic_processing_state() noexcept {
    low_end_.reset_audio_state();
    center_front_.reset_audio_state();
    center_surround_.reset_audio_state();
    upmixing_.reset_audio_state();
    upmix_gain_.reset_audio_state();
    compressor_.reset();
    // CoreProcessor_force_dynamic_parameters_ resets this stage a second
    // time after the rest of the graph in the selected native build.
    upmix_gain_.reset_audio_state();
}

void Am4hpCoreProcessor::reset_audio_state() noexcept {
    if (!configured_)
        return;
    reset_dynamic_processing_state();
    linear_fader_.reset();
    auto parameters = dynamic_state_.parameters();
    parameters.preset = applied_preset_;
    (void)dynamic_state_.initialize(parameters);
    pending_preset_ = applied_preset_;
    ready_preset_ = applied_preset_;
    preset_pending_ = false;
    renderer_reconfigure_ready_ = false;
    seed_hp_dynamic_tuning(applied_preset_);
    apply_hp_dynamic_tuning(true);
    for (auto& block : planes_) block.fill(0.0f);
    center_.fill(0.0f);
    renderer_front_left_.fill(0.0f);
    renderer_front_right_.fill(0.0f);
    zero_.fill(0.0f);
    low_end_center_zero_.fill(0.0f);
    low_end_lfe_zero_.fill(0.0f);
    low_end_height_left_zero_.fill(0.0f);
    low_end_height_right_zero_.fill(0.0f);
    low_end_center_previous_.fill(0.0f);
    low_end_secondary_previous_.fill(0.0f);
    upmix_45_left_.fill(0.0f);
    upmix_45_right_.fill(0.0f);
    surround_left_.fill(0.0f);
    surround_right_.fill(0.0f);
    surround_center_.fill(0.0f);
    height_left_.fill(0.0f);
    height_right_.fill(0.0f);
    height_surround_left_.fill(0.0f);
    height_surround_right_.fill(0.0f);
    accumulated_left_.fill(0.0f);
    accumulated_right_.fill(0.0f);
    debug_snapshot_ = {};
    renderer_pending_ = false;
}

bool Am4hpCoreProcessor::set_preset(
    std::uint32_t preset, std::string* error) noexcept {
    auto fail = [error](const char* message) noexcept {
        if (error) *error = message;
        return false;
    };
    if (!constructed_)
        return fail("AM4HP Core is not constructed");
    auto parameters = dynamic_state_.parameters();
    parameters.preset = preset;
    Am4hpDynamicUpdatePlan plan;
    if (!dynamic_state_.update(
            parameters, true, linear_fader_.get_state(), &plan))
        return fail("invalid AM4HP Core preset");
    if (!plan.preset_changed)
        return true;
    pending_preset_ = preset;
    preset_pending_ = true;
    if (plan.action == Am4hpDynamicUpdateAction::start_linear_fade)
        linear_fader_.set_direction(1u);
    return true;
}

void Am4hpCoreProcessor::hp_dynamic_targets(
    std::uint32_t preset,
    float& renderer_gain_db,
    float& front_gain_db,
    float& late_rt60,
    float& early_distance_db,
    float& late_output_db,
    float& height_eq_db,
    float& late_shelf_coeff,
    float& core_output_db) const noexcept {
    const bool surround = (layout_mask_ & 0x30u) == 0x30u;
    if (preset == 0u) {
        renderer_gain_db = 3.0f;
        front_gain_db = surround ? 2.0f : 0.0f;
        late_rt60 = 0.300000011920929f;
        early_distance_db = -6.0f;
        late_output_db = -6.0f;
        height_eq_db = surround ? 0.0f : 1.0f;
        late_shelf_coeff = 0.699999988079071f;
        core_output_db = surround ? 2.5f : 0.5f;
        return;
    }
    if (preset == 1u) {
        renderer_gain_db = 5.5f;
        front_gain_db = surround ? 3.0f : 1.0f;
        late_rt60 = 0.5f;
        early_distance_db = -7.0f;
        late_output_db = -14.0f;
        height_eq_db = surround ? -1.0f : 0.0f;
        late_shelf_coeff = 0.600000023841858f;
        core_output_db = surround ? 3.0f : 1.0f;
        return;
    }
    if (preset == 3u) {
        renderer_gain_db = 10.0f;
        front_gain_db = surround ? 5.0f : 3.0f;
        late_rt60 = 1.0f;
        early_distance_db = -9.0f;
        late_output_db = -25.0f;
        height_eq_db = surround ? -5.0f : -3.0f;
        late_shelf_coeff = 0.699999988079071f;
        core_output_db = surround ? 4.0f : 2.0f;
        return;
    }
    renderer_gain_db = 8.5f;
    front_gain_db = surround ? 4.0f : 2.0f;
    late_rt60 = 0.800000011920929f;
    early_distance_db = -8.0f;
    late_output_db = -20.0f;
    height_eq_db = surround ? -3.0f : -2.0f;
    late_shelf_coeff = 0.800000011920929f;
    core_output_db = surround ? 3.5f : 1.5f;
}

void Am4hpCoreProcessor::seed_hp_dynamic_tuning(
    std::uint32_t preset) noexcept {
    hp_dynamic_targets(preset, smoothed_renderer_gain_db_,
                       smoothed_front_gain_db_, smoothed_late_rt60_,
                       smoothed_early_distance_db_,
                       smoothed_late_output_db_, smoothed_height_eq_db_,
                       smoothed_late_shelf_coeff_, smoothed_core_output_db_);
    const bool surround = (layout_mask_ & 0x30u) == 0x30u;
    smoothed_upmix_scale_db_ = (preset < 2u ? 9.0f : 8.0f)
        + (surround ? 1.0f : 0.0f);
    static const float kLowEndDb[] = {4.0f, 5.0f, 5.5f, 6.0f};
    smoothed_low_end_db_ = kLowEndDb[preset];
}

void Am4hpCoreProcessor::apply_hp_dynamic_tuning(bool snap) noexcept {
    if (!constructed_ && !configured_)
        return;
    const float coeff = snap ? 1.0f : hp_smoothing_coeff_;
    float renderer_target = 8.5f;
    float front_target = 2.0f;
    float late_target = 0.800000011920929f;
    float early_target = -8.0f;
    float late_output_target = -20.0f;
    float height_eq_target = -2.0f;
    float late_shelf_target = 0.800000011920929f;
    float core_output_target = 1.5f;
    hp_dynamic_targets(dynamic_state_.parameters().preset, renderer_target,
                       front_target, late_target, early_target,
                       late_output_target, height_eq_target, late_shelf_target,
                       core_output_target);
    smoothed_renderer_gain_db_ +=
        (renderer_target - smoothed_renderer_gain_db_) * coeff;
    const bool surround = (layout_mask_ & 0x30u) == 0x30u;
    const float upmix_scale_target =
        (dynamic_state_.parameters().preset < 2u ? 9.0f : 8.0f)
        + (surround ? 1.0f : 0.0f);
    smoothed_upmix_scale_db_ +=
        (upmix_scale_target - smoothed_upmix_scale_db_) * coeff;
    smoothed_front_gain_db_ +=
        (front_target - smoothed_front_gain_db_) * coeff;
    smoothed_late_rt60_ += (late_target - smoothed_late_rt60_) * coeff;
    smoothed_early_distance_db_ +=
        (early_target - smoothed_early_distance_db_) * coeff;
    smoothed_late_output_db_ +=
        (late_output_target - smoothed_late_output_db_) * coeff;
    smoothed_height_eq_db_ +=
        (height_eq_target - smoothed_height_eq_db_) * coeff;
    smoothed_late_shelf_coeff_ +=
        (late_shelf_target - smoothed_late_shelf_coeff_) * coeff;
    smoothed_core_output_db_ +=
        (core_output_target - smoothed_core_output_db_) * coeff;
    static const float kLowEndDb[] = {4.0f, 5.0f, 5.5f, 6.0f};
    const float low_end_target =
        kLowEndDb[dynamic_state_.parameters().preset];
    smoothed_low_end_db_ += (low_end_target - smoothed_low_end_db_) * coeff;
    renderer_gain_ = am4hp_output_gain_linear(smoothed_renderer_gain_db_);
    if (smoothed_low_end_db_ == 5.5f) {
        const std::uint32_t native_low_end_bits = 0x3ff11b6au;
        std::memcpy(&native_low_end_scale_, &native_low_end_bits,
                    sizeof(native_low_end_scale_));
    } else {
        native_low_end_scale_ = am4hp_output_gain_linear(smoothed_low_end_db_);
    }
    if (smoothed_core_output_db_ == 1.5f) {
        const std::uint32_t native_core_bits = 0x3f9820d7u;
        std::memcpy(&native_core_gain_, &native_core_bits,
                    sizeof(native_core_gain_));
    } else if (smoothed_core_output_db_ == 3.5f) {
        const std::uint32_t native_core_bits = 0x3fbf84a6u;
        std::memcpy(&native_core_gain_, &native_core_bits,
                    sizeof(native_core_gain_));
    } else {
        native_core_gain_ = am4hp_output_gain_linear(smoothed_core_output_db_);
    }
    (void)center_front_.set_gain_db(smoothed_front_gain_db_);
    (void)upmixing_.set_height_eq_gain_db(smoothed_height_eq_db_);
}

float Am4hpCoreProcessor::hp_early_distance_gain_scale() const noexcept {
    static const float kBankDb[] = {-6.0f, -7.0f, -8.0f, -9.0f};
    const float bank_db = kBankDb[applied_preset_];
    const float bank_linear = am4hp_output_gain_linear(bank_db);
    if (bank_linear == 0.0f)
        return 0.0f;
    return am4hp_output_gain_linear(smoothed_early_distance_db_) / bank_linear;
}

float Am4hpCoreProcessor::hp_late_output_gain_scale() const noexcept {
    static const float kBankDb[] = {-6.0f, -14.0f, -20.0f, -25.0f};
    const float bank_db = kBankDb[applied_preset_];
    const float bank_linear = am4hp_output_gain_linear(bank_db);
    if (bank_linear == 0.0f)
        return 0.0f;
    return am4hp_output_gain_linear(smoothed_late_output_db_) / bank_linear;
}

bool Am4hpCoreProcessor::take_renderer_reconfigure(
    std::uint32_t& preset) noexcept {
    if (!renderer_reconfigure_ready_)
        return false;
    preset = ready_preset_;
    renderer_reconfigure_ready_ = false;
    return true;
}

bool Am4hpCoreProcessor::prepare_renderer_input(
    const PhysicalBlocks& physical, RendererInput& renderer_input) noexcept {
    if (!constructed_ || renderer_pending_)
        return false;
    apply_hp_dynamic_tuning(false);
    std::vector<const Block*> input(31u, nullptr);
    std::vector<Block*> output(31u, nullptr);
    for (unsigned slot = 0u; slot != 6u; ++slot) {
        if ((layout_mask_ & (1u << slot)) == 0u)
            continue;
        if (!physical[slot])
            return false;
        input[slot] = physical[slot];
        output[slot] = &planes_[slot];
    }
    if (!headroom_.process(input, output))
        return false;
    debug_snapshot_.headroom_left = planes_[0];
    debug_snapshot_.headroom_right = planes_[1];

    std::array<Block*, 8> low_end_planes{};
    // Native Headroom writes the front pair into local work buffers, then
    // passes those same buffers to LowEnd. Core copies that result into the
    // CenterFront work pair; its in-place result becomes Upmixing's input.
    low_end_planes[2] = &planes_[0];
    low_end_planes[3] = &planes_[1];
    low_end_planes[4] = (layout_mask_ & 4u) ? &planes_[2]
                                           : &low_end_center_zero_;
    low_end_planes[5] = &low_end_lfe_zero_;
    low_end_planes[6] = (layout_mask_ & 0x10u) ? &planes_[4]
                                               : &low_end_height_left_zero_;
    low_end_planes[7] = (layout_mask_ & 0x20u) ? &planes_[5]
                                               : &low_end_height_right_zero_;
    if (!low_end_.process(low_end_planes, accumulated_left_, accumulated_right_))
        return false;
    // This is a distinct native stage gain, not the final Core output gain.
    // It is applied to the two LowEnd accumulators before CenterFront adds
    // its scaled center and before HPv2 Renderer output is accumulated.
    for (std::size_t i = 0u; i != 32u; ++i) {
        accumulated_left_[i] *= native_low_end_scale_;
        accumulated_right_[i] *= native_low_end_scale_;
    }
    debug_snapshot_.low_end_left = accumulated_left_;
    debug_snapshot_.low_end_right = accumulated_right_;
    debug_snapshot_.low_end_state_bits = low_end_.debug_state_bits();
    debug_snapshot_.center_input_left = planes_[0];
    debug_snapshot_.center_input_right = planes_[1];
    renderer_front_left_ = planes_[0];
    renderer_front_right_ = planes_[1];

    Block scaled_center{};
    const Block* external_center = (layout_mask_ & 4u) ? &planes_[2] : nullptr;
    if (!center_front_.process(planes_[0], planes_[1], external_center,
                               scaled_center, center_))
        return false;
    debug_snapshot_.center_scaled = scaled_center;
    debug_snapshot_.center_generated = center_front_.last_generated();
    debug_snapshot_.center_mutated_left = center_front_.last_mutated_input0();
    debug_snapshot_.center_mutated_right = center_front_.last_mutated_input1();
    debug_snapshot_.center_state_bits = center_front_.debug_state_bits();
    debug_snapshot_.center_state_before_bits = center_front_.debug_state_before_bits();
    debug_snapshot_.center_control_bits = center_front_.debug_control_bits();
    debug_snapshot_.center_control_before_bits =
        center_front_.debug_control_before_bits();
    debug_snapshot_.center_processor_bits = center_front_.debug_processor_bits();
    debug_snapshot_.center_energy_sums = center_front_.debug_energy_sums();
    debug_snapshot_.center_filtered = center_;
    for (std::size_t i = 0u; i < 32u; ++i) {
        accumulated_left_[i] += scaled_center[i];
        accumulated_right_[i] += scaled_center[i];
    }
    low_end_center_previous_ = center_;

    // Native Upmixing copies descriptor front planes into local v14/v15
    // buffers and passes a compact stereo XinN vector [v14, v15]. XinN's
    // stereo input mask therefore consumes indices 0/1, not Core descriptor
    // slots 2/3.
    Block xinn_front_left = planes_[0];
    Block xinn_front_right = planes_[1];
    // Native profile-dependent Upmixing gain at Core+572.
    float native_upmix_scale = 0.0f;
    if (smoothed_upmix_scale_db_ == 8.0f) {
        const std::uint32_t native_upmix_scale_bits = 0x4020c2bfu;
        std::memcpy(&native_upmix_scale, &native_upmix_scale_bits,
                    sizeof(native_upmix_scale));
    } else if (smoothed_upmix_scale_db_ == 9.0f) {
        const std::uint32_t native_upmix_scale_bits = 0x40346063u;
        std::memcpy(&native_upmix_scale, &native_upmix_scale_bits,
                    sizeof(native_upmix_scale));
    } else {
        native_upmix_scale = am4hp_output_gain_linear(smoothed_upmix_scale_db_);
    }
    for (std::size_t i = 0u; i != 32u; ++i) {
        xinn_front_left[i] *= native_upmix_scale;
        xinn_front_right[i] *= native_upmix_scale;
    }
    std::array<void*, 31> spans{};
    spans[0] = xinn_front_left.data();
    spans[1] = xinn_front_right.data();
    const bool surround = (layout_mask_ & 0x30u) == 0x30u;
    Block xinn_surround_left{};
    Block xinn_surround_right{};
    if (surround) {
        // As with the external-center front pair, native keeps the original
        // SL/SR work pair for HPv2. CenterSurround mutates only local copies
        // which are then scaled and consumed by XinN's second Engine1.
        surround_left_ = planes_[4];
        surround_right_ = planes_[5];
        xinn_surround_left = surround_left_;
        xinn_surround_right = surround_right_;
        if (!center_surround_.process(xinn_surround_left,
                                      xinn_surround_right,
                                      surround_center_))
            return false;
        for (std::size_t sample = 0u; sample != 32u; ++sample) {
            xinn_surround_left[sample] *= native_upmix_scale;
            xinn_surround_right[sample] *= native_upmix_scale;
        }
        spans[4] = xinn_surround_left.data();
        spans[5] = xinn_surround_right.data();
    } else {
        spans[4] = upmix_45_left_.data();
        spans[5] = upmix_45_right_.data();
    }
    spans[9] = height_left_.data();
    spans[10] = height_right_.data();
    spans[13] = height_surround_left_.data();
    spans[14] = height_surround_right_.data();
    if (!upmixing_.process(spans.data(),
                           {&height_left_, &height_right_},
                           {&height_surround_left_, &height_surround_right_}))
        return false;
    // With an external center plane native Core keeps the original front
    // work pair for HPv2 and lets CenterFront mutate only a separate pair
    // consumed by XinN.  It scales that preserved renderer pair by +2 dB
    // after XinN returns (0x54ACD6..0x54AD04).
    if ((layout_mask_ & 4u) != 0u) {
        const std::uint32_t native_front_scale_bits = 0x3fa12478u;
        float native_front_scale = 0.0f;
        std::memcpy(&native_front_scale, &native_front_scale_bits,
                    sizeof(native_front_scale));
        for (std::size_t sample = 0u; sample != 32u; ++sample) {
            renderer_front_left_[sample] *= native_front_scale;
            renderer_front_right_[sample] *= native_front_scale;
        }
        if (surround) {
            for (std::size_t sample = 0u; sample != 32u; ++sample) {
                surround_left_[sample] *= native_front_scale;
                surround_right_[sample] *= native_front_scale;
            }
        }
    }
    debug_snapshot_.raw_height_left = upmixing_.raw_hl_left();
    debug_snapshot_.raw_height_right = upmixing_.raw_hl_right();
    debug_snapshot_.raw_height_surround_left = upmixing_.raw_hls_left();
    debug_snapshot_.raw_height_surround_right = upmixing_.raw_hls_right();
    std::vector<Block*> gain_planes(31u, nullptr);
    gain_planes[4] = surround ? &surround_left_ : &upmix_45_left_;
    gain_planes[5] = surround ? &surround_right_ : &upmix_45_right_;
    gain_planes[9] = &height_left_;
    gain_planes[10] = &height_right_;
    gain_planes[13] = &height_surround_left_;
    gain_planes[14] = &height_surround_right_;
    if (!upmix_gain_.process(gain_planes))
        return false;
    debug_snapshot_.upmix_left = upmix_45_left_;
    debug_snapshot_.upmix_right = upmix_45_right_;
    low_end_secondary_previous_ = upmix_45_left_;

    renderer_input = {(layout_mask_ & 4u) ? &renderer_front_left_ : &planes_[0],
                      (layout_mask_ & 4u) ? &renderer_front_right_ : &planes_[1],
                      &center_,
                      surround ? &surround_left_ : &upmix_45_left_,
                      surround ? &surround_right_ : &upmix_45_right_,
                      &height_left_, &height_right_,
                      &height_surround_left_, &height_surround_right_};
    for (std::size_t i = 0u; i != renderer_input.size(); ++i)
        debug_snapshot_.renderer_input[i] = *renderer_input[i];
    renderer_pending_ = true;
    return true;
}

bool Am4hpCoreProcessor::finish_renderer_output(
    const Block& renderer_left, const Block& renderer_right,
    Block& output_left, Block& output_right) noexcept {
    if (!constructed_ || !renderer_pending_)
        return false;
    for (std::size_t i = 0u; i < 32u; ++i) {
        accumulated_left_[i] += renderer_left[i] * renderer_gain_;
        accumulated_right_[i] += renderer_right[i] * renderer_gain_;
    }
    debug_snapshot_.accumulated_before_core_gain_left = accumulated_left_;
    debug_snapshot_.accumulated_before_core_gain_right = accumulated_right_;
    // Native sub_54BB80 applies DynamicProcessing+144 (smoothed PresetManager
    // v10) immediately before the linear fader/compressor stage.
    const float core_output_gain =
        dynamic_state_.output_gain_linear() * native_core_gain_;
    for (std::size_t i = 0u; i < 32u; ++i) {
        accumulated_left_[i] *= core_output_gain;
        accumulated_right_[i] *= core_output_gain;
    }
    std::array<float, 32> fader_gains{};
    if ((linear_fader_.get_state() & 0xFFFFFFFEu) == 2u) {
        if (!linear_fader_.next_gains(fader_gains))
            return false;
        for (std::size_t i = 0u; i < 32u; ++i) {
            accumulated_left_[i] *= fader_gains[i];
            accumulated_right_[i] *= fader_gains[i];
        }
        // The last fade-down block still contains output from the old
        // renderer. Native then forces the requested tuning, resets the
        // dependent state and reverses the fader before its compressor runs;
        // the rebuilt renderer is first used by the following quantum.
        if (linear_fader_.get_state() == 1u) {
            apply_hp_dynamic_tuning(true);
            if (preset_pending_ && pending_preset_ != applied_preset_) {
                reset_dynamic_processing_state();
                applied_preset_ = pending_preset_;
                ready_preset_ = applied_preset_;
                renderer_reconfigure_ready_ = true;
            }
            preset_pending_ = false;
            linear_fader_.set_direction(0u);
        }
    }
    debug_snapshot_.accumulated_before_compressor_left = accumulated_left_;
    debug_snapshot_.accumulated_before_compressor_right = accumulated_right_;
    if (!compressor_.process(accumulated_left_, accumulated_right_))
        return false;
    for (std::size_t i = 0u; i < 32u; ++i) {
        output_left[i] = std::clamp(accumulated_left_[i], -1.0f, 1.0f);
        output_right[i] = std::clamp(accumulated_right_[i], -1.0f, 1.0f);
    }
    renderer_pending_ = false;
    return true;
}

} // namespace auro3d
