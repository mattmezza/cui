/* SPDX-License-Identifier: MIT */
#include "cui/cui_editor.h"
#include <math.h>
#include <string.h>
bool cui_editor_state_init(cui_editor_state *s, cui_edit *edit, cui_textarea_line *lines,
                           size_t capacity) {
    if (!s || !edit || !cui_edit_get_state(edit).multiline ||
        cui_edit_get_content(edit).token_count)
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
    if (!ui || !id || !s || !s->edit || cui_edit_get_content(s->edit).token_count ||
        !s->view.lines || !s->view.capacity || s->view.capacity > 65536 ||
        !isfinite(metrics.font_size) || metrics.font_size <= 0 || !isfinite(metrics.line_height) ||
        metrics.line_height <= 0 || o.textarea.text_style.weight < CUI_TEXT_WEIGHT_NORMAL ||
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

static bool cui_editor_replace_valid(cui_editor_state *s, cui_str query, cui_str replacement) {
    return s && s->edit && !cui_edit_get_content(s->edit).token_count && query.length &&
           cui_edit_valid_utf8(query) && cui_edit_valid_utf8(replacement);
}
cui_edit_result cui_editor_replace_current(cui_editor_state *s, cui_str query,
                                           cui_str replacement) {
    if (!cui_editor_replace_valid(s, query, replacement))
        return CUI_EDIT_INVALID_ARGUMENT;
    cui_edit_state state = cui_edit_get_state(s->edit);
    size_t start = state.cursor < state.anchor ? state.cursor : state.anchor;
    size_t end = state.cursor > state.anchor ? state.cursor : state.anchor;
    cui_str text = cui_edit_get_text(s->edit);
    if (end - start != query.length || memcmp(text.data + start, query.data, query.length))
        return CUI_EDIT_BOUNDARY;
    cui_edit_result result = cui_edit_replace_range(s->edit, start, end, replacement);
    if (result == CUI_EDIT_OK)
        cui_editor_cancel_completion(s);
    return result;
}
static bool cui_editor_storage_overlap(const void *a, size_t an, const void *b, size_t bn) {
    if (!an || !bn)
        return false;
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return x >= y ? x - y < bn : y - x < an;
}
cui_edit_result cui_editor_replace_all(cui_editor_state *s, cui_str query, cui_str replacement,
                                       char *scratch, size_t capacity, size_t *count) {
    if (!cui_editor_replace_valid(s, query, replacement) || !scratch || !count)
        return CUI_EDIT_INVALID_ARGUMENT;
    cui_str text = cui_edit_get_text(s->edit);
    if (cui_editor_storage_overlap(scratch, capacity, text.data, text.length + 1) ||
        cui_editor_storage_overlap(scratch, capacity, query.data, query.length) ||
        cui_editor_storage_overlap(scratch, capacity, replacement.data, replacement.length) ||
        cui_editor_storage_overlap(scratch, capacity, count, sizeof(*count)) ||
        cui_editor_storage_overlap(count, sizeof(*count), text.data, text.length + 1))
        return CUI_EDIT_INVALID_ARGUMENT;
    size_t matches = cui_editor_match_count(s, query);
    if (!matches) {
        *count = 0;
        return CUI_EDIT_OK;
    }
    size_t length = text.length - matches * query.length;
    if (replacement.length && matches > (SIZE_MAX - length) / replacement.length)
        return CUI_EDIT_CAPACITY;
    length += matches * replacement.length;
    if (length >= capacity)
        return CUI_EDIT_CAPACITY;
    size_t at = 0, out = 0;
    while (at < text.length) {
        if (query.length <= text.length - at && cui_editor_boundary(text, at) &&
            cui_editor_boundary(text, at + query.length) &&
            !memcmp(text.data + at, query.data, query.length)) {
            if (replacement.length)
                memcpy(scratch + out, replacement.data, replacement.length);
            out += replacement.length;
            at += query.length;
        } else
            scratch[out++] = text.data[at++];
    }
    scratch[out] = 0;
    cui_edit_result result =
        cui_edit_replace_range(s->edit, 0, text.length, (cui_str){scratch, length});
    if (result != CUI_EDIT_OK)
        return result;
    cui_edit_select(s->edit, 0, 0);
    s->view.scroll_x = s->view.scroll_y = 0;
    cui_editor_cancel_completion(s);
    *count = matches;
    return CUI_EDIT_OK;
}

static unsigned char cui_editor_ascii_fold(unsigned char c, bool insensitive) {
    return insensitive && c >= 'A' && c <= 'Z' ? (unsigned char)(c + ('a' - 'A')) : c;
}
static size_t cui_editor_scan_matches(cui_str text, cui_str query, bool insensitive,
                                      cui_editor_match *output) {
    if (!query.length || query.length > text.length)
        return 0;
    size_t count = 0, at = 0;
    while (at <= text.length - query.length) {
        bool match = cui_editor_boundary(text, at) && cui_editor_boundary(text, at + query.length);
        for (size_t j = 0; match && j < query.length; ++j)
            match = cui_editor_ascii_fold((unsigned char)text.data[at + j], insensitive) ==
                    cui_editor_ascii_fold((unsigned char)query.data[j], insensitive);
        if (match) {
            if (output)
                output[count] = (cui_editor_match){at, at + query.length};
            ++count;
            at += query.length;
        } else
            ++at;
    }
    return count;
}
static uint64_t cui_editor_query_hash(cui_str query) {
    uint64_t hash = UINT64_C(1469598103934665603);
    for (size_t i = 0; i < query.length; ++i) {
        hash ^= (unsigned char)query.data[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}
bool cui_editor_matcher_init(cui_editor_matcher *matcher, cui_editor_match *matches,
                             size_t capacity) {
    if (!matcher || (capacity && !matches) || capacity > SIZE_MAX / sizeof(*matches) ||
        cui_editor_storage_overlap(matcher, sizeof(*matcher), matches, capacity * sizeof(*matches)))
        return false;
    *matcher =
        (cui_editor_matcher){.matches = matches, .capacity = capacity, .case_insensitive = true};
    return true;
}
static cui_edit_result cui_editor_matcher_preflight(cui_editor_state *editor,
                                                    const cui_editor_matcher *matcher,
                                                    cui_str query, cui_str text, bool insensitive,
                                                    size_t *count) {
    if (!editor || !editor->edit || !matcher || (matcher->capacity && !matcher->matches) ||
        matcher->capacity > SIZE_MAX / sizeof(*matcher->matches) ||
        cui_edit_get_content(editor->edit).token_count || !cui_edit_valid_utf8(query) ||
        cui_editor_storage_overlap(query.data, query.length, cui_edit_get_text(editor->edit).data,
                                   cui_edit_get_text(editor->edit).length + 1) ||
        cui_editor_storage_overlap(matcher->matches, matcher->capacity * sizeof(*matcher->matches),
                                   text.data, text.length + 1) ||
        cui_editor_storage_overlap(matcher->matches, matcher->capacity * sizeof(*matcher->matches),
                                   query.data, query.length))
        return CUI_EDIT_INVALID_ARGUMENT;
    *count = cui_editor_scan_matches(text, query, insensitive, NULL);
    return *count > matcher->capacity ? CUI_EDIT_CAPACITY : CUI_EDIT_OK;
}
cui_edit_result cui_editor_matcher_set_query(cui_editor_state *editor, cui_editor_matcher *matcher,
                                             cui_str query, bool insensitive) {
    if (!editor || !editor->edit)
        return CUI_EDIT_INVALID_ARGUMENT;
    cui_str text = cui_edit_get_text(editor->edit);
    size_t count;
    cui_edit_result result =
        cui_editor_matcher_preflight(editor, matcher, query, text, insensitive, &count);
    if (result != CUI_EDIT_OK)
        return result;
    uint64_t hash = cui_editor_query_hash(query),
             revision = cui_edit_get_state(editor->edit).revision;
    bool same = matcher->initialized && matcher->document == editor->edit &&
                matcher->query.length == query.length && matcher->query_hash == hash &&
                matcher->case_insensitive == insensitive && matcher->revision == revision;
    cui_editor_scan_matches(text, query, insensitive, matcher->matches);
    matcher->query = query;
    matcher->query_hash = hash;
    matcher->case_insensitive = insensitive;
    matcher->document = editor->edit;
    matcher->revision = revision;
    matcher->count = count;
    if (!same || matcher->current >= count)
        matcher->current = 0;
    matcher->active = matcher->initialized = true;
    return CUI_EDIT_OK;
}
cui_edit_result cui_editor_matcher_sync(cui_editor_state *editor, cui_editor_matcher *matcher) {
    if (!editor || !editor->edit || !matcher || !matcher->initialized)
        return CUI_EDIT_INVALID_ARGUMENT;
    bool active = matcher->active;
    cui_edit_result result =
        cui_editor_matcher_set_query(editor, matcher, matcher->query, matcher->case_insensitive);
    if (result == CUI_EDIT_OK)
        matcher->active = active;
    return result;
}
void cui_editor_matcher_close(cui_editor_matcher *matcher) {
    if (matcher)
        matcher->active = false;
}
cui_edit_result cui_editor_matcher_navigate(cui_editor_state *editor, cui_editor_matcher *matcher,
                                            bool reverse) {
    cui_edit_result result = cui_editor_matcher_sync(editor, matcher);
    if (result != CUI_EDIT_OK)
        return result;
    if (!matcher->count)
        return CUI_EDIT_BOUNDARY;
    size_t next = reverse ? (matcher->current ? matcher->current - 1 : matcher->count - 1)
                          : (matcher->current + 1 == matcher->count ? 0 : matcher->current + 1);
    cui_editor_match range = matcher->matches[next];
    result = cui_edit_select(editor->edit, range.start, range.end);
    if (result == CUI_EDIT_OK)
        matcher->current = next;
    return result;
}
cui_edit_result cui_editor_matcher_anchor(cui_editor_state *editor, cui_editor_matcher *matcher,
                                          size_t offset) {
    if (!editor || !editor->edit || offset > cui_edit_get_text(editor->edit).length ||
        !cui_editor_boundary(cui_edit_get_text(editor->edit), offset))
        return CUI_EDIT_BOUNDARY;
    cui_edit_result result = cui_editor_matcher_sync(editor, matcher);
    if (result != CUI_EDIT_OK)
        return result;
    if (!matcher->count)
        return CUI_EDIT_BOUNDARY;
    size_t current = 0;
    while (current + 1 < matcher->count && matcher->matches[current].end < offset)
        ++current;
    matcher->current = current;
    return CUI_EDIT_OK;
}
cui_edit_result cui_editor_matcher_replace_current(cui_editor_state *editor,
                                                   cui_editor_matcher *matcher, cui_str replacement,
                                                   char *scratch, size_t capacity) {
    if (!editor || !editor->edit || !matcher || !matcher->initialized || !scratch ||
        !cui_edit_valid_utf8(replacement))
        return CUI_EDIT_INVALID_ARGUMENT;
    /* Probe synchronization without modifying caller session/ranges on rejection. */
    cui_str text = cui_edit_get_text(editor->edit);
    size_t count;
    cui_edit_result result = cui_editor_matcher_preflight(editor, matcher, matcher->query, text,
                                                          matcher->case_insensitive, &count);
    if (result != CUI_EDIT_OK)
        return result;
    if (!count)
        return CUI_EDIT_BOUNDARY;
    size_t current = matcher->revision == cui_edit_get_state(editor->edit).revision &&
                             matcher->document == editor->edit && matcher->current < count
                         ? matcher->current
                         : 0;
    /* Obtain current range without writing match storage. */
    cui_editor_match range = {0};
    size_t at = 0, index = 0;
    while (at <= text.length - matcher->query.length) {
        cui_str suffix = {text.data + at, text.length - at};
        bool match =
            cui_editor_boundary(text, at) && cui_editor_boundary(text, at + matcher->query.length);
        for (size_t j = 0; match && j < matcher->query.length; ++j)
            match =
                cui_editor_ascii_fold((unsigned char)suffix.data[j], matcher->case_insensitive) ==
                cui_editor_ascii_fold((unsigned char)matcher->query.data[j],
                                      matcher->case_insensitive);
        if (match) {
            if (index++ == current) {
                range = (cui_editor_match){at, at + matcher->query.length};
                break;
            }
            at += matcher->query.length;
        } else
            ++at;
    }
    size_t remaining = text.length - (range.end - range.start);
    if (replacement.length > SIZE_MAX - remaining || remaining + replacement.length >= capacity)
        return CUI_EDIT_CAPACITY;
    size_t bytes = matcher->capacity * sizeof(*matcher->matches);
    if (cui_editor_storage_overlap(scratch, capacity, text.data, text.length + 1) ||
        cui_editor_storage_overlap(scratch, capacity, matcher->query.data, matcher->query.length) ||
        cui_editor_storage_overlap(scratch, capacity, replacement.data, replacement.length) ||
        cui_editor_storage_overlap(scratch, capacity, matcher->matches, bytes))
        return CUI_EDIT_INVALID_ARGUMENT;
    memcpy(scratch, text.data, range.start);
    if (replacement.length)
        memcpy(scratch + range.start, replacement.data, replacement.length);
    memcpy(scratch + range.start + replacement.length, text.data + range.end,
           text.length - range.end);
    cui_str candidate = {scratch, remaining + replacement.length};
    scratch[candidate.length] = 0;
    size_t next_count =
        cui_editor_scan_matches(candidate, matcher->query, matcher->case_insensitive, NULL);
    if (next_count > matcher->capacity)
        return CUI_EDIT_CAPACITY;
    result = cui_edit_replace_range(editor->edit, range.start, range.end, replacement);
    if (result != CUI_EDIT_OK)
        return result;
    cui_editor_scan_matches(cui_edit_get_text(editor->edit), matcher->query,
                            matcher->case_insensitive, matcher->matches);
    matcher->current = current + 1 < count ? current : 0;
    if (matcher->current >= next_count)
        matcher->current = next_count ? next_count - 1 : 0;
    matcher->count = next_count;
    matcher->document = editor->edit;
    matcher->revision = cui_edit_get_state(editor->edit).revision;
    matcher->active = true;
    cui_editor_cancel_completion(editor);
    return CUI_EDIT_OK;
}

cui_edit_result cui_editor_matcher_replace_all(cui_editor_state *editor,
                                               cui_editor_matcher *matcher, cui_str replacement,
                                               char *scratch, size_t capacity, size_t *replaced) {
    if (!editor || !editor->edit || !matcher || !matcher->initialized || !scratch || !replaced ||
        !cui_edit_valid_utf8(replacement))
        return CUI_EDIT_INVALID_ARGUMENT;
    cui_str text = cui_edit_get_text(editor->edit);
    size_t count;
    cui_edit_result result = cui_editor_matcher_preflight(editor, matcher, matcher->query, text,
                                                          matcher->case_insensitive, &count);
    if (result != CUI_EDIT_OK)
        return result;
    size_t bytes = matcher->capacity * sizeof(*matcher->matches);
    if (cui_editor_storage_overlap(scratch, capacity, text.data, text.length + 1) ||
        cui_editor_storage_overlap(scratch, capacity, matcher->query.data, matcher->query.length) ||
        cui_editor_storage_overlap(scratch, capacity, replacement.data, replacement.length) ||
        cui_editor_storage_overlap(scratch, capacity, matcher->matches, bytes) ||
        cui_editor_storage_overlap(scratch, capacity, replaced, sizeof(*replaced)) ||
        cui_editor_storage_overlap(replaced, sizeof(*replaced), text.data, text.length + 1) ||
        cui_editor_storage_overlap(replaced, sizeof(*replaced), matcher, sizeof(*matcher)) ||
        cui_editor_storage_overlap(replaced, sizeof(*replaced), matcher->matches, bytes) ||
        cui_editor_storage_overlap(replaced, sizeof(*replaced), matcher->query.data,
                                   matcher->query.length))
        return CUI_EDIT_INVALID_ARGUMENT;
    if (!count) {
        *replaced = 0;
        return CUI_EDIT_OK;
    }
    size_t length = text.length - count * matcher->query.length;
    if (replacement.length && count > (SIZE_MAX - length) / replacement.length)
        return CUI_EDIT_CAPACITY;
    length += count * replacement.length;
    if (length >= capacity)
        return CUI_EDIT_CAPACITY;
    size_t at = 0, out = 0;
    while (at < text.length) {
        bool match = matcher->query.length <= text.length - at && cui_editor_boundary(text, at) &&
                     cui_editor_boundary(text, at + matcher->query.length);
        for (size_t j = 0; match && j < matcher->query.length; ++j)
            match = cui_editor_ascii_fold((unsigned char)text.data[at + j],
                                          matcher->case_insensitive) ==
                    cui_editor_ascii_fold((unsigned char)matcher->query.data[j],
                                          matcher->case_insensitive);
        if (match) {
            if (replacement.length)
                memcpy(scratch + out, replacement.data, replacement.length);
            at += matcher->query.length;
            out += replacement.length;
        } else
            scratch[out++] = text.data[at++];
    }
    scratch[out] = 0;
    cui_str candidate = {scratch, length};
    size_t next_count =
        cui_editor_scan_matches(candidate, matcher->query, matcher->case_insensitive, NULL);
    if (next_count > matcher->capacity)
        return CUI_EDIT_CAPACITY;
    result = cui_edit_replace_range(editor->edit, 0, text.length, candidate);
    if (result != CUI_EDIT_OK)
        return result;
    cui_editor_scan_matches(cui_edit_get_text(editor->edit), matcher->query,
                            matcher->case_insensitive, matcher->matches);
    matcher->count = next_count;
    matcher->current = 0;
    matcher->document = editor->edit;
    matcher->revision = cui_edit_get_state(editor->edit).revision;
    cui_edit_select(editor->edit, 0, 0);
    editor->view.scroll_x = editor->view.scroll_y = 0;
    cui_editor_cancel_completion(editor);
    *replaced = count;
    return CUI_EDIT_OK;
}
bool cui_editor_search_enable_replace(cui_editor_search_state *state, cui_editor_matcher *matcher,
                                      cui_edit *replacement, char *scratch, size_t capacity) {
    if (!state || !state->query || !matcher || !replacement || replacement == state->query ||
        cui_edit_get_state(replacement).multiline || !scratch || !capacity ||
        matcher->capacity > SIZE_MAX / sizeof(*matcher->matches) ||
        (matcher->capacity && !matcher->matches))
        return false;
    state->matcher = matcher;
    state->replacement = replacement;
    state->scratch = scratch;
    state->scratch_capacity = capacity;
    state->replace_mode = false;
    return true;
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
