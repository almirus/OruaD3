#include "prepare_mix.hpp"

namespace auro3d::encode {

bool prepare_mix(
    const std::vector<EncodedGroupPcm>& groups,
    MutableFrameDescriptor& carrier,
    bool copy_raw,
    std::string& error) {
    error.clear();
    std::uint32_t destination_mask = 0u;
    for (const EncodedGroupPcm& group : groups) {
        if (group.carrier_channel_id >= kCodecV3ChannelCount
            || (carrier.layout & (1u << group.carrier_channel_id)) == 0u
            || carrier.channel_ptr[group.carrier_channel_id] == nullptr) {
            error = "group has no carrier destination channel";
            return false;
        }
        const std::uint32_t destination_bit = std::uint32_t{1} << group.carrier_channel_id;
        if ((destination_mask & destination_bit) != 0u) {
            error = "multiple groups target the same carrier destination channel";
            return false;
        }
        destination_mask |= destination_bit;
        if (group.samples.size() != carrier.frame_count) {
            error = "group sample count does not match carrier unit block";
            return false;
        }
        if (group.quantization_shift < 3u || group.quantization_shift > 12u) {
            error = "group quantization shift is outside Config::validate range";
            return false;
        }
        std::int32_t* destination = carrier.channel_ptr[group.carrier_channel_id];
        if (copy_raw) {
            for (std::uint32_t sample = 0; sample < carrier.frame_count; ++sample)
                destination[sample] = group.samples[sample];
        } else {
            // Native scalar code is `*dst = *src << shift`; SSE uses PSLLD.
            // Cast through uint32_t to define the same 32-bit bit operation.
            for (std::uint32_t sample = 0; sample < carrier.frame_count; ++sample) {
                destination[sample] = static_cast<std::int32_t>(
                    static_cast<std::uint32_t>(group.samples[sample]) << group.quantization_shift);
            }
        }
        for (std::uint32_t sample = 0; sample < carrier.frame_count; ++sample) {
            const std::uint32_t raw = static_cast<std::uint32_t>(destination[sample]);
            if ((raw >> 24u) != 0u && (raw >> 24u) != 0xFFu) {
                error = "prepared carrier sample exceeds signed PCM24 range";
                return false;
            }
        }
    }
    if (destination_mask != carrier.layout) {
        error = "carrier layout has an unpopulated destination channel";
        return false;
    }
    return true;
}

} // namespace auro3d::encode
