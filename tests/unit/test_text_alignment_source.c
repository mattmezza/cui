/* SPDX-License-Identifier: MIT */
#include "cui/cui_label.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static cui_size measure(void *u, cui_str text, float size, cui_text_style style) {
    (void)u;
    (void)size;
    (void)style;
    size_t count = 0;
    for (size_t i = 0; i < text.length; ++i)
        count += ((unsigned char)text.data[i] & 0xc0u) != 0x80u;
    return (cui_size){count * 10, 12};
}
static void near(float a, float b) {
    assert(fabsf(a - b) < .01f);
}
static cui_context *create(size_t bytes) {
    return cui_create(
        (cui_config){.max_elements = 128, .frame_bytes = bytes, .measure_styled = measure});
}
static void rows(cui_text_align alignment, bool wrap, bool edges) {
    cui_context *ui = create(16384);
    assert(ui);
    for (int frame = 0; frame < 2; ++frame) {
        float width = frame ? 100 : 50;
        assert(cui_begin_frame(ui, 200, 200, 0));
        cui_begin_box(ui, 1,
                      (cui_box){.width = width,
                                .padding = 5,
                                .custom_padding = edges,
                                .padding_edges = {.left = 3, .right = 7, .top = 4, .bottom = 6}});
        char text[] = "AA BB\n\nאב\nZ\n";
        cui_text_color_span span = {0, sizeof(text) - 1, {255, 0, 0, 255}};
        assert(cui_text_paragraph_colored_aligned(ui, cui_string(text), 16, (cui_color){0}, 20,
                                                  (cui_text_style){0}, wrap, &span, 1, alignment));
        cui_end_box(ui);
        memset(text, 'x', sizeof(text) - 1);
        cui_draw_list draw = cui_end_frame(ui);
        assert(draw.count == (wrap && !frame ? 4u : 3u));
        float factor = alignment == CUI_TEXT_ALIGN_RIGHT    ? 1
                       : alignment == CUI_TEXT_ALIGN_CENTER ? .5f
                                                            : 0;
        for (size_t i = 0; i < draw.count; ++i) {
            const cui_draw_command *d = &draw.commands[i];
            assert(d->type == CUI_DRAW_TEXT && cui_text_line_source_valid(d->text_source));
            near(d->bounds.x, (edges ? 3 : 5) + (width - 10 - d->bounds.w) * factor);
            assert(d->text_color_count == 1 && d->text_colors[0].color.r == 255);
            assert(d->text_colors[0].byte_start == d->text_source.byte_start);
            assert(d->text_colors[0].byte_end == d->text_source.byte_end);
        }
        /* Empty and trailing rows contribute height, but no text commands. */
        near(cui_bounds(ui, 1).h, (wrap && !frame ? 6 : 5) * 20 + 10);
        const cui_draw_command *rtl = &draw.commands[draw.count - 2];
        assert(rtl->text_source.paragraph.length == 4 &&
               !memcmp(rtl->text_source.paragraph.data, "אב", 4));
        /* A copied retained command preserves aligned geometry across frames. */
        cui_draw_command saved = *rtl;
        char paragraph[4];
        cui_text_color_span saved_span = rtl->text_colors[0];
        memcpy(paragraph, rtl->text_source.paragraph.data, 4);
        saved.text_source.paragraph.data = paragraph;
        saved.text.data = paragraph;
        saved.text_colors = &saved_span;
        float x = saved.bounds.x;
        assert(cui_begin_frame(ui, 200, 200, 0));
        (void)cui_end_frame(ui);
        near(saved.bounds.x, x);
        assert(!memcmp(saved.text.data, "אב", 4));
    }
    cui_destroy(ui);
}
static void oversized(void) {
    cui_context *ui = create(16384);
    assert(ui && cui_begin_frame(ui, 100, 100, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 10});
    assert(cui_text_paragraph_colored_aligned(ui, cui_string("ä̈̈"), 16, (cui_color){0}, 20,
                                              (cui_text_style){0}, true, NULL, 0,
                                              CUI_TEXT_ALIGN_RIGHT));
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(draw.count == 1 && draw.commands[0].text.length == 7);
    near(draw.commands[0].bounds.x, -30);
    cui_destroy(ui);
}
static void validation(void) {
    cui_context *ui = create(16384);
    assert(ui && cui_begin_frame(ui, 100, 100, 0));
    assert(!cui_text_paragraph_colored_aligned(ui, cui_string("x"), 16, (cui_color){0}, 20,
                                               (cui_text_style){0}, true, NULL, 0,
                                               (cui_text_align)99));
    assert(!cui_text_paragraph_colored_aligned(ui, (cui_str){"\xff", 1}, 16, (cui_color){0}, 20,
                                               (cui_text_style){0}, true, NULL, 0,
                                               CUI_TEXT_ALIGN_RIGHT));
    cui_text_line(ui, cui_string("legacy"), 16, (cui_color){0}, 20, (cui_text_style){0});
    cui_draw_list draw = cui_end_frame(ui);
    assert(draw.count == 1);
    near(draw.commands[0].bounds.x, 0);
    cui_destroy(ui);
    ui = create(48);
    assert(ui && cui_begin_frame(ui, 100, 100, 0));
    assert(!cui_text_paragraph_colored_aligned(ui, cui_string("capacity exhaustion"), 16,
                                               (cui_color){0}, 20, (cui_text_style){0}, true, NULL,
                                               0, CUI_TEXT_ALIGN_CENTER));
    assert(cui_end_frame(ui).count == 0);
    assert(cui_begin_frame(ui, 100, 100, 0));
    cui_text_line(ui, cui_string("x"), 16, (cui_color){0}, 20, (cui_text_style){0});
    assert(cui_end_frame(ui).count == 1);
    cui_destroy(ui);
}
static void label(void) {
    cui_theme theme = {.font_size = 16,
                       .text = {0, 0, 0, 255},
                       .muted = {20, 20, 20, 255},
                       .info = {0, 0, 255, 255}};
    cui_context *ui = create(16384);
    assert(ui && cui_begin_frame(ui, 100, 100, 0));
    assert(cui_label_source(ui, &theme,
                            (cui_label_options){.primary = cui_string("AA"),
                                                .secondary = cui_string("B"),
                                                .has_secondary = true,
                                                .highlight = cui_string("B"),
                                                .alignment = CUI_TEXT_ALIGN_RIGHT}));
    cui_draw_list draw = cui_end_frame(ui);
    assert(draw.count == 1);
    near(draw.commands[0].bounds.x, 60);
    assert(draw.commands[0].text_color_count == 3 &&
           draw.commands[0].text_colors[2].color.b == 255);
    assert(cui_begin_frame(ui, 100, 100, 0));
    assert(cui_label_source(ui, &theme,
                            (cui_label_options){.primary = cui_string("secret"),
                                                .masked = true,
                                                .semantic_id = 12,
                                                .alignment = CUI_TEXT_ALIGN_CENTER}));
    draw = cui_end_frame(ui);
    assert(draw.count == 1);
    assert(draw.commands[0].text.length == 18 && draw.commands[0].text_color_count == 0);
    assert(!memcmp(draw.commands[0].text.data, "••••••", 18));
    near(draw.commands[0].bounds.x, 20);
    cui_destroy(ui);
}
int main(void) {
    for (int a = CUI_TEXT_ALIGN_LEFT; a <= CUI_TEXT_ALIGN_RIGHT; ++a) {
        rows((cui_text_align)a, true, false);
        rows((cui_text_align)a, false, false);
        rows((cui_text_align)a, true, true);
        rows((cui_text_align)a, false, true);
    }
    validation();
    oversized();
    label();
    return 0;
}
