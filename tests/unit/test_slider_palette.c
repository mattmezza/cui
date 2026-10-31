/* SPDX-License-Identifier: MIT */
#include "cui/cui_slider_palette.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static void near(float actual, double expected) {
    assert(fabs(actual - expected) < 2e-6);
}
static double decoded(unsigned char c) {
    double x = c / 255.0;
    return x <= .04045 ? x / 12.92 : pow((x + .055) / 1.055, 2.4);
}
static void encoded(cui_slider_palette_color actual, cui_color bytes) {
    near(actual.r, decoded(bytes.r));
    near(actual.g, decoded(bytes.g));
    near(actual.b, decoded(bytes.b));
    near(actual.a, bytes.a / 255.0);
}
static void conversions(void) {
    cui_slider_palette_color c, saved = {2, 3, 4, 5};
    c = saved;
    assert(!cui_slider_palette_color_from_source_hsla(NAN, 1, .5f, 1, &c));
    assert(!memcmp(&c, &saved, sizeof(c)));
    const float bad[] = {NAN, INFINITY, -1, 2};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        for (size_t j = 0; j < 4; j++) {
            float v[] = {.5f, .5f, .5f, .5f};
            v[j] = bad[i];
            c = saved;
            assert(!cui_slider_palette_color_from_source_hsla(v[0], v[1], v[2], v[3], &c));
            assert(!memcmp(&c, &saved, sizeof(c)));
        }
    }
    assert(!cui_slider_palette_color_from_srgba8((cui_color){0}, NULL));
    assert(!cui_slider_palette_color_from_source_hsla(0, 0, 0, 0, NULL));
    const float expected[][3] = {{1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 1, 1},
                                 {0, 0, 1}, {1, 0, 1}, {1, 0, 0}};
    for (unsigned i = 0; i < 7; i++) {
        assert(cui_slider_palette_color_from_source_hsla(i / 6.0f, 1, .5f, 0, &c));
        near(c.r, expected[i][0]);
        near(c.g, expected[i][1]);
        near(c.b, expected[i][2]);
        assert(c.a == 0); /* Hidden source RGB must survive alpha0. */
    }
    assert(cui_slider_palette_color_from_source_hsla(0, 1, 2e-8f, 1, &c));
    assert(c.r >= 0 && c.g >= 0 && c.b >= 0);
    assert(cui_slider_palette_color_from_srgba8((cui_color){128, 64, 32, 0}, &c));
    encoded(c, (cui_color){128, 64, 32, 0});
    assert(c.r > 0 && c.a == 0);
    assert(cui_slider_palette_color_from_source_hsla(0, 0, .5f, .5f, &c));
    assert(c.r == .5f && c.g == .5f && c.b == .5f && c.a == .5f); /* No transfer decode. */
}
static void values(void) {
    cui_slider_palette p = {0};
    assert(cui_slider_palette_valid(&p));
    p.thumb.space = (cui_slider_brush_space)99;
    p.thumb.projection = (cui_slider_brush_projection)99;
    p.thumb.angle_degrees = NAN;
    p.thumb.stops[0].position = NAN;
    p.thumb.stops[1].color.r = NAN;
    assert(cui_slider_palette_valid(&p)); /* Inactive fields ignored. */
    p.thumb.kind = (cui_slider_brush_kind)99;
    assert(!cui_slider_palette_valid(&p));
    p = (cui_slider_palette){0};
    p.thumb.kind = CUI_SLIDER_BRUSH_LINEAR;
    p.thumb.solid.r = NAN;
    p.thumb.stops[0].position = 1;
    p.thumb.stops[1].position = 0;
    p.thumb.angle_degrees = 360;
    assert(cui_slider_palette_valid(&p));
    p.thumb.space = CUI_SLIDER_BRUSH_OKLAB;
    assert(cui_slider_palette_valid(&p));
    p.thumb.stops[0].position = 0;
    assert(cui_slider_palette_valid(&p)); /* Equal authored stops. */
    cui_slider_palette saved = p;
    p.thumb.stops[1].color.a = INFINITY;
    assert(!cui_slider_palette_valid(&p));
    p = saved;
    p.thumb.stops[0].position = -.1f;
    assert(!cui_slider_palette_valid(&p));
    p = saved;
    p.thumb.angle_degrees = 361;
    assert(!cui_slider_palette_valid(&p));
    p = saved;
    p.thumb.projection = (cui_slider_brush_projection)1;
    assert(!cui_slider_palette_valid(&p));
    p = saved;
    p.thumb.space = (cui_slider_brush_space)99;
    assert(!cui_slider_palette_valid(&p));
    p = saved;
    p.bar.r = -.1f;
    assert(!cui_slider_palette_valid(&p));
    p = saved;
    p.ring.a = 1.1f;
    assert(!cui_slider_palette_valid(&p));
    assert(!cui_slider_palette_valid(NULL));
}
static void resolver(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (unsigned i = 0; i < 4; i++) {
        cui_slider_palette p;
        assert(cui_slider_palette_builtin_get((cui_slider_palette_builtin)i, &p));
        assert(cui_slider_palette_valid(&p));
        encoded(p.bar, themes[i].primary);
        encoded(p.ring, themes[i].focus);
        encoded(p.thumb.solid, themes[i].on_primary);
        assert(p.thumb.kind == CUI_SLIDER_BRUSH_SOLID && p.thumb.angle_degrees == 0);
    }
    cui_theme custom = {0};
    custom.primary = (cui_color){15, 23, 42, 0};
    custom.on_primary = (cui_color){255, 255, 255, 128};
    custom.focus = (cui_color){1, 2, 3, 4};
    custom.dark = true;
    custom.radius = NAN;
    cui_slider_palette p;
    assert(cui_slider_palette_from_theme(&custom, &p));
    assert(cui_slider_palette_valid(&p) && p.thumb.kind == CUI_SLIDER_BRUSH_SOLID);
    encoded(p.bar, custom.primary);
    encoded(p.thumb.solid, custom.on_primary);
    encoded(p.ring, custom.focus);
    custom.dark = false;
    cui_slider_palette q;
    assert(cui_slider_palette_from_theme(&custom, &q));
    assert(!memcmp(&p, &q, sizeof(p))); /* No inferred source theme identity. */
    memset(&p, 0xa5, sizeof(p));
    q = p;
    assert(!cui_slider_palette_from_theme(NULL, &p));
    assert(!memcmp(&p, &q, sizeof(p)));
    assert(!cui_slider_palette_builtin_get((cui_slider_palette_builtin)-1, &p));
    assert(!memcmp(&p, &q, sizeof(p)));
    assert(!cui_slider_palette_builtin_get((cui_slider_palette_builtin)4, &p));
    assert(!memcmp(&p, &q, sizeof(p)));
    assert(!cui_slider_palette_preset_get(SIZE_MAX, &p));
    assert(!memcmp(&p, &q, sizeof(p)));
    assert(!cui_slider_palette_preset_get(cui_slider_palette_preset_count(), &p));
    assert(!memcmp(&p, &q, sizeof(p)));
    assert(!cui_slider_palette_from_theme(&custom, NULL));
    assert(!cui_slider_palette_preset_get(0, NULL));
    assert(!cui_slider_palette_builtin_get(0, NULL));
    assert(!cui_slider_palette_preset_name(SIZE_MAX).length);
    assert(cui_slider_palette_preset_count() == 38 &&
           cui_slider_palette_preset_count() == cui_theme_preset_count());
    for (size_t i = 0; i < cui_slider_palette_preset_count(); i++) {
        cui_str a = cui_slider_palette_preset_name(i), b = cui_theme_preset_name(i);
        assert(a.length == b.length && !memcmp(a.data, b.data, a.length));
        assert(cui_slider_palette_preset_get(i, &p) && cui_slider_palette_valid(&p));
    }
}
static void color_json(cui_slider_palette_color c) {
    printf("[%.9g,%.9g,%.9g,%.9g]", c.r, c.g, c.b, c.a);
}
static void dump(void) {
    puts("[");
    for (size_t i = 0; i < cui_slider_palette_preset_count(); i++) {
        cui_slider_palette p;
        cui_str n = cui_slider_palette_preset_name(i);
        assert(cui_slider_palette_preset_get(i, &p));
        printf("%s{\"name\":\"%.*s\",\"bar\":", i ? "," : "", (int)n.length, n.data);
        color_json(p.bar);
        printf(",\"ring\":");
        color_json(p.ring);
        printf(",\"kind\":%d,\"solid\":", (int)p.thumb.kind);
        color_json(p.thumb.solid);
        printf(",\"space\":%d,\"projection\":%d,\"angle\":%.9g,\"stops\":[", (int)p.thumb.space,
               (int)p.thumb.projection, p.thumb.angle_degrees);
        for (unsigned j = 0; j < 2; j++) {
            printf("%s[%.9g,", j ? "," : "", p.thumb.stops[j].position);
            color_json(p.thumb.stops[j].color);
            printf("]");
        }
        printf("]}");
    }
    puts("]");
}
int main(int argc, char **argv) {
    conversions();
    values();
    resolver();
    if (argc == 2 && !strcmp(argv[1], "--dump"))
        dump();
    return 0;
}
