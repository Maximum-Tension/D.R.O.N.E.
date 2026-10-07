@echo off
rem builds ai.exe (the chat), ai-train.exe, ai-test.exe and the skills in mind\skills
setlocal EnableDelayedExpansion
cd /d "%~dp0"
set CORE=src\JIT.c src\WEB.c src\KERNEL.c src\REFERENCE.c src\LEARN.c src\LANGUAGE.c src\BRAIN.c src\THREAD.c src\DATA.c src\RECIPE.c
if not exist mind\skills mkdir mind\skills
echo building ai.exe ...
gcc -O2 -static -Wl,--stack,16777216 -o ai.exe %CORE% src\MEMORY.c src\CALCULATOR.c src\FILE_SYSTEM.c src\REFLEX.c src\NEAREST_NEIGHBOR.c src\KNOWLEDGE.c src\SENSE.c src\TAUGHT.c src\SOLVE.c src\AGENTS.c src\SKILLS.c src\PACKAGES.c src\SAYINGS.c src\WORK.c src\CHAT.c || goto fail
echo building ai-train.exe ...
gcc -O2 -static -o ai-train.exe %CORE% src\TRAIN.c || goto fail
echo building ai-test.exe ...
gcc -O2 -static -Wl,--stack,16777216 -o ai-test.exe %CORE% src\TEST_KERNEL.c src\TEST_SHAPE.c src\TEST_RECIPE.c src\TEST_SIGNALS.c src\NEAREST_NEIGHBOR.c src\TEST_GROWTH.c src\TEST_TREE.c src\TEST_BRAIN.c src\KNOWLEDGE.c src\TEST_KNOWLEDGE.c src\CALCULATOR.c src\SENSE.c src\TEST_SENSE.c src\TEST_MAIN.c || goto fail
echo building the skills ...
rem src\skills\COMPUTER.c becomes mind\skills\computer.dll
for %%S in (src\skills\*.c) do (
	set NAME=%%~nS
	for %%L in (a b c d e f g h i j k l m n o p q r s t u v w x y z) do set NAME=!NAME:%%L=%%L!
	echo   !NAME!.dll
	gcc -O2 -shared -static-libgcc -o mind\skills\!NAME!.dll %%S || goto fail
)
echo done.
goto :eof
:fail
echo BUILD FAILED
exit /b 1
