@echo off
setlocal
chcp 65001 >nul
cd /d "%~dp0..\..\.."
python tools/auro3d-decode/regression/run_regression.py %*
exit /b %ERRORLEVEL%
