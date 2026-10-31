/* SPDX-License-Identifier: MIT */
#ifndef CUI_TEXT_ALLOC_INTERNAL_H
#define CUI_TEXT_ALLOC_INTERNAL_H
#include "../core/cui_alignment_internal.h"
#include "cui/cui.h"
#include <stdlib.h>
#include <string.h>
/* Private counted CUI payload; opaque FT/HB/SB storage is explicitly separate.
 * Callback blocks are malloc-aligned, distinct and writable. NULL owner preserves
 * the exact legacy malloc/calloc/free route (no prefix, counters or work limits). */
typedef void *(*cui_text_allocate_internal)(void *, size_t);
typedef void (*cui_text_release_internal)(void *, void *);
typedef struct {
    cui_text_allocate_internal allocate;
    cui_text_release_internal release;
    void *user;
    size_t cpu_limit, metadata_limit, cpu_used, metadata_used, cpu_peak, metadata_peak;
    size_t input_bytes, attempts, segments, glyphs, coverage_pixels, dependencies;
    size_t fail_dependency; /* Private deterministic fault seam; zero disables. */
    const void *faces[64];
    size_t face_count;
    cui_error error;
    bool callback_active;
} cui_text_owner_internal;
typedef union {
    cui_fundamental_alignment_internal alignment;
    struct {
        size_t bytes;
        bool metadata;
    } value;
} cui_text_allocation_internal;
static inline void *cui_text_alloc_internal(cui_text_owner_internal *o, size_t count, size_t size,
                                            bool clear, bool metadata) {
    if (!o)
        return clear ? calloc(count, size) : malloc(count * size);
    if (o->error != CUI_OK || o->callback_active || !o->allocate || !o->release ||
        (size && count > SIZE_MAX / size)) {
        o->error = CUI_ERROR_CAPACITY;
        return NULL;
    }
    size_t n = count * size, header = sizeof(cui_text_allocation_internal);
    size_t *used = metadata ? &o->metadata_used : &o->cpu_used;
    size_t limit = metadata ? o->metadata_limit : o->cpu_limit;
    if (o->cpu_limit > (size_t)32 * 1024 * 1024 || o->metadata_limit > (size_t)1024 * 1024 ||
        n > SIZE_MAX - header || *used > limit || n + header > limit - *used) {
        o->error = CUI_ERROR_CAPACITY;
        return NULL;
    }
    o->callback_active = true;
    cui_text_allocation_internal *p = o->allocate(o->user, n + header);
    o->callback_active = false;
    if (!p) {
        o->error = CUI_ERROR_CAPACITY;
        return NULL;
    }
    p->value.bytes = n + header;
    p->value.metadata = metadata;
    *used += n + header;
    size_t *peak = metadata ? &o->metadata_peak : &o->cpu_peak;
    if (*used > *peak)
        *peak = *used;
    if (clear)
        memset(p + 1, 0, n);
    return p + 1;
}
static inline void cui_text_free_internal(cui_text_owner_internal *o, void *pointer) {
    if (!o) {
        free(pointer);
        return;
    }
    if (!pointer)
        return;
    cui_text_allocation_internal *p = (cui_text_allocation_internal *)pointer - 1;
    size_t *used = p->value.metadata ? &o->metadata_used : &o->cpu_used;
    *used -= p->value.bytes;
    o->callback_active = true;
    o->release(o->user, p);
    o->callback_active = false;
}
static inline bool cui_text_dependency_internal(cui_text_owner_internal *o) {
    if (!o)
        return true;
    if (o->error != CUI_OK)
        return false;
    ++o->dependencies;
    if (o->fail_dependency && o->dependencies == o->fail_dependency) {
        o->error = CUI_ERROR_CAPACITY;
        return false;
    }
    return true;
}
static inline bool cui_text_face_internal(cui_text_owner_internal *o, const void *face) {
    if (!o)
        return true;
    for (size_t i = 0; i < o->face_count; ++i)
        if (o->faces[i] == face)
            return true;
    if (o->face_count >= 64) {
        o->error = CUI_ERROR_CAPACITY;
        return false;
    }
    o->faces[o->face_count++] = face;
    return true;
}
#endif
