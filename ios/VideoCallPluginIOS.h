#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// iOS 端视频通话插件委托协议。
///
/// 一般情况下，游戏层使用 C++ 回调接口即可。
/// 该协议主要用于：
/// - 沙盒测试 App (UIKit/SwiftUI) 中直接接收插件状态；
/// - 调试 iOS 平台差异化行为和权限结果。
@protocol VideoCallPluginDelegate <NSObject>

- (void)onRtcEngineInitResult:(NSInteger)code message:(NSString *)msg;
- (void)onRtcEngineDestroyed;

- (void)onJoinChannelResult:(NSInteger)code
                channelName:(NSString *)channelName
                     userId:(NSString *)userId;

- (void)onLeaveChannelResult:(NSInteger)code;

- (void)onRemoteUserJoined:(NSString *)userId;
- (void)onRemoteUserLeft:(NSString *)userId reason:(NSInteger)reason;

- (void)onLocalVideoPublished:(NSInteger)code;
- (void)onLocalAudioPublished:(NSInteger)code;

- (void)onRemoteVideoSubscribed:(NSInteger)code userId:(NSString *)userId;
- (void)onRemoteAudioSubscribed:(NSInteger)code userId:(NSString *)userId;

- (void)onPermissionResultWithType:(NSInteger)type code:(NSInteger)code;
- (void)onError:(NSInteger)code message:(NSString *)msg;

@end

/// iOS 封装单例，负责：
/// - 对接 C++ 核心插件接口；
/// - 使用 Objective‑C API 处理权限；
/// - 给可视化沙盒测试 App 提供便捷调用入口。
@interface VideoCallPluginIOS : NSObject

@property (nonatomic, weak, nullable) id<VideoCallPluginDelegate> delegate;

+ (instancetype)sharedInstance;

- (int)initializeWithAppId:(NSString *)appId debugMode:(BOOL)debugMode;
- (int)destroy;

- (int)joinChannel:(NSString *)channelName
            userId:(NSString *)userId
             token:(nullable NSString *)token;

- (int)leaveChannel;
- (int)reconnectChannel;

- (int)enableLocalCamera:(BOOL)enable;
- (int)switchCamera;

- (int)enableLocalMicrophone:(BOOL)enable;
- (int)setLocalVolume:(NSInteger)volume;

- (int)subscribeRemoteVideo:(NSString *)userId subscribe:(BOOL)subscribe;
- (int)subscribeRemoteAudio:(NSString *)userId subscribe:(BOOL)subscribe;

// 权限相关接口（内部会调用 iOS 原生 API）
- (int)requestCameraPermission;
- (int)requestMicrophonePermission;
- (int)checkCameraPermissionState;
- (int)checkMicrophonePermissionState;

@end

NS_ASSUME_NONNULL_END
