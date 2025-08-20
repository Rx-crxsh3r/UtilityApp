@echo off
echo Cleaning build artifacts...
echo.

REM Navigate to project root
cd /d "%~dp0"

REM Clean source directory
cd src
del /q *.o 2>nul
del /q *.exe 2>nul
echo Cleaned src directory

REM Clean resources directory
cd ..\resources
del /q *.o 2>nul
echo Cleaned resources directory

echo.
echo Cleanup completed!
pause
