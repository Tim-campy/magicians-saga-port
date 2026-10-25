@echo off
setlocal
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
if errorlevel 1 exit /b %errorlevel%
set "PATH=%SAGA_VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%SAGA_VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
cmake -S "%~dp0.." -B "%~dp0..\build\viewer" -G Ninja -DCMAKE_BUILD_TYPE=Release -DSAGA_BUILD_VIEWER=ON -DBUILD_TESTING=ON
if errorlevel 1 exit /b %errorlevel%
cmake --build "%~dp0..\build\viewer" --parallel 4
if errorlevel 1 exit /b %errorlevel%
ctest --test-dir "%~dp0..\build\viewer" --output-on-failure
exit /b %errorlevel%
