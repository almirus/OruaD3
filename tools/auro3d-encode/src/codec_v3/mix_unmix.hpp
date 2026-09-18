#pragma once

#include "mix_mix2.hpp"
#include "mix_mix3.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Scalar port of `auro:mix:unmix<...,array<int,1>>`.
bool mix2_unmix_reconstruct(
    const std::vector<std::int32_t>& carrier,
    const std::vector<std::uint64_t>& indices,
    const std::vector<std::int32_t>& residuals,
    const Mix2MixerSeeds& seeds,
    std::vector<std::int32_t>& primary,
    std::vector<std::int32_t>& secondary,
    std::string& error);

/// Scalar port of `auro:mix:unmix<...,array<int,2>>` and the
/// three `mix3:Train:proceed<N>` specializations.
bool mix3_unmix_reconstruct(
    const std::vector<std::int32_t>& carrier,
    const std::vector<std::uint64_t>& indices,
    const std::vector<std::array<std::int32_t, 2>>& residuals,
    const Mix3MixerSeeds& seeds,
    std::vector<std::int32_t>& primary,
    std::vector<std::int32_t>& secondary,
    std::vector<std::int32_t>& tertiary,
    std::string& error);

} // namespace auro3d:encode
