/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_text.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    cui_font *font;
    bool fail;
    unsigned calls;
    size_t last_bytes;
} host;
static cui_size measure(void *user, cui_str text, float size) {
    return cui_font_measure(((host *)user)->font, text, size);
}
static cui_text_layout_status geometry(void *user, cui_str text, float size, cui_text_style style,
                                       cui_text_geometry *out) {
    host *h = user;
    h->calls++;
    h->last_bytes = text.length;
    for (size_t at = 0; at < text.length; at += 3)
        assert(at + 3 <= text.length && !memcmp(text.data + at, "•", 3));
    if (h->fail)
        return CUI_TEXT_LAYOUT_CAPACITY;
    return cui_font_text_geometry(h->font, text, size, style, out);
}
static cui_draw_list draws;
static cui_input_result frame(cui_context *ui, cui_edit *edit, cui_input_state *state,
                              cui_input_options o) {
    assert(cui_begin_frame(ui, 500, 150, .016f));
    cui_theme theme = cui_theme_light();
    cui_begin_box(ui, 1, (cui_box){.width = -1, .height = -1, .padding = 16});
    cui_input_result result = cui_input_ex(ui, 10, &theme, cui_string("Number"), edit, state, o);
    cui_end_box(ui);
    draws = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void pixel(void *user, int x, int y, int width, int height, int pitch,
                  const uint8_t *alpha) {
    (void)x;
    (void)y;
    for (int row = 0; row < height; row++)
        for (int col = 0; col < width; col++)
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
    host h = {.font = font};
    cui_context *ui =
        cui_create((cui_config){.measure = measure, .measure_user = &h, .text_geometry = geometry});
    assert(ui);
    char document[128] = "", work[128];
    cui_edit *edit = cui_edit_create((cui_edit_config){document, sizeof document, false, 4096, 16});
    assert(edit);
    cui_input_state state = {0};
    cui_input_options o = {.masked = true,
                           .content_type = CUI_TEXT_INPUT_PURPOSE_PASSWORD,
                           .format = {.kind = CUI_EDIT_FORMAT_NUMBER,
                                      .has_separator = true,
                                      .separator = ',',
                                      .fraction_limited = true,
                                      .fraction_digits = 2},
                           .format_workspace = {work, sizeof work}};
    assert(frame(ui, edit, &state, o).error == CUI_EDIT_OK);
    cui_focus(ui, 10);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_TEXT, .text_data = "１２３４。５６７", .text_length = 24}));
    assert(frame(ui, edit, &state, o).changed && !strcmp(document, "1,234.56") &&
           h.last_bytes == 24);
    size_t pixels = 0;
    bool found = false;
    for (size_t i = 0; i < draws.count; i++)
        if (draws.commands[i].type == CUI_DRAW_TEXT && draws.commands[i].text.length == 24) {
            assert(!memcmp(draws.commands[i].text.data, "••••••••", 24));
            assert(cui_font_draw(font, draws.commands[i].text, draws.commands[i].font_size, 0, 0,
                                 pixel, &pixels));
            found = true;
        }
    assert(found && pixels > 0 && cui_edit_get_state(edit).cursor == 8);
    assert(cui_edit_select(edit, 0, 8) == CUI_EDIT_OK);
    h.fail = true;
    unsigned calls = h.calls;
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_TEXT, .text_data = "56781234", .text_length = 8}));
    cui_input_result result = frame(ui, edit, &state, o);
    assert(result.error == CUI_EDIT_PRESENTATION && !strcmp(document, "1,234.56") &&
           h.calls == calls + 1);
    assert(cui_edit_get_state(edit).anchor == 0 && cui_edit_get_state(edit).cursor == 8);
    h.fail = false;
    cui_text_metrics_changed(ui);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_TEXT, .text_data = "56781234", .text_length = 8}));
    result = frame(ui, edit, &state, o);
    assert(result.changed && !strcmp(document, "56,781,234") && h.last_bytes == 30);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_Z, .modifiers = CUI_MOD_CTRL}));
    assert(frame(ui, edit, &state, o).changed && !strcmp(document, "1,234.56"));
    assert(cui_edit_select(edit, 0, 8) == CUI_EDIT_OK);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_COMPOSITION_START, .composition_target = 10}));
    assert(frame(ui, edit, &state, o).error == CUI_EDIT_OK);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                                           .composition_target = 10,
                                           .text_data = "５６７８",
                                           .text_length = 12,
                                           .composition_anchor = 3,
                                           .composition_cursor = 12}));
    assert(frame(ui, edit, &state, o).error == CUI_EDIT_OK && !strcmp(document, "1,234.56") &&
           h.last_bytes == 12);
    assert(state.behavior.composition->anchor == 1 && state.behavior.composition->cursor == 4);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_COMMIT,
                                           .composition_target = 10,
                                           .text_data = "５６７８",
                                           .text_length = 12}));
    assert(frame(ui, edit, &state, o).changed && !strcmp(document, "5,678") && h.last_bytes == 15);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_Z, .modifiers = CUI_MOD_CTRL}));
    assert(frame(ui, edit, &state, o).changed && !strcmp(document, "1,234.56"));
    cui_destroy(ui);
    cui_edit_destroy(edit);
    cui_font_close(font);
    return 0;
}
