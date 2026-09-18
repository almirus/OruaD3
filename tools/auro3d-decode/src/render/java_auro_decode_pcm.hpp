#pragma once

#include <cstdint>
#include <vector>

namespace auro3d {

/// Соответствует входному циклу Java_com_google_android_exoplayer2_ext_auro3d_
/// AuroAudioProcessor_AuroDecode (:): interleaved s24le → planar int32.
void unpack_exoplayer_s24le_interleaved_to_planar_i32(
    const std::uint8_t* src,
    unsigned channel_count,
    unsigned block_size,
    std::vector<std::int32_t>& planar_out);

} // namespace auro3d
