#include "cycler.hpp"

#include <limits>

namespace auro3d::encode {

bool MetadataCycler::due(
    std::uint64_t current_sample_end) const {
    const std::int64_t end = current_sample_end > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())
        ? std::numeric_limits<std::int64_t>::max()
        : static_cast<std::int64_t>(current_sample_end);
    return configured && enabled && period != 0u
        && cursor >= 0 && cursor < end;
}

bool MetadataCycler::consume(
    std::uint64_t current_sample_end) {
    const bool selected = due(current_sample_end);
    if (selected) {
        const std::uint64_t interval = period == 0u
            ? 0u : static_cast<std::uint64_t>(period) - 1u;
        const std::uint64_t scheduled = current_sample_end >
                std::numeric_limits<std::uint64_t>::max() - interval
            ? std::numeric_limits<std::uint64_t>::max()
            : current_sample_end + interval;
        cursor = scheduled > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())
            ? std::numeric_limits<std::int64_t>::max()
            : static_cast<std::int64_t>(scheduled);
    } else if (cursor < 0) {
        ++cursor;
    }
    return selected;
}

void MetadataCycler::advance(std::uint64_t current_sample_end) {
    static_cast<void>(consume(current_sample_end));
}

EncoderMetadataSelection EncoderCyclers::select_and_advance(
    std::uint64_t current_sample_end) {
    // Same transition order as Encoder:update_cyclers_.
    EncoderMetadataSelection selected{};
    selected.auromatic = auromatic.consume(current_sample_end);
    selected.opcode_50 = opcode_50.consume(current_sample_end);
    selected.encoder_version =
        encoder_version.consume(current_sample_end);
    selected.opcode_6e = opcode_6e.consume(current_sample_end);
    // Native's final secondary-downmix branch has no negative-cursor
    // increment fallback. set_dynamic_params arms it at zero, but retain the
    // exact stationary-negative behavior for an unarmed/default cycler.
    selected.secondary_downmix =
        secondary_downmix.due(current_sample_end);
    if (selected.secondary_downmix)
        static_cast<void>(secondary_downmix.consume(current_sample_end));
    return selected;
}

void EncoderCyclers::advance(std::uint64_t current_sample_end) {
    static_cast<void>(select_and_advance(current_sample_end));
}

std::uint32_t default_metadata_cycler_period(std::uint32_t sample_rate) {
    const double rate =
        static_cast<double>(static_cast<std::int32_t>(sample_rate));
    return static_cast<std::uint32_t>(static_cast<std::int32_t>(rate * 0.9583));
}

void init_metadata_cycler_periods(
    EncoderCyclers& cyclers,
    std::uint32_t sample_rate) {
    const std::uint32_t period = default_metadata_cycler_period(sample_rate);
    MetadataCycler* const all[] = {
        &cyclers.auromatic,
        &cyclers.secondary_downmix,
        &cyclers.opcode_50,
        &cyclers.encoder_version,
        &cyclers.opcode_6e,
    };
    for (MetadataCycler* cycler : all) {
        cycler->period = period;
        cycler->configured = period != 0u;
    }
}

void apply_construct_default_cycler_payloads(EncoderCyclers& cyclers) {
    // Encoder:set_cyclers_:
    // if (!+6211) { dword(+6208)=17171203; cursor=-1; }
    // The high byte of 0x01060303 is +6211, so this write enables the cycler.
    // else if bytes != 3.3.6 { force 3.3.6; cursor = -1; }
    if (!cyclers.encoder_version.enabled) {
        cyclers.encoder_version.armed_value = kConstructDefaultEncoderVersion;
        cyclers.encoder_version.enabled = true;
        cyclers.encoder_version.cursor = -1;
    } else if (cyclers.encoder_version.armed_value
        != kConstructDefaultEncoderVersion) {
        cyclers.encoder_version.armed_value = kConstructDefaultEncoderVersion;
        cyclers.encoder_version.cursor = -1;
    }
    // if (!+6236 || +6232 != -1659869902) {
    // +6232 = -1659869902; +6236 = 1; cursor = -2; }
    if (!cyclers.opcode_6e.enabled
        || cyclers.opcode_6e.armed_value != kConstructDefaultOpcode6eValue) {
        cyclers.opcode_6e.armed_value = kConstructDefaultOpcode6eValue;
        cyclers.opcode_6e.enabled = true;
        cyclers.opcode_6e.cursor = -2;
    }
}

void arm_metadata_cycler_on_value_change(
    MetadataCycler& cycler,
    std::uint32_t value) {
    // set_dynamic_params secondary opcode_50:
    // if (!enabled || value != new) { value=new; enabled=1; cursor=0; }
    if (!cycler.enabled || cycler.armed_value != value) {
        cycler.armed_value = value;
        cycler.enabled = true;
        cycler.cursor = 0;
    }
}

void arm_auromatic_cycler_on_value_change(
    MetadataCycler& cycler,
    std::uint32_t value) {
    // set_dynamic_params auromatic:
    // if (enabled && word == new) return;
    // if (!enabled) enabled=1; word=new; cursor=0;
    if (cycler.enabled && cycler.armed_value == value)
        return;
    if (!cycler.enabled)
        cycler.enabled = true;
    cycler.armed_value = value;
    cycler.cursor = 0;
}

} // namespace auro3d:encode
