/* SPDX-License-Identifier: MIT */
#include "cui/cui_disclosure.h"
bool cui_base_collapsible(cui_context *ui, cui_id id, cui_str label, bool *open, bool disabled) {
    if (!open)
        return false;
    cui_interaction interaction =
        cui_interact(ui, id, CUI_ROLE_BUTTON, label, disabled, *open, *open);
    cui_set_semantic_details(
        ui, id,
        (cui_semantic_details){.expanded = *open, .controls = id ^ UINT64_C(0x1d8f322539deca61)});
    if (disabled || !interaction.activated)
        return false;
    *open = !*open;
    cui_update_semantic(ui, id, *open, *open);
    cui_set_semantic_details(
        ui, id,
        (cui_semantic_details){.expanded = *open, .controls = id ^ UINT64_C(0x1d8f322539deca61)});
    return true;
}
bool cui_base_accordion(cui_context *ui, const cui_disclosure_item *items, size_t count, bool *open,
                        cui_accordion_options options) {
    if (!items || !open || !count)
        return false;
    bool changed = false;
    for (size_t i = 0; i < count; ++i) {
        cui_interaction interaction =
            cui_interact(ui, items[i].id, CUI_ROLE_BUTTON, items[i].label,
                         options.disabled || items[i].disabled, open[i], open[i]);
        if (interaction.activated && !options.disabled && !items[i].disabled &&
            (!open[i] || options.collapsible)) {
            bool next = !open[i];
            if (next && !options.multiple) {
                for (size_t j = 0; j < count; ++j) {
                    changed |= open[j];
                    open[j] = false;
                }
            }
            open[i] = next;
            changed = true;
        }
    }
    size_t event_count = 0;
    const cui_event *events = cui_frame_events(ui, &event_count);
    size_t cursor = count;
    for (size_t e = 0; e < event_count && !options.disabled; ++e) {
        if (events[e].type != CUI_EVENT_KEY_DOWN)
            continue;
        size_t from = count;
        cui_id target = cui_event_target(ui, e);
        for (size_t i = 0; i < count; ++i)
            if (items[i].id == target)
                from = i;
        if (from == count || items[from].disabled)
            continue;
        if (cursor == count)
            cursor = from;
        cui_key key = events[e].key;
        int direction = key == CUI_KEY_DOWN || key == CUI_KEY_HOME ? 1 : -1;
        if (key != CUI_KEY_UP && key != CUI_KEY_DOWN && key != CUI_KEY_HOME && key != CUI_KEY_END)
            continue;
        size_t next = key == CUI_KEY_HOME ? count - 1 : key == CUI_KEY_END ? 0 : cursor;
        for (size_t n = 0; n < count; ++n) {
            next = direction > 0 ? (next + 1) % count : (next ? next - 1 : count - 1);
            if (!items[next].disabled) {
                cursor = next;
                cui_focus(ui, items[next].id);
                break;
            }
        }
    }
    for (size_t i = 0; i < count; ++i) {
        cui_update_semantic(ui, items[i].id, open[i], open[i]);
        cui_set_semantic_details(
            ui, items[i].id,
            (cui_semantic_details){.expanded = open[i],
                                   .controls = items[i].id ^ UINT64_C(0x1d8f322539deca61)});
    }
    return changed;
}
