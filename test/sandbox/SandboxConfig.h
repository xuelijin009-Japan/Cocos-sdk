#pragma once

#include <string>

namespace videocall {

// 沙盒测试配置，用于隔离正式环境与测试环境。
// 通过读取 config.json，设置：
// - 测试 AppID / 频道名 / 用户 ID 列表；
// - 异常网络模拟参数（丢包率、延迟）；
// - 日志输出路径等。
struct SandboxConfig {
    std::string appId;
    std::string channelName;
    std::string localUserId;

    int userCount{2};

    // 异常网络模拟参数
    int packetLossPercent{0};    // 0-50
    int latencyMs{0};            // 0-1000

    // 日志文件路径
    std::string logFilePath;

    // 是否启用弱网/断网模拟
    bool enableNetworkSimulation{false};

    // 从 JSON 字符串解析配置（简化实现，实际项目可对接成熟 JSON 库）
    static SandboxConfig fromJson(const std::string& json);
};

class SandboxConfigLoader {
public:
    // 从文件路径加载配置
    static SandboxConfig loadFromFile(const std::string& path);
};

} // namespace videocall
