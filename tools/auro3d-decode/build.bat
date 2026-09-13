@echo off
setlocal
chcp 65001 >nul

set "VSDEV=%ProgramFiles%\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEV%" set "VSDEV=%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEV%" set "VSDEV=%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEV%" set "VSDEV=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEV%" set "VSDEV=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEV%" (
  echo VsDevCmd.bat not found. Установите VS 2022 или поправьте путь в build.bat.
  exit /b 1
)

call "%VSDEV%" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b 1

cd /d "%~dp0"
if not exist "..\..\bin\Release" mkdir "..\..\bin\Release"
if not exist "..\..\bin\obj\auro3d-decode" mkdir "..\..\bin\obj\auro3d-decode"

rc /nologo /fo "..\..\bin\obj\auro3d-decode\xinn_presets.res" xinn_presets.rc
if errorlevel 1 exit /b 1

cl /nologo /std:c++17 /utf-8 /EHsc /O2 /D_CRT_SECURE_NO_WARNINGS /I. /Isrc /Ithird_party\qrcodegen ^
  src\app\main.cpp src\app\decoder.cpp src\app\output_layout.cpp src\app\cx_probe.cpp src\app\cx_gain.cpp src\app\cx_decode.cpp src\app\awc_lossless.cpp src\app\lfe_decode.cpp src\app\restore_lfe.cpp src\app\sasc_apply.cpp src\app\sasc_plan.cpp src\app\sasc_resample.cpp src\app\support_qr.cpp third_party\qrcodegen\qrcodegen.cpp ^
  src\io\wav_writer.cpp ^
  src\render\binaural_renderer.cpp src\render\am4hp_upmix_gain.cpp src\render\am4hp_headroom.cpp src\render\am4hp_linear_fader.cpp src\render\am4hp_parameters.cpp src\render\am4hp_low_end.cpp src\render\headphone_lfe_processing.cpp src\render\headphone_multi_delay.cpp src\render\headphone_all_pass.cpp src\render\headphone_room.cpp src\render\headphone_wall_material_filter.cpp src\render\headphone_wiir.cpp src\render\headphone_early_reflection.cpp src\render\headphone_pca_accumulator.cpp src\render\headphone_pca_score_state.cpp src\render\headphone_pca_bank.cpp src\render\headphone_late_reverb_path.cpp src\render\headphone_late_reverb_band.cpp src\render\headphone_late_reverb_core.cpp src\render\headphone_source_manager.cpp src\render\headphone_fgwht.cpp src\render\headphone_explicit_source_path.cpp src\render\peak_limiter.cpp src\render\java_auro_decode_pcm.cpp ^
  src\render\am4hp_equalizer.cpp ^
  src\render\am4hp_equalizer_profiles.cpp ^
  src\render\am4hp_input_layout.cpp ^
  src\render\am4hp_center_front.cpp ^
  src\render\am4hp_center_surround.cpp ^
  src\render\am4hp_core_descriptor.cpp ^
  src\render\am4hp_input_presets.cpp ^
  src\render\am4hp_xinn_runtime.cpp ^
  src\render\am4hp_upmixing.cpp ^
  src\render\am4hp_compressor.cpp ^
  src\render\am4hp_core_processor.cpp ^
  src\render\am4hp_center_front_profiles.cpp ^
  src\render\am4hp_low_end_profiles.cpp ^
  src\util\auro3deng_strength.cpp ^
  src\auro3deng\codec_dsp\codec_dsp.cpp ^
  src\auro3deng\android\android.cpp ^
  src\auro3deng\asc4he\asc4he.cpp ^
  "..\..\bin\obj\auro3d-decode\xinn_presets.res" ^
  /Fo:"..\..\bin\obj\auro3d-decode\\" ^
  /Fe:"..\..\bin\Release\orua3d-decode.exe"
if errorlevel 1 exit /b 1

echo Сборка: ..\..\bin\Release\orua3d-decode.exe
exit /b 0
