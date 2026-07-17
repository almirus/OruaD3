#include "sasc_resample.hpp"

#include <algorithm>
#include <limits>

namespace auro3d {
namespace sasc {

namespace {

// sample_rate::filter::s / s_tilde, stored as sign-extended 64-bit values in
// libauro and selected by Synthesizer<int>::initialize (0x4990B0).
constexpr std::array<std::int32_t, 16> kS = {{
    -54, 928, -7678, 40666, -156853, 483108, -1341966, 5176153,
    5176153, -1341966, 483108, -156853, 40666, -7678, 928, -54
}};
constexpr std::array<std::int32_t, 16> kSTilde = {{
    -27, 464, -3839, 20333, -78427, 241554, -670983, 2588077,
    2588077, -670983, 241554, -78427, 20333, -3839, 464, -27
}};

std::int32_t wrap_add(std::int32_t a, std::int32_t b) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(a) + static_cast<std::uint32_t>(b));
}

std::int32_t wrap_sub(std::int32_t a, std::int32_t b) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(a) - static_cast<std::uint32_t>(b));
}

std::int32_t convolve(
    Fir16State& state,
    const std::array<std::int32_t, 16>& coefficients,
    std::int32_t input) {
    state.history[state.position] = input;
    std::int64_t sum = 0;
    std::size_t sample = (state.position + 1u) & 15u;
    for (std::size_t coefficient = 0; coefficient < 16u; ++coefficient) {
        sum += static_cast<std::int64_t>(coefficients[coefficient]) *
               static_cast<std::int64_t>(state.history[sample]);
        sample = (sample + 1u) & 15u;
    }
    state.position = (state.position + 1u) & 15u;
    return static_cast<std::int32_t>(sum / (std::int64_t{1} << 23u));
}

std::int32_t delay(DelayState& state, std::int32_t input) {
    if (state.history.empty())
        return input;
    const std::int32_t output = state.history[state.position];
    state.history[state.position] = input;
    state.position = (state.position + 1u) % state.history.size();
    return output;
}

void initialize_delay(DelayState& state, std::size_t length) {
    state.history.assign(length, 0);
    state.position = 0;
}

} // namespace

void reset_factor2(Factor2State& state, std::uint32_t initial_zero_count) {
    state = Factor2State{};
    initialize_delay(state.delay_first, 8u);
    initialize_delay(state.delay_second, 15u);
    state.initial_zero_count = initial_zero_count;
    state.initialized = true;
}

void reset_factor4(Factor4State& state) {
    state = Factor4State{};
    reset_factor2(state.first_stage, 32u);
    reset_factor2(state.second_stage, 96u);
    state.initialized = true;
}

bool synthesize_factor2(
    Factor2State& state,
    const std::vector<std::int32_t>& first,
    const std::vector<std::int32_t>& second,
    std::vector<std::int32_t>& output,
    std::string& error) {
    error.clear();
    if (first.size() != second.size()) {
        error = "SASC factor2 input size mismatch";
        return false;
    }
    if (first.size() > std::numeric_limits<std::size_t>::max() / 2u) {
        error = "SASC factor2 output size overflow";
        return false;
    }
    if (!state.initialized)
        reset_factor2(state);

    std::vector<std::int32_t> lifted_first(first);
    std::vector<std::int32_t> lifted_second(second);
    for (std::size_t i = 0; i < first.size(); ++i) {
        lifted_second[i] = wrap_add(
            lifted_second[i], convolve(state.s_tilde, kSTilde, lifted_first[i]));
        lifted_second[i] = delay(state.delay_second, lifted_second[i]);
    }
    for (std::size_t i = 0; i < first.size(); ++i) {
        lifted_first[i] = wrap_sub(
            lifted_first[i], convolve(state.s, kS, lifted_second[i]));
        lifted_first[i] = delay(state.delay_first, lifted_first[i]);
    }

    output.resize(first.size() * 2u);
    if (!first.empty()) {
        output[0] = state.prior_second;
        std::size_t destination = 1u;
        for (std::size_t i = 0; i + 1u < first.size(); ++i) {
            output[destination++] = lifted_first[i];
            output[destination++] = lifted_second[i];
        }
        output[destination] = lifted_first.back();
        state.prior_second = lifted_second.back();
    }

    const std::size_t zero_count = std::min<std::size_t>(
        state.initial_zero_count, output.size());
    std::fill(output.begin(), output.begin() + static_cast<std::ptrdiff_t>(zero_count), 0);
    state.initial_zero_count -= static_cast<std::uint32_t>(zero_count);
    return true;
}

bool synthesize_factor4(
    Factor4State& state,
    const std::vector<std::int32_t>& first_level,
    const std::vector<std::int32_t>& second_level,
    const std::vector<std::int32_t>& base,
    std::vector<std::int32_t>& output,
    std::string& error) {
    if (first_level.size() != base.size()) {
        error = "SASC factor4 first-stage input size mismatch";
        return false;
    }
    if (base.size() > std::numeric_limits<std::size_t>::max() / 2u ||
        second_level.size() != base.size() * 2u) {
        error = "SASC factor4 second-stage input size mismatch";
        return false;
    }
    if (!state.initialized)
        reset_factor4(state);

    std::vector<std::int32_t> intermediate;
    if (!synthesize_factor2(
            state.first_stage, first_level, base, intermediate, error))
        return false;
    return synthesize_factor2(
        state.second_stage, second_level, intermediate, output, error);
}

} // namespace sasc
} // namespace auro3d
