#pragma once

#include "channel_metadata.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

/// Creates the channel metadata/context prefix for a caller-supplied signed
/// residual codebook. The selector is derived only when the native base-index
/// table represents `values.size()` exactly; no layout or entropy policy is
/// inferred here.
bool make_channel_codebook_prefix(
    std::uint32_t mode,
    std::uint32_t bit_width,
    const std::vector<std::int32_t>& values0,
    const std::vector<std::int32_t>& values1,
    const std::array<std::uint32_t, 3>& channel_ids,
    std::uint32_t third_word,
    std::uint32_t golomb_parameter,
    std::uint32_t& selector,
    std::array<std::uint32_t, 3>& metadata_words,
    std::vector<std::uint32_t>& context_words,
    std::string& error);

} // namespace auro3d::encode
