/* SPDX-License-Identifier: MIT */
#include "output.h"
#include <float.h>
#include <stdlib.h>
_Static_assert(sizeof(float) == 4 && FLT_RADIX == 2 && FLT_MANT_DIG == 24,
               "F32 PCM requires binary32 float");
#include <threads.h>
typedef struct {
    video_audio_ticket ticket;
    size_t ack;
} entry;
struct video_sdl_output {
    video_sdl_backend b;
    thrd_t owner;
    video_audio_host *host;
    entry *ledger;
    float *scratch;
    size_t max, chunk, head, count;
    uint64_t submitted, drained;
    bool running, faulted;
};
static bool owner(video_sdl_output *s) {
    return thrd_equal(s->owner, thrd_current());
}
/* Called only under backend stream lock. The successful-put prefix and exact
 * current input queue determine software mixer drain, not audible playback. */
static bool reconcile(video_sdl_output *s) {
    if (!s->b.format(s->b.user))
        return false;
    int bytes = s->b.queued(s->b.user);
    uint64_t pending = s->submitted - s->drained;
    if (bytes < 0 || bytes % 8 || (uint64_t)bytes / 8 > pending)
        return false;
    uint64_t amount = pending - (uint64_t)bytes / 8;
    while (amount) {
        if (!s->count || !s->host)
            return false;
        entry *e = &s->ledger[s->head];
        size_t n = e->ticket.frames - e->ack;
        if (n > amount)
            n = (size_t)amount;
        if (!video_audio_consume(s->host, e->ticket.generation, e->ticket.first_sequence + e->ack,
                                 n))
            return false;
        e->ack += n;
        amount -= n;
        s->drained += n;
        if (e->ack == e->ticket.frames) {
            s->head = (s->head + 1) % s->max;
            s->count--;
        }
    }
    return true;
}
static bool fail(video_sdl_output *s) {
    s->faulted = true;
    s->running = false;
    /* Never acquire a device lock while holding the stream lock. */
    s->b.pause(s->b.user);
    return false;
}
static bool start(void *u, video_audio_host *h) {
    video_sdl_output *s = u;
    if (!owner(s) || s->faulted || (s->host && s->host != h))
        return false;
    if (!s->b.lock(s->b.user))
        return false;
    bool valid = reconcile(s);
    s->b.unlock(s->b.user);
    if (!valid)
        return fail(s);
    if (!s->b.resume(s->b.user))
        return false;
    s->host = h;
    s->running = true;
    return true;
}
static bool stop(void *u, bool discard) {
    video_sdl_output *s = u;
    if (!owner(s) || !s->b.pause(s->b.user))
        return false;
    s->running = false;
    if (!s->b.lock(s->b.user))
        return false;
    bool ok = true;
    /* Failure means queue/put state may be ambiguous. Never infer consumption
     * from clearing it. Normal stop reconciles the frozen prefix first. */
    if (!s->faulted)
        ok = reconcile(s);
    if (!ok)
        s->faulted = true;
    if (discard) {
        ok = s->b.clear(s->b.user);
        if (ok) {
            s->submitted = s->drained = 0;
            s->head = s->count = 0;
            s->host = NULL;
            s->faulted = false;
        }
    }
    s->b.unlock(s->b.user);
    return ok;
}
video_sdl_output *video_sdl_create(video_sdl_backend b, size_t max, size_t chunk) {
    if (!max || max > 65536 || !chunk || chunk > max || !b.lock || !b.unlock || !b.format ||
        !b.queued || !b.put || !b.pause || !b.resume || !b.clear || !b.close)
        return NULL;
    video_sdl_output *s = calloc(1, sizeof(*s));
    if (!s)
        return NULL;
    s->ledger = calloc(max, sizeof(entry));
    s->scratch = calloc(chunk * 2, sizeof(float));
    if (!s->ledger || !s->scratch) {
        free(s->ledger);
        free(s->scratch);
        free(s);
        return NULL;
    }
    s->b = b;
    s->max = max;
    s->chunk = chunk;
    s->owner = thrd_current();
    if (!b.pause(b.user))
        goto bad;
    if (!b.lock(b.user))
        goto bad;
    bool valid = b.format(b.user) && b.queued(b.user) == 0;
    b.unlock(b.user);
    if (!valid)
        goto bad;
    return s;
bad:
    free(s->ledger);
    free(s->scratch);
    free(s);
    return NULL;
}
video_audio_output video_sdl_interface(video_sdl_output *s) {
    return (video_audio_output){start, stop, s};
}
bool video_sdl_pump(video_sdl_output *s) {
    if (!owner(s) || s->faulted)
        return false;
    if (!s->running)
        return true;
    if (!s->b.lock(s->b.user))
        return fail(s);
    bool ok = reconcile(s);
    size_t pending = (size_t)(s->submitted - s->drained);
    if (ok && pending < s->max && !video_audio_poll(s->host).ended) {
        size_t n = s->max - pending;
        if (n > s->chunk)
            n = s->chunk;
        video_audio_ticket ticket;
        if (s->submitted > UINT64_MAX - n || s->count == s->max ||
            !video_audio_render(s->host, s->scratch, n, &ticket))
            ok = false;
        else if (!s->b.put(s->b.user, s->scratch, n * 8))
            ok = false;
        else {
            s->ledger[(s->head + s->count) % s->max] = (entry){ticket, 0};
            s->count++;
            s->submitted += n;
        }
    }
    s->b.unlock(s->b.user);
    return ok ? true : fail(s);
}
video_sdl_snapshot video_sdl_poll(video_sdl_output *s) {
    return (video_sdl_snapshot){s->submitted,   s->drained, (size_t)(s->submitted - s->drained),
                                s->count,       s->running, s->faulted,
                                s->host != NULL};
}
bool video_sdl_destroy(video_sdl_output *s) {
    if (!s || !owner(s) || s->host)
        return false;
    s->b.close(s->b.user);
    free(s->ledger);
    free(s->scratch);
    free(s);
    return true;
}
