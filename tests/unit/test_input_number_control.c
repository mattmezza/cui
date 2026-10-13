/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <string.h>
static cui_draw_list draws;
static unsigned predicates;
static bool policy(void *user, cui_str value) {
    (void)user;
    predicates++;
    return !memchr(value.data, 'x', value.length);
}
static cui_str paste(void *user) {
    (void)user;
    return cui_string("１２３４。５６７");
}
static cui_input_result frame(cui_context *ui, cui_edit *edit, cui_input_state *state,
                              cui_input_options options) {
    assert(cui_begin_frame(ui, 500, 150, .016f));
    cui_theme theme = cui_theme_light();
    cui_begin_box(ui, 1, (cui_box){.width = -1, .height = -1, .padding = 16});
    cui_input_result result =
        cui_input_ex(ui, 10, &theme, cui_string("Number"), edit, state, options);
    cui_end_box(ui);
    draws = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static bool drawn(const char *text) {
    for (size_t i = 0; i < draws.count; i++)
        if (draws.commands[i].type == CUI_DRAW_TEXT &&
            draws.commands[i].text.length == strlen(text) &&
            !memcmp(draws.commands[i].text.data, text, strlen(text)))
            return true;
    return false;
}
static void key(cui_context *ui, cui_key key) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key, .modifiers = CUI_MOD_CTRL}));
}
int main(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    char document[128] = "", workspace[128];
    cui_edit *edit = cui_edit_create((cui_edit_config){document, sizeof document, false, 4096, 16});
    assert(edit);
    cui_input_state state = {0};
    cui_platform clipboard = {.capabilities = CUI_PLATFORM_CLIPBOARD, .clipboard_get = paste};
    cui_input_options options = {.format = {.kind = CUI_EDIT_FORMAT_NUMBER,
                                            .has_separator = true,
                                            .separator = ',',
                                            .fraction_limited = true,
                                            .fraction_digits = 2},
                                 .format_workspace = {workspace, sizeof workspace},
                                 .normalization = CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF,
                                 .clipboard = &clipboard,
                                 .validate = policy};
    assert(frame(ui, edit, &state, options).error == CUI_EDIT_OK);
    cui_focus(ui, 10);
    key(ui, CUI_KEY_V);
    cui_input_result result = frame(ui, edit, &state, options);
    assert(result.changed && !strcmp(document, "1,234.56") && drawn("1,234.56") &&
           cui_edit_get_state(edit).cursor == 8);
    size_t count;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    assert(count == 1 && semantics[0].value == 8 && !semantics[0].details.invalid &&
           !semantics[0].details.secret);
    assert(cui_edit_select(edit, 0, 8) == CUI_EDIT_OK);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_COMPOSITION_START, .composition_target = 10}));
    assert(frame(ui, edit, &state, options).error == CUI_EDIT_OK);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                                           .composition_target = 10,
                                           .text_data = "５６７８",
                                           .text_length = 12,
                                           .composition_anchor = 3,
                                           .composition_cursor = 12}));
    assert(frame(ui, edit, &state, options).error == CUI_EDIT_OK && !strcmp(document, "1,234.56") &&
           drawn("5678"));
    assert(state.behavior.composition->anchor == 1 && state.behavior.composition->cursor == 4);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_COMMIT,
                                           .composition_target = 10,
                                           .text_data = "５６７８",
                                           .text_length = 12}));
    assert(frame(ui, edit, &state, options).changed && !strcmp(document, "5,678") &&
           drawn("5,678"));
    key(ui, CUI_KEY_Z);
    assert(frame(ui, edit, &state, options).changed && !strcmp(document, "1,234.56"));
    key(ui, CUI_KEY_Y);
    assert(frame(ui, edit, &state, options).changed && !strcmp(document, "5,678"));
    assert(cui_edit_select(edit, 0, 5) == CUI_EDIT_OK);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                                           .composition_target = 10,
                                           .text_data = "−。",
                                           .text_length = 6,
                                           .composition_anchor = 3,
                                           .composition_cursor = 6}));
    assert(frame(ui, edit, &state, options).error == CUI_EDIT_OK && drawn("-.") &&
           !strcmp(document, "5,678"));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_COMPOSITION_CANCEL, .composition_target = 10}));
    assert(frame(ui, edit, &state, options).error == CUI_EDIT_OK && drawn("5,678") &&
           !state.behavior.composition->active);
    options.format_workspace.bytes = 4;
    unsigned previous = predicates;
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_TEXT, .text_data = "５６７８", .text_length = 12}));
    result = frame(ui, edit, &state, options);
    assert(result.error == CUI_EDIT_CAPACITY && !strcmp(document, "5,678") &&
           predicates == previous && cui_edit_get_state(edit).anchor == 0 &&
           cui_edit_get_state(edit).cursor == 5);
    options.format_workspace.bytes = sizeof workspace;
    options.readonly = true;
    key(ui, CUI_KEY_V);
    assert(!frame(ui, edit, &state, options).changed && !strcmp(document, "5,678"));
    options.readonly = false;
    options.disabled = true;
    key(ui, CUI_KEY_V);
    assert(!frame(ui, edit, &state, options).changed && !strcmp(document, "5,678"));
    options.disabled = false;
    options.masked = true;
    options.content_type = CUI_TEXT_INPUT_PURPOSE_PASSWORD;
    char a[16], b[16], c[16];
    cui_input_mask_storage storage = {{a, b, c}, {sizeof a, sizeof b, sizeof c}};
    options.mask_storage = &storage;
    assert(frame(ui, edit, &state, options).error == CUI_EDIT_OK && drawn("•••••") &&
           !drawn("5,678"));
    cui_focus(ui, 10);
    previous = predicates;
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_TEXT, .text_data = "１２３４５６７８", .text_length = 24}));
    result = frame(ui, edit, &state, options);
    assert(result.error == CUI_EDIT_CAPACITY && !strcmp(document, "5,678") &&
           predicates == previous && drawn("•••••"));
    cui_destroy(ui);
    cui_edit_destroy(edit);
    return 0;
}
