#pragma once

#include <cstdint>
#include <vector>

namespace auro3deng {

struct DownmixRule {
    std::uint32_t source_channel = 0;
    std::uint32_t destination_channel = 0;
    std::uint32_t gain_index = 0;
};

bool cx_downmix_engine_plan(
    std::uint32_t source_layout,
    std::uint32_t target_layout,
    std::vector<DownmixRule>& rules,
    std::uint32_t& output_layout);

} // namespace auro3deng
