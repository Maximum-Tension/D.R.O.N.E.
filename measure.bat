@echo off
setlocal
cd /d "%~dp0"
if not exist ai-train.exe call build.bat
if exist work_measure rmdir /s /q work_measure
mkdir work_measure\mind\memory\general
mkdir work_measure\mind\language
copy /y mind_original\start.web work_measure\mind\memory\general\brain.web >nul
copy /y mind_original\start.voc work_measure\mind\memory\general\vocab.map >nul
copy /y mind\language\words.lex work_measure\mind\language\words.lex >nul
echo Measuring the installed brain (mind_original\start.web). Nothing is changed. About 10-20 minutes...
echo Low perplexity on a file it trains on, but high on unseen chat (valid.txt), means it is memorizing that file.
cd work_measure
..\ai-train.exe --measure --valid ..\data\valid.txt ..\data\persona_chat_clean.txt ..\data\daily_dialog_clean.txt ..\data\topical_part_clean.txt ..\data\squad_qa.txt ..\data\tasks_honest.txt > ..\measure.txt 2>&1
cd ..
rmdir /s /q work_measure
echo.
type measure.txt
echo.
echo Saved in measure.txt - send it to me.
pause
