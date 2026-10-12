/* SPDX-License-Identifier: MIT */
#include "cui/cui_textarea.h"
#include <math.h>
#include <string.h>
static size_t cui_textarea_next(cui_str text, size_t at) {
    if (at >= text.length)
        return text.length;
    do {
        at++;
    } while (at < text.length && ((unsigned char)text.data[at] & 0xc0) == 0x80);
    return at;
}
static size_t cui_textarea_previous(cui_str text, size_t at) {
    if (!at)
        return 0;
    do {
        at--;
    } while (at && ((unsigned char)text.data[at] & 0xc0) == 0x80);
    return at;
}
static float cui_textarea_width(cui_context *ui, cui_str text, size_t start, size_t end,
                                float font) {
    return cui_measure_text(ui, (cui_str){text.data + start, end - start}, font).width;
}
/* Exponential bracketing avoids measuring the entire remaining document for
 * every narrow wrapped row. Prefix widths use the host's text measurement. */
static size_t cui_textarea_fit(cui_context *ui, cui_str text, size_t start, size_t end, float font,
                               float width) {
    size_t low = start, high = end - start > 64 ? start + 64 : end;
    while (high < end && ((unsigned char)text.data[high] & 0xc0) == 0x80)
        high++;
    while (cui_textarea_width(ui, text, start, high, font) <= width) {
        low = high;
        if (high == end)
            return end;
        size_t span = high - start;
        high = span > (end - start) / 2 ? end : start + span * 2;
        while (high < end && ((unsigned char)text.data[high] & 0xc0) == 0x80)
            high++;
    }
    while (low < high) {
        size_t middle = low + (high - low + 1) / 2;
        while (middle < high && ((unsigned char)text.data[middle] & 0xc0) == 0x80)
            middle++;
        if (cui_textarea_width(ui, text, start, middle, font) <= width)
            low = middle;
        else
            high = cui_textarea_previous(text, middle);
    }
    return low;
}
bool cui_textarea_state_init(cui_textarea_state *state, cui_textarea_line *lines, size_t capacity) {
    if (!state || !lines || !capacity || capacity > 65536)
        return false;
    *state = (cui_textarea_state){.lines = lines, .capacity = capacity};
    return true;
}
cui_textarea_options cui_textarea_default(void) {
    return (cui_textarea_options){0};
}
cui_id cui_textarea_viewport_id(cui_id id) {
    return id ^ UINT64_C(0x850b7bf0a81c9365);
}
bool cui_textarea_measure(cui_textarea_options options, float root, cui_textarea_metrics *out) {
    if (!out || options.size < CUI_TEXTAREA_MEDIUM || options.size > CUI_TEXTAREA_CUSTOM ||
        options.tab_size > 4096 || !isfinite(root) || root <= 0 || root > 4096)
        return false;
    float density = root / 16;
    float font = options.size == CUI_TEXTAREA_XSMALL  ? 12
                 : options.size == CUI_TEXTAREA_LARGE ? 16
                                                      : 14;
    float px = options.size == CUI_TEXTAREA_XSMALL  ? 4
               : options.size == CUI_TEXTAREA_SMALL ? 8
               : options.size == CUI_TEXTAREA_LARGE ? 12
                                                    : 10;
    float py = options.size == CUI_TEXTAREA_XSMALL  ? 0
               : options.size == CUI_TEXTAREA_SMALL ? 2
               : options.size == CUI_TEXTAREA_LARGE ? 10
                                                    : 8;
    if (options.size == CUI_TEXTAREA_CUSTOM) {
        if (!isfinite(options.custom_size) || options.custom_size <= 0 ||
            options.custom_size > 4096)
            return false;
        font = options.custom_size * .875f / density;
        px = 8;
        py = 2;
    }
    *out = (cui_textarea_metrics){font * density, 20 * density, px * density, py * density};
    return true;
}
static size_t cui_textarea_cursor_line(const cui_textarea_state *state, size_t cursor) {
    for (size_t i = 0; i < state->count; i++) {
        cui_textarea_line line = state->lines[i];
        if (cursor < line.end ||
            (cursor == line.end &&
             (line.hard_break || state->upstream_affinity || i + 1 == state->count)) ||
            cursor < line.next)
            return i;
    }
    return state->count ? state->count - 1 : 0;
}
cui_textarea_error cui_textarea_layout(cui_context *ui, cui_edit *edit, cui_textarea_state *state,
                                       cui_textarea_metrics metrics, float width, bool wrap) {
    if (!ui || !edit || !state || !state->lines || !state->capacity || state->capacity > 65536 ||
        !cui_edit_get_state(edit).multiline || !isfinite(width) || width <= 0 ||
        !isfinite(metrics.font_size) || metrics.font_size <= 0 || !isfinite(metrics.line_height) ||
        metrics.line_height <= 0) {
        if (state)
            state->error = CUI_TEXTAREA_INVALID;
        return CUI_TEXTAREA_INVALID;
    }
    cui_str text = cui_edit_get_text(edit);
    state->count = 0;
    state->font_size = metrics.font_size;
    state->line_height = metrics.line_height;
    state->wrap_width = width;
    if (text.length > 65536)
        return state->error = CUI_TEXTAREA_CAPACITY;
    size_t start = 0;
    float maximum = 0;
    for (;;) {
        if (state->count == state->capacity) {
            state->count = 0;
            return state->error = CUI_TEXTAREA_CAPACITY;
        }
        size_t end = start;
        while (end < text.length && text.data[end] != '\n' && text.data[end] != '\r')
            end++;
        size_t next = end;
        bool hard = end < text.length;
        if (hard) {
            next++;
            if (text.data[end] == '\r' && next < text.length && text.data[next] == '\n')
                next++;
        }
        size_t finish = end;
        if (wrap && start < end) {
            finish = cui_textarea_fit(ui, text, start, end, metrics.font_size, width);
            if (finish == start)
                finish = cui_textarea_next(text, start);
            if (finish < end) {
                size_t word = finish;
                while (word > start && text.data[word - 1] != ' ' && text.data[word - 1] != '\t')
                    word--;
                if (word > start)
                    finish = word;
                next = finish;
                hard = false;
            }
        }
        float measured = cui_textarea_width(ui, text, start, finish, metrics.font_size);
        state->lines[state->count++] = (cui_textarea_line){start, finish, next, measured, hard};
        maximum = fmaxf(maximum, measured);
        if (finish == text.length)
            break;
        start = next;
    }
    cui_edit_state current = cui_edit_get_state(edit);
    state->caret_line = cui_textarea_cursor_line(state, current.cursor);
    state->revision = current.revision;
    state->cursor = current.cursor;
    state->anchor = current.anchor;
    state->initialized = true;
    state->maximum_x = wrap ? 0 : fmaxf(0, maximum + 1 - width);
    state->maximum_y = fmaxf(0, state->count * metrics.line_height - fmaxf(0, state->viewport.h));
    state->scroll_x =
        isfinite(state->scroll_x) ? fminf(state->maximum_x, fmaxf(0, state->scroll_x)) : 0;
    state->scroll_y =
        isfinite(state->scroll_y) ? fminf(state->maximum_y, fmaxf(0, state->scroll_y)) : 0;
    return state->error = CUI_TEXTAREA_OK;
}
static size_t cui_textarea_offset(cui_context *ui, cui_str text, const cui_textarea_state *state,
                                  size_t row, float x) {
    cui_textarea_line line = state->lines[row];
    if (x <= 0)
        return line.start;
    if (x >= line.width)
        return line.end;
    size_t left = cui_textarea_fit(ui, text, line.start, line.end, state->font_size, x);
    size_t right = cui_textarea_next(text, left);
    if (right > line.end)
        right = line.end;
    float a = cui_textarea_width(ui, text, line.start, left, state->font_size);
    float b = cui_textarea_width(ui, text, line.start, right, state->font_size);
    return x - a < b - x ? left : right;
}
size_t cui_textarea_hit_test(cui_context *ui, cui_edit *edit, cui_textarea_state *state, float x,
                             float y) {
    if (!ui || !edit || !state || !state->count || !isfinite(x) || !isfinite(y))
        return SIZE_MAX;
    float relative = fmaxf(0, (y - state->viewport.y + state->scroll_y) / state->line_height);
    size_t row = relative >= state->count ? state->count - 1 : (size_t)relative;
    size_t cursor = cui_textarea_offset(ui, cui_edit_get_text(edit), state, row,
                                        x - state->viewport.x + state->scroll_x);
    state->upstream_affinity = cursor == state->lines[row].end && !state->lines[row].hard_break;
    return cursor;
}
static void cui_textarea_reveal(cui_context *ui, cui_edit *edit, cui_textarea_state *state) {
    cui_str text = cui_edit_get_text(edit);
    cui_textarea_line line = state->lines[state->caret_line];
    size_t cursor = state->cursor < line.end ? state->cursor : line.end;
    float x = cui_textarea_width(ui, text, line.start, cursor, state->font_size);
    float y = state->caret_line * state->line_height;
    if (x < state->scroll_x)
        state->scroll_x = x;
    if (x + 1 > state->scroll_x + state->viewport.w)
        state->scroll_x = x + 1 - state->viewport.w;
    if (y < state->scroll_y)
        state->scroll_y = y;
    if (y + state->line_height > state->scroll_y + state->viewport.h)
        state->scroll_y = y + state->line_height - state->viewport.h;
    state->scroll_x = fminf(state->maximum_x, fmaxf(0, state->scroll_x));
    state->scroll_y = fminf(state->maximum_y, fmaxf(0, state->scroll_y));
}
static cui_edit_result cui_textarea_clipboard(cui_edit *edit, cui_event event,
                                              cui_textarea_options options) {
    cui_edit_state selection = cui_edit_get_state(edit);
    size_t first = selection.anchor < selection.cursor ? selection.anchor : selection.cursor;
    size_t last = selection.anchor > selection.cursor ? selection.anchor : selection.cursor;
    bool paste = event.key == CUI_KEY_V;
    if ((paste && options.readonly) || (!paste && first == last))
        return CUI_EDIT_OK;
    const cui_platform *host = options.clipboard;
    if (!host || !(host->capabilities & CUI_PLATFORM_CLIPBOARD) ||
        (paste ? !host->clipboard_get : !host->clipboard_set))
        return CUI_EDIT_CLIPBOARD_UNAVAILABLE;
    if (paste)
        return cui_edit_insert(edit, host->clipboard_get(host->user));
    cui_str text = cui_edit_get_text(edit);
    if (!host->clipboard_set(host->user, (cui_str){text.data + first, last - first}))
        return CUI_EDIT_CLIPBOARD_UNAVAILABLE;
    return event.key == CUI_KEY_X && !options.readonly ? cui_edit_delete(edit) : CUI_EDIT_OK;
}
cui_textarea_result cui_base_textarea(cui_context *ui, cui_id id, cui_str name, cui_edit *edit,
                                      cui_textarea_state *state, cui_textarea_options options,
                                      cui_textarea_metrics metrics) {
    cui_textarea_result result = {0};
    if (!ui || !id || !edit || !state || (name.length && !name.data)) {
        result.error = CUI_TEXTAREA_INVALID;
        return result;
    }
    cui_edit_state before = cui_edit_get_state(edit);
    bool external =
        state->initialized && (state->revision != before.revision ||
                               state->cursor != before.cursor || state->anchor != before.anchor);
    if (external) {
        state->preferred_valid = false;
        state->upstream_affinity = false;
        state->dragging = false;
    }
    float width = state->viewport.w > 0 ? state->viewport.w : state->wrap_width;
    result.error = cui_textarea_layout(ui, edit, state, metrics, width, !options.no_wrap);
    if (result.error != CUI_TEXTAREA_OK)
        return result;
    cui_interaction interaction =
        cui_interact(ui, id, CUI_ROLE_TEXTBOX, name, options.disabled, false, (float)before.length);
    cui_set_tab_stop(ui, id, !options.no_tab_stop);
    cui_set_semantic_details(ui, id,
                             (cui_semantic_details){.handles_tab = !options.disabled &&
                                                                   !options.readonly &&
                                                                   !options.no_indent});
    if (options.custom_role && !cui_set_semantic_role(ui, id, options.role)) {
        result.error = CUI_TEXTAREA_INVALID;
        return result;
    }
    if (options.disabled)
        state->dragging = false;
    float old_x = state->scroll_x, old_y = state->scroll_y;
    bool old_affinity = state->upstream_affinity;
    size_t old_row = state->caret_line;
    bool reveal = external && cui_focused(ui) == id && !options.disabled;
    size_t count = 0;
    const cui_event *events = cui_frame_events(ui, &count);
    for (size_t i = 0; i < count; i++) {
        cui_event event = events[i];
        if (event.type == CUI_EVENT_CANCEL)
            state->dragging = false;
        if (options.disabled || cui_event_consumed(ui, i) || cui_event_target(ui, i) != id ||
            (event.type == CUI_EVENT_KEY_DOWN &&
             (event.ime_composing || event.prefer_character_input)))
            continue;
        cui_edit_state event_before = cui_edit_get_state(edit);
        cui_edit_result error = CUI_EDIT_OK;
        bool handled = false, move = false;
        bool command = (event.modifiers & (CUI_MOD_CTRL | CUI_MOD_META)) != 0;
        bool shift = (event.modifiers & CUI_MOD_SHIFT) != 0;
        bool primary = event.pointer_button == CUI_POINTER_PRIMARY;
        if (event.type == CUI_EVENT_SCROLL && isfinite(event.dx) && isfinite(event.dy)) {
            state->scroll_x =
                fminf(state->maximum_x, fmaxf(0, state->scroll_x - event.dx * metrics.line_height));
            state->scroll_y =
                fminf(state->maximum_y, fmaxf(0, state->scroll_y - event.dy * metrics.line_height));
            handled = state->scroll_x != old_x || state->scroll_y != old_y;
        } else if (primary && (event.type == CUI_EVENT_POINTER_DOWN ||
                               ((state->dragging || interaction.pressed) &&
                                (event.type == CUI_EVENT_POINTER_MOVE ||
                                 event.type == CUI_EVENT_POINTER_UP)))) {
            size_t cursor = cui_textarea_hit_test(ui, edit, state, event.x, event.y);
            cui_edit_state selection = cui_edit_get_state(edit);
            size_t anchor =
                event.type == CUI_EVENT_POINTER_DOWN && !shift ? cursor : selection.anchor;
            error = cui_edit_select(edit, anchor, cursor);
            state->dragging = event.type != CUI_EVENT_POINTER_UP;
            move = handled = true;
        } else if (event.type == CUI_EVENT_KEY_DOWN && command &&
                   (event.key == CUI_KEY_C || event.key == CUI_KEY_X || event.key == CUI_KEY_V)) {
            error = cui_textarea_clipboard(edit, event, options);
            handled = true;
        } else if (event.type == CUI_EVENT_KEY_DOWN && command &&
                   (event.key == CUI_KEY_UP || event.key == CUI_KEY_DOWN)) {
            error =
                cui_edit_move(edit, event.key == CUI_KEY_UP ? CUI_EDIT_START : CUI_EDIT_END, shift);
            state->upstream_affinity = false;
            state->preferred_valid = false;
            move = handled = true;
        } else if (event.type == CUI_EVENT_KEY_DOWN &&
                   (event.key == CUI_KEY_UP || event.key == CUI_KEY_DOWN ||
                    event.key == CUI_KEY_PAGEUP || event.key == CUI_KEY_PAGEDOWN)) {
            cui_edit_state selection = cui_edit_get_state(edit);
            size_t row = state->caret_line;
            cui_textarea_line line = state->lines[row];
            if (!state->preferred_valid) {
                state->preferred_x = cui_textarea_width(
                    ui, cui_edit_get_text(edit), line.start,
                    selection.cursor < line.end ? selection.cursor : line.end, metrics.font_size);
                state->preferred_valid = true;
            }
            size_t step = event.key == CUI_KEY_PAGEUP || event.key == CUI_KEY_PAGEDOWN
                              ? (size_t)fmaxf(1, floorf(state->viewport.h / metrics.line_height))
                              : 1;
            bool up = event.key == CUI_KEY_UP || event.key == CUI_KEY_PAGEUP;
            row = up ? (step > row ? 0 : row - step)
                     : (step >= state->count - row ? state->count - 1 : row + step);
            size_t cursor =
                cui_textarea_offset(ui, cui_edit_get_text(edit), state, row, state->preferred_x);
            state->upstream_affinity =
                cursor == state->lines[row].end && !state->lines[row].hard_break;
            error = cui_edit_select(edit, shift ? selection.anchor : cursor, cursor);
            move = handled = true;
        } else if (event.type == CUI_EVENT_KEY_DOWN && !command &&
                   ((event.key == CUI_KEY_RIGHT && state->upstream_affinity &&
                     state->caret_line + 1 < state->count &&
                     state->cursor == state->lines[state->caret_line].end &&
                     !state->lines[state->caret_line].hard_break) ||
                    (event.key == CUI_KEY_LEFT && !state->upstream_affinity && state->caret_line &&
                     state->cursor == state->lines[state->caret_line].start &&
                     !state->lines[state->caret_line - 1].hard_break)) &&
                   (shift || state->cursor == state->anchor)) {
            state->upstream_affinity = event.key == CUI_KEY_LEFT;
            state->preferred_valid = false;
            move = handled = true;
        } else if (event.type == CUI_EVENT_KEY_DOWN && !command &&
                   (event.key == CUI_KEY_HOME || event.key == CUI_KEY_END)) {
            cui_edit_state selection = cui_edit_get_state(edit);
            cui_textarea_line line = state->lines[state->caret_line];
            size_t cursor = event.key == CUI_KEY_HOME ? line.start : line.end;
            state->upstream_affinity = event.key == CUI_KEY_END && !line.hard_break;
            error = cui_edit_select(edit, shift ? selection.anchor : cursor, cursor);
            move = handled = true;
            state->preferred_valid = false;
        } else if (event.type == CUI_EVENT_KEY_DOWN && event.key == CUI_KEY_ENTER) {
            if (!options.readonly) {
                result.pressed_enter = true;
                result.enter_secondary = command;
                result.enter_shift = shift;
                if (!options.submit_on_enter || shift) {
                    error = cui_edit_apply_event(edit, event);
                    move = true;
                }
            }
            state->upstream_affinity = false;
            state->preferred_valid = false;
            handled = true;
        } else if (event.type == CUI_EVENT_KEY_DOWN && event.key == CUI_KEY_TAB && !command &&
                   !options.readonly && !options.no_indent) {
            error = cui_edit_indent(edit, shift, options.tab_size ? options.tab_size : 2,
                                    options.hard_tabs);
            state->upstream_affinity = false;
            state->preferred_valid = false;
            move = handled = true;
        } else if (event.type == CUI_EVENT_TEXT ||
                   (event.type == CUI_EVENT_KEY_DOWN &&
                    (event.key == CUI_KEY_LEFT || event.key == CUI_KEY_RIGHT ||
                     event.key == CUI_KEY_HOME || event.key == CUI_KEY_END ||
                     event.key == CUI_KEY_ENTER || event.key == CUI_KEY_BACKSPACE ||
                     event.key == CUI_KEY_DELETE ||
                     (command && (event.key == CUI_KEY_A || event.key == CUI_KEY_Z ||
                                  event.key == CUI_KEY_Y))))) {
            bool navigation = event.type == CUI_EVENT_KEY_DOWN &&
                              (event.key == CUI_KEY_LEFT || event.key == CUI_KEY_RIGHT ||
                               event.key == CUI_KEY_HOME || event.key == CUI_KEY_END ||
                               (command && event.key == CUI_KEY_A));
            if (!options.readonly || navigation) {
                if (event.modifiers & CUI_MOD_META)
                    event.modifiers |= CUI_MOD_CTRL;
                error = cui_edit_apply_event(edit, event);
                move = true;
            }
            state->upstream_affinity = false;
            state->preferred_valid = false;
            handled = true;
        }
        cui_edit_state event_after = cui_edit_get_state(edit);
        if (event_after.revision != event_before.revision) {
            state->preferred_valid = false;
            state->upstream_affinity = false;
            move = true;
        }
        if (handled)
            cui_consume_event(ui, i);
        if (result.edit_error == CUI_EDIT_OK && error != CUI_EDIT_OK)
            result.edit_error = error;
        result.error = cui_textarea_layout(ui, edit, state, metrics, width, !options.no_wrap);
        if (result.error != CUI_TEXTAREA_OK)
            break;
        reveal |= move;
    }
    if (result.error == CUI_TEXTAREA_OK && reveal)
        cui_textarea_reveal(ui, edit, state);
    cui_edit_state after = cui_edit_get_state(edit);
    result.changed = after.revision != before.revision;
    result.selection_changed = after.cursor != before.cursor || after.anchor != before.anchor ||
                               old_affinity != state->upstream_affinity ||
                               old_row != state->caret_line;
    result.scrolled = old_x != state->scroll_x || old_y != state->scroll_y;
    cui_update_semantic(ui, id, false, (float)after.length);
    cui_set_semantic_details(
        ui, id,
        (cui_semantic_details){.multiline = true,
                               .readonly = options.readonly,
                               .handles_tab =
                                   !options.disabled && !options.readonly && !options.no_indent,
                               .required = options.required,
                               .invalid = options.invalid || result.edit_error != CUI_EDIT_OK ||
                                          result.error != CUI_TEXTAREA_OK,
                               .cursor = CUI_CURSOR_TEXT,
                               .scrollable = true,
                               .scroll_x = state->scroll_x,
                               .scroll_y = state->scroll_y,
                               .scroll_max_x = state->maximum_x,
                               .scroll_max_y = state->maximum_y});
    if (result.changed || result.selection_changed || result.scrolled)
        cui_invalidate(ui);
    return result;
}
