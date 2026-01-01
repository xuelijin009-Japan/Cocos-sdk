#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "VideoCallPlugin.h"

namespace cocos2d {
class Sprite;
class Texture2D;
}

namespace videocall {

// 前向声明
class AgoraRtcWrapper;

// 简单日志工具，用于插件内部调试和沙盒环境日志收集
class Logger {
public:
    static Logger& instance();

    void setLogToFile(bool enable, const std::string& path = "");

    void logInfo(const std::string& msg);
    void logWarn(const std::string& msg);
    void logError(const std::string& msg);

private:
    Logger();
    ~Logger();

    void write(const char* level, const std::string& msg);

    bool logToFile_;
    std::string filePath_;
    std::mutex mutex_;
};

// 频道和用户状态管理
class ChannelManager {
public:
    ChannelManager();

    int joinChannel(const std::string& channelName,
                    const std::string& userId,
                    const std::string& token,
                    AgoraRtcWrapper* engine,
                    IVideoCallCallback* cb);

    int leaveChannel(AgoraRtcWrapper* engine, IVideoCallCallback* cb);

    int reconnect(AgoraRtcWrapper* engine, IVideoCallCallback* cb);

    int getOnlineUsers(const char** userList, int* listSize);

    void onRemoteUserJoined(const std::string& userId, IVideoCallCallback* cb);
    void onRemoteUserLeft(const std::string& userId, int reason, IVideoCallCallback* cb);

    const std::string& getCurrentChannel() const { return channelName_; }
    const std::string& getLocalUserId() const { return localUserId_; }

private:
    std::string channelName_;
    std::string localUserId_;
    std::set<std::string> onlineUsers_;
    std::mutex mutex_;
};

// 音视频控制
class AVController {
public:
    AVController();

    int enableLocalCamera(bool enable, AgoraRtcWrapper* engine, IVideoCallCallback* cb);
    int switchCamera(AgoraRtcWrapper* engine);

    int enableLocalMicrophone(bool enable, AgoraRtcWrapper* engine, IVideoCallCallback* cb);
    int setLocalVolume(int volume, AgoraRtcWrapper* engine);

    int subscribeRemoteVideo(const std::string& userId, bool subscribe, AgoraRtcWrapper* engine, IVideoCallCallback* cb);
    int subscribeRemoteAudio(const std::string& userId, bool subscribe, AgoraRtcWrapper* engine, IVideoCallCallback* cb);
};

// 视频渲染器：管理 Cocos Sprite 与视频帧的映射关系
class VideoRenderManager {
public:
    VideoRenderManager();

    int initRenderer(cocos2d::Sprite* sprite, const std::string& userId);
    int setRenderParams(const std::string& userId,
                        float x,
                        float y,
                        float width,
                        float height,
                        int alpha);
    int destroyRenderer(const std::string& userId);

    void onVideoFrame(const std::string& userId,
                      const unsigned char* data,
                      int width,
                      int height,
                      int format);

private:
    struct RenderTarget {
        cocos2d::Sprite* sprite{nullptr};
        cocos2d::Texture2D* texture{nullptr};
    };

    std::map<std::string, RenderTarget> targets_;
    std::mutex mutex_;
};

// 权限管理（移动端有实现，PC 端直接返回已授权）
class PermissionManager {
public:
    static PermissionManager& instance();

    int requestCamera(IVideoCallCallback* cb);
    int requestMicrophone(IVideoCallCallback* cb);

    int checkCameraState() const;  // 0-已授权，1-未授权，2-申请中
    int checkMicrophoneState() const;

private:
    PermissionManager();

    enum State { Granted = 0, Denied = 1, Pending = 2 };

    mutable std::mutex mutex_;
    State cameraState_;
    State micState_;
};

// 声网 RTC 封装抽象（实际对接声网 SDK 时只需在实现中调用 Agora API）
class AgoraRtcWrapper {
public:
    AgoraRtcWrapper();
    ~AgoraRtcWrapper();

    int initialize(const std::string& appId, bool debugMode);
    int destroy();

    int joinChannel(const std::string& channelName,
                    const std::string& userId,
                    const std::string& token);
    int leaveChannel();

    int enableLocalVideo(bool enable);
    int enableLocalAudio(bool enable);
    int switchCamera();
    int setLocalVolume(int volume);

    int subscribeRemoteVideo(const std::string& userId, bool subscribe);
    int subscribeRemoteAudio(const std::string& userId, bool subscribe);

    // TODO: 这里可以增加设置编码参数、弱网控制等接口

private:
    bool initialized_;
    void* engineHandle_;
};

// 核心单例，连接 C 接口与内部模块
class VideoCallCore {
public:
    static VideoCallCore& instance();

    int init(const std::string& appId, bool debugMode, IVideoCallCallback* cb);
    int destroy();

    int getState() const;

    int joinChannel(const std::string& channelName,
                    const std::string& userId,
                    const std::string& token);
    int leaveChannel();
    int reconnectChannel();

    int getOnlineUsers(const char** userList, int* listSize);

    int enableLocalCamera(bool enable);
    int switchCamera();

    int enableLocalMicrophone(bool enable);
    int setLocalVolume(int volume);

    int subscribeRemoteVideo(const std::string& userId, bool subscribe);
    int subscribeRemoteAudio(const std::string& userId, bool subscribe);

    int initRenderer(cocos2d::Sprite* sprite, const std::string& userId);
    int setRendererParams(const std::string& userId,
                          float x,
                          float y,
                          float width,
                          float height,
                          int alpha);
    int destroyRenderer(const std::string& userId);

    void onVideoFrameInternal(const std::string& userId,
                              const unsigned char* data,
                              int width,
                              int height,
                              int format);

    IVideoCallCallback* getCallback() const { return callback_; }

private:
    VideoCallCore();
    ~VideoCallCore();

    mutable std::mutex mutex_;
    std::unique_ptr<AgoraRtcWrapper> engine_;
    ChannelManager channelMgr_;
    AVController avController_;
    VideoRenderManager renderMgr_;
    IVideoCallCallback* callback_;
    RtcEngineState state_;
};

} // namespace videocall
