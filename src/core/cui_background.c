/* SPDX-License-Identifier: MIT */
#include "cui/cui_background.h"
#include <math.h>

static double bg_clamp(double x) {
    return fmax(0, fmin(1, x));
}
static bool bg_unit(float x) {
    return isfinite(x) && x >= 0 && x <= 1;
}
static bool bg_color_valid(cui_background_color c) {
    return bg_unit(c.r) && bg_unit(c.g) && bg_unit(c.b) && bg_unit(c.a);
}
static double bg_decode(double x) {
    return x <= .04045 ? x / 12.92 : pow((x + .055) / 1.055, 2.4);
}
static double bg_encode(double x) {
    return x <= .0031308 ? x * 12.92 : 1.055 * pow(x, 1. / 2.4) - .055;
}
cui_background_color cui_background_color_from_srgba8(cui_color c) {
    return (cui_background_color){(float)bg_decode(c.r / 255.), (float)bg_decode(c.g / 255.),
                                  (float)bg_decode(c.b / 255.), c.a / 255.f};
}
bool cui_background_color_from_source_038_hsla(float h, float s, float l, float a,
                                               cui_background_color *out) {
    if (!out || !bg_unit(h) || !bg_unit(s) || !bg_unit(l) || !bg_unit(a))
        return false;
    double hue = h * 6., chroma = (1 - fabs(2. * l - 1)) * s;
    double x = chroma * (1 - fabs(fmod(hue, 2) - 1)), m = l - chroma / 2;
    double r = 0, g = 0, b = 0;
    if (hue < 1) {
        r = chroma;
        g = x;
    } else if (hue < 2) {
        r = x;
        g = chroma;
    } else if (hue < 3) {
        g = chroma;
        b = x;
    } else if (hue < 4) {
        g = x;
        b = chroma;
    } else if (hue < 5) {
        r = x;
        b = chroma;
    } else {
        r = chroma;
        b = x;
    }
    *out = (cui_background_color){(float)(r + m), (float)(g + m), (float)(b + m), a};
    return true;
}
bool cui_background_valid(const cui_background *bg) {
    if (!bg)
        return false;
    if (bg->kind == CUI_BACKGROUND_SOLID)
        return bg_color_valid(bg->solid);
    return bg->kind == CUI_BACKGROUND_LINEAR &&
           (bg->model == CUI_BACKGROUND_CSS || bg->model == CUI_BACKGROUND_SOURCE_038) &&
           (bg->space == CUI_BACKGROUND_SRGB || bg->space == CUI_BACKGROUND_OKLAB) &&
           isfinite(bg->angle_degrees) && bg->angle_degrees >= 0 && bg->angle_degrees <= 360 &&
           bg_color_valid(bg->stops[0].color) && bg_color_valid(bg->stops[1].color) &&
           bg_unit(bg->stops[0].position) && bg_unit(bg->stops[1].position) &&
           (bg->model != CUI_BACKGROUND_CSS || bg->stops[0].position <= bg->stops[1].position);
}
bool cui_background_make_solid(cui_background_color color, cui_background *out) {
    cui_background value = {.kind = CUI_BACKGROUND_SOLID, .solid = color};
    if (!out || !cui_background_valid(&value))
        return false;
    *out = value;
    return true;
}
bool cui_background_make_linear(cui_background_model model, cui_background_space space, float angle,
                                cui_background_stop first, cui_background_stop second,
                                cui_background *out) {
    cui_background value = {.kind = CUI_BACKGROUND_LINEAR,
                            .model = model,
                            .space = space,
                            .angle_degrees = angle,
                            .stops = {first, second}};
    if (!out || !cui_background_valid(&value))
        return false;
    *out = value;
    return true;
}
/* Audited Plot/source transforms; kept local so legacy Plot/effects/text math and
 * byte output remain unchanged. Core has no component dependency. */
static void bg_to_lab(const double rgb[3], double out[3]) {
    double l = cbrt(.4122214708 * rgb[0] + .5363325363 * rgb[1] + .0514459929 * rgb[2]);
    double m = cbrt(.2119034982 * rgb[0] + .6806995451 * rgb[1] + .1073969566 * rgb[2]);
    double s = cbrt(.0883024619 * rgb[0] + .2817188376 * rgb[1] + .6299787005 * rgb[2]);
    out[0] = .2104542553 * l + .7936177850 * m - .0040720468 * s;
    out[1] = 1.9779984951 * l - 2.4285922050 * m + .4505937099 * s;
    out[2] = .0259040371 * l + .7827717662 * m - .8086757660 * s;
}
static void bg_from_lab(const double lab[3], double out[3]) {
    double l = lab[0] + .3963377774 * lab[1] + .2158037573 * lab[2];
    double m = lab[0] - .1055613458 * lab[1] - .0638541728 * lab[2];
    double s = lab[0] - .0894841775 * lab[1] - 1.2914855480 * lab[2];
    l *= l * l;
    m *= m * m;
    s *= s * s;
    out[0] = 4.0767416621 * l - 3.3077115913 * m + .2309699292 * s;
    out[1] = -1.2684380046 * l + 2.6097574011 * m - .3413193965 * s;
    out[2] = -.0041960863 * l - .7034186147 * m + 1.7076147010 * s;
}
static bool bg_coordinate(float x) {
    return isfinite(x) && fabsf(x) <= 1000000;
}
bool cui_background_sample(const cui_background *bg, cui_rect frame, float x, float y,
                           cui_background_pixel *out) {
    if (!out || !cui_background_valid(bg) || !bg_coordinate(frame.x) || !bg_coordinate(frame.y) ||
        !isfinite(frame.w) || !isfinite(frame.h) || frame.w <= 0 || frame.h <= 0 ||
        frame.w > 1000000 || frame.h > 1000000 || !bg_coordinate(x) || !bg_coordinate(y))
        return false;
    cui_background_color c = {0};
    if (bg->kind == CUI_BACKGROUND_SOLID)
        c = bg->solid;
    double rgb[3] = {c.r, c.g, c.b}, alpha = c.a;
    if (bg->kind == CUI_BACKGROUND_LINEAR) {
        double angle = fmod(bg->angle_degrees, 360) * (3.14159265358979323846 / 180);
        double dx = sin(angle), dy = -cos(angle), width = frame.w, height = frame.h;
        double length;
        if (bg->model == CUI_BACKGROUND_SOURCE_038) {
            if (width > height)
                dy *= height / width;
            else
                dx *= width / height;
            length = hypot(dx, dy) * (fabs(dx) > fabs(dy) ? width : height);
        } else
            length = fabs(width * dx) + fabs(height * dy);
        double position = .5 + ((double)x - frame.x - width / 2) * dx / length +
                          ((double)y - frame.y - height / 2) * dy / length;
        double interval = (double)bg->stops[1].position - bg->stops[0].position;
        double t = interval == 0 ? (position >= bg->stops[1].position ? 1 : 0)
                                 : (position - bg->stops[0].position) / interval;
        t = bg_clamp(t);
        cui_background_color a = bg->stops[0].color, b = bg->stops[1].color;
        double first[3] = {a.r, a.g, a.b}, second[3] = {b.r, b.g, b.b};
        alpha = a.a * (1 - t) + b.a * t;
        if (bg->space == CUI_BACKGROUND_OKLAB) {
            double tmp[3];
            bg_to_lab(first, tmp);
            for (size_t i = 0; i < 3; i++)
                first[i] = tmp[i];
            bg_to_lab(second, tmp);
            for (size_t i = 0; i < 3; i++)
                second[i] = tmp[i];
        } else
            for (size_t i = 0; i < 3; i++) {
                first[i] = bg_encode(first[i]);
                second[i] = bg_encode(second[i]);
            }
        double mixed[3];
        for (size_t i = 0; i < 3; i++) {
            mixed[i] =
                bg->model == CUI_BACKGROUND_CSS
                    ? (alpha == 0 ? 0 : (first[i] * a.a * (1 - t) + second[i] * b.a * t) / alpha)
                    : first[i] * (1 - t) + second[i] * t;
        }
        if (bg->space == CUI_BACKGROUND_OKLAB)
            bg_from_lab(mixed, rgb);
        else
            for (size_t i = 0; i < 3; i++)
                rgb[i] = bg_decode(mixed[i]);
    }
    cui_background_pixel value = {(float)(rgb[0] * alpha), (float)(rgb[1] * alpha),
                                  (float)(rgb[2] * alpha), (float)alpha};
    if (value.a == 0)
        value = (cui_background_pixel){0};
    *out = value;
    return true;
}
bool cui_background_sample_srgba8(const cui_background *bg, cui_rect frame, float x, float y,
                                  cui_color *out) {
    cui_background_pixel p;
    if (!out || !cui_background_sample(bg, frame, x, y, &p))
        return false;
    cui_color value = {0};
    if (p.a > 0) {
        value.r = (uint8_t)floor(bg_clamp(bg_encode(bg_clamp((double)p.r / p.a))) * 255 + .5);
        value.g = (uint8_t)floor(bg_clamp(bg_encode(bg_clamp((double)p.g / p.a))) * 255 + .5);
        value.b = (uint8_t)floor(bg_clamp(bg_encode(bg_clamp((double)p.b / p.a))) * 255 + .5);
        value.a = (uint8_t)floor(p.a * 255. + .5);
    }
    *out = value;
    return true;
}
