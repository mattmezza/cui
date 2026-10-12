/* SPDX-License-Identifier: MIT */
#include "cui/cui_picker.h"
#include <stdio.h>
#include <string.h>
static cui_id cui_picker_part(cui_id id, uint64_t index) {
    return id ^ (UINT64_C(0x19f617380ad34759) + UINT64_C(0x9e3779b97f4a7c15) * index);
}
int cui_date_compare(cui_date a, cui_date b) {
    if (a.year != b.year)
        return a.year < b.year ? -1 : 1;
    if (a.month != b.month)
        return a.month < b.month ? -1 : 1;
    return a.day == b.day ? 0 : a.day < b.day ? -1 : 1;
}
void cui_calendar_init(cui_calendar_state *state, cui_date date) {
    if (!state)
        return;
    if (!cui_date_valid(date))
        date = (cui_date){2000, 1, 1};
    *state = (cui_calendar_state){.month = {date.year, date.month, 1}, .cursor = date};
}
void cui_calendar_grid(cui_date month, unsigned first, cui_date out[42]) {
    if (!out)
        return;
    month.day = 1;
    if (!cui_date_valid(month)) {
        memset(out, 0, 42 * sizeof(*out));
        return;
    }
    first %= 7;
    int lead = (cui_date_weekday(month) - (int)first + 7) % 7;
    for (int i = 0; i < 42; ++i)
        out[i] = cui_date_add_days(month, i - lead);
}
bool cui_calendar_date_enabled(cui_date date, cui_calendar_options o) {
    return !o.disabled && cui_date_valid(date) &&
           (!cui_date_valid(o.minimum) || cui_date_compare(date, o.minimum) >= 0) &&
           (!cui_date_valid(o.maximum) || cui_date_compare(date, o.maximum) <= 0) &&
           (!o.is_disabled || !o.is_disabled(o.disabled_user, date));
}
cui_id cui_calendar_day_id(cui_id root, cui_date date) {
    return cui_date_valid(date)
               ? cui_picker_part(root, 100 + (uint64_t)date.year * 10000 +
                                           (uint64_t)date.month * 100 + (unsigned)date.day)
               : 0;
}
cui_id cui_calendar_navigation_id(cui_id root, unsigned part) {
    return cui_picker_part(root, part);
}
static unsigned cui_picker_months(cui_calendar_options options) {
    return options.months < 1 ? 1 : options.months > 3 ? 3 : options.months;
}
static bool cui_picker_selected(cui_date date, cui_date_selection selection, bool range) {
    if (!cui_date_valid(selection.start))
        return false;
    if (!range || !cui_date_valid(selection.end))
        return !cui_date_compare(date, selection.start);
    return cui_date_compare(date, selection.start) >= 0 &&
           cui_date_compare(date, selection.end) <= 0;
}
static cui_date cui_picker_target_date(cui_id id, cui_id target, cui_date month, unsigned months,
                                       unsigned first) {
    /* Neighbor cells duplicate across months; stable date IDs still identify one day. */
    for (unsigned m = 0; m < months; ++m) {
        cui_date grid[42];
        cui_calendar_grid(cui_date_add_months(month, (int)m), first, grid);
        for (unsigned i = 0; i < 42; ++i)
            if (cui_date_valid(grid[i]) && cui_calendar_day_id(id, grid[i]) == target)
                return grid[i];
    }
    return (cui_date){0};
}
static void cui_picker_select(cui_calendar_state *state, cui_date_selection *selection,
                              cui_date date, bool range) {
    if (range && state->selecting_end && cui_date_valid(selection->start)) {
        selection->end = date;
        if (cui_date_compare(selection->start, selection->end) > 0) {
            cui_date swap = selection->start;
            selection->start = selection->end;
            selection->end = swap;
        }
        state->selecting_end = false;
    } else {
        selection->start = date;
        selection->end = (cui_date){0};
        state->selecting_end = range;
    }
    state->cursor = date;
}
cui_calendar_result cui_calendar_reduce(cui_context *ui, cui_id id, cui_calendar_state *s,
                                        cui_date_selection *selection, cui_calendar_options o) {
    cui_calendar_result result = {0};
    if (!ui || !s || !selection)
        return result;
    if (!cui_date_valid(s->month))
        cui_calendar_init(s, selection->start);
    s->month.day = 1;
    if (!cui_date_valid(s->cursor))
        s->cursor = cui_date_valid(selection->start) ? selection->start : s->month;
    cui_date old_month = s->month;
    cui_date_selection before = *selection;
    unsigned months = cui_picker_months(o);
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    cui_id keyboard_source = 0;
    for (size_t e = 0; e < count && !o.disabled; ++e) {
        cui_id activation = cui_event_activation(ui, e);
        int shift = activation == cui_calendar_navigation_id(id, 1)   ? -12
                    : activation == cui_calendar_navigation_id(id, 2) ? -1
                    : activation == cui_calendar_navigation_id(id, 3) ? 1
                    : activation == cui_calendar_navigation_id(id, 4) ? 12
                                                                      : 0;
        if (shift) {
            cui_date next = cui_date_add_months(s->month, shift);
            if (cui_date_valid(next)) {
                s->month = next;
                s->cursor = next;
                result.navigated = true;
            }
            continue;
        }
        cui_date target =
            cui_picker_target_date(id, cui_event_target(ui, e), old_month, months, o.first_weekday);
        if (!cui_date_valid(target))
            continue;
        if (activation && cui_calendar_date_enabled(target, o)) {
            cui_picker_select(s, selection, target, o.range);
            result.activated = true;
        }
        if (events[e].type != CUI_EVENT_KEY_DOWN)
            continue;
        if (keyboard_source != cui_event_target(ui, e))
            s->cursor = target;
        keyboard_source = cui_event_target(ui, e);
        cui_key key = events[e].key;
        int step = 0;
        cui_date next = s->cursor;
        bool ctrl = (events[e].modifiers & (CUI_MOD_CTRL | CUI_MOD_META)) != 0;
        if (ctrl && (key == CUI_KEY_LEFT || key == CUI_KEY_RIGHT || key == CUI_KEY_UP ||
                     key == CUI_KEY_DOWN)) {
            int amount = key == CUI_KEY_LEFT    ? -1
                         : key == CUI_KEY_RIGHT ? 1
                         : key == CUI_KEY_UP    ? -12
                                                : 12;
            next = cui_date_add_months(s->cursor, amount);
            step = amount < 0 ? -1 : 1;
        } else if (key == CUI_KEY_LEFT || key == CUI_KEY_RIGHT || key == CUI_KEY_UP ||
                   key == CUI_KEY_DOWN) {
            step = key == CUI_KEY_LEFT ? -1 : key == CUI_KEY_RIGHT ? 1 : key == CUI_KEY_UP ? -7 : 7;
            next = cui_date_add_days(s->cursor, step);
        } else if (key == CUI_KEY_HOME || key == CUI_KEY_END) {
            int weekday = (cui_date_weekday(s->cursor) - (int)(o.first_weekday % 7) + 7) % 7;
            next = cui_date_add_days(s->cursor, key == CUI_KEY_HOME ? -weekday : 6 - weekday);
            step = key == CUI_KEY_HOME ? 1 : -1;
        }
        if (!step)
            continue;
        for (unsigned skip = 0;
             skip < 366 && cui_date_valid(next) && !cui_calendar_date_enabled(next, o); ++skip)
            next = cui_date_add_days(next, step);
        if (cui_calendar_date_enabled(next, o)) {
            s->cursor = next;
            cui_date end = cui_date_add_months(s->month, (int)months - 1);
            end.day = cui_date_days_in_month(end.year, end.month);
            if (cui_date_compare(next, s->month) < 0 || cui_date_compare(next, end) > 0)
                s->month = (cui_date){next.year, next.month, 1};
            cui_focus(ui, cui_calendar_day_id(id, next));
            result.navigated = true;
        }
    }
    cui_date final_month = cui_date_add_months(s->month, (int)months - 1);
    final_month.day = cui_date_days_in_month(final_month.year, final_month.month);
    if (!cui_calendar_date_enabled(s->cursor, o) || cui_date_compare(s->cursor, s->month) < 0 ||
        cui_date_compare(s->cursor, final_month) > 0) {
        cui_date candidate = s->month;
        for (unsigned i = 0;
             i < 93 && cui_date_valid(candidate) && cui_date_compare(candidate, final_month) <= 0;
             ++i) {
            if (cui_calendar_date_enabled(candidate, o)) {
                s->cursor = candidate;
                break;
            }
            candidate = cui_date_add_days(candidate, 1);
        }
    }
    result.changed = cui_date_compare(before.start, selection->start) != 0 ||
                     cui_date_compare(before.end, selection->end) != 0;
    if (result.changed || result.navigated)
        cui_invalidate(ui);
    return result;
}
void cui_calendar_submit(cui_context *ui, cui_id id, cui_calendar_state *s,
                         cui_date_selection selection, cui_calendar_options o) {
    if (!ui || !s)
        return;
    unsigned months = cui_picker_months(o);
    cui_interact(ui, id, CUI_ROLE_GROUP, cui_string("Calendar"), o.disabled, false, 0);
    cui_set_tab_stop(ui, id, false);
    const char *names[] = {"Previous year", "Previous month", "Next month", "Next year"};
    const int shifts[] = {-12, -1, 1, 12};
    for (unsigned i = 0; i < 4; ++i)
        cui_interact(
            ui, cui_calendar_navigation_id(id, i + 1), CUI_ROLE_BUTTON, cui_string(names[i]),
            o.disabled || !cui_date_valid(cui_date_add_months(s->month, shifts[i])), false, 0);
    cui_date seen[126];
    size_t seen_count = 0;
    for (unsigned m = 0; m < months; ++m) {
        cui_date month = cui_date_add_months(s->month, (int)m), grid[42];
        cui_calendar_grid(month, o.first_weekday, grid);
        for (unsigned i = 0; i < 42; ++i) {
            if (!cui_date_valid(grid[i]) || grid[i].month != month.month ||
                grid[i].year != month.year)
                continue;
            bool duplicate = false;
            for (size_t j = 0; j < seen_count; ++j)
                if (!cui_date_compare(seen[j], grid[i]))
                    duplicate = true;
            if (duplicate)
                continue;
            seen[seen_count++] = grid[i];
            char name[16];
            snprintf(name, sizeof(name), "%04d-%02d-%02d", grid[i].year, grid[i].month,
                     grid[i].day);
            cui_id day = cui_calendar_day_id(id, grid[i]);
            bool selected = cui_picker_selected(grid[i], selection, o.range);
            cui_interact(ui, day, CUI_ROLE_BUTTON, cui_string(name),
                         !cui_calendar_date_enabled(grid[i], o), selected, (float)grid[i].day);
            cui_set_semantic_details(ui, day,
                                     (cui_semantic_details){.selected = selected, .parent = id});
            cui_set_tab_stop(ui, day, !cui_date_compare(s->cursor, grid[i]));
        }
    }
}
cui_calendar_result cui_base_calendar(cui_context *ui, cui_id id, cui_calendar_state *s,
                                      cui_date_selection *selection, cui_calendar_options o) {
    cui_calendar_result result = cui_calendar_reduce(ui, id, s, selection, o);
    if (selection)
        cui_calendar_submit(ui, id, s, *selection, o);
    return result;
}
void cui_date_picker_init(cui_date_picker_state *s, cui_id popup, cui_date initial) {
    if (!s)
        return;
    *s = (cui_date_picker_state){0};
    cui_overlay_init(&s->overlay, popup, true);
    cui_calendar_init(&s->calendar, initial);
}
void cui_base_date_picker_trigger(cui_context *ui, cui_id id, cui_str name,
                                  cui_date_picker_state *s, bool disabled) {
    if (!ui || !s)
        return;
    if (s->overlay.open && s->overlay.has_bounds)
        cui_overlay_set_bounds(&s->overlay, cui_bounds(ui, s->overlay.id));
    cui_overlay_update(ui, &s->overlay);
    if (disabled && s->overlay.open)
        cui_overlay_close(ui, &s->overlay);
    size_t count;
    cui_frame_events(ui, &count);
    for (size_t e = 0; e < count && !disabled; ++e)
        if (cui_event_activation(ui, e) == id) {
            if (s->overlay.open)
                cui_overlay_close(ui, &s->overlay);
            else
                cui_overlay_open(ui, &s->overlay, id);
        }
    cui_interact(ui, id, CUI_ROLE_BUTTON, name, disabled, false, 0);
    cui_set_semantic_details(
        ui, id, (cui_semantic_details){.expanded = s->overlay.open, .controls = s->overlay.id});
}

cui_calendar_result cui_base_date_picker(cui_context *ui, cui_id trigger, cui_id calendar,
                                         cui_str name, cui_date_picker_state *s,
                                         cui_date_selection *selection, cui_date_picker_options o) {
    cui_calendar_result result = {0};
    if (!ui || !s || !selection)
        return result;
    cui_base_date_picker_trigger(ui, trigger, name, s, o.disabled);
    if (s->overlay.open) {
        cui_calendar_options options = o.calendar;
        options.disabled = options.disabled || o.disabled;
        result = cui_calendar_reduce(ui, calendar, &s->calendar, selection, options);
        if (result.activated && (!options.range || !s->calendar.selecting_end))
            cui_overlay_close(ui, &s->overlay);
    }
    cui_set_semantic_details(
        ui, trigger,
        (cui_semantic_details){.expanded = s->overlay.open, .controls = s->overlay.id});
    return result;
}
bool cui_time_valid(cui_time time) {
    return time.hour < 24 && time.minute < 60 && time.second < 60;
}
bool cui_time_parse(cui_str text, cui_time *time) {
    if (!time || !text.data || (text.length != 5 && text.length != 8))
        return false;
    if (text.data[2] != ':' || (text.length == 8 && text.data[5] != ':'))
        return false;
    for (size_t i = 0; i < text.length; ++i)
        if (i != 2 && i != 5 && (text.data[i] < '0' || text.data[i] > '9'))
            return false;
    cui_time parsed = {(unsigned)(text.data[0] - '0') * 10 + (unsigned)(text.data[1] - '0'),
                       (unsigned)(text.data[3] - '0') * 10 + (unsigned)(text.data[4] - '0'),
                       text.length == 8
                           ? (unsigned)(text.data[6] - '0') * 10 + (unsigned)(text.data[7] - '0')
                           : 0};
    if (!cui_time_valid(parsed))
        return false;
    *time = parsed;
    return true;
}
size_t cui_time_format(cui_time time, bool seconds, char *out, size_t capacity) {
    if (!cui_time_valid(time)) {
        if (out && capacity)
            out[0] = 0;
        return 0;
    }
    char text[9];
    int n = seconds ? snprintf(text, sizeof(text), "%02u:%02u:%02u", time.hour, time.minute,
                               time.second)
                    : snprintf(text, sizeof(text), "%02u:%02u", time.hour, time.minute);
    size_t length = n > 0 ? (size_t)n : 0;
    if (out && capacity) {
        size_t copy = length < capacity ? length : capacity - 1;
        memcpy(out, text, copy);
        out[copy] = 0;
    }
    return length;
}
cui_id cui_time_segment_id(cui_id id, unsigned segment) {
    return cui_picker_part(id, 10 + segment);
}
static unsigned cui_picker_time_next(unsigned segment, cui_time_options options, int direction) {
    const unsigned segments[4] = {0, 1, options.seconds ? 2 : 3, 3};
    unsigned count = 2 + (options.seconds ? 1 : 0) + (options.twelve_hour ? 1 : 0), index = 0;
    for (unsigned i = 0; i < count; ++i)
        if (segments[i] == segment)
            index = i;
    return segments[direction < 0 ? (index ? index - 1 : count - 1) : (index + 1) % count];
}
bool cui_base_time_field(cui_context *ui, cui_id id, cui_str name, cui_time_state *state,
                         cui_time *time, cui_time_options options) {
    if (!ui || !state || !time || !cui_time_valid(*time))
        return false;
    cui_time before = *time;
    if (state->segment > 3 || (state->segment == 2 && !options.seconds) ||
        (state->segment == 3 && !options.twelve_hour))
        state->segment = 0;
    if (state->digit_count > 1)
        state->digit_count = 0;
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    cui_id source = 0;
    for (size_t e = 0; e < count && !options.disabled && !options.readonly; ++e) {
        cui_id target = cui_event_target(ui, e);
        unsigned segment = 4;
        for (unsigned i = 0; i < 4; ++i)
            if (target == cui_time_segment_id(id, i))
                segment = i;
        if (segment == 4 || (segment == 2 && !options.seconds) ||
            (segment == 3 && !options.twelve_hour))
            continue;
        if (source != target && (source || state->segment != segment)) {
            state->segment = segment;
            state->digit_count = 0;
        }
        source = target;
        if (events[e].type == CUI_EVENT_POINTER_DOWN) {
            state->segment = segment;
            state->digit_count = 0;
        }
        if (events[e].type == CUI_EVENT_KEY_DOWN) {
            cui_key key = events[e].key;
            if (key == CUI_KEY_LEFT || key == CUI_KEY_RIGHT) {
                state->segment =
                    cui_picker_time_next(state->segment, options, key == CUI_KEY_LEFT ? -1 : 1);
                state->digit_count = 0;
                cui_focus(ui, cui_time_segment_id(id, state->segment));
            } else if (key == CUI_KEY_UP || key == CUI_KEY_DOWN || key == CUI_KEY_HOME ||
                       key == CUI_KEY_END) {
                unsigned *value = state->segment == 0   ? &time->hour
                                  : state->segment == 1 ? &time->minute
                                                        : &time->second;
                unsigned maximum = state->segment == 0 ? 24 : 60;
                if (state->segment == 3)
                    time->hour = (time->hour + 12) % 24;
                else if (key == CUI_KEY_HOME)
                    *value = 0;
                else if (key == CUI_KEY_END)
                    *value = maximum - 1;
                else
                    *value = key == CUI_KEY_UP ? (*value + 1) % maximum
                                               : (*value ? *value - 1 : maximum - 1);
                state->digit_count = 0;
            }
        } else if (events[e].type == CUI_EVENT_TEXT) {
            const char *text = events[e].text;
            for (size_t i = 0; i < sizeof(events[e].text) && text[i]; ++i) {
                if (state->segment == 3) {
                    if (text[i] == 'a' || text[i] == 'A')
                        time->hour %= 12;
                    else if (text[i] == 'p' || text[i] == 'P')
                        time->hour = time->hour % 12 + 12;
                    continue;
                }
                if (text[i] < '0' || text[i] > '9')
                    continue;
                unsigned digit = (unsigned)(text[i] - '0');
                state->draft = state->digit_count ? state->draft * 10 + digit : digit;
                ++state->digit_count;
                unsigned maximum = state->segment == 0 ? (options.twelve_hour ? 12 : 23) : 59;
                if (state->draft <= maximum &&
                    !(options.twelve_hour && state->segment == 0 && !state->draft)) {
                    unsigned *value = state->segment == 0   ? &time->hour
                                      : state->segment == 1 ? &time->minute
                                                            : &time->second;
                    *value = state->segment == 0 && options.twelve_hour
                                 ? state->draft % 12 + (time->hour >= 12 ? 12 : 0)
                                 : state->draft;
                }
                if (state->digit_count >= 2) {
                    state->digit_count = 0;
                    state->segment = cui_picker_time_next(state->segment, options, 1);
                    cui_focus(ui, cui_time_segment_id(id, state->segment));
                }
            }
        }
    }
    cui_interact(ui, id, CUI_ROLE_GROUP, name, options.disabled, false, 0);
    cui_set_tab_stop(ui, id, false);
    const char *labels[] = {"Hour", "Minute", "Second", "AM/PM"};
    for (unsigned i = 0; i < 4; ++i) {
        if ((i == 2 && !options.seconds) || (i == 3 && !options.twelve_hour))
            continue;
        cui_id segment = cui_time_segment_id(id, i);
        unsigned value = i == 0   ? time->hour
                         : i == 1 ? time->minute
                         : i == 2 ? time->second
                                  : time->hour >= 12;
        cui_interact(ui, segment, CUI_ROLE_TEXTBOX, cui_string(labels[i]), options.disabled, false,
                     (float)value);
        cui_set_semantic_details(ui, segment,
                                 (cui_semantic_details){.readonly = options.readonly,
                                                        .parent = id,
                                                        .minimum = 0,
                                                        .maximum = i == 0   ? 23
                                                                   : i == 3 ? 1
                                                                            : 59});
        cui_set_tab_stop(ui, segment, i == state->segment);
    }
    bool changed =
        before.hour != time->hour || before.minute != time->minute || before.second != time->second;
    if (changed)
        cui_invalidate(ui);
    return changed;
}
