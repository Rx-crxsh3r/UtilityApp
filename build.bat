@echo off
echo ========================================
echo Building UtilityApp with Modular Architecture and Feature Modules...
echo ========================================

REM Navigate to project root
cd /d "%~dp0"

REM Create build directory if it doesn't exist
if not exist build mkdir build

echo [1/5] Compiling resources...
windres resources\resources.rc -o build\resources.o
if %errorlevel% neq 0 (
    echo ERROR: Failed to compile resources
    pause
    exit /b 1
)

echo [2/5] Compiling core modules...
gcc -c src\main.cpp -o build\main.o
gcc -c src\failsafe.cpp -o build\failsafe.o
gcc -c src\input_blocker.cpp -o build\input_blocker.o
gcc -c src\tray_icon.cpp -o build\tray_icon.o
gcc -c src\audio_manager.cpp -o build\audio_manager.o
gcc -c src\custom_notifications.cpp -o build\custom_notifications.o
gcc -c src\notifications.cpp -o build\notifications.o
gcc -c src\overlay.cpp -o build\overlay.o
gcc -c src\features\lock_input\lock_input_tab.cpp -o build\lock_input_tab.o
gcc -c src\ui\productivity_tab.cpp -o build\productivity_tab.o
gcc -c src\ui\privacy_tab.cpp -o build\privacy_tab.o
gcc -c src\features\appearance\appearance_tab.cpp -o build\appearance_tab.o
gcc -c src\features\data_management\data_tab.cpp -o build\data_tab.o
if %errorlevel% neq 0 (
    echo ERROR: Failed to compile core modules
    pause
    exit /b 1
)

echo [3/5] Compiling settings system...
gcc -c src\settings.cpp -o build\settings.o
gcc -c src\features\lock_input\hotkey_manager.cpp -o build\hotkey_manager.o
gcc -c src\features\appearance\overlay_manager.cpp -o build\overlay_manager.o
gcc -c src\features\lock_input\password_manager.cpp -o build\password_manager.o
gcc -c src\settings\settings_core.cpp -o build\settings_core.o
gcc -c src\features\lock_input\timer_manager.cpp -o build\timer_manager.o
if %errorlevel% neq 0 (
    echo ERROR: Failed to compile settings system
    pause
    exit /b 1
)

echo [4/5] Compiling feature modules...
gcc -c src\features\privacy\privacy_manager.cpp -o build\privacy_manager.o
gcc -c src\features\productivity\productivity_manager.cpp -o build\productivity_manager.o
if %errorlevel% neq 0 (
    echo ERROR: Failed to compile feature modules
    pause
    exit /b 1
)

echo [5/5] Linking executable...
gcc -o UtilityApp.exe ^
    build\main.o ^
    build\failsafe.o ^
    build\input_blocker.o ^
    build\tray_icon.o ^
    build\audio_manager.o ^
    build\custom_notifications.o ^
    build\notifications.o ^
    build\overlay.o ^
    build\lock_input_tab.o ^
    build\productivity_tab.o ^
    build\privacy_tab.o ^
    build\appearance_tab.o ^
    build\data_tab.o ^
    build\settings.o ^
    build\hotkey_manager.o ^
    build\overlay_manager.o ^
    build\password_manager.o ^
    build\settings_core.o ^
    build\timer_manager.o ^
    build\privacy_manager.o ^
    build\productivity_manager.o ^
    build\resources.o ^
    -static-libgcc -static-libstdc++ -std=c++17 -mwindows -lgdi32 -luser32 -lshell32 -ladvapi32 -lcomctl32 -lstdc++ -lwinmm -lmsimg32 -ldwmapi

if %errorlevel% neq 0 (
    echo ERROR: Failed to link executable
    pause
    exit /b 1
)

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo.
echo Modular UtilityApp has been built successfully!
echo.
echo Features compiled:
echo  ✓ Core input blocking system
echo  ✓ Privacy management features  
echo  ✓ Productivity enhancement tools
echo  ✓ Advanced settings system
echo  ✓ Custom notifications
echo  ✓ Audio feedback system
echo  ✓ Overlay management
echo  ✓ Tray icon integration
echo  ✓ Failsafe mechanisms
echo.
echo Created: UtilityApp.exe
echo Build completed at: %date% %time%
echo.
echo The application is ready to run!
pause
