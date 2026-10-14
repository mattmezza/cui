/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_text.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static cui_draw_list draws;
static cui_input_result frame(cui_context *ui, cui_edit *edit, cui_input_state *state,
                              cui_input_options options) {
    assert(cui_begin_frame(ui, 500, 150, .016f));
    cui_theme theme = cui_theme_light();
    theme.selection = (cui_color){17, 33, 49, 255};
    cui_begin_box(ui, 1, (cui_box){.width = -1, .height = -1, .padding = 16});
    cui_input_result r = cui_input_ex(ui, 10, &theme, cui_string("Pointer"), edit, state, options);
    cui_end_box(ui);
    draws = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return r;
}
static void event(cui_context *ui, cui_text_geometry *g, cui_event_type type, unsigned clicks,
                  size_t byte) {
    cui_text_caret caret[2];
    size_t count;
    assert(g->ops->carets(g->session, byte, caret, 2, &count) == CUI_TEXT_LAYOUT_OK && count);
    cui_rect viewport = cui_bounds(ui, cui_input_viewport_id(10));
    assert(cui_queue_event(ui, (cui_event){.type = type,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .click_count = clicks,
                                           .x = viewport.x + caret[0].x + .01f,
                                           .y = viewport.y + 2}));
}
static void pixel(void *user, int x, int y, int w, int h, int pitch, const unsigned char *alpha) {
    (void)x;
    (void)y;
    for (int row = 0; row < h; row++)
        for (int col = 0; col < w; col++)
            if (alpha[row * pitch + col])
                (*(size_t *)user)++;
}
int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : getenv("CUI_TEST_FONT");
    if (!path || !path[0])
        return 77;
    cui_font *font = cui_font_open(path);
    if (!font)
        return 77;
    cui_context *ui = cui_create((cui_config){.measure = cui_font_measure,
                                              .measure_user = font,
                                              .text_geometry = cui_font_text_geometry});
    assert(ui);
    char document[128] = "rök é text";
    cui_edit *edit = cui_edit_create((cui_edit_config){document, sizeof document, false, 2048, 16});
    assert(edit);
    cui_input_state state = {0};
    cui_input_options options = {.readonly = true};
    assert(frame(ui, edit, &state, options).error == CUI_EDIT_OK);
    cui_focus(ui, 10);
    frame(ui, edit, &state, options);
    cui_theme theme = cui_theme_light();
    cui_input_metrics metrics;
    assert(cui_input_measure(options, theme.font_size, &metrics));
    cui_text_geometry g = {0};
    assert(cui_font_text_geometry(font, cui_string(document), metrics.font_size,
                                  (cui_text_style){0}, &g) == CUI_TEXT_LAYOUT_OK);
    event(ui, &g, CUI_EVENT_POINTER_DOWN, 2, 1);
    assert(!frame(ui, edit, &state, options).changed);
    cui_edit_state selected = cui_edit_get_state(edit);
    assert(selected.anchor == 0 && selected.cursor == 4);
    cui_text_selection_rect rectangles[4];
    size_t count;
    assert(g.ops->selection(g.session, 0, 4, rectangles, 4, &count) == CUI_TEXT_LAYOUT_OK &&
           count > 0);
    float expected_width = 0, painted_width = 0;
    for (size_t i = 0; i < count; i++)
        expected_width += rectangles[i].bounds.w;
    bool painted = false;
    size_t pixels = 0;
    for (size_t i = 0; i < draws.count; i++) {
        cui_draw_command d = draws.commands[i];
        if (d.type == CUI_DRAW_RECT && d.color.r == 17 && d.color.g == 33 && d.color.b == 49) {
            painted_width += d.bounds.w;
            painted = true;
        }
        if (d.type == CUI_DRAW_TEXT && d.text.length == strlen(document))
            assert(cui_font_draw(font, d.text, d.font_size, 0, 0, pixel, &pixels));
    }
    assert(painted && pixels && fabsf(painted_width - expected_width) < .02f);
    event(ui, &g, CUI_EVENT_POINTER_MOVE, 0, 10);
    frame(ui, edit, &state, options);
    selected = cui_edit_get_state(edit);
    assert(selected.anchor == 0 && selected.cursor == 10);
    event(ui, &g, CUI_EVENT_POINTER_UP, 0, 10);
    frame(ui, edit, &state, options);
    event(ui, &g, CUI_EVENT_POINTER_DOWN, 2, 5);
    frame(ui, edit, &state, options);
    selected = cui_edit_get_state(edit);
    assert(selected.anchor == 5 && selected.cursor == 8);
    event(ui, &g, CUI_EVENT_POINTER_UP, 0, 8);
    frame(ui, edit, &state, options);
    event(ui, &g, CUI_EVENT_POINTER_DOWN, 3, 1);
    frame(ui, edit, &state, options);
    selected = cui_edit_get_state(edit);
    assert(selected.anchor == 0 && selected.cursor == strlen(document));
    event(ui, &g, CUI_EVENT_POINTER_UP, 0, strlen(document));
    frame(ui, edit, &state, options);
    options.disabled = true;
    event(ui, &g, CUI_EVENT_POINTER_DOWN, 1, 1);
    frame(ui, edit, &state, options);
    selected = cui_edit_get_state(edit);
    assert(selected.anchor == 0 && selected.cursor == strlen(document));
    g.ops->release(g.session);
    cui_destroy(ui);
    cui_edit_destroy(edit);
    cui_font_close(font);
    return 0;
}
