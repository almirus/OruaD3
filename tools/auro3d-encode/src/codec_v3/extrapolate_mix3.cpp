#include "extrapolate_mix3.hpp"

namespace auro3d::encode {
namespace {

std::int32_t add(std::int32_t left, std::int32_t right) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) + static_cast<std::uint32_t>(right));
}

std::int32_t sub(std::int32_t left, std::int32_t right) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) - static_cast<std::uint32_t>(right));
}

std::int32_t div4(std::int32_t previous, std::int32_t predictor) {
    const std::int32_t sum = static_cast<std::int32_t>(
        3u * static_cast<std::uint32_t>(predictor) + static_cast<std::uint32_t>(previous));
    const std::int32_t adjusted = sum >= 0 ? sum : static_cast<std::int32_t>(sum + 3);
    const std::uint32_t raw = static_cast<std::uint32_t>(adjusted);
    const std::uint32_t fill = adjusted < 0 ? 0xE0000000u : 0u;
    return static_cast<std::int32_t>((raw >> 2u) | fill);
}

bool sums_to_carrier(
    std::int32_t carrier,
    std::int32_t primary,
    std::int32_t secondary,
    std::int32_t tertiary) {
    return carrier == add(add(primary, secondary), tertiary);
}

} // namespace

bool encode_extrapolate_mix3_initial(
    const std::vector<std::int32_t>& carrier,
    const std::vector<std::int32_t>& primary,
    const std::vector<std::int32_t>& secondary,
    const std::vector<std::int32_t>& tertiary,
    const ExtrapolateMix3Seeds& seeds,
    ExtrapolateMix3State& state,
    std::vector<std::int32_t>& errors,
    std::string& error) {
    error.clear();
    errors.clear();
    state = {};
    if (carrier.size() < 3u || primary.size() < 3u || secondary.size() < 3u || tertiary.size() < 3u) {
        error = "mix3 requires the first three samples";
        return false;
    }
    for (std::size_t index = 0; index < 3u; ++index) {
        if (!sums_to_carrier(carrier[index], primary[index], secondary[index], tertiary[index])) {
            error = "mix3 components do not sum to carrier";
            return false;
        }
    }

    const std::int32_t error0 = sub(secondary[0], seeds.seed0_primary);
    const std::int32_t error1 = sub(tertiary[0], seeds.seed0_secondary);
    const std::int32_t base0 = sub(
        sub(carrier[0], add(error0, error1)),
        add(seeds.seed0_primary, seeds.seed0_secondary));
    if (base0 != primary[0]) {
        error = "mix3 sample zero does not match native seed state";
        return false;
    }
    errors.push_back(error0);
    errors.push_back(error1);
    state.last_a = seeds.seed0_secondary;
    state.last_c = base0;
    state.phase = 1u;

    const std::int32_t error2 = sub(primary[1], seeds.seed1_primary);
    const std::int32_t error3 = sub(tertiary[1], seeds.seed1_secondary);
    const std::int32_t base1 = sub(
        sub(carrier[1], add(error2, error3)),
        add(seeds.seed1_primary, seeds.seed1_secondary));
    if (base1 != secondary[1]) {
        error = "mix3 sample one does not match native seed state";
        return false;
    }
    errors.push_back(error2);
    errors.push_back(error3);
    state.last_b = state.last_c;
    state.last_c = base1;
    state.last_a = seeds.seed1_primary;
    state.phase = 2u;

    const std::int32_t predictor = static_cast<std::int32_t>(
        4u * static_cast<std::uint32_t>(state.last_a)
        - 3u * static_cast<std::uint32_t>(state.last_b));
    const std::int32_t predicted_primary = div4(state.last_b, predictor);
    const std::int32_t error4 = sub(primary[2], predicted_primary);
    const std::int32_t error5 = sub(secondary[2], seeds.seed2_primary);
    const std::int32_t base2 = sub(
        sub(carrier[2], add(error4, error5)),
        add(predicted_primary, seeds.seed2_primary));
    if (base2 != tertiary[2]) {
        error = "mix3 sample two does not match native predictor state";
        return false;
    }
    errors.push_back(error4);
    errors.push_back(error5);
    state.last_b = state.last_c;
    state.last_c = base2;
    state.last_a = seeds.seed2_primary;
    state.predictor = predictor;
    state.phase = 3u;
    return true;
}

} // namespace auro3d:encode
