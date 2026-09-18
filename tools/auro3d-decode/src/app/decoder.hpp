#pragma once

#include "../auro3deng/detail/runtime_api.hpp"
#include "../auro3deng/detail/processor_io.hpp"

#include "progress.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace auro3d {

constexpr unsigned kLegacyJniBlockSizeV3 = 1024u;
constexpr unsigned kDefaultJniBlockSize = 832u;
constexpr unsigned kEngineV4ChannelLimit = 31u;
constexpr unsigned kDefaultOutputChannels = 2u;
constexpr unsigned kCurrentNativeExportChannelLimit = 27u;
constexpr unsigned kDefaultRoomPreset = 0u;         // HOME
constexpr unsigned kDefaultHrtfPreset = 0u;         // HPV2
constexpr unsigned kDefaultVirtualizerMode = 0u;    // ENABLED

const char* auro_channel_layout_to_string(std::uint32_t layout);

struct DecoderConfig {
    uint32_t sample_rate = 0;
    uint16_t channels = 0;
    uint16_t bits_per_sample = 16;
    uint32_t channel_mask = 0;
    uint32_t block_size = 0;
};

struct AuroAdolInstructionInfo {
    std::uint32_t sync_block_index = 0;
    std::uint32_t block_index = 0;
    std::uint32_t tag = 0;
    std::uint32_t opcode = 0;
    bool has_value = false;
    std::uint32_t value = 0;
    std::uint8_t value_bits = 0;
    bool has_value2 = false;
    std::uint32_t value2 = 0;
    std::uint8_t value2_bits = 0;
    bool has_decoded_layout = false;
    std::uint32_t decoded_layout = 0;
    bool has_carrier_layout = false;
    std::uint32_t carrier_layout = 0;
    bool has_primary_downmix_gain = false;
    std::uint8_t primary_downmix_channel = 0;
    std::uint8_t primary_downmix_scaler = 0;
    bool has_limit_simple = false;
    std::uint8_t limit_simple_scaler = 0;
    bool has_secondary_downmix_gains = false;
    std::uint32_t secondary_downmix_packed = 0;
    bool has_auromatic = false;
    std::uint8_t auromatic_profile = 0;
    std::uint8_t auromatic_mode = 0;
    bool has_encoder_version = false;
    std::uint32_t encoder_version = 0;
    bool has_loudness = false;
    std::uint32_t loudness_raw = 0;
    std::uint32_t occurrences = 1;
};

struct AuroMetadataInfo {
    bool found = false;
    std::uint32_t layout_id = 0;
    std::string layout_name;
    unsigned output_channels = 0;
    std::uint32_t carrier_layout_id = 0;
    std::string carrier_layout_name;
    unsigned carrier_channels = 0;
    bool uses_mix3 = false;
    bool has_closest_layout_without_mix3 = false;
    std::uint32_t closest_layout_without_mix3 = 0;
    std::string closest_layout_without_mix3_name;
    unsigned carrier_channel = 0;
    std::uint64_t sync_sample = 0;
    unsigned block_size = 0;
    std::uint32_t scanned_sync_blocks = 0;
    std::uint32_t adol_block_count = 0;
    std::vector<AuroAdolInstructionInfo> adol_instructions;
};

struct NativeDecoderConfigState {
    std::uint64_t block_bits = 0;
    std::uint32_t block_words = 0;
    std::uint64_t buffer_count = 0;
    std::uint32_t stage0_count = 0;
    std::uint32_t stage1_count = 0;
    std::uint32_t input_bytes_unit = 0;
    std::uint32_t input_mask = 0;
    std::uint32_t requested_output_mask = 0;
    std::uint32_t effective_output_mask = 0;
    std::uint32_t input_layout_dimension = 0;
    std::uint32_t requested_output_layout_dimension = 0;
    std::uint32_t effective_output_layout_dimension = 0;
    bool input_has_height_layer = false;
    bool requested_output_has_height_layer = false;
    bool effective_output_has_height_layer = false;
    std::uint32_t input_mask_count = 0;
    std::uint32_t requested_output_mask_count = 0;
    std::uint32_t effective_output_mask_count = 0;
    std::uint32_t extra_flags = 0;
    bool input_is_subset_of_effective_output = false;
};

struct NativeRuntimeConfigurationState {
    bool auro_update_is_stereo_device = false;
    bool auro_update_headset_connected = false;
    std::uint32_t auro_update_decoder_mode = 0;
    std::uint32_t auro_update_output_sample_type = 0;
    std::uint32_t auro_update_output_bit_depth = 0;
    std::uint32_t auro_update_output_layout_mask = 0;
    std::uint32_t auro_update_pcm_input_layout_mask = 0;
    std::uint32_t auro_update_pcm_input_sample_rate = 0;
    std::uint32_t auro_update_pcm_input_sample_type = 0;
    bool auro_update_channels_backs_before_surrounds = false;
    bool auro_update_virtualization_enabled = false;
    bool auro_update_listening_mode_auro3d = false;
    bool auro_update_abr_mode_enabled = false;
    std::uint32_t auro_update_hp_user_preset = 0;
    std::uint32_t auro_update_hp_hrtf_preset = 0;
    std::uint32_t virtualizer_mode = 0;
    std::uint32_t effective_virtualizer_mode = 1;
    std::uint32_t room_preset = 0;
    std::uint32_t hrtf_preset = 0;
    std::uint16_t output_audio_configuration_bits = 0;
    std::uint32_t derived_listening_mode = 2;
    std::uint32_t target_device = 0;
    bool dynamic_request_flag = false;
    bool dynamic_headphone_flag = false;
    bool is_abr = false;
    bool headphone_connected = false;
    bool stereo_device_connected = false;
};

struct NativeA3dengStaticConfigurationState {
    std::uint32_t renderer_count = 0;
    std::uint32_t pipeline_audio_block_size = 0;
    bool disable_limiter = false;
    float smoothing_s = 0.0f;
    bool hdmi_carrier_valid = true;
    bool hdmi_allow_2_0 = true;
    bool hdmi_allow_2_1 = true;
    bool hdmi_allow_4_0 = false;
    bool hdmi_allow_5_0 = false;
    bool hdmi_allow_5_1 = true;
    bool hdmi_allow_7_0 = false;
    bool hdmi_allow_7_1 = true;
    bool hdmi_disable_mix3 = false;
    bool hdmi_disable_transcoding_downmix_limiter = false;
    std::uint32_t hdmi_quality = 3;
    std::uint32_t diagnostics_mode = 0;
    std::uint32_t output_mode = 0;
    bool api_available = false;
    bool instance_created = false;
};

struct NativeDynamicParametersState {
    std::uint32_t effective_virtualizer_mode = 1;
    std::uint32_t listening_mode = 2;
    std::uint32_t room_preset = 0;
    std::uint32_t hrtf_preset = 0;
};

struct NativeA3dengRenderState {
    std::uint32_t decoder_mode = 2;
    std::uint32_t input_sample_rate = 0;
    std::uint32_t input_sample_type = 1;
    std::uint32_t input_channel_mask = 0;
    std::uint32_t input_channel_count = 0;
    std::uint32_t input_block_count = 0;
    std::size_t input_bytes_per_block = 0;
    std::uint32_t output_sample_rate = 0;
    std::uint32_t output_channel_mask = 0;
    std::uint32_t output_channel_count = 0;
    std::uint32_t output_sample_type = 1;
    std::uint32_t output_bit_depth = 24;
    std::uint32_t output_block_count = 0;
    std::uint32_t pruned_output_channel_mask = 0;
    std::uint32_t pruned_output_channel_count = 0;
    std::uint64_t pruned_output_max_sample = 0;
    bool pruned_output_info_valid = false;
    std::uint32_t pipeline_audio_block_size = 0;
    std::uint64_t output_info_packed = 0;
    std::size_t render_bytes_per_block = 0;
    std::size_t maximum_output_bytecount = 0;
    std::uint64_t maximum_output_bytecount_packed = 0;
    std::int64_t jni_maximum_output_bytecount_return = -1;
    std::int64_t pop_return_bytes = -1;
    bool render_available = false;
};

enum class DecodeError {
    Ok = 0,
    InitFailed,
    BadInput,
    IoError,
    NotImplemented,
    InvalidOutputLayout,
};

const char* decode_error_message(DecodeError e);

/// Resample interleaved PCM via FFmpeg. Output is always PCM24 LE at target_sample_rate.
bool resample_interleaved_pcm_to_rate(
    const std::vector<std::uint8_t>& pcm_in,
    unsigned bits_per_sample,
    unsigned channels,
    unsigned sample_rate_in,
    unsigned sample_rate_out,
    std::vector<std::uint8_t>& pcm_out,
    std::string& err,
    const ProgressFn& progress = {});

/// Весь PCM s24le из WAV (или сырой файл при raw: нужны sample_rate и channel_count) → interleaved int32 (24-bit sign-extended).
bool load_all_pcm_s24le_interleaved_i32(
    const std::string& path,
    bool raw,
    uint32_t raw_sample_rate,
    unsigned raw_channels,
    std::vector<std::int32_t>& interleaved_out,
    DecoderConfig& cfg_out,
    std::string& err);

/// Читает заголовок WAV с начала файла (наращивая буфер до появления chunk data). PCM = PCM 24-bit LE.
bool probe_wav_pcm_s24le(
    const std::string& path,
    std::size_t& pcm_byte_offset,
    std::size_t& pcm_byte_length,
    std::uint32_t& sample_rate,
    std::uint16_t& channels,
    std::string& err);

/// Общая подготовка для потокового чтения: WAV (probe) или raw (весь файл), проверка pcm_len % (3*channels)==0.
bool resolve_s24le_interleaved_pcm_region(
    const std::string& path,
    bool raw,
    std::uint32_t raw_sample_rate,
    unsigned raw_channels,
    std::size_t& pcm_byte_offset,
    std::size_t& pcm_byte_length,
    std::uint32_t& sample_rate_hz,
    unsigned& channel_count,
    std::string& err);

/// Непрерывное окно interleaved PCM24 → int32 (без полной загрузки файла).
bool read_pcm24_interleaved_frames_i32(
    const std::string& path,
    std::size_t pcm_byte_offset,
    std::size_t pcm_byte_length,
    unsigned channels,
    std::uint64_t first_frame,
    std::size_t frame_count,
    std::vector<std::int32_t>& interleaved_out,
    std::string& err);

/// Реализовано по: входной PCM24 слой и рабочий baseline decode/render path.
class Decoder {
public:
    /// Для сырого s24le без WAV: частота, каналы, размер блока (как в Java Initialize).
    void set_raw_pcm24_params(uint32_t sample_rate_hz, unsigned channel_count, unsigned block_size);

    /// Override the native 832-sample host block size.
    void set_block_size(unsigned block_size) { block_request_ = block_size; }

    void set_progress_callback(ProgressFn callback) { progress_ = std::move(callback); }

    /// 0..100 decode progress across input stream + latency drain.
    int decode_percent() const;

    /// Сила рендера baseline AURO-DSP (0..15, таблица strength_translate).
    void set_dsp_strength(unsigned strength) { dsp_strength_ = strength; }

    /// AURO export channel count. 0 = auto from Auro metadata.
    /// Missing height channels are generated by the native Auro-Matic/XinN path.
    void set_dsp_output_channels(unsigned channels) {
        dsp_output_channels_req_ = channels;
        dsp_output_layout_mask_req_ = 0;
        dsp_output_layout_mask_specified_ = false;
    }

    /// Explicit Auro output bitmask (native output_layout). 0 = auto.
    /// When set, overrides set_dsp_output_channels for layout selection.
    void set_dsp_output_layout_mask(std::uint32_t mask) {
        dsp_output_layout_mask_req_ = mask;
        dsp_output_layout_mask_specified_ = mask != 0u;
        if (mask != 0u) {
            unsigned n = 0;
            for (std::uint32_t m = mask; m != 0u; m >>= 1u)
                n += static_cast<unsigned>(m & 1u);
            dsp_output_channels_req_ = n;
        } else {
            dsp_output_channels_req_ = 0;
        }
    }

    /// Binaural output is rendered downstream by main.cpp; the decoder then
    /// emits the discrete input channels unchanged when no target is set.
    void set_binaural(bool on) { binaural_requested_ = on; }

    /// Path-specific detail for the last open/decode failure (may be empty).
    const std::string& last_error_detail() const { return last_error_detail_; }

    /// Post-dematrix legacy XinN upmix active after open.
    bool meta_auromatic_upmix() const { return meta_auromatic_upmix_; }
    bool legacy_auromatic_upmix() const { return legacy_auromatic_upmix_; }
    /// >1 when XinN ran at a lower rate than the host (e.g. 96→48).
    std::uint32_t meta_xinn_rate_decimation() const { return meta_xinn_rate_decimation_; }
    /// Core rate the Matic/XinN engine was configured at (0 = host rate).
    std::uint32_t meta_xinn_core_rate() const { return meta_xinn_core_rate_; }
    /// Legacy (no metadata) path: whole-file FFmpeg downsample before XinN.
    bool legacy_auromatic_ffmpeg_downsampled() const {
        return legacy_auromatic_ffmpeg_downsampled_;
    }
    std::uint32_t legacy_auromatic_source_rate_hz() const {
        return legacy_auromatic_source_rate_hz_;
    }

    /// Headroom в dB для baseline AURO-DSP (0..24), применяется как множитель к gain.
    void set_dsp_headroom_db(float db);
    void set_output_bits(unsigned bits) { output_bits_ = (bits == 24u) ? 24u : 16u; }

    void set_room_preset(unsigned preset);
    void set_hrtf_preset(unsigned preset);
    void set_virtualizer_mode(unsigned mode);
    void set_output_audio_devices(bool headphone_connected, bool stereo_device_connected);

    DecodeError open(const std::string& path);
    DecodeError decode_next(std::vector<std::uint8_t>& pcm_out);
    bool exhausted() const;
    DecoderConfig config() const;
    const NativeDecoderConfigState& native_config_state() const { return native_config_state_; }
    const NativeRuntimeConfigurationState& native_runtime_configuration() const { return native_runtime_configuration_; }
    const NativeA3dengStaticConfigurationState& native_a3deng_static_configuration() const { return native_a3deng_static_configuration_; }
    const NativeDynamicParametersState& native_dynamic_parameters() const { return native_dynamic_parameters_; }
    const NativeA3dengRenderState& native_a3deng_render_state() const { return native_a3deng_render_state_; }
    const AuroMetadataInfo& auro_metadata() const { return auro_metadata_; }
    const std::vector<std::uint32_t>& output_channel_slot_map() const { return output_channel_slot_map_; }
    std::uint64_t dsp_clipped_samples() const { return dsp_clipped_samples_; }
    std::uint64_t latency_samples() const { return codec_v3_latency_samples_; }
    std::uint64_t source_sample_count() const { return source_sample_count_; }
    void close();

private:
    void rebuild_native_io_descriptors();
    void rebuild_native_config_state();
    void rebuild_native_a3deng_static_configuration_state();
    void rebuild_native_runtime_configuration_state();
    void rebuild_native_dynamic_parameters_state();
    void rebuild_native_a3deng_render_state();
    void rebuild_a3deng_partial_blob();
    bool apply_native_dynamic_parameters_update();
    bool validate_native_processor_io_model() const;
    void rebuild_codec_v3_partial_state();
    void rebuild_auro_decoder_impl_state();
    void rebuild_native_xinn_partial_state();
    void rebuild_native_asc4he_partial_state();
    void run_codec_v3_partial_step();
    std::uint64_t render_codec_v3_a3deng_pop(
        std::uint8_t* a3deng_base,
        std::uint8_t* output_bytes,
        std::uint32_t frames,
        std::uint32_t output_mask,
        std::uint32_t input_mask);
    static std::uint64_t a3deng_codec_v3_pop_render_trampoline(
        void* user,
        std::uint8_t* a3deng_base,
        std::uint8_t* output_bytes,
        std::uint32_t frames,
        std::uint32_t output_mask,
        std::uint32_t input_mask);
    static std::int64_t codec_v3_processor_process_bridge(
        std::uint8_t* processor_base,
        const auro3deng::ProcessorIOBufferDesc* in_desc,
        const auro3deng::ProcessorIOBufferDesc* out_desc);
    bool run_native_xinn_partial_step(std::uint32_t copy_back_mask);
    bool run_native_asc4he_partial_step(std::uint32_t copy_back_mask);
    void rebuild_codec_v3_output_generator_state();
    void parser_rebind_frame_parse_results_impl(std::uint64_t frame_ptr);
    void apply_native_input_channel_mapping();
    void apply_native_output_channel_mapping();
    std::int32_t* native_input_buffer(unsigned index);
    const std::int32_t* native_input_buffer(unsigned index) const;
    std::int32_t* native_work_buffer(unsigned index);
    const std::int32_t* native_work_buffer(unsigned index) const;
    bool read_pcm_bytes(std::size_t absolute_offset, std::size_t byte_count, std::uint8_t* dst) const;
    const std::uint8_t* pcm_block_ptr(std::size_t absolute_offset, std::size_t byte_count);

    std::vector<std::uint8_t> file_bytes_;
    std::string pcm_stream_path_;
    std::string demux_temp_path_;
    bool pcm_streamed_ = false;
    bool owns_demux_temp_ = false;
    mutable std::ifstream pcm_stream_in_;
    std::vector<std::uint8_t> pcm_read_scratch_;
    std::size_t pcm_begin_ = 0;
    std::size_t pcm_length_ = 0;
    std::size_t read_pos_ = 0;
    std::uint64_t input_padding_samples_ = 0;
    std::uint64_t input_stream_cursor_ = 0;
    std::uint64_t source_sample_count_ = 0;
    std::uint64_t codec_v3_latency_samples_ = 0;
    std::uint32_t codec_v3_drain_blocks_remaining_ = 0;
    std::uint32_t codec_v3_drain_blocks_total_ = 0;
    ProgressFn progress_;

    uint32_t sample_rate_ = 0;
    unsigned channel_count_ = 0;
    unsigned block_size_ = 0;
    unsigned block_request_ = 0;
    bool binaural_requested_ = false;
    unsigned dsp_output_channels_req_ = 0;
    unsigned dsp_output_channels_ = 0;
    std::uint32_t dsp_output_layout_mask_req_ = 0;
    bool dsp_output_layout_mask_specified_ = false;
    std::string last_error_detail_;
    float dsp_headroom_gain_ = 1.0f;
    unsigned output_bits_ = 24;
    std::uint64_t dsp_clipped_samples_ = 0;
    std::uint32_t input_wav_channel_mask_ = 0;
    bool opened_ = false;
    bool raw_forced_ = false;
    bool legacy_auromatic_upmix_ = false;
    bool legacy_auromatic_ffmpeg_downsampled_ = false;
    std::uint32_t legacy_auromatic_source_rate_hz_ = 0;
    /// Encoded stream: dematrix to metadata layout, then XinN/bed-synth to a
    /// compatible larger --dsp-output-layout (e.g. 5.1 → 5.1_4H).
    bool meta_auromatic_upmix_ = false;
    std::uint32_t meta_upmix_source_mask_ = 0;
    /// 2 when the host rate is above 48 kHz and the 48 kHz Matic/XinN engine is
    /// fed through the native factor-2 matic_resample Down/Up pair.
    std::uint32_t meta_xinn_rate_decimation_ = 1;
    /// Core rate the Matic/XinN engine is configured at (0 = host rate).
    std::uint32_t meta_xinn_core_rate_ = 0;
    unsigned dsp_strength_ = 12;
    unsigned room_preset_ = kDefaultRoomPreset;
    unsigned hrtf_preset_ = kDefaultHrtfPreset;
    unsigned virtualizer_mode_ = kDefaultVirtualizerMode;
    bool headphone_connected_ = true;
    bool stereo_device_connected_ = true;
    bool listening_mode_auro3d_ = true;
    std::uint32_t a3deng_output_mode_ = 0;

    std::vector<std::int32_t> planar_scratch_;
    std::vector<std::int32_t> native_input_buffer_storage_;
    std::vector<std::int32_t> native_work_buffer_storage_;
    std::vector<std::uint8_t> native_xinn_step_state_;
    std::vector<std::uint32_t> native_xinn_sample_rate_words_;
    std::vector<float> native_xinn_float_span_storage_;
    std::vector<float> native_xinn_process_scratch_storage_;
    std::vector<std::uint8_t> native_xinn_block_records_storage_;
    std::vector<float> native_xinn_tail_input_storage_;
    std::vector<std::uint8_t> native_xinn_state_before_tail_;
    std::vector<float> native_xinn_scratch_before_tail_;
    std::uint32_t native_xinn_tail_samples_ = 0;
    /// Persistent native matic_resample factor-2 FIR state (one per channel).
    std::vector<float> native_xinn_down_history_;
    std::vector<float> native_xinn_up_history_;
    double native_upmix_limiter_envelope_ = 0.0;
    bool native_xinn_partial_ready_ = false;
    std::uint32_t native_xinn_partial_input_mask_ = 0;
    std::uint32_t native_xinn_partial_output_mask_ = 0;
    std::uint32_t native_xinn_partial_mode_ = 0;
    std::vector<std::uint8_t> native_asc4he_processor_state_;
    std::vector<std::uint8_t> native_asc4he_static_params_;
    std::vector<float> native_asc4he_float_span_storage_;
    std::vector<std::uint64_t> native_asc4he_channel_table_;
    bool native_asc4he_partial_ready_ = false;
    std::uint32_t native_asc4he_partial_input_mask_ = 0;
    std::uint32_t native_asc4he_partial_output_mask_ = 0;
    std::vector<std::uint32_t> output_channel_slot_map_;
    unsigned native_work_buffer_count_ = 0;
    unsigned native_input_buffer_count_ = 0;
    std::uint32_t input_channel_mask_ = 0;
    std::uint32_t requested_output_channel_mask_ = 0;
    std::uint32_t output_channel_mask_ = 0;
    AuroMetadataInfo auro_metadata_{};
    NativeDecoderConfigState native_config_state_{};
    NativeRuntimeConfigurationState native_runtime_configuration_{};
    NativeA3dengStaticConfigurationState native_a3deng_static_configuration_{};
    NativeDynamicParametersState native_dynamic_parameters_{};
    NativeA3dengRenderState native_a3deng_render_state_{};
    std::vector<std::uint8_t> a3deng_partial_blob_;
    std::vector<std::uint8_t> auro_decoder_impl_blob_;
    bool a3deng_partial_blob_constructed_ = false;
    std::uint32_t a3deng_partial_blob_block_size_ = 0u;
    std::uint32_t a3deng_partial_blob_output_mode_ = 0u;
    auro3deng::ProcessorIOBufferDesc input_desc_{};
    auro3deng::ProcessorIOBufferDesc output_desc_{};
    auro3deng::CodecV3DispatchStateEb5a0 codec_v3_dispatch_{};
    auro3deng::FormatDetectorState1056c0 codec_v3_format_detector_{};
    auro3deng::SyncDetectorState105ee0 codec_v3_sync_detector_{};
    auro3deng::DelayLineState106b40 codec_v3_delay_line_{};
    std::vector<auro3deng::DelayLineBufferSlot106b40> codec_v3_delay_line_slots_;
    std::vector<std::int32_t> codec_v3_delay_line_storage_;
    std::vector<std::uint8_t> codec_v3_output_generator_state_;
    std::vector<std::uint8_t> codec_v3_output_table_storage_;
    std::vector<std::uint8_t> codec_v3_segment_ctx_storage_;
    std::vector<std::uint8_t> codec_v3_segment_ranges_storage_;
    std::vector<std::uint32_t> codec_v3_segment_started_;
    std::vector<std::uint64_t> codec_v3_segment_frame_ptrs_;
    std::vector<std::uint8_t> codec_v3_fake_frame_deque_storage_;
    std::vector<std::uint8_t> codec_v3_ready_frame_deque_storage_;
    std::vector<std::uint8_t> codec_v3_fake_frame_deque_frame_storage_;
    std::vector<std::uint8_t> codec_v3_ready_frame_deque_frame_storage_;
    std::vector<std::uint8_t> codec_v3_fake_frame_storage_;
    std::vector<std::uint8_t> codec_v3_fake_frame_storage_next_;
    std::vector<std::uint8_t> codec_v3_ready_frame_storage_;
    std::vector<std::uint8_t> codec_v3_ready_frame_storage_next_;
    std::vector<std::uint8_t> codec_v3_fake_frame_channels_storage_;
    std::vector<std::uint32_t> codec_v3_fake_frame_channel_words_;
    std::vector<std::uint8_t> codec_v3_fake_frame_channel_ctx_storage_;
    std::vector<std::uint8_t> codec_v3_fake_parse_result_pool_state_;
    std::vector<std::uint8_t> codec_v3_fake_parse_result_pool_storage_;
    std::vector<std::uint8_t> codec_v3_channel_parser_storage_;
    std::uint32_t codec_v3_active_decode_probe_slots_ = 31u;
    std::vector<std::int32_t> codec_v3_output_errors_storage_;
    std::vector<std::int32_t> codec_v3_output_scratch_storage_;
    std::uint64_t codec_v3_parser_timeline_cursor_ = 0;
    std::uint64_t codec_v3_og_timeline_cursor_ = 0;
    std::uint32_t codec_v3_parser_state_ = 0;
    std::uint32_t input_signal_channel_mask_ = 0x7FFFFFFu;
};

} // namespace auro3d
