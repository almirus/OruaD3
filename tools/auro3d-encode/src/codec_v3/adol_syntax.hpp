#pragma once

#include "pcm_metadata.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// One explicit ADOL instruction. The decoder accepts either one scalar
/// payload or the 0x40 two-byte form; this representation keeps both forms
/// distinguishable and deliberately supplies no semantic defaults.
struct AdolInstruction {
    std::uint8_t opcode = 0;
    std::uint32_t value = 0;
    std::uint32_t value_2 = 0;
};

/// Returns the scalar payload width accepted by adol_parse. A zero return
/// means either the terminating opcode 0, the special two-byte opcode 0x40,
/// or an unsupported opcode.
std::uint32_t adol_scalar_payload_bits(std::uint8_t opcode);

/// Validates an ADOL instruction list without consuming a metadata writer.
/// The same opcode widths and payload bounds are used by write_adol_block.
bool validate_adol_block(
    const std::vector<AdolInstruction>& instructions,
    std::string& error);

/// Serializes one ADOL tagged block exactly as adol_parse reads it: tag 1,
/// instruction records, then the zero-opcode terminator. The instruction list
/// cannot contain a terminator itself because that would make later entries
/// unreachable to the decoder.
bool write_adol_block(
    PcmMetadataFalseWriter& writer,
    const std::vector<AdolInstruction>& instructions);

} // namespace auro3d:encode
