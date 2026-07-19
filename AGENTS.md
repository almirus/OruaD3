# Orua D3: remaining decoder work

## Objective

Finish `tools/auro3d-decode` strictly from `decopiled/libauro.so.c`.
Do not hide decoder defects with filename, container, sample-rate, layout, gain,
clipping, interpolation, or frame-alignment heuristics.

Only the unfinished work is listed below.

## Priority 1: fix codec-v3 frames split across host calls

The current codec-v3 port is still incorrect when one embedded codec frame
spans multiple host blocks.

Current evidence:

- `test_files/correlation/auro2d.flac` has `metadata_block=1024` and produces
  silence, clicks, and long full-scale plateaus with host block 832; host block
  1024 is clean.
- `test_files/Amplitude16 - Auro-3D - 2.35.mkv` has
  `metadata_block=1000`, `sync_sample=1024`, and clips/clicks when its codec
  frames cross host-call boundaries.
- The parsed `ParseResult` remains unchanged across a split frame.
- Golomb-Rice output decoded as one frame or as consecutive pieces is identical.
- Divergence starts in continued Extrapolate reconstruction, before PCM export
  gain and after valid, bounded carrier/error input.

The current code contains a temporary generic workaround in
`tools/auro3d-decode/src/app/decoder.cpp`:

- automatic host size `lcm(metadata_block, 32)`;
- zero prefix used to align the first codec sync to a host boundary;
- export trimming that removes this prefix.

This is not the native behavior. Native A3DENG uses host block 832 and supports
1000/1024-sample codec frames crossing calls. Remove this workaround after the
real state-continuity bug is fixed.

Port and compare these paths instruction-for-instruction:

- `Decoder_process`, IDA `0x52AD60`: write delay line, format detector, parser,
  output generator, then advance;
- `Config_initialize`, IDA `0x52D7B0`;
- `FormatDetector_process`, IDA `0x52D060`, fixed 32-sample subblocks;
- `Extrapolate_initialize`, IDA `0x52DEF0`;
- `Extrapolate_process`, IDA `0x52DF10`;
- mix2 reconstruction near `0x52E1C0`;
- mix3 reconstruction, IDA `0x52E5C0`;
- OutputGenerator frame initialization, segment routing, GR/Extrapolate state
  selection, and frame-pop order around the calls above;
- DelayLine buffer/offset selection and `Buffer_get_channel` source addressing.

Add a deterministic split-equivalence test: initialize one parsed frame once,
decode its carrier/errors as a complete frame, then decode the same data in two
or more pieces. Reconstructed PCM and final GR/Extrapolate state must be exactly
equal for modes 2 and 3.

Acceptance criteria:

- restore the normal default host block 832;
- remove the LCM/alignment-prefix workaround;
- `auro2d.flac` at block 832 is sample-equivalent to the clean block-1024
  decode after latency alignment;
- Amplitude16 decodes at block 832 without clicks, full-scale plateaus, or
  clipping;
- decoded files contain exactly the source sample count;
- no PCM deglitching, rail replacement, forced attenuation, or metadata-LSB
  damage is used.

## Priority 2: validate AuroCX PCM against the native decoder

Bitstream consumption alone is not sufficient. Validate reconstructed PCM in
the native HDMI-7.1 Float32 domain with:

```text
tools/auro3d-decode/regression/compare_oracle_f32_hdmi8.py
```

Remaining work:

1. Capture native output beyond the multichannel onset. The P80X ANR limit
   stops long continuous Frida runs around AU32; use the emulator or another
   non-ANR host.
2. Compare all eight HDMI channels, not only the early mostly-silent prefix.
3. Locate every remaining PCM mismatch in AWC, ICC, LFE, SASC, routing, or
   latency before changing code.
4. Obtain a true discrete-height/planar oracle if possible. The current Android
   API rejects 12-channel output mask `0x67BF`; do not treat an 8-channel HDMI
   dump as proof of full 12/14-channel equality.
5. Add exact or tolerance-bounded PCM checks to automated regression.

Existing oracle facts that must be preserved:

- native HDMI order is `FL,FR,C,LFE,LB,RB,LS,RS`;
- `auro_cx.mp4` AUs 0..260 currently align at oracle lag `+2816` samples;
- early FL correlation is about `0.995`, while the other channels are mostly
  silent in that captured interval;
- AWC LPC state is frame-local; only LFE interpolation state crosses AUs.

## Priority 3: finish unexercised AuroCX branches

1. Port ESPCAP `ObjectRenderer::compute_panning_gains`, IDA `0x4BF010`, for
   linked-object SASC steps. Do not invent object panning. Current corpus files
   have `object_groups=0`, so add a real or synthetic exercising case.
2. Exercise `set_mono_top_` (`0x435580`) and `set_stereo_top_` (`0x435C50`)
   with valid present top blocks. Current corpus does not contain schema
   channels 28/29 or a present mono/stereo-top block.
3. Add deterministic schema/PDU cursor traces beyond AU2. Delta AUs reuse
   stored counts and PDU templates and must never fall back to scanning.
4. Add channel-separation invariants for bed, height, Top, LFE, and SASC cancel
   paths. Correlated channels alone are not proof of correct routing.
5. Verify lossless and near-lossless AWC/ICC output against the native PCM
   oracle, including short/overlap and long SIMD rounding paths.

## AuroCX invariants that must not regress

- Bit order within bytes is little-endian; `Bits::get` reads bit 0 first.
- PDU sizes, MP4 offsets, and bit lengths use checked 64-bit arithmetic.
- Initial and delta schema state persists across AUs.
- Every selected AWC, LFE, and SASC PDU is consumed exactly.
- Lossless and transparent/near-lossless policies remain separate.
- Lossless ICC mix at `0x449640` uses rounded Q23 for `count>=12` and
  truncating division for the short/overlap path.
- SASC playback SCG layer is 2 for full discrete bed+height output; layer-0
  stereo fold and layer-1 height cancels are both applied.
- Layer-0 includes native LFE-to-FL/FR routes at gain indices 104/105. Do not
  mute or skip them to force a preferred listening result.
- Schema channel 12 (`T`) is mono-top; stereo-top uses channels 28/29.
- Do not hardcode corpus filenames, PDU group sizes, or layouts.

## Corpus required for regression

Native codec-v3:

- `test_files/correlation/auro2d.flac`
- `test_files/Amplitude16 - Auro-3D - 2.35.mkv`
- `test_files/mixed files (auro and multichannel)/01. Pastoral Awakenings (2.1 24_96).flac`
- `test_files/mixed files (auro and multichannel)/02. Fast Channel Identification Test - Auro13.1 (48 kHz - audio only) (7.1 24_48).flac`

AuroCX: all eight MP4 files in `test_files/aurocx/`, covering lossless,
near-lossless, LFE, AWC, compact beds, custom gains, and SASC.

## Build and verification

```powershell
cd "C:\Users\USER\IdeaProjects\Orua D3\tools\auro3d-decode"
cmd /c build.bat
```

Treat `RC1109` for `xinn_presets.res` as a failed build even if an older EXE
exists. The binary is:

```text
C:\Users\USER\IdeaProjects\Orua D3\bin\Release\auro3d-decode.exe
```

After every decoder edit:

1. build the executable;
2. decode both codec-v3 failure files above with the default block;
3. inspect per-channel peak, RMS, peak-count, duration, and sample count;
4. run the complete AuroCX corpus for exact PDU consumption;
5. run the available native-oracle PCM comparison;
6. run `git diff --check` for every touched decoder file, `AGENTS.md`, and
   `doc/auro_decoder.md`.

Do not claim a fix from `dsp_clipped_samples=0`: OutputGenerator can clamp to
PCM24 rails before that counter sees the samples. Check the written PCM itself.

## Repository rules

- The worktree is intentionally dirty and contains user files and unrelated
  changes. Preserve them.
- Do not reset, checkout, delete, mass-format, stage, or commit unless the user
  explicitly requests it.
- Use small reviewable patches.
- Follow `decopiled/libauro.so.c`; do not guess DSP behavior.
- If static analysis leaves two plausible interpretations, use the installed
  native library as an oracle without modifying account data or downloading
  media.
- Update `doc/auro_decoder.md` only when a branch is confirmed by decompilation
  or native behavior.
