#pragma once

#include "adol_syntax.hpp"
#include "channel_payload.hpp"
#include "frame_descriptor.hpp"
#include "pcm_mux.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace auro3d::encode {

constexpr std::uint16_t kCodecV3SerializedChannelHeader = 0x010Au;

/// One fully serialized codec-v3 channel payload. `words` are the PCM24
/// carrier samples consumed by ChannelParser; `crc_word` is stored in the
/// channel slot beside that sample span.
struct EncodedChannelFrame {
    std::uint32_t channel_id = 0;
    std::uint32_t quantization_shift = 0;
    std::uint16_t channel_header = 0;
    bool a3d_config_flag = false;
    std::array<std::uint32_t, 3> metadata_words{};
    std::vector<std::int32_t> words;
    std::uint32_t word_count = 0;
    std::uint64_t serialized_bits = 0;
    std::uint16_t crc_word = 0;
    bool base_scaler_present = false;
    std::uint8_t base_scaler_index = 0;
};

/// Per-source primary downmix from compose::Channel helpers @ 0x5181E0 /
/// 0x518360 / 0x518630 via from_primary_downmix_gain @ 0x514F50. Becomes
/// channel-parser opcode 64 (channel id + scaler index).
struct PrimaryDownmixGain {
    std::uint32_t channel_id = 0;
    std::uint8_t scaler_index = 0;
};

/// Collects non-zero Encoder+6368 original-map entries for the group's
/// analysis source ids. Empty when `input_scaler_indices` is null.
bool collect_primary_downmix_gains(
    const std::vector<std::uint32_t>& source_ids,
    const std::array<std::uint8_t, 31>* input_scaler_indices,
    std::vector<PrimaryDownmixGain>& out,
    std::string& error);

/// Serializes the channel header, metadata, extrapolate seeds, context and
/// parser stream in the exact order consumed by ChannelParser_process. The
/// caller owns mode selection and supplies every field; no entropy or mix
/// policy is inferred here.
bool encode_channel_frame(
    std::uint32_t channel_id,
    std::uint32_t quantization_shift,
    std::uint16_t channel_header,
    std::uint32_t bit_width,
    std::uint32_t selector,
    const std::array<std::uint32_t, 3>& channel_ids,
    std::uint32_t third_word,
    std::uint32_t mode,
    const std::vector<std::int32_t>& seeds,
    const std::vector<std::uint32_t>& context_words,
    const std::vector<std::uint32_t>& stream_words,
    EncodedChannelFrame& out,
    std::string& error);

/// Variant of encode_channel_frame for the variable-width parser instruction
/// stream. The sequence must include its native opcode-zero terminator.
bool encode_channel_frame_sequence(
    std::uint32_t channel_id,
    std::uint32_t quantization_shift,
    std::uint16_t channel_header,
    std::uint32_t bit_width,
    std::uint32_t selector,
    const std::array<std::uint32_t, 3>& channel_ids,
    std::uint32_t third_word,
    std::uint32_t mode,
    const std::vector<std::int32_t>& seeds,
    const std::vector<std::uint32_t>& context_words,
    const std::vector<ChannelParserInstruction>& instructions,
    EncodedChannelFrame& out,
    std::string& error);

/// Serializes the native arity-1/direct channel form.  Its metadata has no
/// VQ context (bit width zero, selector zero); the parser therefore advances
/// directly from the metadata prefix to the empty stream and validates only
/// the channel CRC.  This is the encoder-side counterpart of Mixer case 1.
bool encode_direct_channel_frame(
    std::uint32_t channel_id,
    std::uint32_t quantization_shift,
    std::uint16_t channel_header,
    std::uint32_t source_channel_id,
    bool base_scaler_present,
    std::uint8_t base_scaler_index,
    const std::vector<PrimaryDownmixGain>& primary_downmix_gains,
    const std::vector<AdolInstruction>& common_adol,
    const std::vector<AdolInstruction>& optional_adol,
    EncodedChannelFrame& out,
    std::string& error);

/// Serializes a mode-2 or mode-3 channel frame from an explicit signed
/// residual codebook.  The parser stream remains caller-owned: this helper
/// only joins the native metadata/context prefix with the supplied stream.
bool encode_codebook_channel_frame(
    std::uint32_t channel_id,
    std::uint32_t quantization_shift,
    std::uint16_t channel_header,
    std::uint32_t mode,
    std::uint32_t bit_width,
    const std::vector<std::int32_t>& values0,
    const std::vector<std::int32_t>& values1,
    const std::array<std::uint32_t, 3>& channel_ids,
    std::uint32_t third_word,
    const std::vector<std::int32_t>& seeds,
    const std::vector<std::uint32_t>& stream_words,
    EncodedChannelFrame& out,
    std::string& error);

/// Codebook-channel counterpart that writes the native parser opcode stream.
bool encode_codebook_channel_frame_sequence(
    std::uint32_t channel_id,
    std::uint32_t quantization_shift,
    std::uint16_t channel_header,
    std::uint32_t mode,
    std::uint32_t bit_width,
    const std::vector<std::int32_t>& values0,
    const std::vector<std::int32_t>& values1,
    const std::array<std::uint32_t, 3>& channel_ids,
    std::uint32_t third_word,
    const std::vector<std::int32_t>& seeds,
    const std::vector<ChannelParserInstruction>& instructions,
    EncodedChannelFrame& out,
    std::string& error);

/// Codebook-channel path using the native Golomb-Rice index stream. The
/// metadata static flags carry `golomb_parameter` (0..15) and the stream
/// contains one index per decoded sample.
bool encode_codebook_channel_frame_golomb(
    std::uint32_t channel_id,
    std::uint32_t quantization_shift,
    std::uint16_t channel_header,
    std::uint32_t mode,
    std::uint32_t bit_width,
    const std::vector<std::int32_t>& values0,
    const std::vector<std::int32_t>& values1,
    const std::array<std::uint32_t, 3>& channel_ids,
    std::uint32_t third_word,
    std::uint32_t golomb_parameter,
    bool base_scaler_present,
    std::uint8_t base_scaler_index,
    const std::vector<PrimaryDownmixGain>& primary_downmix_gains,
    const std::vector<AdolInstruction>& common_adol,
    const std::vector<AdolInstruction>& optional_adol,
    const std::vector<std::int32_t>& seeds,
    const std::vector<std::uint32_t>& indices,
    EncodedChannelFrame& out,
    std::string& error);

/// Extends a serialized payload to the unit sample span with zero carrier
/// words and recomputes the channel CRC over the complete span.  Composer
/// writes a fixed unit-sized channel range even when the parser payload ends
/// earlier; this helper keeps that padding explicit and deterministic.
bool finalize_channel_frame_span(
    std::uint32_t frame_count,
    EncodedChannelFrame& frame,
    std::string& error);

/// ORs the serialized channel payload into the native carrier PCM span.  The
/// composer treats payload words as occupied carrier bit positions; source
/// PCM remains untouched in every other bit and sample.
bool merge_channel_frame_payload(
    std::vector<std::int32_t>& carrier_samples,
    const EncodedChannelFrame& frame,
    std::string& error);

/// Ports compose::Channel::mux @ 0x518C50's pre-serializer word transform.
/// The native channel bit depth is 24 and the configured group shift selects
/// the number of leading carrier bits retained before mux metadata is added.
bool prepare_channel_mux_words(
    const std::vector<std::int32_t>& source_samples,
    std::uint32_t quantization_shift,
    bool shift_left,
    std::vector<std::int32_t>& mux_words,
    std::string& error);

/// Complete scalar counterpart of compose::Channel::mux @ 0x518C50: apply
/// the native 24-bit shift/mask transform and serialize the projector records
/// into the resulting PCM words. `shift_left` is the native boolean branch.
bool mux_channel_words(
    const std::vector<std::int32_t>& source_samples,
    std::uint32_t quantization_shift,
    bool shift_left,
    const std::vector<PcmMuxProjectorRecord>& records,
    bool append_terminator,
    std::vector<std::int32_t>& mux_words,
    std::uint16_t& crc,
    std::string& error);

} // namespace auro3d::encode
