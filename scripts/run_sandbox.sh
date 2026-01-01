#!/usr/bin/env bash

# 一键启动 macOS / Linux 沙盒测试环境
# 可通过第一个参数传入自定义 config.json 路径

CFG="sandbox_config.json"
if [ "$1" != "" ]; then
  CFG="$1"
fi

if [ ! -f "$CFG" ]; then
  echo "Sandbox config not found: $CFG"
  exit 1
fi

./VideoCallSandbox "$CFG"
