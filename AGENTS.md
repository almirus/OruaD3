# Orua D3 / AuroCX reverse-engineering guide

## Goal

Implement the AuroCX subtype decoder in `tools/auro3d-decode` from the
decompiled `decopiled/libauro.so.c`. Work incrementally: first make the bitstream
syntax observable and testable, then port reconstruction stages. Do not guess
PCM algorithms when the corresponding decompiled path has not been identified.

The schema and payload parsers now complete all seven corpus files without a
bitstream scan, cached-payload replay, or residual under-read. The current
target is PCM validation against the native decoder, exact SASC routing, and
bounded-memory output.

Current priority order:

1. validate PCM in the HDMI-7.1 Float32 oracle domain
   (`regression/compare_oracle_f32_hdmi8.py`); extend capture past
   multichannel onset (P80X ANR ~9s caps continuous Frida at ≈AU32 —
   use emulator or non-ANR host); full 12ch discrete equality still open;
2. port ESPCAP `ObjectRenderer::compute_panning_gains` (`0x4BF010`) for
   linked-object SASC steps (Planner order after level 2 is identified;
   corpus MP4s have `object_groups=0`);
3. add channel-separation invariants to the automated AuroCX regression;
4. exercise `set_stereo_top_` / `set_mono_top_` with a corpus or synthetic AU
   that sets present top blocks (schema+apply already ported).
   `auro_downmix_v1_Engine_calculate` non-height outer gate returns false in
   native (`0x58EF80`); `tgt_class_hint` is `(tgt_lo!=3) & ((uint8_t)tgt>>2)`.
   SASC step gains: `calculate_gains_` `+3576`/`+3577` post-scale uses rounded
   Q23 (`bias` then `>>23`); `compensate_channel_gains` uses truncating `/`.
   Lossless ICC mix (`icc_decode_subblock` `0x449640`): long blocks (`count>=12`)
   use rounded Q23 SIMD; short/overlap path truncates `/ 0x800000`.

## Repository state and ownership

- The worktree is intentionally dirty and contains unrelated user changes.
- Preserve all existing changes. Do not reset, checkout, delete, mass-format,
  stage, or commit unless explicitly requested.
- New AuroCX work currently lives in:
  - `tools/auro3d-decode/src/app/cx_probe.hpp`
  - `tools/auro3d-decode/src/app/cx_probe.cpp`
  - `tools/auro3d-decode/src/app/cx_bits.hpp`
  - `tools/auro3d-decode/src/app/cx_decode.hpp`
  - `tools/auro3d-decode/src/app/cx_decode.cpp`
  - `tools/auro3d-decode/src/app/awc_lossless.hpp`
  - `tools/auro3d-decode/src/app/awc_lossless.cpp`
  - `tools/auro3d-decode/src/app/awc_lpc_tables.hpp`
  - `tools/auro3d-decode/src/app/awc_transparent.hpp`
  - `tools/auro3d-decode/src/app/lfe_decode.hpp`
  - `tools/auro3d-decode/src/app/lfe_decode.cpp`
  - `tools/auro3d-decode/src/app/lfe_coefficients.hpp`
  - `tools/auro3d-decode/src/app/sasc_plan.hpp`
  - `tools/auro3d-decode/src/app/sasc_plan.cpp`
  - `tools/auro3d-decode/src/app/sasc_apply.hpp`
  - `tools/auro3d-decode/src/app/sasc_apply.cpp`
  - `tools/auro3d-decode/src/app/main.cpp`
  - `tools/auro3d-decode/build.bat`
  - `doc/auro_decoder.md`
- `cx_probe.cpp` is compact and partly one-line code. Prefer small, reviewable
  patches. A broader cleanup should be a separate change after behavior is
  covered by regression checks.

## What already works

`auro3d-decode --probe -i <file.mp4>` currently passes the initial and AU2
schema invariants on all seven corpus files. It:

- detects the MP4 `a3ds` audio sample entry;
- reads the `acxd` decoder configuration;
- reconstructs access-unit offsets correctly using `stsc`, `stsz`, and
  `stco`/`co64`;
- validates the `A3 DC 0D ED` access-unit marker;
- decodes XOR/VLQ blob framing (observed segment id `1`);
- decodes the initial schema header counts;
- reports program-to-bed references;
- decodes explicit channel descriptors and channel-to-audio-stream mapping;
- traverses channel and ambisonics bed variants, including optional names;
- traverses object groups and switch groups and records object stream mappings;
- handles compact beds whose channel list comes from the `acxd` layout;
- decodes initial PDU vectors and AWC/LFE stream ranges;
- distinguishes lossless and transparent/near-lossless AWC headers;
- decodes the small initial AWC payload configuration;
- preserves `CxSchemaDecodeState` and decodes the AU2 delta PDU templates;
- reports complete, plausible AU2 PDU vectors for all seven files.
- derives SASC SCG channel counts from selected beds, object groups, and linked
  object groups.

`auro3d-decode -i <file.mp4> -o <file.wav>` auto-selects AuroCX when the MP4
has an `a3ds` sample entry (same path formerly gated by `--decode-cx`): end-to-end
MP4 loop, lossless and transparent AWC, ICC
reconstruction (lossless ICP mix matches `0x449640`: rounded Q23 when
`count>=12`, truncating `/` for short/overlap; bitdepth downshift is
toward-zero), LFE and SASC modules, schema-bed mapping, and PCM24 WAV output.
A fresh 2026-07-16 build completes all eight `test_files/aurocx/*.mp4` files
(`8/8`) through auto CX decode (schema, AWC/LFE, SASC plan/apply, PCM24 WAV).
Yamamoto near-lossless Top routing follows native `calculate_gains_` /
height-tail order before `label101` (`0x436D80` / engine `label209`).
Alessandro custom channel downmix uses `Downmixer::append_` /
`set_layout_independent_gains_` (`0x4365F0` / `0x434EA0`) plus 2d→1d
source-gain overrides. Height beds with schema `gain0` use
`add_3d_to_2d_src_gains_` (`0x435520`) / `set_3d_to_2d_src_gains_` (`0x435230`)
and the `+920` destination post-scale when `calculate_` sets `+3576`
(src_dim≥3 && tgt_dim<3), confirmed on `auro_cx.mp4` (12ch, no `acxd`).
Height `gain1` fills the 2d→1d `+2408` table / FL–FR `+1416` post-scale
(append_ `a2[45]`); no-height still uses `gain0` for those slots.
`set_mono_top_` (`0x435580`) applies via `details::inv_sqrt` table
`qword_1DB190` (`0x433C20`). `set_stereo_top_` (`0x435C50`) is ported for
schema channels **28/29** (`StereoTopDownmix` types #3/#4 at ChannelDownmix
`+80`/`+104`); channel 12 (`T`) remains `MonoTopDownmix` only. No current
corpus MP4 carries channels 28/29 or a present mono/stereo-top block.
MP4 track detection ignores nested `minf/hdlr`
(`url `) so it cannot clear `mdia/hdlr=soun`. Native PCM: compare in the
HDMI-7.1 Float32 domain (emulator max 8ch), not a native 12ch dump. On
`auro_cx.mp4` AUs 0..260, FL corr≈0.9947 at lag **+2816** (oracle delayed)
via `regression/compare_oracle_f32_hdmi8.py`. Full 12ch discrete / exact
PCM24 equality still open. PCM24 output is streamed one AU at a time.
Native LPC state is reset for each AWC frame; only the LFE interpolation
filter state crosses access-unit boundaries.

SASC channel-bed decoding distinguishes the Header+60 mode-query flag from the
SCG apply layer. The bit is consumed by `calculate_mode` during processor
initialization; it does not gate `Processor::run_`. PDU `source_layer`
(`sasc_channel_bed_layer`) only selects bed
carriers for planning; it is not the `scg::details::decode` filter. Full
discrete bed+height WAV uses playback SCG layer **2** (native SCG `+448` from
`add_scg` ← `calculate_mode` / `SCG::initialize`), so `step.layer < 2` cancels
both height (layer 1) and stereo-fold (layer 0). Passing PDU `source_layer=0`
skips all cancels and leaves height audible in the bed (confirmed on
`auro_cx.mp4`: corr≈0.55–0.77 with layer 0 vs ≈0 with layer 2). Files with
An AU with `config_flag=0` retains the SCG context created during processor
initialization; only a processor that never created an SCG has a no-op plan.

Layer-0 stereo fold for layouts with FL|FR (`get_layouts` → L0=`3`) includes
native LFE→FL/FR routes from `auro_downmix_v1_plan_add_center_routes`
(`implementation.inc`): gain indices **104/105**, default table **−3 dB**
(same Q23 family as `1/√2`). With SCG+448=2 those steps are cancelled like
any other layer-0 edge — do **not** special-case mute/skip LFE cancel to
force silent FL/FR. On `auro_cx.mp4` channel-ID LFE section, listening
`FL≈FR≈−LFE/√2` is consistent with that cancel when FL/FR carriers are
near-zero (fold absent in the bitstream for that window); native discrete
layer 2 keeps the same math. Height dematrix (layer 1) remains the validated
path; LFE fold is not a separate bug to invent around.

Known layouts:

- `0x01bf`: 7.1
- `0x663f`: 5.1+4H (9.1)
- `0x7fbf`: 7.1+5H+T (13.1)
- Some lossless files carry `0x8060`; their explicit schema channel list is
  authoritative and the `acxd` value is not yet named.

Initial AWC configuration observations:

- lossless: payload length is `9 + 4 * stream_count` bits; observed common
  preamble `10`, per-stream parameter `2`;
- transparent/near-lossless: payload length is `8 + 12 * stream_count` bits;
  observed common preamble `5`, per-stream parameter `2`;
- near-lossless layouts usually use a separate LFE PDU for stream 0.

## Current decode status and warnings

Do not treat the initial schema syntax as invariant for all frames. Delta AUs
reuse stored counts and PDU templates and omit the initial PDU count prefix.
The AU2 probe currently handles this on the corpus, but later-AU coverage still
comes primarily from the end-to-end decoder and needs deterministic traces.

The PDU vector and every payload now decode from the direct ConfigData cursor;
the former compact-bed and AWC start-bit scanners have been removed.

All eight files currently complete with direct schema and payload parsing.
Markus lossless and Alessandro near-lossless produce non-clipped, signal-like
30-second `astats` results. HDMI-7.1 native float oracle matches FL at
corr≈0.995 on an AU prefix; full 12ch discrete PCM equality and
object-based SASC routing remain open — do not treat consumption alone
as a complete PCM oracle.

The LFE residual parser now feeds native-order persistent multistage
interpolators for factors 40/80/160/320/640. Each stage performs its own float
multiply-accumulate before feeding the next stage; the old combined impulse
response is gone. The coefficient sets are
the `if40_*`, `if80_*`, `if160_*`, `if320_*`, and `if640_*` tables used by
`multirate::interpolation::create`; their response peaks are at the native
delays 122/245/493/988/1977 samples. LFE and both AWC policies are required to
consume every selected PDU bit exactly.

The missing cursor branch was `auro_cx_user_Extension_t` in ConfigData. Each
extension reads two base-8 VLQs encoded as 4-bit chunks, then a VLQ(8) payload
byte count and exactly that many bytes. Skipping only the extension count left
the PDU vector 40 or 48 bits early on affected AUs.

## Decompiled-code anchors

All line numbers below refer to the current `decopiled/libauro.so.c`. Function
names and IDA addresses are stronger anchors than line numbers if the file is
regenerated.

### Schema block and dynamic configuration

- Around line `253830`, IDA `0x416EB0`:
  `Traits<auro_cx_schema_Block_t>::traverse<...Decoder...>`.
  This shows the order: decode `ConfigHeader`, copy context counts, traverse
  `ConfigData`, then decode `Audio.pdus`.
- Around line `253890`, IDA `0x416FC0`:
  `schema::coder::decode<auro_cx_schema_ConfigHeader_t,...>`.
  Port this field-by-field and record the bit offset after every conditional.
- Around line `254726`:
  `Traits<auro_cx_schema_ConfigData_t>::traverse<...Decoder...>`.
  This is the primary next anchor for initial versus delta metadata.

### AWC lossless

- Around line `297243`, IDA `0x443340`:
  `awc::common::Processor<lossless::Policy>::parse_`.
- Calls to the key syntax functions are near lines `297464` and `297470`.
- ICC/pair subblock reader begins around line `298231`.
- LPC execution call is near line `300343`.
- LPC lossless decoder body begins around line `300462`.
- `lossless::Policy::parse_bitdepth_mode` begins around line `303329`.
- `lossless::Policy::parse_angle` begins around line `303406`.
- `parse_lpc_order_and_error_scale_factor_precision_info` begins around line
  `303433`.
- LDC dispatch and per-subelement decode calls are around `305240`-`305620`.
- `run_length::signaling::Decoder::decode`, IDA `0x4512D0`, begins around line
  `311738`. Entropy types 2 and 3 are distinct: type 3 grows its remainder width
  by the unary prefix and adds the exponential bucket offset.
- Native common-Golomb reads consume `golomb_param + 1` bits after
  `pop_all_ones`: one zero terminator followed by exactly `golomb_param`
  remainder bits. Do not read an additional remainder bit.
- `Reader::pop_all_ones`, IDA `0x338E30`, counts consecutive one bits and leaves
  the following zero unconsumed. Do not invert this into a zero-run counter.
- `run_length::alternating::Decoder::decode`, IDA `0x4539D0`, begins around line
  `313863`. Port each `(marker_type, index_type)` switch branch independently;
  do not collapse type 2 and type 3 into a shared Golomb helper.
  For `(type 2|3, fixed)` a zero-width fixed index is dense and decodes exactly
  `count` marker symbols; it does not reserve or omit the last output slot.
  For `(type 2|3, type 2|3)`, decode the index jump with the second entropy
  policy, then the nonzero marker with the first policy. Both positions retain
  their own type-2/type-3 behavior.
  The same high-level order applies to all 16 type pairs: decode a jump with the
  second policy, stop successfully if it lands exactly at `count`, otherwise
  decode `marker+1` with the first policy and continue after that output slot.
- `Params::read_subelements_granules_`, IDA `0x445490`, begins around line
  `299346`. In partitioned mode each encoded property count advances through
  the granule-property vector; successive subelements sum disjoint ranges, not
  repeated prefixes from element zero.
- `Properties::initialize_diff_granule_sizes`, IDA `0x449A50`, begins around
  line `303493`. It branches on the first granule value, not vector length: when
  `first > 1`, output is `[1, first-1, remaining...]`; otherwise it is an exact
  copy, including for multi-element vectors.
- `Params::read`, IDA `0x445060`, passes the decoded custom maximum into
  `read_subelements_`. That function clamps the property vector to the maximum,
  partitions and entropy-decodes only that prefix, and the LDC decoder
  zero-fills the remaining output elements. Do not substitute the full vector
  length for the custom maximum.
  Its first flag enables the LDC payload: zero produces an all-zero vector.
  When enabled, the second flag selects the optional maximum; granule
  partitioning is used whether that second flag is zero or one.
- `run_length_dispatch::Params::read`, IDA `0x445B70`, reads and stores entropy
  parameters before decoding. Dense entropy type 2/3 must reuse the stored
  four-bit Golomb parameter; reading a second parameter shifts the stream.

### AWC transparent / near-lossless

- Around line `301351`:
  `awc::common::Processor<transparent::Policy>::parse_`.
- Bit-depth mode call is near line `301576`.
- ICC/pair subblock reader begins around line `302282`.
- LPC execution call is near line `302811`.
- Transparent LPC decoder body begins around line `302942`.
- `transparent::Policy::parse_bitdepth_mode` begins around line `316063`.
- LPC order/precision parser begins around line `316100`.

Useful search command:

```powershell
rg -n "ConfigHeader_t|ConfigData_t|generation::awc|ErrorReconstructor|ErrorScaler" decopiled/libauro.so.c
```

## Test corpus

Files are in `test_files/aurocx/`. Eight known AuroCX MP4s cover these cases:

- Lori Lieberman: 9.1 near-lossless; `LFE + AWC(2) + AWC(3) + AWC(4) + SASC`.
- Markus Schulz: 13.1 lossless; `AWC(2) + AWC(7) + AWC(5) + SASC`.
- Ola Onabule: 9.1 lossless; `AWC(2) + AWC(4) + AWC(4) + SASC`.
- Alessandro Quarta: compact 7.1 near-lossless;
  `LFE + AWC(2) + AWC(5) + SASC`.
- Yamamoto 13.1 lossless: `AWC(2) + AWC(7) + AWC(5) + SASC`.
- Yamamoto 13.1 near-lossless 1664 and 3328 kbps:
  `LFE + AWC(2) + AWC(6) + AWC(5) + SASC`.
- `auro_cx.mp4`: 11.1 near-lossless, no `acxd`; schema bed
  `FL,FR,C,LFE,LS,RS,LB,RB,HL,HR,HLS,HRS`;
  `LFE + AWC(2) + AWC(5) + AWC(4) + SASC` with 3d→2d custom `gain0`.

Do not hardcode filenames or these group sizes into the parser.

## Build and regression commands

Build from the tool directory:

```powershell
cd "C:\Users\USER\IdeaProjects\Orua D3\tools\auro3d-decode"
cmd /c build.bat
```

If `rc` reports `RC1109` for `bin/obj/auro3d-decode/xinn_presets.res`, treat the
build as failed even if an older `bin/Release/auro3d-decode.exe` exists. Record
the executable timestamp whenever results come from that older binary.

Binary:

```text
C:\Users\USER\IdeaProjects\Orua D3\bin\Release\auro3d-decode.exe
```

Probe one file:

```powershell
& "C:\Users\USER\IdeaProjects\Orua D3\bin\Release\auro3d-decode.exe" --probe -i "<absolute mp4 path>"
```

Run the AuroCX corpus:

```powershell
$exe = (Resolve-Path bin/Release/auro3d-decode.exe).Path
Get-ChildItem test_files/aurocx -File -Filter '*.mp4' |
  Where-Object { $_.Name -match 'AUROCX|Auro9_1|Auro13_1|auro_cx' } |
  ForEach-Object {
    Write-Output "`n=== $($_.Name) ==="
    & $exe --probe -i $_.FullName
  }
```

After edits always run:

```powershell
git diff --check -- tools/auro3d-decode/src/app/cx_probe.cpp tools/auro3d-decode/src/app/cx_probe.hpp doc/auro_decoder.md
```

For AuroCX decoder changes include all touched decoder files and `AGENTS.md` in
the `git diff --check` invocation.

Expected initial-AU invariant for all seven files:

- `auro_cx_detected=1`
- `sync_a3dc0ded=1`
- `schema status=header_decoded`
- `pdus ... complete=1`
- PDU types/grouping match the corpus list above.

## Recommended implementation plan (current)

### Phase 1: exact frame schema (mostly complete on AU1/AU2 corpus)

1. Add a trace-friendly bit reader result that can record field name, start bit,
   width, value, and failure point without changing normal output.
2. Port `ConfigHeader` from `0x416FC0` exactly. Keep decoded state from AU 1 and
   pass it into AU 2; do not parse every AU into a fresh context.
3. Port the conditional `ConfigData` traversal around line `254726`.
4. Compare bit offsets for AU 1 and AU 2 across all seven files.
5. Extend deterministic cursor traces beyond AU2.

Acceptance reached for the current corpus without a PDU-vector scan.

### Phase 2/3: lossless AWC syntax and reconstruction (active)

1. Add small integer-only tests for entropy symbols, signaling zero-runs, and
   alternating marker/index pairs.
2. Verify reconstructed PCM/ICC against a native-oracle capture or a confirmed
   invariant before declaring lossless complete.

Acceptance: one complete lossless file produces 1024 samples per referenced
stream per audio AU, with exact PDU consumption and frame-local LPC history.

### Phase 4: near-lossless and output completion

1. Confirm SASC order/routing and implement object-group branches only from
   identified native paths.
2. Add AuroCX PCM invariants to the automated regression suite; strict
   consumption cases already cover all seven MP4s.
   JSON covers the separate WAV/codec-v3 path, not these MP4s.

## Android emulator as an oracle

Use the emulator only when static analysis leaves two plausible interpretations.
The installed Artist Connection app and its native `libauro.so` can serve as a
behavioral oracle. Prefer non-invasive checks: logcat, packet/format observation,
and debugger traces. Do not modify account data, refresh credentials, call
catalog APIs, or download media unless explicitly requested.

Known tools from the current machine:

```text
ADB: D:\Distrib\Android TV\adb\adb.exe
AVD: test (Medium_Phone / android-29)
emulator: %LOCALAPPDATA%\Android\Sdk\emulator\emulator.exe -avd test
device: emulator-5554
package: com.streamsoftinc.artistconnection
Frida: Kahlo MCP (user-frida-kahlo), frida-server healthy on emulator
```

### Oracle status (2026-07-17)

- **P80X app update (21:26):** `com.streamsoftinc.artistconnection`
  `versionName=1.26.36` `versionCode=350` (arm64 split). Attach by
  process name `com.streamsoftinc.artistconnection` — Frida display name
  `Artist Connection` can resolve to an unrelated process.
- **libauro.so (arm64)** from `split_config.arm64_v8a.apk`: size
  `7098080`, MD5 `5dfd6c4c47ecbade773914cb77b8e2e7`. Differs from the
  old **x86_64** baseline MD5 `2b8fb6765cecdbc45346d360f2fa22c5` (arch),
  but JNI `AuroVersion()` still returns **`Version: 4.0.14-9d106532`** —
  same generation as `decopiled/libauro.so.c`. Do **not** trust raw IDA
  file offsets on arm64 without symbol rebase; prefer Java A3DENG /
  exported JNI.
- Java API still `androidx.media3.decoder.auroenginev4.A3DENG` with
  `auro_update_params` / `AuroUpdateParams.output_layout` (long). Native
  update entry is **`AuroUpdate2(J, AuroUpdateParams)`** (`AuroUpdate`
  absent). `AuroPop(J, Buffer, I)` unchanged.
- No catalog download; local AU bytes only (app cache / Frida).
- **Push reject root cause (fixed in harness):** JNI `AuroPush` uses
  `GetDirectBufferAddress`. Heap `ByteBuffer.wrap()` → NULL → false.
  App path allocates `ByteBuffer.allocateDirect` (`AuroEngineDecoder`).
  `maxOut=0` / packed `0x100000000` after ACX configure is a JNI low32=0
  quirk, not a hard reject; pop still works.
- **Working 8ch Frida path:** `tools/auro3d-decode/oracle/oracle_acx_pcm_job.js`
  (+ CLI `oracle_acx_pcm_frida.js` / `run_oracle_f32.py`) —
  `initialize(ACX)` → HDMI → `set_channels_backs_before_surrounds(true)` →
  `set_num_output_channels(8)` (Layout_7_1 mask `0x01BF`; API rejects 12) →
  `set_decoder_input_format(audio/a3ds@48k)` → Float32/32 update →
  direct-buffer `push` + `AuroPop` with capacity ≥ `832*8*4`
  (`part_byte_count=26624`).
- **12ch discrete dump — BLOCKED (2026-07-17, P80X / AC 1.26.36):**
  Root cause is **not** a late `AuroPop` mirror prune. Native
  `A3DENG::update` → `Decoder::configure` rejects mask `0x67BF` with
  `AURO_A3DENG_V4_RC_INVALID_OUTPUT_LAYOUT` (RC **255**) for HDMI and
  Soundbar `target_device`. With `channels_backs_before_surrounds=true`
  (`hdmi_mapping=1`), `sub_31ACE0` builds only the 8-slot HDMI bed order
  so configure succeeds as 8ch while `get_output_channel_count()` still
  reports `Mask_count(0x67BF)=12` until the first pop. With mapping=0 the
  layout size logs as **12**, but configure fails and the prior 8ch
  pipeline remains — `AuroPop` stays `26624=832×8×4`. Forcing
  `Decoder::configure` return 0 via Frida makes `update()` succeed but
  does **not** rebuild a 12ch pipeline (pop still 8ch). Harness:
  `oracle/oracle_acx_pcm_12ch_job.js` (spawn needs `AppKillService`
  bypass via `Java.performNow` — adb launch hits background-service
  crash). Resolve by **mangled symbol** (`DebugSymbol` /
  `enumerateExports`), never raw IDA offsets on arm64.
- **HDMI 8ch order** (`hdmi_mapping==1` / `kHdmiOrder_1db2b0` in
  `android.cpp`): FL,FR,C,LFE,LB,RB,LS,RS (Auro ids `0,1,2,3,7,8,4,5`).
  Schema bed `0x67BF` WAVE-sorted 12ch places the same 8 beds first, then
  HL,HR,HLS,HRS — so 12→8 for HDMI is select-first-8 (no guessed height
  fold). Compare: `regression/compare_oracle_f32_hdmi8.py`.
- Captured `auro_cx.mp4` AUs 0..260 → `bin/Release/oracle_pcm_f32_0_260.bin`
  (8ch interleaved float). Aligned table (lag **+2816**, oracle delayed):
  FL corr≈0.9947 rms_err≈0.0011; FR/C/LFE/LB/RB/SL/SR near-silent in
  **both** streams for this AU window (ours FR first becomes active ≈AU
  260.8). Not a routing bug in the early prefix — content gate. Exact PCM24
  equality not claimed. Keep validating in HDMI-8 until a path accepts
  discrete height output (or CX planar dump under working 8ch config).
- **P80X (`192.168.0.238:5555`) ANR wall (~9s):** continuous Frida
  `Java.perform` / decode bursts are force-finished by ActivityManager
  around AU≈32 even with background workers and setTimeout chunking.
  Short packs (≤31 AUs) and fresh-engine mid packs succeed; stable capture
  past AU≳300 still needs the emulator (or a non-ANR host). Harness:
  `oracle/run_oracle_f32_attach.py` (async bursts) + restored
  `oracle/oracle_acx_pcm_*.js` / `pack_aus_from_mp4.py`. App also dies on
  plain `am start` via `AppKillService` background restriction — Frida
  **spawn** + early `ContextWrapper.startService` skip is required.
- Next for 12ch: patch/bypass the **layout validator inside**
  `Decoder::configure` (symbol
  `_ZN4auro6a3deng2v47Decoder9configureERK48auro_a3deng_v4_parameter_decoder_Configuration_t`)
  so the pipeline actually builds 12 slots; or dump CX planar buffers
  under working HDMI-8 config before output remap. Else keep HDMI-8
  oracle + emulator capture through multichannel onset (AU≳300).
  ESPCAP object panning (`0x4BF010`) remains the linked-SASC gap.

Useful first checks:

```powershell
& "D:\Distrib\Android TV\adb\adb.exe" devices
& "$env:LOCALAPPDATA\Android\Sdk\emulator\emulator.exe" -list-avds
& "D:\Distrib\Android TV\adb\adb.exe" -s emulator-5554 shell pidof com.streamsoftinc.artistconnection
```

If native tracing is needed, first identify the loaded `libauro.so` build and
verify that its addresses correspond to this decompilation. ASLR means IDA
addresses must be rebased to the module load address (observed base this
session: `0x775ad96c1000`).

## Coding rules for this task

- Use little-endian bit order within bytes: current `Bits::get` reads bit 0
  first. Do not silently replace it with an MSB-first reader.
- Treat every conditional read as fallible and preserve the exact failure bit.
- Use 64-bit arithmetic for bit lengths, MP4 offsets, and payload bounds.
- Never trust PDU sizes until they are checked against remaining blob bits.
- Avoid heuristics in the PCM path. If a recovery heuristic remains in probe
  output, label it explicitly.
- Keep lossless and transparent policies separate where the decompiled code does.
- Do not reset state per packet unless the format signals a reset.
- AWC LPC is a confirmed frame-local exception: `Processor::run_` clears the
  decoder initialization field at `a1 + 392` for every output stream block;
  recursion is retained only across subblocks inside that block.
- Update `doc/auro_decoder.md` when a format branch becomes confirmed.
