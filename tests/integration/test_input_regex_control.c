/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_regex.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static cui_context *ui;
static cui_edit *edit;
static cui_input_state state;
static cui_regex compiled;
static cui_regex_session session;
static cui_input_options options;
static cui_input_result result;
static cui_draw_list draws;
static char document[256];
static unsigned predicates, copies;
static char paste_value[256];
static bool custom(void *p, cui_str value) {
    (void)p;
    predicates++;
    return !memchr(value.data, 'z', value.length);
}
static bool copy(void *p, cui_str value) {
    (void)p;
    (void)value;
    copies++;
    return true;
}
static cui_str paste(void *p) {
    (void)p;
    return cui_string(paste_value);
}
static cui_platform clipboard = {
    .capabilities = CUI_PLATFORM_CLIPBOARD, .clipboard_set = copy, .clipboard_get = paste};
static void frame(void) {
    assert(cui_begin_frame(ui, 500, 160, .016f));
    cui_theme t = cui_theme_light();
    cui_begin_box(ui, 1, (cui_box){.width = -1, .height = -1, .padding = 16});
    result = cui_input_ex(ui, 10, &t, cui_string("Regex"), edit, &state, options);
    cui_end_box(ui);
    draws = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static bool painted(const char *text) {
    for (size_t i = 0; i < draws.count; i++)
        if (draws.commands[i].type == CUI_DRAW_TEXT &&
            draws.commands[i].text.length == strlen(text) &&
            !memcmp(draws.commands[i].text.data, text, strlen(text)))
            return true;
    return false;
}
int regex_probe_init(void) {
    assert(!ui);
    predicates = 0;
    copies = 0;
    ui = cui_create((cui_config){0});
    assert(ui);
    document[0] = 0;
    edit = cui_edit_create((cui_edit_config){document, sizeof document, false, 4096, 32});
    assert(edit);
    assert(cui_regex_compile(cui_string("^[0-9]*$"), (cui_regex_options){0}, &compiled) ==
           CUI_REGEX_OK);
    assert(cui_regex_session_create(&compiled, 1048576, &session) == CUI_REGEX_OK);
    options = (cui_input_options){.matcher = cui_regex_matcher(&session),
                                  .validate = custom,
                                  .clipboard = &clipboard,
                                  .normalization = CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF};
    frame();
    cui_focus(ui, 10);
    frame();
    return 0;
}
int regex_probe_event(int kind, const char *text, size_t length, size_t anchor, size_t cursor) {
    static const cui_event_type types[] = {
        CUI_EVENT_TEXT, CUI_EVENT_COMPOSITION_START, CUI_EVENT_COMPOSITION_UPDATE,
        CUI_EVENT_COMPOSITION_COMMIT, CUI_EVENT_COMPOSITION_CANCEL};
    assert(kind >= 0 && kind < 5);
    assert(cui_queue_event(ui, (cui_event){.type = types[kind],
                                           .text_data = text,
                                           .text_length = length,
                                           .composition_target = kind ? 10 : 0,
                                           .composition_anchor = anchor,
                                           .composition_cursor = cursor}));
    frame();
    return result.error;
}
int regex_probe_key(int which) {
    static const cui_key keys[] = {CUI_KEY_Z,         CUI_KEY_Y,      CUI_KEY_X, CUI_KEY_V,
                                   CUI_KEY_BACKSPACE, CUI_KEY_DELETE, CUI_KEY_A};
    assert(which >= 0 && which < 7);
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                        .key = keys[which],
                                        .modifiers = which == 4 || which == 5 ? 0 : CUI_MOD_CTRL}));
    frame();
    return result.error;
}
const char *regex_probe_text(void) {
    return document;
}
size_t regex_probe_length(void) {
    return strlen(document);
}
unsigned regex_probe_flags(void) {
    return (result.changed ? 1u : 0u) | (result.rejected ? 2u : 0u) |
           (state.behavior.composition && state.behavior.composition->active ? 4u : 0u);
}
unsigned regex_probe_predicates(void) {
    return predicates;
}
unsigned regex_probe_copies(void) {
    return copies;
}
int regex_probe_painted(const char *p) {
    return painted(p);
}
void regex_probe_readonly(int value) {
    options.readonly = value != 0;
    frame();
}
void regex_probe_masked(int value) {
    options.masked = value != 0;
    options.content_type =
        value ? CUI_TEXT_INPUT_PURPOSE_PASSWORD : CUI_TEXT_INPUT_PURPOSE_UNSPECIFIED;
    frame();
}
void regex_probe_seed(const char *p) {
    assert(cui_edit_select(edit, 0, strlen(document)) == CUI_EDIT_OK);
    assert(cui_edit_insert(edit, cui_string(p)) == CUI_EDIT_OK);
    frame();
}
int regex_probe_pattern(const char *p) {
    cui_regex next = {0};
    int error = cui_regex_compile(cui_string(p), (cui_regex_options){0}, &next);
    if (error)
        return error;
    error = cui_regex_session_rebind(&session, &next, 1048576);
    assert(cui_regex_destroy(&next) == CUI_REGEX_OK);
    if (!error)
        options.matcher = cui_regex_matcher(&session);
    return error;
}
void regex_probe_select_all(void) {
    assert(cui_edit_select(edit, 0, strlen(document)) == CUI_EDIT_OK);
}
unsigned regex_probe_config(int field) {
    cui_text_input_state focused = {0};
    (void)cui_get_text_input_config(ui, &focused);
    switch (field) {
    case 0:
        return (unsigned)focused.owner;
    case 1:
        return (unsigned)focused.config.type;
    case 2:
        return (unsigned)focused.config.purpose;
    case 3:
        return focused.multiline;
    default:
        assert(false);
        return 0;
    }
}
uint64_t regex_probe_generation(void) {
    cui_text_input_state focused = {0};
    (void)cui_get_text_input_config(ui, &focused);
    return focused.generation;
}
void *regex_probe_alloc(size_t n) {
    return malloc(n);
}
void regex_probe_free(void *p) {
    free(p);
}
void regex_probe_destroy(void) {
    cui_destroy(ui);
    cui_edit_destroy(edit);
    assert(cui_regex_session_destroy(&session) == CUI_REGEX_OK);
    assert(cui_regex_destroy(&compiled) == CUI_REGEX_OK);
    ui = NULL;
    state = (cui_input_state){0};
}
int regex_probe_run(void) {
    regex_probe_init();
    assert(regex_probe_event(0, "12", 2, 0, 0) == CUI_EDIT_OK && result.changed && painted("12"));
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    assert(n == 1 && sem[0].value == 2 && !sem[0].details.invalid);
    assert(regex_probe_event(0, "x", 1, 0, 0) == CUI_EDIT_OK && result.rejected &&
           !result.changed && !strcmp(document, "12"));
    strcpy(paste_value, "34");
    assert(regex_probe_key(3) == CUI_EDIT_OK && !strcmp(document, "1234"));
    regex_probe_select_all();
    assert(regex_probe_key(2) == CUI_EDIT_OK && copies == 1 && !document[0]);
    assert(regex_probe_key(0) == CUI_EDIT_OK && !strcmp(document, "1234"));
    assert(regex_probe_key(1) == CUI_EDIT_OK && !document[0]);
    regex_probe_seed("bad");
    assert(regex_probe_event(0, "x", 1, 0, 0) == CUI_EDIT_OK && !result.rejected &&
           !strcmp(document, "badx"));
    regex_probe_seed("12");
    regex_probe_select_all();
    assert(regex_probe_event(1, NULL, 0, 0, 0) == CUI_EDIT_OK);
    assert(regex_probe_event(2, "56", 2, 2, 2) == CUI_EDIT_OK && !strcmp(document, "12") &&
           painted("56"));
    assert(regex_probe_event(2, "x", 1, 1, 1) == CUI_EDIT_OK && result.rejected && painted("56"));
    assert(regex_probe_event(3, "56", 2, 2, 2) == CUI_EDIT_OK && result.changed &&
           !strcmp(document, "56"));
    assert(regex_probe_key(0) == CUI_EDIT_OK && !strcmp(document, "12"));
    regex_probe_select_all();
    assert(regex_probe_event(1, NULL, 0, 0, 0) == CUI_EDIT_OK);
    assert(regex_probe_event(3, "x", 1, 1, 1) == CUI_EDIT_OK && result.rejected &&
           !state.behavior.composition->active && !strcmp(document, "12"));
    assert(regex_probe_event(0, "7\r\n", 3, 0, 0) == CUI_EDIT_OK && !strcmp(document, "7"));
    regex_probe_readonly(1);
    assert(regex_probe_event(0, "8", 1, 0, 0) == CUI_EDIT_OK && !result.changed &&
           !strcmp(document, "7"));
    regex_probe_readonly(0);
    regex_probe_masked(1);
    assert(painted("•") && !painted("7"));
    regex_probe_select_all();
    assert(regex_probe_key(2) == CUI_EDIT_OK && copies == 1 && !strcmp(document, "7"));
    regex_probe_masked(0);
    assert(regex_probe_pattern("^[\\p{Greek}]*$") == CUI_REGEX_OK);
    regex_probe_seed("");
    assert(regex_probe_event(0, "ΔΣ", 4, 0, 0) == CUI_EDIT_OK && !strcmp(document, "ΔΣ"));
    assert(regex_probe_pattern("[") == CUI_REGEX_PATTERN);
    assert(regex_probe_event(0, "x", 1, 0, 0) == CUI_EDIT_OK && result.rejected &&
           !strcmp(document, "ΔΣ"));
    /* Stale descriptor fails hard before custom acceptance. */
    cui_text_match stale = options.matcher;
    assert(cui_regex_session_rebind(&session, &compiled, 1048576) == CUI_REGEX_OK);
    options.matcher = stale;
    unsigned calls = predicates;
    assert(regex_probe_event(0, "9", 1, 0, 0) == CUI_EDIT_MATCHER_STALE && predicates == calls &&
           !strcmp(document, "ΔΣ"));
    regex_probe_destroy();
    return 0;
}
#ifndef CUI_REGEX_BROWSER_PROBE
int main(void) {
    return regex_probe_run();
}
#endif
