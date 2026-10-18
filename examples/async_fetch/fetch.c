/* SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L
#include "fetch.h"
#include <errno.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
struct af_worker {
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t changed;
    atomic_bool stopping;
    atomic_uint_fast64_t latest;
    atomic_uint wake_failures;
    bool pending;
    char url[AF_URL_MAX + 1];
    uint64_t generation;
    af_result completions[AF_COMPLETIONS];
    size_t head, count;
    af_transport_fn transport;
    af_wake_fn wake;
    void *transport_user, *wake_user;
};
typedef struct {
    af_worker *worker;
    uint64_t generation;
} cancel_context;
static bool cancelled(void *user) {
    cancel_context *c = user;
    return atomic_load(&c->worker->stopping) || atomic_load(&c->worker->latest) != c->generation;
}
static bool valid_url(const char *url, size_t *length) {
    if (!url)
        return false;
    size_t n = 0;
    while (n <= AF_URL_MAX && url[n]) {
        unsigned char c = (unsigned char)url[n++];
        if (c <= 32 || c >= 127)
            return false;
    }
    if (n > AF_URL_MAX || n <= 8 || memcmp(url, "https://", 8))
        return false;
    *length = n;
    return true;
}
/* Retry transient notification failure without polling the UI or allocating a
 * timer/thread. Stop interrupts these condition-variable delays. Persistent
 * rejection still requires an independent UI event; no wake API can bypass it. */
static void notify(af_worker *w) {
    for (unsigned attempt = 0; attempt < 3; ++attempt) {
        if (atomic_load(&w->stopping))
            return;
        if (w->wake(w->wake_user))
            return;
        atomic_fetch_add(&w->wake_failures, 1);
        if (attempt == 2)
            return;
        struct timespec until;
        if (clock_gettime(CLOCK_REALTIME, &until))
            return;
        until.tv_nsec += (long)(attempt + 1) * 10000000;
        if (until.tv_nsec >= 1000000000) {
            ++until.tv_sec;
            until.tv_nsec -= 1000000000;
        }
        pthread_mutex_lock(&w->mutex);
        int rc = 0;
        while (!atomic_load(&w->stopping) && rc == 0)
            rc = pthread_cond_timedwait(&w->changed, &w->mutex, &until);
        pthread_mutex_unlock(&w->mutex);
        if (rc && rc != ETIMEDOUT)
            return;
    }
}
static void *work(void *user) {
    af_worker *w = user;
    for (;;) {
        char url[AF_URL_MAX + 1];
        uint64_t generation;
        pthread_mutex_lock(&w->mutex);
        while (!w->pending && !atomic_load(&w->stopping))
            pthread_cond_wait(&w->changed, &w->mutex);
        if (atomic_load(&w->stopping)) {
            pthread_mutex_unlock(&w->mutex);
            break;
        }
        memcpy(url, w->url, sizeof url);
        generation = w->generation;
        w->pending = false;
        pthread_mutex_unlock(&w->mutex);
        af_result result = {.generation = generation, .status = AF_CANCELLED};
        cancel_context context = {w, generation};
        if (!cancelled(&context))
            w->transport(w->transport_user, url, cancelled, &context, &result);
        result.generation = generation; /* Transport cannot change request identity. */
        if (result.size > AF_BODY_MAX) {
            result.size = 0;
            result.status = AF_TOO_LARGE;
        }
        result.body[result.size] = 0;
        result.error[AF_ERROR_MAX - 1] = 0;
        pthread_mutex_lock(&w->mutex);
        while (w->count == AF_COMPLETIONS && !atomic_load(&w->stopping))
            pthread_cond_wait(&w->changed, &w->mutex);
        if (atomic_load(&w->stopping)) {
            pthread_mutex_unlock(&w->mutex);
            break;
        }
        w->completions[(w->head + w->count) % AF_COMPLETIONS] = result;
        ++w->count;
        pthread_mutex_unlock(&w->mutex);
        /* Notification only: never call CUI or mutate the UI model here. */
        notify(w);
    }
    return NULL;
}
af_worker *af_worker_create(af_transport_fn transport, void *user, af_wake_fn wake,
                            void *wake_user) {
    if (!transport || !wake)
        return NULL;
    af_worker *w = calloc(1, sizeof *w);
    if (!w)
        return NULL;
    atomic_init(&w->stopping, false);
    atomic_init(&w->latest, 0);
    atomic_init(&w->wake_failures, 0);
    w->transport = transport;
    w->transport_user = user;
    w->wake = wake;
    w->wake_user = wake_user;
    if (pthread_mutex_init(&w->mutex, NULL)) {
        free(w);
        return NULL;
    }
    if (pthread_cond_init(&w->changed, NULL)) {
        pthread_mutex_destroy(&w->mutex);
        free(w);
        return NULL;
    }
    if (pthread_create(&w->thread, NULL, work, w)) {
        pthread_cond_destroy(&w->changed);
        pthread_mutex_destroy(&w->mutex);
        free(w);
        return NULL;
    }
    return w;
}
bool af_worker_submit(af_worker *w, const char *url, uint64_t generation) {
    size_t n;
    if (!w || !generation || !valid_url(url, &n))
        return false;
    pthread_mutex_lock(&w->mutex);
    bool accepted = !w->pending && !atomic_load(&w->stopping);
    if (accepted) {
        memcpy(w->url, url, n + 1);
        w->generation = generation;
        w->pending = true;
        atomic_store(&w->latest, generation);
        pthread_cond_broadcast(&w->changed);
    }
    pthread_mutex_unlock(&w->mutex);
    return accepted;
}
void af_worker_cancel(af_worker *w, uint64_t generation) {
    if (w)
        atomic_store(&w->latest, generation);
}
bool af_worker_take(af_worker *w, af_result *out) {
    if (!w || !out)
        return false;
    pthread_mutex_lock(&w->mutex);
    bool available = w->count != 0;
    if (available) {
        *out = w->completions[w->head];
        w->head = (w->head + 1) % AF_COMPLETIONS;
        --w->count;
        pthread_cond_broadcast(&w->changed);
    }
    pthread_mutex_unlock(&w->mutex);
    return available;
}
unsigned af_worker_wake_failures(af_worker *w) {
    return w ? atomic_load(&w->wake_failures) : 0;
}
void af_worker_destroy(af_worker *w) {
    if (!w)
        return;
    pthread_mutex_lock(&w->mutex);
    atomic_store(&w->stopping, true);
    pthread_cond_broadcast(&w->changed);
    pthread_mutex_unlock(&w->mutex);
    pthread_join(w->thread, NULL);
    pthread_cond_destroy(&w->changed);
    pthread_mutex_destroy(&w->mutex);
    free(w);
}
void af_model_init(af_model *m) {
    *m = (af_model){.owner = pthread_self()};
}
static bool owner(const af_model *m) {
    return m && pthread_equal(m->owner, pthread_self());
}
af_start_result af_model_start(af_model *m, af_worker *w, const char *url) {
    size_t n;
    if (!owner(m))
        return AF_WRONG_THREAD;
    if (m->loading)
        return AF_DUPLICATE;
    if (m->generation >= UINT64_MAX - 1 || !valid_url(url, &n))
        return AF_INVALID;
    uint64_t generation = m->generation + 1;
    if (!af_worker_submit(w, url, generation))
        return AF_BUSY;
    memcpy(m->url, url, n + 1);
    m->generation = generation;
    m->loading = true;
    m->result = (af_result){.generation = generation};
    return AF_STARTED;
}
bool af_model_cancel(af_model *m, af_worker *w) {
    if (!owner(m) || !m->loading)
        return false;
    /* Reserve UINT64_MAX as a terminal cancelled generation. */
    if (m->generation != UINT64_MAX)
        ++m->generation;
    af_worker_cancel(w, m->generation);
    m->loading = false;
    m->result = (af_result){.generation = m->generation, .status = AF_CANCELLED};
    return true;
}
bool af_model_apply(af_model *m, const af_result *r) {
    if (!owner(m) || !r || r->size > AF_BODY_MAX)
        return false;
    if (!m->loading || r->generation != m->generation) {
        ++m->stale;
        return false;
    }
    m->result = *r;
    m->result.body[m->result.size] = 0;
    m->result.error[AF_ERROR_MAX - 1] = 0;
    m->loading = false;
    ++m->applied;
    return true;
}
size_t af_model_drain(af_model *m, af_worker *w) {
    if (!owner(m))
        return 0;
    af_result result;
    size_t count = 0;
    while (af_worker_take(w, &result)) {
        (void)af_model_apply(m, &result);
        ++count;
    }
    return count;
}
