#pragma once

#include "cx_probe.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace auro3d {
namespace cx {

struct SchemaPosition {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct SchemaSpread {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct ResolvedObjectMetadata {
    std::vector<std::int64_t> gain_q23;
    std::vector<SchemaPosition> positions;
    std::vector<SchemaSpread> spreads;
};

struct ReferenceGainSelection {
    std::size_t program_index = 0;
    std::size_t switch_group_index = 0;
    std::size_t switch_element_index = 0;
    bool use_switch_group = false;
};

std::int64_t gain_to_scaler_q23(const AuroCxIntegralGainInfo& gain);
AuroCxIntegralGainInfo get_bed_ref_gain(
    const CxSchemaParseResult& schema,
    std::size_t bed_index,
    const ReferenceGainSelection& selection);
AuroCxIntegralGainInfo get_object_group_ref_gain(
    const CxSchemaParseResult& schema,
    std::size_t object_group_index,
    const ReferenceGainSelection& selection);
AuroCxIntegralGainInfo relative_object_ref_gain(
    const AuroCxIntegralGainInfo& object_group_gain,
    const AuroCxIntegralGainInfo& bed_gain);
bool find_reference_location(
    const CxSchemaParseResult& schema,
    std::size_t bed_index,
    const std::vector<std::uint32_t>& linked_object_groups,
    ReferenceGainSelection& selection);
SchemaPosition position_to_schema(
    const AuroCxSchemaPositionInfo& position,
    bool position_16bit);
SchemaSpread spread_to_schema(const AuroCxSchemaSpreadInfo& spread);
bool resolve_gain_subblocks(
    const std::vector<AuroCxSchemaGainSubblockInfo>& encoded,
    std::size_t subblock_count,
    bool use_default,
    std::vector<AuroCxIntegralGainInfo>& resolved);
bool resolve_position_subblocks(
    const std::vector<AuroCxSchemaPositionSubblockInfo>& encoded,
    std::size_t subblock_count,
    bool use_default,
    const AuroCxSchemaPositionInfo& default_position,
    std::vector<AuroCxSchemaPositionInfo>& resolved);
bool resolve_spread_subblocks(
    const std::vector<AuroCxSchemaSpreadSubblockInfo>& encoded,
    std::size_t subblock_count,
    bool use_default,
    const AuroCxSchemaSpreadInfo& default_spread,
    std::vector<AuroCxSchemaSpreadInfo>& resolved);
bool resolve_object_metadata(
    const CxSchemaParseResult& schema,
    std::size_t group_index,
    std::size_t object_index,
    ResolvedObjectMetadata& resolved);

} // namespace cx
} // namespace auro3d
