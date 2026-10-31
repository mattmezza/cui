/* SPDX-License-Identifier: MIT */
#include "../../backends/software/cui_software_v2_paint_internal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
bool background_v1_sample(const cui_background *, cui_rect, float, float, cui_background_pixel *);
bool software_v2_original_reference(const cui_draw_list *, cui_color, unsigned, unsigned,
                                    unsigned char *);
enum { WIDTH = 16, HEIGHT = 12, STRIDE = WIDTH * 4 + 7, CAPACITY = HEIGHT * STRIDE + 19 };
static cui_render_v2_options options(void) {
    return (cui_render_v2_options){.size = sizeof(cui_render_v2_options),
                                   .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1,
                                   .viewport_width = WIDTH,
                                   .viewport_height = HEIGHT,
                                   .scale_x = 1,
                                   .scale_y = 1,
                                   .cpu_bytes = 32 * 1024 * 1024,
                                   .metadata_bytes = 1024 * 1024,
                                   .texture_bytes = 32 * 1024 * 1024,
                                   .transfer_bytes = 32 * 1024 * 1024,
                                   .max_pixels = 8 * 1024 * 1024,
                                   .max_pixels_per_background = 4 * 1024 * 1024};
}
static cui_draw_list_v2 list(cui_draw_command_v2 *c, size_t n) {
    return (cui_draw_list_v2){.header_size = sizeof(cui_draw_list_v2),
                              .abi_version = CUI_DRAW_ABI_V2,
                              .command_stride = sizeof(*c),
                              .commands = c,
                              .count = n,
                              .width = WIDTH,
                              .height = HEIGHT,
                              .required_features = CUI_DRAW_V2_FEATURE_BACKGROUNDS,
                              .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1};
}
static cui_draw_command_v2 legacy(cui_draw_type kind, cui_rect bounds, cui_color color) {
    return (cui_draw_command_v2){
        .kind = (uint32_t)kind, .payload.legacy = {.type = kind, .bounds = bounds, .color = color}};
}
typedef struct {
    size_t calls, frees, live, peak, bytes, fail, requested[2];
    cui_software_v2_target *target;
    const cui_draw_list_v2 *list;
    const cui_render_v2_options *options;
    unsigned char before[CAPACITY];
    bool check_reentrant;
} allocator;
static void *allocate(void *user, size_t n);
static void deallocate(void *user, void *p) {
    allocator *a = user;
    assert(a->target->busy);
    if (a->check_reentrant)
        assert(cui_software_v2_paint(a->target, a->list, (cui_color){0}, a->options, allocate,
                                     deallocate, a) == CUI_ERROR_INVALID_ACTION);
    a->live--;
    a->frees++;
    free(p);
}
static void *allocate(void *user, size_t n) {
    allocator *a = user;
    assert(a->target->busy);
    assert(memcmp(a->target->pixels, a->before, CAPACITY) == 0);
    if (a->check_reentrant)
        assert(cui_software_v2_paint(a->target, a->list, (cui_color){0}, a->options, allocate,
                                     deallocate, a) == CUI_ERROR_INVALID_ACTION);
    a->calls++;
    if (a->calls <= 2)
        a->requested[a->calls - 1] = n;
    if (a->fail == a->calls)
        return NULL;
    a->bytes += n;
    a->peak = a->bytes;
    a->live++;
    return malloc(n);
}
static cui_error paint(cui_software_v2_target *t, const cui_draw_list_v2 *l,
                       const cui_render_v2_options *o, cui_color clear, allocator *a) {
    a->target = t;
    a->list = l;
    a->options = o;
    memcpy(a->before, t->pixels, CAPACITY);
    cui_error error = cui_software_v2_paint(t, l, clear, o, allocate, deallocate, a);
    assert(!t->busy && !a->live);
    if (error != CUI_OK)
        assert(memcmp(a->before, t->pixels, CAPACITY) == 0);
    return error;
}
/* Independent scalar oracle: whole physical viewport, logical center/clip tests,
 * circle/ellipse containment, frozen pre-helper color sampler, integer source-over. */
static unsigned char byte(double x) {
    return (unsigned char)floor(fmax(0, fmin(1, x)) * 255 + .5);
}
static unsigned char encoded(double x) {
    x = fmax(0, fmin(1, x));
    return byte(x <= .0031308 ? 12.92 * x : 1.055 * pow(x, 1. / 2.4) - .055);
}
static void over(unsigned char *d, cui_color c) {
    uint64_t coverage = (uint64_t)c.a * 255 + (uint64_t)d[3] * (255 - c.a);
    unsigned source[3] = {c.r, c.g, c.b};
    for (unsigned i = 0; i < 3; ++i)
        d[i] = coverage ? (unsigned char)(((uint64_t)source[i] * c.a * 255 +
                                           (uint64_t)d[i] * d[3] * (255 - c.a) + coverage / 2) /
                                          coverage)
                        : 0;
    d[3] = (unsigned char)((coverage + 127) / 255);
}
static bool ellipse_shape(double x, double y, double width, double height, const double rx[4],
                          const double ry[4]) {
    if (x < 0 || y < 0 || x >= width || y >= height)
        return false;
    for (unsigned i = 0; i < 4; ++i) {
        double cx = i & 1 ? width - rx[i] : rx[i];
        double cy = i & 2 ? height - ry[i] : ry[i];
        if (rx[i] > 0 && ry[i] > 0 && (i & 1 ? x > cx : x < cx) && (i & 2 ? y > cy : y < cy)) {
            double dx = (x - cx) / rx[i], dy = (y - cy) / ry[i];
            if (dx * dx + dy * dy > 1)
                return false;
        }
    }
    return true;
}
static void radii(cui_corner_radii r, double width, double height, double out[4]) {
    double v[4] = {r.top_left, r.top_right, r.bottom_left, r.bottom_right};
    for (unsigned i = 0; i < 4; ++i)
        out[i] = fmin(fmax(v[i], 0), fmin(width, height) / 2);
}
static bool shape(const cui_draw_command *c, double x, double y) {
    double r[4];
    cui_corner_radii corners = c->custom_corners
                                   ? c->corner_radii
                                   : (cui_corner_radii){c->radius, c->radius, c->radius, c->radius};
    radii(corners, c->bounds.w, c->bounds.h, r);
    x -= c->bounds.x;
    y -= c->bounds.y;
    if (!ellipse_shape(x, y, c->bounds.w, c->bounds.h, r, r))
        return false;
    if (c->type != CUI_DRAW_BORDER)
        return true;
    cui_border_edges e = c->custom_border_edges
                             ? c->border_edges
                             : (cui_border_edges){c->stroke, c->stroke, c->stroke, c->stroke};
    double l = fmax(e.left, 0), rr = fmax(e.right, 0), t = fmax(e.top, 0), b = fmax(e.bottom, 0);
    double rx[4] = {fmax(0, r[0] - l), fmax(0, r[1] - rr), fmax(0, r[2] - l), fmax(0, r[3] - rr)};
    double ry[4] = {fmax(0, r[0] - t), fmax(0, r[1] - t), fmax(0, r[2] - b), fmax(0, r[3] - b)};
    return !ellipse_shape(x - l, y - t, c->bounds.w - l - rr, c->bounds.h - t - b, rx, ry);
}
static cui_color bg(cui_background_draw_v2 b, double x, double y) {
    cui_background_pixel p;
    assert(background_v1_sample(&b.brush, b.bounds, (float)x, (float)y, &p));
    cui_color c = {0};
    if (p.a > 0 && b.opacity > 0)
        c = (cui_color){encoded((double)p.r / p.a), encoded((double)p.g / p.a),
                        encoded((double)p.b / p.a), byte((double)p.a * b.opacity)};
    return c;
}
static void mesh_reference(unsigned char *pixel, const cui_mesh_paint *m,
                           const cui_render_v2_options *o, double x, double y) {
    for (size_t k = 0; k < m->vertex_count; k += 3) {
        double px[3], py[3], ss[3], tt[3];
        for (unsigned i = 0; i < 3; ++i) {
            const cui_mesh_vertex *v = &m->vertices[k + i];
            px[i] = (m->origin_x + (double)m->scale * v->x) * o->scale_x + o->origin_x;
            py[i] = (m->origin_y + (double)m->scale * v->y) * o->scale_y + o->origin_y;
            ss[i] = v->s;
            tt[i] = v->t;
        }
        double determinant = (px[1] - px[0]) * (py[2] - py[0]) - (py[1] - py[0]) * (px[2] - px[0]);
        if (!determinant)
            continue;
        if (determinant < 0) {
            double z = px[1];
            px[1] = px[2];
            px[2] = z;
            z = py[1];
            py[1] = py[2];
            py[2] = z;
            z = ss[1];
            ss[1] = ss[2];
            ss[2] = z;
            z = tt[1];
            tt[1] = tt[2];
            tt[2] = z;
            determinant = -determinant;
        }
        bool inside = true;
        for (unsigned i = 0; i < 3; ++i) {
            unsigned j = (i + 1) % 3;
            double edge = (px[j] - px[i]) * (y - py[i]) - (py[j] - py[i]) * (x - px[i]);
            if (edge < 0 || (edge == 0 && !(py[j] < py[i] || (py[j] == py[i] && px[j] > px[i]))))
                inside = false;
        }
        if (!inside)
            continue;
        double dsx =
            ((ss[1] - ss[0]) * (py[2] - py[0]) - (ss[2] - ss[0]) * (py[1] - py[0])) / determinant;
        double dsy =
            ((px[1] - px[0]) * (ss[2] - ss[0]) - (px[2] - px[0]) * (ss[1] - ss[0])) / determinant;
        double dtx =
            ((tt[1] - tt[0]) * (py[2] - py[0]) - (tt[2] - tt[0]) * (py[1] - py[0])) / determinant;
        double dty =
            ((px[1] - px[0]) * (tt[2] - tt[0]) - (px[2] - px[0]) * (tt[1] - tt[0])) / determinant;
        double coverage = 1;
        if (hypot(dsx, dsy) >= .001) {
            double s = ss[0] + dsx * (x - px[0]) + dsy * (y - py[0]);
            double t = tt[0] + dtx * (x - px[0]) + dty * (y - py[0]);
            double f = s * s - t, derivative = hypot(2 * s * dsx - dtx, 2 * s * dsy - dty);
            coverage = derivative > 0 ? fmax(0, fmin(1, .5 - f / derivative))
                       : f < 0        ? 1
                       : f > 0        ? 0
                                      : .5;
        }
        cui_color c = {encoded(m->color.a > 0 ? m->color.r / m->color.a : 0),
                       encoded(m->color.a > 0 ? m->color.g / m->color.a : 0),
                       encoded(m->color.a > 0 ? m->color.b / m->color.a : 0),
                       byte(m->color.a * coverage)};
        over(pixel, c);
    }
}
static void reference(const cui_draw_list_v2 *l, const cui_render_v2_options *o, cui_color clear,
                      unsigned char *pixels) {
    for (unsigned py = 0; py < HEIGHT; ++py)
        for (unsigned px = 0; px < WIDTH; ++px) {
            unsigned char *p = pixels + ((size_t)py * WIDTH + px) * 4;
            p[0] = clear.r;
            p[1] = clear.g;
            p[2] = clear.b;
            p[3] = clear.a;
            double x = (px + .5 - o->origin_x) / o->scale_x,
                   y = (py + .5 - o->origin_y) / o->scale_y;
            double clips[64][4], clip[4] = {-1e100, -1e100, 1e100, 1e100};
            size_t depth = 0;
            for (size_t i = 0; i < l->count; ++i) {
                const cui_draw_command_v2 *v = &l->commands[i];
                if (v->kind == CUI_DRAW_CLIP_PUSH) {
                    memcpy(clips[depth++], clip, sizeof clip);
                    cui_rect b = v->payload.legacy.bounds;
                    clip[0] = fmax(clip[0], b.x);
                    clip[1] = fmax(clip[1], b.y);
                    clip[2] = fmin(clip[2], (double)b.x + b.w);
                    clip[3] = fmin(clip[3], (double)b.y + b.h);
                    continue;
                }
                if (v->kind == CUI_DRAW_CLIP_POP) {
                    memcpy(clip, clips[--depth], sizeof clip);
                    continue;
                }
                if (x < clip[0] || y < clip[1] || x >= clip[2] || y >= clip[3])
                    continue;
                if (v->kind == CUI_DRAW_V2_BACKGROUND) {
                    cui_background_draw_v2 b = v->payload.background;
                    double r[4];
                    radii(b.radii, b.bounds.w, b.bounds.h, r);
                    if (ellipse_shape(x - b.bounds.x, y - b.bounds.y, b.bounds.w, b.bounds.h, r, r))
                        over(p, bg(b, x, y));
                    continue;
                }
                const cui_draw_command *c = &v->payload.legacy;
                if (c->type == CUI_DRAW_MESH) {
                    mesh_reference(p, &c->mesh, o, px + .5, py + .5);
                    continue;
                }
                if (!shape(c, x, y))
                    continue;
                cui_color color = c->color;
                if (c->type == CUI_DRAW_IMAGE) {
                    const cui_bitmap_paint *b = &c->bitmap;
                    unsigned sx = (unsigned)fmin(
                        b->bitmap.width - 1,
                        floor(b->source.x + (x - c->bounds.x) / c->bounds.w * b->source.w));
                    unsigned sy = (unsigned)fmin(
                        b->bitmap.height - 1,
                        floor(b->source.y + (y - c->bounds.y) / c->bounds.h * b->source.h));
                    const unsigned char *q =
                        b->bitmap.pixels + (size_t)sy * b->bitmap.stride + sx * 4;
                    color =
                        (cui_color){q[0], q[1], q[2], (unsigned char)lroundf(q[3] * b->opacity)};
                    if (b->grayscale)
                        color.r = color.g = color.b = (unsigned char)lroundf(
                            .2126f * color.r + .7152f * color.g + .0722f * color.b);
                    if (b->custom_tint) {
                        color.r = (unsigned char)((unsigned)color.r * b->tint.r / 255);
                        color.g = (unsigned char)((unsigned)color.g * b->tint.g / 255);
                        color.b = (unsigned char)((unsigned)color.b * b->tint.b / 255);
                        color.a = (unsigned char)((unsigned)color.a * b->tint.a / 255);
                    }
                }
                over(p, color);
            }
        }
}
static unsigned char image[2 * 12] = {255, 0,  70,  255, 20,  255, 40, 128, 99, 99, 99, 99,
                                      30,  60, 255, 0,   120, 240, 10, 210, 99, 99, 99, 99};
static cui_mesh_vertex vertices[6] = {{0, 0, 0, 0},  {4, 0, 0, 0}, {0, 4, 0, 0},
                                      {1, 1, -1, 1}, {5, 1, 1, 1}, {3, 5, 0, -1}};
static size_t fixture(cui_draw_command_v2 *c) {
    c[0] = legacy(CUI_DRAW_RECT, (cui_rect){-.25f, .25f, 9, 7}, (cui_color){30, 90, 220, 100});
    c[0].payload.legacy.radius = 2;
    c[1] = legacy(CUI_DRAW_CLIP_PUSH, (cui_rect){1.125f, .375f, 5.5f, 5.75f}, (cui_color){0});
    c[2] = (cui_draw_command_v2){.kind = CUI_DRAW_V2_BACKGROUND,
                                 .payload.background = {.bounds = {.25f, .25f, 8, 6},
                                                        .radii = {1.25f, .5f, 2, .25f},
                                                        .opacity = .625f}};
    assert(cui_background_make_linear(
        CUI_BACKGROUND_CSS, CUI_BACKGROUND_SRGB, 90,
        (cui_background_stop){.position = 0, .color = {.8f, .1f, .2f, .25f}},
        (cui_background_stop){.position = 1, .color = {.05f, .7f, .4f, .8f}},
        &c[2].payload.background.brush));
    c[3] = legacy(CUI_DRAW_IMAGE, (cui_rect){1.25f, .5f, 4.5f, 4.25f}, (cui_color){0});
    c[3].payload.legacy.radius = .75f;
    c[3].payload.legacy.bitmap =
        (cui_bitmap_paint){.bitmap = {.pixels = image, .width = 2, .height = 2, .stride = 12},
                           .source = {0, 0, 2, 2},
                           .opacity = .7f,
                           .grayscale = true,
                           .custom_tint = true,
                           .tint = {220, 130, 255, 180}};
    c[4] =
        legacy(CUI_DRAW_BORDER, (cui_rect){.5f, 1.25f, 6.5f, 4.5f}, (cui_color){245, 40, 80, 190});
    c[4].payload.legacy.custom_corners = true;
    c[4].payload.legacy.corner_radii = (cui_corner_radii){1.5f, 2, .25f, .75f};
    c[4].payload.legacy.custom_border_edges = true;
    c[4].payload.legacy.border_edges = (cui_border_edges){.5f, 1, .75f, .25f};
    c[5] = legacy(CUI_DRAW_MESH, (cui_rect){NAN, NAN, NAN, NAN}, (cui_color){0});
    c[5].payload.legacy.mesh = (cui_mesh_paint){.vertices = vertices,
                                                .vertex_count = 6,
                                                .origin_x = 1.25f,
                                                .origin_y = .5f,
                                                .scale = .75f,
                                                .color = {.1f, .2f, .05f, .5f}};
    c[6] = legacy(CUI_DRAW_CLIP_PUSH, (cui_rect){2.5f, 2.25f, 1.25f, 2.5f}, (cui_color){0});
    c[7] = legacy(CUI_DRAW_BORDER, (cui_rect){2.25f, 2, 2, 3}, (cui_color){90, 200, 50, 90});
    c[7].payload.legacy.stroke = .5f;
    c[7].payload.legacy.radius = .25f;
    c[8] = legacy(CUI_DRAW_CLIP_POP, (cui_rect){NAN, NAN, NAN, NAN}, (cui_color){0});
    c[9] = c[8];
    c[10] = legacy(CUI_DRAW_RECT, (cui_rect){8.25f, 1, 2.75f, 3.5f}, (cui_color){220, 50, 90, 70});
    return 11;
}
static void equal_pixels(const unsigned char *actual, const unsigned char *expected) {
    for (unsigned y = 0; y < HEIGHT; ++y) {
        for (unsigned x = 0; x < WIDTH * 4; ++x)
            if (actual[(size_t)y * STRIDE + x] != expected[(size_t)y * WIDTH * 4 + x]) {
                fprintf(stderr, "pixel mismatch y=%u byte=%u actual=%u expected=%u\n", y, x,
                        actual[(size_t)y * STRIDE + x], expected[(size_t)y * WIDTH * 4 + x]);
                abort();
            }
        for (unsigned x = WIDTH * 4; x < STRIDE; ++x)
            assert(actual[(size_t)y * STRIDE + x] == 0xa7);
    }
    for (size_t i = HEIGHT * STRIDE; i < CAPACITY; ++i)
        assert(actual[i] == 0xa7);
}
static void pixels(void) {
    cui_draw_command_v2 c[12];
    cui_draw_list_v2 l = list(c, fixture(c));
    unsigned char actual[CAPACITY], expected[WIDTH * HEIGHT * 4];
    const float scales[][4] = {{1, 1, 0, 0},
                               {1.5f, .75f, .375f, -.25f},
                               {.5f, 2.25f, -.625f, .875f},
                               {64, 1.f / 65536, -120, .5f - 1.f / 131072},
                               {1.f / 65536, 64, .5f - 1.f / 131072, -120}};
    for (unsigned model = 0; model < 2; ++model)
        for (unsigned space = 0; space < 2; ++space)
            for (unsigned i = 0; i < sizeof(scales) / sizeof(scales[0]); ++i)
                for (unsigned transparent = 0; transparent < 2; ++transparent) {
                    c[2].payload.background.brush.model = (cui_background_model)model;
                    c[2].payload.background.brush.space = (cui_background_space)space;
                    c[2].payload.background.brush.angle_degrees = model ? 223 : 37;
                    c[2].payload.background.brush.stops[0].position = model ? .8f : 0;
                    c[2].payload.background.brush.stops[1].position = model ? .2f : 1;
                    c[2].payload.background.brush.stops[0].color.a = transparent ? 0 : .25f;
                    c[3].payload.legacy.bitmap.grayscale = space != 0;
                    c[3].payload.legacy.bitmap.custom_tint = model != 0;
                    cui_render_v2_options o = options();
                    o.scale_x = scales[i][0];
                    o.scale_y = scales[i][1];
                    o.origin_x = scales[i][2];
                    o.origin_y = scales[i][3];
                    cui_color clear = {9, 17, 25, transparent ? 0 : 70};
                    memset(actual, 0xa7, sizeof actual);
                    cui_software_v2_target t = {actual, WIDTH, HEIGHT, STRIDE, CAPACITY, false};
                    allocator a = {.check_reentrant = true};
                    assert(paint(&t, &l, &o, clear, &a) == CUI_OK);
                    assert(a.calls == 2 && a.frees == 2 &&
                           a.peak <= o.cpu_bytes + o.metadata_bytes);
                    reference(&l, &o, clear, expected);
                    equal_pixels(actual, expected);
                }
    /* Exact original renderer bytes for ordinary-only unit-scale commands. */
    fixture(c);
    memmove(c + 2, c + 3, 9 * sizeof(*c));
    l.count = 10;
    cui_draw_command old[12];
    for (size_t i = 0; i < l.count; ++i) {
        old[i] = c[i].payload.legacy;
        /* V1 validates inactive bounds too; canonicalize only those fields. */
        if (old[i].type == CUI_DRAW_MESH || old[i].type == CUI_DRAW_CLIP_POP)
            old[i].bounds = (cui_rect){0};
    }
    cui_draw_list oldlist = {.commands = old, .count = l.count, .width = WIDTH, .height = HEIGHT};
    assert(software_v2_original_reference(&oldlist, (cui_color){9, 17, 25, 0}, WIDTH, HEIGHT,
                                          expected));
    cui_render_v2_options o = options();
    memset(actual, 0xa7, sizeof actual);
    cui_software_v2_target t = {actual, WIDTH, HEIGHT, STRIDE, CAPACITY, false};
    allocator a = {0};
    assert(paint(&t, &l, &o, (cui_color){9, 17, 25, 0}, &a) == CUI_OK);
    equal_pixels(actual, expected);
    /* Outside corners preserve hidden clear RGB; inside transparent fill canonicalizes. */
    c[0] = (cui_draw_command_v2){.kind = CUI_DRAW_V2_BACKGROUND,
                                 .payload.background = {.bounds = {1, 1, 6, 6},
                                                        .radii = {3, 3, 3, 3},
                                                        .opacity = 1,
                                                        .brush = {.kind = CUI_BACKGROUND_SOLID}}};
    l = list(c, 1);
    memset(actual, 0xa7, sizeof actual);
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){9, 17, 25, 0}, &a) == CUI_OK);
    assert(actual[STRIDE + 4] == 9 && actual[3 * STRIDE + 3 * 4] == 0);
    reference(&l, &o, (cui_color){9, 17, 25, 0}, expected);
    equal_pixels(actual, expected);
}
static void edge_pixels(void) {
    cui_draw_command_v2 c[4];
    c[0] = legacy(CUI_DRAW_CLIP_PUSH, (cui_rect){.1f, .25f, .2f, 4}, (cui_color){0});
    c[1] = (cui_draw_command_v2){.kind = CUI_DRAW_V2_BACKGROUND,
                                 .payload.background = {.bounds = {-999999, .25f, 1000000, 6},
                                                        .opacity = .00001f,
                                                        .brush = {.kind = CUI_BACKGROUND_SOLID,
                                                                  .solid = {.8f, .2f, .1f, .5f}}}};
    c[2] = legacy(CUI_DRAW_CLIP_POP, (cui_rect){0}, (cui_color){0});
    cui_draw_list_v2 l = list(c, 3);
    cui_render_v2_options o = options();
    o.origin_x = .2f;
    unsigned char actual[CAPACITY], expected[WIDTH * HEIGHT * 4];
    memset(actual, 0xa7, sizeof actual);
    cui_software_v2_target t = {actual, WIDTH, HEIGHT, STRIDE, CAPACITY, false};
    allocator a = {0};
    cui_color clear = {99, 77, 55, 0};
    assert(paint(&t, &l, &o, clear, &a) == CUI_OK);
    reference(&l, &o, clear, expected);
    equal_pixels(actual, expected);
    /* Whole shape can lie outside the sampler point domain when clipped empty. */
    c[0].payload.legacy.bounds = (cui_rect){0, 0, 0, 0};
    c[1].payload.background.bounds = (cui_rect){1000000, 0, 1000000, 8};
    o.origin_x = -1000000;
    a = (allocator){0};
    assert(paint(&t, &l, &o, clear, &a) == CUI_OK);
    reference(&l, &o, clear, expected);
    equal_pixels(actual, expected);
    /* Without the empty clip, covered centers exceed the sampler domain. */
    l = list(c + 1, 1);
    a = (allocator){0};
    assert(paint(&t, &l, &o, clear, &a) == CUI_ERROR_INVALID_ACTION && !a.calls);
}
static void failures(void) {
    cui_draw_command_v2 c[12];
    cui_draw_list_v2 l = list(c, fixture(c));
    cui_render_v2_options o = options();
    unsigned char actual[CAPACITY];
    memset(actual, 0xa7, sizeof actual);
    cui_software_v2_target t = {actual, WIDTH, HEIGHT, STRIDE, CAPACITY, false};
    allocator a = {0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_OK);
    size_t cpu = a.requested[0], metadata = a.requested[1];
    for (size_t fail = 1; fail <= 2; ++fail) {
        a = (allocator){.fail = fail};
        assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_CAPACITY);
        a = (allocator){0};
        assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_OK);
    }
    o.cpu_bytes = cpu - 1;
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_CAPACITY && !a.calls);
    o = options();
    o.metadata_bytes = metadata - 1;
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_CAPACITY && !a.calls);
    o = options();
    o.cpu_bytes = cpu;
    o.metadata_bytes = metadata;
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_OK);
    assert(a.requested[0] == cpu && a.requested[1] == metadata);
    o = options();
    o.texture_bytes = 1;
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_CAPACITY && !a.calls);
    o = options();
    o.transfer_bytes = 1;
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_CAPACITY && !a.calls);
    o = options();
    for (unsigned k = 0; k < 5; ++k) {
        fixture(c);
        if (k == 0)
            c[10].kind = UINT32_MAX;
        else if (k == 1) {
            c[10] = legacy(CUI_DRAW_IMAGE, (cui_rect){1, 1, 3, 3}, (cui_color){0});
        } else if (k == 2)
            c[10] = legacy(CUI_DRAW_TEXT, (cui_rect){0}, (cui_color){0});
        else if (k == 3) {
            c[10] = legacy(CUI_DRAW_DROP_SHADOW, (cui_rect){0}, (cui_color){0});
            c[10].payload.legacy.shadow =
                (cui_drop_shadow_paint){.caster = {1, 1, 2, 2}, .layer = {.color = {0, 0, 0, 100}}};
        } else {
            c[10] = legacy(CUI_DRAW_INSET_SHADOW, (cui_rect){0}, (cui_color){0});
            c[10].payload.legacy.shadow =
                (cui_drop_shadow_paint){.caster = {1, 1, 2, 2}, .layer = {.color = {0, 0, 0, 100}}};
        }
        a = (allocator){0};
        assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_INVALID_ACTION && !a.calls);
    }
    fixture(c);
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_OK);
    t.stride = WIDTH * 4 - 1;
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_CAPACITY && !a.calls);
    t.stride = SIZE_MAX;
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_CAPACITY && !a.calls);
    t.stride = STRIDE;
    t.capacity = (HEIGHT - 1) * STRIDE + WIDTH * 4 - 1;
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_CAPACITY && !a.calls);
    t.capacity = CAPACITY;
    o.viewport_width--;
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_INVALID_ACTION && !a.calls);
    o = options();
    c[3].payload.legacy.bitmap.bitmap.pixels = actual;
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_INVALID_ACTION && !a.calls);
    union {
        cui_fundamental_alignment_internal alignment;
        cui_mesh_vertex mesh[CAPACITY / sizeof(cui_mesh_vertex) + 1];
        unsigned char bytes[CAPACITY];
    } alias;
    memset(&alias, 0xa7, sizeof(alias));
    memcpy(alias.mesh, vertices, 3 * sizeof(*vertices));
    fixture(c);
    c[5].payload.legacy.mesh.vertices = alias.mesh;
    c[5].payload.legacy.mesh.vertex_count = 3;
    t.pixels = alias.bytes;
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){0}, &a) == CUI_ERROR_INVALID_ACTION && !a.calls);
    t.pixels = actual;
    fixture(c);
    l = list(c, 0);
    a = (allocator){0};
    assert(paint(&t, &l, &o, (cui_color){80, 90, 100, 0}, &a) == CUI_OK && a.calls == 1);
}
int main(void) {
    pixels();
    edge_pixels();
    failures();
    return 0;
}
