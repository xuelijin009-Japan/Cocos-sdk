#include "VideoCallPlugin.h"

#include "src/core/VideoCallCore.h"

using namespace cocos2d;
using namespace videocall;

extern "C" {

int rtcEngine_init(const char* appId, bool debugMode, IVideoCallCallback* callback) {
    if (!appId || !callback) {
        if (callback) {
            callback->onError(VIDEO_CALL_ERR_INVALID_APPID, "appId or callback is null");
        }
        return VIDEO_CALL_ERR_INVALID_APPID;
    }
    return VideoCallCore::instance().init(appId, debugMode, callback);
}

int rtcEngine_destroy() {
    return VideoCallCore::instance().destroy();
}

int rtcEngine_getState() {
    return VideoCallCore::instance().getState();
}

int channel_join(const char* channelName, const char* userId, const char* token) {
    std::string ch = channelName ? channelName : "";
    std::string uid = userId ? userId : "";
    std::string tk = token ? token : "";
    return VideoCallCore::instance().joinChannel(ch, uid, tk);
}

int channel_leave() {
    return VideoCallCore::instance().leaveChannel();
}

int channel_reconnect() {
    return VideoCallCore::instance().reconnectChannel();
}

int channel_getOnlineUsers(const char** userList, int* listSize) {
    return VideoCallCore::instance().getOnlineUsers(userList, listSize);
}

int video_enableLocalCamera(bool enable) {
    return VideoCallCore::instance().enableLocalCamera(enable);
}

int video_switchCamera() {
    return VideoCallCore::instance().switchCamera();
}

int audio_enableLocalMicrophone(bool enable) {
    return VideoCallCore::instance().enableLocalMicrophone(enable);
}

int audio_setLocalVolume(int volume) {
    return VideoCallCore::instance().setLocalVolume(volume);
}

int video_subscribeRemoteStream(const char* userId, bool subscribe) {
    std::string uid = userId ? userId : "";
    return VideoCallCore::instance().subscribeRemoteVideo(uid, subscribe);
}

int audio_subscribeRemoteStream(const char* userId, bool subscribe) {
    std::string uid = userId ? userId : "";
    return VideoCallCore::instance().subscribeRemoteAudio(uid, subscribe);
}

int videoRender_init(Sprite* renderNode, const char* userId) {
    std::string uid = userId ? userId : "";
    return VideoCallCore::instance().initRenderer(renderNode, uid);
}

int videoRender_setParams(const char* userId,
                          float x,
                          float y,
                          float width,
                          float height,
                          int alpha) {
    std::string uid = userId ? userId : "";
    return VideoCallCore::instance().setRendererParams(uid, x, y, width, height, alpha);
}

int videoRender_destroy(const char* userId) {
    std::string uid = userId ? userId : "";
    return VideoCallCore::instance().destroyRenderer(uid);
}

int permission_requestCamera() {
    return PermissionManager::instance().requestCamera(VideoCallCore::instance().getCallback());
}

int permission_requestMicrophone() {
    return PermissionManager::instance().requestMicrophone(VideoCallCore::instance().getCallback());
}

int permission_checkCameraState() {
    return PermissionManager::instance().checkCameraState();
}

int permission_checkMicrophoneState() {
    return PermissionManager::instance().checkMicrophoneState();
}

} // extern "C"
