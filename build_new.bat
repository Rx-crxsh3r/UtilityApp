@echo off
echo ========================================
echo Building UtilityApp with Modular Architecture and Feature Modules...
echo ========================================
echo.

REM Navigate to project root
cd /d "%~dp0"

echo [1/5] Compiling resources...
windres resources\resources.rc -o resources.o
if %errorlevel% neq 0 (
    echo ERROR: Failed to compile resources
    pause
    exit /b 1
)

echo [2/5] Compiling core source files...
g++ -c src\main.cpp -o main.o -std=c++17 -Isrc -Isrc\features
g++ -c src\input_blocker.cpp -o input_blocker.o -std=c++17 -Isrc
g++ -c src\tray_icon.cpp -o tray_icon.o -std=c++17 -Isrc
g++ -c src\failsafe.cpp -o failsafe.o -std=c++17 -Isrc
g++ -c src\settings.cpp -o settings.o -std=c++17 -Isrc -Isrc\features
if %errorlevel% neq 0 (
    echo ERROR: Failed to compile core source files
    pause
    exit /b 1
)

echo [3/5] Compiling settings components...
g++ -c src\settings\settings_core.cpp -o settings_core.o -std=c++17 -Isrc -Isrc\features
g++ -c src\settings\hotkey_manager.cpp -o hotkey_manager.o -std=c++17 -Isrc
g++ -c src\settings\overlay_manager.cpp -o overlay_manager.o -std=c++17 -Isrc
if %errorlevel% neq 0 (
    echo ERROR: Failed to compile settings components
    pause
    exit /b 1
)

echo [4/5] Compiling feature modules...
g++ -c src\features\privacy\privacy_manager.cpp -o privacy_manager.o -std=c++17 -Isrc -Isrc\features
g++ -c src\features\productivity\productivity_manager.cpp -o productivity_manager.o -std=c++17 -Isrc -Isrc\features
if %errorlevel% neq 0 (
    echo ERROR: Failed to compile feature modules
    pause
    exit /b 1
)

echo [5/5] Linking final executable...
g++ -o UtilityApp.exe resources.o main.o input_blocker.o tray_icon.o failsafe.o settings.o settings_core.o hotkey_manager.o overlay_manager.o privacy_manager.o productivity_manager.o -static-libgcc -static-libstdc++ -std=c++17 -mwindows -luser32 -lshell32 -ladvapi32 -lcomctl32 -lcomdlg32 -lgdi32 -lole32 -ldwmapi -lcrypt32 -lsetupapi -lcfgmgr32
if %errorlevel% neq 0 (
    echo ERROR: Failed to link executable
    pause
    exit /b 1
)

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo Created: UtilityApp.exe
echo Build completed at: %date% %time%
echo.
pause
