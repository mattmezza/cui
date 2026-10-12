/* SPDX-License-Identifier: MIT */
#include "cui/cui_numeric.h"
#include "cui/cui_components.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static float cui_numeric_semantic_value(double value) {
    return (float)(value > FLT_MAX ? FLT_MAX : value < -FLT_MAX ? -FLT_MAX : value);
}
static cui_id cui_numeric_child(cui_id id, unsigned index) {
    return id ^ (UINT64_C(0xa620ffb851de91c3) + UINT64_C(0x9e3779b97f4a7c15) * index);
}
static cui_edit_result cui_numeric_set_draft(cui_edit *edit, double value) {
    char text[96];
    size_t n = cui_number_format(value, text, sizeof(text));
    if (!n)
        return CUI_EDIT_INVALID_ARGUMENT;
    cui_edit_state state = cui_edit_get_state(edit);
    cui_edit_result result = cui_edit_select(edit, 0, state.length);
    if (result != CUI_EDIT_OK)
        return result;
    return cui_edit_insert(edit, (cui_str){text, n});
}
static bool cui_numeric_prefix(const char *s, size_t n, bool exponent) {
    size_t i = 0, digits = 0;
    if (i < n && (s[i] == '+' || s[i] == '-'))
        ++i;
    while (i < n && s[i] >= '0' && s[i] <= '9') {
        ++i;
        ++digits;
    }
    if (i < n && s[i] == '.') {
        ++i;
        while (i < n && s[i] >= '0' && s[i] <= '9') {
            ++i;
            ++digits;
        }
    }
    if (exponent && digits && i < n && (s[i] == 'e' || s[i] == 'E')) {
        ++i;
        if (i < n && (s[i] == '+' || s[i] == '-'))
            ++i;
        while (i < n && s[i] >= '0' && s[i] <= '9')
            ++i;
    }
    return i == n;
}
static cui_edit_result cui_numeric_insert(cui_edit *edit, cui_str text, bool exponent) {
    char normalized[128];
    size_t n = 0;
    for (size_t i = 0; i < text.length;) {
        unsigned char c = (unsigned char)text.data[i];
        if (c < 128) {
            normalized[n++] = (char)c;
            ++i;
        } else if (i + 2 < text.length && c == 0xef && (unsigned char)text.data[i + 1] == 0xbc) {
            unsigned char tail = (unsigned char)text.data[i + 2];
            if (tail >= 0x90 && tail <= 0x99)
                normalized[n++] = (char)('0' + tail - 0x90);
            else if (tail == 0x8b)
                normalized[n++] = '+';
            else if (tail == 0x8d)
                normalized[n++] = '-';
            else if (tail == 0x8e)
                normalized[n++] = '.';
            else
                return CUI_EDIT_INVALID_ARGUMENT;
            i += 3;
        } else if (i + 2 < text.length && c == 0xe3 && (unsigned char)text.data[i + 1] == 0x80 &&
                   (unsigned char)text.data[i + 2] == 0x82) {
            normalized[n++] = '.';
            i += 3;
        } else
            return CUI_EDIT_INVALID_ARGUMENT;
        if (n >= sizeof(normalized))
            return CUI_EDIT_CAPACITY;
    }
    cui_edit_state state = cui_edit_get_state(edit);
    cui_str old = cui_edit_get_text(edit);
    size_t begin = state.anchor < state.cursor ? state.anchor : state.cursor;
    size_t end = state.anchor > state.cursor ? state.anchor : state.cursor;
    if (old.length > 127 || n > 127 - (old.length - (end - begin)))
        return CUI_EDIT_CAPACITY;
    char candidate[128];
    size_t length = old.length - (end - begin) + n;
    memcpy(candidate, old.data, begin);
    memcpy(candidate + begin, normalized, n);
    memcpy(candidate + begin + n, old.data + end, old.length - end);
    if (!cui_numeric_prefix(candidate, length, exponent))
        return CUI_EDIT_INVALID_ARGUMENT;
    return cui_edit_insert(edit, (cui_str){normalized, n});
}
static double cui_numeric_clamp(double value, cui_number_options o) {
    if (o.has_minimum && isfinite(o.minimum) && value < o.minimum)
        value = o.minimum;
    if (o.has_maximum && isfinite(o.maximum) && value > o.maximum)
        value = o.maximum;
    return value;
}
cui_number_result cui_number_input(cui_context *ui, cui_id id, const cui_theme *t, cui_str name,
                                   cui_number_state *s, double *value, cui_number_options o) {
    cui_number_result result = {0};
    if (!ui || !t || !s || !s->edit || !value || !isfinite(*value) ||
        cui_edit_get_state(s->edit).multiline || (o.has_minimum && !isfinite(o.minimum)) ||
        (o.has_maximum && !isfinite(o.maximum)) ||
        (o.has_minimum && o.has_maximum && o.minimum > o.maximum)) {
        result.error = CUI_EDIT_INVALID_ARGUMENT;
        result.invalid = true;
        return result;
    }
    bool focused = cui_focused(ui) == id;
    if (!s->initialized || *value != s->last_value) {
        result.error = cui_numeric_set_draft(s->edit, *value);
        s->initialized = true;
        s->invalid = false;
    }
    double original = *value;
    bool editable = !o.disabled && !o.readonly;
    size_t count = 0;
    const cui_event *events = cui_frame_events(ui, &count);
    bool commit = s->was_focused && !focused && editable;
    for (size_t i = 0; i < count && editable; ++i) {
        const cui_event *e = &events[i];
        if (cui_event_target(ui, i) != id)
            continue;
        cui_edit_result error = CUI_EDIT_OK;
        if (e->type == CUI_EVENT_TEXT) {
            size_t n = 0;
            while (n < sizeof(e->text) && e->text[n])
                ++n;
            error = cui_numeric_insert(s->edit, (cui_str){e->text, n}, o.allow_exponent);
        } else if (e->type == CUI_EVENT_KEY_DOWN) {
            bool extend = (e->modifiers & CUI_MOD_SHIFT) != 0;
            if (e->key == CUI_KEY_ENTER) {
                commit = true;
                result.submitted = true;
            } else if (e->key == CUI_KEY_UP || e->key == CUI_KEY_DOWN) {
                double draft = *value;
                cui_number_parse(cui_edit_get_text(s->edit), &draft);
                int direction = e->key == CUI_KEY_UP ? 1 : -1;
                result.step_direction = direction;
                if (cui_number_step(&draft, direction, o)) {
                    error = cui_numeric_set_draft(s->edit, draft);
                    if (error == CUI_EDIT_OK)
                        *value = draft;
                }
            } else if (e->key == CUI_KEY_A && (e->modifiers & (CUI_MOD_CTRL | CUI_MOD_META))) {
                error = cui_edit_select(s->edit, 0, cui_edit_get_state(s->edit).length);
            } else if (e->key == CUI_KEY_LEFT || e->key == CUI_KEY_RIGHT) {
                error = cui_edit_move(
                    s->edit, e->key == CUI_KEY_LEFT ? CUI_EDIT_PREVIOUS : CUI_EDIT_NEXT, extend);
            } else if (e->key == CUI_KEY_HOME || e->key == CUI_KEY_END) {
                error = cui_edit_move(
                    s->edit, e->key == CUI_KEY_HOME ? CUI_EDIT_START : CUI_EDIT_END, extend);
            } else if (e->key == CUI_KEY_BACKSPACE)
                error = cui_edit_backspace(s->edit);
            else if (e->key == CUI_KEY_DELETE)
                error = cui_edit_delete(s->edit);
        }
        if (result.error == CUI_EDIT_OK && error != CUI_EDIT_OK)
            result.error = error;
    }
    double parsed;
    s->invalid = !cui_number_parse(cui_edit_get_text(s->edit), &parsed);
    if (!s->invalid && editable) {
        *value = commit ? cui_numeric_clamp(parsed, o) : parsed;
        if (commit) {
            cui_edit_result error = cui_numeric_set_draft(s->edit, *value);
            if (result.error == CUI_EDIT_OK)
                result.error = error;
            if (error != CUI_EDIT_OK)
                *value = parsed;
        }
    }
    float width = isfinite(o.width) && o.width > 0 ? o.width : 200;
    cui_begin_box(ui, cui_numeric_child(id, 0), (cui_box){.gap = t->gap, .row = true});
    if (o.prefix.length)
        cui_text(ui, o.prefix, t->font_size, t->text);
    cui_begin_box(ui, id,
                  (cui_box){.width = width,
                            .padding = t->padding,
                            .gap = t->gap * .5f,
                            .radius = t->radius,
                            .background = t->surface,
                            .border = s->invalid ? t->danger
                                      : focused  ? t->focus
                                                 : t->border,
                            .border_width = 1});
    cui_interact(ui, id, CUI_ROLE_TEXTBOX, name, o.disabled, false,
                 cui_numeric_semantic_value(*value));
    cui_set_semantic_details(
        ui, id,
        (cui_semantic_details){
            .readonly = o.readonly,
            .invalid = s->invalid,
            .minimum = cui_numeric_semantic_value(o.has_minimum ? o.minimum : -FLT_MAX),
            .maximum = cui_numeric_semantic_value(o.has_maximum ? o.maximum : FLT_MAX)});
    cui_text(ui, name, t->font_size * .8f, t->muted);
    cui_text(ui, cui_edit_get_text(s->edit), t->font_size, o.disabled ? t->muted : t->text);
    cui_end_box(ui);
    bool decrease = cui_button(ui, cui_numeric_child(id, 1), t, cui_string("-"),
                               !editable || (o.has_minimum && *value <= o.minimum));
    bool increase = cui_button(ui, cui_numeric_child(id, 2), t, cui_string("+"),
                               !editable || (o.has_maximum && *value >= o.maximum));
    if (decrease || increase) {
        result.step_direction = increase ? 1 : -1;
        double stepped = *value;
        if (cui_number_step(&stepped, result.step_direction, o)) {
            cui_edit_result error = cui_numeric_set_draft(s->edit, stepped);
            if (result.error == CUI_EDIT_OK)
                result.error = error;
            if (error == CUI_EDIT_OK) {
                *value = stepped;
                s->invalid = false;
                cui_invalidate(ui);
            }
        }
    }
    if (o.suffix.length)
        cui_text(ui, o.suffix, t->font_size, t->text);
    cui_end_box(ui);
    cui_update_semantic(ui, id, false, cui_numeric_semantic_value(*value));
    s->last_value = *value;
    s->was_focused = focused;
    result.invalid = s->invalid;
    result.changed = *value != original;
    return result;
}
static bool cui_numeric_page_button(cui_context *ui, cui_id id, const cui_theme *t, size_t *page,
                                    size_t target, const char *name, bool disabled) {
    if (cui_button(ui, id, t, cui_string(name), disabled)) {
        *page = target;
        return true;
    }
    return false;
}
bool cui_pagination(cui_context *ui, cui_id id, const cui_theme *t, size_t *page, size_t total,
                    bool compact, bool disabled) {
    if (!ui || !t || !page)
        return false;
    size_t original = *page;
    if (!total)
        *page = 0;
    else if (*page < 1)
        *page = 1;
    else if (*page > total)
        *page = total;
    size_t current = *page;
    cui_begin_box(ui, id, (cui_box){.row = true, .gap = t->gap});
    cui_interact(ui, id, CUI_ROLE_GROUP, cui_string("Pagination"), disabled, false, (float)current);
    cui_set_tab_stop(ui, id, false);
    cui_numeric_page_button(ui, cui_numeric_child(id, 1), t, page, 1, "First",
                            disabled || current <= 1);
    cui_numeric_page_button(ui, cui_numeric_child(id, 2), t, page,
                            current > 1 ? current - 1 : current, "Previous",
                            disabled || current <= 1);
    if (compact) {
        char label[64];
        snprintf(label, sizeof(label), "%zu / %zu", current, total);
        cui_text(ui, cui_string(label), t->font_size, t->text);
    } else {
        cui_page_item items[11];
        size_t n = cui_pagination_items(current, total, 2, items);
        for (size_t i = 0; i < n; ++i) {
            if (items[i].ellipsis) {
                cui_text(ui, cui_string("..."), t->font_size, t->muted);
                continue;
            }
            char label[32];
            snprintf(label, sizeof(label), "%zu", items[i].page);
            cui_id child = cui_numeric_child(id, 10 + (unsigned)i);
            cui_numeric_page_button(ui, child, t, page, items[i].page, label,
                                    disabled || items[i].page == current);
            cui_set_semantic_details(
                ui, child,
                (cui_semantic_details){.selected = items[i].page == current, .parent = id});
        }
    }
    cui_numeric_page_button(ui, cui_numeric_child(id, 3), t, page,
                            current < total ? current + 1 : current, "Next",
                            disabled || !total || current >= total);
    cui_numeric_page_button(ui, cui_numeric_child(id, 4), t, page, total, "Last",
                            disabled || !total || current >= total);
    if (!compact) {
        cui_page_item items[11];
        size_t n = cui_pagination_items(current, total, 2, items);
        for (size_t i = 0; i < n; ++i)
            if (!items[i].ellipsis)
                cui_set_semantic_details(
                    ui, cui_numeric_child(id, 10 + (unsigned)i),
                    (cui_semantic_details){.selected = items[i].page == *page, .parent = id});
    }
    cui_end_box(ui);
    cui_update_semantic(ui, id, false, (float)*page);
    if (*page != original)
        cui_invalidate(ui);
    return *page != original;
}
bool cui_rating(cui_context *ui, cui_id id, const cui_theme *t, cui_str name, float *value,
                cui_rating_options o) {
    if (!ui || !t || !value)
        return false;
    unsigned maximum = o.maximum ? o.maximum : 5;
    if (maximum > 100)
        maximum = 100;
    float before = *value, step = o.half_steps ? .5f : 1;
    if (!isfinite(*value))
        *value = 0;
    *value = fmaxf(0, fminf((float)maximum, roundf(*value / step) * step));
    float width = isfinite(o.width) && o.width > 0 ? o.width : maximum * 36;
    float slot = fmaxf(1, (width - t->padding * 2 - t->gap * (maximum - 1)) / maximum);
    cui_begin_box(ui, cui_numeric_child(id, 0), (cui_box){.row = true, .gap = t->gap});
    cui_begin_box(ui, id,
                  (cui_box){.row = true,
                            .width = width,
                            .padding = t->padding,
                            .gap = t->gap,
                            .radius = t->radius,
                            .background = t->surface});
    if (!o.readonly)
        cui_base_slider(ui, id, name, value, 0, (float)maximum, step, o.disabled);
    else
        cui_interact(ui, id, CUI_ROLE_SLIDER, name, o.disabled, false, *value);
    cui_set_semantic_details(
        ui, id,
        (cui_semantic_details){.readonly = o.readonly, .minimum = 0, .maximum = (float)maximum});
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    if (o.clearable && !o.disabled && !o.readonly)
        for (size_t i = 0; i < count; ++i)
            if (cui_event_target(ui, i) == id && events[i].type == CUI_EVENT_KEY_DOWN &&
                (events[i].key == CUI_KEY_DELETE || events[i].key == CUI_KEY_BACKSPACE))
                *value = 0;
    for (unsigned i = 0; i < maximum; ++i) {
        cui_id child = cui_numeric_child(id, 10 + i);
        cui_begin_box(ui, child, (cui_box){.width = slot, .height = t->control_height});
        char label[40];
        snprintf(label, sizeof(label), "%u of %u", i + 1, maximum);
        cui_interaction interaction =
            cui_interact(ui, child, o.readonly ? CUI_ROLE_LABEL : CUI_ROLE_BUTTON,
                         cui_string(label), o.disabled, false, *value);
        cui_set_tab_stop(ui, child, false);
        cui_set_semantic_details(ui, child,
                                 (cui_semantic_details){.selected = *value >= (float)i + 1,
                                                        .parent = id,
                                                        .readonly = o.readonly});
        if (interaction.activated && !o.readonly && !o.disabled) {
            float selected = (float)i + 1;
            if (o.half_steps) {
                cui_rect bounds = cui_bounds(ui, child);
                for (size_t e = 0; e < count; ++e)
                    if (events[e].type == CUI_EVENT_POINTER_UP &&
                        cui_event_target(ui, e) == child && events[e].x < bounds.x + bounds.w / 2)
                        selected -= .5f;
            }
            *value = selected;
            cui_focus(ui, id);
            cui_invalidate(ui);
        }
        const char *star = *value >= (float)i + 1 ? "★" : *value > (float)i ? "◐" : "☆";
        cui_text(ui, cui_string(star), t->font_size * 1.3f, o.disabled ? t->muted : t->primary);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    if (o.clearable && cui_button(ui, cui_numeric_child(id, 1), t, cui_string("Clear"),
                                  o.disabled || o.readonly)) {
        *value = 0;
        cui_focus(ui, id);
        cui_invalidate(ui);
    }
    cui_end_box(ui);
    cui_update_semantic(ui, id, false, *value);
    for (unsigned i = 0; i < maximum; ++i)
        cui_set_semantic_details(ui, cui_numeric_child(id, 10 + i),
                                 (cui_semantic_details){.selected = *value >= (float)i + 1,
                                                        .parent = id,
                                                        .readonly = o.readonly});
    return !isfinite(before) || *value != before;
}
bool cui_stepper(cui_context *ui, cui_id id, const cui_theme *t, const cui_step *steps,
                 size_t count, size_t *current, bool vertical, bool disabled) {
    if (!ui || !t || !current || (!steps && count) || count > 100)
        return false;
    size_t before = *current;
    if (count && *current >= count)
        *current = count - 1;
    else if (!count)
        *current = 0;
    cui_begin_box(ui, id, (cui_box){.row = !vertical, .gap = t->gap});
    cui_interact(ui, id, CUI_ROLE_GROUP, cui_string("Steps"), disabled, false, (float)*current);
    cui_set_tab_stop(ui, id, false);
    for (size_t i = 0; i < count; ++i) {
        const char *status = steps[i].status == CUI_STEP_ERROR      ? "Error"
                             : steps[i].status == CUI_STEP_COMPLETE ? "Complete"
                             : steps[i].status == CUI_STEP_ACTIVE   ? "Current"
                                                                    : "Pending";
        cui_begin_box(ui, cui_numeric_child(id, 100 + (unsigned)i), (cui_box){.gap = t->gap});
        cui_id trigger = cui_numeric_child(id, 1 + (unsigned)i);
        if (cui_button(ui, trigger, t, steps[i].title, disabled || steps[i].disabled))
            *current = i;
        cui_set_semantic_details(ui, trigger,
                                 (cui_semantic_details){.selected = i == *current,
                                                        .description = steps[i].description,
                                                        .parent = id});
        cui_text(ui, cui_string(status), t->font_size * .8f,
                 steps[i].status == CUI_STEP_ERROR ? t->danger : t->muted);
        if (steps[i].description.length)
            cui_text(ui, steps[i].description, t->font_size * .8f, t->muted);
        cui_end_box(ui);
    }
    for (size_t i = 0; i < count; ++i)
        cui_set_semantic_details(ui, cui_numeric_child(id, 1 + (unsigned)i),
                                 (cui_semantic_details){.selected = i == *current,
                                                        .description = steps[i].description,
                                                        .parent = id});
    cui_end_box(ui);
    cui_update_semantic(ui, id, false, (float)*current);
    if (*current != before)
        cui_invalidate(ui);
    return *current != before;
}
