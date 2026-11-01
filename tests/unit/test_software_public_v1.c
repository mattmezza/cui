/* SPDX-License-Identifier: MIT */
/* Includes a mechanically generated single-bool candidate until production is
 * authorized, then the canonical backend; never regenerate the original source. */
#include "../../src/core/cui_alignment_internal.h"
#include "software_public_v1_alloc_internal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef CUI_SOFTWARE_PUBLIC_CANDIDATE
#error Supply the measured source candidate or canonical backend path.
#endif
#define malloc sw_test_malloc
#define calloc sw_test_calloc
#define realloc sw_test_realloc
#define free sw_test_free
#include CUI_SOFTWARE_PUBLIC_CANDIDATE
#undef malloc
#undef calloc
#undef realloc
#undef free
#include "../fixtures/background_v1/frozen_abi.h"
typedef struct {
    char operation;
    size_t bytes;
    unsigned token;
} event;
typedef struct {
    event events[256];
    size_t count, calls, fail, live;
    unsigned next;
} trace;
static trace active;
typedef union {
    cui_fundamental_alignment_internal alignment;
    struct {
        size_t bytes;
        unsigned token;
    } value;
} allocation;
static void event_add(char kind, size_t bytes, unsigned token) {
    assert(active.count < sizeof active.events / sizeof *active.events);
    active.events[active.count++] = (event){kind, bytes, token};
}
static void *get(size_t n, bool clear, char operation) {
    bool fail = ++active.calls == active.fail;
    unsigned token = fail ? 0 : ++active.next;
    event_add(operation, n, token);
    if (fail)
        return NULL;
    allocation *p = malloc(sizeof(*p) + n);
    assert(p);
    p->value.bytes = n;
    p->value.token = token;
    ++active.live;
    if (clear)
        memset(p + 1, 0, n);
    return p + 1;
}
void *sw_test_malloc(size_t n) {
    return get(n, false, 'm');
}
void *sw_test_calloc(size_t n, size_t size) {
    assert(!size || n <= SIZE_MAX / size);
    return get(n * size, true, 'c');
}
void *sw_test_realloc(void *p, size_t n) {
    if (!p)
        return get(n, false, 'r');
    allocation *old = (allocation *)p - 1;
    unsigned token = old->value.token;
    if (++active.calls == active.fail) {
        event_add('r', n, 0);
        return NULL;
    }
    event_add('r', n, token);
    allocation *next = realloc(old, sizeof(*next) + n);
    assert(next);
    next->value.bytes = n;
    return next + 1;
}
void sw_test_free(void *p) {
    if (!p) {
        event_add('f', 0, 0);
        return;
    }
    allocation *a = (allocation *)p - 1;
    event_add('f', a->value.bytes, a->value.token);
    assert(active.live);
    --active.live;
    free(a);
}
typedef struct {
    trace allocation_trace;
    unsigned char first[8 * 6 * 4], resized[9 * 7 * 4];
    unsigned caps;
} result;
static const unsigned char *pixels(bool original, const void *r, unsigned *w, unsigned *h) {
    return original ? sw_test_v1_pixels(r, w, h) : cui_software_pixels(r, w, h);
}
static result exercise(bool original) {
    active = (trace){0};
    void *r = original ? sw_test_v1_create(8, 6) : cui_software_create(8, 6, NULL);
    assert(r);
    unsigned w = 0, h = 0;
    const unsigned char *initial = pixels(original, r, &w, &h);
    assert(w == 8 && h == 6);
    for (unsigned i = 0; i < 8 * 6 * 4; ++i)
        assert(!initial[i]);
    cui_renderer api = original ? sw_test_v1_interface(r) : cui_software_interface(r);
    assert(api.user == r && api.render && api.destroy &&
           !strcmp(api.name, "CUI deterministic software"));
    cui_draw_command c[] = {
        {.type = CUI_DRAW_RECT, .bounds = {0, 0, 8, 6}, .radius = 1, .color = {20, 80, 160, 120}},
        {.type = CUI_DRAW_CLIP_PUSH, .bounds = {1, 1, 5, 4}},
        {.type = CUI_DRAW_BORDER,
         .bounds = {1, 1, 5, 4},
         .radius = 1,
         .stroke = 1,
         .color = {200, 30, 70, 127}},
        {.type = CUI_DRAW_DROP_SHADOW,
         .shadow = {.caster = {1, 1, 3, 2},
                    .radii = {.5f, .25f, .75f, 1},
                    .layer = {.sigma = 1.25f, .color = {30, 60, 90, 173}}}},
        {.type = CUI_DRAW_INSET_SHADOW,
         .shadow = {.caster = {1, 1, 3, 2},
                    .radii = {.5f, .25f, .75f, 1},
                    .layer = {.offset_x = .5f, .sigma = .75f, .color = {90, 40, 110, 123}}}},
        {.type = CUI_DRAW_CLIP_POP}};
    cui_draw_list list = {.commands = c, .count = sizeof c / sizeof *c, .width = 8, .height = 6};
    cui_color clear = {21, 31, 41, 53};
    assert(api.render(api.user, &list, clear));
    result out = {.caps = api.capabilities};
    memcpy(out.first, pixels(original, r, NULL, NULL), sizeof out.first);
    assert(api.render(api.user, &list, clear));
    assert(!memcmp(out.first, pixels(original, r, NULL, NULL), sizeof out.first));
    const unsigned char *before = pixels(original, r, NULL, NULL);
    active.fail = active.calls + 1;
    assert(!(original ? sw_test_v1_resize(r, 9, 7) : cui_software_resize(r, 9, 7)));
    assert(pixels(original, r, &w, &h) == before && w == 8 && h == 6);
    assert(!memcmp(before, out.first, sizeof out.first));
    active.fail = 0;
    assert(original ? sw_test_v1_resize(r, 9, 7) : cui_software_resize(r, 9, 7));
    const unsigned char *resized = pixels(original, r, &w, &h);
    assert(w == 9 && h == 7);
    for (unsigned i = 0; i < 9 * 7 * 4; ++i)
        assert(!resized[i]);
    list.width = 9;
    list.height = 7;
    assert(api.render(api.user, &list, clear));
    memcpy(out.resized, pixels(original, r, NULL, NULL), sizeof out.resized);
    api.destroy(api.user);
    assert(!active.live);
    out.allocation_trace = active;
    return out;
}
static trace constructor_failure(bool original, size_t fail) {
    active = (trace){.fail = fail};
    void *r = original ? sw_test_v1_create(8, 6) : cui_software_create(8, 6, NULL);
    assert(!r && !active.live);
    return active;
}
static void compare_trace(const trace *a, const trace *b, size_t old_size, size_t new_size) {
    assert(a->count == b->count && a->calls == b->calls);
    for (size_t i = 0; i < a->count; ++i) {
        event x = a->events[i], y = b->events[i];
        assert(x.operation == y.operation && x.token == y.token);
        if ((i == 0 || x.token == 1) && x.bytes == old_size)
            assert(y.bytes == new_size);
        else
            assert(x.bytes == y.bytes);
    }
}
int main(void) {
    size_t old[10];
    sw_test_v1_layout(old);
    const size_t now[] = {sizeof(cui_software),
                          _Alignof(cui_software),
                          offsetof(cui_software, shadows),
                          offsetof(cui_software, width),
                          offsetof(cui_software, height),
                          offsetof(cui_software, pixels),
                          offsetof(cui_software, font),
                          offsetof(cui_software, clip),
                          offsetof(cui_software, ink),
                          offsetof(cui_software, text_paint)};
    assert(now[0] >= old[0] && now[0] - old[0] <= old[1]);
    for (size_t i = 1; i < 10; ++i)
        assert(now[i] == old[i]);
    assert(offsetof(cui_software, v2_busy) >= old[9] + sizeof(cui_text_paint));
    result original = exercise(true), candidate = exercise(false);
    assert(original.caps == candidate.caps &&
           !memcmp(original.first, candidate.first, sizeof original.first));
    assert(!memcmp(original.resized, candidate.resized, sizeof original.resized));
    trace *a = &original.allocation_trace, *b = &candidate.allocation_trace;
    compare_trace(a, b, old[0], now[0]);
    for (size_t fail = 1; fail <= 2; ++fail) {
        trace x = constructor_failure(true, fail), y = constructor_failure(false, fail);
        compare_trace(&x, &y, old[0], now[0]);
    }
    printf("V1 object%zu->%zu align%zu, %zu allocation calls/%zu events, pixels/counts/old offsets "
           "unchanged\\n",
           old[0], now[0], old[1], a->calls, a->count);
    return 0;
}
