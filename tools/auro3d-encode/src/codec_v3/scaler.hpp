#pragma once

#include <cstdint>

namespace auro3d::encode {

/// Exact scalar gain conversion from auro::scaler::gain_to_scaler @ 0x53D830.
/// Gains at or above +144 dB map to zero; -infinity naturally maps to +inf.
bool scaler_from_gain_db(float gain_db, float& scaler);

/// Exact auro::scaler::ix_to_scaler @ 0x53D7F0. Index 255 is the native
/// positive-infinity sentinel; indices 0..240 use the embedded table.
bool scaler_from_index(std::uint8_t index, float& scaler);

/// Exact lower-bound selection performed by scaler_to_ix @ 0x53D750.
/// Accepted scalers are in [1,16]; the returned table value is the first
/// native scaler greater than or equal to the requested value.
bool scaler_to_index(
    float requested,
    std::uint8_t& index,
    float& scaler);

/// Exact auro::scaler::validate_ix @ 0x53D7E0. This validator clamps 0xF0
/// and 0xFF to 0xF0; callers that need the infinity sentinel must use
/// scaler_from_index before validation.
std::uint8_t validate_scaler_index(std::uint8_t index);

} // namespace auro3d::encode
