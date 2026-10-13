/* SPDX-License-Identifier: MIT */
#include "cui/cui_text.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static const char *font_path(const char *env, const char *const *paths, size_t count) {
    const char *requested = getenv(env);
    if (requested) {
        cui_font *f = cui_font_open(requested);
        assert(f);
        cui_font_close(f);
        return requested;
    }
    for (size_t i = 0; i < count; ++i) {
        cui_font *f = cui_font_open(paths[i]);
        if (f) {
            cui_font_close(f);
            return paths[i];
        }
    }
    return NULL;
}
typedef struct {
    uint64_t hash;
    unsigned glyphs;
} glyph_probe;
static void glyph(void *user, int x, int y, int w, int h, int pitch, const unsigned char *data) {
    glyph_probe *p = user;
    p->hash = p->hash * 1099511628211ULL + (unsigned)x;
    p->hash = p->hash * 1099511628211ULL + (unsigned)y;
    p->hash = p->hash * 1099511628211ULL + (unsigned)w;
    p->hash = p->hash * 1099511628211ULL + (unsigned)h;
    ++p->glyphs;
    assert(pitch >= w || !h);
    for (int row = 0; row < h; ++row)
        for (int col = 0; col < w; ++col)
            p->hash = p->hash * 1099511628211ULL + data[row * pitch + col];
}
static glyph_probe draw(cui_font *font, cui_text_style style) {
    glyph_probe p = {UINT64_C(1469598103934665603), 0};
    assert(cui_font_draw_styled(font, cui_string("WiiW"), 24, style, 0, 0, glyph, &p));
    assert(p.glyphs > 0);
    return p;
}
static void matches(cui_font *root, cui_text_style style, cui_font *face) {
    cui_size selected = cui_font_measure_styled(root, cui_string("WiiW"), 24, style);
    cui_size actual = cui_font_measure(face, cui_string("WiiW"), 24);
    assert(selected.width == actual.width && selected.height == actual.height);
    cui_font_metrics a, b;
    assert(cui_font_get_metrics(root, 24, style, &a));
    assert(cui_font_get_metrics(face, 24, (cui_text_style){0}, &b));
    assert(a.ascent == b.ascent && a.descent == b.descent && a.line_height == b.line_height);
    glyph_probe x = draw(root, style), y = draw(face, (cui_text_style){0});
    assert(x.hash == y.hash && x.glyphs == y.glyphs);
}
static void layout(cui_context *ui, cui_font *root, cui_font *first, cui_font *second) {
    assert(cui_begin_frame(ui, 600, 300, .016f));
    cui_text_ex(ui, cui_string("WiiW"), 24, (cui_color){0, 0, 0, 255}, 0,
                (cui_text_style){.family = 31});
    cui_text_ex(ui, cui_string("WiiW"), 24, (cui_color){0, 0, 0, 255}, 0,
                (cui_text_style){.family = 32});
    cui_draw_list list = cui_end_frame(ui);
    unsigned found = 0;
    for (size_t i = 0; i < list.count; ++i)
        if (list.commands[i].type == CUI_DRAW_TEXT) {
            cui_draw_command c = list.commands[i];
            assert(c.text_style.family == (found == 0 ? 31U : 32U));
            cui_font *face = found == 0 ? first : second;
            assert(fabsf(c.bounds.w - cui_font_measure(face, cui_string("WiiW"), 24).width) < .01f);
            glyph_probe expected = draw(face, (cui_text_style){0});
            glyph_probe actual = draw(root, c.text_style);
            assert(actual.hash == expected.hash);
            ++found;
        }
    assert(found == 2 && cui_last_error(ui) == CUI_OK);
}
int main(void) {
    const char *const regular_paths[] = {"/usr/share/fonts/noto/NotoSans-Regular.ttf",
                                         "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                                         "/usr/share/fonts/liberation/LiberationSans-Regular.ttf"};
    const char *const family_paths[] = {"/usr/share/fonts/liberation/LiberationMono-Regular.ttf",
                                        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
                                        "/usr/share/fonts/noto/NotoSansMono-Regular.ttf"};
    const char *regular = font_path("CUI_TEST_FONT", regular_paths, 3),
               *family = font_path("CUI_TEST_FAMILY_FONT", family_paths, 3);
    if (!regular || !family) {
        puts("font-family test skipped: set CUI_TEST_FONT and CUI_TEST_FAMILY_FONT to distinct "
             "faces");
        return 77;
    }
    cui_font *root = cui_font_open(regular), *mono = cui_font_open(family),
             *other = cui_font_open(regular);
    assert(root && mono && other);
    cui_size sans = cui_font_measure(root, cui_string("WiiW"), 24),
             code = cui_font_measure(mono, cui_string("WiiW"), 24);
    assert(sans.width != code.width);
    cui_text_style normal = {.family = 31}, second = {.family = 32},
                   strong = {.weight = CUI_TEXT_WEIGHT_BOLD, .family = 31},
                   missing = {.weight = CUI_TEXT_WEIGHT_BOLD,
                              .italic = true,
                              .monospace = true,
                              .family = 31};
    matches(root, normal, root);
    assert(cui_font_set_style(root, normal, mono));
    assert(cui_font_set_style(root, second, other));
    assert(cui_font_set_style(root, (cui_text_style){.family = UINT32_MAX}, mono));
    assert(cui_font_set_style(root, normal, mono)); /* Repeated borrowed registration. */
    matches(root, normal, mono);
    matches(root, missing, mono);
    assert(cui_font_set_style(root, strong, other));
    matches(root, strong, other);
    assert(cui_font_set_style(root, (cui_text_style){.weight = CUI_TEXT_WEIGHT_BOLD}, mono));
    matches(root, (cui_text_style){.weight = CUI_TEXT_WEIGHT_BOLD, .family = 999}, mono);
    matches(root, (cui_text_style){.family = 999}, root);
    assert(!cui_font_set_style(root, normal, root));
    assert(!cui_font_set_style(root, (cui_text_style){0}, mono));
    assert(!cui_font_set_style(root, (cui_text_style){.weight = (cui_text_weight)99, .family = 31},
                               mono));
    cui_context *ui = cui_create((cui_config){.width = 600,
                                              .height = 300,
                                              .measure = cui_font_measure,
                                              .measure_styled = cui_font_measure_styled,
                                              .font_metrics = cui_font_get_metrics,
                                              .measure_user = root});
    assert(ui);
    layout(ui, root, mono, other);
    assert(!cui_font_register_style(root, normal, "/cui-test/missing-family.ttf"));
    matches(root, normal, mono);
    for (unsigned i = 0; i < 8; ++i)
        assert(cui_font_register_style(root, normal, regular));
    matches(root, normal, other);
    assert(!cui_font_register_style(root, normal, "/cui-test/missing-family.ttf"));
    matches(root, normal, other);
    cui_text_metrics_changed(ui);
    layout(ui, root, other, other);
    /* Owned replacement/removal cannot release aliases of separately borrowed faces. */
    matches(root, (cui_text_style){.family = UINT32_MAX}, mono);
    assert(cui_font_set_style(root, normal, mono));
    matches(root, normal, mono);
    cui_text_metrics_changed(ui);
    layout(ui, root, mono, other);
    assert(cui_font_set_style(root, normal, NULL));
    matches(root, normal, root);
    matches(root, strong, other); /* Removing regular does not remove an exact variant. */
    matches(root, missing, root);
    assert(cui_font_set_style(root, (cui_text_style){.family = 456}, NULL));
    assert(!cui_font_register_style(root, (cui_text_style){0}, family));
    cui_font_metrics saved = {31, 7, 45}, out = saved;
    assert(!cui_font_get_metrics(root, NAN, normal, &out));
    assert(out.ascent == saved.ascent && out.descent == saved.descent &&
           out.line_height == saved.line_height);
    assert(cui_font_register_style(root, normal, family)); /* Owned face released at root close. */
    assert(cui_font_register_style(root, (cui_text_style){.family = UINT32_MAX, .italic = true},
                                   regular));
    cui_destroy(ui);
    cui_font_close(root);
    assert(cui_font_measure(mono, cui_string("WiiW"), 24).width == code.width);
    assert(cui_font_measure(other, cui_string("WiiW"), 24).width == sans.width);
    cui_font_close(mono);
    cui_font_close(other);
    puts("font-family measurement metrics layout fallback and ownership passed");
    return 0;
}
