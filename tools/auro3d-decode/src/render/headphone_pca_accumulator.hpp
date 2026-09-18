#pragma once

#include <array>

namespace auro3d {

// The selected native source_*_apply_pca_gains path uses separately
// rounded multiply and add operations over one 32-sample block.
class HeadphonePcaAccumulator {
public:
    static void add(const std::array<float, 32>& input,
                    float gain,
                    std::array<float, 32>& output) noexcept;
};

}
