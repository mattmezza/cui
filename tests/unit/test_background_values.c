/* SPDX-License-Identifier: MIT */
#include "cui/cui_background.h"
#include "cui/cui_plot_view.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void close_pixel(cui_background_pixel a, cui_background_pixel b) {
    assert(fabsf(a.r - b.r) < .000003f && fabsf(a.g - b.g) < .000003f &&
           fabsf(a.b - b.b) < .000003f && fabsf(a.a - b.a) < .000003f);
}
static cui_background gradient(cui_background_model model, cui_background_space space, float angle,
                               cui_background_color a, cui_background_color b) {
    cui_background bg;
    assert(cui_background_make_linear(model, space, angle, (cui_background_stop){a, .25f},
                                      (cui_background_stop){b, .75f}, &bg));
    return bg;
}
static void goldens(void) {
    static const struct {
        int model, space;
        float angle, w, h, x, y;
        cui_background_pixel expected;
    } cases[] = {
#include "../fixtures/background_values/golden.inc"
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        cui_background bg =
            gradient((cui_background_model)cases[i].model, (cui_background_space)cases[i].space,
                     cases[i].angle, (cui_background_color){1, 0, 0, .25f},
                     (cui_background_color){0, 0, 1, .75f});
        cui_background_pixel p;
        assert(cui_background_sample(&bg, (cui_rect){0, 0, cases[i].w, cases[i].h}, cases[i].x,
                                     cases[i].y, &p));
        close_pixel(p, cases[i].expected);
        assert(cui_background_sample(&bg, (cui_rect){100, -200, cases[i].w, cases[i].h},
                                     cases[i].x + 100, cases[i].y - 200, &p));
        close_pixel(p, cases[i].expected);
    }
}
static void alpha_and_color(void) {
    cui_background_color red = {1, 0, 0, 0}, blue = {0, 0, 1, 1};
    cui_background bg = gradient(CUI_BACKGROUND_CSS, CUI_BACKGROUND_SRGB, 90, red, blue);
    cui_background_pixel p;
    cui_color byte;
    assert(cui_background_sample(&bg, (cui_rect){0, 0, 100, 20}, 50, 10, &p));
    close_pixel(p, (cui_background_pixel){0, 0, .5f, .5f});
    assert(cui_background_sample_srgba8(&bg, (cui_rect){0, 0, 100, 20}, 50, 10, &byte));
    assert(byte.r == 0 && byte.g == 0 && byte.b == 255 && byte.a == 128);
    bg.model = CUI_BACKGROUND_SOURCE_038;
    assert(cui_background_sample(&bg, (cui_rect){0, 0, 100, 20}, 50, 10, &p));
    close_pixel(p, (cui_background_pixel){.10702057f, 0, .10702057f, .5f});
    bg.stops[1].color.a = 0;
    assert(cui_background_sample_srgba8(&bg, (cui_rect){0, 0, 100, 20}, 50, 10, &byte));
    assert(byte.r == 0 && byte.g == 0 && byte.b == 0 && byte.a == 0);
    cui_background_color c = cui_background_color_from_srgba8((cui_color){128, 64, 255, 0});
    assert(fabsf(c.r - .2158605f) < .000001f && fabsf(c.g - .05126946f) < .000001f && c.b == 1 &&
           c.a == 0);
    assert(cui_background_color_from_source_038_hsla(0, 0, .5f, 0, &c));
    assert(c.r == .5f && c.g == .5f && c.b == .5f && c.a == 0); /* NO sRGB decode */
    for (unsigned hue = 0; hue <= 6; hue++) {
        assert(cui_background_color_from_source_038_hsla(hue / 6.f, 1, .5f, 1, &c));
        assert(cui_background_make_solid(c, &bg));
        assert(cui_background_sample_srgba8(&bg, (cui_rect){0, 0, 1, 1}, 0, 0, &byte));
        assert(byte.a == 255);
    }
    /* Oklab conversion remains unclipped until byte export. */
    bg = gradient(CUI_BACKGROUND_CSS, CUI_BACKGROUND_OKLAB, 90, (cui_background_color){1, 0, 0, 1},
                  (cui_background_color){0, 0, 1, 1});
    assert(cui_background_sample(&bg, (cui_rect){0, 0, 100, 20}, 50, 10, &p));
    close_pixel(p, (cui_background_pixel){.2637343f, .08657168f, .36282432f, 1});
    assert(cui_background_sample_srgba8(&bg, (cui_rect){0, 0, 100, 20}, 50, 10, &byte));
    assert(byte.r == 140 && byte.g == 83 && byte.b == 162 && byte.a == 255);
}
static void gamut_and_roundtrip(void) {
    cui_background bg =
        gradient(CUI_BACKGROUND_CSS, CUI_BACKGROUND_OKLAB, 90, (cui_background_color){0, 1, 0, 1},
                 (cui_background_color){0, 0, 1, 1});
    cui_background_pixel p;
    cui_color bytes;
    assert(cui_background_sample(&bg, (cui_rect){0, 0, 100, 20}, 50, 10, &p));
    close_pixel(p, (cui_background_pixel){-.08198920f, .40042638f, .51938389f, 1});
    assert(cui_background_sample_srgba8(&bg, (cui_rect){0, 0, 100, 20}, 50, 10, &bytes));
    assert(bytes.r == 0 && bytes.g == 170 && bytes.b == 191 && bytes.a == 255);
    for (unsigned i = 0; i < 256; i++) {
        cui_color input = {(uint8_t)i, (uint8_t)(255 - i), (uint8_t)((i * 37) % 256), 255};
        cui_background_color linear = cui_background_color_from_srgba8(input);
        assert(cui_background_make_solid(linear, &bg));
        assert(cui_background_sample_srgba8(&bg, (cui_rect){0, 0, 1, 1}, 0, 0, &bytes));
        assert(memcmp(&bytes, &input, sizeof bytes) == 0);
    }
    const float invalid[] = {NAN, INFINITY, -INFINITY, -.01f, 1.01f};
    for (unsigned channel = 0; channel < 4; channel++) {
        for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; i++) {
            cui_background_color c = {.2f, .3f, .4f, .5f};
            switch (channel) {
            case 0:
                c.r = invalid[i];
                break;
            case 1:
                c.g = invalid[i];
                break;
            case 2:
                c.b = invalid[i];
                break;
            default:
                c.a = invalid[i];
                break;
            }
            cui_background before = bg;
            assert(!cui_background_make_solid(c, &bg));
            assert(memcmp(&before, &bg, sizeof bg) == 0);
            cui_background_stop a = {c, 0}, b = {{0, 0, 1, 1}, 1};
            assert(
                !cui_background_make_linear(CUI_BACKGROUND_CSS, CUI_BACKGROUND_SRGB, 0, a, b, &bg));
            assert(memcmp(&before, &bg, sizeof bg) == 0);
        }
    }
    cui_background copy = bg;
    bg.solid.r = 0;
    assert(copy.solid.r > 0); /* independent owned copy */
}
static void hsla_unit_gamut(void) {
    const float lightness[] = {0,        0x1p-149f, 0x1p-126f, 3e-17f,         1e-10f,
                               .000001f, .5f,       .999999f,  0x1.fffffep-1f, 1};
    const float saturation[] = {0, .25f, .5f, 0x1.fffffep-1f, 1};
    for (unsigned hue = 0; hue <= 360; ++hue)
        for (size_t l = 0; l < sizeof lightness / sizeof lightness[0]; ++l)
            for (size_t s = 0; s < sizeof saturation / sizeof saturation[0]; ++s) {
                cui_background_color color;
                cui_background solid;
                assert(cui_background_color_from_source_038_hsla(hue / 360.f, saturation[s],
                                                                 lightness[l], .5f, &color));
                assert(cui_background_make_solid(color, &solid));
                assert(cui_background_valid(&solid));
            }
    cui_background_color tiny;
    assert(cui_background_color_from_source_038_hsla(0, 1, 3e-17f, 1, &tiny));
    assert(tiny.r == 2 * 3e-17f && tiny.g == 0 && tiny.b == 0 && tiny.a == 1);
    cui_background_color color = {.1f, .2f, .3f, .4f}, before = color;
    const float bad[] = {NAN, INFINITY, -INFINITY, -.000001f, 1.000001f};
    for (unsigned channel = 0; channel < 4; ++channel)
        for (size_t i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
            float h = .25f, s = 1, l = 3e-17f, a = .5f;
            switch (channel) {
            case 0:
                h = bad[i];
                break;
            case 1:
                s = bad[i];
                break;
            case 2:
                l = bad[i];
                break;
            default:
                a = bad[i];
                break;
            }
            assert(!cui_background_color_from_source_038_hsla(h, s, l, a, &color));
            assert(memcmp(&color, &before, sizeof color) == 0);
        }
}
static void endpoints(void) {
    cui_background bg =
        gradient(CUI_BACKGROUND_CSS, CUI_BACKGROUND_SRGB, 90, (cui_background_color){1, 0, 0, 1},
                 (cui_background_color){0, 0, 1, 1});
    cui_background_pixel p;
    assert(cui_background_sample(&bg, (cui_rect){0, 0, 100, 20}, -100, 0, &p));
    close_pixel(p, (cui_background_pixel){1, 0, 0, 1});
    assert(cui_background_sample(&bg, (cui_rect){0, 0, 100, 20}, 200, 0, &p));
    close_pixel(p, (cui_background_pixel){0, 0, 1, 1});
    bg.stops[0].position = bg.stops[1].position = .5f;
    assert(cui_background_sample(&bg, (cui_rect){0, 0, 100, 20}, 50, 10, &p));
    close_pixel(p, (cui_background_pixel){0, 0, 1, 1});
    assert(cui_background_sample(&bg, (cui_rect){0, 0, 100, 20}, 49, 10, &p));
    close_pixel(p, (cui_background_pixel){1, 0, 0, 1});
    bg.model = CUI_BACKGROUND_SOURCE_038;
    bg.stops[0].position = .75f;
    bg.stops[1].position = .25f;
    assert(cui_background_sample(&bg, (cui_rect){0, 0, 100, 20}, 0, 10, &p));
    close_pixel(p, (cui_background_pixel){0, 0, 1, 1});
    assert(cui_background_sample(&bg, (cui_rect){0, 0, 100, 20}, 100, 10, &p));
    close_pixel(p, (cui_background_pixel){1, 0, 0, 1});
    bg.angle_degrees = 0;
    cui_background_pixel first;
    assert(cui_background_sample(&bg, (cui_rect){0, 0, 100, 20}, 50, 7, &first));
    bg.angle_degrees = 360;
    assert(cui_background_sample(&bg, (cui_rect){0, 0, 100, 20}, 50, 7, &p));
    close_pixel(p, first);
}
static void rejection(void) {
    cui_background valid =
        gradient(CUI_BACKGROUND_CSS, CUI_BACKGROUND_SRGB, 90, (cui_background_color){1, 0, 0, 1},
                 (cui_background_color){0, 0, 1, 1});
    cui_background_pixel sentinel = {.1f, .2f, .3f, .4f}, output;
    cui_color bytes = {11, 22, 33, 44};
    for (unsigned i = 0; i < 18; i++) {
        cui_background bad = valid;
        cui_rect frame = {0, 0, 100, 20};
        float x = 50, y = 10;
        switch (i) {
        case 0:
            bad.kind = (cui_background_kind)99;
            break;
        case 1:
            bad.model = (cui_background_model)-1;
            break;
        case 2:
            bad.space = (cui_background_space)99;
            break;
        case 3:
            bad.angle_degrees = NAN;
            break;
        case 4:
            bad.angle_degrees = 361;
            break;
        case 5:
            bad.stops[0].color.r = -.1f;
            break;
        case 6:
            bad.stops[1].color.a = INFINITY;
            break;
        case 7:
            bad.stops[0].position = .9f;
            break;
        case 8:
            bad.stops[1].position = NAN;
            break;
        case 9:
            frame.w = 0;
            break;
        case 10:
            frame.h = -1;
            break;
        case 11:
            frame.x = 1000001;
            break;
        case 12:
            frame.y = NAN;
            break;
        case 13:
            frame.w = 1000001;
            break;
        case 14:
            x = INFINITY;
            break;
        case 15:
            y = -1000001;
            break;
        case 16:
            bad.stops[0].color.b = NAN;
            break;
        case 17:
            bad.stops[0].position = -.1f;
            break;
        }
        output = sentinel;
        assert(!cui_background_sample(&bad, frame, x, y, &output));
        assert(memcmp(&output, &sentinel, sizeof output) == 0);
        cui_color b = bytes;
        assert(!cui_background_sample_srgba8(&bad, frame, x, y, &b));
        assert(memcmp(&b, &bytes, sizeof b) == 0);
    }
    cui_background before = valid;
    assert(!cui_background_make_solid((cui_background_color){NAN, 0, 0, 1}, &valid));
    assert(memcmp(&before, &valid, sizeof valid) == 0);
    assert(!cui_background_make_linear(CUI_BACKGROUND_CSS, CUI_BACKGROUND_SRGB, -1, valid.stops[0],
                                       valid.stops[1], &valid));
    assert(memcmp(&before, &valid, sizeof valid) == 0);
    cui_background_color c = {1, 0, 0, 1}, saved = c;
    assert(!cui_background_color_from_source_038_hsla(NAN, 0, 0, 1, &c));
    assert(memcmp(&c, &saved, sizeof c) == 0);
    assert(!cui_background_valid(NULL));
    assert(!cui_background_sample(NULL, (cui_rect){0, 0, 1, 1}, 0, 0, &output));
    assert(!cui_background_sample(&valid, (cui_rect){0, 0, 1, 1}, 0, 0, NULL));
    assert(!cui_background_make_solid(c, NULL));
    /* Inactive fields are ignored, even invalid enum/NaN values. */
    valid.solid = (cui_background_color){NAN, NAN, NAN, NAN};
    assert(cui_background_valid(&valid));
    assert(cui_background_sample(&valid, (cui_rect){0, 0, FLT_MIN, FLT_MIN}, 0, 0, &output));
    valid.kind = CUI_BACKGROUND_SOLID;
    valid.solid = c;
    valid.model = (cui_background_model)99;
    valid.space = (cui_background_space)99;
    valid.angle_degrees = NAN;
    valid.stops[0].color.r = NAN;
    assert(cui_background_valid(&valid));
    assert(cui_background_sample(&valid, (cui_rect){0, 0, 1, 1}, 0, 0, &output));
    close_pixel(output, (cui_background_pixel){1, 0, 0, 1});
    /* Constructors are by-value owned; caller mutation cannot change a copy. */
    assert(cui_background_make_solid(valid.solid, &valid));
    assert(valid.model == 0 && valid.space == 0 && valid.angle_degrees == 0 &&
           valid.stops[0].color.r == 0);
}
static void plot_source_oracle(void) {
    for (unsigned space = 0; space < 2; space++) {
        cui_plot_hsla_background legacy = {.kind = CUI_PLOT_BACKGROUND_LINEAR,
                                           .color_space = (cui_plot_color_space)space,
                                           .angle = 45,
                                           .from = {{.1f, .8f, .6f, .25f}, .25},
                                           .to = {{.7f, .9f, .4f, .75f}, .75}};
        cui_background_color a, b;
        assert(cui_background_color_from_source_038_hsla(.1f, .8f, .6f, .25f, &a));
        assert(cui_background_color_from_source_038_hsla(.7f, .9f, .4f, .75f, &b));
        cui_background bg =
            gradient(CUI_BACKGROUND_SOURCE_038, (cui_background_space)space, 45, a, b);
        for (unsigned i = 0; i < 11; i++) {
            cui_plot_linear_rgba old;
            cui_background_pixel p;
            assert(cui_plot_hsla_background_sample(&legacy, (cui_plot_rect){10, 20, 130, 60},
                                                   (cui_plot_point){10 + i * 12, 20 + i * 4}, 1,
                                                   &old) == CUI_PLOT_OK);
            assert(cui_background_sample(&bg, (cui_rect){10, 20, 120, 40}, 10 + i * 12, 20 + i * 4,
                                         &p));
            close_pixel(p, (cui_background_pixel){old.r, old.g, old.b, old.a});
        }
    }
}
int main(void) {
    goldens();
    alpha_and_color();
    endpoints();
    hsla_unit_gamut();
    gamut_and_roundtrip();
    rejection();
    plot_source_oracle();
    puts("background owned values and pure sampler: PASS");
    return 0;
}
