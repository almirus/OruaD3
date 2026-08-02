#include "select_loudness.hpp"

#include <limits>

namespace auro3d::encode {
namespace {

LoudnessMetadata measurement_to_metadata(
    std::uint32_t schedule_type,
    const LoudnessMeasurement& measurement) {
    LoudnessMetadata out{};
    out.type = schedule_type;
    out.value = measurement.value_absent
        ? -std::numeric_limits<double>::infinity()
        : static_cast<double>(measurement.value);
    out.value_2_present = measurement.value_2_present;
    if (measurement.value_2_present) {
        out.value_2 = measurement.value_2_absent
            ? -std::numeric_limits<double>::infinity()
            : static_cast<double>(measurement.value_2);
    }
    out.value_3_present = measurement.value_3_present;
    if (measurement.value_3_present) {
        out.value_3 = measurement.value_3_absent
            ? -std::numeric_limits<double>::infinity()
            : static_cast<double>(measurement.value_3);
    }
    return out;
}

} // namespace

std::vector<LoudnessScheduleEntry> make_default_loudness_schedule(
    std::uint32_t sample_rate) {
    const double rate = static_cast<double>(static_cast<std::int32_t>(sample_rate));
    const std::int32_t period_5 =
        static_cast<std::int32_t>(0.0833 * rate);
    const std::int32_t period_4 =
        static_cast<std::int32_t>(0.3125 * rate);
    const std::int32_t period_rest =
        static_cast<std::int32_t>(rate * 0.9583);

    std::vector<LoudnessScheduleEntry> out;
    out.reserve(6u);
    const std::uint32_t types[6] = {5u, 4u, 0u, 1u, 2u, 3u};
    const std::int32_t periods[6] = {
        period_5, period_4, period_rest, period_rest, period_rest,
        period_rest,
    };
    for (std::size_t i = 0; i < 6u; ++i) {
        LoudnessScheduleEntry entry{};
        entry.type = types[i];
        entry.cursor = -1;
        entry.period = periods[i];
        entry.measurement.type = types[i];
        entry.enabled = false;
        out.push_back(entry);
    }
    return out;
}

bool apply_loudness_measurement(
    std::vector<LoudnessScheduleEntry>& schedule,
    const LoudnessMeasurement& measurement,
    std::string& error) {
    error.clear();
    if (measurement.type > 5u) {
        error = "loudness measurement type is outside native range";
        return false;
    }
    bool matched = false;
    for (LoudnessScheduleEntry& entry : schedule) {
        if (entry.type != measurement.type)
            continue;
        entry.measurement = measurement;
        entry.enabled = true;
        matched = true;
    }
    if (!matched) {
        error = "loudness measurement type has no schedule entry";
        return false;
    }
    return true;
}

bool select_loudness(
    std::vector<LoudnessScheduleEntry>& schedule,
    std::uint64_t unit_start_sample,
    bool& present,
    LoudnessMetadata& out,
    std::string& error) {
    error.clear();
    present = false;
    out = {};

    const std::int64_t unit_start =
        unit_start_sample
            > static_cast<std::uint64_t>(
                std::numeric_limits<std::int64_t>::max())
        ? std::numeric_limits<std::int64_t>::max()
        : static_cast<std::int64_t>(unit_start_sample);

    for (LoudnessScheduleEntry& entry : schedule) {
        if (!entry.enabled)
            continue;
        const bool due = entry.cursor < 0
            || unit_start
                > static_cast<std::int64_t>(entry.period) + entry.cursor;
        if (!due)
            continue;

        // from_loudness @ 0x4FFA90 + loudness::codec::encode @ 0x4FFDD0.
        if (entry.measurement.type > 5u) {
            error = "loudness schedule measurement type is invalid";
            return false;
        }
        LoudnessMetadata selected = measurement_to_metadata(
            entry.type, entry.measurement);
        std::uint32_t packed = 0u;
        if (!pack_loudness_metadata(selected, packed, error))
            return false;

        out = selected;
        present = true;
        entry.cursor = unit_start;
        // Types 4 and 5 are one-shot: (type & ~1) == 4.
        if ((entry.type & 0xFFFFFFFEu) == 4u)
            entry.enabled = false;
        return true;
    }
    return true;
}

} // namespace auro3d::encode
