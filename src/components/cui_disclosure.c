/* SPDX-License-Identifier: MIT */
#include "cui/cui_disclosure.h"
static void cui_disclosure_trigger(cui_context *ui, cui_id id, const cui_theme *theme,
                                   cui_str label, bool open, bool disabled, bool bordered) {
    cui_box box = {.height = theme->control_height,
                   .padding = theme->padding,
                   .gap = theme->gap,
                   .row = true,
                   .radius = theme->radius,
                   .background = theme->surface,
                   .border = theme->border,
                   .border_width = bordered ? 1 : 0};
    if (cui_focused(ui) == id) {
        box.border = theme->focus;
        box.border_width = 2;
    }
    cui_begin_box(ui, id, box);
    cui_text(ui, cui_string(open ? "-" : "+"), theme->font_size,
             disabled ? theme->muted : theme->primary);
    cui_text(ui, label, theme->font_size, disabled ? theme->muted : theme->text);
    cui_end_box(ui);
}
bool cui_collapsible(cui_context *ui, cui_id id, const cui_theme *theme, cui_str label, bool *open,
                     bool disabled, cui_disclosure_panel_fn panel, void *user) {
    if (!theme || !open)
        return false;
    bool changed = cui_base_collapsible(ui, id, label, open, disabled);
    cui_begin_box(ui, id ^ UINT64_C(0xb7c1c9d453922783), (cui_box){.gap = theme->gap});
    cui_disclosure_trigger(ui, id, theme, label, *open, disabled, true);
    if (*open && panel) {
        cui_begin_box(ui, id ^ UINT64_C(0x1d8f322539deca61),
                      (cui_box){.padding = theme->padding, .gap = theme->gap});
        panel(ui, 0, user);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    return changed;
}
bool cui_accordion(cui_context *ui, cui_id container, const cui_theme *theme,
                   const cui_disclosure_item *items, size_t count, bool *open,
                   cui_accordion_options options, cui_disclosure_panel_fn panel, void *user) {
    if (!theme || !items || !open)
        return false;
    bool changed = cui_base_accordion(ui, items, count, open, options);
    cui_begin_box(ui, container, (cui_box){.gap = theme->gap});
    for (size_t i = 0; i < count; ++i) {
        cui_disclosure_trigger(ui, items[i].id, theme, items[i].label, open[i],
                               options.disabled || items[i].disabled, options.bordered);
        if (open[i] && panel) {
            cui_begin_box(ui, items[i].id ^ UINT64_C(0x1d8f322539deca61),
                          (cui_box){.padding = theme->padding, .gap = theme->gap});
            panel(ui, i, user);
            cui_end_box(ui);
        }
    }
    cui_end_box(ui);
    return changed;
}
