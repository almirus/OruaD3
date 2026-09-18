#include "pcm_mux.hpp"

#include "crc.hpp"

#include <limits>

namespace auro3d::encode {

namespace {

bool checked_output_count(
    std::uint32_t source_count,
    std::uint32_t bitline_count,
    std::size_t& output_count) {
    const std::uint64_t count = source_count;
    const std::uint64_t lines = bitline_count;
    const std::uint64_t skipped = count == 0u ? 0u : (count - 1u) / 16u;
    if (count != 0u && lines > std::numeric_limits<std::uint64_t>::max() / count)
        return false;
    const std::uint64_t raw_total = lines * count;
    if (skipped > raw_total)
        return false;
    const std::uint64_t total = raw_total - skipped;
    if (total > std::numeric_limits<std::size_t>::max())
        return false;
    output_count = static_cast<std::size_t>(total);
    return true;
}

} // namespace

bool make_pcm_mux_mask_plan(
    std::uint32_t source_count,
    std::uint32_t bitline_count,
    std::vector<std::uint32_t>& masks,
    std::string& error) {
    error.clear();
    masks.clear();
    if (bitline_count < 3u || bitline_count > 16u) {
        error = "codec-v3 PCM mux bitline count must be in the native range 3..16";
        return false;
    }
    if (source_count < 16u) {
        error = "codec-v3 PCM mux requires at least sixteen source samples";
        return false;
    }
    std::size_t output_count = 0;
    if (!checked_output_count(source_count, bitline_count, output_count)) {
        error = "codec-v3 PCM mux output size overflows size_t";
        return false;
    }
    masks.reserve(output_count);
    const std::size_t first_region = static_cast<std::size_t>(bitline_count) * 16u;
    // fill_masks_ emits the first three bitlines as four sixteen-word runs.
    // Higher bitlines are a cyclic descending run (N-1 3), not separate
    // sixteen-word powers. This is the layout consumed by every native
    // Projector<N>:project specialization.
    for (std::uint32_t line = 0; line < 3u; ++line) {
        const std::uint32_t mask = std::uint32_t{1} << line;
        for (std::uint32_t sample = 0; sample < 16u; ++sample)
            masks.push_back(mask);
    }
    const std::uint32_t tail_lines = bitline_count - 3u;
    for (std::size_t index = masks.size(); index < first_region; ++index) {
        const std::uint32_t tail_index = static_cast<std::uint32_t>(index - 48u);
        const std::uint32_t line = bitline_count - 1u - (tail_index % tail_lines);
        masks.push_back(std::uint32_t{1} << line);
    }
    for (std::uint32_t sample = 16u; sample < source_count; ++sample) {
        const std::uint32_t last_line = (sample & 15u) == 0u ? 1u : 0u;
        for (std::uint32_t line = bitline_count; line-- > last_line;)
            masks.push_back(std::uint32_t{1} << line);
    }
    if (masks.size() != output_count || masks.size() < first_region) {
        error = "codec-v3 PCM mux mask plan has an invalid native size";
        masks.clear();
        return false;
    }
    return true;
}

bool serialize_pcm_mux_projector_selectors(
    std::uint32_t source_count,
    std::uint32_t bitline_count,
    const std::array<std::uint32_t, 36>& selectors,
    std::vector<std::uint32_t>& masks,
    std::string& error) {
    if (source_count != 16u || bitline_count > 8u) {
        // Static 36-byte selector prefix covers only the first 16 samples with
        // N<=8. N in 3..16 source_count>=16 use serialize_pcm_mux_projector_records.
        error =
            "codec-v3 PCM mux static selector prefix requires source_count==16 "
            "and bitline_count<=8; use projector records for N>8 or longer spans";
        masks.clear();
        return false;
    }
    if (!make_pcm_mux_mask_plan(source_count, bitline_count, masks, error))
        return false;
    std::size_t cursor = 0u;
    bool complete = false;
    const auto apply_field = [&](std::uint32_t value, std::uint32_t width) {
        if (complete)
            return true;
        if (width == 0u || cursor > masks.size() || width > masks.size() - cursor) {
            error = "codec-v3 PCM mux selector stream is shorter than its mask plan";
            return false;
        }
        for (std::uint32_t bit = width; bit-- != 0u;) {
            if (((value >> bit) & 1u) == 0u)
                masks[cursor] = 0u;
            ++cursor;
        }
        complete = cursor == masks.size();
        return true;
    };
    if (!apply_field(selectors[0], 8u))
        return false;
    for (std::size_t index = 1u; index <= 4u; ++index) {
        if (!apply_field(selectors[index] & 1u, 1u))
            return false;
    }
    if (!apply_field(selectors[5], 4u)
        || !apply_field(selectors[6], 8u)
        || !apply_field(selectors[7], 8u)
        || !apply_field(selectors[8] & 1u, 1u)
        || !apply_field(selectors[9] & 1u, 1u)
        || !apply_field(selectors[10], 2u)
        || !apply_field(selectors[11], 4u)
        || !apply_field(selectors[12], 8u)
        || !apply_field(selectors[13], 8u)
        || !apply_field(selectors[14], 8u)
        || !apply_field(selectors[15], 8u)) {
        return false;
    }
    for (const std::size_t index : {16u, 20u, 24u, 28u}) {
        const std::uint32_t value = selectors[index] < 31u ? selectors[index] : 255u;
        if (!apply_field(value, 8u))
            return false;
    }
    for (std::size_t index = 32u; index != 36u; ++index) {
        if (!apply_field(selectors[index], 8u))
            return false;
    }
    if (!complete) {
        error = "codec-v3 PCM mux selector stream did not consume its mask plan";
        masks.clear();
        return false;
    }
    return true;
}

bool serialize_pcm_mux_projector_record(
    std::uint32_t selector,
    const std::array<std::uint32_t, 3>& values,
    std::uint32_t value_count,
    std::vector<std::uint32_t>& masks,
    std::size_t& cursor,
    std::string& error) {
    error.clear();
    if (selector > 0xFFu || value_count > values.size()) {
        error = "codec-v3 PCM mux projector record has an invalid selector payload";
        return false;
    }
    const auto apply_field = [&](std::uint32_t value, std::uint32_t width) {
        if (width == 0u || width > 32u || cursor > masks.size()
            || width > masks.size() - cursor
            || (width != 32u && (value >> width) != 0u)) {
            error = "codec-v3 PCM mux projector record exceeds its mask plan";
            return false;
        }
        for (std::uint32_t bit = width; bit-- != 0u;) {
            if (((value >> bit) & 1u) == 0u)
                masks[cursor] = 0u;
            ++cursor;
        }
        return true;
    };

    // Every native overload consumes the eight selector bits first. A zero
    // selector is the dynamic-record terminator and has no value payload.
    if (!apply_field(selector, 8u))
        return false;
    if (selector == 0u) {
        if (value_count != 0u) {
            error = "codec-v3 PCM mux terminator has a payload";
            return false;
        }
        return true;
    }

    if (selector == 64u) {
        if (value_count != 2u) {
            error = "codec-v3 PCM mux selector 64 requires two values";
            return false;
        }
        return apply_field(values[0], 8u) && apply_field(values[1], 8u);
    }
    if (selector == 100u) {
        if (value_count != 3u) {
            error = "codec-v3 PCM mux selector 100 requires three values";
            return false;
        }
        return apply_field(values[0], 8u)
            && apply_field(values[1], 8u)
            && apply_field(values[2], 8u);
    }

    std::uint32_t width = 0u;
    if (selector == 1u || selector == 3u || (selector >= 90u && selector <= 98u))
        width = 16u;
    else if (selector == 2u || selector == 4u || selector == 30u || selector == 31u
        || selector == 65u || selector == 71u || (selector >= 80u && selector <= 88u))
        width = 8u;
    else if (selector == 14u || selector == 70u
        || (selector >= 110u && selector <= 118u)
        || (selector >= 128u && selector <= 133u)
        || (selector >= 140u && selector <= 145u))
        width = 32u;
    else if (selector >= 101u && selector <= 108u)
        width = 24u;
    else {
        error = "codec-v3 PCM mux projector selector is not accepted by native serializer";
        return false;
    }
    if (value_count != 1u) {
        error = "codec-v3 PCM mux projector scalar record has the wrong value count";
        return false;
    }
    return apply_field(values[0], width);
}

bool serialize_pcm_mux_projector_records(
    std::uint32_t source_count,
    std::uint32_t bitline_count,
    const std::vector<PcmMuxProjectorRecord>& records,
    bool append_terminator,
    std::vector<std::uint32_t>& masks,
    std::string& error) {
    error.clear();
    masks.clear();
    if (!make_pcm_mux_mask_plan(source_count, bitline_count, masks, error))
        return false;

    std::size_t cursor = 0u;
    for (const PcmMuxProjectorRecord& record : records) {
        if (!serialize_pcm_mux_projector_record(
                record.selector, record.values, record.value_count,
                masks, cursor, error)) {
            masks.clear();
            return false;
        }
        if (record.selector == 0u) {
            error = "codec-v3 PCM mux projector record stream contains an early terminator";
            masks.clear();
            return false;
        }
    }
    if (append_terminator) {
        const std::array<std::uint32_t, 3> values{};
        if (!serialize_pcm_mux_projector_record(
                0u, values, 0u, masks, cursor, error)) {
            masks.clear();
            return false;
        }
    }
    if (cursor != masks.size()) {
        error = "codec-v3 PCM mux projector record stream did not consume its mask plan";
        masks.clear();
        return false;
    }
    return true;
}

bool apply_pcm_mux_projection(
    std::uint32_t bitline_count,
    std::vector<std::uint32_t>& words,
    const std::vector<std::uint32_t>& masks,
    std::string& error) {
    error.clear();
    if (bitline_count < 3u || bitline_count > 16u) {
        error = "codec-v3 PCM mux bitline count must be in the native range 3..16";
        return false;
    }
    if (words.size() < 16u || masks.size() < static_cast<std::size_t>(bitline_count) * 16u) {
        error = "codec-v3 PCM mux projection range is shorter than the native prefix";
        return false;
    }
    if (words.size() > std::numeric_limits<std::uint32_t>::max()) {
        error = "codec-v3 PCM mux source range exceeds the native count type";
        return false;
    }
    for (const std::uint32_t word : words) {
        const std::uint32_t high = word >> 24u;
        if (high != 0u && high != 0xFFu) {
            error = "codec-v3 PCM mux source contains a non-PCM24 word";
            return false;
        }
    }
    std::size_t expected_mask_count = 0u;
    if (!checked_output_count(
            static_cast<std::uint32_t>(words.size()), bitline_count, expected_mask_count)
        || masks.size() != expected_mask_count) {
        error = "codec-v3 PCM mux projection mask count does not match the source range";
        return false;
    }
    const std::vector<std::uint32_t> source = words;
    const std::size_t tail_start = 48u;
    const std::size_t tail_stride = static_cast<std::size_t>(bitline_count - 3u);
    for (std::size_t sample = 0; sample < 16u; ++sample) {
        std::uint32_t value = source[sample] | masks[sample]
            | masks[32u + sample];
        for (std::size_t tail = 0; tail < tail_stride; ++tail)
            value |= masks[tail_start + sample * tail_stride + tail];
        if ((value >> 24u) != 0u && (value >> 24u) != 0xFFu) {
            error = "codec-v3 PCM mux projection exceeds signed PCM24 range";
            return false;
        }
        words[sample] = value;
    }
    std::size_t cursor = static_cast<std::size_t>(bitline_count) * 16u;
    for (std::size_t sample = 16u; sample < words.size(); ++sample) {
        const std::size_t consumed = (sample & 15u) == 0u
            ? static_cast<std::size_t>(bitline_count - 1u)
            : static_cast<std::size_t>(bitline_count);
        if (cursor + consumed > masks.size()) {
            error = "codec-v3 PCM mux projection consumed beyond its mask plan";
            return false;
        }
        std::uint32_t value = source[sample];
        for (std::size_t index = 0; index < consumed; ++index)
            value |= masks[cursor + index];
        if ((value >> 24u) != 0u && (value >> 24u) != 0xFFu) {
            error = "codec-v3 PCM mux projection exceeds signed PCM24 range";
            return false;
        }
        words[sample] = value;
        cursor += consumed;
    }
    if (cursor != masks.size()) {
        error = "codec-v3 PCM mux projection did not consume its mask plan";
        return false;
    }
    return true;
}

bool apply_pcm_mux_crc_projection(
    std::vector<std::uint32_t>& words,
    const std::vector<std::uint32_t>& masks,
    std::string& error) {
    error.clear();
    if (words.size() < 16u || masks.size() < 32u) {
        error = "codec-v3 PCM mux CRC projection requires sixteen output words";
        return false;
    }
    for (std::size_t sample = 0; sample < 16u; ++sample) {
        const std::uint32_t value = words[sample] | masks[16u + sample];
        if ((value >> 24u) != 0u && (value >> 24u) != 0xFFu) {
            error = "codec-v3 PCM mux CRC projection exceeds signed PCM24 range";
            return false;
        }
        words[sample] = value;
    }
    return true;
}

bool compute_pcm_mux_crc16(
    const std::vector<std::uint32_t>& words,
    std::uint16_t& crc,
    std::string& error) {
    error.clear();
    crc = 0u;
    if (words.size() < 16u) {
        error = "codec-v3 PCM mux CRC requires at least sixteen words";
        return false;
    }
    Crc16 state;
    state.process_words(words.data(), words.size());
    // Crc16 keeps the table state byte-swapped. Projector:project_crc
    // consumes the native complemented word in wire bit order, which is
    // exactly stored_word, not the unswapped complement of raw.
    crc = state.stored_word();
    return true;
}

bool apply_pcm_mux_records(
    std::uint32_t bitline_count,
    std::vector<std::uint32_t>& words,
    const std::vector<PcmMuxProjectorRecord>& records,
    bool append_terminator,
    std::uint16_t& crc,
    std::string& error) {
    error.clear();
    crc = 0u;
    if (words.size() > std::numeric_limits<std::uint32_t>::max()) {
        error = "codec-v3 PCM mux source range exceeds the native count type";
        return false;
    }
    std::vector<std::uint32_t> masks;
    if (!serialize_pcm_mux_projector_records(
            static_cast<std::uint32_t>(words.size()), bitline_count,
            records, append_terminator, masks, error)
        || !apply_pcm_mux_projection(bitline_count, words, masks, error)
        || !compute_pcm_mux_crc16(words, crc, error)) {
        return false;
    }
    // Projector:project_crc consumes the CRC-derived mask words after the
    // data CRC has been calculated; including them in the calculation would
    // feed the closure bits back into the checksum.
    if (!apply_pcm_mux_crc_projection(words, masks, error))
        return false;
    return true;
}

} // namespace auro3d:encode
