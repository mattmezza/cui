/* SPDX-License-Identifier: MIT */
/* Private same-TU localized kind55 adapter; no foreign callbacks in submission. */
#ifndef CUI_BRIDGE_TIME_INTERNAL_H
#define CUI_BRIDGE_TIME_INTERNAL_H
#include "cui/cui_time_field_v1.h"

_Static_assert(sizeof(cui_bridge_time_token_v1) == 20, "Time token ABI");
_Static_assert(sizeof(cui_bridge_time_name_v1) == 68, "Time name ABI");
_Static_assert(sizeof(cui_bridge_time_field_config_v1) == 616, "Time config ABI");
_Static_assert(sizeof(cui_bridge_time_field_result_v1) == 80, "Time result ABI");
_Static_assert(sizeof(cui_bridge_time_field_change_v1) == 64, "Time change ABI");
_Static_assert(offsetof(cui_bridge_time_field_config_v1, edit_generation) == 56, "Time generation");
_Static_assert(offsetof(cui_bridge_time_field_config_v1, tokens) == 64, "Time tokens");
_Static_assert(offsetof(cui_bridge_time_field_config_v1, names) == 344, "Time names");
_Static_assert(offsetof(cui_bridge_time_field_change_v1, frame_sequence) == 24, "Time frame");
_Static_assert(offsetof(cui_bridge_time_field_result_v1, previous) == 48, "Time previous");
typedef struct {
    cui_bridge_time_field_config_v1 config;
    cui_time_field_state_v1 state;
    cui_bridge_time_field_result_v1 result;
    uint64_t last_seen;
    bool seen, admitted, observed;
    cui_bridge_app *app;
    uint32_t slot, generation;
} bridge_time_owned;
struct bridge_time_app {
    const void *owner;
    bridge_time_owned **nodes;
    cui_bridge_time_field_change_v1 *queue;
    uint32_t capacity, head, count, configured, staged;
    bool active, failed_frame;
};
static cui_bridge_status bridge_time_thread_check(cui_bridge_app *app) {
    return app->time_fields && app->time_fields->owner != bridge_slider_owner()
               ? CUI_BRIDGE_INVALID_STATE
               : CUI_BRIDGE_OK;
}
static cui_bridge_status bridge_time_ready(cui_bridge_app *app) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!BRIDGE_SLIDER_SUPPORTED)
        return CUI_BRIDGE_BACKEND_UNAVAILABLE;
    if (cui_frame_active(app->ui))
        return CUI_BRIDGE_INVALID_STATE;
    return CUI_BRIDGE_OK;
}
static cui_bridge_status bridge_time_get(cui_bridge_app *app, cui_bridge_node handle,
                                         bridge_time_owned **out) {
    cui_bridge_status status = bridge_time_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!app->time_fields)
        return CUI_BRIDGE_INVALID_STATE;
    cui_bridge_node_record *node = bridge_node(app, handle);
    if (!node)
        return CUI_BRIDGE_STALE_HANDLE;
    if (node->kind != CUI_BRIDGE_TIME_FIELD || !app->time_fields->nodes[handle.slot])
        return CUI_BRIDGE_INVALID_STATE;
    *out = app->time_fields->nodes[handle.slot];
    return CUI_BRIDGE_OK;
}
static cui_time bridge_time_value(cui_bridge_time t) {
    return (cui_time){t.hour, t.minute, t.second};
}
static cui_bridge_time bridge_time_copy(cui_time t) {
    return (cui_bridge_time){t.hour, t.minute, t.second};
}
static bool bridge_time_zero(cui_bridge_time t) {
    return !t.hour && !t.minute && !t.second;
}
static uint32_t bridge_time_status(cui_time_text_status_v1 s) {
    switch (s) {
    case CUI_TIME_TEXT_OK: return CUI_BRIDGE_TIME_OK_V1;
    case CUI_TIME_TEXT_INVALID_ARGUMENT: return CUI_BRIDGE_TIME_INVALID_ARGUMENT_V1;
    case CUI_TIME_TEXT_INVALID_CONFIG: return CUI_BRIDGE_TIME_INVALID_CONFIG_V1;
    case CUI_TIME_TEXT_INVALID_SYNTAX: return CUI_BRIDGE_TIME_INVALID_SYNTAX_V1;
    case CUI_TIME_TEXT_INVALID_VALUE: return CUI_BRIDGE_TIME_INVALID_VALUE_V1;
    case CUI_TIME_TEXT_OUT_OF_RANGE: return CUI_BRIDGE_TIME_OUT_OF_RANGE_V1;
    case CUI_TIME_TEXT_BUFFER_TOO_SMALL: return CUI_BRIDGE_TIME_BUFFER_TOO_SMALL_V1;
    }
    return UINT32_MAX;
}
static cui_bridge_status bridge_time_bytes(const uint8_t *bytes, uint32_t length,
                                            uint32_t capacity, bool empty) {
    if (length > capacity || (!length && !empty))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    for (uint32_t i = length; i < capacity; i++)
        if (bytes[i])
            return CUI_BRIDGE_INVALID_ARGUMENT;
    if (memchr(bytes, 0, length))
        return CUI_BRIDGE_INVALID_UTF8;
    char copy[65];
    memcpy(copy, bytes, length);
    copy[length] = 0;
    size_t found = 0;
    return bridge_utf8_limit(copy, capacity, &found);
}
/* Views borrow only the immutable owned/candidate config during this call. */
static void bridge_time_options(const cui_bridge_time_field_config_v1 *c,
                                  cui_time_locale_v1 *locale, cui_time_field_options_v1 *o) {
    cui_time_field_options_init_v1(o);
    cui_time_locale_init_v1(locale);
    for (unsigned i = 0; i < 10; i++)
        locale->digits[i] = (cui_str){(const char *)c->tokens[i].bytes, c->tokens[i].length};
    locale->separator = (cui_str){(const char *)c->tokens[10].bytes, c->tokens[10].length};
    locale->period_separator = (cui_str){(const char *)c->tokens[11].bytes, c->tokens[11].length};
    locale->am = (cui_str){(const char *)c->tokens[12].bytes, c->tokens[12].length};
    locale->pm = (cui_str){(const char *)c->tokens[13].bytes, c->tokens[13].length};
    if (c->flags & CUI_BRIDGE_TIME_FIELD_DISABLED_V1) o->flags |= CUI_TIME_FIELD_DISABLED_V1;
    if (c->flags & CUI_BRIDGE_TIME_FIELD_READONLY_V1) o->flags |= CUI_TIME_FIELD_READONLY_V1;
    if (c->flags & CUI_BRIDGE_TIME_FIELD_OWNER_INVALID_V1) o->flags |= CUI_TIME_FIELD_OWNER_INVALID_V1;
    o->text.cycle = c->cycle == CUI_BRIDGE_TIME_H12_V1 ? CUI_TIME_TEXT_H12 : CUI_TIME_TEXT_H23;
    o->text.precision = c->precision == CUI_BRIDGE_TIME_SECOND_V1 ? CUI_TIME_TEXT_SECOND : CUI_TIME_TEXT_MINUTE;
    if (c->bounds_flags & CUI_BRIDGE_TIME_HAS_MINIMUM_V1) o->text.flags |= CUI_TIME_TEXT_HAS_MINIMUM_V1;
    if (c->bounds_flags & CUI_BRIDGE_TIME_HAS_MAXIMUM_V1) o->text.flags |= CUI_TIME_TEXT_HAS_MAXIMUM_V1;
    o->text.minimum = bridge_time_value(c->minimum);
    o->text.maximum = bridge_time_value(c->maximum);
    o->text.locale = locale;
    o->edit_generation = c->edit_generation;
    for (unsigned i = 0; i < 4; i++)
        o->segment_names[i] = (cui_str){(const char *)c->names[i].bytes, c->names[i].length};
}
static cui_bridge_status bridge_time_config(const cui_bridge_time_field_config_v1 *c) {
    if ((c->flags & ~7u) || c->cycle > 1 || c->precision > 1 || (c->bounds_flags & ~3u) ||
        c->reserved[0] || c->reserved[1] || !c->edit_generation ||
        (!(c->bounds_flags & CUI_BRIDGE_TIME_HAS_MINIMUM_V1) && !bridge_time_zero(c->minimum)) ||
        (!(c->bounds_flags & CUI_BRIDGE_TIME_HAS_MAXIMUM_V1) && !bridge_time_zero(c->maximum)))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    for (unsigned i = 0; i < 14; i++) {
        cui_bridge_status s = bridge_time_bytes(c->tokens[i].bytes, c->tokens[i].length, 16, false);
        if (s != CUI_BRIDGE_OK) return s;
    }
    for (unsigned i = 0; i < 4; i++) {
        cui_bridge_status s = bridge_time_bytes(c->names[i].bytes, c->names[i].length, 64, true);
        if (s != CUI_BRIDGE_OK) return s;
    }
    cui_time_locale_v1 locale;
    cui_time_field_options_v1 options;
    bridge_time_options(c, &locale, &options);
    cui_time_text_status_v1 s = cui_time_validate_v1((cui_time){0}, &options.text);
    return s == CUI_TIME_TEXT_OK || s == CUI_TIME_TEXT_OUT_OF_RANGE
               ? CUI_BRIDGE_OK : CUI_BRIDGE_INVALID_ARGUMENT;
}
uint32_t cui_bridge_time_field_versions(void) { return BRIDGE_SLIDER_SUPPORTED ? 1u : 0u; }
uint32_t cui_bridge_time_field_config_size_v1(void) { return sizeof(cui_bridge_time_field_config_v1); }
uint32_t cui_bridge_time_field_result_size_v1(void) { return sizeof(cui_bridge_time_field_result_v1); }
uint32_t cui_bridge_time_field_change_size_v1(void) { return sizeof(cui_bridge_time_field_change_v1); }
cui_bridge_status cui_bridge_time_field_enable_v1(cui_bridge_app *app, uint32_t requested) {
    if (requested > 4096) return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_status s = bridge_time_ready(app);
    if (s != CUI_BRIDGE_OK) return s;
    uint32_t capacity = requested ? requested : 4096;
    if (app->time_fields)
        return capacity == app->time_fields->capacity ? CUI_BRIDGE_OK : CUI_BRIDGE_INVALID_STATE;
    uint64_t pointers = ((uint64_t)app->max_nodes + 1) * sizeof(bridge_time_owned *);
    uint64_t offset = sizeof(bridge_time_app) + pointers;
    uint64_t alignment = _Alignof(cui_bridge_time_field_change_v1);
    offset = (offset + alignment - 1) / alignment * alignment;
    uint64_t bytes = offset + (uint64_t)capacity * sizeof(cui_bridge_time_field_change_v1);
#if SIZE_MAX < UINT64_MAX
    if (bytes > SIZE_MAX) return CUI_BRIDGE_CAPACITY;
#endif
    bridge_time_app *state = calloc(1, (size_t)bytes);
    if (!state) return CUI_BRIDGE_OUT_OF_MEMORY;
    state->owner = bridge_slider_owner();
    state->nodes = (bridge_time_owned **)(state + 1);
    state->queue = (cui_bridge_time_field_change_v1 *)((unsigned char *)state + (size_t)offset);
    state->capacity = capacity;
    app->time_fields = state;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_time_field_configure_v1(cui_bridge_app *app, cui_bridge_node handle,
                                                     const cui_bridge_time_field_config_v1 *c) {
    if (!c || c->struct_size != sizeof *c || c->version != 1) return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_status s = bridge_time_ready(app);
    if (s != CUI_BRIDGE_OK) return s;
    if (!app->time_fields) return CUI_BRIDGE_INVALID_STATE;
    cui_bridge_node_record *node = bridge_node(app, handle);
    if (!node) return CUI_BRIDGE_STALE_HANDLE;
    if (node->kind != CUI_BRIDGE_TIME_FIELD) return CUI_BRIDGE_INVALID_STATE;
    s = bridge_time_config(c);
    if (s != CUI_BRIDGE_OK) return s;
    bridge_time_owned *owned = app->time_fields->nodes[handle.slot];
    if (!owned) {
        if (app->time_fields->configured == app->time_fields->capacity) return CUI_BRIDGE_CAPACITY;
        owned = calloc(1, sizeof *owned);
        if (!owned) return CUI_BRIDGE_OUT_OF_MEMORY;
        cui_time_locale_v1 locale;
        cui_time_field_options_v1 options;
        bridge_time_options(c, &locale, &options);
        /* Config is independent of the existing owner's domain/precision. */
        if (cui_time_field_state_init_v1(&owned->state, (cui_time){0}, &options) != CUI_TIME_TEXT_OK) {
            free(owned);
            return CUI_BRIDGE_INVALID_ARGUMENT;
        }
        owned->app = app;
        owned->slot = handle.slot;
        owned->generation = handle.generation;
        app->time_fields->nodes[handle.slot] = owned;
        app->time_fields->configured++;
    }
    owned->config = *c;
    owned->observed = false;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_time_field_get_config_v1(cui_bridge_app *app, cui_bridge_node handle,
                                                      cui_bridge_time_field_config_v1 *out) {
    if (!out || out->struct_size != sizeof *out || out->version != 1) return CUI_BRIDGE_INVALID_ARGUMENT;
    bridge_time_owned *owned;
    cui_bridge_status s = bridge_time_get(app, handle, &owned);
    if (s == CUI_BRIDGE_OK) *out = owned->config;
    return s;
}
cui_bridge_status cui_bridge_time_field_get_result_v1(cui_bridge_app *app, cui_bridge_node handle,
                                                      cui_bridge_time_field_result_v1 *out) {
    if (!out || out->struct_size != sizeof *out || out->version != 1) return CUI_BRIDGE_INVALID_ARGUMENT;
    bridge_time_owned *owned;
    cui_bridge_status s = bridge_time_get(app, handle, &owned);
    if (s != CUI_BRIDGE_OK) return s;
    if (app->time_fields->failed_frame) return CUI_BRIDGE_CORE_ERROR;
    if (!owned->observed) return CUI_BRIDGE_INVALID_STATE;
    *out = owned->result;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_time_field_clear_validation_v1(cui_bridge_app *app, cui_bridge_node handle) {
    bridge_time_owned *owned;
    cui_bridge_status s = bridge_time_get(app, handle, &owned);
    if (s != CUI_BRIDGE_OK) return s;
    return cui_time_field_clear_validation_v1(&owned->state) == CUI_TIME_TEXT_OK
               ? CUI_BRIDGE_OK : CUI_BRIDGE_INVALID_STATE;
}
cui_bridge_status cui_bridge_time_field_poll_change_v1(cui_bridge_app *app,
                                                       cui_bridge_time_field_change_v1 *out) {
    if (!out || out->struct_size != sizeof *out || out->version != 1) return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_status s = bridge_time_ready(app);
    if (s != CUI_BRIDGE_OK) return s;
    bridge_time_app *state = app->time_fields;
    if (!state) return CUI_BRIDGE_INVALID_STATE;
    while (state->count) {
        cui_bridge_time_field_change_v1 copy = state->queue[state->head];
        bool live = bridge_node(app, copy.node) != NULL;
        state->head = (state->head + 1) % state->capacity;
        state->count--;
        if (live) { *out = copy; return CUI_BRIDGE_OK; }
    }
    return CUI_BRIDGE_EMPTY;
}
static void bridge_time_remove(cui_bridge_app *app, uint32_t slot) {
    if (!app || !app->time_fields || !slot || slot > app->max_nodes ||
        !app->time_fields->nodes[slot]) return;
    free(app->time_fields->nodes[slot]);
    app->time_fields->nodes[slot] = NULL;
    app->time_fields->configured--;
}
static cui_bridge_status bridge_time_preflight(cui_bridge_app *app) {
    bridge_time_app *state = app->time_fields;
    if (!state) return CUI_BRIDGE_OK;
    if (app->last_core_frame == UINT64_MAX || state->configured > state->capacity - state->count)
        return CUI_BRIDGE_CAPACITY;
    state->staged = 0;
    state->active = true;
    for (uint32_t i = 1; i <= app->max_nodes; i++)
        if (state->nodes[i]) {
            state->nodes[i]->seen = false;
            state->nodes[i]->admitted = false;
        }
    return CUI_BRIDGE_OK;
}
static void bridge_time_observe(void *user, const cui_time_change *event) {
    bridge_time_owned *owned = user;
    bridge_time_app *state = owned->app->time_fields;
    if (state->staged >= state->configured || state->count + state->staged >= state->capacity) {
        owned->app->frame_error = CUI_BRIDGE_CORE_ERROR;
        return;
    }
    uint32_t index = (state->head + state->count + state->staged) % state->capacity;
    state->queue[index] = (cui_bridge_time_field_change_v1){
        .struct_size = sizeof(cui_bridge_time_field_change_v1), .version = 1,
        .node = {owned->slot, owned->generation}, .frame_sequence = event->event.frame_sequence,
        .previous = bridge_time_copy(event->previous), .value = bridge_time_copy(event->value)};
    state->staged++;
}
static void bridge_time_render(cui_bridge_app *app, uint32_t slot) {
    bridge_time_owned *owned = app->time_fields->nodes[slot];
    cui_bridge_node_record *node = &app->nodes[slot];
    if (owned->seen) { app->frame_error = CUI_BRIDGE_CORE_ERROR; return; }
    owned->seen = true;
    uint64_t frame = cui_frame_sequence(app->ui);
    bool ready = frame && cui_frame_paint_enabled(app->ui) && cui_frame_input_snapshot_ready(app->ui) &&
                 owned->last_seen && owned->last_seen == frame - 1;
    owned->admitted = ready;
    owned->last_seen = frame;
    cui_time_locale_v1 locale;
    cui_time_field_options_v1 options;
    bridge_time_options(&owned->config, &locale, &options);
    if (node->disabled) options.flags |= CUI_TIME_FIELD_DISABLED_V1;
    cui_time before = node->time;
    uint32_t staged = app->time_fields->staged;
    cui_time_field_result_v1 r = cui_time_field_v1(app->ui, node->id, &app->theme,
        cui_string(node->text), &owned->state, &node->time, &options,
        (cui_time_handler){bridge_time_observe, owned});
    uint64_t rejected = r.rejected_count;
    if (rejected > UINT32_MAX) { app->frame_error = CUI_BRIDGE_CORE_ERROR; return; }
    owned->result = (cui_bridge_time_field_result_v1){
        .struct_size = sizeof(cui_bridge_time_field_result_v1), .version = 1,
        .node = {slot, node->generation}, .frame_sequence = frame,
        .status = bridge_time_status(r.status), .validation = bridge_time_status(r.validation),
        .changed = r.changed ? 1u : 0u, .rejected_count = (uint32_t)r.rejected_count,
        .previous = bridge_time_copy(before), .value = bridge_time_copy(node->time),
        .queued_change_count = app->time_fields->staged - staged};
    if (app->time_fields->staged != staged) {
        uint32_t index = (app->time_fields->head + app->time_fields->count + staged) % app->time_fields->capacity;
        app->time_fields->queue[index].validation = owned->result.validation;
        app->time_fields->queue[index].rejected_count = owned->result.rejected_count;
    }
    if (owned->result.status == UINT32_MAX || owned->result.validation == UINT32_MAX)
        app->frame_error = CUI_BRIDGE_CORE_ERROR;
}
static void bridge_time_finish(cui_bridge_app *app, cui_bridge_status status) {
    bridge_time_app *state = app->time_fields;
    if (!state || !state->active) return;
    for (uint32_t i = 0; i < state->staged; i++)
        state->queue[(state->head + state->count + i) % state->capacity].frame_status = (uint32_t)status;
    state->count += state->staged;
    state->staged = 0;
    state->active = false;
    bool valid = status == CUI_BRIDGE_OK && cui_accessibility_numeric_snapshot_ready(app->ui) &&
                 cui_last_error(app->ui) == CUI_OK;
    if (!valid) state->failed_frame = true;
    bool recovered = false;
    for (uint32_t i = 1; i <= app->max_nodes; i++)
        if (state->nodes[i]) {
            bridge_time_owned *owned = state->nodes[i];
            owned->observed = valid && owned->seen && owned->admitted;
            owned->result.frame_status = (uint32_t)status;
            recovered |= owned->observed;
            if (!owned->seen) owned->last_seen = 0;
        }
    if (valid && recovered) state->failed_frame = false;
}
#endif
