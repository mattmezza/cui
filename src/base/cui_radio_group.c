/* SPDX-License-Identifier: MIT */
#include "cui/cui_radio_group.h"
#include <math.h>
static size_t cui_radio_group_find(const cui_radio_group_item *items, size_t count, cui_id id) {
    for (size_t i = 0; i < count; i++)
        if (items[i].id == id)
            return i;
    return count;
}
static size_t cui_radio_group_enabled(const cui_radio_group_item *items, size_t count, size_t from,
                                      int direction) {
    for (size_t n = 0; n < count; n++) {
        from = direction > 0 ? (from + 1) % count : (from + count - 1) % count;
        if (!items[from].disabled)
            return from;
    }
    return count;
}
cui_radio_group_result cui_base_radio_group(cui_context *ui, cui_id id,
                                            const cui_radio_group_item *items, size_t count,
                                            cui_id *selected, cui_radio_group_state *state,
                                            cui_radio_group_options o) {
    cui_radio_group_result r = {0};
    if (!ui || !id || !selected || !state || count > CUI_RADIO_GROUP_MAX || (count && !items) ||
        !isfinite(o.gap) || o.gap < 0 || o.size > CUI_CONTROL_XSMALL ||
        o.size < CUI_CONTROL_MEDIUM || o.design_size < CUI_DESIGN_INHERIT ||
        o.design_size > CUI_DESIGN_9XL || (o.name.length && !o.name.data))
        return r;
    for (size_t i = 0; i < count; i++) {
        if (!items[i].id || items[i].id == id || (items[i].label.length && !items[i].label.data))
            return r;
        for (size_t j = 0; j < i; j++)
            if (items[i].id == items[j].id)
                return r;
    }
    r.valid = true;
    size_t first = count;
    for (size_t i = 0; i < count; i++)
        if (!items[i].disabled) {
            first = i;
            break;
        }
    cui_id focused = cui_focused(ui);
    size_t active = cui_radio_group_find(items, count, focused),
           chosen = cui_radio_group_find(items, count, *selected);
    size_t tab = chosen < count && !items[chosen].disabled ? chosen : first;
    if (!o.disabled && first < count &&
        (active < count || (state->had_focus && (!focused || focused == state->focused)))) {
        if (active == count || items[active].disabled) {
            active = tab;
            cui_focus(ui, items[active].id);
        }
        tab = active;
    } else if ((o.disabled && active < count) ||
               (first == count && state->had_focus && focused == state->focused))
        cui_focus(ui, 0);
    size_t event_count;
    const cui_event *events = cui_frame_events(ui, &event_count);
    for (size_t e = 0; e < event_count; e++) {
        if (cui_event_consumed(ui, e))
            continue;
        size_t target = cui_radio_group_find(items, count, cui_event_target(ui, e));
        size_t activate = cui_radio_group_find(items, count, cui_event_activation(ui, e));
        size_t request = count;
        if (activate < count) {
            cui_cancel_event_activation(ui, e);
            cui_consume_event(ui, e);
            if (!o.disabled && !items[activate].disabled)
                request = activate;
        } else if (!o.disabled && target < count && !items[target].disabled &&
                   events[e].type == CUI_EVENT_KEY_DOWN) {
            size_t current = cui_radio_group_find(items, count, cui_focused(ui));
            if (current < count && !items[current].disabled)
                target = current;
            switch (events[e].key) {
            case CUI_KEY_RIGHT:
            case CUI_KEY_DOWN:
                request = cui_radio_group_enabled(items, count, target, 1);
                break;
            case CUI_KEY_LEFT:
            case CUI_KEY_UP:
                request = cui_radio_group_enabled(items, count, target, -1);
                break;
            case CUI_KEY_HOME:
                request = first;
                break;
            case CUI_KEY_END:
                for (size_t i = count; i > 0; i--)
                    if (!items[i - 1].disabled) {
                        request = i - 1;
                        break;
                    }
                break;
            default:
                break;
            }
            if (request < count)
                cui_consume_event(ui, e);
        }
        if (request < count) {
            cui_focus(ui, items[request].id);
            tab = request;
            if (*selected != items[request].id) {
                cui_id before = *selected;
                r.requested = true;
                if (o.on_change)
                    o.on_change(o.change_user, items[request].id);
                else
                    *selected = items[request].id;
                r.changed |= before != *selected;
            }
        }
    }
    if (!o.disabled && !o.skip_tab && tab < count)
        r.tab_stop = items[tab].id;
    state->focused = cui_focused(ui);
    state->had_focus = cui_radio_group_find(items, count, state->focused) < count;
    cui_interact(ui, id, CUI_ROLE_RADIOGROUP, o.name, o.disabled, false, 0);
    cui_set_tab_stop(ui, id, false);
    cui_set_semantic_details(ui, id,
                             (cui_semantic_details){.orientation = o.vertical
                                                                       ? CUI_ORIENTATION_VERTICAL
                                                                       : CUI_ORIENTATION_HORIZONTAL,
                                                    .required = o.required,
                                                    .invalid = o.invalid});
    for (size_t i = 0; i < count; i++) {
        cui_interact(ui, items[i].id, CUI_ROLE_RADIO, items[i].label,
                     o.disabled || items[i].disabled, *selected == items[i].id, (float)i);
        cui_set_tab_stop(ui, items[i].id, items[i].id == r.tab_stop);
        cui_set_semantic_details(ui, items[i].id,
                                 (cui_semantic_details){.parent = id,
                                                        .pos_in_set = (unsigned)i + 1,
                                                        .set_size = (unsigned)count,
                                                        .required = o.required,
                                                        .invalid = o.invalid});
    }
    if (r.changed)
        cui_invalidate(ui);
    return r;
}
