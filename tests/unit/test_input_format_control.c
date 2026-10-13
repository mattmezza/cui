/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <string.h>

static cui_draw_list draws;
static cui_input_result frame(cui_context *ui, cui_edit *edit, cui_input_state *state,
                              cui_input_options o) {
    assert(cui_begin_frame(ui, 500, 150, .016f));
    cui_theme t = cui_theme_light();
    cui_begin_box(ui, 1, (cui_box){.width = -1, .height = -1, .padding = 16});
    cui_input_result result = cui_input_ex(ui, 10, &t, cui_string("Formatted"), edit, state, o);
    cui_end_box(ui);
    draws = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static bool drawn(const char *p) {
    for (size_t i = 0; i < draws.count; i++)
        if (draws.commands[i].type == CUI_DRAW_TEXT && draws.commands[i].text.length == strlen(p) &&
            !memcmp(draws.commands[i].text.data, p, strlen(p)))
            return true;
    return false;
}
static void key(cui_context *ui, cui_key k) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k, .modifiers = CUI_MOD_CTRL}));
}
static unsigned copies, predicates;
static char copied[32];
static bool copy(void *p, cui_str value) {
    (void)p;
    copies++;
    memcpy(copied, value.data, value.length);
    copied[value.length] = 0;
    return true;
}
static cui_str paste(void *p) {
    (void)p;
    return cui_string("1234");
}
static bool valid(void *p, cui_str value) {
    (void)p;
    predicates++;
    return !memchr(value.data, 'x', value.length);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    char document[128] = "", workspace[128];
    cui_edit *edit = cui_edit_create((cui_edit_config){document, sizeof document, false, 2048, 12});
    assert(edit);
    cui_input_state state = {0};
    cui_platform clipboard = {
        .capabilities = CUI_PLATFORM_CLIPBOARD, .clipboard_set = copy, .clipboard_get = paste};
    cui_input_options o = {.format = {.kind = CUI_EDIT_FORMAT_PATTERN, .pattern = {"99-99", 5}},
                           .format_workspace = {workspace, sizeof workspace},
                           .clipboard = &clipboard,
                           .validate = valid};
    assert(frame(ui, edit, &state, o).error == CUI_EDIT_OK && drawn("__-__"));
    cui_focus(ui, 10);
    key(ui, CUI_KEY_V);
    cui_input_result result = frame(ui, edit, &state, o);
    assert(result.changed && result.error == CUI_EDIT_OK && !strcmp(document, "12-34") &&
           drawn("12-34"));
    size_t n;
    const cui_semantic *items = cui_semantics(ui, &n);
    assert(n == 1 && items[0].value == 5 && !items[0].details.invalid);
    assert(cui_edit_get_state(edit).cursor == 5);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_TEXT, .text_data = "5x", .text_length = 2}));
    result = frame(ui, edit, &state, o);
    assert(result.rejected && !result.changed && !strcmp(document, "12-34"));
    key(ui, CUI_KEY_Z);
    result = frame(ui, edit, &state, o);
    assert(result.changed && !document[0] && drawn("__-__"));
    key(ui, CUI_KEY_Y);
    assert(frame(ui, edit, &state, o).changed && !strcmp(document, "12-34"));
    assert(cui_edit_select(edit, 0, 5) == CUI_EDIT_OK);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_COMPOSITION_START, .composition_target = 10}));
    assert(frame(ui, edit, &state, o).error == CUI_EDIT_OK);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                                           .composition_target = 10,
                                           .text_data = "5678",
                                           .text_length = 4,
                                           .composition_anchor = 4,
                                           .composition_cursor = 4}));
    assert(frame(ui, edit, &state, o).error == CUI_EDIT_OK && !strcmp(document, "12-34") &&
           drawn("5678"));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_COMMIT,
                                           .composition_target = 10,
                                           .text_data = "5678",
                                           .text_length = 4}));
    assert(frame(ui, edit, &state, o).changed && !strcmp(document, "56-78") && drawn("56-78"));
    key(ui, CUI_KEY_Z);
    assert(frame(ui, edit, &state, o).changed && !strcmp(document, "12-34"));
    o.masked = true;
    o.content_type = CUI_TEXT_INPUT_PURPOSE_PASSWORD;
    assert(frame(ui, edit, &state, o).error == CUI_EDIT_OK && drawn("•••••") && !drawn("12-34"));
    assert(cui_edit_select(edit, 0, 5) == CUI_EDIT_OK);
    key(ui, CUI_KEY_X);
    assert(!frame(ui, edit, &state, o).changed && copies == 0 && !strcmp(document, "12-34"));
    o.masked = false;
    o.content_type = CUI_TEXT_INPUT_PURPOSE_UNSPECIFIED;
    assert(frame(ui, edit, &state, o).error == CUI_EDIT_OK);
    o.format_workspace.bytes = 4;
    unsigned calls = predicates;
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_TEXT, .text_data = "5678", .text_length = 4}));
    result = frame(ui, edit, &state, o);
    assert(result.error == CUI_EDIT_CAPACITY && !strcmp(document, "12-34") && predicates == calls);
    assert(cui_edit_get_state(edit).anchor == 0 && cui_edit_get_state(edit).cursor == 5);
    o.format_workspace.bytes = sizeof workspace;
    o.readonly = true;
    key(ui, CUI_KEY_V);
    assert(!frame(ui, edit, &state, o).changed && !strcmp(document, "12-34"));
    cui_destroy(ui);
    cui_edit_destroy(edit);
    return 0;
}
