#pragma once

#include "am4hp_compressor.hpp"
#include "am4hp_headroom.hpp"
#include "am4hp_input_layout.hpp"
#include "am4hp_linear_fader.hpp"
#include "am4hp_low_end.hpp"
#include "am4hp_parameters.hpp"
#include "am4hp_center_front.hpp"
#include "am4hp_center_surround.hpp"
#include "am4hp_upmix_gain.hpp"
#include "am4hp_upmixing.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace auro3d {

// Stateful AM4HP CoreProcessor owner for the verified 48-kHz stereo and
// 5.0-bed native configurations. The renderer is deliberately a separate
// phase: native CoreProcessor hands nine planes to HPv2 and only then applies
// output accumulation, fader, compressor, and clamp.
class Am4hpCoreProcessor {
public:
    using Block = std::array<float, 32>;
    using PhysicalBlocks = Am4hpInputLayout::PhysicalBlocks;
    using RendererInput = std::array<Block*, 9>;

    bool construct(std::uint32_t sample_rate,
                   std::uint32_t layout_mask,
                   float output_gain = 1.0f,
                   std::string* error = nullptr) noexcept;
    void reset_audio_state() noexcept;
    bool set_preset(std::uint32_t preset,
                    std::string* error = nullptr) noexcept;
    bool take_renderer_reconfigure(std::uint32_t& preset) noexcept;
    float hp_late_feedback_rt60() const noexcept {
        return smoothed_late_rt60_;
    }
    float hp_early_distance_gain_scale() const noexcept;
    float hp_early_distance_db() const noexcept {
        return smoothed_early_distance_db_;
    }
    float hp_late_output_gain_scale() const noexcept;
    float hp_late_shelf_coeff() const noexcept { return smoothed_late_shelf_coeff_; }
    float hp_late_shelf_hz() const noexcept {
        static constexpr float kShelfHz[] = {
            5000.0f, 6000.0f, 8000.0f, 9000.0f};
        return kShelfHz[applied_preset_];
    }
    std::uint32_t requested_preset() const noexcept {
        return dynamic_state_.parameters().preset;
    }
    std::uint32_t applied_preset() const noexcept { return applied_preset_; }

    bool prepare_renderer_input(const PhysicalBlocks& physical,
                                RendererInput& renderer_input) noexcept;
    bool finish_renderer_output(const Block& renderer_left,
                                const Block& renderer_right,
                                Block& output_left,
                                Block& output_right) noexcept;

    struct DebugSnapshot {
        Block headroom_left{};
        Block headroom_right{};
        Block low_end_left{};
        Block low_end_right{};
        std::array<std::uint32_t, 8> low_end_state_bits{};
        Block center_input_left{};
        Block center_input_right{};
        Block center_scaled{};
        Block center_generated{};
        Block center_mutated_left{};
        Block center_mutated_right{};
        std::array<std::uint32_t, 9> center_state_bits{};
        std::array<std::uint32_t, 9> center_state_before_bits{};
        std::array<std::uint32_t, 27> center_control_bits{};
        std::array<std::uint32_t, 27> center_control_before_bits{};
        std::array<std::uint32_t, 266> center_processor_bits{};
        std::array<float, 4> center_energy_sums{};
        Block center_filtered{};
        Block upmix_left{};
        Block upmix_right{};
        Block raw_height_left{};
        Block raw_height_right{};
        Block raw_height_surround_left{};
        Block raw_height_surround_right{};
        std::array<Block, 9> renderer_input{};
        Block accumulated_before_core_gain_left{};
        Block accumulated_before_core_gain_right{};
        Block accumulated_before_compressor_left{};
        Block accumulated_before_compressor_right{};
    };
    const DebugSnapshot& debug_snapshot() const noexcept { return debug_snapshot_; }

    bool constructed() const noexcept { return constructed_; }

private:
    void reset_dynamic_processing_state() noexcept;

    void apply_hp_dynamic_tuning(bool snap) noexcept;
    void seed_hp_dynamic_tuning(std::uint32_t preset) noexcept;
    void hp_dynamic_targets(std::uint32_t preset,
                            float& renderer_gain_db,
                            float& front_gain_db,
                            float& late_rt60,
                            float& early_distance_db,
                            float& late_output_db,
                            float& height_eq_db,
                            float& late_shelf_coeff,
                            float& core_output_db) const noexcept;

    bool configured_ = false;
    bool constructed_ = false;
    std::uint32_t sample_rate_ = 0u;
    std::uint32_t layout_mask_ = 0u;
    float output_gain_ = 1.0f;
    // Native DynamicProcessing+144 Core+1384 is lin(PresetManager v10):
    // stereo 1.5→2.0 dB and surround 3.5→4.0 dB across Core presets 2→3.
    // Preset-2 identity keeps the captured words below while smoothed dB is
    // exactly the preset-2 target.
    float native_core_gain_ = 1.1885021924972534f;
    // Native Core+0x4D8 DynamicProcessing[0] is lin(PresetManager a2[0]).
    // Stereo/surround tables write 5.5 dB on preset 2 and 6.0 dB on preset 3;
    // DynamicProcessing_from_tuning smooths that dB then powf to
    // the LowEnd scale. Preset-2 identity is the captured word 0x3ff11b6a.
    float native_low_end_scale_ = 1.8836491107940674f;
    // Native Renderer_process_float_32_ applies the verified default
    // renderer gain (+8.5 dB) before Core accumulation. This is separate from
    // the caller-facing output gain and is currently fixed to the captured
    // AM4HP profile until dynamic renderer updates are wired through.
    float renderer_gain_ = 2.6607251167297363f;
    // Native PresetManager Smoothing coeff is 32 (rate * 0.05). Process
    // applies it every 32-sample quantum via; force_dynamic snaps
    // with coeff 1, then restores 0.05.
    float hp_smoothing_coeff_ = 32.0f / (48000.0f * 0.05f);
    float smoothed_renderer_gain_db_ = 8.5f;
    float smoothed_upmix_scale_db_ = 8.0f;
    float smoothed_front_gain_db_ = 2.0f;
    float smoothed_late_rt60_ = 0.800000011920929f;
    float smoothed_early_distance_db_ = -8.0f;
    float smoothed_late_output_db_ = -20.0f;
    float smoothed_height_eq_db_ = -2.0f;
    float smoothed_late_shelf_coeff_ = 0.800000011920929f;
    float smoothed_low_end_db_ = 5.5f;
    float smoothed_core_output_db_ = 1.5f;

    Am4hpHeadroom headroom_;
    Am4hpLowEnd low_end_;
    Am4hpCenterFront center_front_;
    Am4hpCenterSurround center_surround_;
    Am4hpUpmixing upmixing_;
    Am4hpUpmixGain upmix_gain_;
    Am4hpLinearFader linear_fader_;
    Am4hpCompressor compressor_;
    Am4hpDynamicState dynamic_state_;
    std::uint32_t applied_preset_ = 2u;
    std::uint32_t pending_preset_ = 2u;
    std::uint32_t ready_preset_ = 2u;
    bool preset_pending_ = false;
    bool renderer_reconfigure_ready_ = false;

    std::array<Block, 31> planes_{};
    Block center_{};
    Block renderer_front_left_{};
    Block renderer_front_right_{};
    Block zero_{};
    // Native Core gives every optional LowEnd input slot its own scratch
    // block. They must not alias: LowEnd runs independent filter states over
    // slots 4..7 and may reuse the blocks later in the same call.
    Block low_end_center_zero_{};
    Block low_end_lfe_zero_{};
    Block low_end_height_left_zero_{};
    Block low_end_height_right_zero_{};
    Block low_end_center_previous_{};
    Block low_end_secondary_previous_{};
    Block upmix_45_left_{};
    Block upmix_45_right_{};
    Block surround_left_{};
    Block surround_right_{};
    Block surround_center_{};
    Block height_left_{};
    Block height_right_{};
    Block height_surround_left_{};
    Block height_surround_right_{};
    Block accumulated_left_{};
    Block accumulated_right_{};
    DebugSnapshot debug_snapshot_{};
    bool renderer_pending_ = false;
};

} // namespace auro3d
