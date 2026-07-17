#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d {
namespace sasc {

struct Fir16State {
    std::array<std::int32_t, 16> history{};
    std::size_t position = 0;
};

struct DelayState {
    std::vector<std::int32_t> history;
    std::size_t position = 0;
};

struct Factor2State {
    Fir16State s;
    Fir16State s_tilde;
    DelayState delay_first;
    DelayState delay_second;
    std::int32_t prior_second = 0;
    std::uint32_t initial_zero_count = 0;
    bool initialized = false;
};

struct Factor4State {
    Factor2State first_stage;
    Factor2State second_stage;
    bool initialized = false;
};

void reset_factor2(Factor2State& state, std::uint32_t initial_zero_count = 32u);
void reset_factor4(Factor4State& state);

bool synthesize_factor2(
    Factor2State& state,
    const std::vector<std::int32_t>& first,
    const std::vector<std::int32_t>& second,
    std::vector<std::int32_t>& output,
    std::string& error);

bool synthesize_factor4(
    Factor4State& state,
    const std::vector<std::int32_t>& first_level,
    const std::vector<std::int32_t>& second_level,
    const std::vector<std::int32_t>& base,
    std::vector<std::int32_t>& output,
    std::string& error);

} // namespace sasc
} // namespace auro3d
