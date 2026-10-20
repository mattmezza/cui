/* SPDX-License-Identifier: MIT */
#include "host.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
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
    thrd_t thread, owner;
    atomic_uint acknowledged;
    atomic_bool done;
    size_t received;
    unsigned closed;
} fixture;
static int producer(void *user) {
    fixture *f = user;
    for (unsigned batch = 0; batch < 256; batch++) {
        int16_t samples[16];
        for (unsigned i = 0; i < 16; i++)
            samples[i] = (int16_t)(batch * 16 + i);
        CHECK(f->deliver(f->delivery_user, samples, 16, CUI_SPEECH_OK));
        while (atomic_load_explicit(&f->acknowledged, memory_order_acquire) != batch + 1)
            thrd_yield();
    }
    atomic_store_explicit(&f->done, true, memory_order_release);
    return 0;
}
static cui_speech_error open_input(void *user, cui_speech_format format,
                                   speech_input_deliver deliver, void *delivery_user,
                                   void **token) {
    fixture *f = user;
    CHECK(format.channels == 1);
    f->deliver = deliver;
    f->delivery_user = delivery_user;
    CHECK(thrd_create(&f->thread, producer, f) == thrd_success);
    *token = f;
    return CUI_SPEECH_OK;
}
static void close_input(void *user, void *token) {
    fixture *f = user;
    CHECK(token == f);
    int result;
    CHECK(thrd_join(f->thread, &result) == thrd_success && result == 0);
    int16_t sample = 1;
    CHECK(!f->deliver(f->delivery_user, &sample, 1, CUI_SPEECH_OK));
    f->closed++;
}
static cui_speech_error recognize(void *user, cui_speech_sink sink, void **token) {
    *token = user;
    CHECK(cui_speech_report_ready(sink));
    return CUI_SPEECH_OK;
}
static void push(void *user, void *token, const int16_t *samples, size_t count) {
    fixture *f = user;
    CHECK(token == f && thrd_equal(thrd_current(), f->owner));
    for (size_t i = 0; i < count; i++)
        CHECK(samples[i] == (int16_t)f->received++);
    if (f->received % 16 == 0)
        atomic_store_explicit(&f->acknowledged, (unsigned)f->received / 16, memory_order_release);
}
static void finish(void *user, void *token) {
    CHECK(user == token);
}
static void close_recognizer(void *user, void *token) {
    CHECK(user == token);
}
int main(void) {
    fixture f = {0};
    f.owner = thrd_current();
    atomic_init(&f.acknowledged, 0);
    atomic_init(&f.done, false);
    speech_input_host *host = speech_input_host_create(
        32, (speech_input_device_ops){&f, open_input, close_input}, NULL, NULL);
    CHECK(host);
    cui_speech_state state = {0};
    char transcript[64];
    cui_speech_message messages[8];
    unsigned char queue[4096];
    CHECK(cui_speech_init(
        &state,
        (cui_speech_workspace){transcript, sizeof transcript, messages, 8, queue, sizeof queue},
        (cui_speech_recognizer){.user = &f,
                                .start = recognize,
                                .push_audio = push,
                                .finish = finish,
                                .close = close_recognizer},
        speech_input_host_provider(host), NULL, NULL));
    CHECK(cui_speech_start(&state));
    while (!atomic_load_explicit(&f.done, memory_order_acquire)) {
        (void)speech_input_host_poll(host, 1, 8);
        (void)cui_speech_pump(&state, 1);
        thrd_yield();
    }
    CHECK(f.received == 4096 && state.status == CUI_SPEECH_RECORDING);
    CHECK(cui_speech_cancel(&state));
    CHECK(f.closed == 1);
    cui_speech_dispose(&state);
    CHECK(speech_input_host_destroy(host));
    puts("PASS: 4096 ordered concurrent samples, bounded ring wrap, joined close and late "
         "rejection");
    return 0;
}
