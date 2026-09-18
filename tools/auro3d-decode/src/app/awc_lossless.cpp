#include "awc_lossless.hpp"
#include "awc_lpc_tables.hpp"

#include <algorithm>
#include <cstring>

namespace auro3d {
namespace awc {
namespace {

constexpr std::int64_t kLpcShift = 0x800000LL;

std::uint32_t index_bits(std::uint32_t maximum) {
    unsigned n = 0;
    while (maximum) {
        ++n;
        maximum >>= 1;
    }
    return n;
}

unsigned ldc_custom_max_read_width(std::size_t total) {
    if (total <= 1)
        return UINT32_MAX;
    if (total == 2)
        return 0;
    return index_bits(static_cast<std::uint32_t>(total - 2u));
}

bool read_flag(cx::Bits& bits, bool& flag) {
    std::uint32_t v = 0;
    if (!bits.get(1, v))
        return false;
    flag = v != 0;
    return true;
}

struct LdcEntropyParams {
    std::uint32_t type = 0;
    unsigned width = 0;
    unsigned golomb_param = 0;
};

std::uint32_t ldc_width_mask(unsigned width) {
    return width >= 32 ? 0xFFFFFFFFu : ((1u << width) - 1u);
}

bool skip_unary_zero_terminator(cx::Bits& bits) {
    std::uint32_t zero = 0;
    return bits.get(1, zero) && zero == 0u;
}

bool read_golomb_payload_after_unary(
    cx::Bits& bits,
    unsigned golomb_param,
    std::uint32_t& masked_rem) {
    if (golomb_param + 1u > bits.remaining_bits())
        return false;
    if (!skip_unary_zero_terminator(bits))
        return false;
    std::uint32_t rem = 0;
    if (!bits.get(golomb_param, rem))
        return false;
    masked_rem = rem & ldc_width_mask(golomb_param);
    return true;
}

bool decode_golomb_rice_block(cx::Bits& bits, unsigned param, std::size_t count, std::vector<std::int32_t>& out);
bool decode_ldc_entropy_common_golomb(cx::Bits& bits, std::size_t count, std::vector<std::int32_t>& out);
bool decode_ldc_entropy_raw(cx::Bits& bits, std::size_t count, std::vector<std::int32_t>& out);
bool decode_ldc_entropy_unary(cx::Bits& bits, std::size_t count, std::vector<std::int32_t>& out);
bool read_ldc_entropy_symbol(cx::Bits& bits, const LdcEntropyParams& params, std::uint32_t& value);

bool read_ldc_entropy_params_body(cx::Bits& bits, LdcEntropyParams& params) {
    if (params.type == 2 || params.type == 3)
        return bits.get(4, params.golomb_param);
    if (params.type == 0) {
        bool has_width = false;
        if (!read_flag(bits, has_width))
            return false;
        if (!has_width) {
            params.width = 0;
            return true;
        }
        std::uint32_t raw = 0;
        if (!bits.get(5, raw))
            return false;
        params.width = raw + 1;
        return true;
    }
    if (params.type == 1)
        return true;
    return false;
}

bool read_ldc_entropy_params(cx::Bits& bits, LdcEntropyParams& params) {
    if (!bits.get(2, params.type))
        return false;
    return read_ldc_entropy_params_body(bits, params);
}

bool decode_ldc_entropy_with_params(cx::Bits& bits, std::size_t count, const LdcEntropyParams& params, std::vector<std::int32_t>& out) {
    if (params.type == 2 || params.type == 3) {
        out.resize(count);
        for (std::size_t i = 0; i < count; ++i) {
            std::uint32_t value = 0;
            if (!read_ldc_entropy_symbol(bits, params, value))
                return false;
            out[i] = static_cast<std::int32_t>(value);
        }
        return true;
    }
    if (params.type == 0) {
        if (!params.width) {
            out.assign(count, 0);
            return true;
        }
        out.resize(count);
        const std::uint32_t mask = ldc_width_mask(params.width);
        for (std::size_t i = 0; i < count; ++i) {
            std::uint32_t raw = 0;
            if (!bits.get(params.width, raw))
                return false;
            out[i] = static_cast<std::int32_t>(raw & mask);
        }
        return true;
    }
    if (params.type == 1)
        return decode_ldc_entropy_unary(bits, count, out);
    return false;
}

bool decode_ldc_signaling_fixed_fixed(
    cx::Bits& bits,
    std::size_t count,
    unsigned width_marker,
    unsigned width_skip,
    std::vector<std::int32_t>& out) {
    if (!width_marker)
        return decode_ldc_entropy_raw(bits, count, out);
    out.assign(count, 0);
    const std::uint32_t mask_marker = ldc_width_mask(width_marker);
    const std::uint32_t mask_skip = width_skip ? ldc_width_mask(width_skip) : 0u;
    std::size_t index = 0;
    while (index < count) {
        std::uint32_t marker = 0;
        if (!bits.get(width_marker, marker))
            return false;
        out[index++] = static_cast<std::int32_t>(marker & mask_marker);
        if (index >= count)
            return true;
        if (out[index - 1] != 0)
            continue;
        if (!width_skip)
            continue;
        std::uint32_t skip = 0;
        if (!bits.get(width_skip, skip))
            return false;
        index += static_cast<std::size_t>(skip & mask_skip);
        if (index > count)
            return false;
    }
    return index == count;
}

bool decode_ldc_signaling_golomb_pair(
    cx::Bits& bits,
    std::size_t count,
    unsigned marker_width,
    unsigned golomb_param,
    std::vector<std::int32_t>& out) {
    if (!marker_width)
        return decode_golomb_rice_block(bits, golomb_param, count, out);
    out.assign(count, 0);
    const std::uint32_t mask_marker = ldc_width_mask(marker_width);
    const std::uint32_t mask_golomb = ldc_width_mask(golomb_param);
    const unsigned golomb_read_bits = golomb_param + 1;
    std::size_t index = 0;
    while (index < count) {
        std::uint32_t marker = 0;
        if (!bits.get(marker_width, marker))
            return false;
        out[index++] = static_cast<std::int32_t>(marker & mask_marker);
        if (index >= count)
            return true;
        if (out[index - 1] != 0)
            continue;
        const std::uint64_t unary = bits.pop_all_ones();
        std::uint32_t remainder = 0;
        if (!read_golomb_payload_after_unary(bits, golomb_param, remainder))
            return false;
        const std::size_t advance =
            static_cast<std::size_t>(remainder | (static_cast<std::uint32_t>(unary) << golomb_param));
        index += advance;
        if (index > count)
            return false;
    }
    return index == count;
}

bool read_ldc_unary_skip_2bit(cx::Bits& bits, std::uint32_t& skip) {
    bool first = false;
    if (!read_flag(bits, first))
        return false;
    if (!first) {
        skip = 0;
        return true;
    }
    bool second = false;
    if (!read_flag(bits, second))
        return false;
    skip = static_cast<std::uint32_t>(second) + 1u;
    return true;
}

bool read_ldc_unary_marker_2bit(cx::Bits& bits, std::uint32_t& marker) {
    bool first = false;
    if (!read_flag(bits, first))
        return false;
    if (!first) {
        marker = 0;
        return true;
    }
    bool second = false;
    if (!read_flag(bits, second))
        return false;
    marker = static_cast<std::uint32_t>(second) + 1u;
    return true;
}

bool decode_ldc_signaling_unary_dense_2bit(cx::Bits& bits, std::size_t count, std::vector<std::int32_t>& out) {
    out.resize(count);
    for (std::size_t i = 0; i < count; ++i) {
        std::uint32_t marker = 0;
        if (!read_ldc_unary_marker_2bit(bits, marker))
            return false;
        out[i] = static_cast<std::int32_t>(marker);
    }
    return true;
}

bool decode_ldc_signaling_unary_fixed(
    cx::Bits& bits,
    std::size_t count,
    unsigned width_skip,
    std::vector<std::int32_t>& out) {
    if (!width_skip)
        return decode_ldc_signaling_unary_dense_2bit(bits, count, out);
    out.assign(count, 0);
    const std::uint32_t mask_skip = ldc_width_mask(width_skip);
    std::size_t index = 0;
    while (index < count) {
        std::uint32_t marker = 0;
        if (!read_ldc_unary_marker_2bit(bits, marker))
            return false;
        out[index++] = static_cast<std::int32_t>(marker);
        if (index >= count)
            return true;
        if (out[index - 1] != 0)
            continue;
        std::uint32_t skip = 0;
        if (!bits.get(width_skip, skip))
            return false;
        skip &= mask_skip;
        if (!skip)
            return false;
        index += static_cast<std::size_t>(skip);
        if (index > count)
            return false;
    }
    return index == count;
}

bool decode_ldc_signaling_unary_unary(cx::Bits& bits, std::size_t count, std::vector<std::int32_t>& out) {
    out.assign(count, 0);
    std::size_t index = 0;
    while (index < count) {
        std::uint32_t marker = 0;
        if (!read_ldc_unary_marker_2bit(bits, marker))
            return false;
        out[index++] = static_cast<std::int32_t>(marker);
        if (index >= count)
            return true;
        if (out[index - 1] != 0)
            continue;
        std::uint32_t skip = 0;
        if (!read_ldc_unary_skip_2bit(bits, skip))
            return false;
        index += skip;
        if (index > count)
            return false;
    }
    return index == count;
}

bool decode_ldc_signaling_fixed_unary(
    cx::Bits& bits,
    std::size_t count,
    unsigned width_marker,
    std::vector<std::int32_t>& out) {
    if (!width_marker)
        return decode_ldc_entropy_unary(bits, count, out);
    out.assign(count, 0);
    const std::uint32_t mask_marker = ldc_width_mask(width_marker);
    std::size_t index = 0;
    while (index < count) {
        std::uint32_t marker = 0;
        if (!bits.get(width_marker, marker))
            return false;
        out[index++] = static_cast<std::int32_t>(marker & mask_marker);
        if (index >= count)
            return true;
        if (out[index - 1] != 0)
            continue;
        std::uint32_t skip = 0;
        if (!read_ldc_unary_skip_2bit(bits, skip))
            return false;
        index += skip;
        if (index > count)
            return false;
    }
    return index == count;
}

bool read_ldc_entropy_symbol(
    cx::Bits& bits,
    const LdcEntropyParams& params,
    std::uint32_t& value) {
    value = 0;
    if (params.type == 0) {
        if (!params.width)
            return true;
        return bits.get(params.width, value);
    }
    if (params.type == 1)
        return read_ldc_unary_marker_2bit(bits, value);
    if (params.type != 2 && params.type != 3)
        return false;

    const std::uint64_t unary64 = bits.pop_all_ones();
    if (params.type == 2 &&
        (params.golomb_param >= 32u || unary64 > (UINT32_MAX >> params.golomb_param)))
        return false;
    if (params.type == 3 && unary64 > 32u - params.golomb_param)
        return false;
    const unsigned unary = static_cast<unsigned>(unary64);
    const unsigned remainder_width = params.type == 3
        ? params.golomb_param + unary
        : params.golomb_param;
    if (remainder_width > 32u || remainder_width + 1u > bits.remaining_bits())
        return false;
    if (!skip_unary_zero_terminator(bits))
        return false;

    std::uint32_t remainder = 0;
    if (!bits.get(remainder_width, remainder))
        return false;
    remainder &= ldc_width_mask(remainder_width);

    if (params.type == 2) {
        value = remainder | static_cast<std::uint32_t>(unary64 << params.golomb_param);
        return true;
    }

    const std::uint64_t unary_offset = unary
        ? (std::uint64_t(ldc_width_mask(unary)) << params.golomb_param)
        : 0u;
    if (unary_offset + remainder > UINT32_MAX)
        return false;
    value = static_cast<std::uint32_t>(remainder + unary_offset);
    return true;
}

bool decode_ldc_signaling_exact(
    cx::Bits& bits,
    std::size_t count,
    const LdcEntropyParams& marker_params,
    const LdcEntropyParams& skip_params,
    std::vector<std::int32_t>& out,
    std::string& detail) {
    out.assign(count, 0);
    std::size_t index = 0;
    while (index < count) {
        std::uint32_t marker = 0;
        if (!read_ldc_entropy_symbol(bits, marker_params, marker)) {
            detail = "marker read failed index=" + std::to_string(index) +
                    " bit=" + std::to_string(bits.position());
            return false;
        }
        out[index++] = static_cast<std::int32_t>(marker);
        if (marker || index >= count)
            continue;

        std::uint32_t skip = 0;
        if (!read_ldc_entropy_symbol(bits, skip_params, skip)) {
            detail = "skip read failed index=" + std::to_string(index) +
                    " bit=" + std::to_string(bits.position());
            return false;
        }
        if (skip > count - index) {
            detail = "skip out of range index=" + std::to_string(index) +
                    " skip=" + std::to_string(skip) + " count=" + std::to_string(count) +
                    " bit=" + std::to_string(bits.position());
            return false;
        }
        index += skip;
    }
    return index == count;
}

bool decode_ldc_alternating_fixed_skip_marker(
    cx::Bits& bits,
    std::size_t count,
    unsigned marker_width,
    unsigned skip_width,
    std::vector<std::int32_t>& out) {
    if (!marker_width || !skip_width)
        return false;
    out.assign(count, 0);
    const std::uint32_t mask_marker = ldc_width_mask(marker_width);
    const std::uint32_t mask_skip = ldc_width_mask(skip_width);
    std::size_t index = 0;
    while (index < count) {
        std::uint32_t skip_raw = 0;
        if (skip_width > bits.remaining_bits() || !bits.get(skip_width, skip_raw))
            return false;
        const std::size_t next = index + static_cast<std::size_t>(skip_raw & mask_skip);
        if (next > count)
            return false;
        if (next == count)
            return true;
        if (marker_width > bits.remaining_bits())
            return false;
        std::uint32_t marker_raw = 0;
        if (!bits.get(marker_width, marker_raw))
            return false;
        out[next] = static_cast<std::int32_t>((marker_raw & mask_marker) + 1);
        index = next + 1;
        if (index >= count)
            return true;
    }
    return index == count;
}

bool decode_ldc_alternating_fixed_dense(cx::Bits& bits, std::size_t count, unsigned width, std::vector<std::int32_t>& out) {
    out.assign(count, 0);
    if (!width) {
        for (std::size_t i = 0; i < count; ++i)
            out[i] = 1;
        return true;
    }
    const std::uint32_t mask = ldc_width_mask(width);
    for (std::size_t index = 0; index < count; ++index) {
        std::uint32_t raw = 0;
        if (!bits.get(width, raw))
            return false;
        out[index] = static_cast<std::int32_t>((raw & mask) + 1);
    }
    return true;
}

bool decode_ldc_alternating_fixed_unary(cx::Bits& bits, std::size_t count, unsigned width, std::vector<std::int32_t>& out) {
    if (!width)
        return false;
    out.assign(count, 0);
    const std::uint32_t mask = ldc_width_mask(width);
    std::size_t index = 0;
    while (index < count) {
        std::uint32_t skip = 0;
        if (!read_ldc_unary_skip_2bit(bits, skip))
            return false;
        index += skip;
        if (index >= count)
            return true;
        std::uint32_t raw = 0;
        if (!bits.get(width, raw))
            return false;
        out[index++] = static_cast<std::int32_t>((raw & mask) + 1);
    }
    return true;
}

bool decode_ldc_signaling(
    cx::Bits& bits,
    std::size_t count,
    const LdcEntropyParams& first,
    const LdcEntropyParams& second,
    std::vector<std::int32_t>& out,
    std::string& detail) {
    // signaling:Decoder:decode (~) always decodes one marker with
    // the first entropy policy, advances one output slot, and only when that
    // marker is zero decodes a skip with the second policy. Entropy type 3 is
    // not ordinary Golomb-Rice: its remainder grows by the unary prefix and
    // carries the ((1 << unary) - 1) << golomb_param offset.
    return decode_ldc_signaling_exact(bits, count, first, second, out, detail);
}

bool decode_ldc_alternating_fixed_sparse(cx::Bits& bits, std::size_t count, unsigned width, std::vector<std::int32_t>& out) {
    if (!width)
        return false;
    out.assign(count, 0);
    const std::uint32_t mask = ldc_width_mask(width);
    for (std::size_t index = 0; index + 1 < count; ++index) {
        std::uint32_t raw = 0;
        if (!bits.get(width, raw))
            return false;
        out[index] = static_cast<std::int32_t>((raw & mask) + 1);
    }
    return true;
}

bool decode_ldc_alternating_golomb_sparse(
    cx::Bits& bits,
    std::size_t count,
    unsigned marker_golomb,
    unsigned index_golomb,
    std::vector<std::int32_t>& out) {
    out.assign(count, 0);
    const std::uint32_t mask_index = ldc_width_mask(index_golomb);
    const unsigned index_read_bits = index_golomb + 1;
    const std::uint32_t mask_marker = ldc_width_mask(marker_golomb);
    const unsigned marker_read_bits = marker_golomb + 1;
    std::size_t index = 0;
    while (index < count) {
        const std::uint64_t unary = bits.pop_all_ones();
        std::uint32_t index_rem = 0;
        if (!read_golomb_payload_after_unary(bits, index_golomb, index_rem))
            return false;
        const std::size_t next =
            static_cast<std::size_t>((index_rem | (static_cast<std::uint32_t>(unary) << index_golomb)) + index);
        if (next > count)
            return false;
        if (next == count)
            return true;
        if (marker_read_bits > bits.remaining_bits())
            return false;
        std::uint32_t marker_rem = 0;
        if (!bits.get(marker_read_bits, marker_rem))
            return false;
        out[next] = static_cast<std::int32_t>((marker_rem & mask_marker) + 1);
        index = next + 1;
        if (index >= count)
            return true;
    }
    return index == count;
}

bool decode_ldc_alternating_fixed_golomb(
    cx::Bits& bits,
    std::size_t count,
    unsigned marker_width,
    unsigned index_golomb,
    std::vector<std::int32_t>& out) {
    out.assign(count, 0);
    const std::uint32_t mask_marker = ldc_width_mask(marker_width);
    const std::uint32_t mask_index = ldc_width_mask(index_golomb);
    const unsigned index_read_bits = index_golomb + 1;
    std::size_t index = 0;
    while (index < count) {
        const std::uint64_t unary = bits.pop_all_ones();
        std::uint32_t index_rem = 0;
        if (!read_golomb_payload_after_unary(bits, index_golomb, index_rem))
            return false;
        const std::size_t next = index + static_cast<std::size_t>(
            (index_rem | (static_cast<std::uint32_t>(unary) << index_golomb)));
        if (next > count)
            return false;
        if (next == count)
            return true;
        if (marker_width > bits.remaining_bits())
            return false;
        std::uint32_t marker_rem = 0;
        if (!bits.get(marker_width, marker_rem))
            return false;
        out[next] = static_cast<std::int32_t>((marker_rem & mask_marker) + 1);
        index = next + 1;
    }
    return index == count;
}

bool decode_ldc_alternating_entropy_fixed(
    cx::Bits& bits,
    std::size_t count,
    const LdcEntropyParams& marker_params,
    unsigned index_width,
    std::vector<std::int32_t>& out,
    std::string& detail) {
    out.assign(count, 0);
    if (!index_width) {
        for (std::size_t index = 0; index < count; ++index) {
            std::uint32_t marker = 0;
            if (!read_ldc_entropy_symbol(bits, marker_params, marker)) {
                detail = "dense marker read failed index=" + std::to_string(index) +
                        " bit=" + std::to_string(bits.position());
                return false;
            }
            out[index] = static_cast<std::int32_t>(marker + 1u);
        }
        return true;
    }

    const std::uint32_t mask_index = ldc_width_mask(index_width);
    std::size_t index = 0;
    while (index < count) {
        if (index_width > bits.remaining_bits()) {
            detail = "fixed index truncated index=" + std::to_string(index) +
                    " bit=" + std::to_string(bits.position());
            return false;
        }
        std::uint32_t skip_rem = 0;
        if (!bits.get(index_width, skip_rem))
            return false;
        const std::size_t next = index + static_cast<std::size_t>(skip_rem & mask_index);
        if (next > count) {
            detail = "fixed index out of range index=" + std::to_string(index) +
                    " skip=" + std::to_string(skip_rem & mask_index) +
                    " count=" + std::to_string(count) +
                    " bit=" + std::to_string(bits.position());
            return false;
        }
        if (next == count)
            return true;
        std::uint32_t marker = 0;
        if (!read_ldc_entropy_symbol(bits, marker_params, marker)) {
            detail = "marker read failed index=" + std::to_string(next) +
                    " bit=" + std::to_string(bits.position());
            return false;
        }
        out[next] = static_cast<std::int32_t>(marker + 1u);
        index = next + 1;
    }
    return true;
}

bool decode_ldc_alternating_entropy_entropy(
    cx::Bits& bits,
    std::size_t count,
    const LdcEntropyParams& marker_params,
    const LdcEntropyParams& index_params,
    std::vector<std::int32_t>& out,
    std::string& detail) {
    out.assign(count, 0);
    std::size_t index = 0;
    while (index < count) {
        std::uint32_t jump = 0;
        if (!read_ldc_entropy_symbol(bits, index_params, jump)) {
            detail = "index read failed index=" + std::to_string(index) +
                    " bit=" + std::to_string(bits.position());
            return false;
        }
        if (jump > count - index) {
            detail = "index out of range index=" + std::to_string(index) +
                    " jump=" + std::to_string(jump) + " count=" + std::to_string(count) +
                    " bit=" + std::to_string(bits.position());
            return false;
        }
        const std::size_t next = index + jump;
        if (next == count)
            return true;
        std::uint32_t marker = 0;
        if (!read_ldc_entropy_symbol(bits, marker_params, marker)) {
            detail = "marker read failed index=" + std::to_string(next) +
                    " bit=" + std::to_string(bits.position());
            return false;
        }
        out[next] = static_cast<std::int32_t>(marker + 1u);
        index = next + 1;
    }
    return true;
}

bool decode_ldc_alternating_unary_dense(cx::Bits& bits, std::size_t count, std::vector<std::int32_t>& out) {
    out.assign(count, 0);
    for (std::size_t index = 0; index < count; ++index) {
        std::uint32_t marker = 0;
        if (!read_ldc_unary_marker_2bit(bits, marker))
            return false;
        out[index] = static_cast<std::int32_t>(marker + 1);
    }
    return true;
}

bool decode_ldc_alternating_unary_fixed(
    cx::Bits& bits,
    std::size_t count,
    unsigned skip_width,
    std::vector<std::int32_t>& out) {
    if (!skip_width)
        return decode_ldc_alternating_unary_dense(bits, count, out);
    out.assign(count, 0);
    const std::uint32_t mask = ldc_width_mask(skip_width);
    std::size_t index = 0;
    while (index < count) {
        std::uint32_t skip = 0;
        if (!bits.get(skip_width, skip))
            return false;
        index += static_cast<std::size_t>(skip & mask);
        if (index >= count)
            return true;
        std::uint32_t marker = 0;
        if (!read_ldc_unary_marker_2bit(bits, marker))
            return false;
        out[index++] = static_cast<std::int32_t>(marker + 1);
    }
    return true;
}

bool decode_ldc_alternating_unary_unary(cx::Bits& bits, std::size_t count, std::vector<std::int32_t>& out) {
    out.assign(count, 0);
    std::size_t index = 0;
    while (index < count) {
        std::uint32_t skip = 0;
        if (!read_ldc_unary_skip_2bit(bits, skip))
            return false;
        index += skip;
        if (index >= count)
            return true;
        std::uint32_t marker = 0;
        if (!read_ldc_unary_marker_2bit(bits, marker))
            return false;
        out[index++] = static_cast<std::int32_t>(marker + 1);
    }
    return true;
}

bool decode_ldc_alternating_golomb_unary(
    cx::Bits& bits,
    std::size_t count,
    unsigned golomb_param,
    std::vector<std::int32_t>& out) {
    // alternating:Decoder (~) case 2 + case 1: 2-bit unary skip, then
    // pop_all_ones + (golomb_param+1)-bit payload written as (mask|unary<<g)+1.
    out.assign(count, 0);
    const std::uint32_t mask = ldc_width_mask(golomb_param);
    const unsigned read_bits = golomb_param + 1;
    std::size_t index = 0;
    while (index < count) {
        std::uint32_t skip = 0;
        if (!read_ldc_unary_skip_2bit(bits, skip))
            return false;
        index += skip;
        if (index >= count)
            return true;
        const std::uint64_t unary = bits.pop_all_ones();
        std::uint32_t rem = 0;
        if (!read_golomb_payload_after_unary(bits, golomb_param, rem))
            return false;
        out[index] = static_cast<std::int32_t>((rem | (static_cast<std::uint32_t>(unary) << golomb_param)) + 1);
        ++index;
    }
    return true;
}

bool decode_ldc_alternating(
    cx::Bits& bits,
    std::size_t count,
    const LdcEntropyParams& first,
    const LdcEntropyParams& second,
    std::vector<std::int32_t>& out,
    std::string& error) {
    std::string detail;
    if (decode_ldc_alternating_entropy_entropy(bits, count, first, second, out, detail))
        return true;
    error = "LDC alternating decode failed t0=" + std::to_string(first.type) +
            " t1=" + std::to_string(second.type) +
            " w0=" + std::to_string(first.width) +
            " w1=" + std::to_string(second.width) +
            " g0=" + std::to_string(first.golomb_param) +
            " g1=" + std::to_string(second.golomb_param) +
            " count=" + std::to_string(count) + " " + detail;
    return false;
}

bool decode_ldc_run_length(cx::Bits& bits, std::size_t count, std::vector<std::int32_t>& out, std::string& error) {
    bool alternating = false;
    if (!read_flag(bits, alternating)) {
        error = "LDC run_length alternating flag read failed";
        return false;
    }
    LdcEntropyParams first{};
    if (!read_ldc_entropy_params(bits, first)) {
        error = "LDC run_length first entropy params read failed";
        return false;
    }
    LdcEntropyParams second{};
    if (!read_ldc_entropy_params(bits, second)) {
        error = "LDC run_length second entropy params read failed";
        return false;
    }
    if (alternating)
        return decode_ldc_alternating(bits, count, first, second, out, error);
    std::string signaling_detail;
    if (!decode_ldc_signaling(bits, count, first, second, out, signaling_detail)) {
        error = "LDC signaling decode failed alt=0 t0=" + std::to_string(first.type) + " t1=" +
                std::to_string(second.type) + " w0=" + std::to_string(first.width) + " w1=" +
                std::to_string(second.width) + " g0=" + std::to_string(first.golomb_param) + " g1=" +
                std::to_string(second.golomb_param) + " count=" + std::to_string(count) + " " + signaling_detail;
        return false;
    }
    return true;
}

bool read_fixed_unsigned(cx::Bits& bits, unsigned width, std::uint32_t& value) {
    return bits.get(width, value);
}

static void ldc_signed_expand(std::vector<std::int32_t>& samples) {
    for (std::int32_t& sample : samples) {
        const std::uint32_t u = static_cast<std::uint32_t>(sample);
        const std::int32_t sign = u & 1;
        sample = sign + (((u & 1) + (u >> 1)) ^ -sign);
    }
}

static void ldc_diff_accumulate(std::vector<std::int32_t>& samples) {
    for (std::size_t i = 1; i < samples.size(); ++i)
        samples[i] = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(samples[i]) +
            static_cast<std::uint32_t>(samples[i - 1]));
}

bool decode_golomb_rice_block(cx::Bits& bits, unsigned param, std::size_t count, std::vector<std::int32_t>& out) {
    out.resize(count);
    for (std::size_t i = 0; i < count; ++i) {
        std::uint64_t value = 0;
        if (!bits.read_golomb_rice(param, value))
            return false;
        if (value > 0x7FFFFFFFu)
            return false;
        out[i] = static_cast<std::int32_t>(value);
    }
    return true;
}

bool decode_ldc_entropy_common_golomb(cx::Bits& bits, std::size_t count, std::vector<std::int32_t>& out) {
    std::uint32_t golomb_param = 0;
    if (!bits.get(4, golomb_param))
        return false;
    return decode_golomb_rice_block(bits, golomb_param, count, out);
}

bool decode_ldc_entropy_raw(cx::Bits& bits, std::size_t count, std::vector<std::int32_t>& out) {
    bool has_width = false;
    if (!read_flag(bits, has_width))
        return false;
    unsigned width = 0;
    if (has_width) {
        std::uint32_t raw = 0;
        if (!bits.get(5, raw))
            return false;
        width = raw + 1;
    }
    out.resize(count);
    if (!width) {
        std::fill(out.begin(), out.end(), 0);
        return true;
    }
    const std::uint32_t mask = width >= 32 ? 0xFFFFFFFFu : ((1u << width) - 1u);
    for (std::size_t i = 0; i < count; ++i) {
        std::uint32_t raw = 0;
        if (!bits.get(width, raw))
            return false;
        out[i] = static_cast<std::int32_t>(raw & mask);
    }
    return true;
}

bool decode_ldc_entropy_unary(cx::Bits& bits, std::size_t count, std::vector<std::int32_t>& out) {
    out.resize(count);
    for (std::size_t i = 0; i < count; ++i) {
        std::uint32_t value = 0;
        if (!read_ldc_unary_marker_2bit(bits, value))
            return false;
        out[i] = static_cast<std::int32_t>(value);
    }
    return true;
}

struct LdcSubelementParams {
    bool use_run_length = false;
    bool alternating = false;
    LdcEntropyParams first{};
    LdcEntropyParams second{};
};

bool read_ldc_subelement_params(cx::Bits& bits, LdcSubelementParams& params, std::string& error) {
    if (!read_flag(bits, params.use_run_length)) {
        error = "LDC subelement run_length flag read failed";
        return false;
    }
    if (params.use_run_length) {
        if (!read_flag(bits, params.alternating)) {
            error = "LDC run_length alternating flag read failed";
            return false;
        }
        if (!read_ldc_entropy_params(bits, params.first)) {
            error = "LDC run_length first entropy params read failed";
            return false;
        }
        if (!read_ldc_entropy_params(bits, params.second)) {
            error = "LDC run_length second entropy params read failed";
            return false;
        }
        return true;
    }
    if (!read_ldc_entropy_params(bits, params.first)) {
        error = "LDC subelement entropy params read failed";
        return false;
    }
    return true;
}

bool decode_ldc_subelement_data(
    cx::Bits& bits,
    std::size_t count,
    const LdcSubelementParams& params,
    std::vector<std::int32_t>& out,
    std::string& error) {
    if (params.use_run_length) {
        if (params.alternating)
            return decode_ldc_alternating(bits, count, params.first, params.second, out, error);
        std::string detail;
        if (!decode_ldc_signaling(bits, count, params.first, params.second, out, detail)) {
            error = "LDC signaling decode failed alt=0 t0=" + std::to_string(params.first.type) +
                    " t1=" + std::to_string(params.second.type) +
                    " w0=" + std::to_string(params.first.width) +
                    " w1=" + std::to_string(params.second.width) +
                    " g0=" + std::to_string(params.first.golomb_param) +
                    " g1=" + std::to_string(params.second.golomb_param) + " " + detail;
            return false;
        }
        return true;
    }
    if (!decode_ldc_entropy_with_params(bits, count, params.first, out)) {
        error = "LDC entropy decode failed type=" + std::to_string(params.first.type);
        return false;
    }
    return true;
}

bool decode_ldc_subelement(cx::Bits& bits, std::size_t count, std::vector<std::int32_t>& out, std::string& error) {
    LdcSubelementParams params{};
    return read_ldc_subelement_params(bits, params, error) &&
           decode_ldc_subelement_data(bits, count, params, out, error);
}

bool read_unsafe_size_plus_one(cx::Bits& bits, unsigned total_bits, std::uint64_t& value);

std::vector<std::uint64_t> clamp_ldc_props_to_total(
    const std::vector<std::uint64_t>& props,
    std::uint64_t total) {
    std::vector<std::uint64_t> out;
    out.reserve(props.size());
    std::uint64_t remaining = total;
    for (std::uint64_t prop : props) {
        if (!remaining)
            break;
        std::uint64_t value = prop;
        if (value >= remaining)
            value = remaining;
        out.push_back(value);
        remaining -= value;
    }
    return out;
}

// read_subelements_granules_ (~): props length is the subelement count; only
// partitioned mode reads extra syntax from the bitstream.
bool read_ldc_subelement_granules(
    cx::Bits& bits,
    std::size_t total_count,
    const std::vector<std::uint64_t>& props,
    std::vector<std::size_t>& sizes,
    std::string& error) {
    sizes.clear();
    error.clear();
    if (!total_count || props.empty()) {
        error = "empty props/total";
        return false;
    }

    if (props.size() == 1) {
        sizes.push_back(total_count);
        return true;
    }

    bool partitioned = false;
    if (!read_flag(bits, partitioned)) {
        error = "partition flag read failed";
        return false;
    }
    if (!partitioned) {
        sizes.reserve(props.size());
        for (std::uint64_t prop : props)
            sizes.push_back(static_cast<std::size_t>(prop));
        return true;
    }

    if (props.size() < 3) {
        sizes.push_back(total_count);
        return true;
    }

    const unsigned count_width = index_bits(static_cast<std::uint32_t>(props.size() - 2));
    const std::uint32_t count_mask = count_width >= 32 ? 0xFFFFFFFFu : ((1u << count_width) - 1u);
    std::uint32_t raw_count = 0;
    if (count_width) {
        if (count_width > bits.remaining_bits() || !bits.get(count_width, raw_count)) {
            error = "partitioned subelement count read failed width=" + std::to_string(count_width);
            return false;
        }
    }
    const std::uint64_t subelement_count = static_cast<std::uint64_t>(raw_count & count_mask) + 1u;
    if (subelement_count < 1) {
        error = "partitioned subelement count invalid";
        return false;
    }

    std::size_t accumulated = 0;
    std::size_t prop_cursor = 0;
    for (std::uint64_t i = 0; i + 1 < subelement_count; ++i) {
        std::uint64_t prop_count = 0;
        if (!read_unsafe_size_plus_one(bits, count_width, prop_count)) {
            error = "partitioned prop-count pair read failed i=" + std::to_string(i);
            return false;
        }
        if (!prop_count || prop_count > props.size() - prop_cursor) {
            error = "partitioned prop-count out of range count=" + std::to_string(prop_count) +
                    " props=" + std::to_string(props.size()) +
                    " cursor=" + std::to_string(prop_cursor);
            return false;
        }
        std::size_t length = 0;
        for (std::uint64_t n = 0; n < prop_count; ++n)
            length += static_cast<std::size_t>(props[prop_cursor + static_cast<std::size_t>(n)]);
        prop_cursor += static_cast<std::size_t>(prop_count);
        if (!length || accumulated + length > total_count) {
            error = "partitioned length invalid i=" + std::to_string(i) + " len=" + std::to_string(length) +
                    " acc=" + std::to_string(accumulated);
            return false;
        }
        sizes.push_back(length);
        accumulated += length;
    }
    if (accumulated >= total_count) {
        error = "partitioned accumulated exceeds total acc=" + std::to_string(accumulated);
        return false;
    }
    sizes.push_back(total_count - accumulated);
    return true;
}

bool initialize_diff_granule_sizes(
    const std::vector<std::uint64_t>& source,
    std::vector<std::uint64_t>& out);

bool decode_ldc_residual_chunks(
    cx::Bits& bits,
    const std::vector<std::uint64_t>& raw_granules,
    std::vector<std::int32_t>& out,
    std::string& error,
    std::string* trace = nullptr) {
    const std::size_t vector_start = bits.position();
    std::size_t total = 0;
    for (std::uint64_t granule : raw_granules)
        total += static_cast<std::size_t>(granule);

    // decode_vector<int> (~): has_residual already consumed; read diff, then
    // ldc:v1:Params:read + ldc:v1:decode<int> for the whole subvector.
    bool diff = false;
    if (!read_flag(bits, diff)) {
        error = "LDC diff flag read failed";
        return false;
    }
    if (trace)
        *trace = "diff=" + std::to_string(diff ? 1 : 0);

    if (total == 0) {
        out.clear();
        return true;
    }

    // Params:read (~): after 'diff', the first flag enables the LDC
    // payload. A clear flag produces an all-zero vector. If set, the next flag
    // selects an optional maximum decoded prefix; granule partitioning is used
    // in both branches.
    bool active = false;
    if (!read_flag(bits, active)) {
        error = "LDC active flag read failed";
        return false;
    }
    if (trace)
        *trace += " active=" + std::to_string(active ? 1 : 0);
    if (!active) {
        if (trace)
            *trace = "diff=" + std::to_string(diff ? 1 : 0) + " active=0";
        out.assign(total, 0);
        return true;
    }

    std::uint64_t max_subelement_size = total;
    bool has_custom_max = false;
    if (!read_flag(bits, has_custom_max)) {
        error = "LDC custom max subelement flag read failed";
        return false;
    }
    if (trace)
        *trace += " custom=" + std::to_string(has_custom_max ? 1 : 0);
    if (has_custom_max) {
        if (total <= 1) {
            error = "LDC custom max subelement size with total<=1";
            return false;
        }
        const unsigned width = ldc_custom_max_read_width(total);
        if (width == UINT32_MAX) {
            error = "LDC custom max subelement size with total<=1";
            return false;
        }
        std::uint32_t raw = 0;
        if (width) {
            if (width > bits.remaining_bits() || !bits.get(width, raw)) {
                error = "LDC custom max subelement size read failed";
                return false;
            }
        }
        max_subelement_size = static_cast<std::uint64_t>(raw) + 1u;
        if (max_subelement_size > total) {
            error = "LDC custom max subelement size exceeds total total=" + std::to_string(total) +
                    " max=" + std::to_string(max_subelement_size) + " width=" + std::to_string(width);
            return false;
        }
    }

    // Properties:(diff_)subvector_granule_sizes: caller provides the subelement sizes.
    std::vector<std::uint64_t> props;
    if (diff) {
        if (!initialize_diff_granule_sizes(raw_granules, props)) {
            error = "LDC diff granule size init failed";
            return false;
        }
    } else {
        props = raw_granules;
    }

    out.clear();
    out.reserve(total);
    std::vector<std::uint64_t> working_props = props;
    if (has_custom_max)
        working_props = clamp_ldc_props_to_total(props, max_subelement_size);

    std::vector<std::size_t> sizes;
    if (!read_ldc_subelement_granules(
            bits,
            static_cast<std::size_t>(max_subelement_size),
            working_props,
            sizes,
            error)) {
        error = "LDC subelement granule sizes read failed: " + error;
        return false;
    }

    std::vector<LdcSubelementParams> subelement_params(sizes.size());
    for (LdcSubelementParams& params : subelement_params) {
        if (!read_ldc_subelement_params(bits, params, error))
            return false;
    }
    if (trace) {
        *trace = "@" + std::to_string(vector_start) + " diff=" + std::to_string(diff ? 1 : 0) +
                 " active=1 custom=" + std::to_string(has_custom_max ? 1 : 0) +
                 " max=" + std::to_string(max_subelement_size) + " parts=";
        for (std::size_t i = 0; i < sizes.size(); ++i) {
            if (i)
                *trace += ',';
            *trace += std::to_string(sizes[i]) + ":" +
                      std::to_string(subelement_params[i].use_run_length ? 1 : 0) + ":" +
                      std::to_string(subelement_params[i].alternating ? 1 : 0) + ":" +
                      std::to_string(subelement_params[i].first.type) + ":" +
                      std::to_string(subelement_params[i].second.type) + ":" +
                      std::to_string(subelement_params[i].first.width) + ":" +
                      std::to_string(subelement_params[i].second.width) + ":" +
                      std::to_string(subelement_params[i].first.golomb_param) + ":" +
                      std::to_string(subelement_params[i].second.golomb_param);
        }
        *trace += " data@" + std::to_string(bits.position());
    }

    std::size_t remaining = static_cast<std::size_t>(max_subelement_size);
    for (std::size_t i = 0; i < sizes.size(); ++i) {
        const std::size_t part = sizes[i];
        if (!remaining)
            break;
        if (!part || part > remaining) {
            error = "LDC subelement size mismatch";
            return false;
        }
        std::vector<std::int32_t> chunk;
        const std::size_t part_start = bits.position();
        if (!decode_ldc_subelement_data(bits, part, subelement_params[i], chunk, error)) {
            if (trace)
                *trace += " part" + std::to_string(i) + "@" + std::to_string(part_start) +
                          "-" + std::to_string(bits.position());
            return false;
        }
        if (trace)
            *trace += " part" + std::to_string(i) + "@" + std::to_string(part_start) +
                      "-" + std::to_string(bits.position());
        out.insert(out.end(), chunk.begin(), chunk.end());
        remaining -= part;
    }
    if (remaining != 0 || out.size() != max_subelement_size) {
        error = "LDC subelement size mismatch";
        return false;
    }
    out.resize(total, 0);

    ldc_signed_expand(out);
    if (diff)
        ldc_diff_accumulate(out);
    return true;
}

void reflect_lpc(std::uint32_t order, const std::int32_t* table_coeffs, std::vector<std::int32_t>& out) {
    out.assign(order, 0);
    if (!order)
        return;
    std::vector<std::int32_t> previous(order, 0);
    std::vector<std::int32_t> current(order, 0);
    previous[0] = table_coeffs[0];
    for (std::uint32_t n = 1; n < order; ++n) {
        const std::int32_t bn = table_coeffs[n];
        for (std::uint32_t k = 0; k < n; ++k) {
            const std::int64_t prod =
                static_cast<std::int64_t>(bn) * previous[n - 1u - k];
            const bool native_shift = (n & 1u) == 0u || k + 1u < n;
            const std::uint64_t shifted = static_cast<std::uint64_t>(
                prod < 0 && native_shift ? prod + 0x7FFFFF : prod);
            const std::int32_t correction = native_shift
                ? static_cast<std::int32_t>(static_cast<std::uint32_t>(shifted >> 23))
                : static_cast<std::int32_t>(prod / kLpcShift);
            current[k] = static_cast<std::int32_t>(
                static_cast<std::uint32_t>(previous[k]) -
                static_cast<std::uint32_t>(correction));
        }
        current[n] = bn;
        previous.swap(current);
    }
    out = std::move(previous);
}

bool icc_mix(
    std::uint32_t angle,
    std::uint32_t source_bitdepth,
    std::uint32_t destination_bitdepth,
    std::int32_t* dst,
    const std::int32_t* src,
    std::size_t count) {
    if (angle >= 63)
        return false;
    std::int64_t gain = kIcpGains[angle];
    if (source_bitdepth <= destination_bitdepth) {
        if (source_bitdepth < destination_bitdepth)
            gain <<= destination_bitdepth - source_bitdepth;
    } else {
        // Policy:icc_decode_subblock: toward-zero arithmetic
        // right-shift — (gain + (((1<<n)-1) & (gain>>63))) >> n.
        const unsigned shift = source_bitdepth - destination_bitdepth;
        const std::int64_t mask =
            shift >= 64u ? -1 : ~(-std::int64_t{1} << shift);
        gain = (gain + (mask & (gain >> 63))) >> shift;
    }
    // Native: count-1 < 11 (or buffer overlap) → scalar truncating `/`;
    // else SIMD rounded Q23 (`bias ` then `>> 23`). Corpus
    // subblocks are long, so the SIMD path is the exercised one.
    const bool use_rounded = count >= 12u;
    for (std::size_t i = 0; i < count; ++i) {
        const std::int64_t mix = gain * static_cast<std::int64_t>(src[i]);
        const std::int64_t scaled = use_rounded
            ? ((mix < 0 ? mix + 0x7FFFFF : mix) >> 23)
            : (mix / kLpcShift);
        dst[i] = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(dst[i]) +
            static_cast<std::uint32_t>(scaled));
    }
    return true;
}

struct StreamChannelState {
    std::vector<std::int32_t> samples;
    std::vector<std::int32_t> pending_errors;
    std::uint32_t pending_sub_len = 0;
    std::uint32_t error_scale = 1;
    std::vector<std::uint32_t> lpc_table_indices;
    bool lpc_history_initialized = false;
};

struct IccPair {
    std::uint32_t left = 0;
    std::uint32_t right = 0;
    std::uint32_t angle = 0;
};

unsigned bitdepth_param_bits_from_config(std::uint8_t error_scale_byte) {
    if (error_scale_byte <= 1)
        return 0;
    return index_bits(static_cast<std::uint32_t>(error_scale_byte) - 1u);
}

unsigned preamble_width(std::uint32_t stream_count) {
    if (stream_count <= 1)
        return 0;
    return index_bits(stream_count - 1);
}

std::uint32_t awc_partition_granule_total(
    std::uint32_t block_size,
    std::uint32_t stream_count,
    std::uint32_t preamble) {
    if (!stream_count || block_size <= preamble)
        return 0;
    const std::uint64_t adjusted =
        static_cast<std::uint64_t>(block_size) + stream_count - preamble - 1u;
    return static_cast<std::uint32_t>(adjusted / stream_count);
}

std::uint32_t awc_interleaved_block_size(
    std::uint32_t samples_per_stream,
    std::uint32_t stream_count) {
    const std::uint64_t total =
        static_cast<std::uint64_t>(samples_per_stream) * stream_count;
    return total <= UINT32_MAX ? static_cast<std::uint32_t>(total) : 0u;
}

bool subvector_granule_sizes(
    std::vector<std::uint64_t>& out,
    std::uint64_t subblock_granule_count,
    std::uint64_t first_segment_cap,
    std::uint64_t segment_size) {
    if (!subblock_granule_count || !segment_size)
        return false;

    out.clear();
    std::uint64_t first = subblock_granule_count;
    if (first_segment_cap < subblock_granule_count)
        first = first_segment_cap;
    out.push_back(first);

    const std::uint64_t remainder = subblock_granule_count - first;
    std::uint64_t repeat_count = 0;
    if ((segment_size | remainder) >> 32)
        repeat_count = remainder / segment_size;
    else
        repeat_count = static_cast<std::uint32_t>(remainder) / static_cast<std::uint32_t>(segment_size);

    for (std::uint64_t i = 0; i < repeat_count; ++i)
        out.push_back(segment_size);

    std::uint64_t sum = 0;
    for (std::uint64_t granule : out)
        sum += granule;
    const std::uint64_t leftover = subblock_granule_count - sum;
    if (!leftover)
        return true;
    if (out.empty())
        return false;
    if (leftover > out.back() / 3u)
        out.push_back(leftover);
    else
        out.back() += leftover;
    return true;
}

bool read_partition_granule_lengths(
    cx::Bits& bits,
    std::uint32_t subblock_count,
    std::uint32_t partition_total,
    std::vector<std::uint32_t>& lengths) {
    lengths.clear();
    if (!subblock_count || !partition_total)
        return false;
    if (subblock_count == 1) {
        lengths.push_back(partition_total);
        return true;
    }

    const unsigned read_width = index_bits(partition_total - 1);
    const std::uint32_t mask = read_width >= 32 ? 0xFFFFFFFFu : ((1u << read_width) - 1u);
    std::uint32_t accumulated = 0;
    lengths.reserve(subblock_count);
    for (std::uint32_t n = 0; n + 1 < subblock_count; ++n) {
        if (read_width > bits.remaining_bits())
            return false;
        std::uint32_t raw = 0;
        if (!bits.get(read_width, raw))
            return false;
        const std::uint32_t length = (raw & mask) + 1u;
        if (!length || accumulated + length > partition_total)
            return false;
        lengths.push_back(length);
        accumulated += length;
    }
    if (accumulated >= partition_total)
        return false;
    lengths.push_back(partition_total - accumulated);
    return true;
}

bool initialize_diff_granule_sizes(
    const std::vector<std::uint64_t>& source,
    std::vector<std::uint64_t>& out) {
    if (source.empty())
        return false;
    if (source[0] <= 1u) {
        out = source;
        return true;
    }
    out.assign(source.size() + 1u, 0);
    out[0] = 1;
    out[1] = source[0] - 1u;
    for (std::size_t i = 1; i < source.size(); ++i)
        out[i + 1] = source[i];
    return true;
}

struct SubblockState {
    std::uint32_t granule_count = 0;
    std::vector<std::uint64_t> ldc_granules;
};

bool read_stream_error_scales(
    cx::Bits& bits,
    std::uint32_t bitdepth_mode,
    std::uint32_t bitdepth_width,
    std::uint8_t error_scale_byte,
    std::size_t stream_count,
    std::vector<std::uint32_t>& scales) {
    scales.assign(stream_count, 1u);
    if (bitdepth_mode == 0) {
        std::fill(scales.begin(), scales.end(), error_scale_byte);
        return true;
    }
    if (bitdepth_mode == 1) {
        std::fill(scales.begin(), scales.end(), bitdepth_width);
        return true;
    }
    if (bitdepth_mode != 2)
        return false;

    const unsigned read_width = bitdepth_param_bits_from_config(error_scale_byte);
    const std::uint32_t mask = read_width >= 32 ? 0xFFFFFFFFu : ((1u << read_width) - 1u);
    for (std::uint32_t& scale : scales) {
        std::uint32_t raw = 0;
        if (!bits.get(read_width, raw))
            return false;
        scale = (raw & mask) + 1u;
    }
    return true;
}

bool read_unsafe_size_plus_one(cx::Bits& bits, unsigned total_bits, std::uint64_t& value) {
    const unsigned first = total_bits < 32u ? total_bits : 32u;
    const unsigned second = total_bits > first ? total_bits - first : 0u;
    std::uint32_t low = 0;
    std::uint32_t high = 0;
    if (first && !bits.get(first, low))
        return false;
    if (second && !bits.get(second, high))
        return false;
    value = static_cast<std::uint64_t>(low) + (static_cast<std::uint64_t>(high) << 32) + 1u;
    return true;
}

bool read_subblock_icc(
    cx::Bits& bits,
    std::uint32_t subblock_count,
    std::uint32_t stream_count,
    unsigned icc_angle_bits,
    std::vector<std::vector<IccPair>>& per_subblock_icc,
    std::string& error) {
    per_subblock_icc.assign(subblock_count, {});
    if (stream_count <= 1)
        return true;

    for (std::uint32_t sub = 0; sub < subblock_count; ++sub) {
        // SubblockICC_t: span is the stream-index range for this subblock (v54 in parse_).
        const std::uint32_t span = stream_count;
        if (span < 2)
            continue;

        const unsigned pair_count_bits = span >= 3 ? index_bits(span - 2) : 0u;
        const unsigned pair_index_bits = span != 1 ? index_bits(span - 1) : 0u;

        bool has_icc = false;
        if (!read_flag(bits, has_icc)) {
            error = "AWC ICC pair flag read failed";
            return false;
        }
        if (!has_icc)
            continue;

        std::uint64_t pair_count = 1;
        if (pair_count_bits) {
            if (!read_unsafe_size_plus_one(bits, pair_count_bits, pair_count)) {
                error = "AWC ICC pair count read failed";
                return false;
            }
        }
        if (pair_count > span - 1) {
            error = "AWC ICC pair count out of range span=" + std::to_string(span) +
                    " pair_count=" + std::to_string(pair_count) + " subblocks=" +
                    std::to_string(subblock_count) + " sub=" + std::to_string(sub) + " bit=" +
                    std::to_string(bits.position());
            return false;
        }

        for (std::uint64_t pair = 0; pair < pair_count; ++pair) {
            if (bits.remaining_bits() < 2u * pair_index_bits + icc_angle_bits) {
                error = "AWC ICC pair truncated";
                return false;
            }
            IccPair icc{};
            if (!bits.get(pair_index_bits, icc.left) || !bits.get(pair_index_bits, icc.right)) {
                error = "AWC ICC pair index read failed";
                return false;
            }
            if (icc.left >= span || icc.right >= span) {
                error = "AWC ICC pair index out of range span=" + std::to_string(span) +
                    " left=" + std::to_string(icc.left) + " right=" + std::to_string(icc.right) +
                    " pair=" + std::to_string(pair) + "/" + std::to_string(pair_count) +
                    " bit=" + std::to_string(bits.position());
                return false;
            }
            std::uint32_t raw = 0;
            if (!bits.get(icc_angle_bits, raw)) {
                error = "AWC ICC angle parse failed";
                return false;
            }
            icc.angle = icc_angle_bits == 4u ? ((raw & 0xFu) + 1u) : (raw & 0x3Fu);
            per_subblock_icc[sub].push_back(icc);
        }
    }
    return true;
}

bool decode_lossless_frame_body(
    cx::Bits& bits,
    const AwcPayloadConfig& config,
    std::uint32_t stream_count,
    std::uint32_t samples_per_access_unit,
    std::vector<std::vector<std::int32_t>>& stream_samples,
    std::string& error,
    std::vector<std::uint32_t>* stream_bitdepths) {
    const std::uint32_t block_size =
        config.block_size ? config.block_size : samples_per_access_unit;
    if (!stream_count || !block_size) {
        error = "invalid AWC stream geometry";
        return false;
    }

    // parse_: v27 = audio_block.nr_samples nr_streams. AWC PDUs
    // feeding SASC factor2/factor4 carry one half/quarter of the output AU.
    if (!config.frame_divisor || samples_per_access_unit % config.frame_divisor) {
        error = "AWC per-stream frame length is not integral";
        return false;
    }
    const std::uint32_t samples_per_stream =
        samples_per_access_unit / config.frame_divisor;
    const std::uint64_t segment_size =
        (config.sample_rate / config.frame_divisor) / 50u;
    if (!segment_size) {
        error = "AWC segment_size is zero";
        return false;
    }

    const unsigned pre_bits = preamble_width(config.frame_divisor);
    std::uint32_t preamble = 0;
    if (pre_bits) {
        if (!bits.get(pre_bits, preamble)) {
            error = "AWC preamble read failed";
            return false;
        }
    }
    if (config.partition_threshold && preamble >= config.partition_threshold) {
        error = "AWC preamble exceeds partition threshold";
        return false;
    }

    std::uint32_t bitdepth_mode = 0;
    std::uint32_t bitdepth_width = 0;
    const unsigned bitdepth_param_bits = bitdepth_param_bits_from_config(config.error_scale_byte);
    if (!parse_bitdepth_mode(bits, bitdepth_param_bits, bitdepth_mode, bitdepth_width)) {
        error = "AWC bitdepth mode parse failed";
        return false;
    }

    std::uint32_t lpc_order = 0;
    if (!parse_lpc_order(bits, lpc_order)) {
        error = "AWC LPC order parse failed";
        return false;
    }
    if (block_size <= lpc_order) {
        error = "AWC block_size must exceed LPC order";
        return false;
    }

    const std::uint32_t partition_total = awc_partition_granule_total(
        block_size, config.frame_divisor, preamble);
    if (!partition_total) {
        error = "AWC partition granule total is zero";
        return false;
    }

    std::uint64_t subblock_count_raw = 0;
    if (!bits.read_vlq(3, subblock_count_raw)) {
        error = "AWC subblock VLQ read failed";
        return false;
    }
    const std::uint32_t subblock_count = static_cast<std::uint32_t>(subblock_count_raw + 1);
    if (!subblock_count) {
        error = "AWC zero subblock count";
        return false;
    }

    std::vector<SubblockState> subblocks(subblock_count);
    std::vector<std::uint32_t> partition_granules;
    if (!read_partition_granule_lengths(bits, subblock_count, partition_total, partition_granules)) {
        error = "AWC subblock partition read failed (partition_total=" + std::to_string(partition_total) +
                " subblocks=" + std::to_string(subblock_count) + " lpc_order=" + std::to_string(lpc_order) +
                " preamble=" + std::to_string(preamble) + " bitdepth_mode=" +
                std::to_string(bitdepth_mode) + " bitdepth_width=" + std::to_string(bitdepth_width) +
                " block_size=" + std::to_string(block_size) + " bit=" + std::to_string(bits.position()) +
                " remaining=" + std::to_string(bits.remaining_bits()) + ")";
        return false;
    }
    for (std::uint32_t i = 0; i < subblock_count; ++i)
        subblocks[i].granule_count = partition_granules[i];

    std::vector<std::vector<IccPair>> per_subblock_icc;
    if (!read_subblock_icc(bits, subblock_count, stream_count, 6u, per_subblock_icc, error)) {
        error = "AWC ICC subblocks=" + std::to_string(subblock_count) + ": " + error;
        return false;
    }

    std::vector<std::uint32_t> stream_error_scales;
    if (!read_stream_error_scales(
            bits,
            bitdepth_mode,
            bitdepth_width,
            config.error_scale_byte,
            stream_count,
            stream_error_scales)) {
        error = "AWC stream error_scale read failed";
        return false;
    }

    std::vector<StreamChannelState> streams(stream_count);
    std::string ldc_trace;
    for (std::uint32_t s = 0; s < stream_count; ++s) {
        streams[s].samples.assign(samples_per_stream, 0);
        streams[s].error_scale = stream_error_scales[s];
    }

    std::size_t sample_offset = 0;
    for (std::uint32_t sub = 0; sub < subblock_count; ++sub) {
        const std::uint64_t first_cap = sub == 0 ? lpc_order : segment_size;
        std::vector<std::uint64_t> ldc_granules;
        if (!subvector_granule_sizes(
                ldc_granules,
                subblocks[sub].granule_count,
                first_cap,
                segment_size)) {
            error = "AWC subvector_granule_sizes failed";
            return false;
        }
        subblocks[sub].ldc_granules = std::move(ldc_granules);

        const std::uint32_t sub_len = subblocks[sub].granule_count;

        for (std::uint32_t s = 0; s < stream_count; ++s) {
            const std::size_t stream_syntax_start = bits.position();
            bool custom_angles = false;
            if (!read_flag(bits, custom_angles)) {
                error = "AWC angle flag read failed sub=" + std::to_string(sub) +
                        " stream=" + std::to_string(s) + " bit=" +
                        std::to_string(bits.position());
                return false;
            }
            streams[s].lpc_table_indices.clear();
            if (custom_angles) {
                streams[s].lpc_table_indices.resize(lpc_order);
                for (std::uint32_t a = 0; a < lpc_order; ++a) {
                    std::uint32_t index = 0;
                    if (!parse_angle(bits, index)) {
                        error = "AWC angle read failed sub=" + std::to_string(sub) +
                                " stream=" + std::to_string(s) + " coeff=" +
                                std::to_string(a) + " bit=" + std::to_string(bits.position()) +
                                " prior={" + ldc_trace + "}";
                        return false;
                    }
                    streams[s].lpc_table_indices[a] = index & 0x3Fu;
                }
            }

            bool has_residual = false;
            if (!read_flag(bits, has_residual)) {
                error = "AWC residual flag read failed";
                return false;
            }
            std::vector<std::int32_t> errors;
            if (has_residual) {
                std::string ldc_error;
                std::string current_trace;
                if (!decode_ldc_residual_chunks(
                        bits, subblocks[sub].ldc_granules, errors, ldc_error, &current_trace)) {
                    error = "AWC LDC residual decode failed (sub=" + std::to_string(sub) +
                            " stream=" + std::to_string(s) + " bit=" + std::to_string(bits.position()) +
                            " stream_start=" + std::to_string(stream_syntax_start) +
                            " preamble=" + std::to_string(preamble) +
                            " mode=" + std::to_string(bitdepth_mode) +
                            " lpc=" + std::to_string(lpc_order) +
                            " subblocks=" + std::to_string(subblock_count) +
                            " sub_len=" + std::to_string(sub_len) +
                            " scale=" + std::to_string(streams[s].error_scale) +
                            " prior={" + ldc_trace + "} current={" + current_trace + "}): " + ldc_error;
                    return false;
                }
                if (!ldc_trace.empty())
                    ldc_trace += ';';
                ldc_trace += std::to_string(sub) + "/" + std::to_string(s) + "@" +
                             std::to_string(stream_syntax_start) + "-" +
                             std::to_string(bits.position()) + " " + current_trace;
            } else {
                errors.assign(sub_len, 0);
            }
            if (errors.size() != sub_len) {
                error = "AWC residual sample count mismatch";
                return false;
            }

            streams[s].pending_errors = std::move(errors);
            streams[s].pending_sub_len = sub_len;
        }

        for (std::uint32_t s = 0; s < stream_count; ++s) {
            const std::uint32_t pending_len = streams[s].pending_sub_len;
            const std::vector<std::int32_t>& errors = streams[s].pending_errors;

            std::vector<std::int32_t> table_coeffs(lpc_order);
            for (std::uint32_t c = 0; c < lpc_order; ++c) {
                const std::uint32_t idx =
                    streams[s].lpc_table_indices.empty()
                        ? 32u
                        : streams[s].lpc_table_indices[c];
                if (idx >= 64u) {
                    error = "AWC LPC table index out of range";
                    return false;
                }
                table_coeffs[c] = kLpcCoeffTable[idx];
            }

            std::vector<std::int32_t> reflected_coeffs;
            reflect_lpc(lpc_order, table_coeffs.data(), reflected_coeffs);
            std::reverse(reflected_coeffs.begin(), reflected_coeffs.end());
            const std::uint32_t padded_lpc_order = (lpc_order + 3u) & ~3u;
            std::vector<std::int32_t> lpc_coeffs(padded_lpc_order, 0);
            std::copy(
                reflected_coeffs.begin(),
                reflected_coeffs.end(),
                lpc_coeffs.begin() + (padded_lpc_order - lpc_order));

            std::vector<std::int32_t> sub_output(pending_len);
            lpc_lossless_decode(
                padded_lpc_order,
                lpc_coeffs.data(),
                sub_output.data(),
                errors.data(),
                errors.size(),
                streams[s].samples.data(),
                sample_offset,
                streams[s].lpc_history_initialized);
            streams[s].lpc_history_initialized =
                streams[s].lpc_history_initialized || pending_len > padded_lpc_order;

            for (std::uint32_t i = 0; i < pending_len; ++i)
                streams[s].samples[sample_offset + i] = sub_output[i];
        }

        sample_offset += sub_len;
    }

    std::size_t icc_sample_offset = 0;
    for (std::uint32_t sub = 0; sub < subblock_count; ++sub) {
        const std::uint32_t sub_len = subblocks[sub].granule_count;
        for (const IccPair& icc : per_subblock_icc[sub]) {
            if (!icc_mix(
                icc.angle,
                streams[icc.left].error_scale,
                streams[icc.right].error_scale,
                streams[icc.right].samples.data() + icc_sample_offset,
                streams[icc.left].samples.data() + icc_sample_offset,
                sub_len)) {
                error = "AWC ICC angle out of range";
                return false;
            }
        }
        icc_sample_offset += sub_len;
    }
    if (icc_sample_offset != sample_offset) {
        error = "AWC ICC sample count mismatch";
        return false;
    }

    stream_samples.clear();
    stream_samples.reserve(stream_count);
    for (std::uint32_t s = 0; s < stream_count; ++s)
        stream_samples.push_back(std::move(streams[s].samples));
    if (stream_bitdepths)
        *stream_bitdepths = std::move(stream_error_scales);

    if (sample_offset != partition_total) {
        error = "AWC partition granule count mismatch";
        return false;
    }

    (void)preamble;
    (void)samples_per_access_unit;
    return true;
}

bool parse_transparent_lpc_order(
    cx::Bits& bits,
    std::uint32_t& order,
    bool& extended) {
    extended = false;
    std::uint32_t raw = 0;
    if (!bits.get(4, raw))
        return false;
    std::uint32_t value = raw & 0xFu;
    if (!value) {
        extended = true;
        if (!bits.get(4, raw))
            return false;
        value = raw & 0xFu;
    }
    order = value + 1u;
    return order >= 1u && order <= 16u;
}

float bits_to_float(std::uint32_t bits) {
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

std::int32_t transparent_error_scale(std::uint8_t raw_byte, std::uint32_t precision_mode) {
    if (precision_mode == 2u)
        return 0;
    const std::int32_t base = kLpcErrorScaleFactor[raw_byte];
    return precision_mode == 0u ? base << 8 : base;
}

std::uint32_t default_transparent_precision_mode(std::uint8_t error_scale_byte) {
    if (error_scale_byte == 16u)
        return 1u;
    if (error_scale_byte == 24u)
        return 0u;
    return 2u;
}

bool icc_mix_transparent(
    std::uint32_t angle,
    std::int32_t* left,
    std::int32_t* right,
    std::size_t count) {
    if (angle > 31u)
        return false;
    if (!count)
        return true;
    const float c0 = bits_to_float(kPcaCoefficientPairs[2u * angle]);
    const float c1 = bits_to_float(kPcaCoefficientPairs[2u * angle + 1u]);
    const float n1 = -c1;
    for (std::size_t i = 0; i < count; ++i) {
        const float l = static_cast<float>(left[i]);
        const float r = static_cast<float>(right[i]);
        left[i] = static_cast<std::int32_t>(c0 * l + n1 * r);
        right[i] = static_cast<std::int32_t>(l * c1 + r * c0);
    }
    return true;
}

void lpc_transparent_decode(
    std::uint32_t order,
    const std::int32_t* coeffs,
    std::int32_t* output,
    const std::int32_t* errors,
    std::size_t error_count,
    const std::int32_t* history,
    std::size_t history_count,
    std::int32_t error_scale,
    bool history_initialized) {
    if (!error_count)
        return;
    const std::uint32_t o = std::min(order, 16u);
    for (std::size_t i = 0; i < error_count; ++i) {
        const auto get_sample = [&](std::size_t back) -> std::int32_t {
            if (!back)
                return 0;
            if (back <= i)
                return output[i - back];
            if (!history_initialized)
                return 0;
            const std::size_t h = back - i;
            if (h <= history_count)
                return history[history_count - h];
            return 0;
        };
        std::int64_t acc = 0;
        for (std::uint32_t k = 0; k < o; ++k)
            acc += static_cast<std::int64_t>(coeffs[k]) * get_sample(o - k);
        const bool recursive = history_initialized || i >= o;
        const std::uint64_t rounded = static_cast<std::uint64_t>(
            acc < 0 && recursive ? acc + 0x7FFFFF : acc);
        const std::int32_t prediction = recursive
            ? static_cast<std::int32_t>(static_cast<std::uint32_t>(rounded >> 23))
            : static_cast<std::int32_t>(acc / kLpcShift);
        const std::uint32_t scaled =
            static_cast<std::uint32_t>(errors[i]) * static_cast<std::uint32_t>(error_scale);
        output[i] = static_cast<std::int32_t>(
            scaled + static_cast<std::uint32_t>(prediction));
    }
}

bool decode_transparent_frame_body(
    cx::Bits& bits,
    const AwcPayloadConfig& config,
    std::uint32_t stream_count,
    std::uint32_t samples_per_access_unit,
    std::vector<std::vector<std::int32_t>>& stream_samples,
    std::string& error,
    std::vector<std::uint32_t>* stream_bitdepths) {
    const std::uint32_t block_size =
        config.block_size ? config.block_size : samples_per_access_unit;
    if (!stream_count || !block_size) {
        error = "invalid transparent AWC stream geometry";
        return false;
    }
    if (!config.frame_divisor || samples_per_access_unit % config.frame_divisor) {
        error = "transparent AWC per-stream frame length is not integral";
        return false;
    }
    const std::uint32_t samples_per_stream =
        samples_per_access_unit / config.frame_divisor;
    const std::uint64_t segment_size =
        (config.sample_rate / config.frame_divisor) / 50u;
    if (!segment_size) {
        error = "transparent AWC segment_size is zero";
        return false;
    }

    const unsigned pre_bits = preamble_width(config.frame_divisor);
    std::uint32_t preamble = 0;
    if (pre_bits) {
        if (!bits.get(pre_bits, preamble)) {
            error = "transparent AWC preamble read failed";
            return false;
        }
    }
    if (config.partition_threshold && preamble >= config.partition_threshold) {
        error = "transparent AWC preamble exceeds partition threshold";
        return false;
    }

    std::uint32_t lpc_order = 0;
    bool lpc_order_extended = false;
    if (!parse_transparent_lpc_order(bits, lpc_order, lpc_order_extended)) {
        error = "transparent AWC LPC order parse failed";
        return false;
    }
    if (block_size <= preamble) {
        error = "transparent AWC block_size must exceed preamble";
        return false;
    }

    const std::uint32_t partition_total = awc_partition_granule_total(
        block_size, config.frame_divisor, preamble);
    if (!partition_total) {
        error = "transparent AWC partition granule total is zero";
        return false;
    }

    std::uint64_t subblock_count_raw = 0;
    if (!bits.read_vlq(3, subblock_count_raw)) {
        error = "transparent AWC subblock VLQ read failed bit=" +
                std::to_string(bits.position()) + " remaining=" +
                std::to_string(bits.remaining_bits()) + " preamble=" +
                std::to_string(preamble) + " lpc=" + std::to_string(lpc_order) +
                " factor=" + std::to_string(config.frame_divisor) +
                " streams=" + std::to_string(stream_count);
        return false;
    }
    const std::uint32_t subblock_count = static_cast<std::uint32_t>(subblock_count_raw + 1u);
    if (!subblock_count) {
        error = "transparent AWC zero subblock count";
        return false;
    }

    std::vector<SubblockState> subblocks(subblock_count);
    std::vector<std::uint32_t> partition_granules;
    if (!read_partition_granule_lengths(bits, subblock_count, partition_total, partition_granules)) {
        error = "transparent AWC subblock partition read failed";
        return false;
    }
    for (std::uint32_t i = 0; i < subblock_count; ++i)
        subblocks[i].granule_count = partition_granules[i];

    std::vector<std::uint32_t> stream_error_scales;
    if (!read_stream_error_scales(
            bits, 0u, 0u, config.error_scale_byte, stream_count, stream_error_scales)) {
        error = "transparent AWC stream error_scale read failed";
        return false;
    }

    std::vector<std::vector<IccPair>> per_subblock_icc;
    if (!read_subblock_icc(bits, subblock_count, stream_count, 4u, per_subblock_icc, error))
        return false;

    std::vector<StreamChannelState> streams(stream_count);
    for (std::uint32_t s = 0; s < stream_count; ++s) {
        streams[s].samples.assign(samples_per_stream, 0);
        streams[s].error_scale = stream_error_scales[s];
    }

    std::size_t sample_offset = 0;
    std::string transparent_trace;
    for (std::uint32_t sub = 0; sub < subblock_count; ++sub) {
        const std::uint64_t first_cap = sub == 0 ? lpc_order : segment_size;
        std::vector<std::uint64_t> ldc_granules;
        if (!subvector_granule_sizes(
                ldc_granules,
                subblocks[sub].granule_count,
                first_cap,
                segment_size)) {
            error = "transparent AWC subvector_granule_sizes failed";
            return false;
        }

        const std::uint32_t sub_len = subblocks[sub].granule_count;

        for (std::uint32_t s = 0; s < stream_count; ++s) {
            const std::size_t stream_syntax_start = bits.position();
            bool custom_angles = false;
            if (!read_flag(bits, custom_angles)) {
                error = "transparent AWC angle flag read failed sub=" +
                        std::to_string(sub) + " stream=" + std::to_string(s) +
                        " bit=" + std::to_string(bits.position()) + " preamble=" +
                        std::to_string(preamble) + " lpc=" + std::to_string(lpc_order) +
                        " subblocks=" + std::to_string(subblock_count) + " sub_len=" +
                        std::to_string(sub_len) + " factor=" +
                        std::to_string(config.frame_divisor);
                return false;
            }

            std::vector<std::int32_t> table_coeffs(lpc_order);
            if (custom_angles) {
                for (std::uint32_t a = 0; a < lpc_order; ++a) {
                    std::uint32_t idx = 0;
                    if (!bits.get(6, idx)) {
                        error = "transparent AWC angle read failed sub=" +
                                std::to_string(sub) + " stream=" + std::to_string(s) +
                                " angle=" + std::to_string(a) + " bit=" +
                                std::to_string(bits.position()) + " prior={" +
                                transparent_trace + "}";
                        return false;
                    }
                    idx &= 0x3Fu;
                    if (idx >= 64u) {
                        error = "transparent AWC LPC table index out of range";
                        return false;
                    }
                    table_coeffs[a] = kLpcCoeffTable[idx];
                }
            } else {
                for (std::uint32_t a = 0; a < lpc_order; ++a)
                    table_coeffs[a] = kLpcCoeffTable[32];
            }

            bool has_residual = false;
            if (!read_flag(bits, has_residual)) {
                error = "transparent AWC residual flag read failed sub=" +
                        std::to_string(sub) + " stream=" + std::to_string(s) +
                        " bit=" + std::to_string(bits.position());
                return false;
            }

            std::vector<std::int32_t> errors;
            std::string current_ldc_trace;
            std::int32_t error_scale = 1;
            if (has_residual) {
                if (!decode_ldc_residual_chunks(
                        bits, ldc_granules, errors, error, &current_ldc_trace)) {
                    error = "transparent AWC LDC vector decode failed sub=" +
                            std::to_string(sub) + " stream=" + std::to_string(s) +
                            " bit=" + std::to_string(bits.position()) + " " + error +
                            " " + current_ldc_trace + " prior={" + transparent_trace + "}";
                    return false;
                }

                std::uint32_t precision_mode =
                    default_transparent_precision_mode(config.error_scale_byte);
                if (lpc_order_extended) {
                    bool precision_flag = false;
                    if (!read_flag(bits, precision_flag)) {
                        error = "transparent AWC error scale precision flag read failed sub=" +
                                std::to_string(sub) + " stream=" + std::to_string(s) +
                                " bit=" + std::to_string(bits.position());
                        return false;
                    }
                    precision_mode = precision_flag ? 1u : 0u;
                }

                std::uint32_t error_scale_raw = 0;
                if (!bits.get(8, error_scale_raw)) {
                    error = "transparent AWC error_scale_factor read failed sub=" +
                            std::to_string(sub) + " stream=" + std::to_string(s) +
                            " bit=" + std::to_string(bits.position());
                    return false;
                }
                error_scale = transparent_error_scale(
                    static_cast<std::uint8_t>(error_scale_raw & 0xFFu), precision_mode);
                if (!error_scale) {
                    error = "transparent AWC invalid error scale";
                    return false;
                }
            } else {
                errors.assign(sub_len, 0);
            }
            if (errors.size() != sub_len) {
                error = "transparent AWC residual sample count mismatch";
                return false;
            }

            if (!transparent_trace.empty())
                transparent_trace += ';';
            transparent_trace += std::to_string(sub) + "/" + std::to_string(s) + "@" +
                    std::to_string(stream_syntax_start) + "-" + std::to_string(bits.position()) +
                    " residual=" + std::to_string(has_residual ? 1 : 0);
            if (has_residual)
                transparent_trace += " " + current_ldc_trace;

            std::vector<std::int32_t> reflected_coeffs;
            reflect_lpc(lpc_order, table_coeffs.data(), reflected_coeffs);
            std::reverse(reflected_coeffs.begin(), reflected_coeffs.end());
            const std::uint32_t padded_lpc_order = (lpc_order + 3u) & ~3u;
            std::vector<std::int32_t> lpc_coeffs(padded_lpc_order, 0);
            std::copy(
                reflected_coeffs.begin(),
                reflected_coeffs.end(),
                lpc_coeffs.begin() + (padded_lpc_order - lpc_order));

            std::vector<std::int32_t> sub_output(sub_len);
            lpc_transparent_decode(
                padded_lpc_order,
                lpc_coeffs.data(),
                sub_output.data(),
                errors.data(),
                errors.size(),
                streams[s].samples.data(),
                sample_offset,
                error_scale,
                streams[s].lpc_history_initialized);
            streams[s].lpc_history_initialized =
                streams[s].lpc_history_initialized || sub_len > padded_lpc_order;

            for (std::uint32_t i = 0; i < sub_len; ++i)
                streams[s].samples[sample_offset + i] = sub_output[i];
        }

        sample_offset += sub_len;
    }

    std::size_t icc_sample_offset = 0;
    for (std::uint32_t sub = 0; sub < subblock_count; ++sub) {
        const std::uint32_t sub_len = subblocks[sub].granule_count;
        for (const IccPair& icc : per_subblock_icc[sub]) {
            if (icc.left >= stream_count || icc.right >= stream_count)
                continue;
            if (!icc_mix_transparent(
                icc.angle,
                streams[icc.left].samples.data() + icc_sample_offset,
                streams[icc.right].samples.data() + icc_sample_offset,
                sub_len)) {
                error = "transparent AWC ICC angle out of range";
                return false;
            }
        }
        icc_sample_offset += sub_len;
    }
    if (icc_sample_offset != sample_offset) {
        error = "transparent AWC ICC sample count mismatch";
        return false;
    }

    stream_samples.clear();
    stream_samples.reserve(stream_count);
    for (std::uint32_t s = 0; s < stream_count; ++s)
        stream_samples.push_back(std::move(streams[s].samples));
    if (stream_bitdepths)
        *stream_bitdepths = std::move(stream_error_scales);

    if (sample_offset != partition_total) {
        error = "transparent AWC partition granule count mismatch";
        return false;
    }

    return true;
}

} // namespace

bool decode_transparent_frame(
    cx::Bits& bits,
    const AwcPayloadConfig& config,
    std::uint32_t stream_count,
    std::uint32_t samples_per_access_unit,
    std::vector<std::vector<std::int32_t>>& stream_samples,
    std::string& error,
    std::vector<std::uint32_t>* stream_bitdepths) {
    if (!stream_count || !samples_per_access_unit) {
        error = "invalid transparent AWC stream geometry";
        return false;
    }
    return decode_transparent_frame_body(
        bits,
        config,
        stream_count,
        samples_per_access_unit,
        stream_samples,
        error,
        stream_bitdepths);
}

bool parse_bitdepth_mode(cx::Bits& bits, unsigned param_bits, std::uint32_t& mode, std::uint32_t& width) {
    width = 0;
    bool first = false;
    if (!read_flag(bits, first))
        return false;
    if (!first) {
        mode = 0;
        return true;
    }
    bool second = false;
    if (!read_flag(bits, second))
        return false;
    if (second) {
        mode = 2;
        return true;
    }
    if (bits.remaining_bits() < param_bits)
        return false;
    std::uint32_t raw = 0;
    if (!bits.get(param_bits, raw))
        return false;
    const std::uint32_t mask = param_bits >= 32 ? 0xFFFFFFFFu : ((1u << param_bits) - 1u);
    mode = 1;
    width = (raw & mask) + 1;
    return true;
}

bool parse_lpc_order(cx::Bits& bits, std::uint32_t& order) {
    std::uint32_t raw = 0;
    if (!bits.get(4, raw))
        return false;
    order = (raw & 0xFu) + 1;
    return order >= 1 && order <= 16;
}

bool parse_angle(cx::Bits& bits, std::uint32_t& angle) {
    if (bits.remaining_bits() < 6)
        return false;
    std::uint32_t raw = 0;
    if (!bits.get(6, raw))
        return false;
    angle = raw & 0x3Fu;
    return true;
}

void lpc_lossless_decode(
    std::uint32_t order,
    const std::int32_t* coeffs,
    std::int32_t* output,
    const std::int32_t* errors,
    std::size_t error_count,
    const std::int32_t* history,
    std::size_t history_count,
    bool history_initialized) {
    if (!error_count)
        return;
    const std::uint32_t o = std::min(order, 16u);
    for (std::size_t i = 0; i < error_count; ++i) {
        const auto get_sample = [&](std::size_t back) -> std::int32_t {
            if (!back)
                return 0;
            if (back <= i)
                return output[i - back];
            if (!history_initialized)
                return 0;
            const std::size_t h = back - i;
            if (h <= history_count)
                return history[history_count - h];
            return 0;
        };
        std::int64_t acc = 0;
        for (std::uint32_t k = 0; k < o; ++k)
            acc += static_cast<std::int64_t>(coeffs[k]) * get_sample(o - k);
        const bool recursive = history_initialized || i >= o;
        const std::uint64_t rounded = static_cast<std::uint64_t>(
            acc < 0 && recursive ? acc + 0x7FFFFF : acc);
        const std::int32_t prediction = recursive
            ? static_cast<std::int32_t>(static_cast<std::uint32_t>(rounded >> 23))
            : static_cast<std::int32_t>(acc / kLpcShift);
        output[i] = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(errors[i]) +
            static_cast<std::uint32_t>(prediction));
    }
}

bool decode_lossless_frame(
    cx::Bits& bits,
    const AwcPayloadConfig& config,
    std::uint32_t stream_count,
    std::uint32_t samples_per_access_unit,
    std::vector<std::vector<std::int32_t>>& stream_samples,
    std::string& error,
    std::vector<std::uint32_t>* stream_bitdepths) {
    if (!stream_count || !samples_per_access_unit) {
        error = "invalid AWC stream geometry";
        return false;
    }

    return decode_lossless_frame_body(
        bits,
        config,
        stream_count,
        samples_per_access_unit,
        stream_samples,
        error,
        stream_bitdepths);
}

} // namespace awc
} // namespace auro3d
