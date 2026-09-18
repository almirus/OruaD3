#pragma once

#include "carrier_frame.hpp"
#include "cts_dmx.hpp"
#include "cycler.hpp"
#include "encode_groups.hpp"
#include "encoder_config.hpp"
#include "native_dither.hpp"
#include "quality_filter.hpp"
#include "select_loudness.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

class EncoderThreadPool;

struct EncoderRuntimeState {
    std::uint64_t processed_frames = 0;
    /// Parameters fixed by the native Encoder instance. Unit block size is
    /// intentionally excluded because the final stream unit may be shorter.
    bool stream_identity_initialized = false;
    std::uint32_t stream_sample_rate = 0;
    std::uint32_t stream_original_layout = 0;
    std::uint32_t stream_carrier_layout = 0;
    std::uint32_t stream_metadata_channel_id = 0;
    /// Constructor-time Config fields, excluding UnitBlock size and the
    /// metadata/Dynamic values which native setters may update.
    EncoderConfig stream_initial_config{};
    EncoderCyclers metadata_cyclers{};
    /// Native Encoder+12680 loudness schedule. Empty until the first
    /// scheduled unit initializes the default six-entry table.
    std::vector<LoudnessScheduleEntry> loudness_schedule{};
    std::uint32_t loudness_schedule_rate = 0;
    /// Last loudness list applied through set_dynamic_params semantics.
    /// Reapplying a constant config would incorrectly re-enable one-shot
    /// types 4/5, while ignoring changes would lose legitimate updates.
    bool dynamic_loudness_initialized = false;
    bool dynamic_params_were_present = false;
    std::vector<LoudnessMeasurement> dynamic_loudness_measurements{};
    /// Native encode:Filter is keyed by carrier channel and survives unit
    /// boundaries. Each entry learns bit-line versus negated PCM error.
    std::array<NativeQualityFilter, 31> quality_filters{};
    /// Rescaler owns its per-shift dither pools for the encoder lifetime.
    NativeDitherState dither{};
    /// Native Encoder+6632 CTS limiter. Its smoothing and peak state crosses
    /// UnitBlocks whenever Config+136 enables coefficient limiting.
    CtsDmxLimiter cts_dmx_limiter{};
    /// Composer+8 delayable ADOL backlog. Instructions that do not fit a
    /// channel in the current unit are retried before new values next time.
    std::vector<AdolInstruction> pending_delayable_adol{};
};

/// Encodes one complete native codec-v3 UnitBlock through every currently
/// ported stage. The caller supplies the configuration carrier channel
/// selected by codec_v3_metadata_carrier_channel; an
/// optional explicit scaler table in EncoderConfig is applied to a private
/// working copy before group analysis. No tail padding or stream scheduling
/// is performed here.
bool encode_v3_complete_unit(
    const std::vector<std::vector<std::int32_t>>& unit_planes,
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    std::uint32_t sample_rate,
    const EncoderConfig& config,
    const std::vector<EncodeGroupPlan>& group_plan,
    std::uint32_t metadata_channel_id,
    EncodedCarrierUnit& out,
    std::string& error);

/// Stateful stream wrapper around encode_v3_complete_unit. It selects
/// optional metadata at the new cumulative sample position, advances all
/// configured native cyclers, and commits runtime state only after successful
/// encoding. Explicit metadata without a configured cycler remains present.
bool encode_v3_scheduled_unit(
    const std::vector<std::vector<std::int32_t>>& unit_planes,
    std::uint32_t original_layout,
    std::uint32_t carrier_layout,
    std::uint32_t sample_rate,
    const EncoderConfig& config,
    const std::vector<EncodeGroupPlan>& group_plan,
    std::uint32_t metadata_channel_id,
    EncoderRuntimeState& state,
    EncodedCarrierUnit& out,
    std::string& error,
    EncoderThreadPool* thread_pool = nullptr);

} // namespace auro3d:encode
