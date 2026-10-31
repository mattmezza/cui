/* SPDX-License-Identifier: MIT */
#ifndef CUI_SOFTWARE_V2_INTERNAL_H
#define CUI_SOFTWARE_V2_INTERNAL_H
#include "../../src/core/cui_alignment_internal.h"
#include "../../src/core/cui_background_prepare_internal.h"
#include "../../src/core/cui_text_paint_internal.h"
#include "cui_shadow.h"
#include <limits.h>
/* Private building block only: structural/resource validation is NOT completed
 * resource preparation or permission to publish/render. No V1 call site uses it. */
typedef struct {
    cui_rect clip;
    cui_background_prepared_query background;
} cui_software_v2_descriptor;
typedef struct {
    size_t commands, backgrounds, background_pixels, background_bytes;
    size_t scratch_bytes, shadow_bytes, metadata_bytes, cpu_bytes, inspected_bytes;
    bool needs_text_preparation, needs_shadow_preparation;
} cui_software_v2_plan;
static inline bool cui_software_v2_add(size_t *total, size_t n, size_t limit) {
    if (*total > limit || n > limit - *total)
        return false;
    *total += n;
    return true;
}
static inline bool cui_software_v2_rect(cui_rect r) {
    return isfinite(r.x) && isfinite(r.y) && isfinite(r.w) && isfinite(r.h) && r.w >= 0 &&
           r.h >= 0 && isfinite((double)r.x + r.w) && isfinite((double)r.y + r.h);
}
static inline cui_rect cui_software_v2_intersection(cui_rect a, cui_rect b) {
    double x = fmax(a.x, b.x), y = fmax(a.y, b.y);
    return (cui_rect){(float)x, (float)y,
                      (float)fmax(0, fmin((double)a.x + a.w, (double)b.x + b.w) - x),
                      (float)fmax(0, fmin((double)a.y + a.h, (double)b.y + b.h) - y)};
}
static inline bool cui_software_v2_shape(const cui_draw_command *c, bool border) {
    if (c->custom_corners) {
        cui_corner_radii r = c->corner_radii;
        if (!isfinite(r.top_left) || !isfinite(r.top_right) || !isfinite(r.bottom_left) ||
            !isfinite(r.bottom_right))
            return false;
    } else if (!isfinite(c->radius))
        return false;
    if (!border)
        return true;
    if (c->custom_border_edges) {
        cui_border_edges e = c->border_edges;
        return isfinite(e.left) && isfinite(e.right) && isfinite(e.top) && isfinite(e.bottom);
    }
    return isfinite(c->stroke);
}
static inline bool cui_software_v2_alias(const void *a, size_t n, const void *out) {
    return cui_background_prepare_disjoint_internal(a, n, out, sizeof(cui_software_v2_plan));
}
/* font_available and tracking_available describe actual host state, not inferred
 * from a fabricated public support record. Font/shaping allocation is deferred. */
static inline cui_error cui_software_v2_validate(const cui_draw_list_v2 *list,
                                                 const cui_render_v2_options *options,
                                                 bool font_available, bool tracking_available,
                                                 cui_software_v2_plan *out) {
    if (!list || !options || !out || !cui_software_v2_alias(list, sizeof(*list), out) ||
        !cui_software_v2_alias(options, sizeof(*options), out) ||
        list->header_size != sizeof(*list) || list->abi_version != CUI_DRAW_ABI_V2 ||
        list->command_stride != sizeof(cui_draw_command_v2) || list->error != CUI_OK ||
        list->background_output != CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1 ||
        (list->required_features & ~CUI_DRAW_V2_FEATURE_BACKGROUNDS) || !isfinite(list->width) ||
        !isfinite(list->height) || list->width < 0 || list->height < 0)
        return CUI_ERROR_INVALID_ACTION;
    if (list->count > 16384)
        return CUI_ERROR_CAPACITY;
    if ((!list->commands && list->count) ||
        !cui_software_v2_alias(list->commands, list->count * sizeof(*list->commands), out))
        return CUI_ERROR_INVALID_ACTION;
    /* Validate every common option even for an empty/legacy-only list. */
    cui_background_draw_v2 empty = {0};
    empty.brush.kind = CUI_BACKGROUND_SOLID;
    cui_background_prepared_query q;
    cui_error error = cui_background_prepare_query(empty, options, (cui_rect){0}, &q);
    if (error != CUI_OK)
        return error;
    cui_software_v2_plan plan = {.commands = list->count};
    size_t target_pixels = (size_t)options->viewport_width * options->viewport_height;
    if (target_pixels > SIZE_MAX / 4)
        return CUI_ERROR_CAPACITY;
    plan.scratch_bytes = target_pixels * 4;
    plan.metadata_bytes = list->count * sizeof(cui_software_v2_descriptor);
    if (plan.metadata_bytes > options->metadata_bytes || plan.scratch_bytes > options->cpu_bytes)
        return CUI_ERROR_CAPACITY;
    /* The shared query clips to the physical viewport itself. Avoid an inverse
     * viewport rounded to float accidentally cropping a fractional cover cell. */
    cui_rect stack[64], clip = {-FLT_MAX / 4, -FLT_MAX / 4, FLT_MAX / 2, FLT_MAX / 2};
    size_t depth = 0;
    for (size_t i = 0; i < list->count; ++i) {
        const cui_draw_command_v2 *v = &list->commands[i];
        if (v->kind == CUI_DRAW_V2_BACKGROUND) {
            if (!(list->required_features & CUI_DRAW_V2_FEATURE_BACKGROUNDS))
                return CUI_ERROR_INVALID_ACTION;
            error = cui_background_prepare_query(v->payload.background, options, clip, &q);
            if (error != CUI_OK)
                return error;
            if (++plan.backgrounds > 4096 ||
                !cui_software_v2_add(&plan.background_pixels, (size_t)q.width * q.height,
                                     options->max_pixels) ||
                !cui_software_v2_add(&plan.background_bytes, q.bytes, options->cpu_bytes))
                return CUI_ERROR_CAPACITY;
            continue;
        }
        if (v->kind > CUI_DRAW_INSET_SHADOW || v->payload.legacy.type != (cui_draw_type)v->kind)
            return CUI_ERROR_INVALID_ACTION;
        const cui_draw_command *c = &v->payload.legacy;
        /* POP has no active geometry/resource fields. */
        if (c->type == CUI_DRAW_CLIP_POP) {
            if (!depth)
                return CUI_ERROR_INVALID_ACTION;
            clip = stack[--depth];
            continue;
        }
        if (c->type == CUI_DRAW_TEXT) {
            if (!isfinite(c->bounds.x) || !isfinite(c->bounds.y))
                return CUI_ERROR_INVALID_ACTION;
        } else if (c->type != CUI_DRAW_MESH && !cui_shadow_type(c->type) &&
                   !cui_software_v2_rect(c->bounds))
            return CUI_ERROR_INVALID_ACTION;
        if (c->type == CUI_DRAW_CLIP_PUSH) {
            if (depth == 64)
                return CUI_ERROR_CAPACITY;
            stack[depth++] = clip;
            clip = cui_software_v2_intersection(clip, c->bounds);
            continue;
        }
        if (c->type == CUI_DRAW_RECT || c->type == CUI_DRAW_BORDER || c->type == CUI_DRAW_IMAGE)
            if (!cui_software_v2_shape(c, c->type == CUI_DRAW_BORDER))
                return CUI_ERROR_INVALID_ACTION;
        if (c->type == CUI_DRAW_IMAGE) {
            cui_bitmap_paint b = c->bitmap;
            if (!cui_bitmap_valid(b.bitmap) || !isfinite(b.opacity) || b.opacity < 0 ||
                b.opacity > 1 || !cui_software_v2_rect(b.source) || b.source.x < 0 ||
                b.source.y < 0 || b.source.w <= 0 || b.source.h <= 0 ||
                (double)b.source.x + b.source.w > b.bitmap.width ||
                (double)b.source.y + b.source.h > b.bitmap.height ||
                !cui_software_v2_alias(b.bitmap.pixels, b.bitmap.stride * b.bitmap.height, out))
                return CUI_ERROR_INVALID_ACTION;
        } else if (c->type == CUI_DRAW_MESH) {
            if (c->mesh.vertex_count > SIZE_MAX / sizeof(cui_mesh_vertex) ||
                !cui_software_v2_add(&plan.inspected_bytes,
                                     c->mesh.vertex_count * sizeof(cui_mesh_vertex),
                                     (size_t)32 * 1024 * 1024))
                return CUI_ERROR_CAPACITY;
            if (!cui_software_v2_alias(c->mesh.vertices,
                                       c->mesh.vertex_count * sizeof(cui_mesh_vertex), out) ||
                !cui_mesh_valid(c->mesh))
                return CUI_ERROR_INVALID_ACTION;
        } else if (cui_shadow_type(c->type)) {
            cui_drop_shadow_raster shadow;
            /* Value/domain validation and conservative uniform-source reservation.
             * Anisotropic shadow reconstruction remains a later painter decision. */
            float scale = fmaxf(options->scale_x, options->scale_y);
            if (!cui_shadow_query(c->type, c->shadow, scale, options->origin_x, options->origin_y,
                                  &shadow))
                return CUI_ERROR_INVALID_ACTION;
            if (!cui_software_v2_add(&plan.shadow_bytes, shadow.bytes, options->cpu_bytes))
                return CUI_ERROR_CAPACITY;
            plan.needs_shadow_preparation = true;
        } else if (c->type == CUI_DRAW_TEXT) {
            if (!font_available || !isfinite(c->font_size) || c->font_size <= 0 ||
                c->font_size > 65535 || c->text_style.weight < CUI_TEXT_WEIGHT_NORMAL ||
                c->text_style.weight > CUI_TEXT_WEIGHT_BOLD ||
                !isfinite(c->text_style.letter_spacing) ||
                (c->text_style.letter_spacing != 0 && !tracking_available) ||
                !cui_text_paint_valid(c->text_paint, true) ||
                (c->text_paint.kind == CUI_TEXT_PAINT_LINEAR && c->text_color_count) ||
                c->text_color_count > CUI_TEXT_COLOR_SPAN_MAX ||
                (c->text_color_count && !c->text_colors))
                return CUI_ERROR_INVALID_ACTION;
            size_t text_bytes =
                c->text_source.paragraph.data ? c->text_source.paragraph.length : c->text.length;
            if (text_bytes > INT32_MAX ||
                !cui_software_v2_add(&plan.inspected_bytes, text_bytes, (size_t)32 * 1024 * 1024))
                return CUI_ERROR_CAPACITY;
            if (c->text_source.paragraph.data) {
                if (!cui_software_v2_alias(c->text_source.paragraph.data, text_bytes, out) ||
                    !cui_text_line_source_valid(c->text_source))
                    return CUI_ERROR_INVALID_ACTION;
                size_t previous = c->text_source.byte_start;
                if (!cui_software_v2_alias(c->text_colors,
                                           c->text_color_count * sizeof(*c->text_colors), out))
                    return CUI_ERROR_INVALID_ACTION;
                for (size_t j = 0; j < c->text_color_count; ++j) {
                    cui_text_line_source range = {c->text_source.paragraph,
                                                  c->text_colors[j].byte_start,
                                                  c->text_colors[j].byte_end};
                    if (!cui_software_v2_add(&plan.inspected_bytes, text_bytes,
                                             (size_t)32 * 1024 * 1024))
                        return CUI_ERROR_CAPACITY;
                    if (range.byte_start < previous || range.byte_end > c->text_source.byte_end ||
                        !cui_text_line_source_valid(range))
                        return CUI_ERROR_INVALID_ACTION;
                    previous = range.byte_end;
                }
            } else if (c->text_source.paragraph.length || c->text_source.byte_start ||
                       c->text_source.byte_end || (c->text.length && !c->text.data) ||
                       c->text_color_count ||
                       !cui_software_v2_alias(c->text.data, c->text.length, out))
                return CUI_ERROR_INVALID_ACTION;
            if (c->text_paint.kind == CUI_TEXT_PAINT_LINEAR &&
                !cui_software_v2_alias(c->text_paint.stops,
                                       c->text_paint.stop_count * sizeof(*c->text_paint.stops),
                                       out))
                return CUI_ERROR_INVALID_ACTION;
            plan.needs_text_preparation = true;
        }
    }
    if (depth)
        return CUI_ERROR_INVALID_ACTION;
    plan.cpu_bytes = plan.scratch_bytes;
    if (!cui_software_v2_add(&plan.cpu_bytes, plan.background_bytes, options->cpu_bytes) ||
        !cui_software_v2_add(&plan.cpu_bytes, plan.shadow_bytes, options->cpu_bytes) ||
        plan.background_bytes > options->texture_bytes ||
        plan.background_bytes > options->transfer_bytes)
        return CUI_ERROR_CAPACITY;
    *out = plan;
    return CUI_OK;
}
/* Optional transaction arena. Future resource producers MUST use these counted
 * allocations; uncounted font/shadow temporaries cannot be declared prepared. */
/* Allocator callbacks follow malloc ownership: each successful nonzero request
 * returns a distinct live writable block of at least the requested byte size,
 * aligned for cui_fundamental_alignment_internal. NULL reports failure. release accepts each
 * returned block exactly once. They must not expose/retain transaction memory after release;
 * workspace API reentrancy during allocate/release is rejected. Offset alignment
 * in take relies on this base-alignment contract, not on an arbitrary void pointer. */
typedef void *(*cui_software_v2_alloc_fn)(void *, size_t);
typedef void (*cui_software_v2_free_fn)(void *, void *);
_Static_assert(_Alignof(cui_draw_command_v2) <= _Alignof(cui_fundamental_alignment_internal),
               "V2 command exceeds workspace base alignment");
_Static_assert(_Alignof(cui_software_v2_descriptor) <= _Alignof(cui_fundamental_alignment_internal),
               "V2 descriptor exceeds workspace base alignment");
_Static_assert(_Alignof(cui_software_v2_alloc_fn) <= _Alignof(cui_fundamental_alignment_internal),
               "allocator pointer exceeds workspace base alignment");
_Static_assert(_Alignof(cui_software_v2_free_fn) <= _Alignof(cui_fundamental_alignment_internal),
               "release pointer exceeds workspace base alignment");
typedef struct {
    void *cpu, *metadata;
    size_t cpu_capacity, metadata_capacity, cpu_used, metadata_used;
    cui_software_v2_free_fn release;
    void *user;
    bool active, busy;
} cui_software_v2_workspace;
static inline void cui_software_v2_workspace_end(cui_software_v2_workspace *w) {
    if (!w || !w->active || w->busy)
        return;
    w->busy = true;
    if (w->cpu)
        w->release(w->user, w->cpu);
    if (w->metadata)
        w->release(w->user, w->metadata);
    *w = (cui_software_v2_workspace){0};
}
static inline cui_error cui_software_v2_workspace_begin(cui_software_v2_workspace *w,
                                                        size_t cpu_bytes, size_t metadata_bytes,
                                                        cui_software_v2_alloc_fn allocate,
                                                        cui_software_v2_free_fn release,
                                                        void *user) {
    if (!w || w->active || w->busy || !allocate || !release)
        return CUI_ERROR_INVALID_ACTION;
    if (cpu_bytes > (size_t)32 * 1024 * 1024 || metadata_bytes > (size_t)1024 * 1024)
        return CUI_ERROR_CAPACITY;
    cui_software_v2_workspace previous = *w;
    w->busy = true;
    cui_software_v2_workspace next = {.cpu_capacity = cpu_bytes,
                                      .metadata_capacity = metadata_bytes,
                                      .release = release,
                                      .user = user,
                                      .active = true};
    if (cpu_bytes && !(next.cpu = allocate(user, cpu_bytes))) {
        *w = previous;
        return CUI_ERROR_CAPACITY;
    }
    if (metadata_bytes && !(next.metadata = allocate(user, metadata_bytes))) {
        cui_software_v2_workspace_end(&next);
        *w = previous;
        return CUI_ERROR_CAPACITY;
    }
    *w = next;
    return CUI_OK;
}
static inline void *cui_software_v2_workspace_take(cui_software_v2_workspace *w, bool metadata,
                                                   size_t bytes, size_t alignment) {
    if (!w || !w->active || w->busy || !bytes || !alignment ||
        alignment > _Alignof(cui_fundamental_alignment_internal) || (alignment & (alignment - 1)))
        return NULL;
    size_t *used = metadata ? &w->metadata_used : &w->cpu_used;
    size_t capacity = metadata ? w->metadata_capacity : w->cpu_capacity;
    size_t padding = (alignment - *used % alignment) % alignment;
    if (*used > capacity || padding > capacity - *used || bytes > capacity - *used - padding)
        return NULL;
    unsigned char *base = metadata ? w->metadata : w->cpu;
    void *result = base + *used + padding;
    *used += padding + bytes;
    return result;
}
#endif
