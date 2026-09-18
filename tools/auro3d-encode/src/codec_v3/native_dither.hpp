#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace auro3d::encode {

constexpr std::size_t kNativeDitherPoolSamples = 20480u;

struct NativeDitherPool {
    bool initialized = false;
    std::mt19937 random{};
    std::vector<std::int32_t> samples;
};

/// Persistent Rescaler dither state. The native encoder seeds its outer
/// MT19937 from Config+144 (or time), takes one value for Rescaler+64, then
/// creates one independently seeded 20480-sample pool for each shift.
struct NativeDitherState {
    bool initialized = false;
    bool config_seed_present = false;
    std::uint32_t config_seed = 0u;
    std::uint32_t rescaler_seed = 0u;
    std::array<NativeDitherPool, 33> pools{};
};

void initialize_native_dither(
    NativeDitherState& state,
    bool seed_present,
    std::uint64_t seed);

/// Adds the same saturating TPDF window selected by native dither:Pool.
/// shift is Rescaler Group+24 + 8 and must be in 1..32.
bool apply_native_dither(
    std::vector<std::int32_t>& samples,
    std::uint32_t shift,
    NativeDitherState& state,
    std::string& error);

} // namespace auro3d:encode
