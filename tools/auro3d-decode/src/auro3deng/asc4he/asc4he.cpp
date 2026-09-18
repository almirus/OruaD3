#include "../detail/preamble.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

// Native auro_centergen_v3_Processor_set_dynamic_parameters.
// Copies the 56-byte dynamic payload into proc+20..+75, then derives the
// per-block gains, thresholds, ranges and ring lengths the float32 process
// reads. The float32 (mode 1) layout is the only one this port processes; the
// fixed32/double (mode 0/2) stores degenerate to the same float fields.
static void auro_centergen_v3_Processor_set_dynamic_parameters_body(
    std::uint8_t* a1, const std::uint8_t* a2) {
    if (!a1 || !a2)
        return;
    std::memcpy(a1 + 20u, a2, 16u);
    std::memcpy(a1 + 36u, a2 + 16u, 16u);
    std::memcpy(a1 + 52u, a2 + 32u, 16u);
    std::memcpy(a1 + 68u, a2 + 48u, 8u);
    *reinterpret_cast<std::uint32_t*>(a1 + 108u) =
        *reinterpret_cast<const std::uint32_t*>(a1 + 20u);

    auto read_f32 = [a1](std::uintptr_t offset) {
        return *reinterpret_cast<const float*>(a1 + offset);
    };
    auto write_f32 = [a1](std::uintptr_t offset, float value) {
        *reinterpret_cast<float*>(a1 + offset) = value;
    };
    // Native: 0 dB pass-through above 144 dB of cut, else linear(-0.05*db).
    auto db_to_lin = [](float db) {
        return db > -144.0f ? std::pow(10.0f, db * 0.050000001f) : 0.0f;
    };

    const float v4 = read_f32(24u);
    write_f32(112u, v4 < 144.0f
        ? std::pow(10.0f, -0.050000001f * v4)
        : 0.0f);
    const float v10 = read_f32(28u);
    const float v13 = read_f32(32u) * v10;
    write_f32(120u, v13);
    write_f32(128u, (v10 - v13) > 0.00000011920929f
        ? v10 - v13
        : 0.00000011920929f);

    write_f32(136u, db_to_lin(read_f32(36u)));
    write_f32(144u, db_to_lin(read_f32(40u)));
    const float v24 = read_f32(44u);
    const float v25 = read_f32(48u);
    write_f32(152u, v24);
    write_f32(160u, std::max(v25 - v24, 0.00000011920929f));

    // Ring lengths in 32-sample quanta: trunc(rate * seconds) 32, min 1.
    const std::int32_t sample_rate =
        *reinterpret_cast<const std::int32_t*>(a1 + 80u);
    const std::int32_t divisor =
        *reinterpret_cast<const std::int32_t*>(a1 + 88u);
    if (sample_rate > 0 && divisor > 0) {
        const auto quanta = [&](float seconds) {
            const auto scaled = static_cast<std::uint32_t>(
                static_cast<std::int32_t>(
                    static_cast<float>(sample_rate) * seconds));
            return std::max(1u, scaled / static_cast<std::uint32_t>(divisor));
        };
        *reinterpret_cast<std::uint32_t*>(a1 + 168u) = quanta(read_f32(52u));
        *reinterpret_cast<std::uint32_t*>(a1 + 172u) = quanta(read_f32(56u));
    }
    write_f32(176u, db_to_lin(read_f32(60u)));
    write_f32(184u, read_f32(64u));
    write_f32(192u, db_to_lin(read_f32(68u)));
    write_f32(200u, db_to_lin(read_f32(72u)));
}

namespace auro3deng {

#include "spatial_and_centergen.inc"
#include "crossover_and_processors.inc"

} // namespace auro3deng
