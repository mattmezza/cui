/* SPDX-License-Identifier: MIT */
#ifndef CUI_SOFTWARE_V2_PAINT_INTERNAL_H
#define CUI_SOFTWARE_V2_PAINT_INTERNAL_H
#include "cui_mesh.h"
#include "cui_shape.h"
#include "cui_software_v2_internal.h"
/* Private incomplete painter: TEXT rejects atomically; shadows use bounded source-profile AREA
 * coverage. Not wired to a V1 renderer, public V2 entry, or capability advertisement. Inputs/target
 * view stay stable through return, including allocator callbacks. Allocators obey the workspace
 * distinct-live-block/alignment contract. Target pixels and input records/resources must be
 * disjoint. */
typedef struct {
    unsigned char *pixels;
    unsigned width, height;
    size_t stride, capacity;
    bool busy;
} cui_software_v2_target;
typedef struct {
    double left, top, right, bottom;
} cui_software_v2_clip;
typedef struct {
    cui_software_v2_clip clip;
    cui_background_prepared_query background;
    unsigned char *pixels;
} cui_software_v2_paint_descriptor;
_Static_assert(_Alignof(cui_software_v2_paint_descriptor) <=
                   _Alignof(cui_fundamental_alignment_internal),
               "paint descriptor exceeds workspace alignment");
static inline cui_software_v2_clip cui_software_v2_clip_all(void) {
    return (cui_software_v2_clip){-FLT_MAX / 4, -FLT_MAX / 4, FLT_MAX / 4, FLT_MAX / 4};
}
static inline cui_software_v2_clip cui_software_v2_clip_push(cui_software_v2_clip a, cui_rect b) {
    return (cui_software_v2_clip){fmax(a.left, b.x), fmax(a.top, b.y),
                                  fmin(a.right, (double)b.x + b.w),
                                  fmin(a.bottom, (double)b.y + b.h)};
}
/* Outward float cover is culling only. Exact double center tests use clip below. */
static inline cui_rect cui_software_v2_clip_cover(cui_software_v2_clip c) {
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
static inline bool cui_software_v2_target_disjoint(const cui_software_v2_target *t, size_t span,
                                                   const void *p, size_t bytes) {
    return cui_background_prepare_disjoint_internal(t->pixels, span, p, bytes) &&
           cui_background_prepare_disjoint_internal(t, sizeof(*t), p, bytes);
}
static inline bool cui_software_v2_pixel_inside(cui_software_v2_clip c, double x, double y) {
    return x >= c.left && x < c.right && y >= c.top && y < c.bottom;
}
/* Same straight-byte source-over arithmetic as immutable V1, including alpha0. */
static inline void cui_software_v2_blend(unsigned char *p, cui_color c) {
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
    cui_software_v2_clip clip;
} cui_software_v2_surface;
static inline void cui_software_v2_emit(void *user, int x, int y, cui_color color) {
    cui_software_v2_surface *s = user;
    const cui_render_v2_options *o = s->options;
    double lx = (x + .5 - o->origin_x) / o->scale_x;
    double ly = (y + .5 - o->origin_y) / o->scale_y;
    if (cui_software_v2_pixel_inside(s->clip, lx, ly))
        cui_software_v2_blend(s->pixels + ((size_t)y * o->viewport_width + (unsigned)x) * 4, color);
}
/* Intersect in double/device space before any integer cast, even for huge input. */
static inline bool cui_software_v2_cover(cui_rect bounds, cui_software_v2_surface *s, int *x0,
                                         int *y0, int *x1, int *y1) {
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
/* Original absolute edges stay separate from distances to the near edge.
 * In particular (x+w)-right_inset preserves a small far-edge inset when x+w
 * cancels to a small value; x+(w-right_inset) would lose it. */
static inline double cui_software_v2_arc(double rx, double ry, double distance) {
    if (rx <= 0 || ry <= 0 || distance >= ry)
        return 0;
    double delta = ry - distance;
    /* Rationalized circular/elliptical arc avoids losing a small edge inset
     * to 1-sqrt(...), and preserves exact 3-4-5 circle boundary cases. */
    double root = sqrt(fmax(0, ry * ry - delta * delta));
    return fmin(rx, (rx / ry) * (delta * delta) / (ry + root));
}
static inline bool cui_software_v2_span(const cui_draw_command *c, const float r[4],
                                        const float e[4], bool inner, double y, double *left,
                                        double *right) {
    double l = inner ? e[0] : 0, rr = inner ? e[1] : 0;
    double t = inner ? e[2] : 0, b = inner ? e[3] : 0;
    double near_x = (double)c->bounds.x + l;
    double far_x = ((double)c->bounds.x + c->bounds.w) - rr;
    double near_y = (double)c->bounds.y + t;
    double far_y = ((double)c->bounds.y + c->bounds.h) - b;
    if (near_x >= far_x || near_y >= far_y || y < near_y || y >= far_y)
        return false;
    double lt = cui_software_v2_arc(fmax(0, r[0] - l), fmax(0, r[0] - t), y - near_y);
    double lb = cui_software_v2_arc(fmax(0, r[2] - l), fmax(0, r[2] - b), far_y - y);
    double rt = cui_software_v2_arc(fmax(0, r[1] - rr), fmax(0, r[1] - t), y - near_y);
    double rb = cui_software_v2_arc(fmax(0, r[3] - rr), fmax(0, r[3] - b), far_y - y);
    *left = near_x + fmax(lt, lb);
    *right = far_x - fmax(rt, rb);
    return *left < *right;
}
/* Error-free addition retains which side of an integer a small endpoint
 * residual occupies. Never round an inside source coordinate to its open end. */
static inline double cui_software_v2_floor_sum(double a, double b) {
    double sum = a + b, z = sum - a;
    double error = (a - (sum - z)) + (b - z);
    double index = floor(sum);
    return error < 0 && sum == index ? index - 1 : index;
}
/* Compute UV from the closer absolute edge; compensate the far source sum so
 * tiny source offsets/widths are not mistaken for an open integer endpoint. */
static inline unsigned cui_software_v2_texel(double point, double near, double far, double source,
                                             double extent, unsigned size) {
    double distance_near = point - near, distance_far = far - point;
    double index;
    if (distance_near <= distance_far)
        index = cui_software_v2_floor_sum(source, distance_near / (far - near) * extent);
    else {
        double end = source + extent, z = end - source;
        double residual = (source - (end - z)) + (extent - z);
        double delta = distance_far / (far - near) * extent;
        index = cui_software_v2_floor_sum(end, residual - delta);
    }
    return (unsigned)fmax(0, fmin((double)size - 1, index));
}
static inline void cui_software_v2_shape_paint(cui_software_v2_surface *s,
                                               const cui_draw_command *c) {
    int x0, y0, x1, y1;
    if (!cui_software_v2_cover(c->bounds, s, &x0, &y0, &x1, &y1))
        return;
    float radii[4], edges[4];
    cui_shape_values(c, radii, edges);
    const cui_render_v2_options *o = s->options;
    for (int y = y0; y < y1; ++y) {
        double ly = (y + .5 - o->origin_y) / o->scale_y;
        double left, right, il = 0, ir = 0;
        if (!cui_software_v2_span(c, radii, edges, false, ly, &left, &right))
            continue;
        bool inner =
            c->type == CUI_DRAW_BORDER && cui_software_v2_span(c, radii, edges, true, ly, &il, &ir);
        for (int x = x0; x < x1; ++x) {
            double lx = (x + .5 - o->origin_x) / o->scale_x;
            if (lx < left || lx >= right || (inner && lx >= il && lx < ir))
                continue;
            cui_color color = c->color;
            if (c->type == CUI_DRAW_IMAGE) {
                const cui_bitmap_paint *p = &c->bitmap;
                unsigned sx =
                    cui_software_v2_texel(lx, c->bounds.x, (double)c->bounds.x + c->bounds.w,
                                          p->source.x, p->source.w, p->bitmap.width);
                unsigned sy =
                    cui_software_v2_texel(ly, c->bounds.y, (double)c->bounds.y + c->bounds.h,
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
            cui_software_v2_emit(s, x, y, color);
        }
    }
}
static inline void cui_software_v2_mesh_paint(cui_software_v2_surface *s, const cui_mesh_paint *p) {
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
                              cui_software_v2_emit, s);
    }
}
static inline void cui_software_v2_shadow_paint(cui_software_v2_surface *surface,
                                                const cui_software_v2_shadow_descriptor *d) {
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
            cui_software_v2_emit(surface, d->x + (int)x, d->y + (int)y, color);
        }
}
_Static_assert(_Alignof(cui_software_v2_shadow_descriptor) <=
                   _Alignof(cui_fundamental_alignment_internal),
               "shadow descriptor exceeds workspace alignment");
static inline cui_error cui_software_v2_paint(cui_software_v2_target *target,
                                              const cui_draw_list_v2 *list, cui_color clear,
                                              const cui_render_v2_options *options,
                                              cui_software_v2_alloc_fn allocate,
                                              cui_software_v2_free_fn release, void *user) {
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
        !cui_software_v2_target_disjoint(target, span, options, sizeof(*options)) ||
        !cui_software_v2_target_disjoint(target, span, list, sizeof(*list)))
        return CUI_ERROR_INVALID_ACTION;
    cui_software_v2_plan plan;
    cui_error error = cui_software_v2_validate(list, options, false, false, &plan);
    if (error != CUI_OK)
        return error;
    if (!cui_software_v2_target_disjoint(target, span, list->commands,
                                         list->count * sizeof(*list->commands)))
        return CUI_ERROR_INVALID_ACTION;
    cui_software_v2_clip stack[64], clip = cui_software_v2_clip_all();
    size_t depth = 0, background_pixels = 0, background_bytes = 0;
    size_t shadows = 0, shadow_bytes = 0, shadow_taps = 0;
    for (size_t i = 0; i < list->count; ++i) {
        const cui_draw_command_v2 *v = &list->commands[i];
        if (v->kind == CUI_DRAW_V2_BACKGROUND) {
            cui_background_prepared_query q;
            error = cui_background_prepare_query(v->payload.background, options,
                                                 cui_software_v2_clip_cover(clip), &q);
            if (error != CUI_OK)
                return error;
            if (!cui_software_v2_add(&background_pixels, (size_t)q.width * q.height,
                                     options->max_pixels) ||
                !cui_software_v2_add(&background_bytes, q.bytes, options->cpu_bytes))
                return CUI_ERROR_CAPACITY;
        } else {
            const cui_draw_command *c = &v->payload.legacy;
            if (c->type == CUI_DRAW_TEXT)
                return CUI_ERROR_INVALID_ACTION;
            if (cui_shadow_type(c->type)) {
                cui_software_v2_shadow_descriptor d;
                error = cui_software_v2_shadow_query(
                    c->type, c->shadow, options,
                    (cui_software_v2_shadow_clip){clip.left, clip.top, clip.right, clip.bottom},
                    CUI_SOFTWARE_V2_SHADOW_CELLS - shadow_bytes,
                    CUI_SOFTWARE_V2_SHADOW_TAPS - shadow_taps, &d);
                if (error != CUI_OK)
                    return error;
                ++shadows;
                shadow_bytes += d.bytes;
                shadow_taps += d.taps;
            }
            if (c->type == CUI_DRAW_CLIP_PUSH) {
                stack[depth++] = clip;
                clip = cui_software_v2_clip_push(clip, c->bounds);
            } else if (c->type == CUI_DRAW_CLIP_POP)
                clip = stack[--depth];
            else if (c->type == CUI_DRAW_IMAGE &&
                     !cui_software_v2_target_disjoint(target, span, c->bitmap.bitmap.pixels,
                                                      c->bitmap.bitmap.stride *
                                                          c->bitmap.bitmap.height))
                return CUI_ERROR_INVALID_ACTION;
            else if (c->type == CUI_DRAW_MESH &&
                     !cui_software_v2_target_disjoint(target, span, c->mesh.vertices,
                                                      c->mesh.vertex_count *
                                                          sizeof(*c->mesh.vertices)))
                return CUI_ERROR_INVALID_ACTION;
        }
    }
    size_t commands_bytes = list->count * sizeof(*list->commands);
    size_t cpu_bytes = commands_bytes,
           metadata_bytes = list->count * sizeof(cui_software_v2_paint_descriptor);
    size_t shadow_metadata = shadows * sizeof(cui_software_v2_shadow_descriptor);
    size_t shadow_alignment = _Alignof(cui_software_v2_shadow_descriptor);
    size_t metadata_padding =
        shadows ? (shadow_alignment - metadata_bytes % shadow_alignment) % shadow_alignment : 0;
    if (!cui_software_v2_add(&metadata_bytes, metadata_padding, options->metadata_bytes) ||
        !cui_software_v2_add(&metadata_bytes, shadow_metadata, options->metadata_bytes) ||
        !cui_software_v2_add(&background_pixels, shadow_bytes, options->max_pixels) ||
        !cui_software_v2_add(&cpu_bytes, shadow_bytes, options->cpu_bytes) ||
        !cui_software_v2_add(&cpu_bytes, plan.scratch_bytes, options->cpu_bytes) ||
        !cui_software_v2_add(&cpu_bytes, background_bytes, options->cpu_bytes) ||
        background_bytes > options->texture_bytes || background_bytes > options->transfer_bytes)
        return CUI_ERROR_CAPACITY;
    target->busy = true;
    cui_software_v2_workspace workspace = {0};
    error = cui_software_v2_workspace_begin(&workspace, cpu_bytes, metadata_bytes, allocate,
                                            release, user);
    if (error != CUI_OK) {
        target->busy = false;
        return error;
    }
    cui_draw_command_v2 *commands =
        list->count ? cui_software_v2_workspace_take(&workspace, false, commands_bytes,
                                                     _Alignof(cui_draw_command_v2))
                    : NULL;
    cui_software_v2_paint_descriptor *descriptors =
        list->count
            ? cui_software_v2_workspace_take(&workspace, true,
                                             list->count * sizeof(cui_software_v2_paint_descriptor),
                                             _Alignof(cui_software_v2_paint_descriptor))
            : NULL;
    unsigned char *scratch =
        cui_software_v2_workspace_take(&workspace, false, plan.scratch_bytes, 1);
    cui_software_v2_shadow_descriptor *shadow_descriptors =
        shadows
            ? cui_software_v2_workspace_take(&workspace, true, shadow_metadata, shadow_alignment)
            : NULL;
    if ((list->count && (!commands || !descriptors)) || !scratch ||
        (shadows && !shadow_descriptors)) {
        cui_software_v2_workspace_end(&workspace);
        target->busy = false;
        return CUI_ERROR_CAPACITY;
    }
    if (list->count)
        memcpy(commands, list->commands, commands_bytes);
    clip = cui_software_v2_clip_all();
    depth = 0;
    size_t shadow_index = 0, prepared_shadow_bytes = 0, prepared_shadow_taps = 0;
    for (size_t i = 0; i < list->count && error == CUI_OK; ++i) {
        const cui_draw_command_v2 *v = &commands[i];
        descriptors[i] = (cui_software_v2_paint_descriptor){.clip = clip};
        if (v->kind == CUI_DRAW_V2_BACKGROUND) {
            cui_background_prepared_query q;
            error = cui_background_prepare_query(v->payload.background, options,
                                                 cui_software_v2_clip_cover(clip), &q);
            if (error != CUI_OK)
                break;
            unsigned char *pixels =
                q.bytes ? cui_software_v2_workspace_take(&workspace, false, q.bytes, 1) : NULL;
            if (q.bytes && !pixels) {
                error = CUI_ERROR_CAPACITY;
                break;
            }
            error = cui_background_prepare_raster(v->payload.background, options,
                                                  cui_software_v2_clip_cover(clip), pixels,
                                                  q.stride, q.bytes, &descriptors[i].background);
            descriptors[i].pixels = pixels;
        } else if (cui_shadow_type((cui_draw_type)v->kind)) {
            cui_software_v2_shadow_descriptor *d = &shadow_descriptors[shadow_index++];
            error = cui_software_v2_shadow_query(
                (cui_draw_type)v->kind, v->payload.legacy.shadow, options,
                (cui_software_v2_shadow_clip){clip.left, clip.top, clip.right, clip.bottom},
                CUI_SOFTWARE_V2_SHADOW_CELLS - prepared_shadow_bytes,
                CUI_SOFTWARE_V2_SHADOW_TAPS - prepared_shadow_taps, d);
            if (error != CUI_OK)
                break;
            d->command_index = i;
            prepared_shadow_bytes += d->bytes;
            prepared_shadow_taps += d->taps;
            d->pixels =
                d->bytes ? cui_software_v2_workspace_take(&workspace, false, d->bytes, 1) : NULL;
            if (d->bytes && !d->pixels) {
                error = CUI_ERROR_CAPACITY;
                break;
            }
            error = cui_software_v2_shadow_raster(d, options, d->pixels, d->bytes);
        } else if (v->kind == CUI_DRAW_CLIP_PUSH) {
            stack[depth++] = clip;
            clip = cui_software_v2_clip_push(clip, v->payload.legacy.bounds);
        } else if (v->kind == CUI_DRAW_CLIP_POP)
            clip = stack[--depth];
    }
    if (error == CUI_OK && (shadow_index != shadows || prepared_shadow_bytes != shadow_bytes ||
                            prepared_shadow_taps != shadow_taps))
        error = CUI_ERROR_INVALID_ACTION;
    /* No pixel mutation, including scratch clear, precedes complete preparation. */
    if (error == CUI_OK) {
        for (size_t i = 0; i < plan.scratch_bytes; i += 4) {
            scratch[i] = clear.r;
            scratch[i + 1] = clear.g;
            scratch[i + 2] = clear.b;
            scratch[i + 3] = clear.a;
        }
        cui_software_v2_surface surface = {.pixels = scratch, .options = options};
        shadow_index = 0;
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
                        cui_software_v2_emit(&surface, q->physical_x + (int)x,
                                             q->physical_y + (int)y,
                                             (cui_color){p[0], p[1], p[2], p[3]});
                    }
            } else if (cui_shadow_type((cui_draw_type)v->kind))
                cui_software_v2_shadow_paint(&surface, &shadow_descriptors[shadow_index++]);
            else if (v->kind == CUI_DRAW_MESH)
                cui_software_v2_mesh_paint(&surface, &v->payload.legacy.mesh);
            else if (v->kind == CUI_DRAW_RECT || v->kind == CUI_DRAW_BORDER ||
                     v->kind == CUI_DRAW_IMAGE)
                cui_software_v2_shape_paint(&surface, &v->payload.legacy);
        }
        for (unsigned y = 0; y < target->height; ++y)
            memcpy(target->pixels + y * target->stride, scratch + (size_t)y * row, row);
    }
    cui_software_v2_workspace_end(&workspace);
    target->busy = false;
    return error;
}
#endif
