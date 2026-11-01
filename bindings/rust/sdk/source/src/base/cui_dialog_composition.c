/* SPDX-License-Identifier: MIT */
#include "cui_dialog_composition_private.h"
#include "cui_dialog_internal.h"
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <string.h>
_Static_assert(sizeof(cui_dialog_slot_style_v1) == 32, "dialog slot style ABI");
_Static_assert(sizeof(cui_dialog_refinement_v1) == 232, "dialog refinement ABI");
_Static_assert(sizeof(cui_dialog_sizing_v1) == 96, "dialog sizing ABI");
#if UINTPTR_MAX == UINT64_MAX
_Static_assert(sizeof(cui_dialog_slot) == 16, "dialog callback LP64 ABI");
_Static_assert(sizeof(cui_dialog_composition_v1) == 152, "dialog composition LP64 ABI");
_Static_assert(sizeof(cui_dialog_layout_v1) == 208, "dialog layout LP64 ABI");
#else
_Static_assert(sizeof(cui_dialog_slot) == 8, "dialog callback ILP32 ABI");
_Static_assert(sizeof(cui_dialog_composition_v1) == 84, "dialog composition ILP32 ABI");
_Static_assert(sizeof(cui_dialog_layout_v1) == 200, "dialog layout ILP32 ABI");
#endif
enum { CUI_DIALOG_BUSY = 1u, CUI_DIALOG_PREVIOUS_BODY = 2u };
static bool cui_dialog_fail(cui_context *ui) {
    if (ui)
        cui_report_diagnostic(ui, &(cui_diagnostic){.error = CUI_ERROR_INVALID_ACTION});
    return false;
}
static bool cui_dialog_finite(float x, bool positive) {
    return isfinite(x) && (positive ? x > 0 : x >= 0);
}
static bool cui_dialog_layout_valid(const cui_dialog_layout_v1 *l) {
    return l && l->struct_size == sizeof *l && l->version == 1 && !l->reserved && !l->reserved2 &&
           !(l->private_flags & ~3u) && l->surface && l->surface->overlay.id &&
           (l->surface->kind == CUI_SURFACE_DIALOG || l->surface->kind == CUI_SURFACE_ALERT);
}
bool cui_dialog_layout_init_v1(cui_dialog_layout_v1 *l, cui_surface_state *s) {
    if (!l || !s || !s->overlay.id ||
        (s->kind != CUI_SURFACE_DIALOG && s->kind != CUI_SURFACE_ALERT))
        return false;
    *l = (cui_dialog_layout_v1){.struct_size = sizeof *l, .version = 1, .surface = s};
    return true;
}
bool cui_dialog_open_v1(cui_context *ui, cui_dialog_layout_v1 *l, cui_id trigger) {
    if (!ui || !cui_dialog_layout_valid(l) || (l->private_flags & CUI_DIALOG_BUSY) ||
        (l->private_context && l->private_context != ui))
        return cui_dialog_fail(ui);
    l->private_context = ui;
    if (l->surface->overlay.open)
        return true;
    l->private_flags = 0;
    l->outer_y = 0;
    l->surface->body_scroll = l->surface->body_maximum = 0;
    l->surface->body_viewport = l->surface->body_content = (cui_rect){0};
    memset(l->completed, 0, sizeof l->completed);
    l->private_submit_frame = 0;
    cui_surface_open(ui, l->surface, trigger);
    return l->surface->overlay.open;
}
static bool cui_dialog_style_valid(cui_dialog_slot_style_v1 s) {
    return !(s.fields & ~63u) && !s.reserved &&
           (!(s.fields & CUI_DIALOG_STYLE_FONT_SIZE) || cui_dialog_finite(s.font_size, true)) &&
           (!(s.fields & CUI_DIALOG_STYLE_PADDING) || cui_dialog_finite(s.padding, false)) &&
           (!(s.fields & CUI_DIALOG_STYLE_GAP) || cui_dialog_finite(s.gap, false)) &&
           (!(s.fields & CUI_DIALOG_STYLE_ALIGN_X) || s.align_x <= CUI_ALIGN_END) &&
           (!(s.fields & CUI_DIALOG_STYLE_ALIGN_Y) || s.align_y <= CUI_ALIGN_END);
}
bool cui_dialog_composition_valid_v1(const cui_dialog_composition_v1 *c) {
    if (!c || c->struct_size != sizeof *c || c->version != 1 || (c->omit & ~63u) ||
        (c->present & c->omit) || (c->present & ~63u) || (!c->name.data && c->name.length) ||
        (!c->description.data && c->description.length))
        return false;
    const cui_dialog_refinement_v1 *r = c->refinement;
    if (!r)
        return true;
    if (r->struct_size != sizeof *r || r->version != 1 || r->reserved || (r->panel_fields & ~63u))
        return false;
    if (((r->panel_fields & CUI_DIALOG_PANEL_RADIUS) && !cui_dialog_finite(r->radius, false)) ||
        ((r->panel_fields & CUI_DIALOG_PANEL_PADDING) && !cui_dialog_finite(r->padding, false)) ||
        ((r->panel_fields & CUI_DIALOG_PANEL_GAP) && !cui_dialog_finite(r->gap, false)) ||
        ((r->panel_fields & CUI_DIALOG_PANEL_BORDER_WIDTH) &&
         !cui_dialog_finite(r->border_width, false)))
        return false;
    for (unsigned i = 0; i < CUI_DIALOG_SLOT_COUNT; ++i)
        if (!cui_dialog_style_valid(r->slots[i]))
            return false;
    return true;
}
bool cui_dialog_sizing_default_v1(cui_surface_options o, cui_dialog_sizing_v1 *out) {
    if (!out || !cui_dialog_finite(o.viewport.w, true) || !cui_dialog_finite(o.viewport.h, true) ||
        !cui_dialog_finite(o.margin, false))
        return false;
    float maximum = o.viewport.h - 2 * o.margin;
    if (!isfinite(maximum) || maximum <= 0)
        return false;
    maximum = fmaxf(96, maximum); /* Source min-height wins over raw max-height. */
    *out = (cui_dialog_sizing_v1){
        .struct_size = sizeof *out,
        .version = 1,
        .mode = CUI_SCROLL_BODY_AUTO,
        .width = 420,
        .min_height = 96,
        .max_height = maximum,
        .content_sizing = {.mode = CUI_SCROLL_REGION_FLEX1,
                           .minimum_policy = CUI_SCROLL_REGION_MIN_INTRINSIC,
                           .max_height = FLT_MAX},
        .body_sizing = {.mode = CUI_SCROLL_REGION_FLEX1, .max_height = FLT_MAX},
        .placement = CUI_DIALOG_SOURCE_TOP};
    return true;
}
static bool cui_dialog_region_valid(cui_scroll_region_sizing_v1 r) {
    if (r.reserved || r.mode < CUI_SCROLL_REGION_FLEX1 || r.mode > CUI_SCROLL_REGION_FIXED ||
        r.minimum_policy > CUI_SCROLL_REGION_MIN_INTRINSIC ||
        !cui_dialog_finite(r.min_height, false) || !cui_dialog_finite(r.max_height, true) ||
        r.min_height > r.max_height)
        return false;
    return r.mode == CUI_SCROLL_REGION_FIXED
               ? cui_dialog_finite(r.requested_height, true) &&
                     r.requested_height >= r.min_height && r.requested_height <= r.max_height
               : r.requested_height == 0;
}
static bool cui_dialog_sizing_valid(const cui_dialog_sizing_v1 *s, uint32_t present) {
    return s && s->struct_size == sizeof *s && s->version == 1 && !s->reserved &&
           s->mode <= CUI_SCROLL_BODY_EXPLICIT && cui_dialog_finite(s->width, true) &&
           cui_dialog_finite(s->min_height, false) && cui_dialog_finite(s->max_height, true) &&
           s->min_height <= s->max_height &&
           (s->mode == CUI_SCROLL_BODY_AUTO ? s->requested_height == 0
                                            : cui_dialog_finite(s->requested_height, true) &&
                                                  s->requested_height >= s->min_height &&
                                                  s->requested_height <= s->max_height) &&
           s->placement <= CUI_DIALOG_CENTERED && !(s->placement_fields & ~3u) &&
           (!(s->placement_fields & CUI_DIALOG_PREFERRED_TOP) || isfinite(s->preferred_top)) &&
           (!(s->placement_fields & CUI_DIALOG_LAYER_OFFSET) ||
            cui_dialog_finite(s->layer_offset, false)) &&
           (!(present & (1u << CUI_DIALOG_CONTENT)) ||
            cui_dialog_region_valid(s->content_sizing)) &&
           (!(present & (1u << CUI_DIALOG_BODY)) || cui_dialog_region_valid(s->body_sizing));
}
bool cui_dialog_resolve(const cui_theme *t, const cui_dialog_refinement_v1 *r,
                        cui_dialog_resolved *out) {
    if (!out)
        return false;
    cui_dialog_resolved v = {.panel = {.padding = 16, .gap = 16},
                             .scrollbar = {.visibility = CUI_SCROLLBAR_VISIBLE},
                             .thumb = {100, 100, 100, 255},
                             .track = {200, 200, 200, 255}};
    if (t) {
        v.panel.background = t->surface;
        v.panel.border = t->border;
        v.panel.border_width = 1;
        v.panel.radius = t->radius;
        v.scrollbar = t->scrollbar;
        if (v.scrollbar.visibility == CUI_SCROLLBAR_INHERIT)
            v.scrollbar.visibility = CUI_SCROLLBAR_VISIBLE;
        v.thumb = t->muted;
        v.track = t->border;
    }
    for (unsigned i = 0; i < CUI_DIALOG_SLOT_COUNT; ++i) {
        v.slots[i] =
            (cui_dialog_slot_style_v1){.foreground = t ? t->text : (cui_color){0, 0, 0, 255},
                                       .font_size = t ? t->font_size : 14,
                                       .gap = 8};
        if (i == CUI_DIALOG_CONTENT)
            v.slots[i].gap = 16;
        if (i == CUI_DIALOG_FOOTER)
            v.slots[i].align_x = CUI_ALIGN_END;
        if (r) {
            const cui_dialog_slot_style_v1 *a = &r->slots[i];
            if (a->fields & 1)
                v.slots[i].foreground = a->foreground;
            if (a->fields & 2)
                v.slots[i].font_size = a->font_size;
            if (a->fields & 4)
                v.slots[i].padding = a->padding;
            if (a->fields & 8)
                v.slots[i].gap = a->gap;
            if (a->fields & 16)
                v.slots[i].align_x = a->align_x;
            if (a->fields & 32)
                v.slots[i].align_y = a->align_y;
        }
        v.slots[i].fields = 63;
    }
    if (r) {
        if (r->panel_fields & 1)
            v.panel.background = r->background;
        if (r->panel_fields & 2)
            v.panel.border = r->border;
        if (r->panel_fields & 4)
            v.panel.radius = r->radius;
        if (r->panel_fields & 8)
            v.panel.padding = r->padding;
        if (r->panel_fields & 16)
            v.panel.gap = r->gap;
        if (r->panel_fields & 32)
            v.panel.border_width = r->border_width;
    }
    *out = v;
    return true;
}
static bool cui_dialog_token(cui_context *ui, const cui_dialog_frame *f) {
    return f && f->active && f->ui == ui && cui_frame_is_submitting(ui) &&
           cui_dialog_layout_valid(f->layout) && (f->layout->private_flags & CUI_DIALOG_BUSY);
}
bool cui_dialog_slot_style_v1_get(const cui_dialog_frame *f, cui_dialog_slot_kind kind,
                                  cui_dialog_slot_style_v1 *out) {
    if (!f || !f->active || !out || (unsigned)kind >= CUI_DIALOG_SLOT_COUNT)
        return false;
    *out = f->resolved->slots[kind];
    return true;
}
bool cui_dialog_slot_begin_v1(cui_context *ui, const cui_dialog_frame *f, cui_dialog_slot_kind kind,
                              cui_id id) {
    if (!cui_dialog_token(ui, f) || !id || (unsigned)kind >= CUI_DIALOG_SLOT_COUNT ||
        f->wrappers >= 64)
        return cui_dialog_fail(ui);
    cui_dialog_slot_style_v1 s = f->resolved->slots[kind];
    cui_begin_box(ui, id,
                  (cui_box){.padding = s.padding,
                            .gap = s.gap,
                            .align_x = (cui_align)s.align_x,
                            .align_y = (cui_align)s.align_y,
                            .row = kind == CUI_DIALOG_FOOTER});
    ++((cui_dialog_frame *)f)->wrappers;
    return true;
}
void cui_dialog_slot_end_v1(cui_context *ui, const cui_dialog_frame *f) {
    if (!cui_dialog_token(ui, f) || !f->wrappers) {
        cui_dialog_fail(ui);
        return;
    }
    cui_end_box(ui);
    --((cui_dialog_frame *)f)->wrappers;
}
bool cui_dialog_action_reduce(cui_context *ui, const cui_dialog_frame *f,
                              cui_dialog_action_kind kind) {
    if (!cui_dialog_token(ui, f) || (unsigned)kind > CUI_DIALOG_CLOSE)
        return cui_dialog_fail(ui);
    cui_surface_state *s = f->layout->surface;
    if (!f->input_ready || !cui_frame_sequence(ui) || !s->overlay.open)
        return false;
    cui_surface_result reason = kind == CUI_DIALOG_CONFIRM  ? CUI_SURFACE_ACCEPTED
                                : kind == CUI_DIALOG_CANCEL ? CUI_SURFACE_CANCELLED
                                                            : CUI_SURFACE_CLOSED;
    if (f->options.decide && !f->options.decide(f->options.decision_user, reason))
        return false;
    cui_surface_close(ui, s, reason);
    return !s->overlay.open;
}
cui_interaction cui_base_dialog_action_v1(cui_context *ui, const cui_dialog_frame *f, cui_id id,
                                          cui_str name, cui_dialog_action_kind kind,
                                          bool disabled) {
    if (!cui_dialog_token(ui, f) || !id || (unsigned)kind > CUI_DIALOG_CLOSE) {
        cui_dialog_fail(ui);
        return (cui_interaction){0};
    }
    cui_interaction a = cui_interact(ui, id, CUI_ROLE_BUTTON, name,
                                     disabled || !f->layout->surface->overlay.open, false, 0);
    if (a.activated)
        (void)cui_dialog_action_reduce(ui, f, kind);
    return a;
}
static cui_id cui_dialog_part(const cui_dialog_frame *f, unsigned p) {
    return cui_surface_part_id(f->layout->surface->overlay.id, p);
}
static cui_box cui_dialog_slot_box(const cui_dialog_frame *f, unsigned kind, float inset) {
    cui_dialog_slot_style_v1 s = f->resolved->slots[kind];
    return (cui_box){.width = f->width,
                     .fixed_width = true,
                     .gap = s.gap,
                     .row = kind == CUI_DIALOG_FOOTER,
                     .align_x = (cui_align)s.align_x,
                     .align_y = (cui_align)s.align_y,
                     .custom_padding = true,
                     .padding_edges = {.left = inset + s.padding,
                                       .right = inset + s.padding,
                                       .top = s.padding,
                                       .bottom = s.padding}};
}
static void cui_dialog_callback(cui_dialog_frame *f, unsigned kind,
                                const cui_dialog_composition_v1 *c) {
    f->active = true;
    if (c->present & (1u << kind)) {
        if (c->slots[kind].emit)
            c->slots[kind].emit(f->ui, f, c->slots[kind].user);
    } else if (kind == CUI_DIALOG_TITLE || kind == CUI_DIALOG_DESCRIPTION) {
        cui_dialog_slot_style_v1 s = f->resolved->slots[kind];
        cui_str text = kind == CUI_DIALOG_TITLE ? c->name : c->description;
        (void)cui_text_paragraph_colored(
            f->ui, text, s.font_size, s.foreground, s.font_size * 1.25f,
            (cui_text_style){.weight = kind == CUI_DIALOG_TITLE ? CUI_TEXT_WEIGHT_SEMIBOLD
                                                                : CUI_TEXT_WEIGHT_NORMAL},
            true, NULL, 0);
    }
    if (f->wrappers) {
        cui_dialog_fail(f->ui);
        while (f->wrappers) {
            cui_end_box(f->ui);
            --f->wrappers;
        }
    }
    f->active = false;
}
static void cui_dialog_gap(cui_dialog_frame *f, unsigned *count, unsigned kind) {
    if ((*count)++) {
        cui_begin_box(f->ui, cui_dialog_part(f, 30 + kind),
                      (cui_box){.width = f->width, .height = 8, .fixed_height = true});
        cui_end_box(f->ui);
    }
}
static const cui_semantic *cui_dialog_semantic(cui_context *ui, cui_id id) {
    size_t count;
    const cui_semantic *s = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (s[i].id == id)
            return &s[i];
    return NULL;
}
static void cui_dialog_body_finish(cui_dialog_frame *f, const cui_scrollbar_style *skin) {
    cui_surface_state *s = f->layout->surface;
    if (!f->body_input_ready || !cui_frame_sequence(f->ui))
        return;
    cui_scroll_state local = {.y = s->body_scroll};
    cui_scroll_options o = {.width = s->body_viewport.w,
                            .height = s->body_viewport.h,
                            .content_width = s->body_content.w,
                            .content_height = s->body_content.h,
                            .vertical = true,
                            .hide_scrollbars = true,
                            .scrollbar_style = skin};
    (void)cui_base_scrollable_finish(f->ui, cui_dialog_part(f, 6), &local, o);
    s->body_scroll = local.y;
    cui_set_box_scroll_offset(f->ui, cui_dialog_part(f, 6), 0, -s->body_scroll);
    const cui_semantic *sem = cui_dialog_semantic(f->ui, cui_dialog_part(f, 6));
    if (sem) {
        cui_semantic_details d = sem->details;
        d.parent = s->overlay.id;
        cui_set_semantic_details(f->ui, sem->id, d);
    }
}
static bool cui_dialog_bar(cui_dialog_frame *f, const cui_scrollbar_style *skin) {
    cui_context *ui = f->ui;
    cui_id owner = cui_dialog_part(f, 6), bar = cui_scrollbar_id(owner, true);
    cui_surface_state *s = f->layout->surface;
    bool hidden = skin->visibility == CUI_SCROLLBAR_HIDDEN;
    float opacity = hidden ? 0 : 1;
    if (!hidden && f->body_input_ready && s->body_maximum > 0 && cui_frame_sequence(ui))
        opacity = cui_scrollbar_opacity(ui, owner, bar, skin, s->body_scroll, false);
    float thickness = isfinite(skin->thickness) && skin->thickness > 0 ? skin->thickness : 10;
    thickness = fminf(thickness, f->width);
    if (hidden)
        return false;
    if (!cui_dialog_semantic(ui, bar)) {
        cui_interact(ui, bar, CUI_ROLE_SCROLLBAR, cui_string("Vertical scroll"), true, false, 0);
        cui_set_semantic_details(ui, bar,
                                 (cui_semantic_details){.parent = owner,
                                                        .controls = owner,
                                                        .orientation = CUI_ORIENTATION_VERTICAL});
    }
    cui_color track = skin->custom_track_color ? skin->track_color : f->resolved->track;
    cui_color thumb = skin->custom_thumb_color ? skin->thumb_color : f->resolved->thumb;
    cui_begin_absolute_box(ui, bar,
                           (cui_box){.background = skin->no_track ? (cui_color){0} : track,
                                     .radius = skin->track_radius,
                                     .custom_opacity = true,
                                     .opacity = opacity},
                           (cui_rect){0, 0, thickness, 1});
    cui_begin_absolute_box(ui, cui_dialog_part(f, 17),
                           (cui_box){.background = thumb, .radius = skin->thumb_radius},
                           (cui_rect){0, 0, thickness, 1});
    cui_end_box(ui);
    cui_end_box(ui);
    return true;
}
static void cui_dialog_default_close(cui_context *ui, const cui_dialog_frame *f, const void *user) {
    (void)user;
    cui_begin_box(ui, cui_dialog_part(f, 1),
                  (cui_box){.width = 24, .height = 24, .fixed_width = true, .fixed_height = true});
    (void)cui_base_dialog_action_v1(ui, f, cui_dialog_part(f, 1), cui_string("Close"),
                                    CUI_DIALOG_CLOSE, false);
    cui_end_box(ui);
}
cui_dialog_submit_result
cui_dialog_compose_resolved(cui_context *ui, cui_dialog_layout_v1 *l, cui_surface_options o,
                            const cui_dialog_sizing_v1 *z, const cui_dialog_composition_v1 *c,
                            const cui_dialog_resolved *r, cui_dialog_close_emitter close,
                            const void *close_user) {
    cui_dialog_submit_result result = {0};
    if (!ui || !cui_frame_is_submitting(ui) || !cui_dialog_layout_valid(l) ||
        (l->private_flags & CUI_DIALOG_BUSY) || (l->private_context && l->private_context != ui) ||
        !r || !cui_dialog_composition_valid_v1(c) || !cui_dialog_sizing_valid(z, c->present) ||
        !isfinite(o.viewport.x) || !isfinite(o.viewport.y) ||
        !cui_dialog_finite(o.viewport.w, true) || !cui_dialog_finite(o.viewport.h, true) ||
        !cui_dialog_finite(o.margin, false)) {
        cui_dialog_fail(ui);
        return result;
    }
    if (cui_last_error(ui) != CUI_OK)
        return result;
    float width = fminf(z->width, o.viewport.w - 2 * o.margin);
    float height = o.viewport.h - 2 * o.margin;
    float offset = (z->placement_fields & CUI_DIALOG_LAYER_OFFSET) ? z->layer_offset : 0;
    float maximum = fmaxf(z->min_height, fminf(z->max_height, height - offset));
    if (!cui_dialog_finite(width, true) || !cui_dialog_finite(height, true) ||
        !cui_dialog_finite(maximum, true)) {
        cui_dialog_fail(ui);
        return result;
    }
    uint64_t sequence = cui_frame_sequence(ui);
    if (sequence && sequence == l->private_submit_frame) {
        cui_dialog_fail(ui);
        return result;
    }
    bool adjacent =
        l->private_submit_frame &&
        sequence == (l->private_submit_frame == UINT64_MAX ? 1 : l->private_submit_frame + 1);
    bool input_ready = sequence && adjacent && cui_frame_input_snapshot_ready(ui);
    bool body_input_ready = input_ready && (l->private_flags & CUI_DIALOG_PREVIOUS_BODY);
    result.valid = true;
    l->private_context = ui;
    if (sequence) {
        /* Input-only previous geometry, same open lifetime and successful prior
         * snapshot. NEVER use this cache to choose current paint/allocation. */
        l->surface->body_viewport =
            body_input_ready ? cui_bounds(ui, cui_surface_part_id(l->surface->overlay.id, 6))
                             : (cui_rect){0};
        l->surface->body_content =
            body_input_ready ? cui_bounds(ui, cui_surface_part_id(l->surface->overlay.id, 7))
                             : (cui_rect){0};
        l->surface->body_maximum =
            body_input_ready ? fmaxf(0, l->surface->body_content.h - l->surface->body_viewport.h)
                             : 0;
        cui_surface_options update = o;
        if (!input_ready)
            update.no_escape_dismiss = update.no_outside_dismiss = true;
        cui_dialog_surface_update(ui, l->surface, update);
    }
    if (!l->surface->overlay.open)
        return result;
    if (!cui_overlay_begin(ui, &l->surface->overlay)) {
        result.valid = false;
        return result;
    }
    l->private_flags |= CUI_DIALOG_BUSY;
    if (sequence && !input_ready) {
        size_t count;
        cui_frame_events(ui, &count);
        for (size_t i = 0; i < count; ++i)
            if (!cui_event_consumed(ui, i) && cui_event_within_scope(ui, i, l->surface->overlay.id))
                cui_consume_event(ui, i);
    }
    if (sequence && !body_input_ready) {
        /* Cancel old context-owned drag through the public hidden helper. No
         * stale geometry, timer touch, allocation or input reduction. */
        float x = 0, y = 0;
        (void)cui_base_scrollbar_overlay(ui, cui_surface_part_id(l->surface->overlay.id, 6), &x, &y,
                                         (cui_rect){0, 0, 1, 1}, (cui_size){1, 1},
                                         (cui_scroll_options){.hide_scrollbars = true});
    }
    memset(l->completed, 0, sizeof l->completed);
    cui_dialog_frame f = {.ui = ui,
                          .layout = l,
                          .options = o,
                          .resolved = r,
                          .width = width,
                          .input_ready = input_ready,
                          .body_input_ready = body_input_ready};
    cui_surface_state *s = l->surface;
    cui_scrollbar_style skin = o.scrollbar_style ? *o.scrollbar_style : r->scrollbar;
    skin.reduced_motion |= o.reduced_motion;
    int16_t layer = o.layer > 0 && o.layer < 32766 ? o.layer : 100;
    if (s->overlay.modal) {
        cui_begin_portal(ui, s->overlay.id ^ UINT64_C(0x86a3e3d76bc1d909),
                         (cui_box){.background = {0, 0, 0, 100}}, o.viewport, layer);
        cui_end_box(ui);
    }
    cui_begin_portal(ui, cui_dialog_part(&f, 16),
                     (cui_box){.align_x = CUI_ALIGN_CENTER,
                               .align_y = z->placement == CUI_DIALOG_CENTERED ? CUI_ALIGN_CENTER
                                                                              : CUI_ALIGN_START},
                     (cui_rect){o.viewport.x + o.margin, o.viewport.y + o.margin,
                                o.viewport.w - 2 * o.margin, height},
                     (int16_t)(layer + 1));
    if (z->placement == CUI_DIALOG_SOURCE_TOP) {
        float preferred =
            (z->placement_fields & CUI_DIALOG_PREFERRED_TOP) ? z->preferred_top : o.viewport.h / 10;
        float cap = preferred + offset - o.margin;
        if (!isfinite(cap)) {
            cui_dialog_fail(ui);
            cap = 0;
        }
        if (cap > 0) {
            cui_begin_box(ui, cui_dialog_part(&f, 18),
                          (cui_box){.width = 1, .height = -1, .max_height = cap});
            cui_end_box(ui);
        }
    }
    cui_box panel = r->panel;
    panel.width = width;
    panel.fixed_width = true;
    panel.padding = 0;
    panel.custom_padding = true;
    panel.padding_edges = (cui_border_edges){.top = r->panel.padding, .bottom = r->panel.padding};
    cui_begin_box(ui, s->overlay.id, panel);
    cui_track_bounds(ui, s->overlay.id, &l->completed[0]);
    cui_interact(ui, s->overlay.id,
                 s->kind == CUI_SURFACE_ALERT ? CUI_ROLE_ALERTDIALOG : CUI_ROLE_DIALOG, c->name,
                 false, false, 0);
    bool title_present = !(c->omit & (1u << CUI_DIALOG_TITLE)) &&
                         ((c->present & (1u << CUI_DIALOG_TITLE)) || c->name.length);
    cui_set_semantic_details(
        ui, s->overlay.id,
        (cui_semantic_details){.description = c->description,
                               .labelled_by = title_present ? cui_dialog_part(&f, 4) : 0});
    cui_begin_box(ui, cui_dialog_part(&f, 13),
                  (cui_box){.width = width, .height = -1, .clip = true});
    cui_track_bounds(ui, cui_dialog_part(&f, 13), &l->completed[8]);
    cui_begin_box(ui, cui_dialog_part(&f, 14), (cui_box){.width = width});
    cui_track_bounds(ui, cui_dialog_part(&f, 14), &l->completed[9]);
    unsigned count = 0;
    const unsigned parts[] = {9, 4, 10, 11};
    bool content = false;
    for (unsigned kind = 0; kind < CUI_DIALOG_BODY && s->overlay.open; ++kind) {
        bool present =
            !(c->omit & (1u << kind)) &&
            ((c->present & (1u << kind)) || (kind == CUI_DIALOG_TITLE && c->name.length) ||
             (kind == CUI_DIALOG_DESCRIPTION && c->description.length));
        if (!present)
            continue;
        cui_dialog_gap(&f, &count, kind);
        cui_begin_box(ui, cui_dialog_part(&f, parts[kind]),
                      cui_dialog_slot_box(&f, kind, r->panel.padding));
        cui_track_bounds(ui, cui_dialog_part(&f, parts[kind]), &l->completed[1 + kind]);
        if (kind == CUI_DIALOG_TITLE)
            cui_interact(ui, cui_dialog_part(&f, 4), CUI_ROLE_LABEL, c->name, false, false, 0);
        cui_dialog_callback(&f, kind, c);
        cui_end_box(ui);
        if (kind == CUI_DIALOG_CONTENT)
            content = true;
    }
    bool body = s->overlay.open && (c->present & (1u << CUI_DIALOG_BODY));
    if (body)
        cui_dialog_gap(&f, &count, CUI_DIALOG_BODY);
    cui_begin_box(ui, cui_dialog_part(&f, 15), (cui_box){.width = width, .height = -1});
    cui_begin_absolute_box(ui, cui_dialog_part(&f, 6),
                           (cui_box){.clip = true, .offset_y = -s->body_scroll},
                           (cui_rect){0, 0, -1, -1});
    if (body) {
        cui_track_bounds(ui, cui_dialog_part(&f, 6), &l->completed[5]);
        cui_interact(ui, cui_dialog_part(&f, 6), CUI_ROLE_GROUP, cui_string("Dialog body"), false,
                     false, 0);
        cui_set_semantic_details(ui, cui_dialog_part(&f, 6),
                                 (cui_semantic_details){.parent = s->overlay.id,
                                                        .scrollable = true,
                                                        .scroll_y = s->body_scroll,
                                                        .scroll_max_y = s->body_maximum});
    }
    cui_box body_box = cui_dialog_slot_box(&f, CUI_DIALOG_BODY, r->panel.padding);
    if (!body) {
        body_box.height = 0;
        body_box.fixed_height = true;
        body_box.padding_edges = (cui_border_edges){0};
    }
    cui_begin_box(ui, cui_dialog_part(&f, 7), body_box);
    if (body) {
        cui_track_bounds(ui, cui_dialog_part(&f, 7), &l->completed[6]);
        cui_dialog_callback(&f, CUI_DIALOG_BODY, c);
    }
    cui_end_box(ui);
    bool bar = false;
    if (body) {
        if (body_input_ready && s->overlay.open) {
            float x = 0;
            (void)cui_base_scrollbar_overlay(
                ui, cui_dialog_part(&f, 6), &x, &s->body_scroll,
                (cui_rect){0, 0, s->body_viewport.w, s->body_viewport.h},
                (cui_size){s->body_content.w, s->body_content.h},
                (cui_scroll_options){.vertical = true, .scrollbar_style = &skin});
        }
        cui_dialog_body_finish(&f, &skin);
        bar = cui_dialog_bar(&f, &skin);
    }
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_box(ui);
    bool footer = s->overlay.open && (c->present & (1u << CUI_DIALOG_FOOTER));
    if (footer) {
        cui_begin_box(ui, cui_dialog_part(&f, 12),
                      cui_dialog_slot_box(&f, CUI_DIALOG_FOOTER, r->panel.padding));
        cui_track_bounds(ui, cui_dialog_part(&f, 12), &l->completed[7]);
        cui_dialog_callback(&f, CUI_DIALOG_FOOTER, c);
        cui_end_box(ui);
    }
    if (s->overlay.open && !o.hide_close && (s->kind != CUI_SURFACE_ALERT || o.show_alert_close)) {
        cui_begin_absolute_box(ui, cui_dialog_part(&f, 19), (cui_box){0},
                               (cui_rect){fmaxf(0, width - 32), 8, 24, 24});
        f.active = true;
        (close ? close : cui_dialog_default_close)(ui, &f, close_user);
        f.active = false;
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_end_box(ui);
    cui_overlay_end(ui);
    if (!s->overlay.open) {
        /* Synchronous dismissal removed this scope's semantics. Do not recreate
         * them or register late constraints against a dismissed owner. Ordinary
         * already-emitted draw data keeps its normal frame lifetime. */
        l->private_flags = 0;
        l->private_submit_frame = 0;
        result.submitted = true;
        return result;
    }
    cui_scroll_body_layout_v2 record = {
        .base = {.struct_size = sizeof record,
                 .version = 2,
                 .mode = z->mode,
                 .panel = s->overlay.id,
                 .outer_viewport = cui_dialog_part(&f, 13),
                 .column = cui_dialog_part(&f, 14),
                 .body_frame = cui_dialog_part(&f, 15),
                 .viewport = cui_dialog_part(&f, 6),
                 .content = cui_dialog_part(&f, 7),
                 .requested_height = z->mode == CUI_SCROLL_BODY_EXPLICIT
                                         ? fminf(maximum, fmaxf(z->min_height, z->requested_height))
                                         : 0,
                 .min_height = z->min_height,
                 .max_height = maximum},
        .content_region = content ? cui_dialog_part(&f, 11) : 0,
        .footer = footer ? cui_dialog_part(&f, 12) : 0,
        .content_sizing = content ? z->content_sizing : (cui_scroll_region_sizing_v1){0},
        .body_sizing = body ? z->body_sizing : (cui_scroll_region_sizing_v1){0}};
    bool ok = cui_layout_scroll_body_v2(ui, &record, &l->outer_y, &s->body_scroll);
    if (bar && ok) {
        float thickness = isfinite(skin.thickness) && skin.thickness > 0 ? skin.thickness : 10;
        cui_scrollbar_track_layout_v1 track = {
            .struct_size = sizeof track,
            .version = 1,
            .flags = CUI_SCROLLBAR_TRACK_VISIBLE | CUI_SCROLLBAR_TRACK_INTERACTIVE,
            .viewport = cui_dialog_part(&f, 6),
            .bar = cui_scrollbar_id(cui_dialog_part(&f, 6), true),
            .thumb = cui_dialog_part(&f, 17),
            .thickness = fminf(thickness, width),
            .minimum_thumb = isfinite(skin.min_thumb) && skin.min_thumb > 0 ? skin.min_thumb : 12};
        ok = cui_layout_scrollbar_track_v1(ui, &track);
    }
    if (sequence) {
        l->private_flags = ok && body ? CUI_DIALOG_PREVIOUS_BODY : 0;
        l->private_submit_frame = ok ? sequence : 0;
    } else
        l->private_flags &= ~CUI_DIALOG_BUSY;
    result.valid = ok && cui_last_error(ui) == CUI_OK;
    result.submitted = true;
    return result;
}
cui_dialog_submit_result cui_base_dialog_compose_v1(cui_context *ui, cui_dialog_layout_v1 *l,
                                                    cui_surface_options o,
                                                    const cui_dialog_sizing_v1 *z,
                                                    const cui_dialog_composition_v1 *c) {
    if (!cui_dialog_composition_valid_v1(c)) {
        cui_dialog_fail(ui);
        return (cui_dialog_submit_result){0};
    }
    cui_dialog_resolved r;
    if (!cui_dialog_resolve(NULL, c->refinement, &r))
        return (cui_dialog_submit_result){0};
    return cui_dialog_compose_resolved(ui, l, o, z, c, &r, NULL, NULL);
}
