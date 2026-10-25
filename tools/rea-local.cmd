@echo off
setlocal
if not defined REA_JADX_HEAP_MIB set "REA_JADX_HEAP_MIB=768"
set "REA_JADX_MCP_JAR=%~dp0..\.tools\jadx\jadx-headless-mcp-0.7.1-all.jar"
if not exist "%REA_JADX_MCP_JAR%" (
  echo Missing JADX JAR: %REA_JADX_MCP_JAR% 1>&2
  exit /b 1
)
call rea.cmd %*
exit /b %errorlevel%
