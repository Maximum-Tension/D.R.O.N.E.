@echo off
cd /d "%~dp0"
if not exist ai.exe call build.bat
ai.exe --know
