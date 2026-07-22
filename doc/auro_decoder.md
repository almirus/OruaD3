# AURO decoder notes

This document describes the current `tools/auro3d-decode` decoder implementation:
how AURO metadata is found in PCM, how layouts are derived, and how missing
channels are reconstructed.

The implementation targets metadata-bearing Auro-Codec streams in PCM carriers.
The AuroCX path is implemented separately for MP4 `a3ds` tracks. The Auro-Matic
XinN synthetic upmixer is intentionally not implemented.

Successful AuroCX decoding writes a channel-mapping XML next to the output WAV.
It records the output and original filenames, decoder type, PCM format, and the
source schema layout (name and mask), plus the schema channel id, name, and
audio-stream index for every emitted channel. Binaural XML retains the original
multichannel source layout while describing a two-channel output.
The `auro_native` channel-mapping XML exposes the same `sourceLayout` and
`sourceLayoutMask` attributes from the metadata-requested output layout; these
also remain multichannel for binaural output.

## Input model

The command line tool reads WAV PCM24 interleaved input and converts it to
planar signed 24-bit samples stored in `int32_t` planes.

The decoder has two layouts:

- `carrier_layout`: the physical PCM channels present in the WAV carrier.
- `decoded_layout`: the logical AURO output layout signaled by metadata.

For 7.1 WAV/FLAC carriers decoded through FFmpeg, physical PCM planes follow
WAVEFORMATEXTENSIBLE order (`BL,BR` before `SL,SR`). The input mapper preserves
that order whenever the WAVE mask matches the signaled AURO carrier mask; using
AURO bit order here swaps LS/RS with LB/RB.

Examples:

- Auro 2D test stream: carrier `2.1`, decoded layout `5.1`.
- Auro 9.1 stream: carrier `5.1`, decoded layout `5.1 + 4H`.
- `7.1_5H1_1T`: the codec-v3 mix3 path reconstructs the complete native
  `7.1 + 5H + T` layout. HC, T, HLS and HRS are codec outputs, not XinN
  expansion.

The output channel mode is printed once after `open()`:

```text
decode_channel_modes carrier_passthrough=... native=... auromatic=...
```

`carrier_passthrough` channels are copied from the input carrier without DSP
gain. `native` channels are reconstructed from Auro-Codec metadata. `auromatic`
marks codec-signaled expansion filled by Auro-Matic/XinN (for example missing
height beyond the native dematrix subset, or 2.1→5.1). Carrier slots that feed
height dematrix are labeled `carrier_dematrix` in the channel-mapping XML and
`--channel-diagram` output: the bed is recovered (and may be silent) while the
paired height is emitted as `native_auro`.

Example for `7.1_5H1_1T` (`--channel-diagram`):

```text
carrier FL + codec -> FL [dematrix bed; may be silent], HL [native AURO]
carrier FR + codec -> FR [dematrix bed; may be silent], HR [native AURO]
carrier C + codec -> C [dematrix bed], HC/T [native AURO]
carrier LS + codec -> LS [dematrix bed], HLS [native AURO]
carrier RS + codec -> RS [dematrix bed], HRS [native AURO]
```

Do not copy raw LS/RS back after mix3. Identification streams confirm that this
would leave the encoded "height surround" announcements in the bed channels.

## Metadata storage

AURO metadata is embedded in ordinary PCM samples. The scanner in
`scan_auro_metadata_pcm24()` walks the PCM payload, searches sync blocks, and
parses ADOL instructions from the detected metadata stream.

The metadata gives:

- decoded/original AURO layout id;
- carrier layout id;
- channel count of decoded and carrier layouts;
- whether the layout uses the three-output `mix3` reconstruction path;
- sync sample offset inside the PCM stream;
- metadata block size;
- ADOL instruction list used for diagnostics.

Important fields are stored in `AuroMetadataInfo`:

- `layout_id`: decoded AURO layout mask/id.
- `carrier_layout_id`: physical carrier mask/id.
- `output_channels`: decoded channel count.
- `carrier_channels`: carrier channel count.
- `uses_mix3`: true for layouts that use three-output reconstruction.
- `sync_sample`: first detected metadata sync sample.

The metadata scanner is only used to choose layout and boot parameters. The
actual channel payload is decoded later by the codec-v3 parser/output generator.
At end of input the decoder feeds zero host blocks through the pipeline for its
configured stage-1 latency, removes the initial latency, and retains exactly the
number of PCM frames reported by the source container. No input frames are
discarded and no final partial block is treated as a complete codec frame.

Legacy XinN accepts 32/44.1/48 kHz. A 96 kHz decoded PCM input is converted to
48 kHz at the 1fs boundary before XinN, matching the resampler position in the
native A3DENG plan. Exported WAVE/temporary-FLAC input carries a speaker mask
derived from the actual output slots, so LS/RS are not reinterpreted as LB/RB.
FLAC itself is limited to eight channels; 5.1.4 and larger exports use WAV.

## Layout selection

`Decoder::open()` scans metadata before allocating codec state.

The input channel slot map is chosen from the WAV channel count plus metadata:

- if metadata has a valid carrier layout matching the WAV channel count, that
  carrier layout is used;
- otherwise the decoder falls back to the WAV/native channel mask.

The per-call input mask may be a subset of the configured carrier layout
(`auro_codec_v3_Decoder_process`, `0x52AD60`). A completely zero padding plane
is omitted from that mask, while a metadata-only plane is retained.
`SyncDetector_process_block` (`0x52C680`) combines the low bits of every plane
present in the mask, so including a zero padding plane prevents the common
16-bit sync preamble from locking. Every bit that remains in the per-call mask
still has a valid non-null PCM pointer, as required by `Decoder_process`.

The requested output layout is chosen from the decoded AURO layout. For the
special `7.1_5H1_1T` stream the current native path selects the closest direct
native subset instead of invoking the excluded Auro-Matic/XinN expansion.

## Decode pipeline

Each `decode_next()` call processes one block:

1. Read one interleaved PCM block from the WAV.
2. Convert it to planar input buffers.
3. Run the codec-v3 partial pipeline:
   - `DelayLine_write`
   - format/sync detector
   - parser stage
   - output generator stage
   - `DelayLine_advance`
4. Copy carrier/bed channels directly to output planes.
5. Write requested output planes to interleaved WAV.

The codec-v3 state is split into these main pieces:

- `FormatDetector`: finds lock/unlock and frame boundaries.
- `SyncDetector`: detects per-channel sync and creates frame descriptors.
- `Parser`: parses channel frame metadata and bitstream words.
- `DelayLine`: keeps carrier PCM blocks needed by prediction.
- `OutputGenerator`: builds timeline segments and reconstructs output channels.

## Frame and segment timing

Metadata sync can start inside a PCM block. For example:

- Auro 2D test: `sync_sample = 0`, `metadata_block = 1024`.
- `7.1_5H1_1T`: `sync_sample = 256`.
- DTS-HD carriers (e.g. Amplitude16): `metadata_block = 1000`,
  `sync_sample = 1024`.

Host block size, embedded codec frame size, and initial sync position are three
independent quantities. The codec frame size is read by
`SyncDetector_process_block` from the carrier (`0x3d0` means 1000); it is not
derived from sample rate or container type. `Config_initialize` (`0x52D7B0`)
requires the host block to be divisible by 32, and `FormatDetector_process`
(`0x52D060`) feeds the sync detector in fixed 32-sample chunks.

The file decoder chooses `lcm(metadata_block, 32)` as its internal host block:
1024-sample codec frames use 1024 and 1000-sample codec frames use 4000. If the
first sync is not already on a host boundary, zero input is prefixed so that it
is; this prefix is removed together with pipeline latency on export. Thus every
embedded codec frame is processed whole without dropping or shifting source
samples. This scheduling is independent of sample rate and container type.

The three consumers keep independent absolute cursors initialized exactly as in
the constructors (the missing x86 call arguments are visible in the ARM build):

```text
FormatDetector = DelayLine_stream_index(delay, 0)
Parser         = DelayLine_stream_index(delay, stage0)
OutputGenerator= DelayLine_stream_index(delay, stage1)
```

Every host call follows `Decoder_process` (`0x52AD60`) literally:
`DelayLine_write_buffer`, format detector, parser, output generator, then
`DelayLine_advance`. There is no fractional frame scheduler or sync pre-skip.
`calculate_latency` (`0x531200`) reports `host_block_size * stage1`; file output
drains that latency, removes the alignment prefix, and trims back to the exact
source frame count.

The selected host size keeps an embedded frame inside one delay-line block.
`DelayLine_get_buffer()` and `Buffer_get_channel()` can therefore use the native
ring-slot pointer and offset directly when passing carrier samples to Extrapolate.

## Channel reconstruction

Frame channel descriptors contain:

- output channel id;
- prediction source channel ids;
- Extrapolate mode;
- quantization shift;
- scale indexes;
- Golomb-Rice packed flags;
- Golomb-Rice bit width and base index;
- seed values for prediction.

For each active frame channel, OutputGenerator runs:

```text
GolombRice_initialize(frame_channel_words, frame_channel_ctx)
Extrapolate_initialize(frame_channel)
GolombRice_get_errors(errors)
Extrapolate_process(delay_line_source, errors, dst0, dst1, dst2)
```

### Golomb-Rice

`GolombRice_get_errors` expands compact residual codes into signed error values.
The implementation follows the decompiled libauro logic:

- unary prefix plus `k` extra bits gives a code index;
- the code index selects packed values from the frame channel context table;
- values are converted from sign-magnitude representation;
- `mode == 3` emits two error values per sample.

### Extrapolate mode 1

Mode 1 is a single-output path. It masks the quantized source sample, scales it,
and writes one destination channel.

### Extrapolate mode 2

Mode 2 reconstructs two channels from one carrier source plus one residual
stream. It keeps a small predictor state across segments, alternates predicted
and residual destinations, applies per-output scale, shifts back to PCM24 range,
and clamps to 24-bit signed PCM.

### Extrapolate mode 3

Mode 3 reconstructs three channels from one carrier source plus two residual
streams. This path is used by 2D expansion and some height layouts. It keeps a
three-phase predictor state:

- first samples are seeded from frame metadata;
- subsequent samples use `4 * last - 3 * previous` prediction;
- residual pairs are added to two of the three outputs depending on phase;
- all three outputs are scaled, shifted back to PCM24 range, and clamped.

## Output channel classes

Current output channels fall into three practical classes.

### Carrier passthrough

If the requested output slot exists in the input carrier mask, it is copied from
input to output after codec processing. This protects bed channels from being
overwritten by partial reconstruction state.

Examples:

- `auro.wav`: `FL,FR,C,LFE,LS,RS`.
- `auro_2d.wav`: `FL,FR,LFE`.
- `7.1_5H1_1T.wav`: `FL,FR,C,LFE,LS,RS,LB,RB`.

### Native decoded

Native decoded channels are present in metadata and reconstructed by
GolombRice/Extrapolate. Height channels in `5.1+4H` are native decoded.

### Auro 2D/Auro-Matic-labeled expansion

For Auro 2D carrier-to-surround streams, metadata can request a decoded 2D bed
layout larger than the carrier. The current code labels these generated channels
as `auromatic` in the log, but they are still reconstructed from codec-v3
metadata and the Extrapolate path. The excluded XinN synthetic upmixer is not
used.

Example for `auro_2d.wav`:

```text
carrier_passthrough=ch0(FL),ch1(FR),ch3(LFE)
auromatic=ch2(C),ch4(LS),ch5(RS)
```

### Output headroom

`--dsp-headroom-db` defaults to `0`. An explicitly requested value is converted
to `10^(-dB/20)` and applied to every output channel, including carrier bed,
decoded bed, LFE, and height channels. DSP strength remains limited to
synthesized/non-carrier channels. AuroCX uses the same global output-headroom
rule.

### Binaural output

`--binaural` is available for both classic AURO and AuroCX. AuroCX feeds each
decoded access unit directly into the embedded AHP/HRTF convolution renderer;
FFT overlap-add state is retained across AU boundaries, so no intermediate
multichannel WAV or whole-file PCM buffer is required. The output is stereo
PCM24 and its XML contains two `binaural_renderer` channels.

## Probe (`--probe`)

`--probe -i <file>` prints diagnostics without writing PCM (`-o` not required).
Dispatch matches decode: MP4 with an `a3ds` sample entry uses the AuroCX
container/schema probe; otherwise the classic/native path opens the file and
prints the same open-time layout/metadata lines as `-v`.

### AuroCX MP4 probe

The AuroCX path is separate from the PCM/WAV codec-v3 decoder. The probe reads
an MP4 `a3ds` track, parses `acxd`, reconstructs access-unit offsets from
`stsc`/`stsz`/`stco`/`co64`, validates the `A3 DC 0D ED` sync prefix, and
decodes XOR/VLQ blob segment `1` as the schema block.
The `audio_coding` probe line classifies AWC PDUs from their policy flag as
`lossless`, `transparent_near_lossless`, or `mixed`; individual AWC PDU lines
also print the selected coding policy.
The AuroCX channel-mapping XML records the same classification in the root
`audioCoding` attribute.

### AWC audio coding

AuroCX does not use a single generic codec such as FLAC or AAC. Its full-band
audio PDUs use AWC with a shared predictive coding structure:

1. each audio frame is divided into one or more subblocks;
2. each stream is reconstructed from an LPC prediction and a residual vector;
3. the residual is carried by LDC entropy syntax: raw, unary, common-Golomb,
   or run-length/alternating variants;
4. optional ICC couples streams inside a subblock.

Lossless AWC keeps this process integer-only. LPC coefficients are selected
from the 64-entry coefficient table, residuals are decoded exactly, and ICC
uses the fixed-point ICP/Q23 gain table. In this policy the decoded PCM is
intended to be bit-exact to the coded source.

Transparent/near-lossless AWC retains the same LPC and LDC residual syntax,
but carries an ErrorScaler for nonzero residual vectors. Reconstruction adds
`residual * error_scale` to the LPC prediction, so the residual has a controlled
quantization step and output is not bit-exact. Its inter-channel transform is
a separate floating-point PCA/ICC path with 32 coefficient pairs, not the
lossless ICP/Q23 path.

LFE is not a normal full-band AWC stream: it has a separate residual syntax and
persistent multistage interpolation for its coded sample-rate factor. The LFE
filter state crosses access-unit boundaries; AWC LPC history is reset for each
AWC frame and is retained only between subblocks of that frame.

### Confirmed schema syntax (segment 1)

- **ConfigHeader** starts with unary codec version/profile, 3-bit sample-rate id
  (20 extra bits when id=7), block-size id, audio-stream unary bit width, and a
  `common` flag. Markus AU1/AU2 use `common=0` with explicit counts (`beds=1`);
  when `common=1`, counts are inherited from `SchemaDecodeState` (requires a valid
  prior AU).
- **Program optional tail** (loudness/DRC/optionals #8/#9) is parsed only on initial
  AUs; delta AUs skip it. `Loudness_t` is three 3/2/2-bit fields plus up to three
  optional `LoudnessDataSet` blocks (9× optional 11-bit levels each). DRC uses
  VLQ(8) byte count + 8 bits per element.
- **ConfigData** follows with program, bed, object, switch, and extension
  metadata. `Audio.pdus` begins at the exact cursor produced by that traversal;
  the decoder does not scan for the PDU vector or payload starts.
- **Bed/Object mappings** retain all bed channel layers, ambisonics component
  streams, object-group streams, and switch-group references. SASC channel
  counts are derived from the selected schema element and linked object groups.
- **PDU vector** count uses unary quotient + `audio_stream_bits` remainder
  (matches decompiled golomb_rice with parameter = stream bit width).
- **ConfigData extensions** follow programs, beds, objects, and switches. Each
  `auro_cx_user_Extension_t` starts with two base-8 VLQs carried in 4-bit
  continuation chunks, followed by a VLQ(8) payload byte count and exactly that
  many payload bytes. This accounts for the observed 40- and 48-bit additions
  before `Audio.pdus`.
- **Initial AWC payload config** (inside schema PDU payload, not frame body):
  lossless AWC uses `9 + 4*stream_count` bits (common preamble often `10`, per-
  stream parameter often `2`); transparent/near-lossless uses
  `8 + 12*stream_count` bits (preamble often `5`).
- **AWC lossless LPC table indices** (`parse_` ~0x443340): per stream/subblock,
  a `custom_angles` flag selects between reading `6*lpc_order` table-index bits
  from the bitstream vs using default index `32` for every coefficient (decompile
  fills `0x20` without reading). Indices are not read a second time after
  residuals. After each lossless AWC frame body, padding bits to the next byte
  boundary are consumed before the next PDU frame in the delta blob.
- **AWC transparent ErrorScaler** (`parse_` ~0x448000): a set residual flag is
  followed by the LDC vector, optional extended-order precision bit, and 8-bit
  error scale factor. A zero residual flag zero-fills the vector and jumps to
  the next stream/subblock without consuming either ErrorScaler field.
- **Partition granule lengths** use `index_bits(total-1)`. LDC custom maximum
  uses `index_bits(total-2)`, while partitioned subelement count/property-count
  fields use `index_bits(property_count-2)`. These are decompiled
  `32 - (BSR(value) ^ 31)` expressions, not `32 - index_bits(value)`.
  AWC lossless `parse_bitdepth_mode` width is
  `index_bits(error_scale_byte - 1)` (~0x4497F0), not `32 - index_bits(...)`.
- LDC alternating golomb+unary (~0x4539D0 case 2+1): 2-bit unary skip, then
  `pop_all_ones` + `(golomb+1)`-bit value as `(mask|unary<<g)+1`.
- LDC alternating fixed+golomb (case 0+2) and golomb+fixed (case 2+0) use
  different index/marker order than the old `golomb_sparse` helper.
- After `pop_all_ones`, lossless LDC/common-Golomb reads consume one unary zero
  terminator followed by exactly `golomb` remainder bits. The native reader
  advances by `golomb+1` total bits and extracts from `bitpos+1`.
- Native `Reader::pop_all_ones` (~0x338E30) counts consecutive one bits and
  leaves the terminating zero for that following read.
- Lossless AWC `parse_` reads subblock ICC before per-subblock error-scale
  values from the bitstream (mode 2); error-scale context fill for mode 0 is
  non-bitstream.
- Lossless `Policy::icc_decode_subblock` (`0x449640`): ICP gain from
  `icp::Gains[angle]`; bitdepth downshift uses toward-zero
  `(gain + (((1<<n)-1) & (gain>>63))) >> n`. For `count>=12` (non-overlapping)
  the SIMD path applies rounded Q23 (`bias 0x7FFFFF` then `>>23`); the short
  or overlapping scalar path uses truncating `/ 0x800000`. Transparent ICC is
  a separate float PCA path (`0x4560xx`), not ICP Q23.
- Partitioned LDC subelement sizes consume granule properties with a moving
  cursor. Each encoded property count sums the next disjoint range; it is not a
  prefix length relative to the start of the property vector.
- Differential LDC granule properties split the first granule whenever its
  value is greater than one: `[first, tail...]` becomes
  `[1, first-1, tail...]`. A first value of zero or one leaves the full vector
  unchanged.
- Alternating LDC with an entropy-coded marker and zero-width fixed index is a
  dense form: it decodes one marker for every output element, including the
  final element.
- When both alternating LDC fields are common-Golomb families, the second field
  encodes the index jump and the first encodes the marker. Type 2 and type 3 are
  selected independently for those two reads.
- An LDC custom maximum limits the property vector and entropy-coded prefix.
  Elements after that prefix are zero-filled by the decoder; they do not carry
  additional subelement syntax.
- `Params::read` (~0x445060) first reads an active flag. A clear flag yields an
  all-zero vector. If active, the next flag controls only the optional maximum;
  the granule partition and per-subelement parameters follow in both branches.
- Entropy parameters are read once by `run_length_dispatch::Params::read`
  (~0x445B70). Dense type-2/type-3 decoding reuses the saved Golomb parameter
  instead of consuming another four bits.

### Access unit 2+ (delta schema)

Confirmed on all seven corpus MP4s: ConfigHeader may inherit stored counts when
`common=1`, but ConfigData and `Audio.pdus` retain their normal field order.
After consuming ConfigData extensions, the PDU count is read as the native
Golomb-Rice value, followed by every PDU header, payload length, and payload.
The decoder does not replay a prior schema or cached residual payload.

Probe reports `schema_bits`, `consumed_bits`, and `post_schema_bits`
(remaining blob bits after schema parse). On delta frames, `consumed_bits`
includes skipped PDU payload bits; AuroCX decode must start AWC/LFE decode at
each PDU's `payload_bit_offset` (recorded during schema parse), not at
`consumed_bits`.

Known declared layouts from `acxd`: `0x01bf` (7.1), `0x663f` (5.1+4H),
`0x7fbf` (7.1+5H+T). Some lossless files carry `0x8060`; their explicit schema
channel list is authoritative over the declared mask name.

## Current limitations

- AuroCX audio rendering is wired end to end through auto path selection
  (`-i`/`-o`: MP4 `a3ds` → CX, else classic native), including
  schema-bed mapping, SASC application, and PCM24 WAV output. A fresh build
  completes all eight `test_files/aurocx` MP4s. Yamamoto near-lossless Top
  (bit 12) level-0 routing matches native height-tail order before `label101`;
  Alessandro applies schema `channel.downmix` via `Downmixer::append_` /
  `set_layout_independent_gains_` (`0x434EA0`) and 2d→1d table overrides.
  Height beds with custom `gain0` follow `add_3d_to_2d_src_gains_`
  (`0x435520`) / `set_3d_to_2d_src_gains_` (`0x435230`) and the `+920`
  destination post-scale when `calculate_` sets `+3576` (confirmed on
  `auro_cx.mp4`, which has no `acxd` box). Nested `minf/hdlr` (`url `) must
  not clear `mdia/hdlr=soun` during MP4 track walk. Native PCM oracle is
  HDMI-7.1 Float32 (device prune ≤8ch): Frida harness
  `oracle/oracle_acx_pcm_job.js` (+ CLI `oracle_acx_pcm_frida.js`). Push
  needs `allocateDirect`; pop part `832*8*4`. With
  `set_channels_backs_before_surrounds(true)` order is FL,FR,C,LFE,LB,RB,
  LS,RS (`kHdmiOrder_1db2b0`). Our WAVE-sorted 12ch bed for `auro_cx`
  (`0x67BF`) matches that for the first 8 planes — compare with
  `regression/compare_oracle_f32_hdmi8.py`. On AUs 0..260, lag **+2816**
  (oracle delayed), FL corr≈0.9947; other HDMI channels near-silent in
  both (multichannel onset ≈AU 261). **12ch discrete oracle (AC 1.26.36 /
  libauro `4.0.14-9d106532` arm64) — blocked:** forcing
  `AuroUpdateParams.output_layout=0x67BF` reports `outCh=12`, but native
  `Decoder::configure` returns
  `AURO_A3DENG_V4_RC_INVALID_OUTPUT_LAYOUT` (255) for HDMI/Soundbar when
  the channel layout actually contains height (`sub_31ACE0` with
  `hdmi_mapping=0` → size 12). With `hdmi_mapping=1` the layout is built
  as 8 bed channels only, configure succeeds, and `AuroPop` emits
  `26624=832×8×4`. Forcing configure RC→0 does not allocate 12ch output.
  Harness: `oracle/oracle_acx_pcm_12ch_job.js`. Full 12ch discrete
  equality still open. PCM24 output is written one AU at a time.
  The `--cx-only` regression checks channel count, 48 kHz/PCM24 geometry,
  nonzero frames, and at least one nonzero PCM byte.
- LFE payload parsing follows `Processor::parse_` at `0x463A90`. Residuals are
  converted through the native minimum-phase `multirate::interpolation` cascade
  for factors 40/80/160/320/640 with filter state retained across access units.
  LFE, lossless AWC, and transparent AWC payloads must be consumed exactly.
- SASC channel-bed `Planner::compute_` (`0x49D840`) calls `sub_49EB20`
  (`0x49EB20`) → `Downmixer::compute_plan` (`0x436890`) for SCG levels 2/1/0.
  Level 2 is identity (coefficients already unity). Levels 1/0 use
  `auro_downmix_v1` rules, but acceptance follows the Downmixer residual mask
  (sequential apply with deferred source clear; must equal the target), not
  `plan[159]` equality — that engine progress mask is often a strict superset
  (confirmed on Lori `0x663f`→`0x3f`). Custom `channel.downmix` overrides the
  default 111-slot gain table through `append_` / `set_layout_independent_gains_`
  (`0x434EA0`); ChannelDownmix +152 is `intra_layer_gains` (fixed count from
  `qword_1E4EA0[id-4]`), not MonoTopDownmix. On 2d→1d layers, +128 gains also
  override 2d→1d source slots (`add_2d_to_1d_src_gains_` / calculate_gains_
  LABEL_15) and FL/FR destination post-scales at +1416.   On 3d→2d/1d layers
  (`calculate_` `+3576`), height `gain0` uses mask `257536` →
  `add_3d_to_2d_src_gains_` (`0x435520`) / `set_3d_to_2d_src_gains_`
  (`0x435230`) and mask `202113527` → `+920` destination post-scale.
  Height `gain1` (`append_` `a2[45]`) feeds mask `786932` → `+2408` and
  FL/FR `+1416`; without height those slots still come from `gain0`.
  Post-scale in `calculate_gains_` (`+3576`/`+3577`) uses rounded Q23
  (`product + (product<0 ? 0x7FFFFF : 0)) >> 23`); relative schema gains
  from `compensate_channel_gains` (`0x4349A0`) use truncating `/ 0x800000`.
  Plan step order matches native: post-scale first, then relative.
  `auro_downmix_v1_Engine_calculate` (`0x58EF80`) `tgt_class_hint` /
  `v200` is `(tgt_lo!=3) & ((uint8_t)tgt >> 2)` (low byte only). Layouts
  that miss that gate return false — there is no separate non-height success
  path.
  `set_mono_top_` (`0x435580`) uses `details::inv_sqrt` (`0x433C20`,
  `qword_1DB190` = Q23 `1/sqrt(1..4)`) into gain slots 16–31.
  `set_stereo_top_` (`0x435C50`) reads `StereoTopDownmix_t` for channels 28/29
  only (ChannelDownmix decode `0x41A6B0` mask `805306368`): optional blocks at
  `+80` (types #3) and `+104` (type #4). Kind 0 → 2 IntegralGains; kind 2|3 →
  1 gain. Gains land in slots 0–13 (`gains[i]` ↔ Downmixer `+(32+8*i)`); channel
  29 selects the odd twin of each even slot. Channel 12 (`T`) is MonoTop only —
  do not parse 28/29 as MonoTop (wrong gain counts). No corpus file currently
  exercises present mono/stereo-top fields (`downmix=0` on T beds; no ch 28/29).
  Apply filter is independent of PDU `source_layer`: `scg::details::decode`
  (`0x495460`) cancels when `step.layer < SCG+448`; full discrete WAV sets
  that layer to **2** so height (planned as layer 1) is dematrixed out of the
  bed. PDU `source_layer` only gates which schema channels feed the planner.
  Header+60 is a `calculate_mode` query flag, not a per-frame apply flag;
  `Processor::run_` (`0x492AA0`) keeps decoding the persistent SCG context on
  later AUs, including AUs whose `config_flag` does not rebuild the planner.
  For `0x67BF`→`0x1BF`→`0x3`, layer 0 also emits LFE→FL/FR
  (`auro_downmix_v1_plan_add_center_routes`, gain indices 104/105, default
  −3 dB). SCG layer 2 cancels those edges; listening `FL≈−LFE/√2` on the
  `auro_cx` LFE sweep matches cancel-on-quiet-carriers, not a missing native
  mute. Do not invent FL/FR silence by dropping LFE layer-0 steps.
  Linked-object branch in `Planner::compute_` (`0x49D840`): after level-2
  `sub_49EB20`, native walks `linked_object_group_idxs`, applies
  `get_object_group_ref_gain` relative to bed ref, then
  `ObjectRenderer::compute_panning_gains` (`0x4BF010` / ESPCAP
  RoomCentricPanner) and emits layer-2 mix steps into bed streams before
  levels 1/0. `get_layouts(linked!=0)` masks the bed with `0xFFEFFFF7`.
  Corpus MP4s have `object_groups=0`; planner validates metadata and passes
  the linked layout flag, but hard-errors only at the unported ESPCAP call.
  Pure object-group SASC (`header_value=0`) with `config_flag` is not
  `Planner::compute_` (needs a bed) — separate hard-error.
- Auro-Matic XinN synthetic upmix is not implemented.
- Full `7.1 + 5H + T` expansion is not implemented; current output uses the
  native direct subset.
- The decoder expects PCM24 WAV input for normal operation.

## Key source files

- `tools/auro3d-decode/main.cpp`: command line, logging, output mode report.
- `tools/auro3d-decode/auro3d_decoder.cpp`: WAV scan, layout selection, block
  decode orchestration, passthrough handling.
- `tools/auro3d-decode/auro3d_decoder.hpp`: public decoder state and metadata
  structures.
- `tools/auro3d-decode/auro3deng_from_ida.cpp`: codec-v3 partial port,
  DelayLine, Parser, OutputGenerator, GolombRice, Extrapolate.
- `tools/auro3d-decode/auro3deng_from_ida.hpp`: codec-v3 runtime structures and
  callback interfaces.
