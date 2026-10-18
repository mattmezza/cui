/* SPDX-License-Identifier: MIT */
#ifndef EXAMPLE_ASYNC_FETCH_H
#define EXAMPLE_ASYNC_FETCH_H
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Example-private API, not part of CUI. All storage is bounded. */
enum { AF_URL_MAX = 2048, AF_BODY_MAX = 8192, AF_ERROR_MAX = 160, AF_COMPLETIONS = 2 };
typedef enum { AF_SUCCESS, AF_FAILURE, AF_CANCELLED, AF_TOO_LARGE } af_status;
typedef struct {
    uint64_t generation;
    af_status status;
    long http_status;
    size_t size;
    unsigned char body[AF_BODY_MAX + 1];
    char error[AF_ERROR_MAX];
} af_result;
typedef bool (*af_cancel_fn)(void *);
/* Runs only on the worker. Do not retain URL, result, cancel or its userdata.
 * Transport/user and wake/user remain alive until worker_destroy has joined.
 * A transport must return in bounded time or cooperate with cancellation. */
typedef void (*af_transport_fn)(void *, const char *, af_cancel_fn, void *, af_result *);
/* Notification must return promptly. Worker retries failure up to three total
 * attempts (10/20 ms interruptible waits); persistent failure retains output
 * until an independent host event drains it. No unconditional wake guarantee. */
typedef bool (*af_wake_fn)(void *);
typedef struct af_worker af_worker;
af_worker *af_worker_create(af_transport_fn, void *, af_wake_fn, void *);
bool af_worker_submit(af_worker *, const char *, uint64_t generation);
void af_worker_cancel(af_worker *, uint64_t replacement_generation);
bool af_worker_take(af_worker *, af_result *);
unsigned af_worker_wake_failures(af_worker *);
void af_worker_destroy(af_worker *); /* Signals stop, joins, then frees. NULL safe. */

typedef struct {
    pthread_t owner;
    uint64_t generation;
    bool loading;
    char url[AF_URL_MAX + 1];
    af_result result;
    unsigned applied, stale;
} af_model;
typedef enum { AF_STARTED, AF_DUPLICATE, AF_BUSY, AF_INVALID, AF_WRONG_THREAD } af_start_result;
void af_model_init(af_model *);
af_start_result af_model_start(af_model *, af_worker *, const char *);
bool af_model_cancel(af_model *, af_worker *);
/* UI-thread only, including apply. Caller may copy a result before applying it;
 * stale/out-of-order generations never overwrite the current state. */
bool af_model_apply(af_model *, const af_result *);
size_t af_model_drain(af_model *, af_worker *);
#endif
