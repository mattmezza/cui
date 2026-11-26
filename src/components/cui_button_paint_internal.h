/* SPDX-License-Identifier: MIT */
#ifndef CUI_BUTTON_PAINT_INTERNAL_H
#define CUI_BUTTON_PAINT_INTERNAL_H
#include "cui/cui_button_styles.h"
#include <math.h>

/* Shared by styled controls that handle interaction and geometry separately. */
static inline cui_button_style_paint cui_button_default_paint(cui_box b, const cui_theme *t,
                                                              cui_button_options o,
                                                              cui_interaction state,
                                                              cui_color *caret) {
    bool disabled = o.disabled;
    bool text = o.variant == CUI_BUTTON_TEXT;
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
    /* Selected/open use their semantic style, independently of pointer state. */
    bool selected = o.selected || o.open;
    bool pressed = state.pressed && !selected;
    bool hovered = state.hovered && !pressed && !selected;
    bool active = pressed || selected;
    cui_color foreground = outlined ? palette.outline_foreground : palette.foreground;
    cui_color caret_color = foreground;
    if (!o.custom_colors) {
        if (o.variant == CUI_BUTTON_GHOST)
            caret_color = t->secondary_foreground;
        else if (o.variant == CUI_BUTTON_LINK)
            caret_color = t->link;
        else if (o.variant == CUI_BUTTON_TEXT) {
            caret_color = t->text;
            caret_color.a = (uint8_t)(caret_color.a * .9f);
        }
    }
    caret_color.a = (uint8_t)(caret_color.a * .75f);
    b.background = outlined ? (active          ? palette.outline_active
                               : state.hovered ? palette.outline_hover
                                               : palette.outline_normal)
                            : (active          ? palette.active
                               : state.hovered ? palette.hover
                                               : palette.normal);
    b.border = outlined ? palette.outline_border : palette.border;
    if (o.variant == CUI_BUTTON_GHOST && !o.custom_colors) {
        b.background = selected && !outlined ? t->buttons[CUI_THEME_BUTTON_SECONDARY].active
                       : active              ? t->buttons[CUI_THEME_BUTTON_DEFAULT].active
                       : hovered             ? t->accent
                                             : (cui_color){0};
        if (hovered && t->dark)
            b.background.a = (uint8_t)(b.background.a * .5f);
        foreground = hovered ? t->accent_foreground : t->secondary_foreground;
    } else if (o.variant == CUI_BUTTON_LINK && !o.custom_colors) {
        b.background = (cui_color){0};
        foreground = selected && outlined ? t->link
                     : active             ? t->link_active
                     : hovered            ? t->link_hover
                                          : t->link;
    } else if (text && !o.custom_colors) {
        b.background = (cui_color){0};
        foreground = t->text;
        foreground.a = (uint8_t)(foreground.a * (selected && outlined ? .9f
                                                 : active             ? .7f
                                                 : hovered            ? 1
                                                                      : .9f));
    }
    if (!outlined && (o.variant == CUI_BUTTON_GHOST || o.variant == CUI_BUTTON_LINK || text))
        b.border_width = 0;
    if (!o.custom_colors && (o.variant == CUI_BUTTON_GHOST || o.variant == CUI_BUTTON_LINK || text))
        b.border = (cui_color){0};
    if (disabled) {
        b.background = outlined ? palette.outline_normal : palette.normal;
        float opacity = outlined ? .5f : kind == CUI_THEME_BUTTON_SECONDARY ? 1.5f : .15f;
        b.background.a = (uint8_t)fminf(255, b.background.a * opacity);
        b.border.a = (uint8_t)fminf(255, b.border.a * (outlined ? .5f : opacity));
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
    b.custom_corners = o.custom_corners;
    b.corner_radii = o.corner_radii;
    b.custom_border_edges = o.custom_border_edges;
    b.border_edges = o.border_edges;
    if (caret)
        *caret = caret_color;
    return (cui_button_style_paint){.root = b, .foreground = foreground};
}
#endif
