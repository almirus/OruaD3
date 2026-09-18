#pragma once

#include "adol_syntax.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Constructs the mandatory layout announcement ADOL record (opcode 0x1e)
/// from the direct native channel-input configuration table. The returned
/// vector is one ADOL block and does not include the serializer's terminator.
bool make_layout_adol_block(
    std::uint32_t original_layout,
    std::vector<AdolInstruction>& instructions,
    std::string& error);

/// Constructs ADOL opcode 0x40 only from an explicit native downmix channel
/// and scaler index. This helper is intentionally separate from any mix
/// policy, which remains in Encoder:process_groups_.
bool append_primary_downmix_gain_adol(
    std::vector<AdolInstruction>& instructions,
    std::uint32_t channel,
    std::uint32_t scaler_index,
    std::string& error);

/// Packs the explicit per-carrier-channel gains used by ADOL opcode 0x46.
/// Native `from_secondary_downmix_gains` quantizes each non-positive gain to
/// a four-bit 1.5 dB step with a -0.75 dB decision offset. Only channels
/// present in the supplied carrier layout are inspected.
bool pack_secondary_downmix_gains(
    std::uint32_t carrier_layout,
    const std::array<float, 31>& gains_db,
    std::uint32_t& packed,
    std::string& error);

/// Appends the scalar 32-bit opcode 0x46 form produced by the native metadata
/// converter. The caller owns gain policy and must provide every channel.
bool append_secondary_downmix_gains_adol(
    std::vector<AdolInstruction>& instructions,
    std::uint32_t carrier_layout,
    const std::array<float, 31>& gains_db,
    std::string& error);

/// Explicit input to `auro:loudness:codec:encode`. Type is the
/// native semantic type 0..5; the corresponding ADOL opcode is selected by
/// the codec table rather than supplied by the caller.
struct LoudnessMetadata {
    std::uint32_t type = 0;
    double value = 0.0;
    bool value_2_present = false;
    double value_2 = 0.0;
    bool value_3_present = false;
    double value_3 = 0.0;
};

/// Packs the native 32-bit loudness payload, including the two optional
/// presence bits and three ten-bit quantities.
bool pack_loudness_metadata(
    const LoudnessMetadata& loudness,
    std::uint32_t& packed,
    std::string& error);

/// Appends the native opcode mapping {0x80,0x81,0x85,0x82,0x83,0x84}.
bool append_loudness_adol(
    std::vector<AdolInstruction>& instructions,
    const LoudnessMetadata& loudness,
    std::string& error);

/// ADOL opcode 0x41 from metadata:from_limit_simple. Channel-
/// compose path (not a UnitBlock cycler); scaler index uses validate_ix.
bool append_limit_simple_adol(
    std::vector<AdolInstruction>& instructions,
    std::uint32_t scaler_index,
    std::string& error);

/// ADOL opcode 0x47 from metadata:from_auromatic. UnitBlock+16
/// cycler; both profile and mode must be < 16.
bool append_auromatic_adol(
    std::vector<AdolInstruction>& instructions,
    std::uint32_t profile,
    std::uint32_t mode,
    std::string& error);

/// ADOL opcode 0x64 from metadata:from_encoder_version.
/// `version` is the native 24-bit value (bytes packed big-endian).
bool append_encoder_version_adol(
    std::vector<AdolInstruction>& instructions,
    std::uint32_t version,
    std::string& error);

/// ADOL opcode 0x50 from UnitBlock+500 Composer. Eight-bit
/// payload; semantic name is not yet confirmed in the decompilation.
bool append_opcode_50_adol(
    std::vector<AdolInstruction>& instructions,
    std::uint32_t value,
    std::string& error);

/// ADOL opcode 0x6E from UnitBlock+536 process_skippable_adol_instructions_.
/// Thirty-two-bit payload; semantic name is not yet confirmed.
bool append_opcode_6e_adol(
    std::vector<AdolInstruction>& instructions,
    std::uint32_t value,
    std::string& error);

} // namespace auro3d:encode
