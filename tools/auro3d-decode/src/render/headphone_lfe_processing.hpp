#pragma once

#include <cstddef>

namespace auro3d {

// The native auro_headphones_v2_LFEProcessing state is a 12-byte gain block.
// It mixes one 32-sample planar output from the supplied source planes; it is
// not the LFE crossover/filter used elsewhere in the Auro pipeline.
class HeadphoneLfeProcessing {
public:
    void construct(float input_scale, bool apply_input_scale) noexcept;
    void set_dynamic_gain(float gain) noexcept;
    void reset_audio_state() noexcept;

    bool process(const float* const* input_planes,
                 std::size_t input_count,
                 float* output_plane) const noexcept;

private:
    float input_scale_ = 1.0f;
    bool apply_input_scale_ = false;
    float dynamic_gain_ = 0.0f;
};

}
