#pragma once

#include "cocos2d.h"
#include "VideoCallPlugin.h"

// 简单的 Cocos-2d-x 测试场景，用于验证插件核心功能：
// - 初始化引擎
// - 加入频道
// - 开启本地音视频
// - 将本地视频渲染到 Sprite 上
// 实际项目中可以在此基础上扩展多用户分屏渲染、悬浮窗等能力。
class VideoCallTestScene : public cocos2d::Scene, public IVideoCallCallback {
public:
    static cocos2d::Scene* createScene();

    virtual bool init() override;

    CREATE_FUNC(VideoCallTestScene);

    // IVideoCallCallback 接口实现
    void onRtcEngineInitResult(int code, const char* msg) override;
    void onRtcEngineDestroyed() override;
    void onJoinChannelResult(int code, const char* channelName, const char* userId) override;
    void onLeaveChannelResult(int code) override;
    void onRemoteUserJoined(const char* userId) override;
    void onRemoteUserLeft(const char* userId, int reason) override;
    void onLocalVideoPublished(int code) override;
    void onLocalAudioPublished(int code) override;
    void onRemoteVideoSubscribed(int code, const char* userId) override;
    void onRemoteAudioSubscribed(int code, const char* userId) override;
    void onVideoFrameReceived(const char* userId,
                              const unsigned char* data,
                              int width,
                              int height,
                              int format) override;
    void onPermissionResult(int type, int code) override;
    void onError(int code, const char* msg) override;

private:
    void setupUI();
    void logMessage(const std::string& msg);

    cocos2d::Label* logLabel_{nullptr};
    cocos2d::Sprite* localVideoSprite_{nullptr};
};
