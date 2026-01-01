#include <jni.h>
#include <string>

#include "VideoCallPlugin.h"
#include "src/core/VideoCallCore.h"

using namespace videocall;

static JavaVM* g_vm = nullptr;
static jobject g_callbackObj = nullptr; // 全局引用 com.cocos.rtc.VideoCallPlugin class (用于静态回调调用)

// Android 端回调适配器，实现 IVideoCallCallback，将事件转发到 Java 静态方法
class AndroidVideoCallCallback : public IVideoCallCallback {
public:
    AndroidVideoCallCallback() = default;
    ~AndroidVideoCallCallback() override = default;

    void onRtcEngineInitResult(int code, const char* msg) override {
        callStaticVoid("handleRtcEngineInitResult", "(ILjava/lang/String;)V", code, msg);
    }

    void onRtcEngineDestroyed() override {
        callStaticVoid("handleRtcEngineDestroyed", "()V");
    }

    void onJoinChannelResult(int code, const char* channelName, const char* userId) override {
        callStaticVoid("handleJoinChannelResult", "(ILjava/lang/String;Ljava/lang/String;)V", code, channelName, userId);
    }

    void onLeaveChannelResult(int code) override {
        callStaticVoid("handleLeaveChannelResult", "(I)V", code);
    }

    void onRemoteUserJoined(const char* userId) override {
        callStaticVoid("handleRemoteUserJoined", "(Ljava/lang/String;)V", userId);
    }

    void onRemoteUserLeft(const char* userId, int reason) override {
        callStaticVoid("handleRemoteUserLeft", "(Ljava/lang/String;I)V", userId, reason);
    }

    void onLocalVideoPublished(int code) override {
        callStaticVoid("handleLocalVideoPublished", "(I)V", code);
    }

    void onLocalAudioPublished(int code) override {
        callStaticVoid("handleLocalAudioPublished", "(I)V", code);
    }

    void onRemoteVideoSubscribed(int code, const char* userId) override {
        callStaticVoid("handleRemoteVideoSubscribed", "(ILjava/lang/String;)V", code, userId);
    }

    void onRemoteAudioSubscribed(int code, const char* userId) override {
        callStaticVoid("handleRemoteAudioSubscribed", "(ILjava/lang/String;)V", code, userId);
    }

    void onVideoFrameReceived(const char* userId,
                              const unsigned char* data,
                              int width,
                              int height,
                              int format) override {
        // Android 端如需 Java 层绘制，可在此扩展一套回调；目前通过 Cocos 纹理渲染即可，Java 不必感知。
        (void)userId;
        (void)data;
        (void)width;
        (void)height;
        (void)format;
    }

    void onPermissionResult(int type, int code) override {
        callStaticVoid("handlePermissionResult", "(II)V", type, code);
    }

    void onError(int code, const char* msg) override {
        callStaticVoid("handleError", "(ILjava/lang/String;)V", code, msg);
    }

private:
    JNIEnv* getEnv() {
        if (!g_vm) return nullptr;
        JNIEnv* env = nullptr;
        if (g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
            if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
                return nullptr;
            }
        }
        return env;
    }

    void callStaticVoid(const char* name, const char* sig) {
        JNIEnv* env = getEnv();
        if (!env || !g_callbackObj) return;
        jclass clazz = env->GetObjectClass(g_callbackObj);
        if (!clazz) return;
        jmethodID mid = env->GetStaticMethodID(clazz, name, sig);
        if (!mid) return;
        env->CallStaticVoidMethod(clazz, mid);
    }

    void callStaticVoid(const char* name, const char* sig, int a1, const char* s1 = nullptr, const char* s2 = nullptr) {
        JNIEnv* env = getEnv();
        if (!env || !g_callbackObj) return;
        jclass clazz = env->GetObjectClass(g_callbackObj);
        if (!clazz) return;
        jmethodID mid = env->GetStaticMethodID(clazz, name, sig);
        if (!mid) return;

        jstring js1 = s1 ? env->NewStringUTF(s1) : nullptr;
        jstring js2 = s2 ? env->NewStringUTF(s2) : nullptr;

        if (strcmp(sig, "(ILjava/lang/String;)V") == 0) {
            env->CallStaticVoidMethod(clazz, mid, a1, js1);
        } else if (strcmp(sig, "(ILjava/lang/String;Ljava/lang/String;)V") == 0) {
            env->CallStaticVoidMethod(clazz, mid, a1, js1, js2);
        } else if (strcmp(sig, "(II)V") == 0) {
            env->CallStaticVoidMethod(clazz, mid, a1, (jint)(intptr_t)js1);
        }

        if (js1) env->DeleteLocalRef(js1);
        if (js2) env->DeleteLocalRef(js2);
    }

    void callStaticVoid(const char* name, const char* sig, const char* s1, int a2) {
        JNIEnv* env = getEnv();
        if (!env || !g_callbackObj) return;
        jclass clazz = env->GetObjectClass(g_callbackObj);
        if (!clazz) return;
        jmethodID mid = env->GetStaticMethodID(clazz, name, sig);
        if (!mid) return;
        jstring js1 = s1 ? env->NewStringUTF(s1) : nullptr;
        env->CallStaticVoidMethod(clazz, mid, js1, a2);
        if (js1) env->DeleteLocalRef(js1);
    }
};

static AndroidVideoCallCallback g_androidCallback;

extern "C" {

jint JNI_OnLoad(JavaVM* vm, void*) {
    g_vm = vm;
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL
Java_com_cocos_rtc_VideoCallPlugin_nativeInitCallbackBridge(JNIEnv* env, jclass clazz) {
    if (g_callbackObj) {
        env->DeleteGlobalRef(g_callbackObj);
        g_callbackObj = nullptr;
    }
    g_callbackObj = env->NewGlobalRef(clazz);
}

JNIEXPORT jint JNICALL
Java_com_cocos_rtc_VideoCallPlugin_nativeRtcEngineInit(JNIEnv* env, jclass, jstring jAppId, jboolean debug) {
    const char* appId = env->GetStringUTFChars(jAppId, nullptr);
    int ret = rtcEngine_init(appId, debug == JNI_TRUE, &g_androidCallback);
    env->ReleaseStringUTFChars(jAppId, appId);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_cocos_rtc_VideoCallPlugin_nativeRtcEngineDestroy(JNIEnv*, jclass) {
    return rtcEngine_destroy();
}

JNIEXPORT jint JNICALL
Java_com_cocos_rtc_VideoCallPlugin_nativeChannelJoin(JNIEnv* env, jclass, jstring jChannel, jstring jUserId, jstring jToken) {
    const char* ch = env->GetStringUTFChars(jChannel, nullptr);
    const char* uid = env->GetStringUTFChars(jUserId, nullptr);
    const char* tk = jToken ? env->GetStringUTFChars(jToken, nullptr) : "";
    int ret = channel_join(ch, uid, tk);
    env->ReleaseStringUTFChars(jChannel, ch);
    env->ReleaseStringUTFChars(jUserId, uid);
    if (jToken) env->ReleaseStringUTFChars(jToken, tk);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_cocos_rtc_VideoCallPlugin_nativeChannelLeave(JNIEnv*, jclass) {
    return channel_leave();
}

JNIEXPORT jint JNICALL
Java_com_cocos_rtc_VideoCallPlugin_nativeChannelReconnect(JNIEnv*, jclass) {
    return channel_reconnect();
}

JNIEXPORT jint JNICALL
Java_com_cocos_rtc_VideoCallPlugin_nativeVideoEnableLocalCamera(JNIEnv*, jclass, jboolean enable) {
    return video_enableLocalCamera(enable == JNI_TRUE);
}

JNIEXPORT jint JNICALL
Java_com_cocos_rtc_VideoCallPlugin_nativeVideoSwitchCamera(JNIEnv*, jclass) {
    return video_switchCamera();
}

JNIEXPORT jint JNICALL
Java_com_cocos_rtc_VideoCallPlugin_nativeAudioEnableLocalMicrophone(JNIEnv*, jclass, jboolean enable) {
    return audio_enableLocalMicrophone(enable == JNI_TRUE);
}

JNIEXPORT jint JNICALL
Java_com_cocos_rtc_VideoCallPlugin_nativeAudioSetLocalVolume(JNIEnv*, jclass, jint volume) {
    return audio_setLocalVolume(static_cast<int>(volume));
}

JNIEXPORT jint JNICALL
Java_com_cocos_rtc_VideoCallPlugin_nativeVideoSubscribeRemote(JNIEnv* env, jclass, jstring jUserId, jboolean subscribe) {
    const char* uid = env->GetStringUTFChars(jUserId, nullptr);
    int ret = video_subscribeRemoteStream(uid, subscribe == JNI_TRUE);
    env->ReleaseStringUTFChars(jUserId, uid);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_cocos_rtc_VideoCallPlugin_nativeAudioSubscribeRemote(JNIEnv* env, jclass, jstring jUserId, jboolean subscribe) {
    const char* uid = env->GetStringUTFChars(jUserId, nullptr);
    int ret = audio_subscribeRemoteStream(uid, subscribe == JNI_TRUE);
    env->ReleaseStringUTFChars(jUserId, uid);
    return ret;
}

} // extern "C"
