/* SPDX-License-Identifier: MIT */
#include "../../src/core/cui_background_prepare_internal.h"
#include <assert.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
bool background_v1_sample(const cui_background *, cui_rect, float, float, cui_background_pixel *);
bool background_v1_sample_srgba8(const cui_background *, cui_rect, float, float, cui_color *);
static cui_render_v2_options options(void) {
    return (cui_render_v2_options){.size = sizeof(cui_render_v2_options),
                                   .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1,
                                   .viewport_width = 32,
                                   .viewport_height = 24,
                                   .scale_x = 1,
                                   .scale_y = 1,
                                   .cpu_bytes = 32 * 1024 * 1024,
                                   .texture_bytes = 32 * 1024 * 1024,
                                   .transfer_bytes = 32 * 1024 * 1024,
                                   .metadata_bytes = 1024 * 1024,
                                   .max_pixels = 8 * 1024 * 1024,
                                   .max_pixels_per_background = 4 * 1024 * 1024};
}
static cui_background_draw_v2 solid(void) {
    return (cui_background_draw_v2){
        .bounds = {1, 2, 8, 6},
        .brush = {.kind = CUI_BACKGROUND_SOLID, .solid = {1, .25f, .5f, 1}},
        .opacity = 1};
}
static unsigned char byte(double x) {
    if (x < 0)
        x = 0;
    if (x > 1)
        x = 1;
    return (unsigned char)floor(x * 255 + .5);
}
static double encode(double x) {
    if (x < 0)
        x = 0;
    if (x > 1)
        x = 1;
    return x <= .0031308 ? 12.92 * x : 1.055 * pow(x, 1. / 2.4) - .055;
}
static cui_color reference_color(cui_background_draw_v2 draw, float x, float y) {
    cui_background_pixel p;
    assert(background_v1_sample(&draw.brush, draw.bounds, x, y, &p));
    double alpha = (double)p.a * draw.opacity;
    if (alpha == 0)
        return (cui_color){0};
    return (cui_color){byte(encode((double)p.r / p.a)), byte(encode((double)p.g / p.a)),
                       byte(encode((double)p.b / p.a)), byte(alpha)};
}
/* Independent long-double corner-distance reference, not the production mask. */
static bool reference_inside(cui_background_draw_v2 d, long double x, long double y) {
    long double l = d.bounds.x, t = d.bounds.y, r = l + d.bounds.w, b = t + d.bounds.h;
    if (x < l || y < t || x >= r || y >= b)
        return false;
    float values[4] = {d.radii.top_left, d.radii.top_right, d.radii.bottom_left,
                       d.radii.bottom_right};
    long double limit = fminl(d.bounds.w, d.bounds.h) / 2;
    for (unsigned i = 0; i < 4; i++) {
        long double radius = fminl(fmaxl(values[i], 0), limit);
        long double cx = (i & 1) ? r - radius : l + radius;
        long double cy = (i & 2) ? b - radius : t + radius;
        if (radius > 0 && ((i & 1) ? x > cx : x < cx) && ((i & 2) ? y > cy : y < cy) &&
            hypotl(x - cx, y - cy) > radius)
            return false;
    }
    return true;
}
static bool same_query(cui_background_prepared_query a, cui_background_prepared_query b) {
    return a.physical_x == b.physical_x && a.physical_y == b.physical_y && a.width == b.width &&
           a.height == b.height && a.stride == b.stride && a.bytes == b.bytes &&
           a.logical_covered.x == b.logical_covered.x &&
           a.logical_covered.y == b.logical_covered.y &&
           a.logical_covered.w == b.logical_covered.w && a.logical_covered.h == b.logical_covered.h;
}
static unsigned cases;
static void compare(cui_background_draw_v2 d, cui_render_v2_options o, cui_rect clip) {
    cui_background_prepared_query q;
    assert(cui_background_prepare_query(d, &o, clip, &q) == CUI_OK);
    double left = fmax(0, fmax((double)d.bounds.x * o.scale_x + o.origin_x,
                               (double)clip.x * o.scale_x + o.origin_x));
    double top = fmax(0, fmax((double)d.bounds.y * o.scale_y + o.origin_y,
                              (double)clip.y * o.scale_y + o.origin_y));
    double right =
        fmin(o.viewport_width, fmin(((double)d.bounds.x + d.bounds.w) * o.scale_x + o.origin_x,
                                    ((double)clip.x + clip.w) * o.scale_x + o.origin_x));
    double bottom =
        fmin(o.viewport_height, fmin(((double)d.bounds.y + d.bounds.h) * o.scale_y + o.origin_y,
                                     ((double)clip.y + clip.h) * o.scale_y + o.origin_y));
    if (right <= left || bottom <= top || !d.bounds.w || !d.bounds.h || !clip.w || !clip.h ||
        !o.viewport_width || !o.viewport_height) {
        cui_background_prepared_query zero = {0};
        assert(same_query(q, zero));
        assert(cui_background_prepare_raster(d, &o, clip, NULL, 0, 0, &q) == CUI_OK);
        return;
    }
    int px = (int)floor(left), py = (int)floor(top);
    unsigned width = (unsigned)ceil(right) - px, height = (unsigned)ceil(bottom) - py;
    assert(q.physical_x == px && q.physical_y == py && q.width == width && q.height == height);
    assert(q.stride == (size_t)width * 4 && q.bytes == (size_t)width * height * 4);
    assert(q.logical_covered.x == (float)((px - (double)o.origin_x) / o.scale_x));
    assert(q.logical_covered.y == (float)((py - (double)o.origin_y) / o.scale_y));
    size_t stride = q.stride + 7, capacity = stride * height;
    unsigned char *pixels = malloc(capacity);
    assert(pixels);
    memset(pixels, 0xa7, capacity);
    cui_background_prepared_query actual;
    assert(cui_background_prepare_raster(d, &o, clip, pixels, stride, capacity, &actual) == CUI_OK);
    assert(actual.stride == stride && actual.bytes == (height - 1) * stride + q.stride);
    actual.stride = q.stride;
    actual.bytes = q.bytes;
    assert(same_query(q, actual));
    for (unsigned row = 0; row < height; row++) {
        for (unsigned column = 0; column < width; column++) {
            long double x = ((long double)px + column + .5L - o.origin_x) / o.scale_x;
            long double y = ((long double)py + row + .5L - o.origin_y) / o.scale_y;
            cui_color expected =
                reference_inside(d, x, y) ? reference_color(d, (float)x, (float)y) : (cui_color){0};
            const unsigned char *at = pixels + (size_t)row * stride + column * 4;
            if (at[0] != expected.r || at[1] != expected.g || at[2] != expected.b ||
                at[3] != expected.a) {
                fprintf(stderr,
                        "prepared reference mismatch case%u xy%u,%u got%u,%u,%u,%u "
                        "expected%u,%u,%u,%u\n",
                        cases, column, row, at[0], at[1], at[2], at[3], expected.r, expected.g,
                        expected.b, expected.a);
                assert(false);
            }
        }
        for (size_t i = q.stride; i < stride; i++)
            assert(pixels[(size_t)row * stride + i] == 0xa7);
    }
    free(pixels);
    cases++;
}
static void grids_and_models(void) {
    const cui_rect clip = {-.25f, .75f, 21.5f, 16.25f};
    const float scales[] = {.5f, 1, 1.25f, 2};
    const float angles[] = {0, 31, 90, 180, 270, 360};
    for (unsigned model = 0; model < 2; model++)
        for (unsigned space = 0; space < 2; space++)
            for (unsigned scale = 0; scale < 4; scale++)
                for (unsigned angle = 0; angle < 6; angle++) {
                    cui_background_draw_v2 d = solid();
                    d.bounds = (cui_rect){-.375f, 1.25f, 18.75f, 13.25f};
                    d.radii = (cui_corner_radii){3.25f, 100, -1, 2.5f};
                    d.opacity = angle % 2 ? .6f : 1;
                    assert(cui_background_make_linear(
                        (cui_background_model)model, (cui_background_space)space, angles[angle],
                        (cui_background_stop){{1, .125f, 0, 0}, model ? .8f : .15f},
                        (cui_background_stop){{.1f, .2f, 1, .875f}, model ? .2f : .85f}, &d.brush));
                    cui_render_v2_options o = options();
                    o.scale_x = scales[scale];
                    o.scale_y = scales[(scale + 1) % 4];
                    o.origin_x = .375f;
                    o.origin_y = -.25f;
                    compare(d, o, clip);
                }
    cui_background_draw_v2 d = solid();
    cui_render_v2_options o = options();
    compare(d, o, (cui_rect){0, 0, 32, 24});
    compare(d, o, (cui_rect){-FLT_MAX / 2, -FLT_MAX / 2, FLT_MAX, FLT_MAX});
    d.opacity = 0;
    compare(d, o, (cui_rect){0, 0, 32, 24});
    d.brush.solid = (cui_background_color){1, 0, 0, 0};
    d.opacity = 1;
    compare(d, o, (cui_rect){0, 0, 32, 24});
    d = solid();
    d.radii = (cui_corner_radii){4, 4, 4, 4};
    d.bounds = (cui_rect){0, 0, 8, 8};
    compare(d, o, (cui_rect){0, 0, 32, 24});
}
static void old_sampler_bytes(void) {
    for (unsigned model = 0; model < 2; model++)
        for (unsigned space = 0; space < 2; space++)
            for (unsigned angle = 0; angle <= 360; angle += 15)
                for (unsigned alpha = 0; alpha < 5; alpha++) {
                    cui_background d;
                    assert(cui_background_make_linear(
                        (cui_background_model)model, (cui_background_space)space, (float)angle,
                        (cui_background_stop){{1, .025f, .7f, alpha * .25f}, .2f},
                        (cui_background_stop){{.1f, .8f, 0, (4 - alpha) * .25f}, .8f}, &d));
                    for (unsigned i = 0; i < 17; i++) {
                        float x = -2 + i * .875f, y = 1 + i * .375f;
                        cui_color old, current;
                        assert(background_v1_sample_srgba8(&d, (cui_rect){-1, .25f, 11, 7}, x, y,
                                                           &old));
                        assert(cui_background_sample_srgba8(&d, (cui_rect){-1, .25f, 11, 7}, x, y,
                                                            &current));
                        assert(memcmp(&old, &current, sizeof(old)) == 0);
                    }
                }
}
static void atomic_error(cui_background_draw_v2 d, cui_render_v2_options o, cui_rect clip,
                         cui_error expected) {
    unsigned char bytes[1024], before[1024];
    memset(bytes, 0x6d, sizeof(bytes));
    memcpy(before, bytes, sizeof(bytes));
    cui_background_prepared_query q, prior;
    memset(&q, 0x35, sizeof(q));
    memcpy(&prior, &q, sizeof(q));
    assert(cui_background_prepare_query(d, &o, clip, &q) == expected);
    assert(memcmp(&q, &prior, sizeof(q)) == 0);
    assert(cui_background_prepare_raster(d, &o, clip, bytes, 64, sizeof(bytes), &q) == expected);
    assert(memcmp(bytes, before, sizeof(bytes)) == 0 && memcmp(&q, &prior, sizeof(q)) == 0);
}
static void errors(void) {
    cui_background_draw_v2 d = solid(), bad;
    cui_render_v2_options o = options(), invalid;
    cui_rect clip = {0, 0, 32, 24};
    for (unsigned i = 0; i < 15; i++) {
        invalid = o;
        bad = d;
        switch (i) {
        case 0:
            invalid.size--;
            break;
        case 1:
            invalid.background_output = 2;
            break;
        case 2:
            invalid.scale_x = 0;
            break;
        case 3:
            invalid.scale_y = NAN;
            break;
        case 4:
            invalid.scale_x = nextafterf(64, INFINITY);
            break;
        case 5:
            invalid.origin_x = INFINITY;
            break;
        case 6:
            bad.brush.solid.r = NAN;
            break;
        case 7:
            bad.bounds.w = -1;
            break;
        case 8:
            bad.bounds.x = nextafterf(1e6f, INFINITY);
            break;
        case 9:
            bad.bounds.h = nextafterf(1e6f, INFINITY);
            break;
        case 10:
            bad.radii.top_left = INFINITY;
            break;
        case 11:
            bad.opacity = -.1f;
            break;
        case 12:
            bad.opacity = NAN;
            break;
        case 13:
            invalid.cpu_bytes = 0;
            break;
        case 14:
            invalid.scale_y = nextafterf(1.f / 65536, 0);
            break;
        }
        atomic_error(bad, invalid, clip, CUI_ERROR_INVALID_ACTION);
    }
    for (unsigned i = 0; i < 8; i++) {
        invalid = o;
        switch (i) {
        case 0:
            invalid.viewport_width = 16385;
            break;
        case 1:
            invalid.max_pixels = 47;
            break;
        case 2:
            invalid.max_pixels_per_background = 47;
            break;
        case 3:
            invalid.cpu_bytes = 191;
            break;
        case 4:
            invalid.texture_bytes = 191;
            break;
        case 5:
            invalid.transfer_bytes = 191;
            break;
        case 6:
            invalid.metadata_bytes = sizeof(cui_background_prepared_query) - 1;
            break;
        case 7:
            invalid.texture_bytes = 32 * 1024 * 1024 + 1;
            break;
        }
        atomic_error(d, invalid, clip, CUI_ERROR_CAPACITY);
    }
    atomic_error(d, o, (cui_rect){0, 0, NAN, 4}, CUI_ERROR_INVALID_ACTION);
    unsigned char bytes[1024], before[1024];
    memset(bytes, 0xb2, sizeof(bytes));
    memcpy(before, bytes, sizeof(bytes));
    cui_background_prepared_query q, prior;
    memset(&q, 0x35, sizeof(q));
    memcpy(&prior, &q, sizeof(q));
    assert(cui_background_prepare_raster(d, &o, clip, bytes, 31, sizeof(bytes), &q) ==
           CUI_ERROR_CAPACITY);
    assert(cui_background_prepare_raster(d, &o, clip, bytes, SIZE_MAX, sizeof(bytes), &q) ==
           CUI_ERROR_CAPACITY);
    assert(cui_background_prepare_raster(d, &o, clip, bytes, 40, 231, &q) == CUI_ERROR_CAPACITY);
    o.cpu_bytes = 231;
    assert(cui_background_prepare_raster(d, &o, clip, bytes, 40, sizeof(bytes), &q) ==
           CUI_ERROR_CAPACITY);
    assert(memcmp(bytes, before, sizeof(bytes)) == 0 && memcmp(&q, &prior, sizeof(q)) == 0);
    o = options();
    assert(cui_background_prepare_raster(d, &o, clip, bytes, 40, sizeof(bytes), &q) == CUI_OK);
    assert(cui_background_prepare_raster(d, &o, clip, NULL, 32, 192, &q) ==
           CUI_ERROR_INVALID_ACTION);
    union {
        cui_render_v2_options options;
        cui_background_prepared_query query;
        unsigned char bytes[1024];
    } overlap;
    memset(overlap.bytes, 0x79, sizeof(overlap));
    overlap.options = o;
    unsigned char overlap_before[sizeof(overlap)];
    memcpy(overlap_before, &overlap, sizeof(overlap));
    assert(cui_background_prepare_query(d, &overlap.options, clip, &overlap.query) ==
           CUI_ERROR_INVALID_ACTION);
    assert(cui_background_prepare_raster(d, &o, clip, overlap.bytes, 32, sizeof(overlap),
                                         &overlap.query) == CUI_ERROR_INVALID_ACTION);
    assert(cui_background_prepare_raster(d, &overlap.options, clip, overlap.bytes, 32,
                                         sizeof(overlap), &q) == CUI_ERROR_INVALID_ACTION);
    assert(memcmp(overlap_before, &overlap, sizeof(overlap)) == 0);
}
static void work_limits_and_density(void) {
    cui_background_draw_v2 d = solid();
    cui_render_v2_options o = options();
    o.viewport_width = o.viewport_height = 4096;
    d.bounds = (cui_rect){0, 0, 2048, 2048};
    cui_rect clip = {0, 0, 4096, 4096};
    cui_background_prepared_query q;
    assert(cui_background_prepare_query(d, &o, clip, &q) == CUI_OK);
    assert(q.bytes == (size_t)16 * 1024 * 1024);
    d.bounds.w = 2049;
    atomic_error(d, o, clip, CUI_ERROR_CAPACITY);
    d = solid();
    d.bounds = (cui_rect){0, 0, .5f, .25f};
    d.radii = (cui_corner_radii){.125f, .125f, .05f, .05f};
    o = options();
    o.scale_x = o.scale_y = 64;
    compare(d, o, (cui_rect){0, 0, 32, 24});
    d.bounds = (cui_rect){-1000000, -1000000, 10, 10};
    o = options();
    o.origin_x = o.origin_y = 1000000;
    compare(d, o, d.bounds);
}
static void phase_domain_empty_and_alpha(void) {
    cui_render_v2_options o = options();
    cui_background_draw_v2 d = solid();
    cui_rect clip = {0, 0, 32, 24};
    cui_background_prepared_query q;
    d.bounds = (cui_rect){900000, 900000, 200000, 200000};
    o.scale_x = o.scale_y = 1.f / 65536;
    o.origin_x = o.origin_y = -13.5f;
    for (unsigned model = 0; model < 2; model++)
        for (unsigned space = 0; space < 2; space++) {
            assert(cui_background_make_linear((cui_background_model)model,
                                              (cui_background_space)space, 31,
                                              (cui_background_stop){{1, 0, 0, .25f}, 0},
                                              (cui_background_stop){{0, 0, 1, 1}, 1}, &d.brush));
            compare(d, o, (cui_rect){900000, 900000, 100000, 100000});
        } /* edges>1e6, covered centers valid; original gradient extent retained */
    o = options();
    o.origin_x = -1e6f;
    d.bounds = (cui_rect){1e6f, 0, 1, 1};
    atomic_error(d, o, (cui_rect){1e6f, 0, 1, 1}, CUI_ERROR_INVALID_ACTION);
    d.bounds = (cui_rect){999999.5f, 0, 1, 1};
    d.radii = (cui_corner_radii){.5f, .5f, .5f, .5f};
    o.origin_x = -999999.5625f;
    o.origin_y = .5f;
    assert(!reference_inside(d, 1000000.0625L, 0));
    atomic_error(d, o, d.bounds, CUI_ERROR_INVALID_ACTION); /* conservative covered corner */
    d = solid();
    o = options();
    d.bounds = (cui_rect){0, 0, 4, 4};
    assert(cui_background_prepare_query(d, &o, (cui_rect){.25f, .25f, .1f, .1f}, &q) == CUI_OK);
    assert(q.width == 1 && q.height == 1);
    unsigned char p[4];
    assert(cui_background_prepare_raster(d, &o, (cui_rect){.25f, .25f, .1f, .1f}, p, 4, 4, &q) ==
           CUI_OK);
    assert(p[3] == 255); /* clip culls; its alpha is NOT baked into raster */
    for (unsigned i = 0; i < 4; i++) {
        d = solid();
        o = options();
        cui_rect empty = clip;
        if (i == 0)
            d.bounds.w = 0;
        if (i == 1)
            empty.h = 0;
        if (i == 2)
            o.viewport_width = 0;
        if (i == 3)
            empty = (cui_rect){100, 100, 1, 1};
        assert(cui_background_prepare_query(d, &o, empty, &q) == CUI_OK);
        cui_background_prepared_query zero = {0};
        assert(same_query(q, zero));
        assert(cui_background_prepare_raster(d, &o, empty, NULL, 0, 0, &q) == CUI_OK);
    }
    d = solid();
    o = options();
    d.bounds = (cui_rect){0, 0, 2, 1};
    d.opacity = .6f;
    assert(cui_background_make_linear(CUI_BACKGROUND_CSS, CUI_BACKGROUND_SRGB, 90,
                                      (cui_background_stop){{1, 0, 0, 0}, 0},
                                      (cui_background_stop){{0, 0, 1, 1}, 1}, &d.brush));
    o.scale_x = .5f;
    assert(cui_background_prepare_raster(d, &o, clip, p, 4, 4, &q) == CUI_OK);
    assert(p[0] == 0 && p[2] == 255 && p[3] == 77); /* alpha.5*.6 quantized ONCE */
    d.opacity = 0;
    assert(cui_background_prepare_raster(d, &o, clip, p, 4, 4, &q) == CUI_OK);
    assert(p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 0);
}
int main(void) {
    old_sampler_bytes();
    grids_and_models();
    errors();
    work_limits_and_density();
    phase_domain_empty_and_alpha();
    printf("background preparation/reference: %u grids PASS; old sampler8500 pixels identical\n",
           cases);
    return 0;
}
