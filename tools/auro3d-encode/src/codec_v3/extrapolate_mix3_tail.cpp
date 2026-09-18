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

std::int32_t prediction(std::int32_t previous, std::int32_t prior) {
    return static_cast<std::int32_t>(
        4u * static_cast<std::uint32_t>(previous)
        - 3u * static_cast<std::uint32_t>(prior));
}

std::int32_t div4(std::int32_t previous, std::int32_t predictor) {
    const std::int32_t sum = static_cast<std::int32_t>(
        3u * static_cast<std::uint32_t>(predictor) + static_cast<std::uint32_t>(previous));
    const std::int32_t adjusted = sum >= 0 ? sum : static_cast<std::int32_t>(sum + 3);
    const std::uint32_t raw = static_cast<std::uint32_t>(adjusted);
    const std::uint32_t fill = adjusted < 0 ? 0xE0000000u : 0u;
    return static_cast<std::int32_t>((raw >> 2u) | fill);
}

} // namespace

bool encode_extrapolate_mix3_tail(
    const std::vector<std::int32_t>& carrier,
    const std::vector<std::int32_t>& primary,
    const std::vector<std::int32_t>& secondary,
    const std::vector<std::int32_t>& tertiary,
    const std::vector<std::int32_t>& baseline_primary,
    const std::vector<std::int32_t>& baseline_secondary,
    const std::vector<std::int32_t>& baseline_tertiary,
    ExtrapolateMix3State& state,
    std::vector<std::int32_t>& errors,
    std::string& error) {
    error.clear();
    errors.clear();
    const std::size_t count = carrier.size();
    if (primary.size() != count || secondary.size() != count || tertiary.size() != count ||
        baseline_primary.size() != count || baseline_secondary.size() != count ||
        baseline_tertiary.size() != count || state.phase < 3u) {
        error = "mix3 tail vectors or state are invalid";
        return false;
    }
    errors.reserve(count * 2u);
    for (std::size_t index = 0; index < count; ++index) {
        const std::int32_t next_predictor = prediction(state.last_a, state.last_b);
        const std::int32_t predicted_secondary = div4(state.last_b, next_predictor);
        const std::uint32_t phase = state.phase % 3u;
        std::int32_t first_error = 0;
        std::int32_t second_error = 0;
        std::int32_t expected_fixed = 0;
        if (phase == 0u) {
            expected_fixed = state.predictor;
            if (baseline_primary[index] != expected_fixed || baseline_secondary[index] != predicted_secondary) {
                error = "mix3 tail phase zero baseline violates native predictor";
                return false;
            }
            if (primary[index] != baseline_primary[index]) {
                error = "mix3 tail phase zero primary must remain unmodified";
                return false;
            }
            first_error = sub(secondary[index], baseline_secondary[index]);
            second_error = sub(tertiary[index], baseline_tertiary[index]);
        } else if (phase == 1u) {
            expected_fixed = state.predictor;
            if (baseline_secondary[index] != expected_fixed || baseline_tertiary[index] != predicted_secondary) {
                error = "mix3 tail phase one baseline violates native predictor";
                return false;
            }
            if (secondary[index] != baseline_secondary[index]) {
                error = "mix3 tail phase one secondary must remain unmodified";
                return false;
            }
            first_error = sub(primary[index], baseline_primary[index]);
            second_error = sub(tertiary[index], baseline_tertiary[index]);
        } else {
            expected_fixed = state.predictor;
            if (baseline_tertiary[index] != expected_fixed || baseline_primary[index] != predicted_secondary) {
                error = "mix3 tail phase two baseline violates native predictor";
                return false;
            }
            if (tertiary[index] != baseline_tertiary[index]) {
                error = "mix3 tail phase two tertiary must remain unmodified";
                return false;
            }
            first_error = sub(primary[index], baseline_primary[index]);
            second_error = sub(secondary[index], baseline_secondary[index]);
        }
        const std::int32_t reconstructed = add(
            add(add(baseline_primary[index], baseline_secondary[index]), baseline_tertiary[index]),
            add(first_error, second_error));
        if (reconstructed != carrier[index]) {
            error = "mix3 tail baseline and residuals do not reconstruct carrier";
            return false;
        }
        errors.push_back(first_error);
        errors.push_back(second_error);

        const std::int32_t old_last_c = state.last_c;
        const std::int32_t old_predictor = state.predictor;
        const std::int32_t updated_v35 = phase == 0u ? baseline_tertiary[index]
            : phase == 1u ? baseline_primary[index] : baseline_secondary[index];
        state.last_a = updated_v35;
        state.last_b = old_last_c;
        state.last_c = old_predictor;
        state.predictor = next_predictor;
        ++state.phase;
    }
    return true;
}

} // namespace auro3d:encode
