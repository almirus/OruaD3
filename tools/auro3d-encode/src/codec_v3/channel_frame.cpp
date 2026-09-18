#include "channel_frame.hpp"

#include "channel_codebook_plan.hpp"
#include "scaler.hpp"

#include <limits>

namespace auro3d::encode {

namespace {

bool write_compose_parser_block(
    ChannelPayloadWriter& writer,
    const std::vector<AdolInstruction>& common_adol,
    bool base_scaler_present,
    std::uint8_t base_scaler_index,
    const std::vector<PrimaryDownmixGain>& primary_downmix_gains,
    const std::vector<AdolInstruction>& optional_adol) {
    // Channel:initialize: limit_simple (parser 65) first, then
    // primary downmix gains (parser 64) from 360/630.
    if (!writer.write_u8(1u))
        return false;
    const auto write_adol = [&writer](const AdolInstruction& instruction) {
        if (instruction.opcode == 0u)
            return false;
        if (instruction.opcode == 64u) {
            return writer.write_parser_instruction(
                instruction.opcode,
                {instruction.value, instruction.value_2});
        }
        if (instruction.opcode == 100u) {
            return writer.write_parser_instruction(
                instruction.opcode,
                {
                    (instruction.value >> 16u) & 0xFFu,
                    (instruction.value >> 8u) & 0xFFu,
                    instruction.value & 0xFFu,
                });
        }
        return writer.write_parser_instruction(
            instruction.opcode, {instruction.value});
    };
    for (const AdolInstruction& instruction : common_adol) {
        if (!write_adol(instruction))
            return false;
    }
    if (base_scaler_present
        && !writer.write_parser_instruction(65u, {base_scaler_index})) {
        return false;
    }
    for (const PrimaryDownmixGain& gain : primary_downmix_gains) {
        if (!writer.write_parser_instruction(
                64u, {gain.channel_id, gain.scaler_index})) {
            return false;
        }
    }
    for (const AdolInstruction& instruction : optional_adol) {
        if (!write_adol(instruction))
            return false;
    }
    return writer.write_parser_terminator();
}

template <typename AdolWriter, typename ResidualWriter>
bool encode_channel_frame_impl(
    std::uint32_t channel_id,
    std::uint32_t quantization_shift,
    std::uint16_t channel_header,
    std::uint32_t bit_width,
    std::uint32_t selector,
    const std::array<std::uint32_t, 3>& channel_ids,
    std::uint32_t third_word,
    std::uint32_t metadata_word0_extra,
    std::uint32_t mode,
    const std::vector<std::int32_t>& seeds,
    AdolWriter&& write_adol,
    ResidualWriter&& write_residual,
    EncodedChannelFrame& out,
    std::string& error) {
    error.clear();
    out = {};
    if (channel_id >= kCodecV3ChannelCount || mode > 3u) {
        error = "channel frame has an invalid channel id or mode";
        return false;
    }
    if (channel_header > 0x1FFu
        || (channel_header >= 0x100u && static_cast<std::uint8_t>(channel_header) > 0x0Au)) {
        error = "channel frame header is outside the native 9-bit range";
        return false;
    }
    std::array<std::uint32_t, 3> metadata_words{};
    if (!make_channel_metadata_words(
            bit_width, selector, channel_ids, third_word, metadata_words)) {
        error = "channel frame metadata words are invalid";
        return false;
    }
    if ((metadata_word0_extra & 0xF0FFF0FFu) != 0u) {
        error = "channel frame metadata flags exceed the native static range";
        return false;
    }
    metadata_words[0] |= metadata_word0_extra;
    ChannelMetadataCombined combined{};
    if (!combine_channel_metadata(channel_header, metadata_words, combined)
        || combined.mode != mode) {
        error = "channel frame metadata mode does not match its channel ids";
        return false;
    }
    ChannelPayloadWriter writer;
    if (!writer.open(quantization_shift)
        || !writer.write_header(channel_header, metadata_words)) {
        error = "channel frame payload prefix does not fit its native bit-line stream";
        return false;
    }
    // a3d:serialize<CountIterator> serializes the fixed A3D
    // header, the mode-2/mode-3 extrapolate seeds, the ADOL vector and its
    // opcode-zero terminator, then context and residual data.
    if (!writer.write_extrapolate_seeds(mode, seeds)) {
        error = "channel frame extrapolate seeds do not fit its native bit-line stream";
        return false;
    }
    if (!write_adol(writer)) {
        error = "channel frame ADOL stream does not fit its native bit-line stream";
        return false;
    }
    if (!write_residual(writer)) {
        error = "channel frame residual stream does not fit its native bit-line stream";
        return false;
    }
    if (writer.bit_count() > writer.payload_capacity_bits()) {
        error = "channel frame payload exceeds its native bit-line capacity";
        return false;
    }
    if (writer.word_count() > std::numeric_limits<std::uint32_t>::max()) {
        error = "channel frame payload is too large";
        return false;
    }
    out.channel_id = channel_id;
    out.quantization_shift = quantization_shift;
    out.channel_header = channel_header;
    out.metadata_words = metadata_words;
    out.word_count = writer.word_count();
    out.serialized_bits = writer.bit_count();
    out.crc_word = writer.stored_crc_word();
    out.words.reserve(writer.words().size());
    for (const std::uint32_t word : writer.words()) {
        const std::uint32_t pcm24 = word & 0xFFFFFFu;
        out.words.push_back(static_cast<std::int32_t>(
            (pcm24 & 0x800000u) != 0u ? pcm24 | 0xFF000000u : pcm24));
    }
    return true;
}

} // namespace

bool collect_primary_downmix_gains(
    const std::vector<std::uint32_t>& source_ids,
    const std::array<std::uint8_t, 31>* input_scaler_indices,
    std::vector<PrimaryDownmixGain>& out,
    std::string& error) {
    error.clear();
    out.clear();
    if (input_scaler_indices == nullptr)
        return true;
    out.reserve(source_ids.size());
    for (const std::uint32_t source_id : source_ids) {
        if (source_id >= kCodecV3ChannelCount
            || source_id >= input_scaler_indices->size()) {
            error = "primary downmix source id is outside codec-v3 range";
            out.clear();
            return false;
        }
        const std::uint8_t index = (*input_scaler_indices)[source_id];
        // prepare_metadata_unit_block_ stores the tree byte only when non-zero.
        if (index == 0u)
            continue;
        if (validate_scaler_index(index) != index) {
            error = "primary downmix scaler index fails native validate_ix";
            out.clear();
            return false;
        }
        out.push_back({source_id, index});
    }
    return true;
}

bool encode_channel_frame(
    std::uint32_t channel_id,
    std::uint32_t quantization_shift,
    std::uint16_t channel_header,
    std::uint32_t bit_width,
    std::uint32_t selector,
    const std::array<std::uint32_t, 3>& channel_ids,
    std::uint32_t third_word,
    std::uint32_t mode,
    const std::vector<std::int32_t>& seeds,
    const std::vector<std::uint32_t>& context_words,
    const std::vector<std::uint32_t>& stream_words,
    EncodedChannelFrame& out,
    std::string& error) {
    return encode_channel_frame_impl(
        channel_id,
        quantization_shift,
        channel_header,
        bit_width,
        selector,
        channel_ids,
        third_word,
        0u,
        mode,
        seeds,
        [](ChannelPayloadWriter&) { return true; },
        [&context_words, &stream_words, mode](ChannelPayloadWriter& writer) {
            return writer.write_context_words(mode, context_words)
                && writer.write_stream_words(stream_words);
        },
        out,
        error);
}

bool encode_channel_frame_sequence(
    std::uint32_t channel_id,
    std::uint32_t quantization_shift,
    std::uint16_t channel_header,
    std::uint32_t bit_width,
    std::uint32_t selector,
    const std::array<std::uint32_t, 3>& channel_ids,
    std::uint32_t third_word,
    std::uint32_t mode,
    const std::vector<std::int32_t>& seeds,
    const std::vector<std::uint32_t>& context_words,
    const std::vector<ChannelParserInstruction>& instructions,
    EncodedChannelFrame& out,
    std::string& error) {
    return encode_channel_frame_impl(
        channel_id,
        quantization_shift,
        channel_header,
        bit_width,
        selector,
        channel_ids,
        third_word,
        1u << 8u,
        mode,
        seeds,
        [&instructions](ChannelPayloadWriter& writer) {
            return writer.write_u8(1u)
                && writer.write_parser_sequence(instructions);
        },
        [&context_words, mode](ChannelPayloadWriter& writer) {
            return writer.write_context_words(mode, context_words);
        },
        out,
        error);
}

bool encode_direct_channel_frame(
    std::uint32_t channel_id,
    std::uint32_t quantization_shift,
    std::uint16_t channel_header,
    std::uint32_t source_channel_id,
    bool base_scaler_present,
    std::uint8_t base_scaler_index,
    const std::vector<PrimaryDownmixGain>& primary_downmix_gains,
    const std::vector<AdolInstruction>& common_adol,
    const std::vector<AdolInstruction>& optional_adol,
    EncodedChannelFrame& out,
    std::string& error) {
    error.clear();
    if (source_channel_id >= kCodecV3ChannelCount) {
        error = "direct channel source id is outside codec-v3 range";
        return false;
    }
    if ((base_scaler_present && base_scaler_index == 0u)
        || (!base_scaler_present && base_scaler_index != 0u)) {
        error = "direct channel base-scaler presence is inconsistent";
        return false;
    }
    for (const PrimaryDownmixGain& gain : primary_downmix_gains) {
        if (gain.channel_id >= kCodecV3ChannelCount
            || gain.scaler_index == 0u
            || validate_scaler_index(gain.scaler_index) != gain.scaler_index) {
            error = "direct channel primary downmix gain is invalid";
            return false;
        }
    }
    const std::array<std::uint32_t, 3> channel_ids = {
        source_channel_id, 255u, 255u};
    // Native Mixer case 1 carries no residual codebook. A zero-width
    // context is valid in ChannelParser state 20 and leaves no payload bits
    // before the CRC-covered carrier words. Rescaler can nevertheless select
    // Group+392 for a hot direct source, in which case opcode 0x41 restores
    // its scale in the decoder just as it does for mixed groups. Per-source
    // original-map gains become parser opcode 64 after that optional 65.
    const bool parser_present =
        !common_adol.empty()
        || base_scaler_present
        || !primary_downmix_gains.empty()
        || !optional_adol.empty();
    const bool encoded = encode_channel_frame_impl(
        channel_id,
        quantization_shift,
        channel_header,
        0u,
        0u,
        channel_ids,
        0u,
        parser_present ? (1u << 8u) : 0u,
        1u,
        {},
        [base_scaler_present,
         base_scaler_index,
         &common_adol,
         &primary_downmix_gains,
         &optional_adol,
         parser_present](ChannelPayloadWriter& writer) {
            return !parser_present
                || write_compose_parser_block(
                    writer,
                    common_adol,
                    base_scaler_present,
                    base_scaler_index,
                    primary_downmix_gains,
                    optional_adol);
        },
        [](ChannelPayloadWriter&) { return true; },
        out,
        error);
    if (!encoded)
        return false;
    out.base_scaler_present = base_scaler_present;
    out.base_scaler_index = base_scaler_index;
    return true;
}

bool encode_codebook_channel_frame_sequence(
    std::uint32_t channel_id,
    std::uint32_t quantization_shift,
    std::uint16_t channel_header,
    std::uint32_t mode,
    std::uint32_t bit_width,
    const std::vector<std::int32_t>& values0,
    const std::vector<std::int32_t>& values1,
    const std::array<std::uint32_t, 3>& channel_ids,
    std::uint32_t third_word,
    const std::vector<std::int32_t>& seeds,
    const std::vector<ChannelParserInstruction>& instructions,
    EncodedChannelFrame& out,
    std::string& error) {
    error.clear();
    std::uint32_t selector = 0u;
    std::array<std::uint32_t, 3> metadata_words{};
    std::vector<std::uint32_t> context_words;
    if (!make_channel_codebook_prefix(
            mode,
            bit_width,
            values0,
            values1,
            channel_ids,
            third_word,
            0u,
            selector,
            metadata_words,
            context_words,
            error)) {
        return false;
    }
    std::array<std::uint32_t, 3> expected_metadata_words{};
    if (!make_channel_metadata_words(
            bit_width,
            selector,
            channel_ids,
            third_word,
            expected_metadata_words)
        || expected_metadata_words != metadata_words) {
        error = "channel codebook metadata prefix is internally inconsistent";
        return false;
    }
    return encode_channel_frame_impl(
        channel_id,
        quantization_shift,
        channel_header,
        bit_width,
        selector,
        channel_ids,
        third_word,
        1u << 8u,
        mode,
        seeds,
        [&instructions](ChannelPayloadWriter& writer) {
            return writer.write_u8(1u)
                && writer.write_parser_sequence(instructions);
        },
        [&context_words, mode](ChannelPayloadWriter& writer) {
            return writer.write_context_words(mode, context_words);
        },
        out,
        error);
}

bool encode_codebook_channel_frame(
    std::uint32_t channel_id,
    std::uint32_t quantization_shift,
    std::uint16_t channel_header,
    std::uint32_t mode,
    std::uint32_t bit_width,
    const std::vector<std::int32_t>& values0,
    const std::vector<std::int32_t>& values1,
    const std::array<std::uint32_t, 3>& channel_ids,
    std::uint32_t third_word,
    const std::vector<std::int32_t>& seeds,
    const std::vector<std::uint32_t>& stream_words,
    EncodedChannelFrame& out,
    std::string& error) {
    error.clear();
    std::uint32_t selector = 0u;
    std::array<std::uint32_t, 3> metadata_words{};
    std::vector<std::uint32_t> context_words;
    if (!make_channel_codebook_prefix(
            mode,
            bit_width,
            values0,
            values1,
            channel_ids,
            third_word,
            0u,
            selector,
            metadata_words,
            context_words,
            error)) {
        return false;
    }
    std::array<std::uint32_t, 3> expected_metadata_words{};
    if (!make_channel_metadata_words(
            bit_width,
            selector,
            channel_ids,
            third_word,
            expected_metadata_words)
        || expected_metadata_words != metadata_words) {
        error = "channel codebook metadata prefix is internally inconsistent";
        return false;
    }
    return encode_channel_frame(
        channel_id,
        quantization_shift,
        channel_header,
        bit_width,
        selector,
        channel_ids,
        third_word,
        mode,
        seeds,
        context_words,
        stream_words,
        out,
        error);
}

bool encode_codebook_channel_frame_golomb(
    std::uint32_t channel_id,
    std::uint32_t quantization_shift,
    std::uint16_t channel_header,
    std::uint32_t mode,
    std::uint32_t bit_width,
    const std::vector<std::int32_t>& values0,
    const std::vector<std::int32_t>& values1,
    const std::array<std::uint32_t, 3>& channel_ids,
    std::uint32_t third_word,
    std::uint32_t golomb_parameter,
    bool base_scaler_present,
    std::uint8_t base_scaler_index,
    const std::vector<PrimaryDownmixGain>& primary_downmix_gains,
    const std::vector<AdolInstruction>& common_adol,
    const std::vector<AdolInstruction>& optional_adol,
    const std::vector<std::int32_t>& seeds,
    const std::vector<std::uint32_t>& indices,
    EncodedChannelFrame& out,
    std::string& error) {
    error.clear();
    if (indices.empty()) {
        error = "Golomb-Rice channel frame requires at least one index";
        return false;
    }
    for (const std::uint32_t index : indices) {
        if (index >= values0.size()) {
            error = "Golomb-Rice channel-frame index exceeds its codebook";
            return false;
        }
    }
    for (const PrimaryDownmixGain& gain : primary_downmix_gains) {
        if (gain.channel_id >= kCodecV3ChannelCount
            || gain.scaler_index == 0u
            || validate_scaler_index(gain.scaler_index) != gain.scaler_index) {
            error = "Golomb-Rice channel primary downmix gain is invalid";
            return false;
        }
    }
    std::uint32_t selector = 0u;
    std::array<std::uint32_t, 3> metadata_words{};
    std::vector<std::uint32_t> context_words;
    if (!make_channel_codebook_prefix(
            mode,
            bit_width,
            values0,
            values1,
            channel_ids,
            third_word,
            golomb_parameter,
            selector,
            metadata_words,
            context_words,
            error)) {
        return false;
    }
    std::array<std::uint32_t, 3> expected_metadata_words{};
    if (!make_channel_metadata_words(
            bit_width,
            selector,
            channel_ids,
            third_word,
            expected_metadata_words)) {
        error = "channel codebook metadata prefix is internally inconsistent";
        return false;
    }
    const bool parser_present =
        !common_adol.empty()
        || base_scaler_present
        || !primary_downmix_gains.empty()
        || !optional_adol.empty();
    const std::uint32_t parser_block_count = parser_present ? 1u : 0u;
    if ((base_scaler_present && base_scaler_index == 0u)
        || (!base_scaler_present && base_scaler_index != 0u)) {
        error = "channel frame base-scaler presence is inconsistent";
        return false;
    }
    metadata_words[0] |= parser_block_count << 8u;
    expected_metadata_words[0] |=
        (golomb_parameter << 24u)
        | (parser_block_count << 8u);
    if (expected_metadata_words != metadata_words) {
        error = "channel codebook metadata prefix is internally inconsistent";
        return false;
    }
    const bool encoded = encode_channel_frame_impl(
        channel_id,
        quantization_shift,
        channel_header,
        bit_width,
        selector,
        channel_ids,
        third_word,
        (golomb_parameter << 24u)
            | (parser_block_count << 8u),
        mode,
        seeds,
        [&primary_downmix_gains,
         &common_adol,
         &optional_adol,
         base_scaler_present,
         base_scaler_index,
         parser_present](ChannelPayloadWriter& writer) {
            return !parser_present
                || write_compose_parser_block(
                    writer,
                    common_adol,
                    base_scaler_present,
                    base_scaler_index,
                    primary_downmix_gains,
                    optional_adol);
        },
        [&context_words,
         &indices,
         golomb_parameter,
         mode](ChannelPayloadWriter& writer) {
            return writer.write_context_words(mode, context_words)
                && writer.write_golomb_rice_values(indices, golomb_parameter);
        },
        out,
        error);
    if (!encoded)
        return false;
    out.base_scaler_present = base_scaler_present;
    out.base_scaler_index = base_scaler_index;
    return true;
}

bool finalize_channel_frame_span(
    std::uint32_t frame_count,
    EncodedChannelFrame& frame,
    std::string& error) {
    error.clear();
    if (frame_count == 0u || frame.channel_id >= kCodecV3ChannelCount
        || frame.word_count != frame.words.size()
        || frame.word_count > frame_count) {
        error = "channel frame span is shorter than its serialized payload"
            " or has an invalid channel (frames="
            + std::to_string(frame_count)
            + ", words=" + std::to_string(frame.word_count)
            + ", stored_words=" + std::to_string(frame.words.size())
            + ", bits=" + std::to_string(frame.serialized_bits)
            + ", bit_line=" + std::to_string(frame.quantization_shift)
            + ", channel=" + std::to_string(frame.channel_id) + ")";
        return false;
    }
    frame.words.resize(frame_count, 0);
    frame.word_count = frame_count;
    Crc16 crc;
    for (const std::int32_t word : frame.words) {
        const std::uint32_t raw = static_cast<std::uint32_t>(word);
        if ((raw >> 24u) != 0u && (raw >> 24u) != 0xFFu) {
            error = "channel frame span contains a non-PCM24 word";
            return false;
        }
        crc.process_words(&raw, 1u);
    }
    frame.crc_word = crc.stored_word();
    return true;
}

bool merge_channel_frame_payload(
    std::vector<std::int32_t>& carrier_samples,
    const EncodedChannelFrame& frame,
    std::string& error) {
    error.clear();
    if (frame.word_count != frame.words.size()
        || frame.words.size() > carrier_samples.size()) {
        error = "channel payload exceeds its carrier PCM span";
        return false;
    }
    for (std::size_t index = 0; index < frame.words.size(); ++index) {
        const std::uint32_t payload = static_cast<std::uint32_t>(frame.words[index]);
        if ((payload >> 24u) != 0u && (payload >> 24u) != 0xFFu) {
            error = "channel payload contains a non-PCM24 word";
            return false;
        }
        const std::uint32_t base = static_cast<std::uint32_t>(carrier_samples[index]);
        if ((base >> 24u) != 0u && (base >> 24u) != 0xFFu) {
            error = "carrier base contains a non-PCM24 word";
            return false;
        }
        const std::uint32_t merged = (base & 0xFF000000u) | (base & 0x00FFFFFFu)
            | (payload & 0x00FFFFFFu);
        carrier_samples[index] = static_cast<std::int32_t>(merged);
    }
    return true;
}

bool prepare_channel_mux_words(
    const std::vector<std::int32_t>& source_samples,
    std::uint32_t quantization_shift,
    bool shift_left,
    std::vector<std::int32_t>& mux_words,
    std::string& error) {
    error.clear();
    mux_words.clear();
    // quantization_shift is the group bit_line. Channel:mux shifts/masks by
    // 24 - Group+4 with Group+4 = headroom = 24 - bit_line, so the effective
    // shift equals bit_line.
    if (quantization_shift < 3u || quantization_shift > 16u) {
        error = "channel mux bit_line is outside the native Projector range";
        return false;
    }
    const std::uint32_t shift = quantization_shift;
    if (shift >= 32u) {
        error = "channel mux shift is outside the native word range";
        return false;
    }
    mux_words.resize(source_samples.size());
    const std::uint32_t mask = shift == 0u ? 0xFFFFFFFFu : 0xFFFFFFFFu << shift;
    for (std::size_t index = 0; index < source_samples.size(); ++index) {
        if (source_samples[index] < -0x800000 || source_samples[index] > 0x7FFFFF) {
            error = "channel mux source sample exceeds signed PCM24 range";
            mux_words.clear();
            return false;
        }
        const std::uint32_t raw = static_cast<std::uint32_t>(source_samples[index]);
        const std::uint32_t value = shift_left
            ? raw << shift
            : raw & mask;
        mux_words[index] = static_cast<std::int32_t>(value);
    }
    return true;
}

bool mux_channel_words(
    const std::vector<std::int32_t>& source_samples,
    std::uint32_t quantization_shift,
    bool shift_left,
    const std::vector<PcmMuxProjectorRecord>& records,
    bool append_terminator,
    std::vector<std::int32_t>& mux_words,
    std::uint16_t& crc,
    std::string& error) {
    error.clear();
    crc = 0u;
    mux_words.clear();
    if (quantization_shift < 3u || quantization_shift > 16u) {
        error = "muxed channel bitline count is outside the native range";
        return false;
    }
    std::vector<std::int32_t> prepared;
    if (!prepare_channel_mux_words(
            source_samples, quantization_shift, shift_left, prepared, error)) {
        return false;
    }
    std::vector<std::uint32_t> words;
    words.reserve(prepared.size());
    for (const std::int32_t value : prepared)
        words.push_back(static_cast<std::uint32_t>(value));
    if (!apply_pcm_mux_records(
            quantization_shift,
            words,
            records,
            append_terminator,
            crc,
            error)) {
        return false;
    }
    mux_words.reserve(words.size());
    for (const std::uint32_t value : words) {
        const std::uint32_t high = value >> 24u;
        if (high != 0u && high != 0xFFu) {
            error = "muxed channel word exceeds signed PCM24 range";
            mux_words.clear();
            return false;
        }
        mux_words.push_back(static_cast<std::int32_t>(value));
    }
    return true;
}

} // namespace auro3d:encode
