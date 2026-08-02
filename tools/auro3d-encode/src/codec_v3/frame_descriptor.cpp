#include "frame_descriptor.hpp"

#include "layout.hpp"

namespace auro3d::encode {
namespace {

bool layout_channel_pointers_present(
    std::uint32_t layout,
    const std::array<const std::int32_t*, kCodecV3ChannelCount>& pointers) {
    for (std::uint32_t id = 0; id < kCodecV3ChannelCount; ++id) {
        if ((layout & (1u << id)) != 0u && pointers[id] == nullptr)
            return false;
    }
    return true;
}

bool layout_channel_pointers_present(
    std::uint32_t layout,
    const std::array<std::int32_t*, kCodecV3ChannelCount>& pointers) {
    for (std::uint32_t id = 0; id < kCodecV3ChannelCount; ++id) {
        if ((layout & (1u << id)) != 0u && pointers[id] == nullptr)
            return false;
    }
    return true;
}

} // namespace

bool make_input_descriptor(
    const std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t layout,
    std::uint32_t sample_rate,
    std::uint32_t frame_count,
    ConstFrameDescriptor& descriptor,
    std::string& error) {
    error.clear();
    descriptor = {};
    std::uint32_t carrier_layout = 0;
    if (codec_planes.size() != kCodecV3ChannelCount) {
        error = "invalid codec input plane count";
        return false;
    }
    if (frame_count == 0u || !codec_v3_unit_block_size_supported(frame_count)
        || !codec_v3_sample_rate_supported(sample_rate) || layout == 0u
        || !codec_v3_carrier_layout(layout, carrier_layout)) {
        error = "invalid native codec-v3 input descriptor shape";
        return false;
    }
    descriptor.frame_count = frame_count;
    descriptor.sample_rate = sample_rate;
    descriptor.sample_bits = kCodecV3PcmBits;
    descriptor.layout = layout;
    for (std::uint32_t id = 0; id < kCodecV3ChannelCount; ++id) {
        if ((layout & (1u << id)) == 0u) {
            if (!codec_planes[id].empty()) {
                error = "inactive codec input plane contains samples";
                return false;
            }
            continue;
        }
        if (codec_planes[id].size() != frame_count) {
            error = "input codec plane length does not match the unit block";
            return false;
        }
        descriptor.channel_ptr[id] = codec_planes[id].data();
    }
    return true;
}

bool make_carrier_unit(
    std::uint32_t carrier_layout,
    std::uint32_t sample_rate,
    std::uint32_t frame_count,
    CarrierUnit& unit,
    std::string& error) {
    error.clear();
    unit = {};
    if (carrier_layout == 0u || frame_count == 0u
        || !codec_v3_unit_block_size_supported(frame_count)
        || !codec_v3_sample_rate_supported(sample_rate)) {
        error = "invalid native codec-v3 carrier descriptor shape";
        return false;
    }
    unit.descriptor.frame_count = frame_count;
    unit.descriptor.sample_rate = sample_rate;
    unit.descriptor.sample_bits = kCodecV3PcmBits;
    unit.descriptor.layout = carrier_layout;
    for (std::uint32_t id = 0; id < kCodecV3ChannelCount; ++id) {
        if ((carrier_layout & (1u << id)) == 0u)
            continue;
        unit.planes[id].resize(frame_count);
        unit.descriptor.channel_ptr[id] = unit.planes[id].data();
    }
    if (!layout_channel_pointers_present(carrier_layout, unit.descriptor.channel_ptr)) {
        error = "could not allocate carrier codec planes";
        return false;
    }
    return true;
}

bool validate_encoder_frame_contract(
    const ConstFrameDescriptor& original,
    const MutableFrameDescriptor& carrier,
    std::string& error) {
    error.clear();
    std::uint32_t expected_carrier = 0;
    if (!codec_v3_carrier_layout(original.layout, expected_carrier)) {
        error = "original layout has no native codec-v3 carrier";
        return false;
    }
    if (carrier.layout != expected_carrier) {
        error = "carrier layout does not match native codec-v3 mapping";
        return false;
    }
    if (original.frame_count != carrier.frame_count) {
        error = "original and carrier unit frame counts differ";
        return false;
    }
    if (original.sample_rate != carrier.sample_rate) {
        error = "original and carrier sample rates differ";
        return false;
    }
    if (original.sample_bits != kCodecV3PcmBits || carrier.sample_bits != kCodecV3PcmBits) {
        error = "codec-v3 unit encoder requires PCM24 descriptors";
        return false;
    }
    if (!layout_channel_pointers_present(original.layout, original.channel_ptr)) {
        error = "missing original-layout channel pointer";
        return false;
    }
    if (!layout_channel_pointers_present(carrier.layout, carrier.channel_ptr)) {
        error = "missing carrier-layout channel pointer";
        return false;
    }
    return true;
}

} // namespace auro3d::encode
