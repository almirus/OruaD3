#pragma once

#include "encoder_defaults.hpp"
#include "layout_metadata.hpp"
#include "dynamic_params.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace auro3d::encode {

/// Native optional dword pair: present flag followed by value.
struct OptionalU32 {
    std::uint32_t present = 0;
    std::uint32_t value = 0;
};

/// bit_line at Config+16: present dword, then low/high payload dwords (+20/+24).
struct OptionalBitLine {
    std::uint32_t present = 0;
    std::uint32_t low = 0;
    std::uint32_t high = 0;
};

/// Fields used by Config::init_defaults @ 0x4F8900 and Config::validate @ 0x4F8AE0.
struct EncoderConfig {
    std::uint32_t sample_rate = 0;       // +0
    std::uint32_t original_layout = 0;   // +4
    std::uint32_t unit_block_size = 0;   // +8
    std::uint32_t profile = 0;           // +12; 0 becomes 2
    OptionalBitLine bit_line{};          // +16/+20/+24
    OptionalU32 field_28{};              // +28/+32
    OptionalU32 thread_workers{};        // +36 present, +40 byte value
    OptionalU32 field_44{};              // +44/+48
    OptionalU32 field_52{};              // +52/+56  (dwords 13/14)
    OptionalU32 field_60{};              // +60/+64
    OptionalU32 field_68{};              // +68/+72
    OptionalU32 field_76{};              // +76/+80
    OptionalU32 field_84{};              // +84/+88
    OptionalU32 field_92{};              // +92/+96  (dwords 23/24)
    OptionalU32 field_100{};             // +100/+104
    OptionalU32 field_108{};             // +108/+112
    OptionalU32 field_116{};             // +116/+120
    OptionalU32 field_124{};             // +124/+128
    /// Native downmix gate: value at +132, optional marker at +136. This
    /// ordering differs from the regular OptionalU32 fields above.
    OptionalU32 field_132{};
    /// Encoder constructor RNG seed at Config+144 with presence byte +152.
    /// The native constructor uses time(nullptr) when this is absent.
    bool dither_seed_present = false;
    std::uint64_t dither_seed = 0u;
    /// Encoder+6352, written by Encoder::reserve_extra_bits @ 0x4E37E0 and
    /// copied into Group+208 for quantizer budget accounting.
    std::uint32_t reserve_extra_bits = 0u;
    NativeClusterDeltasBackend cluster_backend =
        NativeClusterDeltasBackend::gvm;
    NativeGvmConfiguration gvm{};
    /// Optional explicit input scaler table for the confirmed scalar portion
    /// of downmix_ @ 0x4E4520. It is absent by default; no gain policy is
    /// inferred from a layout or filename.
    bool input_scalers_present = false;
    std::array<std::uint8_t, 31> input_scaler_indices{};
    /// Confirmed Dynamic subset (original gains, carrier gains → secondary
    /// downmix, optional opcode_50/auromatic, loudness measurements).
    /// Original gains seed scaler indices; carrier gains feed ADOL 0x46;
    /// loudness measurements enable schedule entries.
    bool dynamic_params_present = false;
    DynamicParams dynamic_params{};
    /// Explicit optional metadata values. They do not enable DSP or select a
    /// downmix policy; callers must provide values confirmed for their mix.
    bool primary_downmix_gain_present = false;
    std::uint8_t primary_downmix_channel = 0;
    std::uint8_t primary_downmix_scaler_index = 0;
    bool secondary_downmix_gains_present = false;
    std::array<float, 31> secondary_downmix_gains_db{};
    bool loudness_metadata_present = false;
    LoudnessMetadata loudness_metadata{};
    /// Channel-compose ADOL 0x41; not a UnitBlock cycler.
    bool limit_simple_present = false;
    std::uint8_t limit_simple_scaler_index = 0;
    /// UnitBlock+16 cycler / ADOL 0x47.
    bool auromatic_present = false;
    std::uint8_t auromatic_profile = 0;
    std::uint8_t auromatic_mode = 0;
    /// UnitBlock+500 cycler / ADOL 0x50.
    bool opcode_50_present = false;
    std::uint8_t opcode_50_value = 0;
    /// UnitBlock+528 cycler / ADOL 0x64.
    bool encoder_version_present = false;
    std::uint32_t encoder_version = 0;
    /// UnitBlock+536 cycler / ADOL 0x6E.
    bool opcode_6e_present = false;
    std::uint32_t opcode_6e_value = 0;
};

std::uint32_t encoder_config_init_defaults(EncoderConfig& config);
std::uint32_t encoder_config_validate(const EncoderConfig& config);

bool encoder_config_prepare(
    EncoderConfig& config,
    std::uint32_t sample_rate,
    std::uint32_t original_layout,
    std::uint32_t unit_block_size,
    std::uint32_t profile,
    std::string& error);

/// Clones a prepared configuration for another native UnitBlock size without
/// discarding explicit scaler, metadata, seed, or backend selections.
bool encoder_config_with_unit_block_size(
    const EncoderConfig& base,
    std::uint32_t unit_block_size,
    EncoderConfig& config,
    std::string& error);

} // namespace auro3d::encode
