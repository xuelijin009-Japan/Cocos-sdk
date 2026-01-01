#include <chrono>
#include <thread>

#include "SandboxConfig.h"
#include "src/core/VideoCallCore.h"

using namespace videocall;

// 简易异常网络模拟：在调用关键 API 前后增加 sleep / 随机丢弃操作即可。
static void applyNetworkSimulation(const SandboxConfig& cfg) {
    if (!cfg.enableNetworkSimulation) return;
    if (cfg.latencyMs > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cfg.latencyMs));
    }
    // 丢包模拟留给具体业务（例如随机跳过部分订阅/推流调用），此处保留扩展点。
}

int main(int argc, char** argv) {
    std::string cfgPath = "sandbox_config.json";
    if (argc > 1) {
        cfgPath = argv[1];
    }

    SandboxConfig cfg = SandboxConfigLoader::loadFromFile(cfgPath);
    if (cfg.appId.empty()) {
        Logger::instance().logError("Sandbox appId empty, abort.");
        return -1;
    }

    if (!cfg.logFilePath.empty()) {
        Logger::instance().setLogToFile(true, cfg.logFilePath);
    }

    Logger::instance().logInfo("Sandbox starting with channel: " + cfg.channelName);

    // 沙盒环境中使用一个简单的回调实现，打印关键事件
    class SandboxCallback : public IVideoCallCallback {
    public:
        void onRtcEngineInitResult(int code, const char* msg) override {
            Logger::instance().logInfo("onRtcEngineInitResult code=" + std::to_string(code) + " msg=" + (msg ? msg : ""));
        }
        void onRtcEngineDestroyed() override {
            Logger::instance().logInfo("onRtcEngineDestroyed");
        }
        void onJoinChannelResult(int code, const char* channelName, const char* userId) override {
            Logger::instance().logInfo("onJoinChannelResult code=" + std::to_string(code));
        }
        void onLeaveChannelResult(int code) override {
            Logger::instance().logInfo("onLeaveChannelResult code=" + std::to_string(code));
        }
        void onRemoteUserJoined(const char* userId) override {
            Logger::instance().logInfo(std::string("remote joined: ") + (userId ? userId : ""));
        }
        void onRemoteUserLeft(const char* userId, int reason) override {
            Logger::instance().logInfo(std::string("remote left: ") + (userId ? userId : ""));
        }
        void onLocalVideoPublished(int code) override {
            Logger::instance().logInfo("onLocalVideoPublished code=" + std::to_string(code));
        }
        void onLocalAudioPublished(int code) override {
            Logger::instance().logInfo("onLocalAudioPublished code=" + std::to_string(code));
        }
        void onRemoteVideoSubscribed(int code, const char* userId) override {
            Logger::instance().logInfo("onRemoteVideoSubscribed code=" + std::to_string(code));
        }
        void onRemoteAudioSubscribed(int code, const char* userId) override {
            Logger::instance().logInfo("onRemoteAudioSubscribed code=" + std::to_string(code));
        }
        void onVideoFrameReceived(const char* userId, const unsigned char* data, int width, int height, int format) override {
            (void)userId;
            (void)data;
            (void)width;
            (void)height;
            (void)format;
        }
        void onPermissionResult(int type, int code) override {
            Logger::instance().logInfo("onPermissionResult type=" + std::to_string(type) + " code=" + std::to_string(code));
        }
        void onError(int code, const char* msg) override {
            Logger::instance().logError("onError code=" + std::to_string(code) + " msg=" + (msg ? msg : ""));
        }
    } callback;

    // 初始化引擎
    applyNetworkSimulation(cfg);
    rtcEngine_init(cfg.appId.c_str(), true, &callback);

    // 加入测试频道
    applyNetworkSimulation(cfg);
    channel_join(cfg.channelName.c_str(), cfg.localUserId.c_str(), "");

    // 打开本地音视频
    applyNetworkSimulation(cfg);
    video_enableLocalCamera(true);
    audio_enableLocalMicrophone(true);

    // 简单等待一段时间，模拟对局过程
    Logger::instance().logInfo("Sandbox running... press Ctrl+C to exit");
    std::this_thread::sleep_for(std::chrono::seconds(30));

    // 退出频道并销毁
    channel_leave();
    rtcEngine_destroy();

    Logger::instance().logInfo("Sandbox finished.");
    return 0;
}
