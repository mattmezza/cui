/* SPDX-License-Identifier: MIT */
#ifndef CUI_SOFTWARE_V2_TEXT_INTERNAL_H
#define CUI_SOFTWARE_V2_TEXT_INTERNAL_H
/* Included after the private software surface/clip helpers. No public API. */
#ifdef CUI_HAS_TEXT
#include "../../src/text/cui_text_prepare_internal.h"
typedef struct {
    cui_text_recorded_glyph_internal *first, *last;
} cui_software_v2_text_range;
_Static_assert(_Alignof(cui_software_v2_text_range) <= _Alignof(cui_fundamental_alignment_internal),
               "text range exceeds transaction alignment");
typedef struct {
    int x0, y0, x1, y1;
} cui_software_v2_glyph_cover;
/* Potential bilinear support includes half a source cell beyond bitmap edges.
 * Clip only culls work; exact center membership never changes filter weights. */
static inline bool cui_software_v2_glyph_query(const cui_text_recorded_glyph_internal *g,
                                               const cui_software_v2_surface *s,
                                               cui_software_v2_glyph_cover *out) {
    if (!g || !out || !isfinite(g->source_density) || g->source_density < 1 ||
        g->source_density > 64 || !g->coverage || !g->width || !g->height || g->width > 4096 ||
        g->height > 4096 || (size_t)g->width * g->height > 4 * 1024 * 1024)
        return false;
    double q = g->source_density;
    double left = fmax(((double)g->x - .5) / q, s->clip.left);
    double top = fmax(((double)g->y - .5) / q, s->clip.top);
    double right = fmin(((double)g->x + g->width + .5) / q, s->clip.right);
    double bottom = fmin(((double)g->y + g->height + .5) / q, s->clip.bottom);
    *out = (cui_software_v2_glyph_cover){0};
    if (left >= right || top >= bottom)
        return true;
    const cui_render_v2_options *o = s->options;
    left = fmax(0, left * o->scale_x + o->origin_x);
    top = fmax(0, top * o->scale_y + o->origin_y);
    right = fmin(o->viewport_width, right * o->scale_x + o->origin_x);
    bottom = fmin(o->viewport_height, bottom * o->scale_y + o->origin_y);
    if (left >= right || top >= bottom)
        return true;
    if (!isfinite(left) || !isfinite(top) || !isfinite(right) || !isfinite(bottom))
        return false;
    *out = (cui_software_v2_glyph_cover){(int)floor(left), (int)floor(top), (int)ceil(right),
                                         (int)ceil(bottom)};
    /* These extrema cover every later inverse center and floor operation. They
     * are double-only until an in-bounds source index is established. */
    double xs[2] = {(out->x0 + .5 - o->origin_x) / o->scale_x,
                    (out->x1 - .5 - o->origin_x) / o->scale_x};
    double ys[2] = {(out->y0 + .5 - o->origin_y) / o->scale_y,
                    (out->y1 - .5 - o->origin_y) / o->scale_y};
    for (unsigned i = 0; i < 2; ++i)
        if (!isfinite(xs[i] * q - g->x - .5) || !isfinite(ys[i] * q - g->y - .5))
            return false;
    return true;
}
/* Occupancy is independent of coverage: a zero byte with positive weight is
 * still a real blend, while zero-weight halo neighbors never touch hidden RGB. */
static inline unsigned
cui_software_v2_glyph_source_sample(const cui_text_recorded_glyph_internal *g, double sx, double sy,
                                    bool *occupied) {
    double tx = sx - g->x - .5, ty = sy - g->y - .5;
    double ix = floor(tx), iy = floor(ty), fx = tx - ix, fy = ty - iy, sum = 0;
    *occupied = false;
    for (unsigned y = 0; y < 2; ++y)
        for (unsigned x = 0; x < 2; ++x) {
            double weight = (x ? fx : 1 - fx) * (y ? fy : 1 - fy), xx = ix + x, yy = iy + y;
            if (weight > 0 && xx >= 0 && xx < g->width && yy >= 0 && yy < g->height) {
                *occupied = true;
                sum += weight * g->coverage[(size_t)yy * g->width + (size_t)xx];
            }
        }
    return (unsigned)lround(sum);
}
static inline unsigned cui_software_v2_glyph_sample(const cui_text_recorded_glyph_internal *g,
                                                    double lx, double ly, bool *occupied) {
    return cui_software_v2_glyph_source_sample(g, lx * g->source_density, ly * g->source_density,
                                               occupied);
}
static inline void cui_software_v2_text_paint(cui_software_v2_surface *s,
                                              cui_software_v2_text_range range) {
    for (const cui_text_recorded_glyph_internal *g = range.first; g;
         g = g == range.last ? NULL : g->next) {
        cui_software_v2_glyph_cover cover = {0};
        (void)cui_software_v2_glyph_query(g, s, &cover); /* immutable, preflighted */
        bool aligned = s->options->scale_x == g->source_density &&
                       s->options->scale_y == g->source_density &&
                       s->options->origin_x == floorf(s->options->origin_x) &&
                       s->options->origin_y == floorf(s->options->origin_y);
        for (int y = cover.y0; y < cover.y1; ++y)
            for (int x = cover.x0; x < cover.x1; ++x) {
                double lx = (x + .5 - s->options->origin_x) / s->options->scale_x;
                double ly = (y + .5 - s->options->origin_y) / s->options->scale_y;
                bool occupied;
                /* Exact native-grid phase avoids divide/multiply roundoff turning
                 * an absent halo neighbor into positive-weight zero coverage. */
                unsigned coverage = aligned ? cui_software_v2_glyph_source_sample(
                                                  g, x + .5 - s->options->origin_x,
                                                  y + .5 - s->options->origin_y, &occupied)
                                            : cui_software_v2_glyph_sample(g, lx, ly, &occupied);
                if (!occupied)
                    continue;
                cui_color ink = g->paint.kind == CUI_TEXT_PAINT_LINEAR
                                    ? cui_text_paint_sample(g->paint, lx, ly)
                                    : g->color;
                ink.a = (unsigned char)((unsigned)ink.a * coverage / 255);
                cui_software_v2_emit(s, x, y, ink);
            }
    }
}
#endif
#endif
