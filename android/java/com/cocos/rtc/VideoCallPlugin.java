package com.cocos.rtc;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;

/**
 * Android 端视频通话插件封装层。
 *
 * 负责：
 * - 加载原生库并通过 JNI 调用 C++ 核心接口；
 * - 处理摄像头/麦克风权限申请；
 * - 提供 Java 层回调接口，便于在沙盒测试 App 或游戏 Java 层中快速验证。
 */
public class VideoCallPlugin {

    static {
        System.loadLibrary("VideoCallPlugin");
    }

    public interface Callback {
        void onRtcEngineInitResult(int code, String msg);
        void onRtcEngineDestroyed();
        void onJoinChannelResult(int code, String channelName, String userId);
        void onLeaveChannelResult(int code);
        void onRemoteUserJoined(String userId);
        void onRemoteUserLeft(String userId, int reason);
        void onLocalVideoPublished(int code);
        void onLocalAudioPublished(int code);
        void onRemoteVideoSubscribed(int code, String userId);
        void onRemoteAudioSubscribed(int code, String userId);
        void onPermissionResult(int type, int code);
        void onError(int code, String msg);
    }

    private static Callback sCallback;

    public static void setCallback(Callback cb) {
        sCallback = cb;
    }

    // 由 native 层回调到 Java 层
    private static void handleRtcEngineInitResult(int code, String msg) {
        if (sCallback != null) sCallback.onRtcEngineInitResult(code, msg);
    }

    private static void handleRtcEngineDestroyed() {
        if (sCallback != null) sCallback.onRtcEngineDestroyed();
    }

    private static void handleJoinChannelResult(int code, String channel, String userId) {
        if (sCallback != null) sCallback.onJoinChannelResult(code, channel, userId);
    }

    private static void handleLeaveChannelResult(int code) {
        if (sCallback != null) sCallback.onLeaveChannelResult(code);
    }

    private static void handleRemoteUserJoined(String userId) {
        if (sCallback != null) sCallback.onRemoteUserJoined(userId);
    }

    private static void handleRemoteUserLeft(String userId, int reason) {
        if (sCallback != null) sCallback.onRemoteUserLeft(userId, reason);
    }

    private static void handleLocalVideoPublished(int code) {
        if (sCallback != null) sCallback.onLocalVideoPublished(code);
    }

    private static void handleLocalAudioPublished(int code) {
        if (sCallback != null) sCallback.onLocalAudioPublished(code);
    }

    private static void handleRemoteVideoSubscribed(int code, String userId) {
        if (sCallback != null) sCallback.onRemoteVideoSubscribed(code, userId);
    }

    private static void handleRemoteAudioSubscribed(int code, String userId) {
        if (sCallback != null) sCallback.onRemoteAudioSubscribed(code, userId);
    }

    private static void handlePermissionResult(int type, int code) {
        if (sCallback != null) sCallback.onPermissionResult(type, code);
    }

    private static void handleError(int code, String msg) {
        if (sCallback != null) sCallback.onError(code, msg);
    }

    // ====== 对外 JNI 封装方法 ======

    public static int init(String appId, boolean debug) {
        return nativeRtcEngineInit(appId, debug);
    }

    public static int destroy() {
        return nativeRtcEngineDestroy();
    }

    public static int joinChannel(String channelName, String userId, String token) {
        return nativeChannelJoin(channelName, userId, token);
    }

    public static int leaveChannel() {
        return nativeChannelLeave();
    }

    public static int reconnect() {
        return nativeChannelReconnect();
    }

    public static int enableLocalCamera(boolean enable) {
        return nativeVideoEnableLocalCamera(enable);
    }

    public static int switchCamera() {
        return nativeVideoSwitchCamera();
    }

    public static int enableLocalMicrophone(boolean enable) {
        return nativeAudioEnableLocalMicrophone(enable);
    }

    public static int setLocalVolume(int volume) {
        return nativeAudioSetLocalVolume(volume);
    }

    public static int subscribeRemoteVideo(String userId, boolean subscribe) {
        return nativeVideoSubscribeRemote(userId, subscribe);
    }

    public static int subscribeRemoteAudio(String userId, boolean subscribe) {
        return nativeAudioSubscribeRemote(userId, subscribe);
    }

    // ====== 权限工具（可用于沙盒测试 App）======

    public static boolean hasCameraPermission(Context context) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return true;
        return context.checkSelfPermission(android.Manifest.permission.CAMERA)
                == PackageManager.PERMISSION_GRANTED;
    }

    public static boolean hasMicPermission(Context context) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return true;
        return context.checkSelfPermission(android.Manifest.permission.RECORD_AUDIO)
                == PackageManager.PERMISSION_GRANTED;
    }

    public static void requestCameraAndMicPermissions(Activity activity, int requestCode) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return;
        activity.requestPermissions(
                new String[]{
                        android.Manifest.permission.CAMERA,
                        android.Manifest.permission.RECORD_AUDIO
                },
                requestCode
        );
    }

    // ====== Native 方法声明 ======

    private static native int nativeRtcEngineInit(String appId, boolean debug);
    private static native int nativeRtcEngineDestroy();
    private static native int nativeChannelJoin(String channel, String userId, String token);
    private static native int nativeChannelLeave();
    private static native int nativeChannelReconnect();
    private static native int nativeVideoEnableLocalCamera(boolean enable);
    private static native int nativeVideoSwitchCamera();
    private static native int nativeAudioEnableLocalMicrophone(boolean enable);
    private static native int nativeAudioSetLocalVolume(int volume);
    private static native int nativeVideoSubscribeRemote(String userId, boolean subscribe);
    private static native int nativeAudioSubscribeRemote(String userId, boolean subscribe);
}
