/* SPDX-License-Identifier: MIT */
#include "cui/cui_slider_palette.h"
#include <math.h>
#include <string.h>
static float cui_slider_palette_decode(unsigned char channel) {
    float v = channel / 255.0f;
    return v <= .04045f ? v / 12.92f : powf((v + .055f) / 1.055f, 2.4f);
}
bool cui_slider_palette_color_from_srgba8(cui_color c, cui_slider_palette_color *out) {
    if (!out)
        return false;
    *out =
        (cui_slider_palette_color){cui_slider_palette_decode(c.r), cui_slider_palette_decode(c.g),
                                   cui_slider_palette_decode(c.b), c.a / 255.0f};
    return true;
}
static bool cui_slider_palette_unit(float v) {
    return isfinite(v) && v >= 0 && v <= 1;
}
bool cui_slider_palette_color_from_source_hsla(float h, float s, float l, float a,
                                               cui_slider_palette_color *out) {
    if (!out || !cui_slider_palette_unit(h) || !cui_slider_palette_unit(s) ||
        !cui_slider_palette_unit(l) || !cui_slider_palette_unit(a))
        return false;
    float hue = h * 6, c = (1 - fabsf(2 * l - 1)) * s;
    float x = c * (1 - fabsf(fmodf(hue, 2) - 1)), m = l - c / 2;
    cui_slider_palette_color result = {m, m, m, a};
    if (hue < 1) {
        result.r += c;
        result.g += x;
    } else if (hue < 2) {
        result.r += x;
        result.g += c;
    } else if (hue < 3) {
        result.g += c;
        result.b += x;
    } else if (hue < 4) {
        result.g += x;
        result.b += c;
    } else if (hue < 5) {
        result.r += x;
        result.b += c;
    } else {
        result.r += c;
        result.b += x;
    }
    /* Bound floating-point roundoff at the mathematically unit-gamut edges. */
    result.r = fminf(1, fmaxf(0, result.r));
    result.g = fminf(1, fmaxf(0, result.g));
    result.b = fminf(1, fmaxf(0, result.b));
    *out = result;
    return true;
}
static bool cui_slider_palette_color_valid(cui_slider_palette_color c) {
    return cui_slider_palette_unit(c.r) && cui_slider_palette_unit(c.g) &&
           cui_slider_palette_unit(c.b) && cui_slider_palette_unit(c.a);
}
bool cui_slider_palette_valid(const cui_slider_palette *p) {
    if (!p || !cui_slider_palette_color_valid(p->bar) || !cui_slider_palette_color_valid(p->ring))
        return false;
    const cui_slider_palette_brush *b = &p->thumb;
    if (b->kind == CUI_SLIDER_BRUSH_SOLID)
        return cui_slider_palette_color_valid(b->solid);
    if (b->kind != CUI_SLIDER_BRUSH_LINEAR || b->projection != CUI_SLIDER_BRUSH_SOURCE_038 ||
        (b->space != CUI_SLIDER_BRUSH_SRGB && b->space != CUI_SLIDER_BRUSH_OKLAB) ||
        !isfinite(b->angle_degrees) || b->angle_degrees < 0 || b->angle_degrees > 360)
        return false;
    for (size_t i = 0; i < 2; i++)
        if (!cui_slider_palette_unit(b->stops[i].position) ||
            !cui_slider_palette_color_valid(b->stops[i].color))
            return false;
    return true;
}
bool cui_slider_palette_from_theme(const cui_theme *t, cui_slider_palette *out) {
    if (!t || !out)
        return false;
    cui_slider_palette p = {0};
    cui_slider_palette_color_from_srgba8(t->primary, &p.bar);
    cui_slider_palette_color_from_srgba8(t->on_primary, &p.thumb.solid);
    cui_slider_palette_color_from_srgba8(t->focus, &p.ring);
    *out = p;
    return true;
}
bool cui_slider_palette_builtin_get(cui_slider_palette_builtin kind, cui_slider_palette *out) {
    if (!out)
        return false;
    cui_theme t;
    switch (kind) {
    case CUI_SLIDER_PALETTE_LIGHT:
        t = cui_theme_light();
        break;
    case CUI_SLIDER_PALETTE_DARK:
        t = cui_theme_dark();
        break;
    case CUI_SLIDER_PALETTE_HIGH_CONTRAST:
        t = cui_theme_high_contrast();
        break;
    case CUI_SLIDER_PALETTE_TERMINAL:
        t = cui_theme_terminal();
        break;
    default:
        return false;
    }
    return cui_slider_palette_from_theme(&t, out);
}
typedef struct {
    const char *name;
    cui_slider_palette palette;
} cui_slider_palette_record;
#include "cui_slider_palette_presets.inc"
size_t cui_slider_palette_preset_count(void) {
    return sizeof(cui_slider_palette_records) / sizeof(cui_slider_palette_records[0]);
}
cui_str cui_slider_palette_preset_name(size_t index) {
    if (index >= cui_slider_palette_preset_count())
        return (cui_str){0};
    const char *name = cui_slider_palette_records[index].name;
    return (cui_str){name, strlen(name)};
}
bool cui_slider_palette_preset_get(size_t index, cui_slider_palette *out) {
    if (!out || index >= cui_slider_palette_preset_count())
        return false;
    *out = cui_slider_palette_records[index].palette;
    return true;
}
