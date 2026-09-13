#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace auro3d {

struct HeadphonePcaFilterRecord;

// Native auro_headphones_v2_WIIR_process works on one 32-sample block. This
// class covers its odd-order first-order section and second-order cascade.
// Each section's audio state is owned by this object.
class HeadphoneWiir {
public:
    struct FirstOrder {
        float scale = 1.0f;
        float output0 = 1.0f;
        float output1 = 0.0f;
        float feedback0 = 0.0f;
        float feedback1 = 0.0f;
    };

    struct Section {
        float scale = 1.0f;
        float output0 = 0.0f;
        float output1 = 0.0f;
        float output2 = 0.0f;
        float feedback0 = 0.0f;
        float feedback1 = 0.0f;
        float feedback2 = 0.0f;
    };

    bool construct(float lambda, const std::vector<Section>& sections);
    bool construct_with_first_order(float lambda,
                                    const FirstOrder& first_order,
                                    const std::vector<Section>& sections);
    bool construct_from_filter(const HeadphonePcaFilterRecord& filter);
    void reset_audio_state() noexcept;
    bool process(const std::array<float, 32>& input,
                 std::array<float, 32>& output) noexcept;

    // Native WIIR_process_2c advances the two channel states together and
    // evaluates the three output taps as c2*y2 + (c0*y0 + c1*y1).
    bool process_pair(const std::array<float, 32>& first_input,
                     const std::array<float, 32>& second_input,
                     std::array<float, 32>& first_output,
                     std::array<float, 32>& second_output,
                     HeadphoneWiir& second_filter) noexcept;

private:
    float lambda_ = 0.0f;
    bool has_first_order_ = false;
    FirstOrder first_order_{};
    std::array<float, 2> first_state_{};
    std::vector<Section> sections_;
    std::vector<std::array<float, 3>> state_;
};

}
