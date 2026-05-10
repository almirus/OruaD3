#pragma once

/// Источники RE: весь DSP декодера — только libauro3d.so, разбор через ida-pro-mcp (RPC к IDA).
/// Использование в APK (JNI, ExoPlayer extension, нативные вызовы) — через JADX/app analysis.

#include "auro3deng_processor_io.hpp"

#include <cstdint>
#include <cstddef>
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

/// Текстовая сводка цепочки Processor→Analyser→Controller и якорь для RE.
/// Якорь: в .so найти `auro_codec_v3_decoder_CRC_t_init` → xrefs (кто вызывает) — обычно рядом
/// с конструктором/инициализацией AURO codec v3 и контекста CRC перед разбором потока.
const char* ida_callchain_ru();

// IDA 0x106F00 — глобальная инициализация CRC-таблицы (512 байт @ unk_41ACF0, флаг byte_41ACE0).
void decoder_crc_t_init_106f00();

// IDA 0x1042A1 — глобальная инициализация таблиц для Extrapolate (float expf LUT в dword_41A910..).
void decoder_channel_extrapolate_t_init_1042a1();

// Literal перенос auro_codec_v3_decoder_Config_initialize @ 0x1033C0.
std::int64_t auro_codec_v3_decoder_Config_initialize(std::int64_t config_base, std::int64_t init_args_base);

// IDA helper auro_channel_Mask_count: возвращает количество установленных битов в маске каналов.
std::uint32_t auro_channel_Mask_count(std::uint32_t mask, std::int64_t /*a2*/, std::uint32_t /*mod_arg*/);

// IDA sub_EB420 — codec-v3 Decoder constructor + callback wiring.
// a1 — Decoder* (верхний объект a3deng), остальные аргументы соответствуют auro_memory_block_Accumulator/Distributor.
std::int64_t sub_eb420(
    std::uint64_t* decoder_slot_ptr,
    std::int64_t a2,
    std::int64_t a3,
    void (__fastcall *a4)(std::uint64_t, std::int64_t),
    std::int64_t a5,
    std::int64_t a6);

// IDA sub_DB290 @ 0xDB290 — верхний orchestrator Decoder/Manager process path.
// Пока перенесён literal-пролог валидации размеров и базовая рамка итерации.
std::int64_t sub_db290_partial(
    std::uint32_t* decoder_owner_base,
    std::int64_t input_desc_base,
    std::uint32_t* output_desc_base,
    void* scratch_base);

// Внешние узлы, которые вызываются в sub_DB290.
std::int32_t auro_a3deng_v3_Decoder_latency(std::uint32_t* decoder_base);
std::int64_t auro_a3deng_v3_Decoder_process(
    std::int64_t decoder_base,
    std::int64_t input_channels_blob,
    std::int64_t scratch_blob,
    std::int64_t out_mask,
    std::int32_t* out_changed);
std::int64_t auro_a3deng_v3_pipeline_Manager_set_initial_latency(std::uint32_t* manager_base, float latency);
std::int64_t auro_a3deng_v3_pipeline_Manager_configure(std::uint32_t* manager_base, void* cfg_blob, std::int32_t initial);
std::int64_t auro_a3deng_v3_pipeline_Manager_process_audio(std::uint32_t* manager_base, void** io_channels);
using AuroMaticV3XinNFl32ProcessFn = std::int64_t (*)(std::uint64_t xinn_state, void** channel_span_31);
using AuroA3dengV4XinNResetFn = void (*)(std::uint64_t step_base);
using AuroMaticEngine2Fl32ProcessFn =
    void* (*)(std::uint64_t engine2_state, void* route_a, void* route_b, void* output_0x300);
std::int64_t auro_matic_Engine2_fl32_construct_57b000_partial(std::uint64_t engine2_state, std::uint64_t memory);
void auro_matic_Engine2_fl32_set_total_clear_frames_57aff0_partial(
    std::uint64_t engine2_state,
    std::uint32_t frames);
void auro_matic_Engine2_fl32_set_preset_57b040_partial(std::uint64_t engine2_state, std::uint64_t preset);
void auro_matic_Engine2_fl32_reset_audio_state_57b050_partial(std::uint64_t engine2_state);
bool auro_matic_Engine2_fl32_partial_clear_57b080_partial(std::uint64_t engine2_state, std::uint32_t* remaining);
std::uint64_t auro_matic_Engine2_fl32_set_output_patch_57c2f0_partial(
    std::uint64_t engine2_state,
    const std::uint32_t* patch_3);
void* auro_matic_Engine2_fl32_process_57b0e0_partial(
    std::uint64_t engine2_state,
    void* route_a_0x80,
    void* route_b_0x80,
    void* output_0x300);
std::int64_t auro_matic_XinN_fl32_process_574fe0_partial(std::uint64_t xinn_state, void** channel_span_31);
std::int64_t auro_matic_v3_XinN_fl32_process_556440_partial(std::uint64_t xinn_v3_state, void** channel_span_31);
void auro_audio_Smooth_fl32_inst_initialize_5aab80_partial(float* smooth_state, std::uint32_t sample_rate, float seconds);
void auro_audio_Smooth_fl32_inst_update_5aabd0_partial(float* smooth_state, float target);
void auro_audio_Smooth_fl32_inst_set_current_5aabe0_partial(float* smooth_state, float current);
void auro_audio_Smooth_fl32_inst_gains_smooth_5aae00_partial(
    float* smooth_state,
    float* gains,
    std::uint32_t count);
void auro_matic_Engine1_fl32_set_total_clear_frames_511698_partial(std::uint8_t* engine1_state, std::uint32_t frames);
std::int64_t auro_matic_Engine1_fl32_construct_583d10_partial(std::uint8_t* engine1_state, std::uint64_t memory);
void auro_matic_Engine1_fl32_set_preset_584ca0_partial(std::uint8_t* engine1_state, std::uint64_t preset);
void auro_matic_Engine1_fl32_set_output_patch_584ac0_partial(std::uint8_t* engine1_state, const std::uint32_t* patch_3);
void auro_matic_Engine1_fl32_reset_audio_state_511760_partial(std::uint8_t* engine1_state);
void auro_matic_Engine1_fl32_get_delayed_frame_511784_partial(
    const std::uint8_t* engine1_state,
    std::uint64_t* out_near,
    std::uint64_t* out_far);
bool auro_matic_Engine1_fl32_partial_clear_5116ec_partial(std::uint8_t* engine1_state, std::uint32_t* remaining);
void auro_matic_Engine1_fl32_process_ext_5117d8_partial(
    std::uint8_t* engine1_state,
    const float* input_a_32,
    const float* input_b_32,
    float* output_0x300,
    const float* gains_32);
void auro_matic_XinN_Early_fl32_process_mode1_5773e0_partial(
    std::uint8_t* early_state,
    void** channel_span_31,
    float* output_0x300,
    const float* gains_32);
void auro_matic_XinN_Early_fl32_process_mode2_5776e0_partial(
    std::uint8_t* early_state,
    void** channel_span_31,
    float* output_a_0x300,
    float* output_b_0x300,
    const float* gains_32);
std::int64_t auro_matic_XinN_Early_fl32_construct_577320_partial(
    std::uint8_t* early_state,
    std::uint64_t memory_a,
    std::uint64_t memory_b);
void auro_matic_XinN_Early_fl32_set_total_clear_frames_5772e0_partial(
    std::uint8_t* early_state,
    std::uint32_t frames);
void auro_matic_XinN_Early_fl32_set_downmix_577e90_partial(
    std::uint8_t* early_state,
    const std::uint8_t* downmix_plan);
void auro_matic_XinN_Early_fl32_set_output_patch_577ec0_partial(
    std::uint8_t* early_state,
    const std::uint32_t* primary_patch_3,
    const std::uint32_t* secondary_patch_3);
void auro_matic_XinN_Early_fl32_set_preset_577f90_partial(std::uint8_t* early_state, std::uint64_t preset);
void auro_matic_XinN_Early_fl32_reset_audio_state_577f00_partial(std::uint8_t* early_state);
bool auro_matic_XinN_Early_fl32_partial_clear_577f40_partial(
    std::uint8_t* early_state,
    std::uint32_t* remaining);
void auro_matic_XinN_Early_fl32_get_delayed_mode1_577400_partial(
    std::uint8_t* engine1_state,
    float* out_near_32,
    float* out_far_32);
void auro_matic_XinN_Early_fl32_mix_delayed_mode2_577bc0_partial(
    std::uint8_t* early_state,
    float* out_near_32,
    float* out_far_32);
void auro_matic_XinN_Late_fl32_construct_579920_partial(std::uint32_t* late_state, std::uint64_t memory);
void auro_matic_XinN_Late_fl32_reset_audio_state_579940_partial(std::uint32_t* late_state);
void auro_matic_XinN_Late_fl32_set_clear_frames_579960_partial(
    std::uint32_t* late_state,
    std::uint32_t early_clear_frames,
    std::uint32_t late_clear_frames);
bool auro_matic_XinN_Late_fl32_partial_clear_579980_partial(std::uint32_t* late_state, std::uint32_t* remaining);
void auro_matic_XinN_Late_fl32_set_preset_579a30_partial(std::uint32_t* late_state, std::uint64_t preset);
void auro_matic_XinN_Late_fl32_set_output_patch_579a50_partial(std::uint32_t* late_state, const std::uint32_t* patch_3);
void* auro_matic_XinN_Late_fl32_process_5799a0_partial(
    std::uint32_t* late_state,
    std::uint64_t engine_state,
    void* output_0x300,
    AuroMaticEngine2Fl32ProcessFn engine2_process);
std::int64_t auro_matic_v3_XinN_Routing_fl32_apply_5575c0_partial(
    std::uint64_t routing_state,
    const float* early_a_0x300,
    const float* early_b_0x300,
    const float* late_0x300,
    void** channel_span_31,
    std::uint32_t mode);
void auro_matic_XinN_parameter_Dynamic_t_default_503fbc_partial(std::uint8_t* dynamic_48);
void auro_matic_v3_XinN_parameter_Dynamic_t_default_4e9fec_partial(std::uint8_t* dynamic_120);
void auro_matic_v3_XinN_Routing_fl32_construct_557420_partial(std::uint8_t* routing_state);
void auro_matic_v3_XinN_Routing_fl32_set_routing_557620_partial(
    std::uint8_t* routing_state,
    const std::uint8_t* routing_72);
std::int64_t auro_matic_v3_XinN_Routing_fl32_configure_557460_partial(
    std::uint8_t* routing_state,
    std::uint32_t mode,
    std::uint32_t output_mask,
    std::uint32_t input_mask);
std::int64_t auro_matic_XinN_fl32_construct_574950_partial(
    std::uint8_t* xinn_state,
    std::uint8_t* routing_state,
    const std::uint64_t* memory_args_5);
void auro_matic_XinN_fl32_set_dynamic_parameters_574a50_partial(
    std::uint8_t* xinn_state,
    const std::uint8_t* dynamic_48);
std::int64_t auro_matic_XinN_fl32_initialize_574be0_partial(
    std::uint8_t* xinn_state,
    std::uint32_t input_mask,
    std::uint32_t output_mask,
    std::uint64_t preset);
std::int64_t auro_matic_v3_XinN_fl32_construct_556180_partial(
    std::uint8_t* xinn_v3_state,
    const std::uint64_t* memory_args_5);
std::int64_t auro_matic_v3_XinN_fl32_configure_556330_partial(
    std::uint8_t* xinn_v3_state,
    std::uint32_t sample_rate,
    const std::uint64_t* config_args_4);
void auro_matic_XinN_fl32_reset_audio_state_574f90_partial(std::uint8_t* xinn_state);
void auro_matic_XinN_fl32_set_preset_574af0_partial(std::uint8_t* xinn_state, std::uint64_t preset);
void auro_matic_v3_XinN_fl32_reset_audio_state_556240_partial(std::uint8_t* xinn_v3_state);
void auro_matic_v3_XinN_fl32_set_preset_556250_partial(std::uint8_t* xinn_v3_state, std::uint64_t preset);
void auro_matic_v3_XinN_fl32_set_dynamic_parameters_556200_partial(
    std::uint8_t* xinn_v3_state,
    const std::uint8_t* dynamic_120);
std::int64_t auro_matic_XinN_fl32_process_inplace_574e30_partial(std::uint64_t xinn_state, void** channel_span_31);
std::int64_t auro_matic_XinN_fl32_process_scratch_574ee0_partial(std::uint64_t xinn_state, void** channel_span_31);
std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_initialize_35b2c0_partial(
    std::uint64_t step_base,
    const std::uint64_t* memory_3);
std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_initialize_355cf8_partial(
    std::uint64_t step_base,
    const std::uint64_t* memory_3);
std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_prepare_35b330_partial(
    std::uint64_t step_base,
    const std::uint8_t* plan_blob,
    const std::uint8_t* update_blob);
void auro_a3deng_v4_pipeline_step_upmix_XinN_reset_audio_state_35b730_partial(std::uint64_t step_base);
void auro_a3deng_v4_pipeline_step_upmix_XinN_reset_audio_state_35609c_partial(std::uint64_t step_base);
std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_update_35b740_partial(
    std::uint64_t step_base,
    const std::uint8_t* update_blob);
std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_update_3560a4_partial(
    std::uint64_t step_base,
    const std::uint8_t* update_blob);
std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_process_35b440_partial(
    std::uint64_t step_base,
    void** io_channels_31,
    std::uint32_t subblock_count,
    const std::uint8_t* block_records_516,
    AuroMaticV3XinNFl32ProcessFn process_fl32,
    AuroA3dengV4XinNResetFn reset_audio_state);
std::int64_t auro_a3deng_v4_pipeline_step_upmix_XinN_process_355eac_partial(
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

// IDA 0x101760 — auro_codec_v3_Decoder_t_construct:
// CRC_t_init → channel_Extrapolate_t_init → Config_initialize → Memory/FormatDetector/Parser/OutputGenerator construct.
// a1 — база codec v3 decoder, a2 — ptr на args для Config_initialize (см. auro_codec_v3_ida::kDecoderConfigInitArg_*),
// a3 — пользовательский контекст (Decoder*, передаётся дальше в Memory_t_construct/OutputGenerator_t_construct).
std::uint32_t decoder_t_construct_101760(std::uint8_t* decoder_base, const void* config_init_args, void* user_ctx);

/// IDA sub_ED070 @ 0xED070
std::int64_t sub_ed070(std::uint8_t* processor_base);

/// IDA sub_DA320 @ 0xDA320 — в бинарнике tail jmp; здесь явные три аргумента (на SysV a2/a3 доходят из Processor_process).
std::int64_t sub_da320(std::uint8_t* processor_base, std::uint8_t* buffer_desc_a2, std::uint8_t* buffer_desc_a3);

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

// Частичный перенос codec-v3 dispatch слоя вокруг sub_EB5A0 / sub_EB840 / sub_EB870 / sub_EB8A0.
// Реальный auro_codec_v3_Decoder_process ещё не перенесён целиком, поэтому вызов делегируется callback'у.
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

struct FormatDetectorState1056c0;
struct SyncDetectorState105ee0;
struct DelayLineState106b40;

using CodecV3ProcessFnEb5a0 = std::int64_t (*)(
    void* user,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t format_word0,
    const CodecV3IoBufferDescEb5a0* output_desc,
    std::uint32_t* io_status);

std::int32_t codec_v3_decoder_process_validate_101800_partial(
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
std::int64_t codec_v3_dispatch_eb5a0_partial(
    CodecV3DispatchStateEb5a0* state,
    const CodecV3IoBufferDescEb5a0* input_desc,
    const CodecV3IoBufferDescEb5a0* output_desc,
    std::uint32_t* io_status,
    CodecV3ProcessFnEb5a0 process,
    void* process_user);

struct CodecV3PartialRuntimeEb5a0 {
    CodecV3DispatchStateEb5a0* dispatch = nullptr;
    FormatDetectorState1056c0* format_detector = nullptr;
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
std::uint64_t delay_line_get_buffer_u64_state_partial(
    std::uint64_t delay_line_ptr,
    std::uint64_t cursor,
    std::uint64_t* io_state_u64);
std::uint64_t delay_line_get_channel_from_buffer_106ab0(
    std::uint64_t delay_line_buffer,
    std::uint32_t channel,
    std::uint64_t start);
std::uint64_t delay_line_advance_106b20(DelayLineState106b40* state);
std::uint64_t delay_line_write_buffer_106ab0(
    DelayLineState106b40* state,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask);

struct FormatDetectorState1056c0 {
    std::uint32_t layout = 0;                 // +304
    std::uint32_t sync_state = 0;             // +308
    std::uint32_t blocks_per_call = 0;        // +336
    std::uint32_t allow_low_9bits = 0;        // +340
    std::uint64_t processed_samples = 0;      // +320
    CodecV3StateChangeSink sink{};            // +288/+296 analogue
};

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

void sync_detector_set_layout_105ee0_partial(SyncDetectorState105ee0* state, std::uint32_t layout_mask);
void sync_detector_process_block_106110_partial(
    SyncDetectorState105ee0* state,
    const std::uint64_t* channel_ptrs_27);

void format_detector_process_1056c0_partial(
    FormatDetectorState1056c0* state,
    const CodecV3IoBufferDescEb5a0* input_desc,
    std::uint32_t input_mask,
    void (*set_layout)(void* user, std::uint32_t layout_mask),
    void (*process_block)(void* user, const std::uint64_t* channel_ptrs_27),
    void* sync_user);

// IDA 0x106CD0 / 0x106D07 helpers:
// Frame_mark_as_unused iterates frame slots and calls channel ParseResult_mark_usage(..., 0).
void downmix_coefficients_initialize_105d30_partial(std::uint64_t coeff_ptr);
void channel_parse_result_construct_103810_partial(std::uint64_t parse_result_ptr);
void channel_parse_result_mark_usage_106d07_partial(std::uint64_t parse_result_ptr, std::uint32_t in_use);
void channel_parse_result_prepare_new_107220_partial(std::uint64_t parse_result_ptr);
std::uint64_t parse_result_pool_required_additional_memory_107190_partial(std::uint32_t pool_count);
std::uint32_t memory_parse_result_pool_count_106d20_partial(
    std::uint32_t block_samples,
    std::uint32_t channel_count);
/// IDA 0x13D750: три Accumulator_add_block — суммарный запрошенный объём полезной нагрузки (24+4+8)*n байт.
std::uint64_t memory_block_info_required_additional_memory_13d750_partial(std::uint32_t n);
/// IDA 0x106D20: аргумент для BlockInfo_t_required_additional_memory — (((*a2 + 255) >> 7) | 1).
std::uint32_t memory_block_info_slot_count_106d20_partial(std::uint32_t block_samples);
/// IDA 0x106D20 хвост: add_block(4, 8*(q&0x7FFFFFFF)) и add_block(4, (12*q)&0x3FFFFFFFCLL).
std::uint64_t memory_accumulator_tail_payload_106d20_partial(std::uint64_t qword_at_a2);
/// IDA 0x13D570: Accumulator_add_block(..., 8, 336 * n) — полезная нагрузка байт.
std::uint64_t memory_frame_deque_required_additional_memory_13d570_partial(std::uint32_t deque_capacity);
/// Current IDA DelayLine required memory: add_block(4, 4*a*b*c) + add_block(8, 256*b); a=*a2, b=*(a2+16), c=*(a2+44).
std::uint64_t memory_delay_line_payload_sum_1068f0_partial(
    std::uint64_t a2_qword0,
    std::uint64_t a2_qword16,
    std::uint32_t a2_dword44);

/// BitReader @ channel_Parser_process: база `parser_u32 + 6` (см. 0x103840). Таблица масок: dword_2732C0 @ 0x2732C0.
void channel_bit_reader_set_data_105440_partial(std::uint64_t br, std::uint64_t words_ptr, std::int32_t word_count);
void channel_bit_reader_set_bitlines_1054b0_partial(std::uint64_t br, std::int32_t bitlines);
std::uint32_t channel_bit_reader_get_unsigned_bits_105330_partial(std::uint64_t br, std::int32_t bit_count);
std::uint32_t channel_bit_reader_get_remaining_nr_bits_105450_partial(std::uint64_t br);
std::int64_t channel_bit_reader_get_signed_bits_105460_partial(std::uint64_t br, std::int32_t bit_count);
void channel_bit_reader_reset_105490_partial(std::uint64_t br);

/// IDA 0x1070D0
void channel_crc_reset_1070d0_partial(std::uint64_t crc_state);
/// IDA 0x106F00 — глобальная таблица word_41ACF0[256] (512 байт), как в .so (lazy, один раз).
void channel_crc_table_init_106f00_partial();
/// IDA 0x1070E0 — обновление CRC по массиву 32-битных слов (счётчик слов в *(uint32*)(crc+4)).
std::uint64_t channel_crc_process_1070e0_partial(
    std::uint64_t crc_state,
    std::uint64_t words_ptr,
    std::int32_t word_count);
/// IDA 0x107170 — сравнение с полями канала (как в хвосте 0x103840).
std::int64_t channel_crc_check_107170_partial(
    std::uint64_t crc_state,
    std::int32_t dword_at_plus4_compare,
    std::int16_t expected_crc_word);
/// IDA 0x105D50 — собирает метаданные канала после чтения слов @+76/+80.
std::int64_t channel_metadata_combine_info_105d50_partial(std::uint64_t frame_channel_ptr);
/// IDA 0x104210 — полный Parser_t_construct: reset parser + инициализация полей канала (+52/+56/+72).
std::int64_t channel_parser_construct_104210_partial(
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

bool parse_result_pool_construct_1071b0_partial(
    std::uint64_t pool_base_ptr,
    std::uint64_t storage_base_ptr,
    std::uint32_t pool_count);
std::uint64_t parse_result_pool_get_new_107220_partial(std::uint64_t pool_base_ptr);
void frame_construct_parse_results_106cd0_partial(
    std::uint64_t frame_ptr,
    void (*construct_parse_result)(std::uint64_t parse_result_ptr));
void frame_construct_106ba0_partial(
    std::uint64_t frame_ptr,
    std::uint64_t start,
    std::uint32_t span,
    std::uint32_t channel_mask,
    std::uint32_t active_mask);
void frame_assign_window_partial(
    std::uint64_t* io_next_frame_start,
    std::uint64_t frame_span,
    std::uint64_t frame_ptr);
void frame_mark_usage_106cd0_partial(
    std::uint64_t frame_ptr,
    std::uint32_t in_use,
    void (*mark_usage)(std::uint64_t parse_result_ptr, std::uint32_t in_use));
void frame_mark_as_unused_106cd0_partial(
    std::uint64_t frame_ptr,
    void (*mark_usage)(std::uint64_t parse_result_ptr, std::uint32_t in_use));
void frame_mark_as_unused_106cd0_default_partial(std::uint64_t frame_ptr);
std::int64_t parser_frame_mark_as_unused_cb_partial(std::uint64_t frame_ptr);

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

std::uint64_t frame_deque_find_first_with_end_after_13d570_partial(
    std::uint64_t frame_deque_ptr,
    std::uint64_t sample_pos);
std::int64_t frame_deque_push_back_13d5d0_partial(
    std::uint64_t frame_deque_ptr,
    std::uint64_t frame_ptr,
    std::uint64_t copy_bytes);
std::uint64_t frame_deque_pop_front_13d670_partial(
    std::uint64_t frame_deque_ptr,
    void (*frame_mark_as_unused)(std::uint64_t frame_ptr));
std::uint64_t frame_deque_pop_front_keep_frame_partial(std::uint64_t frame_deque_ptr);

struct ParserRebindContext103610 {
    std::uint64_t parse_result_pool_base = 0;
    std::uint32_t* channel_words_base = nullptr; // 8 u32 per channel index
    std::size_t channel_words_count = 0;         // number of channel-index slots
    std::uint8_t* channel_ctx_base = nullptr;    // 64 bytes per channel index
    std::size_t channel_ctx_count = 0;           // number of channel-index slots
    std::uint8_t* channel_parser_base = nullptr; // 64 bytes per parser slot
    std::size_t channel_parser_count = 0;        // number of parser slots
};

void parser_rebind_frame_parse_results_103610_partial(
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

void parser_refresh_payload_partial(
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

std::int64_t parser_ready_frame_push_copy_partial(
    std::uint64_t frame_deque_ptr,
    std::uint64_t frame_ptr,
    std::uint64_t copy_bytes,
    const ParserReadyFrameCopyContext* copy_ctx);
void parser_state_sink_notify_partial(ParserStateSinkContext* sink, std::uint32_t state);

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

std::int64_t decoder_run_parser_1034e0_partial(DecoderParserRunContext1034e0* ctx);

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

std::int64_t decoder_run_parser_stage_1034e0_partial(DecoderParserStageContext1034e0* ctx);

struct DecoderOutputStageContext1024a9 {
    DelayLineState106b40* delay_line = nullptr;
    std::uint8_t* output_generator_base = nullptr;
    std::uint8_t* output_table_base = nullptr;
    std::size_t output_table_size = 0;
    const std::uint64_t* output_channel_ptrs_27 = nullptr;
    std::uint8_t* ready_frame_deque_base = nullptr;
    std::uint64_t total_samples = 0;
    std::uint32_t* produced_output_mask = nullptr;
};

std::int64_t decoder_run_output_stage_1024a9_partial(
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

void decoder_init_fake_frame_channels_106ba0_partial(
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

void decoder_init_frame_deques_13d5d0_partial(DecoderInitFrameDequesContext13d5d0* ctx);

struct DecoderDispatchRunContextEb5a0 {
    CodecV3DispatchStateEb5a0* dispatch = nullptr;
    FormatDetectorState1056c0* format_detector = nullptr;
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

std::int64_t decoder_run_dispatch_eb5a0_partial(DecoderDispatchRunContextEb5a0* ctx);

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

std::int64_t decoder_run_step_101800_partial(DecoderStepRunContext101800* ctx);

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
};

struct OutputGeneratorExtrapolateSources;

// Callback-интерфейс для следующей фазы 0x1024A9 (decode/copy по сегментам).
struct OutputGeneratorApplyCallbacks {
    void* user = nullptr;
    std::uint64_t (*get_delay_line_channel)(void* user, std::uint32_t channel, std::uint64_t start) = nullptr;
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
// Вызывается только если seg.frame_ptr != 0 && seg.frame_has_started.
std::int64_t output_generator_prepare_frame_channels_1024a9(
    const OutputGeneratorSegment& seg,
    const OutputGeneratorFrameInitCallbacks& cb);

// Объединённый частичный pipeline сегмента в порядке IDA:
// optional frame-init -> decode/copy -> накопление channel mask.
std::int64_t output_generator_process_segments_1024a9(
    const OutputGeneratorSegmentPlan& plan,
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
    std::uint32_t* io_channel_mask_out);

// Entry-point по IDA 0x102240: build-plan -> pre-segments callback -> segment loop -> update cursor/pop_front.
// external_mask_inout соответствует аргументу a3 в оригинале (*a3 |= seg_mask).
std::int64_t output_generator_process_1024a9(
    std::uint8_t* output_generator_base,
    std::uint64_t output_channels_table_base,
    const OutputGeneratorRuntimeFns1024a9& fns,
    std::uint32_t* external_mask_inout);

// Совместимый wrapper для существующих вызовов.
std::int64_t output_generator_process_1024a9_partial(
    std::uint8_t* output_generator_base,
    std::uint64_t output_channels_table_base,
    const OutputGeneratorRuntimeFns1024a9& fns,
    std::uint32_t* external_mask_inout);

struct DecoderOutputGeneratorRunContext1024a9 {
    std::uint8_t* output_generator_base = nullptr;
    std::uint8_t* output_channels_table = nullptr;
    std::size_t output_channels_table_size = 0;
    const std::uint64_t* output_channel_ptrs_27 = nullptr;
    std::uint64_t delay_line_ptr = 0;
    std::uint64_t frame_deque_ptr = 0;
    std::uint64_t total_samples = 0;
    std::uint32_t* produced_output_mask = nullptr;
    OutputGeneratorRuntimeFns1024a9 runtime_fns{};
};

std::int64_t decoder_run_output_generator_1024a9_partial(
    const DecoderOutputGeneratorRunContext1024a9* ctx);

void output_generator_construct_52af20_partial(
    std::uint8_t* output_generator_base,
    const std::uint8_t* output_config,
    std::uint64_t memory_base);

std::uint8_t* auro_a3deng_v4_android_A3DENG_construct_319ae0_partial(
    std::uint8_t* a3deng_base,
    std::uint32_t pipeline_audio_block_size,
    std::uint32_t output_mode);
bool auro_a3deng_v4_android_A3DENG_update_319e60_partial(
    std::uint8_t* a3deng_base,
    const std::uint8_t* settings_0x34);
std::uint64_t auro_a3deng_v4_android_A3DENG_get_output_info_31b4e0_partial(
    std::uint8_t* a3deng_base);
std::uint64_t auro_a3deng_v4_android_A3DENG_get_maximum_output_bytecount_31b5a0_partial(
    std::uint8_t* a3deng_base);
std::int64_t auro_a3deng_v4_android_A3DENG_push_31af50_partial(
    std::uint8_t* a3deng_base,
    const std::uint8_t* input_bytes,
    std::uint32_t input_byte_count);
std::int64_t auro_a3deng_v4_android_A3DENG_pop_31b7a0_partial(
    std::uint8_t* a3deng_base,
    std::uint8_t* output_bytes,
    std::int32_t output_byte_count);
std::uint32_t auro_a3deng_v4_android_A3DENG_get_output_layout_31b330_partial(
    std::uint8_t* a3deng_base);
std::uint32_t auro_a3deng_v4_android_A3DENG_get_output_channel_count_31b400_partial(
    std::uint8_t* a3deng_base);
void auro_a3deng_v4_android_channel_layout_31ace0_partial(
    std::uint8_t* layout_0x188,
    std::uint32_t channel_mask,
    std::uint32_t hdmi_channel_mapping);

// Точный выбор источников для Extrapolate_process из 0x1024A9:
// frame_channel[27..29] -> output table channel ptr + 4*segment_start, иначе fallback в scratch.
OutputGeneratorExtrapolateSources output_generator_select_extrapolate_sources_1024a9(
    std::uint64_t output_channels_table_base,
    std::uint64_t frame_channel_ptr,
    std::uint64_t segment_start,
    std::uint64_t scratch_base,
    std::uint64_t total_samples);

} // namespace auro3deng
