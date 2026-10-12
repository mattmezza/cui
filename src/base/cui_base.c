/* SPDX-License-Identifier: MIT */
#include "cui/cui_base.h"
#include <math.h>
cui_interaction cui_base_button(cui_context *ui, cui_id id, cui_str name, bool disabled) {
    return cui_interact(ui, id, CUI_ROLE_BUTTON, name, disabled, false, 0);
}
static bool cui_base_binary(cui_context *ui, cui_id id, cui_str name, bool *value, bool disabled,
                            cui_role role) {
    if (!value)
        return false;
    cui_interaction s = cui_interact(ui, id, role, name, disabled, *value, 0);
    if (disabled || !s.activated)
        return false;
    *value = !*value;
    cui_update_semantic(ui, id, *value, *value ? 1 : 0);
    return true;
}
bool cui_base_checkbox(cui_context *u, cui_id i, cui_str n, bool *v, bool d) {
    return cui_base_binary(u, i, n, v, d, CUI_ROLE_CHECKBOX);
}
bool cui_base_switch(cui_context *u, cui_id i, cui_str n, bool *v, bool d) {
    return cui_base_binary(u, i, n, v, d, CUI_ROLE_SWITCH);
}
bool cui_base_toggle(cui_context *u, cui_id i, cui_str n, bool *v, bool d) {
    return cui_base_binary(u, i, n, v, d, CUI_ROLE_BUTTON);
}
bool cui_base_check(cui_context *ui, cui_id id, cui_str name, cui_check_state *v, bool d) {
    if (!v)
        return false;
    cui_interaction s =
        cui_interact(ui, id, CUI_ROLE_CHECKBOX, name, d, *v == CUI_CHECK_CHECKED, (float)*v);
    if (d || !s.activated)
        return false;
    *v = *v == CUI_CHECK_CHECKED ? CUI_CHECK_UNCHECKED : CUI_CHECK_CHECKED;
    cui_update_semantic(ui, id, *v == CUI_CHECK_CHECKED, (float)*v);
    return true;
}
bool cui_base_radio(cui_context *ui, cui_id id, cui_str name, int *value, int option,
                    bool disabled) {
    if (!value)
        return false;
    cui_interaction s =
        cui_interact(ui, id, CUI_ROLE_RADIO, name, disabled, *value == option, (float)option);
    if (disabled || !s.activated || *value == option)
        return false;
    *value = option;
    cui_update_semantic(ui, id, true, (float)option);
    return true;
}
static float cui_base_clamp(float x, float a, float b) {
    return x < a ? a : x > b ? b : x;
}
static float cui_base_quantize(float x, float a, float b, float step) {
    x = cui_base_clamp(x, a, b);
    if (step > 0 && isfinite(step) && x != b)
        x = a + roundf((x - a) / step) * step;
    return cui_base_clamp(x, a, b);
}
bool cui_base_slider(cui_context *ui, cui_id id, cui_str name, float *value, float min, float max,
                     float step, bool disabled) {
    if (!value || !isfinite(min) || !isfinite(max) || max < min)
        return false;
    float old = *value;
    float next = isfinite(old) ? cui_base_clamp(old, min, max) : min;
    cui_interaction s = cui_interact(ui, id, CUI_ROLE_SLIDER, name, disabled, false, next);
    if (disabled)
        return false;
    cui_rect b = cui_bounds(ui, id);
    size_t count = 0;
    const cui_event *events = cui_frame_events(ui, &count);
    bool dragging = s.pressed;
    for (size_t i = 0; i < count; i++) {
        cui_event e = events[i];
        if ((e.type == CUI_EVENT_POINTER_DOWN || e.type == CUI_EVENT_POINTER_UP) &&
            e.pointer_button != CUI_POINTER_PRIMARY)
            continue;
        if (cui_event_target(ui, i) != id)
            continue;
        if (e.type == CUI_EVENT_POINTER_DOWN)
            dragging = true;
        if (dragging &&
            (e.type == CUI_EVENT_POINTER_DOWN || e.type == CUI_EVENT_POINTER_MOVE ||
             e.type == CUI_EVENT_POINTER_UP) &&
            b.w > 0)
            next = cui_base_quantize(min + (max - min) * cui_base_clamp((e.x - b.x) / b.w, 0, 1),
                                     min, max, step);
        if (e.type == CUI_EVENT_POINTER_UP || e.type == CUI_EVENT_CANCEL)
            dragging = false;
        if (e.type == CUI_EVENT_KEY_DOWN) {
            float increment = step > 0 && isfinite(step) ? step : (max - min) / 100;
            if (e.key == CUI_KEY_HOME)
                next = min;
            else if (e.key == CUI_KEY_END)
                next = max;
            else if (e.key == CUI_KEY_LEFT || e.key == CUI_KEY_DOWN)
                next = cui_base_quantize(next - increment, min, max, step);
            else if (e.key == CUI_KEY_RIGHT || e.key == CUI_KEY_UP)
                next = cui_base_quantize(next + increment, min, max, step);
        }
    }
    *value = next;
    cui_update_semantic(ui, id, false, next);
    return old != next;
}

static bool cui_input_readonly_event(cui_event event) {
    if (event.type != CUI_EVENT_KEY_DOWN)
        return false;
    return event.key == CUI_KEY_LEFT || event.key == CUI_KEY_RIGHT || event.key == CUI_KEY_HOME ||
           event.key == CUI_KEY_END ||
           (event.key == CUI_KEY_A && (event.modifiers & (CUI_MOD_CTRL | CUI_MOD_META)));
}
cui_input_result cui_base_input_ex(cui_context *ui, cui_id id, cui_str name, cui_edit *edit,
                                   cui_input_behavior_state *state,
                                   cui_base_input_options options) {
    cui_input_result result = {.error = CUI_EDIT_OK};
    if (!edit || !state || !ui || !id) {
        result.error = CUI_EDIT_INVALID_ARGUMENT;
        return result;
    }
    cui_edit_state before = cui_edit_get_state(edit);
    cui_interaction interaction =
        cui_interact(ui, id, CUI_ROLE_TEXTBOX, name, options.disabled, false, (float)before.length);
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    if (options.disabled)
        state->dragging = false;
    for (size_t i = 0; i < count; ++i) {
        cui_event event = events[i];
        if (event.type == CUI_EVENT_CANCEL)
            state->dragging = false;
        if (options.disabled || cui_event_consumed(ui, i) || cui_event_target(ui, i) != id)
            continue;
        if (event.type == CUI_EVENT_KEY_DOWN &&
            (event.key == CUI_KEY_LEFT || event.key == CUI_KEY_RIGHT))
            cui_consume_event(ui, i);
        cui_edit_result error = CUI_EDIT_OK;
        bool primary = event.pointer_button == CUI_POINTER_PRIMARY;
        if (primary && event.type == CUI_EVENT_POINTER_DOWN)
            state->dragging = true;
        if (options.hit_test && primary &&
            (event.type == CUI_EVENT_POINTER_DOWN ||
             ((state->dragging || interaction.pressed) &&
              (event.type == CUI_EVENT_POINTER_MOVE || event.type == CUI_EVENT_POINTER_UP)))) {
            cui_edit_state current = cui_edit_get_state(edit);
            size_t cursor =
                options.hit_test(options.hit_test_user, cui_edit_get_text(edit), event.x, event.y);
            size_t anchor =
                event.type == CUI_EVENT_POINTER_DOWN && !(event.modifiers & CUI_MOD_SHIFT)
                    ? cursor
                    : current.anchor;
            error = cui_edit_select(edit, anchor, cursor);
        } else if (event.type == CUI_EVENT_KEY_DOWN &&
                   (event.modifiers & (CUI_MOD_CTRL | CUI_MOD_META)) &&
                   (event.key == CUI_KEY_C || event.key == CUI_KEY_X || event.key == CUI_KEY_V)) {
            const cui_platform *host = options.clipboard;
            cui_edit_state current = cui_edit_get_state(edit);
            size_t start = current.anchor < current.cursor ? current.anchor : current.cursor;
            size_t end = current.anchor > current.cursor ? current.anchor : current.cursor;
            bool paste = event.key == CUI_KEY_V;
            if ((paste && !options.readonly) || (!paste && start != end)) {
                if (!host || !(host->capabilities & CUI_PLATFORM_CLIPBOARD) ||
                    (paste ? !host->clipboard_get : !host->clipboard_set))
                    error = CUI_EDIT_CLIPBOARD_UNAVAILABLE;
                else if (paste)
                    error = cui_edit_insert(edit, host->clipboard_get(host->user));
                else {
                    cui_str text = cui_edit_get_text(edit);
                    if (!host->clipboard_set(host->user, (cui_str){text.data + start, end - start}))
                        error = CUI_EDIT_CLIPBOARD_UNAVAILABLE;
                    else if (event.key == CUI_KEY_X && !options.readonly)
                        error = cui_edit_delete(edit);
                }
            }
        } else if (!options.readonly || cui_input_readonly_event(event))
            error = cui_edit_apply_event(edit, event);
        if (primary && event.type == CUI_EVENT_POINTER_UP)
            state->dragging = false;
        if (!before.multiline && event.type == CUI_EVENT_KEY_DOWN && event.key == CUI_KEY_ENTER &&
            !event.repeat)
            result.submitted = true;
        if (result.error == CUI_EDIT_OK && error != CUI_EDIT_OK)
            result.error = error;
    }
    cui_edit_state after = cui_edit_get_state(edit);
    result.changed = before.revision != after.revision;
    cui_update_semantic(ui, id, false, (float)after.length);
    cui_set_semantic_details(
        ui, id,
        (cui_semantic_details){.readonly = options.readonly,
                               .required = options.required,
                               .invalid = options.invalid || result.error != CUI_EDIT_OK,
                               .multiline = after.multiline});
    return result;
}
cui_input_result cui_base_input(cui_context *ui, cui_id id, cui_str name, cui_edit *edit,
                                bool disabled) {
    cui_input_behavior_state state = {0};
    return cui_base_input_ex(ui, id, name, edit, &state,
                             (cui_base_input_options){.disabled = disabled});
}

size_t cui_base_input_hit_test(void *user, cui_str text, float x, float y) {
    (void)y;
    cui_input_hit_geometry *hit = user;
    if (!hit || !hit->ui || !isfinite(x) || !isfinite(hit->font_size) || hit->font_size <= 0 ||
        !cui_edit_valid_utf8(text))
        return SIZE_MAX;
    float target = x - hit->viewport.x + hit->scroll_x;
    if (target <= 0 || !text.length)
        return 0;
    size_t low = 0, high = text.length;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        while (middle > low && ((unsigned char)text.data[middle] & 0xc0) == 0x80)
            middle--;
        size_t next = middle + 1;
        while (next < text.length && ((unsigned char)text.data[next] & 0xc0) == 0x80)
            next++;
        float width = cui_measure_text(hit->ui, (cui_str){text.data, next}, hit->font_size).width;
        if (width <= target)
            low = next;
        else
            high = middle;
    }
    if (low == text.length)
        return low;
    size_t next = low + 1;
    while (next < text.length && ((unsigned char)text.data[next] & 0xc0) == 0x80)
        next++;
    float left = cui_measure_text(hit->ui, (cui_str){text.data, low}, hit->font_size).width;
    float right = cui_measure_text(hit->ui, (cui_str){text.data, next}, hit->font_size).width;
    return target < (left + right) * .5f ? low : next;
}
