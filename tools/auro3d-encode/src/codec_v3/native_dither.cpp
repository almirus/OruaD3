#include "native_dither.hpp"

#include <ctime>
#include <limits>

namespace auro3d::encode {
namespace {

std::int32_t saturating_add(
    std::int32_t value,
    std::int32_t dither) {
    const std::int64_t sum =
        static_cast<std::int64_t>(value)
        + static_cast<std::int64_t>(dither);
    if (sum > std::numeric_limits<std::int32_t>::max())
        return std::numeric_limits<std::int32_t>::max();
    if (sum < std::numeric_limits<std::int32_t>::min())
        return std::numeric_limits<std::int32_t>::min();
    return static_cast<std::int32_t>(sum);
}

bool initialize_pool(
    NativeDitherPool& pool,
    std::uint32_t rescaler_seed,
    std::uint32_t shift,
    std::string& error) {
    if (shift == 0u || shift > 32u) {
        error = "native dither shift is outside 1..32";
        return false;
    }

    // Rescaler:goc_dither_pool_ seeds a temporary MT19937 with
    // `shift + Rescaler+64`; its first output is the explicit Pool seed.
    std::mt19937 seed_generator(rescaler_seed + shift);
    pool.random.seed(seed_generator());
    pool.samples.resize(kNativeDitherPoolSamples);

    // Pool constructs [0, 2^shift] and the generator at
    // subtracts two independently reduced MT outputs.
    const std::uint64_t modulus =
        shift == 32u
        ? (std::uint64_t{1} << 32u) + 1u
        : (std::uint64_t{1} << shift) + 1u;
    for (std::int32_t& sample : pool.samples) {
        const std::uint64_t first =
            static_cast<std::uint64_t>(pool.random()) % modulus;
        const std::uint64_t second =
            static_cast<std::uint64_t>(pool.random()) % modulus;
        const std::int64_t triangular =
            static_cast<std::int64_t>(first)
            - static_cast<std::int64_t>(second);
        if (triangular > std::numeric_limits<std::int32_t>::max()
            || triangular < std::numeric_limits<std::int32_t>::min()) {
            error = "native dither sample exceeds int32 storage";
            return false;
        }
        sample = static_cast<std::int32_t>(triangular);
    }
    pool.initialized = true;
    return true;
}

} // namespace

void initialize_native_dither(
    NativeDitherState& state,
    bool seed_present,
    std::uint64_t seed) {
    state = {};
    const std::uint32_t encoder_seed =
        seed_present
        ? static_cast<std::uint32_t>(seed)
        : static_cast<std::uint32_t>(std::time(nullptr));
    state.config_seed_present = seed_present;
    state.config_seed = seed_present ? encoder_seed : 0u;
    // Encoder:Encoder initializes its MT19937 from Config+144
    // and stores the first tempered value as the Rescaler seed.
    std::mt19937 encoder_random(encoder_seed);
    state.rescaler_seed = encoder_random();
    state.initialized = true;
}

bool apply_native_dither(
    std::vector<std::int32_t>& samples,
    std::uint32_t shift,
    NativeDitherState& state,
    std::string& error) {
    error.clear();
    if (!state.initialized) {
        error = "native dither state was not initialized";
        return false;
    }
    if (shift == 0u || shift >= state.pools.size()) {
        error = "native dither shift is outside pool table";
        return false;
    }
    if (samples.size() + 1u >= kNativeDitherPoolSamples) {
        error = "native dither frame is too large for its 20480-sample pool";
        return false;
    }
    NativeDitherPool& pool = state.pools[shift];
    if (!pool.initialized
        && !initialize_pool(
            pool, state.rescaler_seed, shift, error)) {
        return false;
    }

    // Pool:get chooses rng % (pool_size - frame_size - 1), then returns a
    // contiguous frame-sized window. Keep the strict native upper gap.
    const std::uint32_t span = static_cast<std::uint32_t>(
        pool.samples.size() - samples.size() - 1u);
    const std::size_t offset =
        static_cast<std::size_t>(pool.random() % span);
    for (std::size_t index = 0u; index < samples.size(); ++index) {
        samples[index] = saturating_add(
            samples[index], pool.samples[offset + index]);
    }
    return true;
}

} // namespace auro3d:encode
