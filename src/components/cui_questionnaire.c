/* SPDX-License-Identifier: MIT */
#include "cui/cui_questionnaire.h"
#include "cui/cui_components.h"
#include <math.h>
#include <stdio.h>
typedef struct {
    float root_gap, gap, content_gap, px, py, height, indicator, mark, shortcut, font, line;
} cui_questionnaire_metrics;
static cui_questionnaire_metrics cui_questionnaire_measure(cui_questionnaire_size size) {
    switch (size) {
    case CUI_QUESTIONNAIRE_XSMALL:
        return (cui_questionnaire_metrics){8, 6, 2, 8, 4, 28, 12, 5, 16, 12, 16};
    case CUI_QUESTIONNAIRE_SMALL:
        return (cui_questionnaire_metrics){12, 8, 2, 10, 8, 36, 14, 6, 18, 14, 20};
    case CUI_QUESTIONNAIRE_LARGE:
        return (cui_questionnaire_metrics){24, 12, 4, 16, 12, 48, 18, 10, 24, 18, 28};
    default:
        return (cui_questionnaire_metrics){16, 10, 2, 12, 10, 44, 16, 8, 20, 16, 24};
    }
}
bool cui_questionnaire_begin(cui_context *ui, cui_id id, const cui_theme *theme,
                             cui_questionnaire_state *s, cui_questionnaire_options o) {
    if (!ui || !id || !theme || !s || !s->initialized)
        return false;
    cui_questionnaire_metrics m = cui_questionnaire_measure(o.size);
    (void)cui_questionnaire_tick(s, cui_frame_time(ui));
    if (s->pending_confirm)
        cui_invalidate(ui);
    cui_interact(ui, id, CUI_ROLE_GROUP, (cui_str){0}, o.disabled, false, 0);
    cui_begin_box(ui, id, (cui_box){.width = o.width > 0 ? o.width : -1, .gap = m.root_gap});
    return true;
}
void cui_questionnaire_end(cui_context *ui) {
    cui_end_box(ui);
}
void cui_questionnaire_progress(cui_context *ui, cui_id id, const cui_theme *t,
                                const cui_questionnaire_state *s, cui_questionnaire_options o) {
    if (!ui || !id || !t || !s || !s->initialized)
        return;
    size_t position = cui_questionnaire_position(s), total = cui_questionnaire_total(s);
    size_t current = position == SIZE_MAX ? 0 : position + 1;
    char label[80];
    int n = snprintf(label, sizeof label, "Question %zu of %zu", current, total);
    if (n < 0 || (size_t)n >= sizeof label)
        return;
    cui_str text = {label, (size_t)n};
    cui_interact(ui, id, CUI_ROLE_PROGRESSBAR, text, false, false,
                 total ? (float)current / (float)total : 0);
    cui_set_semantic_details(ui, id, (cui_semantic_details){.minimum = 0, .maximum = (float)total});
    cui_begin_box(ui, id, (cui_box){.width = -1});
    cui_text_ex(ui, text, o.size == CUI_QUESTIONNAIRE_LARGE ? 14 : 12, t->muted, 0,
                (cui_text_style){.weight = CUI_TEXT_WEIGHT_MEDIUM});
    cui_end_box(ui);
}
bool cui_questionnaire_choice_control(cui_context *ui, cui_id id, const cui_theme *t,
                                      cui_questionnaire_state *s, size_t item, size_t choice,
                                      cui_questionnaire_options o) {
    if (!ui || !id || !t || !s || !s->initialized || item != s->current || item >= s->count ||
        choice >= s->items[item].choice_count)
        return false;
    const cui_questionnaire_item *d = &s->items[item];
    const cui_questionnaire_choice *c = &d->choices[choice];
    bool disabled = o.disabled || s->storage.items[item].disabled ||
                    s->storage.choices[s->storage.items[item].choice_offset + choice].disabled;
    bool selected = cui_questionnaire_choice_selected(s, item, choice);
    cui_str label = c->label.length ? c->label : c->value;
    cui_interaction interaction = cui_interact(
        ui, id, d->multiple ? CUI_ROLE_CHECKBOX : CUI_ROLE_RADIO, label, disabled, selected, 0);
    bool changed = interaction.activated && !disabled;
    if (changed) {
        (void)cui_questionnaire_choose(s, item, choice, cui_frame_time(ui), cui_reduced_motion(ui));
        selected = cui_questionnaire_choice_selected(s, item, choice);
        cui_update_semantic(ui, id, selected, selected ? 1 : 0);
        cui_invalidate(ui);
    }
    cui_questionnaire_metrics m = cui_questionnaire_measure(o.size);
    bool invalid = cui_questionnaire_item_error(s, item).kind != CUI_QUESTIONNAIRE_VALID;
    cui_set_semantic_details(ui, id,
                             (cui_semantic_details){.invalid = invalid,
                                                    .required = d->required,
                                                    .pointer_activatable = true});
    cui_begin_box(ui, id,
                  (cui_box){.width = -1,
                            .padding = m.py,
                            .row = true,
                            .gap = 0,
                            .radius = t->radius_lg,
                            .border_width = 1,
                            .border = invalid    ? t->danger
                                      : selected ? t->primary
                                                 : t->input,
                            .background = interaction.hovered ? t->surface : t->background,
                            .custom_opacity = disabled,
                            .opacity = .5f});
    /* Side spacers supply independent horizontal padding and the card's minimum
     * height without centering the indicator against multi-line descriptions. */
    cui_begin_box(
        ui, 0, (cui_box){.width = m.px - m.py, .fixed_width = true, .height = m.height - 2 * m.py});
    cui_end_box(ui);
    cui_begin_box(ui, 0, (cui_box){.width = -1, .row = true, .gap = m.gap});
    cui_begin_box(ui, 0, (cui_box){.height = m.line, .align_y = CUI_ALIGN_CENTER});
    cui_begin_box(ui, 0,
                  (cui_box){.width = m.indicator,
                            .height = m.indicator,
                            .radius = d->multiple ? t->radius : m.indicator / 2,
                            .border_width = 1,
                            .border = selected ? t->primary : t->input,
                            .background = selected ? t->primary : t->input_background,
                            .align_x = CUI_ALIGN_CENTER,
                            .align_y = CUI_ALIGN_CENTER});
    if (selected) {
        if (d->multiple)
            cui_checkmark(ui, m.indicator - 2, t->on_primary);
        else {
            cui_begin_box(ui, 0,
                          (cui_box){.width = m.mark,
                                    .height = m.mark,
                                    .radius = m.mark / 2,
                                    .background = t->on_primary});
            cui_end_box(ui);
        }
    }
    cui_end_box(ui);
    cui_end_box(ui);
    cui_begin_box(ui, 0, (cui_box){.width = -1, .gap = m.content_gap});
    cui_text_with_line_height(ui, label, m.font, t->text, m.line);
    if (c->description.length)
        cui_text_with_line_height(ui, c->description, m.font > 14 ? m.font - 2 : 12, t->muted,
                                  m.line);
    cui_end_box(ui);
    char key = cui_questionnaire_shortcut(s, item, choice, o.numeric_shortcuts);
    if (key) {
        cui_begin_box(
            ui, 0, (cui_box){.width = m.shortcut, .height = m.line, .align_y = CUI_ALIGN_CENTER});
        cui_begin_box(ui, 0,
                      (cui_box){.width = m.shortcut,
                                .height = m.shortcut,
                                .border_width = 1,
                                .border = t->input,
                                .radius = t->radius,
                                .align_x = CUI_ALIGN_CENTER,
                                .align_y = CUI_ALIGN_CENTER});
        cui_text_ex(ui, (cui_str){&key, 1}, m.font * .625f, t->muted, 0,
                    (cui_text_style){.weight = CUI_TEXT_WEIGHT_MEDIUM, .monospace = true});
        cui_end_box(ui);
        cui_end_box(ui);
    }
    cui_end_box(ui); /* answer row */
    cui_begin_box(ui, 0, (cui_box){.width = m.px - m.py, .fixed_width = true});
    cui_end_box(ui);
    cui_end_box(ui); /* card */
    return changed;
}
void cui_questionnaire_heading(cui_context *ui, cui_id id, const cui_theme *t,
                               const cui_questionnaire_state *s, size_t item,
                               cui_questionnaire_options o) {
    if (!ui || !id || !t || !s || !s->initialized || item >= s->count || item != s->current)
        return;
    const cui_questionnaire_item *d = &s->items[item];
    cui_questionnaire_metrics m = cui_questionnaire_measure(o.size);
    cui_begin_box(ui, id, (cui_box){.width = -1, .gap = m.content_gap});
    cui_text_ex(ui, d->label.length ? d->label : d->name, m.font + 2, t->text, m.line + 4,
                (cui_text_style){.weight = CUI_TEXT_WEIGHT_MEDIUM});
    if (d->description.length)
        cui_text_with_line_height(ui, d->description, m.font > 14 ? m.font - 2 : 12, t->muted,
                                  m.line);
    cui_end_box(ui);
}
void cui_questionnaire_error_control(cui_context *ui, cui_id id, const cui_theme *t,
                                     const cui_questionnaire_state *s, size_t item,
                                     cui_questionnaire_options o) {
    if (!ui || !id || !t || !s || !s->initialized || item >= s->count || item != s->current)
        return;
    cui_questionnaire_error e = cui_questionnaire_item_error(s, item);
    if (e.kind == CUI_QUESTIONNAIRE_VALID)
        return;
    cui_str message = e.kind == CUI_QUESTIONNAIRE_MESSAGE ? e.message
                      : e.kind == CUI_QUESTIONNAIRE_REQUIRED
                          ? cui_string("This question is required")
                          : cui_string("Answer or skip this question");
    cui_interact(ui, id, CUI_ROLE_ALERT, message, false, false, 0);
    cui_begin_box(ui, id, (cui_box){.width = -1});
    cui_text(ui, message, o.size == CUI_QUESTIONNAIRE_LARGE ? 16 : 12, t->danger);
    cui_end_box(ui);
}
bool cui_questionnaire_action_control(cui_context *ui, cui_id id, const cui_theme *t,
                                      cui_questionnaire_state *s, cui_questionnaire_action action,
                                      cui_str label, cui_questionnaire_options o) {
    if (!ui || !id || !t || !s || !s->initialized || s->current >= s->count)
        return false;
    size_t position = cui_questionnaire_position(s), total = cui_questionnaire_total(s);
    bool visible = action == CUI_QUESTIONNAIRE_PREVIOUS ? position > 0
                   : action == CUI_QUESTIONNAIRE_SKIP   ? !s->items[s->current].required
                   : action == CUI_QUESTIONNAIRE_NEXT
                       ? position + 1 < total
                       : action == CUI_QUESTIONNAIRE_FINISH && position + 1 == total;
    if (!visible)
        return false;
    if (!label.length)
        label = cui_string(action == CUI_QUESTIONNAIRE_PREVIOUS ? "Previous"
                           : action == CUI_QUESTIONNAIRE_SKIP   ? "Skip"
                           : action == CUI_QUESTIONNAIRE_NEXT   ? "Next"
                                                                : "Submit");
    bool activated = cui_button_ex(
        ui, id, t, label,
        (cui_button_options){.size = (cui_control_size)o.size,
                             .disabled = o.disabled,
                             .variant = action == CUI_QUESTIONNAIRE_PREVIOUS ? CUI_BUTTON_OUTLINE
                                        : action == CUI_QUESTIONNAIRE_SKIP   ? CUI_BUTTON_GHOST
                                                                             : CUI_BUTTON_PRIMARY});
    if (!activated)
        return false;
    bool result = action == CUI_QUESTIONNAIRE_PREVIOUS ? cui_questionnaire_previous(s)
                  : action == CUI_QUESTIONNAIRE_SKIP   ? cui_questionnaire_skip(s)
                  : action == CUI_QUESTIONNAIRE_NEXT   ? cui_questionnaire_next(s)
                                                       : cui_questionnaire_submit(s);
    cui_invalidate(ui);
    return result;
}
cui_questionnaire_result cui_questionnaire_input_control(cui_context *ui, cui_id id,
                                                         const cui_theme *t,
                                                         cui_questionnaire_state *s, size_t item,
                                                         cui_questionnaire_input_state *view,
                                                         const cui_platform *clipboard,
                                                         cui_questionnaire_options o) {
    if (!ui || !id || !t || !s || !s->initialized || !view)
        return CUI_QUESTIONNAIRE_INVALID;
    if (item >= s->count)
        return CUI_QUESTIONNAIRE_UNKNOWN_ITEM;
    const cui_questionnaire_item *d = &s->items[item];
    if (!d->input)
        return CUI_QUESTIONNAIRE_ANSWER_MISMATCH;
    if (item != s->current)
        return CUI_QUESTIONNAIRE_OK;
    if (!o.disabled && !s->storage.items[item].disabled && !d->input_disabled &&
        cui_focused(ui) == id && cui_questionnaire_freeform(s, item).length) {
        size_t count;
        const cui_event *events = cui_frame_events(ui, &count);
        for (size_t n = 0; n < count; n++)
            if (!cui_event_consumed(ui, n) && events[n].type == CUI_EVENT_KEY_DOWN &&
                events[n].key == CUI_KEY_ENTER && !events[n].repeat && !events[n].modifiers) {
                if (cui_questionnaire_confirm(s)) {
                    cui_consume_event(ui, n);
                    cui_invalidate(ui);
                }
                if (s->current != item)
                    return CUI_QUESTIONNAIRE_OK;
            }
    }
    cui_questionnaire_metrics m = cui_questionnaire_measure(o.size);
    cui_input_state input = {.behavior = {.dragging = view->dragging}, .scroll_x = view->scroll_x};
    cui_input_result result = cui_input_ex(
        ui, id, t, d->input_label.length ? d->input_label : d->label, d->input, &input,
        (cui_input_options){
            .width = -1,
            .size = (cui_input_size)o.size,
            .disabled = o.disabled || s->storage.items[item].disabled || d->input_disabled,
            .required = d->required,
            .invalid = cui_questionnaire_item_error(s, item).kind != CUI_QUESTIONNAIRE_VALID,
            .clipboard = clipboard,
            .custom_padding = true,
            .padding_x = m.px});
    view->dragging = input.behavior.dragging;
    view->scroll_x = input.scroll_x;
    if (result.error != CUI_EDIT_OK)
        return CUI_QUESTIONNAIRE_INVALID;
    if (result.changed) {
        cui_questionnaire_result sync = cui_questionnaire_sync_input(s, item, true);
        cui_invalidate(ui);
        return sync;
    }
    return CUI_QUESTIONNAIRE_OK;
}
bool cui_questionnaire_radio_keys(cui_context *ui, cui_questionnaire_state *s, const cui_id *ids,
                                  size_t count) {
    if (!ui || !s || !s->initialized || s->current >= s->count || !ids)
        return false;
    size_t item = s->current;
    const cui_questionnaire_item *d = &s->items[item];
    if (d->multiple || count != d->choice_count || s->storage.items[item].disabled)
        return false;
    bool changed = false;
    size_t event_count;
    const cui_event *events = cui_frame_events(ui, &event_count);
    for (size_t e = 0; e < event_count && s->current == item; e++) {
        if (cui_event_consumed(ui, e) || events[e].type != CUI_EVENT_KEY_DOWN || events[e].repeat ||
            events[e].modifiers ||
            (events[e].key != CUI_KEY_LEFT && events[e].key != CUI_KEY_RIGHT))
            continue;
        size_t current = SIZE_MAX;
        for (size_t c = 0; c < count; c++)
            if (ids[c] && cui_focused(ui) == ids[c] &&
                !s->storage.choices[s->storage.items[item].choice_offset + c].disabled)
                current = c;
        if (current == SIZE_MAX)
            continue;
        for (size_t step = 1; step <= count; step++) {
            size_t next = events[e].key == CUI_KEY_LEFT ? (current + count - step) % count
                                                        : (current + step) % count;
            if (!ids[next] ||
                s->storage.choices[s->storage.items[item].choice_offset + next].disabled)
                continue;
            (void)cui_questionnaire_activate(s, item, next);
            cui_focus(ui, ids[next]);
            cui_consume_event(ui, e);
            cui_invalidate(ui);
            changed = true;
            break;
        }
    }
    return changed;
}
