#include "VideoCallCore.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>

#include "cocos2d.h"
#include "IAgoraRtcEngine.h"

using namespace cocos2d;

namespace videocall {

// ---------------- Logger -----------------

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::Logger() : logToFile_(false) {}
Logger::~Logger() {}

void Logger::setLogToFile(bool enable, const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    logToFile_ = enable;
    filePath_ = path;
}

void Logger::logInfo(const std::string& msg) { write("INFO", msg); }
void Logger::logWarn(const std::string& msg) { write("WARN", msg); }
void Logger::logError(const std::string& msg) { write("ERROR", msg); }

void Logger::write(const char* level, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string line = std::string("[VideoCall][") + level + "] " + msg + "\n";
    std::cerr << line;
    if (logToFile_ && !filePath_.empty()) {
        std::ofstream ofs(filePath_, std::ios::app);
        if (ofs.is_open()) {
            ofs << line;
        }
    }
}

// --------------- ChannelManager -----------------

ChannelManager::ChannelManager() = default;

int ChannelManager::joinChannel(const std::string& channelName,
                                const std::string& userId,
                                const std::string& token,
                                AgoraRtcWrapper* engine,
                                IVideoCallCallback* cb) {
    if (channelName.empty()) {
        if (cb) cb->onError(VIDEO_CALL_ERR_CHANNEL_NAME_EMPTY, "channel name empty");
        return VIDEO_CALL_ERR_CHANNEL_NAME_EMPTY;
    }
    if (!engine) {
        if (cb) cb->onError(VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED, "engine not initialized");
        return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    }

    int ret = engine->joinChannel(channelName, userId, token);
    if (ret != VIDEO_CALL_OK) {
        if (cb) cb->onJoinChannelResult(VIDEO_CALL_ERR_JOIN_CHANNEL_FAILED, channelName.c_str(), userId.c_str());
        return VIDEO_CALL_ERR_JOIN_CHANNEL_FAILED;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        channelName_ = channelName;
        localUserId_ = userId;
        onlineUsers_.clear();
        onlineUsers_.insert(userId);
    }

    if (cb) cb->onJoinChannelResult(VIDEO_CALL_OK, channelName.c_str(), userId.c_str());
    return VIDEO_CALL_OK;
}

int ChannelManager::leaveChannel(AgoraRtcWrapper* engine, IVideoCallCallback* cb) {
    if (!engine) {
        if (cb) cb->onError(VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED, "engine not initialized");
        return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    }
    int ret = engine->leaveChannel();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        channelName_.clear();
        localUserId_.clear();
        onlineUsers_.clear();
    }
    if (cb) cb->onLeaveChannelResult(ret == 0 ? VIDEO_CALL_OK : VIDEO_CALL_ERR_JOIN_CHANNEL_FAILED);
    return ret == 0 ? VIDEO_CALL_OK : VIDEO_CALL_ERR_JOIN_CHANNEL_FAILED;
}

int ChannelManager::reconnect(AgoraRtcWrapper* engine, IVideoCallCallback* cb) {
    std::string ch;
    std::string uid;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ch = channelName_;
        uid = localUserId_;
    }
    if (ch.empty() || uid.empty()) {
        if (cb) cb->onError(VIDEO_CALL_ERR_JOIN_CHANNEL_FAILED, "no previous channel to reconnect");
        return VIDEO_CALL_ERR_JOIN_CHANNEL_FAILED;
    }
    // 简单实现：直接重新 join
    return joinChannel(ch, uid, "", engine, cb);
}

int ChannelManager::getOnlineUsers(const char** userList, int* listSize) {
    if (!userList || !listSize) return -1;
    std::lock_guard<std::mutex> lock(mutex_);
    *listSize = static_cast<int>(onlineUsers_.size());
    int index = 0;
    for (const auto& u : onlineUsers_) {
        userList[index++] = u.c_str();
    }
    return VIDEO_CALL_OK;
}

void ChannelManager::onRemoteUserJoined(const std::string& userId, IVideoCallCallback* cb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        onlineUsers_.insert(userId);
    }
    if (cb) cb->onRemoteUserJoined(userId.c_str());
}

void ChannelManager::onRemoteUserLeft(const std::string& userId, int reason, IVideoCallCallback* cb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        onlineUsers_.erase(userId);
    }
    if (cb) cb->onRemoteUserLeft(userId.c_str(), reason);
}

// --------------- AVController -----------------

AVController::AVController() = default;

int AVController::enableLocalCamera(bool enable, AgoraRtcWrapper* engine, IVideoCallCallback* cb) {
    if (!engine) {
        if (cb) cb->onError(VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED, "engine not initialized");
        return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    }
    int ret = engine->enableLocalVideo(enable);
    if (cb) cb->onLocalVideoPublished(ret == 0 ? VIDEO_CALL_OK : ret);
    return ret;
}

int AVController::switchCamera(AgoraRtcWrapper* engine) {
    if (!engine) return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    return engine->switchCamera();
}

int AVController::enableLocalMicrophone(bool enable, AgoraRtcWrapper* engine, IVideoCallCallback* cb) {
    if (!engine) {
        if (cb) cb->onError(VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED, "engine not initialized");
        return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    }
    int ret = engine->enableLocalAudio(enable);
    if (cb) cb->onLocalAudioPublished(ret == 0 ? VIDEO_CALL_OK : ret);
    return ret;
}

int AVController::setLocalVolume(int volume, AgoraRtcWrapper* engine) {
    if (!engine) return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    return engine->setLocalVolume(volume);
}

int AVController::subscribeRemoteVideo(const std::string& userId, bool subscribe, AgoraRtcWrapper* engine, IVideoCallCallback* cb) {
    if (!engine) {
        if (cb) cb->onError(VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED, "engine not initialized");
        return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    }
    int ret = engine->subscribeRemoteVideo(userId, subscribe);
    if (cb) cb->onRemoteVideoSubscribed(ret == 0 ? VIDEO_CALL_OK : ret, userId.c_str());
    return ret;
}

int AVController::subscribeRemoteAudio(const std::string& userId, bool subscribe, AgoraRtcWrapper* engine, IVideoCallCallback* cb) {
    if (!engine) {
        if (cb) cb->onError(VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED, "engine not initialized");
        return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    }
    int ret = engine->subscribeRemoteAudio(userId, subscribe);
    if (cb) cb->onRemoteAudioSubscribed(ret == 0 ? VIDEO_CALL_OK : ret, userId.c_str());
    return ret;
}

// --------------- VideoRenderManager -----------------

VideoRenderManager::VideoRenderManager() = default;

int VideoRenderManager::initRenderer(Sprite* sprite, const std::string& userId) {
    if (!sprite) {
        return VIDEO_CALL_ERR_RENDER_NODE_NULL;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    RenderTarget& target = targets_[userId];
    target.sprite = sprite;
    target.texture = nullptr; // 延迟创建纹理
    return VIDEO_CALL_OK;
}

int VideoRenderManager::setRenderParams(const std::string& userId,
                                        float x,
                                        float y,
                                        float width,
                                        float height,
                                        int alpha) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = targets_.find(userId);
    if (it == targets_.end() || !it->second.sprite) {
        return VIDEO_CALL_ERR_RENDER_NODE_NULL;
    }
    Sprite* sprite = it->second.sprite;
    sprite->setPosition(Vec2(x, y));
    sprite->setContentSize(Size(width, height));
    sprite->setOpacity(static_cast<GLubyte>(alpha));
    return VIDEO_CALL_OK;
}

int VideoRenderManager::destroyRenderer(const std::string& userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = targets_.find(userId);
    if (it != targets_.end()) {
        // 纹理由 Sprite 管理，这里不手动释放
        targets_.erase(it);
    }
    return VIDEO_CALL_OK;
}

void VideoRenderManager::onVideoFrame(const std::string& userId,
                                      const unsigned char* data,
                                      int width,
                                      int height,
                                      int format) {
    if (!data || width <= 0 || height <= 0) return;
    if (format != VIDEO_FRAME_FORMAT_RGBA) {
        Logger::instance().logWarn("unsupported video frame format, expected RGBA");
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = targets_.find(userId);
    if (it == targets_.end() || !it->second.sprite) return;

    RenderTarget& target = it->second;
    if (!target.texture) {
        target.texture = new Texture2D();
    }

    // 将 RGBA 数据更新到纹理，然后绑定到 Sprite
    Texture2D::PixelFormat fmt = Texture2D::PixelFormat::RGBA8888;
    target.texture->initWithData(data,
                                 width * height * 4,
                                 fmt,
                                 width,
                                 height,
                                 Size(width, height));
    target.sprite->setTexture(target.texture);
}

// --------------- PermissionManager -----------------

PermissionManager& PermissionManager::instance() {
    static PermissionManager inst;
    return inst;
}

PermissionManager::PermissionManager()
    : cameraState_(Granted), micState_(Granted) {
}

int PermissionManager::requestCamera(IVideoCallCallback* cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    cameraState_ = Granted; // PC 端直接授权；移动端由平台层覆盖实现
    if (cb) cb->onPermissionResult(PERMISSION_TYPE_CAMERA, 0);
    return 0;
}

int PermissionManager::requestMicrophone(IVideoCallCallback* cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    micState_ = Granted;
    if (cb) cb->onPermissionResult(PERMISSION_TYPE_MICROPHONE, 0);
    return 0;
}

int PermissionManager::checkCameraState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(cameraState_);
}

int PermissionManager::checkMicrophoneState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(micState_);
}

// --------------- AgoraRtcWrapper -----------------

AgoraRtcWrapper::AgoraRtcWrapper() : initialized_(false), engineHandle_(nullptr) {}
AgoraRtcWrapper::~AgoraRtcWrapper() { destroy(); }

static agora::rtc::IRtcEngine* getRtcEngine(void* handle) {
    return reinterpret_cast<agora::rtc::IRtcEngine*>(handle);
}

static agora::uid_t parseUid(const std::string& userId) {
    if (userId.empty()) return 0;
    try {
        return static_cast<agora::uid_t>(std::stoul(userId));
    } catch (...) {
        return 0;
    }
}

int AgoraRtcWrapper::initialize(const std::string& appId, bool debugMode) {
    if (appId.empty()) {
        return VIDEO_CALL_ERR_INVALID_APPID;
    }
    if (initialized_) {
        return VIDEO_CALL_OK;
    }

    agora::rtc::IRtcEngine* engine = agora::rtc::createAgoraRtcEngine();
    if (!engine) {
        Logger::instance().logError("Failed to create Agora RtcEngine");
        return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    }

    agora::rtc::RtcEngineContext ctx;
    ctx.appId = appId.c_str();
    ctx.eventHandler = nullptr; // 如需监听 SDK 事件，可在此处实现 IRtcEngineEventHandler

    int ret = engine->initialize(ctx);
    if (ret != 0) {
        Logger::instance().logError("Agora engine initialize failed: " + std::to_string(ret));
        engine->release(true);
        return ret;
    }

    // 默认开启音视频功能，编码参数可按棋牌场景在此补充设置
    engine->enableVideo();
    engine->enableAudio();

    initialized_ = true;
    engineHandle_ = engine;
    (void)debugMode;
    return VIDEO_CALL_OK;
}

int AgoraRtcWrapper::destroy() {
    if (!initialized_) return VIDEO_CALL_OK;

    agora::rtc::IRtcEngine* engine = getRtcEngine(engineHandle_);
    if (engine) {
        engine->leaveChannel();
        engine->release(true);
    }
    engineHandle_ = nullptr;
    initialized_ = false;
    return VIDEO_CALL_OK;
}

int AgoraRtcWrapper::joinChannel(const std::string& channelName,
                                 const std::string& userId,
                                 const std::string& token) {
    if (!initialized_) return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    agora::rtc::IRtcEngine* engine = getRtcEngine(engineHandle_);
    if (!engine) return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;

    const char* tokenC = token.empty() ? nullptr : token.c_str();
    agora::uid_t uid = parseUid(userId);

    Logger::instance().logInfo("Agora joinChannel: " + channelName + ", uid:" + std::to_string(uid));
    int ret = engine->joinChannel(tokenC, channelName.c_str(), nullptr, uid);
    return ret;
}

int AgoraRtcWrapper::leaveChannel() {
    if (!initialized_) return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    agora::rtc::IRtcEngine* engine = getRtcEngine(engineHandle_);
    if (!engine) return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;

    Logger::instance().logInfo("Agora leaveChannel");
    return engine->leaveChannel();
}

int AgoraRtcWrapper::enableLocalVideo(bool enable) {
    if (!initialized_) return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    agora::rtc::IRtcEngine* engine = getRtcEngine(engineHandle_);
    if (!engine) return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;

    if (enable) {
        engine->enableVideo();
        return engine->muteLocalVideoStream(false);
    } else {
        return engine->muteLocalVideoStream(true);
    }
}

int AgoraRtcWrapper::enableLocalAudio(bool enable) {
    if (!initialized_) return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    agora::rtc::IRtcEngine* engine = getRtcEngine(engineHandle_);
    if (!engine) return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;

    if (enable) {
        engine->enableAudio();
        return engine->muteLocalAudioStream(false);
    } else {
        return engine->muteLocalAudioStream(true);
    }
}

int AgoraRtcWrapper::switchCamera() {
    if (!initialized_) return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    agora::rtc::IRtcEngine* engine = getRtcEngine(engineHandle_);
    if (!engine) return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    // 在移动端 SDK 中有效；PC 上调用将返回错误码但不会崩溃
    return engine->switchCamera();
}

int AgoraRtcWrapper::setLocalVolume(int volume) {
    if (!initialized_) return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    agora::rtc::IRtcEngine* engine = getRtcEngine(engineHandle_);
    if (!engine) return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;

    if (volume < 0) volume = 0;
    if (volume > 400) volume = 400; // Agora 录音信号音量建议范围 0-400
    return engine->adjustRecordingSignalVolume(volume);
}

int AgoraRtcWrapper::subscribeRemoteVideo(const std::string& userId, bool subscribe) {
    if (!initialized_) return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    agora::rtc::IRtcEngine* engine = getRtcEngine(engineHandle_);
    if (!engine) return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;

    agora::uid_t uid = parseUid(userId);
    return engine->muteRemoteVideoStream(uid, !subscribe);
}

int AgoraRtcWrapper::subscribeRemoteAudio(const std::string& userId, bool subscribe) {
    if (!initialized_) return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    agora::rtc::IRtcEngine* engine = getRtcEngine(engineHandle_);
    if (!engine) return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;

    agora::uid_t uid = parseUid(userId);
    return engine->muteRemoteAudioStream(uid, !subscribe);
}

// --------------- VideoCallCore -----------------

VideoCallCore& VideoCallCore::instance() {
    static VideoCallCore inst;
    return inst;
}

VideoCallCore::VideoCallCore()
    : engine_(new AgoraRtcWrapper()), callback_(nullptr), state_(RTC_ENGINE_STATE_UNINITIALIZED) {}

VideoCallCore::~VideoCallCore() { destroy(); }

int VideoCallCore::init(const std::string& appId, bool debugMode, IVideoCallCallback* cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ != RTC_ENGINE_STATE_UNINITIALIZED) {
        return VIDEO_CALL_OK;
    }
    callback_ = cb;
    if (appId.empty()) {
        if (cb) cb->onRtcEngineInitResult(VIDEO_CALL_ERR_INVALID_APPID, "appId is empty");
        return VIDEO_CALL_ERR_INVALID_APPID;
    }
    int ret = engine_->initialize(appId, debugMode);
    if (ret == VIDEO_CALL_OK) {
        state_ = RTC_ENGINE_STATE_INITIALIZED;
        if (cb) cb->onRtcEngineInitResult(VIDEO_CALL_OK, "success");
    } else {
        if (cb) cb->onRtcEngineInitResult(ret, "engine initialize failed");
    }
    return ret;
}

int VideoCallCore::destroy() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == RTC_ENGINE_STATE_UNINITIALIZED) return VIDEO_CALL_OK;
    engine_->destroy();
    state_ = RTC_ENGINE_STATE_UNINITIALIZED;
    if (callback_) callback_->onRtcEngineDestroyed();
    callback_ = nullptr;
    return VIDEO_CALL_OK;
}

int VideoCallCore::getState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(state_);
}

int VideoCallCore::joinChannel(const std::string& channelName,
                               const std::string& userId,
                               const std::string& token) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == RTC_ENGINE_STATE_UNINITIALIZED) {
        if (callback_) callback_->onError(VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED, "engine not initialized");
        return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    }
    state_ = RTC_ENGINE_STATE_CONNECTING;
    int ret = channelMgr_.joinChannel(channelName, userId, token, engine_.get(), callback_);
    if (ret == VIDEO_CALL_OK) {
        state_ = RTC_ENGINE_STATE_CONNECTED;
    } else {
        state_ = RTC_ENGINE_STATE_INITIALIZED;
    }
    return ret;
}

int VideoCallCore::leaveChannel() {
    std::lock_guard<std::mutex> lock(mutex_);
    int ret = channelMgr_.leaveChannel(engine_.get(), callback_);
    state_ = RTC_ENGINE_STATE_INITIALIZED;
    return ret;
}

int VideoCallCore::reconnectChannel() {
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = RTC_ENGINE_STATE_CONNECTING;
    int ret = channelMgr_.reconnect(engine_.get(), callback_);
    if (ret == VIDEO_CALL_OK) {
        state_ = RTC_ENGINE_STATE_CONNECTED;
    } else {
        state_ = RTC_ENGINE_STATE_INITIALIZED;
    }
    return ret;
}

int VideoCallCore::getOnlineUsers(const char** userList, int* listSize) {
    std::lock_guard<std::mutex> lock(mutex_);
    return channelMgr_.getOnlineUsers(userList, listSize);
}

int VideoCallCore::enableLocalCamera(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    return avController_.enableLocalCamera(enable, engine_.get(), callback_);
}

int VideoCallCore::switchCamera() {
    std::lock_guard<std::mutex> lock(mutex_);
    return avController_.switchCamera(engine_.get());
}

int VideoCallCore::enableLocalMicrophone(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    return avController_.enableLocalMicrophone(enable, engine_.get(), callback_);
}

int VideoCallCore::setLocalVolume(int volume) {
    std::lock_guard<std::mutex> lock(mutex_);
    return avController_.setLocalVolume(volume, engine_.get());
}

int VideoCallCore::subscribeRemoteVideo(const std::string& userId, bool subscribe) {
    std::lock_guard<std::mutex> lock(mutex_);
    return avController_.subscribeRemoteVideo(userId, subscribe, engine_.get(), callback_);
}

int VideoCallCore::subscribeRemoteAudio(const std::string& userId, bool subscribe) {
    std::lock_guard<std::mutex> lock(mutex_);
    return avController_.subscribeRemoteAudio(userId, subscribe, engine_.get(), callback_);
}

int VideoCallCore::initRenderer(Sprite* sprite, const std::string& userId) {
    return renderMgr_.initRenderer(sprite, userId);
}

int VideoCallCore::setRendererParams(const std::string& userId,
                                     float x,
                                     float y,
                                     float width,
                                     float height,
                                     int alpha) {
    return renderMgr_.setRenderParams(userId, x, y, width, height, alpha);
}

int VideoCallCore::destroyRenderer(const std::string& userId) {
    return renderMgr_.destroyRenderer(userId);
}

void VideoCallCore::onVideoFrameInternal(const std::string& userId,
                                         const unsigned char* data,
                                         int width,
                                         int height,
                                         int format) {
    renderMgr_.onVideoFrame(userId, data, width, height, format);
    if (callback_) {
        callback_->onVideoFrameReceived(userId.c_str(), data, width, height, format);
    }
}

} // namespace videocall
