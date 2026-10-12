/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
static void cui_choice_draw(cui_context *ui, const cui_choice_item *item, const cui_theme *theme,
                            bool selected, bool disabled, cui_tab_variant variant) {
    cui_box box = {.height = theme->control_height,
                   .padding = theme->padding,
                   .radius = theme->radius,
                   .background = selected ? theme->primary : theme->surface,
                   .border = cui_focused(ui) == item->id ? theme->focus : theme->border,
                   .border_width = cui_focused(ui) == item->id ? 2 : 1};
    if (variant == CUI_TAB_UNDERLINE) {
        box.background = theme->background;
        box.radius = 0;
        box.padding = 0;
        box.border_width = 0;
    } else if (variant == CUI_TAB_PILL) {
        box.radius = theme->control_height * .5f;
        if (!selected)
            box.border_width = 0;
    } else if (variant == CUI_TAB_OUTLINE) {
        box.background = theme->background;
        box.border = selected ? theme->primary : theme->border;
    } else if (variant == CUI_TAB_SEGMENTED) {
        box.radius = 0;
    }
    if (cui_focused(ui) == item->id) {
        box.border = theme->focus;
        box.border_width = 2;
    }
    cui_begin_box(ui, item->id, box);
    if (variant == CUI_TAB_UNDERLINE)
        cui_begin_box(ui, 0,
                      (cui_box){.height = theme->control_height > 2 ? theme->control_height - 2 : 0,
                                .padding = theme->padding});
    cui_text(ui, item->label, theme->font_size,
             disabled ? theme->muted
             : selected && variant != CUI_TAB_UNDERLINE && variant != CUI_TAB_OUTLINE
                 ? theme->on_primary
                 : theme->text);
    if (variant == CUI_TAB_UNDERLINE) {
        cui_end_box(ui);
        cui_begin_box(ui, 0,
                      (cui_box){.width = -1,
                                .height = 2,
                                .background = selected ? theme->primary : theme->background});
        cui_end_box(ui);
    }
    cui_end_box(ui);
}
bool cui_tabs(cui_context *ui, cui_id container, const cui_theme *theme,
              const cui_choice_item *items, size_t count, cui_id *selected,
              cui_tabs_options options, cui_tab_panel_fn panel, void *user) {
    if (!theme || !items || !selected)
        return false;
    bool changed = cui_base_tabs(ui, items, count, selected, options);
    cui_begin_box(ui, container, (cui_box){.gap = theme->gap});
    cui_id list_id = container ^ UINT64_C(0xcda0312561f50cab);
    cui_interact(ui, list_id, CUI_ROLE_TABLIST, cui_string(""), options.disabled, false, 0);
    cui_begin_box(ui, container ^ UINT64_C(0xcda0312561f50cab),
                  (cui_box){.row = !options.vertical, .gap = theme->gap});
    size_t active = count;
    for (size_t i = 0; i < count; ++i) {
        cui_choice_draw(ui, &items[i], theme, items[i].id == *selected,
                        options.disabled || items[i].disabled, options.variant);
        cui_set_semantic_details(
            ui, items[i].id,
            (cui_semantic_details){.selected = items[i].id == *selected,
                                   .controls = items[i].id ^ UINT64_C(0x691a17db80e22f45),
                                   .parent = list_id});
        if (items[i].id == *selected && !items[i].disabled)
            active = i;
    }
    cui_end_box(ui);
    if (active != count && panel) {
        cui_id panel_id = items[active].id ^ UINT64_C(0x691a17db80e22f45);
        cui_interact(ui, panel_id, CUI_ROLE_TABPANEL, items[active].label, false, false, 0);
        cui_set_semantic_details(
            ui, panel_id,
            (cui_semantic_details){.labelled_by = items[active].id, .parent = container});
        cui_begin_box(ui, items[active].id ^ UINT64_C(0x691a17db80e22f45),
                      (cui_box){.padding = theme->padding, .gap = theme->gap});
        panel(ui, active, user);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    return changed;
}
bool cui_toggle_group(cui_context *ui, cui_id container, const cui_theme *theme,
                      const cui_choice_item *items, size_t count, bool *selected,
                      cui_toggle_group_options options) {
    if (!theme || !items || !selected)
        return false;
    bool changed = cui_base_toggle_group(ui, items, count, selected, options);
    cui_interact(ui, container, CUI_ROLE_GROUP, cui_string(""), options.disabled, false, 0);
    cui_begin_box(ui, container, (cui_box){.row = !options.vertical, .gap = theme->gap});
    for (size_t i = 0; i < count; ++i)
        cui_choice_draw(ui, &items[i], theme, selected[i], options.disabled || items[i].disabled,
                        CUI_TAB_DEFAULT);
    cui_end_box(ui);
    return changed;
}
