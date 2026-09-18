#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Five state words emitted by the native mix3 mixer.
struct Mix3MixerSeeds {
    std::array<std::int32_t, 5> values{};
};

/// Direct scalar port of the native mix3 reconstruction path. `indices`
/// selects two-component residual entries, and every source plane must have
/// the same length as the carrier. The caller supplies the already trained
/// VQ residual table; this function performs no quantization.
bool mix3_mixer_reconstruct(
    const std::vector<std::int32_t>& primary,
    const std::vector<std::int32_t>& secondary,
    const std::vector<std::int32_t>& tertiary,
    const std::vector<std::uint64_t>& indices,
    const std::vector<std::array<std::int32_t, 2>>& residuals,
    std::vector<std::int32_t>& carrier,
    Mix3MixerSeeds& seeds,
    std::string& error);

/// Converts the eight-byte metadata residual representation used by
/// `auro_codec_v3_metadata_Mix3_t` into the native two-int residual entries.
bool unpack_mix3_residual_table(
    const std::vector<std::int64_t>& packed,
    std::vector<std::array<std::int32_t, 2>>& residuals,
    std::string& error);

bool pack_mix3_residual_table(
    const std::vector<std::array<std::int32_t, 2>>& residuals,
    std::vector<std::int64_t>& packed,
    std::string& error);

} // namespace auro3d:encode
