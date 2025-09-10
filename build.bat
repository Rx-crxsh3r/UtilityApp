@echo off
echo ========================================
echo Building UtilityApp with Modular Architecture and Feature Modules...
echo ========================================

REM Navigate to project root
cd /d "%~dp0"

REM Create obj directory if it doesn't exist
if not exist obj mkdir obj

echo [1/5] Compiling resources...
windres resources\resources.rc -o obj\resources.o
if %errorlevel% neq 0 (
    echo ERROR: Failed to compile resources
    pause
    exit /b 1
)

echo [2/5] Compiling core modules...
gcc -c src\main.cpp -o obj\main.o
gcc -c src\failsafe.cpp -o obj\failsafe.o
gcc -c src\input_blocker.cpp -o obj\input_blocker.o
gcc -c src\tray_icon.cpp -o obj\tray_icon.o
gcc -c src\audio_manager.cpp -o obj\audio_manager.o
gcc -c src\custom_notifications.cpp -o obj\custom_notifications.o
gcc -c src\notifications.cpp -o obj\notifications.o
gcc -c src\overlay.cpp -o obj\overlay.o
gcc -c src\features\lock_input\lock_input_tab.cpp -o obj\lock_input_tab.o
gcc -c src\ui\productivity_tab.cpp -o obj\productivity_tab.o
gcc -c src\ui\privacy_tab.cpp -o obj\privacy_tab.o
gcc -c src\features\appearance\appearance_tab.cpp -o obj\appearance_tab.o
gcc -c src\features\data_management\data_tab.cpp -o obj\data_tab.o
if %errorlevel% neq 0 (
    echo ERROR: Failed to compile core modules
    pause
    exit /b 1
)

echo [3/5] Compiling settings system...
gcc -c src\settings.cpp -o obj\settings.o
gcc -c src\features\lock_input\hotkey_manager.cpp -o obj\hotkey_manager.o
gcc -c src\features\appearance\overlay_manager.cpp -o obj\overlay_manager.o
gcc -c src\features\lock_input\password_manager.cpp -o obj\password_manager.o
gcc -c src\settings\settings_core.cpp -o obj\settings_core.o
gcc -c src\features\lock_input\timer_manager.cpp -o obj\timer_manager.o
if %errorlevel% neq 0 (
    echo ERROR: Failed to compile settings system
    pause
    exit /b 1
)

echo [4/5] Compiling feature modules...
gcc -c src\features\privacy\privacy_manager.cpp -o obj\privacy_manager.o
gcc -c src\features\productivity\productivity_manager.cpp -o obj\productivity_manager.o
if %errorlevel% neq 0 (
    echo ERROR: Failed to compile feature modules
    pause
    exit /b 1
)

echo [5/5] Linking executable...
gcc -o UtilityApp.exe ^
    obj\main.o ^
    obj\failsafe.o ^
    obj\input_blocker.o ^
    obj\tray_icon.o ^
    obj\audio_manager.o ^
    obj\custom_notifications.o ^
    obj\notifications.o ^
    obj\overlay.o ^
    obj\lock_input_tab.o ^
    obj\productivity_tab.o ^
    obj\privacy_tab.o ^
    obj\appearance_tab.o ^
    obj\data_tab.o ^
    obj\settings.o ^
    obj\hotkey_manager.o ^
    obj\overlay_manager.o ^
    obj\password_manager.o ^
    obj\settings_core.o ^
    obj\timer_manager.o ^
    obj\privacy_manager.o ^
    obj\productivity_manager.o ^
    obj\resources.o ^
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
