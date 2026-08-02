#include "pcm_metadata.hpp"

#include "layout.hpp"

#include <algorithm>
#include <limits>

namespace auro3d::encode {
namespace {

std::uint16_t crc16_ccitt_update(std::uint16_t crc, std::uint8_t byte) {
    crc = static_cast<std::uint16_t>(crc ^ (static_cast<std::uint16_t>(byte) << 8u));
    for (std::uint32_t bit = 0; bit != 8u; ++bit) {
        crc = (crc & 0x8000u) != 0u
            ? static_cast<std::uint16_t>((crc << 1u) ^ 0x1021u)
            : static_cast<std::uint16_t>(crc << 1u);
    }
    return crc;
}

std::uint8_t pcm24_byte(std::int32_t sample, std::uint32_t byte_index) {
    return static_cast<std::uint8_t>((static_cast<std::uint32_t>(sample) >> (8u * byte_index)) & 0xFFu);
}

std::uint16_t compute_v25_from_first16(
    const std::vector<std::int32_t>& samples,
    std::size_t start) {
    std::uint16_t words[32]{};
    for (std::uint32_t index = 0; index != 16u; ++index) {
        const std::uint32_t sample = static_cast<std::uint32_t>(samples[start + index]);
        words[2u * index] = static_cast<std::uint16_t>(sample & 0xFFFFu);
        words[2u * index + 1u] = static_cast<std::uint16_t>(sample >> 16u);
    }
    const std::uint32_t v22 = (((2u * words[0]) & 0xFFFFFFFCu) | (words[2] & 3u)) >> 1u;
    const auto bit_4 = [&words](std::uint32_t index) {
        return (2u * (words[index] & 0xFFu)) & 4u;
    };
    std::uint32_t a = (((2u * words[4]) & 4u) | (8u * (v22 & 3u)) | (words[6] & 3u)) >> 1u;
    a &= 0x0Fu;
    const std::uint32_t b = (((2u * words[8]) & 4u) | (8u * a) | (words[10] & 3u)) >> 1u;
    const std::uint32_t c = (bit_4(12u) | (8u * b) | (words[14] & 3u)) >> 1u;
    const std::uint32_t d = (bit_4(16u) | (8u * c) | (words[18] & 3u)) >> 1u;
    const std::uint32_t e = (bit_4(20u) | (8u * d) | (words[22] & 3u)) >> 1u;
    const std::uint32_t f = bit_4(24u) | (8u * e) | (words[26] & 3u);
    return static_cast<std::uint16_t>(((2u * f) & 0xFFFFFFFCu) | (words[28] & 2u) | ((words[30] >> 1u) & 1u));
}

std::uint16_t crc_closure_residual(
    const std::vector<std::int32_t>& samples,
    std::size_t block_start,
    std::size_t block_samples) {
    std::uint16_t crc = 0;
    if (!compute_pcm_metadata_crc16(samples, block_start, block_samples, crc))
        return 0;
    return static_cast<std::uint16_t>(crc ^ compute_v25_from_first16(samples, block_start));
}

void set_low_bit(std::vector<std::int32_t>& samples, std::size_t index, std::uint32_t mask, bool value) {
    std::uint32_t sample = static_cast<std::uint32_t>(samples[index]);
    sample = value ? (sample | mask) : (sample & ~mask);
    samples[index] = static_cast<std::int32_t>(sample);
}

} // namespace

bool locate_pcm_metadata_bit_true(
    std::uint64_t bit_position,
    std::uint32_t mux_m,
    std::size_t block_samples,
    PcmMetadataBitLocation& out) {
    out = {};
    if (mux_m < 3u || mux_m > 24u)
        return false;

    std::uint64_t sample_offset = 0;
    std::uint64_t remainder = 0;
    std::uint32_t mask = 0;
    if (bit_position > 0x2Full) {
        if (bit_position >= 16ull * mux_m) {
            sample_offset = bit_position / mux_m;
            remainder = bit_position % mux_m;
        } else {
            const std::uint64_t adjusted = bit_position - 48ull;
            sample_offset = adjusted / (mux_m - 3u);
            remainder = adjusted % (mux_m - 3u);
        }
        mask = 1u << static_cast<std::uint32_t>(mux_m - 1u - remainder);
    } else if (bit_position >= 0x10ull) {
        if (bit_position <= 0x1Full) {
            sample_offset = bit_position - 16ull;
            mask = 2u;
        } else {
            sample_offset = bit_position - 32ull;
            mask = 4u;
        }
    } else {
        sample_offset = bit_position;
        mask = 1u;
    }
    if (sample_offset >= block_samples || sample_offset > std::numeric_limits<std::size_t>::max())
        return false;
    out.sample_offset = static_cast<std::size_t>(sample_offset);
    out.bit_mask = mask;
    return true;
}

bool locate_pcm_metadata_bit_false(
    std::uint64_t bit_position,
    std::uint32_t mux_m,
    std::uint32_t mux_param,
    std::size_t block_samples,
    PcmMetadataBitLocation& out) {
    out = {};
    if (mux_m < 3u || mux_m > 24u
        || mux_param != 16u * mux_m - 1u
        || bit_position < mux_m) {
        return false;
    }

    std::uint64_t sample_offset = 0;
    std::uint64_t remainder = 0;
    std::uint32_t mask = 0;
    if (bit_position > 0x2Full) {
        if (bit_position >= 16ull * mux_m) {
            const std::uint64_t quotient = (bit_position - mux_m) / mux_param;
            if (quotient > std::numeric_limits<std::uint64_t>::max() - bit_position)
                return false;
            const std::uint64_t adjusted = quotient + bit_position;
            sample_offset = adjusted / mux_m;
            remainder = adjusted % mux_m;
        } else {
            const std::uint64_t adjusted = bit_position - 48ull;
            sample_offset = adjusted / (mux_m - 3u);
            remainder = adjusted % (mux_m - 3u);
        }
        mask = 1u << static_cast<std::uint32_t>(mux_m - 1u - remainder);
    } else if (bit_position >= 0x10ull) {
        if (bit_position <= 0x1Full) {
            sample_offset = bit_position - 16ull;
            mask = 2u;
        } else {
            sample_offset = bit_position - 32ull;
            mask = 4u;
        }
    } else {
        sample_offset = bit_position;
        mask = 1u;
    }
    if (sample_offset >= block_samples || sample_offset > std::numeric_limits<std::size_t>::max())
        return false;
    out.sample_offset = static_cast<std::size_t>(sample_offset);
    out.bit_mask = mask;
    return true;
}

bool write_pcm_metadata_bit_true(
    std::vector<std::int32_t>& carrier_samples,
    std::size_t block_start,
    std::uint64_t bit_position,
    std::uint32_t mux_m,
    bool value) {
    if (block_start > carrier_samples.size())
        return false;
    PcmMetadataBitLocation location{};
    if (!locate_pcm_metadata_bit_true(
            bit_position, mux_m, carrier_samples.size() - block_start, location)) {
        return false;
    }
    std::uint32_t sample = static_cast<std::uint32_t>(carrier_samples[block_start + location.sample_offset]);
    sample = value ? (sample | location.bit_mask) : (sample & ~location.bit_mask);
    carrier_samples[block_start + location.sample_offset] = static_cast<std::int32_t>(sample);
    return true;
}

bool clear_pcm_metadata_storage(
    std::vector<std::int32_t>& carrier_samples,
    std::size_t block_start,
    std::size_t block_samples,
    std::uint32_t mux_m) {
    if (block_start > carrier_samples.size() ||
        block_samples > carrier_samples.size() - block_start
        || mux_m < 3u || mux_m > 24u) {
        return false;
    }
    if (static_cast<std::uint64_t>(block_samples)
        > std::numeric_limits<std::uint64_t>::max() / mux_m) {
        return false;
    }
    const std::uint64_t positions = static_cast<std::uint64_t>(block_samples) * mux_m;
    for (std::uint64_t position = 0; position < positions; ++position) {
        if (!write_pcm_metadata_bit_true(
                carrier_samples, block_start, position, mux_m, false)) {
            return false;
        }
    }
    return true;
}

bool write_pcm_metadata_sync_header(
    std::vector<std::int32_t>& carrier_samples,
    std::size_t block_start,
    std::size_t block_samples,
    std::uint32_t mux_m) {
    if (block_start > carrier_samples.size()
        || block_samples > carrier_samples.size() - block_start
        || block_samples < 16u
        || block_samples > std::numeric_limits<std::uint32_t>::max()
        || !codec_v3_unit_block_size_supported(static_cast<std::uint32_t>(block_samples))
        || mux_m < 3u
        || mux_m > 14u) {
        return false;
    }
    const std::uint32_t raw_block = block_samples == 1000u
        ? 0x3D0u
        : static_cast<std::uint32_t>(block_samples - 16u);
    // parse_sync_pcm24_at can only reconstruct a multiple of 16 from this
    // header (with 1000 represented by its native special word).
    if ((raw_block & 0xFu) != 0u || raw_block > 0xFF0u)
        return false;

    const std::uint32_t c = raw_block >> 5u;
    const std::uint32_t b = (c & 0x78u) >> 2u;
    const std::uint32_t a = (b & 0xF8u) >> 2u;
    auto set_low_metadata_bits = [&carrier_samples, block_start](std::size_t index, std::uint32_t mask, bool value) {
        std::uint32_t sample = static_cast<std::uint32_t>(carrier_samples[block_start + index]);
        sample = value ? (sample | mask) : (sample & ~mask);
        carrier_samples[block_start + index] = static_cast<std::int32_t>(sample);
    };

    // The parser requires the sync sentinel (LSB=1) in all first 16 samples.
    // Other metadata bits in this header begin clear before fields are applied.
    for (std::size_t index = 0; index < 16u; ++index) {
        std::uint32_t sample = static_cast<std::uint32_t>(carrier_samples[block_start + index]);
        sample = (sample & ~7u) | 1u;
        carrier_samples[block_start + index] = static_cast<std::int32_t>(sample);
    }
    set_low_metadata_bits(0u, 4u, (a & 4u) != 0u);
    set_low_metadata_bits(1u, 2u, (a & 1u) != 0u);
    set_low_metadata_bits(1u, 4u, (a & 2u) != 0u);
    set_low_metadata_bits(2u, 4u, (b & 4u) != 0u);
    set_low_metadata_bits(3u, 2u, (b & 1u) != 0u);
    set_low_metadata_bits(3u, 4u, (b & 2u) != 0u);
    set_low_metadata_bits(4u, 4u, (c & 4u) != 0u);
    set_low_metadata_bits(5u, 4u, (c & 2u) != 0u);
    set_low_metadata_bits(6u, 4u, (c & 1u) != 0u);
    set_low_metadata_bits(7u, 4u, (raw_block & 0x10u) != 0u);

    // parse_sync_pcm24_at reads this four-bit field MSB-first through the
    // true mux iterator initialized at bit position 44 with m=3.
    const std::uint32_t mux_code = 14u - mux_m;
    for (std::uint32_t bit = 0; bit < 4u; ++bit) {
        if (!write_pcm_metadata_bit_true(
                carrier_samples, block_start, 44u + bit, 3u,
                ((mux_code >> (3u - bit)) & 1u) != 0u)) {
            return false;
        }
    }
    return true;
}

bool PcmMetadataFalseWriter::open(
    std::vector<std::int32_t>& carrier_samples,
    std::size_t block_start,
    std::size_t block_samples,
    std::uint32_t mux_m) {
    samples_ = nullptr;
    block_start_ = 0;
    block_samples_ = 0;
    mux_m_ = 0;
    position_ = 0;
    bits_remaining_ = 0;
    if (block_start > carrier_samples.size()
        || block_samples > carrier_samples.size() - block_start
        || block_samples < 16u
        || block_samples > std::numeric_limits<std::uint32_t>::max()
        || !codec_v3_unit_block_size_supported(static_cast<std::uint32_t>(block_samples))
        || mux_m < 3u || mux_m > 14u) {
        return false;
    }
    const std::uint64_t sample_count = static_cast<std::uint64_t>(block_samples);
    if (sample_count > std::numeric_limits<std::uint64_t>::max() / mux_m)
        return false;
    const std::uint64_t total_bits = sample_count * mux_m - ((sample_count - 1u) / 16u);
    if (total_bits <= 32u)
        return false;
    samples_ = &carrier_samples;
    block_start_ = block_start;
    block_samples_ = block_samples;
    mux_m_ = mux_m;
    // a3d::serialize<..., Projector<N>> advances past sixteen sync masks and
    // sixteen CRC masks before serializing anything. The fixed sixteen-bit
    // A3D header therefore occupies positions 32..47 for every N, and the
    // Channel stream begins at position 48.
    position_ = 32u;
    bits_remaining_ = total_bits - 32u;
    return true;
}

bool PcmMetadataFalseWriter::write_unsigned(std::uint32_t value, std::uint32_t bit_count) {
    if (!valid() || bit_count > 32u || bits_remaining_ < bit_count)
        return false;
    if (bit_count != 32u && (value >> bit_count) != 0u)
        return false;
    for (std::uint32_t bit = bit_count; bit != 0u; --bit) {
        PcmMetadataBitLocation location{};
        const std::uint32_t projector_cycle = 16u * mux_m_ - 1u;
        if (!locate_pcm_metadata_bit_false(
                position_,
                mux_m_,
                projector_cycle,
                block_samples_,
                location)) {
            return false;
        }
        std::uint32_t sample = static_cast<std::uint32_t>((*samples_)[block_start_ + location.sample_offset]);
        const bool set = ((value >> (bit - 1u)) & 1u) != 0u;
        sample = set ? (sample | location.bit_mask) : (sample & ~location.bit_mask);
        (*samples_)[block_start_ + location.sample_offset] = static_cast<std::int32_t>(sample);
        ++position_;
        --bits_remaining_;
    }
    return true;
}

bool PcmMetadataFalseWriter::skip_bits(std::uint64_t bit_count) {
    if (!valid() || bit_count > bits_remaining_
        || position_ > std::numeric_limits<std::uint64_t>::max() - bit_count) {
        return false;
    }
    position_ += bit_count;
    bits_remaining_ -= bit_count;
    return true;
}

bool compute_pcm_metadata_crc16(
    const std::vector<std::int32_t>& carrier_samples,
    std::size_t block_start,
    std::size_t block_samples,
    std::uint16_t& crc) {
    crc = 0;
    if (block_start > carrier_samples.size() || block_samples > carrier_samples.size() - block_start)
        return false;
    const std::size_t first = std::min<std::size_t>(block_samples, 16u);
    for (std::size_t index = 0; index < first; ++index) {
        const std::int32_t sample = carrier_samples[block_start + index];
        crc = crc16_ccitt_update(crc, static_cast<std::uint8_t>(pcm24_byte(sample, 0u) & 0xFDu));
        crc = crc16_ccitt_update(crc, pcm24_byte(sample, 1u));
        crc = crc16_ccitt_update(crc, pcm24_byte(sample, 2u));
    }
    for (std::size_t index = first; index < block_samples; ++index) {
        const std::int32_t sample = carrier_samples[block_start + index];
        crc = crc16_ccitt_update(crc, pcm24_byte(sample, 0u));
        crc = crc16_ccitt_update(crc, pcm24_byte(sample, 1u));
        crc = crc16_ccitt_update(crc, pcm24_byte(sample, 2u));
    }
    crc = static_cast<std::uint16_t>(~crc);
    return true;
}

bool compute_pcm_metadata_v25(
    const std::vector<std::int32_t>& carrier_samples,
    std::size_t block_start,
    std::uint16_t& value) {
    value = 0;
    if (block_start > carrier_samples.size()
        || carrier_samples.size() - block_start < 16u) {
        return false;
    }
    value = compute_v25_from_first16(carrier_samples, block_start);
    return true;
}

bool seal_pcm_metadata_crc16(
    std::vector<std::int32_t>& carrier_samples,
    std::size_t block_start,
    std::size_t block_samples) {
    if (block_start > carrier_samples.size() || block_samples < 16u ||
        block_samples > carrier_samples.size() - block_start) {
        return false;
    }

    // These are header bits that parse_sync_pcm24_at does not consume. Bit 1
    // is also masked out of the scanner CRC, so the full-rank closure comes
    // from the decoder's own v25 projection rather than payload mutation.
    struct Slot { std::uint8_t sample; std::uint8_t mask; };
    constexpr Slot slots[] = {
        {0u, 2u}, {2u, 2u}, {4u, 2u}, {5u, 2u}, {6u, 2u}, {7u, 2u},
        {8u, 2u}, {9u, 2u}, {10u, 2u}, {11u, 2u}, {12u, 2u}, {13u, 2u},
        {14u, 2u}, {15u, 2u}, {8u, 4u}, {9u, 4u}, {10u, 4u}, {11u, 4u},
    };
    static_assert(sizeof(slots) / sizeof(slots[0]) >= 16u, "CRC closure needs sixteen slots");

    for (const Slot& slot : slots)
        set_low_bit(carrier_samples, block_start + slot.sample, slot.mask, false);
    const std::uint16_t baseline = crc_closure_residual(carrier_samples, block_start, block_samples);

    std::uint16_t basis[16]{};
    std::uint32_t basis_coefficients[16]{};
    for (std::uint32_t slot_index = 0; slot_index < sizeof(slots) / sizeof(slots[0]); ++slot_index) {
        const Slot& slot = slots[slot_index];
        set_low_bit(carrier_samples, block_start + slot.sample, slot.mask, true);
        std::uint16_t vector = static_cast<std::uint16_t>(
            baseline ^ crc_closure_residual(carrier_samples, block_start, block_samples));
        set_low_bit(carrier_samples, block_start + slot.sample, slot.mask, false);
        std::uint32_t coefficients = std::uint32_t{1} << slot_index;
        for (std::uint32_t pivot = 0; pivot < 16u; ++pivot) {
            if ((vector & (std::uint16_t{1} << pivot)) == 0u)
                continue;
            if (basis[pivot] != 0u) {
                vector = static_cast<std::uint16_t>(vector ^ basis[pivot]);
                coefficients ^= basis_coefficients[pivot];
                continue;
            }
            basis[pivot] = vector;
            basis_coefficients[pivot] = coefficients;
            break;
        }
    }
    for (std::uint32_t bit = 0; bit < 16u; ++bit) {
        if (basis[bit] == 0u)
            return false;
    }

    std::uint16_t residual = baseline;
    std::uint32_t solution = 0;
    for (std::uint32_t pivot = 0; pivot < 16u; ++pivot) {
        if ((residual & (std::uint16_t{1} << pivot)) == 0u)
            continue;
        residual = static_cast<std::uint16_t>(residual ^ basis[pivot]);
        solution ^= basis_coefficients[pivot];
    }
    for (std::uint32_t slot_index = 0; slot_index < sizeof(slots) / sizeof(slots[0]); ++slot_index) {
        if ((solution & (std::uint32_t{1} << slot_index)) != 0u) {
            const Slot& slot = slots[slot_index];
            set_low_bit(carrier_samples, block_start + slot.sample, slot.mask, true);
        }
    }
    return residual == 0u &&
        crc_closure_residual(carrier_samples, block_start, block_samples) == 0u;
}

} // namespace auro3d::encode
