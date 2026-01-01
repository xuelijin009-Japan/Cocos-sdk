cmake_minimum_required(VERSION 3.20)
project(VideoCallPlugin CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 插件核心源文件
set(VIDEOCALL_CORE_SOURCES
    src/VideoCallPlugin.cpp
    src/core/VideoCallCore.cpp
)

set(VIDEOCALL_CORE_HEADERS
    include/VideoCallPlugin.h
    src/core/VideoCallCore.h
)

# 头文件搜索路径
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 生成静态库，游戏工程可按需改为 SHARED
add_library(VideoCallPlugin STATIC
    ${VIDEOCALL_CORE_SOURCES}
    ${VIDEOCALL_CORE_HEADERS}
)

# 示例：如需链接声网 C++ SDK，可在此添加：
# target_link_libraries(VideoCallPlugin PRIVATE agora_rtc_sdk)

# 测试/沙盒可执行程序（可选）
add_executable(VideoCallSandbox
    test/sandbox/SandboxMain.cpp
    test/sandbox/SandboxConfig.cpp
)

target_link_libraries(VideoCallSandbox PRIVATE VideoCallPlugin)
