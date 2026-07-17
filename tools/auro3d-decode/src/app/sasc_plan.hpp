#pragma once

#include "cx_probe.hpp"
#include "sasc_apply.hpp"

#include <string>
#include <vector>

namespace auro3d {
namespace sasc {

// Rebuild SCG mixing steps from the current schema SASC PDU state.
// Steps are tagged with scg::Layer (0..2). Empty steps is a valid identity plan.
bool build_plans(
    const CxSchemaParseResult& schema,
    std::uint32_t object_groups,
    std::uint16_t declared_layout,
    bool has_declared_layout,
    std::vector<std::vector<Step>>& plans,
    std::uint32_t& scratch_stream,
    std::string& error);

} // namespace sasc
} // namespace auro3d
