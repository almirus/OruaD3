#include "channel_payload.hpp"

#include "golomb_rice.hpp"

#include <limits>

namespace auro3d::encode {

bool native_rescaler_opcode_bitwidth(
    std::uint8_t opcode,
    std::uint32_t& payload_bits) {
    switch (opcode) {
    case 1u:
    case 3u:
    case 90u:
    case 91u:
    case 92u:
    case 93u:
    case 94u:
    case 95u:
    case 96u:
    case 97u:
    case 98u:
        payload_bits = 16u;
        return true;
    case 2u:
    case 4u:
    case 31u:
    case 81u:
    case 82u:
    case 83u:
    case 84u:
    case 85u:
    case 86u:
    case 87u:
    case 88u:
        payload_bits = 8u;
        return true;
    case 14u:
    case 111u:
    case 112u:
    case 113u:
    case 114u:
    case 115u:
    case 116u:
    case 117u:
    case 118u:
    case 140u:
    case 141u:
    case 142u:
    case 143u:
    case 144u:
        payload_bits = 32u;
        return true;
    case 101u:
    case 102u:
    case 103u:
    case 104u:
    case 105u:
    case 106u:
    case 107u:
    case 108u:
        payload_bits = 24u;
        return true;
    default:
        payload_bits = 0u;
        return false;
    }
}

bool calculate_native_rescaler_bit_cost(
    const NativeRescalerAccounting& accounting,
    bool scaler_present,
    std::uint32_t& bit_cost,
    std::string& error) {
    error.clear();
    std::uint64_t cost =
        accounting.group_268_present ? 72u : 32u;
    if (accounting.group_274_present)
        cost |= 16u;
    if (accounting.group_284_present)
        cost += 40u;
    if (accounting.group_292_present)
        cost += 16u;
    cost += static_cast<std::uint64_t>(
        accounting.group_240_field_32_present_count) * 24u;
    if (accounting.group_299_present)
        cost += 32u;
    if (accounting.group_304_present)
        cost += 40u;

    for (const ChannelParserInstruction& instruction :
         accounting.instructions) {
        std::uint32_t payload_bits = 0u;
        if (!native_rescaler_opcode_bitwidth(
                instruction.opcode, payload_bits)) {
            error = "rescaler instruction opcode has no native bit width";
            return false;
        }
        if (instruction.payload.size() != 1u) {
            error = "rescaler instruction must contain exactly one payload value";
            return false;
        }
        if (payload_bits < 32u
            && instruction.payload[0] >= (1u << payload_bits)) {
            error = "rescaler instruction payload exceeds its native bit width";
            return false;
        }
        cost += 8u + payload_bits;
    }
    if (scaler_present)
        cost += 16u;
    if (cost > std::numeric_limits<std::uint32_t>::max()) {
        error = "rescaler metadata bit cost exceeds 32-bit storage";
        return false;
    }
    bit_cost = static_cast<std::uint32_t>(cost);
    return true;
}

bool write_lsb_bitstream(
    ChannelBitWriter& writer,
    const LsbBitWriter& source,
    std::uint64_t bit_offset,
    std::uint64_t bit_count) {
    if (bit_offset > source.bit_count()
        || bit_count > source.bit_count() - bit_offset) {
        return false;
    }
    const auto& bytes = source.bytes();
    for (std::uint64_t index = 0; index < bit_count; ++index) {
        const std::uint64_t position = bit_offset + index;
        const std::uint32_t bit =
            (bytes[static_cast<std::size_t>(position >> 3u)]
                >> static_cast<std::uint32_t>(position & 7u)) & 1u;
        if (!writer.write_unsigned(bit, 1u))
            return false;
    }
    return true;
}

bool pack_channel_context_values(
    const std::vector<std::uint32_t>& values,
    std::uint32_t bit_width,
    std::vector<std::uint32_t>& packed_words,
    std::string& error) {
    error.clear();
    packed_words.clear();
    if (bit_width > 32u) {
        error = "channel context bit width is outside the native range";
        return false;
    }
    if (bit_width == 0u) {
        if (!values.empty())
            error = "zero-width channel context cannot contain values";
        return values.empty();
    }
    const std::uint64_t value_count = values.size();
    if (value_count > std::numeric_limits<std::uint64_t>::max() / bit_width) {
        error = "channel context size overflows its bit count";
        return false;
    }
    const std::uint64_t total_bits = value_count * bit_width;
    if (total_bits > std::numeric_limits<std::uint64_t>::max() - 31u) {
        error = "channel context word count overflows";
        return false;
    }
    const std::uint64_t word_count = (total_bits + 31u) / 32u;
    if (word_count > packed_words.max_size()) {
        error = "channel context word count exceeds storage capacity";
        return false;
    }
    packed_words.assign(static_cast<std::size_t>(word_count), 0u);
    std::uint64_t cursor = 0u;
    const std::uint32_t value_mask = bit_width == 32u
        ? 0xFFFFFFFFu : (std::uint32_t{1} << bit_width) - 1u;
    for (std::size_t value_index = 0; value_index < values.size(); ++value_index) {
        const std::uint32_t value = values[value_index];
        if (bit_width < 32u && value > value_mask) {
            error = "channel context value at index "
                + std::to_string(value_index)
                + " exceeds its metadata bit width";
            packed_words.clear();
            return false;
        }
        const std::uint32_t raw = value & value_mask;
        for (std::uint32_t bit = 0; bit < bit_width; ++bit) {
            const std::uint32_t source_bit = bit_width - 1u - bit;
            if ((raw & (std::uint32_t{1} << source_bit)) != 0u) {
                const std::size_t word = static_cast<std::size_t>(cursor >> 5u);
                packed_words[word] |= std::uint32_t{1} << (31u - (cursor & 31u));
            }
            ++cursor;
        }
    }
    return cursor == total_bits;
}

bool codec_v3_channel_bitlines_from_quant_shift(
    std::uint32_t quant_shift,
    std::uint32_t& bitlines) {
    // `quant_shift` here is the group bit_line retained on EncodedGroupPcm /
    // EncodedChannelFrame. Decoder BitReader width is `24 - headroom` with
    // headroom `24 - bit_line`, i.e. bitlines == bit_line.
    bitlines = quant_shift;
    return bitlines >= 3u && bitlines <= 16u;
}

bool ChannelPayloadWriter::open(std::uint32_t quant_shift) {
    std::uint32_t bitlines = 0;
    header_written_ = false;
    channel_header_ = 0;
    metadata_words_ = {};
    return codec_v3_channel_bitlines_from_quant_shift(quant_shift, bitlines)
        && writer_.reset(bitlines);
}

bool ChannelPayloadWriter::write_header(
    std::uint16_t header,
    const std::array<std::uint32_t, 3>& metadata_words) {
    if (header_written_ || !writer_.write_unsigned(header, 16u))
        return false;
    for (const std::uint32_t word : metadata_words) {
        if (!writer_.write_unsigned(word, 32u))
            return false;
    }
    channel_header_ = header;
    metadata_words_ = metadata_words;
    header_written_ = true;
    return true;
}

bool ChannelPayloadWriter::write_unsigned(std::uint32_t value, std::uint32_t bits) {
    return header_written_ && writer_.write_unsigned(value, bits);
}

bool ChannelPayloadWriter::write_signed(std::int32_t value, std::uint32_t bits) {
    return header_written_ && writer_.write_signed(value, bits);
}

bool ChannelPayloadWriter::write_extrapolate_seeds(
    std::uint32_t mode,
    const std::vector<std::int32_t>& seeds) {
    if (!header_written_ || mode > 3u)
        return false;
    ChannelMetadataCombined combined{};
    if (!combine_channel_metadata(channel_header_, metadata_words_, combined) ||
        combined.mode != mode) {
        return false;
    }
    const std::size_t expected = mode == 3u ? 5u : mode == 2u ? 2u : 0u;
    if (seeds.size() != expected)
        return false;
    // a3d::serialize<Projector::Iterator> @ 0x51D010 writes the two-value
    // mode-2 vector in decoder-frame order: element 1 first, then element 0.
    // The mix2 seed vector at Group+616 remains in mixer order; only its wire
    // representation is reversed. Mode 3 has its own five-value order.
    if (mode == 2u) {
        return writer_.write_signed(seeds[1], 32u)
            && writer_.write_signed(seeds[0], 32u);
    }
    for (const std::int32_t seed : seeds) {
        if (!writer_.write_signed(seed, 32u))
            return false;
    }
    return true;
}

bool ChannelPayloadWriter::write_context_words(
    std::uint32_t mode,
    const std::vector<std::uint32_t>& packed_words) {
    if (!header_written_ || mode > 3u)
        return false;
    ChannelMetadataCombined combined{};
    if (!combine_channel_metadata(channel_header_, metadata_words_, combined) ||
        combined.mode != mode) {
        return false;
    }
    const std::uint64_t dimensions = mode == 3u ? 2u : 1u;
    const std::uint64_t base = static_cast<std::uint64_t>(combined.base_index);
    if (base != 0u
        && combined.bit_width > std::numeric_limits<std::uint64_t>::max() / base) {
        return false;
    }
    const std::uint64_t base_bits = base * combined.bit_width;
    if (base_bits != 0u
        && dimensions > std::numeric_limits<std::uint64_t>::max() / base_bits) {
        return false;
    }
    const std::uint64_t total_bits = base_bits * dimensions;
    if (total_bits > std::numeric_limits<std::uint64_t>::max() - 31u)
        return false;
    const std::uint64_t expected_words = (total_bits + 31u) / 32u;
    if (packed_words.size() != expected_words)
        return false;
    std::uint64_t remaining = total_bits;
    for (std::size_t index = 0; index < packed_words.size(); ++index) {
        if (remaining > 32u) {
            if (!writer_.write_unsigned(packed_words[index], 32u))
                return false;
            remaining -= 32u;
            continue;
        }
        const std::uint32_t final_bits = static_cast<std::uint32_t>(remaining);
        const std::uint32_t value = final_bits == 32u
            ? packed_words[index]
            : packed_words[index] >> (32u - final_bits);
        if (!writer_.write_unsigned(value, final_bits))
            return false;
        remaining = 0u;
    }
    return remaining == 0u;
}

bool ChannelPayloadWriter::write_context_values(
    std::uint32_t mode,
    const std::vector<std::uint32_t>& values,
    std::uint32_t bit_width,
    std::string& error) {
    error.clear();
    std::vector<std::uint32_t> packed_words;
    if (!pack_channel_context_values(values, bit_width, packed_words, error))
        return false;
    if (!write_context_words(mode, packed_words)) {
        error = "packed channel context does not match metadata geometry";
        return false;
    }
    return true;
}

bool ChannelPayloadWriter::write_stream_words(
    const std::vector<std::uint32_t>& words) {
    if (!header_written_)
        return false;
    for (const std::uint32_t word : words) {
        if (!writer_.write_unsigned(word, 32u))
            return false;
    }
    return true;
}

bool ChannelPayloadWriter::write_lsb_bitstream(
    const LsbBitWriter& source,
    std::uint64_t bit_offset,
    std::uint64_t bit_count) {
    if (!header_written_)
        return false;
    return auro3d::encode::write_lsb_bitstream(
        writer_, source, bit_offset, bit_count);
}

bool ChannelPayloadWriter::write_golomb_rice_values(
    const std::vector<std::uint32_t>& values,
    std::uint32_t parameter) {
    // Channel metadata carries the static parameter in flags bits 24..27;
    // unlike the storage-level writer, this parser stream cannot represent
    // parameters above fifteen.
    if (!header_written_ || parameter > 15u)
        return false;
    LsbBitWriter source;
    if (!write_golomb_rice_block(source, values, parameter))
        return false;
    return write_lsb_bitstream(source, 0u, source.bit_count());
}

bool ChannelPayloadWriter::write_parser_instruction(
    std::uint8_t opcode,
    const std::vector<std::uint32_t>& payload) {
    if (!header_written_ || !writer_.write_unsigned(opcode, 8u))
        return false;
    auto write_one = [this, &payload](std::uint32_t bits) {
        return payload.size() == 1u && writer_.write_unsigned(payload[0], bits);
    };
    auto write_two = [this, &payload](std::uint32_t first_bits, std::uint32_t second_bits) {
        return payload.size() == 2u
            && writer_.write_unsigned(payload[0], first_bits)
            && writer_.write_unsigned(payload[1], second_bits);
    };
    auto write_three = [this, &payload](
                           std::uint32_t first_bits,
                           std::uint32_t second_bits,
                           std::uint32_t third_bits) {
        return payload.size() == 3u
            && writer_.write_unsigned(payload[0], first_bits)
            && writer_.write_unsigned(payload[1], second_bits)
            && writer_.write_unsigned(payload[2], third_bits);
    };
    if (opcode == 0u)
        return payload.empty();
    if (opcode == 64u)
        return write_two(8u, 8u);
    if (opcode == 100u)
        return write_three(8u, 8u, 8u);
    if (opcode == 65u)
        return write_one(8u);
    if (opcode == 70u)
        return write_one(32u);
    if (opcode == 30u)
        return write_one(8u);
    if ((opcode >= 100u && opcode <= 108u))
        return write_one(24u);
    if (opcode == 1u || opcode == 3u || (opcode >= 90u && opcode <= 98u))
        return write_one(16u);
    if (opcode == 2u || opcode == 4u || opcode == 31u || opcode == 71u
        || (opcode >= 80u && opcode <= 88u)) {
        return write_one(8u);
    }
    if (opcode == 14u || (opcode >= 110u && opcode <= 118u)
        || (opcode >= 128u && opcode <= 133u) || (opcode >= 140u && opcode <= 145u)) {
        return write_one(32u);
    }
    // Keep the instruction table closed: unknown opcodes are not padded or
    // treated as raw words because the decoder consumes their width from the
    // opcode itself.
    return false;
}

bool ChannelPayloadWriter::write_parser_sequence(
    const std::vector<ChannelParserInstruction>& instructions) {
    if (instructions.empty() || instructions.back().opcode != 0u
        || !instructions.back().payload.empty()) {
        return false;
    }
    for (std::size_t index = 0; index < instructions.size(); ++index) {
        const ChannelParserInstruction& instruction = instructions[index];
        if (instruction.opcode == 0u && index + 1u != instructions.size())
            return false;
        if (!write_parser_instruction(instruction.opcode, instruction.payload))
            return false;
    }
    return true;
}

std::uint16_t ChannelPayloadWriter::stored_crc_word() const {
    Crc16 crc;
    const auto& words = writer_.words();
    crc.process_words(words.data(), words.size());
    return crc.stored_word();
}

} // namespace auro3d::encode
