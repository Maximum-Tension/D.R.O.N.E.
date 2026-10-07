@echo off
cd /d "%~dp0"
if not exist ai-test.exe call build.bat
ai-test.exe
ai.exe --selftest
ai.exe --eval tests\skills_eval.txt
