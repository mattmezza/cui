/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
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
        cui_interaction interaction =
            cui_interact(ui, items[i].id, CUI_ROLE_TAB, items[i].label,
                         options.disabled || items[i].disabled, *selected == items[i].id, (float)i);
        if (interaction.activated && !options.disabled && !items[i].disabled)
            *selected = items[i].id;
    }
    size_t event_count = 0, cursor = count;
    const cui_event *events = cui_frame_events(ui, &event_count);
    for (size_t e = 0; e < event_count && !options.disabled; ++e) {
        if (events[e].type != CUI_EVENT_KEY_DOWN)
            continue;
        size_t from = count;
        for (size_t i = 0; i < count; ++i)
            if (items[i].id == cui_event_target(ui, e))
                from = i;
        if (from == count || items[from].disabled)
            continue;
        if (cursor == count)
            cursor = from;
        size_t next = cui_choice_navigate(items, count, cursor, events[e].key, options.vertical);
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
        cui_interaction interaction =
            cui_interact(ui, items[i].id, CUI_ROLE_BUTTON, items[i].label,
                         options.disabled || items[i].disabled, selected[i], selected[i]);
        if (!interaction.activated || options.disabled || items[i].disabled)
            continue;
        bool next = !selected[i];
        if (!next && !options.allow_empty) {
            size_t active = 0;
            for (size_t j = 0; j < count; ++j)
                active += selected[j];
            if (active == 1)
                continue;
        }
        if (next && !options.multiple)
            for (size_t j = 0; j < count; ++j)
                selected[j] = false;
        selected[i] = next;
        changed = true;
    }
    size_t event_count = 0, cursor = count;
    const cui_event *events = cui_frame_events(ui, &event_count);
    for (size_t e = 0; e < event_count && !options.disabled; ++e) {
        if (events[e].type != CUI_EVENT_KEY_DOWN)
            continue;
        size_t from = count;
        for (size_t i = 0; i < count; ++i)
            if (items[i].id == cui_event_target(ui, e))
                from = i;
        if (from == count || items[from].disabled)
            continue;
        if (cursor == count)
            cursor = from;
        size_t next = cui_choice_navigate(items, count, cursor, events[e].key, options.vertical);
        if (next != count) {
            cursor = next;
            cui_focus(ui, items[next].id);
        }
    }
    for (size_t i = 0; i < count; ++i)
        cui_update_semantic(ui, items[i].id, selected[i], selected[i]);
    return changed;
}
