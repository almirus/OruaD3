#include "headphone_pca_accumulator.hpp"

namespace auro3d {

void HeadphonePcaAccumulator::add(const std::array<float, 32>& input,
                                  float gain,
                                  std::array<float, 32>& output) noexcept {
    for (std::size_t i = 0u; i < input.size(); ++i) {
        // The selected x86_64 oracle rounds the multiply before the add.
        // Keeping the product volatile prevents contraction into FMA under
        // optimized builds and matches captured Explicit source vectors.
        volatile float product = input[i] * gain;
        output[i] += product;
    }
}

}
