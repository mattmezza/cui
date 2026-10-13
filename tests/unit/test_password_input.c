/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static unsigned copies, allocations, releases, predicates;
static bool policy(void *user, cui_str value) {
    (void)user;
    predicates++;
    return !memchr(value.data, 'X', value.length);
}
static void semantics(cui_context *ui, bool secret, bool password) {
    size_t count;
    const cui_semantic *items = cui_semantics(ui, &count);
    bool found = false;
    for (size_t i = 0; i < count; i++)
        if (items[i].id == 10) {
            assert(items[i].details.secret == secret && items[i].details.password == password);
            if (secret)
                assert(items[i].value == 0);
            found = true;
        }
    assert(found);
}
static bool fail_allocate;
static bool copy(void *u, cui_str value) {
    (void)u;
    (void)value;
    copies++;
    return true;
}
static void *allocate(void *u, size_t n) {
    (void)u;
    allocations++;
    return fail_allocate ? NULL : malloc(n);
}
static void release(void *u, void *p) {
    (void)u;
    releases++;
    free(p);
}
static cui_draw_list painted;
static cui_input_result frame(cui_context *ui, cui_edit *e, cui_input_state *s,
                              cui_input_options o) {
    assert(cui_begin_frame(ui, 500, 150, .016f));
    cui_theme t = cui_theme_light();
    cui_begin_box(ui, 1, (cui_box){.width = -1, .height = -1, .padding = 16});
    cui_input_result r = cui_input_ex(ui, 10, &t, cui_string("Secret"), e, s, o);
    cui_button(ui, 20, &t, cui_string("Other"), false);
    cui_end_box(ui);
    painted = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return r;
}
static void key(cui_context *ui, cui_key k, unsigned mods) {
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k, .modifiers = mods}));
}
static void text(cui_context *ui, const char *p) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_TEXT, .text_data = p, .text_length = strlen(p)}));
}
static void secret_draw(void) {
    bool found = false;
    for (size_t i = 0; i < painted.count; i++)
        if (painted.commands[i].type == CUI_DRAW_TEXT) {
            cui_str t = painted.commands[i].text;
            for (size_t j = 0; j + 7 <= t.length; j++)
                assert(memcmp(t.data + j, "private", 7));
            if (t.length && (unsigned char)t.data[0] == 0xe2)
                found = true;
        }
    assert(found);
}
int main(void) {
    char buffer[128] = "private words";
    cui_edit *e = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                    .capacity = sizeof buffer,
                                                    .history_bytes = 4096,
                                                    .max_history_steps = 16});
    cui_context *ui = cui_create((cui_config){0});
    assert(e && ui);
    cui_input_state state = {0};
    cui_platform platform = {.capabilities = CUI_PLATFORM_CLIPBOARD, .clipboard_set = copy};
    cui_input_options o = {.masked = true,
                           .content_type = CUI_TEXT_INPUT_PURPOSE_PASSWORD,
                           .clipboard = &platform,
                           .mask_allocator = {.allocate = allocate, .release = release}};
    assert(frame(ui, e, &state, o).error == CUI_EDIT_OK);
    secret_draw();
    semantics(ui, true, true);
    cui_focus(ui, 10);
    frame(ui, e, &state, o);
    cui_text_input_state config;
    assert(cui_get_text_input_config(ui, &config) && config.owner == 10 && config.explicit_config &&
           config.config.type == CUI_TEXT_INPUT_PASSWORD_HIDDEN);
    unsigned idle = allocations;
    frame(ui, e, &state, o);
    assert(allocations == idle);
    assert(cui_edit_select(e, 0, 13) == CUI_EDIT_OK);
    key(ui, CUI_KEY_C, CUI_MOD_CTRL);
    key(ui, CUI_KEY_X, CUI_MOD_CTRL);
    assert(!frame(ui, e, &state, o).changed && !strcmp(buffer, "private words") && copies == 0);
    assert(cui_edit_select(e, 13, 13) == CUI_EDIT_OK);
    key(ui, CUI_KEY_LEFT, CUI_MOD_CTRL);
    frame(ui, e, &state, o);
    assert(cui_edit_get_state(e).cursor == 0);
    key(ui, CUI_KEY_RIGHT, CUI_MOD_CTRL | CUI_MOD_SHIFT);
    frame(ui, e, &state, o);
    assert(cui_edit_get_state(e).anchor == 0 && cui_edit_get_state(e).cursor == 13);
    fail_allocate = true;
    text(ui, "longer candidate value");
    cui_edit_state before = cui_edit_get_state(e);
    assert(frame(ui, e, &state, o).error == CUI_EDIT_CAPACITY && !strcmp(buffer, "private words"));
    assert(cui_edit_get_state(e).anchor == before.anchor &&
           cui_edit_get_state(e).cursor == before.cursor);
    secret_draw();
    fail_allocate = false;
    text(ui, "a\xC3\xA9\xF0\x9F\x98\x80");
    assert(frame(ui, e, &state, o).changed);
    secret_draw();
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    assert(frame(ui, e, &state, o).changed && !strcmp(buffer, "private words"));
    char a[40] = {0}, b[40] = {0}, c[40] = {0};
    cui_input_mask_storage fixed = {.buffers = {a, b, c},
                                    .capacities = {sizeof a, sizeof b, sizeof c}};
    o.validate = policy;
    o.mask_storage = &fixed;
    o.mask_allocator = (cui_input_mask_allocator){0};
    assert(frame(ui, e, &state, o).error == CUI_EDIT_OK);
    assert(cui_edit_select(e, 13, 13) == CUI_EDIT_OK);
    unsigned calls = predicates;
    text(ui, "extra");
    before = cui_edit_get_state(e);
    assert(frame(ui, e, &state, o).error == CUI_EDIT_CAPACITY && !strcmp(buffer, "private words"));
    assert(cui_edit_get_state(e).revision == before.revision &&
           cui_edit_get_state(e).cursor == before.cursor);
    assert(predicates == calls);
    assert(cui_edit_select(e, 0, 13) == CUI_EDIT_OK);
    text(ui, "X");
    cui_input_result rejected = frame(ui, e, &state, o);
    assert(rejected.rejected && !rejected.changed && !strcmp(buffer, "private words") &&
           predicates == calls + 2);
    assert(cui_edit_get_state(e).anchor == 0 && cui_edit_get_state(e).cursor == 13);
    secret_draw();
    assert(cui_edit_select(e, 0, 13) == CUI_EDIT_OK);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_COMPOSITION_START, .composition_target = 10}));
    frame(ui, e, &state, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                                           .composition_target = 10,
                                           .text_data = "😀é",
                                           .text_length = 6,
                                           .composition_anchor = 4,
                                           .composition_cursor = 6}));
    assert(frame(ui, e, &state, o).error == CUI_EDIT_OK && !strcmp(buffer, "private words"));
    secret_draw();
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                                           .composition_target = 10,
                                           .text_data = "more than thirteen scalars",
                                           .text_length = 25,
                                           .composition_anchor = 0,
                                           .composition_cursor = 25}));
    assert(frame(ui, e, &state, o).error == CUI_EDIT_CAPACITY && !strcmp(buffer, "private words"));
    assert(state.behavior.composition->length == 6);
    secret_draw();
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_COMMIT,
                                           .composition_target = 10,
                                           .text_data = "😀é",
                                           .text_length = 6}));
    assert(frame(ui, e, &state, o).changed && !strcmp(buffer, "😀é"));
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    assert(frame(ui, e, &state, o).changed && !strcmp(buffer, "private words"));
    cui_input_mask_storage alias = {.buffers = {buffer, b, c},
                                    .capacities = {sizeof buffer, sizeof b, sizeof c}};
    o.mask_storage = &alias;
    before = cui_edit_get_state(e);
    assert(frame(ui, e, &state, o).error == CUI_EDIT_INVALID_ARGUMENT &&
           !strcmp(buffer, "private words"));
    assert(cui_edit_get_state(e).revision == before.revision &&
           cui_edit_get_state(e).anchor == before.anchor &&
           cui_edit_get_state(e).cursor == before.cursor);
    o.mask_storage = NULL;
    o.masked = false;
    cui_focus(ui, 10);
    frame(ui, e, &state, o);
    semantics(ui, true, true);
    assert(cui_get_text_input_config(ui, &config) &&
           config.config.type == CUI_TEXT_INPUT_PASSWORD_VISIBLE);
    o.readonly = true;
    frame(ui, e, &state, o);
    assert(!cui_get_text_input_config(ui, &config));
    o.readonly = false;
    o.content_type = CUI_TEXT_INPUT_PURPOSE_UNSPECIFIED;
    frame(ui, e, &state, o);
    assert(cui_get_text_input_config(ui, &config) && config.config.type == CUI_TEXT_INPUT_TEXT);
    cui_focus(ui, 20);
    assert(!cui_get_text_input_config(ui, &config));
    cui_destroy(ui);
    cui_edit_destroy(e);
    assert(releases > 0);
    return 0;
}
