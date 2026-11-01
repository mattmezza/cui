/* SPDX-License-Identifier: MIT */
#ifndef CUI_CPU_V2_PAINT_INTERNAL_H
#define CUI_CPU_V2_PAINT_INTERNAL_H
// clang-format off
#include "cui/cui_text.h"
#include "../software/cui_mesh.h"
#include "../software/cui_shape.h"
#include "cui_cpu_v2_internal.h"
// clang-format on
/* Private painter behind the explicit software V2 entry, never the V1 route.
 * Owned with-font TEXT and bounded source-profile AREA shadows. Inputs/target view stay stable
 * through return, including allocator callbacks. Allocators obey the workspace
 * distinct-live-block/alignment contract. Target pixels and input records/resources must be
 * disjoint. Entire opaque font graphs/dependency storage must also be disjoint
 * and stable, with no nested/concurrent render on the same font. A pointer-byte
 * font sentinel cannot validate the opaque graph allocation ranges. */
typedef struct {
    unsigned char *pixels;
    unsigned width, height;
    size_t stride, capacity;
    bool busy;
} cui_cpu_v2_target;
typedef struct {
    double left, top, right, bottom;
} cui_cpu_v2_clip;
typedef struct {
    cui_cpu_v2_clip clip;
    cui_background_prepared_query background;
    unsigned char *pixels;
} cui_cpu_v2_paint_descriptor;
_Static_assert(_Alignof(cui_cpu_v2_paint_descriptor) <=
                   _Alignof(cui_fundamental_alignment_internal),
               "paint descriptor exceeds workspace alignment");
static inline cui_cpu_v2_clip cui_cpu_v2_clip_all(void) {
    return (cui_cpu_v2_clip){-FLT_MAX / 4, -FLT_MAX / 4, FLT_MAX / 4, FLT_MAX / 4};
}
static inline cui_cpu_v2_clip cui_cpu_v2_clip_push(cui_cpu_v2_clip a, cui_rect b) {
    return (cui_cpu_v2_clip){fmax(a.left, b.x), fmax(a.top, b.y), fmin(a.right, (double)b.x + b.w),
                             fmin(a.bottom, (double)b.y + b.h)};
}
/* Outward float cover is culling only. Exact double center tests use clip below. */
static inline cui_rect cui_cpu_v2_clip_cover(cui_cpu_v2_clip c) {
    if (c.left >= c.right || c.top >= c.bottom)
        return (cui_rect){0};
    float x = (float)c.left, y = (float)c.top;
    if (x > c.left)
        x = nextafterf(x, -INFINITY);
    if (y > c.top)
        y = nextafterf(y, -INFINITY);
    float w = (float)(c.right - x), h = (float)(c.bottom - y);
    if ((double)x + w < c.right)
        w = nextafterf(w, INFINITY);
    if ((double)y + h < c.bottom)
        h = nextafterf(h, INFINITY);
    return (cui_rect){x, y, w, h};
}
static inline bool cui_cpu_v2_target_disjoint(const cui_cpu_v2_target *t, size_t span,
                                              const void *p, size_t bytes) {
    return cui_background_prepare_disjoint_internal(t->pixels, span, p, bytes) &&
           cui_background_prepare_disjoint_internal(t, sizeof(*t), p, bytes);
}
static inline bool cui_cpu_v2_pixel_inside(cui_cpu_v2_clip c, double x, double y) {
    return x >= c.left && x < c.right && y >= c.top && y < c.bottom;
}
/* Same straight-byte source-over arithmetic as immutable V1, including alpha0. */
static inline void cui_cpu_v2_blend(unsigned char *p, cui_color c) {
    unsigned alpha = c.a * 255u + p[3] * (255u - c.a);
    if (!alpha) {
        memset(p, 0, 4);
        return;
    }
    unsigned src[3] = {c.r, c.g, c.b};
    for (unsigned i = 0; i < 3; ++i)
        p[i] =
            (unsigned char)((src[i] * c.a * 255u + p[i] * p[3] * (255u - c.a) + alpha / 2) / alpha);
    p[3] = (unsigned char)((alpha + 127) / 255);
}
typedef struct {
    unsigned char *pixels;
    const cui_render_v2_options *options;
    cui_cpu_v2_clip clip;
} cui_cpu_v2_surface;
static inline void cui_cpu_v2_emit(void *user, int x, int y, cui_color color) {
    cui_cpu_v2_surface *s = user;
    const cui_render_v2_options *o = s->options;
    double lx = (x + .5 - o->origin_x) / o->scale_x;
    double ly = (y + .5 - o->origin_y) / o->scale_y;
    if (cui_cpu_v2_pixel_inside(s->clip, lx, ly))
        cui_cpu_v2_blend(s->pixels + ((size_t)y * o->viewport_width + (unsigned)x) * 4, color);
}
/* Intersect in double/device space before any integer cast, even for huge input. */
static inline bool cui_cpu_v2_cover(cui_rect bounds, cui_cpu_v2_surface *s, int *x0, int *y0,
                                    int *x1, int *y1) {
    const cui_render_v2_options *o = s->options;
    double left = fmax(bounds.x, s->clip.left), top = fmax(bounds.y, s->clip.top);
    double right = fmin((double)bounds.x + bounds.w, s->clip.right);
    double bottom = fmin((double)bounds.y + bounds.h, s->clip.bottom);
    if (left >= right || top >= bottom)
        return false;
    left = fmax(0, left * o->scale_x + o->origin_x);
    top = fmax(0, top * o->scale_y + o->origin_y);
    right = fmin(o->viewport_width, right * o->scale_x + o->origin_x);
    bottom = fmin(o->viewport_height, bottom * o->scale_y + o->origin_y);
    if (left >= right || top >= bottom)
        return false;
    *x0 = (int)floor(left);
    *y0 = (int)floor(top);
    *x1 = (int)ceil(right);
    *y1 = (int)ceil(bottom);
    return true;
}
#include "cui_cpu_v2_text_internal.h"
/* Original absolute edges stay separate from distances to the near edge.
 * In particular (x+w)-right_inset preserves a small far-edge inset when x+w
 * cancels to a small value; x+(w-right_inset) would lose it. */
static inline double cui_cpu_v2_arc(double rx, double ry, double distance) {
    if (rx <= 0 || ry <= 0 || distance >= ry)
        return 0;
    double delta = ry - distance;
    /* Rationalized circular/elliptical arc avoids losing a small edge inset
     * to 1-sqrt(...), and preserves exact 3-4-5 circle boundary cases. */
    double root = sqrt(fmax(0, ry * ry - delta * delta));
    return fmin(rx, (rx / ry) * (delta * delta) / (ry + root));
}
static inline bool cui_cpu_v2_span(const cui_draw_command *c, const float r[4], const float e[4],
                                   bool inner, double y, double *left, double *right) {
    double l = inner ? e[0] : 0, rr = inner ? e[1] : 0;
    double t = inner ? e[2] : 0, b = inner ? e[3] : 0;
    double near_x = (double)c->bounds.x + l;
    double far_x = ((double)c->bounds.x + c->bounds.w) - rr;
    double near_y = (double)c->bounds.y + t;
    double far_y = ((double)c->bounds.y + c->bounds.h) - b;
    if (near_x >= far_x || near_y >= far_y || y < near_y || y >= far_y)
        return false;
    double lt = cui_cpu_v2_arc(fmax(0, r[0] - l), fmax(0, r[0] - t), y - near_y);
    double lb = cui_cpu_v2_arc(fmax(0, r[2] - l), fmax(0, r[2] - b), far_y - y);
    double rt = cui_cpu_v2_arc(fmax(0, r[1] - rr), fmax(0, r[1] - t), y - near_y);
    double rb = cui_cpu_v2_arc(fmax(0, r[3] - rr), fmax(0, r[3] - b), far_y - y);
    *left = near_x + fmax(lt, lb);
    *right = far_x - fmax(rt, rb);
    return *left < *right;
}
/* Error-free addition retains which side of an integer a small endpoint
 * residual occupies. Never round an inside source coordinate to its open end. */
static inline double cui_cpu_v2_floor_sum(double a, double b) {
    double sum = a + b, z = sum - a;
    double error = (a - (sum - z)) + (b - z);
    double index = floor(sum);
    return error < 0 && sum == index ? index - 1 : index;
}
/* Compute UV from the closer absolute edge; compensate the far source sum so
 * tiny source offsets/widths are not mistaken for an open integer endpoint. */
static inline unsigned cui_cpu_v2_texel(double point, double near, double far, double source,
                                        double extent, unsigned size) {
    double distance_near = point - near, distance_far = far - point;
    double index;
    if (distance_near <= distance_far)
        index = cui_cpu_v2_floor_sum(source, distance_near / (far - near) * extent);
    else {
        double end = source + extent, z = end - source;
        double residual = (source - (end - z)) + (extent - z);
        double delta = distance_far / (far - near) * extent;
        index = cui_cpu_v2_floor_sum(end, residual - delta);
    }
    return (unsigned)fmax(0, fmin((double)size - 1, index));
}
static inline void cui_cpu_v2_shape_paint(cui_cpu_v2_surface *s, const cui_draw_command *c) {
    int x0, y0, x1, y1;
    if (!cui_cpu_v2_cover(c->bounds, s, &x0, &y0, &x1, &y1))
        return;
    float radii[4], edges[4];
    cui_shape_values(c, radii, edges);
    const cui_render_v2_options *o = s->options;
    for (int y = y0; y < y1; ++y) {
        double ly = (y + .5 - o->origin_y) / o->scale_y;
        double left, right, il = 0, ir = 0;
        if (!cui_cpu_v2_span(c, radii, edges, false, ly, &left, &right))
            continue;
        bool inner =
            c->type == CUI_DRAW_BORDER && cui_cpu_v2_span(c, radii, edges, true, ly, &il, &ir);
        for (int x = x0; x < x1; ++x) {
            double lx = (x + .5 - o->origin_x) / o->scale_x;
            if (lx < left || lx >= right || (inner && lx >= il && lx < ir))
                continue;
            cui_color color = c->color;
            if (c->type == CUI_DRAW_IMAGE) {
                const cui_bitmap_paint *p = &c->bitmap;
                unsigned sx = cui_cpu_v2_texel(lx, c->bounds.x, (double)c->bounds.x + c->bounds.w,
                                               p->source.x, p->source.w, p->bitmap.width);
                unsigned sy = cui_cpu_v2_texel(ly, c->bounds.y, (double)c->bounds.y + c->bounds.h,
                                               p->source.y, p->source.h, p->bitmap.height);
                const unsigned char *pixel =
                    p->bitmap.pixels + (size_t)sy * p->bitmap.stride + sx * 4;
                color = (cui_color){pixel[0], pixel[1], pixel[2],
                                    (unsigned char)lroundf(pixel[3] * p->opacity)};
                if (p->grayscale)
                    color.r = color.g = color.b = (unsigned char)lroundf(
                        .2126f * color.r + .7152f * color.g + .0722f * color.b);
                if (p->custom_tint) {
                    color.r = (unsigned char)((unsigned)color.r * p->tint.r / 255);
                    color.g = (unsigned char)((unsigned)color.g * p->tint.g / 255);
                    color.b = (unsigned char)((unsigned)color.b * p->tint.b / 255);
                    color.a = (unsigned char)((unsigned)color.a * p->tint.a / 255);
                }
            }
            cui_cpu_v2_emit(s, x, y, color);
        }
    }
}
static inline void cui_cpu_v2_mesh_paint(cui_cpu_v2_surface *s, const cui_mesh_paint *p) {
    const cui_render_v2_options *o = s->options;
    cui_color color = {cui_mesh_cpu_channel(p->color.r, p->color.a),
                       cui_mesh_cpu_channel(p->color.g, p->color.a),
                       cui_mesh_cpu_channel(p->color.b, p->color.a), 0};
    for (size_t i = 0; i < p->vertex_count; i += 3) {
        cui_mesh_cpu_vertex v[3];
        for (size_t j = 0; j < 3; ++j) {
            const cui_mesh_vertex *in = &p->vertices[i + j];
            v[j] = (cui_mesh_cpu_vertex){
                (p->origin_x + (double)p->scale * in->x) * o->scale_x + o->origin_x,
                (p->origin_y + (double)p->scale * in->y) * o->scale_y + o->origin_y, in->s, in->t};
        }
        cui_mesh_cpu_triangle(v, color, p->color.a, o->viewport_width, o->viewport_height,
                              (cui_rect){0, 0, (float)o->viewport_width, (float)o->viewport_height},
                              cui_cpu_v2_emit, s);
    }
}
static inline void cui_cpu_v2_shadow_paint(cui_cpu_v2_surface *surface,
                                           const cui_cpu_v2_shadow_descriptor *d) {
    const cui_render_v2_options *o = surface->options;
    cui_color color =
        d->inset ? d->profile.inset.paint.layer.color : d->profile.drop.paint.layer.color;
    for (unsigned y = 0; y < d->height; ++y)
        for (unsigned x = 0; x < d->width; ++x) {
            double l = d->q * ((double)d->x + x - o->origin_x) / o->scale_x + d->phase_x;
            double t = d->q * ((double)d->y + y - o->origin_y) / o->scale_y + d->phase_y;
            if (l >= d->support.right || l + d->dx <= d->support.left || t >= d->support.bottom ||
                t + d->dy <= d->support.top)
                continue;
            color.a = d->pixels[(size_t)y * d->width + x];
            cui_cpu_v2_emit(surface, d->x + (int)x, d->y + (int)y, color);
        }
}
_Static_assert(_Alignof(cui_cpu_v2_shadow_descriptor) <=
                   _Alignof(cui_fundamental_alignment_internal),
               "shadow descriptor exceeds workspace alignment");
typedef enum {
    CUI_CPU_V2_FRAME_EMPTY,
    CUI_CPU_V2_FRAME_PREPARING,
    CUI_CPU_V2_FRAME_READY,
    CUI_CPU_V2_FRAME_RELEASING
} cui_cpu_v2_frame_state;
typedef struct cui_cpu_v2_frame {
    cui_cpu_v2_frame_state state;
    const struct cui_cpu_v2_frame *self;
    cui_cpu_v2_workspace workspace;
    unsigned char *pixels;
    unsigned width, height;
    size_t stride, bytes;
#ifdef CUI_HAS_TEXT
    cui_text_owner_internal text_owner;
    cui_text_prepare_session text_session;
#endif
} cui_cpu_v2_frame;
typedef struct {
    unsigned width, height;
    cui_cpu_v2_exclusion excluded[3];
    size_t exclusion_count;
    bool *owner_busy;
} cui_cpu_v2_frame_request;
_Static_assert(sizeof(cui_cpu_v2_frame) <= 2048, "fixed CPU frame control exceeds guard");
/* Zero initialization is required. Init never retires a live/copy-owned frame. */
static inline void cui_cpu_v2_frame_init(cui_cpu_v2_frame *f) {
    if (f && f->state == CUI_CPU_V2_FRAME_EMPTY && (!f->self || f->self == f))
        *f = (cui_cpu_v2_frame){.self = f};
}
/* Internal cleanup owns PREPARING failure as well as READY release. Claim the
 * teardown state before any dependency/allocator callback can recurse. */
static inline void cui_cpu_v2_frame_dispose(cui_cpu_v2_frame *f) {
    f->state = CUI_CPU_V2_FRAME_RELEASING;
#ifdef CUI_HAS_TEXT
    cui_text_prepare_session_end(&f->text_session);
#endif
    cui_cpu_v2_workspace_end(&f->workspace);
    *f = (cui_cpu_v2_frame){.self = f};
}
static inline void cui_cpu_v2_frame_release(cui_cpu_v2_frame *f) {
    if (f && f->self == f && f->state == CUI_CPU_V2_FRAME_READY)
        cui_cpu_v2_frame_dispose(f);
}
static inline cui_error cui_cpu_v2_frame_prepare_internal(
    cui_cpu_v2_frame *frame, const cui_cpu_v2_frame_request *request, const cui_draw_list_v2 *list,
    cui_color clear, const cui_render_v2_options *options, cui_font *font,
    cui_cpu_v2_alloc_fn allocate, cui_cpu_v2_free_fn release, void *user, bool *secondary_busy) {
    if (!frame)
        return CUI_ERROR_INVALID_ACTION;
    if (frame->state != CUI_CPU_V2_FRAME_EMPTY || (frame->self && frame->self != frame))
        return CUI_ERROR_LIFECYCLE;
    if (!request || !list || !options || !allocate || !release || !request->width ||
        !request->height || request->width > 16384 || request->height > 16384 ||
        options->viewport_width != request->width || options->viewport_height != request->height ||
        request->exclusion_count > 3 ||
        !cui_background_prepare_disjoint_internal(frame, sizeof(*frame), request,
                                                  sizeof(*request)) ||
        (request->owner_busy &&
         (!cui_background_prepare_disjoint_internal(frame, sizeof(*frame), request->owner_busy,
                                                    sizeof(*request->owner_busy)) ||
          !cui_background_prepare_disjoint_internal(request, sizeof(*request), request->owner_busy,
                                                    sizeof(*request->owner_busy)))))
        return CUI_ERROR_INVALID_ACTION;
    if (secondary_busy && (!cui_background_prepare_disjoint_internal(
                               frame, sizeof(*frame), secondary_busy, sizeof(*secondary_busy)) ||
                           !cui_background_prepare_disjoint_internal(
                               request, sizeof(*request), secondary_busy, sizeof(*secondary_busy))))
        return CUI_ERROR_INVALID_ACTION;
    cui_cpu_v2_exclusion ranges[7] = {{frame, sizeof(*frame)}, {request, sizeof(*request)}};
    size_t range_count = 2;
    for (size_t i = 0; i < request->exclusion_count; ++i)
        ranges[range_count++] = request->excluded[i];
    if (request->owner_busy)
        ranges[range_count++] =
            (cui_cpu_v2_exclusion){request->owner_busy, sizeof(*request->owner_busy)};
    if (secondary_busy)
        ranges[range_count++] = (cui_cpu_v2_exclusion){secondary_busy, sizeof(*secondary_busy)};
    cui_cpu_v2_plan plan;
    bool font_available = false, tracking_available = false, context_available = false;
#ifdef CUI_HAS_TEXT
    font_available = font != NULL;
    if (font_available) {
        tracking_available = (cui_font_text_capabilities() & CUI_TEXT_CAP_LETTER_SPACING) != 0;
        context_available = cui_text_prepare_context_available_internal();
    }
#else
    (void)font;
#endif
    cui_error error = cui_cpu_v2_validate_ranges(list, options, font_available, tracking_available,
                                                 context_available, &plan, ranges, range_count);
    if (error != CUI_OK)
        return error;
    cui_cpu_v2_clip stack[64], clip = cui_cpu_v2_clip_all();
    size_t depth = 0, background_pixels = 0, background_bytes = 0;
    size_t shadows = 0, shadow_bytes = 0, shadow_taps = 0, texts = 0;
    for (size_t i = 0; i < list->count; ++i) {
        const cui_draw_command_v2 *v = &list->commands[i];
        if (v->kind == CUI_DRAW_V2_BACKGROUND) {
            cui_background_prepared_query q;
            error = cui_background_prepare_query(v->payload.background, options,
                                                 cui_cpu_v2_clip_cover(clip), &q);
            if (error != CUI_OK)
                return error;
            if (!cui_cpu_v2_add(&background_pixels, (size_t)q.width * q.height,
                                options->max_pixels) ||
                !cui_cpu_v2_add(&background_bytes, q.bytes, options->cpu_bytes))
                return CUI_ERROR_CAPACITY;
        } else {
            const cui_draw_command *c = &v->payload.legacy;
            if (c->type == CUI_DRAW_TEXT) {
                ++texts;
                if (!cui_cpu_v2_ranges_disjoint(font, 1, ranges, range_count))
                    return CUI_ERROR_INVALID_ACTION;
            }
            if (cui_shadow_type(c->type)) {
                cui_cpu_v2_shadow_descriptor d;
                error = cui_cpu_v2_shadow_query(
                    c->type, c->shadow, options,
                    (cui_cpu_v2_shadow_clip){clip.left, clip.top, clip.right, clip.bottom},
                    CUI_CPU_V2_SHADOW_CELLS - shadow_bytes, CUI_CPU_V2_SHADOW_TAPS - shadow_taps,
                    &d);
                if (error != CUI_OK)
                    return error;
                ++shadows;
                shadow_bytes += d.bytes;
                shadow_taps += d.taps;
            }
            if (c->type == CUI_DRAW_CLIP_PUSH) {
                stack[depth++] = clip;
                clip = cui_cpu_v2_clip_push(clip, c->bounds);
            } else if (c->type == CUI_DRAW_CLIP_POP)
                clip = stack[--depth];
        }
    }
    if (texts && !font_available)
        return CUI_ERROR_INVALID_ACTION;
    size_t commands_bytes = list->count * sizeof(*list->commands);
    size_t cpu_bytes = commands_bytes,
           metadata_bytes = list->count * sizeof(cui_cpu_v2_paint_descriptor);
    size_t shadow_metadata = shadows * sizeof(cui_cpu_v2_shadow_descriptor);
    size_t shadow_alignment = _Alignof(cui_cpu_v2_shadow_descriptor);
    size_t metadata_padding =
        shadows ? (shadow_alignment - metadata_bytes % shadow_alignment) % shadow_alignment : 0;
    if (!cui_cpu_v2_add(&metadata_bytes, metadata_padding, options->metadata_bytes) ||
        !cui_cpu_v2_add(&metadata_bytes, shadow_metadata, options->metadata_bytes) ||
        !cui_cpu_v2_add(&background_pixels, shadow_bytes, options->max_pixels) ||
        !cui_cpu_v2_add(&cpu_bytes, shadow_bytes, options->cpu_bytes) ||
        !cui_cpu_v2_add(&cpu_bytes, plan.scratch_bytes, options->cpu_bytes) ||
        !cui_cpu_v2_add(&cpu_bytes, background_bytes, options->cpu_bytes) ||
        background_bytes > options->texture_bytes || background_bytes > options->transfer_bytes)
        return CUI_ERROR_CAPACITY;
#ifdef CUI_HAS_TEXT
    size_t text_metadata = texts * sizeof(cui_cpu_v2_text_range);
    size_t text_alignment = _Alignof(cui_cpu_v2_text_range);
    size_t text_padding =
        texts ? (text_alignment - metadata_bytes % text_alignment) % text_alignment : 0;
    if (!cui_cpu_v2_add(&metadata_bytes, text_padding, options->metadata_bytes) ||
        !cui_cpu_v2_add(&metadata_bytes, text_metadata, options->metadata_bytes))
        return CUI_ERROR_CAPACITY;
    cui_text_owner_internal *text_owner = &frame->text_owner;
    cui_text_prepare_session *text_session = &frame->text_session;
#endif
    /* All preceding capability/profile/geometry/source checks are pure: no
     * allocator/provider callbacks. Guard the owner at the first callback boundary. */
    /* Guard ranges and all source extents have been validated before reading
     * either flag. A different EMPTY frame cannot recurse on the same owner. */
    if ((request->owner_busy && *request->owner_busy) || (secondary_busy && *secondary_busy))
        return CUI_ERROR_LIFECYCLE;
    frame->state = CUI_CPU_V2_FRAME_PREPARING;
    frame->self = frame;
    if (request->owner_busy)
        *request->owner_busy = true;
    if (secondary_busy)
        *secondary_busy = true;
#ifdef CUI_HAS_TEXT
    *text_owner =
        (cui_text_owner_internal){.allocate = allocate,
                                  .release = release,
                                  .user = user,
                                  .cpu_limit = options->cpu_bytes - cpu_bytes,
                                  .metadata_limit = options->metadata_bytes - metadata_bytes};
    *text_session = (cui_text_prepare_session){.owner = text_owner};
#endif
    cui_cpu_v2_workspace *workspace = &frame->workspace;
    error = cui_cpu_v2_workspace_reserve(workspace, cpu_bytes, metadata_bytes, allocate, release,
                                         user, true);
    if (error != CUI_OK) {
        cui_cpu_v2_frame_dispose(frame);
        return error;
    }
    cui_draw_command_v2 *commands =
        list->count ? cui_cpu_v2_workspace_take(workspace, false, commands_bytes,
                                                _Alignof(cui_draw_command_v2))
                    : NULL;
    cui_cpu_v2_paint_descriptor *descriptors =
        list->count ? cui_cpu_v2_workspace_take(workspace, true,
                                                list->count * sizeof(cui_cpu_v2_paint_descriptor),
                                                _Alignof(cui_cpu_v2_paint_descriptor))
                    : NULL;
    unsigned char *scratch = cui_cpu_v2_workspace_take(workspace, false, plan.scratch_bytes, 1);
    cui_cpu_v2_shadow_descriptor *shadow_descriptors =
        shadows ? cui_cpu_v2_workspace_take(workspace, true, shadow_metadata, shadow_alignment)
                : NULL;
#ifdef CUI_HAS_TEXT
    cui_cpu_v2_text_range *text_ranges =
        texts ? cui_cpu_v2_workspace_take(workspace, true, text_metadata, text_alignment) : NULL;
    if (texts && !text_ranges) {
        cui_cpu_v2_frame_dispose(frame);
        return CUI_ERROR_CAPACITY;
    }
    size_t text_index = 0;
#endif
    if ((list->count && (!commands || !descriptors)) || !scratch ||
        (shadows && !shadow_descriptors)) {
        cui_cpu_v2_frame_dispose(frame);
        return CUI_ERROR_CAPACITY;
    }
    if (list->count)
        memcpy(commands, list->commands, commands_bytes);
    clip = cui_cpu_v2_clip_all();
    depth = 0;
    size_t shadow_index = 0, prepared_shadow_bytes = 0, prepared_shadow_taps = 0;
    for (size_t i = 0; i < list->count && error == CUI_OK; ++i) {
        const cui_draw_command_v2 *v = &commands[i];
        descriptors[i] = (cui_cpu_v2_paint_descriptor){.clip = clip};
        if (v->kind == CUI_DRAW_V2_BACKGROUND) {
            cui_background_prepared_query q;
            error = cui_background_prepare_query(v->payload.background, options,
                                                 cui_cpu_v2_clip_cover(clip), &q);
            if (error != CUI_OK)
                break;
            unsigned char *pixels =
                q.bytes ? cui_cpu_v2_workspace_take(workspace, false, q.bytes, 1) : NULL;
            if (q.bytes && !pixels) {
                error = CUI_ERROR_CAPACITY;
                break;
            }
            error = cui_background_prepare_raster(v->payload.background, options,
                                                  cui_cpu_v2_clip_cover(clip), pixels, q.stride,
                                                  q.bytes, &descriptors[i].background);
            descriptors[i].pixels = pixels;
#ifdef CUI_HAS_TEXT
        } else if (v->kind == CUI_DRAW_TEXT) {
            cui_text_recorded_glyph_internal *previous = text_session->last;
            float q = fmaxf(1, fmaxf(options->scale_x, options->scale_y));
            if (!cui_text_prepare_line(text_session, font, &v->payload.legacy, q)) {
                error = text_owner->error == CUI_OK ? CUI_ERROR_INVALID_ACTION : text_owner->error;
                break;
            }
            cui_cpu_v2_text_range range = {previous ? previous->next : text_session->first,
                                           text_session->last};
            if (range.last == previous)
                range = (cui_cpu_v2_text_range){0};
            text_ranges[text_index++] = range;
            cui_cpu_v2_surface surface = {.options = options, .clip = clip};
            size_t count = 0;
            for (cui_text_recorded_glyph_internal *g = range.first; g;
                 g = g == range.last ? NULL : g->next) {
                cui_cpu_v2_glyph_cover cover;
                if (++count > 65536 || !cui_cpu_v2_glyph_query(g, &surface, &cover) ||
                    !cui_cpu_v2_add(&background_pixels,
                                    (size_t)(cover.x1 - cover.x0) * (size_t)(cover.y1 - cover.y0),
                                    options->max_pixels)) {
                    error = CUI_ERROR_CAPACITY;
                    break;
                }
            }
#endif
        } else if (cui_shadow_type((cui_draw_type)v->kind)) {
            cui_cpu_v2_shadow_descriptor *d = &shadow_descriptors[shadow_index++];
            error = cui_cpu_v2_shadow_query(
                (cui_draw_type)v->kind, v->payload.legacy.shadow, options,
                (cui_cpu_v2_shadow_clip){clip.left, clip.top, clip.right, clip.bottom},
                CUI_CPU_V2_SHADOW_CELLS - prepared_shadow_bytes,
                CUI_CPU_V2_SHADOW_TAPS - prepared_shadow_taps, d);
            if (error != CUI_OK)
                break;
            d->command_index = i;
            prepared_shadow_bytes += d->bytes;
            prepared_shadow_taps += d->taps;
            d->pixels = d->bytes ? cui_cpu_v2_workspace_take(workspace, false, d->bytes, 1) : NULL;
            if (d->bytes && !d->pixels) {
                error = CUI_ERROR_CAPACITY;
                break;
            }
            error = cui_cpu_v2_shadow_raster(d, options, d->pixels, d->bytes);
        } else if (v->kind == CUI_DRAW_CLIP_PUSH) {
            stack[depth++] = clip;
            clip = cui_cpu_v2_clip_push(clip, v->payload.legacy.bounds);
        } else if (v->kind == CUI_DRAW_CLIP_POP)
            clip = stack[--depth];
    }
    if (error == CUI_OK && (shadow_index != shadows || prepared_shadow_bytes != shadow_bytes ||
                            prepared_shadow_taps != shadow_taps))
        error = CUI_ERROR_INVALID_ACTION;
#ifdef CUI_HAS_TEXT
    if (error == CUI_OK && text_index != texts)
        error = CUI_ERROR_INVALID_ACTION;
#endif
    /* No pixel mutation, including scratch clear, precedes complete preparation. */
    if (error == CUI_OK) {
        for (size_t i = 0; i < plan.scratch_bytes; i += 4) {
            scratch[i] = clear.r;
            scratch[i + 1] = clear.g;
            scratch[i + 2] = clear.b;
            scratch[i + 3] = clear.a;
        }
        cui_cpu_v2_surface surface = {.pixels = scratch, .options = options};
        shadow_index = 0;
#ifdef CUI_HAS_TEXT
        text_index = 0;
#endif
        for (size_t i = 0; i < list->count; ++i) {
            const cui_draw_command_v2 *v = &commands[i];
            surface.clip = descriptors[i].clip;
            if (v->kind == CUI_DRAW_V2_BACKGROUND) {
                const cui_background_prepared_query *q = &descriptors[i].background;
                for (unsigned y = 0; y < q->height; ++y)
                    for (unsigned x = 0; x < q->width; ++x) {
                        double lx =
                            ((double)q->physical_x + x + .5 - options->origin_x) / options->scale_x;
                        double ly =
                            ((double)q->physical_y + y + .5 - options->origin_y) / options->scale_y;
                        /* Occupancy skip only: never paint outside shape. Covered bytes
                         * already include radius and opacity; blend exactly once. */
                        if (!cui_background_prepare_inside_internal(v->payload.background, lx, ly))
                            continue;
                        const unsigned char *p =
                            descriptors[i].pixels + (size_t)y * q->stride + x * 4;
                        cui_cpu_v2_emit(&surface, q->physical_x + (int)x, q->physical_y + (int)y,
                                        (cui_color){p[0], p[1], p[2], p[3]});
                    }
            }
#ifdef CUI_HAS_TEXT
            else if (v->kind == CUI_DRAW_TEXT)
                cui_cpu_v2_text_paint(&surface, text_ranges[text_index++]);
#endif
            else if (cui_shadow_type((cui_draw_type)v->kind))
                cui_cpu_v2_shadow_paint(&surface, &shadow_descriptors[shadow_index++]);
            else if (v->kind == CUI_DRAW_MESH)
                cui_cpu_v2_mesh_paint(&surface, &v->payload.legacy.mesh);
            else if (v->kind == CUI_DRAW_RECT || v->kind == CUI_DRAW_BORDER ||
                     v->kind == CUI_DRAW_IMAGE)
                cui_cpu_v2_shape_paint(&surface, &v->payload.legacy);
        }
        frame->pixels = scratch;
        frame->width = request->width;
        frame->height = request->height;
        frame->stride = (size_t)request->width * 4;
        frame->bytes = plan.scratch_bytes;
        frame->state = CUI_CPU_V2_FRAME_READY;
    } else
        cui_cpu_v2_frame_dispose(frame);
    return error;
}
/* Address-stable private ownership handle; all borrowed resources/font graphs
 * remain valid through release. EMPTY/READY state is never caller-writable. */
static inline cui_error cui_cpu_v2_frame_prepare(cui_cpu_v2_frame *frame,
                                                 const cui_cpu_v2_frame_request *request,
                                                 const cui_draw_list_v2 *list, cui_color clear,
                                                 const cui_render_v2_options *options,
                                                 cui_font *font, cui_cpu_v2_alloc_fn allocate,
                                                 cui_cpu_v2_free_fn release, void *user) {
    return cui_cpu_v2_frame_prepare_internal(frame, request, list, clear, options, font, allocate,
                                             release, user, NULL);
}
static inline cui_error
cui_cpu_v2_paint_with_font_excluding(cui_cpu_v2_target *target, const cui_draw_list_v2 *list,
                                     cui_color clear, const cui_render_v2_options *options,
                                     cui_font *font, cui_cpu_v2_alloc_fn allocate,
                                     cui_cpu_v2_free_fn release, void *user, const void *excluded,
                                     size_t excluded_bytes, bool *owner_busy) {
    if (!target || target->busy || !list || !options || !allocate || !release || !target->pixels ||
        !target->width || !target->height || target->width > 16384 || target->height > 16384 ||
        options->viewport_width != target->width || options->viewport_height != target->height)
        return CUI_ERROR_INVALID_ACTION;
    size_t row = (size_t)target->width * 4;
    if (target->stride < row || target->height - 1 > (SIZE_MAX - row) / target->stride)
        return CUI_ERROR_CAPACITY;
    size_t span = (target->height - 1) * target->stride + row;
    if (span > target->capacity)
        return CUI_ERROR_CAPACITY;
    if (!cui_background_prepare_disjoint_internal(target->pixels, span, target, sizeof(*target)) ||
        !cui_cpu_v2_target_disjoint(target, span, options, sizeof(*options)) ||
        !cui_cpu_v2_target_disjoint(target, span, list, sizeof(*list)))
        return CUI_ERROR_INVALID_ACTION;
    cui_cpu_v2_frame frame = {0};
    cui_cpu_v2_frame_request request = {
        .width = target->width,
        .height = target->height,
        .excluded = {{excluded, excluded_bytes}, {target->pixels, span}, {target, sizeof(*target)}},
        .exclusion_count = 3,
        .owner_busy = owner_busy};
    /* Both guards are claimed only after all pure source preflight. */
    cui_error error = cui_cpu_v2_frame_prepare_internal(
        &frame, &request, list, clear, options, font, allocate, release, user, &target->busy);
    if (error == CUI_OK) {
        for (unsigned y = 0; y < target->height; ++y)
            memcpy(target->pixels + y * target->stride, frame.pixels + (size_t)y * row, row);
        cui_cpu_v2_frame_release(&frame);
    }
    target->busy = false;
    return error;
}
/* Legacy private callers have no additional enclosing owner range/guard. The
 * initialized adapter supplies both; it clears owner_busy after all cleanup. */
static inline cui_error cui_cpu_v2_paint_with_font(cui_cpu_v2_target *target,
                                                   const cui_draw_list_v2 *list, cui_color clear,
                                                   const cui_render_v2_options *options,
                                                   cui_font *font, cui_cpu_v2_alloc_fn allocate,
                                                   cui_cpu_v2_free_fn release, void *user) {
    return cui_cpu_v2_paint_with_font_excluding(target, list, clear, options, font, allocate,
                                                release, user, NULL, 0, NULL);
}
/* No-font wrapper retains preallocation TEXT refusal and exact nontext requests. */
static inline cui_error cui_cpu_v2_paint(cui_cpu_v2_target *target, const cui_draw_list_v2 *list,
                                         cui_color clear, const cui_render_v2_options *options,
                                         cui_cpu_v2_alloc_fn allocate, cui_cpu_v2_free_fn release,
                                         void *user) {
    return cui_cpu_v2_paint_with_font(target, list, clear, options, NULL, allocate, release, user);
}
#endif
