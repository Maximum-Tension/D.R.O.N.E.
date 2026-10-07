@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"
title Lucy - training round

rem ================= settings you can change =================
rem HOURS   how long to train (one test after every hour). 3-4 is best: longer rounds
rem         start memorizing the chat data instead of getting better at talking
rem THREADS how many CPU threads to use (default: all of them)
set HOURS=6
set THREADS=%NUMBER_OF_PROCESSORS%
rem ROUND   bump this number to start a new round from the installed brain
set ROUND=10
rem ============================================================

set CHUNKMIN=60
if defined QUICK set CHUNKMIN=%QUICK%
set /a CHUNKS=HOURS
if %CHUNKS% LSS 1 set CHUNKS=1
set /a ANNEAL=1
if %CHUNKS% LSS 2 set ANNEAL=0
set /a MAIN=CHUNKS-ANNEAL
rem steady hours: PersonaChat is mostly memorized, so each hour uses a different 15%% of it
set FILES=..\data\skills_sense.txt ..\data\sense_live.txt ..\data\tasks_honest.txt@0.25 ..\data\persona_chat_clean.txt@0.15 ..\data\daily_dialog_clean.txt ..\data\topical_part_clean.txt ..\data\squad_qa.txt
rem cool-down hour: mostly the assistant's own voice (skills and honest replies)
set COOLFILES=..\data\skills_sense.txt ..\data\sense_live.txt ..\data\tasks_honest.txt@0.25 ..\data\persona_chat_clean.txt@0.05 ..\data\daily_dialog_clean.txt@0.5 ..\data\topical_part_clean.txt@0.5 ..\data\squad_qa.txt@0.5
set LR=0.0002

echo.
echo  Lucy - training round
echo  =====================
echo  1. build Lucy (ai.exe) and run the engine tests
echo  2. check the training data (python)
echo  3. start from the installed brain in mind_original\start.web
echo  4. train for %HOURS% hour(s) on %THREADS% threads with fresh practice conversations every hour (last hour: cool-down),
echo     test after every hour, keep the best
echo  5. install the best brain and pack trained_brain.zip for you to send back
echo.
echo  You can stop any time with Ctrl+C. Run this file again to continue where it stopped.
echo  Keep the computer plugged in and awake.
echo.

where gcc >nul 2>nul
if errorlevel 1 (echo  gcc was not found. Install MinGW-w64 and add its bin folder to PATH. & goto :fail)
where python >nul 2>nul
if errorlevel 1 (echo  python was not found. Install Python 3 and tick "Add python.exe to PATH". & goto :fail)

if not defined NOBUILD (
    echo [1/5] building...
    call build.bat
    if errorlevel 1 goto :fail
    echo       running engine tests...
    ai-test.exe > engine_test.txt 2>&1
    findstr /c:"ALL ENGINE TESTS PASSED" engine_test.txt >nul
    if errorlevel 1 (echo  The engine tests failed. Send me engine_test.txt. & goto :fail)
    echo       engine tests passed.
)

if not exist data\tasks_honest.txt (
    echo [2/5] making the real-request data, this downloads about 600 MB once...
    python tools\prep_tasks.py data\tasks_honest.txt data\_downloads
    if errorlevel 1 goto :fail
) else (
    echo [2/5] training data is there.
)
python tools\rename_ai.py
if errorlevel 1 goto :fail

set WD=work_round%ROUND%
if not exist %WD% mkdir %WD%
rem a round started before the mind folders: its brain moves over
if exist %WD%\memory\global\brain.web if not exist %WD%\mind\memory\general\brain.web (
    mkdir %WD%\mind\memory\general 2>nul
    move /y %WD%\memory\global\*.* %WD%\mind\memory\general >nul
)
if exist %WD%\lang\words.lex if not exist %WD%\mind\language\words.lex (
    mkdir %WD%\mind\language 2>nul
    move /y %WD%\lang\words.lex %WD%\mind\language\words.lex >nul
)
if not exist %WD%\mind_original mkdir %WD%\mind_original
if not exist %WD%\mind\language mkdir %WD%\mind\language
if not exist %WD%\ckpt mkdir %WD%\ckpt
set RESHAPE=
if not exist %WD%\mind\memory\general\brain.web (
    echo [3/5] starting from the brain in mind_original\start.web
    copy /y mind_original\start.web %WD%\mind_original\start.web >nul
    copy /y mind_original\start.voc %WD%\mind_original\start.voc >nul
    copy /y mind\language\words.lex %WD%\mind\language\words.lex >nul
    >%WD%\done.txt echo 0
) else (
    echo [3/5] continuing the brain in %WD%\mind\memory\general
)
if not exist %WD%\done.txt >%WD%\done.txt echo 0

cd %WD%
:loop
set /p DONE=<done.txt
set /a DONE=DONE
if !DONE! GEQ %CHUNKS% goto :finish
set /a NEXT=DONE+1
if !NEXT! LEQ %MAIN% (set LRMODE=steady& set USEFILES=%FILES%) else (set LRMODE=cool& set USEFILES=%COOLFILES%)
set /a SEED=ROUND*1000+NEXT
echo [4/5] hour !NEXT! of %CHUNKS%: new practice conversations (seed !SEED!), about 2 minutes...
python ..\tools\gen_skills.py --train ..\data\skills_live.txt --eval ..\data\_unused_eval.txt --convs 200000 --eval-convs 0 --seed !SEED! > gen_log.txt 2>&1
if errorlevel 1 (echo  making practice data failed, see %WD%\gen_log.txt & cd .. & goto :fail)
pushd ..
ai.exe --sense-filter data\skills_live.txt data\skills_sense.txt > %WD%\sense_log.txt 2>&1
python tools\gen_sense.py --out data\sense_live.txt --convs 40000 --seed !SEED! >> %WD%\sense_log.txt 2>&1
popd
if not exist ..\data\sense_live.txt (echo  making the thought practice data failed, see %WD%\sense_log.txt & cd .. & goto :fail)
if "!LRMODE!"=="cool" (echo       training %CHUNKMIN% minutes, learning rate cooling down from %LR% to almost 0   ^(log: %WD%\train_log.txt^)) else (echo       training %CHUNKMIN% minutes, steady learning rate %LR%   ^(log: %WD%\train_log.txt^))
..\ai-train.exe !RESHAPE! --rope --lr %LR% --lr-mode !LRMODE! --data-seed !SEED! --minutes %CHUNKMIN% --save-every 20 --threads %THREADS% --word-budget 20000 --max-params 20000000 --valid ..\data\valid.txt --valid-file vperp.txt !USEFILES! >> train_log.txt 2>&1
if errorlevel 1 (echo  training stopped with an error, see %WD%\train_log.txt & cd .. & goto :fail)
if not exist mind\memory\general\brain.web (echo  no brain was saved, see %WD%\train_log.txt & cd .. & goto :fail)
set RESHAPE=
copy /y mind\memory\general\brain.web ckpt\brain_!NEXT!.web >nul
copy /y mind\memory\general\vocab.map ckpt\vocab_!NEXT!.map >nul
copy /y mind\language\words.lex ckpt\words_!NEXT!.lex >nul
call :score !NEXT!
>done.txt echo !NEXT!
goto :loop

:score
set CK=%1
echo       testing checkpoint %CK% on the held-out tests, about 5-10 minutes (it prints when done)...
if exist evaldir rmdir /s /q evaldir
mkdir evaldir\mind\memory\general
mkdir evaldir\mind\language
mkdir evaldir\mind\skills
mkdir evaldir\mind_original
mkdir evaldir\files
copy /y ..\files\*.* evaldir\files >nul
copy /y ..\mind_original\self.txt evaldir\mind_original >nul
copy /y ..\mind\language\*.txt evaldir\mind\language >nul
copy /y ..\mind\memory\*.mem evaldir\mind\memory >nul
copy /y ..\mind\memory\catalog.txt evaldir\mind\memory >nul
copy /y ..\mind\skills\*.dll evaldir\mind\skills >nul
copy /y ckpt\words_%CK%.lex evaldir\mind\language\words.lex >nul
copy /y ckpt\brain_%CK%.web evaldir\mind\memory\general\brain.web >nul
copy /y ckpt\vocab_%CK%.map evaldir\mind\memory\general\vocab.map >nul
pushd evaldir
if exist ..\ckpt\score_%CK%.num del ..\ckpt\score_%CK%.num
if exist ..\ckpt\user_%CK%.num del ..\ckpt\user_%CK%.num
..\..\ai.exe --eval ..\..\data\eval_skills.txt --seed 1 --score-file ..\ckpt\score_%CK%.num > ..\ckpt\score_%CK%.txt 2>&1
..\..\ai.exe --eval ..\..\data\eval_user.txt --seed 1 --score-file ..\ckpt\user_%CK%.num > ..\ckpt\user_%CK%.txt 2>&1
popd
set SC=0
if exist ckpt\score_%CK%.num set /p SC=<ckpt\score_%CK%.num
set /a SC=SC
set US=0
if exist ckpt\user_%CK%.num set /p US=<ckpt\user_%CK%.num
set /a US=US
set VP=0
if exist vperp.txt set /p VP=<vperp.txt
set /a VP=VP
set /a PW=SC/10
set /a PF=SC%%10
set /a UW=US/10
set /a UF=US%%10
set /a VW=VP/10
set /a VF=VP%%10
echo       hour %CK%: skill test !PW!.!PF!%%   your test !UW!.!UF!%%   unseen-chat perplexity !VW!.!VF! (lower is better)
>> scores.txt echo hour %CK%: skill test !PW!.!PF!%%   your test !UW!.!UF!%%   unseen-chat perplexity !VW!.!VF!
if not exist best_perp.txt (>best_perp.txt echo 999999)
set /p BP=<best_perp.txt
set /a BP=BP
if !VP! GTR 0 if !VP! LSS !BP! (>best_perp.txt echo !VP!& set BP=!VP!)
rem a candidate may be at most 5%% worse on unseen chat than the best hour
set /a LIM=BP*105/100
set OKP=1
if !VP! GTR !LIM! set OKP=0
if !OKP!==0 echo       (not a candidate: it got worse on unseen chat, which means it started memorizing)
if not exist best_score.txt (>best_score.txt echo 0)
set /p BEST=<best_score.txt
set /a BEST=BEST
set /a TOT=SC+US
if !OKP!==1 if !TOT! GTR !BEST! (
    >best_score.txt echo !TOT!
    >best_chunk.txt echo %CK%
    echo       this is the best so far.
)
goto :eof

:finish
cd ..
if not exist %WD%\best_chunk.txt (echo  nothing was trained yet & goto :fail)
set /p BC=<%WD%\best_chunk.txt
set /a BC=BC
echo [5/5] installing the best brain (hour !BC!)
if not exist mind_original\start_before_round%ROUND%.web (
    copy /y mind_original\start.web mind_original\start_before_round%ROUND%.web >nul
    copy /y mind_original\start.voc mind_original\start_before_round%ROUND%.voc >nul
    copy /y mind\language\words.lex mind\language\words_before_round%ROUND%.lex >nul
)
copy /y %WD%\ckpt\brain_!BC!.web mind_original\start.web >nul
copy /y %WD%\ckpt\vocab_!BC!.map mind_original\start.voc >nul
copy /y %WD%\ckpt\words_!BC!.lex mind\language\words.lex >nul
if exist mind\memory\general\brain.web (
    move /y mind\memory\general\brain.web mind\memory\general\brain_before_training.web >nul
    if exist mind\memory\general\vocab.map move /y mind\memory\general\vocab.map mind\memory\general\vocab_before_training.map >nul
    if exist mind\memory\general\brain.web.opt del mind\memory\general\brain.web.opt
)
if exist trained_brain.zip del trained_brain.zip
powershell -NoProfile -Command "Compress-Archive -Force -Path 'mind_original\start.web','mind_original\start.voc','mind\language\words.lex','%WD%\scores.txt','%WD%\train_log.txt' -DestinationPath 'trained_brain.zip'" >nul 2>nul
echo.
echo  Done. Scores per hour:
type %WD%\scores.txt
echo.
echo  The best brain is installed: run chat.bat to talk to it.
echo  Send me trained_brain.zip (about 25 MB) so I can check it too.
echo  To train longer, raise HOURS at the top of this file and run it again.
echo.
pause
exit /b 0

:fail
echo.
echo  Stopped. Nothing is lost: run train_round.bat again to continue.
echo.
pause
exit /b 1
