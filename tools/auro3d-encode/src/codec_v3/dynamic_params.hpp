#pragma once

#include "cts_dmx.hpp"
#include "select_loudness.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// One 12-byte Dynamic channel gain: present dword, zero dword, gain float.
/// Native set_dynamic_params rejects present gains outside [-24,0]
/// and requires the middle dword to be zero.
struct DynamicChannelGain {
    bool present = false;
    float gain_db = 0.0f;
};

/// Optional Dynamic opcode 0x50 at struct offset 744 (Encoder+952 after copy).
/// set_dynamic_params rejects present values greater than 3.
struct DynamicOpcode50 {
    bool present = false;
    std::uint32_t value = 0;
};

/// Optional Dynamic auromatic at struct offset 752 (Encoder+960 after copy).
/// Profile and mode bytes must each be <= 15 when present.
struct DynamicAuromatic {
    bool present = false;
    std::uint8_t profile = 0;
    std::uint8_t mode = 0;
};

/// Confirmed subset of auro_codec_v3_unit_encoder_parameter_Dynamic_t used by
/// Encoder:set_dynamic_params before/after the DeepCopy AST visitor:
/// original-layout gains, carrier-layout gains, loudness measurements, and
/// the optional opcode-0x50 auromatic payloads that feed UnitBlock cyclers.
/// encoder_version opcode 0x6E are not written by this native function.
struct DynamicParams {
    std::array<DynamicChannelGain, 31> original_gains{};
    std::array<DynamicChannelGain, 31> carrier_gains{};
    DynamicOpcode50 opcode_50{};
    DynamicAuromatic auromatic{};
    std::vector<LoudnessMeasurement> loudness_measurements{};
};

/// Validates original gains (status 395), carrier gains against the carrier
/// layout mask (status 396), opcode_50 auromatic ranges, and loudness
/// measurements against the type-specific ranges in set_dynamic_params.
bool validate_dynamic_params(
    const DynamicParams& params,
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    std::string& error);

/// gain_to_scaler + scaler_to_ix for every present original gain. Requires
/// Config+132 value == 1 (native rejects otherwise). When write_cts_gains
/// is true (Config+136), fills Encoder+6384-equivalent entries from the
/// quantized table scaler.
bool apply_dynamic_original_gains(
    const DynamicParams& params,
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    bool write_cts_gains,
    std::array<std::uint8_t, 31>& scaler_indices,
    bool& scalers_present,
    std::array<CtsGainEntry, 31>& cts_gains,
    std::string& error);

/// Maps present carrier gains into secondary-downmix float gains (ADOL 0x46
/// path via from_secondary_downmix_gains). Absent layout channels
/// stay 0 dB, matching the zeroed optional slots native packs into v59.
/// Sets secondary_present only when at least one carrier gain is present.
bool apply_dynamic_carrier_gains(
    const DynamicParams& params,
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    bool& secondary_present,
    std::array<float, 31>& secondary_gains_db,
    std::string& error);

/// Copies present Dynamic opcode_50 auromatic payloads onto the matching
/// EncoderConfig UnitBlock cycler fields (Encoder+6280 +6304).
bool apply_dynamic_cycler_payloads(
    const DynamicParams& params,
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    bool& opcode_50_present,
    std::uint32_t& opcode_50_value,
    bool& auromatic_present,
    std::uint32_t& auromatic_profile,
    std::uint32_t& auromatic_mode,
    std::string& error);

/// Enables matching loudness schedule entries from Dynamic measurements.
bool apply_dynamic_loudness_measurements(
    const DynamicParams& params,
    std::vector<LoudnessScheduleEntry>& schedule,
    std::string& error);

} // namespace auro3d:encode
