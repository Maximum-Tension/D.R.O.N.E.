@echo off
cd /d "%~dp0"
echo building gpuprobe.exe ...
gcc -O2 -static -o gpuprobe.exe src\GPU.c src\GPU_PROBE.c src\THREAD.c
if errorlevel 1 (echo BUILD FAILED & pause & exit /b 1)
echo running the GPU probe (a few seconds)...
gpuprobe.exe 2048 > gpu_probe.txt 2>&1
type gpu_probe.txt
echo.
echo Saved as gpu_probe.txt - send it to me.
pause
