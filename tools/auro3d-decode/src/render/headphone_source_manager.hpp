#pragma once

#include "headphone_early_reflection.hpp"
#include "headphone_explicit_source_path.hpp"
#include "headphone_late_reverb_core.hpp"
#include "headphone_late_reverb_path.hpp"
#include "headphone_pca_bank.hpp"
#include "headphone_wiir.hpp"
#include "headphone_wall_material_filter.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace auro3d {

// Stateful source_Manager process recovered at sub_561A30. This implementation
// accepts only captured mode-0/mode-2 HPv2 graphs at native-accepted rates.
class HeadphoneSourceManager {
public:
    using Block = std::array<float, 32>;
    using RendererInput = std::array<Block*, 9>;

    struct RendererInputDescriptor {
        std::uint32_t frame_count = 0u;
        std::uint32_t sample_rate = 0u;
        RendererInput planes{};
    };

    bool construct(const HeadphonePcaBank& bank);
    void reset_audio_state() noexcept;
    bool process(const std::vector<std::array<float, 32>>& inputs,
                 std::array<float, 32>& first_output,
                 std::array<float, 32>& second_output) noexcept;
    // AM4HP CoreProcessor's native headphone-renderer boundary has exactly
    // nine planar inputs. Keep the conversion explicit instead of making the
    // caller rely on vector order or silently dropping a plane.
    bool process_renderer_input(const RendererInput& inputs,
                                std::array<float, 32>& first_output,
                                std::array<float, 32>& second_output) noexcept;
    bool process_renderer_descriptor(
        const RendererInputDescriptor& descriptor,
        std::array<float, 32>& first_output,
        std::array<float, 32>& second_output) noexcept;
    bool set_late_feedback_rt60(float rt60_seconds) noexcept;
    bool set_early_distance_gain_scale(float scale) noexcept;
    bool set_early_distance_gain_db(float gain_db) noexcept;
    bool set_late_output_gain_scale(float scale) noexcept;
    bool set_late_dynamic_shelf(float damping_coeff,
                                float frequency_hz) noexcept;
    const std::vector<std::array<float, 32>>& first_scores() const noexcept {
        return first_scores_;
    }
    const std::vector<std::array<float, 32>>& second_scores() const noexcept {
        return second_scores_;
    }
    void set_debug_capture_early_scores(bool enabled) noexcept {
        debug_capture_early_scores_ = enabled;
    }
    const std::vector<std::array<float, 32>>& debug_early_first_scores() const noexcept {
        return debug_early_first_scores_;
    }
    const std::vector<std::array<float, 32>>& debug_early_second_scores() const noexcept {
        return debug_early_second_scores_;
    }
    const std::vector<std::array<float, 32>>& debug_explicit_first_scores() const noexcept {
        return debug_explicit_first_scores_;
    }
    const std::vector<std::array<float, 32>>& debug_explicit_second_scores() const noexcept {
        return debug_explicit_second_scores_;
    }
    std::vector<float> debug_early_distance_gains() const;
    struct DebugEarlyContribution {
        std::uint32_t native_slot = 0u;
        Block first_ear{};
        Block second_ear{};
        Block first_pre_gain{};
        Block second_pre_gain{};
        std::vector<Block> first_scores;
        std::vector<Block> second_scores;
    };
    const std::vector<DebugEarlyContribution>& debug_early_contributions() const noexcept {
        return debug_early_contributions_;
    }

private:
    struct ExplicitSource {
        std::uint32_t native_slot = 0u;
        std::size_t input_slot = 0u;
        HeadphoneExplicitSourcePath path;
        std::vector<float> first_gains;
        std::vector<float> second_gains;
    };
    struct EarlySource {
        std::uint32_t native_slot = 0u;
        std::size_t input_slot = 0u;
        HeadphoneEarlyReflectionPath path;
        std::vector<float> first_gains;
        std::vector<float> second_gains;
    };
    struct LateSource {
        HeadphoneLateReverbPath path;
        std::vector<float> first_gains;
        std::vector<float> second_gains;
    };

    bool rebuild_am4hp_shared_delays();

    std::size_t source_count_ = 0u;
    std::size_t score_count_ = 0u;
    std::uint32_t sample_rate_ = 0u;
    std::vector<float> late_input_gains_;
    std::vector<std::uint8_t> wall_material_enabled_;
    std::vector<HeadphoneWallMaterialFilter> wall_first_filters_;
    std::vector<HeadphoneWallMaterialFilter> wall_second_filters_;
    std::vector<ExplicitSource> explicit_sources_;
    std::vector<EarlySource> early_sources_;
    bool am4hp_profile_ = false;
    // Native AM4HP Manager keeps raw and WallMaterial-filtered input delays
    // separately for each source.
    std::vector<HeadphoneMultiDelay> am4hp_input_delays_;
    std::vector<HeadphoneMultiDelay> am4hp_wall_input_delays_;
    HeadphoneLateReverbCore late_core_;
    std::vector<LateSource> late_sources_;
    std::vector<HeadphoneWiir> first_filters_;
    std::vector<HeadphoneWiir> second_filters_;
    bool debug_capture_early_scores_ = false;
    std::vector<std::array<float, 32>> debug_early_first_scores_;
    std::vector<std::array<float, 32>> debug_early_second_scores_;
    std::vector<std::array<float, 32>> debug_explicit_first_scores_;
    std::vector<std::array<float, 32>> debug_explicit_second_scores_;
    std::vector<DebugEarlyContribution> debug_early_contributions_;
    std::vector<std::array<float, 32>> first_scores_;
    std::vector<std::array<float, 32>> second_scores_;
};

}
