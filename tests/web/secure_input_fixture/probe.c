/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <string.h>
__attribute__((import_module("host"), import_name("measure"))) extern float
host_measure(const char *, size_t, float);
static cui_size measure(void *user, cui_str text, float size) {
    (void)user;
    return (cui_size){host_measure(text.data, text.length, size), size};
}
static cui_context *ui;
static cui_edit *edit;
static cui_input_state state;
static char buffer[512];
static cui_input_options options;
static cui_draw_list list;
static cui_input_result last_result;
static char masks[3][32];
static cui_input_mask_storage fixed = {.buffers = {masks[0], masks[1], masks[2]},
                                       .capacities = {32, 32, 32}};
int probe_init(void) {
    ui = cui_create((cui_config){.measure = measure});
    edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                             .capacity = sizeof buffer,
                                             .history_bytes = 8192,
                                             .max_history_steps = 32});
    options = (cui_input_options){.masked = true,
                                  .content_type = CUI_TEXT_INPUT_PURPOSE_PASSWORD,
                                  .normalization = CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF};
    return ui && edit;
}
void probe_frame(void) {
    cui_begin_frame(ui, 500, 150, .016f);
    cui_theme t = cui_theme_light();
    cui_begin_box(ui, 1, (cui_box){.width = -1, .height = -1, .padding = 16});
    last_result = cui_input_ex(ui, 10, &t, cui_string("Password"), edit, &state, options);
    cui_end_box(ui);
    list = cui_end_frame(ui);
}
void probe_focus(int value) {
    cui_focus(ui, value ? 10 : 0);
}
void probe_options(int masked, int purpose, int readonly, int disabled) {
    options.masked = masked != 0;
    options.content_type = (cui_text_input_purpose)purpose;
    options.readonly = readonly != 0;
    options.disabled = disabled != 0;
}
int probe_config(int field) {
    cui_text_input_state s;
    if (!cui_get_text_input_config(ui, &s))
        return 0;
    switch (field) {
    case 0:
        return (int)s.owner;
    case 1:
        return s.config.type;
    case 2:
        return s.config.purpose;
    case 4:
        return s.multiline;
    default:
        return (int)s.generation;
    }
}
int probe_text(const char *p, size_t n) {
    return cui_queue_event(ui,
                           (cui_event){.type = CUI_EVENT_TEXT, .text_data = p, .text_length = n});
}
int probe_composition(int kind, const char *p, size_t n, size_t anchor, size_t cursor) {
    cui_event_type types[] = {CUI_EVENT_COMPOSITION_START, CUI_EVENT_COMPOSITION_UPDATE,
                              CUI_EVENT_COMPOSITION_COMMIT, CUI_EVENT_COMPOSITION_CANCEL};
    if (kind < 0 || kind > 3)
        return 0;
    return cui_queue_event(ui, (cui_event){.type = types[kind],
                                           .text_data = p,
                                           .text_length = n,
                                           .composition_anchor = anchor,
                                           .composition_cursor = cursor,
                                           .composition_target = 10});
}
int probe_key(int key, unsigned mods) {
    return cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = (cui_key)key, .modifiers = mods});
}
const char *probe_value(void) {
    return buffer;
}
size_t probe_length(void) {
    return cui_edit_get_text(edit).length;
}
const char *probe_draw(size_t i) {
    return i < list.count ? list.commands[i].text.data : 0;
}
size_t probe_draw_length(size_t i) {
    return i < list.count ? list.commands[i].text.length : 0;
}
size_t probe_count(void) {
    return list.count;
}
int probe_active(void) {
    return state.behavior.composition && state.behavior.composition->active;
}
int probe_key_id(int id) {
    cui_key keys[] = {CUI_KEY_A,         CUI_KEY_Z, CUI_KEY_LEFT, CUI_KEY_RIGHT,
                      CUI_KEY_BACKSPACE, CUI_KEY_C, CUI_KEY_X};
    return id >= 0 && id < 7 ? keys[id] : CUI_KEY_NONE;
}

void probe_multiline(int multiline) {
    if (state.behavior.composition)
        (void)cui_edit_composition_event(edit, state.behavior.composition,
                                         (cui_event){.type = CUI_EVENT_COMPOSITION_CANCEL}, false);
    cui_edit_destroy(edit);
    buffer[0] = 0;
    state = (cui_input_state){0};
    edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                             .capacity = sizeof buffer,
                                             .multiline = multiline != 0,
                                             .history_bytes = 8192,
                                             .max_history_steps = 32});
    options = (cui_input_options){0};
}

int probe_selection(size_t anchor, size_t cursor) {
    return cui_edit_select(edit, anchor, cursor);
}

void probe_fixed(int enabled) {
    options.mask_storage = enabled ? &fixed : NULL;
}
int probe_error(void) {
    return last_result.error;
}
int probe_capacity_error(void) {
    return CUI_EDIT_CAPACITY;
}

float probe_command(size_t i, int field) {
    if (i >= list.count)
        return 0;
    const cui_draw_command *c = &list.commands[i];
    switch (field) {
    case 0:
        return c->type;
    case 1:
        return c->bounds.x;
    case 2:
        return c->bounds.y;
    case 3:
        return c->bounds.w;
    case 4:
        return c->bounds.h;
    case 5:
        return c->font_size;
    case 6:
        return c->stroke;
    case 7:
        return c->color.r;
    case 8:
        return c->color.g;
    case 9:
        return c->color.b;
    default:
        return c->color.a;
    }
}
