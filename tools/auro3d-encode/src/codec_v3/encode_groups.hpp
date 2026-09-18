#pragma once

#include "original_channels.hpp"
#include "encode_group.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

struct EncodeGroupPlan {
    std::uint32_t carrier_channel = 0;
    OriginalChannelGroup sources{};
};

/// Walks every carrier channel of original_layout through
/// get_original_channels. Fails if any present carrier channel is
/// rejected by the native table.
bool build_encode_group_plan(
    std::uint32_t original_layout,
    std::vector<EncodeGroupPlan>& groups,
    std::string& error);

/// Instantiates EncodeGroup objects for one unit at the first configured bit
/// line. Callers that mirror Encoder:add_ candidate retries should use
/// materialize_encode_group instead.
bool materialize_encode_groups(
    const std::vector<EncodeGroupPlan>& plan,
    const std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t bit_line_low,
    std::uint32_t bit_line_high,
    std::uint32_t gvm_common_limit,
    std::vector<EncodeGroup>& groups,
    std::string& error);

/// Materializes one planned group for a single candidate bit line. Native
/// Encoder:add_ retries this operation independently for every group while
/// walking its configured low..high bit-line interval.
bool materialize_encode_group(
    const EncodeGroupPlan& plan,
    const std::vector<std::vector<std::int32_t>>& codec_planes,
    std::uint32_t bit_line,
    std::uint32_t gvm_common_limit,
    EncodeGroup& group,
    std::string& error);

} // namespace auro3d:encode
