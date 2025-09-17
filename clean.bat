@echo off
echo ========================================
echo Cleaning UltraLight UtilityApp Build
echo ========================================
echo.

REM Navigate to project root
cd /d "%~dp0"

echo [1/2] Cleaning build directory...
if exist build (
    rmdir /s /q build
    echo Removed build directory and all contents
) else (
    echo Build directory not found - nothing to clean
)

echo [2/2] Cleaning root directory executable...
if exist UtilityApp.exe (
    del UtilityApp.exe
    echo Removed UtilityApp.exe
) else (
    echo UtilityApp.exe not found - nothing to clean
)

echo.
echo ========================================
echo Cleanup Complete!
echo ========================================
echo Cleaned:
echo - build/ directory (all object files and build artifacts)
echo - UtilityApp.exe (from root directory)
echo.
echo Clean completed at: %date% %time%
echo.
if "%1" neq "silent" pause
