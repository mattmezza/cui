/* SPDX-License-Identifier: MIT */
#include "cui/cui_edit.h"
#include <stdlib.h>
#include <string.h>
typedef struct {
    size_t length, cursor, anchor;
} cui_edit_private_snapshot;
struct cui_edit {
    cui_edit_config config;
    size_t length, cursor, anchor, preferred_column;
    bool preferred_column_valid;
    char *history, *scratch;
    cui_edit_private_snapshot *states;
    size_t slots, count, index;
    uint64_t revision;
};
static bool cui_edit_private_decode(const unsigned char *s, size_t n, size_t *width) {
    if (!n || s[0] == 0)
        return false;
    unsigned char a = s[0];
    size_t z;
    unsigned cp;
    if (a < 0x80) {
        *width = 1;
        return true;
    }
    if (a >= 0xc2 && a <= 0xdf) {
        z = 2;
        cp = a & 31u;
    } else if (a >= 0xe0 && a <= 0xef) {
        z = 3;
        cp = a & 15u;
    } else if (a >= 0xf0 && a <= 0xf4) {
        z = 4;
        cp = a & 7u;
    } else
        return false;
    if (n < z)
        return false;
    for (size_t i = 1; i < z; i++) {
        if ((s[i] & 0xc0u) != 0x80u)
            return false;
        cp = (cp << 6) | (s[i] & 63u);
    }
    if ((z == 2 && cp < 0x80) || (z == 3 && cp < 0x800) || (z == 4 && cp < 0x10000) ||
        cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
        return false;
    *width = z;
    return true;
}
bool cui_edit_valid_utf8(cui_str text) {
    if (text.length && !text.data)
        return false;
    size_t i = 0;
    while (i < text.length) {
        size_t z;
        if (!cui_edit_private_decode((const unsigned char *)text.data + i, text.length - i, &z))
            return false;
        i += z;
    }
    return true;
}
static bool cui_edit_private_accepted(cui_edit *e, cui_str t) {
    if (!cui_edit_valid_utf8(t))
        return false;
    if (!e->config.multiline)
        for (size_t i = 0; i < t.length; i++)
            if (t.data[i] == '\n' || t.data[i] == '\r')
                return false;
    return true;
}
static bool cui_edit_private_boundary(cui_edit *e, size_t p) {
    return p <= e->length &&
           (p == e->length || ((unsigned char)e->config.buffer[p] & 0xc0u) != 0x80u);
}
static void cui_edit_private_save_state(cui_edit *e) {
    if (e->slots)
        e->states[e->index] = (cui_edit_private_snapshot){e->length, e->cursor, e->anchor};
}
static void cui_edit_private_record(cui_edit *e) {
    if (!e->slots)
        return;
    if (!e->count) {
        e->count = 1;
        e->index = 0;
        memcpy(e->history, e->config.buffer, e->length + 1);
        cui_edit_private_save_state(e);
        return;
    }
    e->count = e->index + 1;
    if (e->count == e->slots) {
        memmove(e->history, e->history + e->config.capacity, (e->slots - 1) * e->config.capacity);
        memmove(e->states, e->states + 1, (e->slots - 1) * sizeof(*e->states));
        e->count--;
    }
    e->index = e->count++;
    memcpy(e->history + e->index * e->config.capacity, e->config.buffer, e->length + 1);
    cui_edit_private_save_state(e);
}
cui_edit *cui_edit_create(cui_edit_config c) {
    if (!c.buffer || !c.capacity)
        return NULL;
    size_t n = 0;
    while (n < c.capacity && c.buffer[n])
        n++;
    if (n == c.capacity)
        return NULL;
    cui_edit *e = calloc(1, sizeof(*e));
    if (!e)
        return NULL;
    e->config = c;
    e->length = e->cursor = e->anchor = n;
    e->scratch = malloc(c.capacity);
    if (!e->scratch) {
        free(e);
        return NULL;
    }
    if (!cui_edit_private_accepted(e, (cui_str){c.buffer, n})) {
        cui_edit_destroy(e);
        return NULL;
    }
    if (c.capacity > SIZE_MAX - sizeof(cui_edit_private_snapshot)) {
        cui_edit_destroy(e);
        return NULL;
    }
    e->slots = c.history_bytes / (c.capacity + sizeof(cui_edit_private_snapshot));
    if (c.max_history_steps && c.max_history_steps < SIZE_MAX && e->slots > c.max_history_steps + 1)
        e->slots = c.max_history_steps + 1;
    if (e->slots < 2)
        e->slots = 0;
    if (e->slots) {
        e->history = malloc(e->slots * c.capacity);
        e->states = calloc(e->slots, sizeof(*e->states));
        if (!e->history || !e->states) {
            cui_edit_destroy(e);
            return NULL;
        }
        cui_edit_private_record(e);
    }
    return e;
}
void cui_edit_destroy(cui_edit *e) {
    if (e) {
        free(e->scratch);
        free(e->history);
        free(e->states);
        free(e);
    }
}
cui_edit_state cui_edit_get_state(const cui_edit *e) {
    return e ? (cui_edit_state){e->length,
                                e->cursor,
                                e->anchor,
                                e->slots && e->index > 0,
                                e->slots && e->index + 1 < e->count,
                                e->revision,
                                e->config.multiline}
             : (cui_edit_state){0};
}
cui_str cui_edit_get_text(const cui_edit *e) {
    return e ? (cui_str){e->config.buffer, e->length} : (cui_str){0};
}
cui_edit_result cui_edit_select(cui_edit *e, size_t a, size_t p) {
    if (!e)
        return CUI_EDIT_INVALID_ARGUMENT;
    if (!cui_edit_private_boundary(e, a) || !cui_edit_private_boundary(e, p))
        return CUI_EDIT_BOUNDARY;
    e->preferred_column_valid = false;
    e->anchor = a;
    e->cursor = p;
    cui_edit_private_save_state(e);
    return CUI_EDIT_OK;
}
static size_t cui_edit_private_previous(cui_edit *e, size_t p) {
    if (p) {
        p--;
        while (p && ((unsigned char)e->config.buffer[p] & 0xc0u) == 0x80u)
            p--;
    }
    return p;
}
static size_t cui_edit_private_next(cui_edit *e, size_t p) {
    if (p < e->length) {
        p++;
        while (p < e->length && ((unsigned char)e->config.buffer[p] & 0xc0u) == 0x80u)
            p++;
    }
    return p;
}
static bool cui_edit_private_word(cui_edit *e, size_t p) {
    if (p >= e->length)
        return false;
    const unsigned char *s = (const unsigned char *)e->config.buffer + p;
    unsigned cp = s[0];
    if (cp >= 0x80) {
        size_t count = cp < 0xe0 ? 2 : cp < 0xf0 ? 3 : 4;
        cp &= count == 2 ? 31u : count == 3 ? 15u : 7u;
        for (size_t i = 1; i < count; i++)
            cp = (cp << 6) | (s[i] & 63u);
    }
    if (cp < 128)
        return (cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z') || (cp >= '0' && cp <= '9') ||
               cp == '_';
    /* Stable, locale-independent useful ranges; not Unicode UAX #29 segmentation. */
    return (cp >= 0xc0 && cp <= 0x2af && cp != 0xd7 && cp != 0xf7) ||
           (cp >= 0x300 && cp <= 0x1fff) || (cp >= 0x2c00 && cp <= 0x2dff) ||
           (cp >= 0x3040 && cp <= 0x30ff) || (cp >= 0x3400 && cp <= 0x9fff) ||
           (cp >= 0xac00 && cp <= 0xd7af) || (cp >= 0xf900 && cp <= 0xfaff) ||
           (cp >= 0xff10 && cp <= 0xff19) || (cp >= 0xff21 && cp <= 0xff3a) ||
           (cp >= 0xff41 && cp <= 0xff5a);
}
static size_t cui_edit_private_line_start(const cui_edit *e, size_t at);
static size_t cui_edit_private_line_end(cui_edit *e, size_t p) {
    while (p < e->length && e->config.buffer[p] != '\n')
        p = cui_edit_private_next(e, p);
    return p;
}
cui_edit_result cui_edit_move(cui_edit *e, cui_edit_motion m, bool extend) {
    if (!e || m < CUI_EDIT_PREVIOUS || m > CUI_EDIT_LINE_NEXT)
        return CUI_EDIT_INVALID_ARGUMENT;
    size_t p = e->cursor;
    bool vertical = m == CUI_EDIT_LINE_PREVIOUS || m == CUI_EDIT_LINE_NEXT;
    size_t column = e->preferred_column;
    if (!vertical)
        e->preferred_column_valid = false;
    if (!extend && e->anchor != e->cursor && (m == CUI_EDIT_PREVIOUS || m == CUI_EDIT_NEXT))
        p = m == CUI_EDIT_PREVIOUS ? (e->anchor < p ? e->anchor : p)
                                   : (e->anchor > p ? e->anchor : p);
    else
        switch (m) {
        case CUI_EDIT_PREVIOUS:
            p = cui_edit_private_previous(e, p);
            break;
        case CUI_EDIT_NEXT:
            p = cui_edit_private_next(e, p);
            break;
        case CUI_EDIT_WORD_PREVIOUS:
            while (p && !cui_edit_private_word(e, cui_edit_private_previous(e, p)))
                p = cui_edit_private_previous(e, p);
            while (p && cui_edit_private_word(e, cui_edit_private_previous(e, p)))
                p = cui_edit_private_previous(e, p);
            break;
        case CUI_EDIT_WORD_NEXT:
            while (p < e->length && cui_edit_private_word(e, p))
                p = cui_edit_private_next(e, p);
            while (p < e->length && !cui_edit_private_word(e, p))
                p = cui_edit_private_next(e, p);
            break;
        case CUI_EDIT_LINE_PREVIOUS:
        case CUI_EDIT_LINE_NEXT: {
            size_t start = cui_edit_private_line_start(e, p);
            if (!e->preferred_column_valid) {
                column = 0;
                for (size_t at = start; at < p; at = cui_edit_private_next(e, at))
                    column++;
            }
            size_t end;
            if (m == CUI_EDIT_LINE_PREVIOUS) {
                if (!start) {
                    p = 0;
                    break;
                }
                end = start - 1;
                start = cui_edit_private_line_start(e, end);
            } else {
                end = cui_edit_private_line_end(e, p);
                if (end == e->length) {
                    p = e->length;
                    break;
                }
                start = end + 1;
                end = cui_edit_private_line_end(e, start);
            }
            p = start;
            for (size_t i = 0; i < column && p < end; i++)
                p = cui_edit_private_next(e, p);
            break;
        }
        case CUI_EDIT_START:
            p = 0;
            break;
        case CUI_EDIT_END:
            p = e->length;
            break;
        case CUI_EDIT_LINE_START:
            while (p && e->config.buffer[p - 1] != '\n')
                p = cui_edit_private_previous(e, p);
            break;
        case CUI_EDIT_LINE_END:
            while (p < e->length && e->config.buffer[p] != '\n')
                p = cui_edit_private_next(e, p);
            break;
        default:
            return CUI_EDIT_INVALID_ARGUMENT;
        }
    cui_edit_result result = cui_edit_select(e, extend ? e->anchor : p, p);
    if (vertical && result == CUI_EDIT_OK) {
        e->preferred_column = column;
        e->preferred_column_valid = true;
    }
    return result;
}
static cui_edit_result cui_edit_private_replace(cui_edit *e, cui_str text, size_t start,
                                                size_t end) {
    if (!cui_edit_valid_utf8(text))
        return CUI_EDIT_INVALID_UTF8;
    if (!cui_edit_private_accepted(e, text))
        return CUI_EDIT_LINE_BREAK;
    size_t remaining = e->length - (end - start);
    if (text.length > e->config.capacity - 1 - remaining)
        return CUI_EDIT_CAPACITY;
    if (!text.length && start == end)
        return CUI_EDIT_OK;
    e->preferred_column_valid = false;
    if (text.length == end - start &&
        (!text.length || !memcmp(e->config.buffer + start, text.data, text.length))) {
        e->cursor = e->anchor = start + text.length;
        cui_edit_private_save_state(e);
        return CUI_EDIT_OK;
    }
    /* Support a slice of the attached buffer without invalidating its source. */
    if (text.length)
        memcpy(e->scratch, text.data, text.length);
    cui_edit_private_save_state(e);
    memmove(e->config.buffer + start + text.length, e->config.buffer + end, e->length - end + 1);
    if (text.length)
        memcpy(e->config.buffer + start, e->scratch, text.length);
    e->length = remaining + text.length;
    e->cursor = e->anchor = start + text.length;
    e->revision++;
    cui_edit_private_record(e);
    return CUI_EDIT_OK;
}
cui_edit_result cui_edit_replace_range(cui_edit *e, size_t start, size_t end, cui_str text) {
    if (!e)
        return CUI_EDIT_INVALID_ARGUMENT;
    if (start > end || !cui_edit_private_boundary(e, start) || !cui_edit_private_boundary(e, end))
        return CUI_EDIT_BOUNDARY;
    return cui_edit_private_replace(e, text, start, end);
}
cui_edit_result cui_edit_insert(cui_edit *e, cui_str text) {
    if (!e)
        return CUI_EDIT_INVALID_ARGUMENT;
    return cui_edit_private_replace(e, text, e->cursor < e->anchor ? e->cursor : e->anchor,
                                    e->cursor > e->anchor ? e->cursor : e->anchor);
}
static bool cui_edit_private_indent_match(const cui_edit *e, size_t at, size_t width,
                                          bool hard_tabs) {
    if (width > e->length - at)
        return false;
    for (size_t i = 0; i < width; i++)
        if (e->config.buffer[at + i] != (hard_tabs ? '\t' : ' '))
            return false;
    return true;
}
static size_t cui_edit_private_line_start(const cui_edit *e, size_t at) {
    while (at && e->config.buffer[at - 1] != '\n')
        at--;
    return at;
}
static void cui_edit_private_indent_commit(cui_edit *e, size_t length, size_t anchor,
                                           size_t cursor) {
    cui_edit_private_save_state(e);
    memcpy(e->config.buffer, e->scratch, length + 1);
    e->length = length;
    e->preferred_column_valid = false;
    e->anchor = anchor;
    e->cursor = cursor;
    e->revision++;
    cui_edit_private_record(e);
}
cui_edit_result cui_edit_indent(cui_edit *e, bool outdent, size_t tab_size, bool hard_tabs) {
    if (!e || !e->config.multiline || !tab_size || tab_size > 4096)
        return CUI_EDIT_INVALID_ARGUMENT;
    size_t unit = hard_tabs ? 1 : tab_size;
    bool block = e->anchor != e->cursor;
    if (!block && !outdent) {
        if (unit > e->config.capacity - 1 - e->length)
            return CUI_EDIT_CAPACITY;
        size_t at = e->cursor;
        memcpy(e->scratch, e->config.buffer, at);
        memset(e->scratch + at, hard_tabs ? '\t' : ' ', unit);
        memcpy(e->scratch + at + unit, e->config.buffer + at, e->length - at + 1);
        cui_edit_private_indent_commit(e, e->length + unit, at + unit, at + unit);
        return CUI_EDIT_OK;
    }
    size_t first = e->anchor < e->cursor ? e->anchor : e->cursor;
    size_t last = e->anchor > e->cursor ? e->anchor : e->cursor;
    size_t first_line = cui_edit_private_line_start(e, first);
    size_t last_line = cui_edit_private_line_start(e, last);
    size_t edits = 0;
    for (size_t at = first_line; at <= last_line;) {
        if (!outdent || cui_edit_private_indent_match(e, at, unit, hard_tabs))
            edits++;
        size_t next = at;
        while (next < e->length && e->config.buffer[next] != '\n')
            next++;
        if (next == e->length)
            break;
        at = next + 1;
    }
    if (!edits)
        return CUI_EDIT_OK;
    if (!outdent && edits > (e->config.capacity - 1 - e->length) / unit)
        return CUI_EDIT_CAPACITY;
    size_t mapped_anchor = e->anchor, mapped_cursor = e->cursor;
    size_t output = 0;
    for (size_t at = 0; at <= e->length;) {
        bool line_start = !at || e->config.buffer[at - 1] == '\n';
        bool selected = line_start && at >= first_line && at <= last_line;
        if (selected && (!outdent || cui_edit_private_indent_match(e, at, unit, hard_tabs))) {
            if (outdent) {
                if (e->anchor >= at + unit)
                    mapped_anchor -= unit;
                else if (e->anchor >= at)
                    mapped_anchor = output;
                if (e->cursor >= at + unit)
                    mapped_cursor -= unit;
                else if (e->cursor >= at)
                    mapped_cursor = output;
                at += unit;
            } else {
                if (e->anchor >= at)
                    mapped_anchor += unit;
                if (e->cursor >= at)
                    mapped_cursor += unit;
                memset(e->scratch + output, hard_tabs ? '\t' : ' ', unit);
                output += unit;
            }
        }
        if (at == e->length)
            break;
        e->scratch[output++] = e->config.buffer[at++];
    }
    e->scratch[output] = 0;
    cui_edit_private_indent_commit(e, output, mapped_anchor, mapped_cursor);
    return CUI_EDIT_OK;
}
static cui_edit_result cui_edit_private_erase(cui_edit *e, bool backwards) {
    if (!e)
        return CUI_EDIT_INVALID_ARGUMENT;
    if (e->anchor != e->cursor)
        return cui_edit_insert(e, (cui_str){0});
    size_t start = backwards ? cui_edit_private_previous(e, e->cursor) : e->cursor,
           end = backwards ? e->cursor : cui_edit_private_next(e, e->cursor);
    return cui_edit_private_replace(e, (cui_str){0}, start, end);
}
cui_edit_result cui_edit_backspace(cui_edit *e) {
    return cui_edit_private_erase(e, true);
}
cui_edit_result cui_edit_delete(cui_edit *e) {
    return cui_edit_private_erase(e, false);
}
static void cui_edit_private_restore(cui_edit *e) {
    cui_edit_private_snapshot s = e->states[e->index];
    e->length = s.length;
    e->preferred_column_valid = false;
    e->cursor = s.cursor;
    e->anchor = s.anchor;
    memcpy(e->config.buffer, e->history + e->index * e->config.capacity, e->length + 1);
}
cui_edit_result cui_edit_undo(cui_edit *e) {
    if (!e)
        return CUI_EDIT_INVALID_ARGUMENT;
    if (!e->slots || !e->index)
        return CUI_EDIT_NO_HISTORY;
    cui_edit_private_save_state(e);
    e->index--;
    cui_edit_private_restore(e);
    e->revision++;
    return CUI_EDIT_OK;
}
cui_edit_result cui_edit_redo(cui_edit *e) {
    if (!e)
        return CUI_EDIT_INVALID_ARGUMENT;
    if (!e->slots || e->index + 1 >= e->count)
        return CUI_EDIT_NO_HISTORY;
    cui_edit_private_save_state(e);
    e->index++;
    cui_edit_private_restore(e);
    e->revision++;
    return CUI_EDIT_OK;
}
cui_edit_result cui_edit_apply_event(cui_edit *e, cui_event v) {
    if (!e)
        return CUI_EDIT_INVALID_ARGUMENT;
    if (v.type == CUI_EVENT_KEY_DOWN && (v.ime_composing || v.prefer_character_input))
        return CUI_EDIT_OK;
    cui_edit_result r = CUI_EDIT_OK;
    if (v.type == CUI_EVENT_TEXT) {
        size_t n = 0;
        while (n < sizeof(v.text) && v.text[n])
            n++;
        r = n == sizeof(v.text) ? CUI_EDIT_INVALID_ARGUMENT
                                : cui_edit_insert(e, (cui_str){v.text, n});
    } else if (v.type == CUI_EVENT_KEY_DOWN) {
        bool shift = (v.modifiers & CUI_MOD_SHIFT) != 0;
        switch (v.key) {
        case CUI_KEY_LEFT:
            r = cui_edit_move(e,
                              (v.modifiers & (CUI_MOD_CTRL | CUI_MOD_ALT)) ? CUI_EDIT_WORD_PREVIOUS
                                                                           : CUI_EDIT_PREVIOUS,
                              shift);
            break;
        case CUI_KEY_RIGHT:
            r = cui_edit_move(e,
                              (v.modifiers & (CUI_MOD_CTRL | CUI_MOD_ALT)) ? CUI_EDIT_WORD_NEXT
                                                                           : CUI_EDIT_NEXT,
                              shift);
            break;
        case CUI_KEY_UP:
            r = cui_edit_move(e,
                              (v.modifiers & (CUI_MOD_CTRL | CUI_MOD_META))
                                  ? CUI_EDIT_START
                                  : CUI_EDIT_LINE_PREVIOUS,
                              shift);
            break;
        case CUI_KEY_DOWN:
            r = cui_edit_move(e,
                              (v.modifiers & (CUI_MOD_CTRL | CUI_MOD_META)) ? CUI_EDIT_END
                                                                            : CUI_EDIT_LINE_NEXT,
                              shift);
            break;
        case CUI_KEY_HOME:
            r = cui_edit_move(
                e, (v.modifiers & CUI_MOD_CTRL) ? CUI_EDIT_START : CUI_EDIT_LINE_START, shift);
            break;
        case CUI_KEY_END:
            r = cui_edit_move(e, (v.modifiers & CUI_MOD_CTRL) ? CUI_EDIT_END : CUI_EDIT_LINE_END,
                              shift);
            break;
        case CUI_KEY_BACKSPACE:
            r = cui_edit_backspace(e);
            break;
        case CUI_KEY_DELETE:
            r = cui_edit_delete(e);
            break;
        case CUI_KEY_ENTER:
            if (e->config.multiline)
                r = cui_edit_insert(e, cui_string("\n"));
            break;
        case CUI_KEY_A:
            if (v.modifiers & (CUI_MOD_CTRL | CUI_MOD_META))
                r = cui_edit_select(e, 0, e->length);
            break;
        case CUI_KEY_Z:
            if (v.modifiers & (CUI_MOD_CTRL | CUI_MOD_META)) {
                if (shift && cui_edit_get_state(e).can_redo)
                    r = cui_edit_redo(e);
                else if (!shift && cui_edit_get_state(e).can_undo)
                    r = cui_edit_undo(e);
            }
            break;
        case CUI_KEY_Y:
            if ((v.modifiers & (CUI_MOD_CTRL | CUI_MOD_META)) && cui_edit_get_state(e).can_redo)
                r = cui_edit_redo(e);
            break;
        default:
            break;
        }
    }
    return r;
}
cui_edit_result cui_edit_apply_events(cui_edit *e, cui_context *ui, cui_id id, bool disabled) {
    if (!e || !ui || !id)
        return CUI_EDIT_INVALID_ARGUMENT;
    if (disabled)
        return CUI_EDIT_OK;
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    cui_edit_result first = CUI_EDIT_OK;
    for (size_t i = 0; i < count; ++i) {
        if (cui_event_consumed(ui, i) || cui_event_target(ui, i) != id ||
            (events[i].type == CUI_EVENT_KEY_DOWN &&
             (events[i].ime_composing || events[i].prefer_character_input)))
            continue;
        if (events[i].type == CUI_EVENT_KEY_DOWN &&
            (events[i].key == CUI_KEY_LEFT || events[i].key == CUI_KEY_RIGHT ||
             events[i].key == CUI_KEY_UP || events[i].key == CUI_KEY_DOWN ||
             events[i].key == CUI_KEY_HOME || events[i].key == CUI_KEY_END))
            cui_consume_event(ui, i);
        cui_edit_result r = cui_edit_apply_event(e, events[i]);
        if (first == CUI_EDIT_OK && r != CUI_EDIT_OK)
            first = r;
    }
    return first;
}
