#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace auro3d {

// Native AM4HP Processor_process expands the active 31-bit layout mask into
// an ascending physical-slot list before invoking CoreProcessor. Keeping
// this as a value object prevents downstream stages from using incidental
// PCM order as a channel identity.
class Am4hpInputLayout {
public:
    using Block = std::array<float, 32>;
    using PhysicalBlocks = std::array<Block*, 31>;
    using CoreBlocks = std::array<Block*, 31>;

    bool construct(std::uint32_t layout_mask) noexcept;
    std::uint32_t mask() const noexcept { return mask_; }
    std::size_t channel_count() const noexcept { return count_; }
    std::uint8_t slot_at(std::size_t active_index) const noexcept;
    bool contains(std::uint8_t slot) const noexcept;

    // Build the compact pointer array passed to CoreProcessor. Every active
    // physical slot must have a block; inactive entries are ignored.
    bool bind_core_blocks(const PhysicalBlocks& physical,
                          CoreBlocks& core) const noexcept;

private:
    std::uint32_t mask_ = 0u;
    std::array<std::uint8_t, 31> slots_{};
    std::size_t count_ = 0u;
};

}
