#pragma once

#include <cstdint>

namespace auro3d::encode {

/// State transition shared by the five metadata cyclers in native
/// Encoder:update_cyclers_. A value is eligible only when its
/// cursor lies in [0, current_sample_end); once emitted, it is rescheduled to
/// current_sample_end + period - 1. Negative cursors advance toward zero.
/// The final secondary-downmix branch is the native exception: an unarmed
/// negative cursor stays negative until set_dynamic_params arms it at zero.

/// Native Encoder offsets and UnitBlock slots (prepare_metadata_unit_block_):
/// auromatic Encoder+6320 UnitBlock+16 → ADOL 0x47
/// secondary_downmix Encoder+6272 UnitBlock+248 → ADOL 0x46
/// opcode_50 Encoder+6296 UnitBlock+500 → ADOL 0x50
/// encoder_version Encoder+6224 UnitBlock+528 → ADOL 0x64
/// opcode_6e Encoder+6248 UnitBlock+536 → ADOL 0x6E
/// Loudness and primary-downmix 0x40 are not these cyclers.
struct MetadataCycler {
    bool configured = false;
    bool enabled = false;
    std::uint32_t period = 0;
    std::int64_t cursor = -1;
    /// Last armed payload (secondary packed dword, opcode_50 value, or
    /// auromatic LE word). Used by set_dynamic_params change detection.
    std::uint32_t armed_value = 0;

    bool due(std::uint64_t current_sample_end) const;
    bool consume(std::uint64_t current_sample_end);
    void advance(std::uint64_t current_sample_end);
};

struct EncoderMetadataSelection {
    bool auromatic = false;
    bool secondary_downmix = false;
    bool opcode_50 = false;
    bool encoder_version = false;
    bool opcode_6e = false;
};

struct EncoderCyclers {
    MetadataCycler auromatic;
    MetadataCycler secondary_downmix;
    MetadataCycler opcode_50;
    MetadataCycler encoder_version;
    MetadataCycler opcode_6e;

    EncoderMetadataSelection select_and_advance(
        std::uint64_t current_sample_end);
    void advance(std::uint64_t current_sample_end);
};

/// Construct path: all five period dwords receive
/// `trunc(0.9583 * sample_rate)`.
std::uint32_t default_metadata_cycler_period(std::uint32_t sample_rate);

/// Native set_cyclers_ seeds Encoder+6208 with LE bytes 03,03,06,01
/// (dword 17171203). The fourth byte is the enabled flag at Encoder+6211;
/// ADOL 0x64 packs the first three bytes as BE.
inline constexpr std::uint32_t kConstructDefaultEncoderVersion = 0x00030306u;

/// Native construct forces Encoder+6232 to signed -1659869902.
inline constexpr std::uint32_t kConstructDefaultOpcode6eValue =
    static_cast<std::uint32_t>(-1659869902);

/// Installs the construct-default period on every cycler. Does not enable or
/// reset cursors (set_dynamic_params owns arming).
void init_metadata_cycler_periods(
    EncoderCyclers& cyclers,
    std::uint32_t sample_rate);

/// Construct payload/cursor defaults after periods are installed:
/// - encoder_version: when disabled, store 3.3.6, enable, and set cursor=-1;
/// - opcode_6e: when disabled or value differs, store default, enable, cursor=-2.
void apply_construct_default_cycler_payloads(EncoderCyclers& cyclers);

/// set_dynamic_params secondary opcode_50 arming: when disabled or value
/// changed, store value, enable, and set cursor to 0.
void arm_metadata_cycler_on_value_change(
    MetadataCycler& cycler,
    std::uint32_t value);

/// set_dynamic_params auromatic arming: unchanged enabled value is a no-op;
/// otherwise enable (if needed), store value, cursor = 0.
void arm_auromatic_cycler_on_value_change(
    MetadataCycler& cycler,
    std::uint32_t value);

} // namespace auro3d:encode
