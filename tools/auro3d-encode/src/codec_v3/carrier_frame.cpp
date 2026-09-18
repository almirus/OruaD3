#include "carrier_frame.hpp"
#include "pcm_mux.hpp"

#include <algorithm>

namespace auro3d::encode {

bool assemble_carrier_unit(
    std::uint32_t carrier_layout,
    std::uint32_t sample_rate,
    std::uint32_t frame_count,
    std::uint32_t metadata_channel_id,
    const std::vector<EncodedChannelFrame>& channels,
    const PcmMetadataBlock& metadata,
    EncodedCarrierUnit& out,
    std::string& error) {
    error.clear();
    out = {};
    if (metadata_channel_id >= kCodecV3ChannelCount
        || (carrier_layout & (std::uint32_t{1} << metadata_channel_id)) == 0u) {
        error = "metadata channel is absent from the carrier layout";
        return false;
    }
    if (!make_carrier_unit(carrier_layout, sample_rate, frame_count, out.carrier, error))
        return false;
    std::vector<bool> seen(kCodecV3ChannelCount, false);
    for (const EncodedChannelFrame& channel : channels) {
        if (channel.channel_id >= kCodecV3ChannelCount
            || (carrier_layout & (std::uint32_t{1} << channel.channel_id)) == 0u
            || seen[channel.channel_id]
            || channel.word_count != channel.words.size()
            || channel.word_count > frame_count) {
            error = "encoded channel does not match the carrier layout or unit size";
            return false;
        }
        std::uint64_t payload_capacity = 0u;
        if (!codec_v3_channel_payload_capacity(
                channel.quantization_shift,
                frame_count,
                payload_capacity)
            || channel.serialized_bits == 0u
            || channel.serialized_bits > payload_capacity) {
            error = "encoded channel has invalid serialized bit accounting";
            return false;
        }
        if ((channel.metadata_words[0] & 0xF0000000u) != 0u) {
            error = "encoded channel uses unsupported dynamic metadata flags";
            return false;
        }
        ChannelMetadataCombined combined{};
        if (!combine_channel_metadata(
                channel.channel_header, channel.metadata_words, combined)
            || combined.mode == 0u
            || combined.mode > 3u) {
            error = "encoded channel metadata does not describe a native parser mode";
            return false;
        }
        for (std::uint32_t source = 0; source < combined.mode; ++source) {
            if (combined.channel_ids[source] >= 31u) {
                error = "encoded channel metadata has an inactive source in its mode";
                return false;
            }
        }
        std::vector<std::int32_t> padded(frame_count, 0);
        std::copy(channel.words.begin(), channel.words.end(), padded.begin());
        for (const std::int32_t word : padded) {
            const std::uint32_t raw = static_cast<std::uint32_t>(word);
            const std::uint32_t high = raw >> 24u;
            if (high != 0u && high != 0xFFu) {
                error = "encoded channel word exceeds signed PCM24 range";
                return false;
            }
        }
        Crc16 crc;
        for (const std::int32_t word : padded) {
            const std::uint32_t value = static_cast<std::uint32_t>(word);
            crc.process_words(&value, 1u);
        }
        if (crc.stored_word() != channel.crc_word) {
            error = "encoded channel CRC does not match its payload words";
            return false;
        }
        if (!validate_pcm_metadata_block(
                padded, 0u, frame_count, channel.quantization_shift)) {
            error = "encoded channel does not have a valid mux sync/CRC";
            return false;
        }
        seen[channel.channel_id] = true;
        std::vector<std::int32_t>& destination = out.carrier.planes[channel.channel_id];
        std::copy(padded.begin(), padded.end(), destination.begin());
    }
    for (std::uint32_t id = 0; id < kCodecV3ChannelCount; ++id) {
        if ((carrier_layout & (std::uint32_t{1} << id)) != 0u && !seen[id]) {
            error = "carrier channel has no serialized payload";
            return false;
        }
    }
    out.metadata_channel_id = metadata_channel_id;
    out.metadata = metadata;
    out.channels = channels;
    // Channel:mux already projected and closed each complete A3D stream.
    // Keep the returned diagnostic frames synchronized with those final
    // carrier spans without applying a second metadata writer.
    for (EncodedChannelFrame& channel : out.channels) {
        channel.words = out.carrier.planes[channel.channel_id];
        channel.word_count = frame_count;
        Crc16 crc;
        for (const std::int32_t word : channel.words) {
            const std::uint32_t raw = static_cast<std::uint32_t>(word);
            crc.process_words(&raw, 1u);
        }
        channel.crc_word = crc.stored_word();
    }
    return true;
}

} // namespace auro3d:encode
