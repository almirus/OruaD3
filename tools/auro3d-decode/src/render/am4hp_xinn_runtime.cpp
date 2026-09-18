#include "am4hp_xinn_runtime.hpp"

#include "../auro3deng/detail/runtime_api.hpp"

#include <array>
#include <cstring>

namespace auro3d {

bool Am4hpXinnRuntime::construct(
    std::uint32_t input_mask, std::string& error) {
    const bool stereo = input_mask == Am4hpInputPresets::kStereoInputMask;
    const bool surround = input_mask == Am4hpInputPresets::kSurroundInputMask;
    if (!stereo && !surround) {
        error = "AM4HP XinN supports only 2.0 or 5.1 input layout";
        return false;
    }
    Am4hpInputPresets presets;
    if (!presets.load_embedded(error)
        || !presets.copy_retargeted_record(input_mask, preset_, error))
        return false;

    std::vector<std::uint8_t> xinn(4096u, 0u);
    std::vector<std::uint8_t> arena(467544u + 4096u, 0u);
    // The native Core constructor seeds this shared XinN arena before the
    // Upmixing constructor is called. Live capture of the exact stereo
    // path shows the seven non-zero uint32 words below at; leaving
    // the block zeroed changes the stateful XinN transient after construction.
    // The enclosing Core arena layout differs because the surround graph
    // owns an additional CenterSurround object before Upmixing. Live
    // constructor capture places this record at word 20236 and
    // stores the original Core layout 0x37 there.
    const std::size_t native_config_offset = surround ? 0x13C30u : 0x13C90u;
    const std::uint32_t native_config_words[] = {
        48000u, surround ? 0x37u : 3u, 2u, 0u, 0u, 1u, 0x3F800000u, 1u,
        0u, 0u, 0u, 0u, 0u, 0u, 1u,
    };
    std::memcpy(arena.data() + native_config_offset,
                native_config_words, sizeof(native_config_words));
    const std::array<std::uint64_t, 5u> memory_args = {
        reinterpret_cast<std::uint64_t>(arena.data()),
        surround ? reinterpret_cast<std::uint64_t>(arena.data() + 40128u) : 0u,
        0u,
        0u,
        0u,
    };
    if (!auro3deng::auro_matic_v3_XinN_fl32_construct(
            xinn.data(), memory_args.data())) {
        error = "AM4HP XinN construction failed";
        return false;
    }
    output_mask_ = stereo ? kStereoOutputMask : kSurroundOutputMask;
    const auto preset_address = reinterpret_cast<std::uint64_t>(
        preset_.data() + 8u);
    if (auro3deng::auro_matic_v3_XinN_fl32_initialize(
            xinn.data(), input_mask, output_mask_, 48000u,
            preset_address) != 0) {
        error = "AM4HP XinN initialization failed";
        return false;
    }
    if (surround) {
        // Native overwrites these exact portions of the
        // 120-byte v3 dynamic record after initialization. Keep the other
        // fields returned by XinN intact, matching the native get-then-set.
        std::array<std::uint8_t, 120u> dynamic{};
        (void)auro3deng::auro_matic_v3_XinN_fl32_get_dynamic_parameters(
            xinn.data(), dynamic.data());
        constexpr std::uint32_t kDynamicWords0[4] = {0u, 2u, 3u, 0u};
        constexpr std::uint32_t kDynamicWords16[4] = {2u, 3u, 0u, 2u};
        // Exact 72-byte routing payload captured at Routing state +72 for
        // native mode 2. The prior partial overwrite left stereo defaults in
        // the tail and muted the generated surround-height buses.
        constexpr std::uint32_t kRoutingWords[18] = {
            0x3F800000u, 0x3F800000u, 0x3F800000u, 0x3F800000u,
            0x3F800000u, 0x3F800000u, 0x3F800000u, 0x3F21866Cu,
            0u, 0u, 0u, 0u,
            0x3F21866Cu, 0x3F800000u, 0u, 0u, 0u, 0u,
        };
        std::memcpy(dynamic.data(), kDynamicWords0, sizeof(kDynamicWords0));
        std::memcpy(dynamic.data() + 16u, kDynamicWords16,
                    sizeof(kDynamicWords16));
        std::uint32_t three = 3u;
        std::memcpy(dynamic.data() + 32u, &three, sizeof(three));
        std::memcpy(dynamic.data() + 48u, kRoutingWords,
                    sizeof(kRoutingWords));
        (void)auro3deng::auro_matic_v3_XinN_fl32_set_dynamic_parameters(
            xinn.data(), dynamic.data());
    }
    xinn_state_ = std::move(xinn);
    arena_ = std::move(arena);
    input_mask_ = input_mask;
    initialized_ = true;
    return true;
}

bool Am4hpXinnRuntime::process(void** channel_span_31) noexcept {
    if (!initialized_ || !channel_span_31)
        return false;
    // Current engine forwards to the base XinN process, which
    // returns the nonzero Routing state on success; only a null state or an
    // unconfigured mode returns 0.
    return auro3deng::auro_matic_v3_XinN_fl32_process(
               reinterpret_cast<std::uint64_t>(xinn_state_.data()),
               channel_span_31) != 0;
}

void Am4hpXinnRuntime::reset_audio_state() noexcept {
    if (initialized_)
        (void)auro3deng::auro_matic_v3_XinN_fl32_reset_audio_state(
            xinn_state_.data());
}

}
