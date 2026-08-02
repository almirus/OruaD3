# codec-v3 native encoder map

This is an evidence log for the code to be ported into `src/codec_v3`.  It is
not a replacement for `decopiled/libauro.so.c`.

## Confirmed public encode contract

`auro::codec::v3::unit::encoder::Encoder::encode` at IDA `0x4E4330` takes an
original-layout input descriptor and a carrier-layout output descriptor.  Before
performing any signal operation it rejects the call unless all of these are
true:

- input and output frame counts are equal, and equal the encoder configured
  block size;
- input and output sample rates are equal, and equal the encoder configured
  sample rate;
- both sample depths are exactly 24 bits;
- a non-null input pointer exists for each channel in the configured original
  layout;
- a non-null output pointer exists for each channel in the carrier layout
  returned by `auro_codec_v3_get_carrier_layout`.

The encoder advances its sample cursor by the input frame count before the DSP
stages.  A successful call performs these operations in this exact order:

```text
downmix_
select_loudness_
process_groups_
prepare_metadata_unit_block_
prepare_mix_
update_cyclers_
```

Failure in any stage returns a non-zero status and must not be covered up by
substituting metadata, padding, or a fallback mix.

`Config::validate` (`0x4F8AE0`) also calls
`auro_codec_v3_is_sample_rate_supported` and
`auro_codec_v3_is_unit_block_size_supported`. The confirmed sample rates are
44.1, 48, 88.2 and 96 kHz. A unit block is valid when it is 1000, or a
16-sample multiple in the native accepted interval; 992 is explicitly rejected.
The host read block is a separate streaming concern and is not substituted for
the encoder unit block.

`Config::init_defaults` (`0x4F8790`) stores the optional dword pairs in
present/value order through `+124`, but the final downmix gate at `+132` is
read as a value followed by its marker at `+136`. Its default qword write is
therefore `value=1,present=0`; clearing that value makes native `downmix_`
reject the call before any group processing.

The optional pair at `Config+36/+40` is the worker count. Native defaults it
to `hardware_concurrency-1`; the standalone CLI exposes the equivalent total
thread count as `--threads N`, where `N=1` selects the serial reference path.
Only independent active-group and bit-line analysis is scheduled in parallel.
UnitBlock order, dither consumption, quality-filter state commits, metadata
cyclers, loudness state, CTS limiting, and carrier output remain ordered.
The parallel result is required to be byte-identical to `--threads 1` for a
fixed seed; `regression/compare_threads.ps1` enforces this contract.

Sources: `decopiled/libauro.so.c`, `Encoder::encode` (`0x4E4330`), lines around
`449640` in the current decompilation; constructor at `0x4E2440`; construction
at `0x4E2DC0`.

## Implemented preparation boundary

`Pcm24InputStream` presents input as 31 codec-v3 channel-ID planes.  The input
WAV's physical interleaved order is bound only by the caller's
`--input-channel-order`; it is never inferred from its channel count or its
WAVE channel mask.  It returns short final host blocks unchanged. Native
`a3deng::pipeline::ac::Encoder::process` either requires an exact multiple of
the configured unit size, or (buffering mode) accumulates into Encoder+632
until a full unit is ready; there is no flush API, and `reset_audio_state`
discards the remainder. Exact source coverage therefore belongs to the
UnitBlock schedule (`plan_codec_v3_unit_blocks`), not to host-block padding.

The DSP port consumes these planes and creates a carrier plane set meeting the
descriptor contract above. Scheduled native-size units can be written after
the confirmed `prepare_mix_` path. Layout ADOL is mandatory; explicit primary
gain, secondary gain, and loudness instructions are supported without
inventing metadata policy.

`frame_descriptor.*` now represents this contract explicitly: each complete
unit has original PCM24 pointers only for its original layout and allocated
PCM24 carrier pointers only for the native carrier layout. It performs the
same shape, format and required-pointer checks before any future DSP stage.

## Confirmed storage bit direction

`auro::bitstream::Writer<...>::write_<unsigned long>` at `0x336980` emits the
low requested value bits first: bit 0 goes to bit 0 of the current output byte,
then the value is shifted right. `LsbBitWriter` is a direct implementation of
this storage primitive and is now used only for the confirmed Golomb-Rice
symbol stream; field order, terminators and padding remain owned by their
corresponding native serializers.
`write_golomb_rice @ 0x450750` emits `value>>k` one bits followed by a zero
terminator, then the low `k` remainder bits. This polarity matches the
decoder's leading-one counter.

`a3d::serialize<Projector::Iterator>` at `0x51D010` serializes the two mix2
Extrapolate seeds in decoder-frame order: Group+616 element 1 first, then
element 0. The mixer-owned vector itself is not reordered. Writing the vector
in storage order swaps the two predictor initial states and produces an
antisymmetric bed/height error even though their decoded sum remains equal to
the carrier.

## Confirmed CRC primitive

`Crc16` ports the codec-v3 CRC update from
`auro_codec_v3_decoder_CRC_process` (`0x52ECA0`) and the stored-word transform
from `CRC_check`. It has no byte-stream adapter: codec serialization must first
establish the native projector's word sequence and only then feed those words
to this primitive.

## Confirmed metadata scheduling

`Encoder::update_cyclers_` (`0x4E6CA0`) updates five independent cursor/period
pairs that feed `prepare_metadata_unit_block_` optional UnitBlock slots:

- auromatic (Encoder+6320) → UnitBlock+16 → ADOL `0x47`
- secondary downmix (Encoder+6272) → UnitBlock+248 → ADOL `0x46`
- opcode `0x50` (Encoder+6296) → UnitBlock+500
- encoder version (Encoder+6224) → UnitBlock+528 → ADOL `0x64`
- opcode `0x6E` (Encoder+6248) → UnitBlock+536

Loudness is `select_loudness_` / Encoder+12712 (schedule queue at +12680), not
one of these cyclers. `select_loudness.*` ports the default six-entry schedule
(types 5,4,0,1,2,3 with periods `trunc(0.0833*sr)`, `trunc(0.3125*sr)`, and
`trunc(0.9583*sr)`), measurement enablement, and the first-due selection that
writes type/packed payload for `prepare_metadata_unit_block_`. Primary-downmix
`0x40` and limit-simple `0x41` are Channel-compose paths
Primary-downmix `0x40` and limit-simple `0x41` are Channel-compose paths
(`from_primary_downmix_gain` / `from_limit_simple`). Channel frames emit
parser opcode 64 for each non-zero Encoder+6368 original-map entry and
parser opcode 65 for Group+392, in that native order after the optional
limit_simple ADOL. `cycler.*` ports the five
cursor transitions, construct-default period `trunc(0.9583*sr)`, and
`set_dynamic_params` arming (enable + cursor=0 on value change for secondary /
opcode_50; auromatic no-op when unchanged). Construct LABEL_91 also seeds
encoder_version payload `0x030306`, enable byte `+6211=1`, cursor `-1`, and
always arms opcode_6e to dword `-1659869902` with enable and cursor `-2`.
ADOL conversion for the confirmed
opcodes is in `layout_metadata.*` / `metadata_factory.*`.

## Confirmed carrier export stage

`Encoder::prepare_mix_` (`0x4E6AB0`) takes completed group PCM, validates each
group's frame count and carrier destination, then either copies it unchanged or
left-shifts every 32-bit sample by the group's quantization shift. `prepare_mix.*`
ports this scalar behavior; `unit_encoder.*` now invokes it after
`process_groups_` creates the analyzed `EncodedGroupPcm` values.

## Confirmed input-rescale stage

The first part of `Encoder::downmix_` (`0x4E4520`) copies original-layout
planes and, for enabled non-unit channel scalers, divides PCM samples by the
native float scaler using truncation toward zero. `input_rescale.*` ports that
arithmetic and now accepts explicit gain or scaler-index arrays. The confirmed
`downmix_` gate is represented by `EncoderConfig::field_132.value == 1`; the
optional coefficient-limit path (`+136` marker) is `cts_dmx.*` /
`cts_dmx_coeff_limit_` and is enabled by `--cts-dmx`.
`EncoderConfig::input_scalers_present` enables the scalar divide for
library callers without selecting a hidden default.

## Reused decoder evidence: PCM metadata transport

The encoder now contains the exact inverse of the decoder's
`MuxIteratorTrue` mapping and its metadata-block CRC calculation, transcribed
from `tools/auro3d-decode/src/app/decoder.cpp`. It exposes bit locations and
writes supplied bits; the complete-unit path supplies the confirmed
sync/header and minimal layout/ADOL fields without inventing optional branches.

`write_pcm_metadata_sync_header` now also ports the inverse of
`parse_sync_pcm24_at`: sync sentinel, block-size field and mux parameter are
derived algebraically from its decoder equations. It does not create the
following ADOL or frame payload.

`PcmMetadataFalseWriter` is the exact writer counterpart of the decoder's
`MuxIteratorFalse`/`read_unsigned_false`, including projector position 32 and
the `16 * bit_line - 1` periodic mask cycle.
It can now place any already-defined metadata field into its native PCM bit
positions; the current factory emits only the confirmed direct channel-input
ADOL announcement and leaves optional scheduler-generated records separate.

`metadata_syntax.*` serializes the fixed metadata prefix in the exact order
used by the decoder's `extract_metadata_from_a3d_block_at`. It requires every
field and extension-word count explicitly, pending a port of the native
`prepare_metadata_unit_block_` optional value construction.

## Next code targets

1. ~~Trace `auro_codec_v3_get_carrier_layout` for every accepted original layout.~~
2. ~~Port `Config::init_defaults` / `Config::validate` (`encoder_config.*`).~~
   The CLI now exposes the confirmed profile table (1..5), while the config
   validator rejects unresolved profile values after native default mapping.
3. ~~Port `get_original_channels` @ `0x50F8E0` and carrier-group planning (`original_channels.*`, `encode_groups.*`).~~
4. ~~Port DetectSilence threshold, `pcm::shift_right`, mix2 `mix::deltas`, arity-1 Mixer copy (`pcm_shift.*`, `mix_deltas.*`, `process_groups.*`).~~
5. ~~Port mix2 `auro::mix::mix` reconstruction as a pure function (`mix_mix2.*`) and wire the confirmed BitShift residual table.~~
6. ~~Port native mix3 delta layout and scalar mixer/seed closure.~~
7. ~~Port one complete `downmix_` branch together with its exact fixed/float
   arithmetic and channel-pointer routing (beyond `input_rescale.*`).~~
   `ChannelLimiter` (`channel_limiter.*`, IDA `0x5065F0`+) is ported:
   ctor/release/max-gain/distribution and both `dmx_limit_coeff` overloads.
   `Limiter::limit_new` + `cts_dmx_coeff_limit_` (`cts_dmx.*`, IDA
   `0x5058D0` / `0x4E70E0`) are ported for Config+136: PCM24→float,
   per-carrier ChannelLimiter, gain_to_scaler/scaler_to_ix into the
   working scaler-index table, then the existing scalar divide path.
   CLI `--cts-dmx` sets the Config+136 marker. Explicit
   `--input-scaler-indices` still seed Encoder+6384-equivalent gains and
   fill `MetadataSourceRef.original_map`. `--input-gains-db` ports the
   confirmed Dynamic original-gain subset of `set_dynamic_params`
   (`dynamic_params.*`): validate [-24,0], `gain_to_scaler`/`scaler_to_ix`,
   optional +6384 write when +136 is set, and loudness measurement enablement
   into the Encoder+12680 schedule. `--carrier-gains-db` ports the second
   31×12-byte Dynamic table validated against Encoder+168 (status 396): present
   carrier gains materialize into secondary-downmix floats for ADOL 0x46 via
   the same path as `from_secondary_downmix_gains` @ `0x4FF600`. Optional
   Dynamic opcode_50 (value ≤ 3) and auromatic (profile/mode ≤ 15) payloads
   after loudness are copied onto the UnitBlock cycler config fields
   (Encoder+952/+960). encoder_version / opcode 0x6E are not in this native
   function: `set_cyclers_` arms version `0x030306` (enabled, cursor `-1`)
   and always arms opcode_6e (`-1659869902`, enabled, cursor `-2`).

   Channel payload BitReader width is the group `bit_line` (Projector N),
   matching decoder `24 - headroom` with headroom `24 - bit_line`. The
   previous `24 - bit_line` writer width was inverted relative to native.
8. ~~Port mix2 `cluster_deltas` Quantization (`cluster_deltas_quant.*` /
   `0x501750`/`0x501F70`/`0x502490`), wire Mixer into `process_groups`.~~
   The confirmed `GVM::configure` field mapping is now retained in
   `EncoderConfig::gvm`. `create_and_configure @ 0x5011F0` backend selection
   is explicit: present mode 3 selects scalar BitShift `Quantization`, while
   other accepted modes select GVM. Profiles 4/5 therefore no longer fail by
   incorrectly passing mode 3 to `GVM::configure`.

   The effective clustering block now covers every Config pair from +52
   through +120, including the previously omitted +76/+80 pair. Factory/GVM
   configuration is derived from those populated fields rather than
   recomputing a second profile table, and Config validation checks that the
   retained backend and mode still agree with +92/+96.

   `GVM::configure` also preserves its pre-override dword +64 value `65793`:
   flags +64/+65/+66 default true and +67 defaults false. An absent optional
   pair now leaves that native default intact instead of clearing it.

   `vq::multi::Factory::create` implementation selection is retained too:
   GVM mode 0 constructs the modern learner, mode 2 constructs `GVMOldFast`,
   and mode 1 has no factory branch and is rejected before group analysis.

   The confirmed pre-learning path is now executable for both dimensions:
   factory implementation and mode are cross-checked, `set_data_<1/2>`
   geometry is enforced, every int32 residual is converted exactly to double,
   and dimension-2 scalars are paired in native order. The optional qword at
   GVM+48/+56 is retained as the deterministic learner RNG seed. Encoder
   Config+144/+152 separately seeds the Rescaler dither path; absent seed
   continues to mean the native `time(nullptr)` branch.

   The post-learning center conversion is ported separately from clustering:
   1D and 2D doubles use native truncating conversion with INT_MIN for NaN or
   out-of-range values. When the config byte at learner +24 requests it, the
   minimum wrapped squared-magnitude center is selected with stable tie order;
   1D clears that scalar and 2D clears the first component exactly as
   `get_centers<int,1/2>` does.

   A complete post-learn adapter now accepts learner `sizes`, `ixs`, and
   double centers, verifies every index population against `get_sizes`,
   enforces the 600-entry selector ceiling, performs center conversion, then
   computes BitSize and fit status.

   The modern learner's cluster arithmetic is now shared for dimensions 1
   and 2: singleton population/sums, normalized squared energy, merged
   population with checked addition, reciprocal-by-population energy, and the
   exact `energy(a)+energy(b)-energy(merged)` merge cost use the operation
   order visible in `Learner<1/2>::learn<double>`.
   Modern and OldFast online learning are both connected to the native
   decrement/+30 search controller. OldFast's full-table comparison is
   expressed as the equivalent minimum of singleton insertion and an
   existing-pair merge. Final clusters are ordered by descending population.
9. ~~Port `prepare_metadata_unit_block_` header + case-1/case-2/case-3 group
   records (`prepare_metadata.*`).~~ Explicit loudness, primary/secondary
   downmix, limit-simple (`0x41`), auromatic (`0x47`), encoder-version
   (`0x64`), and UnitBlock cycler opcodes `0x50`/`0x6E` ADOL conversion is
   ported. Cyclers gate the five UnitBlock optional slots and advance
   transactionally with successful UnitBlocks. `select_loudness_` and the
   default Encoder+12680 schedule are ported; Dynamic measurement injection
   uses `apply_loudness_measurement` without inventing AST policy. Dynamic
   carrier gains and opcode_50/auromatic payloads from `set_dynamic_params`
   materialize onto EncoderConfig metadata before cycler gating.

## Serializer boundaries now represented in code

`channel_frame.*` is the explicit inverse boundary for the decoder's channel
parser states (`channel_metadata_combine_info` and
`channel_parser_process_103840`). It accepts native header/metadata words,
mode-dependent seeds/context and parser stream words, then returns the packed
carrier words and the stored CRC. It does not choose entropy parameters or
analysis values.

`channel_codebook_plan.*` packs signed residual tables into the parser's
mode-dependent context geometry. `encode_codebook_channel_frame_golomb` adds
the confirmed static Golomb-Rice stream: metadata word0 bits 24..27 carry the
`k` parameter, while the index symbols are written LSB-first into the
MSB-oriented channel words. `channel_frame_plan.*` now exposes a strict
arity-1/2/3 builder using the `k=1..7` mode already selected by native
`BitSize::calculate`; it rejects any difference between the actual serialized
index length and the population-derived level cost used by the quantizer.
The standalone GR writer precomputes and verifies its exact unary-plus-
remainder bit count before accepting a block.
The shared metadata constant `kCodecV3ChannelCodebookMaxEntries=600` is the
largest context capacity produced by selector `0x53`; native
`error_center_index` rounds a shorter residual table up to the next capacity
(8, 10, 12, 14, 16, then 4/8-entry ranges). Channel serialization pads that
selected tail with zero residuals, while quantization rejects tables larger
than 600 entries before they reach channel serialization.

`channel_frame_plan.*` builds the confirmed arity-1/2/3 A3D stream, overlays
its serialized bits on the prepared carrier, and closes sync/CRC independently
for every carrier channel using that group's `bit_line`.
The scratch writer retains logical MSB-first fields; final projection uses the
inverse `MuxIteratorFalse` mapping. Native serialization skips projector masks
0..15 (sync) and 16..31 (CRC), writes the fixed A3D header at masks 32..47,
then starts the Channel stream at mask 48. The false iterator uses the exact
`16 * bit_line - 1` mask-cycle length from `Projector::fill_masks_`. It emits
the native frame code, flags and `14-bit_line` nibble, then projects CRC bits
into the sixteen reserved prefix masks exactly as `Projector::project_crc`.
`ChannelBitWriter` now exposes the exact reserved-bit-aware payload capacity
and remaining capacity for each serialized word span.
`unit_encoder.*` composes these stages for one complete unit and emits an
`EncodedCarrierUnit`; it retains the native metadata header and group-record
vector for validation, distributes delayable ADOL by remaining channel
capacity, and checks the configuration carrier against `set_carrier_`
priority LFE/RB/C/RS/FR. The CLI's exact UnitBlock schedule owns source-length
finalization without padding.
`mux_channel_words` exposes the confirmed `Channel::mux` shift/mask branch
with dynamic projector and CRC-prefix projection as a low-level helper. The
production unit path emits the complete header, ADOL and residual stream
through `ChannelBitWriter` and closes every channel independently.
`carrier_frame.*` places those already serialized and CRC-closed words into the
native carrier layout without a second metadata overlay. Missing channel
payloads, duplicate IDs and any span other than the
declared unit length are rejected; retained UnitBlock group records are also
checked for native arity/headroom/channel bounds at the output boundary. No
zero-fill policy is hidden in this layer.
The retained channel-word view and its CRC are recomputed from the final
carrier planes so they describe the emitted samples, including sync/CRC bits.
The output boundary reads the fixed header and complete per-channel ADOL list
back through the same false iterator and requires exact equality with the
placement plan.

`metadata_factory.*` emits the minimal direct channel-input ADOL announcement
(opcode `0x1e`) and the corresponding four-byte metadata configuration table.
It is limited to layouts accepted by the direct native configuration mapping.

The PCM sync field is written through the confirmed inverse mapping
`mux_code = 14 - m`; `m` is the selected group's `bit_line`, matching
`Channel::mux` rather than an independent CLI parameter.

`flac_output.*` is a container-only writer: FLAC STREAMINFO plus verbatim
PCM24 subframes and standard FLAC CRC8/CRC16. It does not add private metadata
or alter codec-v3 carrier bits. The writer accumulates actual encoded block
and frame sizes and patches STREAMINFO only after the declared source span is
complete; placeholder bounds are never left in a successfully closed file.
Frames use FLAC variable-block numbering by absolute sample offset because a
1000-sample codec unit produces a 232-sample subframe before subsequent full
frames. Sample numbers use the complete shortest-form 1..7-byte FLAC UTF-8
encoding across STREAMINFO's 36-bit total-sample range.

`encoder_defaults.*` ports `unit::encoder::default_bit_line` (`0x4F85A0`) as
an explicit packed low/high word pair. It is a parameter primitive only; the
encoder still must select the profile and parameter from its traced group
configuration.

`CarrierOutput` selects the existing PCM24 WAV writer or the verbatim FLAC
writer by extension. Both enforce the declared channel order and total frame
count; neither performs codec analysis or alters carrier samples.
For WAVEFORMATEXTENSIBLE, native carrier IDs are translated to Microsoft
speaker bits and physically sorted by those bits. Internal 7.1 order
`FL,FR,C,LFE,LS,RS,LB,RB` is therefore written as canonical WAV order
`FL,FR,C,LFE,LB,RB,LS,RS`; PCM metadata retains the native carrier layout.

`validate_pcm_metadata_block` recomputes the decoder scanner's PCM CRC and its
first-16-sample expected value after embedding. Carrier assembly rejects a
unit if this closure is not exact.

`pcm_mux.*` ports the native `Projector<unsigned int,N>` mask geometry and its
`project`/`project_crc` cursor rules for N=3..16. The first sixteen samples
contain fixed runs for bitlines 0..2 and a cyclic descending run for bitlines
3..N-1; every later sixteenth output omits the final bitline for the frame
CRC. Projection accepts the separately serialized projector words; it does not
infer bitline selectors or entropy parameters. The static selector prefix (the
36-byte iterator record) is implemented for its complete N=3..8 prefix. The
dynamic record switch at `0x520D50` is also represented, including scalar
8/16/24/32-bit records and selectors 64/100 with two/three 8-bit payloads;
record scheduling and the caller's entropy values remain separate.

`serialize_pcm_mux_projector_records` now wraps the dynamic record serializer
with checked mask-plan construction, optional zero-selector termination, and
exact cursor consumption. It deliberately does not synthesize record values
or scheduling from PCM.

`CarrierOutput::write_unit` is the final descriptor gate before a container
writer receives samples. It requires every active carrier plane to have the
declared unit length and rejects non-empty inactive planes.

Channel payload words are canonicalized to sign-extended PCM24 before carrier
assembly. WAV and FLAC writers reject samples outside the signed 24-bit range;
no implicit truncation or rail clipping is performed at the container edge.

## cluster_deltas Quantization (mix2)

`cluster_deltas_quant.*` ports:

- `Quantization::run_` @ `0x501750` (mix2 when deltas/samples == 1): bit budget
  `N*bit_line - ((N-1)>>4) - (Group+208 + sum(Group+184))`. With the
  standalone default Group+268 absent, Rescaler overwrites Group+196 with
  32 bits; the alternate present branch uses 72. It adds another 16 bits when
  Group+392 carries a base scaler. The active costs now enter the allowance.
  Optional Group+274/+284/+292/+299/+304 and Group+240-map records are
  accounted with the exact native 16/40/16/32/40/24-bit operations.
  Group+312 uses the closed `rescaler::opcode_bitwidth @ 0x5011A0` table:
  opcodes 1/3/90..98 carry 16 bits, 2/4/31/81..88 carry 8 bits,
  101..108 carry 24 bits, and 14/111..118/140..144 carry 32 bits; each
  instruction also costs its 8-bit opcode. Unknown opcodes are rejected.
  Group creation supplies `Encoder+6352`, written directly by
  `Encoder::reserve_extra_bits @ 0x4E37E0`, as `a5`; this is not Config
  `field_52`. The standalone default is zero and the CLI exposes the native
  setter explicitly. Quantization tries `BitShift` shift 0 then
  1..30;
- VQ `BitShift<int,100>::process_` @ `0x501F70`: bias=`(100<<s)+((1<<s)>>1)`,
  bins `(bias+sample)>>s` clamped to 201, codebook by descending histogram
  count, residual `(bin<<s)-(uint32)(100<<s)`, OOR samples append raw residual
  with level `1`;
- `BitSize::calculate` @ `0x502490` using `error_center_index` /
  `nr_error_centers_per_index` @ `0x50F850` / `0x50F7F0`.

The cost calculation is shared with the two-dimensional `GVM::compute_fit_`
shape: mix3 residual centers are charged twice at the selected signed width,
while level scheduling uses the same modes 1..7.

GVM level tables carry `Learner::get_sizes` populations, and GVM records
require the population sum to equal the unit sample count before
serialization.

`compute_fit_` result status is also exact: cost above the allowance is status
1; a fitting table is status 2 unless its spare bits are strictly greater than
`0.1 * sample_count`, in which case it is status 3. The native Group+520 flag
is retained as true for statuses 2 and 3 and is checked again before output.

`process_groups` feeds the resulting indices/residuals into
`mix2_mixer_reconstruct`. Backend execution is not blended: Quantization
profiles try BitShift 0..30 and fail when none fits; GVM profiles run the
factory-selected modern or OldFast learner. The native Quantization backend
rejects two-dimensional mix3 residuals.

The BitShift scale/bias/sample addition follows the native qword fields at
`0x501F70`: scale and bias use `100LL << shift`, and the signed sample is
converted to unsigned 64-bit for the addition before the logical bin shift.
Residual reconstruction still keeps the low dword of the scale, matching the
native store into the signed residual vector.

The mix predictors preserve native 32-bit wrap order: even-rounded samples are
added as unsigned dwords, then the wrapped sum is reinterpreted as signed
before arithmetic division by two; mix3 four-way predictors add three before
the signed arithmetic right shift for negative deltas.
All encoder-side SAR/PSRAD operations now use an explicit 32-bit sign-fill
helper, avoiding host-language signed-shift behavior while preserving native
wraparound values.

The static Golomb-Rice channel serializer keeps the selected parameter in
metadata word0 bits 24..27 while the parser channel header is the byte-ordered
native fixed word `0x010A`. The parameter is not part of that 16-bit field.

`compute_mix3_deltas` now ports the three native `mix3::deltas` specializations
(`0,2,4`, `1,1,5`, and `2,2,4`) and their fixed zero slots. The scalar
`mix3_mixer_reconstruct` port mirrors the native `mix3::mix<0/1/2>` loops and
the five seed fixups. Its residual width now follows the native
`BitSize::calculate` max-absolute-value rule instead of a fixed 24-bit field.

`prepare_mix_` @ `0x4E6AB0` left-shifts carrier PCM by **Group+24 (`bit_line`)**,
not the VQ codebook shift. `EncodedGroupPcm::quantization_shift` is therefore
`bit_line`; VQ shift is kept on `AnalyzedEncodeGroup::vq_shift` with Mixer seeds
and indices/residuals for later metadata/payload ports.

`scaler.cpp` now ports the confirmed scalar pieces at `0x53D7E0`, `0x53D7F0`
and `0x53D830`: index validation clamps values at `0xF0`, index `0xFF` maps
to the native infinity sentinel, indices `0..0xF0` use the exact 241-entry
table, and explicit gain dB values use `powf(10, -0.05f * gain)` with the
native `>=144 dB` zero result.

DetectSilence all-silent writes one zero analysis frame and sets
`Group+192 = dword_287F20[0] = 0` (not arity-indexed).
The analysis port also removes individually silent source frames before
dispatch: one remaining source uses Mixer case 1, two remaining sources use
the confirmed mix2 path, and three active sources use the configured
two-dimensional GVM learner.

The input geometry gate at the start of `GVM::run_` @ `0x502AA0` is ported
for both learners: residual scalar count must divide the PCM sample count
exactly, both counters must fit native 32-bit storage, and only dimension 1
(mix2) or dimension 2 (mix3) is accepted. The shape is retained through
analysis and metadata preparation.

The same port now retains Group+36/+40/+44/+48 search bounds, selects the
dimension-specific starting cluster count, and implements the post-fit state
transitions: status 1 decrements toward +48, status 2 accepts, and status 3
adds 30 before the next +36 clamp unless already at +36. Each transition
reruns the selected learner at the requested cluster count before evaluating
the next fit status.

`Encoder::create_group_ @ 0x4E8360` initialization is now applied after the
bare Group constructor: +36 is Config +56 when present or 150, +40 and +44
are `dword_287EF0[bit_line-3]`, and +48 is one. Thus profile 2 begins a
bit-line-3 GVM search at 15 clusters under its common limit 200, rather than
incorrectly remaining at the constructor value one.

`prepare_metadata.*` ports the unit header (native `unit_block_size`,
`original_layout`, Config `field_28` value, and fixed word `2561`) and per-group
records at UnitBlock+552 (stride 128) for analysis arity
1, 2, and 3:

- case 1 (silent / arity-1): carrier ch, `24-bit_line`, source id from analysis
  Frame+24; optional Encoder+6368 original-map left unset;
- case 2 (mix2): both source ids, seeds from Group+616 (two int32), VQ byte
  Group+70, residual bit-width Group+67, residuals trimmed to `max(index)+1`,
  indices as low dwords of Group+448; residual count must map to a native
  channel metadata selector.
- case 3 (mix3): three source ids, five int32 seeds from Group+616, VQ byte
  Group+70, residual bit-width Group+67, eight-byte residuals from Group+496,
  and low dwords of Group+448 indices; the deduplicated pair count must map to
  a native channel metadata selector.

Mix3 residual context width follows `BitSize::calculate` at `0x502490`:
`floor(log2(max_abs)) + 2`, with a minimum of two bits and the native 32-bit
storage ceiling.

Explicit loudness and primary/secondary-downmix metadata conversion is wired
through the scheduled metadata selection together with limit-simple,
auromatic, encoder-version, and UnitBlock opcodes `0x50`/`0x6E`. Complete-unit
WAV/FLAC muxing uses the unified Channel projector stream. Exact source-length
scheduling uses only native UnitBlock
sizes (256..1024 in steps of 16, plus 1000), rejects unrepresentable lengths,
and never pads or trims PCM.
