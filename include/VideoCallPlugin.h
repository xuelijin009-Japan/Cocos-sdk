#pragma once

#include <string>

namespace cocos2d {
class Sprite;
}

// 错误码定义
enum VideoCallErrorCode {
    VIDEO_CALL_OK = 0,
    VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED = 1001,
    VIDEO_CALL_ERR_INVALID_APPID = 1002,
    VIDEO_CALL_ERR_CHANNEL_NAME_EMPTY = 2001,
    VIDEO_CALL_ERR_JOIN_CHANNEL_FAILED = 2002,
    VIDEO_CALL_ERR_CAMERA_PERMISSION_DENIED = 3001,
    VIDEO_CALL_ERR_MIC_PERMISSION_DENIED = 3002,
    VIDEO_CALL_ERR_RENDER_NODE_NULL = 4001,
    VIDEO_CALL_ERR_UNSUPPORTED_PLATFORM = 5001
};

// 引擎状态
enum RtcEngineState {
    RTC_ENGINE_STATE_UNINITIALIZED = 0,
    RTC_ENGINE_STATE_INITIALIZED = 1,
    RTC_ENGINE_STATE_CONNECTING = 2,
    RTC_ENGINE_STATE_CONNECTED = 3
};

// 视频帧格式
enum VideoFrameFormat {
    VIDEO_FRAME_FORMAT_UNKNOWN = 0,
    VIDEO_FRAME_FORMAT_RGBA = 1
};

// 权限类型
enum PermissionType {
    PERMISSION_TYPE_CAMERA = 0,
    PERMISSION_TYPE_MICROPHONE = 1
};

class IVideoCallCallback {
public:
    virtual ~IVideoCallCallback() {}

    // 引擎初始化结果回调
    virtual void onRtcEngineInitResult(int code, const char* msg) = 0;

    // 引擎销毁完成回调
    virtual void onRtcEngineDestroyed() = 0;

    // 加入频道结果回调
    virtual void onJoinChannelResult(int code, const char* channelName, const char* userId) = 0;

    // 离开频道结果回调
    virtual void onLeaveChannelResult(int code) = 0;

    // 远端用户加入频道回调
    virtual void onRemoteUserJoined(const char* userId) = 0;

    // 远端用户离开频道回调
    virtual void onRemoteUserLeft(const char* userId, int reason) = 0;

    // 本地视频发布结果回调
    virtual void onLocalVideoPublished(int code) = 0;

    // 本地音频发布结果回调
    virtual void onLocalAudioPublished(int code) = 0;

    // 远端视频订阅结果回调
    virtual void onRemoteVideoSubscribed(int code, const char* userId) = 0;

    // 远端音频订阅结果回调
    virtual void onRemoteAudioSubscribed(int code, const char* userId) = 0;

    // 视频帧数据回调（用于渲染）
    virtual void onVideoFrameReceived(const char* userId,
                                      const unsigned char* data,
                                      int width,
                                      int height,
                                      int format) = 0;

    // 权限申请结果回调（仅移动端）
    // type: 0-摄像头，1-麦克风；code:0-授权通过，1-拒绝
    virtual void onPermissionResult(int type, int code) = 0;

    // 错误信息回调
    virtual void onError(int code, const char* msg) = 0;
};

// 导出符号定义
#if defined(_WIN32) || defined(_WIN64)
#  ifdef VIDEO_CALL_PLUGIN_EXPORTS
#    define VIDEO_CALL_API __declspec(dllexport)
#  else
#    define VIDEO_CALL_API __declspec(dllimport)
#  endif
#else
#  define VIDEO_CALL_API
#endif

extern "C" {

// 初始化RTC引擎
// 参数：appId - 声网AppID；debugMode - 是否开启调试模式；callback - 事件回调接口实例
// 返回值：0-成功，非0-错误码
VIDEO_CALL_API int rtcEngine_init(const char* appId, bool debugMode, IVideoCallCallback* callback);

// 销毁RTC引擎
// 返回值：0-成功，非0-错误码
VIDEO_CALL_API int rtcEngine_destroy();

// 获取当前引擎状态
// 返回值：引擎状态枚举值
VIDEO_CALL_API int rtcEngine_getState();

// 加入频道
VIDEO_CALL_API int channel_join(const char* channelName, const char* userId, const char* token);

// 离开频道
VIDEO_CALL_API int channel_leave();

// 重连频道
VIDEO_CALL_API int channel_reconnect();

// 获取当前频道内在线用户列表
VIDEO_CALL_API int channel_getOnlineUsers(const char** userList, int* listSize);

// 开启/关闭本地摄像头
VIDEO_CALL_API int video_enableLocalCamera(bool enable);

// 切换前后摄像头（仅移动端有效）
VIDEO_CALL_API int video_switchCamera();

// 开启/关闭本地麦克风
VIDEO_CALL_API int audio_enableLocalMicrophone(bool enable);

// 调节本地麦克风音量
VIDEO_CALL_API int audio_setLocalVolume(int volume);

// 订阅/取消订阅远端用户视频流
VIDEO_CALL_API int video_subscribeRemoteStream(const char* userId, bool subscribe);

// 订阅/取消订阅远端用户音频流
VIDEO_CALL_API int audio_subscribeRemoteStream(const char* userId, bool subscribe);

// 初始化视频渲染器
VIDEO_CALL_API int videoRender_init(cocos2d::Sprite* renderNode, const char* userId);

// 设置视频渲染参数
VIDEO_CALL_API int videoRender_setParams(const char* userId,
                                         float x,
                                         float y,
                                         float width,
                                         float height,
                                         int alpha);

// 销毁视频渲染器
VIDEO_CALL_API int videoRender_destroy(const char* userId);

// 移动端权限接口
VIDEO_CALL_API int permission_requestCamera();
VIDEO_CALL_API int permission_requestMicrophone();
VIDEO_CALL_API int permission_checkCameraState();
VIDEO_CALL_API int permission_checkMicrophoneState();

} // extern "C"
