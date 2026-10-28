@echo off
rem =====================================================================
rem  Builds the x86-64 Windows release configuration with clang-cl,
rem  AVX2 code generation and ThinLTO (or the PGO preset).
rem
rem  Usage: build_optimized.bat [x64-clang-release|x64-clang-pgo]
rem =====================================================================
setlocal
cd /d "%~dp0"

set PRESET=%~1
if "%PRESET%"=="" set PRESET=x64-clang-release

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo Error: vswhere.exe not found. Visual Studio might not be installed.
    exit /b 1
)

set "VS_PATH="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)

if "%VS_PATH%"=="" (
    echo Error: Could not find a Visual Studio installation with C++ tools.
    exit /b 1
)

call "%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul
if errorlevel 1 exit /b 1

cmake --preset %PRESET%
if errorlevel 1 exit /b 1

cmake --build --preset %PRESET% -j 8
if errorlevel 1 exit /b 1

echo Success: build completed (%PRESET%).
