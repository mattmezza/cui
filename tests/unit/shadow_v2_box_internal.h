/* SPDX-License-Identifier: MIT */
/* TEST ONLY. No production geometry, renderer, public API, cache or allocator. */
#ifndef CUI_TEST_SHADOW_BOX_H
#define CUI_TEST_SHADOW_BOX_H
#define SHADOW_BOX_MAX_CELLS 64u
#define SHADOW_BOX_MAX_TAPS ((size_t)1024 * 1024)
typedef struct {
    int64_t left, top, right, bottom;
} shadow_box_support;
typedef bool (*shadow_box_read)(void *, int64_t, int64_t, unsigned *);
typedef struct {
    int64_t x0, y0, x1, y1;
    double left, top, right, bottom;
} shadow_box_cell;
/* Raw footprints, not cropped footprints, determine the denominator. All output
 * means publish together after preflight and every read succeeds. Evaluation
 * diagnostics intentionally report work even on read failure. No silent fallback. */
static cui_error shadow_box_integrate(double left, double top, double dx, double dy, unsigned width,
                                      unsigned height, shadow_box_support support, size_t cap,
                                      shadow_box_read read, void *user, double *out,
                                      size_t *evaluated) {
    if (!out || !evaluated || !read || !width || !height || width > SHADOW_BOX_MAX_CELLS / height ||
        !isfinite(left) || !isfinite(top) || !isfinite(dx) || !isfinite(dy) || dx < 1 || dy < 1 ||
        fabs(left) > 0x1p50 || fabs(top) > 0x1p50 || dx > 0x1p24 || dy > 0x1p24 ||
        support.left > support.right || support.top > support.bottom ||
        support.left < -INT64_C(4503599627370496) || support.right > INT64_C(4503599627370496) ||
        support.top < -INT64_C(4503599627370496) || support.bottom > INT64_C(4503599627370496))
        return CUI_ERROR_INVALID_ACTION;
    if (cap > SHADOW_BOX_MAX_TAPS)
        return CUI_ERROR_CAPACITY;
    *evaluated = 0;
    shadow_box_cell plan[SHADOW_BOX_MAX_CELLS];
    size_t taps = 0;
    for (unsigned y = 0; y < height; ++y)
        for (unsigned x = 0; x < width; ++x) {
            shadow_box_cell *p = &plan[y * width + x];
            *p = (shadow_box_cell){.left = left + x * dx,
                                   .top = top + y * dy,
                                   .right = left + (x + 1.) * dx,
                                   .bottom = top + (y + 1.) * dy};
            int64_t l = (int64_t)floor(p->left), r = (int64_t)ceil(p->right);
            int64_t t = (int64_t)floor(p->top), b = (int64_t)ceil(p->bottom);
            p->x0 = l > support.left ? l : support.left;
            p->y0 = t > support.top ? t : support.top;
            p->x1 = r < support.right ? r : support.right;
            p->y1 = b < support.bottom ? b : support.bottom;
            if (p->x0 >= p->x1 || p->y0 >= p->y1)
                continue;
            uint64_t nx = (uint64_t)(p->x1 - p->x0), ny = (uint64_t)(p->y1 - p->y0);
            if (nx > cap - taps || ny > (cap - taps) / nx)
                return CUI_ERROR_CAPACITY;
            taps += (size_t)(nx * ny);
        }
    double result[SHADOW_BOX_MAX_CELLS] = {0}, denominator = dx * dy;
    for (unsigned i = 0; i < width * height; ++i) {
        const shadow_box_cell *p = &plan[i];
        double total = 0;
        if (p->x0 >= p->x1 || p->y0 >= p->y1)
            continue;
        for (int64_t y = p->y0; y < p->y1; ++y)
            for (int64_t x = p->x0; x < p->x1; ++x) {
                double wx = fmax(0, fmin(p->right, x + 1.) - fmax(p->left, (double)x));
                double wy = fmax(0, fmin(p->bottom, y + 1.) - fmax(p->top, (double)y));
                unsigned byte;
                (*evaluated)++;
                if (!read(user, x, y, &byte) || byte > 255)
                    return CUI_ERROR_INVALID_ACTION;
                total += byte * wx * wy;
            }
        result[i] = total / denominator;
    }
    memcpy(out, result, width * height * sizeof(*out));
    return CUI_OK;
}
/* Duplicate audited f32 materialization only for this experiment. A reusable
 * shared private geometry seam is still a design/review prerequisite. No use of
 * old raster-query 16K-side/16M-allocation admission to plan a large q profile. */
static cui_rect shadow_box_geometry(cui_drop_shadow_paint p, float q, float ox, float oy) {
    float l = (p.caster.x + p.layer.offset_x) - p.layer.spread;
    float t = (p.caster.y + p.layer.offset_y) - p.layer.spread;
    float w = p.caster.w + (p.layer.spread + p.layer.spread);
    float h = p.caster.h + (p.layer.spread + p.layer.spread);
    float x = l * q + ox, y = t * q + oy;
    float far_x = (l + w) * q + ox, far_y = (t + h) * q + oy;
    float near_x = floorf(x), near_y = floorf(y);
    return (cui_rect){near_x, near_y, fmaxf(ceilf(far_x), near_x) - near_x,
                      fmaxf(ceilf(far_y), near_y) - near_y};
}
static shadow_box_support shadow_box_source_support(const scenario *s, bool inset, float q,
                                                    float ox, float oy) {
    cui_drop_shadow_paint p = s->paint;
    cui_rect b;
    if (inset) {
        p.layer.spread = -p.layer.spread;
        cui_rect hole = shadow_box_geometry(p, q, ox, oy);
        p.layer.offset_x = p.layer.offset_y = p.layer.spread = 0;
        b = shadow_box_geometry(p, q, ox, oy);
        if (hole.w <= 0 || hole.h <= 0)
            return (shadow_box_support){0};
    } else
        b = shadow_box_geometry(p, q, ox, oy);
    if (!s->paint.layer.color.a || b.w <= 0 || b.h <= 0)
        return (shadow_box_support){0};
    float margin = inset ? 0 : 3 * (p.layer.sigma * q);
    return (shadow_box_support){(int64_t)floorf(b.x - margin), (int64_t)floorf(b.y - margin),
                                (int64_t)ceilf(b.x + b.w + margin),
                                (int64_t)ceilf(b.y + b.h + margin)};
}
typedef struct {
    const scenario *s;
    bool inset;
    float q, ox, oy;
} shadow_box_context;
static bool shadow_box_source(void *user, int64_t x, int64_t y, unsigned *byte) {
    shadow_box_context *c = user;
    float a = -1;
    cui_drop_shadow_paint p = c->s->paint;
    p.layer.color.a = 255;
    bool ok = c->inset ? cui_effect_inset_shadow_sample(p, c->q, c->ox, c->oy, (float)(x + .5),
                                                        (float)(y + .5), &a)
                       : cui_effect_drop_shadow_sample(p, c->q, c->ox, c->oy, (float)(x + .5),
                                                       (float)(y + .5), &a);
    if (!ok || !isfinite(a) || a < 0 || a > 1)
        return false;
    *byte = (unsigned)lroundf(a * 255);
    return true;
}
static cui_error shadow_box_candidate(const scenario *s, bool inset, size_t cap, unsigned char *out,
                                      size_t *taps) {
    if (!isfinite(s->sx) || !isfinite(s->sy) || s->sx < 1.f / 65536 || s->sy < 1.f / 65536 ||
        s->sx > 64 || s->sy > 64 || !isfinite(s->ox) || !isfinite(s->oy) || fabsf(s->ox) > 1e6f ||
        fabsf(s->oy) > 1e6f)
        return CUI_ERROR_INVALID_ACTION;
    float q = fmaxf(1, fmaxf(s->sx, s->sy));
    shadow_box_context context = {s, inset, q, phase(q, s->ox, s->sx), phase(q, s->oy, s->sy)};
    /* One separate pure domain probe; not a source tap, never a raster query. */
    unsigned ignored;
    if (!shadow_box_source(&context, 0, 0, &ignored))
        return CUI_ERROR_INVALID_ACTION;
    shadow_box_support support = shadow_box_source_support(s, inset, q, context.ox, context.oy);
    double means[WIDTH * HEIGHT];
    cui_error e = shadow_box_integrate(-q * (double)s->ox / s->sx + context.ox,
                                       -q * (double)s->oy / s->sy + context.oy, q / (double)s->sx,
                                       q / (double)s->sy, WIDTH, HEIGHT, support, cap,
                                       shadow_box_source, &context, means, taps);
    if (e != CUI_OK)
        return e;
    unsigned char bytes[WIDTH * HEIGHT];
    for (unsigned i = 0; i < WIDTH * HEIGHT; ++i) {
        long a = lroundf((float)means[i] * (s->paint.layer.color.a / 255.f));
        assert(a >= 0 && a <= 255);
        bytes[i] = (unsigned char)a;
    }
    memcpy(out, bytes, sizeof bytes);
    return CUI_OK;
}
#endif
