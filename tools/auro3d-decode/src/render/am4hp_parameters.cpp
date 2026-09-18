#include "am4hp_parameters.hpp"

#include <cmath>
#include <cstring>

namespace auro3d {

std::uint32_t check_am4hp_static_parameters(
    const Am4hpStaticParameters& p) noexcept {
    // auro_matic_hp_v4_CoreProcessor_check_static_parameters.
    if (p.sample_rate != 48000u && p.sample_rate != 44100u)
        return 303u;
    if ((p.layout_mask & 0xFFFFFFC3u) != 3u)
        return 304u;
    if (p.reserved_field != 0u)
        return 306u;
    if (p.channel_count > 3u)
        return 305u;
    if (p.has_tuning && (p.tuning_scale < 0.5f || p.tuning_scale > 2.0f))
        return 314u;
    if (p.has_mode && p.mode > 1u)
        return 315u;
    return 0u;
}

bool encode_am4hp_static_parameters(
    const Am4hpStaticParameters& p,
    std::array<std::uint32_t, 10>& words) noexcept {
    if (check_am4hp_static_parameters(p) != 0u)
        return false;
    words.fill(0u);
    words[0] = p.sample_rate;
    words[1] = p.layout_mask;
    words[2] = p.channel_count;
    words[3] = p.reserved_field;
    words[5] = p.has_tuning ? 1u : 0u;
    std::memcpy(&words[6], &p.tuning_scale, sizeof(p.tuning_scale));
    words[7] = p.has_mode ? 1u : 0u;
    words[8] = p.mode;
    return true;
}

Am4hpStaticParameters am4hp_native_static_48000_stereo() noexcept {
    Am4hpStaticParameters p;
    p.sample_rate = 48000u;
    p.layout_mask = 3u;
    p.channel_count = 2u;
    p.has_tuning = true;
    p.tuning_scale = 1.0f;
    p.has_mode = true;
    p.mode = 1u;
    return p;
}

Am4hpDynamicParameters am4hp_native_dynamic_48000_default() noexcept {
    Am4hpDynamicParameters p;
    p.preset = 2u;
    return p;
}

std::uint32_t check_am4hp_dynamic_parameters(
    const Am4hpDynamicParameters& p) noexcept {
    // Native limits are inclusive and are checked in this exact order.
    if (!std::isfinite(p.center_gain_db) || std::abs(p.center_gain_db) > 4.0f)
        return 307u;
    if (!std::isfinite(p.height_gain_db) || std::abs(p.height_gain_db) > 6.0f)
        return 308u;
    if (!std::isfinite(p.surround_gain_db) || std::abs(p.surround_gain_db) > 3.0f)
        return 309u;
    if (!std::isfinite(p.room_mix_db) || p.room_mix_db < -1.0f || p.room_mix_db > 4.0f)
        return 310u;
    if (!std::isfinite(p.output_gain_db) || p.output_gain_db < -144.0f || p.output_gain_db > 3.0f)
        return 311u;
    if (p.preset > 3u)
        return 312u;
    return 0u;
}

bool encode_am4hp_dynamic_parameters(
    const Am4hpDynamicParameters& p,
    std::array<std::uint32_t, 6>& words) noexcept {
    if (check_am4hp_dynamic_parameters(p) != 0u)
        return false;
    const float values[5]{p.center_gain_db, p.height_gain_db,
                          p.surround_gain_db, p.room_mix_db,
                          p.output_gain_db};
    std::memcpy(words.data(), values, sizeof(values));
    words[5] = p.preset;
    return true;
}

float am4hp_output_gain_linear(float output_gain_db) noexcept {
    if (!std::isfinite(output_gain_db) || output_gain_db <= -144.0f)
        return 0.0f;
    return std::pow(10.0f, output_gain_db * 0.050000001f);
}

Am4hpDynamicUpdatePlan plan_am4hp_dynamic_update(
    const Am4hpDynamicParameters& current,
    const Am4hpDynamicParameters& requested,
    bool dynamic_tuning_active,
    std::uint32_t linear_fader_state) noexcept {
    Am4hpDynamicUpdatePlan plan;
    plan.validation_code = check_am4hp_dynamic_parameters(requested);
    if (plan.validation_code != 0u)
        return plan;

    plan.output_gain_linear = am4hp_output_gain_linear(requested.output_gain_db);
    plan.preset_changed = current.preset != requested.preset;
    if (!dynamic_tuning_active) {
        plan.action = Am4hpDynamicUpdateAction::force_dynamic;
    } else if (plan.preset_changed
        && (linear_fader_state == 0u || linear_fader_state == 2u)) {
        // Native: (get_state & 0xFFFFFFFD) == 0, then set_direction(1).
        plan.action = Am4hpDynamicUpdateAction::start_linear_fade;
    } else {
        plan.action = Am4hpDynamicUpdateAction::apply_in_place;
    }
    return plan;
}

bool Am4hpDynamicState::initialize(
    const Am4hpDynamicParameters& parameters) noexcept {
    std::array<std::uint32_t, 6> payload{};
    if (!encode_am4hp_dynamic_parameters(parameters, payload))
        return false;
    parameters_ = parameters;
    payload_words_ = payload;
    output_gain_linear_ = am4hp_output_gain_linear(parameters.output_gain_db);
    last_action_ = Am4hpDynamicUpdateAction::force_dynamic;
    initialized_ = true;
    return true;
}

bool Am4hpDynamicState::update(
    const Am4hpDynamicParameters& parameters,
    bool dynamic_tuning_active,
    std::uint32_t linear_fader_state,
    Am4hpDynamicUpdatePlan* plan_out) noexcept {
    if (!initialized_)
        return false;
    const auto plan = plan_am4hp_dynamic_update(
        parameters_, parameters, dynamic_tuning_active, linear_fader_state);
    if (plan_out)
        *plan_out = plan;
    if (plan.validation_code != 0u)
        return false;
    std::array<std::uint32_t, 6> payload{};
    if (!encode_am4hp_dynamic_parameters(parameters, payload))
        return false;
    parameters_ = parameters;
    payload_words_ = payload;
    output_gain_linear_ = plan.output_gain_linear;
    last_action_ = plan.action;
    return true;
}

}
