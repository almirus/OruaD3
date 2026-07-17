#include "sasc_apply.hpp"

#include <algorithm>

namespace auro3d {
namespace sasc {
namespace {

std::int32_t mul_gain(std::int32_t sample, std::int64_t gain) {
    const std::int64_t prod = static_cast<std::int64_t>(sample) * gain;
    const std::uint64_t rounded = static_cast<std::uint64_t>(
        prod < 0 ? prod + 0x7FFFFF : prod);
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(rounded >> 23));
}

bool align_gain_bitdepth(
    std::int32_t gain,
    std::uint32_t dst_bitdepth,
    std::uint32_t src_bitdepth,
    std::int64_t& aligned) {
    if (dst_bitdepth >= src_bitdepth) {
        const unsigned shift = dst_bitdepth - src_bitdepth;
        if (shift >= 64u)
            return false;
        aligned = static_cast<std::int64_t>(
            static_cast<std::uint64_t>(static_cast<std::int64_t>(gain)) << shift);
        return true;
    }
    const unsigned shift = src_bitdepth - dst_bitdepth;
    if (shift >= 64u)
        return false;
    const std::int64_t value = gain;
    const std::uint64_t remainder_mask = (std::uint64_t{1} << shift) - 1u;
    aligned = (value + static_cast<std::int64_t>(
        remainder_mask & static_cast<std::uint64_t>(value >> 63))) >> shift;
    return true;
}

std::int32_t add_wrapped(std::int32_t lhs, std::int32_t rhs) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(lhs) + static_cast<std::uint32_t>(rhs));
}

void apply_gain_slice(
    std::vector<std::int32_t>& dst,
    std::size_t frame_offset,
    std::size_t frame_length,
    std::int32_t gain) {
    const std::size_t end = frame_offset + frame_length;
    for (std::size_t i = frame_offset; i < end; ++i)
        dst[i] = mul_gain(dst[i], gain);
}

void apply_gain_and_add_slice(
    std::vector<std::int32_t>& dst,
    const std::vector<std::int32_t>& src,
    std::size_t frame_offset,
    std::size_t frame_length,
    std::int64_t gain) {
    const std::size_t end = frame_offset + frame_length;
    for (std::size_t i = frame_offset; i < end; ++i)
        dst[i] = add_wrapped(dst[i], mul_gain(src[i], gain));
}

bool contains_slice(
    const std::vector<std::int32_t>& stream,
    std::size_t frame_offset,
    std::size_t frame_length) {
    return frame_offset <= stream.size() &&
           frame_length <= stream.size() - frame_offset;
}

} // namespace

bool apply_steps(
    std::vector<std::vector<std::int32_t>>& streams,
    const std::vector<std::uint32_t>& stream_bitdepths,
    const std::vector<Step>& steps,
    std::size_t frame_offset,
    std::size_t frame_length,
    std::string& error,
    std::optional<std::uint32_t> config_layer) {
    if (steps.empty() || !frame_length)
        return true;

    const std::uint32_t layer = config_layer.value_or(0);
    for (auto it = steps.rbegin(); it != steps.rend(); ++it) {
        const Step& step = *it;
        if (step.disabled)
            continue;
        if (step.dst_stream >= streams.size()) {
            error = "SASC dst stream out of range";
            return false;
        }
        if (!contains_slice(streams[step.dst_stream], frame_offset, frame_length)) {
            error = "SASC dst stream slice out of range";
            return false;
        }
        if (step.src_stream != step.dst_stream && step.layer < layer) {
            if (step.src_stream >= streams.size()) {
                error = "SASC src stream out of range";
                return false;
            }
            if (!contains_slice(streams[step.src_stream], frame_offset, frame_length)) {
                error = "SASC src stream slice out of range";
                return false;
            }
            if (step.src_stream >= stream_bitdepths.size() ||
                step.dst_stream >= stream_bitdepths.size()) {
                error = "SASC stream bitdepth out of range";
                return false;
            }
            std::int64_t aligned_gain = 0;
            if (!align_gain_bitdepth(
                -step.gain,
                stream_bitdepths[step.dst_stream],
                stream_bitdepths[step.src_stream],
                aligned_gain)) {
                error = "SASC stream bitdepth delta out of range";
                return false;
            }
            apply_gain_and_add_slice(
                streams[step.dst_stream],
                streams[step.src_stream],
                frame_offset,
                frame_length,
                aligned_gain);
        } else if (step.src_stream == step.dst_stream && step.layer >= layer) {
            if (step.gain == kUnityGain)
                continue;
            apply_gain_slice(streams[step.dst_stream], frame_offset, frame_length, step.gain);
        }
    }
    return true;
}

} // namespace sasc
} // namespace auro3d
