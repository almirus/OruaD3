#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace auro3d::encode {

/// Location of one codec-v3 metadata bit in a PCM24 carrier block. This is the
/// inverse of the decoder's MuxIteratorTrue reader.
struct PcmMetadataBitLocation {
    std::size_t sample_offset = 0;
    std::uint32_t bit_mask = 0;
};

bool locate_pcm_metadata_bit_true(
    std::uint64_t bit_position,
    std::uint32_t mux_m,
    std::size_t block_samples,
    PcmMetadataBitLocation& out);

bool locate_pcm_metadata_bit_false(
    std::uint64_t bit_position,
    std::uint32_t mux_m,
    /// Native Projector mask-cycle length, `16 * mux_m - 1`.
    std::uint32_t mux_param,
    std::size_t block_samples,
    PcmMetadataBitLocation& out);

bool write_pcm_metadata_bit_true(
    std::vector<std::int32_t>& carrier_samples,
    std::size_t block_start,
    std::uint64_t bit_position,
    std::uint32_t mux_m,
    bool value);

/// Clears every physical metadata slot addressed by the codec-v3 true mux.
/// This includes reserved/sync-gap slots, which the scanner requires to be
/// zero. It never touches PCM bits outside the selected mux plane.
bool clear_pcm_metadata_storage(
    std::vector<std::int32_t>& carrier_samples,
    std::size_t block_start,
    std::size_t block_samples,
    std::uint32_t mux_m);

/// Inverse of the decoder's parse_sync_pcm24_at header extraction. It writes
/// only codec-v3 metadata bits 0..2 in the first sixteen carrier samples and
/// the four-bit mux parameter field at positions 44..47.
bool write_pcm_metadata_sync_header(
    std::vector<std::int32_t>& carrier_samples,
    std::size_t block_start,
    std::size_t block_samples,
    std::uint32_t mux_m);

/// Counterpart of the decoder's MuxIteratorFalse. It begins at projector
/// position 32, immediately after the sixteen sync and sixteen CRC masks.
/// The caller writes or skips the fixed 16-bit A3D header before the Channel
/// stream, using the same MSB-first order as read_unsigned_false.
class PcmMetadataFalseWriter {
public:
    bool open(
        std::vector<std::int32_t>& carrier_samples,
        std::size_t block_start,
        std::size_t block_samples,
        std::uint32_t mux_m);
    bool write_unsigned(std::uint32_t value, std::uint32_t bit_count);
    bool skip_bits(std::uint64_t bit_count);
    bool write_bool(bool value) { return write_unsigned(value ? 1u : 0u, 1u); }
    std::uint64_t bits_remaining() const { return bits_remaining_; }
    bool valid() const { return samples_ != nullptr; }

private:
    std::vector<std::int32_t>* samples_ = nullptr;
    std::size_t block_start_ = 0;
    std::size_t block_samples_ = 0;
    std::uint32_t mux_m_ = 0;
    std::uint64_t position_ = 0;
    std::uint64_t bits_remaining_ = 0;
};

/// Native scanner CRC over a metadata PCM block, including its special masking
/// of bit 1 in the first 16 low PCM bytes.
bool compute_pcm_metadata_crc16(
    const std::vector<std::int32_t>& carrier_samples,
    std::size_t block_start,
    std::size_t block_samples,
    std::uint16_t& crc);

/// Native expected CRC relation value reconstructed from the first sixteen
/// PCM24 samples. Exposed for deterministic post-write validation.
bool compute_pcm_metadata_v25(
    const std::vector<std::int32_t>& carrier_samples,
    std::size_t block_start,
    std::uint16_t& value);

/// Stores a CRC closure in otherwise unused sync-header slots. The target is
/// the exact equality checked by extract_metadata_from_a3d_block_at:
/// compute_pcm_metadata_crc16(block) == compute_v25_from_first16_at(block).
/// Header fields and payload bits are preserved.
bool seal_pcm_metadata_crc16(
    std::vector<std::int32_t>& carrier_samples,
    std::size_t block_start,
    std::size_t block_samples);

} // namespace auro3d:encode
