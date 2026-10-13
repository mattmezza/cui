/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "../base/cui_design_metrics.h"
#include "cui/cui_icon.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static bool cui_components_design(const cui_theme *theme, cui_design_size size,
                                  cui_design_metrics *metrics) {
    if (size == CUI_DESIGN_INHERIT) {
        *metrics = (cui_design_metrics){0};
        return true;
    }
    return theme && cui_design_measure(size, theme->font_size / 16, metrics);
}
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
static bool cui_button_dimension(float value) {
    return isfinite(value) && value > 0 && value <= 4096;
}
static cui_str cui_button_label(cui_context *ui, cui_button_state *state, cui_str name, float font,
                                float width) {
    if (!state || !cui_button_dimension(width) || !name.length || !name.data ||
        cui_measure_text(ui, name, font).width <= width)
        return name;
    cui_str ellipsis = cui_string("…");
    state->label[0] = 0;
    float tail = cui_measure_text(ui, ellipsis, font).width;
    if (tail > width)
        return (cui_str){state->label, 0};
    size_t prefix = 0;
    while (prefix < name.length) {
        size_t next = cui_grapheme_next(name, prefix);
        if (next <= prefix || next > sizeof(state->label) - 4 ||
            cui_measure_text(ui, (cui_str){name.data, next}, font).width + tail > width)
            break;
        prefix = next;
    }
    memcpy(state->label, name.data, prefix);
    memcpy(state->label + prefix, ellipsis.data, ellipsis.length);
    state->label[prefix + ellipsis.length] = 0;
    return (cui_str){state->label, prefix + ellipsis.length};
}
static void cui_button_paint_state(cui_context *ui, cui_id id, const cui_theme *t, cui_str name,
                                   cui_button_options o, cui_interaction state) {
    cui_design_metrics design;
    if (!ui || !t || !cui_components_design(t, o.design_size, &design))
        return;
    bool disabled = o.disabled;
    cui_box b = cui_components_control(t, disabled, false);
    float density = isfinite(t->font_size) && t->font_size > 0 ? t->font_size / 16 : 1;
    float font = (o.size == CUI_CONTROL_XSMALL  ? 12
                  : o.size == CUI_CONTROL_LARGE ? 16
                                                : 14) *
                 density;
    float padding = (o.size == CUI_CONTROL_XSMALL  ? 4
                     : o.size == CUI_CONTROL_SMALL ? 8
                     : o.size == CUI_CONTROL_LARGE ? 12
                                                   : 10) *
                    density;
    float gap = (o.size == CUI_CONTROL_XSMALL || o.size == CUI_CONTROL_SMALL ? 4 : 8) * density;
    float icon = (o.size == CUI_CONTROL_XSMALL  ? 12
                  : o.size == CUI_CONTROL_SMALL ? 14
                  : o.size == CUI_CONTROL_LARGE ? 24
                                                : 16) *
                 density;
    b.height = (o.size == CUI_CONTROL_XSMALL  ? 20
                : o.size == CUI_CONTROL_SMALL ? 24
                                              : 32) *
               density;
    if (o.design_size != CUI_DESIGN_INHERIT) {
        font = design.font_size;
        padding = design.padding_x;
        b.height = design.control_height;
        gap = design.gap;
        icon = design.indicator_size;
    }
    if (cui_button_dimension(o.height))
        b.height = o.height;
    if (cui_button_dimension(o.font_size))
        font = o.font_size;
    if (cui_button_dimension(o.icon_size))
        icon = o.icon_size;
    if (o.compact) {
        if (o.design_size != CUI_DESIGN_INHERIT)
            padding *= .8f;
        else
            padding = (o.size == CUI_CONTROL_XSMALL  ? 4
                       : o.size == CUI_CONTROL_SMALL ? 6
                                                     : 8) *
                      density;
    }
    bool text = o.variant == CUI_BUTTON_TEXT;
    if (o.variant == CUI_BUTTON_LINK || text)
        padding = 0;
    if (text && !cui_button_dimension(o.height))
        b.height = 0;
    b.padding = 0;
    b.gap = 0;
    b.width = isfinite(o.width) ? o.width : 0;
    b.fixed_width = b.width > 0;
    if (o.icon_only) {
        padding = 0;
        if (text && !b.height)
            b.height = icon;
        if (!b.width) {
            b.width = b.height;
            b.fixed_width = true;
        }
    }
    if (o.compact && !b.width && !o.icon_only && !o.content && !o.children && !o.leading_content &&
        !o.trailing_content) {
        float measured = 0;
        unsigned parts = 0;
        if (name.length) {
            measured += cui_measure_text(ui, name, font).width;
            parts++;
        }
        if (o.loading || o.icon.length) {
            measured += icon;
            parts++;
        } else if (o.leading.length) {
            measured += cui_measure_text(ui, o.leading, font).width;
            parts++;
        }
        if (o.trailing_icon.length) {
            measured += icon;
            parts++;
        } else if (o.trailing.length) {
            measured += cui_measure_text(ui, o.trailing, font).width;
            parts++;
        }
        if (o.dropdown_caret) {
            measured += icon;
            parts++;
        }
        measured += padding * 2 + (parts ? parts - 1 : 0) * gap;
        if (isfinite(measured)) {
            b.width = fmaxf(b.height, measured);
            b.fixed_width = true;
        }
    }
    b.align_x = CUI_ALIGN_CENTER;
    b.align_y = CUI_ALIGN_CENTER;
    b.clip = b.width > 0;
    if (o.rounding == CUI_BUTTON_ROUNDED_NONE)
        b.radius = 0;
    else if (o.rounding == CUI_BUTTON_ROUNDED_SMALL)
        b.radius = t->radius * .5f;
    else if (o.rounding == CUI_BUTTON_ROUNDED_LARGE)
        b.radius = t->radius * 2;
    else if (o.rounding == CUI_BUTTON_ROUNDED_CUSTOM && isfinite(o.radius) && o.radius >= 0)
        b.radius = o.radius;
    cui_theme_button_kind kind =
        o.variant == CUI_BUTTON_SECONDARY ? CUI_THEME_BUTTON_SECONDARY
        : o.variant == CUI_BUTTON_DANGER  ? CUI_THEME_BUTTON_DANGER
        : o.variant == CUI_BUTTON_WARNING ? CUI_THEME_BUTTON_WARNING
        : o.variant == CUI_BUTTON_SUCCESS ? CUI_THEME_BUTTON_SUCCESS
        : o.variant == CUI_BUTTON_INFO    ? CUI_THEME_BUTTON_INFO
        : o.variant == CUI_BUTTON_OUTLINE || o.variant == CUI_BUTTON_DEFAULT
            ? CUI_THEME_BUTTON_DEFAULT
            : CUI_THEME_BUTTON_PRIMARY;
    cui_theme_button_colors palette = o.custom_colors ? o.colors : t->buttons[kind];
    if (o.variant == CUI_BUTTON_CUSTOM && !o.custom_colors)
        palette = (cui_theme_button_colors){.foreground = t->text, .outline_foreground = t->text};
    bool outlined = o.outline || o.variant == CUI_BUTTON_OUTLINE;
    bool active = state.pressed || o.selected;
    cui_color foreground = outlined ? palette.outline_foreground : palette.foreground;
    b.background = outlined ? (active          ? palette.outline_active
                               : state.hovered ? palette.outline_hover
                                               : palette.outline_normal)
                            : (active          ? palette.active
                               : state.hovered ? palette.hover
                                               : palette.normal);
    b.border = outlined ? palette.outline_border : palette.border;
    if (o.variant == CUI_BUTTON_GHOST && !o.custom_colors) {
        b.background = active          ? t->buttons[CUI_THEME_BUTTON_SECONDARY].active
                       : state.hovered ? t->accent
                                       : (cui_color){0};
        foreground = state.hovered ? t->accent_foreground : t->secondary_foreground;
        if (state.pressed)
            b.background = t->buttons[CUI_THEME_BUTTON_DEFAULT].active;
    } else if (o.variant == CUI_BUTTON_LINK && !o.custom_colors) {
        b.background = (cui_color){0};
        foreground = active ? t->link_active : state.hovered ? t->link_hover : t->link;
    } else if (text && !o.custom_colors) {
        b.background = (cui_color){0};
        foreground = t->text;
        foreground.a = (uint8_t)(foreground.a * (active ? .7f : state.hovered ? 1 : .9f));
    }
    if (o.variant == CUI_BUTTON_GHOST || o.variant == CUI_BUTTON_LINK || text)
        b.border_width = 0;
    if (disabled) {
        b.background = outlined ? palette.outline_normal : palette.normal;
        float opacity = outlined ? .5f : kind == CUI_THEME_BUTTON_SECONDARY ? 1.5f : .15f;
        b.background.a = (uint8_t)fminf(255, b.background.a * opacity);
        b.border.a = (uint8_t)(b.border.a * .5f);
        if (o.variant == CUI_BUTTON_GHOST || o.variant == CUI_BUTTON_LINK || text)
            b.background.a = 0;
        if (o.variant == CUI_BUTTON_DEFAULT && !o.custom_colors && !outlined) {
            b.background = t->input_background;
            b.background.a /= 2;
            b.border = t->input;
            b.border.a /= 2;
        }
        foreground = t->muted;
        foreground.a /= 2;
    } else if (o.loading) {
        b.custom_opacity = true;
        b.opacity = .8f;
    }
    if (!o.hide_focus_ring && state.focus_visible) {
        b.border = t->focus;
        b.border_width = 2;
    }
    cui_begin_box(ui, id, b);
    cui_begin_box(ui, 0, (cui_box){.width = padding, .fixed_width = true, .height = 1});
    cui_end_box(ui);
    cui_begin_box(ui, 0,
                  (cui_box){.width = b.width ? -1 : 0,
                            .row = true,
                            .clip = b.width > 0,
                            .align_x = CUI_ALIGN_CENTER,
                            .align_y = CUI_ALIGN_CENTER,
                            .gap = gap});
    if (text && !o.icon_only && !o.content) {
        float minimum = fmaxf(font * 1.25f, cui_measure_text(ui, name, font).height);
        cui_begin_box(
            ui, 0,
            (cui_box){.width = 0, .fixed_width = true, .height = minimum, .fixed_height = true});
        cui_end_box(ui);
    }
    cui_icon_options artwork = {
        .size = CUI_ICON_CUSTOM, .custom_size = icon, .custom_color = true, .color = foreground};
    if (o.leading_content)
        o.leading_content(ui, o.leading_user);
    else if (o.loading) {
        if (o.loading_icon.length) {
            if (!cui_reduced_motion(ui)) {
                double phase = fmod(cui_frame_time(ui), .8) / .8;
                double eased =
                    phase < .5 ? 2 * phase * phase : 1 - (-2 * phase + 2) * (-2 * phase + 2) / 2;
                artwork.custom_transform = true;
                artwork.transform = cui_icon_rotation((float)(eased * 6.2831853071795864769));
            }
            if (cui_icon_named(ui, id ^ UINT64_C(0x7185e28e9fb974db), t, o.loading_icon, artwork) ==
                    CUI_ICON_OK &&
                !cui_reduced_motion(ui))
                cui_invalidate(ui);
        } else
            (void)cui_icon_spinner(ui, id ^ UINT64_C(0x7185e28e9fb974db), t, artwork);
    } else if (o.icon.length)
        (void)cui_icon_named(ui, id ^ UINT64_C(0x7185e28e9fb974db), t, o.icon, artwork);
    else if (o.leading.length)
        cui_text_with_line_height(ui, o.leading, font, foreground, font * 1.25f);
    if (o.content)
        o.content(ui, o.content_user);
    else if (!o.icon_only) {
        cui_str visible = cui_button_label(ui, o.state, name, font, o.label_width);
        cui_begin_box(ui, 0,
                      (cui_box){.width = cui_button_dimension(o.label_width) ? o.label_width : 0,
                                .fixed_width = cui_button_dimension(o.label_width),
                                .clip = cui_button_dimension(o.label_width),
                                .align_x = CUI_ALIGN_CENTER});
        if (cui_button_dimension(o.label_width))
            cui_text_line(ui, visible, font, foreground, font * 1.25f, (cui_text_style){0});
        else
            cui_text_with_line_height(ui, visible, font, foreground, font * 1.25f);
        if (o.variant == CUI_BUTTON_LINK && visible.length) {
            float underline = cui_measure_text(ui, visible, font).width;
            if (cui_button_dimension(o.label_width))
                underline = fminf(underline, o.label_width);
            cui_begin_box(ui, 0,
                          (cui_box){.width = underline, .height = 1, .background = foreground});
            cui_end_box(ui);
        }
        cui_end_box(ui);
    }
    if (o.children)
        o.children(ui, o.children_user);
    if (o.trailing_content)
        o.trailing_content(ui, o.trailing_user);
    else if (o.trailing_icon.length)
        (void)cui_icon_named(ui, id ^ UINT64_C(0x52a4e31), t, o.trailing_icon, artwork);
    else if (o.trailing.length)
        cui_text_with_line_height(ui, o.trailing, font, foreground, font * 1.25f);
    if (o.dropdown_caret) {
        if (b.width) {
            cui_begin_box(ui, 0, (cui_box){.width = -1});
            cui_end_box(ui);
        }
        (void)cui_icon_named(ui, id ^ UINT64_C(0x7e51d), t, cui_string("chevron-down"),
                             (cui_icon_options){.size = CUI_ICON_CUSTOM,
                                                .custom_size = icon,
                                                .custom_color = true,
                                                .color = foreground});
    }
    cui_end_box(ui);
    cui_begin_box(ui, 0, (cui_box){.width = padding, .fixed_width = true, .height = 1});
    cui_end_box(ui);
    cui_end_box(ui);
}
void cui_button_paint(cui_context *ui, cui_id id, const cui_theme *t, cui_str name,
                      cui_button_options o) {
    cui_interaction state = cui_interaction_state(ui, id);
    if (o.disabled || o.loading)
        state.hovered = state.pressed = false;
    cui_button_paint_state(ui, id, t, name, o, state);
}
bool cui_button_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_str name,
                   cui_button_options o) {
    cui_design_metrics design;
    if (!ui || !t || !cui_components_design(t, o.design_size, &design))
        return false;
    bool inert = o.disabled || o.loading;
    cui_str accessible = o.accessibility_label.length ? o.accessibility_label : name;
    cui_interaction interaction = cui_base_button(ui, id, accessible, inert);
    if (o.variant == CUI_BUTTON_LINK)
        (void)cui_set_semantic_role(ui, id, CUI_ROLE_LINK);
    cui_update_semantic(ui, id, o.toggle_semantics && o.toggled,
                        o.toggle_semantics && o.toggled ? 1 : 0);
    cui_set_semantic_details(
        ui, id,
        (cui_semantic_details){.description = o.loading ? cui_string("Loading") : (cui_str){0},
                               .busy = o.loading,
                               .no_pointer_focus = o.prevent_pointer_focus,
                               .pointer_activatable = o.prevent_pointer_focus});
    cui_set_tab_stop(ui, id, !o.skip_tab);
    if (o.custom_cursor)
        (void)cui_set_cursor(ui, id, o.cursor);
    else if (!inert && (o.variant == CUI_BUTTON_LINK || o.variant == CUI_BUTTON_TEXT))
        (void)cui_set_cursor(ui, id, CUI_CURSOR_POINTER);
    if (o.state) {
        if (o.state->id != id)
            *o.state = (cui_button_state){.id = id};
        bool hovered = !inert && interaction.hovered;
        if (!inert && o.on_hover &&
            ((o.state->initialized && o.state->hovered != hovered) ||
             (!o.state->initialized && hovered)))
            o.on_hover(o.action_user, hovered);
        o.state->hovered = hovered;
        o.state->initialized = true;
        o.state->next_wake_seconds = 0;
    }
    if (interaction.activated && o.on_click)
        o.on_click(o.action_user);
    cui_button_paint_state(ui, id, t, name, o, interaction);
    if (o.state && o.tooltip.length) {
        cui_hover_options hover = o.tooltip_options;
        hover.trigger = id;
        hover.dt = cui_frame_delta(ui);
        hover.disabled = inert;
        size_t count = 0;
        const cui_event *events = cui_frame_events(ui, &count);
        for (size_t i = 0; i < count; i++)
            if (events[i].type == CUI_EVENT_CANCEL && cui_event_target(ui, i) == id)
                o.state->tooltip = (cui_hover_state){.suppressed = true};
        cui_tooltip(ui, id ^ UINT64_C(0x83a46d), t, &o.state->tooltip, o.tooltip, hover);
        double now = cui_frame_time(ui);
        if (o.state->tooltip.pending && isfinite(now) && now <= 1e12)
            o.state->next_wake_seconds = now + 1.0 / 60;
    } else if (o.state)
        o.state->tooltip = (cui_hover_state){0};
    return interaction.activated;
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
static cui_color cui_checkbox_mix(cui_color from, cui_color to, float amount) {
    return (cui_color){
        (uint8_t)(from.r + (to.r - from.r) * amount), (uint8_t)(from.g + (to.g - from.g) * amount),
        (uint8_t)(from.b + (to.b - from.b) * amount), (uint8_t)(from.a + (to.a - from.a) * amount)};
}
bool cui_checkbox_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_str label,
                     cui_check_state *value, cui_checkbox_options options) {
    cui_design_metrics design;
    if (!ui || !t || !value || !cui_components_design(t, options.design_size, &design))
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
    if (options.design_size != CUI_DESIGN_INHERIT) {
        size = design.indicator_size;
        font = design.font_size;
    }
    float checked = *value == CUI_CHECK_CHECKED ? 1 : 0;
    float mixed = *value == CUI_CHECK_MIXED ? 1 : 0;
    if (options.state) {
        cui_checkbox_state *state = options.state;
        if (state->id != id) {
            *state = (cui_checkbox_state){.id = id};
        }
        bool reduced = options.reduced_motion || cui_reduced_motion(ui);
        cui_disclosure_motion_step_ex(&state->checked, checked != 0, cui_frame_delta(ui),
                                      t->spring_control, reduced);
        cui_disclosure_motion_step_ex(&state->mixed, mixed != 0, cui_frame_delta(ui),
                                      t->spring_control, reduced);
        /* Extreme finite spring settings can exceed float storage after integration. */
        if (!isfinite(state->checked.progress) || !isfinite(state->checked.velocity)) {
            state->checked.progress = checked;
            state->checked.velocity = 0;
        }
        if (!isfinite(state->mixed.progress) || !isfinite(state->mixed.velocity)) {
            state->mixed.progress = mixed;
            state->mixed.velocity = 0;
        }
        bool active = state->checked.progress != checked || state->checked.velocity != 0 ||
                      state->mixed.progress != mixed || state->mixed.velocity != 0;
        double now = cui_frame_time(ui);
        state->next_wake_seconds = active && isfinite(now) && now <= 1e12 ? now + 1.0 / 60 : 0;
        if (state->next_wake_seconds)
            cui_invalidate(ui);
        checked = fminf(1, fmaxf(0, state->checked.progress));
        mixed = fminf(1, fmaxf(0, state->mixed.progress));
    }
    bool selected = *value != CUI_CHECK_UNCHECKED;
    cui_color border = options.invalid ? t->danger : selected ? t->primary : t->input;
    cui_color background = selected ? t->primary : t->input_background;
    cui_color foreground = t->on_primary;
    if (options.state) {
        float visible = fminf(1, checked + mixed);
        background = cui_checkbox_mix(t->input_background, t->primary, visible);
        if (!options.invalid)
            border = cui_checkbox_mix(t->input, t->primary, visible);
    }
    if (options.disabled) {
        border.a /= 2;
        if (selected)
            background.a /= 2;
        foreground.a /= 2;
    }
    bool bounded = isfinite(options.width) && options.width > 0;
    float width = bounded ? fmaxf(size + 8 * density, options.width) : 0;
    cui_box outer = {.width = width,
                     .fixed_width = bounded,
                     .row = true,
                     .gap = 8 * density,
                     .radius = t->radius * .5f};
    if (!options.hide_focus_ring && cui_focused(ui) == id) {
        outer.border = t->focus;
        outer.border_width = 2;
    }
    cui_begin_box(ui, id, outer);
    cui_begin_box(ui, 0, (cui_box){.width = size, .fixed_width = true, .gap = 0});
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
    if (options.state) {
        float glyph = fmaxf(0, size - 4 * density);
        if (checked > 0) {
            cui_color color = foreground;
            color.a = (uint8_t)(color.a * checked);
            cui_begin_absolute_box(
                ui, 0, (cui_box){0},
                (cui_rect){(size - glyph) / 2, (size - glyph) / 2, glyph, glyph});
            cui_checkmark(ui, glyph, color);
            cui_end_box(ui);
        }
        if (mixed > 0) {
            cui_color color = foreground;
            color.a = (uint8_t)(color.a * mixed);
            float bar = fmaxf(0, size - 6 * density);
            cui_begin_absolute_box(
                ui, 0, (cui_box){.background = color, .radius = density},
                (cui_rect){(size - bar) / 2, (size - 2 * density) / 2, bar, 2 * density});
            cui_end_box(ui);
        }
    } else if (*value == CUI_CHECK_CHECKED) {
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
        cui_begin_box(ui, 0,
                      (cui_box){.width = bounded ? width - size - 8 * density : 0,
                                .fixed_width = bounded,
                                .gap = 4 * density});
        if (label.length)
            cui_text(ui, label, font, options.disabled ? t->muted : t->text);
        if (options.content)
            options.content(ui, options.content_user);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    if (options.state && options.tooltip.length) {
        cui_hover_options hover = options.tooltip_options;
        hover.trigger = id;
        hover.dt = cui_frame_delta(ui);
        hover.disabled = options.disabled;
        size_t count = 0;
        const cui_event *events = cui_frame_events(ui, &count);
        for (size_t i = 0; i < count; i++)
            if (events[i].type == CUI_EVENT_CANCEL && cui_event_target(ui, i) == id)
                options.state->tooltip = (cui_hover_state){.suppressed = true};
        cui_tooltip(ui, id ^ UINT64_C(0x43c19b7), t, &options.state->tooltip, options.tooltip,
                    hover);
        if (options.state->tooltip.pending) {
            double now = cui_frame_time(ui);
            if (isfinite(now) && now <= 1e12)
                options.state->next_wake_seconds = now + 1.0 / 60;
        }
    } else if (options.state) {
        options.state->tooltip = (cui_hover_state){0};
    }
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
        if (o.design_size != CUI_DESIGN_INHERIT) {
            cui_design_metrics design;
            if (cui_components_design(t, o.design_size, &design))
                line_height = design.line_height;
        }
        cui_text_with_line_height(ui, label, font, o.disabled ? t->muted : t->text, line_height);
    }
    cui_end_box(ui);
}

bool cui_switch_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_str label, bool *value,
                   cui_switch_state *state, cui_switch_options o) {
    cui_design_metrics design;
    if (!ui || !id || !t || !value || !cui_components_design(t, o.design_size, &design) ||
        o.size < CUI_CONTROL_MEDIUM || o.size > CUI_CONTROL_XSMALL || !isfinite(o.width) ||
        !isfinite(o.now_seconds) || o.now_seconds < 0 || !isfinite(t->font_size) ||
        t->font_size <= 0 || !isfinite(t->radius) || t->radius < 0 ||
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
    float font = (o.size == CUI_CONTROL_XSMALL  ? 12
                  : o.size == CUI_CONTROL_LARGE ? 16
                                                : 14) *
                 t->font_size / 16;
    if (o.design_size != CUI_DESIGN_INHERIT) {
        height = design.indicator_size + 4;
        width = height * 1.8f;
        font = design.font_size;
    }
    float thumb = height - 4, target = *value ? width - thumb - 4 : 0;
    float radius = t->radius >= 4 ? height : t->radius;
    if (state)
        cui_components_spring_step(state, target, t->spring_move, o.now_seconds, o.reduced_motion);
    float position = state ? (float)state->position : target;
    bool label_present = label.length || o.content;
    cui_begin_box(ui, id,
                  (cui_box){.width = o.width,
                            .row = true,
                            .gap = label_present ? 8 : 0,
                            .align_y = o.design_size == CUI_DESIGN_INHERIT ? CUI_ALIGN_START
                                                                           : CUI_ALIGN_CENTER});
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
           o.design_size >= CUI_DESIGN_INHERIT && o.design_size <= CUI_DESIGN_9XL &&
           o.variant >= CUI_TOGGLE_GHOST && o.variant <= CUI_TOGGLE_OUTLINE && isfinite(o.width) &&
           (!label.length || label.data) &&
           (!o.accessibility_label.length || o.accessibility_label.data) &&
           (!o.tooltip.length || o.tooltip.data);
}

void cui_toggle_paint(cui_context *ui, cui_id id, const cui_theme *t, cui_str label, bool value,
                      cui_toggle_options o) {
    cui_design_metrics design;
    if (!ui || !id || !t || !cui_components_design(t, o.design_size, &design) ||
        !cui_components_toggle_valid(label, o) || !isfinite(t->font_size) || t->font_size <= 0 ||
        !isfinite(t->radius) || t->radius < 0)
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
    if (o.design_size != CUI_DESIGN_INHERIT) {
        side = design.control_height;
        padding = design.padding_x;
        font = design.font_size;
    }
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
                                  o.design_size == CUI_DESIGN_INHERIT ? font * 1.25f
                                                                      : design.line_height);
    cui_begin_box(ui, 0, (cui_box){.width = padding, .height = 0, .fixed_height = true});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_box(ui);
}
bool cui_toggle_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_str label, bool *value,
                   cui_hover_state *tooltip_state, cui_toggle_options o) {
    cui_design_metrics design;
    if (!ui || !id || !t || !value || !cui_components_design(t, o.design_size, &design) ||
        !cui_components_toggle_valid(label, o) || !isfinite(t->font_size) || t->font_size <= 0 ||
        !isfinite(t->radius) || t->radius < 0)
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
    cui_design_metrics design;
    if (!ui || !id || !t || !cui_components_design(t, o.design_size, &design) ||
        (count && (!items || !values)) || o.size < CUI_CONTROL_MEDIUM ||
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
        if (o.design_size != CUI_DESIGN_INHERIT)
            child.design_size = o.design_size;
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
    if (changed && o.on_change) {
        /* A controlled callback may replace siblings that were already painted.
           Publish the owner's final state now and repaint the completed group. */
        for (size_t i = 0; i < count; i++)
            cui_update_semantic(ui, items[i].id, values[i], values[i] ? 1 : 0);
        cui_invalidate(ui);
    }
    return changed;
}

bool cui_radio_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_str name, int *value,
                  int option, cui_radio_options options) {
    cui_design_metrics design;
    if (!ui || !t || !value || !cui_components_design(t, options.design_size, &design))
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
    if (options.design_size != CUI_DESIGN_INHERIT) {
        size = design.indicator_size;
        font = design.font_size;
    }
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
cui_badge_options cui_badge_defaults(void) {
    return (cui_badge_options){.maximum = 99, .size = CUI_CONTROL_MEDIUM};
}
bool cui_badge_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_badge_options o,
                  cui_button_content_fn content, void *user) {
    if (!ui || !id || !t || !cui_design_valid(o.design_size) || o.variant < CUI_BADGE_COUNT ||
        o.variant > CUI_BADGE_ICON || o.size < CUI_CONTROL_MEDIUM || o.size > CUI_CONTROL_XSMALL ||
        (o.name.length && !o.name.data) || (o.icon.length && !o.icon.data))
        return false;
    cui_design_metrics dm = {0};
    if (o.design_size && !cui_design_resolve(o.design_size, t->font_size / 16, &dm))
        return false;
    float badge_font = o.design_size ? dm.font_size * .75f : 10;
    char number[3 * sizeof(size_t) + 3];
    if (o.count > o.maximum)
        snprintf(number, sizeof(number), "%zu+", o.maximum);
    else
        snprintf(number, sizeof(number), "%zu", o.count);
    bool visible = o.variant != CUI_BADGE_COUNT || o.count > 0;
    cui_begin_box(ui, id, (cui_box){0});
    if (content)
        content(ui, user);
    if (visible) {
        float extent = o.size == CUI_CONTROL_LARGE ? 24 : o.size == CUI_CONTROL_MEDIUM ? 16 : 10;
        if (o.design_size)
            extent = dm.indicator_size;
        float x = o.variant != CUI_BADGE_COUNT ? 0
                                               : (o.size == CUI_CONTROL_LARGE    ? 1
                                                  : o.size == CUI_CONTROL_MEDIUM ? 3
                                                                                 : 4) *
                                                     strlen(number);
        float y = o.variant != CUI_BADGE_COUNT   ? 0
                  : o.size == CUI_CONTROL_LARGE  ? 2
                  : o.size == CUI_CONTROL_MEDIUM ? -3
                                                 : -4;
        cui_id overlay = id ^ UINT64_C(0x35d9258f62be9a0c);
        cui_id indicator = id ^ UINT64_C(0x2780a7c4f39251db);
        cui_begin_absolute_box(
            ui, overlay,
            (cui_box){.align_x = CUI_ALIGN_END,
                      .align_y = o.variant == CUI_BADGE_ICON ? CUI_ALIGN_END : CUI_ALIGN_START},
            (cui_rect){x, y, -1, -1});
        cui_box mark = {.background = o.custom_color ? o.color : t->danger,
                        .radius = 1024,
                        .align_x = CUI_ALIGN_CENTER,
                        .align_y = CUI_ALIGN_CENTER};
        if (o.variant == CUI_BADGE_COUNT) {
            mark.height = o.design_size ? dm.line_height * .75f + dm.padding_y : 14;
            mark.width =
                fmaxf(mark.height, cui_measure_text(ui, cui_string(number), badge_font).width +
                                       (o.design_size ? dm.padding_x : 4));
        } else {
            mark.width = mark.height = o.variant == CUI_BADGE_DOT
                                           ? (o.design_size ? dm.indicator_size * .375f : 6)
                                           : extent;
            if (o.variant == CUI_BADGE_ICON) {
                mark.border = t->background;
                mark.border_width = 1;
            }
        }
        cui_begin_box(ui, indicator, mark);
        cui_str name = o.name.length                  ? o.name
                       : o.variant == CUI_BADGE_COUNT ? cui_string(number)
                       : o.variant == CUI_BADGE_DOT   ? cui_string("Notification")
                                                      : cui_string("Status");
        cui_interact(ui, indicator, CUI_ROLE_STATUS, name, false, false, (float)o.count);
        cui_set_semantic_details(ui, indicator,
                                 (cui_semantic_details){.parent = id, .no_focus = true});
        cui_set_tab_stop(ui, indicator, false);
        if (o.variant == CUI_BADGE_COUNT)
            cui_text_with_line_height(ui, cui_string(number), badge_font,
                                      (cui_color){255, 255, 255, 255}, badge_font);
        else if (o.variant == CUI_BADGE_ICON)
            cui_icon_named(ui, indicator ^ UINT64_C(0x2c38f42991a75e63), t, o.icon,
                           (cui_icon_options){.custom_size = extent - 2,
                                              .size = CUI_ICON_CUSTOM,
                                              .color = {255, 255, 255, 255},
                                              .custom_color = true});
        cui_end_box(ui);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    return true;
}
cui_progress_options cui_progress_defaults(void) {
    return (cui_progress_options){.source_motion = true, .duration = .18f};
}
bool cui_progress_ex(cui_context *ui, cui_id id, const cui_theme *t, float fraction, float width,
                     cui_progress_options o) {
    if (!ui || !id || !t || !cui_design_valid(o.design_size) || o.size < CUI_CONTROL_MEDIUM ||
        o.size > CUI_CONTROL_XSMALL || (o.name.length && !o.name.data))
        return false;
    cui_design_metrics dm = {0};
    if (o.design_size && !cui_design_resolve(o.design_size, t->font_size / 16, &dm))
        return false;
    bool reduced = o.reduced_motion || cui_reduced_motion(ui);
    cui_progress_geometry geometry =
        cui_progress_measure(fraction, o.indeterminate, o.source_motion, o.now_seconds, reduced);
    bool moving = geometry.animating;
    if (!o.indeterminate && o.motion) {
        cui_progress_motion_result transition;
        if (!cui_progress_animate(o.motion, geometry.fraction, o.now_seconds,
                                  o.duration == 0 ? .18f : o.duration, reduced, &transition))
            return false;
        geometry.length = transition.value;
        moving = transition.active;
    }
    if (width <= 0 || !isfinite(width))
        width = 160;
    float height = isfinite(o.height) && o.height > 0 ? o.height
                   : o.size == CUI_CONTROL_XSMALL     ? 4
                   : o.size == CUI_CONTROL_SMALL      ? 6
                   : o.size == CUI_CONTROL_LARGE      ? 10
                                                      : 8;
    if (o.design_size && !(isfinite(o.height) && o.height > 0))
        height = dm.control_height * .25f;
    cui_color color = o.custom_color ? o.color : t->primary;
    cui_color track = color;
    track.a = (uint8_t)(track.a * .2f);
    float radius = t->radius == 0 ? 0 : height * .5f;
    cui_begin_box(ui, id,
                  (cui_box){.width = width,
                            .height = height,
                            .radius = radius,
                            .custom_corners = o.custom_corners,
                            .corner_radii = o.corners,
                            .background = track,
                            .row = true,
                            .clip = true});
    cui_base_progress(ui, id, o.name, geometry.fraction, o.indeterminate);
    float offset = width * geometry.start, fill_width = width * geometry.length;
    if (offset > 0) {
        cui_begin_box(ui, id ^ UINT64_C(0x1829d4cd19342f11),
                      (cui_box){.width = offset, .height = height});
        cui_end_box(ui);
    }
    if (fill_width > 0) {
        cui_begin_box(ui, id ^ UINT64_C(0x7129d4cd19342f11),
                      (cui_box){.width = fill_width,
                                .height = height,
                                .radius = radius,
                                .custom_corners = o.custom_corners,
                                .corner_radii = o.corners,
                                .background = color});
        cui_end_box(ui);
    }
    cui_end_box(ui);
    if (moving)
        cui_invalidate(ui);
    return moving;
}
void cui_progress(cui_context *ui, cui_id id, const cui_theme *t, float fraction, float width) {
    cui_progress_ex(ui, id, t, fraction, width, (cui_progress_options){0});
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
    cui_color track = t->primary;
    track.a = (uint8_t)lroundf(track.a * .2f);
    cui_begin_box(ui, id ^ UINT64_C(0x39c1a2170384edaa),
                  (cui_box){.width = 240,
                            .height = 8,
                            .fixed_height = true,
                            .clip = true,
                            .background = track,
                            .radius = t->radius == 0 ? 0 : 4});
    float fill = fmaxf(0, fminf(1, ratio)) * 240;
    if (fill > 0) {
        cui_begin_box(ui, 0,
                      (cui_box){.width = fill,
                                .height = 8,
                                .fixed_height = true,
                                .background = t->primary,
                                .radius = t->radius == 0 ? 0 : 4});
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_end_box(ui);
    return changed;
}
void cui_separator(cui_context *ui, cui_id id, const cui_theme *t, float width) {
    cui_begin_box(
        ui, id, (cui_box){.width = width != 0 ? width : 160, .height = 1, .background = t->border});
    cui_end_box(ui);
}

cui_separator_options cui_separator_defaults(void) {
    return (cui_separator_options){.length = 160};
}
bool cui_separator_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_separator_options o) {
    if (!ui || !id || !t || !isfinite(o.length) || o.length < 0 || o.length > 4096 ||
        (o.label.length && !o.label.data) || !isfinite(t->font_size) || t->font_size <= 0 ||
        t->font_size > 4096)
        return false;
    float length = o.length ? o.length : 160;
    float font = t->font_size * .75f, line_height = t->font_size;
    float label_width = o.has_label ? cui_measure_text(ui, o.label, font).width + 16 : 0;
    float label_height = o.has_label ? line_height + 8 : 0;
    if (!isfinite(label_width) || !isfinite(label_height))
        return false;
    float width = o.vertical ? fmaxf(1, label_width) : length;
    float height = o.vertical ? length : fmaxf(1, label_height);
    cui_color color = o.custom_color ? o.color : t->border;
    cui_begin_box(
        ui, id,
        (cui_box){.width = width, .height = height, .fixed_width = true, .fixed_height = true});
    float increment = o.dashed ? 6 : length;
    for (float offset = 0; offset < length; offset += increment) {
        float segment = o.dashed ? fminf(4, length - offset) : length;
        cui_rect rect = o.vertical ? (cui_rect){(width - 1) / 2, offset, 1, segment}
                                   : (cui_rect){offset, (height - 1) / 2, segment, 1};
        cui_begin_absolute_box(ui, 0, (cui_box){.background = color}, rect);
        cui_end_box(ui);
    }
    if (o.has_label) {
        cui_begin_absolute_box(ui, 0, (cui_box){.background = t->background},
                               (cui_rect){(width - label_width) / 2, (height - label_height) / 2,
                                          label_width, label_height});
        cui_begin_absolute_box(ui, 0, (cui_box){0},
                               (cui_rect){8, 4, label_width - 16, line_height});
        cui_text_line(ui, o.label, font, t->muted, line_height, (cui_text_style){0});
        cui_end_box(ui);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    return true;
}

bool cui_input_measure(cui_input_options options, float root_font_size,
                       cui_input_metrics *metrics) {
    if (!metrics || options.size < CUI_INPUT_MEDIUM || options.size > CUI_INPUT_CUSTOM ||
        !isfinite(root_font_size) || root_font_size <= 0 || root_font_size > 4096 ||
        !isfinite(options.density) || options.density < 0 || options.density > 256 ||
        !isfinite(options.height) || options.height < 0 || options.height > 1000000 ||
        !isfinite(options.padding_x) || options.padding_x < 0 || options.padding_x > 1000000 ||
        !isfinite(options.padding_y) || options.padding_y < 0 || options.padding_y > 1000000)
        return false;
    float d = options.density > 0 ? options.density : root_font_size / 16;
    float h = 32, font = 14, px = 10, py = 8, gap = 6;
    switch (options.design_size == CUI_DESIGN_INHERIT ? options.size : CUI_INPUT_MEDIUM) {
    case CUI_INPUT_XSMALL:
        h = 20;
        font = 12;
        px = 4;
        py = 0;
        break;
    case CUI_INPUT_SMALL:
        h = 24;
        px = 8;
        py = 2;
        gap = 4;
        break;
    case CUI_INPUT_LARGE:
        h = 44;
        font = 16;
        px = 12;
        py = 10;
        gap = 8;
        break;
    case CUI_INPUT_CUSTOM:
        if (!isfinite(options.custom_size) || options.custom_size <= 0 ||
            options.custom_size > 4096)
            return false;
        h = options.custom_size / d;
        font = options.custom_size * .875f / d;
        px = 8;
        py = 2;
        break;
    default:
        break;
    }
    float line_height = 20 * d;
    if (options.design_size != CUI_DESIGN_INHERIT) {
        cui_design_metrics design;
        if (!cui_design_measure(options.design_size, d, &design))
            return false;
        h = design.control_height / d;
        font = design.font_size / d;
        px = design.padding_x / d;
        py = design.padding_y / d;
        gap = design.gap / d;
        line_height = design.line_height;
    }
    *metrics =
        (cui_input_metrics){options.custom_height || options.height > 0 ? options.height : h * d,
                            font * d,
                            line_height,
                            options.custom_padding ? options.padding_x : px * d,
                            options.custom_padding ? options.padding_y : py * d,
                            gap * d};
    return true;
}
cui_id cui_input_viewport_id(cui_id id) {
    return id ^ UINT64_C(0xb1642b97583fbd63);
}
static float cui_input_alignment(float available, float text, cui_align align) {
    float extra = fmaxf(0, available - text);
    return align == CUI_ALIGN_CENTER ? extra * .5f : align == CUI_ALIGN_END ? extra : 0;
}
/* Optional visual session is owned by a UI resource, shared by reduction and
 * paint in this frame. Collection refreshes it even when a host changes its
 * font registrations without changing the editor bytes. */
typedef struct {
    cui_text_geometry geometry;
    char *text;
    size_t length;
    float size;
    bool attempted, valid;
    const cui_edit *model;
    uint64_t revision;
    size_t caret_byte;
    cui_text_affinity affinity;
    bool affinity_valid;
} cui_input_visual_cache;
static void cui_input_visual_clear(void *payload) {
    cui_input_visual_cache *cache = payload;
    cui_release_text_geometry(&cache->geometry);
    free(cache->text);
    cache->geometry = (cui_text_geometry){0};
    cache->text = NULL;
    cache->attempted = cache->valid = false;
    cache->length = 0;
    cache->size = 0;
}
static cui_input_visual_cache *cui_input_visual_resource(cui_context *ui, cui_id id) {
    if (!cui_has_text_geometry(ui))
        return NULL;
    return cui_resource_get(ui, id, UINT64_C(0x290fa458736bc921), sizeof(cui_input_visual_cache),
                            cui_input_visual_clear, cui_input_visual_clear);
}
static void cui_input_visual_bind(cui_input_visual_cache *cache, const cui_edit *edit) {
    if (!cache)
        return;
    uint64_t revision = cui_edit_get_state(edit).revision;
    if (cache->model != edit || cache->revision != revision)
        cache->affinity_valid = false;
    cache->model = edit;
    cache->revision = revision;
}
static bool cui_input_visual_acquire(cui_input_visual_cache *cache, cui_context *ui, cui_str text,
                                     float size) {
    if (!cache || text.length > 65535)
        return false;
    if (cache->attempted && cache->size == size && cache->length == text.length &&
        (!text.length || !memcmp(cache->text, text.data, text.length)))
        return cache->valid;
    cui_input_visual_clear(cache);
    cache->text = malloc(text.length + 1);
    if (!cache->text)
        return false;
    if (text.length)
        memcpy(cache->text, text.data, text.length);
    cache->text[text.length] = 0;
    cache->length = text.length;
    cache->size = size;
    cache->attempted = true;
    cache->valid =
        cui_get_text_geometry(ui, (cui_str){cache->text, text.length}, size, (cui_text_style){0},
                              &cache->geometry) == CUI_TEXT_LAYOUT_OK;
    return cache->valid;
}
typedef struct {
    cui_input_hit_geometry measured;
    cui_input_visual_cache *cache;
} cui_input_cached_hit;
static size_t cui_input_cached_hit_test(void *payload, cui_str text, float x, float y) {
    cui_input_cached_hit *hit = payload;
    if (hit->cache && hit->cache->model)
        cui_input_visual_bind(hit->cache, hit->cache->model);
    if (cui_input_visual_acquire(hit->cache, hit->measured.ui, text, hit->measured.font_size)) {
        cui_input_visual_hit_geometry visual = {hit->measured, &hit->cache->geometry,
                                                &hit->cache->affinity};
        size_t byte = cui_base_input_visual_hit_test(&visual, text, x, y);
        if (byte != SIZE_MAX) {
            hit->cache->caret_byte = byte;
            hit->cache->affinity_valid = true;
            return byte;
        }
    }
    return cui_base_input_measured_hit_test(&hit->measured, text, x, y);
}
static bool cui_input_visual_caret(const cui_text_geometry *geometry, size_t byte,
                                   cui_text_affinity preferred, float *x) {
    cui_text_caret carets[2];
    size_t count;
    if (geometry->ops->carets(geometry->session, byte, carets, 2, &count) != CUI_TEXT_LAYOUT_OK ||
        !count || count > 2)
        return false;
    size_t chosen = 0;
    for (size_t i = 0; i < count; ++i) {
        if (carets[i].byte_offset != byte || !isfinite(carets[i].x) || !isfinite(carets[i].y) ||
            !isfinite(carets[i].height) || carets[i].height < 0 ||
            (unsigned)carets[i].affinity > CUI_TEXT_DOWNSTREAM)
            return false;
        if (carets[i].affinity == preferred)
            chosen = i;
    }
    *x = carets[chosen].x;
    return true;
}
static bool cui_input_visual_selection(const cui_text_geometry *geometry, size_t start, size_t end,
                                       cui_text_selection_rect **rects, size_t *count) {
    size_t needed;
    if (geometry->ops->selection(geometry->session, start, end, NULL, 0, &needed) !=
            CUI_TEXT_LAYOUT_OK ||
        needed > 4096)
        return false;
    cui_text_selection_rect *result = needed ? malloc(needed * sizeof *result) : NULL;
    if (needed && !result)
        return false;
    size_t written;
    if (geometry->ops->selection(geometry->session, start, end, result, needed, &written) !=
            CUI_TEXT_LAYOUT_OK ||
        written != needed) {
        free(result);
        return false;
    }
    for (size_t i = 0; i < needed; ++i) {
        cui_rect bounds = result[i].bounds;
        if (!isfinite(bounds.x) || !isfinite(bounds.y) || !isfinite(bounds.w) ||
            !isfinite(bounds.h) || bounds.w < 0 || bounds.h < 0 || result[i].byte_start < start ||
            result[i].byte_end > end || result[i].byte_start > result[i].byte_end) {
            free(result);
            return false;
        }
    }
    *rects = result;
    *count = needed;
    return true;
}
void cui_input_paint(cui_context *ui, cui_id id, const cui_theme *t, cui_str name, cui_edit *edit,
                     cui_input_state *state, cui_input_options options) {
    (void)name;
    cui_input_metrics metrics;
    if (!ui || !id || !t || !edit || !state ||
        !cui_input_measure(options, t->font_size, &metrics) || !isfinite(options.width) ||
        options.text_align < CUI_ALIGN_START || options.text_align > CUI_ALIGN_END)
        return;
    bool multiline = cui_edit_get_state(edit).multiline;
    bool focused = cui_focused(ui) == id && !options.disabled;
    float border = options.no_appearance || options.no_border ? 0 : 1;
    cui_color background = t->input_background, foreground = options.disabled ? t->muted : t->text;
    if (options.disabled) {
        background = t->input;
        background.a = (uint8_t)lroundf(background.a * .2f);
        foreground.a = (uint8_t)lroundf(foreground.a * .5f);
    }
    cui_color frame_border = options.invalid                       ? t->danger
                             : focused && !options.hide_focus_ring ? t->focus
                                                                   : t->input;
    float width = options.width ? options.width : 280;
    cui_begin_box(ui, id,
                  (cui_box){.width = width,
                            .height = multiline ? 0 : metrics.height,
                            .fixed_height = !multiline,
                            .row = !multiline,
                            .align_y = CUI_ALIGN_CENTER,
                            .clip = true,
                            .radius = options.no_appearance ? 0 : t->radius,
                            .background = options.no_appearance ? (cui_color){0} : background,
                            .border = frame_border,
                            .border_width = border});
    cui_str text = cui_edit_composition_view(edit, state->behavior.composition);
    cui_edit_state selection = cui_edit_composition_selection(edit, state->behavior.composition);
    if (multiline) {
        cui_text_with_line_height(ui, text.length ? text : cui_string(" "), metrics.font_size,
                                  foreground, metrics.line_height);
        cui_end_box(ui);
        return;
    }
    float inset = metrics.padding_x + border;
    cui_id viewport_id = cui_input_viewport_id(id);
    float viewport_width = width > 0 ? fmaxf(0, width - 2 * inset) : cui_bounds(ui, viewport_id).w;
    float text_width = 0, caret = 0;
    cui_input_visual_cache *cache = cui_input_visual_resource(ui, id);
    cui_input_visual_bind(cache, edit);
    cui_text_selection_rect *visual_selection = NULL, *visual_composition = NULL;
    size_t visual_count = 0, composition_count = 0;
    size_t first = selection.anchor < selection.cursor ? selection.anchor : selection.cursor;
    size_t last = selection.anchor > selection.cursor ? selection.anchor : selection.cursor;
    bool visual = cui_input_visual_acquire(cache, ui, text, metrics.font_size);
    if (visual)
        visual =
            cui_input_visual_caret(&cache->geometry, selection.cursor,
                                   cache->affinity_valid && cache->caret_byte == selection.cursor
                                       ? cache->affinity
                                       : CUI_TEXT_DOWNSTREAM,
                                   &caret) &&
            cui_input_visual_selection(&cache->geometry, first, last, &visual_selection,
                                       &visual_count);
    if (visual && state->behavior.composition && state->behavior.composition->active) {
        cui_edit_composition *composition = state->behavior.composition;
        visual = cui_input_visual_selection(&cache->geometry, composition->start,
                                            composition->start + composition->length,
                                            &visual_composition, &composition_count);
    }
    if (visual)
        text_width = cache->geometry.width;
    else {
        free(visual_selection);
        visual_selection = NULL;
        visual_count = 0;
        free(visual_composition);
        visual_composition = NULL;
        composition_count = 0;
        text_width = cui_measure_text(ui, text, metrics.font_size).width;
        caret =
            cui_measure_text(ui, (cui_str){text.data, selection.cursor}, metrics.font_size).width;
    }

    float total = text_width + (focused ? 1 : 0);
    if (!isfinite(state->scroll_x) || state->scroll_x < 0)
        state->scroll_x = 0;
    if (focused && viewport_width > 0) {
        if (caret < state->scroll_x)
            state->scroll_x = caret;
        if (caret + 1 > state->scroll_x + viewport_width)
            state->scroll_x = caret + 1 - viewport_width;
    }
    state->scroll_x = fminf(state->scroll_x, fmaxf(0, total - viewport_width));
    cui_begin_box(ui, 0, (cui_box){.width = inset});
    cui_end_box(ui);
    cui_begin_box(ui, viewport_id,
                  (cui_box){.width = -1,
                            .height = metrics.line_height,
                            .fixed_height = true,
                            .clip = true,
                            .align_x = options.text_align,
                            .offset_x = -state->scroll_x});
    cui_begin_box(ui, id ^ UINT64_C(0xb1642b97583fbd64),
                  (cui_box){.width = fmaxf(text_width, 1),
                            .fixed_width = true,
                            .height = metrics.line_height,
                            .fixed_height = true});
    if (!visual && focused && state->behavior.composition && state->behavior.composition->active) {
        cui_edit_composition *c = state->behavior.composition;
        float left = cui_measure_text(ui, (cui_str){text.data, c->start}, metrics.font_size).width;
        float right =
            cui_measure_text(ui, (cui_str){text.data, c->start + c->length}, metrics.font_size)
                .width;
        cui_set_underlay(ui, (cui_rect){left, metrics.line_height - 1, right - left, 1}, t->caret,
                         (cui_corner_radii){0});
    }
    if (!visual && focused && first != last) {
        float left = cui_measure_text(ui, (cui_str){text.data, first}, metrics.font_size).width;
        float right = cui_measure_text(ui, (cui_str){text.data, last}, metrics.font_size).width;
        cui_set_underlay(ui, (cui_rect){left, 0, right - left, metrics.line_height}, t->selection,
                         (cui_corner_radii){0});
    }
    if (visual && focused) {
        for (size_t i = 0; i < visual_count; ++i) {
            cui_rect bounds = visual_selection[i].bounds;
            bounds.y = 0;
            bounds.h = metrics.line_height;
            cui_begin_absolute_box(ui, 0, (cui_box){.background = t->selection}, bounds);
            cui_end_box(ui);
        }
        for (size_t i = 0; i < composition_count; ++i) {
            cui_rect bounds = visual_composition[i].bounds;
            bounds.y = metrics.line_height - 1;
            bounds.h = 1;
            cui_begin_absolute_box(ui, 0, (cui_box){.background = t->caret}, bounds);
            cui_end_box(ui);
        }
    }
    free(visual_selection);
    free(visual_composition);
    if (text.length) {
        if (visual)
            cui_text_line(ui, text, metrics.font_size, foreground, metrics.line_height,
                          (cui_text_style){0});
        else
            cui_text_with_line_height(ui, text, metrics.font_size, foreground, metrics.line_height);
    } else if (options.placeholder.length) {
        cui_color placeholder = t->muted;
        if (options.disabled)
            placeholder.a = (uint8_t)lroundf(placeholder.a * .5f);
        if (visual)
            cui_text_line(ui, options.placeholder, metrics.font_size, placeholder,
                          metrics.line_height, (cui_text_style){0});
        else
            cui_text_with_line_height(ui, options.placeholder, metrics.font_size, placeholder,
                                      metrics.line_height);
    }
    if (focused) {
        cui_rect viewport = cui_bounds(ui, viewport_id);
        cui_set_text_input_rect(
            ui, id,
            (cui_rect){viewport.x + caret - state->scroll_x, viewport.y, 1, metrics.line_height});
        float height = metrics.line_height * .85f;
        cui_begin_absolute_box(ui, id ^ UINT64_C(0xb1642b97583fbd65),
                               (cui_box){.background = t->caret},
                               (cui_rect){caret, (metrics.line_height - height) * .5f, 1, height});
        cui_end_box(ui);
        if (visual)
            (void)cui_set_text_input_geometry_rect(ui, id, id ^ UINT64_C(0xb1642b97583fbd65),
                                                   metrics.line_height);
    }
    cui_end_box(ui);
    cui_end_box(ui);
    cui_begin_box(ui, 0, (cui_box){.width = inset});
    cui_end_box(ui);
    cui_end_box(ui);
}
cui_input_result cui_input_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_str name,
                              cui_edit *edit, cui_input_state *state, cui_input_options options) {
    cui_input_metrics metrics;
    if (!ui || !t || !edit || !state || !cui_input_measure(options, t->font_size, &metrics) ||
        !isfinite(options.width) || options.text_align < CUI_ALIGN_START ||
        options.text_align > CUI_ALIGN_END ||
        (options.placeholder.length && !options.placeholder.data))
        return (cui_input_result){.error = CUI_EDIT_INVALID_ARGUMENT};
    cui_id viewport_id = cui_input_viewport_id(id);
    cui_input_hit_geometry hit = {ui, cui_bounds(ui, viewport_id), metrics.font_size,
                                  state->scroll_x};
    bool multiline = cui_edit_get_state(edit).multiline;
    cui_input_cached_hit cached_hit = {hit, multiline ? NULL : cui_input_visual_resource(ui, id)};
    cui_input_visual_bind(cached_hit.cache, edit);
    cui_str committed = cui_edit_get_text(edit);
    float text_width = cui_input_visual_acquire(cached_hit.cache, ui, committed, metrics.font_size)
                           ? cached_hit.cache->geometry.width
                           : cui_measure_text(ui, committed, metrics.font_size).width;
    cached_hit.measured.viewport.x +=
        cui_input_alignment(hit.viewport.w, text_width, options.text_align);
    state->behavior.composition = cui_base_composition(ui, id, state->behavior.composition,
                                                       &state->behavior.composition_owned);
    if (state->behavior.composition && state->behavior.composition_owned)
        state->behavior.composition->grow_view = true;
    cui_input_result result = cui_base_input_ex(
        ui, id, name, edit, &state->behavior,
        (cui_base_input_options){.disabled = options.disabled,
                                 .readonly = options.readonly,
                                 .required = options.required,
                                 .invalid = options.invalid,
                                 .clipboard = options.clipboard,
                                 .validate = options.validate,
                                 .validate_user = options.validate_user,
                                 .normalization = options.normalization,
                                 .delegate_up_down = options.delegate_up_down,
                                 .hit_test = multiline ? NULL : cui_input_cached_hit_test,
                                 .hit_test_user = &cached_hit});
    options.invalid = options.invalid || result.error != CUI_EDIT_OK;
    cui_input_paint(ui, id, t, name, edit, state, options);
    return result;
}

cui_input_result cui_input(cui_context *ui, cui_id id, const cui_theme *t, cui_str name,
                           cui_edit *edit, bool disabled) {
    if (!ui || !id || !t || !edit)
        return (cui_input_result){.error = CUI_EDIT_INVALID_ARGUMENT};
    cui_input_state *state =
        cui_resource_get(ui, id, UINT64_C(0x5a2a3f8d00e3af41), sizeof(*state), NULL, NULL);
    if (!state)
        return (cui_input_result){.error = CUI_EDIT_CAPACITY};
    return cui_input_ex(ui, id, t, name, edit, state, (cui_input_options){.disabled = disabled});
}

cui_progress_circle_options cui_progress_circle_defaults(void) {
    return (cui_progress_circle_options){.progress = cui_progress_defaults(), .raster_scale = 2};
}
bool cui_progress_circle(cui_context *ui, cui_id id, const cui_theme *theme, float fraction,
                         cui_progress_circle_state *state, cui_progress_circle_options options,
                         cui_button_content_fn content, void *user) {
    cui_progress_options p = options.progress;
    if (!ui || !id || !theme || !cui_design_valid(p.design_size) || !state ||
        p.size < CUI_CONTROL_MEDIUM || p.size > CUI_CONTROL_XSMALL ||
        (p.name.length && !p.name.data))
        return false;
    cui_design_metrics dm = {0};
    if (p.design_size && !cui_design_resolve(p.design_size, theme->font_size / 16, &dm))
        return false;
    bool reduced = p.reduced_motion || cui_reduced_motion(ui);
    cui_progress_geometry progress =
        cui_progress_measure(fraction, p.indeterminate, true, p.now_seconds, reduced);
    bool moving = progress.animating;
    cui_progress_motion staged = {0};
    if (!p.indeterminate && p.motion) {
        staged = *p.motion;
        cui_progress_motion_result transition;
        if (!cui_progress_animate(&staged, progress.fraction, p.now_seconds,
                                  p.duration == 0 ? .18f : p.duration, reduced, &transition))
            return false;
        progress.length = transition.value;
        moving = transition.active;
    }
    float diameter = options.diameter == 0 ? (p.size == CUI_CONTROL_XSMALL  ? 8
                                              : p.size == CUI_CONTROL_SMALL ? 12
                                              : p.size == CUI_CONTROL_LARGE ? 20
                                                                            : 16)
                                           : options.diameter;
    if (p.design_size && options.diameter == 0)
        diameter = dm.control_height;
    cui_progress_circle_geometry geometry;
    cui_bitmap bitmap;
    if (!cui_progress_circle_measure(progress, diameter, &geometry) ||
        !cui_progress_circle_raster(geometry, p.custom_color ? p.color : theme->primary,
                                    options.raster_scale == 0 ? 2 : options.raster_scale,
                                    state->pixels, state->capacity, &bitmap))
        return false;
    if (!p.indeterminate && p.motion)
        *p.motion = staged;
    if (++state->revision == 0)
        ++state->revision;
    bitmap.revision = state->revision;
    cui_begin_box(ui, id,
                  (cui_box){.width = diameter,
                            .height = diameter,
                            .fixed_height = true,
                            .align_x = CUI_ALIGN_CENTER,
                            .align_y = CUI_ALIGN_CENTER});
    cui_base_progress(ui, id, p.name, fraction, p.indeterminate);
    cui_set_bitmap_underlay(ui, (cui_underlay_layout){.grow_width = true, .grow_height = true},
                            (cui_bitmap_paint){.bitmap = bitmap, .opacity = 1},
                            (cui_corner_radii){0});
    if (content)
        content(ui, user);
    cui_end_box(ui);
    if (moving)
        cui_invalidate(ui);
    return moving;
}
