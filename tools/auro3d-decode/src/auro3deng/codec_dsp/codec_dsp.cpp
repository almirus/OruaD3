#include "../detail/preamble.hpp"
#include "downmix_plan.hpp"

namespace auro3deng {

#include "../codec_v3/implementation.inc"
#include "../output_generator/implementation.inc"
#include "../a3deng_v3/implementation.inc"
#include "../a3deng_v3/auromatic_engine.inc"
#include "../auromatic_downmix/implementation.inc"

bool cx_downmix_engine_plan(
    std::uint32_t source_layout,
    std::uint32_t target_layout,
    std::vector<DownmixRule>& rules,
    std::uint32_t& output_layout) {
    rules.clear();
    output_layout = 0;
    alignas(16) std::uint8_t engine[56]{};
    alignas(16) std::int32_t plan[kDownmixPlanIntCount]{};
    auro_downmix_v1_engine_t_construct(engine);
    if (!auro_downmix_v1_engine_calculate(
            reinterpret_cast<std::int32_t*>(engine),
            plan,
            source_layout,
            target_layout))
        return false;

    const std::uint64_t count = *reinterpret_cast<const std::uint64_t*>(plan);
    if (count > 52u)
        return false;
    rules.reserve(static_cast<std::size_t>(count));
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(plan);
    for (std::uint64_t index = 0; index < count; ++index) {
        const auto* row = bytes + 8u + 12u * static_cast<std::size_t>(index);
        DownmixRule rule{};
        std::memcpy(&rule.source_channel, row, sizeof(rule.source_channel));
        std::memcpy(&rule.destination_channel, row + 4u, sizeof(rule.destination_channel));
        std::memcpy(&rule.gain_index, row + 8u, sizeof(rule.gain_index));
        if (rule.source_channel >= 31u || rule.destination_channel >= 31u ||
            rule.gain_index >= 111u)
            return false;
        rules.push_back(rule);
    }

    // Native calculate_gains_: apply rules
    // in order with deferred source clearing (multi-hop, e.g. Top→C→FL).
    // plan[159] must not be required to equal the target.
    std::uint32_t present = source_layout;
    std::uint32_t clear_mask = 0;
    for (const auto& rule : rules) {
        const std::uint32_t src_bit = std::uint32_t{1} << rule.source_channel;
        const std::uint32_t dst_bit = std::uint32_t{1} << rule.destination_channel;
        if ((present & src_bit) == 0u)
            continue;
        if (rule.source_channel == rule.destination_channel)
            return false;
        present |= dst_bit;
        clear_mask |= src_bit;
    }
    present &= ~clear_mask;
    if (present != target_layout)
        return false;
    output_layout = target_layout;
    return true;
}

} // namespace auro3deng
