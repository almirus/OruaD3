#pragma once

#include "am4hp_input_layout.hpp"

#include <array>
#include <cstdint>

namespace auro3d {

// Address-free representation of the 17 x __m128 (34 qword) descriptor
// consumed by the native AM4HP CoreProcessor. The wrapper expands the
// physical mask into an ascending compact pointer list at qwords 2..32.
class Am4hpCoreDescriptor {
public:
    using Block = Am4hpInputLayout::Block;
    static constexpr std::size_t kQwordCount = 34u;

    bool construct(std::uint32_t sample_rate,
                   const Am4hpInputLayout& layout,
                   const Am4hpInputLayout::PhysicalBlocks& physical) noexcept;

    std::uint32_t sample_rate() const noexcept { return sample_rate_; }
    std::uint32_t mask() const noexcept { return mask_; }
    std::size_t active_count() const noexcept { return active_count_; }
    const std::array<std::uint64_t, kQwordCount>& qwords() const noexcept {
        return qwords_;
    }

private:
    std::array<std::uint64_t, kQwordCount> qwords_{};
    std::uint32_t sample_rate_ = 0u;
    std::uint32_t mask_ = 0u;
    std::size_t active_count_ = 0u;
};

}
