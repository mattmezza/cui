/* SPDX-License-Identifier: MIT */
#include "host.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                                \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

typedef struct {
    speech_input_deliver deliver;
    void *delivery_user;
    cui_speech_sink recognizer_sink;
    unsigned opened, closed, recognized_closed, finished, wakes, events[5];
    cui_speech_error open_error, last_error;
    bool partial_token, synchronous, wake_fails, close_delivery_rejected;
    int16_t received[1024];
    size_t count;
} fixture;

typedef struct {
    fixture f;
    speech_input_host *host;
    cui_speech_state state;
    char transcript[128];
    cui_speech_message messages[16];
    unsigned char queue[4096];
} scenario;

static cui_speech_error open_input(void *user, cui_speech_format format,
                                   speech_input_deliver deliver, void *delivery_user,
                                   void **token) {
    fixture *f = user;
    CHECK(format.sample_rate == 16000 && (format.channels == 1 || format.channels == 2));
    f->opened++;
    f->deliver = deliver;
    f->delivery_user = delivery_user;
    *token = f->open_error == CUI_SPEECH_OK || f->partial_token ? f : NULL;
    if (f->synchronous) {
        const int16_t samples[] = {1, 2, 3};
        CHECK(deliver(delivery_user, samples, 3, CUI_SPEECH_OK));
    }
    return f->open_error;
}
static void close_input(void *user, void *token) {
    fixture *f = user;
    CHECK(token == f);
    f->closed++;
    const int16_t sample = 99;
    f->close_delivery_rejected = !f->deliver(f->delivery_user, &sample, 1, CUI_SPEECH_OK);
}
static bool wake(void *user) {
    fixture *f = user;
    f->wakes++;
    return !f->wake_fails;
}
static cui_speech_error recognize(void *user, cui_speech_sink sink, void **token) {
    fixture *f = user;
    f->recognizer_sink = sink;
    *token = f;
    CHECK(cui_speech_report_ready(sink));
    return CUI_SPEECH_OK;
}
static void push(void *user, void *token, const int16_t *samples, size_t count) {
    fixture *f = user;
    CHECK(token == f && f->count + count <= 1024);
    memcpy(f->received + f->count, samples, count * sizeof *samples);
    f->count += count;
}
static void finish(void *user, void *token) {
    fixture *f = user;
    CHECK(token == f);
    f->finished++;
}
static void close_recognizer(void *user, void *token) {
    fixture *f = user;
    CHECK(token == f);
    f->recognized_closed++;
}
static void event(void *user, cui_speech_event event) {
    fixture *f = user;
    f->events[event.type]++;
    f->last_error = event.error;
}
static void init(scenario *s, size_t capacity) {
    memset(s, 0, sizeof *s);
    fixture *f = &s->f;
    s->host = speech_input_host_create(
        capacity, (speech_input_device_ops){f, open_input, close_input}, wake, f);
    CHECK(s->host);
    CHECK(cui_speech_init(&s->state,
                          (cui_speech_workspace){s->transcript, sizeof s->transcript, s->messages,
                                                 16, s->queue, sizeof s->queue},
                          (cui_speech_recognizer){.user = f,
                                                  .start = recognize,
                                                  .push_audio = push,
                                                  .finish = finish,
                                                  .close = close_recognizer},
                          speech_input_host_provider(s->host), event, f));
}
static void dispose(scenario *s) {
    cui_speech_dispose(&s->state);
    CHECK(speech_input_host_destroy(s->host));
}
static void poll(scenario *s) {
    (void)speech_input_host_poll(s->host, 1, 1024);
    (void)cui_speech_pump(&s->state, 1);
}
static void send(scenario *s, const int16_t *samples, size_t count, bool expected) {
    CHECK(s->f.deliver(s->f.delivery_user, samples, count, CUI_SPEECH_OK) == expected);
}

int main(void) {
    scenario s;
    init(&s, 8);
    s.f.synchronous = true;
    CHECK(cui_speech_start(&s.state));
    CHECK(!speech_input_host_destroy(s.host));
    CHECK(!speech_input_host_poll(s.host, 0.0 / 0.0, 8));
    poll(&s);
    CHECK(s.f.count == 3 && s.f.received[2] == 3);
    const int16_t values[] = {4, 5, 6, 7, 8, 9, 10, 11};
    send(&s, values, 8, true);
    CHECK(speech_input_host_poll(s.host, 1, 3));
    CHECK(cui_speech_pump(&s.state, 1));
    poll(&s);
    CHECK(s.f.count == 11 && s.f.received[3] == 4 && s.f.received[10] == 11);
    cui_speech_sink stale = s.f.recognizer_sink;
    CHECK(cui_speech_cancel(&s.state));
    CHECK(s.f.closed == 1 && s.f.close_delivery_rejected);
    CHECK(!cui_speech_report_phrase(stale, ((cui_str){"stale", 5})));
    CHECK(cui_speech_start(&s.state));
    CHECK(!cui_speech_report_phrase(stale, ((cui_str){"stale", 5})));
    CHECK(cui_speech_stop(&s.state, 2));
    CHECK(s.f.closed == 2 && s.f.finished == 1);
    CHECK(!speech_input_host_poll(s.host, 2, 8));
    CHECK(cui_speech_report_finish(s.f.recognizer_sink));
    CHECK(cui_speech_pump(&s.state, 2));
    CHECK(s.f.events[CUI_SPEECH_FINAL] == 1);
    dispose(&s);
    CHECK(s.f.closed == 2 && s.f.recognized_closed == 2);

    init(&s, 8);
    s.f.open_error = CUI_SPEECH_NO_INPUT;
    CHECK(!cui_speech_start(&s.state));
    CHECK(s.f.closed == 0 && s.f.recognized_closed == 1);
    s.f.partial_token = true;
    CHECK(!cui_speech_start(&s.state));
    CHECK(s.f.closed == 1 && s.f.close_delivery_rejected);
    dispose(&s);

    init(&s, 8);
    CHECK(cui_speech_start(&s.state));
    send(&s, values, 8, true);
    send(&s, values, 1, false);
    poll(&s);
    CHECK(s.f.last_error == CUI_SPEECH_CAPACITY && s.f.closed == 1 && s.f.count == 0);
    dispose(&s);

    init(&s, 8);
    CHECK(cui_speech_start(&s.state));
    s.f.wake_fails = true;
    send(&s, values, 1, false);
    poll(&s);
    CHECK(s.f.last_error == CUI_SPEECH_INPUT_ERROR && s.f.closed == 1);
    dispose(&s);

    init(&s, 8);
    CHECK(cui_speech_start(&s.state));
    CHECK(!s.f.deliver(s.f.delivery_user, NULL, 0, CUI_SPEECH_INPUT_ERROR));
    poll(&s);
    CHECK(s.f.last_error == CUI_SPEECH_INPUT_ERROR);
    dispose(&s);

    init(&s, 8);
    CHECK(cui_speech_start(&s.state));
    CHECK(cui_speech_set_stop_timeout(&s.state, 1));
    CHECK(cui_speech_stop(&s.state, 2));
    CHECK(cui_speech_pump(&s.state, 4));
    CHECK(s.state.status == CUI_SPEECH_IDLE && s.f.closed == 1);
    dispose(&s);

    init(&s, 8);
    s.state.storage.queue_capacity = 1;
    CHECK(cui_speech_start(&s.state));
    send(&s, values, 1, true);
    poll(&s);
    CHECK(s.f.last_error == CUI_SPEECH_CAPACITY && s.f.closed == 1);
    dispose(&s);
    init(&s, 5);
    s.state.recognizer.format.channels = 2;
    CHECK(cui_speech_start(&s.state));
    send(&s, values, 4, true);
    CHECK(speech_input_host_poll(s.host, 1, 3));
    CHECK(cui_speech_pump(&s.state, 1));
    CHECK(s.f.count == 2);
    poll(&s);
    CHECK(s.f.count == 4 && s.f.received[3] == 7);
    send(&s, values, 3, false);
    poll(&s);
    CHECK(s.f.last_error == CUI_SPEECH_INPUT_ERROR);
    dispose(&s);
    puts(
        "PASS: offline input lifecycle, PCM order, overflow, wake/error, cancellation and restart");
    return 0;
}
