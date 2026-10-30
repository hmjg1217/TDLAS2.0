@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\build.ps1" -Configuration Debug
set "BUILD_EXIT=%ERRORLEVEL%"
if /I not "%~1"=="--no-pause" pause
exit /b %BUILD_EXIT%
