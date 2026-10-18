/* SPDX-License-Identifier: MIT */
#include "../../src/core/cui_text_paint_internal.h"
#include "cui/cui_software.h"
#include "cui/cui_textarea.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static cui_text_style tracking(float value) {
    return (cui_text_style){.letter_spacing = value};
}
static cui_text_layout *layout(cui_font *font, const char *text, float spacing) {
    cui_text_layout *result = NULL;
    assert(cui_font_layout_line(font, cui_string(text), 32, tracking(spacing), CUI_BIDI_AUTO_LTR, 0,
                                SIZE_MAX, NULL, &result) == CUI_TEXT_LAYOUT_OK);
    return result;
}
static void clusters(cui_font *font, const char *text) {
    cui_text_layout *plain = layout(font, text, 0);
    cui_text_layout_view a;
    assert(cui_text_layout_get(plain, &a));
    size_t eligible = 0;
    float min_advance = 10000;
    for (size_t i = 0; i < a.cluster_count; ++i) {
        float advance = a.clusters[i].right - a.clusters[i].left;
        if (advance > 0) {
            ++eligible;
            min_advance = fminf(min_advance, advance);
        }
    }
    for (int pass = 0; pass < 2; ++pass) {
        float gap = pass ? -fminf(1, min_advance / 2) : 3;
        cui_text_layout *spaced = layout(font, text, gap);
        cui_text_layout_view b;
        assert(cui_text_layout_get(spaced, &b));
        assert(a.glyph_count == b.glyph_count && a.cluster_count == b.cluster_count);
        assert(a.run_count == b.run_count && a.height == b.height);
        assert(fabsf(b.width - (a.width + (eligible ? eligible - 1 : 0) * gap)) < .001f);
        float shift = 0;
        size_t remaining = eligible;
        for (size_t i = 0; i < a.cluster_count; ++i) {
            const cui_text_cluster *x = &a.clusters[i], *y = &b.clusters[i];
            bool eligible_cluster = x->right > x->left;
            float add = eligible_cluster && --remaining ? gap : 0;
            assert(x->byte_start == y->byte_start && x->byte_end == y->byte_end);
            assert(fabsf(y->left - (x->left + shift)) < .001f);
            assert(fabsf(y->right - (x->right + shift + add)) < .001f);
            assert(y->right >= y->left);
            for (size_t j = 0; j < x->glyph_count; ++j) {
                size_t k = x->glyph_start + j;
                assert(a.glyphs[k].glyph_id == b.glyphs[k].glyph_id);
                assert(a.glyphs[k].cluster_byte == b.glyphs[k].cluster_byte);
                assert(a.glyphs[k].y == b.glyphs[k].y);
                assert(fabsf(b.glyphs[k].x - (a.glyphs[k].x + shift)) < .001f);
            }
            cui_text_caret carets[4], hit;
            size_t count;
            assert(cui_text_layout_carets(spaced, y->byte_start, carets, 4, &count) ==
                   CUI_TEXT_LAYOUT_OK);
            for (size_t j = 0; j < count; ++j) {
                assert(cui_text_layout_hit_test(spaced, carets[j].x, 0, carets[j].affinity, &hit) ==
                       CUI_TEXT_LAYOUT_OK);
                assert(fabsf(hit.x - carets[j].x) < .001f);
            }
            shift += add;
        }
        cui_text_selection_rect rects[64];
        size_t count;
        assert(cui_text_layout_selection(spaced, 0, strlen(text), rects, 64, &count) ==
               CUI_TEXT_LAYOUT_OK);
        float sum = 0;
        for (size_t i = 0; i < count; ++i)
            sum += rects[i].bounds.w;
        assert(fabsf(sum - b.width) < .01f);
        cui_size measured = cui_font_measure_styled(font, cui_string(text), 32, tracking(gap));
        assert(fabsf(measured.width - b.width) < .001f);
        cui_text_layout_destroy(spaced);
    }
    cui_text_layout_destroy(plain);
}
static cui_config config(cui_font *font) {
    return (cui_config){.max_elements = 256,
                        .max_commands = 512,
                        .frame_bytes = 65536,
                        .measure_styled = cui_font_measure_styled,
                        .measure_user = font,
                        .text_geometry = cui_font_text_geometry,
                        .text_geometry_line = cui_font_text_geometry_line,
                        .text_capabilities = cui_font_text_capabilities()};
}
static void unsupported(cui_font *font) {
    cui_config c = config(font);
    c.text_capabilities = 0;
    cui_context *ui = cui_create(c);
    assert(ui);
    assert(cui_text_capabilities(ui) == 0);
    cui_text_geometry geometry = {0};
    assert(cui_get_text_geometry(ui, cui_string("AB"), 32, tracking(2), &geometry) ==
           CUI_TEXT_LAYOUT_UNAVAILABLE);
    assert(cui_measure_text_ex(ui, cui_string("AB"), 32, tracking(2)).width == 0);
    assert(cui_begin_frame(ui, 300, 200, 0));
    assert(!cui_text_paragraph_colored(ui, cui_string("AB"), 32, (cui_color){255, 255, 255, 255},
                                       40, tracking(2), true, NULL, 0));
    cui_draw_list list = cui_end_frame(ui);
    assert(!list.count);
    cui_destroy(ui);
}
static void wrapping(cui_font *font) {
    cui_context *ui = cui_create(config(font));
    assert(ui);
    const char *text = "MMMM MMMM MMMM";
    size_t counts[3] = {0};
    float widths[] = {400, 90, 400};
    for (int pass = 0; pass < 3; ++pass) {
        assert(cui_begin_frame(ui, widths[pass], 500, 0));
        cui_begin_box(ui, 1, (cui_box){.width = -1});
        assert(cui_text_paragraph_colored_aligned(ui, cui_string(text), 32,
                                                  (cui_color){255, 255, 255, 255}, 40, tracking(3),
                                                  true, NULL, 0, CUI_TEXT_ALIGN_CENTER));
        cui_end_box(ui);
        cui_draw_list list = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        for (size_t i = 0; i < list.count; ++i)
            if (list.commands[i].type == CUI_DRAW_TEXT) {
                ++counts[pass];
                const cui_draw_command *cmd = &list.commands[i];
                cui_text_geometry g = {0};
                assert(cui_get_text_geometry_line(ui, cmd->text_source, 32, tracking(3), &g) ==
                       CUI_TEXT_LAYOUT_OK);
                assert(fabsf(cmd->bounds.w - g.width) < .02f);
                cui_release_text_geometry(&g);
            }
    }
    assert(counts[0] == counts[2] && counts[1] > counts[0]);
    assert(cui_begin_frame(ui, 300, 100, 0));
    assert(!cui_text_paragraph_colored(ui, cui_string("AB"), 32, (cui_color){255, 255, 255, 255},
                                       40, tracking(-10000), true, NULL, 0));
    (void)cui_end_frame(ui);
    cui_destroy(ui);
}
/* The same frame queries both tracking values: retained row geometry must
 * not reuse the first style's cached shape. No edit revision changes. */
static void retained(cui_font *font) {
    cui_context *ui = cui_create(config(font));
    assert(ui);
    char buffer[64] = "MMMM";
    cui_edit *edit = cui_edit_create(
        (cui_edit_config){.buffer = buffer, .capacity = sizeof buffer, .multiline = true});
    assert(edit);
    cui_textarea_line lines[16];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, lines, 16));
    cui_textarea_metrics metrics;
    assert(cui_textarea_measure(cui_textarea_default(), 16, &metrics));
    assert(cui_begin_frame(ui, 400, 300, 0));
    float widths[3], positions[3];
    float gaps[] = {0, 3, 0};
    for (size_t i = 0; i < 3; ++i) {
        assert(cui_textarea_layout_ex(ui, edit, &state, metrics, 300, false, tracking(gaps[i])) ==
               CUI_TEXTAREA_OK);
        widths[i] = state.lines[0].width;
        positions[i] = cui_textarea_row_x(ui, edit, &state, 0, 4);
        assert(fabsf(widths[i] - positions[i]) < .001f);
        char submitted[] = "MM";
        cui_text_line(ui, cui_string(submitted), 32, (cui_color){255, 255, 255, 255}, 40,
                      tracking(gaps[i]));
        submitted[0] = 'X';
    }
    assert(fabsf(widths[1] - widths[0] - 9) < .001f);
    assert(widths[2] == widths[0] && positions[2] == positions[0]);
    cui_draw_list list = cui_end_frame(ui);
    size_t seen = 0;
    for (size_t i = 0; i < list.count; ++i)
        if (list.commands[i].type == CUI_DRAW_TEXT) {
            const cui_draw_command *cmd = &list.commands[i];
            assert(seen < 3 && cmd->text_style.letter_spacing == gaps[seen++]);
            assert(cmd->text.length == 2 && cmd->text.data[0] == 'M');
            cui_size expected =
                cui_font_measure_styled(font, cui_string("MM"), 32, cmd->text_style);
            assert(fabsf(expected.width - cmd->bounds.w) < .01f);
        }
    assert(seen == 3);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static int ink_right(cui_software *renderer) {
    unsigned w, h;
    const unsigned char *p = cui_software_pixels(renderer, &w, &h);
    int right = -1;
    for (unsigned y = 0; y < h; ++y)
        for (unsigned x = 0; x < w; ++x)
            if (p[4 * (y * w + x)] > 30)
                right = (int)x > right ? (int)x : right;
    return right;
}
static void pixels(cui_font *font) {
    cui_software *renderer = cui_software_create(400, 120, font);
    assert(renderer);
    cui_draw_command cmd = {.type = CUI_DRAW_TEXT,
                            .bounds = {10, 10, 300, 80},
                            .font_size = 48,
                            .text = {"MM", 2},
                            .color = {255, 255, 255, 255}};
    cui_draw_list list = {&cmd, 1, 400, 120};
    assert(cui_software_render(renderer, &list, (cui_color){0, 0, 0, 255}));
    int plain = ink_right(renderer);
    cmd.text_style = tracking(12);
    assert(!cui_text_tracking_supported(&list, false));
    assert(cui_text_tracking_supported(&list, true));
    assert(cui_software_render(renderer, &list, (cui_color){0, 0, 0, 255}));
    assert(ink_right(renderer) == plain + 12);
    cmd.text_style = tracking(-4);
    assert(cui_software_render(renderer, &list, (cui_color){0, 0, 0, 255}));
    assert(ink_right(renderer) == plain - 4);
    cui_software_destroy(renderer);
}
int main(int argc, char **argv) {
    assert(argc >= 2);
    cui_font *font = cui_font_open(argv[1]);
    assert(font);
    if (!(cui_font_text_capabilities() & CUI_TEXT_CAP_LETTER_SPACING)) {
        assert(cui_font_measure_styled(font, cui_string("AB"), 32, tracking(2)).width == 0);
        cui_text_geometry g = {0};
        assert(cui_font_text_geometry(font, cui_string("AB"), 32, tracking(2), &g) ==
               CUI_TEXT_LAYOUT_UNAVAILABLE);
        cui_software *r = cui_software_create(100, 100, font);
        assert(r);
        cui_draw_command cmd = {.type = CUI_DRAW_TEXT,
                                .bounds = {0, 0, 100, 100},
                                .font_size = 32,
                                .text = {"AB", 2},
                                .text_style = {.letter_spacing = 2},
                                .color = {255, 255, 255, 255}};
        cui_draw_list list = {&cmd, 1, 100, 100};
        assert(!cui_software_render(r, &list, (cui_color){0}));
        cui_software_destroy(r);
        cui_font_close(font);
        return 0;
    }
    clusters(font, "AV fi ffi");
    clusters(font, "a\xcc\x81"
                   "b");
    clusters(font, "A\xe2\x80\x8b"
                   "B");
    clusters(font, "M");
    clusters(font, "");
    if (argc > 2) {
        cui_font *arabic = cui_font_open(argv[2]);
        assert(arabic);
        clusters(arabic, "سلام");
        clusters(arabic, "مرحبا بالعالم");
        cui_font *fallbacks[] = {arabic};
        assert(cui_font_set_fallbacks(font, fallbacks, 1));
        clusters(font, "abc مرحبا 123");
        assert(cui_font_set_fallbacks(font, NULL, 0));
        cui_font_close(arabic);
    }
    cui_text_layout *sentinel = (void *)1;
    assert(cui_font_layout_line(font, cui_string("AB"), 32, tracking(-10000), CUI_BIDI_AUTO_LTR, 0,
                                SIZE_MAX, NULL, &sentinel) == CUI_TEXT_LAYOUT_INVALID);
    assert(sentinel == (void *)1);
    unsupported(font);
    wrapping(font);
    retained(font);
    pixels(font);
    cui_font_close(font);
    puts("letter spacing: clusters, bidi, geometry, wrap and pixels passed");
    return 0;
}
