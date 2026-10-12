/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_icon.h"
#include <math.h>
static cui_box cui_components_control(const cui_theme *t, bool disabled, bool selected) {
    return (cui_box){.height = t->control_height,
                     .padding = t->padding,
                     .gap = t->gap,
                     .radius = t->radius,
                     .row = true,
                     .background = selected && !disabled ? t->primary : t->surface,
                     .border = disabled ? t->muted : t->border,
                     .border_width = 1};
}
static void cui_components_focus(cui_context *ui, cui_id id, const cui_theme *t, cui_box *b) {
    if (cui_focused(ui) == id) {
        b->border = t->focus;
        b->border_width = 2;
    }
}
bool cui_button_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_str name,
                   cui_button_options options) {
    if (!ui || !t)
        return false;
    bool disabled = options.disabled || options.loading;
    cui_interaction state = cui_base_button(ui, id, name, disabled);
    if (options.variant == CUI_BUTTON_LINK)
        (void)cui_set_semantic_role(ui, id, CUI_ROLE_LINK);
    cui_box b = cui_components_control(t, disabled, false);
    float scale = options.size == CUI_CONTROL_XSMALL  ? .65f
                  : options.size == CUI_CONTROL_SMALL ? .8f
                  : options.size == CUI_CONTROL_LARGE ? 1.2f
                                                      : 1;
    b.height *= scale;
    b.padding *= scale;
    b.width = isfinite(options.width) ? options.width : 0;
    b.align_x = CUI_ALIGN_CENTER;
    b.align_y = CUI_ALIGN_CENTER;
    cui_theme_button_kind kind = options.variant == CUI_BUTTON_SECONDARY
                                     ? CUI_THEME_BUTTON_SECONDARY
                                 : options.variant == CUI_BUTTON_DANGER  ? CUI_THEME_BUTTON_DANGER
                                 : options.variant == CUI_BUTTON_WARNING ? CUI_THEME_BUTTON_WARNING
                                 : options.variant == CUI_BUTTON_SUCCESS ? CUI_THEME_BUTTON_SUCCESS
                                 : options.variant == CUI_BUTTON_INFO    ? CUI_THEME_BUTTON_INFO
                                 : options.variant == CUI_BUTTON_OUTLINE ? CUI_THEME_BUTTON_DEFAULT
                                                                         : CUI_THEME_BUTTON_PRIMARY;
    cui_theme_button_colors palette = t->buttons[kind];
    bool outlined = options.outline || options.variant == CUI_BUTTON_OUTLINE;
    bool active = state.pressed || options.selected;
    cui_color foreground = outlined ? palette.outline_foreground : palette.foreground;
    b.background = outlined ? (active          ? palette.outline_active
                               : state.hovered ? palette.outline_hover
                                               : palette.outline_normal)
                            : (active          ? palette.active
                               : state.hovered ? palette.hover
                                               : palette.normal);
    b.border = outlined ? palette.outline_border : palette.border;
    if (options.variant == CUI_BUTTON_GHOST) {
        b.background = active          ? t->buttons[CUI_THEME_BUTTON_SECONDARY].active
                       : state.hovered ? t->accent
                                       : (cui_color){0};
        foreground = state.hovered ? t->accent_foreground : t->secondary_foreground;
        b.border_width = 0;
        if (state.pressed)
            b.background = t->buttons[CUI_THEME_BUTTON_DEFAULT].active;
    } else if (options.variant == CUI_BUTTON_LINK) {
        b.background = (cui_color){0};
        b.border_width = 0;
        foreground = active ? t->link_active : state.hovered ? t->link_hover : t->link;
    }
    if (disabled) {
        b.background = outlined ? palette.outline_normal : palette.normal;
        float opacity = outlined ? .5f : kind == CUI_THEME_BUTTON_SECONDARY ? 1.5f : .15f;
        b.background.a = (uint8_t)fminf(255, b.background.a * opacity);
        b.border.a = (uint8_t)(b.border.a * .5f);
        if (options.variant == CUI_BUTTON_GHOST || options.variant == CUI_BUTTON_LINK)
            b.background.a = 0;
    }
    cui_components_focus(ui, id, t, &b);
    /* Selected paint is distinct from an accessibility toggle state. */
    cui_update_semantic(ui, id, false, 0);
    if (options.loading)
        cui_set_semantic_details(
            ui, id, (cui_semantic_details){.description = cui_string("Loading"), .busy = true});
    cui_begin_box(ui, id, b);
    foreground = disabled ? t->muted : foreground;
    if (options.loading)
        (void)cui_icon_spinner(
            ui, id ^ UINT64_C(0x7185e28e9fb974db), t,
            (cui_icon_options){.size = options.size == CUI_CONTROL_XSMALL  ? CUI_ICON_XSMALL
                                       : options.size == CUI_CONTROL_SMALL ? CUI_ICON_SMALL
                                       : options.size == CUI_CONTROL_LARGE ? CUI_ICON_LARGE
                                                                           : CUI_ICON_MEDIUM,
                               .custom_color = true,
                               .color = foreground});
    else if (options.leading.length)
        cui_text(ui, options.leading, t->font_size * scale, foreground);
    if (options.content)
        options.content(ui, options.content_user);
    else
        cui_text(ui, name, t->font_size * scale, foreground);
    if (options.trailing.length)
        cui_text(ui, options.trailing, t->font_size * scale, foreground);
    cui_end_box(ui);
    return state.activated;
}
bool cui_button(cui_context *ui, cui_id id, const cui_theme *t, cui_str name, bool disabled) {
    return cui_button_ex(ui, id, t, name, (cui_button_options){.disabled = disabled});
}
static const unsigned char cui_components_check_pixels[16 * 16 * 4] = {
    255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255,
    0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255,
    255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255,
    255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,
    255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255,
    0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255,
    255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255,
    255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,
    255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255,
    0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255,
    255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255,
    255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,
    255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 80,  255, 255, 255,
    80,  255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255,
    255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255,
    255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 96,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255,
    0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255,
    255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255,
    255, 255, 96,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 96,  255, 255, 255, 0,
    255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255,
    0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255,
    255, 0,   255, 255, 255, 48,  255, 255, 255, 239, 255, 255, 255, 255, 255, 255, 255, 128, 255,
    255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,
    255, 255, 255, 207, 255, 255, 255, 207, 255, 255, 255, 16,  255, 255, 255, 0,   255, 255, 255,
    0,   255, 255, 255, 0,   255, 255, 255, 48,  255, 255, 255, 239, 255, 255, 255, 255, 255, 255,
    255, 159, 255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255,
    255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 207, 255, 255, 255, 255, 255, 255, 255, 207,
    255, 255, 255, 16,  255, 255, 255, 0,   255, 255, 255, 48,  255, 255, 255, 239, 255, 255, 255,
    255, 255, 255, 255, 159, 255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255,
    255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 16,  255,
    255, 255, 207, 255, 255, 255, 255, 255, 255, 255, 207, 255, 255, 255, 32,  255, 255, 255, 239,
    255, 255, 255, 255, 255, 255, 255, 159, 255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255,
    0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255,
    255, 0,   255, 255, 255, 0,   255, 255, 255, 16,  255, 255, 255, 207, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 207, 255, 255, 255, 16,  255, 255, 255, 0,
    255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255,
    0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255,
    255, 16,  255, 255, 255, 207, 255, 255, 255, 255, 255, 255, 255, 207, 255, 255, 255, 16,  255,
    255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,
    255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255,
    0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 16,  255, 255, 255, 128, 255, 255,
    255, 16,  255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255,
    255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,
    255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255,
    0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255,
    255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255,
    255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,
    255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255,
    0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255,
    255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255,
    255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,
    255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255,
    0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,   255, 255, 255, 0,
};
void cui_checkmark(cui_context *ui, float size, cui_color color) {
    if (!ui || !isfinite(size) || size <= 0)
        return;
    cui_paint_bitmap(ui, 0,
                     (cui_bitmap_paint){.bitmap = {cui_components_check_pixels, 16, 16, 64, 1},
                                        .opacity = 1,
                                        .custom_tint = true,
                                        .tint = color},
                     (cui_box){.width = size, .height = size});
}
bool cui_checkbox_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_str label,
                     cui_check_state *value, cui_checkbox_options options) {
    if (!ui || !t || !value)
        return false;
    cui_str name = options.accessibility_label.length ? options.accessibility_label : label;
    bool changed = cui_base_check(ui, id, name, value, options.disabled);
    cui_set_semantic_details(
        ui, id, (cui_semantic_details){.required = options.required, .invalid = options.invalid});
    cui_set_tab_stop(ui, id, !options.skip_tab);
    float density = t->font_size / 16;
    float size = (options.size == CUI_CONTROL_XSMALL  ? 12
                  : options.size == CUI_CONTROL_SMALL ? 14
                  : options.size == CUI_CONTROL_LARGE ? 18
                                                      : 16) *
                 density;
    float font = (options.size == CUI_CONTROL_XSMALL  ? 12
                  : options.size == CUI_CONTROL_LARGE ? 16
                                                      : 14) *
                 density;
    bool selected = *value != CUI_CHECK_UNCHECKED;
    cui_color border = options.invalid ? t->danger : selected ? t->primary : t->input;
    cui_color background = selected ? t->primary : t->input_background;
    cui_color foreground = t->on_primary;
    if (options.disabled) {
        border.a /= 2;
        if (selected)
            background.a /= 2;
        foreground.a /= 2;
    }
    cui_box outer = {.row = true, .gap = 8 * density, .radius = t->radius * .5f};
    if (!options.hide_focus_ring && cui_focused(ui) == id) {
        outer.border = t->focus;
        outer.border_width = 2;
    }
    cui_begin_box(ui, id, outer);
    cui_begin_box(ui, 0, (cui_box){.width = size, .gap = 0});
    if (label.length || options.content) {
        cui_begin_box(ui, 0, (cui_box){.height = size * .125f});
        cui_end_box(ui);
    }
    cui_begin_box(ui, 0,
                  (cui_box){.width = size,
                            .height = size,
                            .background = background,
                            .border = border,
                            .border_width = 1,
                            .radius = fminf(t->radius, 4 * density),
                            .align_x = CUI_ALIGN_CENTER,
                            .align_y = CUI_ALIGN_CENTER});
    if (*value == CUI_CHECK_CHECKED) {
        cui_checkmark(ui, size - 4 * density, foreground);
    } else if (*value == CUI_CHECK_MIXED) {
        cui_begin_box(ui, 0,
                      (cui_box){.width = size - 6 * density,
                                .height = 2 * density,
                                .radius = density,
                                .background = foreground});
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_end_box(ui);
    if (label.length || options.content) {
        cui_begin_box(ui, 0, (cui_box){.gap = 4 * density});
        if (label.length)
            cui_text(ui, label, font, options.disabled ? t->muted : t->text);
        if (options.content)
            options.content(ui, options.content_user);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    return changed;
}
bool cui_checkbox(cui_context *ui, cui_id id, const cui_theme *t, cui_str name, bool *value,
                  bool disabled) {
    if (!value)
        return false;
    cui_check_state state = *value ? CUI_CHECK_CHECKED : CUI_CHECK_UNCHECKED;
    bool changed =
        cui_checkbox_ex(ui, id, t, name, &state, (cui_checkbox_options){.disabled = disabled});
    *value = state == CUI_CHECK_CHECKED;
    return changed;
}
static cui_id cui_components_child_id(cui_id parent, uint64_t child) {
    uint64_t x = parent + child * UINT64_C(0x9e3779b97f4a7c15);
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x ^= x >> 31;
    return x ? x : 1;
}
cui_switch_options cui_switch_options_default(void) {
    return (cui_switch_options){0};
}
cui_id cui_switch_track_id(cui_id id) {
    return cui_components_child_id(id, 5001);
}
cui_id cui_switch_thumb_id(cui_id id) {
    return cui_components_child_id(id, 5002);
}
static void cui_components_spring_step(cui_switch_state *s, double target,
                                       cui_spring_settings policy, double now, bool reduced) {
    s->next_wake_seconds = 0;
    if (!s->initialized || reduced || !policy.travel || policy.response == 0) {
        s->position = s->target = target;
        s->velocity = 0;
        s->updated_at = now;
        s->initialized = true;
        return;
    }
    if (s->position == target && s->velocity == 0) {
        s->target = target;
        return;
    }
    double dt = fmax(0, now - s->updated_at), w = 6.283185307179586 / policy.response,
           z = policy.damping;
    double y = s->position - s->target, v = s->velocity, position = y, velocity = v;
    if (z <= 1 && z * w * dt > 700) {
        position = velocity = 0;
    } else if (z < 1) {
        double wd = w * sqrt(1 - z * z), phase = fmod(dt, 6.283185307179586 / wd) * wd;
        double decay = exp(-z * w * dt), c = cos(phase), sn = sin(phase);
        position = decay * (y * c + (v + z * w * y) / wd * sn);
        velocity = decay * (v * c - (z * w * v + w * w * y) / wd * sn);
    } else if (z == 1) {
        double decay = exp(-w * dt), b = v + w * y;
        position = decay * (y + b * dt);
        velocity = decay * (v - w * b * dt);
    } else {
        double root = sqrt(z * z - 1), r1 = -w / (z + root), r2 = -w * (z + root);
        double a = (v - r2 * y) / (r1 - r2), b = y - a, e1 = exp(r1 * dt), e2 = exp(r2 * dt);
        position = a * e1 + b * e2;
        velocity = r1 * a * e1 + r2 * b * e2;
    }
    s->position = s->target + position;
    s->velocity = velocity;
    s->target = target;
    s->updated_at = fmax(s->updated_at, now);
    if (!isfinite(s->position) || !isfinite(s->velocity) ||
        (fabs(s->position - target) <= policy.epsilon && fabs(s->velocity) <= policy.epsilon)) {
        s->position = target;
        s->velocity = 0;
    } else
        s->next_wake_seconds = now + 1. / 60;
}
static void cui_components_switch_label(cui_context *ui, const cui_theme *t, cui_str label,
                                        cui_switch_options o, float font) {
    cui_begin_box(ui, 0, (cui_box){.width = o.width ? -1 : 0});
    if (o.content)
        o.content(ui, o.content_user);
    else if (label.length) {
        float line_height = o.size == CUI_CONTROL_SMALL || o.size == CUI_CONTROL_XSMALL ? 16
                            : o.size == CUI_CONTROL_LARGE                               ? 24
                                                                                        : 20;
        cui_text_with_line_height(ui, label, font, o.disabled ? t->muted : t->text, line_height);
    }
    cui_end_box(ui);
}

bool cui_switch_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_str label, bool *value,
                   cui_switch_state *state, cui_switch_options o) {
    if (!ui || !id || !t || !value || o.size < CUI_CONTROL_MEDIUM || o.size > CUI_CONTROL_XSMALL ||
        !isfinite(o.width) || !isfinite(o.now_seconds) || o.now_seconds < 0 ||
        !isfinite(t->font_size) || t->font_size <= 0 || !isfinite(t->radius) || t->radius < 0 ||
        !isfinite(t->spring_move.response) || t->spring_move.response < 0 ||
        !isfinite(t->spring_move.damping) || t->spring_move.damping < 0 ||
        !isfinite(t->spring_move.epsilon) || t->spring_move.epsilon <= 0 ||
        (label.length && !label.data) ||
        (o.accessibility_label.length && !o.accessibility_label.data) ||
        (o.tooltip.length && !o.tooltip.data))
        return false;
    cui_str name = o.accessibility_label.length ? o.accessibility_label : label;
    bool requested = *value;
    bool changed = cui_base_switch(ui, id, name, &requested, o.disabled);
    if (changed) {
        if (o.on_change)
            o.on_change(o.change_user, requested);
        else
            *value = requested;
    }
    cui_update_semantic(ui, id, *value, *value ? 1 : 0);
    cui_set_tab_stop(ui, id, !o.skip_tab);
    bool small = o.size == CUI_CONTROL_SMALL || o.size == CUI_CONTROL_XSMALL;
    float width = small ? 28 : o.size == CUI_CONTROL_LARGE ? 44 : 36;
    float height = small ? 16 : o.size == CUI_CONTROL_LARGE ? 24 : 20;
    float thumb = height - 4, target = *value ? width - thumb - 4 : 0;
    float font = (o.size == CUI_CONTROL_XSMALL  ? 12
                  : o.size == CUI_CONTROL_LARGE ? 16
                                                : 14) *
                 t->font_size / 16;
    float radius = t->radius >= 4 ? height : t->radius;
    if (state)
        cui_components_spring_step(state, target, t->spring_move, o.now_seconds, o.reduced_motion);
    float position = state ? (float)state->position : target;
    bool label_present = label.length || o.content;
    cui_begin_box(ui, id,
                  (cui_box){.width = o.width,
                            .row = true,
                            .gap = label_present ? 8 : 0,
                            .align_y = CUI_ALIGN_START});
    if (o.label_left && label_present)
        cui_components_switch_label(ui, t, label, o, font);
    cui_color background = *value ? (o.custom_color ? o.color : t->primary) : t->switch_background;
    if (o.disabled)
        background.a /= 2;
    bool focused = !o.disabled && !o.hide_focus_ring && cui_focused(ui) == id;
    cui_id track = cui_switch_track_id(id);
    cui_begin_box(ui, track,
                  (cui_box){.width = width,
                            .height = height,
                            .padding = 2,
                            .radius = radius,
                            .background = background,
                            .border = focused ? t->focus : (cui_color){0},
                            .border_width = 1,
                            .clip = true,
                            .offset_x = position,
                            .align_y = CUI_ALIGN_CENTER});
    cui_begin_box(
        ui, cui_switch_thumb_id(id),
        (cui_box){
            .width = thumb, .height = thumb, .radius = radius, .background = t->switch_thumb});
    cui_end_box(ui);
    cui_end_box(ui);
    if (!o.label_left && label_present)
        cui_components_switch_label(ui, t, label, o, font);
    cui_end_box(ui);
    if (focused) {
        cui_rect bounds = cui_bounds(ui, track);
        cui_color color = t->focus;
        color.a /= 2;
        if (bounds.w > 0) {
            cui_begin_portal(ui, cui_components_child_id(id, 5003),
                             (cui_box){.radius = radius + 3, .border = color, .border_width = 3},
                             (cui_rect){bounds.x - 3, bounds.y - 3, bounds.w + 6, bounds.h + 6}, 1);
            cui_end_box(ui);
        }
    }
    if (state && o.tooltip.length) {
        cui_hover_options tooltip = o.tooltip_options;
        tooltip.trigger = id;
        tooltip.disabled |= o.disabled;
        cui_tooltip(ui, cui_components_child_id(id, 5004), t, &state->tooltip, o.tooltip, tooltip);
    }
    if (state && state->next_wake_seconds)
        cui_invalidate(ui);
    return changed;
}
bool cui_switch(cui_context *ui, cui_id id, const cui_theme *t, cui_str name, bool *value,
                bool disabled) {
    return cui_switch_ex(ui, id, t, name, value, NULL, (cui_switch_options){.disabled = disabled});
}

cui_toggle_options cui_toggle_options_default(void) {
    return (cui_toggle_options){0};
}
static bool cui_components_toggle_valid(cui_str label, cui_toggle_options o) {
    float radii[] = {o.corner_radii.top_left, o.corner_radii.top_right, o.corner_radii.bottom_left,
                     o.corner_radii.bottom_right};
    float edges[] = {o.border_edges.left, o.border_edges.right, o.border_edges.top,
                     o.border_edges.bottom};
    for (size_t i = 0; i < 4; i++) {
        if (o.custom_corners && (!isfinite(radii[i]) || radii[i] < 0))
            return false;
        if (o.custom_border_edges && (!isfinite(edges[i]) || edges[i] < 0))
            return false;
    }
    return o.size >= CUI_CONTROL_MEDIUM && o.size <= CUI_CONTROL_XSMALL &&
           o.variant >= CUI_TOGGLE_GHOST && o.variant <= CUI_TOGGLE_OUTLINE && isfinite(o.width) &&
           (!label.length || label.data) &&
           (!o.accessibility_label.length || o.accessibility_label.data) &&
           (!o.tooltip.length || o.tooltip.data);
}

void cui_toggle_paint(cui_context *ui, cui_id id, const cui_theme *t, cui_str label, bool value,
                      cui_toggle_options o) {
    if (!ui || !id || !t || !cui_components_toggle_valid(label, o) || !isfinite(t->font_size) ||
        t->font_size <= 0 || !isfinite(t->radius) || t->radius < 0)
        return;
    bool accent = value || (!o.disabled && cui_hovered(ui, id));
    float side = o.size == CUI_CONTROL_XSMALL  ? 20
                 : o.size == CUI_CONTROL_SMALL ? 24
                 : o.size == CUI_CONTROL_LARGE ? 36
                                               : 32;
    float padding = o.size == CUI_CONTROL_XSMALL  ? 2
                    : o.size == CUI_CONTROL_SMALL ? 4
                    : o.size == CUI_CONTROL_LARGE ? 12
                                                  : 8;
    float font = (o.size == CUI_CONTROL_XSMALL  ? 12
                  : o.size == CUI_CONTROL_LARGE ? 16
                                                : 14) *
                 t->font_size / 16;
    cui_box box = {.width = o.width,
                   .height = side,
                   .radius = t->radius,
                   .background = accent                            ? t->accent
                                 : o.variant == CUI_TOGGLE_OUTLINE ? t->background
                                                                   : (cui_color){0},
                   .border = t->border,
                   .border_width = o.variant == CUI_TOGGLE_OUTLINE ? 1 : 0,
                   .align_x = CUI_ALIGN_CENTER,
                   .align_y = CUI_ALIGN_CENTER,
                   .custom_corners = o.custom_corners,
                   .corner_radii = o.corner_radii,
                   .custom_border_edges = o.custom_border_edges && o.variant == CUI_TOGGLE_OUTLINE,
                   .border_edges = o.border_edges};
    if (o.focus_ring && !o.disabled)
        cui_components_focus(ui, id, t, &box);
    cui_begin_box(ui, id, box);
    /* A zero-height minimum spacer preserves min-width without constraining arbitrary children. */
    if (o.width == 0) {
        cui_begin_box(ui, 0, (cui_box){.width = side, .height = 0, .fixed_height = true});
        cui_end_box(ui);
    }
    cui_begin_box(ui, 0, (cui_box){.row = true, .height = -1, .align_y = CUI_ALIGN_CENTER});
    cui_begin_box(ui, 0, (cui_box){.width = padding, .height = 0, .fixed_height = true});
    cui_end_box(ui);
    if (o.content)
        o.content(ui, o.content_user);
    else if (label.length)
        cui_text_with_line_height(ui, label, font, accent ? t->accent_foreground : t->text,
                                  font * 1.25f);
    cui_begin_box(ui, 0, (cui_box){.width = padding, .height = 0, .fixed_height = true});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_box(ui);
}
bool cui_toggle_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_str label, bool *value,
                   cui_hover_state *tooltip_state, cui_toggle_options o) {
    if (!ui || !id || !t || !value || !cui_components_toggle_valid(label, o) ||
        !isfinite(t->font_size) || t->font_size <= 0 || !isfinite(t->radius) || t->radius < 0)
        return false;
    cui_str name = o.accessibility_label.length ? o.accessibility_label
                   : o.tooltip.length           ? o.tooltip
                                                : label;
    bool requested = *value;
    bool changed = cui_base_toggle(ui, id, name, &requested, o.disabled);
    if (changed) {
        if (o.on_change)
            o.on_change(o.change_user, requested);
        else
            *value = requested;
    }
    cui_update_semantic(ui, id, *value, *value ? 1 : 0);
    cui_set_tab_stop(ui, id, !o.skip_tab);
    cui_toggle_paint(ui, id, t, label, *value, o);
    if (tooltip_state && o.tooltip.length) {
        cui_hover_options tooltip = o.tooltip_options;
        tooltip.trigger = id;
        tooltip.disabled |= o.disabled;
        cui_tooltip(ui, cui_components_child_id(id, 5100), t, tooltip_state, o.tooltip, tooltip);
    }
    return changed;
}
bool cui_toggle(cui_context *ui, cui_id id, const cui_theme *t, cui_str label, bool *value,
                bool disabled) {
    return cui_toggle_ex(ui, id, t, label, value, NULL, (cui_toggle_options){.disabled = disabled});
}
typedef struct {
    cui_toggle_group_style_options *options;
    size_t count, index;
    const bool *values;
} cui_components_toggle_request;
static void cui_components_toggle_group_request(void *user, bool requested) {
    cui_components_toggle_request *r = user;
    for (size_t i = 0; i < r->count; i++)
        r->options->requested_values[i] = r->values[i];
    r->options->requested_values[r->index] = requested;
    r->options->on_change(r->options->change_user, r->options->requested_values, r->count);
}
bool cui_toggle_group_ex(cui_context *ui, cui_id id, const cui_theme *t,
                         const cui_toggle_item *items, bool *values, size_t count,
                         cui_toggle_group_style_options o) {
    if (!ui || !id || !t || (count && (!items || !values)) || o.size < CUI_CONTROL_MEDIUM ||
        o.size > CUI_CONTROL_XSMALL || o.variant < CUI_TOGGLE_GHOST ||
        o.variant > CUI_TOGGLE_OUTLINE || !isfinite(o.gap) || o.gap < 0 ||
        (o.accessibility_label.length && !o.accessibility_label.data) ||
        (o.on_change &&
         (!o.requested_values || o.requested_capacity < count || o.requested_values == values)))
        return false;
    if (!isfinite(t->font_size) || t->font_size <= 0 || !isfinite(t->radius) || t->radius < 0)
        return false;
    if (o.on_change) {
        uintptr_t a = (uintptr_t)values, b = (uintptr_t)o.requested_values;
        if ((a > b ? a - b : b - a) < count * sizeof(bool))
            return false;
    }
    for (size_t i = 0; i < count; i++) {
        if (!items[i].id || items[i].id == id ||
            !cui_components_toggle_valid(items[i].label, items[i].options))
            return false;
        for (size_t j = 0; j < i; j++)
            if (items[i].id == items[j].id)
                return false;
    }
    if (o.on_change)
        for (size_t i = 0; i < count; i++)
            o.requested_values[i] = values[i];
    cui_interact(ui, id, CUI_ROLE_TOOLBAR, o.accessibility_label, o.disabled, false, 0);
    cui_set_semantic_details(ui, id,
                             (cui_semantic_details){.orientation = CUI_ORIENTATION_HORIZONTAL});
    cui_begin_box(ui, id,
                  (cui_box){.row = true,
                            .gap = o.custom_gap  ? o.gap
                                   : o.segmented ? 0
                                                 : 8});
    bool changed = false;
    for (size_t i = 0; i < count; i++) {
        cui_toggle_options child = items[i].options;
        child.size = o.size;
        child.variant = o.variant;
        child.disabled |= o.disabled;
        child.on_change = NULL;
        child.change_user = NULL;
        if (o.segmented) {
            child.custom_corners = true;
            child.corner_radii = (cui_corner_radii){.top_left = i == 0 ? t->radius : 0,
                                                    .bottom_left = i == 0 ? t->radius : 0,
                                                    .top_right = i + 1 == count ? t->radius : 0,
                                                    .bottom_right = i + 1 == count ? t->radius : 0};
            child.custom_border_edges = true;
            float border = o.variant == CUI_TOGGLE_OUTLINE ? 1 : 0;
            child.border_edges = (cui_border_edges){
                .left = i == 0 ? border : 0, .right = border, .top = border, .bottom = border};
        }
        cui_components_toggle_request request = {&o, count, i, values};
        if (o.on_change) {
            child.on_change = cui_components_toggle_group_request;
            child.change_user = &request;
        }
        changed |= cui_toggle_ex(ui, items[i].id, t, items[i].label, &values[i],
                                 items[i].tooltip_state, child);
    }
    cui_end_box(ui);
    return changed;
}

bool cui_radio_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_str name, int *value,
                  int option, cui_radio_options options) {
    if (!ui || !t || !value)
        return false;
    cui_str accessible = options.accessibility_label.length ? options.accessibility_label : name;
    bool changed = cui_base_radio(ui, id, accessible, value, option, options.disabled);
    cui_set_semantic_details(
        ui, id, (cui_semantic_details){.required = options.required, .invalid = options.invalid});
    cui_set_tab_stop(ui, id, !options.skip_tab);
    float density = t->font_size / 16;
    float size = (options.size == CUI_CONTROL_XSMALL  ? 12
                  : options.size == CUI_CONTROL_SMALL ? 14
                  : options.size == CUI_CONTROL_LARGE ? 18
                                                      : 16) *
                 density;
    float font = (options.size == CUI_CONTROL_XSMALL  ? 12
                  : options.size == CUI_CONTROL_LARGE ? 16
                                                      : 14) *
                 density;
    bool selected = *value == option;
    cui_color ink = options.invalid ? t->danger : selected ? t->primary : t->input;
    cui_color dot = t->on_primary;
    cui_color background = selected ? t->primary : t->input_background;
    if (options.disabled) {
        ink.a /= 2;
        dot.a /= 2;
        if (selected)
            background.a /= 2;
    }
    cui_box outer = {.row = true, .gap = 8 * density, .radius = t->radius * .5f};
    if (!options.hide_focus_ring && cui_focused(ui) == id) {
        outer.border = t->focus;
        outer.border_width = 2;
    }
    cui_begin_box(ui, id, outer);
    cui_begin_box(ui, 0, (cui_box){.width = size});
    if (name.length || options.content) {
        cui_begin_box(ui, 0, (cui_box){.height = size * .125f});
        cui_end_box(ui);
    }
    cui_begin_box(ui, 0,
                  (cui_box){.width = size,
                            .height = size,
                            .radius = size / 2,
                            .border_width = 1,
                            .border = ink,
                            .background = background,
                            .align_x = CUI_ALIGN_CENTER,
                            .align_y = CUI_ALIGN_CENTER});
    if (selected) {
        cui_checkmark(ui, size - 4 * density, dot);
    }
    cui_end_box(ui);
    cui_end_box(ui);
    if (name.length || options.content) {
        cui_begin_box(ui, 0, (cui_box){.gap = 4 * density});
        if (name.length)
            cui_text(ui, name, font, options.disabled ? t->muted : t->text);
        if (options.content)
            options.content(ui, options.content_user);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    return changed;
}
bool cui_radio(cui_context *ui, cui_id id, const cui_theme *t, cui_str name, int *value, int option,
               bool disabled) {
    return cui_radio_ex(ui, id, t, name, value, option, (cui_radio_options){.disabled = disabled});
}
void cui_label(cui_context *ui, const cui_theme *t, cui_str label) {
    cui_text(ui, label, t->font_size, t->text);
}
void cui_badge(cui_context *ui, cui_id id, const cui_theme *t, cui_str label) {
    cui_begin_box(ui, id, (cui_box){.padding = 4, .radius = t->radius, .background = t->primary});
    cui_text(ui, label, t->font_size * .8f, t->on_primary);
    cui_end_box(ui);
}
void cui_progress(cui_context *ui, cui_id id, const cui_theme *t, float fraction, float width) {
    if (!isfinite(fraction))
        fraction = 0;
    fraction = fraction < 0 ? 0 : fraction > 1 ? 1 : fraction;
    if (width <= 0 || !isfinite(width))
        width = 160;
    cui_begin_box(ui, id,
                  (cui_box){.width = width,
                            .height = 8,
                            .radius = t->radius,
                            .background = t->border,
                            .row = true});
    if (fraction > 0) {
        cui_id fill = id ^ UINT64_C(0x7129d4cd19342f11);
        cui_begin_box(ui, fill,
                      (cui_box){.width = width * fraction,
                                .height = 8,
                                .radius = t->radius,
                                .background = t->primary});
        cui_end_box(ui);
    }
    cui_end_box(ui);
}
bool cui_slider(cui_context *ui, cui_id id, const cui_theme *t, cui_str name, float *value,
                float min, float max, float step, bool disabled) {
    cui_box b = cui_components_control(t, disabled, false);
    b.width = 260;
    b.row = false;
    b.height = 0;
    cui_components_focus(ui, id, t, &b);
    cui_begin_box(ui, id, b);
    bool changed = cui_base_slider(ui, id, name, value, min, max, step, disabled);
    cui_text(ui, name, t->font_size, disabled ? t->muted : t->text);
    float ratio = value && isfinite(*value) && max > min ? (*value - min) / (max - min) : 0;
    cui_progress(ui, id ^ UINT64_C(0x39c1a2170384edaa), t, ratio, 240);
    cui_end_box(ui);
    return changed;
}
void cui_separator(cui_context *ui, cui_id id, const cui_theme *t, float width) {
    cui_begin_box(
        ui, id, (cui_box){.width = width != 0 ? width : 160, .height = 1, .background = t->border});
    cui_end_box(ui);
}

static void cui_components_input_span(cui_context *ui, cui_str text, const cui_theme *t,
                                      bool selected, bool disabled) {
    if (!text.length)
        return;
    cui_size extent = cui_measure_text(ui, text, t->font_size);
    cui_begin_box(ui, 0,
                  (cui_box){.width = extent.width,
                            .height = extent.height,
                            .background = selected ? t->primary : (cui_color){0}});
    cui_text(ui, text, t->font_size, disabled ? t->muted : selected ? t->on_primary : t->text);
    cui_end_box(ui);
}
static void cui_components_input_caret(cui_context *ui, const cui_theme *t, float height) {
    cui_begin_box(ui, 0, (cui_box){.width = 1, .height = height, .background = t->text});
    cui_end_box(ui);
}
cui_id cui_input_viewport_id(cui_id id) {
    return id ^ UINT64_C(0xb1642b97583fbd63);
}
void cui_input_paint(cui_context *ui, cui_id id, const cui_theme *t, cui_str name, cui_edit *edit,
                     cui_input_state *state, cui_input_options options) {
    if (!ui || !id || !t || !edit || !state)
        return;
    cui_id viewport_id = cui_input_viewport_id(id);
    cui_input_hit_geometry hit = {ui, cui_bounds(ui, viewport_id), t->font_size, state->scroll_x};
    bool multiline = cui_edit_get_state(edit).multiline;
    cui_box box = cui_components_control(t, options.disabled, false);
    box.width = options.width ? options.width : 280;
    box.height = 0;
    box.row = false;
    box.clip = true;
    if (options.invalid)
        box.border = t->danger;
    else
        cui_components_focus(ui, id, t, &box);
    cui_begin_box(ui, id, box);
    cui_text(ui, name, t->font_size * .8f, t->muted);
    cui_str text = cui_edit_get_text(edit);
    cui_edit_state selection = cui_edit_get_state(edit);
    bool focused = cui_focused(ui) == id && !options.disabled;
    if (multiline) {
        cui_text(ui, text.length ? text : cui_string(" "), t->font_size,
                 options.disabled ? t->muted : t->text);
    } else {
        float height = cui_measure_text(ui, cui_string("M"), t->font_size).height;
        float viewport_width = hit.viewport.w;
        if (box.width > 0)
            viewport_width = fmaxf(1, box.width - 2 * box.padding);
        float caret =
            cui_measure_text(ui, (cui_str){text.data, selection.cursor}, t->font_size).width;
        float total = cui_measure_text(ui, text, t->font_size).width + (focused ? 1 : 0);
        if (!isfinite(state->scroll_x) || state->scroll_x < 0)
            state->scroll_x = 0;
        if (focused && viewport_width > 0) {
            if (caret < state->scroll_x)
                state->scroll_x = caret;
            if (caret + 1 > state->scroll_x + viewport_width)
                state->scroll_x = caret + 1 - viewport_width;
        }
        state->scroll_x = fminf(state->scroll_x, fmaxf(0, total - viewport_width));
        cui_begin_box(ui, viewport_id,
                      (cui_box){.width = -1,
                                .height = height,
                                .row = true,
                                .clip = true,
                                .offset_x = -state->scroll_x});
        if (!text.length && options.placeholder.length) {
            if (focused)
                cui_components_input_caret(ui, t, height);
            cui_text(ui, options.placeholder, t->font_size, t->muted);
        } else {
            size_t first =
                selection.anchor < selection.cursor ? selection.anchor : selection.cursor;
            size_t last = selection.anchor > selection.cursor ? selection.anchor : selection.cursor;
            cui_components_input_span(ui, (cui_str){text.data, first}, t, false, options.disabled);
            if (focused && selection.cursor == first)
                cui_components_input_caret(ui, t, height);
            cui_components_input_span(ui, (cui_str){text.data + first, last - first}, t, focused,
                                      options.disabled);
            if (focused && selection.cursor != first)
                cui_components_input_caret(ui, t, height);
            cui_components_input_span(ui, (cui_str){text.data + last, text.length - last}, t, false,
                                      options.disabled);
        }
        cui_end_box(ui);
    }
    cui_end_box(ui);
}
cui_input_result cui_input_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_str name,
                              cui_edit *edit, cui_input_state *state, cui_input_options options) {
    if (!ui || !t || !edit || !state)
        return (cui_input_result){.error = CUI_EDIT_INVALID_ARGUMENT};
    cui_id viewport_id = cui_input_viewport_id(id);
    cui_input_hit_geometry hit = {ui, cui_bounds(ui, viewport_id), t->font_size, state->scroll_x};
    bool multiline = cui_edit_get_state(edit).multiline;
    cui_input_result result = cui_base_input_ex(
        ui, id, name, edit, &state->behavior,
        (cui_base_input_options){.disabled = options.disabled,
                                 .readonly = options.readonly,
                                 .required = options.required,
                                 .invalid = options.invalid,
                                 .clipboard = options.clipboard,
                                 .hit_test = multiline ? NULL : cui_base_input_hit_test,
                                 .hit_test_user = &hit});
    options.invalid = options.invalid || result.error != CUI_EDIT_OK;
    cui_input_paint(ui, id, t, name, edit, state, options);
    return result;
}

cui_input_result cui_input(cui_context *ui, cui_id id, const cui_theme *t, cui_str name,
                           cui_edit *edit, bool disabled) {
    cui_input_state state = {0};
    return cui_input_ex(ui, id, t, name, edit, &state, (cui_input_options){.disabled = disabled});
}
