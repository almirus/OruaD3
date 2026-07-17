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

cl /nologo /std:c++17 /utf-8 /EHsc /O2 /D_CRT_SECURE_NO_WARNINGS /I. /Isrc ^
  src\app\main.cpp src\app\decoder.cpp src\app\cx_probe.cpp src\app\cx_gain.cpp src\app\cx_decode.cpp src\app\awc_lossless.cpp src\app\lfe_decode.cpp src\app\sasc_apply.cpp src\app\sasc_plan.cpp src\app\sasc_resample.cpp ^
  src\io\wav_writer.cpp ^
  src\render\binaural_renderer.cpp src\render\java_auro_decode_pcm.cpp ^
  src\util\auro3deng_strength.cpp ^
  src\auro3deng\codec_dsp\codec_dsp.cpp ^
  src\auro3deng\android\android.cpp ^
  src\auro3deng\asc4he\asc4he.cpp ^
  "..\..\bin\obj\auro3d-decode\xinn_presets.res" ^
  /Fo:"..\..\bin\obj\auro3d-decode\\" ^
  /Fe:"..\..\bin\Release\auro3d-decode.exe"
if errorlevel 1 exit /b 1

echo Сборка: ..\..\bin\Release\auro3d-decode.exe
exit /b 0
