@echo off
setlocal
chcp 65001 >nul

set "VSDEV=%ProgramFiles%\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEV%" set "VSDEV=%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEV%" set "VSDEV=%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEV%" set "VSDEV=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEV%" set "VSDEV=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEV%" (
  echo VsDevCmd.bat not found. Install VS 2022 or update this path.
  exit /b 1
)

call "%VSDEV%" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b 1

cd /d "%~dp0"
if not exist "..\..\bin\Release" mkdir "..\..\bin\Release"
if not exist "..\..\bin\obj\auro3d-encode" mkdir "..\..\bin\obj\auro3d-encode"

cl /nologo /std:c++17 /utf-8 /EHsc /O2 /D_CRT_SECURE_NO_WARNINGS /I. /Isrc ^
  src\app\main.cpp src\app\wav_input.cpp src\app\wav_output.cpp src\app\flac_output.cpp src\app\carrier_output.cpp src\app\pcm_input.cpp src\codec_v3\layout.cpp src\codec_v3\frame_descriptor.cpp src\codec_v3\bit_writer.cpp src\codec_v3\channel_bit_writer.cpp src\codec_v3\channel_payload.cpp src\codec_v3\channel_metadata.cpp src\codec_v3\channel_codebook_plan.cpp src\codec_v3\channel_frame.cpp src\codec_v3\channel_frame_plan.cpp src\codec_v3\carrier_frame.cpp src\codec_v3\metadata_factory.cpp src\codec_v3\encoder_defaults.cpp src\codec_v3\encoder_config.cpp src\codec_v3\original_channels.cpp src\codec_v3\encode_group.cpp src\codec_v3\encode_groups.cpp src\codec_v3\pcm_shift.cpp src\codec_v3\native_dither.cpp src\codec_v3\mix_deltas.cpp src\codec_v3\mix_mix2.cpp src\codec_v3\mix_mix3.cpp src\codec_v3\mix_unmix.cpp src\codec_v3\cluster_deltas_quant.cpp src\codec_v3\quality_filter.cpp src\codec_v3\quality_measure.cpp src\codec_v3\process_groups.cpp src\codec_v3\prepare_metadata.cpp src\codec_v3\golomb_rice.cpp src\codec_v3\error_quantization.cpp src\codec_v3\packed_codebook.cpp src\codec_v3\extrapolate_math.cpp src\codec_v3\extrapolate_mix2.cpp src\codec_v3\extrapolate_mix3.cpp src\codec_v3\extrapolate_mix3_tail.cpp src\codec_v3\crc.cpp src\codec_v3\cycler.cpp src\codec_v3\select_loudness.cpp src\codec_v3\dynamic_params.cpp src\codec_v3\prepare_mix.cpp src\codec_v3\input_rescale.cpp src\codec_v3\scaler.cpp src\codec_v3\channel_limiter.cpp src\codec_v3\cts_dmx.cpp src\codec_v3\unit_encoder.cpp src\codec_v3\pcm_metadata.cpp src\codec_v3\pcm_mux.cpp src\codec_v3\metadata_syntax.cpp src\codec_v3\adol_syntax.cpp src\codec_v3\metadata_block.cpp src\codec_v3\channel_config.cpp src\codec_v3\layout_metadata.cpp ^
  /Fo:"..\..\bin\obj\auro3d-encode\\" ^
  /Fe:"..\..\bin\Release\orua3d-encode.exe"
if errorlevel 1 exit /b 1

echo Build: ..\..\bin\Release\orua3d-encode.exe
exit /b 0
