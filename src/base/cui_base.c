/* SPDX-License-Identifier: MIT */
#include "cui/cui_base.h"
#include <math.h>
static float cui_progress_fraction(float value) {
    return isfinite(value) ? fmaxf(0, fminf(1, value)) : 0;
}
static double cui_progress_quadratic(double value) {
    return value < .5 ? 2 * value * value : 1 - 2 * (1 - value) * (1 - value);
}
cui_progress_geometry cui_progress_measure(float fraction, bool indeterminate, bool source_motion,
                                           double now, bool reduced) {
    cui_progress_geometry result = {.fraction = cui_progress_fraction(fraction)};
    if (!indeterminate) {
        result.length = result.fraction;
        return result;
    }
    bool moving = !reduced && isfinite(now) && now >= 0;
    result.animating = moving;
    if (source_motion) {
        if (!moving) {
            result.start = .325f;
            result.length = .35f;
        } else {
            double delta = fmod(now, 1);
            double start = cui_progress_quadratic(fmax(0, fmin(1, (delta - .5) * 2)));
            double end = cui_progress_quadratic(1 - delta);
            result.start = (float)start;
            result.length = (float)fmax(0, 1 - start - end);
        }
    } else {
        double phase = moving ? .5 - .5 * cos(fmod(now, 1.6) * (6.283185307179586 / 1.6)) : .5;
        result.start = (float)(.7 * phase);
        result.length = .3f;
    }
    return result;
}
float cui_base_progress(cui_context *ui, cui_id id, cui_str name, float fraction,
                        bool indeterminate) {
    float value = cui_progress_fraction(fraction);
    if (!ui || !id || (name.length && !name.data))
        return value;
    cui_interact(ui, id, CUI_ROLE_PROGRESSBAR, name.length ? name : cui_string("Progress"), false,
                 false, indeterminate ? 0 : value);
    cui_set_semantic_details(
        ui, id,
        (cui_semantic_details){
            .minimum = 0, .maximum = 1, .busy = indeterminate, .no_focus = true});
    cui_set_tab_stop(ui, id, false);
    return value;
}
static float cui_progress_ease(float fraction) {
    if (fraction <= 0 || fraction >= 1)
        return fraction;
    double low = 0, high = 1, parameter = fraction;
    for (unsigned i = 0; i < 24; i++) {
        parameter = (low + high) * .5;
        double inverse = 1 - parameter;
        double x = 3 * inverse * inverse * parameter * .2 + parameter * parameter * parameter;
        if (x < fraction)
            low = parameter;
        else
            high = parameter;
    }
    return (float)(3 * (1 - parameter) * parameter * parameter + parameter * parameter * parameter);
}
static void cui_progress_motion_sample(cui_progress_motion *state, double now) {
    if (!state->active)
        return;
    double elapsed = now - state->start;
    float fraction = (float)fmax(0, fmin(1, elapsed / state->duration));
    state->value = state->from + (state->target - state->from) * cui_progress_ease(fraction);
    if (fraction >= 1) {
        state->value = state->target;
        state->active = false;
    }
}
bool cui_progress_animate(cui_progress_motion *state, float value, double now, float duration,
                          bool reduced, cui_progress_motion_result *result) {
    if (!state || !result || !isfinite(value) || !isfinite(now) || now < 0 || !isfinite(duration) ||
        duration < 0 || duration > 60 ||
        (state->ready &&
         (!isfinite(state->start) || !isfinite(state->last_time) || !isfinite(state->from) ||
          !isfinite(state->target) || !isfinite(state->value) || !isfinite(state->duration) ||
          state->duration < 0 || state->duration > 60 || state->start < 0 ||
          state->last_time < state->start || state->from < 0 || state->from > 1 ||
          state->target < 0 || state->target > 1 || state->value < 0 || state->value > 1 ||
          (state->active && state->duration <= 0))))
        return false;
    value = cui_progress_fraction(value);
    cui_progress_motion next = *state;
    if (!next.ready) {
        next = (cui_progress_motion){.ready = true,
                                     .from = value,
                                     .target = value,
                                     .value = value,
                                     .start = now,
                                     .last_time = now,
                                     .duration = duration};
    } else {
        now = fmax(now, next.last_time);
        cui_progress_motion_sample(&next, now);
        if (next.target != value) {
            next.from = next.value;
            next.target = value;
            next.start = now;
            next.duration = duration;
            next.active = next.from != value;
        }
        if (reduced || duration == 0) {
            next.from = next.target = next.value = value;
            next.active = false;
            next.duration = duration;
            next.start = now;
        }
        next.last_time = now;
        cui_progress_motion_sample(&next, now);
    }
    *state = next;
    *result = (cui_progress_motion_result){next.value, next.active};
    return true;
}
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
    /* Earlier siblings may already have painted the old controlled value. */
    cui_invalidate(ui);
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
    return event.key == CUI_KEY_LEFT || event.key == CUI_KEY_RIGHT || event.key == CUI_KEY_UP ||
           event.key == CUI_KEY_DOWN || event.key == CUI_KEY_HOME || event.key == CUI_KEY_END ||
           (event.key == CUI_KEY_A && (event.modifiers & (CUI_MOD_CTRL | CUI_MOD_META)));
}
cui_input_result cui_base_input_reduce_event(cui_context *ui, cui_id id, cui_edit *edit,
                                             cui_input_behavior_state *state,
                                             cui_base_input_options options, size_t i) {
    cui_input_result result = {.error = CUI_EDIT_OK};
    size_t count = 0;
    const cui_event *events = cui_frame_events(ui, &count);
    if (!ui || !id || !edit || !state || !events || i >= count) {
        result.error = CUI_EDIT_INVALID_ARGUMENT;
        return result;
    }
    cui_edit_state before = cui_edit_get_state(edit);
    if (options.disabled)
        state->dragging = false;
    cui_event event = events[i];
    if (event.type == CUI_EVENT_CANCEL)
        state->dragging = false;
    if (cui_event_consumed(ui, i) && event.type == CUI_EVENT_POINTER_UP &&
        event.pointer_button == CUI_POINTER_PRIMARY)
        state->dragging = false;
    if (options.disabled || cui_event_consumed(ui, i) || cui_event_target(ui, i) != id ||
        (event.type == CUI_EVENT_KEY_DOWN && (event.ime_composing || event.prefer_character_input)))
        return result;
    if (options.delegate_up_down && event.type == CUI_EVENT_KEY_DOWN &&
        (event.key == CUI_KEY_UP || event.key == CUI_KEY_DOWN))
        return result;
    if (options.delegate_home_end && event.type == CUI_EVENT_KEY_DOWN &&
        (event.key == CUI_KEY_HOME || event.key == CUI_KEY_END))
        return result;
    if (event.type == CUI_EVENT_KEY_DOWN &&
        (event.key == CUI_KEY_LEFT || event.key == CUI_KEY_RIGHT || event.key == CUI_KEY_UP ||
         event.key == CUI_KEY_DOWN || event.key == CUI_KEY_HOME || event.key == CUI_KEY_END))
        cui_consume_event(ui, i);
    cui_edit_result error = CUI_EDIT_OK;
    bool primary = event.pointer_button == CUI_POINTER_PRIMARY;
    if (primary && event.type == CUI_EVENT_POINTER_DOWN)
        state->dragging = true;
    if (options.hit_test && primary &&
        (event.type == CUI_EVENT_POINTER_DOWN ||
         ((state->dragging || cui_pressed(ui, id)) &&
          (event.type == CUI_EVENT_POINTER_MOVE || event.type == CUI_EVENT_POINTER_UP)))) {
        cui_edit_state current = cui_edit_get_state(edit);
        size_t cursor =
            options.hit_test(options.hit_test_user, cui_edit_get_text(edit), event.x, event.y);
        size_t snapped;
        if (event.type != CUI_EVENT_POINTER_DOWN &&
            cui_edit_token_boundary(cui_edit_get_content(edit), cursor, false, &snapped) ==
                CUI_EDIT_OK &&
            snapped == current.cursor)
            cursor = snapped;
        size_t anchor = event.type == CUI_EVENT_POINTER_DOWN && !(event.modifiers & CUI_MOD_SHIFT)
                            ? cursor
                            : current.anchor;
        error = cui_edit_select_atomic(edit, anchor, cursor);
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
    result.error = error;
    result.changed = before.revision != cui_edit_get_state(edit).revision;
    return result;
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
    (void)cui_interact(ui, id, CUI_ROLE_TEXTBOX, name, options.disabled, false,
                       (float)before.length);
    size_t count = 0;
    (void)cui_frame_events(ui, &count);
    if (options.disabled)
        state->dragging = false;
    for (size_t i = 0; i < count; ++i) {
        cui_input_result event = cui_base_input_reduce_event(ui, id, edit, state, options, i);
        result.submitted |= event.submitted;
        if (result.error == CUI_EDIT_OK && event.error != CUI_EDIT_OK)
            result.error = event.error;
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
