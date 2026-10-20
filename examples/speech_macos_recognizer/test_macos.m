/* SPDX-License-Identifier: MIT */
/* Headless native transport fixture: never constructs an Apple recognizer,
 * requests OS authorization, opens a device, or creates a recognition task. */
#import "host_macos_private.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                                \
            abort();                                                                               \
        }                                                                                          \
    } while (0)
@interface FakeRequest : NSObject <CUISpeechRequest>
@property(nonatomic) BOOL shouldReportPartialResults;
@property(nonatomic) BOOL requiresOnDeviceRecognition;
@property(nonatomic) BOOL addsPunctuation;
@property(nonatomic) unsigned samples;
@property(nonatomic) unsigned ended;
@property(nonatomic) int16_t last;
@end
@implementation FakeRequest
- (void)appendAudioPCMBuffer:(AVAudioPCMBuffer *)buffer {
    CHECK(buffer.format.sampleRate == 16000 && buffer.format.channelCount == 1);
    CHECK(buffer.frameLength > 0 && buffer.frameLength <= 256 && buffer.int16ChannelData);
    self.samples += buffer.frameLength;
    self.last = buffer.int16ChannelData[0][buffer.frameLength - 1];
}
- (void)endAudio {
    self.ended++;
}
@end
@interface FakeTask : NSObject <CUISpeechTask>
@property(nonatomic) unsigned cancelled;
@end
@implementation FakeTask
- (void)cancel {
    self.cancelled++;
}
@end
@interface FakeBackend : NSObject <CUISpeechBackend>
@property(nonatomic) speech_service_status status;
@property(nonatomic, copy) CUISpeechResult callback;
@property(nonatomic, copy) void (^authorization)(speech_authorization);
@property(nonatomic, strong) FakeRequest *request;
@property(nonatomic, strong) FakeTask *task;
@property(nonatomic) unsigned authorizationRequests;
@property(nonatomic) unsigned starts;
@end
@implementation FakeBackend
- (speech_service_status)snapshot {
    return self.status;
}
- (void)authorize:(void (^)(speech_authorization))completion {
    self.authorizationRequests++;
    self.authorization = completion;
}
- (id<CUISpeechRequest>)newRequest {
    self.request = [FakeRequest new];
    return self.request;
}
- (id<CUISpeechTask>)startRequest:(id<CUISpeechRequest>)request result:(CUISpeechResult)result {
    CHECK(request.requiresOnDeviceRecognition && request.shouldReportPartialResults &&
          request.addsPunctuation);
    self.starts++;
    self.callback = result;
    self.task = [FakeTask new];
    return self.task;
}
@end
typedef struct {
    cui_speech_state state;
    cui_speech_sink sink;
    char transcript[16384];
    cui_speech_message messages[32];
    unsigned char bytes[32768];
    unsigned errors, finals, input_closed;
    cui_speech_error error;
} fixture;
static cui_speech_error input_start(void *user, cui_speech_format format, cui_speech_sink sink,
                                    void **token) {
    fixture *f = user;
    CHECK(speech_recognizer_format_supported(format));
    f->sink = sink;
    *token = f;
    return CUI_SPEECH_OK;
}
static void input_close(void *user, void *token) {
    CHECK(user == token);
    ((fixture *)user)->input_closed++;
}
static void event(void *user, cui_speech_event event) {
    fixture *f = user;
    if (event.type == CUI_SPEECH_ERROR) {
        f->errors++;
        f->error = event.error;
    }
    if (event.type == CUI_SPEECH_FINAL)
        f->finals++;
}
static FakeBackend *fake(void) {
    FakeBackend *value = [FakeBackend new];
    value.status =
        (speech_service_status){true, true, true, true, true, SPEECH_AUTH_GRANTED, {16000, 1}};
    return value;
}
static speech_recognizer_host *host(speech_macos_service *service) {
    speech_recognizer_host *h = speech_recognizer_host_create(
        speech_macos_service_ops(service), speech_recognizer_posix_mutex(), NULL, NULL);
    CHECK(h);
    return h;
}
static void initialize(fixture *f, speech_recognizer_host *h) {
    memset(f, 0, sizeof *f);
    CHECK(cui_speech_init(&f->state,
                          (cui_speech_workspace){f->transcript, sizeof f->transcript, f->messages,
                                                 32, f->bytes, sizeof f->bytes},
                          speech_recognizer_host_provider(h),
                          (cui_speech_input){f, input_start, input_close}, event, f));
}
static void poll(fixture *f, speech_recognizer_host *h) {
    (void)speech_recognizer_host_poll(h, 8);
    (void)cui_speech_pump(&f->state, 1);
}
int main(void) {
    @autoreleasepool {
        FakeBackend *backend = fake();
        speech_macos_service *service = speech_macos_service_create_injected(backend);
        speech_recognizer_host *h = host(service);
        fixture f;
        initialize(&f, h);
        CHECK(cui_speech_start(&f.state));
        poll(&f, h);
        int16_t pcm[600];
        for (size_t i = 0; i < 600; i++)
            pcm[i] = (int16_t)i;
        CHECK(cui_speech_report_audio(f.sink, pcm, 600, 1));
        poll(&f, h);
        CHECK(backend.request.samples == 600 && backend.request.last == 599);
        backend.callback(@"你好", NO, NO, nil);
        poll(&f, h);
        CHECK(!strcmp(f.transcript, "你好"));
        CHECK(cui_speech_stop(&f.state, 1));
        CHECK(backend.request.ended == 1);
        backend.callback(@"你好。", YES, NO, nil);
        poll(&f, h);
        CHECK(f.finals == 1 && !f.errors && backend.task.cancelled == 1 && f.input_closed == 1);
        cui_speech_dispose(&f.state);
        CHECK(speech_recognizer_host_destroy(h));
        speech_macos_service_destroy(service);
        backend.callback(@"late after host/service free", YES, NO, nil);
        backend.callback = nil;

        backend = fake();
        service = speech_macos_service_create_injected(backend);
        h = host(service);
        initialize(&f, h);
        CHECK(cui_speech_start(&f.state));
        NSString *oversized = [@"x" stringByPaddingToLength:SPEECH_RECOGNIZER_TEXT
                                                 withString:@"x"
                                            startingAtIndex:0];
        backend.callback(oversized, NO, NO, nil);
        poll(&f, h);
        CHECK(f.errors == 1 && f.error == CUI_SPEECH_CAPACITY);
        cui_speech_dispose(&f.state);
        CHECK(speech_recognizer_host_destroy(h));
        speech_macos_service_destroy(service);
        backend.callback = nil;

        backend = fake();
        service = speech_macos_service_create_injected(backend);
        h = host(service);
        initialize(&f, h);
        CHECK(cui_speech_start(&f.state));
        backend.callback(@"draft despite final", YES, NO,
                         [NSError errorWithDomain:@"Fixture" code:1 userInfo:nil]);
        poll(&f, h);
        CHECK(f.errors == 1 && !f.finals && !strcmp(f.transcript, "draft despite final"));
        cui_speech_dispose(&f.state);
        CHECK(speech_recognizer_host_destroy(h));
        speech_macos_service_destroy(service);
        backend.callback = nil;

        backend = fake();
        speech_service_status status = backend.status;
        status.authorization = SPEECH_AUTH_UNKNOWN;
        status.usage_description = false;
        backend.status = status;
        service = speech_macos_service_create_injected(backend);
        h = host(service);
        CHECK(speech_recognizer_host_prepare(h) == CUI_SPEECH_UNSUPPORTED &&
              !backend.authorizationRequests);
        status.usage_description = true;
        backend.status = status;
        CHECK(speech_recognizer_host_prepare(h) == CUI_SPEECH_OK &&
              backend.authorizationRequests == 1);
        CHECK(speech_recognizer_host_destroy(h));
        speech_macos_service_destroy(service);
        backend.authorization(SPEECH_AUTH_GRANTED); /* Late block owns endpoint only. */
        backend.authorization = nil;
        puts("PASS: injected ObjC policy setters, PCM/UTF-8, retained blocks and late shutdown");
    }
    return 0;
}
