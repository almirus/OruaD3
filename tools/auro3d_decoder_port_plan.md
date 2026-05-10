# Auro3D decoder port plan

## Scope

- Primary source: IDA MCP analysis of `libauro.so`.
- Runtime helpers: Frida/Kahlo only for track-dependent state and preset/runtime pointers.
- Android/JADX MCP: only for app-side settings/control flow.
- Do not spend time on `frida_trace_a3deng_live.js` while the main decoder is incomplete.
- No build/regression until the decoder is ready. Current verification is limited to lint-style checks.

## Still missing or only partially ported

## Latest progress

- A3DENG static model:
  - `get_output_info` local packing now mirrors IDA `0x31B4E0`: low 32 bits are constructor `pipeline_audio_block_size` (`this+52`), with API output block count in bits 32..63.
  - Corrected the previous `this+52` interpretation after re-checking `A3DENG::create_instance_` at `0x31A790` and `A3DENG::pop_` at `0x31B7F0`; `pop_` uses it as block size, while sample bits come from the output audio-block sample-type table.
  - Added a local static configuration state for the visible `create_instance_` overrides: pipeline block size, HDMI carrier allow flags, HDMI quality, output mode, and instance/API availability.
  - JNI `AuroInitialize` at `0x318C90` confirmed constructor args: `a3` -> `this+52` pipeline block size and `a4` -> `this+56` output mode.
  - `A3DENG::pop` at `0x31B7A0` is now represented as `pop_return_bytes`: rendered byte count on success, `-1` when `pop_` fails.
  - `get_maximum_output_bytecount` at `0x31B5A0` is now modeled: low32 is the maximum rendered bytes and high32 carries the native validity flag.
  - JNI `AuroGetMaximumOutputBytecount` at `0x3190D0` is represented by `jni_maximum_output_bytecount_return`: `-1` unless native packed high validity byte is set, otherwise low32 byte count.
  - JNI `AuroPush` at `0x318FE0` passes direct byte-buffer address plus caller byte count into `A3DENG::push`; JNI `AuroPop` at `0x319120` returns `-1` for null/non-positive output buffer or `A3DENG::pop` failure.
  - `A3DENG::~A3DENG` at `0x319D00`, `destroy_instance_` at `0x319DD0`, and JNI `AuroRelease` at `0x318D10` confirm vtable `+0x38` instance destruction and instance pointer clearing.
  - `input_block_size` at `0x31B280` matches the existing push block-size derivation helper.
  - `get_output_layout` at `0x31B330` and `get_output_channel_count` at `0x31B400` use configured output mask until `pop_` marks pruned output info valid at `this+728`; pruned count is at `this+0x150`, channel ids start at `this+0x160` with 16-byte entries. Local render state now exposes pruned output mask/count, currently mirrored from requested output until engine-pop storage is fully validated.
  - `Engine::pop_on_scratchpad_` at `0x31F2C0` confirms `PrunedOutputInfo` qword0 is max output sample end and qword+8 begins the copied 0x188-byte ChannelMap payload.
  - Runtime dynamic parameter derivation now follows `A3DENG::update` at `0x319E60` for actual virtualization mode and listening mode instead of the older `AuroDecoderImpl` formulas.
  - Runtime config now records the `A3DENG::update` target-device branch, `dynamic_request_flag`, `dynamic_headphone_flag`, and `is_abr` branch state.
  - JNI `AuroUpdate2` at `0x318D60` plus `JNI_OnLoad` at `0x317A30` mapped the dynamic Java fields: `virtualization_enabled` -> settings+40 and `listening_mode_auro3d` -> settings+41.
  - `Settings::virtualization_mode` at `0x319980`, `Settings::listening_mode` at `0x3199A0`, and `Settings::Config::target_device` at `0x3198B0` are now annotated in IDA and mirrored by local helpers.
  - `Settings::compare` at `0x319A00` is annotated in IDA: low compare byte is config change; high byte is dynamic change for virtualization/listening/HP presets.
  - `A3DENG::push` local state now tracks the IDA `0x31AF50` input block-count/channel-count byte contract.
  - `sub_31ACE0` channel mapping now stops after the fixed BacksBeforeSurrounds table when `hdmi_channel_mapping == 1`, matching IDA instead of appending the generic 0..30 mask order.
- Codec-v3 lifecycle/output:
  - Dispatch no longer hardcodes 64 samples in the active partial path; it carries the caller block size through dispatch validation.
  - Dispatch/process validation now carries a distinct required output mask instead of assuming input mask equals output mask, matching `Decoder_process` use of decoder state `+40`.
  - Output-generator segment planning now keeps a single zero-length segment and stops, matching the downstream zero-length handling without filling the guard window with duplicate segments.
  - Corrected `OutputGenerator_process` address tracking to function start `0x102240`; `0x1024A9` is the internal `GolombRice_initialize` call site.
  - Rechecked `OutputGenerator_process` against the saved IDA MCP decompile after live decompilation of `0x102240` failed in the current IDA session.
  - The raw segment loop now matches the IDA cursor/ready-frame order: process/copy, OR `*a3` with segment mask, advance `this+824` by segment length, then mark/pop the frame only when `frame_end == new_cursor`.
  - The legacy `output_generator_build_segment_plan_1024a9` helper now uses the same signed 32-bit start/end delta handling as IDA and the runtime helper, avoiding unsigned underflow when a frame has already started.
  - `OutputGenerator_process` produced-channel mask (`a3`) is now preserved through the local output-stage bridge into `CodecV3DispatchStateEb5a0::produced_output_mask` instead of being discarded after the call.
  - The local `decoder_run_step_101800_partial` now mirrors the IDA `0x101800` output-plane order: parser first, clear required output channels, then run `OutputGenerator_process`, then advance the delay-line. The old carrier preseed in `Decoder::decode` was removed because the native copy branch belongs inside OutputGenerator.
  - Rechecked current-version IDA MCP symbols for `ParseResult`: `ParseResultPool_get_new` still pre-increments the cursor, but the current stride is `3672`, `ParseResult_mark_usage` writes at `+3664`, and `ParseResult_t_construct` only clears header `0..191` plus tail `3648..3667`. Local pool sizing, indexing, ready-frame copy size, and construct behavior now follow those current-version facts instead of the older saved `0x10...` dump.
  - Rechecked current-version `channel_Parser_process` in IDA MCP. Local `ParseResult` payload offsets now match current code: context words at `+192`, stream words at `+1920`, context count at `+3648`, stream count at `+3656`. The parser opcode dispatch table was adjusted to the current opcode groups, including ADOL original-layout decoding for opcode `30`. Carrier-layout opcode `70` now uses the current `get_carrier_layout` mapping and the current `dword_289FC0` 16-entry coefficient table instead of the older guarded 8-entry table path.
  - Rechecked current-version `Parser_process`, `Frame_t_construct`, and `Config_initialize` in IDA MCP. The top parser loop now checks each frame slot's active flag independently, matching IDA and avoiding a false skip when slot 0 is inactive. `Frame_t_construct` now uses current invalid channel id `31` and scans 31 mask bits. `Config_initialize` now uses current error codes `401..404` and `Mask_count(mask & 0x7fffffff)` semantics.
  - Rechecked current-version `Decoder_process`, `DelayLine`, `FormatDetector`, `SyncDetector`, `OutputGenerator_t_construct`, `OutputGenerator_process`, and `Metadata_combine_info` in IDA MCP. Codec-v3 local IO descriptors, delay-line slots, format-detector block pointer arrays, output-channel table, fake frame buffers, parser-slot storage, and metadata channel-id handling now follow the current 31-channel contract (`0..30`, mask `0x7fffffff`), while the legacy A3DENG processor descriptor remains isolated until its own current layout is verified.
  - Corrected current `OutputGenerator` object offsets from IDA: total samples `+824`, timeline cursor `+840`, `DelayLine* +848`, frame deque `+856`, `BlockInfo* +864`, error buffer `+872`, scratch buffer `+880`. The local segment context now uses the native `BlockInfo` shape and pointer slot, and the error buffer allocation follows current `Memory_t_construct` (`2 * block_samples` int32 values).
  - Rechecked current `DelayLine_get_buffer` in IDA MCP. Local delay-line lookup now returns null for `cursor >= absolute_cursor` and uses the native wrapped slot formula `write_index - (block_distance + 1)`, avoiding reads from the current write slot.
  - The codec-v3 parser payload refresh limit now follows the current 31-channel descriptor instead of the older processor-layer 27-channel limit.
  - Rechecked current `Frame_t_construct`: channel masks are scanned over 31 bits, but the frame object only has 9 native slot descriptors copied by `FrameDeque_push_back` (`0x150` bytes). Local fake-frame storage and construction now keep that 9-slot frame capacity instead of exposing 31 frame slots to `OutputGenerator`.
  - Updated local fake-frame channel initialization to stop at the native 9 frame slots and copy only `0x150` bytes into the next fake frame, matching current `FrameDeque_push_back`.
  - Rechecked current `Config_initialize`; local code now stores config field `+56` from init args `+28` on success, matching IDA.

### 1. A3DENG wrapper

Partially ported: offsets, settings layout, vtable offsets, and part of channel mapping.

Still needed:

- `A3DENG::A3DENG` at `0x319AE0`
  - Full constructor behavior.
  - API/instance creation.
  - Initial object fields, storage, mutex/layout setup.

- `A3DENG::update` at `0x319E60`
  - Full config path.
  - Input/output descriptors.
  - Dynamic get/set branches.
  - Configure call through API vtable `+0x78`.

- `A3DENG::push` at `0x31AF50`
  - Real input push path.
  - Block-size handling.
  - Input descriptor/storage handling.
  - API call through vtable `+0xC0`.

- `A3DENG::pop` at `0x31B7A0`
- `A3DENG::pop_` at `0x31B7F0`
  - Real native output contract.
  - Output size must be `channel_count * sample_bits * block_frames / 8`.
  - No stereo assumptions.
  - Full engine-pop pruned output-info contents at `this+328` after native vtable `+0xC8`.

- `A3DENG::get_output_info` at `0x31B4E0`
  - Local state function matching libauro output info packing.
  - Output block count, sample rate, and sample type handling.

- `sub_31ACE0`
  - Exact channel table behavior for default mapping.
  - Exact channel table behavior for `hdmi_channel_mapping == 1`.
  - Full mask/order verification.

### 2. Codec v3 core

Partially ported through local `*_partial` functions.

Still needed:

- `Decoder_t_construct` at `0x101760`
- `Config_initialize` at `0x1033C0`
- `decoder_run_step` at `0x101800`
- `decoder_run_dispatch` at `0xEB5A0`
- `AuroDecoderImpl::Initialize` at `0xD71E0`
- `AuroDecoderImpl::Decode` at `0xD7830`

Main gaps:

- Full object layout.
- Native allocator/memory paths.
- Accumulator/distributor behavior.
- Parser/output lifecycle without local scaffolding.
- Exact decoder state transitions.

### 3. Parser, entropy, and channel decode

Partially ported. Needs full branch audit against IDA.

Still needed:

- `channel_parser_construct`
- `channel_parser_process`
- `channel_GolombRice_initialize`
- `channel_GolombRice_get_errors`
- `channel_Extrapolate_initialize`
- `channel_Extrapolate_process`
- CRC and error-state paths.

Main gaps:

- All opcode/mode branches.
- All failure states.
- Exact parser result/deque interaction.

### 4. Output generator

Partially ported through local bridges.

Still needed:

- `OutputGenerator_process` at `0x102240`
- `decoder_run_output_stage_1024a9_partial`
- `frame_deque_*`
- `delay_line_*`

Main gaps:

- Ready-frame handling.
- Unused-frame marking.
- Segment lifecycle.
- Pop semantics.
- Native delay-line behavior.

### 5. Native XinN / Auro-Matic

Partially ported. Current code no longer uses synthetic height reconstruction.

Still needed:

- Real preset blobs/pointers from `libauro.so` or runtime.
- Full `StepUpmixXinN` initialize/process/reset/update.
- `Engine1` construct/process_ext/get_delayed/partial_clear.
- `Engine2` construct/process/set_output_patch/delay/reverb.
- `XinN Early` mode1/mode2.
- `XinN Late`.
- `XinN Routing`.

Important:

- Frida is only a helper for runtime variables and preset pointers.
- Main implementation must come from IDA/static code.
- If native XinN is unavailable for height output, decoder should fail explicitly instead of using heuristics.

### 6. Multichannel output

Live Frida currently observes default stereo output only. That is not the decoder limit.

Still needed:

- Output mask/channel count exactly as `A3DENG::pop_`.
- Output layout through native masks and native slot map.
- Height/native slots support.
- `--dsp-output-channels` mapping through native slots.
- No fallback stereo/downmix assumptions.

### 7. JADX / Android integration

Still needed:

- Resolve `A3DENG.AuroUpdateParams` field names and map them to `NativeRuntimeConfigurationState`.
- Confirm defaults and route-change behavior in:
  - `AuroAudioProcessor`
  - `AuroEngineAudioSink`
  - `AuroEngineDecoder`

JADX is only for app-side control flow and settings; decoder logic remains based on `libauro.so`.

## Work order

1. Finish A3DENG static model:
   - Constructor.
   - Update.
   - Push.
   - Pop/pop_.
   - Get output info.
   - `sub_31ACE0`.

2. Finish codec-v3 decoder lifecycle:
   - `Initialize`.
   - `Decode`.
   - Memory/object layout.
   - Parser/output/dispatch.

3. Finish parser/entropy/extrapolate correctness:
   - All modes.
   - All branch paths.
   - All error paths.

4. Finish output generator:
   - Frame deque.
   - Delay line.
   - Segment lifecycle.
   - Ready-frame behavior.

5. Finish XinN/Auro-Matic:
   - Preset data.
   - Engine1/Engine2.
   - Early/Late/Routing.
   - Multichannel height output.

6. Wire CLI/runtime controls:
   - Native output masks.
   - Native channel slot map.
   - Multichannel output contract.

7. Only after decoder readiness:
   - Build.
   - Regression.

## Current verification policy

- Allowed now:
  - Symbol search.
  - Lint-style static checks.
  - Brace/structure checks.

- Not allowed until decoder is ready:
  - Full build.
  - Regression suite.
