@echo off
setlocal

REM 一键启动 Windows 沙盒测试环境
REM 可通过第一个参数传入自定义 config.json 路径

set CFG=sandbox_config.json
if not "%1"=="" (
    set CFG=%1
)

if not exist %CFG% (
    echo Sandbox config not found: %CFG%
    goto :eof
)

VideoCallSandbox.exe %CFG%

endlocal
