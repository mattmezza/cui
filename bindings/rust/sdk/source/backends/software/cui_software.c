/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include "../../src/core/cui_text_paint_internal.h"
#include "cui_mesh.h"
#include "cui_shadow.h"
#include "cui_shape.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct cui_software {
    cui_shadow_cache shadows;
    unsigned width, height;
    unsigned char *pixels;
    cui_font *font;
    cui_rect clip;
    cui_color ink;
    cui_text_paint text_paint;
};
static cui_rect intersect(cui_rect a, cui_rect b) {
    float x = fmaxf(a.x, b.x), y = fmaxf(a.y, b.y), r = fminf(a.x + a.w, b.x + b.w),
          d = fminf(a.y + a.h, b.y + b.h);
    return (cui_rect){x, y, fmaxf(0, r - x), fmaxf(0, d - y)};
}
bool cui_software_resize(cui_software *r, unsigned w, unsigned h) {
    if (!r || !w || !h || w > 16384 || h > 16384 || (size_t)w > SIZE_MAX / 4 / h)
        return false;
    if (r->width == w && r->height == h)
        return true;
    unsigned char *p = calloc((size_t)w * h, 4);
    if (!p)
        return false;
    free(r->pixels);
    r->pixels = p;
    r->width = w;
    r->height = h;
    return true;
}
cui_software *cui_software_create(unsigned w, unsigned h, cui_font *f) {
    cui_software *r = calloc(1, sizeof(*r));
    if (!r)
        return NULL;
    r->font = f;
    if (!cui_software_resize(r, w, h)) {
        free(r);
        return NULL;
    }
    return r;
}
void cui_software_destroy(cui_software *r) {
    if (r) {
        cui_shadow_destroy(&r->shadows);
        free(r->pixels);
        free(r);
    }
}
static void blend(cui_software *r, int x, int y, cui_color c) {
    if (x < 0 || y < 0 || (unsigned)x >= r->width || (unsigned)y >= r->height ||
        x + 0.5f < r->clip.x || y + 0.5f < r->clip.y || x + 0.5f >= r->clip.x + r->clip.w ||
        y + 0.5f >= r->clip.y + r->clip.h)
        return;
    unsigned char *p = r->pixels + ((size_t)y * r->width + (unsigned)x) * 4;
    unsigned alpha = c.a * 255u + p[3] * (255u - c.a);
    if (!alpha) {
        memset(p, 0, 4);
        return;
    }
    unsigned src[3] = {c.r, c.g, c.b};
    for (unsigned i = 0; i < 3; i++)
        p[i] =
            (unsigned char)((src[i] * c.a * 255u + p[i] * p[3] * (255u - c.a) + alpha / 2) / alpha);
    p[3] = (unsigned char)((alpha + 127) / 255);
}
static void mesh_pixel(void *userdata, int x, int y, cui_color color) {
    blend(userdata, x, y, color);
}
static void mesh(cui_software *r, const cui_mesh_paint *p) {
    cui_mesh_cpu_raster(p, r->width, r->height, r->clip, mesh_pixel, r);
}
static void rectangle(cui_software *r, const cui_draw_command *c) {
    cui_rect area = intersect(c->bounds, r->clip);
    if (area.w <= 0 || area.h <= 0)
        return;
    int x0 = (int)floorf(area.x), y0 = (int)floorf(area.y), x1 = (int)ceilf(area.x + area.w),
        y1 = (int)ceilf(area.y + area.h);
    float radii[4], edges[4];
    cui_shape_values(c, radii, edges);
    for (int y = y0; y < y1; y++) {
        float left, right, il = 0, ir = 0;
        float yy = y + .5f - c->bounds.y;
        if (!cui_shape_span(c, radii, edges, false, yy, &left, &right))
            continue;
        bool inner =
            c->type == CUI_DRAW_BORDER && cui_shape_span(c, radii, edges, true, yy, &il, &ir);
        for (int x = x0; x < x1; x++) {
            float xx = x + .5f - c->bounds.x;
            if (xx >= left && xx < right && (!inner || xx < il || xx >= ir))
                blend(r, x, y, c->color);
        }
    }
}
static bool bitmap(cui_software *r, const cui_draw_command *c) {
    const cui_bitmap_paint *p = &c->bitmap;
    cui_rect src = p->source;
    if (!cui_bitmap_valid(p->bitmap) || !isfinite(p->opacity) || p->opacity < 0 || p->opacity > 1 ||
        !isfinite(src.x) || !isfinite(src.y) || !isfinite(src.w) || !isfinite(src.h) || src.x < 0 ||
        src.y < 0 || src.w <= 0 || src.h <= 0 || src.x + src.w > p->bitmap.width ||
        src.y + src.h > p->bitmap.height)
        return false;
    cui_rect area = intersect(c->bounds, r->clip);
    if (area.w <= 0 || area.h <= 0 || c->bounds.w <= 0 || c->bounds.h <= 0)
        return true;
    float radii[4], edges[4];
    cui_shape_values(c, radii, edges);
    for (int y = (int)floorf(area.y); y < (int)ceilf(area.y + area.h); y++) {
        float left, right, yy = y + .5f - c->bounds.y;
        if (!cui_shape_span(c, radii, edges, false, yy, &left, &right))
            continue;
        unsigned sy =
            (unsigned)fminf(p->bitmap.height - 1, floorf(src.y + yy / c->bounds.h * src.h));
        for (int x = (int)floorf(area.x); x < (int)ceilf(area.x + area.w); x++) {
            float xx = x + .5f - c->bounds.x;
            if (xx < left || xx >= right)
                continue;
            unsigned sx =
                (unsigned)fminf(p->bitmap.width - 1, floorf(src.x + xx / c->bounds.w * src.w));
            const unsigned char *pixel = p->bitmap.pixels + sy * p->bitmap.stride + sx * 4;
            cui_color color = {pixel[0], pixel[1], pixel[2],
                               (unsigned char)lroundf(pixel[3] * p->opacity)};
            if (p->grayscale)
                color.r = color.g = color.b =
                    (unsigned char)lroundf(.2126f * color.r + .7152f * color.g + .0722f * color.b);
            if (p->custom_tint) {
                color.r = (unsigned char)((unsigned)color.r * p->tint.r / 255);
                color.g = (unsigned char)((unsigned)color.g * p->tint.g / 255);
                color.b = (unsigned char)((unsigned)color.b * p->tint.b / 255);
                color.a = (unsigned char)((unsigned)color.a * p->tint.a / 255);
            }
            blend(r, x, y, color);
        }
    }
    return true;
}
#ifdef CUI_HAS_TEXT
static void glyph(void *user, int x, int y, int w, int h, int pitch, const unsigned char *data) {
    cui_software *r = user;
    if (!data)
        return;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            cui_color c = r->text_paint.kind == CUI_TEXT_PAINT_LINEAR
                              ? cui_text_paint_sample(r->text_paint, x + i + .5, y + j + .5)
                              : r->ink;
            c.a = (uint8_t)((unsigned)c.a * data[j * pitch + i] / 255);
            blend(r, x + i, y + j, c);
        }
}
static void color_glyph(void *user, int x, int y, int w, int h, int pitch,
                        const unsigned char *data, cui_color color) {
    cui_software *r = user;
    r->ink = color;
    glyph(user, x, y, w, h, pitch, data);
}
static bool text_draw(cui_software *r, const cui_draw_command *c) {
    r->text_paint = c->text_paint;
    if (!cui_text_paint_valid(c->text_paint, true))
        return false;
    if (c->text_paint.kind == CUI_TEXT_PAINT_LINEAR && c->text_color_count)
        return false;
    if (c->text_color_count)
        return cui_font_draw_context_line_colored(
            r->font, c->text_source, c->font_size, c->text_style, c->bounds.x, c->bounds.y,
            c->color, c->text_colors, c->text_color_count, color_glyph, r);
    return cui_font_draw_context_line(r->font, c->text, c->text_source, c->font_size, c->text_style,
                                      c->bounds.x, c->bounds.y, glyph, r);
}

#endif
bool cui_software_render(cui_software *r, const cui_draw_list *list, cui_color clear) {
    if (!r || !list || (!list->commands && list->count))
        return false;
#ifdef CUI_HAS_TEXT
    if (!cui_text_tracking_supported(list, cui_font_text_capabilities() != 0))
        return false;
#else
    if (!cui_text_tracking_supported(list, false))
        return false;
#endif
    if (!cui_text_paints_supported(list, true))
        return false;
    cui_draw_list prepared;
    if (!cui_shadow_prepare(&r->shadows, list, 1, 1, 0, 0, &prepared))
        return false;
    list = &prepared;
    /* Reject any invalid borrowed mesh before clearing or drawing the target. */
    for (size_t i = 0; i < list->count; i++)
        if (list->commands[i].type == CUI_DRAW_MESH && !cui_mesh_valid(list->commands[i].mesh))
            return false;
    for (size_t i = 0; i < (size_t)r->width * r->height; i++) {
        r->pixels[i * 4] = clear.r;
        r->pixels[i * 4 + 1] = clear.g;
        r->pixels[i * 4 + 2] = clear.b;
        r->pixels[i * 4 + 3] = clear.a;
    }
    cui_rect stack[64];
    size_t depth = 0;
    r->clip = (cui_rect){0, 0, (float)r->width, (float)r->height};
    for (size_t i = 0; i < list->count; i++) {
        const cui_draw_command *c = &list->commands[i];
        if (!isfinite(c->bounds.x) || !isfinite(c->bounds.y) || !isfinite(c->bounds.w) ||
            !isfinite(c->bounds.h) || !cui_shape_valid(c) || !isfinite(c->radius) ||
            !isfinite(c->stroke))
            return false;
        switch (c->type) {
        case CUI_DRAW_MESH:
            mesh(r, &c->mesh);
            break;
        case CUI_DRAW_INSET_SHADOW:
        case CUI_DRAW_DROP_SHADOW:
        case CUI_DRAW_IMAGE:
            if (!bitmap(r, c))
                return false;
            break;
        case CUI_DRAW_RECT:
        case CUI_DRAW_BORDER:
            rectangle(r, c);
            break;
        case CUI_DRAW_CLIP_PUSH:
            if (depth == 64)
                return false;
            stack[depth++] = r->clip;
            r->clip = intersect(r->clip, c->bounds);
            break;
        case CUI_DRAW_CLIP_POP:
            if (!depth)
                return false;
            r->clip = stack[--depth];
            break;
        case CUI_DRAW_TEXT:
#ifdef CUI_HAS_TEXT
            r->ink = c->color;
            if (!r->font || !text_draw(r, c))
                return false;
#else
            return false;
#endif
            break;
        default:
            return false;
        }
    }
    return depth == 0;
}
const unsigned char *cui_software_pixels(const cui_software *r, unsigned *w, unsigned *h) {
    if (!r)
        return NULL;
    if (w)
        *w = r->width;
    if (h)
        *h = r->height;
    return r->pixels;
}
bool cui_software_save_ppm(const cui_software *r, const char *path) {
    if (!r || !path)
        return false;
    FILE *f = fopen(path, "wb");
    if (!f)
        return false;
    bool ok = fprintf(f, "P6\n%u %u\n255\n", r->width, r->height) > 0;
    for (size_t i = 0; ok && i < (size_t)r->width * r->height; i++)
        ok = fwrite(r->pixels + i * 4, 1, 3, f) == 3;
    if (fclose(f))
        ok = false;
    return ok;
}
static bool render(void *r, const cui_draw_list *list, cui_color clear) {
    return cui_software_render(r, list, clear);
}
static void destroy(void *r) {
    cui_software_destroy(r);
}
cui_renderer cui_software_interface(cui_software *r) {
    return (cui_renderer){r, "CUI deterministic software",
                          CUI_RENDER_ROUNDED | CUI_RENDER_CLIP | CUI_RENDER_IMAGE |
                              CUI_RENDER_MESH | CUI_RENDER_DROP_SHADOW | CUI_RENDER_INSET_SHADOW
#ifdef CUI_HAS_TEXT
                              |
                              (r && r->font ? CUI_RENDER_TEXT | CUI_RENDER_TEXT_LINEAR_GRADIENT : 0)
#endif
                              ,
                          render, destroy};
}
