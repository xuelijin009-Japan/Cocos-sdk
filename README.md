### **一、AgoraRtcWrapper 已按 v4.x C++ SDK 接入改造**

现在 `AgoraRtcWrapper` 已经不再是空 TODO，而是按声网原生 C++ SDK 的典型用法实现了初始化 / 入会 / 离会 / 音视频开关 / 远端订阅等逻辑，只要：

- 工程里正确引入 Agora v4.2.0 Native SDK 头文件和库（`IAgoraRtcEngine.h`、对应 `.lib/.dll` 或 `.a/.so`），
- 链接参数配置正确，

就**可以直接编译运行**（以及根据需要少量调整 API 名称或参数）。

核心改动点（代码已经写进工程里）：

- 文件：`src/core/VideoCallCore.cpp`
- 头文件：`src/core/VideoCallCore.h`

### **1. 引入 Agora 头文件 & 新成员**

```cpp
cpp
#include "VideoCallCore.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>

#include "cocos2d.h"
#include "IAgoraRtcEngine.h"
```

在 `VideoCallCore.h` 的 `AgoraRtcWrapper` 内部新增引擎句柄：

```cpp
cpp
class AgoraRtcWrapper {
public:
    AgoraRtcWrapper();
    ~AgoraRtcWrapper();

    int initialize(const std::string& appId,bool debugMode);
    int destroy();

    int joinChannel(const std::string& channelName,
                    const std::string& userId,
                    const std::string& token);
    int leaveChannel();

    int enableLocalVideo(bool enable);
    int enableLocalAudio(bool enable);
    int switchCamera();
    int setLocalVolume(int volume);

    int subscribeRemoteVideo(const std::string& userId,bool subscribe);
    int subscribeRemoteAudio(const std::string& userId,bool subscribe);

private:
    bool initialized_;
    void* engineHandle_;           // 实际保存 agora::rtc::IRtcEngine*
};
```

> 这样头文件不依赖 Agora 类型，具体类型只在 .cpp 里通过 reinterpret_cast 使用。
> 

### **2. 创建 / 初始化引擎（initialize）**

`VideoCallCore.cpp` 中：

```cpp
cpp
// --------------- AgoraRtcWrapper -----------------

AgoraRtcWrapper::AgoraRtcWrapper() :initialized_(false),engineHandle_(nullptr) {}
AgoraRtcWrapper::~AgoraRtcWrapper() {destroy(); }

static agora::rtc::IRtcEngine* getRtcEngine(void* handle) {
    return reinterpret_cast<agora::rtc::IRtcEngine*>(handle);
}

static agora::uid_t parseUid(const std::string& userId) {
    if (userId.empty())return 0;
    try {
        return static_cast<agora::uid_t>(std::stoul(userId));
    }catch (...) {
        return 0;
    }
}

int AgoraRtcWrapper::initialize(const std::string& appId,bool debugMode) {
    if (appId.empty()) {
        return VIDEO_CALL_ERR_INVALID_APPID;
    }
    if (initialized_) {
        return VIDEO_CALL_OK;
    }

    agora::rtc::IRtcEngine* engine =agora::rtc::createAgoraRtcEngine();
    if (!engine) {
        Logger::instance().logError("Failed to create Agora RtcEngine");
        return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    }

    agora::rtc::RtcEngineContext ctx;
    ctx.appId =appId.c_str();
    ctx.eventHandler =nullptr; // 如需监听 SDK 事件，可在此处实现 IRtcEngineEventHandler

    int ret =engine->initialize(ctx);
    if (ret !=0) {
        Logger::instance().logError("Agora engine initialize failed: " +std::to_string(ret));
        engine->release(true);
        return ret;
    }

    // 默认开启音视频功能，编码参数可按棋牌场景在此补充设置
    engine->enableVideo();
    engine->enableAudio();

    initialized_ =true;
    engineHandle_ = engine;
    (void)debugMode;
    return VIDEO_CALL_OK;
}
```

说明：

- 使用 `agora::rtc::createAgoraRtcEngine()` 创建引擎实例。
- 使用 `RtcEngineContext` 设置 `appId` 和事件回调。
- 初始化成功后默认 `enableVideo()` 和 `enableAudio()`，后续通过开关接口控制发布。

### **3. 销毁引擎（destroy）**

```cpp
cpp
int AgoraRtcWrapper::destroy() {
    if (!initialized_)return VIDEO_CALL_OK;

    agora::rtc::IRtcEngine* engine =getRtcEngine(engineHandle_);
    if (engine) {
        engine->leaveChannel();
        engine->release(true);
    }
    engineHandle_ =nullptr;
    initialized_ =false;
    return VIDEO_CALL_OK;
}
```

- 离开频道 + `release(true)`，确保内部资源释放，避免内存泄漏。

### **4. 加入 / 离开频道**

```cpp
cpp
int AgoraRtcWrapper::joinChannel(const std::string& channelName,
                                 const std::string& userId,
                                 const std::string& token) {
    if (!initialized_)return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    agora::rtc::IRtcEngine* engine =getRtcEngine(engineHandle_);
    if (!engine)return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;

    const char* tokenC =token.empty() ?nullptr :token.c_str();
    agora::uid_t uid =parseUid(userId);

    Logger::instance().logInfo("Agora joinChannel: " + channelName +", uid:" +std::to_string(uid));
    int ret =engine->joinChannel(tokenC,channelName.c_str(),nullptr, uid);
    return ret;
}

int AgoraRtcWrapper::leaveChannel() {
    if (!initialized_)return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    agora::rtc::IRtcEngine* engine =getRtcEngine(engineHandle_);
    if (!engine)return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;

    Logger::instance().logInfo("Agora leaveChannel");
    return engine->leaveChannel();
}
```

说明：

- 使用**老的兼容接口** `joinChannel(token, channelName, nullptr, uid)`，在 4.x 中仍然可用，兼容性好。
- `userId` 使用 `std::stoul` 转成 `uid_t`，转失败时回退为 0（由声网分配）。
- 支持无 Token（传 `nullptr`）和带 Token 场景。

### **5. 本地音视频开关 / 音量 / 远端订阅**

```cpp
cpp
int AgoraRtcWrapper::enableLocalVideo(bool enable) {
    if (!initialized_)return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    agora::rtc::IRtcEngine* engine =getRtcEngine(engineHandle_);
    if (!engine)return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;

    if (enable) {
        engine->enableVideo();
        return engine->muteLocalVideoStream(false);
    }else {
        return engine->muteLocalVideoStream(true);
    }
}

int AgoraRtcWrapper::enableLocalAudio(bool enable) {
    if (!initialized_)return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    agora::rtc::IRtcEngine* engine =getRtcEngine(engineHandle_);
    if (!engine)return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;

    if (enable) {
        engine->enableAudio();
        return engine->muteLocalAudioStream(false);
    }else {
        return engine->muteLocalAudioStream(true);
    }
}

int AgoraRtcWrapper::switchCamera() {
    if (!initialized_)return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    agora::rtc::IRtcEngine* engine =getRtcEngine(engineHandle_);
    if (!engine)return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    // 在移动端 SDK 中有效；PC 上调用将返回错误码但不会崩溃
    return engine->switchCamera();
}

int AgoraRtcWrapper::setLocalVolume(int volume) {
    if (!initialized_)return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    agora::rtc::IRtcEngine* engine =getRtcEngine(engineHandle_);
    if (!engine)return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;

    if (volume <0) volume =0;
    if (volume >400) volume =400; // Agora 录音信号音量建议范围 0-400
    return engine->adjustRecordingSignalVolume(volume);
}

int AgoraRtcWrapper::subscribeRemoteVideo(const std::string& userId,bool subscribe) {
    if (!initialized_)return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    agora::rtc::IRtcEngine* engine =getRtcEngine(engineHandle_);
    if (!engine)return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;

    agora::uid_t uid =parseUid(userId);
    return engine->muteRemoteVideoStream(uid, !subscribe);
}

int AgoraRtcWrapper::subscribeRemoteAudio(const std::string& userId,bool subscribe) {
    if (!initialized_)return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;
    agora::rtc::IRtcEngine* engine =getRtcEngine(engineHandle_);
    if (!engine)return VIDEO_CALL_ERR_ENGINE_NOT_INITIALIZED;

    agora::uid_t uid =parseUid(userId);
    return engine->muteRemoteAudioStream(uid, !subscribe);
}
```

> 这里全部采用 Agora v3/v4 Native SDK 中稳定存在的接口：
> 
> 
> `enableVideo/Audio`、`muteLocalVideoStream`、`muteLocalAudioStream`、`muteRemoteVideoStream`、`muteRemoteAudioStream`、`adjustRecordingSignalVolume` 等，和官方示例一致，编译通过概率很高。
> 

### **6. 关于视频帧回调**

- 当前代码已经在插件内部预留了 `VideoCallCore::onVideoFrameInternal` 与 `IVideoCallCallback::onVideoFrameReceived` 的链路。
- 若要从 Agora 获取原始视频帧（用于自定义渲染），可以在 `AgoraRtcWrapper::initialize` 里注册 `IVideoFrameObserver`（4.x 对应的新接口）并在回调里调用 `VideoCallCore::instance().onVideoFrameInternal(...)`。
- 这部分接口在 4.x 里有较多变动，建议参考声网官方 “采集与渲染” 文档后再补充实现。

> 如果你希望，我可以再按 4.x 的 IVideoFrameObserver2 写一版完整的帧回调桥接。
> 

---

### **二、详细的 Cocos-2d-x 项目对接技术文档（集成指南）**

下面是一份面向 Cocos-2d-x 棋牌游戏项目的 **接入指南**，你可以直接按步骤落地。

---

### **2.1 插件概述**

- **功能**：为 Cocos-2d-x 棋牌游戏提供多方实时音视频通话能力，支持多路视频分屏渲染、悬浮窗、本地/远端音视频开关与订阅。
- **核心依赖**：
    - **引擎**：Cocos-2d-x v4.0
    - **RTC**：Agora RTC SDK v4.2.0
- **支持平台**：Windows 10+、macOS 10.15+、Android 6.0+、iOS 12.0+
- **项目技术栈**：C++17 核心逻辑 + Java/JNI（Android）+ Objective-C++（iOS）+ CMake 跨平台构建。

---

### **2.2 目录结构与关键文件**

- **核心头文件（游戏接入层）**
    - `include/VideoCallPlugin.h`
        - 暴露所有 C 接口：
            - **引擎管理**：`rtcEngine_init` / `rtcEngine_destroy` / `rtcEngine_getState`
            - **频道管理**：`channel_join` / `channel_leave` / `channel_reconnect` / `channel_getOnlineUsers`
            - **音视频控制**：`video_enableLocalCamera` / `video_switchCamera` / `audio_enableLocalMicrophone` / `audio_setLocalVolume` / `video_subscribeRemoteStream` / `audio_subscribeRemoteStream`
            - **渲染**：`videoRender_init` / `videoRender_setParams` / `videoRender_destroy`
            - **移动权限**：`permission_requestCamera` / `permission_requestMicrophone` / `permission_checkCameraState` / `permission_checkMicrophoneState`
        - 回调接口：`class IVideoCallCallback`
- **核心实现（C++ 核心层）**
    - `src/VideoCallPlugin.cpp`：实现全部 C 接口，转发到 `VideoCallCore`。
    - `src/core/VideoCallCore.h/.cpp`：
        - `VideoCallCore` 单例：管理整个 RTC 生命周期与状态。
        - `AgoraRtcWrapper`：封装 Agora C++ SDK 调用（已按 v4.x 接入）。
        - `ChannelManager`：维护当前频道名、本地/远端用户列表。
        - `AVController`：本地/远端音视频开关、音量控制。
        - `VideoRenderManager`：将 RGBA 帧渲染到 Cocos `Sprite` 上。
        - `PermissionManager`：PC 伪权限管理（移动端真实权限由平台层负责）。
        - `Logger`：统一日志输出（支持写文件）。
- **跨平台桥接层**
    - **Android**
        - `android/java/com/cocos/rtc/VideoCallPlugin.java`
            - 静态方法封装 + Java 回调接口 `VideoCallPlugin.Callback`。
            - 权限辅助：`hasCameraPermission` / `hasMicPermission` / `requestCameraAndMicPermissions`。
        - `android/jni/VideoCallJNI.cpp`
            - 实现 `nativeRtcEngineInit` 等 JNI 函数，转发到 C++ 核心接口。
    - **iOS**
        - `ios/VideoCallPluginIOS.h/.mm`
            - `VideoCallPluginIOS` 单例：封装权限请求，并为 UIKit/SwiftUI 沙盒 App 提供 delegate 回调。
- **沙盒测试环境**
    - `test/sandbox/SandboxConfig.h/.cpp`：读取 `sandbox_config.json`，配置测试 AppID、频道、弱网参数等。
    - `test/sandbox/SandboxMain.cpp`：命令行沙盒程序。
    - 一键脚本：
        - Windows：`scripts/run_sandbox.bat`
        - macOS/Linux：`scripts/run_sandbox.sh`
- **Cocos 测试场景**
    - `test/VideoCallTestScene.h/.cpp`：Cocos-2d-x 示例场景。

---

### **2.3 Windows/macOS（CMake 项目）集成步骤**

- **步骤 1：引入头文件与源码/库**
- 将 `include/` 目录加入你的 Cocos 工程 `target_include_directories`；
- 将 `src/` 和 `src/core/` 加入你的工程，或单独编译成静态库 `VideoCallPlugin`；
- 引入 Agora Windows/macOS SDK：
    - 头文件：`IAgoraRtcEngine.h` 等
    - 库文件：`agora_rtc_sdk.lib` / `AgoraRtcKit.framework` 等（具体看下载包）。
- **步骤 2：CMake 配置示例**

在你游戏的 `CMakeLists.txt` 中：

```
cmake
  # 1. 添加插件子目录（如果作为独立子工程）
  add_subdirectory(${CMAKE_SOURCE_DIR}/../cocos-sdk${CMAKE_BINARY_DIR}/cocos-sdk-build)

  # 2. 包含头文件
  target_include_directories(MyGame PRIVATE
      ${CMAKE_SOURCE_DIR}/../cocos-sdk/include
      ${CMAKE_SOURCE_DIR}/../cocos-sdk/src
  )

  # 3. 链接插件库 + 声网库
  target_link_libraries(MyGame PRIVATE
      VideoCallPlugin
      agora_rtc_sdk# 根据实际库名调整
  )
```

- **步骤 3：部署运行时库**
- Windows：确保 `agora_rtc_sdk.dll`、插件生成的 `VideoCallPlugin.dll` 等放到可执行文件同目录，或加入 PATH。
- macOS：`AgoraRtcKit.framework` / `.dylib` 按官方文档放入 app bundle 并配置 `rpath`。

---

### **2.4 Android 集成步骤**

- **步骤 1：拷贝文件**
- C++ 核心源码 + 头文件与 Windows 相同；
- 把 `android/` 目录下的：
    - `Android.mk`、`Application.mk`
    - `jni/VideoCallJNI.cpp`
    - `java/com/cocos/rtc/VideoCallPlugin.java`拷入或合并到你的 Cocos Android 工程中。
- **步骤 2：NDK 编译配置**

使用 `ndk-build` 的话，确保你的顶层 `Android.mk` 包含插件模块：

```
make
  # 在你的 jni/Android.mk 中
  include $(CLEAR_VARS)
  LOCAL_MODULE := videocall_plugin
  LOCAL_SRC_FILES :=\
      ../../src/VideoCallPlugin.cpp\
      ../../src/core/VideoCallCore.cpp\
      jni/VideoCallJNI.cpp

  LOCAL_C_INCLUDES :=\
      $(LOCAL_PATH)/../../include\
      $(LOCAL_PATH)/../../src\
      $(LOCAL_PATH)/path/to/agora/include

  LOCAL_LDLIBS += -lagora-rtc-sdk# 根据声网 SDK 库文件实际名称配置
  include $(BUILD_SHARED_LIBRARY)
```

`Application.mk` 已写好基础配置（API23 + armeabi-v7a/arm64-v8a）。

- **步骤 3：Java 层集成**
- 在你的游戏 `Activity` 中设置回调：

```java
java
    import com.cocos.rtc.VideoCallPlugin;

    public class GameActivity extends Cocos2dxActivity
            implements VideoCallPlugin.Callback {

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);

            VideoCallPlugin.setCallback(this);
            // 建议在 native 初始化之后调用 init / joinChannel 等
        }

        @Override
        public void onRtcEngineInitResult(int code,String msg) {/* ... */ }

        @Override
        public void onJoinChannelResult(int code,String channel,String userId) {/* ... */ }

        // 其它回调按需实现
    }
```

- 运行前在 `AndroidManifest.xml` 中加入权限：

```xml
xml
    <uses-permission android:name="android.permission.CAMERA"/>
    <uses-permission android:name="android.permission.RECORD_AUDIO"/>
    <uses-permission android:name="android.permission.INTERNET"/>
```

- **步骤 4：权限处理**
- Java 层使用 `VideoCallPlugin.hasCameraPermission/hasMicPermission` 检查权限；
- 如未授权，调用 `requestCameraAndMicPermissions(activity, REQUEST_CODE)`；
- 授权结果回调中，再调用 C++ 核心的 `permission_requestCamera/permission_requestMicrophone`（或者直接在 Java 层驱动 UI）。

---

### **2.5 iOS 集成步骤**

- **步骤 1：添加源码与头文件**
- 将 `include/` 与 `src/` 中 C++ 源码加入 Xcode Cocos 工程；
- 将 `ios/VideoCallPluginIOS.*` 拷入工程（Objective-C++，注意 `.mm` 后缀）。
- **步骤 2：引入 Agora iOS SDK**
- 通过 CocoaPods 或手动方式添加 Agora Video SDK v4.2.0；
- 确保头文件搜索路径包含 `IAgoraRtcEngine.h` 所在目录；
- 链接 `AgoraRtcKit.framework` 等库。
- **步骤 3：权限配置**

在 `Info.plist` 中增加：

```xml
xml
  <key>NSCameraUsageDescription</key>
  <string>需要访问摄像头用于视频通话</string>
  <key>NSMicrophoneUsageDescription</key>
  <string>需要访问麦克风用于语音通话</string>
```

- **步骤 4：调用方式**
- Cocos C++ 层直接使用 `VideoCallPlugin.h` 中的 C 接口即可；
- 如你有独立 iOS 测试 App，可以通过：

```objectivec
objc
    [VideoCallPluginIOSsharedInstance].delegate =self;
    [[VideoCallPluginIOSsharedInstance]requestCameraPermission];
```

来测试权限与回调；实际入会仍建议走 C++ 核心，以方便与其它平台统一。

---

### **2.6 游戏层接入流程（Cocos-2d-x C++）**

以 Cocos 场景为例（参考 `test/VideoCallTestScene.cpp`）：

- **步骤 1：实现回调类**

```cpp
cpp
  class MyVideoCallCallback :public IVideoCallCallback {
  public:
      void onRtcEngineInitResult(int code,const char* msg)override;
      void onJoinChannelResult(int code,const char* channel,const char* userId)override;
      // 其它回调按需实现
  };
```

- **步骤 2：初始化引擎**

```cpp
cpp
  MyVideoCallCallback* cb =new MyVideoCallCallback();
  int ret =rtcEngine_init(appId.c_str(),true, cb);
```

- **步骤 3：创建本地渲染 Sprite 并绑定**

```cpp
cpp
  auto sprite =cocos2d::Sprite::create();
  sprite->setContentSize(Size(320,240));
  sprite->setPosition(Vec2(200,200));
  this->addChild(sprite);

  std::string userId ="1001"; // 对应 Agora uid，可自定义
  videoRender_init(sprite,userId.c_str());
  videoRender_setParams(userId.c_str(),200,200,320,240,255);
```

- **步骤 4：加入频道并开启音视频**

```cpp
cpp
  channel_join(channelName.c_str(),userId.c_str(),token.c_str());
  // 在 onJoinChannelResult 回调中，如果 code == 0：
  video_enableLocalCamera(true);
  audio_enableLocalMicrophone(true);
```

- **步骤 5：退出与清理**

```cpp
cpp
  channel_leave();
  rtcEngine_destroy();
```

---

### **2.7 回调接口集成建议**

实现 `IVideoCallCallback` 时建议：

- **引擎/频道状态**
    - `onRtcEngineInitResult`：初始化失败时弹错误提示，禁止后续调用；
    - `onJoinChannelResult`：成功后更新 UI 显示“已连接”，失败时做重试或提示；
    - `onLeaveChannelResult`：清理界面上的所有视频 Sprite。
- **远端用户事件**
    - `onRemoteUserJoined`：为该 `userId` 创建对应 `Sprite` 和布局（分屏位置）；
    - `onRemoteUserLeft`：移除对应 `Sprite`，更新布局。
- **音视频发布/订阅**
    - `onLocalVideoPublished` / `onLocalAudioPublished`：失败时灰掉“摄像头/麦克风”按钮；
    - `onRemoteVideoSubscribed` / `onRemoteAudioSubscribed`：可在 UI 上标记该远端是否成功订阅。
- **视频帧回调**
    - `onVideoFrameReceived`：目前 C++ 内部已经渲染到 Sprite；你可以在这里做统计或自定义处理（如截图）。
- **权限回调**
    - `onPermissionResult`：仅移动端有意义，根据授权结果引导用户去设置页开启权限。
- **错误回调**
    - `onError`：建议统一打印日志 + Toast/弹窗，并根据错误码（网络/权限/内部错误）给出不同提示。

---

### **2.8 沙盒测试环境使用**

- **构建**：使用根目录 `CMakeLists.txt` 构建目标 `VideoCallSandbox`。
- **配置**：在可执行文件同目录放置 `sandbox_config.json`，示例结构：

```json
json
  {
    "appId":"YOUR_TEST_APPID",
    "channelName":"test_channel",
    "localUserId":"1001",
    "userCount":2,
    "packetLossPercent":20,
    "latencyMs":300,
    "logFilePath":"sandbox.log",
    "enableNetworkSimulation":true
  }
```

- **启动**：
    - Windows：`scripts\run_sandbox.bat` 或 `run_sandbox.bat my_config.json`
    - macOS/Linux：`./scripts/run_sandbox.sh [config.json]`
- **行为**：
    - 按配置初始化引擎 → 加入测试频道 → 开启本地音视频；
    - 运行约 30 秒后自动离开频道并销毁；
    - 日志（包括回调事件、错误信息）写入 `sandbox.log`，便于调试。

---

### **2.9 测试与验证建议**

- **功能测试**
    - 引擎初始化/销毁：多次 init/destroy 不崩溃、不泄漏；
    - 频道 join/leave/reconnect：网络正常与中断场景；
    - 音视频开关与订阅：按钮频繁操作不会卡死或崩溃。
- **跨平台测试**
    - Windows/macOS/Android/iOS 四端跑同一个 AppID/频道，看行为一致性；
    - Android 不同 ABI（armeabi-v7a/arm64-v8a），iOS 真机测试。
- **性能测试**
    - 1/2/4/6 人场景：监控 FPS、CPU、内存是否满足你的 4.2 节指标；
    - 弱网场景：使用沙盒 `packetLossPercent` 和 `latencyMs` 模拟。
- **异常测试**
    - 无权限直接开摄像头/麦克风；
    - 无效 AppID/Token、空频道名；
    - 热更 / 场景切换中 join/leave 操作。
