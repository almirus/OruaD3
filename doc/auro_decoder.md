# AURO decoder notes

This document describes the current `tools/auro3d-decode` decoder implementation:
how AURO metadata is found in PCM, how layouts are derived, and how missing
channels are reconstructed.

The implementation targets metadata-bearing Auro-Codec streams in PCM carriers.
The AuroCX path and the Auro-Matic XinN synthetic upmixer are intentionally not
implemented.

## Input model

The command line tool reads WAV PCM24 interleaved input and converts it to
planar signed 24-bit samples stored in `int32_t` planes.

The decoder has two layouts:

- `carrier_layout`: the physical PCM channels present in the WAV carrier.
- `decoded_layout`: the logical AURO output layout signaled by metadata.

Examples:

- Auro 2D test stream: carrier `2.1`, decoded layout `5.1`.
- Auro 9.1 stream: carrier `5.1`, decoded layout `5.1 + 4H`.
- `7.1_5H1_1T`: metadata says `7.1 + 5H + T`, but current native export keeps
  the direct native subset (`7.1 + 2H`) because the remaining expansion depends
  on the excluded Auro-Matic/XinN branch.

The output channel mode is printed once after `open()`:

```text
decode_channel_modes carrier_passthrough=... native=... auromatic=...
```

`carrier_passthrough` channels are copied from the input carrier without DSP
gain. `native` channels are reconstructed from Auro-Codec metadata. `auromatic`
marks codec-signaled non-height 2D expansion, for example 2.1 carrier to 5.1.

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

## Layout selection

`Decoder::open()` scans metadata before allocating codec state.

The input channel slot map is chosen from the WAV channel count plus metadata:

- if metadata has a valid carrier layout matching the WAV channel count, that
  carrier layout is used;
- otherwise the decoder falls back to the WAV/native channel mask.

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

- Auro 2D test: `sync_sample = 512` with block size `1024`.
- `7.1_5H1_1T`: `sync_sample = 256`.

The output generator cannot simply decode at block-aligned timeline positions.
It must line up its timeline with codec frame starts. Current code derives an
output timeline delay from `sync_sample`:

```text
sync_offset = sync_sample % block_size
delay = sync_offset ? (2 * block_size - sync_offset) : block_size
og_cursor = parser_cursor - delay
```

This makes output segments match full codec frames instead of decoding only the
tail of each frame.

The `DelayLine_get_buffer()` result also returns an internal offset into the
selected ring slot. When the requested source range crosses a ring-slot boundary,
the decoder builds a contiguous temporary window before passing samples into
Extrapolate. Without this, generated channels read samples from the wrong memory
area and become wideband noise.

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

## Current limitations

- AuroCX audio rendering is not implemented. `--probe-cx` detects an MP4
  `a3ds` track, parses its `acxd` descriptor and MP4 sample tables, validates
  the `A3 DC 0D ED` access-unit sync, decodes blob framing, and reports the
  schema header counts and declared layout. The probe also decodes the first
  program-to-bed reference, channel IDs, channel-to-audio-stream mapping, and
  PDU types plus AWC/LFE audio-stream ranges. Both explicit channel-descriptor
  beds and compact beds that rely on the `acxd` declared layout are recognized;
  the latter currently uses structural PDU-vector recovery. Known layouts
  include 7.1 (`0x01bf`), 5.1+4H (`0x663f`), and 7.1+5H+T (`0x7fbf`).
  MP4 access-unit offsets are reconstructed from `stsc`, `stsz`, and
  `stco`/`co64` rather than treating chunk offsets as packet offsets. The probe
  also reports an experimental parse of the second access unit, which exposes
  the transition from static schema configuration to dynamic AWC frame data.
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
