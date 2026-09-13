#include "headphone_pca_bank.hpp"
#include "headphone_pca_score_state.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <windows.h>

namespace auro3d {
namespace {

constexpr std::int32_t kQ23 = 1 << 23;

struct GridSplit {
    std::uint32_t azimuth[2]{};
    std::uint32_t elevation[2]{};
    std::int32_t azimuth_fraction = 0;
    std::int32_t elevation_fraction = 0;
};

class Reader {
public:
    Reader(const void* data, std::size_t size)
        : cursor_(static_cast<const std::uint8_t*>(data)), remaining_(size) {}

    bool bytes(void* output, std::size_t count) {
        if (count > remaining_) return false;
        std::memcpy(output, cursor_, count);
        cursor_ += count;
        remaining_ -= count;
        return true;
    }
    bool skip(std::size_t count) {
        if (count > remaining_) return false;
        cursor_ += count;
        remaining_ -= count;
        return true;
    }
    bool u32(std::uint32_t& value) { return bytes(&value, sizeof(value)); }
    std::size_t remaining() const noexcept { return remaining_; }

private:
    const std::uint8_t* cursor_;
    std::size_t remaining_;
};

bool checked_product(std::uint32_t first, std::uint32_t second,
                     std::uint32_t& result) {
    const std::uint64_t value = static_cast<std::uint64_t>(first) * second;
    if (value > (std::numeric_limits<std::uint32_t>::max)()) return false;
    result = static_cast<std::uint32_t>(value);
    return true;
}

std::int32_t native_round_q23(std::int64_t product) noexcept {
    const auto value = static_cast<std::uint64_t>(product);
    const auto rounded = (value >> 23)
        + ((static_cast<std::uint32_t>(product) >> 22) & 1u);
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(rounded));
}

bool split_angle_grid(std::int32_t azimuth, std::uint32_t azimuth_count,
                      std::int32_t elevation, std::uint32_t elevation_count,
                      GridSplit& split) noexcept {
    if (!azimuth_count || !elevation_count) return false;
    constexpr std::int32_t pole = 90 * kQ23;
    constexpr std::int32_t half_turn = 180 * kQ23;
    if (elevation <= -pole) {
        elevation = -half_turn - elevation;
        azimuth += azimuth >= 0 ? -half_turn : half_turn;
    } else if (elevation >= pole) {
        elevation = half_turn - elevation;
        azimuth += azimuth >= 0 ? -half_turn : half_turn;
    }
    const std::int32_t elevation_span = elevation_count > 0
        ? static_cast<std::int32_t>(elevation_count - 1u) : 1;
    const std::int32_t elevation_unit = (elevation + pole) / 180;
    const std::int32_t elevation_coordinate = native_round_q23(
        (static_cast<std::int64_t>(elevation_span) << 23) * elevation_unit);
    const std::int32_t elevation_floor = elevation_coordinate >> 23;
    split.elevation_fraction = elevation_coordinate - elevation_floor * kQ23;
    const std::int32_t elevation_ceil = elevation_floor
        + (split.elevation_fraction != 0 ? 1 : 0);
    if (elevation_floor < 0 || elevation_ceil < 0
        || elevation_floor >= static_cast<std::int32_t>(elevation_count)
        || elevation_ceil >= static_cast<std::int32_t>(elevation_count)) return false;
    split.elevation[0] = static_cast<std::uint32_t>(elevation_floor);
    split.elevation[1] = static_cast<std::uint32_t>(elevation_ceil);

    std::int32_t azimuth_coordinate = 0;
    if (360u % azimuth_count || (azimuth_count & 1u)) {
        const std::int64_t product =
            (static_cast<std::int64_t>(azimuth / 360) + (kQ23 >> 1))
            * (static_cast<std::int64_t>(azimuth_count) << 23);
        azimuth_coordinate = static_cast<std::int32_t>(product >> 23);
    } else {
        const std::int32_t step = 360 / static_cast<std::int32_t>(azimuth_count);
        azimuth_coordinate = static_cast<std::int32_t>(
            (static_cast<std::int64_t>(azimuth) << 23)
            / (static_cast<std::int64_t>(step) << 23));
        if (azimuth < 0)
            azimuth_coordinate += static_cast<std::int32_t>(azimuth_count << 23);
    }
    const std::int32_t azimuth_floor = azimuth_coordinate >> 23;
    split.azimuth_fraction = azimuth_coordinate - azimuth_floor * kQ23;
    const std::int32_t azimuth_ceil = azimuth_floor
        + (split.azimuth_fraction != 0 ? 1 : 0);
    const auto wrap = [azimuth_count](std::int32_t value) {
        std::int32_t wrapped = value % static_cast<std::int32_t>(azimuth_count);
        if (wrapped < 0) wrapped += static_cast<std::int32_t>(azimuth_count);
        return static_cast<std::uint32_t>(wrapped);
    };
    split.azimuth[0] = wrap(azimuth_floor);
    split.azimuth[1] = wrap(azimuth_ceil);
    return true;
}

float bits_to_float(std::uint32_t bits) noexcept {
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

float multiply_add_separate(float accumulator, float gain, float value) noexcept {
    volatile float product = gain * value;
    return accumulator + product;
}

}

void HeadphonePcaBank::clear() noexcept {
    component_count_ = 0;
    azimuth_count_ = 0;
    elevation_count_ = 0;
    filter_orders_.clear();
    definitions_.clear();
    validation_vectors_.clear();
    wiir_validation_vectors_.clear();
    explicit_validation_vectors_.clear();
    early_reflection_validation_vectors_.clear();
    late_reverb_validation_vectors_.clear();
    fgwht_validation_vectors_.clear();
    late_reverb_core_configs_.clear();
    late_reverb_core_sequence_.clear();
    source_manager_configs_.clear();
    source_manager_sequence_.clear();
}

bool HeadphonePcaBank::load(const void* data, std::size_t size,
                            std::string& error) {
    clear();
    error.clear();
    if (!data) { error = "null HPv2 bank"; return false; }
    Reader reader(data, size);
    char magic[8]{};
    std::uint32_t version = 0, filter_order_count = 0, definition_count = 0;
    if (!reader.bytes(magic, sizeof(magic))
        || std::memcmp(magic, "AHPV2B01", sizeof(magic)) != 0
        || !reader.u32(version) || (version != 13 && version != 14)
        || !reader.u32(component_count_)
        || !reader.u32(azimuth_count_)
        || !reader.u32(elevation_count_)
        || !reader.u32(filter_order_count)
        || !reader.u32(definition_count)) {
        error = "invalid HPv2 bank header";
        clear();
        return false;
    }
    if (!component_count_ || !azimuth_count_ || !elevation_count_
        || !filter_order_count || !definition_count
        || component_count_ > 256 || azimuth_count_ > 4096
        || elevation_count_ > 4096 || filter_order_count > 256
        || definition_count > 256) {
        error = "unsupported HPv2 bank dimensions";
        clear();
        return false;
    }
    filter_orders_.resize(filter_order_count);
    if (!reader.bytes(filter_orders_.data(), filter_orders_.size())) {
        error = "truncated HPv2 filter orders";
        clear();
        return false;
    }
    const std::size_t header_size = 8u + 6u * sizeof(std::uint32_t)
        + filter_orders_.size();
    if (!reader.skip((4u - (header_size & 3u)) & 3u)) {
        error = "truncated HPv2 header padding";
        clear();
        return false;
    }

    std::uint32_t expected_grid = 0, expected_filters = 0;
    if (!checked_product(azimuth_count_, elevation_count_, expected_grid)
        || !checked_product(component_count_, filter_order_count, expected_filters)) {
        error = "HPv2 dimensions overflow";
        clear();
        return false;
    }
    try {
        definitions_.reserve(definition_count);
        for (std::uint32_t di = 0; di < definition_count; ++di) {
            HeadphonePcaDefinition definition;
            std::uint32_t grid_count = 0, filters_per_ear = 0, reserved = 0;
            if (!reader.u32(definition.sample_rate) || !reader.u32(grid_count)
                || !reader.u32(filters_per_ear) || !reader.u32(reserved)
                || grid_count != expected_grid || filters_per_ear != expected_filters
                || reserved != 0) {
                error = "invalid HPv2 definition header";
                clear();
                return false;
            }
            definition.grid.resize(grid_count);
            for (auto& entry : definition.grid) {
                std::uint32_t itd = 0;
                entry.first_ear_f32_bits.resize(component_count_);
                entry.second_ear_f32_bits.resize(component_count_);
                if (!reader.u32(itd)
                    || !reader.bytes(entry.first_ear_f32_bits.data(),
                                     component_count_ * sizeof(std::uint32_t))
                    || !reader.bytes(entry.second_ear_f32_bits.data(),
                                     component_count_ * sizeof(std::uint32_t))) {
                    error = "truncated HPv2 PCA grid";
                    clear();
                    return false;
                }
                entry.itd_samples = static_cast<std::int32_t>(itd);
            }
            for (std::uint32_t ear = 0; ear < 2; ++ear) {
                auto& filters = definition.filters[ear];
                filters.resize(filters_per_ear);
                for (std::uint32_t fi = 0; fi < filters_per_ear; ++fi) {
                    auto& record = filters[fi];
                    std::uint8_t header[4]{};
                    std::uint32_t coefficient_bytes = 0, record_reserved = 0;
                    if (!reader.bytes(header, sizeof(header))
                        || !reader.u32(record.gain_f32_bits)
                        || !reader.u32(coefficient_bytes)
                        || !reader.u32(record_reserved)) {
                        error = "truncated HPv2 filter header";
                        clear();
                        return false;
                    }
                    record.component = header[0];
                    record.order_index = header[1];
                    record.order = header[2];
                    record.ear = header[3];
                    const std::uint32_t expected_component = fi / filter_order_count;
                    const std::uint32_t expected_order_index = fi % filter_order_count;
                    const std::uint32_t expected_bytes = 24u * ((record.order + 1u) / 2u);
                    if (record.component != expected_component
                        || record.order_index != expected_order_index
                        || record.order != filter_orders_[record.order_index]
                        || record.ear != ear || record_reserved != 0
                        || coefficient_bytes != expected_bytes) {
                        error = "invalid HPv2 filter record";
                        clear();
                        return false;
                    }
                    record.coefficients.resize(coefficient_bytes);
                    if (!reader.bytes(record.coefficients.data(), coefficient_bytes)) {
                        error = "truncated HPv2 filter coefficients";
                        clear();
                        return false;
                    }
                }
            }
            definitions_.push_back(std::move(definition));
        }
        std::uint32_t validation_count = 0;
        if (!reader.u32(validation_count) || validation_count > 10000u) {
            error = "invalid HPv2 validation-vector count";
            clear();
            return false;
        }
        validation_vectors_.resize(validation_count);
        for (auto& vector : validation_vectors_) {
            std::uint32_t azimuth = 0, elevation = 0, itd = 0;
            if (!reader.u32(vector.score_count) || !reader.u32(azimuth)
                || !reader.u32(elevation) || !reader.u32(vector.sample_rate)
                || !reader.u32(itd) || vector.score_count > component_count_) {
                error = "invalid HPv2 validation vector";
                clear();
                return false;
            }
            vector.azimuth_q23_degrees = static_cast<std::int32_t>(azimuth);
            vector.elevation_q23_degrees = static_cast<std::int32_t>(elevation);
            vector.itd_samples = static_cast<std::int32_t>(itd);
            vector.first_ear_f32_bits.resize(vector.score_count);
            vector.second_ear_f32_bits.resize(vector.score_count);
            if (!reader.bytes(vector.first_ear_f32_bits.data(),
                              vector.score_count * sizeof(std::uint32_t))
                || !reader.bytes(vector.second_ear_f32_bits.data(),
                                 vector.score_count * sizeof(std::uint32_t))) {
                error = "truncated HPv2 validation vector";
                clear();
                return false;
            }
        }
        std::uint32_t wiir_validation_count = 0;
        if (!reader.u32(wiir_validation_count) || wiir_validation_count > 256u) {
            error = "invalid HPv2 WIIR validation-vector count";
            clear();
            return false;
        }
        wiir_validation_vectors_.resize(wiir_validation_count);
        for (auto& vector : wiir_validation_vectors_) {
            if (!reader.u32(vector.slot) || !reader.u32(vector.native_call_index)
                || !reader.bytes(vector.left_input_f32_bits.data(),
                                 vector.left_input_f32_bits.size() * sizeof(std::uint32_t))
                || !reader.bytes(vector.right_input_f32_bits.data(),
                                 vector.right_input_f32_bits.size() * sizeof(std::uint32_t))
                || !reader.bytes(vector.left_output_f32_bits.data(),
                                 vector.left_output_f32_bits.size() * sizeof(std::uint32_t))
                || !reader.bytes(vector.right_output_f32_bits.data(),
                                 vector.right_output_f32_bits.size() * sizeof(std::uint32_t))) {
                error = "truncated HPv2 WIIR validation vector";
                clear();
                return false;
            }
        }
        std::uint32_t explicit_validation_count = 0;
        if (!reader.u32(explicit_validation_count) || explicit_validation_count > 256u) {
            error = "invalid HPv2 Explicit validation-vector count";
            clear();
            return false;
        }
        explicit_validation_vectors_.resize(explicit_validation_count);
        for (auto& vector : explicit_validation_vectors_) {
            std::uint32_t azimuth = 0, elevation = 0, pca_itd = 0, scaled_itd = 0;
            std::uint32_t has_validation = 1u;
            if (!reader.u32(vector.slot) || !reader.u32(azimuth)
                || !reader.u32(elevation) || !reader.u32(vector.sample_rate)
                || !reader.u32(vector.mode)
                || !reader.u32(vector.distance_delay_samples)
                || !reader.u32(vector.distance_gain_f32_bits)
                || !reader.u32(pca_itd) || !reader.u32(scaled_itd)
                || !reader.u32(vector.score_count)
                || !reader.u32(vector.secondary_layout_count)
                || (version >= 14u && !reader.u32(has_validation))
                || has_validation > 1u
                || vector.score_count > component_count_) {
                error = "invalid HPv2 Explicit validation vector";
                clear();
                return false;
            }
            vector.azimuth_q23_degrees = static_cast<std::int32_t>(azimuth);
            vector.elevation_q23_degrees = static_cast<std::int32_t>(elevation);
            vector.pca_itd_samples = static_cast<std::int32_t>(pca_itd);
            vector.scaled_itd_samples = static_cast<std::int32_t>(scaled_itd);
            vector.has_validation = has_validation != 0u;
            vector.first_pca_gain_f32_bits.resize(vector.score_count);
            vector.second_pca_gain_f32_bits.resize(vector.score_count);
            if (!reader.bytes(vector.first_pca_gain_f32_bits.data(),
                              vector.score_count * sizeof(std::uint32_t))
                || !reader.bytes(vector.second_pca_gain_f32_bits.data(),
                                 vector.score_count * sizeof(std::uint32_t))
                || !reader.bytes(vector.input_f32_bits.data(), sizeof(vector.input_f32_bits))
                || !reader.bytes(vector.first_output_f32_bits.data(),
                                 sizeof(vector.first_output_f32_bits))
                || !reader.bytes(vector.second_output_f32_bits.data(),
                                 sizeof(vector.second_output_f32_bits))) {
                error = "truncated HPv2 Explicit vector blocks";
                clear();
                return false;
            }
            auto read_manager = [&reader, &vector](
                std::vector<std::array<std::uint32_t, 32>>& blocks) {
                blocks.resize(vector.score_count);
                for (auto& block : blocks)
                    if (!reader.bytes(block.data(), sizeof(block))) return false;
                return true;
            };
            if (!read_manager(vector.first_manager_before)
                || !read_manager(vector.second_manager_before)
                || !read_manager(vector.first_manager_after)
                || !read_manager(vector.second_manager_after)) {
                error = "truncated HPv2 Explicit manager blocks";
                clear();
                return false;
            }
        }
        std::uint32_t early_validation_count = 0;
        if (!reader.u32(early_validation_count) || early_validation_count > 256u) {
            error = "invalid HPv2 EarlyReflection validation-vector count";
            clear();
            return false;
        }
        early_reflection_validation_vectors_.resize(early_validation_count);
        for (auto& vector : early_reflection_validation_vectors_) {
            std::uint32_t azimuth = 0, elevation = 0, distance = 0;
            std::uint32_t pca_itd = 0, scaled_itd = 0;
            std::uint32_t has_validation = 1u;
            if (!reader.u32(vector.slot) || !reader.u32(azimuth)
                || !reader.u32(elevation) || !reader.u32(distance)
                || !reader.u32(vector.sample_rate) || !reader.u32(vector.mode)
                || !reader.u32(vector.diffusion_enabled)
                || !reader.u32(vector.distance_delay_samples)
                || !reader.u32(vector.distance_gain_f32_bits)
                || !reader.u32(pca_itd) || !reader.u32(scaled_itd)
                || !reader.u32(vector.all_pass_delay_samples)
                || !reader.u32(vector.all_pass_coefficient_f32_bits)
                || !reader.u32(vector.score_count)
                || !reader.u32(vector.manager_score_count)
                || (version >= 14u && !reader.u32(has_validation))
                || has_validation > 1u
                || vector.score_count > component_count_
                || vector.manager_score_count > component_count_) {
                error = "invalid HPv2 EarlyReflection validation vector";
                clear();
                return false;
            }
            vector.azimuth_q23_degrees = static_cast<std::int32_t>(azimuth);
            vector.elevation_q23_degrees = static_cast<std::int32_t>(elevation);
            vector.distance_q23 = static_cast<std::int32_t>(distance);
            vector.pca_itd_samples = static_cast<std::int32_t>(pca_itd);
            vector.scaled_itd_samples = static_cast<std::int32_t>(scaled_itd);
            vector.has_validation = has_validation != 0u;
            vector.first_pca_gain_f32_bits.resize(vector.score_count);
            vector.second_pca_gain_f32_bits.resize(vector.score_count);
            if (!reader.bytes(vector.first_pca_gain_f32_bits.data(),
                              vector.score_count * sizeof(std::uint32_t))
                || !reader.bytes(vector.second_pca_gain_f32_bits.data(),
                                 vector.score_count * sizeof(std::uint32_t))
                || !reader.bytes(vector.distance_output_f32_bits.data(),
                                 sizeof(vector.distance_output_f32_bits))
                || !reader.bytes(vector.gain_output_f32_bits.data(),
                                 sizeof(vector.gain_output_f32_bits))
                || !reader.bytes(vector.diffusion_output_f32_bits.data(),
                                 sizeof(vector.diffusion_output_f32_bits))
                || !reader.bytes(vector.first_output_f32_bits.data(),
                                 sizeof(vector.first_output_f32_bits))
                || !reader.bytes(vector.second_output_f32_bits.data(),
                                 sizeof(vector.second_output_f32_bits))) {
                error = "truncated HPv2 EarlyReflection vector blocks";
                clear();
                return false;
            }
            auto read_manager = [&reader, &vector](
                std::vector<std::array<std::uint32_t, 32>>& blocks) {
                blocks.resize(vector.score_count);
                for (auto& block : blocks)
                    if (!reader.bytes(block.data(), sizeof(block))) return false;
                return true;
            };
            if (!read_manager(vector.first_manager_before)
                || !read_manager(vector.second_manager_before)
                || !read_manager(vector.first_manager_after)
                || !read_manager(vector.second_manager_after)) {
                error = "truncated HPv2 EarlyReflection manager blocks";
                clear();
                return false;
            }
        }
        std::uint32_t late_validation_count = 0;
        if (!reader.u32(late_validation_count) || late_validation_count > 256u) {
            error = "invalid HPv2 LateReverb validation-vector count";
            clear();
            return false;
        }
        late_reverb_validation_vectors_.resize(late_validation_count);
        for (auto& vector : late_reverb_validation_vectors_) {
            std::uint32_t azimuth = 0, elevation = 0, pca_itd = 0;
            if (!reader.u32(vector.slot) || !reader.u32(azimuth)
                || !reader.u32(elevation) || !reader.u32(vector.sample_rate)
                || !reader.u32(pca_itd) || !reader.u32(vector.score_count)
                || !reader.u32(vector.manager_score_count)
                || vector.score_count > component_count_
                || vector.manager_score_count > component_count_) {
                error = "invalid HPv2 LateReverb validation vector";
                clear();
                return false;
            }
            vector.azimuth_q23_degrees = static_cast<std::int32_t>(azimuth);
            vector.elevation_q23_degrees = static_cast<std::int32_t>(elevation);
            vector.pca_itd_samples = static_cast<std::int32_t>(pca_itd);
            vector.first_pca_gain_f32_bits.resize(vector.score_count);
            vector.second_pca_gain_f32_bits.resize(vector.score_count);
            if (!reader.bytes(vector.first_pca_gain_f32_bits.data(),
                              vector.score_count * sizeof(std::uint32_t))
                || !reader.bytes(vector.second_pca_gain_f32_bits.data(),
                                 vector.score_count * sizeof(std::uint32_t))
                || !reader.bytes(vector.input_f32_bits.data(),
                                 sizeof(vector.input_f32_bits))) {
                error = "truncated HPv2 LateReverb vector blocks";
                clear();
                return false;
            }
            auto read_manager = [&reader, &vector](
                std::vector<std::array<std::uint32_t, 32>>& blocks) {
                blocks.resize(vector.score_count);
                for (auto& block : blocks)
                    if (!reader.bytes(block.data(), sizeof(block))) return false;
                return true;
            };
            if (!read_manager(vector.first_manager_before)
                || !read_manager(vector.second_manager_before)
                || !read_manager(vector.first_manager_after)
                || !read_manager(vector.second_manager_after)) {
                error = "truncated HPv2 LateReverb manager blocks";
                clear();
                return false;
            }
        }
        std::uint32_t fgwht_validation_count = 0;
        if (!reader.u32(fgwht_validation_count)
            || fgwht_validation_count > 256u) {
            error = "invalid HPv2 FGWHT validation-vector count";
            clear();
            return false;
        }
        fgwht_validation_vectors_.resize(fgwht_validation_count);
        for (auto& vector : fgwht_validation_vectors_) {
            if (!reader.u32(vector.native_call_index)
                || !reader.u32(vector.count)
                || vector.count < 2u || vector.count > 32u
                || (vector.count & (vector.count - 1u)) != 0u
                || !reader.bytes(vector.state_before_f32_bits.data(),
                                 sizeof(vector.state_before_f32_bits))
                || !reader.bytes(vector.state_after_f32_bits.data(),
                                 sizeof(vector.state_after_f32_bits))) {
                error = "invalid HPv2 FGWHT validation vector";
                clear();
                return false;
            }
            vector.blocks_before_f32_bits.resize(vector.count);
            vector.blocks_after_f32_bits.resize(vector.count);
            for (auto& block : vector.blocks_before_f32_bits) {
                if (!reader.bytes(block.data(), sizeof(block))) {
                    error = "truncated HPv2 FGWHT input blocks";
                    clear();
                    return false;
                }
            }
            for (auto& block : vector.blocks_after_f32_bits) {
                if (!reader.bytes(block.data(), sizeof(block))) {
                    error = "truncated HPv2 FGWHT output blocks";
                    clear();
                    return false;
                }
            }
        }

        std::uint32_t core_config_count = 0;
        if (!reader.u32(core_config_count) || core_config_count > 8u) {
            error = "invalid HPv2 LateReverb core-config count";
            clear();
            return false;
        }
        late_reverb_core_configs_.resize(core_config_count);
        for (auto& config : late_reverb_core_configs_) {
            std::uint32_t band_count = 0;
            if (!reader.u32(config.sample_rate)
                || !reader.u32(config.input_filter_enabled)
                || !reader.u32(config.mode)
                || !reader.u32(config.output_layout)
                || !reader.u32(config.all_pass_count)
                || !reader.u32(config.output_gain_f32_bits)
                || !reader.u32(config.damping_f32_bits)
                || !reader.u32(config.rt60_f32_bits)
                || !reader.u32(config.input_delay_samples)
                || !reader.bytes(config.input_filter_f32_bits.data(),
                                 sizeof(config.input_filter_f32_bits))
                || !reader.bytes(config.damping_correction_filter_f32_bits.data(),
                                 sizeof(config.damping_correction_filter_f32_bits))
                || !reader.bytes(config.band_damping_filter_f32_bits.data(),
                                 sizeof(config.band_damping_filter_f32_bits))
                || !reader.bytes(config.fgwht_state_f32_bits.data(),
                                 sizeof(config.fgwht_state_f32_bits))
                || !reader.u32(config.modulation_depth_f32_bits)
                || !reader.u32(config.modulation_rate_f32_bits)
                || !reader.u32(config.modulation_phase_f32_bits)
                || !reader.u32(band_count)
                || !config.sample_rate || !config.all_pass_count
                || config.all_pass_count > 16u || !band_count
                || band_count > 32u) {
                error = "invalid HPv2 LateReverb core config";
                clear();
                return false;
            }
            config.bands.resize(band_count);
            for (auto& band : config.bands) {
                std::uint32_t all_pass_count = 0;
                if (!reader.u32(band.slot)
                    || !reader.u32(band.delay_samples)
                    || !reader.u32(band.feedback_gain_f32_bits)
                    || !reader.u32(band.total_delay_samples)
                    || !reader.u32(all_pass_count)
                    || all_pass_count != config.all_pass_count) {
                    error = "invalid HPv2 LateReverb band config";
                    clear();
                    return false;
                }
                band.all_passes.resize(all_pass_count);
                for (auto& all_pass : band.all_passes) {
                    if (!reader.u32(all_pass.delay_samples)
                        || !reader.u32(all_pass.coefficient_f32_bits)) {
                        error = "truncated HPv2 LateReverb AllPass config";
                        clear();
                        return false;
                    }
                }
            }
        }

        std::uint32_t core_sequence_count = 0;
        if (!reader.u32(core_sequence_count) || core_sequence_count > 4096u) {
            error = "invalid HPv2 LateReverb core-sequence count";
            clear();
            return false;
        }
        late_reverb_core_sequence_.resize(core_sequence_count);
        for (auto& vector : late_reverb_core_sequence_) {
            std::uint32_t output_count = 0;
            if (!reader.u32(vector.native_call_index)
                || !reader.u32(vector.slot)
                || !reader.u32(output_count)
                || !output_count || output_count > 32u
                || !reader.bytes(vector.input_f32_bits.data(),
                                 sizeof(vector.input_f32_bits))) {
                error = "invalid HPv2 LateReverb core-sequence vector";
                clear();
                return false;
            }
            vector.output_f32_bits.resize(output_count);
            for (auto& block : vector.output_f32_bits) {
                if (!reader.bytes(block.data(), sizeof(block))) {
                    error = "truncated HPv2 LateReverb core output";
                    clear();
                    return false;
                }
            }
        }

        std::uint32_t manager_config_count = 0;
        if (!reader.u32(manager_config_count) || manager_config_count > 8u) {
            error = "invalid HPv2 source-Manager config count";
            clear();
            return false;
        }
        source_manager_configs_.resize(manager_config_count);
        for (auto& config : source_manager_configs_) {
            if (!reader.u32(config.sample_rate)
                || !reader.u32(config.source_count)
                || !reader.u32(config.early_reflection_count)
                || !reader.u32(config.late_reverb_source_count)
                || !reader.u32(config.pca_score_count)
                || !reader.u32(config.reflection_enabled)
                || !reader.u32(config.reflection_mode)
                || !reader.u32(config.late_reverb_disabled)
                || !reader.u32(config.pca_filter_order_index)
                || !reader.u32(config.first_pca_count)
                || !reader.u32(config.second_pca_count)
                || !config.source_count || config.source_count > 32u) {
                error = "invalid HPv2 source-Manager config";
                clear();
                return false;
            }
            config.late_input_gain_f32_bits.resize(config.source_count);
            if (!reader.bytes(config.late_input_gain_f32_bits.data(),
                              config.source_count * sizeof(std::uint32_t))) {
                error = "truncated HPv2 source-Manager gains";
                clear();
                return false;
            }
            std::uint32_t wall_count = 0;
            if (!reader.u32(wall_count) || wall_count != config.source_count) {
                error = "invalid HPv2 source-Manager WallMaterial count";
                clear();
                return false;
            }
            config.wall_material_enabled.resize(config.source_count);
            config.wall_first_coefficients_f32_bits.resize(config.source_count);
            config.wall_second_coefficients_f32_bits.resize(config.source_count);
            for (std::size_t source = 0u; source < config.source_count; ++source) {
                std::uint32_t enabled = 0;
                if (!reader.u32(enabled)
                    || enabled > 1u
                    || !reader.bytes(config.wall_first_coefficients_f32_bits[source].data(),
                                     5u * sizeof(std::uint32_t))
                    || !reader.bytes(config.wall_second_coefficients_f32_bits[source].data(),
                                     5u * sizeof(std::uint32_t))) {
                    error = "truncated HPv2 source-Manager WallMaterial config";
                    clear();
                    return false;
                }
                config.wall_material_enabled[source] =
                    static_cast<std::uint8_t>(enabled);
            }
        }

        std::uint32_t manager_sequence_count = 0;
        if (!reader.u32(manager_sequence_count)
            || manager_sequence_count > 4096u) {
            error = "invalid HPv2 source-Manager sequence count";
            clear();
            return false;
        }
        source_manager_sequence_.resize(manager_sequence_count);
        for (auto& vector : source_manager_sequence_) {
            std::uint32_t input_count = 0;
            if (!reader.u32(vector.native_call_index)
                || !reader.u32(vector.slot)
                || !reader.u32(input_count)
                || !input_count || input_count > 32u) {
                error = "invalid HPv2 source-Manager sequence vector";
                clear();
                return false;
            }
            vector.input_f32_bits.resize(input_count);
            for (auto& block : vector.input_f32_bits) {
                if (!reader.bytes(block.data(), sizeof(block))) {
                    error = "truncated HPv2 source-Manager input";
                    clear();
                    return false;
                }
            }
            if (!reader.bytes(vector.output_before_f32_bits.data(),
                              sizeof(vector.output_before_f32_bits))
                || !reader.bytes(vector.output_after_f32_bits.data(),
                                 sizeof(vector.output_after_f32_bits))
                || !reader.bytes(vector.score_f32_bits.data(),
                                 sizeof(vector.score_f32_bits))) {
                error = "truncated HPv2 source-Manager output";
                clear();
                return false;
            }
        }
    } catch (...) {
        error = "HPv2 bank allocation failed";
        clear();
        return false;
    }
    if (reader.remaining() != 0) {
        error = "trailing bytes in HPv2 bank";
        clear();
        return false;
    }
    return true;
}

bool HeadphonePcaBank::load_embedded(std::string& error) {
    return load_embedded_resource(103u, error);
}

bool HeadphonePcaBank::load_embedded_resource(unsigned resource_id,
                                               std::string& error) {
    HRSRC resource = FindResourceW(nullptr, MAKEINTRESOURCEW(resource_id),
                                   MAKEINTRESOURCEW(10));
    if (!resource) { error = "embedded HPv2 bank resource not found"; return false; }
    HGLOBAL loaded = LoadResource(nullptr, resource);
    const void* data = loaded ? LockResource(loaded) : nullptr;
    const DWORD size = SizeofResource(nullptr, resource);
    if (!data || !size) { error = "embedded HPv2 bank resource is empty"; return false; }
    return load(data, static_cast<std::size_t>(size), error);
}

bool HeadphonePcaBank::apply_am4hp_static_source_distances(
    unsigned preset, unsigned layout) noexcept {
    if (preset > 3u || source_manager_configs_.size() != 1u
        || source_manager_configs_.front().source_count != 9u
        || explicit_validation_vectors_.empty())
        return false;
    static const float kNearDistance[] = {2.0f, 3.0f, 5.0f, 6.0f};
    const float near_distance = kNearDistance[preset];
    const unsigned near_count = ((~layout & 0x30u) != 0u) ? 3u : 5u;
    constexpr float kFarDistance = 10.0f;
    constexpr float kQ23Scale = 8388608.0f;
    constexpr float kInvQ23 = 0.00000011920929f;
    constexpr float kSoundSpeed = 340.0f;
    float min_distance = near_distance;
    if (kFarDistance < min_distance)
        min_distance = kFarDistance;
    for (auto& vector : explicit_validation_vectors_) {
        std::uint32_t input_slot = 0u;
        if (vector.slot >= 9u && vector.slot < 18u)
            input_slot = vector.slot - 9u;
        else if (vector.slot < 9u)
            input_slot = vector.slot;
        else
            return false;
        const float distance =
            input_slot < near_count ? near_distance : kFarDistance;
        if (vector.sample_rate == 0u)
            return false;
        volatile float scaled = distance * kQ23Scale;
        const float rounding = scaled < 0.0f ? -0.5f : 0.5f;
        const int distance_q23 =
            static_cast<int>(static_cast<float>(scaled + rounding));
        volatile float meters = static_cast<float>(distance_q23) * kInvQ23;
        volatile float raw = meters * static_cast<float>(
            static_cast<int>(vector.sample_rate)) / kSoundSpeed;
        const int raw_delay = static_cast<int>(static_cast<float>(raw));
        volatile float min_raw = min_distance * static_cast<float>(
            static_cast<int>(vector.sample_rate)) / kSoundSpeed;
        const float compensation = static_cast<float>(
            static_cast<int>(static_cast<float>(min_raw)));
        const int delay = static_cast<int>(
            static_cast<float>(raw_delay) - compensation);
        if (delay < 0)
            return false;
        volatile float gain_meters = static_cast<float>(distance_q23) * kInvQ23;
        const float gain = 1.0f / (gain_meters > 0.1f ? static_cast<float>(gain_meters) : 0.1f);
        vector.distance_delay_samples = static_cast<std::uint32_t>(delay);
        std::memcpy(&vector.distance_gain_f32_bits, &gain, sizeof(gain));
    }
    return true;
}

const HeadphonePcaDefinition* HeadphonePcaBank::find_definition(
    std::uint32_t sample_rate) const noexcept {
    const auto found = std::find_if(definitions_.begin(), definitions_.end(),
        [sample_rate](const HeadphonePcaDefinition& item) {
            return item.sample_rate == sample_rate;
        });
    return found == definitions_.end() ? nullptr : &*found;
}

const HeadphonePcaGridEntry* HeadphonePcaBank::grid_entry(
    std::uint32_t sample_rate, std::uint32_t azimuth_index,
    std::uint32_t elevation_index) const noexcept {
    const auto* definition = find_definition(sample_rate);
    if (!definition || azimuth_index >= azimuth_count_
        || elevation_index >= elevation_count_) return nullptr;
    return &definition->grid[elevation_index * azimuth_count_ + azimuth_index];
}

const HeadphonePcaFilterRecord* HeadphonePcaBank::filter(
    std::uint32_t sample_rate, std::uint32_t component, std::uint32_t ear,
    std::uint32_t order_index) const noexcept {
    const auto* definition = find_definition(sample_rate);
    if (!definition || component >= component_count_ || ear >= 2
        || order_index >= filter_orders_.size()) return nullptr;
    return &definition->filters[ear][component * filter_orders_.size() + order_index];
}

bool HeadphonePcaBank::interpolate(
    std::uint32_t sample_rate, std::int32_t azimuth_q23_degrees,
    std::int32_t elevation_q23_degrees, std::uint32_t score_count,
    std::int32_t& itd_samples, std::vector<float>& first_ear,
    std::vector<float>& second_ear) const noexcept {
    const auto* definition = find_definition(sample_rate);
    GridSplit split;
    if (!definition || score_count > component_count_
        || !split_angle_grid(azimuth_q23_degrees, azimuth_count_,
                             elevation_q23_degrees, elevation_count_, split))
        return false;
    const std::int32_t cross = native_round_q23(
        static_cast<std::int64_t>(split.azimuth_fraction)
        * split.elevation_fraction);
    const std::int32_t weights[4] = {
        kQ23 - split.elevation_fraction - (split.azimuth_fraction - cross),
        split.azimuth_fraction - cross,
        split.elevation_fraction - cross,
        cross,
    };
    const HeadphonePcaGridEntry* entries[4] = {
        grid_entry(sample_rate, split.azimuth[0], split.elevation[0]),
        grid_entry(sample_rate, split.azimuth[1], split.elevation[0]),
        grid_entry(sample_rate, split.azimuth[0], split.elevation[1]),
        grid_entry(sample_rate, split.azimuth[1], split.elevation[1]),
    };
    try {
        first_ear.assign(score_count, 0.0f);
        second_ear.assign(score_count, 0.0f);
    } catch (...) {
        return false;
    }
    std::int32_t itd_q23 = 0;
    for (std::uint32_t corner = 0; corner < 4; ++corner) {
        if (weights[corner] < 839) continue;
        itd_q23 += native_round_q23(
            static_cast<std::int64_t>(entries[corner]->itd_samples)
            * kQ23 * weights[corner]);
        const float gain = static_cast<float>(weights[corner])
            * 0x1.0p-23f;
        for (std::uint32_t component = 0; component < score_count; ++component) {
            first_ear[component] = multiply_add_separate(
                first_ear[component], gain,
                bits_to_float(entries[corner]->first_ear_f32_bits[component]));
            second_ear[component] = multiply_add_separate(
                second_ear[component], gain,
                bits_to_float(entries[corner]->second_ear_f32_bits[component]));
        }
    }
    itd_samples = itd_q23 < 0 ? -((-itd_q23) >> 23) : itd_q23 >> 23;
    return true;
}

bool HeadphonePcaBank::interpolate_itd(
    std::uint32_t sample_rate, std::int32_t azimuth_q23_degrees,
    std::int32_t elevation_q23_degrees, std::int32_t& itd_samples) const noexcept {
    std::vector<float> first, second;
    return interpolate(sample_rate, azimuth_q23_degrees,
                       elevation_q23_degrees, 0u, itd_samples, first, second);
}

bool HeadphonePcaBank::populate_score_state(
    std::uint32_t sample_rate, std::int32_t azimuth_q23_degrees,
    std::int32_t elevation_q23_degrees,
    HeadphonePcaScoreState& state) const noexcept {
    std::int32_t itd_samples = 0;
    std::vector<float> first, second;
    if (!interpolate(sample_rate, azimuth_q23_degrees,
                     elevation_q23_degrees,
                     static_cast<std::uint32_t>(state.score_count()),
                     itd_samples, first, second)
        || !state.set_gains(first, second))
        return false;
    state.set_itd_samples(itd_samples);
    return true;
}

}
