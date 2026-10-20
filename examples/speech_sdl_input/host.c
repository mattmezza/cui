/* SPDX-License-Identifier: MIT */
#include "host.h"
#include <limits.h>
#include <math.h>
#include <stdatomic.h>
#include <stdlib.h>

struct speech_input_host {
    speech_input_device_ops ops;
    speech_input_wake wake;
    void *wake_user, *device;
    cui_speech_sink sink;
    cui_speech_format format;
    int16_t *samples;
    unsigned slots;
    atomic_uint read, write;
    atomic_int error;
    atomic_bool accepting;
    bool active, error_reported;
};

static void terminal(speech_input_host *h, cui_speech_error error) {
    int expected = CUI_SPEECH_OK;
    (void)atomic_compare_exchange_strong(&h->error, &expected, error);
}

static bool deliver(void *user, const int16_t *samples, size_t count, cui_speech_error error) {
    speech_input_host *h = user;
    if (!atomic_load_explicit(&h->accepting, memory_order_acquire))
        return false;
    if (error != CUI_SPEECH_OK)
        terminal(h, error == CUI_SPEECH_CAPACITY ? error : CUI_SPEECH_INPUT_ERROR);
    if (atomic_load(&h->error) == CUI_SPEECH_OK) {
        unsigned w = atomic_load_explicit(&h->write, memory_order_relaxed);
        unsigned r = atomic_load_explicit(&h->read, memory_order_acquire);
        unsigned used = w >= r ? w - r : h->slots - r + w;
        if ((count && !samples) || count % h->format.channels)
            terminal(h, CUI_SPEECH_INPUT_ERROR);
        else if (count > h->slots - 1 - used)
            terminal(h, CUI_SPEECH_CAPACITY);
        else {
            for (size_t i = 0; i < count; i++) {
                h->samples[w] = samples[i];
                w = w + 1 == h->slots ? 0 : w + 1;
            }
            atomic_store_explicit(&h->write, w, memory_order_release);
        }
    }
    if (h->wake && !h->wake(h->wake_user))
        terminal(h, CUI_SPEECH_INPUT_ERROR);
    return atomic_load(&h->error) == CUI_SPEECH_OK;
}

static cui_speech_error start(void *user, cui_speech_format format, cui_speech_sink sink,
                              void **token) {
    speech_input_host *h = user;
    *token = NULL;
    if (h->active || !format.channels || format.channels > 256 || !format.sample_rate ||
        format.channels > h->slots - 1)
        return CUI_SPEECH_INVALID;
    h->format = format;
    h->sink = sink;
    h->error_reported = false;
    atomic_store(&h->read, 0);
    atomic_store(&h->write, 0);
    atomic_store(&h->error, CUI_SPEECH_OK);
    atomic_store_explicit(&h->accepting, true, memory_order_release);
    void *device = NULL;
    cui_speech_error error = h->ops.open(h->ops.user, format, deliver, h, &device);
    if (error != CUI_SPEECH_OK || !device) {
        atomic_store(&h->accepting, false);
        /* Defensive cleanup for an injected provider returning a partial token. */
        if (device)
            h->ops.close(h->ops.user, device);
        h->sink = (cui_speech_sink){0};
        return error == CUI_SPEECH_OK ? CUI_SPEECH_INPUT_ERROR : error;
    }
    h->device = device;
    h->active = true;
    *token = h;
    return CUI_SPEECH_OK;
}

static void close_capture(void *user, void *token) {
    speech_input_host *h = user;
    if (token != h || !h->active)
        return;
    atomic_store_explicit(&h->accepting, false, memory_order_release);
    void *device = h->device;
    h->device = NULL;
    h->active = false;
    h->ops.close(h->ops.user, device); /* Must quiesce producer before reset/free. */
    h->sink = (cui_speech_sink){0};
}

speech_input_host *speech_input_host_create(size_t capacity, speech_input_device_ops ops,
                                            speech_input_wake wake, void *wake_user) {
    if (!capacity || capacity >= UINT_MAX || capacity >= SIZE_MAX / sizeof(int16_t) || !ops.open ||
        !ops.close)
        return NULL;
    speech_input_host *h = calloc(1, sizeof *h);
    if (!h)
        return NULL;
    h->samples = malloc(capacity * sizeof *h->samples + sizeof *h->samples);
    if (!h->samples) {
        free(h);
        return NULL;
    }
    h->slots = (unsigned)capacity + 1;
    h->ops = ops;
    h->wake = wake;
    h->wake_user = wake_user;
    atomic_init(&h->read, 0);
    atomic_init(&h->write, 0);
    atomic_init(&h->error, CUI_SPEECH_OK);
    atomic_init(&h->accepting, false);
    if (!atomic_is_lock_free(&h->read) || !atomic_is_lock_free(&h->write) ||
        !atomic_is_lock_free(&h->error) || !atomic_is_lock_free(&h->accepting)) {
        free(h->samples);
        free(h);
        return NULL;
    }
    return h;
}

bool speech_input_host_destroy(speech_input_host *h) {
    if (!h)
        return true;
    if (h->active)
        return false;
    free(h->samples);
    free(h);
    return true;
}

cui_speech_input speech_input_host_provider(speech_input_host *h) {
    return h ? (cui_speech_input){h, start, close_capture} : (cui_speech_input){0};
}

bool speech_input_host_poll(speech_input_host *h, double now, size_t max_samples) {
    if (!h || !h->active || !isfinite(now) || h->error_reported)
        return false;
    int error = atomic_load(&h->error);
    if (error != CUI_SPEECH_OK) {
        h->error_reported = true;
        return cui_speech_report_error(h->sink, (cui_speech_error)error);
    }
    bool reported = false;
    unsigned r = atomic_load_explicit(&h->read, memory_order_relaxed);
    unsigned w = atomic_load_explicit(&h->write, memory_order_acquire);
    unsigned available = w >= r ? w - r : h->slots - r + w;
    size_t remaining = available < max_samples ? available : max_samples;
    remaining -= remaining % h->format.channels;
    while (remaining) {
        int16_t scratch[256];
        size_t count = remaining < 256 ? remaining : 256;
        count -= count % h->format.channels;
        for (size_t i = 0; i < count; i++) {
            scratch[i] = h->samples[r];
            r = r + 1 == h->slots ? 0 : r + 1;
        }
        atomic_store_explicit(&h->read, r, memory_order_release);
        if (!cui_speech_report_audio(h->sink, scratch, count, now))
            return reported;
        reported = true;
        remaining -= count;
    }
    return reported;
}
