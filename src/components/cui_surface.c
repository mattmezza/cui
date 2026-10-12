/* SPDX-License-Identifier: MIT */
#include "cui/cui_surface.h"
#include "cui/cui_components.h"
bool cui_surface_begin(cui_context *ui, const cui_theme *t, cui_surface_state *s, cui_str title,
                       cui_str description, cui_surface_options o) {
    cui_rect bounds = s ? cui_surface_geometry(s->kind, o) : (cui_rect){0};
    if (!t || bounds.w <= 0 || bounds.h <= 0 ||
        !cui_base_surface_begin(ui, s, title, description, o))
        return false;
    bounds = cui_surface_current_geometry(s, o);
    int16_t layer = o.layer > 0 && o.layer < 32766 ? o.layer : 100;
    cui_rect handle = cui_surface_resize_bounds(s, o);
    if (handle.w > 0 && handle.h > 0) {
        cui_id resize = cui_surface_part_id(s->overlay.id, 5);
        cui_begin_portal(ui, resize,
                         (cui_box){.background = cui_focused(ui) == resize ? t->focus : t->border},
                         handle, (int16_t)(layer + 2));
        cui_base_surface_submit_resize(ui, s, o);
        cui_end_box(ui);
    }
    if (s->overlay.modal) {
        cui_begin_portal(ui, s->overlay.id ^ UINT64_C(0x86a3e3d76bc1d909),
                         (cui_box){.background = {0, 0, 0, 100}}, o.viewport, layer);
        cui_end_box(ui);
    }
    cui_begin_portal(ui, s->overlay.id,
                     (cui_box){.padding = t->padding,
                               .gap = t->gap,
                               .radius = t->radius,
                               .background = t->surface,
                               .border = t->border,
                               .border_width = 1},
                     bounds, (int16_t)(layer + 1));
    cui_begin_box(ui, 0, (cui_box){.width = -1, .row = true, .gap = t->gap});
    cui_begin_box(ui, cui_surface_part_id(s->overlay.id, 4), (cui_box){.width = -1});
    cui_text(ui, title, t->font_size * 1.1f, t->text);
    cui_end_box(ui);
    if (!o.hide_close && (s->kind != CUI_SURFACE_ALERT || o.show_alert_close))
        cui_button_ex(ui, cui_surface_part_id(s->overlay.id, 1), t, cui_string("Close"),
                      (cui_button_options){.variant = CUI_BUTTON_GHOST, .size = CUI_CONTROL_SMALL});
    cui_end_box(ui);
    if (description.length)
        cui_text(ui, description, t->font_size, t->muted);
    cui_begin_box(ui, 0, (cui_box){.width = -1, .height = -1, .gap = t->gap, .clip = true});
    cui_overlay_set_bounds(&s->overlay, bounds);
    return true;
}
void cui_surface_footer_begin(cui_context *ui, const cui_theme *t) {
    if (!ui || !t)
        return;
    cui_end_box(ui);
    cui_begin_box(ui, 0,
                  (cui_box){.width = -1, .row = true, .gap = t->gap, .align_x = CUI_ALIGN_END});
}
void cui_surface_end(cui_context *ui) {
    cui_end_box(ui);
    cui_end_box(ui);
    cui_base_surface_end(ui);
}
void cui_alert_dialog(cui_context *ui, const cui_theme *t, cui_surface_state *s, cui_str title,
                      cui_str description, cui_str confirm, cui_str cancel, cui_surface_options o) {
    if (!s || s->kind != CUI_SURFACE_ALERT)
        return;
    if (!cui_surface_begin(ui, t, s, title, description, o))
        return;
    cui_begin_box(ui, 0,
                  (cui_box){.width = -1, .row = true, .gap = t->gap, .align_x = CUI_ALIGN_END});
    cui_button_ex(ui, cui_surface_part_id(s->overlay.id, 3), t, cancel,
                  (cui_button_options){.variant = CUI_BUTTON_OUTLINE});
    cui_button_ex(ui, cui_surface_part_id(s->overlay.id, 2), t, confirm,
                  (cui_button_options){.variant = CUI_BUTTON_DANGER});
    cui_end_box(ui);
    cui_surface_end(ui);
}
