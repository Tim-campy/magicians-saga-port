@echo off
setlocal
rem CMake and Ninja must decode localized compiler dependency output consistently.
chcp 65001 >nul
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo Visual Studio Installer vswhere.exe not found. Use CMake directly from your developer shell. 1>&2
  exit /b 1
)
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "SAGA_VS=%%I"
if not defined SAGA_VS (
  echo Visual Studio C++ tools not found. 1>&2
  exit /b 1
)
call "%SAGA_VS%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b %errorlevel%
rem vcvars can select the host language; override it for Ninja /showIncludes parsing.
set "VSLANG=1033"
set "PATH=%SAGA_VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%SAGA_VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
cmake --fresh -S "%~dp0.." -B "%~dp0..\build\desktop" -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
if errorlevel 1 exit /b %errorlevel%
cmake --build "%~dp0..\build\desktop"
if errorlevel 1 exit /b %errorlevel%
ctest --test-dir "%~dp0..\build\desktop" --output-on-failure
exit /b %errorlevel%
