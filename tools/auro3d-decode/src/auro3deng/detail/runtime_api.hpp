#pragma once

/// Источники RE: весь DSP декодера — только libauro3d.so, разбор через ida-pro-mcp (RPC к IDA).
/// Использование в APK (JNI, ExoPlayer extension, нативные вызовы) — через JADX/app analysis.

#include "processor_io.hpp"

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace auro3deng {

/// Начало объекта Processor перекрыто полями Analyser (см. auro_a3deng_v3_Processor_t_construct → Analyser_t_construct).
constexpr std::uintptr_t kProcessor_Analyser_ctx_ptr = 8;   // второй аргумент construct: снова Processor*
constexpr std::uintptr_t kProcessor_Analyser_inner_fn = 16; // третий аргумент: sub_DA320
constexpr std::uintptr_t kProcessor_Analyser_thunk_fn = 256; // *(_QWORD*)(a1+256) = sub_ED070

/// sub_DA320: add rdi, 140h; jmp Controller_process_audio (tail-call).
constexpr std::uintptr_t kProcessor_to_Controller_audio = 0x140;
constexpr std::uintptr_t kController_error_flag = 0x25E6E0;
constexpr std::uintptr_t kController_mask_bits = 0x25E690;
constexpr std::uintptr_t kController_bytes_unit = 0x25E694;
constexpr std::uintptr_t kController_process_fn = 0x25E6E8;

// 0xD9AE0 timing/stat offsets inside Processor.
constexpr std::uintptr_t kProcessorPerf_elapsed_sum_qword = 0x25E958;    // += elapsed
constexpr std::uintptr_t kProcessorPerf_ratio_sum_float = 0x25E960;      // += bytes/field4
constexpr std::uintptr_t kProcessorPerf_best_ratio_float = 0x25E964;     // if (best > cur) best = cur
constexpr std::uintptr_t kProcessorPerf_best_elapsed_qword = 0x25E968;   // elapsed for best ratio
constexpr std::uintptr_t kProcessorPerf_best_ratio_input_float = 0x25E970; // input ratio for best ratio

// IDA 0x106F00 — глобальная инициализация CRC-таблицы (512 байт @ unk_41ACF0, флаг byte_41ACE0).
void decoder_crc_t_init_106f00();

// IDA 0x1042A1 — глобальная инициализация таблиц для Extrapolate (float expf LUT в dword_41A910..).
void decoder_channel_extrapolate_t_init_1042a1();

// Literal перенос auro_codec_v3_decoder_Config_initialize @ 0x1033C0.
std::int64_t auro_codec_v3_decoder_Config_initialize(std::int64_t config_base, std::int64_t init_args_base);

// IDA helper auro_channel_Mask_count: возвращает количество установленных битов в маске каналов.
std::uint32_t auro_channel_Mask_count(std::uint32_t mask, std::int64_t /*a2*/, std::uint32_t /*mod_arg*/);

// IDA auro_channel_Layout_dimension @ 0x5A98B0.
std::uint32_t auro_channel_Layout_dimension(std::uint32_t layout);

// IDA sub_EB420 — codec-v3 Decoder constructor + callback wiring.
// a1 — Decoder* (верхний объект a3deng), остальные аргументы соответствуют auro_memory_block_Accumulator/Distributor.
std::int64_t construct_codec_v3_decoder_runtime(
    std::uint64_t* decoder_slot_ptr,
    std::int64_t a2,
    std::int64_t a3,
    void (__fastcall *a4)(std::uint64_t, std::int64_t),
    std::int64_t a5,
    std::int64_t a6);

// IDA sub_DB290 @ 0xDB290 — верхний orchestrator Decoder/Manager process path.
// Пока перенесён literal-пролог валидации размеров и базовая рамка итерации.
std::int64_t /* Decompiled name: auro_a3deng_v3_process_decoder_manager */
auro_a3deng_v3_process_decoder_manager(
    std::uint32_t* decoder_owner_base,
    std::int64_t input_desc_base,
    std::uint32_t* output_desc_base,
    void* scratch_base);

// Внешние узлы, которые вызываются в sub_DB290.
std::int64_t auro_a3deng_v3_Decoder_latency(std::uint32_t* decoder_base);
std::int64_t auro_a3deng_v3_Decoder_t_construct(
    std::int64_t decoder_base,
    std::int64_t static_parameters,
    std::int64_t memory_block,
    std::int64_t notify_sink);
std::int64_t auro_a3deng_v3_Decoder_t_check_static_parameters(std::int64_t static_parameters);
std::int64_t auro_a3deng_v3_Decoder_update(std::int64_t decoder_base, std::int64_t update_config);
std::int64_t auro_a3deng_v3_Decoder_process(
    std::int64_t decoder_base,
    std::int64_t input_channels_blob,
    std::int64_t output_channels_blob,
    std::int64_t out_mask,
    std::int32_t* out_changed);
void auro_a3deng_v3_Decoder_allow_decoding(std::int64_t decoder_base, std::int32_t enabled);
std::int64_t auro_a3deng_v3_Decoder_reset_audio_state(std::uint64_t* decoder_base);
std::int64_t /* Decompiled name: auro_a3deng_v3_copy_input_to_output_channels */
auro_a3deng_v3_copy_input_to_output_channels(
    std::int64_t decoder_base,
    std::int64_t input_channels,
    std::int64_t output_channels,
    std::uint32_t* out_mask);
std::uint64_t /* Decompiled name: auro_a3deng_v3_copy_channels_and_return_mask */
auro_a3deng_v3_copy_channels_and_return_mask(
    std::int64_t decoder_base,
    std::int64_t input_channels,
    std::int64_t output_channels,
    std::uint32_t* out_mask);
std::int64_t /* Decompiled name: auro_a3deng_v3_reset_decoder_runtime_state */
auro_a3deng_v3_reset_decoder_runtime_state(std::uint64_t* decoder_base);
void /* Decompiled name: auro_a3deng_v3_set_input_state_and_notify */
auro_a3deng_v3_set_input_state_and_notify(std::int64_t decoder_base, std::int32_t value);
void /* Decompiled name: auro_a3deng_v3_set_decode_state_and_notify */
auro_a3deng_v3_set_decode_state_and_notify(std::int64_t decoder_base, std::int32_t value);
std::int64_t /* Decompiled name: auro_a3deng_v3_apply_decode_decisions */
auro_a3deng_v3_apply_decode_decisions(
    std::int64_t decoder_base,
    std::int64_t next_state_src,
    std::int64_t decisions,
    std::uint32_t decision_count);
std::int64_t /* Decompiled name: auro_a3deng_v3_process_codec_block */
auro_a3deng_v3_process_codec_block(
    std::int64_t decoder_base,
    const void* input_channels,
    std::int64_t output_channels,
    std::uint32_t* io_status);
std::int64_t auro_a3deng_v3_parameter_CutoffFrequency_from_int(std::uint32_t value);
std::int64_t auro_a3deng_v3_parameter_CutoffFrequency_to_int(std::uint32_t value);
float auro_a3deng_v3_strength_translate_to_float(std::uint32_t value);
float auro_a3deng_v3_strength_translate_to_dB(std::uint32_t value);
std::int64_t auro_a3deng_v3_strength_check_range(std::uint32_t value);
std::int64_t auro_a3deng_v3_strength_get_default();
std::int64_t auro_a3deng_v3_pipeline_Manager_set_initial_latency(std::uint32_t* manager_base, float latency);
std::int64_t auro_a3deng_v3_pipeline_Manager_configure(std::uint32_t* manager_base, void* cfg_blob, std::int32_t initial);
std::int64_t auro_a3deng_v3_pipeline_Manager_process_audio(std::uint32_t* manager_base, void** io_channels);
using AuroMaticV3XinNFl32ProcessFn = std::int64_t (*)(std::uint64_t xinn_state, void** channel_span_31);
using AuroA3dengV4XinNResetFn = std::int64_t (*)(std::uint64_t step_base);
using AuroMaticEngine2Fl32ProcessFn =
    void* (*)(std::uint64_t engine2_state, void* route_a, void* route_b, void* output_0x300);
std::int64_t /* Decompiled name: auro_matic_Engine2_fl32_construct */
auro_matic_Engine2_fl32_construct(std::uint64_t engine2_state, std::uint64_t memory);
void /* Decompiled name: auro_matic_Engine2_fl32_set_total_clear_frames */
auro_matic_Engine2_fl32_set_total_clear_frames(
    std::uint64_t engine2_state,
    std::uint32_t frames);
void /* Decompiled name: auro_matic_Engine2_fl32_set_preset */
auro_matic_Engine2_fl32_set_preset(std::uint64_t engine2_state, std::uint64_t preset);
std::int64_t /* Decompiled name: auro_matic_Engine2_fl32_reset_audio_state */
auro_matic_Engine2_fl32_reset_audio_state(std::uint64_t engine2_state);
bool /* Decompiled name: auro_matic_Engine2_fl32_partial_clear */
auro_matic_Engine2_fl32_partial_clear(std::uint64_t engine2_state, std::uint32_t* remaining);
void /* Decompiled name: auro_matic_Engine2_fl32_set_output_patch */
auro_matic_Engine2_fl32_set_output_patch(
    std::uint64_t engine2_state,
    const std::uint32_t* patch_3);
void* /* Decompiled name: auro_matic_Engine2_fl32_process */
auro_matic_Engine2_fl32_process(
    std::uint64_t engine2_state,
    void* route_a_0x80,
    void* route_b_0x80,
    void* output_0x300);
std::int64_t /* Decompiled name: auro_matic_XinN_fl32_process */
auro_matic_XinN_fl32_process(std::uint64_t xinn_state, void** channel_span_31);
std::int64_t /* Decompiled name: auro_matic_v3_XinN_fl32_process */
auro_matic_v3_XinN_fl32_process(std::uint64_t xinn_v3_state, void** channel_span_31);
void /* Decompiled name: auro_audio_Smooth_fl32_inst_initialize */
auro_audio_Smooth_fl32_inst_initialize(float* smooth_state, std::uint32_t sample_rate, float seconds);
void /* Decompiled name: auro_audio_Smooth_fl32_inst_update */
auro_audio_Smooth_fl32_inst_update(float* smooth_state, float target);
void /* Decompiled name: auro_audio_Smooth_fl32_inst_set_current */
auro_audio_Smooth_fl32_inst_set_current(float* smooth_state, float current);
void /* Decompiled name: auro_audio_Smooth_fl32_inst_gains_smooth */
auro_audio_Smooth_fl32_inst_gains_smooth(
    float* smooth_state,
    float* gains,
    std::uint32_t count);
void /* Decompiled name: auro_matic_Engine1_fl32_set_total_clear_frames */
auro_matic_Engine1_fl32_set_total_clear_frames(std::uint8_t* engine1_state, std::uint32_t frames);
std::int64_t /* Decompiled name: auro_matic_Engine1_fl32_construct */
auro_matic_Engine1_fl32_construct(std::uint8_t* engine1_state, std::uint64_t memory);
void /* Decompiled name: auro_matic_Engine1_fl32_set_preset */
auro_matic_Engine1_fl32_set_preset(std::uint8_t* engine1_state, std::uint64_t preset);
std::int64_t /* Decompiled name: auro_matic_Engine1_fl32_set_output_patch */
auro_matic_Engine1_fl32_set_output_patch(std::uint8_t* engine1_state, const std::uint32_t* patch_3);
std::int64_t /* Decompiled name: auro_matic_Engine1_fl32_reset_audio_state */
auro_matic_Engine1_fl32_reset_audio_state(std::uint8_t* engine1_state);
void /* Decompiled name: auro_matic_Engine1_fl32_get_delayed_frame */
auro_matic_Engine1_fl32_get_delayed_frame(
    const std::uint8_t* engine1_state,
    std::uint64_t* out_near,
    std::uint64_t* out_far);
bool /* Decompiled name: auro_matic_Engine1_fl32_partial_clear */
auro_matic_Engine1_fl32_partial_clear(std::uint8_t* engine1_state, std::uint32_t* remaining);
void /* Decompiled name: auro_matic_Engine1_fl32_process_ext */
auro_matic_Engine1_fl32_process_ext(
    std::uint8_t* engine1_state,
    const float* input_a_32,
    const float* input_b_32,
    float* output_0x300,
    const float* gains_32);
void /* Decompiled name: auro_matic_XinN_Early_fl32_process_mode1 */
auro_matic_XinN_Early_fl32_process_mode1(
    std::uint8_t* early_state,
    void** channel_span_31,
    float* output_0x300,
    const float* gains_32);
void /* Decompiled name: auro_matic_XinN_Early_fl32_process_mode2 */
auro_matic_XinN_Early_fl32_process_mode2(
    std::uint8_t* early_state,
    void** channel_span_31,
    float* output_a_0x300,
    float* output_b_0x300,
    const float* gains_32);
std::int64_t /* Decompiled name: auro_matic_XinN_Early_fl32_construct */
auro_matic_XinN_Early_fl32_construct(
    std::uint8_t* early_state,
    std::uint64_t memory_a,
    std::uint64_t memory_b);
void /* Decompiled name: auro_matic_XinN_Early_fl32_set_total_clear_frames */
auro_matic_XinN_Early_fl32_set_total_clear_frames(
    std::uint8_t* early_state,
    std::uint32_t frames);
void /* Decompiled name: auro_matic_XinN_Early_fl32_set_downmix */
auro_matic_XinN_Early_fl32_set_downmix(
    std::uint8_t* early_state,
    const std::uint8_t* downmix_plan);
std::int64_t /* Decompiled name: auro_matic_XinN_Early_fl32_set_output_patch */
auro_matic_XinN_Early_fl32_set_output_patch(
    std::uint8_t* early_state,
    const std::uint32_t* primary_patch_3,
    const std::uint32_t* secondary_patch_3);
std::uint64_t /* Decompiled name: auro_matic_XinN_Early_fl32_set_preset */
auro_matic_XinN_Early_fl32_set_preset(std::uint8_t* early_state, std::uint64_t preset);
std::int64_t /* Decompiled name: auro_matic_XinN_Early_fl32_reset_audio_state */
auro_matic_XinN_Early_fl32_reset_audio_state(std::uint8_t* early_state);
bool /* Decompiled name: auro_matic_XinN_Early_fl32_partial_clear */
auro_matic_XinN_Early_fl32_partial_clear(
    std::uint8_t* early_state,
    std::uint32_t* remaining);
void /* Decompiled name: auro_matic_XinN_Early_fl32_get_delayed_mode1 */
auro_matic_XinN_Early_fl32_get_delayed_mode1(
    std::uint8_t* engine1_state,
    float* out_near_32,
    float* out_far_32);
void /* Decompiled name: auro_matic_XinN_Early_fl32_mix_delayed_mode2 */
auro_matic_XinN_Early_fl32_mix_delayed_mode2(
    std::uint8_t* early_state,
    float* out_near_32,
    float* out_far_32);
std::int64_t /* Decompiled name: auro_matic_XinN_Late_fl32_construct */
auro_matic_XinN_Late_fl32_construct(std::uint32_t* late_state, std::uint64_t memory);
std::int64_t /* Decompiled name: auro_matic_XinN_Late_fl32_reset_audio_state */
auro_matic_XinN_Late_fl32_reset_audio_state(std::uint32_t* late_state);
void /* Decompiled name: auro_matic_XinN_Late_fl32_set_clear_frames */
auro_matic_XinN_Late_fl32_set_clear_frames(
    std::uint32_t* late_state,
    std::uint32_t early_clear_frames,
    std::uint32_t late_clear_frames);
bool /* Decompiled name: auro_matic_XinN_Late_fl32_partial_clear */
auro_matic_XinN_Late_fl32_partial_clear(std::uint32_t* late_state, std::uint32_t* remaining);
void /* Decompiled name: auro_matic_XinN_Late_fl32_set_preset */
auro_matic_XinN_Late_fl32_set_preset(std::uint32_t* late_state, std::uint64_t preset);
void /* Decompiled name: auro_matic_XinN_Late_fl32_set_output_patch */
auro_matic_XinN_Late_fl32_set_output_patch(std::uint32_t* late_state, const std::uint32_t* patch_3);
void* /* Decompiled name: auro_matic_XinN_Late_fl32_process */
auro_matic_XinN_Late_fl32_process(
    std::uint32_t* late_state,
    std::uint64_t engine_state,
    void* output_0x300,
    AuroMaticEngine2Fl32ProcessFn engine2_process);
std::int64_t /* Decompiled name: auro_matic_v3_XinN_Routing_fl32_apply */
auro_matic_v3_XinN_Routing_fl32_apply(
    std::uint64_t routing_state,
    const float* early_a_0x300,
    const float* early_b_0x300,
    const float* late_0x300,
    void** channel_span_31,
    std::uint32_t mode);
void /* Decompiled name: auro_matic_XinN_parameter_Dynamic_t_default */
auro_matic_XinN_parameter_Dynamic_t_default(std::uint8_t* dynamic_48);
void /* Decompiled name: auro_matic_v3_XinN_parameter_Dynamic_t_default */
auro_matic_v3_XinN_parameter_Dynamic_t_default(std::uint8_t* dynamic_120);
void /* Decompiled name: auro_matic_v3_XinN_Routing_fl32_construct */
auro_matic_v3_XinN_Routing_fl32_construct(std::uint8_t* routing_state);
std::uint64_t /* Decompiled name: auro_matic_v3_XinN_Routing_fl32_set_routing */
auro_matic_v3_XinN_Routing_fl32_set_routing(
    std::uint8_t* routing_state,
    const std::uint8_t* routing_72);
std::uint64_t /* Decompiled name: auro_matic_v3_XinN_Routing_fl32_get_routing */
auro_matic_v3_XinN_Routing_fl32_get_routing(
    const std::uint8_t* routing_state,
    std::uint8_t* routing_72);
std::int64_t /* Decompiled name: auro_matic_v3_XinN_Routing_fl32_configure */
auro_matic_v3_XinN_Routing_fl32_configure(
    std::uint8_t* routing_state,
    std::uint32_t mode,
    std::uint32_t output_mask,
    std::uint32_t input_mask);
std::int64_t /* Decompiled name: auro_matic_XinN_fl32_construct */
auro_matic_XinN_fl32_construct(
    std::uint8_t* xinn_state,
    std::uint8_t* routing_state,
    const std::uint64_t* memory_args_5);
void /* Decompiled name: auro_matic_XinN_fl32_set_dynamic_parameters */
auro_matic_XinN_fl32_set_dynamic_parameters(
    std::uint8_t* xinn_state,
    const std::uint8_t* dynamic_48);
void /* Decompiled name: auro_matic_XinN_fl32_get_dynamic_parameters */
auro_matic_XinN_fl32_get_dynamic_parameters(
    const std::uint8_t* xinn_state,
    std::uint8_t* dynamic_48);
std::int64_t /* Decompiled name: auro_matic_XinN_fl32_initialize */
auro_matic_XinN_fl32_initialize(
    std::uint8_t* xinn_state,
    std::uint32_t input_mask,
    std::uint32_t output_mask,
    std::uint64_t preset);
std::int64_t /* Decompiled name: auro_matic_v3_XinN_fl32_construct */
auro_matic_v3_XinN_fl32_construct(
    std::uint8_t* xinn_v3_state,
    const std::uint64_t* memory_args_5);
std::int64_t /* Decompiled name: auro_matic_v3_XinN_fl32_configure */
auro_matic_v3_XinN_fl32_configure(
    std::uint8_t* xinn_v3_state,
    std::uint32_t sample_rate,
    const std::uint64_t* config_args_4);
std::int64_t /* Decompiled name: auro_matic_XinN_fl32_reset_audio_state */
auro_matic_XinN_fl32_reset_audio_state(std::uint8_t* xinn_state);
std::int64_t /* Decompiled name: auro_matic_XinN_fl32_partial_clear */
auro_matic_XinN_fl32_partial_clear(
    std::uint8_t* xinn_state,
    std::uint32_t* remaining);
void /* Decompiled name: auro_matic_XinN_fl32_set_preset */
auro_matic_XinN_fl32_set_preset(std::uint8_t* xinn_state, std::uint64_t preset);
std::int64_t /* Decompiled name: auro_matic_v3_XinN_fl32_reset_audio_state */
auro_matic_v3_XinN_fl32_reset_audio_state(std::uint8_t* xinn_v3_state);
void /* Decompiled name: auro_matic_v3_XinN_fl32_set_preset */
auro_matic_v3_XinN_fl32_set_preset(std::uint8_t* xinn_v3_state, std::uint64_t preset);
std::uint64_t /* Decompiled name: auro_matic_v3_XinN_fl32_set_dynamic_parameters */
auro_matic_v3_XinN_fl32_set_dynamic_parameters(
    std::uint8_t* xinn_v3_state,
    const std::uint8_t* dynamic_120);
std::uint64_t /* Decompiled name: auro_matic_v3_XinN_fl32_get_dynamic_parameters */
auro_matic_v3_XinN_fl32_get_dynamic_parameters(
    const std::uint8_t* xinn_v3_state,
    std::uint8_t* dynamic_120);
void /* Decompiled name: auro_matic_v3_XinN_fl32_update_peak_amplitude */
auro_matic_v3_XinN_fl32_update_peak_amplitude(
    std::uint8_t* xinn_v3_state,
    float* peak_31);
std::int64_t /* Decompiled name: auro_matic_XinN_fl32_process_inplace */
auro_matic_XinN_fl32_process_inplace(std::uint64_t xinn_state, void** channel_span_31);
std::int64_t /* Decompiled name: auro_matic_XinN_fl32_process_scratch */
auro_matic_XinN_fl32_process_scratch(std::uint64_t xinn_state, void** channel_span_31);
constexpr std::size_t kXinnPlanBlobBytesPortable = 96u;
constexpr std::size_t kXinnUpdateBlobBytesPortable = 276u;

std::uint32_t xinn_prepare_mode_from_input_mask_portable(std::uint32_t input_mask);
void xinn_fill_tuning_static_defaults_portable(
    std::uint8_t* tuning_static,
    bool surround_mode,
    std::uint32_t room_preset);
void xinn_fill_tuning_dynamic_defaults_portable(std::uint8_t* tuning_dynamic, bool surround_mode);
void xinn_write_plan_update_blobs_portable(
    std::uint8_t* plan,
    std::uint8_t* update,
    std::uint32_t input_mask,
    std::uint32_t output_mask,
    std::uint32_t mode,
    std::uint32_t room_preset);

std::int64_t /* Decompiled name: auro_a3deng_v4_pipeline_step_upmix_XinN_initialize_impl */
auro_a3deng_v4_pipeline_step_upmix_XinN_initialize_impl(
    std::uint64_t step_base,
    const std::uint64_t* memory_3);
std::int64_t /* Decompiled name: auro_a3deng_v4_pipeline_step_upmix_XinN_initialize */
auro_a3deng_v4_pipeline_step_upmix_XinN_initialize(
    std::uint64_t step_base,
    const std::uint64_t* memory_3);
std::int64_t /* Decompiled name: auro_a3deng_v4_pipeline_step_upmix_XinN_prepare */
auro_a3deng_v4_pipeline_step_upmix_XinN_prepare(
    std::uint64_t step_base,
    const std::uint8_t* plan_blob,
    const std::uint8_t* update_blob);
std::int64_t /* Decompiled name: auro_a3deng_v4_pipeline_step_upmix_XinN_reset_audio_state_impl */
auro_a3deng_v4_pipeline_step_upmix_XinN_reset_audio_state_impl(std::uint64_t step_base);
std::int64_t /* Decompiled name: auro_a3deng_v4_pipeline_step_upmix_XinN_reset_audio_state */
auro_a3deng_v4_pipeline_step_upmix_XinN_reset_audio_state(std::uint64_t step_base);
std::int64_t /* Decompiled name: auro_a3deng_v4_pipeline_step_upmix_XinN_update_impl */
auro_a3deng_v4_pipeline_step_upmix_XinN_update_impl(
    std::uint64_t step_base,
    const std::uint8_t* update_blob);
std::int64_t /* Decompiled name: auro_a3deng_v4_pipeline_step_upmix_XinN_update */
auro_a3deng_v4_pipeline_step_upmix_XinN_update(
    std::uint64_t step_base,
    const std::uint8_t* update_blob);
std::int64_t /* Decompiled name: auro_a3deng_v4_pipeline_step_upmix_XinN_calculate_info */
auro_a3deng_v4_pipeline_step_upmix_XinN_calculate_info(
    std::uint64_t step_base,
    std::uint8_t* info_base);
std::int64_t /* Decompiled name: auro_a3deng_v4_pipeline_step_upmix_XinN_process_impl */
auro_a3deng_v4_pipeline_step_upmix_XinN_process_impl(
    std::uint64_t step_base,
    void** io_channels_31,
    std::uint32_t subblock_count,
    const std::uint8_t* block_records_516,
    AuroMaticV3XinNFl32ProcessFn process_fl32,
    AuroA3dengV4XinNResetFn reset_audio_state);
std::int64_t /* Decompiled name: auro_a3deng_v4_pipeline_step_upmix_XinN_process */
auro_a3deng_v4_pipeline_step_upmix_XinN_process(
    std::uint64_t step_base,
    void** io_channels_31,
    std::uint32_t subblock_count,
    const std::uint8_t* block_records_516,
    AuroMaticV3XinNFl32ProcessFn process_fl32,
    AuroA3dengV4XinNResetFn reset_audio_state);
void auro_a3deng_v3_pipeline_step_Upmix_set_bypass(std::uint32_t* upmix_base, std::int64_t bypass);
std::int32_t auro_a3deng_v3_pipeline_Manager_get_internal_channel_id(
    std::uint32_t* manager_base,
    std::uint32_t external_channel,
    std::int32_t* out_internal_channel,
    double reserved);

// Узлы памяти/декодера, которые дергает sub_EB420.
void auro_memory_block_Accumulator_t_construct(
    std::uint64_t* acc,
    std::int64_t a2,
    std::int64_t a3,
    void (__fastcall *cb)(std::uint64_t, std::int64_t),
    std::int64_t a5,
    std::int64_t a6,
    std::uint32_t initial_blocks,
    std::uint32_t align,
    std::uint64_t mask,
    std::uint64_t tail);
void auro_codec_v3_Decoder_t_required_additional_memory(std::uint64_t* acc, std::uint64_t* out_pair);
void auro_memory_block_Distributor_t_construct(void* dist, std::uint64_t* base, const char* type_tag);
std::uint32_t auro_codec_v3_Decoder_t_construct(std::int64_t decoder_base, std::int64_t dist, std::int64_t init_args);
void auro_codec_v3_Decoder_set_sync_callback(std::uint64_t* decoder_base, void* fn, void* user);
void auro_codec_v3_Decoder_set_content_callback(std::uint64_t decoder_base, void* fn, void* user);
void auro_codec_v3_Decoder_set_decide_decode_callback(std::uint64_t decoder_base, void* fn, void* user);
void auro_codec_v3_Decoder_set_metadata_callback(std::uint64_t decoder_base, void* fn, void* user);

// IDA 0x101760 — auro_codec_v3_Decoder_t_construct:
// CRC_t_init → channel_Extrapolate_t_init → Config_initialize → Memory/FormatDetector/Parser/OutputGenerator construct.
// a1 — база codec v3 decoder, a2 — ptr на args для Config_initialize (см. auro_codec_v3_ida::kDecoderConfigInitArg_*),
// a3 — пользовательский контекст (Decoder*, передаётся дальше в Memory_t_construct/OutputGenerator_t_construct).
std::uint32_t decoder_t_construct_101760(std::uint8_t* decoder_base, const void* config_init_args, void* user_ctx);

/// IDA sub_ED070 @ 0xED070
std::int64_t process_analyser_audio(std::uint8_t* processor_base);

/// IDA sub_DA320 @ 0xDA320 — в бинарнике tail jmp; здесь явные три аргумента (на SysV a2/a3 доходят из Processor_process).
std::int64_t forward_analyser_to_controller(
    std::uint8_t* processor_base,
    std::uint8_t* input_buffer_desc,
    std::uint8_t* output_buffer_desc);

/// IDA auro_a3deng_v3_Analyser_process @ 0xED500
std::int64_t analyser_process(std::uint8_t* processor_base, const std::uint32_t* sample_increment);

/// Минимальный перенос ветки auro_a3deng_v3_Controller_process_audio @ 0xDA870:
/// если error_flag!=0 и output.layout_or_kind<=1, обнуляет активные каналы output по маске channel_mask.
/// Возвращает 129 при несовместимом output layout_or_kind.
std::int64_t controller_process_audio_zero_fill(
    std::uint32_t channel_mask,
    std::uint32_t bytes_per_unit,
    std::uint32_t error_flag,
    const void* output_desc_raw,
    std::size_t output_desc_size);

/// Обёртка IDA 0xDA870: при error_flag==0 зовёт callback [ctrl+0x25E6E8](ctrl,a2,a3),
/// иначе выполняет zero-fill ветку.
std::int64_t controller_process_audio_da870(
    std::uint8_t* controller_base,
    const void* input_desc_raw,
    const void* output_desc_raw,
    std::size_t output_desc_size);

/// Пролог валидации auro_a3deng_v3_Processor_process @ 0xD9AE0 (до вызова Analyser_process):
/// проверки layout/field8/кратности/field4/указателей каналов для in/out с кодами возврата 122..136.
struct ProcessorIoExpectDa9ae0 {
    std::int32_t in_layout = 0;          // dword_25E7C8
    std::int32_t in_mask = 0;            // dword_25E7BC
    std::int32_t in_bytes_unit = 0;      // dword_25E7C0
    std::int32_t in_field4 = 0;          // dword_25E7C4
    std::int32_t in_field8_when_layout1 = 0; // dword_25E7CC

    std::int32_t out_layout = 0;         // dword_25E7DC
    std::int32_t out_mask = 0;           // dword_25E7D0
    std::int32_t out_bytes_unit = 0;     // dword_25E7D4
    std::int32_t out_field4 = 0;         // dword_25E7D8
    std::int32_t out_field8_when_layout1 = 0; // dword_25E7E0
};

std::int32_t processor_process_validate_da9ae0(
    const ProcessorIOBufferDesc* in_desc,
    const ProcessorIOBufferDesc* out_desc,
    const ProcessorIoExpectDa9ae0& ex);

/// Минимальный перенос 0xD9AE0: validate_io + вызов Analyser_process.
/// Полный хвост с auro_chrono_Measurement_t_get_elapsed и статистикой времени пока не перенесён.
std::int64_t processor_process_da9ae0_minimal(
    std::uint8_t* processor_base,
    const ProcessorIOBufferDesc* in_desc,
    const ProcessorIOBufferDesc* out_desc);

/// Порядок как в 0xD9AE0: validate -> analyser_process -> update_timing_stats -> return analyser rc.
/// elapsed_ticks передаётся снаружи (в оригинале берётся из auro_chrono_Measurement_t_get_elapsed).
std::int64_t processor_process_da9ae0_minimal_with_elapsed(
    std::uint8_t* processor_base,
    const ProcessorIOBufferDesc* in_desc,
    const ProcessorIOBufferDesc* out_desc,
    std::int64_t elapsed_ticks);

/// Перенос хвоста 0xD9AE0: обновление perf-статистики после analyser_process.
void processor_update_timing_stats_da9ae0(
    std::uint8_t* processor_base,
    const ProcessorIOBufferDesc* in_desc,
    std::int64_t elapsed_ticks);

struct AuroDecoderImplInitParams {
    std::uint32_t sample_rate = 0;
    std::uint32_t block_size_samples = 0;
    std::uint32_t input_mask = 0;
    std::uint32_t output_mask = 0; // effective output mask for Processor_process validate
    std::uint64_t input_channel_ptrs[27]{};
    std::uint64_t output_channel_ptrs[27]{};
};

/// IDA AuroDecoderImpl::Initialize @ 0xD71E0 — заполнение IO-дескрипторов @ +40/+272.
bool /* Decompiled name: auro_decoder_impl_initialize */
auro_decoder_impl_initialize(
    std::uint8_t* impl_base,
    const AuroDecoderImplInitParams* params);

/// IDA AuroDecoderImpl::Decode @ 0xD7830 — vtable+168 → Processor_process (0xD9AE0).
/// processor_process=nullptr → processor_process_da9ae0_minimal на блоке @ +552.
std::int32_t /* Decompiled name: auro_decoder_impl_decode */
auro_decoder_impl_decode(
    std::uint8_t* impl_base,
    std::int64_t (*processor_process)(
        std::uint8_t* processor_base,
        const ProcessorIOBufferDesc* in_desc,
        const ProcessorIOBufferDesc* out_desc));

// Частичный перенос codec-v3 dispatch слоя вокруг sub_EB5A0 / sub_EB840 / sub_EB870 / sub_EB8A0.
// Codec-v3 dispatch / decoder_process перенесён в codec_v3_decoder_process_101800 / auro_codec_v3_Decoder_process.
struct CodecV3StateChangeSink {
    void* user = nullptr;
    void (*notify)(void* user, std::int64_t kind) = nullptr; // 0=sync changed, 1=content/decide changed
};

struct CodecV3DispatchStateEb5a0 {
    std::uint32_t format_word0 = 0;        // a1+252144, передаётся в auro_codec_v3_Decoder_process как третий аргумент
    std::uint32_t sample_rate = 0;         // a1+252152
    std::uint32_t block_size = 0;
    std::uint32_t required_output_mask = 0;
    std::uint32_t produced_output_mask = 0;
    std::uint32_t sync_state = 0;          // a1+252120
    std::uint32_t content_state = 0;       // a1+252116
    std::uint32_t decide_decode_state = 0; // a1+252112
    std::uint32_t decide_decode_gate = 0;  // a1+252124
    std::uint32_t last_result = 0;         // a1+252128
    CodecV3StateChangeSink sink{};
};

struct CodecV3IoBufferDescEb5a0 {
    std::uint32_t total_samples = 0;
    std::uint32_t sample_rate = 0;
    std::uint32_t bits_per_sample = 0;
    std::uint32_t reserved = 0;
    std::uint64_t channel_ptr[31]{};
};
static_assert(sizeof(CodecV3IoBufferDescEb5a0) == 264, "codec-v3 IO descriptor must match current IDA layout.");

struct FormatDetectorTail1056c0;
struct SyncDetectorState105ee0;
struct DelayLineState106b40;

using CodecV3ProcessFnEb5a0 = std::int64_t (*)(
    void* user,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t format_word0,
    const CodecV3IoBufferDescEb5a0* output_desc,
    std::uint32_t* io_status);

std::int32_t /* Decompiled name: codec_v3_decoder_process_validate */
codec_v3_decoder_process_validate(
    const CodecV3DispatchStateEb5a0* state,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask,
    const CodecV3IoBufferDescEb5a0* output_desc);

void codec_v3_sync_callback_eb840(CodecV3DispatchStateEb5a0* state, int value);
void codec_v3_content_callback_eb870(CodecV3DispatchStateEb5a0* state, int value);
void codec_v3_decide_decode_callback_eb8a0(
    CodecV3DispatchStateEb5a0* state,
    std::uint32_t next_state,
    std::uint32_t* decisions,
    int decision_count);
/// IDA OutputGenerator pre-segments callback @ og+792: (ctx, ranges_base, started_base).
std::int64_t /* Decompiled name: codec_v3_pre_segments_decide_decode */
codec_v3_pre_segments_decide_decode(
    std::uint64_t ctx,
    std::uint64_t ranges_base,
    std::uint64_t started_base);
std::int64_t /* Decompiled name: codec_v3_pre_segments_decide_decode_decoder */
codec_v3_pre_segments_decide_decode_decoder(
    std::uint64_t decoder_base,
    std::uint64_t ranges_base,
    std::uint64_t started_base);
/// Host-friendly decide_decode: layouts + allow_decoding without native decoder/blob ctx.
std::int64_t /* Decompiled name: codec_v3_pre_segments_decide_decode_layouts */
codec_v3_pre_segments_decide_decode_layouts(
    std::uint32_t segment_count,
    std::uint64_t ranges_base,
    std::uint64_t started_base,
    std::uint32_t output_layout,
    std::uint32_t target_layout,
    std::uint32_t allow_decoding,
    CodecV3DispatchStateEb5a0* aggregate_dispatch);
std::int64_t /* Decompiled name: codec_v3_dispatch */
codec_v3_dispatch(
    CodecV3DispatchStateEb5a0* state,
    const CodecV3IoBufferDescEb5a0* input_desc,
    const CodecV3IoBufferDescEb5a0* output_desc,
    std::uint32_t* io_status,
    CodecV3ProcessFnEb5a0 process,
    void* process_user);

struct CodecV3PartialRuntimeEb5a0 {
    CodecV3DispatchStateEb5a0* dispatch = nullptr;
    FormatDetectorTail1056c0* format_detector = nullptr;
    SyncDetectorState105ee0* sync_detector = nullptr;
    DelayLineState106b40* delay_line = nullptr;
    void (*set_layout)(void* user, std::uint32_t layout_mask) = nullptr;
    void (*process_block)(void* user, const std::uint64_t* channel_ptrs_27) = nullptr;
    void* sync_user = nullptr;
};

std::int64_t codec_v3_process_partial_eb5a0(
    void* user,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t format_word0,
    const CodecV3IoBufferDescEb5a0* output_desc,
    std::uint32_t* io_status);

using CodecV3ParserProcessFn101800 = std::int64_t (*)(std::uint8_t* parser_base);

struct CodecV3ParserRuntimeFns1034e0 {
    std::uint64_t (*delay_line_get_buffer)(std::uint64_t delay_line_ptr, std::uint64_t cursor, std::uint64_t* io_base) = nullptr;
    std::uint64_t (*frame_deque_find_first_with_end_after)(std::uint64_t frame_deque_ptr, std::uint64_t sample_pos) = nullptr;
    std::int64_t (*frame_deque_push_back)(std::uint64_t frame_deque_ptr, std::uint64_t frame_ptr) = nullptr;
    std::uint64_t (*frame_deque_pop_front)(std::uint64_t frame_deque_ptr) = nullptr;
    std::int64_t (*frame_mark_as_unused)(std::uint64_t frame_ptr) = nullptr;
    void (*content_state_callback)(std::uint64_t ctx, std::uint32_t state) = nullptr;
    std::uint64_t content_state_ctx = 0;
};

// Перенос auro_codec_v3_decoder_Parser_process @ 0x1034E0.
// parser_base соответствует "a1" (qword layout из IDA), runtime_fns — внешний bridge к deque/pool/channel parser.
std::int64_t codec_v3_parser_process_1034e0(
    std::uint8_t* parser_base,
    const CodecV3ParserRuntimeFns1034e0* runtime_fns);

// Ближайший перенос верхнего уровня auro_codec_v3_Decoder_process @ 0x101800:
// validate -> DelayLine_write -> FormatDetector -> Parser -> zero output planes -> OutputGenerator -> DelayLine_advance.
// parser_process можно передать извне (например, адаптер к локальной реализации Parser_process).
std::int32_t codec_v3_decoder_process_validate_101800(
    const std::uint8_t* decoder_base,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask,
    const CodecV3IoBufferDescEb5a0* output_desc);

std::int64_t codec_v3_decoder_process_101800(
    std::uint8_t* decoder_base,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask,
    CodecV3IoBufferDescEb5a0* output_desc,
    std::uint32_t* io_status,
    const void* output_runtime_fns,
    CodecV3ParserProcessFn101800 parser_process);

std::int64_t auro_codec_v3_Decoder_process(
    std::uint64_t decoder_base,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask,
    CodecV3IoBufferDescEb5a0* output_desc,
    std::uint32_t* io_status);

struct DelayLineBufferSlot106b40 {
    std::uint32_t channel_mask = 0;
    std::uint32_t reserved = 0;
    std::uint64_t channel_ptr[31]{};
};
static_assert(sizeof(DelayLineBufferSlot106b40) == 256, "DelayLine slot size must match current IDA 0x100 stride.");

struct DelayLineState106b40 {
    std::uint32_t samples_per_block = 0;
    std::uint32_t reserved_4 = 0;
    std::uint64_t reserved_8 = 0;
    std::uint64_t ring_storage_base = 0;
    std::uint32_t ring_slot_count = 0;
    std::uint32_t write_slot_index = 0;
    std::uint64_t absolute_cursor = 0;
};
static_assert(offsetof(DelayLineState106b40, ring_storage_base) == 16, "DelayLine storage base offset must match IDA.");
static_assert(offsetof(DelayLineState106b40, ring_slot_count) == 24, "DelayLine ring slot count offset must match IDA.");
static_assert(offsetof(DelayLineState106b40, absolute_cursor) == 32, "DelayLine cursor offset must match IDA.");

std::uint64_t delay_line_get_buffer_106b40(
    DelayLineState106b40* state,
    std::uint64_t cursor,
    std::int64_t* io_state);
std::uint64_t /* Decompiled name: delay_line_get_buffer_u64_state */
delay_line_get_buffer_u64_state(
    std::uint64_t delay_line_ptr,
    std::uint64_t cursor,
    std::uint64_t* io_state_u64);
std::uint64_t delay_line_get_channel_from_buffer_106ab0(
    std::uint64_t delay_line_buffer,
    std::uint32_t channel,
    std::uint64_t start);
std::uint64_t /* Decompiled name: delay_line_stream_index */
delay_line_stream_index(
    const DelayLineState106b40* state,
    std::uint32_t stage_count);
std::uint64_t delay_line_advance_106b20(DelayLineState106b40* state);
std::uint64_t delay_line_write_buffer_106ab0(
    DelayLineState106b40* state,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask);

/// Tail of integrated FormatDetector blob @ decoder+288..+343 (libauro `FormatDetector_t_construct`).
struct FormatDetectorTail1056c0 {
    void (*sink_notify)(void* user, std::int64_t kind) = nullptr; // +288
    void* sink_user = nullptr;                                    // +296
    std::uint32_t layout = 0;                                     // +304
    std::uint32_t sync_state = 0;                                 // +308
    std::uint64_t frame_deque_ptr = 0;                            // +312
    std::uint64_t processed_samples = 0;                          // +320
    std::uint64_t expected_frame_end = 0;                         // +328
    std::uint32_t blocks_per_call = 0;                            // +336
    std::uint32_t allow_low_9bits = 0;                            // +340
};
static_assert(sizeof(FormatDetectorTail1056c0) == 56, "FormatDetector tail must be 56 bytes.");

using FormatDetectorState1056c0 = FormatDetectorTail1056c0;

struct SyncDetectorChannelState105ee0 {
    std::uint32_t channel = 0;          // +0 within 24-byte slot
    std::uint32_t aligned_word = 0;     // +4 (low 16 bits used)
    std::uint32_t aligned_bit = 0;      // +8
    std::uint32_t aligned_code = 0;     // +12
    std::uint32_t history_bit1 = 0;     // +16
    std::uint32_t history_bit2 = 0;     // +20
};
static_assert(sizeof(SyncDetectorChannelState105ee0) == 24, "SyncDetector slot size must match IDA stride.");

struct SyncDetectorState105ee0 {
    std::uint32_t state = 0;             // +0
    std::uint32_t counter_4 = 0;         // +4
    std::uint32_t accum_8 = 0;           // +8
    std::uint32_t accum_c = 0;           // +12
    std::uint32_t word_10 = 0;           // +16
    std::uint32_t reserved_14 = 0;
    void* notify_ctx = nullptr;          // +24
    void (*notify)(void* ctx, std::int64_t kind, std::uint64_t a, std::uint64_t b) = nullptr; // +32
    std::uint32_t layout = 0;            // +40
    std::uint32_t enabled = 0;           // +44
    SyncDetectorChannelState105ee0 channels[9]{}; // +48 .. +263
    std::uint32_t active_channel_count = 0; // +264
    std::uint32_t detect_counter = 0;    // +268
    std::uint32_t detect_bit_110 = 0;    // +272
    std::uint32_t detect_bit_114 = 0;    // +276
    std::uint32_t detect_target = 0;     // +280
};

/// Full FormatDetector region @ decoder+232: SyncDetector + tail (344 bytes, ends before Parser @ +576).
struct FormatDetectorRegion1056c0 {
    SyncDetectorState105ee0 sync{};
    FormatDetectorTail1056c0 tail{};
};
static_assert(offsetof(FormatDetectorRegion1056c0, tail) == 288, "FormatDetector tail must start @ +288.");
static_assert(sizeof(FormatDetectorRegion1056c0) == 344, "FormatDetector region must be 344 bytes.");

void /* Decompiled name: sync_detector_t_construct */
sync_detector_t_construct(SyncDetectorState105ee0* state);
void /* Decompiled name: sync_detector_set_callback */
sync_detector_set_callback(
    SyncDetectorState105ee0* state,
    void (*notify)(void* ctx, std::int64_t kind, std::uint64_t a, std::uint64_t b),
    void* notify_ctx);
const std::uint32_t* /* Decompiled name: sync_detector_get_common_header */
sync_detector_get_common_header(const SyncDetectorState105ee0* state);
const SyncDetectorChannelState105ee0* /* Decompiled name: sync_detector_find_channel_header */
sync_detector_find_channel_header(
    const SyncDetectorState105ee0* state,
    std::uint32_t channel);
void /* Decompiled name: sync_detector_set_layout */
sync_detector_set_layout(SyncDetectorState105ee0* state, std::uint32_t layout_mask);
void /* Decompiled name: sync_detector_process_block */
sync_detector_process_block(
    SyncDetectorState105ee0* state,
    const std::uint64_t* channel_ptrs_27);

void /* Decompiled name: format_detector_t_construct */
format_detector_t_construct(
    FormatDetectorRegion1056c0* region,
    std::uint8_t* decoder_base,
    std::uint8_t* memory_base);
void /* Decompiled name: format_detector_sync_callback */
format_detector_sync_callback(
    void* region_raw,
    std::int64_t kind,
    std::uint64_t rel_start,
    std::uint64_t span);

void /* Decompiled name: format_detector_process */
format_detector_process(
    FormatDetectorState1056c0* state,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask,
    void (*set_layout)(void* user, std::uint32_t layout_mask),
    void (*process_block)(void* user, const std::uint64_t* channel_ptrs_27),
    void* sync_user);

void /* Decompiled name: parser_t_construct */
parser_t_construct(
    std::uint8_t* parser_base,
    const std::uint8_t* decoder_base,
    std::uint8_t* memory_base);
std::int64_t codec_v3_parser_process_integrated_1034e0(std::uint8_t* parser_base);

// IDA 0x106CD0 / 0x106D07 helpers:
// Frame_mark_as_unused iterates frame slots and calls channel ParseResult_mark_usage(..., 0).
void /* Decompiled name: downmix_coefficients_initialize */
downmix_coefficients_initialize(std::uint64_t coeff_ptr);
void /* Decompiled name: channel_parse_result_construct */
channel_parse_result_construct(std::uint64_t parse_result_ptr);
void /* Decompiled name: channel_parse_result_mark_usage */
channel_parse_result_mark_usage(std::uint64_t parse_result_ptr, std::uint32_t in_use);
void /* Decompiled name: channel_parse_result_prepare_new */
channel_parse_result_prepare_new(std::uint64_t parse_result_ptr);
std::uint64_t /* Decompiled name: parse_result_pool_required_additional_memory */
parse_result_pool_required_additional_memory(std::uint32_t pool_count);
std::uint32_t /* Decompiled name: memory_parse_result_pool_count */
memory_parse_result_pool_count(
    std::uint32_t block_samples,
    std::uint32_t channel_count);
/// IDA 0x13D750: три Accumulator_add_block — суммарный запрошенный объём полезной нагрузки (24+4+8)*n байт.
std::uint64_t /* Decompiled name: memory_block_info_required_additional_memory */
memory_block_info_required_additional_memory(std::uint32_t n);
/// IDA 0x106D20: аргумент для BlockInfo_t_required_additional_memory — (((*a2 + 255) >> 7) | 1).
std::uint32_t /* Decompiled name: memory_block_info_slot_count */
memory_block_info_slot_count(std::uint32_t block_samples);
/// IDA 0x13D750 / auro_codec_v3_decoder_BlockInfo_t_construct @ 0x530900.
std::uint32_t /* Decompiled name: block_info_construct */
block_info_construct(std::uint8_t* block_info, std::uint32_t slot_count);
/// IDA 0x106D20 хвост: add_block(4, 8*(q&0x7FFFFFFF)) и add_block(4, (12*q)&0x3FFFFFFFCLL).
std::uint64_t /* Decompiled name: memory_accumulator_tail_payload */
memory_accumulator_tail_payload(std::uint64_t qword_at_a2);
/// IDA 0x13D570: Accumulator_add_block(..., 8, 336 * n) — полезная нагрузка байт.
std::uint64_t /* Decompiled name: memory_frame_deque_required_additional_memory */
memory_frame_deque_required_additional_memory(std::uint32_t deque_capacity);
/// Current IDA DelayLine required memory: add_block(4, 4*a*b*c) + add_block(8, 256*b); a=*a2, b=*(a2+16), c=*(a2+44).
std::uint64_t /* Decompiled name: memory_delay_line_payload_sum */
memory_delay_line_payload_sum(
    std::uint64_t a2_qword0,
    std::uint64_t a2_qword16,
    std::uint32_t a2_dword44);

/// BitReader @ channel_Parser_process: база `parser_u32 + 6` (см. 0x103840). Таблица масок: dword_2732C0 @ 0x2732C0.
void /* Decompiled name: channel_bit_reader_set_data */
channel_bit_reader_set_data(std::uint64_t br, std::uint64_t words_ptr, std::int32_t word_count);
void /* Decompiled name: channel_bit_reader_set_bitlines */
channel_bit_reader_set_bitlines(std::uint64_t br, std::int32_t bitlines);
std::uint32_t /* Decompiled name: channel_bit_reader_get_unsigned_bits */
channel_bit_reader_get_unsigned_bits(std::uint64_t br, std::int32_t bit_count);
std::uint32_t /* Decompiled name: channel_bit_reader_get_remaining_nr_bits */
channel_bit_reader_get_remaining_nr_bits(std::uint64_t br);
std::int64_t /* Decompiled name: channel_bit_reader_get_signed_bits */
channel_bit_reader_get_signed_bits(std::uint64_t br, std::int32_t bit_count);
void /* Decompiled name: channel_bit_reader_reset */
channel_bit_reader_reset(std::uint64_t br);

/// IDA 0x1070D0
void /* Decompiled name: channel_crc_reset */
channel_crc_reset(std::uint64_t crc_state);
/// IDA 0x106F00 — глобальная таблица word_41ACF0[256] (512 байт), как в .so (lazy, один раз).
void /* Decompiled name: channel_crc_table_init */
channel_crc_table_init();
/// IDA 0x1070E0 — обновление CRC по массиву 32-битных слов (счётчик слов в *(uint32*)(crc+4)).
std::uint64_t /* Decompiled name: channel_crc_process */
channel_crc_process(
    std::uint64_t crc_state,
    std::uint64_t words_ptr,
    std::int32_t word_count);
/// IDA 0x107170 — сравнение с полями канала (как в хвосте 0x103840).
std::int64_t /* Decompiled name: channel_crc_check */
channel_crc_check(
    std::uint64_t crc_state,
    std::int32_t dword_at_plus4_compare,
    std::int16_t expected_crc_word);
/// IDA 0x105D50 — собирает метаданные канала после чтения слов @+76/+80.
std::int64_t /* Decompiled name: channel_metadata_combine_info */
channel_metadata_combine_info(std::uint64_t frame_channel_ptr);
/// IDA 0x104210 — полный Parser_t_construct: reset parser + инициализация полей канала (+52/+56/+72).
std::int64_t /* Decompiled name: channel_parser_construct */
channel_parser_construct(
    std::uint64_t parser_base,
    std::uint64_t frame_channel_ptr,
    std::uint64_t frame_meta_16b_ptr,
    std::uint64_t frame_slot_ptr);

// Перенос auro_codec_v3_decoder_channel_Parser_process @ 0x103840.
std::int64_t channel_parser_process_103840(
    std::uint64_t parser_base,
    std::uint64_t frame_channel_ptr,
    std::uint64_t channel_ptr,
    std::uint32_t word_count,
    std::uint32_t* out_mode);

bool /* Decompiled name: parse_result_pool_construct */
parse_result_pool_construct(
    std::uint64_t pool_base_ptr,
    std::uint64_t storage_base_ptr,
    std::uint32_t pool_count);
std::uint64_t /* Decompiled name: parse_result_pool_get_new */
parse_result_pool_get_new(std::uint64_t pool_base_ptr);
void /* Decompiled name: frame_construct_parse_results */
frame_construct_parse_results(
    std::uint64_t frame_ptr,
    void (*construct_parse_result)(std::uint64_t parse_result_ptr));
void /* Decompiled name: frame_construct */
frame_construct(
    std::uint64_t frame_ptr,
    std::uint64_t start,
    std::uint32_t span,
    std::uint32_t channel_mask,
    std::uint32_t active_mask);
void /* Decompiled name: frame_assign_window */
frame_assign_window(
    std::uint64_t* io_next_frame_start,
    std::uint64_t frame_span,
    std::uint64_t frame_ptr);
void /* Decompiled name: frame_mark_usage */
frame_mark_usage(
    std::uint64_t frame_ptr,
    std::uint32_t in_use,
    void (*mark_usage)(std::uint64_t parse_result_ptr, std::uint32_t in_use));
void /* Decompiled name: frame_mark_as_unused */
frame_mark_as_unused(
    std::uint64_t frame_ptr,
    void (*mark_usage)(std::uint64_t parse_result_ptr, std::uint32_t in_use));
void /* Decompiled name: frame_mark_as_unused_106cd0_default */
frame_mark_as_unused_106cd0_default(std::uint64_t frame_ptr);
std::int64_t /* Decompiled name: parser_frame_mark_as_unused_cb */
parser_frame_mark_as_unused_cb(std::uint64_t frame_ptr);

// FrameDeque (IDA 0x13D570 / 0x13D5D0 / 0x13D670) minimal runtime layout.
struct FrameDequeState13d570 {
    std::uint64_t slot_ptrs[2]{};
    std::uint64_t frame_ptrs[2]{};
    std::uint64_t head = 0;
    std::uint64_t count = 0;
    std::uint32_t capacity = 2;
    std::uint64_t cursor = 0;
    std::uint64_t next_frame_start = 0;
    std::uint64_t frame_span = 0;
};

std::uint64_t /* Decompiled name: frame_deque_find_first_with_end_after */
frame_deque_find_first_with_end_after(
    std::uint64_t frame_deque_ptr,
    std::uint64_t sample_pos);
std::int64_t /* Decompiled name: frame_deque_push_back */
frame_deque_push_back(
    std::uint64_t frame_deque_ptr,
    std::uint64_t frame_ptr,
    std::uint64_t copy_bytes);
std::uint64_t /* Decompiled name: frame_deque_pop_front */
frame_deque_pop_front(
    std::uint64_t frame_deque_ptr,
    void (*frame_mark_as_unused)(std::uint64_t frame_ptr));
std::uint64_t /* Decompiled name: frame_deque_pop_front_keep_frame */
frame_deque_pop_front_keep_frame(std::uint64_t frame_deque_ptr);
std::uint64_t /* Decompiled name: frame_deque_pop_back */
frame_deque_pop_back(
    std::uint64_t frame_deque_ptr,
    void (*frame_mark_as_unused)(std::uint64_t frame_ptr));

struct ParserRebindContext103610 {
    std::uint64_t parse_result_pool_base = 0;
    std::uint32_t* channel_words_base = nullptr; // 8 u32 per channel index
    std::size_t channel_words_count = 0;         // number of channel-index slots
    std::uint8_t* channel_ctx_base = nullptr;    // 64 bytes per channel index
    std::size_t channel_ctx_count = 0;           // number of channel-index slots
    std::uint8_t* channel_parser_base = nullptr; // 64 bytes per parser slot
    std::size_t channel_parser_count = 0;        // number of parser slots
};

void /* Decompiled name: parser_rebind_frame_parse_results */
parser_rebind_frame_parse_results(
    std::uint64_t frame_ptr,
    const ParserRebindContext103610* ctx);

struct ParserPayloadRefreshContext {
    std::uint64_t frame_deque_ptr = 0;
    std::uint64_t output_generator_base = 0;
    std::uint32_t input_channel_limit = 31;
    std::uint32_t block_size = 0;
    std::uint32_t* channel_words_base = nullptr; // 8 u32 per channel index
    std::size_t channel_words_count = 0;
    std::uint8_t* channel_ctx_base = nullptr;    // 64 bytes per channel index
    std::size_t channel_ctx_count = 0;
};

void /* Decompiled name: parser_refresh_payload */
parser_refresh_payload(
    const CodecV3IoBufferDescEb5a0* input_desc,
    const ParserPayloadRefreshContext* ctx);

struct ParserReadyFrameCopyContext {
    std::uint8_t* ready_parse_result_base = nullptr;
    std::size_t ready_parse_result_size = 0;
    std::uint32_t copied_slot_capacity = 0;
    std::size_t parse_result_bytes = 0;
};

struct ParserStateSinkContext {
    CodecV3DispatchStateEb5a0* dispatch = nullptr;
    std::uint32_t* state_ptr = nullptr;
};

std::int64_t /* Decompiled name: parser_ready_frame_push_copy */
parser_ready_frame_push_copy(
    std::uint64_t frame_deque_ptr,
    std::uint64_t frame_ptr,
    std::uint64_t copy_bytes,
    const ParserReadyFrameCopyContext* copy_ctx);
void /* Decompiled name: parser_state_sink_notify */
parser_state_sink_notify(ParserStateSinkContext* sink, std::uint32_t state);

struct DecoderParserRunContext1034e0 {
    std::uint64_t timeline_cursor = 0;
    std::uint64_t delay_line_ptr = 0;
    std::uint64_t block_size = 0;
    std::uint64_t frame_deque_ptr = 0;
    std::uint64_t ready_frame_deque_ptr = 0;
    std::uint64_t parse_result_pool_ptr = 0;
    std::uint8_t* parser_slots_base = nullptr;
    std::size_t parser_slots_size = 0;
    std::uint32_t* parser_state_inout = nullptr;
    CodecV3ParserRuntimeFns1034e0 runtime_fns{};
};

std::int64_t /* Decompiled name: decoder_run_parser */
decoder_run_parser(DecoderParserRunContext1034e0* ctx);

struct DecoderParserStageContext1034e0 {
    CodecV3DispatchStateEb5a0* dispatch = nullptr;
    DelayLineState106b40* delay_line = nullptr;
    std::uint8_t* output_generator_base = nullptr;
    std::uint8_t* frame_deque_base = nullptr;
    std::uint8_t* ready_frame_deque_base = nullptr;
    std::uint8_t* parse_result_pool_base = nullptr;
    std::uint8_t* parser_slots_base = nullptr;
    std::size_t parser_slots_size = 0;
    std::uint8_t* ready_parse_result_base = nullptr;
    std::size_t ready_parse_result_size = 0;
    std::uint64_t block_size = 0;
    std::uint64_t* timeline_cursor_ptr = nullptr;
    std::uint32_t* parser_state_ptr = nullptr;
    CodecV3ParserRuntimeFns1034e0 runtime_fns{};
};

std::int64_t /* Decompiled name: decoder_run_parser_stage */
decoder_run_parser_stage(DecoderParserStageContext1034e0* ctx);

struct DecoderOutputStageContext1024a9 {
    DelayLineState106b40* delay_line = nullptr;
    std::uint8_t* output_generator_base = nullptr;
    std::uint8_t* output_table_base = nullptr;
    std::size_t output_table_size = 0;
    const std::uint64_t* output_channel_ptrs_27 = nullptr;
    const std::uint64_t* input_channel_ptrs_27 = nullptr;
    std::uint32_t input_mask = 0;
    std::uint8_t* ready_frame_deque_base = nullptr;
    std::uint64_t total_samples = 0;
    std::uint32_t* produced_output_mask = nullptr;
};

std::int64_t /* Decompiled name: decoder_run_output_stage */
decoder_run_output_stage(
    const DecoderOutputStageContext1024a9* ctx,
    const void* runtime_fns);

struct DecoderInitFakeFrameChannelsContext106ba0 {
    std::uint8_t* output_generator_base = nullptr;
    std::uint8_t* fake_frame = nullptr;
    std::uint8_t* fake_frame_next = nullptr;
    std::uint8_t* fake_frame_channels_base = nullptr;
    std::size_t fake_frame_channel_bytes = 0;
    std::uint32_t* fake_frame_channel_words_base = nullptr;
    std::uint8_t* fake_frame_channel_ctx_base = nullptr;
    std::uint64_t parse_result_pool_base = 0;
    std::uint32_t input_mask = 0;
    std::uint32_t block_size = 0;
    std::uint32_t active_decode_probe_slots = 0;
};

void /* Decompiled name: decoder_init_fake_frame_channels */
decoder_init_fake_frame_channels(
    const DecoderInitFakeFrameChannelsContext106ba0* ctx);

struct DecoderInitFrameDequesContext13d5d0 {
    FrameDequeState13d570* frame_deque = nullptr;
    FrameDequeState13d570* ready_frame_deque = nullptr;
    std::uint8_t* fake_frame = nullptr;
    std::uint8_t* fake_frame_next = nullptr;
    std::uint8_t* ready_frame = nullptr;
    std::uint8_t* ready_frame_next = nullptr;
    std::uint32_t deque_capacity = 0;
    std::uint64_t frame_span = 0;
    std::uint32_t frame_copy_bytes = 0;
    void (*rebind_frame_parse_results)(void* user, std::uint64_t frame_ptr) = nullptr;
    void* rebind_user = nullptr;
};

void /* Decompiled name: decoder_init_frame_deques */
decoder_init_frame_deques(DecoderInitFrameDequesContext13d5d0* ctx);

struct DecoderDispatchRunContextEb5a0 {
    CodecV3DispatchStateEb5a0* dispatch = nullptr;
    FormatDetectorTail1056c0* format_detector = nullptr;
    SyncDetectorState105ee0* sync_detector = nullptr;
    DelayLineState106b40* delay_line = nullptr;
    std::uint32_t sample_rate = 0;
    std::uint32_t block_size = 0;
    std::uint32_t input_mask = 0;
    std::uint32_t output_mask = 0;
    const std::uint64_t* input_channel_ptrs_27 = nullptr;
    const std::uint64_t* output_channel_ptrs_27 = nullptr;
    void (*set_layout)(void* user, std::uint32_t layout_mask) = nullptr;
    void (*process_block)(void* user, const std::uint64_t* channel_ptrs_27) = nullptr;
    void* sync_user = nullptr;
};

std::int64_t /* Decompiled name: decoder_run_dispatch */
decoder_run_dispatch(DecoderDispatchRunContextEb5a0* ctx);

struct DecoderStepRunContext101800 {
    DecoderDispatchRunContextEb5a0* dispatch_ctx = nullptr;
    const CodecV3IoBufferDescEb5a0* parser_input_desc = nullptr;
    const ParserPayloadRefreshContext* payload_ctx = nullptr;
    std::uint64_t delay_line_ptr = 0;
    void (*run_parser_stage)(void* user) = nullptr;
    void* parser_user = nullptr;
    std::int64_t (*run_output_stage)(void* user) = nullptr;
    void* output_user = nullptr;
};

std::int64_t /* Decompiled name: decoder_run_step */
decoder_run_step(DecoderStepRunContext101800* ctx);

// Перенос auro_codec_v3_decoder_OutputGenerator_process @ 0x102240.
struct OutputGeneratorSegment {
    std::uint64_t start = 0;
    std::uint64_t len = 0;
    std::uint32_t channel_mask = 0;
    std::uint32_t frame_flags = 0;
    std::uint64_t frame_ptr = 0;
    bool frame_has_started = false;
    bool prefer_started_decode_path = false; // IDA v131: started flag для выбора decode-подветки.
};
static_assert(sizeof(OutputGeneratorSegment) >= 24, "OutputGeneratorSegment must hold IDA 24-byte range payload.");

struct OutputGeneratorSegmentPlan {
    std::vector<OutputGeneratorSegment> segments;
    std::uint32_t segment_count = 0;
    std::uint64_t delay_line_buffer = 0; // exact IDA buffer from DelayLine_get_buffer(...)
    std::int64_t delay_line_state_offset = 0;
};

struct OutputGeneratorExtrapolateSources;

// Callback-интерфейс для следующей фазы 0x1024A9 (decode/copy по сегментам).
struct OutputGeneratorApplyCallbacks {
    void* user = nullptr;
    bool copy_input_enabled = false;
    std::uint32_t input_mask = 0;
    std::uint64_t output_block_start = 0;
    std::uint64_t (*get_delay_line_channel)(void* user, std::uint32_t channel, std::uint64_t start) = nullptr;
    std::uint64_t (*get_input_channel_base)(void* user, std::uint32_t channel) = nullptr;
    std::uint64_t (*get_output_channel_base)(void* user, std::uint32_t channel) = nullptr;
    std::int64_t (*decode_channel_segment)(
        void* user,
        const OutputGeneratorSegment* seg,
        std::uint32_t channel,
        std::uint64_t src_ptr,
        std::uint64_t dst_ptr,
        std::uint64_t sample_count,
        const OutputGeneratorExtrapolateSources* extrap_sources) = nullptr;
};

// Callback-интерфейс для IDA-блока 0x10247B..0x1024CB:
// цикл по frame channels с вызовами GolombRice_initialize + Extrapolate_initialize.
struct OutputGeneratorFrameInitCallbacks {
    void* user = nullptr;
    std::uint32_t (*get_frame_channel_count)(void* user, std::uint64_t frame_ptr) = nullptr;
    std::uint64_t (*get_frame_channel_ptr)(void* user, std::uint64_t frame_ptr, std::uint32_t idx) = nullptr;
    std::uint64_t (*get_golombrice_state_ptr)(void* user, std::uint32_t idx) = nullptr;
    std::uint64_t (*get_extrapolate_state_ptr)(void* user, std::uint32_t idx) = nullptr;
    std::uint32_t* (*get_channel_words_ptr)(void* user, std::uint64_t frame_channel_ptr) = nullptr;
    std::uint64_t (*get_channel_ctx_ptr)(void* user, std::uint64_t frame_channel_ptr) = nullptr;
    std::int64_t (*golombrice_initialize)(
        void* user,
        std::uint64_t gr_state_ptr,
        std::uint32_t* words_ptr,
        std::uint64_t ctx_ptr) = nullptr;
    std::int64_t (*extrapolate_initialize)(
        void* user,
        std::uint64_t extrap_state_ptr,
        std::uint64_t frame_channel_ptr) = nullptr;
};

OutputGeneratorSegmentPlan output_generator_build_segment_plan_1024a9(
    std::uint8_t* output_generator_base,
    std::uint32_t timeline_cursor,
    std::uint32_t* io_channel_mask_out);

// Частичный перенос decode/copy-ветки: проход по активным каналам сегмента.
// Если seg.frame_ptr!=0 && seg.frame_has_started==true, вызывается decode_channel_segment;
// иначе выполняется прямое копирование int32 сэмплов из delay-line в output.
std::int64_t output_generator_apply_segments_1024a9(
    const OutputGeneratorSegmentPlan& plan,
    const OutputGeneratorApplyCallbacks& cb,
    std::uint32_t* io_channel_mask_out);

// Частичный перенос фазы frame-init (0x10247B..0x1024CB) для одного сегмента.
// Вызывается только если frame_start == timeline_cursor (IDA 0x102472..0x10247B).
std::int64_t output_generator_prepare_frame_channels_1024a9(
    const OutputGeneratorSegment& seg,
    std::uint64_t timeline_cursor,
    const OutputGeneratorFrameInitCallbacks& cb);

// Объединённый частичный pipeline сегмента в порядке IDA:
// optional frame-init -> decode/copy -> накопление channel mask.
std::int64_t output_generator_process_segments_1024a9(
    const OutputGeneratorSegmentPlan& plan,
    std::uint64_t timeline_cursor_at_entry,
    const OutputGeneratorFrameInitCallbacks* frame_init_cb,
    const OutputGeneratorApplyCallbacks& apply_cb,
    std::uint32_t* io_channel_mask_out);

// Raw-adapter к layout из IDA для частичного запуска 0x102240 на "живом" OutputGenerator-объекте.
// Здесь frame-init callbacks собираются автоматически из смещений frame/channel.
struct OutputGeneratorRuntimeFns1024a9 {
    std::uint64_t (*delay_line_get_channel)(std::uint64_t delay_line_buffer, std::uint32_t channel, std::uint64_t start) = nullptr;
    std::uint64_t (*delay_line_get_buffer)(std::uint64_t delay_line_ptr, std::uint64_t timeline_cursor, std::int64_t* io_state) = nullptr;
    std::uint64_t (*frame_deque_find_first_with_end_after)(std::uint64_t frame_deque_ptr, std::uint64_t sample_pos) = nullptr;
    std::int64_t (*pre_segments_callback)(
        std::uint64_t ctx,
        std::uint64_t ranges_base,
        std::uint64_t started_base) = nullptr; // v6[99](v6[100], *v133, v133[1])
    std::uint64_t pre_segments_ctx = 0;
    void (*metadata_update_callback)(
        std::uint64_t ctx,
        std::uint64_t metadata_table) = nullptr; // v6[101](v6[102], v6 + 111)
    std::uint64_t metadata_update_ctx = 0;
    std::int64_t (*decode_channel_segment)(
        std::uint8_t* output_generator_base,
        const OutputGeneratorSegment* seg,
        std::uint32_t channel,
        std::uint64_t src_ptr,
        std::uint64_t dst_ptr,
        std::uint64_t sample_count,
        const OutputGeneratorExtrapolateSources* extrap_sources) = nullptr;
    std::int64_t (*golombrice_get_errors)(
        std::uint64_t gr_state_ptr,
        std::uint64_t frame_channel_ptr,
        std::uint64_t errors_buf_ptr,
        std::uint64_t sample_count) = nullptr;
    std::int64_t (*extrapolate_process)(
        std::uint64_t ex_state_ptr,
        std::uint64_t channel_ptr,
        std::uint64_t errors_buf_ptr,
        std::uint64_t src0_ptr,
        std::uint64_t src1_ptr,
        std::uint64_t src2_ptr,
        std::uint64_t sample_count) = nullptr;
    std::int64_t (*golombrice_initialize)(std::uint64_t gr_state_ptr, std::uint32_t* words_ptr, std::uint64_t ctx_ptr) = nullptr;
    std::int64_t (*extrapolate_initialize)(std::uint64_t ex_state_ptr, std::uint64_t frame_channel_ptr) = nullptr;
    void (*frame_mark_as_unused)(std::uint64_t frame_ptr) = nullptr;
    void (*frame_deque_pop_front)(std::uint64_t frame_deque_ptr) = nullptr;
};

struct OutputGeneratorExtrapolateSources {
    std::uint64_t src0 = 0; // индекс [27] или fallback scratch+0*total
    std::uint64_t src1 = 0; // индекс [28] или fallback scratch+1*total
    std::uint64_t src2 = 0; // индекс [29] или fallback scratch+2*total
    std::uint32_t frame_slot_index = 0xFFFFFFFFu; // slot index in frame table (если известен)
};

std::int64_t output_generator_process_segments_raw_1024a9(
    std::uint8_t* output_generator_base,
    const OutputGeneratorSegmentPlan& plan,
    std::uint64_t output_channels_table_base,
    const OutputGeneratorRuntimeFns1024a9& fns,
    std::uint32_t* io_channel_mask_out,
    const CodecV3IoBufferDescEb5a0* input_desc = nullptr,
    std::uint32_t input_mask = 0);

std::int64_t /* Decompiled name: output_generator_cross_fade */
output_generator_cross_fade(
    std::uint8_t* output_generator_base,
    std::uint64_t output_channels_table_base,
    std::uint32_t fade_in_mask,
    std::uint64_t delay_line_buffer,
    std::uint64_t segment_start = 0u);

// Entry-point по IDA 0x102240: build-plan -> pre-segments callback -> segment loop -> update cursor/pop_front.
// external_mask_inout соответствует аргументу a3 в оригинале (*a3 |= seg_mask).
std::int64_t output_generator_process_1024a9(
    std::uint8_t* output_generator_base,
    std::uint64_t output_channels_table_base,
    const OutputGeneratorRuntimeFns1024a9& fns,
    std::uint32_t* external_mask_inout,
    const CodecV3IoBufferDescEb5a0* input_desc = nullptr,
    std::uint32_t input_mask = 0);

// Совместимый wrapper для существующих вызовов.
std::int64_t /* Decompiled name: output_generator_process */
output_generator_process(
    std::uint8_t* output_generator_base,
    std::uint64_t output_channels_table_base,
    const OutputGeneratorRuntimeFns1024a9& fns,
    std::uint32_t* external_mask_inout,
    const CodecV3IoBufferDescEb5a0* input_desc = nullptr,
    std::uint32_t input_mask = 0);

struct DecoderOutputGeneratorRunContext1024a9 {
    std::uint8_t* output_generator_base = nullptr;
    std::uint8_t* output_channels_table = nullptr;
    std::size_t output_channels_table_size = 0;
    const std::uint64_t* output_channel_ptrs_27 = nullptr;
    const std::uint64_t* input_channel_ptrs_27 = nullptr;
    std::uint32_t input_mask = 0;
    std::uint64_t delay_line_ptr = 0;
    std::uint64_t frame_deque_ptr = 0;
    std::uint64_t total_samples = 0;
    std::uint32_t* produced_output_mask = nullptr;
    OutputGeneratorRuntimeFns1024a9 runtime_fns{};
};

std::int64_t /* Decompiled name: decoder_run_output_generator */
decoder_run_output_generator(
    const DecoderOutputGeneratorRunContext1024a9* ctx);

void /* Decompiled name: output_generator_construct */
output_generator_construct(
    std::uint8_t* output_generator_base,
    const std::uint8_t* output_config,
    std::uint64_t memory_base);

std::uint8_t* /* Decompiled name: auro_a3deng_v4_android_A3DENG_construct */
auro_a3deng_v4_android_A3DENG_construct(
    std::uint8_t* a3deng_base,
    std::uint32_t pipeline_audio_block_size,
    std::uint32_t output_mode);
std::uint8_t* /* Decompiled name: auro_a3deng_v4_android_A3DENG_AuroInitialize */
auro_a3deng_v4_android_A3DENG_AuroInitialize(
    std::uint32_t pipeline_audio_block_size,
    std::uint32_t output_mode);
void /* Decompiled name: auro_a3deng_v4_android_A3DENG_destroy */
auro_a3deng_v4_android_A3DENG_destroy(std::uint8_t* a3deng_base);
std::string /* Decompiled name: auro_a3deng_v4_android_A3DENG_AuroVersion */
auro_a3deng_v4_android_A3DENG_AuroVersion();
std::int64_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_AuroIsValid */
auro_a3deng_v4_android_A3DENG_AuroIsValid(
    std::uint8_t* a3deng_base);
bool /* Decompiled name: auro_a3deng_v4_android_A3DENG_AuroRelease */
auro_a3deng_v4_android_A3DENG_AuroRelease(std::uint8_t* a3deng_base);
std::int64_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_AuroReset */
auro_a3deng_v4_android_A3DENG_AuroReset(
    std::uint8_t* a3deng_base);
std::int64_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_AuroPush */
auro_a3deng_v4_android_A3DENG_AuroPush(
    std::uint8_t* a3deng_base,
    const std::uint8_t* input_bytes,
    std::int32_t input_byte_count);
std::int64_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_AuroGetLatencyUs */
auro_a3deng_v4_android_A3DENG_AuroGetLatencyUs(
    std::uint8_t* a3deng_base);
std::int64_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_AuroGetOutputChannelCount */
auro_a3deng_v4_android_A3DENG_AuroGetOutputChannelCount(
    std::uint8_t* a3deng_base);
std::int64_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_AuroPop */
auro_a3deng_v4_android_A3DENG_AuroPop(
    std::uint8_t* a3deng_base,
    std::uint8_t* output_bytes,
    std::int32_t output_byte_count);
std::int64_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_AuroGetMaximumOutputBytecount */
auro_a3deng_v4_android_A3DENG_AuroGetMaximumOutputBytecount(
    std::uint8_t* a3deng_base);
std::int64_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_AuroInputBlockSize */
auro_a3deng_v4_android_A3DENG_AuroInputBlockSize(
    std::uint8_t* a3deng_base);
std::int64_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_AuroResetAudioState */
auro_a3deng_v4_android_A3DENG_AuroResetAudioState(
    std::uint8_t* a3deng_base);
bool /* Decompiled name: auro_a3deng_v4_android_A3DENG_AuroSetDebugPath */
auro_a3deng_v4_android_A3DENG_AuroSetDebugPath(
    std::uint8_t* a3deng_base,
    const char* path_utf8,
    const char* tag_utf8);

/// Поля в порядке JNI AuroUpdate2 (Artist Connection libauro.so @ 0x318D60).
struct A3DENGSettingsFields318d60 {
    bool is_stereo_device = false;
    bool headset_connected = false;
    std::uint32_t decoder_mode = 0u;
    std::uint32_t output_layout_mask = 0u;
    std::uint32_t output_sample_type = 0u;
    std::uint32_t output_bit_depth = 0u;
    std::uint32_t pcm_input_layout_mask = 0u;
    std::uint32_t pcm_input_sample_rate = 0u;
    std::uint32_t pcm_input_sample_type = 0u;
    bool channels_backs_before_surrounds = false;
    bool abr_mode_enabled = false;
    bool virtualization_enabled = false;
    bool listening_mode_auro3d = false;
    std::uint32_t hp_user_preset = 0u;
    std::uint32_t hp_hrtf_preset = 0u;
};

void auro_a3deng_v4_android_A3DENG_settings_pack_like_jni_318d60(
    std::uint8_t* out_0x34,
    const A3DENGSettingsFields318d60& f);
std::int64_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_AuroUpdate2 */
auro_a3deng_v4_android_A3DENG_AuroUpdate2(
    std::uint8_t* a3deng_base,
    const A3DENGSettingsFields318d60& f);

bool /* Decompiled name: auro_a3deng_v4_android_A3DENG_update */
auro_a3deng_v4_android_A3DENG_update(
    std::uint8_t* a3deng_base,
    const std::uint8_t* settings_0x34);
bool a3deng_output_info_valid_31b4e0(std::uint64_t output_info);
std::uint64_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_get_output_info */
auro_a3deng_v4_android_A3DENG_get_output_info(
    std::uint8_t* a3deng_base);
std::uint64_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_get_maximum_output_bytecount */
auro_a3deng_v4_android_A3DENG_get_maximum_output_bytecount(
    std::uint8_t* a3deng_base);
std::uint32_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_input_block_size */
auro_a3deng_v4_android_A3DENG_input_block_size(
    const std::uint8_t* a3deng_base);
struct A3DENGVersionFields31bbe0 {
    std::uint32_t major = 0u;
    std::uint32_t minor = 0u;
    std::uint32_t patch = 0u;
    std::uint32_t beta = 0x7FFFFFFFu;
    const char* tag = nullptr;
};
bool /* Decompiled name: auro_a3deng_v4_android_A3DENG_get_version */
auro_a3deng_v4_android_A3DENG_get_version(
    const std::uint8_t* a3deng_base,
    A3DENGVersionFields31bbe0* out);
bool /* Decompiled name: auro_a3deng_v4_android_A3DENG_destroy_instance */
auro_a3deng_v4_android_A3DENG_destroy_instance(
    std::uint8_t* a3deng_base);
bool /* Decompiled name: auro_a3deng_v4_android_A3DENG_create_instance */
auro_a3deng_v4_android_A3DENG_create_instance(
    std::uint8_t* a3deng_base,
    std::uint32_t decoder_mode);
std::uint8_t* /* Decompiled name: auro_a3deng_v4_android_A3DENG_settings */
auro_a3deng_v4_android_A3DENG_settings(
    std::uint8_t* a3deng_base);
const std::uint8_t* /* Decompiled name: auro_a3deng_v4_android_A3DENG_settings */
auro_a3deng_v4_android_A3DENG_settings(
    const std::uint8_t* a3deng_base);
std::uint32_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_calculate_output_sample_rate */
auro_a3deng_v4_android_A3DENG_calculate_output_sample_rate(
    std::uint32_t input_sample_rate,
    std::uint32_t output_mode);
std::uint32_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_get_output_sample_rate */
auro_a3deng_v4_android_A3DENG_get_output_sample_rate(
    const std::uint8_t* a3deng_base);
std::uint32_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_Settings_Config_target_device */
auro_a3deng_v4_android_A3DENG_Settings_Config_target_device(
    const std::uint8_t* settings_0x34);
bool /* Decompiled name: auro_a3deng_v4_android_A3DENG_Settings_Config_equals */
auro_a3deng_v4_android_A3DENG_Settings_Config_equals(
    const std::uint8_t* lhs_0x34,
    const std::uint8_t* rhs_0x34);
std::uint32_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_Settings_virtualization_mode */
auro_a3deng_v4_android_A3DENG_Settings_virtualization_mode(
    const std::uint8_t* settings_0x34);
std::uint32_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_Settings_listening_mode */
auro_a3deng_v4_android_A3DENG_Settings_listening_mode(
    const std::uint8_t* settings_0x34);
bool /* Decompiled name: auro_a3deng_v4_android_A3DENG_reset */
auro_a3deng_v4_android_A3DENG_reset(std::uint8_t* a3deng_base);
std::int64_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_get_latency_nr_samples */
auro_a3deng_v4_android_A3DENG_get_latency_nr_samples(
    std::uint8_t* a3deng_base);
std::int64_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_get_latency_us */
auro_a3deng_v4_android_A3DENG_get_latency_us(
    std::uint8_t* a3deng_base);
std::int64_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_push */
auro_a3deng_v4_android_A3DENG_push(
    std::uint8_t* a3deng_base,
    const std::uint8_t* input_bytes,
    std::uint32_t input_byte_count);
bool /* Decompiled name: auro_a3deng_v4_android_A3DENG_reset_audio_state */
auro_a3deng_v4_android_A3DENG_reset_audio_state(std::uint8_t* a3deng_base);
bool /* Decompiled name: auro_a3deng_v4_android_A3DENG_set_debug_path */
auro_a3deng_v4_android_A3DENG_set_debug_path(
    std::uint8_t* a3deng_base,
    const char* path_utf8,
    const char* tag_utf8);
std::int64_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_pop */
auro_a3deng_v4_android_A3DENG_pop(
    std::uint8_t* a3deng_base,
    std::uint8_t* output_bytes,
    std::int32_t output_byte_count);
bool /* Decompiled name: auro_a3deng_v4_android_A3DENG_pop_internal */
auro_a3deng_v4_android_A3DENG_pop_internal(
    std::uint8_t* a3deng_base,
    std::uint8_t*& output_bytes,
    std::int32_t& remaining_output_byte_count);
std::uint32_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_get_output_layout */
auro_a3deng_v4_android_A3DENG_get_output_layout(
    std::uint8_t* a3deng_base);
std::uint32_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_get_output_channel_count */
auro_a3deng_v4_android_A3DENG_get_output_channel_count(
    std::uint8_t* a3deng_base);

/// Host hook: synthetic A3DENG pop (mode 2) → partial codec-v3 decode вместо passthrough.
using A3dengCodecV3PopRenderFn = std::uint64_t (*)(
    void* user,
    std::uint8_t* a3deng_base,
    std::uint8_t* output_bytes,
    std::uint32_t frames,
    std::uint32_t output_mask,
    std::uint32_t input_mask);
void /* Decompiled name: auro_a3deng_v4_android_A3DENG_set_codec_v3_pop_render_hook */
auro_a3deng_v4_android_A3DENG_set_codec_v3_pop_render_hook(
    std::uint8_t* a3deng_base,
    A3dengCodecV3PopRenderFn fn,
    void* user);
const std::uint8_t* /* Decompiled name: auro_a3deng_v4_android_A3DENG_partial_queue_input_data */
auro_a3deng_v4_android_A3DENG_partial_queue_input_data(
    std::uint8_t* a3deng_base);
std::size_t /* Decompiled name: auro_a3deng_v4_android_A3DENG_partial_queue_input_size */
auro_a3deng_v4_android_A3DENG_partial_queue_input_size(
    std::uint8_t* a3deng_base);
bool /* Decompiled name: auro_a3deng_v4_android_A3DENG_consume_interleaved_input_to_planar_i32 */
auro_a3deng_v4_android_A3DENG_consume_interleaved_input_to_planar_i32(
    std::uint8_t* a3deng_base,
    std::uint32_t frames,
    std::uint32_t input_mask,
    const std::uint64_t* out_channel_ptrs_27,
    std::uint32_t plane_stride_samples);
std::uint64_t /* Decompiled name: a3deng_write_pruned_interleaved_from_planar_i32 */
a3deng_write_pruned_interleaved_from_planar_i32(
    std::uint8_t* a3deng_base,
    std::uint8_t* output_bytes,
    std::uint32_t frames,
    const std::uint64_t* channel_ptrs_27,
    std::uint32_t output_sample_type);

void /* Decompiled name: auro_a3deng_v4_android_channel_layout */
auro_a3deng_v4_android_channel_layout(
    std::uint8_t* layout_0x188,
    std::uint32_t channel_mask,
    std::uint32_t hdmi_channel_mapping);

/// sub_31CB30 @ 0x31CB30 — три QWORD как `std::vector` begin/end/cap конца.
void /* Decompiled name: a3deng_u32_vector_assign_sub */
a3deng_u32_vector_assign_sub(
    std::uint8_t* vector_base24,
    const void* src_bytes,
    std::size_t uint32_element_count);

/// Порядок слотов при hdmi_channel_mapping==1 (xmmword_1DB2B0 в sub_31ACE0).
std::uint32_t /* Decompiled name: a3deng_channel_mask_slots_hdmi_back_before_surround */
a3deng_channel_mask_slots_hdmi_back_before_surround(
    std::uint32_t* out_slots,
    std::uint32_t out_cap,
    std::uint32_t channel_mask);

/// auro_iir_biquad_parameter_Config_float32_t_compute @ 0x6387D0 (float64 construct/update/get_coeffs → float32 coeffs).
std::int64_t /* Decompiled name: auro_iir_biquad_parameter_Config_float32_t_compute */
auro_iir_biquad_parameter_Config_float32_t_compute(
    std::uint8_t* param_stack12,
    std::uint64_t cfg_ptr,
    std::uint8_t* coeff_state_out);

constexpr std::size_t kAsc4heElevationEqStateBytes = 160u;
std::int64_t /* Decompiled name: auro_asc4he_v1_ElevationEQ_initialize */
auro_asc4he_v1_ElevationEQ_initialize(std::uint8_t* state, std::uint64_t cfg_ptr);
void /* Decompiled name: auro_asc4he_v1_ElevationEQ_reset_audio_state */
auro_asc4he_v1_ElevationEQ_reset_audio_state(std::uint8_t* state);
std::int64_t /* Decompiled name: auro_asc4he_v1_ElevationEQ_process */
auro_asc4he_v1_ElevationEQ_process(float* block, std::uint64_t* io_pair);

constexpr std::size_t kAsc4heVirtualHeightStateBytes = 2048u;
std::int64_t /* Decompiled name: auro_asc4he_v1_blocked_Delay_initialize */
auro_asc4he_v1_blocked_Delay_initialize(
    std::uint8_t* delay_state,
    std::int32_t a2,
    std::uint8_t* buffer_base,
    std::uint32_t buffer_bytes,
    std::int32_t a5);
std::int64_t /* Decompiled name: auro_asc4he_v1_blocked_Delay_reset_audio_state */
auro_asc4he_v1_blocked_Delay_reset_audio_state(std::uint8_t* delay_state);
std::int64_t /* Decompiled name: auro_asc4he_v1_blocked_Delay_process */
auro_asc4he_v1_blocked_Delay_process(
    std::uint8_t* delay_state,
    std::uint8_t* io_pair_16);

constexpr std::size_t kAsc4heDelay7msStateBytes = 3360u;
std::int64_t /* Decompiled name: auro_asc4he_v1_Delay7ms_initialize */
auro_asc4he_v1_Delay7ms_initialize(std::uint8_t* state, std::uint32_t sample_rate);
std::int64_t /* Decompiled name: auro_asc4he_v1_Delay7ms_reset_audio_state */
auro_asc4he_v1_Delay7ms_reset_audio_state(std::uint8_t* state);
std::int64_t /* Decompiled name: auro_asc4he_v1_Delay7ms_process */
auro_asc4he_v1_Delay7ms_process(std::uint8_t* state, std::uint8_t* io_pair_16);

constexpr std::size_t kAsc4heDelay10msStateBytes = 4384u;
std::int64_t /* Decompiled name: auro_asc4he_v1_Delay10ms_initialize */
auro_asc4he_v1_Delay10ms_initialize(
    std::uint8_t* state,
    std::int32_t delay_us,
    std::uint32_t sample_rate);
std::int64_t /* Decompiled name: auro_asc4he_v1_Delay10ms_reset_audio_state */
auro_asc4he_v1_Delay10ms_reset_audio_state(std::uint8_t* state);
std::int64_t /* Decompiled name: auro_asc4he_v1_Delay10ms_process */
auro_asc4he_v1_Delay10ms_process(std::uint8_t* state, std::uint8_t* io_pair_16);

constexpr std::size_t kAsc4heDecorrelatorStateBytes = 320u;
std::int64_t /* Decompiled name: auro_asc4he_v1_Decorrelator_initialize */
auro_asc4he_v1_Decorrelator_initialize(std::uint8_t* state, std::uint64_t cfg_ptr);
void /* Decompiled name: auro_asc4he_v1_Decorrelator_reset_audio_state */
auro_asc4he_v1_Decorrelator_reset_audio_state(std::uint8_t* state);
std::int64_t /* Decompiled name: auro_asc4he_v1_Decorrelator_process */
auro_asc4he_v1_Decorrelator_process(float* block, std::uint64_t* io_pair);

constexpr std::size_t kAsc4heCrossTalkCompensationStateBytes = 112u;
std::int64_t /* Decompiled name: auro_asc4he_v1_CrossTalkCompensation_initialize */
auro_asc4he_v1_CrossTalkCompensation_initialize(
    std::uint8_t* state,
    const std::uint8_t* params,
    std::uint32_t sample_rate);
void /* Decompiled name: auro_asc4he_v1_CrossTalkCompensation_reset_audio_state */
auro_asc4he_v1_CrossTalkCompensation_reset_audio_state(std::uint8_t* state);
float* /* Decompiled name: auro_asc4he_v1_CrossTalkCompensation_process */
auro_asc4he_v1_CrossTalkCompensation_process(float* state, float** io_pair);

std::int64_t /* Decompiled name: auro_asc4he_v1_VirtualHeight_initialize */
auro_asc4he_v1_VirtualHeight_initialize(std::uint8_t* state, std::uint64_t cfg_ptr);
std::int64_t /* Decompiled name: auro_asc4he_v1_VirtualHeight_reset_audio_state */
auro_asc4he_v1_VirtualHeight_reset_audio_state(std::uint8_t* state);
std::int64_t /* Decompiled name: auro_asc4he_v1_VirtualHeight_process */
auro_asc4he_v1_VirtualHeight_process(std::uint8_t* state, void** channel_ptrs);

constexpr std::size_t kAsc4heCrossTalkStateBytes = 128u;
std::int64_t /* Decompiled name: auro_asc4he_v1_CrossTalk_initialize */
auro_asc4he_v1_CrossTalk_initialize(std::uint8_t* state, const float* params2);
void /* Decompiled name: auro_asc4he_v1_CrossTalk_reset_audio_state */
auro_asc4he_v1_CrossTalk_reset_audio_state(std::uint8_t* state);
std::int64_t /* Decompiled name: auro_asc4he_v1_CrossTalk_process */
auro_asc4he_v1_CrossTalk_process(std::uint8_t* state, std::uint64_t* io_pair);

constexpr std::size_t kAsc4heCenterGenFixedDefaultsBytes = 20u;
constexpr std::size_t kAsc4heCenterGenDynamicDefaultsBytes = 56u;
void /* Decompiled name: auro_asc4he_v1_CenterGen_get_default_fixed_parameters */
auro_asc4he_v1_CenterGen_get_default_fixed_parameters(std::uint8_t* out20);
void /* Decompiled name: auro_asc4he_v1_CenterGen_get_default_dynamic_parameters */
auro_asc4he_v1_CenterGen_get_default_dynamic_parameters(std::uint8_t* out56);

/// auro_centergen_v3_Processor_t @ libauro (memset 0x428 в construct).
constexpr std::size_t kCentergenV3ProcessorBytes = 0x428u;
void /* Decompiled name: auro_centergen_v3_default_fixed_params */
auro_centergen_v3_default_fixed_params(std::uint8_t* out20);
void /* Decompiled name: auro_centergen_v3_default_dynamic_params */
auro_centergen_v3_default_dynamic_params(std::uint8_t* out56);
std::int64_t /* Decompiled name: auro_centergen_v3_Processor_set_fixed_parameters */
auro_centergen_v3_Processor_set_fixed_parameters(std::uint8_t* proc, const std::uint8_t* fixed20);
void /* Decompiled name: auro_centergen_v3_Processor_set_dynamic_parameters */
auro_centergen_v3_Processor_set_dynamic_parameters(std::uint8_t* proc, const std::uint8_t* dyn56);
std::uint64_t /* Decompiled name: auro_centergen_v3_Processor_t_construct */
auro_centergen_v3_Processor_t_construct(std::uint8_t* proc, std::int32_t* sample_rate_and_mode);
std::int64_t /* Decompiled name: auro_asc4he_v1_CenterGen_initialize */
auro_asc4he_v1_CenterGen_initialize(
    std::uint8_t* proc,
    std::int32_t sample_rate_hz,
    const std::uint8_t* fixed20,
    const std::uint8_t* dyn56);
std::int64_t /* Decompiled name: auro_asc4he_v1_CenterGen_reset_audio_state */
auro_asc4he_v1_CenterGen_reset_audio_state(std::uint8_t* proc);
std::int64_t /* Decompiled name: auro_centergen_v3_Processor_reset_audio_state */
auro_centergen_v3_Processor_reset_audio_state(std::uint8_t* proc);
std::int64_t /* Decompiled name: auro_centergen_v3_Processor_get_fixed_parameters */
auro_centergen_v3_Processor_get_fixed_parameters(const std::uint8_t* proc, std::uint8_t* out20);
std::int64_t /* Decompiled name: auro_centergen_v3_Processor_get_dynamic_parameters */
auro_centergen_v3_Processor_get_dynamic_parameters(const std::uint8_t* proc, std::uint8_t* out56);
std::int64_t /* Decompiled name: auro_asc4he_v1_CenterGen_get_fixed_parameters */
auro_asc4he_v1_CenterGen_get_fixed_parameters(const std::uint8_t* proc, std::uint8_t* out20);
std::int64_t /* Decompiled name: auro_asc4he_v1_CenterGen_set_dynamic_parameters */
auro_asc4he_v1_CenterGen_set_dynamic_parameters(std::uint8_t* proc, const std::uint8_t* dyn56);
std::int64_t /* Decompiled name: auro_asc4he_v1_CenterGen_get_dynamic_parameters */
auro_asc4he_v1_CenterGen_get_dynamic_parameters(const std::uint8_t* proc, std::uint8_t* out56);
std::int64_t /* Decompiled name: auro_centergen_v3_Processor_process */
auro_centergen_v3_Processor_process(
    std::uint8_t* proc,
    std::uint64_t io_pair_q0,
    std::uint64_t io_pair_q1,
    std::uint8_t* block_or_side_ctx,
    std::int32_t frame_count);
std::int64_t /* Decompiled name: auro_asc4he_v1_CenterGen_process */
auro_asc4he_v1_CenterGen_process(
    std::uint8_t* proc,
    std::uint64_t* io_pair,
    std::uint8_t* block_or_side_ctx);

constexpr std::size_t kAsc4heCenterCrossOverStateBytes = 60u;
std::int64_t /* Decompiled name: auro_asc4he_v1_CenterCrossOver_initialize */
auro_asc4he_v1_CenterCrossOver_initialize(
    std::uint8_t* state,
    std::uint64_t sample_rate,
    std::int32_t enabled);
void /* Decompiled name: auro_asc4he_v1_CenterCrossOver_reset_audio_state */
auro_asc4he_v1_CenterCrossOver_reset_audio_state(std::uint8_t* state);
std::int64_t /* Decompiled name: auro_asc4he_v1_CenterCrossOver_process */
auro_asc4he_v1_CenterCrossOver_process(
    std::uint8_t* state,
    std::uint64_t out_center,
    std::uint64_t in_center);

// Точный выбор источников для Extrapolate_process из 0x1024A9:
// frame_channel[27..29] -> output table channel ptr + 4*segment_start, иначе fallback в scratch.
constexpr std::size_t kAsc4heCrossoverStateBytes = 504u;
std::int64_t /* Decompiled name: auro_asc4he_v1_Crossover_initialize */
auro_asc4he_v1_Crossover_initialize(
    std::uint8_t* state,
    std::uint32_t sample_rate,
    std::int32_t mode,
    std::int32_t channels);
std::int64_t /* Decompiled name: auro_asc4he_v1_Crossover_reset_audio_state */
auro_asc4he_v1_Crossover_reset_audio_state(std::uint8_t* state);
std::int64_t /* Decompiled name: auro_asc4he_v1_Crossover_process */
auro_asc4he_v1_Crossover_process(
    std::uint8_t* state,
    std::uint64_t* in_pair,
    std::uint64_t* low_pair,
    std::uint64_t* mid_pair,
    std::uint64_t* top_pair = nullptr);

constexpr std::size_t kAsc4heSideUpCrossoverStateBytes = 124u;
std::int64_t /* Decompiled name: auro_asc4he_v1_SideUpCrossover_initialize */
auro_asc4he_v1_SideUpCrossover_initialize(
    std::uint8_t* state,
    std::int32_t enabled,
    std::uint32_t cutoff_bits,
    std::uint32_t sample_rate);
std::int64_t /* Decompiled name: auro_asc4he_v1_SideUpCrossover_reset_audio_state */
auro_asc4he_v1_SideUpCrossover_reset_audio_state(std::uint8_t* state);
std::int64_t /* Decompiled name: auro_asc4he_v1_SideUpCrossover_process */
auro_asc4he_v1_SideUpCrossover_process(
    std::uint8_t* state,
    std::uint64_t* in_pair,
    std::uint64_t* high_pair,
    std::uint64_t* low_pair,
    std::uint64_t* add_pair);

constexpr std::size_t kAsc4heSurroundSatellitesStateBytes = 168u;
std::int64_t /* Decompiled name: auro_asc4he_v1_sb_SurroundSatellites_reset_audio_state */
auro_asc4he_v1_sb_SurroundSatellites_reset_audio_state(std::uint8_t* state);
std::int64_t /* Decompiled name: auro_asc4he_v1_sb_SurroundSatellites_initialize */
auro_asc4he_v1_sb_SurroundSatellites_initialize(
    std::uint8_t* state,
    const std::uint8_t* params,
    std::uint32_t sample_rate);
std::int64_t /* Decompiled name: auro_asc4he_v1_sb_SurroundSatellites_process_2_2 */
auro_asc4he_v1_sb_SurroundSatellites_process_2_2(
    float* state,
    std::uint64_t* channel_table);
void /* Decompiled name: auro_asc4he_v1_sb_SurroundSatellites_process_2_0 */
auro_asc4he_v1_sb_SurroundSatellites_process_2_0(
    float* state,
    std::uint64_t* channel_table);

constexpr std::size_t kAsc4heHeightSatellitesStateBytes = 4116u;
std::int64_t /* Decompiled name: auro_asc4he_v1_sb_HeightSatellites_reset_audio_state */
auro_asc4he_v1_sb_HeightSatellites_reset_audio_state(std::uint8_t* state);
std::int64_t /* Decompiled name: auro_asc4he_v1_sb_HeightSatellites_initialize */
auro_asc4he_v1_sb_HeightSatellites_initialize(
    std::uint8_t* state,
    const std::uint8_t* params,
    std::uint32_t sample_rate,
    std::uint32_t crossover_mode);
std::int64_t /* Decompiled name: auro_asc4he_v1_sb_HeightSatellites_process_2_0 */
auro_asc4he_v1_sb_HeightSatellites_process_2_0(
    std::uint8_t* state,
    std::uint64_t* channel_table,
    std::uint8_t* a3,
    float* work_128);
std::int64_t /* Decompiled name: auro_asc4he_v1_sb_HeightSatellites_process_2_2 */
auro_asc4he_v1_sb_HeightSatellites_process_2_2(
    std::uint8_t* state,
    std::uint8_t* channel_table,
    std::uint8_t* a3,
    float* work_128);

constexpr std::size_t kAsc4heCGenXOverBlockStateBytes = 1656u;
std::int64_t /* Decompiled name: auro_asc4he_v1_CGenXOverBlock_initialize */
auro_asc4he_v1_CGenXOverBlock_initialize(
    std::uint8_t* state,
    std::uint32_t sample_rate,
    std::uint32_t crossover_mode,
    const std::uint8_t* fixed20,
    const std::uint8_t* dyn56);
std::int64_t /* Decompiled name: auro_asc4he_v1_CGenXOverBlock_get_cgen_dynamic_parameters */
auro_asc4he_v1_CGenXOverBlock_get_cgen_dynamic_parameters(
    const std::uint8_t* state,
    std::uint8_t* out56);
std::int64_t /* Decompiled name: auro_asc4he_v1_CGenXOverBlock_set_cgen_dynamic_parameters */
auro_asc4he_v1_CGenXOverBlock_set_cgen_dynamic_parameters(
    std::uint8_t* state,
    const std::uint8_t* dyn56);
std::int64_t /* Decompiled name: auro_asc4he_v1_CGenXOverBlock_reset_audio_state */
auro_asc4he_v1_CGenXOverBlock_reset_audio_state(std::uint8_t* state);
float* /* Decompiled name: auro_asc4he_v1_CGenXOverBlock_process_without_external_center */
auro_asc4he_v1_CGenXOverBlock_process_without_external_center(
    std::uint8_t* state,
    float** io_pair,
    float** height_pair,
    float** surround_pair,
    std::uint64_t a5,
    float* work_768);
float* /* Decompiled name: auro_asc4he_v1_CGenXOverBlock_process_with_external_center */
auro_asc4he_v1_CGenXOverBlock_process_with_external_center(
    std::uint8_t* state,
    float** io_pair,
    float* external_center,
    float** height_pair,
    float** surround_pair,
    std::uint32_t flags,
    std::uint64_t work_bytes,
    float* work_768);

bool /* Decompiled name: is_median_symmetric */
is_median_symmetric(std::uint32_t layout);
constexpr std::size_t kAsc4heBaseProcessorBytes = 0x3718u;
constexpr std::size_t kAsc4heSbProcessorBytes = 20196u;
std::int64_t /* Decompiled name: auro_asc4he_v1_base_Processor_t_construct */
auro_asc4he_v1_base_Processor_t_construct(
    std::uint8_t* state,
    const std::uint8_t* construct_params_32,
    const std::uint8_t* config);
std::int64_t /* Decompiled name: auro_asc4he_v1_base_Processor_default_process */
auro_asc4he_v1_base_Processor_default_process(
    std::uint8_t* state,
    std::uint64_t channel_table,
    std::uint64_t scratch_bytes,
    std::uint64_t scratch);
std::int64_t /* Decompiled name: auro_asc4he_v1_base_Processor_default_reset_audio_state */
auro_asc4he_v1_base_Processor_default_reset_audio_state(std::uint8_t* state);
std::int64_t /* Decompiled name: auro_asc4he_v1_base_Processor_default_initialize */
auro_asc4he_v1_base_Processor_default_initialize(
    std::uint8_t* state,
    const std::uint8_t* params,
    std::uint32_t sample_rate);
std::int64_t /* Decompiled name: auro_asc4he_v1_sb_Processor_t_is_supported */
auro_asc4he_v1_sb_Processor_t_is_supported(std::uint32_t layout);
std::int64_t /* Decompiled name: auro_asc4he_v1_sb_Processor_t_get_required_input_layout */
auro_asc4he_v1_sb_Processor_t_get_required_input_layout(
    std::uint32_t layout,
    std::uint32_t* out_layout);
std::int64_t /* Decompiled name: auro_asc4he_v1_sb_Processor_t_construct */
auro_asc4he_v1_sb_Processor_t_construct(
    std::uint8_t* state,
    const std::uint8_t* config);
std::int64_t /* Decompiled name: auro_asc4he_v1_sb_Processor_surround_reset */
auro_asc4he_v1_sb_Processor_surround_reset(std::uint8_t* state);
std::int64_t /* Decompiled name: auro_asc4he_v1_sb_Processor_surround_initialize */
auro_asc4he_v1_sb_Processor_surround_initialize(
    std::uint8_t* state,
    const std::uint8_t* params,
    std::uint32_t sample_rate);
std::int64_t /* Decompiled name: auro_asc4he_v1_sb_Processor_surround_process_2_2 */
auro_asc4he_v1_sb_Processor_surround_process_2_2(
    std::uint8_t* state,
    std::uint64_t channel_table,
    std::uint64_t scratch_bytes,
    std::uint64_t scratch);
std::int64_t /* Decompiled name: auro_asc4he_v1_sb_Processor_surround_process_2_0 */
auro_asc4he_v1_sb_Processor_surround_process_2_0(
    std::uint8_t* state,
    std::uint64_t channel_table,
    std::uint64_t scratch_bytes,
    std::uint64_t scratch);
std::int64_t /* Decompiled name: auro_asc4he_v1_sb_Processor_height_reset */
auro_asc4he_v1_sb_Processor_height_reset(std::uint8_t* state);
std::int64_t /* Decompiled name: auro_asc4he_v1_sb_Processor_height_initialize */
auro_asc4he_v1_sb_Processor_height_initialize(
    std::uint8_t* state,
    const std::uint8_t* params,
    std::uint32_t sample_rate);
std::int64_t /* Decompiled name: auro_asc4he_v1_sb_Processor_height_process_2_2 */
auro_asc4he_v1_sb_Processor_height_process_2_2(
    std::uint8_t* state,
    std::uint64_t channel_table,
    std::uint64_t scratch_bytes,
    std::uint64_t scratch);
std::int64_t /* Decompiled name: auro_asc4he_v1_sb_Processor_height_process_2_0 */
auro_asc4he_v1_sb_Processor_height_process_2_0(
    std::uint8_t* state,
    std::uint64_t channel_table,
    std::uint64_t scratch_bytes,
    std::uint64_t scratch);
std::int64_t /* Decompiled name: auro_asc4he_v1_multichannel_Processor_t_is_supported */
auro_asc4he_v1_multichannel_Processor_t_is_supported(std::uint32_t layout);
std::int64_t /* Decompiled name: auro_asc4he_v1_multichannel_Processor_t_get_required_input_layout */
auro_asc4he_v1_multichannel_Processor_t_get_required_input_layout(
    std::uint32_t layout,
    std::uint32_t* out_layout);
std::int64_t /* Decompiled name: auro_asc4he_v1_multichannel_Processor_t_construct */
auro_asc4he_v1_multichannel_Processor_t_construct(
    std::uint8_t* state,
    const std::uint8_t* config);
std::int64_t /* Decompiled name: auro_asc4he_v1_multichannel_Processor_process */
auro_asc4he_v1_multichannel_Processor_process(
    std::uint8_t* state,
    std::uint64_t channel_table,
    std::uint64_t scratch_bytes,
    std::uint64_t scratch);
std::int64_t /* Decompiled name: auro_asc4he_v1_multichannel_Processor_reset */
auro_asc4he_v1_multichannel_Processor_reset(std::uint8_t* state);
std::int64_t /* Decompiled name: auro_asc4he_v1_multichannel_Processor_initialize */
auro_asc4he_v1_multichannel_Processor_initialize(
    std::uint8_t* state,
    const std::uint8_t* params,
    std::uint32_t sample_rate);
std::int64_t /* Decompiled name: auro_asc4he_v1_multichannel_HXs_process */
auro_asc4he_v1_multichannel_HXs_process(
    std::uint8_t* state,
    std::uint64_t* channel_table,
    std::uint64_t scratch_bytes,
    float* scratch);
std::int64_t /* Decompiled name: auro_asc4he_v1_multichannel_HXs_reset */
auro_asc4he_v1_multichannel_HXs_reset(std::uint8_t* state);
std::int64_t /* Decompiled name: auro_asc4he_v1_multichannel_HXs_initialize */
auro_asc4he_v1_multichannel_HXs_initialize(
    std::uint8_t* state,
    const std::uint8_t* params,
    std::uint32_t sample_rate);
std::int64_t /* Decompiled name: auro_asc4he_v1_ss_Processor_t_is_supported */
auro_asc4he_v1_ss_Processor_t_is_supported(std::uint32_t layout);
std::int64_t /* Decompiled name: auro_asc4he_v1_ss_Processor_t_get_required_input_layout */
auro_asc4he_v1_ss_Processor_t_get_required_input_layout(
    std::uint32_t layout,
    std::uint32_t* out_layout);
std::int64_t /* Decompiled name: auro_asc4he_v1_ss_Processor_t_construct */
auro_asc4he_v1_ss_Processor_t_construct(
    std::uint8_t* state,
    const std::uint8_t* config);
std::int64_t /* Decompiled name: auro_asc4he_v1_Processor_t_is_supported */
auro_asc4he_v1_Processor_t_is_supported(
    std::uint32_t processor_type,
    std::uint32_t layout);
std::int64_t /* Decompiled name: auro_asc4he_v1_Processor_t_get_required_input_layout */
auro_asc4he_v1_Processor_t_get_required_input_layout(
    std::uint32_t processor_type,
    std::uint32_t layout,
    std::uint32_t* out_layout);
std::int64_t /* Decompiled name: auro_asc4he_v1_TuningManager_t_get */
auro_asc4he_v1_TuningManager_t_get(
    std::uint8_t* out240,
    std::uint32_t processor_type,
    std::uint32_t layout);
std::int64_t /* Decompiled name: auro_asc4he_v1_Processor_t_check_static_parameters */
auro_asc4he_v1_Processor_t_check_static_parameters(const std::uint8_t* params);
std::uint8_t* /* Decompiled name: auro_asc4he_v1_Processor_t_construct */
auro_asc4he_v1_Processor_t_construct(
    std::uint8_t* state,
    const std::uint8_t* params);
std::int64_t /* Decompiled name: auro_asc4he_v1_Processor_t_get_latency */
auro_asc4he_v1_Processor_t_get_latency();
std::int64_t /* Decompiled name: auro_asc4he_v1_Processor_get_dynamic_parameters */
auro_asc4he_v1_Processor_get_dynamic_parameters(
    const std::uint8_t* state,
    std::uint8_t* out_params);
std::int64_t /* Decompiled name: auro_asc4he_v1_Processor_set_dynamic_parameters */
auro_asc4he_v1_Processor_set_dynamic_parameters(
    std::uint8_t* state,
    const std::uint8_t* params);
std::int64_t /* Decompiled name: auro_asc4he_v1_Processor_reset_audio_state */
auro_asc4he_v1_Processor_reset_audio_state(std::uint8_t* state);
std::int64_t /* Decompiled name: auro_asc4he_v1_Processor_process */
auro_asc4he_v1_Processor_process(
    std::uint8_t* state,
    std::uint32_t* block_desc);

OutputGeneratorExtrapolateSources output_generator_select_extrapolate_sources_1024a9(
    std::uint64_t output_channels_table_base,
    std::uint64_t frame_channel_ptr,
    std::uint64_t segment_start,
    std::uint64_t scratch_base,
    std::uint64_t total_samples);

} // namespace auro3deng
