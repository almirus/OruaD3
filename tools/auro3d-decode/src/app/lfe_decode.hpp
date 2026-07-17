#pragma once

#include "cx_bits.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d {
namespace lfe {

struct InterpolationStageState {
    std::uint32_t factor = 0;
    std::vector<float> coefficients;
    std::vector<float> history;
};

struct InterpolationState {
    std::uint32_t factor = 0;
    std::uint32_t next_phase = 0;
    bool phase_valid = false;
    std::vector<InterpolationStageState> stages;
    std::vector<float> pending;
};

// Decode one LFE stream residual block from the PDU payload bitstream.
// Matches lfe::Processor::parse_ signed residual read (~0x463AC0).
bool decode_stream_residuals(
    cx::Bits& bits,
    std::size_t sample_count,
    std::vector<std::int32_t>& out,
    std::string& error);

// Decode LFE PDU payload for all streams covered by the PDU.
bool decode_lfe_payload(
    cx::Bits& bits,
    std::uint32_t block_size,
    std::uint32_t resample_factor,
    std::uint32_t first_stream_index,
    std::uint32_t pdu_stream_count,
    std::vector<InterpolationState>& interpolation_states,
    std::vector<std::vector<std::int32_t>>& stream_samples,
    std::string& error);

std::uint32_t default_resample_factor(std::uint32_t sample_rate);

} // namespace lfe
} // namespace auro3d
