@echo off
echo Building UtilityApp...
echo.

REM Navigate to project root
cd /d "%~dp0"

REM Compile resources
echo [1/3] Compiling resources...
cd resources
windres resources.rc -o resources.o
if %errorlevel% neq 0 (
    echo ERROR: Failed to compile resources
    pause
    exit /b 1
)

REM Navigate to source directory
cd ..\src

REM Compile source files
echo [2/3] Compiling source files...
g++ -c main.cpp tray_icon.cpp input_blocker.cpp failsafe.cpp notifications.cpp
if %errorlevel% neq 0 (
    echo ERROR: Failed to compile source files
    pause
    exit /b 1
)

REM Link executable
echo [3/3] Linking executable...
REM ✅ ADDED -mwindows FLAG HERE to create a GUI app instead of a console app
g++ main.o tray_icon.o input_blocker.o failsafe.o notifications.o ../resources/resources.o -mwindows -luser32 -lkernel32 -lshell32 -lgdi32 -o utilityapp.exe
if %errorlevel% neq 0 (
    echo ERROR: Failed to link executable
    pause
    exit /b 1
)

echo.
echo SUCCESS: Build completed!
echo Executable created: src\utilityapp.exe
echo.
pause