#include "dynamic_params.hpp"

#include "scaler.hpp"

#include <cmath>
#include <limits>

namespace auro3d::encode {
namespace {

bool validate_loudness_measurement(
    const LoudnessMeasurement& measurement,
    std::string& error) {
    auto in_range = [](float value, float lo, float hi) {
        return value >= lo && value <= hi;
    };
    // set_dynamic_params rejects value_absent on the primary float and on
    // every present optional float (the +4/+16/+28 "absent" dwords).
    if (measurement.value_absent) {
        error = "dynamic loudness primary value absent flag must be clear";
        return false;
    }
    switch (measurement.type) {
    case 0u:
    case 2u:
        if (!in_range(measurement.value, -35.0f, -2.0f)
            || (measurement.value_2_present
                && (measurement.value_2_absent
                    || !in_range(measurement.value_2, -35.0f, -2.0f)))
            || (measurement.value_3_present
                && (measurement.value_3_absent
                    || !in_range(measurement.value_3, -35.0f, -2.0f)))) {
            error = "dynamic loudness type 0/2 value is out of range";
            return false;
        }
        break;
    case 1u:
        if (!in_range(measurement.value, 0.0f, 30.0f)
            || (measurement.value_2_present
                && (measurement.value_2_absent
                    || !in_range(measurement.value_2, 0.0f, 30.0f)))
            || (measurement.value_3_present
                && (measurement.value_3_absent
                    || !in_range(measurement.value_3, 0.0f, 30.0f)))) {
            error = "dynamic loudness type 1 value is out of range";
            return false;
        }
        break;
    case 3u:
        if (!in_range(measurement.value, -1.0f, 4.0f)
            || (measurement.value_2_present
                && (measurement.value_2_absent
                    || !in_range(measurement.value_2, -1.0f, 4.0f)))
            || (measurement.value_3_present
                && (measurement.value_3_absent
                    || !in_range(measurement.value_3, -1.0f, 4.0f)))) {
            error = "dynamic loudness type 3 value is out of range";
            return false;
        }
        break;
    case 4u:
    case 5u:
        if (!in_range(measurement.value, -100.0f, 0.0f)
            || (measurement.value_2_present
                && (measurement.value_2_absent
                    || !in_range(measurement.value_2, -100.0f, 0.0f)))
            || (measurement.value_3_present
                && (measurement.value_3_absent
                    || !in_range(measurement.value_3, -100.0f, 0.0f)))) {
            error = "dynamic loudness type 4/5 value is out of range";
            return false;
        }
        break;
    default:
        error = "dynamic loudness type is outside native range";
        return false;
    }
    return true;
}

bool validate_gain_table(
    const std::array<DynamicChannelGain, 31>& gains,
    std::uint32_t layout_mask,
    const char* outside_layout_error,
    const char* range_error,
    std::string& error) {
    for (std::uint32_t id = 0; id < 31u; ++id) {
        const DynamicChannelGain& gain = gains[id];
        const bool in_layout =
            (layout_mask & (std::uint32_t{1} << id)) != 0u;
        if (!in_layout) {
            if (gain.present) {
                error = outside_layout_error;
                return false;
            }
            continue;
        }
        if (!gain.present)
            continue;
        if (!std::isfinite(gain.gain_db)
            || gain.gain_db < -24.0f || gain.gain_db > 0.0f) {
            error = range_error;
            return false;
        }
    }
    return true;
}

float cts_gain_from_table_scaler(float table_scaler) {
    // set_dynamic_params @ 0x4E3xxx after scaler_to_ix:
    //   v38 = -144; if (scaler > 0) v38 = max(20*log10(scaler), -144);
    //   Encoder+6384 = -v38;
    float v38 = -144.0f;
    if (table_scaler > 0.0f)
        v38 = std::fmax(std::log10(table_scaler) * 20.0f, -144.0f);
    return -v38;
}

} // namespace

bool validate_dynamic_params(
    const DynamicParams& params,
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    std::string& error) {
    error.clear();
    if (!validate_gain_table(
            params.original_gains,
            original_layout,
            "dynamic original gain present outside layout",
            "dynamic original gain is outside [-24,0] dB",
            error)) {
        return false;
    }
    // Second 31×12-byte table validated against Encoder+168 (carrier mask);
    // native rejection status 396.
    if (!validate_gain_table(
            params.carrier_gains,
            carrier_layout,
            "dynamic carrier gain present outside carrier layout",
            "dynamic carrier gain is outside [-24,0] dB",
            error)) {
        return false;
    }
    // v51[91]/v51[92]: present opcode_50 must be <= 3.
    if (params.opcode_50.present && params.opcode_50.value > 3u) {
        error = "dynamic opcode 0x50 value must be <= 3";
        return false;
    }
    // v51[93]/v52/v53: auromatic profile and mode must each be <= 15.
    if (params.auromatic.present
        && (params.auromatic.profile > 15u || params.auromatic.mode > 15u)) {
        error = "dynamic auromatic profile/mode must be <= 15";
        return false;
    }
    for (const LoudnessMeasurement& measurement :
         params.loudness_measurements) {
        if (!validate_loudness_measurement(measurement, error))
            return false;
    }
    return true;
}

bool apply_dynamic_original_gains(
    const DynamicParams& params,
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    bool write_cts_gains,
    std::array<std::uint8_t, 31>& scaler_indices,
    bool& scalers_present,
    std::array<CtsGainEntry, 31>& cts_gains,
    std::string& error) {
    error.clear();
    scalers_present = false;
    cts_gains = {};
    if (!validate_dynamic_params(
            params, original_layout, carrier_layout, error)) {
        return false;
    }

    for (std::uint32_t id = 0; id < 31u; ++id) {
        const DynamicChannelGain& gain = params.original_gains[id];
        if (!gain.present)
            continue;
        float scaler = 0.0f;
        if (!scaler_from_gain_db(gain.gain_db, scaler)) {
            error = "dynamic gain_to_scaler failed";
            return false;
        }
        std::uint8_t index = 0u;
        float table_scaler = 0.0f;
        if (!scaler_to_index(scaler, index, table_scaler)) {
            // Native rejects the Dynamic apply when scaler_to_ix fails or
            // Config+132 value is not 1. Callers must keep the downmix gate.
            error = "dynamic scaler_to_ix failed";
            return false;
        }
        scaler_indices[id] = index;
        scalers_present = true;
        if (write_cts_gains) {
            cts_gains[id].gain_db = cts_gain_from_table_scaler(table_scaler);
            cts_gains[id].present = true;
        }
    }
    return true;
}

bool apply_dynamic_carrier_gains(
    const DynamicParams& params,
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    bool& secondary_present,
    std::array<float, 31>& secondary_gains_db,
    std::string& error) {
    error.clear();
    secondary_present = false;
    if (!validate_dynamic_params(
            params, original_layout, carrier_layout, error)) {
        return false;
    }

    bool any_present = false;
    for (const DynamicChannelGain& gain : params.carrier_gains) {
        if (gain.present) {
            any_present = true;
            break;
        }
    }
    if (!any_present)
        return true;

    // Native packs present middle+float qwords into a 31-slot optional array
    // (absent slots remain zero) then calls from_secondary_downmix_gains with
    // Encoder+168. Mirror that as float dB gains for ADOL 0x46.
    secondary_gains_db = {};
    for (std::uint32_t id = 0; id < 31u; ++id) {
        const DynamicChannelGain& gain = params.carrier_gains[id];
        if (gain.present)
            secondary_gains_db[id] = gain.gain_db;
    }
    secondary_present = true;
    return true;
}

bool apply_dynamic_cycler_payloads(
    const DynamicParams& params,
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    bool& opcode_50_present,
    std::uint32_t& opcode_50_value,
    bool& auromatic_present,
    std::uint32_t& auromatic_profile,
    std::uint32_t& auromatic_mode,
    std::string& error) {
    error.clear();
    if (!validate_dynamic_params(
            params, original_layout, carrier_layout, error)) {
        return false;
    }
    // Encoder+952/+956 → cycler +6280 when present.
    if (params.opcode_50.present) {
        opcode_50_present = true;
        opcode_50_value = params.opcode_50.value;
    }
    // Encoder+960/+964 word → cycler +6304 when present.
    if (params.auromatic.present) {
        auromatic_present = true;
        auromatic_profile = params.auromatic.profile;
        auromatic_mode = params.auromatic.mode;
    }
    return true;
}

bool apply_dynamic_loudness_measurements(
    const DynamicParams& params,
    std::vector<LoudnessScheduleEntry>& schedule,
    std::string& error) {
    error.clear();
    for (const LoudnessMeasurement& measurement :
         params.loudness_measurements) {
        if (!validate_loudness_measurement(measurement, error))
            return false;
        if (!apply_loudness_measurement(schedule, measurement, error))
            return false;
    }
    return true;
}

} // namespace auro3d::encode
