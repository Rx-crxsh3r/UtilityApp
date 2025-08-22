@echo off
echo ========================================
echo Cleaning UltraLight UtilityApp Build
echo ========================================
echo.

REM Navigate to project root
cd /d "%~dp0"

REM Clean root directory executables and object files
echo [1/7] Cleaning root directory...
del /q *.exe 2>nul
del /q *.o 2>nul

REM Clean build directory
echo [2/7] Cleaning build directory...
cd build
del /q *.exe 2>nul
del /q *.o 2>nul
cd ..

REM Clean source directory
echo [3/7] Cleaning src directory...
cd src
del /q *.o 2>nul
del /q *.exe 2>nul

REM Clean UI subdirectories
echo [4/7] Cleaning UI directories...
cd ui
del /q *.o 2>nul

cd dialogs
del /q *.o 2>nul
cd ..

cd tabs
del /q *.o 2>nul
cd ..
cd ..

REM Clean settings directory
echo [5/7] Cleaning settings directory...
cd settings
del /q *.o 2>nul
cd ..

REM Clean feature modules
echo [6/7] Cleaning feature modules...
cd features

REM Clean privacy features
cd privacy
del /q *.o 2>nul
cd ..

REM Clean productivity features
cd productivity
del /q *.o 2>nul
cd ..
cd ..

REM Clean resources directory
echo [7/7] Cleaning resources directory...
cd resources
del /q *.o 2>nul
cd ..

echo.
echo ========================================
echo Cleanup Complete!
echo ========================================
echo Removed from all directories:
echo - All .exe executables
echo - All .o object files  
echo - All build artifacts
echo.
echo Cleaned directories:
echo - Root directory
echo - build/
echo - src/
echo - src/ui/
echo - src/ui/dialogs/
echo - src/ui/tabs/
echo - src/settings/
echo - src/features/privacy/
echo - src/features/productivity/
echo - resources/
echo.
echo Clean completed at: %date% %time%
echo.
if "%1" neq "silent" pause
