@echo off
setlocal
rem Decode localized compiler dependency output consistently with CMake/Ninja.
chcp 65001 >nul
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo Visual Studio Installer vswhere.exe not found. 1>&2
  exit /b 1
)
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "SAGA_VS=%%I"
if not defined SAGA_VS (
  echo Visual Studio C++ tools not found. 1>&2
  exit /b 1
)
call "%SAGA_VS%\VC\Auxiliary\Build\vcvars64.bat"
if not "%errorlevel%"=="0" exit /b %errorlevel%
set "VSLANG=1033"
set "PATH=%SAGA_VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%SAGA_VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
cmake --fresh -S "%~dp0.." -B "%~dp0..\build\viewer" -G Ninja -DCMAKE_BUILD_TYPE=Release -DSAGA_BUILD_VIEWER=ON -DBUILD_TESTING=ON
if not "%errorlevel%"=="0" exit /b %errorlevel%
rem 7.7 GB RAM host: keep parallel compiler processes bounded (override with CMAKE_BUILD_PARALLEL_LEVEL).
if not defined CMAKE_BUILD_PARALLEL_LEVEL set "CMAKE_BUILD_PARALLEL_LEVEL=4"
cmake --build "%~dp0..\build\viewer"
if not "%errorlevel%"=="0" exit /b %errorlevel%
ctest --test-dir "%~dp0..\build\viewer" --output-on-failure
exit /b %errorlevel%
