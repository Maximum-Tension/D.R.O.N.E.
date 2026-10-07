@echo off
setlocal
cd /d "%~dp0"
title Lucy - clean up
if not exist train_round.bat goto :notcode
if not exist src\CODE.h goto :notcode

echo.
echo  Lucy - clean up
echo  ===============
echo  This removes leftovers that nothing uses any more:
echo    work\                     old round 1-2 training folder (its best brain is already mind_original\start.web)
echo    data\skills16.txt         old practice data (rounds 1-2)
echo    data\gen_eval16.txt       old practice test (rounds 1-2)
echo    gpuprobe.exe, gpu_probe.txt   GPU probe program and result (I already have the result)
echo    engine_test.txt           last engine test output (made again on every build)
echo.
echo  It never touches: work_round3 and newer (training in progress), mind_original\start.*,
echo  mind\ (memory, language, skills), the data the trainer reads, src\, tools\, tests\, recipes\, or your TEO folder.
echo.
set ok=
set /p ok= Type YES to clean:
if /i not "%ok%"=="YES" (echo  Nothing was deleted. & goto :end)

if exist work (rmdir /s /q work & echo  removed work\)
if exist data\skills16.txt (del /q data\skills16.txt & echo  removed data\skills16.txt)
if exist data\gen_eval16.txt (del /q data\gen_eval16.txt & echo  removed data\gen_eval16.txt)
if exist gpuprobe.exe (del /q gpuprobe.exe & echo  removed gpuprobe.exe)
if exist gpu_probe.txt (del /q gpu_probe.txt & echo  removed gpu_probe.txt)
if exist engine_test.txt (del /q engine_test.txt & echo  removed engine_test.txt)

echo.
echo  Lucy's files folder has test leftovers from chats (folders a, anymore, anymorethere, trash,
echo  and number.like). The hourly tests copy this folder, so extra files there can confuse them.
set ok=
set /p ok= Empty files\ except welcome.txt? (Y/N):
if /i "%ok%"=="Y" call :cleanfiles

echo.
echo  mind_original\ keeps older brains as backups. The current brain is mind_original\start.web.
echo    start_before_training.*  the very first brain, before round 1
echo    start_before_round2.*    the brain after round 1
echo  Round 3 will save the current brain as start_before_round3.* when it installs its result.
set ok=
set /p ok= Delete these two older backups? (Y/N):
if /i "%ok%"=="Y" call :cleanbrains

echo.
echo  Done.
goto :end

:cleanfiles
if not exist files\welcome.txt (echo  files\welcome.txt is missing, so files\ was left alone. & goto :eof)
for /d %%D in (files\*) do (rmdir /s /q "%%D" & echo  removed %%D\)
for %%F in (files\*) do call :delkeep "%%F"
goto :eof

:cleanbrains
for %%F in (mind_original\start_before_training.web mind_original\start_before_training.voc mind\language\words_before_training.lex mind_original\start_before_round2.web mind_original\start_before_round2.voc mind\language\words_before_round2.lex) do call :delone "%%F"
goto :eof

:delkeep
if /i "%~nx1"=="welcome.txt" goto :eof
:delone
if not exist "%~1" goto :eof
del /q "%~1"
echo  removed %~1
goto :eof

:notcode
echo  This file must be in Lucy's folder (next to train_round.bat). Nothing was deleted.

:end
echo.
pause
