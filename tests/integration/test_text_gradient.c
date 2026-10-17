/* SPDX-License-Identifier: MIT */
#include "../../src/core/cui_text_paint_internal.h"
#include "cui/cui_software.h"
#include "cui/cui_typography.h"
#include <assert.h>
#include <string.h>
static cui_text_gradient_stop stops[] = {{0, {255, 0, 0, 255}}, {1, {0, 0, 255, 255}}};
static cui_text_paint paint(cui_text_paint_anchor anchor) {
    return (cui_text_paint){
        .kind = CUI_TEXT_PAINT_LINEAR, .x1 = 1, .anchor = anchor, .stops = stops, .stop_count = 2};
}
static void sampler(void) {
    cui_text_gradient_stop alpha[] = {{0, {255, 0, 0, 0}}, {1, {0, 0, 255, 255}}};
    cui_text_paint p = paint(CUI_TEXT_ANCHOR_BLOCK);
    p.stops = alpha;
    cui_color c = cui_text_paint_sample(p, .5, 0);
    assert(c.r == 0 && c.b == 255 && c.a == 128);
    cui_text_gradient_stop hard[] = {{0, {255, 0, 0, 255}},
                                     {.5, {255, 0, 0, 255}},
                                     {.5, {0, 0, 255, 255}},
                                     {1, {0, 0, 255, 255}}};
    p.stops = hard;
    p.stop_count = 4;
    assert(cui_text_paint_valid(p, false));
    assert(cui_text_paint_sample(p, .499, 0).r == 255);
    assert(cui_text_paint_sample(p, .5, 0).b == 255);
    p.x1 = 0;
    assert(!cui_text_paint_valid(p, false));
    p.x1 = 1;
    hard[1].position = NAN;
    assert(!cui_text_paint_valid(p, false));
}
static void core(cui_font *font) {
    cui_context *ui = cui_create((cui_config){.measure_styled = cui_font_measure_styled,
                                              .measure_user = font,
                                              .text_geometry_line = cui_font_text_geometry_line,
                                              .max_elements = 128,
                                              .frame_bytes = 65536});
    assert(ui);
    cui_rect saved[16];
    size_t saved_count = 0;
    cui_text_caret saved_carets[16][2] = {0};
    cui_text_selection_rect saved_selection[16][16] = {0};
    size_t saved_caret_count[16] = {0}, saved_selection_count[16] = {0};
    for (int pass = 0; pass < 3; ++pass) {
        assert(cui_begin_frame(ui, 250, 250, 0));
        cui_begin_box(ui, 1, (cui_box){.width = 120, .custom_opacity = true, .opacity = .5f});
        cui_begin_box(ui, 2, (cui_box){.width = -1, .custom_opacity = true, .opacity = .5f});
        cui_text_gradient_stop local[] = {{0, {255, 0, 0, 255}}, {1, {0, 0, 255, 255}}};
        cui_text_paint p = paint(pass == 2 ? CUI_TEXT_ANCHOR_LINE : CUI_TEXT_ANCHOR_BLOCK);
        p.stops = local;
        if (!pass)
            assert(cui_text_paragraph_colored_aligned(
                ui, cui_string("MMMM MMMM\nMM"), 32, (cui_color){255, 255, 255, 255}, 40,
                (cui_text_style){0}, true, NULL, 0, CUI_TEXT_ALIGN_CENTER));
        else
            assert(cui_text_paragraph_painted(ui, cui_string("MMMM MMMM\nMM"), 32, 40,
                                              (cui_text_style){0}, true, CUI_TEXT_ALIGN_CENTER, p));
        local[0].color = (cui_color){0}; /* Must not mutate retained paint. */
        cui_end_box(ui);
        cui_end_box(ui);
        cui_draw_list list = cui_end_frame(ui);
        size_t text_count = 0;
        float block_x0 = 0, block_x1 = 0;
        for (size_t i = 0; i < list.count; ++i) {
            const cui_draw_command *d = &list.commands[i];
            if (d->type != CUI_DRAW_TEXT)
                continue;
            cui_text_geometry geometry = {0};
            assert(cui_get_text_geometry_line(ui, d->text_source, d->font_size, d->text_style,
                                              &geometry) == CUI_TEXT_LAYOUT_OK);
            cui_text_caret carets[2] = {0};
            cui_text_selection_rect selection[16] = {0};
            size_t nc = 0, ns = 0;
            assert(geometry.ops->carets(geometry.session, d->text_source.byte_start, carets, 2,
                                        &nc) == CUI_TEXT_LAYOUT_OK);
            assert(geometry.ops->selection(geometry.session, d->text_source.byte_start,
                                           d->text_source.byte_end, selection, 16,
                                           &ns) == CUI_TEXT_LAYOUT_OK);
            cui_release_text_geometry(&geometry);
            if (!pass) {
                saved[saved_count++] = d->bounds;
                memcpy(saved_carets[text_count], carets, sizeof(carets));
                memcpy(saved_selection[text_count], selection, sizeof(selection));
                saved_caret_count[text_count] = nc;
                saved_selection_count[text_count] = ns;
            } else {
                assert(!memcmp(&saved[text_count], &d->bounds, sizeof(cui_rect)));
                assert(nc == saved_caret_count[text_count] &&
                       ns == saved_selection_count[text_count]);
                assert(!memcmp(saved_carets[text_count], carets, nc * sizeof(*carets)));
                assert(!memcmp(saved_selection[text_count], selection, ns * sizeof(*selection)));
                assert(d->text_paint.stops[0].color.r == 255);
                assert(d->text_paint.stops[0].color.a == 63);
                if (pass == 1) {
                    if (!text_count) {
                        block_x0 = d->text_paint.x0;
                        block_x1 = d->text_paint.x1;
                    }
                    assert(d->text_paint.x0 == block_x0 && d->text_paint.x1 == block_x1);
                    assert(block_x1 - block_x0 == 120);
                } else {
                    assert(d->text_paint.x0 == d->bounds.x);
                    assert(d->text_paint.x1 == d->bounds.x + d->bounds.w);
                }
            }
            ++text_count;
        }
        assert(text_count >= 3 && (!pass || text_count == saved_count));
        assert(cui_last_error(ui) == CUI_OK);
    }
    assert(cui_begin_frame(ui, 250, 250, 0));
    cui_text_paint invalid = paint(CUI_TEXT_ANCHOR_BLOCK);
    invalid.x1 = NAN;
    assert(!cui_text_paragraph_painted(ui, cui_string("bad"), 32, 40, (cui_text_style){0}, true,
                                       CUI_TEXT_ALIGN_LEFT, invalid));
    assert(cui_end_frame(ui).count == 0);
    cui_destroy(ui);
}
static void pixels(cui_font *font) {
    cui_software *r = cui_software_create(100, 100, font);
    assert(r);
    cui_draw_command command = {.type = CUI_DRAW_TEXT,
                                .bounds = {0, 0, 60, 80},
                                .text = {"M", 1},
                                .font_size = 64,
                                .color = {255, 255, 255, 255},
                                .text_paint = paint(CUI_TEXT_ANCHOR_LINE)};
    command.text_paint.x1 = 50;
    cui_draw_list list = {&command, 1, 100, 100};
    assert(cui_software_render(r, &list, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    const unsigned char *data = cui_software_pixels(r, &w, &h);
    size_t red = 0, blue = 0;
    for (unsigned y = 0; y < h; ++y)
        for (unsigned x = 0; x < w; ++x) {
            const unsigned char *p = data + ((size_t)y * w + x) * 4;
            red += p[0] > p[2] + 20;
            blue += p[2] > p[0] + 20;
        }
    assert(red > 20 && blue > 20); /* Both colors occur inside one M, not per glyph. */
    unsigned char saved[100 * 100 * 4];
    memcpy(saved, data, sizeof(saved));
    assert(!cui_text_paints_supported(&list, false)); /* GPU preflight contract. */
    command.text_paint.kind = (cui_text_paint_kind)99;
    assert(!cui_software_render(r, &list, (cui_color){255, 255, 255, 255}));
    assert(!memcmp(saved, cui_software_pixels(r, &w, &h), sizeof(saved)));
    cui_software_destroy(r);
}
int main(int argc, char **argv) {
    assert(argc == 2);
    cui_font *font = cui_font_open(argv[1]);
    assert(font);
    sampler();
    core(font);
    pixels(font);
    cui_font_close(font);
    return 0;
}
