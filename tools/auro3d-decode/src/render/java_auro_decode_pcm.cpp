#include "java_auro_decode_pcm.hpp"

namespace auro3d {

static std::int32_t s24le_triplet_to_i32(const std::uint8_t* p) {
    const unsigned b0 = p[0];
    const unsigned b1 = p[1];
    const unsigned b2 = p[2];
    const int v20 = static_cast<int>((b2 << 16) | (b1 << 8));
    int v21 = static_cast<int>(b0) + v20 - 0x1000000;
    if (static_cast<std::int8_t>(b2) >= 0)
        v21 = static_cast<int>(b0) + v20;
    return v21;
}

void unpack_exoplayer_s24le_interleaved_to_planar_i32(
    const std::uint8_t* src,
    unsigned channel_count,
    unsigned block_size,
    std::vector<std::int32_t>& planar_out) {
    planar_out.resize(static_cast<std::size_t>(channel_count) * block_size);
    if (channel_count == 0 || block_size == 0)
        return;
    const unsigned stride = 3u * channel_count;
    for (unsigned ch = 0; ch < channel_count; ++ch) {
        const std::size_t plane_base = static_cast<std::size_t>(ch) * block_size;
        unsigned byte_off = ch * 3u;
        for (unsigned s = 0; s < block_size; ++s) {
            planar_out[plane_base + s] = s24le_triplet_to_i32(src + byte_off);
            byte_off += stride;
        }
    }
}

} // namespace auro3d
