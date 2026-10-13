/* SPDX-License-Identifier: MIT */
#include "cui/cui_content.h"
#include "cui/cui_components.h"
#include "cui/cui_icon.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
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
    cui_color accent = o.kind == CUI_ALERT_ERROR     ? t->danger
                       : o.kind == CUI_ALERT_SUCCESS ? t->success
                       : o.kind == CUI_ALERT_WARNING ? t->warning
                                                     : t->info;
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
        const char *name = o.kind == CUI_ALERT_SUCCESS   ? "circle-check"
                           : o.kind == CUI_ALERT_WARNING ? "triangle-alert"
                           : o.kind == CUI_ALERT_ERROR   ? "circle-alert"
                                                         : "info";
        cui_icon_named(ui, cui_content_child(id, 3), t,
                       o.icon_name.length ? o.icon_name : cui_string(name),
                       (cui_icon_options){.size = CUI_ICON_CUSTOM,
                                          .custom_size = t->font_size * scale,
                                          .custom_color = true,
                                          .color = accent});
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
bool cui_avatar_image_init(cui_avatar_image *s, unsigned char *storage, size_t capacity,
                           unsigned diameter) {
    if (!s || !storage || !diameter || diameter > 512 || capacity < (size_t)diameter * diameter * 4)
        return false;
    *s = (cui_avatar_image){.storage = storage, .capacity = capacity, .diameter = diameter};
    return true;
}
bool cui_avatar_image_prepare(cui_avatar_image *s, cui_bitmap source) {
    if (!s || !s->storage || !s->diameter || s->diameter > 512 ||
        s->capacity < (size_t)s->diameter * s->diameter * 4 || !cui_bitmap_valid(source) ||
        source.width > 16384 || source.height > 16384)
        return false;
    size_t source_bytes = source.stride * source.height;
    uintptr_t a = (uintptr_t)s->storage, b = (uintptr_t)source.pixels;
    size_t bytes = (size_t)s->diameter * s->diameter * 4;
    if ((a >= b && a - b < source_bytes) || (b >= a && b - a < bytes))
        return false;
    if (s->ready && s->source.pixels == source.pixels && s->source.width == source.width &&
        s->source.height == source.height && s->source.stride == source.stride &&
        s->source.revision == source.revision)
        return true;
    double extent = fmin(source.width, source.height), scale = extent / s->diameter;
    double radius = s->diameter * .5;
    for (unsigned y = 0; y < s->diameter; y++)
        for (unsigned x = 0; x < s->diameter; x++) {
            double sx = fmax(
                0, fmin(source.width - 1, (x + .5) * scale + (source.width - extent) * .5 - .5));
            double sy = fmax(
                0, fmin(source.height - 1, (y + .5) * scale + (source.height - extent) * .5 - .5));
            unsigned x0 = (unsigned)sx, y0 = (unsigned)sy;
            unsigned x1 = x0 + 1 < source.width ? x0 + 1 : x0,
                     y1 = y0 + 1 < source.height ? y0 + 1 : y0;
            double fx = sx - x0, fy = sy - y0;
            double coverage =
                fmax(0, fmin(1, radius + .5 - hypot(x + .5 - radius, y + .5 - radius)));
            unsigned char *out = s->storage + ((size_t)y * s->diameter + x) * 4;
            for (unsigned c = 0; c < 4; c++) {
                double top = source.pixels[(size_t)y0 * source.stride + x0 * 4 + c] * (1 - fx) +
                             source.pixels[(size_t)y0 * source.stride + x1 * 4 + c] * fx;
                double bottom = source.pixels[(size_t)y1 * source.stride + x0 * 4 + c] * (1 - fx) +
                                source.pixels[(size_t)y1 * source.stride + x1 * 4 + c] * fx;
                out[c] = (unsigned char)floor(
                    (top * (1 - fy) + bottom * fy) * (c == 3 ? coverage : 1) + .5);
            }
        }
    uint64_t revision = s->bitmap.revision + 1;
    s->bitmap = (cui_bitmap){s->storage, s->diameter, s->diameter, s->diameter * 4,
                             revision ? revision : 1};
    s->source = source;
    s->ready = true;
    return true;
}
bool cui_avatar_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_str name,
                   cui_avatar_options o) {
    if (!ui || !id || !t)
        return false;
    float size = fminf(cui_content_positive(o.size, 48), 512);
    float ring = isfinite(o.ring_width) ? fmaxf(0, fminf(o.ring_width, size * .25f)) : 0;
    bool photo = o.image_state && cui_avatar_image_prepare(o.image_state, o.image);
    cui_begin_box(
        ui, id,
        (cui_box){.width = size,
                  .height = size,
                  .fixed_width = true,
                  .fixed_height = true,
                  .radius = size * .5f,
                  .padding = photo ? ring : 0,
                  .align_x = CUI_ALIGN_CENTER,
                  .align_y = CUI_ALIGN_CENTER,
                  .background = photo ? (o.custom_ring ? o.ring : t->surface) : t->primary,
                  .border = cui_focused(ui) == id ? t->primary : t->border,
                  .border_width = photo ? 0 : 1});
    cui_interaction interaction =
        cui_interact(ui, id, o.interactive ? CUI_ROLE_BUTTON : CUI_ROLE_IMAGE,
                     name.length ? name : cui_string("Anonymous"), o.disabled, false, 0);
    if (interaction.focus_visible)
        cui_set_outline(ui, 2, t->primary,
                        (cui_corner_radii){size / 2, size / 2, size / 2, size / 2});
    if (photo)
        cui_paint_bitmap(ui, cui_content_child(id, 31),
                         (cui_bitmap_paint){.bitmap = o.image_state->bitmap, .opacity = 1},
                         (cui_box){.width = size - 2 * ring,
                                   .height = size - 2 * ring,
                                   .fixed_width = true,
                                   .fixed_height = true});
    else {
        char initials[9];
        size_t n = cui_avatar_initials(name, initials);
        cui_text(ui, (cui_str){initials, n}, size * .38f, t->on_primary);
    }
    cui_end_box(ui);
    size_t count;
    const cui_semantic *items = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; i++)
        if (items[i].id == id) {
            cui_semantic_details d = items[i].details;
            d.pointer_activatable = o.interactive;
            d.focusable = o.interactive;
            d.no_focus = !o.interactive;
            cui_set_semantic_details(ui, id, d);
            cui_set_tab_stop(ui, id, o.interactive);
            if (o.interactive)
                cui_set_cursor(ui, id, o.disabled ? CUI_CURSOR_NOT_ALLOWED : CUI_CURSOR_POINTER);
            break;
        }
    bool activated = false;
    size_t n;
    cui_frame_events(ui, &n);
    for (size_t i = 0; i < n; i++)
        if (o.interactive && !o.disabled && !cui_event_consumed(ui, i) &&
            cui_event_activation(ui, i) == id) {
            cui_consume_event(ui, i);
            activated = true;
        }
    return activated;
}
size_t cui_avatar_stack(cui_context *ui, cui_id id, const cui_theme *t,
                        const cui_avatar_item *items, size_t count, cui_avatar_stack_options o) {
    if (!ui || !id || !t || !count || count > 64 || !items)
        return SIZE_MAX;
    if (isfinite(o.width) && o.width > 0 && o.width < 1)
        return SIZE_MAX;
    float size = fminf(cui_content_positive(o.size, 48), 512);
    float overlap = cui_content_positive(o.overlap, size * .25f);
    overlap = fminf(overlap, size * .75f);
    float step = size - overlap;
    size_t visible = o.max_visible ? o.max_visible : 3;
    if (visible > count)
        visible = count;
    if (isfinite(o.width) && o.width > 0) {
        size = fminf(size, o.width);
        overlap = fminf(overlap, size * .75f);
        step = size - overlap;
        size_t slots = 1 + (size_t)fminf(64, fmaxf(0, floorf((o.width - size) / step)));
        if (visible + (visible < count ? 1 : 0) > slots)
            visible = count <= slots ? count : slots - 1;
    }
    size_t slots = visible + (visible < count ? 1 : 0);
    float width = size + step * (float)(slots - 1);
    cui_begin_box(
        ui, id,
        (cui_box){.width = width, .height = size, .fixed_width = true, .fixed_height = true});
    cui_content_semantic(ui, id, CUI_ROLE_GROUP, o.name.length ? o.name : cui_string("People"),
                         false, false, 0);
    size_t activated = SIZE_MAX;
    for (size_t i = 0; i < visible; i++) {
        cui_id child = id ^ (UINT64_C(0x7396dacf16000000) +
                             (items[i].key ? items[i].key : i + 1) * UINT64_C(0x9e3779b97f4a7c15));
        cui_begin_absolute_box(ui, cui_content_child(child, 32), (cui_box){0},
                               (cui_rect){step * (float)i, 0, size, size});
        if (cui_avatar_ex(ui, child, t, items[i].name,
                          (cui_avatar_options){.size = size,
                                               .ring_width = o.ring_width,
                                               .image = items[i].image,
                                               .image_state = items[i].image_state,
                                               .custom_ring = true,
                                               .ring = t->background,
                                               .interactive = o.interactive,
                                               .disabled = o.disabled}))
            activated = i;
        cui_end_box(ui);
    }
    if (visible < count) {
        char label[48], text[16];
        snprintf(label, sizeof label, "%zu more people", count - visible);
        snprintf(text, sizeof text, "+%zu", count - visible);
        cui_id child = cui_content_child(id, 99);
        cui_begin_absolute_box(ui, child,
                               (cui_box){.radius = size * .5f,
                                         .background = t->secondary,
                                         .border = t->background,
                                         .border_width = o.ring_width,
                                         .align_x = CUI_ALIGN_CENTER,
                                         .align_y = CUI_ALIGN_CENTER},
                               (cui_rect){step * (float)visible, 0, size, size});
        cui_content_semantic(ui, child, CUI_ROLE_IMAGE, cui_string(label), false, false, 0);
        cui_text(ui, cui_string(text), size * .28f, t->text);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    return activated;
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
        cui_color accent = v == CUI_BUBBLE_SUCCESS   ? t->success
                           : v == CUI_BUBBLE_WARNING ? t->warning
                           : v == CUI_BUBBLE_ERROR   ? t->danger
                                                     : t->info;
        bg = cui_content_blend(t->surface, accent, .15f);
    } else if (v == CUI_BUBBLE_GHOST || v == CUI_BUBBLE_OUTLINE)
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
typedef struct {
    cui_rect content;
    float width, placed_height, maximum, font_size, padding, gap;
    uint64_t revision;
    bool measured;
} cui_hover_card_measurement;
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
    bool intrinsic = card && (!isfinite(o.height) || o.height <= 0);
    cui_hover_card_measurement *measurement = NULL;
    float max_height = o.viewport.h;
    if (intrinsic && isfinite(o.max_height) && o.max_height > 0)
        max_height = fminf(max_height, o.max_height);
    float height =
        cui_content_positive(o.height, card ? 120 : t->font_size * 1.5f + t->padding * 2);
    if (intrinsic) {
        measurement = cui_resource_get(ui, id, UINT64_C(0x29ca7264138d9517), sizeof(*measurement),
                                       NULL, NULL);
        if (!measurement)
            return false;
        height = measurement->measured ? measurement->content.h + 2 * t->padding
                                       : 2 * t->padding + t->font_size;
        height = fminf(max_height, fmaxf(0, height));
        if (!measurement->measured || measurement->width != width ||
            measurement->maximum != max_height || measurement->font_size != t->font_size ||
            measurement->padding != t->padding || measurement->gap != t->gap ||
            measurement->revision != o.content_revision ||
            fabsf(measurement->placed_height - height) > .01f)
            cui_invalidate(ui);
        measurement->width = width;
        measurement->maximum = max_height;
        measurement->font_size = t->font_size;
        measurement->padding = t->padding;
        measurement->gap = t->gap;
        measurement->revision = o.content_revision;
        measurement->placed_height = height;
        measurement->measured = true;
    }
    cui_popup_result placement =
        cui_popup_place(anchor, (cui_size){width, height}, o.viewport, o.side, o.gap);
    if (intrinsic) {
        /* The first content measurement is not known at begin. Keep that frame
         * within the viewport while retaining its actual fit height for the
         * scheduled collision-placement redraw. */
        max_height = fminf(max_height, fmaxf(0, o.viewport.y + o.viewport.h - placement.bounds.y));
        placement.bounds.h = 0;
    }
    float padding =
        card ? t->padding
             : fminf(t->padding,
                     fmaxf(0, (height - cui_measure_text(ui, name, t->font_size).height) / 2));
    cui_begin_portal(ui, id,
                     (cui_box){.padding = padding,
                               .max_height = intrinsic ? max_height : 0,
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
    if (!cui_content_hover_panel(ui, id, t, state, name, options, true))
        return false;
    /* A clipping container permits horizontal overflow in Clay. Constrain a
     * non-scrolling inner content box so text actually reflows at card width. */
    float width = fminf(cui_content_positive(options.width, 240), options.viewport.w);
    cui_begin_box(
        ui, cui_content_child(id, 17),
        (cui_box){.width = fmaxf(0, width - 2 * t->padding), .fixed_width = true, .gap = t->gap});
    if (!isfinite(options.height) || options.height <= 0) {
        cui_hover_card_measurement *measurement = cui_resource_get(
            ui, id, UINT64_C(0x29ca7264138d9517), sizeof(*measurement), NULL, NULL);
        if (measurement)
            cui_track_bounds(ui, cui_content_child(id, 17), &measurement->content);
    }
    return true;
}
void cui_end_hover_card(cui_context *ui) {
    cui_end_box(ui);
    cui_end_box(ui);
}
