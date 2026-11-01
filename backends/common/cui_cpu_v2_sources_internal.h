/* SPDX-License-Identifier: MIT */
#ifndef CUI_CPU_V2_SOURCES_INTERNAL_H
#define CUI_CPU_V2_SOURCES_INTERNAL_H
#include "../../src/core/cui_background_prepare_internal.h"
#include "cui/cui_draw_v2.h"
typedef struct {
    const void *pointer;
    size_t bytes;
} cui_cpu_v2_exclusion;
static inline bool cui_cpu_v2_ranges_disjoint(const void *p, size_t bytes,
                                              const cui_cpu_v2_exclusion *ranges, size_t count) {
    for (size_t i = 0; i < count; ++i)
        if (!cui_background_prepare_disjoint_internal(p, bytes, ranges[i].pointer, ranges[i].bytes))
            return false;
    return true;
}
static inline bool cui_cpu_v2_has_exclusion(const cui_cpu_v2_exclusion *ranges, size_t count) {
    for (size_t i = 0; i < count; ++i)
        if (ranges[i].bytes)
            return true;
    return false;
}
typedef enum {
    CUI_CPU_SOURCE_IMAGE,
    CUI_CPU_SOURCE_MESH,
    CUI_CPU_SOURCE_TEXT_BYTES,
    CUI_CPU_SOURCE_TEXT_SPANS,
    CUI_CPU_SOURCE_TEXT_STOPS
} cui_cpu_v2_source_part;
/* Command/range records and all input ranges stay borrowed/stable. out is a
 * writable caller-local record disjoint from inputs, never renderer/frame state.
 * No pointee reads, geometry, font/native calls or callbacks. Inactive selector
 * returns NULL/0; active zero extent retains the selected pointer. Error leaves
 * out unchanged. Arithmetic checks here do not impose aggregate work policy. */
static inline cui_error cui_cpu_v2_source_range(const cui_draw_command_v2 *v,
                                                cui_cpu_v2_source_part part,
                                                cui_cpu_v2_exclusion *out) {
    if (!v || !out || part < CUI_CPU_SOURCE_IMAGE || part > CUI_CPU_SOURCE_TEXT_STOPS)
        return CUI_ERROR_INVALID_ACTION;
    cui_cpu_v2_exclusion result = {0};
    if (v->kind == CUI_DRAW_V2_BACKGROUND) {
        *out = result;
        return CUI_OK;
    }
    if (v->kind > CUI_DRAW_INSET_SHADOW || v->payload.legacy.type != (cui_draw_type)v->kind)
        return CUI_ERROR_INVALID_ACTION;
    const cui_draw_command *c = &v->payload.legacy;
    if (v->kind == CUI_DRAW_IMAGE && part == CUI_CPU_SOURCE_IMAGE) {
        const cui_bitmap *b = &c->bitmap.bitmap;
        if (b->height && b->stride > SIZE_MAX / b->height)
            return CUI_ERROR_CAPACITY;
        result = (cui_cpu_v2_exclusion){b->pixels, b->stride * b->height};
    } else if (v->kind == CUI_DRAW_MESH && part == CUI_CPU_SOURCE_MESH) {
        if (c->mesh.vertex_count > SIZE_MAX / sizeof(*c->mesh.vertices))
            return CUI_ERROR_CAPACITY;
        result = (cui_cpu_v2_exclusion){c->mesh.vertices,
                                        c->mesh.vertex_count * sizeof(*c->mesh.vertices)};
    } else if (v->kind == CUI_DRAW_TEXT) {
        if (part == CUI_CPU_SOURCE_TEXT_BYTES) {
            cui_str source = c->text_source.paragraph.data ? c->text_source.paragraph : c->text;
            result = (cui_cpu_v2_exclusion){source.data, source.length};
        } else if (part == CUI_CPU_SOURCE_TEXT_SPANS && c->text_source.paragraph.data) {
            if (c->text_color_count > SIZE_MAX / sizeof(*c->text_colors))
                return CUI_ERROR_CAPACITY;
            result = (cui_cpu_v2_exclusion){c->text_colors,
                                            c->text_color_count * sizeof(*c->text_colors)};
        } else if (part == CUI_CPU_SOURCE_TEXT_STOPS &&
                   c->text_paint.kind == CUI_TEXT_PAINT_LINEAR) {
            if (c->text_paint.stop_count > SIZE_MAX / sizeof(*c->text_paint.stops))
                return CUI_ERROR_CAPACITY;
            result = (cui_cpu_v2_exclusion){c->text_paint.stops, c->text_paint.stop_count *
                                                                     sizeof(*c->text_paint.stops)};
        }
    }
    *out = result;
    return CUI_OK;
}
/* Existing validator calls this only at its original alias sites, after its
 * original arithmetic/style guards. Do not move a whole source walk into it. */
static inline bool cui_cpu_v2_source_alias(const cui_draw_command_v2 *v,
                                           cui_cpu_v2_source_part part, const void *out,
                                           size_t out_bytes, const cui_cpu_v2_exclusion *ranges,
                                           size_t count) {
    cui_cpu_v2_exclusion source;
    return cui_cpu_v2_source_range(v, part, &source) == CUI_OK &&
           cui_background_prepare_disjoint_internal(source.pointer, source.bytes, out, out_bytes) &&
           cui_cpu_v2_ranges_disjoint(source.pointer, source.bytes, ranges, count);
}
static inline bool cui_cpu_v2_source_extent(cui_cpu_v2_exclusion source) {
    return !source.bytes ||
           (source.pointer && source.bytes <= UINTPTR_MAX - (uintptr_t)source.pointer);
}
static inline bool cui_cpu_v2_source_add(size_t *total, size_t bytes, size_t limit) {
    if (*total > limit || bytes > limit - *total)
        return false;
    *total += bytes;
    return true;
}
/* New prequery policy only. All record pointers describe accessible objects;
 * exclusion descriptors are borrowed/stable caller-local controls (max7).
 * Checks known extents/overlaps, not opaque storage accessibility/graph identity.
 * No out record, allocation, source-content read or owner mutation. */
static inline cui_error cui_cpu_v2_sources_exclude(const cui_draw_list_v2 *list,
                                                   const cui_render_v2_options *options,
                                                   const cui_cpu_v2_exclusion *ranges,
                                                   size_t count) {
    if (!list || count > 7 || (!ranges && count) ||
        !cui_cpu_v2_source_extent((cui_cpu_v2_exclusion){list, sizeof(*list)}) ||
        !cui_cpu_v2_ranges_disjoint(list, sizeof(*list), ranges, count) ||
        (options && (!cui_cpu_v2_source_extent((cui_cpu_v2_exclusion){options, sizeof(*options)}) ||
                     !cui_cpu_v2_ranges_disjoint(options, sizeof(*options), ranges, count))))
        return CUI_ERROR_INVALID_ACTION;
    if (list->header_size != sizeof(*list) || list->abi_version != CUI_DRAW_ABI_V2 ||
        list->command_stride != sizeof(*list->commands) || list->error != CUI_OK ||
        list->background_output != CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1 ||
        (list->required_features & ~CUI_DRAW_V2_FEATURE_BACKGROUNDS))
        return CUI_ERROR_INVALID_ACTION;
    if (list->count > 16384 || list->count > SIZE_MAX / sizeof(*list->commands))
        return CUI_ERROR_CAPACITY;
    cui_cpu_v2_exclusion commands = {list->commands, list->count * sizeof(*list->commands)};
    if (!cui_cpu_v2_source_extent(commands) ||
        !cui_cpu_v2_ranges_disjoint(commands.pointer, commands.bytes, ranges, count))
        return CUI_ERROR_INVALID_ACTION;
    size_t text_bytes = 0, inspected = 0;
    for (size_t i = 0; i < list->count; ++i) {
        const cui_draw_command_v2 *v = &list->commands[i];
        cui_cpu_v2_exclusion sources[CUI_CPU_SOURCE_TEXT_STOPS + 1];
        for (unsigned part = CUI_CPU_SOURCE_IMAGE; part <= CUI_CPU_SOURCE_TEXT_STOPS; ++part) {
            cui_error error =
                cui_cpu_v2_source_range(v, (cui_cpu_v2_source_part)part, &sources[part]);
            if (error != CUI_OK)
                return error;
        }
        cui_cpu_v2_exclusion bytes = sources[CUI_CPU_SOURCE_TEXT_BYTES];
        if (v->kind == CUI_DRAW_TEXT) {
            const cui_draw_command *c = &v->payload.legacy;
            if (c->text_color_count > CUI_TEXT_COLOR_SPAN_MAX ||
                (c->text_paint.kind == CUI_TEXT_PAINT_LINEAR && c->text_paint.stop_count > 8))
                return CUI_ERROR_CAPACITY;
            if ((c->text_color_count && (!c->text_source.paragraph.data || !c->text_colors)) ||
                (!c->text_source.paragraph.data &&
                 (c->text_source.paragraph.length || c->text_source.byte_start ||
                  c->text_source.byte_end)) ||
                (c->text_paint.kind != CUI_TEXT_PAINT_SOLID &&
                 c->text_paint.kind != CUI_TEXT_PAINT_LINEAR) ||
                (c->text_paint.kind == CUI_TEXT_PAINT_SOLID && c->text_paint.stop_count) ||
                (c->text_paint.kind == CUI_TEXT_PAINT_LINEAR &&
                 (!c->text_paint.stops || c->text_paint.stop_count < 2 ||
                  c->text_paint.stop_count > 8 || c->text_color_count)))
                return CUI_ERROR_INVALID_ACTION;
            if (bytes.bytes > 65536 || !cui_cpu_v2_source_add(&text_bytes, bytes.bytes, 256 * 1024))
                return CUI_ERROR_CAPACITY;
            size_t repeats = 1 + c->text_color_count;
            if ((bytes.bytes && repeats > SIZE_MAX / bytes.bytes) ||
                !cui_cpu_v2_source_add(&inspected, repeats * bytes.bytes, (size_t)32 * 1024 * 1024))
                return CUI_ERROR_CAPACITY;
        }
        for (unsigned part = CUI_CPU_SOURCE_IMAGE; part <= CUI_CPU_SOURCE_TEXT_STOPS; ++part) {
            cui_cpu_v2_exclusion source = sources[part];
            if (part == CUI_CPU_SOURCE_MESH &&
                !cui_cpu_v2_source_add(&inspected, source.bytes, (size_t)32 * 1024 * 1024))
                return CUI_ERROR_CAPACITY;
            if (!cui_cpu_v2_source_extent(source) ||
                !cui_cpu_v2_ranges_disjoint(source.pointer, source.bytes, ranges, count))
                return CUI_ERROR_INVALID_ACTION;
        }
    }
    return CUI_OK;
}
#endif
