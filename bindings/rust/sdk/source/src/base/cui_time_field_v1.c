/* SPDX-License-Identifier: MIT */
#include "cui_time_field_v1_internal.h"
#include <string.h>

bool cui_time_field_v1_equal(cui_time a, cui_time b) {
    return a.hour == b.hour && a.minute == b.minute && a.second == b.second;
}
cui_str cui_time_field_v1_name(const cui_time_field_options_v1 *o, unsigned i) {
    static const char *names[] = {"Hour", "Minute", "Second", "AM/PM"};
    return o->segment_names[i].length ? o->segment_names[i] : cui_string(names[i]);
}
static bool cui_tfw_state(const cui_time_field_state_v1 *s) {
    return s && s->struct_size == sizeof(*s) && s->version == 1 && !s->reserved[0] &&
           !s->reserved[1] && !s->reserved_state && !s->reserved_tail && s->segment < 4 &&
           s->digit_count < 2 && (!s->digit_count || s->draft < 10) && s->last_cycle < 2 &&
           s->last_precision < 2 && s->pending_length <= 16 && s->edit_generation &&
           (s->validation == CUI_TIME_TEXT_OK || s->validation == CUI_TIME_TEXT_INVALID_SYNTAX ||
            s->validation == CUI_TIME_TEXT_INVALID_VALUE ||
            s->validation == CUI_TIME_TEXT_OUT_OF_RANGE);
}
static cui_time_text_status_v1 cui_tfw_config(const cui_time_field_options_v1 *o) {
    if (!o)
        return CUI_TIME_TEXT_INVALID_ARGUMENT;
    if (o->struct_size != sizeof(*o) || o->version != 1 || o->reserved || (o->flags & ~7u) ||
        !o->edit_generation)
        return CUI_TIME_TEXT_INVALID_CONFIG;
    cui_time_text_status_v1 status = cui_time_validate_v1((cui_time){0}, &o->text);
    if (status == CUI_TIME_TEXT_INVALID_CONFIG || status == CUI_TIME_TEXT_INVALID_ARGUMENT)
        return CUI_TIME_TEXT_INVALID_CONFIG;
    for (unsigned i = 0; i < 4; i++) {
        cui_str n = o->segment_names[i];
        if ((!n.data && n.length) || n.length > 64 ||
            (n.length && (!cui_edit_valid_utf8(n) || memchr(n.data, 0, n.length))))
            return CUI_TIME_TEXT_INVALID_CONFIG;
    }
    return CUI_TIME_TEXT_OK;
}
cui_time_text_status_v1 cui_time_field_options_init_v1(cui_time_field_options_v1 *out) {
    if (!out)
        return CUI_TIME_TEXT_INVALID_ARGUMENT;
    cui_time_field_options_v1 o = {.struct_size = sizeof(o), .version = 1, .edit_generation = 1};
    cui_time_text_options_init_v1(&o.text);
    *out = o;
    return CUI_TIME_TEXT_OK;
}
cui_time_text_status_v1 cui_time_field_state_init_v1(cui_time_field_state_v1 *out, cui_time value,
                                                     const cui_time_field_options_v1 *o) {
    if (!out)
        return CUI_TIME_TEXT_INVALID_ARGUMENT;
    cui_time_text_status_v1 status = cui_tfw_config(o);
    if (status != CUI_TIME_TEXT_OK)
        return status;
    status = cui_time_validate_v1(value, &o->text);
    if (status != CUI_TIME_TEXT_OK && status != CUI_TIME_TEXT_OUT_OF_RANGE)
        return status;
    *out = (cui_time_field_state_v1){.struct_size = sizeof(*out),
                                     .version = 1,
                                     .validation = (uint32_t)status,
                                     .last_cycle = o->text.cycle,
                                     .last_precision = o->text.precision,
                                     .edit_generation = o->edit_generation,
                                     .last_value = value};
    return CUI_TIME_TEXT_OK;
}
cui_time_text_status_v1 cui_time_field_clear_validation_v1(cui_time_field_state_v1 *s) {
    if (!cui_tfw_state(s))
        return CUI_TIME_TEXT_INVALID_ARGUMENT;
    s->validation = CUI_TIME_TEXT_OK;
    return CUI_TIME_TEXT_OK;
}
static bool cui_tfw_visible(const cui_time_field_options_v1 *o, unsigned i) {
    return i < 2 || (i == 2 && o->text.precision) || (i == 3 && o->text.cycle);
}
static unsigned cui_tfw_next(const cui_time_field_options_v1 *o, unsigned i, bool forward) {
    unsigned found = i;
    if (forward) {
        for (unsigned j = i + 1; j < 4; j++)
            if (cui_tfw_visible(o, j)) {
                found = j;
                break;
            }
    } else {
        for (unsigned j = i; j > 0; j--)
            if (cui_tfw_visible(o, j - 1)) {
                found = j - 1;
                break;
            }
    }
    return found;
}
static void cui_tfw_reset(cui_time_field_state_v1 *s) {
    s->digit_count = 0;
    s->draft = 0;
    s->pending_length = 0;
}
static unsigned cui_tfw_value(cui_time t, const cui_time_field_options_v1 *o, unsigned segment) {
    if (!segment)
        return o->text.cycle ? (t.hour % 12 ? t.hour % 12 : 12) : t.hour;
    if (segment == 1)
        return t.minute;
    if (segment == 2)
        return t.second;
    return t.hour >= 12;
}
static bool cui_tfw_replace(cui_time_field_state_v1 *s, cui_time *t,
                            const cui_time_field_options_v1 *o, unsigned value) {
    cui_time next = *t;
    if (!s->segment)
        next.hour = o->text.cycle ? value % 12 + (t->hour >= 12 ? 12 : 0) : value;
    else if (s->segment == 1)
        next.minute = value;
    else if (s->segment == 2)
        next.second = value;
    else
        next.hour = t->hour % 12 + (value ? 12 : 0);
    cui_time_text_status_v1 status = cui_time_validate_v1(next, &o->text);
    s->validation = (uint32_t)status;
    if (status != CUI_TIME_TEXT_OK)
        return false;
    *t = next;
    return true;
}
static cui_str cui_tfw_token(const cui_time_locale_v1 *l, unsigned i) {
    if (i < 10)
        return l->digits[i];
    if (i == 10)
        return l->separator;
    if (i == 11)
        return l->period_separator;
    return i == 12 ? l->am : l->pm;
}
static bool cui_tfw_digit(cui_time_field_state_v1 *s, cui_time *t,
                          const cui_time_field_options_v1 *o, unsigned digit) {
    if (s->segment == 3) {
        s->validation = CUI_TIME_TEXT_INVALID_VALUE;
        return false;
    }
    unsigned maximum = s->segment == 0 ? (o->text.cycle ? 12 : 23) : 59;
    unsigned minimum = s->segment == 0 && o->text.cycle ? 1 : 0;
    unsigned pair = s->draft * 10 + digit;
    bool complete = s->digit_count && pair >= minimum && pair <= maximum;
    unsigned value = complete ? pair : digit;
    if (!complete)
        complete = digit * 10 > maximum;
    s->digit_count = complete ? 0 : 1;
    s->draft = digit;
    bool accepted;
    if (value < minimum || value > maximum) {
        s->validation = CUI_TIME_TEXT_INVALID_VALUE;
        accepted = false;
    } else
        accepted = cui_tfw_replace(s, t, o, value);
    if (complete)
        s->segment = cui_tfw_next(o, s->segment, true);
    return accepted;
}
static cui_str cui_tfw_text(const cui_event *e) {
    if (e->text_data || e->text_length)
        return (cui_str){e->text_data, e->text_length};
    size_t n = 0;
    while (n < sizeof(e->text) && e->text[n])
        n++;
    return (cui_str){e->text, n};
}
static bool cui_tfw_text_edit(cui_time_field_state_v1 *s, cui_time *t,
                              const cui_time_field_options_v1 *o, cui_str text) {
    if (!text.length || text.length > 160 || !text.data) {
        s->pending_length = 0;
        s->validation = CUI_TIME_TEXT_INVALID_SYNTAX;
        return false;
    }
    cui_time full;
    cui_time_text_status_v1 parsed = cui_time_parse_v1(text, &o->text, &full);
    if (parsed == CUI_TIME_TEXT_OK) {
        *t = full;
        cui_tfw_reset(s);
        s->validation = 0;
        return true;
    }
    if (parsed == CUI_TIME_TEXT_INVALID_VALUE || parsed == CUI_TIME_TEXT_OUT_OF_RANGE) {
        /* Complete grammar with an invalid candidate is an atomic full-value
         * attempt, never a segmented stream that can replace its diagnosis. */
        s->pending_length = 0;
        s->validation = (uint32_t)parsed;
        return false;
    }
    cui_time_locale_v1 ascii;
    cui_time_locale_init_v1(&ascii);
    const cui_time_locale_v1 *l = o->text.locale ? o->text.locale : &ascii;
    if (!o->text.locale && s->segment == 3 && text.length == 1 &&
        (text.data[0] == 'a' || text.data[0] == 'A' || text.data[0] == 'p' ||
         text.data[0] == 'P')) {
        cui_tfw_reset(s);
        return cui_tfw_replace(s, t, o, text.data[0] == 'p' || text.data[0] == 'P');
    }
    cui_time_field_state_v1 draft = *s;
    cui_time next = *t;
    bool accepted = true, special = false;
    size_t tokens = 0;
    for (size_t at = 0; at < text.length; at++) {
        if (draft.pending_length >= 16)
            goto syntax;
        draft.pending[draft.pending_length++] = text.data[at];
        bool prefix = false;
        unsigned match = 14;
        for (unsigned i = 0; i < 14; i++) {
            cui_str token = cui_tfw_token(l, i);
            if (draft.pending_length <= token.length &&
                !memcmp(draft.pending, token.data, draft.pending_length)) {
                prefix = true;
                if (draft.pending_length == token.length)
                    match = i;
            }
        }
        if (!prefix)
            goto syntax;
        if (match == 14)
            continue;
        draft.pending_length = 0;
        tokens++;
        if (match < 10) {
            if (!cui_tfw_digit(&draft, &next, o, match))
                accepted = false;
        } else if (match >= 12) {
            special = true;
            if (draft.segment != 3 || !o->text.cycle)
                goto syntax;
            cui_tfw_reset(&draft);
            if (!cui_tfw_replace(&draft, &next, o, match == 13))
                accepted = false;
        } else {
            special = true;
            unsigned segment = cui_tfw_next(o, draft.segment, true);
            if ((match == 10 && (draft.segment > 1 || segment == 3)) ||
                (match == 11 && (!o->text.cycle || draft.segment == 3)))
                goto syntax;
            draft.segment = match == 11 ? 3 : segment;
            cui_tfw_reset(&draft);
            draft.validation = 0;
        }
    }
    if (special && tokens > 1) {
        s->validation =
            (uint32_t)(parsed == CUI_TIME_TEXT_OK ? CUI_TIME_TEXT_INVALID_SYNTAX : parsed);
        s->pending_length = 0;
        return false;
    }
    *s = draft;
    *t = next;
    return accepted;
syntax:
    s->pending_length = 0;
    s->validation = CUI_TIME_TEXT_INVALID_SYNTAX;
    return false;
}
static void cui_tfw_submit(cui_context *ui, cui_id id, cui_str name,
                           const cui_time_field_state_v1 *s, const cui_time *t,
                           const cui_time_field_options_v1 *o) {
    (void)s;
    bool disabled = (o->flags & CUI_TIME_FIELD_DISABLED_V1) != 0;
    cui_interact(ui, id, CUI_ROLE_GROUP, name, disabled, false, 0);
    cui_set_tab_stop(ui, id, false);
    for (unsigned i = 0; i < 4; i++)
        if (cui_tfw_visible(o, i)) {
            cui_id child = cui_time_segment_id(id, i);
            cui_interact(ui, child, CUI_ROLE_TEXTBOX, cui_time_field_v1_name(o, i), disabled, false,
                         (float)cui_tfw_value(*t, o, i));
            cui_set_semantic_details(
                ui, child,
                (cui_semantic_details){.readonly = (o->flags & CUI_TIME_FIELD_READONLY_V1) != 0,
                                       .parent = id,
                                       .minimum = i == 0 && o->text.cycle ? 1 : 0,
                                       .maximum = i == 0   ? (o->text.cycle ? 12 : 23)
                                                  : i == 3 ? 1
                                                           : 59});
            cui_set_tab_stop(ui, child, true);
        }
}
cui_time_field_result_v1 cui_time_field_v1_compose(cui_context *ui, cui_id id, cui_str name,
                                                   cui_time_field_state_v1 *s, cui_time *t,
                                                   const cui_time_field_options_v1 *o) {
    cui_time_field_result_v1 r = {.status = CUI_TIME_TEXT_INVALID_ARGUMENT};
    if (!ui || !id || !t || !cui_tfw_state(s) || (!name.data && name.length) ||
        !cui_frame_is_submitting(ui))
        return r;
    r.status = cui_tfw_config(o);
    if (r.status != CUI_TIME_TEXT_OK)
        return r;
    r.status = cui_time_validate_v1(*t, &o->text);
    if (r.status != CUI_TIME_TEXT_OK && r.status != CUI_TIME_TEXT_OUT_OF_RANGE)
        return r;
    if (cui_last_error(ui) != CUI_OK) {
        r.status = CUI_TIME_TEXT_INVALID_ARGUMENT;
        return r;
    }
    cui_time before = *t;
    uint64_t frame = cui_frame_sequence(ui);
    bool writable = frame && cui_frame_paint_enabled(ui) && !(o->flags & 3u) &&
                    cui_pointer_scope_allowed(ui, 0);
    if (writable && s->last_frame_sequence == frame) {
        r.status = CUI_TIME_TEXT_INVALID_ARGUMENT;
        return r;
    }
    bool consecutive = frame > 1 && s->last_frame_sequence == frame - 1;
    bool ready = writable && consecutive && cui_frame_input_snapshot_ready(ui);
    if (writable) {
        bool reset = !consecutive || !cui_time_field_v1_equal(s->last_value, *t) ||
                     s->edit_generation != o->edit_generation || s->last_cycle != o->text.cycle ||
                     s->last_precision != o->text.precision;
        if (reset) {
            cui_tfw_reset(s);
            s->validation = (uint32_t)r.status;
        }
        if (!cui_tfw_visible(o, s->segment))
            s->segment = 0;
        s->edit_generation = o->edit_generation;
        s->last_cycle = o->text.cycle;
        s->last_precision = o->text.precision;
        cui_id focus = cui_focused(ui);
        bool focused = false;
        for (unsigned i = 0; i < 4; i++)
            if (cui_tfw_visible(o, i) && focus == cui_time_segment_id(id, i)) {
                focused = true;
                if (s->segment != i)
                    cui_tfw_reset(s);
                s->segment = i;
            }
        if (!focused)
            cui_tfw_reset(s);
        size_t count = 0;
        const cui_event *events = cui_frame_events(ui, &count);
        cui_id source = 0;
        const cui_id dispatched_focus = focus;
        for (size_t e = 0; ready && e < count; e++) {
            if (!cui_frame_input_snapshot_ready(ui) || cui_last_error(ui) != CUI_OK) {
                r.status = CUI_TIME_TEXT_INVALID_ARGUMENT;
                break;
            }
            if (cui_event_consumed(ui, e) || !cui_pointer_scope_allowed(ui, 0))
                continue;
            cui_id target = cui_event_target(ui, e);
            unsigned segment = 4;
            for (unsigned i = 0; i < 4; i++)
                if (cui_tfw_visible(o, i) && target == cui_time_segment_id(id, i))
                    segment = i;
            if (segment == 4)
                continue;
            const cui_event *event = &events[e];
            if (event->type == CUI_EVENT_KEY_DOWN &&
                (event->ime_composing || event->prefer_character_input))
                continue;
            if (event->type == CUI_EVENT_POINTER_DOWN &&
                event->pointer_button == CUI_POINTER_PRIMARY) {
                s->segment = segment;
                source = target;
                cui_tfw_reset(s);
                cui_focus(ui, target);
                cui_consume_event(ui, e);
                continue;
            }
            if (event->type != CUI_EVENT_TEXT && event->type != CUI_EVENT_KEY_DOWN)
                continue;
            if (event->type == CUI_EVENT_KEY_DOWN) {
                if (event->key == CUI_KEY_TAB) {
                    /* Core already performed global traversal; preserve its focus
                     * after any earlier local auto-advance, without retargeting. */
                    cui_tfw_reset(s);
                    cui_focus(ui, dispatched_focus);
                    continue;
                }
                if (event->modifiers ||
                    (event->key != CUI_KEY_LEFT && event->key != CUI_KEY_RIGHT &&
                     event->key != CUI_KEY_ESCAPE && event->key != CUI_KEY_UP &&
                     event->key != CUI_KEY_DOWN && event->key != CUI_KEY_HOME &&
                     event->key != CUI_KEY_END && event->key != CUI_KEY_BACKSPACE &&
                     event->key != CUI_KEY_DELETE))
                    continue;
            }
            /* Dispatch targets are immutable evidence, not the evolving editor
             * selection. Consecutive TEXT events may all target the entry Hour. */
            if (source != target && (source || s->segment != segment)) {
                s->segment = segment;
                cui_tfw_reset(s);
            }
            source = target;
            if (event->type == CUI_EVENT_TEXT) {
                if (!cui_tfw_text_edit(s, t, o, cui_tfw_text(event)))
                    r.rejected_count++;
                cui_consume_event(ui, e);
                cui_focus(ui, cui_time_segment_id(id, s->segment));
                continue;
            }
            cui_key key = event->key;
            if (key == CUI_KEY_TAB || event->modifiers)
                continue;
            bool recognized = true, accepted = true;
            if (key == CUI_KEY_LEFT || key == CUI_KEY_RIGHT) {
                s->segment = cui_tfw_next(o, s->segment, key == CUI_KEY_RIGHT);
                cui_tfw_reset(s);
                s->validation = 0;
                cui_focus(ui, cui_time_segment_id(id, s->segment));
            } else if (key == CUI_KEY_ESCAPE)
                cui_tfw_reset(s);
            else if (key == CUI_KEY_UP || key == CUI_KEY_DOWN || key == CUI_KEY_HOME ||
                     key == CUI_KEY_END || key == CUI_KEY_BACKSPACE || key == CUI_KEY_DELETE) {
                unsigned minimum = s->segment == 0 && o->text.cycle ? 1 : 0;
                unsigned maximum = s->segment == 0   ? (o->text.cycle ? 12 : 23)
                                   : s->segment == 3 ? 1
                                                     : 59;
                unsigned value = cui_tfw_value(*t, o, s->segment);
                if (key == CUI_KEY_BACKSPACE || key == CUI_KEY_DELETE)
                    value = 0;
                else if (key == CUI_KEY_HOME)
                    value = minimum;
                else if (key == CUI_KEY_END)
                    value = maximum;
                else if (key == CUI_KEY_UP)
                    value = value >= maximum ? minimum : value + 1;
                else
                    value = value <= minimum ? maximum : value - 1;
                cui_tfw_reset(s);
                accepted = cui_tfw_replace(s, t, o, value);
            } else
                recognized = false;
            if (recognized) {
                cui_consume_event(ui, e);
                if (!accepted)
                    r.rejected_count++;
            }
        }
        if (ready) {
            cui_id focused_id = cui_focused(ui);
            bool visible_focus = false;
            for (unsigned i = 0; i < 4; i++)
                if (cui_tfw_visible(o, i) && focused_id == cui_time_segment_id(id, i)) {
                    visible_focus = true;
                    if (s->segment != i)
                        cui_tfw_reset(s);
                    s->segment = i;
                }
            if (!visible_focus)
                cui_tfw_reset(s);
        }
        s->last_frame_sequence = frame;
        s->last_value = *t;
    }
    if (r.status != CUI_TIME_TEXT_INVALID_ARGUMENT)
        r.status = CUI_TIME_TEXT_OK;
    cui_tfw_submit(ui, id, name, s, t, o);
    if (cui_last_error(ui) != CUI_OK)
        r.status = CUI_TIME_TEXT_INVALID_ARGUMENT;
    r.changed = !cui_time_field_v1_equal(before, *t);
    r.validation = (cui_time_text_status_v1)s->validation;
    if (r.changed)
        cui_invalidate(ui);
    return r;
}
void cui_time_field_v1_observe(cui_context *ui, cui_id id, cui_time before, cui_time *t,
                               const cui_time_field_options_v1 *o, cui_time_handler handler,
                               cui_time_field_result_v1 *r) {
    if (r->status != CUI_TIME_TEXT_OK || !r->changed || !handler.on_change || (o->flags & 3u) ||
        !cui_frame_paint_enabled(ui) || !cui_pointer_scope_allowed(ui, 0) ||
        !cui_frame_input_snapshot_ready(ui))
        return;
    uint64_t frame = cui_frame_sequence(ui);
    if (!frame)
        return;
    cui_time_change event = {.event = {.kind = CUI_COMPONENT_EVENT_CHANGE,
                                       .target = id,
                                       .source = CUI_COMPONENT_EVENT_SOURCE_UNKNOWN,
                                       .flags = CUI_COMPONENT_EVENT_HAS_FRAME,
                                       .frame_sequence = frame},
                             .previous = before,
                             .value = *t};
    handler.on_change(handler.user, &event);
    r->change_count = 1;
}
cui_time_field_result_v1 cui_base_time_field_v1(cui_context *ui, cui_id id, cui_str name,
                                                cui_time_field_state_v1 *s, cui_time *t,
                                                const cui_time_field_options_v1 *o,
                                                cui_time_handler handler) {
    cui_time before = t ? *t : (cui_time){0};
    cui_time_field_result_v1 r = cui_time_field_v1_compose(ui, id, name, s, t, o);
    if (r.status == CUI_TIME_TEXT_OK)
        cui_time_field_v1_observe(ui, id, before, t, o, handler, &r);
    return r;
}
