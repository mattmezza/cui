/* SPDX-License-Identifier: MIT */
#ifndef SPEECH_RECOGNIZER_MACOS_PRIVATE_H
#define SPEECH_RECOGNIZER_MACOS_PRIVATE_H
/* Private Objective-C service seam: never include from a consumer C header. */
#import "host_macos.h"
#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
@protocol CUISpeechRequest <NSObject>
@property(nonatomic) BOOL shouldReportPartialResults;
@property(nonatomic) BOOL requiresOnDeviceRecognition;
@property(nonatomic) BOOL addsPunctuation;
- (void)appendAudioPCMBuffer:(AVAudioPCMBuffer *)buffer;
- (void)endAudio;
@end
@protocol CUISpeechTask <NSObject>
- (void)cancel;
@end
typedef void (^CUISpeechResult)(NSString *, BOOL, BOOL, NSError *);
@protocol CUISpeechBackend <NSObject>
- (speech_service_status)snapshot;
- (void)authorize:(void (^)(speech_authorization))completion;
- (id<CUISpeechRequest>)newRequest;
- (id<CUISpeechTask>)startRequest:(id<CUISpeechRequest>)request result:(CUISpeechResult)result;
@end
/* CI injects an ObjC backend, so production transport runs without constructing
 * SFSpeechRecognizer or invoking a real permission/task API. */
speech_macos_service *speech_macos_service_create_injected(id<CUISpeechBackend>);
#endif
