@echo off
setlocal
chcp 65001 >nul

set "VSDEV=%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
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

cl /nologo /std:c++17 /utf-8 /EHsc /O2 /D_CRT_SECURE_NO_WARNINGS ^
  main.cpp auro3d_decoder.cpp java_auro_decode_pcm.cpp auro3deng_from_ida.cpp auro3deng_strength.cpp wav_writer.cpp ^
  /Fo:"..\..\bin\obj\auro3d-decode\\" ^
  /Fe:"..\..\bin\Release\auro3d-decode.exe"
if errorlevel 1 exit /b 1

echo Сборка: ..\..\bin\Release\auro3d-decode.exe
exit /b 0
