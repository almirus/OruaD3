# Auro3D codec-v3 encoder — implementation plan

## Параметры командной строки

Пример обычного запуска:

```text
orua3d-encode -i input.flac --input-channel-order FL,FR,C,LFE,LS,RS
```

### Вход и выход

| Параметр | Значение и влияние |
|---|---|
| `-i`, `--input FILE` | Входной PCM24 WAV или многоканальный FLAC. Для FLAC требуется `ffmpeg`: файл временно декодируется в PCM24 WAV, после чего обрабатывается тем же поточным чтением, что и WAV. Параметр обязателен. |
| `--output FILE` | Выходной carrier WAV или FLAC. Если не указан, имя строится как `<исходное_имя>_<layout>_auro.wav`, например `track_5.1.4H_auro.wav`. |
| `--input-channel-order LIST` | Порядок каналов во входном interleaved-файле, например `FL,FR,C,LFE,LS,RS` или `FL,FR,C,LFE,LB,RB,LS,RS`. Влияет на привязку PCM-сэмплов к codec-v3 channel ID и на исходную Auro-раскладку. |
| `--input-layout LAYOUT` | Не задаёт раскладку, а проверяет её. Она должна совпадать с каналами из `--input-channel-order`. |
| `--output-layout LAYOUT` | Проверяет физическую раскладку выбранного carrier-контейнера. Произвольное преобразование этим параметром не выполняется. |

Если `--input-channel-order` не указан, для WAV используется маска
`WAVEFORMATEXTENSIBLE`. Для FLAC маску формирует `ffmpeg`; для надёжности
порядок каналов лучше указывать явно.

### Планирование и качество кодирования

| Параметр | Значение и влияние |
|---|---|
| `--host-block N` | Размер блока чтения входа (по умолчанию `832`). Меняет границы host-вызовов, но не должен менять carrier-результат. Используется для проверки split-call непрерывности. |
| `--unit-block N` | Предпочтительный размер native codec-v3 UnitBlock (по умолчанию `1024`). Допустимы native-размеры; влияет на границы кадров, метаданные и нагрузку обработки. Последний блок при необходимости дополняется нулевыми сэмплами, а выход обрезается до исходной длины. |
| `--profile N` | Профиль native codec-v3 от `1` до `5` (по умолчанию `2`). Выбирает таблицы конфигурации, bit-line и backend кластеризации; влияет на размер и качество carrier. |
| `--reserve-extra-bits N` | Значение native `Encoder::reserve_extra_bits` (по умолчанию `160`). Резервирует место в бюджете payload; большее значение может уменьшить доступный бюджет квантования. |
| `--seed N` | Фиксированный seed рескейлера/dither. Делает результат воспроизводимым между запусками. Диапазон `0..4294967295`. |
| `--no-dither` | Отключает native dither. Меняет низкоуровневые PCM-значения carrier, особенно на тихих/нулевых участках. |
| `--cts-dmx` | Включает ограничение коэффициентов secondary scalar downmix (`cts_dmx_coeff_limit_`). Влияет только на соответствующую native downmix-ветку. |
| `--input-scaler-indices LIST` | Явно задаёт native scaler index `0..255` для каждого входного канала в порядке WAV. Меняет gain/rescale перед анализом; количество значений должно совпадать с числом каналов. |
| `--input-gains-db LIST` | Dynamic gains исходных каналов в dB, диапазон `[-24,0]`. Переданные значения попадают в native Dynamic metadata; пустые элементы оставляют канал без записи. |
| `--carrier-gains-db LIST` | Dynamic gains каналов carrier в dB, в порядке carrier-layout. Влияет на вторичное downmix и соответствующие metadata records. |

### Диагностика

| Параметр | Значение и влияние |
|---|---|
| `--validate` | Полностью выполняет анализ, сериализацию, metadata/CRC и структурную проверку каждого UnitBlock, но не создаёт выходной контейнер. Полезен для проверки входа и конфигурации. |
| `-v`, `--verbose` | Печатает в `stderr` путь входа, sample rate, количество каналов/сэмплов, block-параметры, порядок каналов, исходную/carrier/output раскладки и итоговый статус. На carrier не влияет. |
| `--trace` | Печатает подробности каждого UnitBlock: группы, режимы анализа, bit budget, serialized bits, CRC и carrier digest. Только диагностика, выходные сэмплы не меняет. |
| `-h`, `--help` | Показывает логотип и эту справку. Запуск без параметров делает то же самое. |
| `--version` | Печатает версию encoder: `0.0.1_alfa`. |

Параметры `--input-layout` и `--output-layout` являются проверками, а не
ремапперами. Для изменения порядка каналов нужно передать правильный полный
список `--input-channel-order`.

### Многопоточность

`--threads N` задаёт общее число потоков кодера в диапазоне `1..256`.
`--threads 1` полностью отключает параллельный анализ и служит эталонным
детерминированным режимом. Если параметр не указан, кодер использует число
обнаруженных логических процессоров. Параллельно анализируются только
независимые активные carrier-группы внутри одного UnitBlock; порядок UnitBlock,
dither, metadata cyclers и запись carrier остаются последовательными.

Число потоков не должно менять выходной файл. Регресс
`regression/compare_threads.ps1` кодирует один источник без параметра,
с `--threads 1` и с явно заданным числом потоков, затем требует полного
совпадения SHA-256 всех трёх carrier-файлов. Для сравнения используется
фиксированный `--seed`, потому что без него native dither по определению
инициализируется временем запуска.

## Scope

This directory is reserved for a standalone encoder of the native Auro-Codec
**codec-v3** PCM carrier format.  The implementation must be derived from
`decopiled/libauro.so.c` and validated against the installed native library.

In scope:

- PCM input in a declared, supported Auro speaker layout;
- codec-v3 carrier generation, including embedded metadata, frame payload,
  Golomb-Rice coding, CRC and packet/frame scheduling;
- layouts and reconstruction modes that are proven by the native encoder;
- deterministic encode/decode and native-oracle regression tests.

Out of scope:

- **AuroCX** (`a3ds`, MP4, AWC, ICC, SASC, schemas, PDUs);
- container muxing beyond a simple PCM/WAV carrier writer;
- invented downmix, gain, clipping, frame-alignment, or metadata heuristics;
- emitting a format variant not demonstrated by the native codec-v3 path.

## Ground rules

1. `decopiled/libauro.so.c` is the behavioral source of truth.  Record every
   ported routine with its symbol and IDA address in the source comments and
   in the test vector manifest. `native_map.md` is the concise evidence log.
2. The existing decoder is a parser and reconstruction reference, not a
   specification for an inverse transform.  Derive analysis, quantization and
   frame writing from the native encoder code before implementing them.
3. Use the installed native library only as an oracle: it may be used to
   generate or inspect test vectors, but must not be required at runtime.
4. Keep codec-v3 state continuous across arbitrary host blocks.  Encoder input
   chunking must not change carrier bytes or decoded PCM (apart from explicitly
   documented final-frame padding).
5. Do not add AuroCX source files, options, probes or tests to this tool.

## Proposed layout

```text
tools/auro3d-encode/
  README.md                         this plan
  build.bat                         independent Windows build
  src/
    app/                            CLI, WAV/PCM I/O, validation
    codec_v3/                       encoder pipeline and bitstream writer
    codec_v3/detail/                constants, native offsets, trace helpers
  regression/
    cases.json                      fixed codec-v3 test-vector manifest
    run_regression.py               reproducible checks
    vectors/                        small, versioned PCM and carrier fixtures
```

Only code genuinely shared with `tools/auro3d-decode` should move into a small
common, codec-v3-only library after byte-for-byte regression proves that the
move changes neither tool.

## Current implementation status

The standalone executable derives the exact original layout from the complete
interleaved channel-name list, validates PCM24 RIFF/WAVE input, streams host
blocks into native unit blocks, and prepares the encoder configuration.
`--input-layout` and `--output-layout` are optional assertions; they never
override the layout implied by the channel scheme:

```text
orua3d-encode --validate --input source.wav --input-channel-order FL,FR,C,LFE,LS,RS
orua3d-encode --input source.wav --input-channel-order FL,FR,C,LFE,LS,RS --output auro2d.wav
orua3d-encode --input source-height.wav --input-channel-order FL,FR,C,LFE,LS,RS,HL,HR,HLS,HRS --output auro3d.wav
```

Native 5.1 Auro 2D output is physically stereo while its embedded codec-v3
carrier mask remains logical 2.1, matching `test_files/correlation/auro2d.flac`.
Input with `HL,HR,HLS,HRS` instead selects the corresponding Auro 3D original
layout and physical 5.1 carrier automatically.

`--validate` now also runs `Config::init_defaults` / `Config::validate`, prints
the native encode-group plan, materializes groups, and runs analysis through
DetectSilence, Rescaler's Q31 `2^39/scaler` conversion followed by
`shift_right(bit_line+8)`, mix2 deltas, mix2
`cluster_deltas` Quantization (`BitShift<100>` @ `0x501F70`) + Mixer
(`0x4EE540`), and arity-1 Mixer copy / silent carrier. Each complete unit then
passes the same channel-frame, carrier merge, and metadata/CRC assembly used by
the output path. It also performs the full output-boundary header, sync-gap,
per-channel ADOL, serialized-size and CRC readback without opening a container;
only after those checks does `--validate` discard the resulting carrier.

The standalone `compute_mix3_deltas` primitive now matches the native three
specializations. Fully active mix3 groups feed their ordered two-component
residuals into the selected native GVM learner. Both modern mode 0 and
`GVMOldFast` mode 2 implement online singleton insertion, minimum-cost cluster
merging, population ordering, center conversion, and compact bit-budget
selection. The confirmed `GVM::run_` input-shape gate is active for both
paths: mix2 must provide one residual scalar per sample and mix3 exactly two.
Counts must be 32-bit representable and divide without a remainder; trace
reports the number of validated one- and two-dimensional groups.
The quantizer budget retains native `Group+208`, but does not confuse it with
profile clustering `field_52`: native Group creation copies Encoder+6352,
which is written by `Encoder::reserve_extra_bits @ 0x4E37E0`.
`--reserve-extra-bits N` exposes that value and defaults to 160. Trace exposes
the remaining payload
allowance, fixed cost, and actually used table bits across encoded groups.
The `GVM::run_` cluster-search controller is also represented explicitly:
dimension-specific starts, the common cap, minimum cluster count, fit statuses
1..3, decrement retries, and native +30 expansion are applied to each learner
result.
Its bounds now come from the confirmed `Encoder::create_group_` writes:
profile clustering limit (or 150), bit-line quality for both dimensional
starts, and minimum one; the temporary constructor defaults are not used for
analysis.
Fit status uses the native strict ten-percent spare-bit threshold, and its
Group+520-compatible flag survives analysis, metadata preparation, trace, and
final carrier validation.
Carrier assembly refreshes retained channel words and CRCs after the reserved
PCM metadata bits are projected, keeping diagnostics tied to emitted carrier
samples.
`channel_frame_plan` can serialize analyzed arity-1/2/3 tables through the
decoder-confirmed metadata/context prefix and a static Golomb-Rice stream
whose native `k=1..7` parameter is selected by `BitSize::calculate`; the
serialized index length must exactly equal its population-derived cost;
the CLI invokes this builder for every scheduled UnitBlock.
`--trace` adds a per-unit carrier range and serialized-word summary without
changing the emitted samples. It reports serialized words, the exact
serialized Channel bit count, a channel-CRC digest, metadata-block
count, a post-metadata carrier FNV digest, and the number of learned GVM
groups. `max_shift_attempts` exposes how many native static candidates were
tested before the selected result.
The accompanying `mix3_mixer_reconstruct` port mirrors the native
`mix3::mix<0/1/2>` loops and five seed fixups.

The all-silent `DetectSilence` path also follows the native Rescaler dither
branch. It keeps one 20480-sample triangular pool per `bit_line + 8`, uses the
same two-stage MT19937 seeding and window selection, saturates before
`shift_right`, and persists the pool RNG across units. `--seed N` maps to the
native Config+144 seed; without it the encoder uses `time(nullptr)`.
`--no-dither` writes zero to the confirmed Config+48 enable value.
`ComputeQuality` retains per-frame measurements but gives this synthetic
zero-frame branch the native aggregate score `bit_line * 0.01`.

`unit_encoder.*` now exposes the complete currently ported pipeline for one
full unit: descriptor validation, group analysis, carrier preparation, strict
channel-frame serialization, merge, and layout metadata/CRC assembly. The
native `prepare_metadata_unit_block_` group records are retained in the result
alongside the carrier rather than discarded; the channel serializer emits the
confirmed layout/ADOL subset inside each native A3D stream. Every carrier
channel uses its analyzed group `bit_line` as the physical mux width; there
is no independent metadata-width setting. Source length is preserved exactly:
a preferred-size prefix is followed
by a bounded schedule of native UnitBlock sizes (256..1024 in steps of 16,
plus 1000). If the source length is not divisible by eight, the final native
unit is zero-padded internally and the container writer trims the carrier back
to the exact source sample count.
The native profile selector is exposed as `--profile 1..5` (default `2`) and
flows through the confirmed `Config::init_defaults` profile table; no profile
aliasing or implicit quality conversion is performed.
Cluster-deltas backend selection follows the native factory: profiles whose
configuration explicitly selects mode 3 use BitShift Quantization only;
remaining profiles use GVM. The two paths are no longer incorrectly chained,
and profile 4/5 initialization accepts their valid Quantization mode.
The effective nine-pair clustering block (+52 through +120) is retained
without the former +76/+80 hole, so explicit Config values and selected
backend cannot diverge during initialization.
For GVM, factory mode is explicit: mode 0 is the modern learner, mode 2 is
OldFast, and native mode 1 has no constructible learner and is rejected with a
specific error.
GVM input preflight now performs the actual native `set_data_<1/2>` shape and
int32-to-double conversion for every point, including ordered 2D pairing, and
tracks whether the learner uses the optional deterministic seed.
The corresponding learner-center output conversion is also available:
truncating SIMD-compatible double-to-int32 behavior, wrapped squared-distance
selection, stable ties, and the optional forced-zero center rule are preserved
for dimensions one and two.
Learned GVM output has a strict finalization boundary for sizes, indices, and
double centers. It validates observed cluster populations, selector capacity,
converted centers, total BitSize, and status 1..3 before a result can enter
codec metadata.
Modern GVM cluster summaries and merge-cost arithmetic are implemented for
both supported dimensions with native population, sum, reciprocal-energy, and
double operation order.
The OldFast full-table decision is implemented with the equivalent global
minimum comparison between inserting a singleton and merging an existing
pair. Both learners emit nonempty clusters in descending population order.
Output validation also cross-checks every channel frame against its matching
group record (arity, source order, and headroom/bit-line), so a structurally
valid frame cannot be emitted with contradictory unit metadata.
It additionally checks the embedded ADOL channel-config tuple against the
original layout before the carrier writer is opened.
ADOL blocks are validated independently for opcode width, terminator usage,
and scalar/two-byte payload bounds before any metadata bits are written.
The fixed metadata prefix is validated separately for field widths and
extension-word cardinality before the ADOL stream is opened.

Native metadata validation trims residual entries at the highest referenced
index while retaining any longer quantizer level schedule, matching the
separate storage lifetimes in `prepare_metadata_unit_block_`.
Short residual tables use the native `error_center_index` capacity rounding;
the channel context is padded to the selected capacity without changing any
sample index or reconstructed value.

Candidate selection reconstructs the encoded carrier through the inverse
mix2/mix3 predictor, applies `ComputeQuality::unscale_`, measures every
original source plane, and retains the lowest native aggregate error level.
The carrier-keyed quality filter persists across UnitBlocks and prunes bit-line
candidates with the confirmed two/three-point rule.

Rescaler peak protection selects the first native scaler-table entry covering
the active-source sum, retries successive indices only after Mixer reports
carrier overflow, and emits the base scaler as parser opcode `0x41`. Its
dynamic 16-bit cost is included in the quantizer allowance; the default
Group+196 cost is 32 bits because Group+268 is absent. The alternate
Group+268-present branch starts at 72 bits.
The complete optional-record accounting and native closed opcode-width table
are exposed as checked helpers; unknown instructions cannot silently consume
an assumed width. Arity-1 frames now carry the same `0x41` scaler record when
an overflow retry has selected Group+392.

The input-rescale layer also exposes the confirmed native scaler gain formula,
index validation, and exact 241-entry index-to-scaler table; index `255`
retains the native infinity sentinel.
Library callers may opt into that scalar `downmix_` operation by setting
`EncoderConfig::input_scalers_present` and supplying all 31 explicit scaler
indices. The CLI accepts the same values with `--input-scaler-indices`, in
the physical order declared by `--input-channel-order`; the default path
performs no inferred gain or routing.
Explicit ADOL metadata supports native primary downmix gain (`0x40`),
secondary per-carrier gains (`0x46`), and loudness types 0..5
(`0x80..0x85`). Their native quantizers and field limits are applied before
the complete channel stream is projected.

Validation already reads the PCM payload in a streaming fashion.  It converts
each interleaved sample to a signed PCM24 `int32_t` plane addressed by the
explicit codec-v3 channel ID; the default host block is 832 samples and can be
changed for split-call tests with `--host-block N`. Padding, when required by
native UnitBlock geometry, is added only after the physical source has ended.

Host calls are reassembled according to an exact native UnitBlock schedule.
The preferred block is 1024 by default; the final units are rebalanced when
needed so their sum covers the source plus at most seven final padding samples.
The written WAV/FLAC still contains exactly the original source length.

## Implementation increments

### 0. Reconnaissance and executable specification

- Locate `auro::codec::v3::unit::encoder::Encoder::{Encoder,construct_,initialize,process}`
  and all callees in `decopiled/libauro.so.c`.
- Trace native calls with controlled PCM vectors: silence, impulse, polarity,
  per-channel tones, random bounded PCM, and a non-frame-aligned tail.
- Produce a concise `native_map.md` before code: configuration fields, state
  offsets, sample/frame sizes, layout mapping, bit order, metadata schedule,
  payload order, checksums, latency and finalization behavior.
- Establish whether the public native encoder API is callable; if it is not,
  derive equivalent traces from the encoder unit and use decoder parsing as a
  structural checker.

**Exit gate:** each emitted carrier field has a native-code location or a
native captured-vector observation; unknown fields stay explicitly marked
unknown rather than guessed.

### 1. Standalone skeleton and strict input contract

- Add `build.bat` and a CLI accepting PCM24 WAV (and, only if needed, raw PCM
  with all format fields explicit).
- Implement checked WAV parsing/writing, planar/interleaved conversion and
  exact channel-layout mapping.  Reject unsupported sample rates, sample
  formats and layouts rather than silently converting them.
- Define an encoder configuration object with original layout, carrier layout,
  sample rate, quality/profile and frame policy only when each is confirmed by
  the native encoder.
- Add a trace mode that reports host calls, encoder frames, metadata blocks,
  payload bit counts, CRCs and emitted sample range without modifying output.

**Exit gate:** identity carrier mode round-trips exact PCM and produces the
same result regardless of input read chunk size.

### 2. Native analysis / carrier construction

- Port the native codec-v3 analysis and layout-specific mix construction in
  the exact arithmetic order (including fixed-point rounding and saturation).
- Start with one native-proven simple mix2 layout; do not begin with mix3.
- Preserve per-frame and cross-frame state exactly, including delay/history
  buffers and the native host block size.
- Add channel-separation tests: one active source channel at a time must reach
  precisely the carrier channels and analysis residuals established by native
  traces.

**Exit gate:** for the initial layout, analysis/carrier intermediate buffers
match native trace values sample-for-sample.

### 3. Residual preparation and Golomb-Rice encoder

- Port residual prediction/Extrapolate inverse, quantizer decisions and any
  rate-control selection from the encoder path; never infer them from the
  decoder's Golomb-Rice reader.
- Implement a bounds-checked MSB-first bit writer only after confirming the
  codec-v3 reader/writer conventions, word packing and terminal padding.
- Port Golomb-Rice symbol mapping, parameter signalling and payload length
  accounting.  Add isolated golden tests for signed residuals, parameter
  boundaries and incomplete final words.
- Implement native CRC calculation and placement; compare each CRC over the
  same byte/word range as the native trace.

**Exit gate:** native decoder accepts every generated simple-layout frame, and
the project's codec-v3 parser reports exactly the expected frame fields,
payload length and CRC.

### 4. Metadata and PCM embedding

- Port codec-v3 metadata/ADOL serialization, sync generation and metadata-LSB
  insertion.  Keep carrier PCM samples and metadata bit manipulation in one
  explicit routine so 24-bit sign handling is auditable.
- Implement `FormatDetector`-compatible block scheduling in fixed 32-sample
  subblocks and confirm first-sync placement, repeated metadata cadence and
  end-of-stream behavior.
- Compare the generated metadata stream with the decoder scanner and native
  metadata parser; check decoded/original layout, carrier layout and all ADOL
  instructions.

**Exit gate:** native decoder detects generated metadata at the expected sample
offset and reports the exact configured layouts without damage to non-metadata
carrier bits.

### 5. Complete layout support

- Port each native-proven mix2 mapping, then mix3 mappings, one table/branch
  at a time.  Include Top and height channels only after their native branches
  are traced.
- For every new mapping, add impulse and channel-separation vectors plus a
  real PCM vector.  No layout aliases, inferred channel maps or automatic
  upmixes are permitted.
- Port all encoder quality/profile branches that affect a valid native output;
  unsupported values must produce a clear error.

**Exit gate:** every supported layout has a native trace, a generated carrier,
and a decoder round-trip test with documented latency alignment.

### 6. Streaming, finalization and reproducibility

- Make `process()` accept arbitrary host blocks, including boundaries inside a
  codec frame.  The encoded result must be invariant for complete-frame versus
  split input feeds.
- Port encoder drain/final frame semantics.  Trim only samples known to be
  encoder latency/padding by the native contract; retain the original source
  sample count in a sidecar/report where PCM WAV cannot encode it directly.
- Make output deterministic: identical configuration and PCM yield bytewise
  identical carrier WAV bytes.

**Exit gate:** feed patterns `1, 31, 32, 127, 832, 1000, 1024` and randomized
chunk sizes produce identical carrier payload and the same decoded length.

## Regression requirements

Each supported codec-v3 vector must run all of the following:

1. encode PCM to a carrier WAV;
2. parse it with the local codec-v3 metadata scanner and frame parser;
3. decode it with `tools/auro3d-decode` at the normal host block size (832);
4. decode it with the installed native decoder when callable;
5. latency-align only by the measured native codec latency, then compare every
   output channel (exact integer comparison where native arithmetic permits it,
   otherwise a documented bounded error);
6. assert source/output sample counts, CRC acceptance, no PCM rails introduced
   by the encoder, and host-chunk equivalence.

The initial corpus should include synthetic vectors for all implemented
layouts.  Existing decoder corpus files are decoder inputs and must not be
treated as encoder ground truth unless their original discrete PCM and native
encoder configuration are available.

## Completion definition

The first usable release supports only layouts whose complete encoder path is
confirmed by the native codec-v3 implementation.  It writes a valid metadata
PCM carrier, native and local decoders accept it, and its regression suite
proves deterministic streaming and per-channel PCM recovery.  AuroCX remains
entirely untouched.
