@echo off
rem =====================================================================
rem  CMake + Visual Studio Dynamic Environment Build Script
rem =====================================================================

set PRESET=x64-release
set BUILD_DIR=out/build/x64-release
set CONFIG=Release

echo [1/4] Searching for Visual Studio installations...

rem Locate the official Microsoft vswhere utility
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo Error: vswhere.exe not found. Visual Studio might not be installed.
    pause
    exit /b 1
)

rem Query vswhere to find the path of the latest Visual Studio installation
set "VS_PATH="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)

if "%VS_PATH%"=="" (
    echo Error: Could not find any Visual Studio installations with C++ tools.
    pause
    exit /b 1
)

echo Found installation at: "%VS_PATH%"

rem =====================================================================
rem [2/4] Loading vcvars environment variables
rem =====================================================================
echo Loading x64 Native Developer Tools environment...

set "VCVARS_BAT=%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat"
if not exist "%VCVARS_BAT%" (
    echo Error: vcvarsall.bat not found at "%VCVARS_BAT%"
    pause
    exit /b 1
)

rem Call the batch file using x64 architecture targets
call "%VCVARS_BAT%" x64
if %errorlevel% neq 0 (
    echo Error: Failed to load vcvars environment.
    pause
    exit /b %errorlevel%
)

rem =====================================================================
rem [3/4] Configuring the CMake Project
rem =====================================================================
echo Creating build directory and running configuration...
if not exist %BUILD_DIR% (
    mkdir %BUILD_DIR%
)

rem Using the "Ninja" or standard generator. Because vcvars is loaded,
rem CMake will automatically use the active MSVC compiler variables.
cmake -S . -B %BUILD_DIR% -G "Ninja"
if %errorlevel% neq 0 (
    echo Warning: Ninja generator failed or not installed. Falling back to Visual Studio solution layout...
    pause
    cmake -B %BUILD_DIR%
)

if %errorlevel% neq 0 (
    echo Error: CMake configuration failed.
    pause
    exit /b %errorlevel%
)

rem =====================================================================
rem [4/4] Compiling the Project
rem =====================================================================
echo Building the project (%CONFIG% configuration)...
cmake --build %BUILD_DIR% --config %CONFIG%
if %errorlevel% neq 0 (
    echo Error: Build process failed.
    pause
    exit /b %errorlevel%
)

echo Success: Build completed successfully!
pause
