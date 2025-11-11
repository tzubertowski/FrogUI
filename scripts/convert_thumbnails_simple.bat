@echo off
REM Simple thumbnail converter using Python/Pillow
REM Usage: convert_thumbnails_simple.bat [roms_directory]

set "ROMS_DIR=%~1"
if "%ROMS_DIR%"=="" set "ROMS_DIR=roms"

REM Check if directory exists
if exist "%ROMS_DIR%\." goto dir_ok
echo Error: ROMS directory '%ROMS_DIR%' not found
echo Usage: %0 [roms_directory]
pause
exit /b 1

:dir_ok
REM Check if Python is installed
python --version >nul 2>&1
if errorlevel 1 goto no_python
goto check_pillow

:no_python
echo Error: Python not found!
echo Please install Python 3 from: https://www.python.org/downloads/
echo Make sure 'python' command is in your PATH
pause
exit /b 1

:check_pillow
REM Check if PIL/Pillow is installed
python -c "from PIL import Image" >nul 2>&1
if errorlevel 1 goto install_pillow
goto run_conversion

:install_pillow
echo Error: Pillow not found!
echo Installing Pillow...
python -m pip install Pillow
if errorlevel 1 goto pillow_failed
goto run_conversion

:pillow_failed
echo Failed to install Pillow
pause
exit /b 1

:run_conversion
REM Get the directory where this script is located
set "SCRIPT_DIR=%~dp0"

REM Call the Python conversion script
python "%SCRIPT_DIR%convert_to_rgb565.py" "%ROMS_DIR%"

if errorlevel 1 goto conversion_failed

echo.
echo Press any key to close...
pause >nul
exit /b 0

:conversion_failed
echo Conversion failed!
pause
exit /b 1
