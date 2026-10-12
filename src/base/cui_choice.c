/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
#include "cui/cui_base.h"
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
    for (size_t e = 0; e < event_count && !options.disabled; ++e) {
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
        if (cui_event_activation(ui, e) &&
            (events[e].type != CUI_EVENT_KEY_UP || items[from].id == cui_event_activation(ui, e))) {
            size_t chosen = events[e].type == CUI_EVENT_POINTER_UP ? target : from;
            *selected = state->active = items[chosen].id;
            activated = true;
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
    size_t begin = options.visible_count ? options.visible_begin : 0;
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
cui_combobox_result cui_base_combobox(cui_context *ui, cui_id input, cui_str name, cui_edit *query,
                                      const cui_choice_item *items, size_t count,
                                      cui_choice_item *filtered, size_t capacity, cui_id *selected,
                                      cui_choice_popup *state, bool disabled) {
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
    cui_input_result input_result = cui_base_input(ui, input, name, query, disabled);
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
            if (events[e].type == CUI_EVENT_POINTER_DOWN || events[e].type == CUI_EVENT_TEXT ||
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
        if (cui_event_target(ui, e) != input || events[e].type != CUI_EVENT_KEY_DOWN)
            continue;
        size_t next = active == matched
                          ? matched
                          : cui_choice_navigate(filtered, matched, active, events[e].key, true);
        if (next != matched) {
            active = next;
            state->list.active = filtered[active].id;
        }
        if (events[e].key == CUI_KEY_ENTER && !events[e].repeat && active != matched)
            commit = true;
    }
    if (commit) {
        cui_id old = *selected;
        *selected = state->list.active;
        cui_overlay_close(ui, &state->overlay);
        cui_set_semantic_details(
            ui, input, (cui_semantic_details){.controls = state->overlay.id, .description = text});
        result.changed = old != *selected;
        return result;
    }
    return result;
}
