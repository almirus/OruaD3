#include "am4hp_core_descriptor.hpp"

namespace auro3d {

bool Am4hpCoreDescriptor::construct(
    std::uint32_t sample_rate,
    const Am4hpInputLayout& layout,
    const Am4hpInputLayout::PhysicalBlocks& physical) noexcept {
    auto clear = [this]() noexcept {
        qwords_.fill(0u);
        sample_rate_ = 0u;
        mask_ = 0u;
        active_count_ = 0u;
    };
    if ((sample_rate != 44100u && sample_rate != 48000u)
        || (layout.mask() & 0x80000000u) != 0u
        || layout.channel_count() == 0u) {
        clear();
        return false;
    }
    qwords_.fill(0u);
    sample_rate_ = sample_rate;
    mask_ = layout.mask();
    active_count_ = 0u;

    // Native descriptor qword 0 is { frame_count=32, sample_rate }.
    qwords_[0] = static_cast<std::uint64_t>(32u)
        | (static_cast<std::uint64_t>(sample_rate) << 32u);
    for (std::uint32_t slot = 0u; slot < 31u; ++slot) {
        if ((mask_ & (1u << slot)) == 0u)
            continue;
        if (!physical[slot]) {
            clear();
            return false;
        }
        if (active_count_ >= 31u)
            return false;
        qwords_[2u + active_count_] =
            reinterpret_cast<std::uint64_t>(physical[slot]->data());
        ++active_count_;
    }
    return true;
}

}
