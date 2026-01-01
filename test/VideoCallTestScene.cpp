#include "VideoCallTestScene.h"

USING_NS_CC;

Scene* VideoCallTestScene::createScene() {
    return VideoCallTestScene::create();
}

bool VideoCallTestScene::init() {
    if (!Scene::init()) {
        return false;
    }

    setupUI();

    std::string testAppId = "YOUR_TEST_APPID";        // 替换为声网测试 AppID
    std::string testChannel = "test_channel";         // 测试频道名
    std::string testUserId = "user_1";                // 测试用户 ID

    int ret = rtcEngine_init(testAppId.c_str(), true, this);
    if (ret != 0) {
        logMessage("rtcEngine_init failed: " + std::to_string(ret));
        return true;
    }

    // 初始化本地视频渲染 Sprite
    localVideoSprite_ = Sprite::create();
    localVideoSprite_->setContentSize(Size(320, 240));
    localVideoSprite_->setPosition(Vec2(200, 200));
    addChild(localVideoSprite_);

    videoRender_init(localVideoSprite_, testUserId.c_str());
    videoRender_setParams(testUserId.c_str(), 200, 200, 320, 240, 255);

    // 加入频道
    channel_join(testChannel.c_str(), testUserId.c_str(), "");

    return true;
}

void VideoCallTestScene::setupUI() {
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    logLabel_ = Label::createWithSystemFont("VideoCall Test", "Arial", 18);
    logLabel_->setAnchorPoint(Vec2(0, 1));
    logLabel_->setPosition(Vec2(origin.x + 10, origin.y + visibleSize.height - 10));
    addChild(logLabel_);

    auto closeItem = MenuItemLabel::create(Label::createWithSystemFont("Back", "Arial", 20),
                                           [this](Ref*) {
                                               Director::getInstance()->popScene();
                                           });
    auto menu = Menu::create(closeItem, nullptr);
    menu->setPosition(Vec2(origin.x + visibleSize.width - 50, origin.y + visibleSize.height - 30));
    addChild(menu);
}

void VideoCallTestScene::logMessage(const std::string& msg) {
    if (logLabel_) {
        logLabel_->setString(msg);
    }
}

// ====== 回调实现：将关键事件打印到屏幕上 ======

void VideoCallTestScene::onRtcEngineInitResult(int code, const char* msg) {
    logMessage("onRtcEngineInitResult: " + std::to_string(code));
}

void VideoCallTestScene::onRtcEngineDestroyed() {
    logMessage("onRtcEngineDestroyed");
}

void VideoCallTestScene::onJoinChannelResult(int code, const char* channelName, const char* userId) {
    logMessage("onJoinChannelResult: " + std::to_string(code));
    if (code == 0) {
        video_enableLocalCamera(true);
        audio_enableLocalMicrophone(true);
    }
}

void VideoCallTestScene::onLeaveChannelResult(int code) {
    logMessage("onLeaveChannelResult: " + std::to_string(code));
}

void VideoCallTestScene::onRemoteUserJoined(const char* userId) {
    logMessage(std::string("remote joined: ") + (userId ? userId : ""));
}

void VideoCallTestScene::onRemoteUserLeft(const char* userId, int reason) {
    logMessage(std::string("remote left: ") + (userId ? userId : ""));
}

void VideoCallTestScene::onLocalVideoPublished(int code) {
    logMessage("onLocalVideoPublished: " + std::to_string(code));
}

void VideoCallTestScene::onLocalAudioPublished(int code) {
    logMessage("onLocalAudioPublished: " + std::to_string(code));
}

void VideoCallTestScene::onRemoteVideoSubscribed(int code, const char* userId) {
    logMessage("onRemoteVideoSubscribed: " + std::to_string(code));
}

void VideoCallTestScene::onRemoteAudioSubscribed(int code, const char* userId) {
    logMessage("onRemoteAudioSubscribed: " + std::to_string(code));
}

void VideoCallTestScene::onVideoFrameReceived(const char* userId,
                                              const unsigned char* data,
                                              int width,
                                              int height,
                                              int format) {
    // C++ 核心已经将视频帧更新到 Sprite 纹理，游戏层可根据需要在此做额外处理
    (void)userId;
    (void)data;
    (void)width;
    (void)height;
    (void)format;
}

void VideoCallTestScene::onPermissionResult(int type, int code) {
    logMessage("onPermissionResult type=" + std::to_string(type) + " code=" + std::to_string(code));
}

void VideoCallTestScene::onError(int code, const char* msg) {
    logMessage("onError: " + std::to_string(code));
}
