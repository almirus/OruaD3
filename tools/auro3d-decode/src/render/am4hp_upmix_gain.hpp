#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace auro3d {

// Native auro_matic_hp_v4_UpmixGain_process (x86_64 0x54EA10) applies one
// gain per active destination plane to 32-sample float blocks. The descriptor
// mapping is explicit: a null plane before the configured count is an error.
class Am4hpUpmixGain {
public:
    bool construct(const std::vector<unsigned>& plane_mapping,
                   const std::vector<float>& gains) noexcept;
    void reset_audio_state() noexcept;
    bool process(const std::vector<std::array<float, 32>*>& planes) const noexcept;
    std::size_t count() const noexcept { return plane_mapping_.size(); }

private:
    std::vector<unsigned> plane_mapping_;
    std::vector<float> gains_;
};

}
