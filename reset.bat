@echo off
cd /d "%~dp0"
echo This deletes what Lucy learned (mind\memory\general, persons, locals and topics):
echo everything learned in chats and by train.bat since the last reset.
echo Her knowledge packages (mind\memory\*.mem), skills (mind\skills), language
echo (mind\language) and starting brain (mind_original) stay as they are.
set /p ok=Type YES to reset: 
if /i not "%ok%"=="YES" goto :eof
for %%D in (general persons locals topics) do if exist mind\memory\%%D rmdir /s /q mind\memory\%%D
echo memory reset.
