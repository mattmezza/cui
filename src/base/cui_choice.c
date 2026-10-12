/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
#include "cui/cui_base.h"
#include <math.h>
static size_t cui_choice_navigate(const cui_choice_item *items, size_t count, size_t from,
                                  cui_key key, bool vertical) {
    int direction = 0;
    if (key == CUI_KEY_HOME) {
        from = count - 1;
        direction = 1;
    } else if (key == CUI_KEY_END) {
        from = 0;
        direction = -1;
    } else if (key == (vertical ? CUI_KEY_DOWN : CUI_KEY_RIGHT))
        direction = 1;
    else if (key == (vertical ? CUI_KEY_UP : CUI_KEY_LEFT))
        direction = -1;
    if (!direction)
        return count;
    for (size_t n = 0; n < count; ++n) {
        from = direction > 0 ? (from + 1) % count : (from ? from - 1 : count - 1);
        if (!items[from].disabled)
            return from;
    }
    return count;
}
bool cui_base_tabs(cui_context *ui, const cui_choice_item *items, size_t count, cui_id *selected,
                   cui_tabs_options options) {
    if (!items || !selected || !count)
        return false;
    cui_id old = *selected;
    for (size_t i = 0; i < count; ++i) {
        cui_interact(ui, items[i].id, CUI_ROLE_TAB, items[i].label,
                     options.disabled || items[i].disabled, *selected == items[i].id, (float)i);
    }
    size_t event_count = 0, cursor = count;
    const cui_event *events = cui_frame_events(ui, &event_count);
    for (size_t e = 0; e < event_count && !options.disabled; ++e) {
        if (events[e].type == CUI_EVENT_KEY_DOWN && events[e].key == CUI_KEY_TAB)
            cursor = count;
        size_t from = count;
        for (size_t i = 0; i < count; ++i)
            if (items[i].id == cui_event_target(ui, e))
                from = i;
        if (from == count || items[from].disabled)
            continue;
        if (cursor == count)
            cursor = from;
        if (cui_event_activation(ui, e) && (events[e].type != CUI_EVENT_KEY_UP ||
                                            items[cursor].id == cui_event_activation(ui, e))) {
            if (events[e].type == CUI_EVENT_POINTER_UP)
                cursor = from;
            *selected = items[cursor].id;
        }
        size_t next =
            events[e].type == CUI_EVENT_KEY_DOWN
                ? cui_choice_navigate(items, count, cursor, events[e].key, options.vertical)
                : count;
        if (next != count) {
            cursor = next;
            cui_focus(ui, items[next].id);
            if (!options.manual_activation)
                *selected = items[next].id;
        }
    }
    size_t stop = count;
    for (size_t i = 0; i < count; ++i)
        if (!items[i].disabled && items[i].id == cui_focused(ui))
            stop = i;
    if (stop == count)
        for (size_t i = 0; i < count; ++i)
            if (!items[i].disabled && items[i].id == *selected)
                stop = i;
    if (stop == count)
        for (size_t i = 0; i < count; ++i)
            if (!items[i].disabled) {
                stop = i;
                break;
            }
    for (size_t i = 0; i < count; ++i) {
        cui_update_semantic(ui, items[i].id, *selected == items[i].id, (float)i);
        cui_set_semantic_details(
            ui, items[i].id,
            (cui_semantic_details){.selected = *selected == items[i].id,
                                   .controls = items[i].id ^ UINT64_C(0x691a17db80e22f45)});
        cui_set_tab_stop(ui, items[i].id, i == stop);
    }
    return old != *selected;
}
bool cui_base_toggle_group(cui_context *ui, const cui_choice_item *items, size_t count,
                           bool *selected, cui_toggle_group_options options) {
    if (!items || !selected || !count)
        return false;
    bool changed = false;
    for (size_t i = 0; i < count; ++i) {
        cui_interact(ui, items[i].id, CUI_ROLE_BUTTON, items[i].label,
                     options.disabled || items[i].disabled, selected[i], selected[i]);
    }
    size_t event_count = 0, cursor = count;
    const cui_event *events = cui_frame_events(ui, &event_count);
    for (size_t e = 0; e < event_count && !options.disabled; ++e) {
        if (events[e].type == CUI_EVENT_KEY_DOWN && events[e].key == CUI_KEY_TAB)
            cursor = count;
        size_t from = count;
        for (size_t i = 0; i < count; ++i)
            if (items[i].id == cui_event_target(ui, e))
                from = i;
        if (from == count || items[from].disabled)
            continue;
        if (cursor == count)
            cursor = from;
        if (cui_event_activation(ui, e) && (events[e].type != CUI_EVENT_KEY_UP ||
                                            items[cursor].id == cui_event_activation(ui, e))) {
            if (events[e].type == CUI_EVENT_POINTER_UP)
                cursor = from;
            bool next_selected = !selected[cursor];
            size_t active_count = 0;
            for (size_t j = 0; j < count; j++)
                active_count += selected[j];
            if (next_selected || options.allow_empty || active_count > 1) {
                if (next_selected && !options.multiple)
                    for (size_t j = 0; j < count; j++)
                        selected[j] = false;
                selected[cursor] = next_selected;
                changed = true;
            }
        }
        size_t next =
            events[e].type == CUI_EVENT_KEY_DOWN
                ? cui_choice_navigate(items, count, cursor, events[e].key, options.vertical)
                : count;
        if (next != count) {
            cursor = next;
            cui_focus(ui, items[next].id);
        }
    }
    for (size_t i = 0; i < count; ++i)
        cui_update_semantic(ui, items[i].id, selected[i], selected[i]);
    return changed;
}

static unsigned char cui_choice_lower(unsigned char c) {
    return c >= 'A' && c <= 'Z' ? (unsigned char)(c + ('a' - 'A')) : c;
}
static bool cui_choice_prefix(cui_str label, const char *prefix, size_t length) {
    if (!label.data || label.length < length)
        return false;
    for (size_t i = 0; i < length; ++i)
        if (cui_choice_lower((unsigned char)label.data[i]) !=
            cui_choice_lower((unsigned char)prefix[i]))
            return false;
    return true;
}
void cui_choice_popup_init(cui_choice_popup *state, cui_id popup_id) {
    if (!state)
        return;
    *state = (cui_choice_popup){0};
    cui_overlay_init(&state->overlay, popup_id, true);
}
static bool cui_choice_scroll_event(cui_context *ui, const cui_choice_item *items, size_t count,
                                    cui_list_state *state, size_t length, size_t *begin,
                                    cui_rect bounds, cui_id scope, cui_id receiving, cui_id routed,
                                    cui_event event) {
    if ((!receiving || (routed != receiving && (!scope || routed != scope))) || !count || !length ||
        event.type != CUI_EVENT_SCROLL || !isfinite(event.dy) || !isfinite(bounds.x) ||
        !isfinite(bounds.y) || !isfinite(bounds.w) || !isfinite(bounds.h) || bounds.w <= 0 ||
        bounds.h <= 0 || event.x < bounds.x || event.x >= bounds.x + bounds.w ||
        event.y < bounds.y || event.y >= bounds.y + bounds.h ||
        !cui_pointer_scope_allowed(ui, scope))
        return false;
    if (length > count)
        length = count;
    if (*begin > count - length)
        *begin = count - length;
    float amount = state->scroll_remainder - event.dy;
    if (!isfinite(amount))
        amount = event.dy > 0 ? -(float)count : (float)count;
    float whole = truncf(amount);
    state->scroll_remainder = amount - whole;
    size_t before = *begin;
    if (whole < 0) {
        float steps = -whole;
        *begin = steps >= (float)*begin ? 0 : *begin - (size_t)steps;
    } else {
        size_t remaining = count - length - *begin;
        *begin += whole >= (float)remaining ? remaining : (size_t)whole;
    }
    if (before == *begin)
        return false;
    bool focused_option = false, active_visible = false;
    for (size_t i = 0; i < count; i++) {
        focused_option |= cui_focused(ui) == items[i].id;
        if (i >= *begin && i < *begin + length && !items[i].disabled &&
            items[i].id == state->active)
            active_visible = true;
    }
    if (!active_visible) {
        state->active = 0;
        for (size_t i = *begin; i < *begin + length; i++)
            if (!items[i].disabled) {
                state->active = items[i].id;
                break;
            }
        if (focused_option)
            cui_focus(ui, state->active);
    }
    cui_invalidate(ui);
    return true;
}
bool cui_base_list(cui_context *ui, const cui_choice_item *items, size_t count, cui_id *selected,
                   cui_list_state *state, cui_list_options options) {
    if (!items || !selected || !state || !count)
        return false;
    bool activated = false;
    if (state->search_length >= sizeof(state->search))
        state->search_length = 0;
    if (options.dt > 0 && options.dt < 60)
        state->search_time += options.dt;
    if (state->search_time > .75f)
        state->search_length = 0;
    size_t event_count = 0;
    const cui_event *events = cui_frame_events(ui, &event_count);
    size_t window_length = options.visible_count ? options.visible_count : count;
    if (window_length > count)
        window_length = count;
    size_t window_begin = options.visible_count ? options.visible_begin : 0;
    if (window_begin > count - window_length)
        window_begin = count - window_length;
    bool scrolled = options.window_changed;
    for (size_t e = 0; e < event_count && !options.disabled; ++e) {
        cui_event event = events[e];
        scrolled |= cui_choice_scroll_event(
            ui, items, count, state, options.visible_count, &window_begin, options.scroll_bounds,
            options.scroll_scope, options.scroll_target, cui_event_target(ui, e), event);
        if (event.type == CUI_EVENT_SCROLL)
            continue;
        size_t target = count, from = count;
        for (size_t i = 0; i < count; ++i) {
            if (items[i].id == cui_event_target(ui, e))
                target = i;
            if (items[i].id == state->active)
                from = i;
        }
        if (target == count || items[target].disabled)
            continue;
        if (from == count || items[from].disabled)
            from = target;
        if (cui_event_activation(ui, e) && !(scrolled && event.type == CUI_EVENT_POINTER_UP) &&
            (events[e].type != CUI_EVENT_KEY_UP || items[from].id == cui_event_activation(ui, e))) {
            size_t chosen = events[e].type == CUI_EVENT_POINTER_UP ? target : from;
            *selected = state->active = items[chosen].id;
            activated = true;
            if (options.on_activate)
                options.on_activate(items[chosen].id, options.activation_user);
        }
        if (events[e].type == CUI_EVENT_KEY_DOWN) {
            size_t next = cui_choice_navigate(items, count, from, events[e].key, true);
            if (next != count) {
                state->active = items[next].id;
                cui_focus(ui, state->active);
                state->search_length = 0;
            }
        } else if (events[e].type == CUI_EVENT_TEXT) {
            size_t len = 0;
            while (len < sizeof(events[e].text) && events[e].text[len])
                ++len;
            if (!len || len >= sizeof(events[e].text))
                continue;
            if (len > sizeof(state->search) - 1 - state->search_length)
                state->search_length = 0;
            if (len > sizeof(state->search) - 1)
                continue;
            bool cycle = len == 1 && state->search_length == 1 &&
                         cui_choice_lower((unsigned char)events[e].text[0]) ==
                             cui_choice_lower((unsigned char)state->search[0]);
            if (!cycle) {
                for (size_t i = 0; i < len; ++i)
                    state->search[state->search_length++] = events[e].text[i];
            }
            state->search_time = 0;
            for (size_t n = 0; n < count; ++n) {
                size_t i = (from + 1 + n) % count;
                if (!items[i].disabled &&
                    cui_choice_prefix(items[i].label, state->search, state->search_length)) {
                    state->active = items[i].id;
                    cui_focus(ui, state->active);
                    break;
                }
            }
        }
    }
    size_t begin = window_begin;
    if (begin >= count)
        begin = count - 1;
    size_t length = options.visible_count ? options.visible_count : count;
    if (length > count - begin)
        length = count - begin;
    if (options.visible_count) {
        for (size_t i = 0; i < count; ++i)
            if (items[i].id == state->active && !items[i].disabled) {
                if (i < begin)
                    begin = i;
                else if (i >= begin + length)
                    begin = i + 1 - length;
            }
    }
    state->first_visible = begin;
    bool pointer_moved = false, keyboard_used = false;
    for (size_t e = 0; e < event_count; ++e) {
        if (events[e].type == CUI_EVENT_POINTER_MOVE)
            pointer_moved = true;
        if (events[e].type == CUI_EVENT_KEY_DOWN || events[e].type == CUI_EVENT_KEY_UP)
            keyboard_used = true;
    }
    for (size_t i = begin; i < begin + length; ++i) {
        cui_interaction interaction = cui_interact(
            ui, items[i].id, options.menu ? CUI_ROLE_MENUITEM : CUI_ROLE_OPTION, items[i].label,
            options.disabled || items[i].disabled, *selected == items[i].id, (float)i);
        if (!options.disabled && !items[i].disabled && interaction.hovered && pointer_moved &&
            !keyboard_used)
            state->active = items[i].id;
    }
    size_t stop = count;
    for (size_t i = begin; i < begin + length; ++i)
        if (!items[i].disabled && items[i].id == state->active)
            stop = i;
    if (stop == count)
        for (size_t i = begin; i < begin + length; ++i)
            if (!items[i].disabled) {
                stop = i;
                state->active = items[i].id;
                break;
            }
    for (size_t i = begin; i < begin + length; ++i) {
        cui_update_semantic(ui, items[i].id, *selected == items[i].id, (float)i);
        cui_set_semantic_details(ui, items[i].id,
                                 (cui_semantic_details){.selected = *selected == items[i].id});
        cui_set_tab_stop(ui, items[i].id, i == stop);
    }
    return activated;
}

void cui_base_choice_popup_trigger(cui_context *ui, cui_id trigger, cui_str name, bool disabled,
                                   const cui_choice_item *items, size_t count, cui_id selected,
                                   cui_choice_popup *state) {
    if (!state || !state->overlay.id)
        return;
    cui_overlay_update(ui, &state->overlay);
    if (disabled && state->overlay.open)
        cui_overlay_close(ui, &state->overlay);
    bool activated =
        cui_interact(ui, trigger, CUI_ROLE_BUTTON, name, disabled, state->overlay.open, 0)
            .activated;
    bool arrow = false, reverse = false;
    size_t event_count = 0;
    const cui_event *events = cui_frame_events(ui, &event_count);
    for (size_t e = 0; e < event_count; e++)
        if (cui_event_target(ui, e) == trigger && events[e].type == CUI_EVENT_KEY_DOWN &&
            (events[e].key == CUI_KEY_DOWN || events[e].key == CUI_KEY_UP)) {
            arrow = true;
            reverse = events[e].key == CUI_KEY_UP;
        }
    if (!disabled && (activated || arrow)) {
        if (state->overlay.open && activated)
            cui_overlay_close(ui, &state->overlay);
        else if (!state->overlay.open) {
            cui_overlay_open(ui, &state->overlay, trigger);
            state->list.search_length = 0;
            state->list.search_time = 0;
            state->list.active = 0;
            if (items) {
                for (size_t i = 0; i < count; i++)
                    if (!items[i].disabled && items[i].id == selected)
                        state->list.active = selected;
                if (!state->list.active)
                    for (size_t n = 0; n < count; n++) {
                        size_t i = reverse ? count - 1 - n : n;
                        if (!items[i].disabled) {
                            state->list.active = items[i].id;
                            break;
                        }
                    }
            }
            cui_focus(ui, state->list.active);
        }
    }
    cui_update_semantic(ui, trigger, state->overlay.open, 0);
    cui_set_semantic_details(
        ui, trigger,
        (cui_semantic_details){.expanded = state->overlay.open, .controls = state->overlay.id});
}

static bool cui_choice_contains(cui_str label, cui_str query) {
    if (!query.length)
        return true;
    if (!label.data || !query.data || query.length > label.length)
        return false;
    for (size_t start = 0; start <= label.length - query.length; start++) {
        bool match = true;
        for (size_t j = 0; j < query.length; j++) {
            unsigned char a = (unsigned char)label.data[start + j],
                          b = (unsigned char)query.data[j];
            if (a >= 'A' && a <= 'Z')
                a = (unsigned char)(a + 'a' - 'A');
            if (b >= 'A' && b <= 'Z')
                b = (unsigned char)(b + 'a' - 'A');
            if (a != b) {
                match = false;
                break;
            }
        }
        if (match)
            return true;
    }
    return false;
}
static cui_combobox_result
cui_choice_reduce_combobox(cui_context *ui, cui_id input, cui_str name, cui_edit *query,
                           const cui_choice_item *items, size_t count, cui_choice_item *filtered,
                           size_t capacity, cui_id *selected, cui_choice_popup *state,
                           bool disabled, bool keep_open, cui_choice_activation_fn commit_fn,
                           void *commit_user) {
    cui_combobox_result result = {.error = CUI_EDIT_OK};
    if (!query || !selected || !state || !state->overlay.id || (!items && count) ||
        (!filtered && capacity) || capacity < count || cui_edit_get_state(query).multiline) {
        result.error = capacity < count ? CUI_EDIT_CAPACITY : CUI_EDIT_INVALID_ARGUMENT;
        return result;
    }
    state->overlay.modal = false;
    cui_overlay_update(ui, &state->overlay);
    if (disabled)
        cui_overlay_close(ui, &state->overlay);
    cui_input_hit_geometry hit = {ui, state->query_viewport, state->query_font_size,
                                  state->query_scroll_x};
    cui_input_result input_result = cui_base_input_ex(
        ui, input, name, query, &state->query_behavior,
        (cui_base_input_options){.disabled = disabled,
                                 .hit_test =
                                     state->query_font_size > 0 ? cui_base_input_hit_test : NULL,
                                 .hit_test_user = &hit});
    result.query_changed = input_result.changed;
    result.error = input_result.error;
    cui_str text = cui_edit_get_text(query);
    size_t matched = 0;
    for (size_t i = 0; i < count; i++)
        if (cui_choice_contains(items[i].label, text))
            filtered[matched++] = items[i];
    result.matching_count = matched;
    size_t event_count = 0;
    const cui_event *events = cui_frame_events(ui, &event_count);
    bool dismiss = false;
    if (!disabled)
        for (size_t e = 0; e < event_count; e++) {
            if (cui_event_target(ui, e) != input)
                continue;
            if (events[e].type == CUI_EVENT_KEY_DOWN && events[e].key == CUI_KEY_ESCAPE)
                dismiss = true;
            if ((events[e].type == CUI_EVENT_POINTER_DOWN &&
                 events[e].pointer_button == CUI_POINTER_PRIMARY) ||
                events[e].type == CUI_EVENT_TEXT ||
                (events[e].type == CUI_EVENT_KEY_DOWN &&
                 (events[e].key == CUI_KEY_DOWN || events[e].key == CUI_KEY_UP))) {
                if (!state->overlay.open)
                    cui_overlay_open(ui, &state->overlay, input);
            }
        }
    if (dismiss)
        cui_overlay_close(ui, &state->overlay);
    cui_set_semantic_details(ui, input,
                             (cui_semantic_details){.expanded = state->overlay.open,
                                                    .controls = state->overlay.id,
                                                    .description = text});
    if (!state->overlay.open)
        return result;
    size_t active = matched;
    for (size_t i = 0; i < matched; i++)
        if (filtered[i].id == state->list.active && !filtered[i].disabled)
            active = i;
    if (active == matched)
        for (size_t i = 0; i < matched; i++)
            if (!filtered[i].disabled) {
                active = i;
                break;
            }
    if (active != matched)
        state->list.active = filtered[active].id;
    bool commit = false;
    for (size_t e = 0; e < event_count && !disabled; e++) {
        if (cui_choice_scroll_event(ui, filtered, matched, &state->list, state->visible_rows,
                                    &state->list.first_visible, state->overlay.bounds,
                                    state->overlay.id,
                                    state->overlay.id ^ UINT64_C(0xd1bc905ee4216f11),
                                    cui_event_target(ui, e), events[e])) {
            result.scrolled = true;
            active = matched;
            for (size_t i = 0; i < matched; i++)
                if (filtered[i].id == state->list.active && !filtered[i].disabled)
                    active = i;
        }
        if (cui_event_target(ui, e) != input || events[e].type != CUI_EVENT_KEY_DOWN)
            continue;
        size_t next = active == matched
                          ? matched
                          : cui_choice_navigate(filtered, matched, active, events[e].key, true);
        if (next != matched) {
            active = next;
            state->list.active = filtered[active].id;
        }
        if (events[e].key == CUI_KEY_ENTER && !events[e].repeat && active != matched) {
            cui_id old = *selected;
            *selected = filtered[active].id;
            state->list.active = *selected;
            if (commit_fn)
                commit_fn(*selected, commit_user);
            result.changed |= keep_open || old != *selected;
            commit = true;
            if (!keep_open)
                break;
        }
    }
    if (commit && !keep_open) {
        cui_overlay_close(ui, &state->overlay);
        cui_set_semantic_details(
            ui, input, (cui_semantic_details){.controls = state->overlay.id, .description = text});
    }
    return result;
}

typedef struct {
    const cui_choice_item *items;
    size_t count;
    bool *selected;
    cui_choice_activation_fn external;
    void *external_user;
} cui_choice_multi_reducer;
static void cui_choice_multi_activate(cui_id item, void *user) {
    cui_choice_multi_reducer *reducer = user;
    for (size_t i = 0; i < reducer->count; i++)
        if (reducer->items[i].id == item) {
            reducer->selected[i] = !reducer->selected[i];
            break;
        }
    if (reducer->external)
        reducer->external(item, reducer->external_user);
}
bool cui_base_list_multi(cui_context *ui, const cui_choice_item *items, size_t count,
                         bool *selected, cui_list_state *state, cui_list_options options) {
    if (!items || !state || !selected || !count)
        return false;
    cui_choice_multi_reducer reducer = {items, count, selected, options.on_activate,
                                        options.activation_user};
    options.on_activate = cui_choice_multi_activate;
    options.activation_user = &reducer;
    cui_id action = 0;
    bool activated = cui_base_list(ui, items, count, &action, state, options);
    for (size_t i = 0; i < count; i++) {
        cui_update_semantic(ui, items[i].id, selected[i], (float)i);
        cui_set_semantic_details(ui, items[i].id, (cui_semantic_details){.selected = selected[i]});
    }
    return activated;
}

bool cui_base_selection_remove(cui_context *ui, cui_id chip, cui_str name, bool *selected,
                               bool disabled) {
    if (!selected)
        return false;
    cui_interact(ui, chip, CUI_ROLE_BUTTON, name, disabled, false, 0);
    cui_set_semantic_details(
        ui, chip, (cui_semantic_details){.description = cui_string("Remove selected option")});
    size_t event_count = 0;
    cui_frame_events(ui, &event_count);
    if (!disabled && *selected)
        for (size_t e = 0; e < event_count; e++)
            if (cui_event_activation(ui, e) == chip) {
                *selected = false;
                return true;
            }
    return false;
}

cui_combobox_result cui_base_combobox(cui_context *ui, cui_id input, cui_str name, cui_edit *query,
                                      const cui_choice_item *items, size_t count,
                                      cui_choice_item *filtered, size_t capacity, cui_id *selected,
                                      cui_choice_popup *state, bool disabled) {
    return cui_choice_reduce_combobox(ui, input, name, query, items, count, filtered, capacity,
                                      selected, state, disabled, false, NULL, NULL);
}
cui_combobox_result cui_base_combobox_multi(cui_context *ui, cui_id input, cui_str name,
                                            cui_edit *query, const cui_choice_item *items,
                                            size_t count, cui_choice_item *filtered,
                                            bool *filtered_selected, size_t capacity,
                                            bool *selected, cui_choice_popup *state,
                                            bool disabled) {
    if (!selected || (!filtered_selected && capacity))
        return (cui_combobox_result){.error = CUI_EDIT_INVALID_ARGUMENT};
    cui_choice_multi_reducer reducer = {items, count, selected, NULL, NULL};
    cui_id action = 0;
    cui_combobox_result result = cui_choice_reduce_combobox(
        ui, input, name, query, items, count, filtered, capacity, &action, state, disabled, true,
        cui_choice_multi_activate, &reducer);
    if (result.error != CUI_EDIT_OK)
        return result;
    size_t source = 0;
    for (size_t i = 0; i < result.matching_count; i++) {
        while (source < count && items[source].id != filtered[i].id)
            source++;
        filtered_selected[i] = source < count ? selected[source++] : false;
    }
    return result;
}

bool cui_menu_entry_is_disabled(const cui_menu_entry *entry) {
    if (!entry)
        return true;
    return entry->disabled || entry->kind == CUI_MENU_SEPARATOR || entry->kind == CUI_MENU_LABEL ||
           (entry->kind == CUI_MENU_CHECK && !entry->checked) ||
           (entry->kind == CUI_MENU_RADIO && !entry->radio_value) ||
           (entry->kind == CUI_MENU_SUBMENU && (!entry->child_count || !entry->children));
}
static size_t cui_menu_find(const cui_menu_entry *items, size_t count, cui_id id) {
    for (size_t i = 0; i < count; i++)
        if (items[i].id == id)
            return i;
    return count;
}
static size_t cui_menu_step(const cui_menu_entry *items, size_t count, size_t from, cui_key key) {
    if (!count)
        return count;
    int direction = 0;
    if (key == CUI_KEY_HOME) {
        from = count - 1;
        direction = 1;
    } else if (key == CUI_KEY_END) {
        from = 0;
        direction = -1;
    } else if (key == CUI_KEY_DOWN)
        direction = 1;
    else if (key == CUI_KEY_UP)
        direction = -1;
    if (!direction)
        return count;
    for (size_t n = 0; n < count; n++) {
        from = direction > 0 ? (from + 1) % count : from ? from - 1 : count - 1;
        if (!cui_menu_entry_is_disabled(&items[from]))
            return from;
    }
    return count;
}
void cui_menu_tree_init(cui_menu_tree_state *state, cui_id popup) {
    if (!state)
        return;
    *state = (cui_menu_tree_state){0};
    cui_overlay_init(&state->overlay, popup, true);
}
void cui_base_menu_entry(cui_context *ui, const cui_menu_entry *entry, bool disabled, bool expanded,
                         cui_id parent) {
    if (!entry)
        return;
    if (entry->kind == CUI_MENU_SEPARATOR)
        return;
    bool checked = entry->kind == CUI_MENU_CHECK && entry->checked ? *entry->checked
                   : entry->kind == CUI_MENU_RADIO && entry->radio_value
                       ? *entry->radio_value == entry->radio_option
                       : false;
    cui_role role = entry->kind == CUI_MENU_LABEL   ? CUI_ROLE_LABEL
                    : entry->kind == CUI_MENU_CHECK ? CUI_ROLE_MENUITEMCHECKBOX
                    : entry->kind == CUI_MENU_RADIO ? CUI_ROLE_MENUITEMRADIO
                                                    : CUI_ROLE_MENUITEM;
    cui_interact(ui, entry->id, role, entry->label, disabled || cui_menu_entry_is_disabled(entry),
                 checked,
                 entry->kind == CUI_MENU_RADIO ? (float)entry->radio_option
                 : checked                     ? 1
                                               : 0);
    cui_set_semantic_details(
        ui, entry->id,
        (cui_semantic_details){.parent = parent,
                               .expanded = expanded,
                               .controls = entry->kind == CUI_MENU_SUBMENU
                                               ? entry->id ^ UINT64_C(0x4b64d23e91a738c1)
                                               : 0,
                               .description = entry->shortcut});
}
bool cui_base_menu_tree(cui_context *ui, cui_id trigger, cui_str name, const cui_menu_entry *items,
                        size_t count, cui_id *action, cui_menu_tree_state *state, bool disabled,
                        float dt, bool keep_open_checks) {
    if (!state || !state->overlay.id || !action || (!items && count))
        return false;
    bool was_open = state->overlay.open;
    if (state->contextual) {
        cui_overlay_update(ui, &state->overlay);
        if (disabled)
            cui_overlay_close(ui, &state->overlay);
    } else {
        cui_choice_popup temporary = {.overlay = state->overlay, .list = state->levels[0]};
        cui_base_choice_popup_trigger(ui, trigger, name, disabled, NULL, 0, 0, &temporary);
        state->overlay = temporary.overlay;
    }
    if (!state->overlay.open) {
        state->depth = 0;
        return false;
    }
    if (!was_open || !state->depth) {
        state->depth = 1;
        state->depth_limit = false;
        size_t queued = 0;
        bool reverse = false;
        const cui_event *opening_events = cui_frame_events(ui, &queued);
        for (size_t e = 0; e < queued; e++)
            if (cui_event_target(ui, e) == trigger &&
                opening_events[e].type == CUI_EVENT_KEY_DOWN && opening_events[e].key == CUI_KEY_UP)
                reverse = true;
        size_t first = cui_menu_step(items, count,
                                     reverse ? 0
                                     : count ? count - 1
                                             : 0,
                                     reverse ? CUI_KEY_UP : CUI_KEY_DOWN);
        state->levels[0].active = first < count ? items[first].id : 0;
        state->levels[0].search_length = 0;
        cui_focus(ui, state->levels[0].active);
    }
    if (state->depth > CUI_MENU_DEPTH_MAX)
        state->depth = CUI_MENU_DEPTH_MAX;
    const cui_menu_entry *menus[CUI_MENU_DEPTH_MAX] = {items};
    size_t counts[CUI_MENU_DEPTH_MAX] = {count};
    for (size_t level = 1; level < state->depth; level++) {
        size_t parent = cui_menu_find(menus[level - 1], counts[level - 1], state->path[level - 1]);
        if (parent == counts[level - 1] || menus[level - 1][parent].kind != CUI_MENU_SUBMENU ||
            cui_menu_entry_is_disabled(&menus[level - 1][parent])) {
            state->depth = level;
            break;
        }
        state->levels[level - 1].active = menus[level - 1][parent].id;
        menus[level] = menus[level - 1][parent].children;
        counts[level] = menus[level - 1][parent].child_count;
    }
    for (size_t level = 0; level < state->depth; level++) {
        cui_list_state *cursor = &state->levels[level];
        if (cursor->search_length >= sizeof(cursor->search))
            cursor->search_length = 0;
        if (dt > 0 && dt < 60)
            cursor->search_time += dt;
        if (cursor->search_time > .75f)
            cursor->search_length = 0;
        size_t current = cui_menu_find(menus[level], counts[level], cursor->active);
        if (current == counts[level] || cui_menu_entry_is_disabled(&menus[level][current])) {
            size_t first = cui_menu_step(menus[level], counts[level],
                                         counts[level] ? counts[level] - 1 : 0, CUI_KEY_DOWN);
            cursor->active = first < counts[level] ? menus[level][first].id : 0;
        }
    }
    size_t event_count = 0, logical_level = state->depth - 1;
    bool redirected = false, activated_any = false;
    const cui_event *events = cui_frame_events(ui, &event_count);
    for (size_t e = 0; e < event_count; e++) {
        cui_id target = cui_event_target(ui, e);
        if (events[e].type == CUI_EVENT_KEY_DOWN && events[e].key == CUI_KEY_ESCAPE) {
            size_t escape_level = state->depth;
            if (target == state->overlay.id)
                escape_level = 0;
            for (size_t level = 1; level < state->depth; level++)
                if (target == (state->path[level - 1] ^ UINT64_C(0x4b64d23e91a738c1)))
                    escape_level = level;
            if (escape_level < state->depth) {
                if (!escape_level) {
                    cui_overlay_close(ui, &state->overlay);
                    state->depth = 0;
                    break;
                }
                state->depth = escape_level;
                logical_level = escape_level - 1;
                cui_focus(ui, state->path[logical_level]);
                redirected = true;
                continue;
            }
        }
        if (events[e].type == CUI_EVENT_POINTER_MOVE && state->row_height > 0) {
            for (size_t reverse = state->depth; reverse > 0; reverse--) {
                size_t level = reverse - 1;
                cui_rect panel = state->panels[level];
                if (events[e].x >= panel.x && events[e].x < panel.x + panel.w &&
                    events[e].y >= panel.y + state->padding &&
                    events[e].y < panel.y + panel.h - state->padding) {
                    float offset = (events[e].y - panel.y - state->padding) / state->row_height;
                    if (offset >= (float)counts[level])
                        break;
                    size_t row = state->levels[level].first_visible + (size_t)offset;
                    if (row < counts[level] && !cui_menu_entry_is_disabled(&menus[level][row]))
                        target = menus[level][row].id;
                    break;
                }
            }
        }
        size_t target_level = state->depth, target_index = 0;
        for (size_t level = 0; level < state->depth; level++) {
            size_t index = cui_menu_find(menus[level], counts[level], target);
            if (index < counts[level]) {
                target_level = level;
                target_index = index;
                break;
            }
        }
        if (target_level == state->depth)
            continue;
        bool keyboard = events[e].type == CUI_EVENT_KEY_DOWN ||
                        events[e].type == CUI_EVENT_KEY_UP || events[e].type == CUI_EVENT_TEXT;
        size_t level = keyboard && redirected ? logical_level : target_level;
        if (level >= state->depth)
            continue;
        const cui_menu_entry *entries = menus[level];
        size_t n = counts[level];
        size_t active = cui_menu_find(entries, n, state->levels[level].active);
        if (!keyboard)
            active = target_index;
        if (active == n || cui_menu_entry_is_disabled(&entries[active]))
            continue;
        const cui_menu_entry *entry = &entries[active];
        if (events[e].type == CUI_EVENT_POINTER_MOVE) {
            state->levels[level].active = entry->id;
            cui_focus(ui, entry->id);
            state->depth = level + 1;
            logical_level = level;
            redirected = true;
        }
        bool activation = cui_event_activation(ui, e) != 0;
        if (events[e].type == CUI_EVENT_KEY_UP && entry->id != cui_event_activation(ui, e))
            activation = false;
        bool open_child =
            entry->kind == CUI_MENU_SUBMENU &&
            (activation || events[e].type == CUI_EVENT_POINTER_MOVE ||
             (events[e].type == CUI_EVENT_KEY_DOWN && events[e].key == CUI_KEY_RIGHT));
        if (open_child) {
            if (level + 1 == CUI_MENU_DEPTH_MAX) {
                state->depth_limit = true;
                continue;
            }
            state->path[level] = entry->id;
            state->depth = level + 2;
            menus[level + 1] = entry->children;
            counts[level + 1] = entry->child_count;
            size_t first =
                cui_menu_step(entry->children, entry->child_count,
                              entry->child_count ? entry->child_count - 1 : 0, CUI_KEY_DOWN);
            state->levels[level + 1] = (cui_list_state){
                .active = first < entry->child_count ? entry->children[first].id : 0};
            cui_focus(ui, state->levels[level + 1].active);
            logical_level = level + 1;
            redirected = true;
            continue;
        }
        if (activation && entry->kind != CUI_MENU_SUBMENU) {
            state->levels[level].active = entry->id;
            if (!keyboard)
                state->depth = level + 1;
            if (entry->kind == CUI_MENU_CHECK)
                *entry->checked = !*entry->checked;
            if (entry->kind == CUI_MENU_RADIO)
                *entry->radio_value = entry->radio_option;
            *action = entry->id;
            if (!keep_open_checks ||
                (entry->kind != CUI_MENU_CHECK && entry->kind != CUI_MENU_RADIO)) {
                cui_overlay_close(ui, &state->overlay);
                state->depth = 0;
            }
            activated_any = true;
            if (!state->overlay.open)
                return true;
            continue;
        }
        if (events[e].type == CUI_EVENT_KEY_DOWN) {
            if (events[e].key == CUI_KEY_LEFT && level) {
                state->depth = level;
                logical_level = level - 1;
                redirected = true;
                cui_focus(ui, state->path[level - 1]);
                continue;
            }
            size_t next = cui_menu_step(entries, n, active, events[e].key);
            if (next < n) {
                state->levels[level].active = entries[next].id;
                state->depth = level + 1;
                logical_level = level;
                redirected = true;
                cui_focus(ui, entries[next].id);
                state->levels[level].search_length = 0;
            }
        } else if (events[e].type == CUI_EVENT_TEXT) {
            cui_list_state *cursor = &state->levels[level];
            size_t length = 0;
            while (length < sizeof(events[e].text) && events[e].text[length])
                length++;
            if (!length || length >= sizeof(events[e].text))
                continue;
            if (length > sizeof(cursor->search) - 1 - cursor->search_length)
                cursor->search_length = 0;
            bool cycle = length == 1 && cursor->search_length == 1 &&
                         cui_choice_lower((unsigned char)events[e].text[0]) ==
                             cui_choice_lower((unsigned char)cursor->search[0]);
            if (!cycle)
                for (size_t j = 0; j < length; j++)
                    cursor->search[cursor->search_length++] = events[e].text[j];
            cursor->search_time = 0;
            for (size_t step = 0; step < n; step++) {
                size_t i = (active + 1 + step) % n;
                if (!cui_menu_entry_is_disabled(&entries[i]) &&
                    cui_choice_prefix(entries[i].label, cursor->search, cursor->search_length)) {
                    cursor->active = entries[i].id;
                    cui_focus(ui, cursor->active);
                    state->depth = level + 1;
                    logical_level = level;
                    redirected = true;
                    break;
                }
            }
        }
    }
    return activated_any;
}

void cui_context_menu_open(cui_context *ui, cui_menu_tree_state *state, cui_id restore_focus,
                           cui_rect anchor) {
    if (!ui || !state || !state->overlay.id || !isfinite(anchor.x) || !isfinite(anchor.y) ||
        !isfinite(anchor.w) || !isfinite(anchor.h) || anchor.w < 0 || anchor.h < 0)
        return;
    state->contextual = true;
    state->anchor = anchor;
    state->depth = 0;
    cui_overlay_open(ui, &state->overlay, restore_focus);
    if (restore_focus)
        state->overlay.trigger = restore_focus;
    state->overlay.has_bounds = false;
    cui_invalidate(ui);
}
bool cui_base_context_menu(cui_context *ui, cui_id target, const cui_menu_entry *items,
                           size_t count, cui_id *action, cui_menu_tree_state *state, bool disabled,
                           float dt, bool keep_open_checks) {
    if (!state || !state->overlay.id || !target)
        return false;
    state->contextual = true;
    cui_rect area = cui_bounds(ui, target);
    size_t event_count = 0;
    const cui_event *events = cui_frame_events(ui, &event_count);
    if (!disabled && cui_pointer_scope_allowed(ui, state->overlay.id))
        for (size_t e = 0; e < event_count; e++) {
            cui_event event = events[e];
            if (event.type == CUI_EVENT_POINTER_DOWN &&
                event.pointer_button == CUI_POINTER_SECONDARY && event.x >= area.x &&
                event.x < area.x + area.w && event.y >= area.y && event.y < area.y + area.h)
                cui_context_menu_open(
                    ui, state, state->overlay.open ? state->overlay.trigger : cui_focused(ui),
                    (cui_rect){event.x, event.y, 0, 0});
        }
    return cui_base_menu_tree(ui, target, cui_string(""), items, count, action, state, disabled, dt,
                              keep_open_checks);
}
