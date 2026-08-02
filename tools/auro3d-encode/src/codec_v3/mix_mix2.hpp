#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Seeds produced by mix2 Mixer @ 0x4EAAC0 / auro::mix::mix @ 0x4EE540.
struct Mix2MixerSeeds {
    std::int32_t seed0 = 0;
    std::int32_t seed1 = 0;
};

/// Direct scalar port of auro::mix::mix mix2 overload @ 0x4EE540 plus the
/// trailing seed/carrier fixups in that function. `indices` selects entries in
/// `residuals` (native vector<array<int,1>> / vector<unsigned long>).
/// Does not invent residuals: the caller must supply Quantization/GVM output.
bool mix2_mixer_reconstruct(
    const std::vector<std::int32_t>& primary,
    const std::vector<std::int32_t>& secondary,
    const std::vector<std::uint64_t>& indices,
    const std::vector<std::int32_t>& residuals,
    std::vector<std::int32_t>& carrier,
    Mix2MixerSeeds& seeds,
    std::string& error);

} // namespace auro3d::encode
