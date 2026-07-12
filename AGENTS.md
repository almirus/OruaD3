# Orua D3 / AuroCX reverse-engineering guide

## Goal

Implement the AuroCX subtype decoder in `tools/auro3d-decode` from the
decompiled `decopiled/libauro.so.c`. Work incrementally: first make the bitstream
syntax observable and testable, then port reconstruction stages. Do not guess
PCM algorithms when the corresponding decompiled path has not been identified.

The immediate target is:

1. correctly decode dynamic `ConfigHeader` / `ConfigData` in access units after
   the initial schema block;
2. expose the real AWC frame payload and subblock boundaries;
3. port AWC lossless first, then near-lossless/transparent;
4. emit per-stream PCM and finally map it to the schema bed.

## Repository state and ownership

- The worktree is intentionally dirty and contains unrelated user changes.
- Preserve all existing changes. Do not reset, checkout, delete, mass-format,
  stage, or commit unless explicitly requested.
- New AuroCX work currently lives in:
  - `tools/auro3d-decode/src/app/cx_probe.hpp`
  - `tools/auro3d-decode/src/app/cx_probe.cpp`
  - `tools/auro3d-decode/src/app/main.cpp`
  - `tools/auro3d-decode/build.bat`
  - `doc/auro_decoder.md`
- `cx_probe.cpp` is compact and partly one-line code. Prefer small, reviewable
  patches. A broader cleanup should be a separate change after behavior is
  covered by regression checks.

## What already works

`auro3d-decode --probe-cx -i <file.mp4>` currently:

- detects the MP4 `a3ds` audio sample entry;
- reads the `acxd` decoder configuration;
- reconstructs access-unit offsets correctly using `stsc`, `stsz`, and
  `stco`/`co64`;
- validates the `A3 DC 0D ED` access-unit marker;
- decodes XOR/VLQ blob framing (observed segment id `1`);
- decodes the initial schema header counts;
- reports program-to-bed references;
- decodes explicit channel descriptors and channel-to-audio-stream mapping;
- handles compact beds whose channel list comes from the `acxd` layout;
- decodes initial PDU vectors and AWC/LFE stream ranges;
- distinguishes lossless and transparent/near-lossless AWC headers;
- decodes the small initial AWC payload configuration;
- reports an experimental parse of access unit 2.

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

## Important warning about access unit 2+

Do not treat the initial schema syntax as invariant for all frames. The second
access unit exposes conditional/delta `ConfigHeader` and `ConfigData` branches.
The current `second_access_unit` PDU report is experimental: it is correct for
some small frames but produces implausible types for several large lossless
frames. Fix the conditional schema syntax before relying on those PDU types.

The compact-bed fallback currently locates a structurally valid PDU vector when
explicit channel-descriptor parsing fails. It is useful and passes the corpus,
but it is a recovery heuristic. Replace it with the exact decompiled compact-bed
syntax once that branch is understood.

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

Files are in `test_files/`. Seven known AuroCX MP4s cover these cases:

- Lori Lieberman: 9.1 near-lossless; `LFE + AWC(2) + AWC(3) + AWC(4) + SASC`.
- Markus Schulz: 13.1 lossless; `AWC(2) + AWC(7) + AWC(5) + SASC`.
- Ola Onabule: 9.1 lossless; `AWC(2) + AWC(4) + AWC(4) + SASC`.
- Alessandro Quarta: compact 7.1 near-lossless;
  `LFE + AWC(2) + AWC(5) + SASC`.
- Yamamoto 13.1 lossless: `AWC(2) + AWC(7) + AWC(5) + SASC`.
- Yamamoto 13.1 near-lossless 1664 and 3328 kbps:
  `LFE + AWC(2) + AWC(6) + AWC(5) + SASC`.

Do not hardcode filenames or these group sizes into the parser.

## Build and regression commands

Build from the tool directory:

```powershell
cd "C:\Users\USER\IdeaProjects\Orua D3\tools\auro3d-decode"
cmd /c build.bat
```

Binary:

```text
C:\Users\USER\IdeaProjects\Orua D3\bin\Release\auro3d-decode.exe
```

Probe one file:

```powershell
& "C:\Users\USER\IdeaProjects\Orua D3\bin\Release\auro3d-decode.exe" --probe-cx -i "<absolute mp4 path>"
```

Run the AuroCX corpus:

```powershell
$exe = (Resolve-Path bin/Release/auro3d-decode.exe).Path
Get-ChildItem test_files -File -Filter '*.mp4' |
  Where-Object { $_.Name -match 'AUROCX|Auro9_1|Auro13_1' } |
  ForEach-Object {
    Write-Output "`n=== $($_.Name) ==="
    & $exe --probe-cx -i $_.FullName
  }
```

After edits always run:

```powershell
git diff --check -- tools/auro3d-decode/src/app/cx_probe.cpp tools/auro3d-decode/src/app/cx_probe.hpp doc/auro_decoder.md
```

Expected initial-AU invariant for all seven files:

- `auro_cx_detected=1`
- `sync_a3dc0ded=1`
- `schema status=header_decoded`
- `pdus ... complete=1`
- PDU types/grouping match the corpus list above.

## Recommended implementation plan

### Phase 1: exact frame schema

1. Add a trace-friendly bit reader result that can record field name, start bit,
   width, value, and failure point without changing normal output.
2. Port `ConfigHeader` from `0x416FC0` exactly. Keep decoded state from AU 1 and
   pass it into AU 2; do not parse every AU into a fresh context.
3. Port the conditional `ConfigData` traversal around line `254726`.
4. Compare bit offsets for AU 1 and AU 2 across all seven files.
5. Remove or narrow the experimental PDU-vector scan once the exact compact-bed
   and delta syntax reaches the correct PDU offset.

Acceptance: AU 2 reports a structurally complete, plausible PDU vector on all
seven files and consumes no bits beyond the decoded blob.

### Phase 2: AWC frame syntax only

1. Introduce standalone structures such as `AwcFrame`, `AwcSubblock`,
   `AwcStreamParameters`, and `AwcIccPair`; avoid coupling them to PCM output.
2. Port lossless `parse_bitdepth_mode`, LPC order/precision, subblock partition,
   and ICC angle syntax.
3. Print deterministic summaries: bit depth, LPC order, coefficient precision,
   subblock lengths, residual bit ranges, ICC pairs.
4. Repeat for transparent/near-lossless only after lossless summaries are stable.

Acceptance: every AWC PDU is consumed exactly, with stable bit counts across
multiple consecutive access units.

### Phase 3: residual and LPC reconstruction

1. Port `DecoderImpl<lossless::ErrorReconstructor>::decode` into an isolated
   module with integer-only unit tests.
2. Decode one AWC group to per-stream PCM buffers; initially disable ICC mixing
   only if the bitstream explicitly indicates no ICC.
3. Add ICC reconstruction and verify lossless round-trip invariants where
   available.
4. Port transparent `ErrorScaler` and its scale-factor syntax.

Acceptance: 1024 samples are produced for every referenced audio stream, with
no under-read/over-read and no discontinuity caused by resetting persistent LPC
state between access units.

### Phase 4: complete MP4 decode path

1. Iterate all sample offsets built from the MP4 sample tables.
2. Preserve decoder/config/LPC state across access units.
3. Route stream indices through the schema channel mapping.
4. Apply LFE PDU and SASC stages in decompiled order.
5. Write multichannel WAV/FLAC through the existing output path.

## Android emulator as an oracle

Use the emulator only when static analysis leaves two plausible interpretations.
The installed Artist Connection app and its native `libauro.so` can serve as a
behavioral oracle. Prefer non-invasive checks: logcat, packet/format observation,
and debugger traces. Do not modify account data, refresh credentials, call
catalog APIs, or download media unless explicitly requested.

Known tools from the current machine:

```text
ADB: D:\Distrib\Android TV\adb\adb.exe
device: emulator-5554
package: com.streamsoftinc.artistconnection
```

Useful first checks:

```powershell
& "D:\Distrib\Android TV\adb\adb.exe" devices
& "D:\Distrib\Android TV\adb\adb.exe" -s emulator-5554 shell pidof com.streamsoftinc.artistconnection
```

If native tracing is needed, first identify the loaded `libauro.so` build and
verify that its addresses correspond to this decompilation. ASLR means IDA
addresses must be rebased to the module load address.

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
- Update `doc/auro_decoder.md` when a format branch becomes confirmed.
