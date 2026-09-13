#include "am4hp_input_layout.hpp"

namespace auro3d {

bool Am4hpInputLayout::construct(std::uint32_t layout_mask) noexcept {
    mask_ = layout_mask;
    count_ = 0u;
    for (std::uint8_t slot = 0u; slot < slots_.size(); ++slot) {
        if ((layout_mask & (std::uint32_t{1u} << slot)) != 0u)
            slots_[count_++] = slot;
    }
    return count_ != 0u;
}

std::uint8_t Am4hpInputLayout::slot_at(std::size_t active_index) const noexcept {
    return active_index < count_ ? slots_[active_index] : 0xFFu;
}

bool Am4hpInputLayout::contains(std::uint8_t slot) const noexcept {
    return slot < 31u && (mask_ & (std::uint32_t{1u} << slot)) != 0u;
}

bool Am4hpInputLayout::bind_core_blocks(
    const PhysicalBlocks& physical, CoreBlocks& core) const noexcept {
    core.fill(nullptr);
    for (std::size_t index = 0u; index < count_; ++index) {
        const auto slot = slots_[index];
        if (!physical[slot]) return false;
        core[index] = physical[slot];
    }
    return count_ != 0u;
}

}
