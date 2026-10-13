/* SPDX-License-Identifier: MIT */
#include "cui/cui_editor.h"
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
cui_textarea_result cui_base_editor(cui_context *ui, cui_id id, cui_str name, cui_editor_state *s,
                                    cui_editor_options o, cui_textarea_metrics metrics) {
    if (!s || !s->edit || (o.language.length && !o.language.data))
        return (cui_textarea_result){.error = CUI_TEXTAREA_INVALID};
    return cui_base_textarea(ui, id, name, s->edit, &s->view, o.textarea, metrics);
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
