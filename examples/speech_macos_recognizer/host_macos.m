/* SPDX-License-Identifier: MIT */
#import "host_macos_private.h"
#import <Speech/Speech.h>
#include <stdlib.h>
#include <string.h>

struct speech_macos_service {
    void *backend;
};
@interface CUIEndpointLease : NSObject
@property(nonatomic, assign) speech_recognizer_endpoint *endpoint;
- (instancetype)initWithEndpoint:(speech_recognizer_endpoint *)endpoint;
@end
@implementation CUIEndpointLease
- (instancetype)initWithEndpoint:(speech_recognizer_endpoint *)endpoint {
    self = [super init];
    if (self) {
        if (!speech_recognizer_endpoint_retain(endpoint))
            return nil;
        _endpoint = endpoint;
    }
    return self;
}
- (void)dealloc {
    speech_recognizer_endpoint_release(_endpoint);
}
@end
@interface CUIServiceOperation : NSObject
@property(nonatomic, strong) CUIEndpointLease *lease;
@property(nonatomic, strong) id<CUISpeechRequest> request;
@property(nonatomic, strong) id<CUISpeechTask> task;
@property(nonatomic, strong) AVAudioFormat *format;
@property(nonatomic) BOOL finished;
@end
@implementation CUIServiceOperation
@end

static id<CUISpeechBackend> backend(speech_macos_service *service) {
    return (__bridge id<CUISpeechBackend>)service->backend;
}
static bool owner(void) {
    return [NSThread isMainThread];
}
static speech_service_status snapshot(void *user) {
    return owner() ? [backend(user) snapshot] : (speech_service_status){0};
}
static cui_speech_error authorize(void *user, speech_recognizer_endpoint *endpoint, void **token) {
    *token = NULL;
    if (!owner())
        return CUI_SPEECH_INVALID;
    speech_service_status status = snapshot(user);
    if (!status.supported || !status.usage_description || !status.locale_supported)
        return CUI_SPEECH_UNSUPPORTED;
    if (status.authorization != SPEECH_AUTH_UNKNOWN)
        return CUI_SPEECH_PERMISSION;
    CUIEndpointLease *lease = [[CUIEndpointLease alloc] initWithEndpoint:endpoint];
    if (!lease)
        return CUI_SPEECH_CAPACITY;
    CUIServiceOperation *operation = [CUIServiceOperation new];
    operation.lease = lease;
    /* The block owns only the lease, never a raw service/host/sink or wake target. */
    [backend(user) authorize:^(speech_authorization auth) {
      (void)speech_recognizer_endpoint_authorized(lease.endpoint, auth);
    }];
    *token = (__bridge_retained void *)operation;
    return CUI_SPEECH_OK;
}
static cui_speech_error start(void *user, speech_recognizer_endpoint *endpoint, bool on_device,
                              void **token) {
    *token = NULL;
    if (!owner() || !on_device)
        return CUI_SPEECH_INVALID;
    speech_service_status status = snapshot(user);
    if (!status.supported || !status.usage_description || !status.locale_supported ||
        !status.on_device || !status.available ||
        !speech_recognizer_format_supported(status.format))
        return CUI_SPEECH_UNSUPPORTED;
    if (status.authorization != SPEECH_AUTH_GRANTED)
        return CUI_SPEECH_PERMISSION;
    CUIEndpointLease *lease = [[CUIEndpointLease alloc] initWithEndpoint:endpoint];
    if (!lease)
        return CUI_SPEECH_CAPACITY;
    CUIServiceOperation *operation = [CUIServiceOperation new];
    operation.lease = lease;
    operation.request = [backend(user) newRequest];
    operation.format = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatInt16
                                                        sampleRate:16000
                                                          channels:1
                                                       interleaved:NO];
    if (!operation.request || !operation.format)
        return CUI_SPEECH_RECOGNIZER_ERROR;
    operation.request.shouldReportPartialResults = YES;
    operation.request.requiresOnDeviceRecognition = YES;
    operation.request.addsPunctuation = YES;
    operation.task = [backend(user)
        startRequest:operation.request
              result:^(NSString *text, BOOL final, BOOL ends, NSError *error) {
                @autoreleasepool {
                    char bytes[SPEECH_RECOGNIZER_TEXT];
                    NSUInteger used = 0;
                    NSRange remaining = NSMakeRange(0, 0);
                    NSUInteger length =
                        text ? [text lengthOfBytesUsingEncoding:NSUTF8StringEncoding] : 0;
                    if (length >= sizeof bytes) {
                        (void)speech_recognizer_endpoint_error(lease.endpoint, CUI_SPEECH_CAPACITY);
                        return;
                    }
                    if (text.length && ![text getBytes:bytes
                                                maxLength:sizeof bytes - 1
                                               usedLength:&used
                                                 encoding:NSUTF8StringEncoding
                                                  options:0
                                                    range:NSMakeRange(0, text.length)
                                           remainingRange:&remaining]) {
                        (void)speech_recognizer_endpoint_error(lease.endpoint,
                                                               CUI_SPEECH_RECOGNIZER_ERROR);
                        return;
                    }
                    if (remaining.length || used != length) {
                        (void)speech_recognizer_endpoint_error(lease.endpoint,
                                                               CUI_SPEECH_RECOGNIZER_ERROR);
                        return;
                    }
                    (void)speech_recognizer_endpoint_result(
                        lease.endpoint, (cui_str){bytes, used}, final, ends,
                        error ? CUI_SPEECH_RECOGNIZER_ERROR : CUI_SPEECH_OK);
                }
              }];
    if (!operation.task)
        return CUI_SPEECH_RECOGNIZER_ERROR;
    *token = (__bridge_retained void *)operation;
    return CUI_SPEECH_OK;
}
static cui_speech_error append(void *user, void *token, const int16_t *samples, size_t count) {
    (void)user;
    if (!owner() || !token || !samples || count > 256)
        return CUI_SPEECH_INVALID;
    CUIServiceOperation *operation = (__bridge CUIServiceOperation *)token;
    if (operation.finished)
        return CUI_SPEECH_INVALID;
    if (!count)
        return CUI_SPEECH_OK;
    AVAudioPCMBuffer *buffer =
        [[AVAudioPCMBuffer alloc] initWithPCMFormat:operation.format
                                      frameCapacity:(AVAudioFrameCount)count];
    if (!buffer || !buffer.int16ChannelData)
        return CUI_SPEECH_CAPACITY;
    memcpy(buffer.int16ChannelData[0], samples, count * sizeof *samples);
    buffer.frameLength = (AVAudioFrameCount)count;
    [operation.request appendAudioPCMBuffer:buffer];
    return CUI_SPEECH_OK;
}
static void finish(void *user, void *token) {
    (void)user;
    if (!owner())
        abort();
    CUIServiceOperation *operation = (__bridge CUIServiceOperation *)token;
    if (!operation.finished) {
        operation.finished = YES;
        [operation.request endAudio];
    }
}
static void close_token(void *user, void *token) {
    (void)user;
    if (!owner())
        abort();
    CUIServiceOperation *operation = (__bridge_transfer CUIServiceOperation *)token;
    [operation.task cancel];
    /* Releasing operation does not promise Apple's retained block is gone. */
}
speech_macos_service *speech_macos_service_create_injected(id<CUISpeechBackend> value) {
    if (!owner() || !value)
        return NULL;
    speech_macos_service *service = calloc(1, sizeof *service);
    if (service)
        service->backend = (__bridge_retained void *)value;
    return service;
}
speech_recognition_service speech_macos_service_ops(speech_macos_service *service) {
    return service ? (speech_recognition_service){service, snapshot, authorize,  start,
                                                  append,  finish,   close_token}
                   : (speech_recognition_service){0};
}
void speech_macos_service_destroy(speech_macos_service *service) {
    if (!service)
        return;
    if (!owner())
        abort();
    id value = (__bridge_transfer id)service->backend;
    (void)value;
    free(service);
}

@interface CUIAppleBackend : NSObject <CUISpeechBackend>
@property(nonatomic, strong) NSLocale *locale;
@property(nonatomic, strong) SFSpeechRecognizer *recognizer;
@end
@implementation CUIAppleBackend
- (speech_service_status)snapshot {
    speech_service_status status = {.supported = true, .format = {16000, 1}};
    id description =
        [[NSBundle mainBundle] objectForInfoDictionaryKey:@"NSSpeechRecognitionUsageDescription"];
    status.usage_description =
        [description isKindOfClass:[NSString class]] &&
        [[description
            stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]]
            length] > 0;
    if (!status.usage_description)
        return status;
    switch ([SFSpeechRecognizer authorizationStatus]) {
    case SFSpeechRecognizerAuthorizationStatusAuthorized:
        status.authorization = SPEECH_AUTH_GRANTED;
        break;
    case SFSpeechRecognizerAuthorizationStatusDenied:
        status.authorization = SPEECH_AUTH_DENIED;
        break;
    case SFSpeechRecognizerAuthorizationStatusRestricted:
        status.authorization = SPEECH_AUTH_RESTRICTED;
        break;
    default:
        status.authorization = SPEECH_AUTH_UNKNOWN;
        break;
    }
    if (!self.recognizer)
        self.recognizer = [[SFSpeechRecognizer alloc] initWithLocale:self.locale];
    status.locale_supported = self.recognizer != nil;
    status.on_device = self.recognizer.supportsOnDeviceRecognition;
    status.available = self.recognizer.available;
    return status;
}
- (void)authorize:(void (^)(speech_authorization))completion {
    [SFSpeechRecognizer requestAuthorization:^(SFSpeechRecognizerAuthorizationStatus status) {
      speech_authorization auth =
          status == SFSpeechRecognizerAuthorizationStatusAuthorized   ? SPEECH_AUTH_GRANTED
          : status == SFSpeechRecognizerAuthorizationStatusRestricted ? SPEECH_AUTH_RESTRICTED
                                                                      : SPEECH_AUTH_DENIED;
      completion(auth);
    }];
}
- (id<CUISpeechRequest>)newRequest {
    return (id<CUISpeechRequest>)[SFSpeechAudioBufferRecognitionRequest new];
}
- (id<CUISpeechTask>)startRequest:(id<CUISpeechRequest>)request result:(CUISpeechResult)result {
    if (!self.recognizer.supportsOnDeviceRecognition || !self.recognizer.available ||
        !request.requiresOnDeviceRecognition ||
        [SFSpeechRecognizer authorizationStatus] != SFSpeechRecognizerAuthorizationStatusAuthorized)
        return nil;
    return (id<CUISpeechTask>)[self.recognizer
        recognitionTaskWithRequest:(SFSpeechAudioBufferRecognitionRequest *)request
                     resultHandler:^(SFSpeechRecognitionResult *recognition, NSError *error) {
                       result(recognition.bestTranscription.formattedString, recognition.isFinal,
                              recognition.speechRecognitionMetadata != nil, error);
                     }];
}
@end
speech_macos_service *speech_macos_service_create(const char *locale) {
    if (!owner() || (locale && strlen(locale) > 255))
        return NULL;
    NSString *tag = locale ? [[NSString alloc] initWithUTF8String:locale] : nil;
    if (locale && (!tag || !tag.length))
        return NULL;
    CUIAppleBackend *value = [CUIAppleBackend new];
    value.locale = tag ? [[NSLocale alloc] initWithLocaleIdentifier:tag] : [NSLocale currentLocale];
    return speech_macos_service_create_injected(value);
}
