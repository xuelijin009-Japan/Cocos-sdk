#import "VideoCallPluginIOS.h"

#import <AVFoundation/AVFoundation.h>

#import "VideoCallPlugin.h"

@interface IOSVideoCallCallback : NSObject <NSObject>
@end

@implementation IOSVideoCallCallback
@end

// 使用 C++ 回调接口，将事件再转发给 Objective‑C delegate。
// 为简化，此处直接在 C 接口使用时，由游戏层传入 C++ 回调为主；
// iOS 封装主要负责权限和沙盒 App 调用入口。

@interface VideoCallPluginIOS ()
@end

@implementation VideoCallPluginIOS

+ (instancetype)sharedInstance {
    static VideoCallPluginIOS* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[VideoCallPluginIOS alloc] init];
    });
    return instance;
}

- (int)initializeWithAppId:(NSString *)appId debugMode:(BOOL)debugMode {
    // 这里只做简单的参数校验，真正的初始化由 C++ 层完成
    if (appId.length == 0) {
        if (self.delegate) {
            [self.delegate onError:1002 message:@"appId is empty"];
        }
        return 1002;
    }
    // 由游戏 C++ 层调用 rtcEngine_init 进行实际初始化
    return 0;
}

- (int)destroy {
    // 销毁同样通过 C++ 接口完成
    return 0;
}

- (int)joinChannel:(NSString *)channelName userId:(NSString *)userId token:(NSString *)token {
    if (channelName.length == 0) {
        if (self.delegate) {
            [self.delegate onError:2001 message:@"channel name empty"];
        }
        return 2001;
    }
    // 实际 join 由 C++ 接口 channel_join 完成
    return 0;
}

- (int)leaveChannel {
    return 0;
}

- (int)reconnectChannel {
    return 0;
}

- (int)enableLocalCamera:(BOOL)enable {
    return 0;
}

- (int)switchCamera {
    return 0;
}

- (int)enableLocalMicrophone:(BOOL)enable {
    return 0;
}

- (int)setLocalVolume:(NSInteger)volume {
    return 0;
}

- (int)subscribeRemoteVideo:(NSString *)userId subscribe:(BOOL)subscribe {
    return 0;
}

- (int)subscribeRemoteAudio:(NSString *)userId subscribe:(BOOL)subscribe {
    return 0;
}

#pragma mark - 权限

- (int)requestCameraPermission {
    AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    if (status == AVAuthorizationStatusAuthorized) {
        if (self.delegate) {
            [self.delegate onPermissionResultWithType:0 code:0];
        }
        return 0;
    }
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
        dispatch_async(dispatch_get_main_queue(), ^{
            if (self.delegate) {
                [self.delegate onPermissionResultWithType:0 code:(granted ? 0 : 1)];
            }
        });
    }];
    return 0;
}

- (int)requestMicrophonePermission {
    AVAudioSessionRecordPermission permission = [[AVAudioSession sharedInstance] recordPermission];
    if (permission == AVAudioSessionRecordPermissionGranted) {
        if (self.delegate) {
            [self.delegate onPermissionResultWithType:1 code:0];
        }
        return 0;
    }
    [[AVAudioSession sharedInstance] requestRecordPermission:^(BOOL granted) {
        dispatch_async(dispatch_get_main_queue(), ^{
            if (self.delegate) {
                [self.delegate onPermissionResultWithType:1 code:(granted ? 0 : 1)];
            }
        });
    }];
    return 0;
}

- (int)checkCameraPermissionState {
    AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    switch (status) {
        case AVAuthorizationStatusAuthorized:
            return 0;
        case AVAuthorizationStatusNotDetermined:
            return 2;
        default:
            return 1;
    }
}

- (int)checkMicrophonePermissionState {
    AVAudioSessionRecordPermission permission = [[AVAudioSession sharedInstance] recordPermission];
    switch (permission) {
        case AVAudioSessionRecordPermissionGranted:
            return 0;
        case AVAudioSessionRecordPermissionUndetermined:
            return 2;
        default:
            return 1;
    }
}

@end
