#pragma once

#include <array>
#include <cstdint>

namespace auro3d {

struct Am4hpStaticParameters {
    std::uint32_t sample_rate = 0u;
    std::uint32_t layout_mask = 0u;
    std::uint32_t channel_count = 0u;
    std::uint32_t reserved_field = 0u;
    bool has_tuning = false;
    float tuning_scale = 0.0f;
    bool has_mode = false;
    std::uint32_t mode = 0u;
};

// Exact CoreProcessor_check_static_parameters result codes from x86_64 IDA.
// Zero means the native AM4HP core accepts the static configuration.
std::uint32_t check_am4hp_static_parameters(
    const Am4hpStaticParameters& parameters) noexcept;

// Native CoreProcessor construct copies the static API record as ten
// contiguous u32 words. The bool fields are represented by native 0/1 words.
bool encode_am4hp_static_parameters(
    const Am4hpStaticParameters& parameters,
    std::array<std::uint32_t, 10>& words) noexcept;

// Static configuration observed in the 48-kHz AM4HP native fixture.
Am4hpStaticParameters am4hp_native_static_48000_stereo() noexcept;

struct Am4hpDynamicParameters {
    float center_gain_db = 0.0f;
    float height_gain_db = 0.0f;
    float surround_gain_db = 0.0f;
    float room_mix_db = 0.0f;
    float output_gain_db = 0.0f;
    std::uint32_t preset = 0u;
};

// Captured DEFAULT CoreProcessor dynamic record uses preset 2.
Am4hpDynamicParameters am4hp_native_dynamic_48000_default() noexcept;

// Exact CoreProcessor_set_dynamic_parameters validation from 0x549C10.
// The update owner may use the result to choose in-place update versus
// reconstruct; this function only validates and never changes audio state.
std::uint32_t check_am4hp_dynamic_parameters(
    const Am4hpDynamicParameters& parameters) noexcept;

// Native CoreProcessor stores the five float fields followed by preset as
// one contiguous six-word payload. This helper preserves the exact IEEE-754
// representation used by the ABI.
bool encode_am4hp_dynamic_parameters(
    const Am4hpDynamicParameters& parameters,
    std::array<std::uint32_t, 6>& words) noexcept;

// Native output gain field at CoreProcessor+1388: dB to linear, with the
// -144 dB hard-zero boundary.
float am4hp_output_gain_linear(float output_gain_db) noexcept;

// Decision made by CoreProcessor_set_dynamic_parameters after the payload
// passes validation.  Native LinearFader states are 0=done-at-start,
// 1=running, and 2=done-at-end for the tested x86_64 build.
enum class Am4hpDynamicUpdateAction : std::uint8_t {
    reject,
    apply_in_place,
    start_linear_fade,
    force_dynamic,
};

struct Am4hpDynamicUpdatePlan {
    std::uint32_t validation_code = 0u;
    Am4hpDynamicUpdateAction action = Am4hpDynamicUpdateAction::reject;
    float output_gain_linear = 0.0f;
    bool preset_changed = false;
};

// Address-free model of the native setter's ownership decision.  This does
// not process audio or claim to replace CoreProcessor; it makes the update
// boundary explicit for the eventual connected implementation.
Am4hpDynamicUpdatePlan plan_am4hp_dynamic_update(
    const Am4hpDynamicParameters& current,
    const Am4hpDynamicParameters& requested,
    bool dynamic_tuning_active,
    std::uint32_t linear_fader_state) noexcept;

// Transactional owner for the six-word CoreProcessor dynamic record. Native
// stores a valid request before dispatching the selected update path; invalid
// requests leave the previous record untouched.
class Am4hpDynamicState {
public:
    bool initialize(const Am4hpDynamicParameters& parameters) noexcept;
    bool update(const Am4hpDynamicParameters& parameters,
                bool dynamic_tuning_active,
                std::uint32_t linear_fader_state,
                Am4hpDynamicUpdatePlan* plan = nullptr) noexcept;

    bool initialized() const noexcept { return initialized_; }
    const Am4hpDynamicParameters& parameters() const noexcept { return parameters_; }
    const std::array<std::uint32_t, 6>& payload_words() const noexcept {
        return payload_words_;
    }
    float output_gain_linear() const noexcept { return output_gain_linear_; }
    Am4hpDynamicUpdateAction last_action() const noexcept { return last_action_; }

private:
    Am4hpDynamicParameters parameters_{};
    std::array<std::uint32_t, 6> payload_words_{};
    float output_gain_linear_ = 0.0f;
    Am4hpDynamicUpdateAction last_action_ = Am4hpDynamicUpdateAction::reject;
    bool initialized_ = false;
};

}
