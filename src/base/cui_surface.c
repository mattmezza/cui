/* SPDX-License-Identifier: MIT */
#include "cui/cui_surface.h"
#include <math.h>
static float cui_surface_positive(float value, float fallback) {
    return isfinite(value) && value > 0 ? value : fallback;
}
void cui_surface_init(cui_surface_state *s, cui_id id, cui_surface_kind kind) {
    if (!s)
        return;
    *s = (cui_surface_state){.kind = kind >= CUI_SURFACE_DIALOG && kind <= CUI_SURFACE_POPOVER
                                         ? kind
                                         : CUI_SURFACE_DIALOG};
    cui_overlay_init(&s->overlay, id, s->kind != CUI_SURFACE_POPOVER);
}
void cui_surface_open(cui_context *ui, cui_surface_state *s, cui_id trigger) {
    if (!s)
        return;
    s->result = CUI_SURFACE_NONE;
    cui_overlay_open(ui, &s->overlay, trigger);
}
void cui_surface_close(cui_context *ui, cui_surface_state *s, cui_surface_result result) {
    if (!ui || !s || !s->overlay.open)
        return;
    s->result = result;
    cui_dismiss_focus_scope(ui, s->overlay.id, s->overlay.trigger);
    cui_overlay_close(ui, &s->overlay);
}
cui_surface_result cui_surface_take_result(cui_surface_state *s) {
    if (!s)
        return CUI_SURFACE_NONE;
    cui_surface_result result = s->result;
    s->result = CUI_SURFACE_NONE;
    return result;
}
cui_id cui_surface_part_id(cui_id id, unsigned part) {
    return id ^ (UINT64_C(0x7e361f4b2da50813) + part * UINT64_C(0x9e3779b97f4a7c15));
}
cui_rect cui_surface_geometry(cui_surface_kind kind, cui_surface_options o) {
    cui_rect v = o.viewport;
    if (!isfinite(v.x) || !isfinite(v.y) || !isfinite(v.w) || !isfinite(v.h) || v.w <= 0 ||
        v.h <= 0)
        return (cui_rect){0};
    float margin = isfinite(o.margin) && o.margin >= 0 ? o.margin : 0;
    margin = fminf(margin, fminf(v.w, v.h) * .49f);
    cui_rect area = {v.x + margin, v.y + margin, v.w - 2 * margin, v.h - 2 * margin};
    float width =
        fminf(area.w, cui_surface_positive(o.width, kind == CUI_SURFACE_SHEET ? 350 : 420));
    float height = fminf(area.h, cui_surface_positive(o.height, 260));
    if (kind == CUI_SURFACE_POPOVER)
        return cui_popup_place(o.anchor, (cui_size){width, height}, area, o.side, 6).bounds;
    if (kind == CUI_SURFACE_SHEET) {
        if (o.edge == CUI_SHEET_TOP || o.edge == CUI_SHEET_BOTTOM)
            return (cui_rect){area.x, o.edge == CUI_SHEET_TOP ? area.y : area.y + area.h - height,
                              area.w, height};
        return (cui_rect){o.edge == CUI_SHEET_LEFT ? area.x : area.x + area.w - width, area.y,
                          width, area.h};
    }
    return (cui_rect){area.x + (area.w - width) / 2, area.y + (area.h - height) / 2, width, height};
}
bool cui_base_surface_update(cui_context *ui, cui_surface_state *s, cui_surface_options o) {
    if (!ui || !s || !s->overlay.open)
        return false;
    s->overlay.modal = s->kind != CUI_SURFACE_POPOVER || o.modal_popover;
    cui_rect previous = cui_bounds(ui, s->overlay.id);
    if (previous.w > 0 && previous.h > 0)
        cui_overlay_set_bounds(&s->overlay, previous);
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    for (size_t i = 0; i < count && s->overlay.open; ++i) {
        cui_event e = events[i];
        cui_id activation = cui_event_activation(ui, i);
        cui_surface_result reason = CUI_SURFACE_NONE;
        if (s->kind == CUI_SURFACE_ALERT && activation == cui_surface_part_id(s->overlay.id, 2))
            reason = CUI_SURFACE_ACCEPTED;
        else if (s->kind == CUI_SURFACE_ALERT &&
                 activation == cui_surface_part_id(s->overlay.id, 3))
            reason = CUI_SURFACE_CANCELLED;
        else if (!o.hide_close && (s->kind != CUI_SURFACE_ALERT || o.show_alert_close) &&
                 activation == cui_surface_part_id(s->overlay.id, 1))
            reason = CUI_SURFACE_CLOSED;
        else if (!o.no_escape_dismiss && e.type == CUI_EVENT_KEY_DOWN && e.key == CUI_KEY_ESCAPE &&
                 !e.repeat && cui_event_target(ui, i) == s->overlay.id)
            reason = CUI_SURFACE_CANCELLED;
        else if (!o.no_outside_dismiss &&
                 (s->kind != CUI_SURFACE_ALERT || o.alert_outside_dismiss) &&
                 e.type == CUI_EVENT_POINTER_DOWN && e.pointer_button == CUI_POINTER_PRIMARY &&
                 s->overlay.has_bounds && cui_pointer_scope_allowed(ui, s->overlay.id) &&
                 (!s->overlay.modal || cui_event_target(ui, i) == s->overlay.id)) {
            cui_rect b = s->overlay.bounds;
            if (e.x < b.x || e.y < b.y || e.x >= b.x + b.w || e.y >= b.y + b.h)
                reason = CUI_SURFACE_CANCELLED;
        }
        if (reason != CUI_SURFACE_NONE && (!o.decide || o.decide(o.decision_user, reason)))
            cui_surface_close(ui, s, reason);
    }
    return !s->overlay.open;
}
bool cui_base_surface_begin(cui_context *ui, cui_surface_state *s, cui_str title,
                            cui_str description, cui_surface_options o) {
    if (!ui || !s)
        return false;
    cui_base_surface_update(ui, s, o);
    if (!s->overlay.open || !cui_overlay_begin(ui, &s->overlay))
        return false;
    cui_interact(ui, s->overlay.id,
                 s->kind == CUI_SURFACE_ALERT ? CUI_ROLE_ALERTDIALOG : CUI_ROLE_DIALOG, title,
                 false, false, 0);
    cui_set_semantic_details(
        ui, s->overlay.id,
        (cui_semantic_details){.description = description,
                               .labelled_by = cui_surface_part_id(s->overlay.id, 4)});
    cui_interact(ui, cui_surface_part_id(s->overlay.id, 4), CUI_ROLE_LABEL, title, false, false, 0);
    return true;
}
void cui_base_surface_end(cui_context *ui) {
    cui_overlay_end(ui);
}
