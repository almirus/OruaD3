#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <string>
#include <vector>

namespace auro3d {

class HeadphonePcaScoreState;

struct HeadphonePcaGridEntry {
    std::int32_t itd_samples = 0;
    std::vector<std::uint32_t> first_ear_f32_bits;
    std::vector<std::uint32_t> second_ear_f32_bits;
};

struct HeadphonePcaFilterRecord {
    std::uint8_t component = 0;
    std::uint8_t order_index = 0;
    std::uint8_t order = 0;
    std::uint8_t ear = 0;
    std::uint32_t gain_f32_bits = 0;
    std::vector<std::uint8_t> coefficients;
};

struct HeadphonePcaDefinition {
    std::uint32_t sample_rate = 0;
    std::vector<HeadphonePcaGridEntry> grid;
    std::vector<HeadphonePcaFilterRecord> filters[2];
};

struct HeadphonePcaValidationVector {
    std::uint32_t score_count = 0;
    std::int32_t azimuth_q23_degrees = 0;
    std::int32_t elevation_q23_degrees = 0;
    std::uint32_t sample_rate = 0;
    std::int32_t itd_samples = 0;
    std::vector<std::uint32_t> first_ear_f32_bits;
    std::vector<std::uint32_t> second_ear_f32_bits;
};

struct HeadphoneWiirValidationVector {
    std::uint32_t slot = 0;
    std::uint32_t native_call_index = 0;
    std::array<std::uint32_t, 32> left_input_f32_bits{};
    std::array<std::uint32_t, 32> right_input_f32_bits{};
    std::array<std::uint32_t, 32> left_output_f32_bits{};
    std::array<std::uint32_t, 32> right_output_f32_bits{};
};

struct HeadphoneExplicitValidationVector {
    bool has_validation = false;
    std::uint32_t slot = 0;
    std::int32_t azimuth_q23_degrees = 0;
    std::int32_t elevation_q23_degrees = 0;
    std::uint32_t sample_rate = 0;
    std::uint32_t mode = 0;
    std::uint32_t distance_delay_samples = 0;
    std::uint32_t distance_gain_f32_bits = 0;
    std::int32_t pca_itd_samples = 0;
    std::int32_t scaled_itd_samples = 0;
    std::uint32_t score_count = 0;
    std::uint32_t secondary_layout_count = 0;
    std::vector<std::uint32_t> first_pca_gain_f32_bits;
    std::vector<std::uint32_t> second_pca_gain_f32_bits;
    std::array<std::uint32_t, 32> input_f32_bits{};
    std::array<std::uint32_t, 32> first_output_f32_bits{};
    std::array<std::uint32_t, 32> second_output_f32_bits{};
    std::vector<std::array<std::uint32_t, 32>> first_manager_before;
    std::vector<std::array<std::uint32_t, 32>> second_manager_before;
    std::vector<std::array<std::uint32_t, 32>> first_manager_after;
    std::vector<std::array<std::uint32_t, 32>> second_manager_after;
};

struct HeadphoneEarlyReflectionValidationVector {
    bool has_validation = false;
    std::uint32_t slot = 0;
    std::int32_t azimuth_q23_degrees = 0;
    std::int32_t elevation_q23_degrees = 0;
    std::int32_t distance_q23 = 0;
    std::uint32_t sample_rate = 0;
    std::uint32_t mode = 0;
    std::uint32_t diffusion_enabled = 0;
    std::uint32_t distance_delay_samples = 0;
    std::uint32_t distance_gain_f32_bits = 0;
    std::int32_t pca_itd_samples = 0;
    std::int32_t scaled_itd_samples = 0;
    std::uint32_t all_pass_delay_samples = 0;
    std::uint32_t all_pass_coefficient_f32_bits = 0;
    std::uint32_t score_count = 0;
    std::uint32_t manager_score_count = 0;
    std::vector<std::uint32_t> first_pca_gain_f32_bits;
    std::vector<std::uint32_t> second_pca_gain_f32_bits;
    std::array<std::uint32_t, 32> distance_output_f32_bits{};
    std::array<std::uint32_t, 32> gain_output_f32_bits{};
    std::array<std::uint32_t, 32> diffusion_output_f32_bits{};
    std::array<std::uint32_t, 32> first_output_f32_bits{};
    std::array<std::uint32_t, 32> second_output_f32_bits{};
    std::vector<std::array<std::uint32_t, 32>> first_manager_before;
    std::vector<std::array<std::uint32_t, 32>> second_manager_before;
    std::vector<std::array<std::uint32_t, 32>> first_manager_after;
    std::vector<std::array<std::uint32_t, 32>> second_manager_after;
};

struct HeadphoneLateReverbValidationVector {
    std::uint32_t slot = 0;
    std::int32_t azimuth_q23_degrees = 0;
    std::int32_t elevation_q23_degrees = 0;
    std::uint32_t sample_rate = 0;
    std::int32_t pca_itd_samples = 0;
    std::uint32_t score_count = 0;
    std::uint32_t manager_score_count = 0;
    std::vector<std::uint32_t> first_pca_gain_f32_bits;
    std::vector<std::uint32_t> second_pca_gain_f32_bits;
    std::array<std::uint32_t, 32> input_f32_bits{};
    std::vector<std::array<std::uint32_t, 32>> first_manager_before;
    std::vector<std::array<std::uint32_t, 32>> second_manager_before;
    std::vector<std::array<std::uint32_t, 32>> first_manager_after;
    std::vector<std::array<std::uint32_t, 32>> second_manager_after;
};

struct HeadphoneFgwhtValidationVector {
    std::uint32_t native_call_index = 0;
    std::uint32_t count = 0;
    std::array<std::uint32_t, 4> state_before_f32_bits{};
    std::array<std::uint32_t, 4> state_after_f32_bits{};
    std::vector<std::array<std::uint32_t, 32>> blocks_before_f32_bits;
    std::vector<std::array<std::uint32_t, 32>> blocks_after_f32_bits;
};

struct HeadphoneLateReverbAllPassConfig {
    std::uint32_t delay_samples = 0;
    std::uint32_t coefficient_f32_bits = 0;
};

struct HeadphoneLateReverbBandConfig {
    std::uint32_t slot = 0;
    std::uint32_t delay_samples = 0;
    std::uint32_t feedback_gain_f32_bits = 0;
    std::uint32_t total_delay_samples = 0;
    std::vector<HeadphoneLateReverbAllPassConfig> all_passes;
};

struct HeadphoneLateReverbCoreConfig {
    std::uint32_t sample_rate = 0;
    std::uint32_t input_filter_enabled = 0;
    std::uint32_t mode = 0;
    std::uint32_t output_layout = 0;
    std::uint32_t all_pass_count = 0;
    std::uint32_t output_gain_f32_bits = 0;
    std::uint32_t damping_f32_bits = 0;
    std::uint32_t rt60_f32_bits = 0;
    std::uint32_t input_delay_samples = 0;
    std::array<std::uint32_t, 6> input_filter_f32_bits{};
    std::array<std::uint32_t, 5> damping_correction_filter_f32_bits{};
    std::array<std::uint32_t, 5> band_damping_filter_f32_bits{};
    std::array<std::uint32_t, 4> fgwht_state_f32_bits{};
    std::uint32_t modulation_depth_f32_bits = 0;
    std::uint32_t modulation_rate_f32_bits = 0;
    std::uint32_t modulation_phase_f32_bits = 0;
    std::vector<HeadphoneLateReverbBandConfig> bands;
};

struct HeadphoneLateReverbCoreSequenceVector {
    std::uint32_t native_call_index = 0;
    std::uint32_t slot = 0;
    std::array<std::uint32_t, 32> input_f32_bits{};
    std::vector<std::array<std::uint32_t, 32>> output_f32_bits;
};

struct HeadphoneSourceManagerConfig {
    std::uint32_t sample_rate = 0;
    std::uint32_t source_count = 0;
    std::uint32_t early_reflection_count = 0;
    std::uint32_t late_reverb_source_count = 0;
    std::uint32_t pca_score_count = 0;
    std::uint32_t reflection_enabled = 0;
    std::uint32_t reflection_mode = 0;
    std::uint32_t late_reverb_disabled = 0;
    std::uint32_t pca_filter_order_index = 0;
    std::uint32_t first_pca_count = 0;
    std::uint32_t second_pca_count = 0;
    std::vector<std::uint32_t> late_input_gain_f32_bits;
    std::vector<std::uint8_t> wall_material_enabled;
    std::vector<std::array<std::uint32_t, 5>> wall_first_coefficients_f32_bits;
    std::vector<std::array<std::uint32_t, 5>> wall_second_coefficients_f32_bits;
};

struct HeadphoneSourceManagerSequenceVector {
    std::uint32_t native_call_index = 0;
    std::uint32_t slot = 0;
    std::vector<std::array<std::uint32_t, 32>> input_f32_bits;
    std::array<std::array<std::uint32_t, 32>, 2> output_before_f32_bits{};
    std::array<std::array<std::uint32_t, 32>, 2> output_after_f32_bits{};
    std::array<std::array<std::uint32_t, 32>, 16> score_f32_bits{};
};

class HeadphonePcaBank {
public:
    bool load(const void* data, std::size_t size, std::string& error);
    bool load_embedded(std::string& error);
    bool load_embedded_resource(unsigned resource_id, std::string& error);

    std::uint32_t component_count() const noexcept { return component_count_; }
    std::uint32_t azimuth_count() const noexcept { return azimuth_count_; }
    std::uint32_t elevation_count() const noexcept { return elevation_count_; }
    const std::vector<std::uint8_t>& filter_orders() const noexcept {
        return filter_orders_;
    }
    const std::vector<HeadphonePcaDefinition>& definitions() const noexcept {
        return definitions_;
    }
    const std::vector<HeadphonePcaValidationVector>& validation_vectors() const noexcept {
        return validation_vectors_;
    }
    const std::vector<HeadphoneWiirValidationVector>& wiir_validation_vectors() const noexcept {
        return wiir_validation_vectors_;
    }
    const std::vector<HeadphoneExplicitValidationVector>& explicit_validation_vectors() const noexcept {
        return explicit_validation_vectors_;
    }
    const std::vector<HeadphoneEarlyReflectionValidationVector>& early_reflection_validation_vectors() const noexcept {
        return early_reflection_validation_vectors_;
    }
    const std::vector<HeadphoneLateReverbValidationVector>& late_reverb_validation_vectors() const noexcept {
        return late_reverb_validation_vectors_;
    }
    const std::vector<HeadphoneFgwhtValidationVector>& fgwht_validation_vectors() const noexcept {
        return fgwht_validation_vectors_;
    }
    const std::vector<HeadphoneLateReverbCoreConfig>& late_reverb_core_configs() const noexcept {
        return late_reverb_core_configs_;
    }
    const std::vector<HeadphoneLateReverbCoreSequenceVector>& late_reverb_core_sequence() const noexcept {
        return late_reverb_core_sequence_;
    }
    const std::vector<HeadphoneSourceManagerConfig>& source_manager_configs() const noexcept {
        return source_manager_configs_;
    }
    const std::vector<HeadphoneSourceManagerSequenceVector>& source_manager_sequence() const noexcept {
        return source_manager_sequence_;
    }
    const HeadphonePcaDefinition* find_definition(std::uint32_t sample_rate) const noexcept;
    const HeadphonePcaGridEntry* grid_entry(std::uint32_t sample_rate,
                                            std::uint32_t azimuth_index,
                                            std::uint32_t elevation_index) const noexcept;
    const HeadphonePcaFilterRecord* filter(std::uint32_t sample_rate,
                                           std::uint32_t component,
                                           std::uint32_t ear,
                                           std::uint32_t order_index) const noexcept;
    bool interpolate(std::uint32_t sample_rate,
                     std::int32_t azimuth_q23_degrees,
                     std::int32_t elevation_q23_degrees,
                     std::uint32_t score_count,
                     std::int32_t& itd_samples,
                     std::vector<float>& first_ear,
                     std::vector<float>& second_ear) const noexcept;
    bool interpolate_itd(std::uint32_t sample_rate,
                         std::int32_t azimuth_q23_degrees,
                         std::int32_t elevation_q23_degrees,
                         std::int32_t& itd_samples) const noexcept;
    bool populate_score_state(std::uint32_t sample_rate,
                              std::int32_t azimuth_q23_degrees,
                              std::int32_t elevation_q23_degrees,
                              HeadphonePcaScoreState& state) const noexcept;
    // Native fade-end StaticTuning_update_hp_from_preset writes
    // into the first 3 (stereo) or 5 (surround) Explicit distances, then
    // Renderer_t_construct rebuilds delay = dist*rate/340 - min(dist)*rate/340
    // and gain = 1/dist. Packed resource 107 still holds the preset-2 5 m 706
    // sample image.
    bool apply_am4hp_static_source_distances(unsigned preset,
                                            unsigned layout) noexcept;

private:
    void clear() noexcept;

    std::uint32_t component_count_ = 0;
    std::uint32_t azimuth_count_ = 0;
    std::uint32_t elevation_count_ = 0;
    std::vector<std::uint8_t> filter_orders_;
    std::vector<HeadphonePcaDefinition> definitions_;
    std::vector<HeadphonePcaValidationVector> validation_vectors_;
    std::vector<HeadphoneWiirValidationVector> wiir_validation_vectors_;
    std::vector<HeadphoneExplicitValidationVector> explicit_validation_vectors_;
    std::vector<HeadphoneEarlyReflectionValidationVector> early_reflection_validation_vectors_;
    std::vector<HeadphoneLateReverbValidationVector> late_reverb_validation_vectors_;
    std::vector<HeadphoneFgwhtValidationVector> fgwht_validation_vectors_;
    std::vector<HeadphoneLateReverbCoreConfig> late_reverb_core_configs_;
    std::vector<HeadphoneLateReverbCoreSequenceVector> late_reverb_core_sequence_;
    std::vector<HeadphoneSourceManagerConfig> source_manager_configs_;
    std::vector<HeadphoneSourceManagerSequenceVector> source_manager_sequence_;
};

}
