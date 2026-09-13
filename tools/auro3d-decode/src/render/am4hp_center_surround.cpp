#include "am4hp_center_surround.hpp"

#include "../auro3deng/detail/runtime_api.hpp"

#include <cmath>
#include <cstring>

namespace auro3d {
namespace {

bool configure_centergen(std::uint8_t* storage,
                         std::uint32_t sample_rate,
                         float tuning) noexcept {
    if ((sample_rate != 44100u && sample_rate != 48000u)
        || !std::isfinite(tuning))
        return false;
    std::int32_t init[2]{static_cast<std::int32_t>(sample_rate), 1};
    if (::auro3deng::auro_centergen_v3_Processor_t_construct(storage, init) == 0u)
        return false;

    std::array<std::uint8_t, 20> fixed{};
    const std::uint64_t fixed_word0 = 1u;
    const std::uint32_t fixed_word4 = 524288400u;
    std::memcpy(fixed.data(), &fixed_word0, sizeof(fixed_word0));
    std::memcpy(fixed.data() + 16u, &fixed_word4, sizeof(fixed_word4));
    (void)::auro3deng::auro_centergen_v3_Processor_set_fixed_parameters(
        storage, fixed.data());

    std::array<std::uint8_t, 56> dynamic{};
    // Native Surround calls the same 0x54DB00 Centergen constructor as the
    // front wrapper. Keep its complete 56-byte dynamic payload; the previous
    // abbreviated payload changed the in-place SL/SR mutation before XinN.
    const float words0[4]{0.0f, 0.8f, 0.2f, -15.0f};
    const float words1[4]{0.0f, 1.0f, 0.05f, 0.5f};
    const float words2[2]{9.0f, 0.99f};
    std::memcpy(dynamic.data() + 4u, words0, sizeof(words0));
    std::memcpy(dynamic.data() + 20u, &tuning, sizeof(tuning));
    std::memcpy(dynamic.data() + 24u, words1, sizeof(words1));
    std::memcpy(dynamic.data() + 40u, words2, sizeof(words2));
    (void)::auro3deng::auro_centergen_v3_Processor_set_dynamic_parameters(
        storage, dynamic.data());
    return true;
}

}

bool Am4hpCenterSurround::construct(std::uint32_t sample_rate,
                                    float tuning) noexcept {
    configured_ = configure_centergen(centergen_.data(), sample_rate, tuning);
    if (configured_)
        reset_audio_state();
    return configured_;
}

void Am4hpCenterSurround::reset_audio_state() noexcept {
    if (configured_)
        (void)::auro3deng::auro_centergen_v3_Processor_reset_audio_state(centergen_.data());
}

bool Am4hpCenterSurround::process(Block& input0,
                                  Block& input1,
                                  Block& output) noexcept {
    if (!configured_)
        return false;
    if (::auro3deng::auro_centergen_v3_Processor_process(
            centergen_.data(),
            reinterpret_cast<std::uint64_t>(input0.data()),
            reinterpret_cast<std::uint64_t>(input1.data()),
            reinterpret_cast<std::uint8_t*>(output.data()), 32) != 0)
        return false;
    return true;
}

bool construct_am4hp_native_center_surround_48000(
    Am4hpCenterSurround& stage) noexcept {
    // StaticProcessing stores -4 dB for the native 0x37 surround wrapper;
    // the resulting Centergen state gain is 0x3f21866c.
    return stage.construct(48000u, -4.0f);
}

}
