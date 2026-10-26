@echo off
setlocal
if not exist "%~dp0..\external\environment.cmd" (
  echo Local tools are not installed. Run tools\setup-tools.ps1 in PowerShell. 1>&2
  exit /b 1
)
if "%~1"=="" (
  echo Usage: tools\with-tools.cmd command [arguments] 1>&2
  exit /b 2
)
call "%~dp0..\external\environment.cmd"
call %*
exit /b %errorlevel%
