#pragma once

#include "flac_output.hpp"
#include "wav_output.hpp"
#include "../codec_v3/carrier_frame.hpp"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace auro3d::encode {

/// Selects the lossless PCM carrier container from the output extension while
/// keeping channel order and sample-count checks identical for WAV and FLAC.
class CarrierOutput {
public:
    bool open(
        const std::string& path,
        std::uint32_t sample_rate,
        const std::vector<std::uint32_t>& channel_order,
        std::uint64_t frame_count,
        std::uint32_t channel_mask,
        std::string& error);
    bool write(
        const std::vector<std::vector<std::int32_t>>& planes,
        std::uint32_t frame_count,
        std::string& error);
    /// Runs the same complete channel/header/ADOL/CRC checks as write_unit
    /// without requiring an output container or emitting samples.
    bool validate_unit(const EncodedCarrierUnit& unit, std::string& error);
    bool write_unit(const EncodedCarrierUnit& unit, std::string& error);
    bool close(std::string& error);

private:
    bool validation_only_ = false;
    std::uint64_t remaining_output_frames_ = 0u;
    std::variant<std::monostate, WavPcm24Writer, FlacPcm24Writer> writer_;
};

} // namespace auro3d::encode
