#include "extrapolate_mix2.hpp"

namespace auro3d::encode {
namespace {

std::int32_t wrap_add(std::int32_t left, std::int32_t right) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) + static_cast<std::uint32_t>(right));
}

std::int32_t wrap_sub(std::int32_t left, std::int32_t right) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) - static_cast<std::uint32_t>(right));
}

std::int32_t wrap_double_sub(std::int32_t base, std::int32_t prior) {
    return static_cast<std::int32_t>(
        2u * static_cast<std::uint32_t>(base) - static_cast<std::uint32_t>(prior));
}

bool carrier_matches_components(
    std::int32_t carrier,
    std::int32_t primary,
    std::int32_t secondary) {
    return carrier == wrap_add(primary, secondary);
}

} // namespace

bool encode_extrapolate_mix2_errors(
    const std::vector<std::int32_t>& carrier,
    const std::vector<std::int32_t>& primary,
    const std::vector<std::int32_t>& secondary,
    const ExtrapolateMix2Seeds& seeds,
    ExtrapolateMix2State& state,
    std::vector<std::int32_t>& errors,
    std::string& error) {
    error.clear();
    errors.clear();
    if (carrier.size() != primary.size() || carrier.size() != secondary.size()) {
        error = "mix2 carrier and component lengths differ";
        return false;
    }
    errors.reserve(carrier.size());
    for (std::size_t index = 0; index < carrier.size(); ++index) {
        if (!carrier_matches_components(carrier[index], primary[index], secondary[index])) {
            error = "mix2 components do not sum to carrier";
            return false;
        }

        std::int32_t error_sample = 0;
        if (state.count == 0u) {
            // Decoder: secondary = seed0_secondary + error; primary = carrier - secondary.
            error_sample = wrap_sub(secondary[index], seeds.seed0_secondary);
            state.prev_primary = primary[index];
        } else if (state.count == 1u) {
            // Decoder: primary = seed0_primary + error; secondary = carrier - primary.
            error_sample = wrap_sub(primary[index], seeds.seed0_primary);
            state.last_residual = seeds.seed0_primary;
            state.prev_secondary = secondary[index];
        } else {
            const bool primary_is_predicted = (state.count & 1u) == 0u;
            const std::int32_t predicted = wrap_double_sub(state.last_residual, state.prev_primary);
            const std::int32_t selected_predicted = primary_is_predicted ? primary[index] : secondary[index];
            const std::int32_t selected_residual = primary_is_predicted ? secondary[index] : primary[index];
            if (selected_predicted != predicted) {
                error = "mix2 component does not match native predictor";
                return false;
            }
            error_sample = wrap_sub(selected_residual, state.last_residual);
            const std::int32_t prior_secondary = state.prev_secondary;
            state.prev_primary = prior_secondary;
            state.last_residual = selected_residual;
            state.prev_secondary = selected_predicted;
        }
        errors.push_back(error_sample);
        ++state.count;
    }
    return true;
}

} // namespace auro3d::encode
