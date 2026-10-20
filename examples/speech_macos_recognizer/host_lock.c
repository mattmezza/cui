/* SPDX-License-Identifier: MIT */
#include "host.h"
#include <pthread.h>
#include <stdlib.h>
static void *create(void) {
    pthread_mutex_t *mutex = malloc(sizeof *mutex);
    if (mutex && pthread_mutex_init(mutex, NULL)) {
        free(mutex);
        mutex = NULL;
    }
    return mutex;
}
static void destroy(void *mutex) {
    (void)pthread_mutex_destroy(mutex);
    free(mutex);
}
static void lock(void *mutex) {
    if (pthread_mutex_lock(mutex))
        abort();
}
static void unlock(void *mutex) {
    if (pthread_mutex_unlock(mutex))
        abort();
}
speech_mutex_ops speech_recognizer_posix_mutex(void) {
    return (speech_mutex_ops){create, destroy, lock, unlock};
}
