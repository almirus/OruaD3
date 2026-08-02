#include "metadata_block.hpp"

namespace auro3d::encode {

bool write_pcm_metadata_block(
    std::vector<std::int32_t>& carrier_samples,
    std::size_t block_start,
    std::size_t block_samples,
    const PcmMetadataBlock& metadata) {
    if (metadata.mux_m < 3u || metadata.mux_m > 14u ||
        metadata.adol_blocks.size() != metadata.prefix.adol_block_count ||
        block_start > carrier_samples.size() ||
        block_samples > carrier_samples.size() - block_start) {
        return false;
    }
    for (std::size_t index = 0; index < block_samples; ++index) {
        const std::uint32_t raw = static_cast<std::uint32_t>(carrier_samples[block_start + index]);
        if ((raw >> 24u) != 0u && (raw >> 24u) != 0xFFu)
            return false;
    }
    if (
        !clear_pcm_metadata_storage(carrier_samples, block_start, block_samples, metadata.mux_m) ||
        !write_pcm_metadata_sync_header(carrier_samples, block_start, block_samples, metadata.mux_m)) {
        return false;
    }

    PcmMetadataFalseWriter writer;
    if (!writer.open(carrier_samples, block_start, block_samples, metadata.mux_m)
        || !writer.skip_bits(16u)
        || !write_pcm_metadata_prefix(writer, metadata.prefix)) {
        return false;
    }
    for (const std::vector<AdolInstruction>& adol_block : metadata.adol_blocks) {
        if (!write_adol_block(writer, adol_block))
            return false;
    }
    return seal_pcm_metadata_crc16(carrier_samples, block_start, block_samples);
}

bool validate_pcm_metadata_block(
    const std::vector<std::int32_t>& carrier_samples,
    std::size_t block_start,
    std::size_t block_samples,
    std::uint32_t mux_m) {
    if (mux_m < 3u || mux_m > 14u || block_samples < 16u
        || block_start > carrier_samples.size()
        || block_samples > carrier_samples.size() - block_start) {
        return false;
    }
    std::uint16_t crc = 0;
    std::uint16_t expected = 0;
    return compute_pcm_metadata_crc16(
               carrier_samples, block_start, block_samples, crc)
        && compute_pcm_metadata_v25(carrier_samples, block_start, expected)
        && crc == expected;
}

} // namespace auro3d::encode
