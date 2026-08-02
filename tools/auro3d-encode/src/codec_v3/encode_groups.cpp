#include "encode_groups.hpp"

#include "encode_group.hpp"
#include "encoder_defaults.hpp"
#include "frame_descriptor.hpp"
#include "layout.hpp"

#include <utility>

namespace auro3d::encode {

bool build_encode_group_plan(
    std::uint32_t original_layout,
    std::vector<EncodeGroupPlan>& groups,
    std::string& error) {
    error.clear();
    groups.clear();

    std::uint32_t carrier_layout = 0u;
    if (!codec_v3_carrier_layout(original_layout, carrier_layout)) {
        error = "original layout has no native carrier mapping";
        return false;
    }

    for (std::uint32_t channel = 0; channel < 31u; ++channel) {
        if ((carrier_layout & (1u << channel)) == 0u)
            continue;
        OriginalChannelGroup sources{};
        const std::uint32_t status =
            get_original_channels(original_layout, channel, sources);
        if (status != 0u) {
            error = "get_original_channels rejected carrier channel "
                + std::to_string(channel) + " (status "
                + std::to_string(status) + ")";
            return false;
        }
        if (sources.arity == 0u || sources.arity > 3u) {
            error = "get_original_channels returned invalid arity";
            return false;
        }
        for (std::uint32_t source = 0; source < sources.arity; ++source) {
            if (sources.channels[source] >= kCodecV3ChannelCount) {
                error = "get_original_channels returned an out-of-range source channel";
                return false;
            }
            for (std::uint32_t prior = 0; prior < source; ++prior) {
                if (sources.channels[prior] == sources.channels[source]) {
                    error = "get_original_channels returned duplicate source channels";
                    return false;
                }
            }
        }
        EncodeGroupPlan plan{};
        plan.carrier_channel = channel;
        plan.sources = sources;
        groups.push_back(plan);
    }
    if (groups.empty()) {
        error = "no encode groups for carrier layout";
        return false;
    }
    return true;
}

bool materialize_encode_groups(
    const std::vector<EncodeGroupPlan>& plan,
    const std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t bit_line_low,
    std::uint32_t bit_line_high,
    std::uint32_t gvm_common_limit,
    std::vector<EncodeGroup>& groups,
    std::string& error) {
    error.clear();
    groups.clear();
    if (codec_planes.size() != 31u) {
        error = "codec planes must be addressed by the 31 codec-v3 channel IDs";
        return false;
    }
    if (bit_line_low > bit_line_high) {
        error = "bit_line range is inverted";
        return false;
    }
    if (bit_line_low < 3u || bit_line_high > 12u) {
        error = "bit_line range is outside codec-v3 Config::validate range";
        return false;
    }
    std::uint32_t plan_carrier_mask = 0u;
    for (const EncodeGroupPlan& entry : plan) {
        if (entry.carrier_channel >= kCodecV3ChannelCount) {
            error = "encode group plan contains an out-of-range carrier channel";
            return false;
        }
        const std::uint32_t bit = std::uint32_t{1} << entry.carrier_channel;
        if ((plan_carrier_mask & bit) != 0u) {
            error = "encode group plan contains duplicate carrier channels";
            return false;
        }
        plan_carrier_mask |= bit;
    }
    if (plan.empty()) {
        error = "encode group plan is empty";
        return false;
    }

    for (const EncodeGroupPlan& entry : plan) {
        EncodeGroup group{};
        if (!materialize_encode_group(
                entry, codec_planes, bit_line_low, gvm_common_limit, group, error)) {
            return false;
        }
        groups.push_back(std::move(group));
    }
    (void)bit_line_high;
    return true;
}

bool materialize_encode_group(
    const EncodeGroupPlan& entry,
    const std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t bit_line,
    std::uint32_t gvm_common_limit,
    EncodeGroup& group,
    std::string& error) {
    error.clear();
    group = {};
    if (entry.carrier_channel >= kCodecV3ChannelCount
        || entry.sources.arity == 0u || entry.sources.arity > 3u) {
        error = "encode group plan has an invalid carrier or source arity";
        return false;
    }
    if (bit_line < 3u || bit_line > 12u) {
        error = "encode group bit line is outside codec-v3 range";
        return false;
    }
    if (codec_planes.size() != 31u) {
        error = "codec planes must be addressed by the 31 codec-v3 channel IDs";
        return false;
    }
    group.carrier_channel = entry.carrier_channel;
    group.sources = entry.sources;
    group.bit_line = bit_line;
    if (!codec_v3_bit_line_quality(group.bit_line, group.bit_line_quality)) {
        error = "codec-v3 bit line has no native quality mapping";
        return false;
    }
    // Encoder::create_group_ @ 0x4E8360 overwrites the constructor's four
    // one-valued GVM bounds after selecting the bit line.
    group.gvm_common_limit =
        gvm_common_limit != 0u ? gvm_common_limit : 150u;
    group.gvm_dimension1_start = group.bit_line_quality;
    group.gvm_dimension2_start = group.bit_line_quality;
    group.gvm_minimum_clusters = 1u;
    // The generic materializer has no Encoder instance; its caller may
    // overwrite this with Encoder::reserve_extra_bits before analysis.
    group.group_field_208 = 0u;
    for (std::uint32_t i = 0; i < entry.sources.arity; ++i) {
        const std::uint32_t channel = entry.sources.channels[i];
        if (channel >= codec_planes.size() || codec_planes[channel].empty()) {
            error = "encode group source channel has no PCM plane";
            return false;
        }
        for (std::uint32_t prior = 0; prior < i; ++prior) {
            if (entry.sources.channels[prior] == channel) {
                error = "encode group plan contains duplicate source channels";
                return false;
            }
        }
        const auto& plane = codec_planes[channel];
        if (!attach_group_channel_pcm(
                group, channel, plane.data(), plane.data() + plane.size(), error)) {
            return false;
        }
    }
    return true;
}

} // namespace auro3d::encode
