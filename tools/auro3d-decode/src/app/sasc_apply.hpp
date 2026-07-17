#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace auro3d {
namespace sasc {

constexpr std::int32_t kUnityGain = 0x800000;
constexpr std::int32_t kMuteGain = 0;

struct Step {
    std::uint32_t src_stream = 0;
    std::uint32_t dst_stream = 0;
    std::int32_t gain = kUnityGain;
    std::uint32_t layer = 0;
    bool disabled = false;
};

// Apply SCG mixing steps to one access-unit slice of per-stream PCM.
// Empty steps is a no-op. Matches scg::details::decode (0x495460):
// cross-stream cancel when step.layer < config_layer; same-stream gain when
// step.layer >= config_layer. For full discrete schema-bed WAV use
// config_layer=2 (not PDU source_layer).
bool apply_steps(
    std::vector<std::vector<std::int32_t>>& streams,
    const std::vector<std::uint32_t>& stream_bitdepths,
    const std::vector<Step>& steps,
    std::size_t frame_offset,
    std::size_t frame_length,
    std::string& error,
    std::optional<std::uint32_t> config_layer = std::nullopt);

} // namespace sasc
} // namespace auro3d
