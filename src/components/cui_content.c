/* SPDX-License-Identifier: MIT */
#include "cui/cui_content.h"
#include "cui/cui_components.h"
#include "cui/cui_icon.h"
#include <math.h>
static float cui_content_positive(float value, float fallback) {
    return isfinite(value) && value > 0 ? value : fallback;
}
static cui_color cui_content_blend(cui_color a, cui_color b, float amount) {
    return (cui_color){(uint8_t)(a.r + (b.r - a.r) * amount), (uint8_t)(a.g + (b.g - a.g) * amount),
                       (uint8_t)(a.b + (b.b - a.b) * amount),
                       (uint8_t)(a.a + (b.a - a.a) * amount)};
}
static void cui_content_semantic(cui_context *ui, cui_id id, cui_role role, cui_str name,
                                 bool disabled, bool checked, float value) {
    cui_interact(ui, id, role, name, disabled, checked, value);
    cui_set_tab_stop(ui, id, false);
}
static cui_id cui_content_child(cui_id id, unsigned index) {
    return id ^ (UINT64_C(0x874e65fa19c80721) + UINT64_C(0x9e3779b97f4a7c15) * index);
}
bool cui_alert(cui_context *ui, cui_id id, const cui_theme *t, cui_alert_options o) {
    if (!ui || !t || o.kind < CUI_ALERT_INFO || o.kind > CUI_ALERT_ERROR ||
        (o.visible && !*o.visible))
        return false;
    float scale = cui_content_positive(o.scale, 1);
    cui_color accent = o.kind == CUI_ALERT_ERROR ? t->danger
                       : o.kind == CUI_ALERT_SUCCESS ? t->success
                       : o.kind == CUI_ALERT_WARNING ? t->warning : t->info;
    const char *kind = o.kind == CUI_ALERT_SUCCESS   ? "Success"
                       : o.kind == CUI_ALERT_WARNING ? "Warning"
                       : o.kind == CUI_ALERT_ERROR   ? "Error"
                                                     : "Information";
    cui_begin_box(ui, id,
                  (cui_box){.width = o.width > 0 ? o.width : 0,
                            .padding = t->padding * scale,
                            .gap = t->gap * scale,
                            .radius = o.banner ? 0 : t->radius,
                            .border = accent,
                            .border_width = 1,
                            .background = cui_content_blend(t->surface, accent, .1f)});
    cui_content_semantic(ui, id, CUI_ROLE_ALERT, o.message, false, false, 0);
    cui_begin_box(ui, cui_content_child(id, 2),
                  (cui_box){.row = true, .align_y = CUI_ALIGN_CENTER, .gap = t->gap * scale});
    if (o.icon.length)
        cui_text(ui, o.icon, t->font_size * .8f * scale, accent);
    else {
        const char *name = o.kind == CUI_ALERT_SUCCESS ? "circle-check"
                           : o.kind == CUI_ALERT_WARNING ? "triangle-alert"
                           : o.kind == CUI_ALERT_ERROR ? "circle-alert" : "info";
        cui_icon_named(ui, cui_content_child(id, 3), t,
                       o.icon_name.length ? o.icon_name : cui_string(name),
                       (cui_icon_options){.size = CUI_ICON_CUSTOM,
                                          .custom_size = t->font_size * scale,
                                          .custom_color = true, .color = accent});
    }
    cui_text(ui, cui_string(kind), t->font_size * .8f * scale, accent);
    cui_end_box(ui);
    if (!o.banner && o.title.length)
        cui_text(ui, o.title, t->font_size * scale, t->text);
    cui_text(ui, o.message, t->font_size * scale, t->text);
    bool closed = false;
    if (o.dismissible) {
        closed = cui_button(ui, cui_content_child(id, 1), t, cui_string("Dismiss"), false);
        if (closed && o.visible)
            *o.visible = false;
    }
    cui_end_box(ui);
    return closed;
}
void cui_avatar(cui_context *ui, cui_id id, const cui_theme *t, cui_str name, float size) {
    if (!ui || !t)
        return;
    size = cui_content_positive(size, 48);
    char initials[9];
    size_t n = cui_avatar_initials(name, initials);
    cui_begin_box(ui, id,
                  (cui_box){.width = size,
                            .height = size,
                            .radius = size / 2,
                            .align_x = CUI_ALIGN_CENTER,
                            .align_y = CUI_ALIGN_CENTER,
                            .background = t->primary,
                            .border = t->border,
                            .border_width = 1});
    cui_content_semantic(ui, id, CUI_ROLE_IMAGE, name.length ? name : cui_string("Anonymous"),
                         false, false, 0);
    cui_text(ui, (cui_str){initials, n}, size * .38f, t->on_primary);
    cui_end_box(ui);
}
void cui_begin_bubble(cui_context *ui, cui_id id, const cui_theme *t, cui_bubble_variant v,
                      float width) {
    if (!ui || !t)
        return;
    cui_color bg = t->surface;
    if (v == CUI_BUBBLE_FILLED)
        bg = t->primary;
    else if (v == CUI_BUBBLE_MUTED)
        bg = cui_content_blend(t->surface, t->border, .5f);
    else if (v == CUI_BUBBLE_TINTED)
        bg = cui_content_blend(t->surface, t->primary, .15f);
    else if (v == CUI_BUBBLE_DESTRUCTIVE)
        bg = cui_content_blend(t->surface, t->danger, .15f);
    else if (v >= CUI_BUBBLE_INFO && v <= CUI_BUBBLE_ERROR) {
        cui_color accent = v == CUI_BUBBLE_SUCCESS ? t->success
                           : v == CUI_BUBBLE_WARNING ? t->warning
                           : v == CUI_BUBBLE_ERROR ? t->danger : t->info;
        bg = cui_content_blend(t->surface, accent, .15f);
    }
    else if (v == CUI_BUBBLE_GHOST || v == CUI_BUBBLE_OUTLINE)
        bg.a = 0;
    cui_begin_box(ui, id,
                  (cui_box){.width = width > 0 ? width : 0,
                            .padding = v == CUI_BUBBLE_GHOST ? 0 : t->padding,
                            .gap = t->gap,
                            .radius = v == CUI_BUBBLE_GHOST ? 0 : t->radius * 2,
                            .background = bg,
                            .border = t->border,
                            .border_width = v == CUI_BUBBLE_OUTLINE ? 1 : 0});
}
void cui_end_bubble(cui_context *ui) {
    cui_end_box(ui);
}
void cui_empty(cui_context *ui, cui_id id, const cui_theme *t, cui_str title, cui_str description,
               float width) {
    if (!ui || !t)
        return;
    cui_begin_box(ui, id,
                  (cui_box){.width = width > 0 ? width : 0,
                            .padding = t->padding * 2,
                            .gap = t->gap,
                            .border = t->border,
                            .border_width = 1,
                            .radius = t->radius});
    cui_content_semantic(ui, id, CUI_ROLE_GROUP, title, false, false, 0);
    if (title.length)
        cui_text(ui, title, t->font_size * 1.2f, t->text);
    if (description.length)
        cui_text(ui, description, t->font_size, t->muted);
    cui_end_box(ui);
}
void cui_begin_group_box(cui_context *ui, cui_id id, const cui_theme *t, cui_str title,
                         float width) {
    if (!ui || !t)
        return;
    cui_begin_box(ui, id,
                  (cui_box){.width = width > 0 ? width : 0,
                            .padding = t->padding,
                            .gap = t->gap,
                            .border = t->border,
                            .border_width = 1,
                            .radius = t->radius,
                            .background = t->surface});
    cui_content_semantic(ui, id, CUI_ROLE_GROUP, title, false, false, 0);
    if (title.length)
        cui_text(ui, title, t->font_size, t->text);
}
void cui_end_group_box(cui_context *ui) {
    cui_end_box(ui);
}
void cui_kbd(cui_context *ui, cui_id id, const cui_theme *t, cui_str chord) {
    if (!ui || !t)
        return;
    cui_begin_box(ui, id,
                  (cui_box){.padding = 4,
                            .radius = t->radius * .5f,
                            .background = t->background,
                            .border = t->border,
                            .border_width = 1});
    cui_content_semantic(ui, id, CUI_ROLE_LABEL, chord, false, false, 0);
    cui_text(ui, chord, t->font_size * .8f, t->text);
    cui_end_box(ui);
}
void cui_spinner(cui_context *ui, cui_id id, const cui_theme *t, float size, float phase,
                 bool reduced) {
    if (!ui || !t)
        return;
    size = cui_content_positive(size, 24);
    phase = isfinite(phase) ? phase - floorf(phase) : 0;
    unsigned head = reduced ? 0 : (unsigned)(phase * 8);
    const unsigned ring[9] = {0, 1, 2, 7, 8, 3, 6, 5, 4};
    cui_begin_box(ui, id, (cui_box){.width = size, .height = size});
    cui_content_semantic(ui, id, CUI_ROLE_STATUS, cui_string("Loading"), false, false, 0);
    for (unsigned y = 0; y < 3; ++y) {
        cui_begin_box(ui, cui_content_child(id, 1 + y),
                      (cui_box){.row = true, .width = size, .height = size / 3});
        for (unsigned x = 0; x < 3; ++x) {
            unsigned index = ring[y * 3 + x];
            cui_color color = t->primary;
            if (index == 8)
                color.a = 0;
            else
                color.a =
                    (uint8_t)(color.a * (.25f + .75f * (float)(8 - ((index + 8 - head) % 8)) / 8));
            cui_begin_box(ui, cui_content_child(id, 5 + y * 3 + x),
                          (cui_box){.width = size / 3, .height = size / 3, .padding = size / 12});
            cui_begin_box(ui, cui_content_child(id, 20 + y * 3 + x),
                          (cui_box){.width = size / 6,
                                    .height = size / 6,
                                    .radius = size / 12,
                                    .background = color});
            cui_end_box(ui);
            cui_end_box(ui);
        }
        cui_end_box(ui);
    }
    cui_end_box(ui);
    if (!reduced)
        cui_invalidate(ui);
}
void cui_skeleton(cui_context *ui, cui_id id, const cui_theme *t, float width, float height,
                  float radius, float phase, bool secondary, bool reduced) {
    if (!ui || !t)
        return;
    phase = isfinite(phase) ? phase - floorf(phase) : 0;
    float alpha = reduced ? 1 : .75f + .25f * cosf(phase * 6.283185307f);
    cui_color color = t->border;
    color.a = (uint8_t)(color.a * alpha * (secondary ? .5f : 1));
    cui_begin_box(ui, id,
                  (cui_box){.width = cui_content_positive(width, 160),
                            .height = cui_content_positive(height, 16),
                            .radius = isfinite(radius) && radius >= 0 ? radius : t->radius,
                            .background = color});
    cui_end_box(ui);
    if (!reduced)
        cui_invalidate(ui);
}
void cui_divider(cui_context *ui, cui_id id, const cui_theme *t, float length, bool vertical) {
    if (!ui || !t)
        return;
    length = cui_content_positive(length, 160);
    cui_begin_box(ui, id,
                  (cui_box){.width = vertical ? 1 : length,
                            .height = vertical ? length : 1,
                            .background = t->border});
    cui_end_box(ui);
}
static bool cui_content_hover_panel(cui_context *ui, cui_id id, const cui_theme *t,
                                    cui_hover_state *state, cui_str name, cui_hover_options o,
                                    bool card) {
    if (!ui || !t || !state)
        return false;
    size_t n = 0;
    const cui_event *events = cui_frame_events(ui, &n);
    bool escape = false;
    for (size_t i = 0; i < n; ++i)
        if (events[i].type == CUI_EVENT_KEY_DOWN && events[i].key == CUI_KEY_ESCAPE)
            escape = true;
    cui_id focused = cui_focused(ui);
    cui_rect previous = cui_bounds(ui, id), focus_bounds = cui_bounds(ui, focused);
    bool inside_focus = card && state->open && focused && focused != o.trigger &&
                        focus_bounds.w > 0 && focus_bounds.h > 0 && focus_bounds.x >= previous.x &&
                        focus_bounds.y >= previous.y &&
                        focus_bounds.x + focus_bounds.w <= previous.x + previous.w &&
                        focus_bounds.y + focus_bounds.h <= previous.y + previous.h;
    bool changed = cui_hover_update(
        state, cui_hovered(ui, o.trigger), card && (cui_hovered(ui, id) || inside_focus),
        focused == o.trigger, escape, o.disabled, o.dt, o.open_delay, o.close_delay);
    if (changed || state->pending)
        cui_invalidate(ui);
    if (!state->open)
        return false;
    cui_rect anchor = cui_bounds(ui, o.trigger);
    if (!isfinite(o.viewport.x) || !isfinite(o.viewport.y) || !isfinite(o.viewport.w) ||
        !isfinite(o.viewport.h) || o.viewport.w <= 0 || o.viewport.h <= 0 || anchor.w <= 0 ||
        anchor.h <= 0) {
        state->open = false;
        return false;
    }
    float width = cui_content_positive(o.width, card ? 240 : 200);
    float height =
        cui_content_positive(o.height, card ? 120 : t->font_size * 1.5f + t->padding * 2);
    cui_popup_result placement =
        cui_popup_place(anchor, (cui_size){width, height}, o.viewport, o.side, o.gap);
    float padding =
        card ? t->padding
             : fminf(t->padding,
                     fmaxf(0, (height - cui_measure_text(ui, name, t->font_size).height) / 2));
    cui_begin_portal(ui, id,
                     (cui_box){.padding = padding,
                               .gap = t->gap,
                               .align_x = card ? CUI_ALIGN_START : CUI_ALIGN_CENTER,
                               .align_y = card ? CUI_ALIGN_START : CUI_ALIGN_CENTER,
                               .radius = t->radius,
                               .background = t->surface,
                               .border = t->border,
                               .border_width = 1,
                               .clip = true},
                     placement.bounds, card ? 110 : 120);
    cui_content_semantic(ui, id, card ? CUI_ROLE_GROUP : CUI_ROLE_LABEL, name, false, false, 0);
    cui_set_semantic_details(ui, id, (cui_semantic_details){.parent = o.trigger});
    if (!card) {
        size_t semantic_count = 0;
        const cui_semantic *semantics = cui_semantics(ui, &semantic_count);
        for (size_t i = 0; i < semantic_count; ++i) {
            if (semantics[i].id == o.trigger) {
                cui_semantic_details details = semantics[i].details;
                details.description = name;
                cui_set_semantic_details(ui, o.trigger, details);
                break;
            }
        }
    }
    return true;
}
bool cui_tooltip(cui_context *ui, cui_id id, const cui_theme *t, cui_hover_state *state,
                 cui_str text, cui_hover_options options) {
    if (!cui_content_hover_panel(ui, id, t, state, text, options, false))
        return false;
    cui_text(ui, text, t->font_size, t->text);
    cui_end_box(ui);
    return true;
}
bool cui_begin_hover_card(cui_context *ui, cui_id id, const cui_theme *t, cui_hover_state *state,
                          cui_str name, cui_hover_options options) {
    return cui_content_hover_panel(ui, id, t, state, name, options, true);
}
void cui_end_hover_card(cui_context *ui) {
    cui_end_box(ui);
}
