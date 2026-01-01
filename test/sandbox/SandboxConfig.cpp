#include "SandboxConfig.h"

#include <fstream>

#include "src/core/VideoCallCore.h"

namespace videocall {

static std::string readAll(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) return std::string();
    return std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}

SandboxConfig SandboxConfig::fromJson(const std::string& json) {
    SandboxConfig cfg;
    // 简化 JSON 解析：仅做字符串查找，正式项目建议使用 nlohmann/json 或其他库。
    auto findValue = [&json](const std::string& key) -> std::string {
        auto pos = json.find(key);
        if (pos == std::string::npos) return "";
        pos = json.find(':', pos);
        if (pos == std::string::npos) return "";
        pos++;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\"')) ++pos;
        size_t end = pos;
        while (end < json.size() && json[end] != '\"' && json[end] != ',' && json[end] != '\n' && json[end] != '}') ++end;
        return json.substr(pos, end - pos);
    };

    auto appId = findValue("appId");
    if (!appId.empty()) cfg.appId = appId;
    auto channel = findValue("channelName");
    if (!channel.empty()) cfg.channelName = channel;
    auto userId = findValue("localUserId");
    if (!userId.empty()) cfg.localUserId = userId;

    auto loss = findValue("packetLossPercent");
    if (!loss.empty()) cfg.packetLossPercent = std::atoi(loss.c_str());
    auto latency = findValue("latencyMs");
    if (!latency.empty()) cfg.latencyMs = std::atoi(latency.c_str());

    auto logPath = findValue("logFilePath");
    if (!logPath.empty()) cfg.logFilePath = logPath;

    auto netSim = findValue("enableNetworkSimulation");
    if (!netSim.empty()) cfg.enableNetworkSimulation = (netSim == "true" || netSim == "1");

    auto userCount = findValue("userCount");
    if (!userCount.empty()) cfg.userCount = std::atoi(userCount.c_str());

    return cfg;
}

SandboxConfig SandboxConfigLoader::loadFromFile(const std::string& path) {
    std::string content = readAll(path);
    if (content.empty()) {
        Logger::instance().logWarn("SandboxConfig file empty or not found: " + path);
        return SandboxConfig();
    }
    return SandboxConfig::fromJson(content);
}

} // namespace videocall
