/* SPDX-License-Identifier: MIT */
#include "host.h"
#include <pthread.h>
#include <stdatomic.h>
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
typedef struct fixture fixture;
typedef struct {
    fixture *f;
    speech_recognizer_endpoint *endpoint;
} token;
struct fixture {
    speech_service_status status;
    speech_recognizer_host *host;
    speech_recognizer_endpoint *latest, *late[8];
    size_t late_count;
    unsigned starts, authorizations, closes, finishes, wakes, input_closed, events[5];
    bool keep_late, wake_failure, synchronous, fail_start;
    cui_speech_error append_error, last_error;
    int16_t pcm[1024];
    size_t pcm_count;
    pthread_t owner;
    atomic_uint *acks[2];
    cui_speech_state state;
    char transcript[32768];
    cui_speech_message messages[64];
    unsigned char queue[65536];
};
static cui_str text(const char *s) {
    return (cui_str){s, strlen(s)};
}
static speech_service_status query(void *user) {
    return ((fixture *)user)->status;
}
static cui_speech_error operation(fixture *f, speech_recognizer_endpoint *e, void **out) {
    token *t = malloc(sizeof *t);
    CHECK(t && speech_recognizer_endpoint_retain(e));
    *t = (token){f, e};
    *out = t;
    f->latest = e;
    if (f->keep_late) {
        CHECK(f->late_count < 8 && speech_recognizer_endpoint_retain(e));
        f->late[f->late_count++] = e;
    }
    return CUI_SPEECH_OK;
}
static cui_speech_error authorize(void *user, speech_recognizer_endpoint *e, void **out) {
    fixture *f = user;
    f->authorizations++;
    return operation(f, e, out);
}
static cui_speech_error start(void *user, speech_recognizer_endpoint *e, bool on_device,
                              void **out) {
    fixture *f = user;
    CHECK(on_device && pthread_equal(pthread_self(), f->owner));
    f->starts++;
    cui_speech_error error = operation(f, e, out);
    if (f->synchronous)
        CHECK(speech_recognizer_endpoint_result(e, text("sync"), false, false, CUI_SPEECH_OK));
    return f->fail_start ? CUI_SPEECH_RECOGNIZER_ERROR : error;
}
static cui_speech_error append(void *user, void *opaque, const int16_t *samples, size_t count) {
    fixture *f = user;
    token *t = opaque;
    CHECK(t->f == f && pthread_equal(pthread_self(), f->owner));
    CHECK(count <= 256 && f->pcm_count + count <= 1024);
    memcpy(f->pcm + f->pcm_count, samples, count * sizeof *samples);
    f->pcm_count += count;
    return f->append_error;
}
static void finish(void *user, void *opaque) {
    fixture *f = user;
    CHECK(((token *)opaque)->f == f);
    f->finishes++;
}
static void close_token(void *user, void *opaque) {
    fixture *f = user;
    token *t = opaque;
    CHECK(t->f == f);
    /* A completion during release is already detached, regardless of thread. */
    CHECK(!speech_recognizer_endpoint_error(t->endpoint, CUI_SPEECH_RECOGNIZER_ERROR));
    speech_recognizer_endpoint_release(t->endpoint);
    free(t);
    f->closes++;
}
static bool wake(void *user) {
    fixture *f = user;
    f->wakes++;
    return !f->wake_failure;
}
static cui_speech_error input_start(void *user, cui_speech_format format, cui_speech_sink sink,
                                    void **out) {
    (void)sink;
    CHECK(speech_recognizer_format_supported(format));
    *out = user;
    return CUI_SPEECH_OK;
}
static void input_close(void *user, void *opaque) {
    fixture *f = user;
    CHECK(opaque == user);
    f->input_closed++;
}
static void event(void *user, cui_speech_event e) {
    fixture *f = user;
    CHECK(pthread_equal(pthread_self(), f->owner));
    f->events[e.type]++;
    f->last_error = e.error;
    if (e.type == CUI_SPEECH_PARTIAL && f->acks[0]) {
        unsigned worker, sequence;
        CHECK(sscanf(e.text.data, "worker %u %u", &worker, &sequence) == 2 && worker < 2);
        CHECK(sequence == atomic_load(f->acks[worker]) + 1);
        atomic_store_explicit(f->acks[worker], sequence, memory_order_release);
    }
}
static void init(fixture *f) {
    memset(f, 0, sizeof *f);
    f->owner = pthread_self();
    f->status =
        (speech_service_status){true, true, true, true, true, SPEECH_AUTH_GRANTED, {16000, 1}};
    f->host = speech_recognizer_host_create(
        (speech_recognition_service){f, query, authorize, start, append, finish, close_token},
        speech_recognizer_posix_mutex(), wake, f);
    CHECK(f->host);
    CHECK(cui_speech_init(&f->state,
                          (cui_speech_workspace){f->transcript, sizeof f->transcript, f->messages,
                                                 64, f->queue, sizeof f->queue},
                          speech_recognizer_host_provider(f->host),
                          (cui_speech_input){f, input_start, input_close}, event, f));
}
static void poll(fixture *f) {
    (void)speech_recognizer_host_poll(f->host, 8);
    (void)cui_speech_pump(&f->state, 1);
}
static void drop(fixture *f) {
    cui_speech_dispose(&f->state);
    CHECK(speech_recognizer_host_destroy(f->host));
    f->host = NULL;
}
static void emit(fixture *f, const char *s, bool final, bool ends) {
    CHECK(speech_recognizer_endpoint_result(f->latest, text(s), final, ends, CUI_SPEECH_OK));
    poll(f);
}
static void release_late(fixture *f) {
    for (size_t i = 0; i < f->late_count; i++)
        speech_recognizer_endpoint_release(f->late[i]);
    f->late_count = 0;
}
static void permissions(void) {
    fixture f;
    init(&f);
    f.status.authorization = SPEECH_AUTH_UNKNOWN;
    f.status.usage_description = false;
    speech_recognizer_host_refresh(f.host);
    CHECK(!cui_speech_available(&f.state));
    CHECK(speech_recognizer_host_prepare(f.host) == CUI_SPEECH_UNSUPPORTED && !f.authorizations);
    f.status.usage_description = true;
    CHECK(speech_recognizer_host_prepare(f.host) == CUI_SPEECH_OK && f.authorizations == 1);
    CHECK(speech_recognizer_host_prepare(f.host) == CUI_SPEECH_INVALID);
    CHECK(!cui_speech_start(&f.state) && f.last_error == CUI_SPEECH_PERMISSION);
    f.status.authorization = SPEECH_AUTH_GRANTED;
    CHECK(speech_recognizer_endpoint_authorized(f.latest, SPEECH_AUTH_GRANTED));
    poll(&f);
    CHECK(speech_recognizer_host_status(f.host).authorization == SPEECH_AUTH_GRANTED &&
          f.closes == 1);
    CHECK(cui_speech_start(&f.state));
    drop(&f);

    init(&f);
    f.status.authorization = SPEECH_AUTH_UNKNOWN;
    f.keep_late = true;
    CHECK(speech_recognizer_host_prepare(f.host) == CUI_SPEECH_OK);
    drop(&f); /* Pending permission callback survives host/state owner lifetime. */
    unsigned wakes = f.wakes;
    CHECK(!speech_recognizer_endpoint_authorized(f.late[0], SPEECH_AUTH_GRANTED));
    CHECK(f.wakes == wakes);
    release_late(&f);

    init(&f);
    f.status.authorization = SPEECH_AUTH_DENIED;
    CHECK(speech_recognizer_host_prepare(f.host) == CUI_SPEECH_PERMISSION && !f.authorizations);
    drop(&f);
    for (unsigned reason = 0; reason < 5; reason++) {
        init(&f);
        if (reason == 0)
            f.status.supported = false;
        if (reason == 1)
            f.status.usage_description = false;
        if (reason == 2)
            f.status.locale_supported = false;
        if (reason == 3)
            f.status.on_device = false;
        if (reason == 4)
            f.status.available = false;
        speech_recognizer_host_refresh(f.host);
        CHECK(!cui_speech_available(&f.state) && !cui_speech_start(&f.state));
        CHECK(f.last_error == CUI_SPEECH_UNSUPPORTED && !f.starts && !f.authorizations);
        drop(&f);
    }
}
static void transcript(void) {
    fixture f;
    init(&f);
    f.synchronous = true;
    CHECK(cui_speech_start(&f.state));
    CHECK(f.events[CUI_SPEECH_PARTIAL] == 0);
    poll(&f);
    CHECK(!strcmp(f.transcript, "sync") && f.state.status == CUI_SPEECH_RECORDING);
    emit(&f, "Hello word", false, true);
    emit(&f, "Hello world, how", false, false);
    CHECK(!strcmp(f.transcript, "Hello world, how"));
    emit(&f, "Hello world.", false, true);
    emit(&f, "Next", false, false);
    CHECK(!strcmp(f.transcript, "Hello world. Next"));
    emit(&f, "Next!", true, false);
    CHECK(!strcmp(f.transcript, "Hello world. Next!") && f.events[CUI_SPEECH_FINAL] == 1);
    CHECK(f.closes == 1 && f.input_closed == 1);
    drop(&f);

    init(&f);
    CHECK(cui_speech_start(&f.state));
    emit(&f, "今天天气", false, true);
    emit(&f, "今天天气很好", false, true);
    emit(&f, "明天", true, false);
    CHECK(!strcmp(f.transcript, "今天天气很好明天"));
    drop(&f);

    init(&f);
    CHECK(cui_speech_start(&f.state));
    CHECK(speech_recognizer_endpoint_result(f.latest, text("not success"), true, false,
                                            CUI_SPEECH_RECOGNIZER_ERROR));
    poll(&f);
    CHECK(f.events[CUI_SPEECH_ERROR] == 1 && !f.events[CUI_SPEECH_FINAL]);
    CHECK(!strcmp(f.transcript, "not success"));
    drop(&f);
    init(&f);
    CHECK(cui_speech_start(&f.state));
    emit(&f, "draft", false, false);
    emit(&f, "", true, false);
    CHECK(!strcmp(f.transcript, "") && f.events[CUI_SPEECH_FINAL] == 1);
    drop(&f);
}
static void lifecycle(void) {
    fixture f;
    init(&f);
    f.keep_late = true;
    CHECK(cui_speech_start(&f.state));
    CHECK(!speech_recognizer_host_destroy(f.host));
    cui_speech_sink stale = {&f.state, f.state.generation};
    CHECK(cui_speech_cancel(&f.state));
    CHECK(cui_speech_start(&f.state));
    CHECK(!speech_recognizer_endpoint_result(f.late[0], text("old"), true, false, CUI_SPEECH_OK));
    CHECK(!cui_speech_report_phrase(stale, text("old")));
    int16_t samples[600];
    for (size_t i = 0; i < 600; i++)
        samples[i] = (int16_t)i;
    cui_speech_sink current = {&f.state, f.state.generation};
    CHECK(cui_speech_report_audio(current, samples, 600, 1));
    poll(&f);
    memset(samples, 0, sizeof samples);
    CHECK(f.pcm_count == 600 && f.pcm[599] == 599);
    CHECK(cui_speech_stop(&f.state, 1));
    CHECK(f.finishes == 1 && f.input_closed == 2);
    f.status.available = false;
    speech_recognizer_host_refresh(f.host);
    poll(&f); /* Availability revocation still closes a finishing task. */
    CHECK(f.closes == 2 && f.last_error == CUI_SPEECH_RECOGNIZER_ERROR);
    drop(&f);
    unsigned wakes = f.wakes;
    for (size_t i = 0; i < f.late_count; i++)
        CHECK(!speech_recognizer_endpoint_error(f.late[i], CUI_SPEECH_RECOGNIZER_ERROR));
    CHECK(wakes == f.wakes);
    release_late(&f);

    init(&f);
    CHECK(cui_speech_start(&f.state));
    CHECK(cui_speech_set_stop_timeout(&f.state, 1));
    CHECK(cui_speech_stop(&f.state, 1));
    CHECK(cui_speech_pump(&f.state, 3));
    CHECK(f.state.status == CUI_SPEECH_IDLE && f.closes == 1);
    drop(&f);

    init(&f);
    f.fail_start = true;
    CHECK(!cui_speech_start(&f.state) && f.closes == 1 && f.input_closed == 0);
    drop(&f);

    init(&f);
    f.status.format.channels = 2;
    speech_recognizer_host_refresh(f.host);
    CHECK(!cui_speech_available(&f.state) && !cui_speech_start(&f.state));
    f.status.format.channels = 1;
    f.state.recognizer.format.sample_rate = 48000;
    CHECK(!cui_speech_start(&f.state) && f.last_error == CUI_SPEECH_INVALID && !f.starts);
    drop(&f);
}
static void bounds(void) {
    fixture f;
    init(&f);
    f.keep_late = true;
    for (unsigned i = 0; i < SPEECH_RECOGNIZER_ENDPOINTS; i++) {
        CHECK(cui_speech_start(&f.state));
        CHECK(cui_speech_cancel(&f.state));
    }
    CHECK(!cui_speech_available(&f.state));
    CHECK(!cui_speech_start(&f.state) && f.last_error == CUI_SPEECH_CAPACITY);
    f.status.authorization = SPEECH_AUTH_UNKNOWN;
    CHECK(speech_recognizer_host_prepare(f.host) == CUI_SPEECH_CAPACITY);
    release_late(&f);
    f.keep_late = false;
    f.status.authorization = SPEECH_AUTH_GRANTED;
    CHECK(cui_speech_start(&f.state));
    drop(&f);

    init(&f);
    CHECK(cui_speech_start(&f.state));
    for (unsigned i = 0; i < SPEECH_RECOGNIZER_EVENTS; i++)
        CHECK(speech_recognizer_endpoint_result(f.latest, text("queued"), false, false,
                                                CUI_SPEECH_OK));
    CHECK(!speech_recognizer_endpoint_result(f.latest, text("overflow"), false, false,
                                             CUI_SPEECH_OK));
    poll(&f);
    CHECK(f.last_error == CUI_SPEECH_CAPACITY && f.closes == 1);
    drop(&f);

    init(&f);
    CHECK(cui_speech_start(&f.state));
    char oversized[SPEECH_RECOGNIZER_TEXT];
    memset(oversized, 'x', sizeof oversized);
    CHECK(speech_recognizer_endpoint_result(f.latest, (cui_str){oversized, sizeof oversized}, false,
                                            false, CUI_SPEECH_OK));
    poll(&f);
    CHECK(f.last_error == CUI_SPEECH_CAPACITY);
    drop(&f);

    init(&f);
    CHECK(cui_speech_start(&f.state));
    CHECK(speech_recognizer_endpoint_result(f.latest, (cui_str){"\xc0\xaf", 2}, false, false,
                                            CUI_SPEECH_OK));
    poll(&f);
    CHECK(f.last_error == CUI_SPEECH_RECOGNIZER_ERROR);
    drop(&f);

    init(&f);
    CHECK(cui_speech_start(&f.state));
    f.wake_failure = true;
    CHECK(!speech_recognizer_endpoint_result(f.latest, text("wake"), false, false, CUI_SPEECH_OK));
    poll(&f);
    CHECK(f.last_error == CUI_SPEECH_RECOGNIZER_ERROR);
    drop(&f);

    init(&f);
    CHECK(cui_speech_start(&f.state));
    f.state.storage.message_capacity = 1;
    CHECK(speech_recognizer_endpoint_result(f.latest, text("final"), true, false, CUI_SPEECH_OK));
    poll(&f);
    CHECK(f.last_error == CUI_SPEECH_CAPACITY && !f.events[CUI_SPEECH_FINAL]);
    drop(&f);
}
typedef struct {
    speech_recognizer_endpoint *endpoint;
    atomic_uint *ack;
    unsigned worker;
} producer_args;
static void *producer(void *opaque) {
    producer_args *args = opaque;
    for (unsigned i = 1; i <= 128; i++) {
        char buffer[32];
        int n = snprintf(buffer, sizeof buffer, "worker %u %u", args->worker, i);
        CHECK(n > 0);
        CHECK(speech_recognizer_endpoint_result(args->endpoint, (cui_str){buffer, (size_t)n}, false,
                                                false, CUI_SPEECH_OK));
        while (atomic_load_explicit(args->ack, memory_order_acquire) != i) {
        }
    }
    speech_recognizer_endpoint_release(args->endpoint);
    return NULL;
}
static void concurrent(void) {
    fixture f;
    init(&f);
    CHECK(cui_speech_start(&f.state));
    atomic_uint ack[2];
    pthread_t threads[2];
    producer_args args[2];
    for (unsigned i = 0; i < 2; i++) {
        atomic_init(&ack[i], 0);
        f.acks[i] = &ack[i];
        CHECK(speech_recognizer_endpoint_retain(f.latest));
        args[i] = (producer_args){f.latest, &ack[i], i};
        CHECK(!pthread_create(&threads[i], NULL, producer, &args[i]));
    }
    while (f.events[CUI_SPEECH_PARTIAL] < 256)
        poll(&f);
    for (unsigned i = 0; i < 2; i++) {
        CHECK(!pthread_join(threads[i], NULL));
        CHECK(atomic_load(&ack[i]) == 128);
    }
    drop(&f);
}
static bool heap_wake(void *user) {
    (*(unsigned *)user)++;
    return true;
}
static void lifetime(void) {
    fixture *heap = malloc(sizeof *heap);
    CHECK(heap);
    init(heap);
    heap->keep_late = true;
    CHECK(cui_speech_start(&heap->state));
    speech_recognizer_endpoint *late = heap->late[0];
    drop(heap);
    free(heap); /* Includes actual CUI state/workspace and service/wake userdata. */
    CHECK(
        !speech_recognizer_endpoint_result(late, text("freed state"), true, false, CUI_SPEECH_OK));
    speech_recognizer_endpoint_release(late);

    fixture f;
    init(&f);
    drop(&f);
    unsigned *wake_userdata = calloc(1, sizeof *wake_userdata);
    CHECK(wake_userdata);
    f.status.authorization = SPEECH_AUTH_UNKNOWN;
    f.keep_late = true;
    f.host = speech_recognizer_host_create(
        (speech_recognition_service){&f, query, authorize, start, append, finish, close_token},
        speech_recognizer_posix_mutex(), heap_wake, wake_userdata);
    CHECK(f.host && speech_recognizer_host_prepare(f.host) == CUI_SPEECH_OK);
    CHECK(speech_recognizer_host_destroy(f.host));
    free(wake_userdata);
    CHECK(!speech_recognizer_endpoint_authorized(f.late[0], SPEECH_AUTH_GRANTED));
    CHECK(!speech_recognizer_endpoint_result(f.late[0], text("after free"), true, false,
                                             CUI_SPEECH_OK));
    release_late(&f);
}
int main(int argc, char **argv) {
    CHECK(argc == 2);
    if (!strcmp(argv[1], "permissions"))
        permissions();
    else if (!strcmp(argv[1], "transcript"))
        transcript();
    else if (!strcmp(argv[1], "lifecycle"))
        lifecycle();
    else if (!strcmp(argv[1], "bounds"))
        bounds();
    else if (!strcmp(argv[1], "concurrent"))
        concurrent();
    else if (!strcmp(argv[1], "lifetime"))
        lifetime();
    else
        CHECK(false);
    printf("PASS: %s (injected only, no OS recognition or microphone)\n", argv[1]);
    return 0;
}
