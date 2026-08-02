#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <string>
#include <vector>

namespace auro3d::encode {

struct PcmMuxProjectorRecord {
    std::uint32_t selector = 0;
    std::array<std::uint32_t, 3> values{};
    std::uint32_t value_count = 0;
};

/// The native Projector stores one mask word per source bitline. Its first
/// sixteen-sample prefix uses three fixed sixteen-word runs followed by the
/// cyclic high-bitline run; later samples have N words, except every
/// sixteenth output sample, where the final bitline is reserved for the frame
/// CRC.
bool make_pcm_mux_mask_plan(
    std::uint32_t source_count,
    std::uint32_t bitline_count,
    std::vector<std::uint32_t>& masks,
    std::string& error);

/// Applies the native static projector-iterator selector layout to the first
/// sixteen samples. Entries 16, 20, 24 and 28 are signed native dwords; values
/// >=31 select the native all-ones fallback. Dynamic records are serialized by
/// `serialize_pcm_mux_projector_record` with an explicit cursor.
bool serialize_pcm_mux_projector_selectors(
    std::uint32_t source_count,
    std::uint32_t bitline_count,
    const std::array<std::uint32_t, 36>& selectors,
    std::vector<std::uint32_t>& masks,
    std::string& error);

/// Serializes one native dynamic projector iterator record from
/// `a3d::serialize<projector::Iterator<unsigned int>>` at 0x520D50. The
/// selector byte is followed by the scalar payload width selected by the
/// native switch; selectors 64 and 100 carry two and three 8-bit values.
bool serialize_pcm_mux_projector_record(
    std::uint32_t selector,
    const std::array<std::uint32_t, 3>& values,
    std::uint32_t value_count,
    std::vector<std::uint32_t>& masks,
    std::size_t& cursor,
    std::string& error);

/// Builds the native mask plan and serializes a complete dynamic projector
/// record stream. The caller supplies records in native iterator order; a
/// zero-selector terminator is appended when requested. Exact mask
/// consumption is required, so truncated or overlong streams fail closed.
bool serialize_pcm_mux_projector_records(
    std::uint32_t source_count,
    std::uint32_t bitline_count,
    const std::vector<PcmMuxProjectorRecord>& records,
    bool append_terminator,
    std::vector<std::uint32_t>& masks,
    std::string& error);

/// Applies Projector::project to a pre-serialized PCM range. `words` contains
/// one word per source sample; `masks` is the larger projector-iterator result
/// consumed by the native cursor.
bool apply_pcm_mux_projection(
    std::uint32_t bitline_count,
    std::vector<std::uint32_t>& words,
    const std::vector<std::uint32_t>& masks,
    std::string& error);

/// Applies Projector::project_crc, routing mask words 16..31 into the first
/// sixteen output samples after data projection.
bool apply_pcm_mux_crc_projection(
    std::vector<std::uint32_t>& words,
    const std::vector<std::uint32_t>& masks,
    std::string& error);

/// Native mux CRC16 at 0x51F390. Unlike channel CRC, the stored value is the
/// complemented register in native byte order (no final byte swap).
bool compute_pcm_mux_crc16(
    const std::vector<std::uint32_t>& words,
    std::uint16_t& crc,
    std::string& error);

/// Complete dynamic mux path used by compose::Channel::mux: serialize the
/// projector iterator, project data bits, compute the native pre-closure CRC,
/// then project the CRC prefix. `crc` is the checksum used to derive that
/// prefix; it is intentionally captured before the prefix bits are applied.
bool apply_pcm_mux_records(
    std::uint32_t bitline_count,
    std::vector<std::uint32_t>& words,
    const std::vector<PcmMuxProjectorRecord>& records,
    bool append_terminator,
    std::uint16_t& crc,
    std::string& error);

} // namespace auro3d::encode
