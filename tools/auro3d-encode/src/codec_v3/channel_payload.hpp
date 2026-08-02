#pragma once

#include "channel_bit_writer.hpp"
#include "bit_writer.hpp"
#include "channel_metadata.hpp"
#include "crc.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

struct ChannelParserInstruction {
    std::uint8_t opcode = 0;
    std::vector<std::uint32_t> payload;
};

/// Optional records counted by Rescaler::operator() @ 0x4EA080 before the
/// cluster-delta quantizer receives its remaining channel budget. Offset names
/// are retained until the corresponding record syntax is fully identified.
struct NativeRescalerAccounting {
    bool group_268_present = false;
    bool group_274_present = false;
    bool group_284_present = false;
    bool group_292_present = false;
    std::uint32_t group_240_field_32_present_count = 0;
    bool group_299_present = false;
    bool group_304_present = false;
    /// Group+312 instructions do not include the parser terminator.
    std::vector<ChannelParserInstruction> instructions;
};

/// Port of encode::rescaler::opcode_bitwidth @ 0x5011A0.
bool native_rescaler_opcode_bitwidth(
    std::uint8_t opcode,
    std::uint32_t& payload_bits);

/// Exact Group+196 accounting. The default direct path is 32 bits; an emitted
/// Group+392 scaler record adds 16 bits.
bool calculate_native_rescaler_bit_cost(
    const NativeRescalerAccounting& accounting,
    bool scaler_present,
    std::uint32_t& bit_cost,
    std::string& error);

/// Appends a storage-level LSB-first bitstream to the channel writer. The
/// channel writer still emits its own MSB-first words; this bridge copies the
/// bits one at a time, preserving the native Golomb-Rice remainder order.
bool write_lsb_bitstream(
    ChannelBitWriter& writer,
    const LsbBitWriter& source,
    std::uint64_t bit_offset,
    std::uint64_t bit_count);

bool pack_channel_context_values(
    const std::vector<std::uint32_t>& values,
    std::uint32_t bit_width,
    std::vector<std::uint32_t>& packed_words,
    std::string& error);

/// Field-level writer for the channel parser stream. It deliberately exposes
/// no group policy: the caller supplies the exact metadata words, seeds and
/// opcode payloads selected by native process_groups_.
class ChannelPayloadWriter {
public:
    bool open(std::uint32_t quant_shift);
    bool write_header(std::uint16_t header, const std::array<std::uint32_t, 3>& metadata_words);
    bool write_unsigned(std::uint32_t value, std::uint32_t bits);
    bool write_signed(std::int32_t value, std::uint32_t bits);
    bool write_extrapolate_seeds(
        std::uint32_t mode,
        const std::vector<std::int32_t>& seeds);
    /// Writes the fixed context prefix consumed by parser states 20/21. Mode
    /// zero uses one context vector; mode three uses the native doubled size.
    bool write_context_words(
        std::uint32_t mode,
        const std::vector<std::uint32_t>& packed_words);
    bool write_context_values(
        std::uint32_t mode,
        const std::vector<std::uint32_t>& values,
        std::uint32_t bit_width,
        std::string& error);
    bool write_stream_words(const std::vector<std::uint32_t>& words);
    bool write_lsb_bitstream(
        const LsbBitWriter& source,
        std::uint64_t bit_offset,
        std::uint64_t bit_count);
    bool write_golomb_rice_values(
        const std::vector<std::uint32_t>& values,
        std::uint32_t parameter);
    bool write_parser_instruction(
        std::uint8_t opcode,
        const std::vector<std::uint32_t>& payload);
    bool write_parser_sequence(const std::vector<ChannelParserInstruction>& instructions);
    bool write_parser_terminator() { return write_parser_instruction(0u, {}); }
    bool write_u8(std::uint8_t value) { return write_unsigned(value, 8u); }
    bool write_u16(std::uint16_t value) { return write_unsigned(value, 16u); }
    bool write_u24(std::uint32_t value) { return write_unsigned(value, 24u); }
    bool write_u32(std::uint32_t value) { return write_unsigned(value, 32u); }
    bool write_opcode(std::uint8_t opcode) { return write_u8(opcode); }

    std::uint32_t bitlines() const { return writer_.bitlines(); }
    std::uint32_t word_count() const { return writer_.word_count(); }
    std::uint64_t bit_count() const { return writer_.bit_count(); }
    std::uint64_t payload_capacity_bits() const { return writer_.payload_capacity_bits(); }
    std::uint64_t remaining_payload_bits() const { return writer_.remaining_payload_bits(); }
    const std::vector<std::uint32_t>& words() const { return writer_.words(); }
    std::uint16_t stored_crc_word() const;

private:
    ChannelBitWriter writer_;
    bool header_written_ = false;
    std::uint16_t channel_header_ = 0;
    std::array<std::uint32_t, 3> metadata_words_{};
};

} // namespace auro3d::encode
