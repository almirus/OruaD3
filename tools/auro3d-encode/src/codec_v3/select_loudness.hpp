#pragma once

#include "layout_metadata.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Native 36-byte measurement copied into schedule entry+20 by
/// Encoder::get_dynamic_params / downmix_ follow-up @ 0x4E3xxx.
struct LoudnessMeasurement {
    std::uint32_t type = 0;
    bool value_absent = false;
    float value = 0.0f;
    bool value_2_present = false;
    bool value_2_absent = false;
    float value_2 = 0.0f;
    bool value_3_present = false;
    bool value_3_absent = false;
    float value_3 = 0.0f;
};

/// One 64-byte Encoder+12680 schedule entry.
struct LoudnessScheduleEntry {
    std::uint32_t type = 0;
    std::int64_t cursor = -1;
    std::int32_t period = 0;
    LoudnessMeasurement measurement{};
    bool enabled = false;
};

/// Default six-entry schedule from Encoder construct path @ 0x4E2xxx:
/// types 5,4,0,1,2,3 with periods trunc(0.0833*sr), trunc(0.3125*sr),
/// and trunc(0.9583*sr) for the rest. Entries start disabled.
std::vector<LoudnessScheduleEntry> make_default_loudness_schedule(
    std::uint32_t sample_rate);

/// Copies a measurement into every matching-type entry and enables it,
/// matching the +968/+976 update loop.
bool apply_loudness_measurement(
    std::vector<LoudnessScheduleEntry>& schedule,
    const LoudnessMeasurement& measurement,
    std::string& error);

/// Encoder::select_loudness_ @ 0x4E48xx. `unit_start_sample` is Encoder+6336
/// (sample cursor before this unit). On success, `present` mirrors +12712 and
/// `out` holds the selected LoudnessMetadata when present.
bool select_loudness(
    std::vector<LoudnessScheduleEntry>& schedule,
    std::uint64_t unit_start_sample,
    bool& present,
    LoudnessMetadata& out,
    std::string& error);

} // namespace auro3d::encode
