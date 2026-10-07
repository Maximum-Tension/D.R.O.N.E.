@echo off
cd /d "%~dp0"
if not exist ai-train.exe call build.bat
if "%~1"=="" (
  ai-train.exe --minutes 60 data
) else (
  ai-train.exe %*
)
