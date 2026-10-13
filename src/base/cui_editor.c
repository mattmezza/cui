/* SPDX-License-Identifier: MIT */
#include "cui/cui_editor.h"
#include <math.h>
#include <string.h>
bool cui_editor_state_init(cui_editor_state *s, cui_edit *edit, cui_textarea_line *lines,
                           size_t capacity) {
    if (!s || !edit || !cui_edit_get_state(edit).multiline)
        return false;
    cui_editor_state next = {.edit = edit};
    if (!cui_textarea_state_init(&next.view, lines, capacity))
        return false;
    *s = next;
    return true;
}
cui_editor_options cui_editor_default(void) {
    cui_editor_options o = {.textarea = cui_textarea_default(), .line_numbers = true};
    o.textarea.no_wrap = true;
    o.textarea.text_style.monospace = true;
    o.textarea.rows = 12;
    return o;
}
static bool cui_editor_boundary(cui_str text, size_t i) {
    return i == text.length || (((unsigned char)text.data[i] & 0xc0) != 0x80);
}
cui_edit_result cui_editor_find(cui_editor_state *s, cui_str query, bool reverse, bool wrap) {
    if (!s || !s->edit || !query.length || !query.data || !cui_edit_valid_utf8(query))
        return CUI_EDIT_INVALID_ARGUMENT;
    cui_str text = cui_edit_get_text(s->edit);
    if (query.length > text.length)
        return CUI_EDIT_BOUNDARY;
    cui_edit_state selection = cui_edit_get_state(s->edit);
    size_t first = selection.cursor < selection.anchor ? selection.cursor : selection.anchor;
    size_t last = selection.cursor > selection.anchor ? selection.cursor : selection.anchor;
    size_t found = SIZE_MAX, wrapped = SIZE_MAX;
    for (size_t i = 0; i <= text.length - query.length; ++i) {
        if (!cui_editor_boundary(text, i) || !cui_editor_boundary(text, i + query.length) ||
            memcmp(text.data + i, query.data, query.length))
            continue;
        if (reverse) {
            wrapped = i;
            if (i + query.length <= first)
                found = i;
        } else {
            if (wrapped == SIZE_MAX)
                wrapped = i;
            if (found == SIZE_MAX && i >= last)
                found = i;
        }
    }
    if (found == SIZE_MAX && wrap)
        found = wrapped;
    return found == SIZE_MAX ? CUI_EDIT_BOUNDARY
                             : cui_edit_select(s->edit, found, found + query.length);
}
cui_id cui_editor_completion_item_id(cui_id id, size_t index) {
    return id ^ UINT64_C(0x6641ab75da98e031) ^
           ((uint64_t)(index + 1) * UINT64_C(0x9e3779b97f4a7c15));
}
static cui_edit_result cui_editor_completion_events(cui_context *ui, cui_id id,
                                                    cui_editor_state *state,
                                                    cui_textarea_options options) {
    if (!state->completion.open)
        return CUI_EDIT_OK;
    cui_edit_state current = cui_edit_get_state(state->edit);
    if (state->completion.revision != current.revision ||
        state->completion.cursor != current.cursor || state->completion.anchor != current.anchor ||
        (cui_focused(ui) && cui_focused(ui) != id)) {
        cui_editor_cancel_completion(state);
        return CUI_EDIT_OK;
    }
    if (!state->completion.items || !state->completion.count ||
        state->completion.selected >= state->completion.count) {
        cui_editor_cancel_completion(state);
        return CUI_EDIT_INVALID_ARGUMENT;
    }
    if (options.disabled || options.readonly)
        return CUI_EDIT_OK;
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    for (size_t i = 0; i < count && state->completion.open; ++i) {
        cui_event e = events[i];
        if (cui_event_consumed(ui, i))
            continue;
        cui_id target = cui_event_target(ui, i);
        bool option = false;
        for (size_t row = 0; target != id && !option && row < state->completion.count; ++row)
            option = target == cui_editor_completion_item_id(id, row);
        if (e.type == CUI_EVENT_CANCEL || (e.type == CUI_EVENT_POINTER_DOWN && !option)) {
            cui_editor_cancel_completion(state);
            break;
        }
        if (target != id && !option)
            continue;
        /* Never accept before an earlier queued text/navigation edit. Let the
         * textarea process that queue in order, then discard stale proposals. */
        if (e.type == CUI_EVENT_TEXT)
            break;
        if (e.type != CUI_EVENT_KEY_DOWN)
            continue;
        if (e.ime_composing || e.prefer_character_input ||
            (e.modifiers & (CUI_MOD_CTRL | CUI_MOD_META | CUI_MOD_ALT)))
            break;
        if (e.key == CUI_KEY_UP) {
            state->completion.selected = state->completion.selected ? state->completion.selected - 1
                                                                    : state->completion.count - 1;
        } else if (e.key == CUI_KEY_DOWN) {
            state->completion.selected = (state->completion.selected + 1) % state->completion.count;
        } else if (e.key == CUI_KEY_ENTER && !(e.modifiers & CUI_MOD_SHIFT)) {
            cui_edit_result result = cui_editor_accept_completion(state);
            cui_consume_event(ui, i);
            return result;
        } else if (e.key == CUI_KEY_ESCAPE) {
            cui_editor_cancel_completion(state);
        } else
            break;
        cui_consume_event(ui, i);
        cui_invalidate(ui);
    }
    return CUI_EDIT_OK;
}
cui_textarea_result cui_base_editor(cui_context *ui, cui_id id, cui_str name, cui_editor_state *s,
                                    cui_editor_options o, cui_textarea_metrics metrics) {
    if (!ui || !id || !s || !s->edit || !s->view.lines || !s->view.capacity ||
        s->view.capacity > 65536 || !isfinite(metrics.font_size) || metrics.font_size <= 0 ||
        !isfinite(metrics.line_height) || metrics.line_height <= 0 ||
        o.textarea.text_style.weight < CUI_TEXT_WEIGHT_NORMAL ||
        o.textarea.text_style.weight > CUI_TEXT_WEIGHT_BOLD || (name.length && !name.data) ||
        (o.language.length && !o.language.data))
        return (cui_textarea_result){.error = CUI_TEXTAREA_INVALID};
    cui_textarea_state probe = s->view;
    float width = probe.viewport.w > 0 ? probe.viewport.w : probe.wrap_width;
    cui_textarea_error layout_error = cui_textarea_layout_ex(
        ui, s->edit, &probe, metrics, width, !o.textarea.no_wrap, o.textarea.text_style);
    if (layout_error != CUI_TEXTAREA_OK)
        return (cui_textarea_result){.error = layout_error};
    cui_edit_state before = cui_edit_get_state(s->edit);
    cui_edit_result completion_error = cui_editor_completion_events(ui, id, s, o.textarea);
    cui_textarea_result result =
        cui_base_textarea(ui, id, name, s->edit, &s->view, o.textarea, metrics);
    cui_edit_state after = cui_edit_get_state(s->edit);
    result.changed |= before.revision != after.revision;
    result.selection_changed |= before.cursor != after.cursor || before.anchor != after.anchor;
    if (completion_error != CUI_EDIT_OK)
        result.edit_error = completion_error;
    if (s->completion.open &&
        (s->completion.revision != after.revision || result.selection_changed))
        cui_editor_cancel_completion(s);
    return result;
}

bool cui_editor_search_init(cui_editor_search_state *s, cui_edit *query) {
    if (!s || !query || cui_edit_get_state(query).multiline)
        return false;
    *s = (cui_editor_search_state){.query = query};
    return true;
}
size_t cui_editor_match_count(const cui_editor_state *s, cui_str query) {
    if (!s || !s->edit || !query.length || !query.data || !cui_edit_valid_utf8(query))
        return 0;
    cui_str text = cui_edit_get_text(s->edit);
    if (query.length > text.length)
        return 0;
    size_t count = 0, i = 0;
    while (i <= text.length - query.length) {
        if (cui_editor_boundary(text, i) && cui_editor_boundary(text, i + query.length) &&
            !memcmp(text.data + i, query.data, query.length)) {
            ++count;
            i += query.length;
        } else {
            ++i;
        }
    }
    return count;
}

bool cui_editor_set_folds(cui_editor_state *state, cui_editor_fold *folds, size_t count) {
    return state && cui_textarea_set_folds(&state->view, state->edit, folds, count);
}
cui_edit_result cui_editor_set_folded(cui_editor_state *state, size_t index, bool collapsed) {
    if (!state || !state->edit || index >= state->view.fold_count ||
        state->view.fold_revision != cui_edit_get_state(state->edit).revision)
        return CUI_EDIT_INVALID_ARGUMENT;
    if (!cui_textarea_set_folds(&state->view, state->edit, state->view.folds,
                                state->view.fold_count))
        return CUI_EDIT_INVALID_ARGUMENT;
    cui_editor_fold *fold = &state->view.folds[index];
    cui_str text = cui_edit_get_text(state->edit);
    cui_edit_state selection = cui_edit_get_state(state->edit);
    size_t line = 0, cursor_line = 0, anchor_line = 0, opening_end = 0;
    for (size_t i = 0; i <= text.length; ++i) {
        if (i < text.length && text.data[i] != '\r' && text.data[i] != '\n')
            continue;
        if (line == fold->start_line)
            opening_end = i;
        if (i == text.length)
            break;
        size_t next = i + 1;
        if (text.data[i] == '\r' && next < text.length && text.data[next] == '\n')
            ++next;
        if (next <= selection.cursor)
            ++cursor_line;
        if (next <= selection.anchor)
            ++anchor_line;
        ++line;
        i = next - 1;
    }
    if (collapsed && ((cursor_line > fold->start_line && cursor_line < fold->end_line) ||
                      (anchor_line > fold->start_line && anchor_line < fold->end_line))) {
        cui_edit_result error = cui_edit_select(state->edit, opening_end, opening_end);
        if (error != CUI_EDIT_OK)
            return error;
    }
    fold->collapsed = collapsed;
    state->view.preferred_valid = false;
    return CUI_EDIT_OK;
}

void cui_editor_cancel_completion(cui_editor_state *state) {
    if (state)
        state->completion = (cui_editor_completion_state){0};
}
cui_edit_result cui_editor_show_completions(cui_editor_state *state,
                                            const cui_editor_completion_item *items, size_t count,
                                            size_t start, size_t end) {
    if (!state || !state->edit || count > 65536 || (count && !items))
        return CUI_EDIT_INVALID_ARGUMENT;
    if (!count) {
        cui_editor_cancel_completion(state);
        return CUI_EDIT_OK;
    }
    cui_str text = cui_edit_get_text(state->edit);
    if (start > end || end > text.length || !cui_editor_boundary(text, start) ||
        !cui_editor_boundary(text, end))
        return CUI_EDIT_INVALID_ARGUMENT;
    for (size_t i = 0; i < count; ++i) {
        if (!items[i].label.length || !items[i].label.data ||
            (items[i].detail.length && !items[i].detail.data) ||
            (items[i].insert_text.length && !items[i].insert_text.data))
            return CUI_EDIT_INVALID_ARGUMENT;
        if (!cui_edit_valid_utf8(items[i].label) || !cui_edit_valid_utf8(items[i].detail) ||
            !cui_edit_valid_utf8(items[i].insert_text))
            return CUI_EDIT_INVALID_UTF8;
    }
    cui_edit_state selection = cui_edit_get_state(state->edit);
    state->completion =
        (cui_editor_completion_state){.items = items,
                                      .count = count,
                                      .replace_start = start,
                                      .replace_end = end,
                                      .cursor = selection.cursor,
                                      .anchor = selection.anchor,
                                      .revision = cui_edit_get_state(state->edit).revision,
                                      .open = true};
    return CUI_EDIT_OK;
}
cui_edit_result cui_editor_accept_completion(cui_editor_state *state) {
    if (!state || !state->edit || !state->completion.open || !state->completion.items ||
        state->completion.selected >= state->completion.count)
        return CUI_EDIT_INVALID_ARGUMENT;
    if (state->completion.revision != cui_edit_get_state(state->edit).revision) {
        cui_editor_cancel_completion(state);
        return CUI_EDIT_INVALID_ARGUMENT;
    }
    cui_edit_result result = cui_edit_replace_range(
        state->edit, state->completion.replace_start, state->completion.replace_end,
        state->completion.items[state->completion.selected].insert_text);
    if (result == CUI_EDIT_OK)
        cui_editor_cancel_completion(state);
    return result;
}
