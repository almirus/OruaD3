#include "adol_syntax.hpp"

namespace auro3d::encode {

std::uint32_t adol_scalar_payload_bits(std::uint8_t opcode) {
    switch (opcode) {
    case 1:
    case 3:
    case 0x5A:
    case 0x5B:
    case 0x5C:
    case 0x5D:
    case 0x5E:
    case 0x5F:
    case 0x60:
    case 0x61:
    case 0x62:
        return 16;

    case 2:
    case 4:
    case 0x1E:
    case 0x1F:
    case 0x41:
    case 0x47:
    case 0x50:
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
    case 0x56:
    case 0x57:
    case 0x58:
        return 8;

    case 0x64:
    case 0x65:
    case 0x66:
    case 0x67:
    case 0x68:
    case 0x69:
    case 0x6A:
    case 0x6B:
    case 0x6C:
        return 24;

    case 0x0E:
    case 0x46:
    case 0x6E:
    case 0x6F:
    case 0x70:
    case 0x71:
    case 0x72:
    case 0x73:
    case 0x74:
    case 0x75:
    case 0x76:
    case 0x80:
    case 0x81:
    case 0x82:
    case 0x83:
    case 0x84:
    case 0x85:
    case 0x8C:
    case 0x8D:
    case 0x8E:
    case 0x8F:
    case 0x90:
        return 32;

    default:
        return 0;
    }
}

bool validate_adol_block(
    const std::vector<AdolInstruction>& instructions,
    std::string& error) {
    error.clear();
    for (const AdolInstruction& instruction : instructions) {
        if (instruction.opcode == 0u) {
            error = "ADOL instruction list contains an embedded terminator";
            return false;
        }
        if (instruction.opcode == 0x40u) {
            if (instruction.value > 0xFFu || instruction.value_2 > 0xFFu) {
                error = "ADOL two-byte instruction payload exceeds 8 bits";
                return false;
            }
            continue;
        }
        const std::uint32_t bits = adol_scalar_payload_bits(instruction.opcode);
        if (bits == 0u
            || (bits != 32u && instruction.value >= (std::uint32_t{1} << bits))
            || instruction.value_2 != 0u) {
            error = "ADOL scalar instruction payload does not match its opcode width";
            return false;
        }
    }
    return true;
}

bool write_adol_block(
    PcmMetadataFalseWriter& writer,
    const std::vector<AdolInstruction>& instructions) {
    std::string validation_error;
    if (!writer.valid() || !validate_adol_block(instructions, validation_error))
        return false;
    if (!writer.write_unsigned(1u, 8u))
        return false;

    for (const AdolInstruction& instruction : instructions) {
        if (instruction.opcode == 0 || !writer.write_unsigned(instruction.opcode, 8u)) {
            return false;
        }

        if (instruction.opcode == 0x40) {
            if (instruction.value > 0xFFu || instruction.value_2 > 0xFFu ||
                !writer.write_unsigned(instruction.value, 8u) ||
                !writer.write_unsigned(instruction.value_2, 8u)) {
                return false;
            }
            continue;
        }

        const std::uint32_t bits = adol_scalar_payload_bits(instruction.opcode);
        if (bits == 0 || (bits != 32u && instruction.value >= (std::uint32_t{1} << bits)) ||
            instruction.value_2 != 0 || !writer.write_unsigned(instruction.value, bits)) {
            return false;
        }
    }

    return writer.write_unsigned(0u, 8u);
}

} // namespace auro3d:encode
