/* SPDX-License-Identifier: MIT */
#ifndef CUI_BRIDGE_DISCLOSURE_INTERNAL_H
#define CUI_BRIDGE_DISCLOSURE_INTERNAL_H
#include "cui/cui_disclosure_events.h"

_Static_assert(sizeof(cui_bridge_disclosure_config_v1) == 172 &&
                   offsetof(cui_bridge_disclosure_config_v1, width) == 16 &&
                   offsetof(cui_bridge_disclosure_config_v1, collapsed_icon) == 44,
               "Disclosure configuration V1 ABI");
_Static_assert(sizeof(cui_bridge_disclosure_trigger_style_v1) == 40 &&
                   offsetof(cui_bridge_disclosure_trigger_style_v1, hover_rgba) == 16 &&
                   offsetof(cui_bridge_disclosure_trigger_style_v1, cursor) == 36,
               "Disclosure trigger style V1 ABI");

static bool bridge_disclosure_dimension(float value) {
    return isfinite(value) && value >= 0 && value <= 4096;
}

static bool bridge_disclosure_icon(const char *name, size_t capacity) {
    const char *end = memchr(name, 0, capacity);
    return end && (end == name ||
                   cui_icon_query((cui_str){name, (size_t)(end - name)}) == CUI_ICON_EMBEDDED);
}

static bool bridge_disclosure_config_valid(const cui_bridge_disclosure_config_v1 *c,
                                           cui_bridge_kind kind) {
    const uint32_t accordion_flags =
        CUI_BRIDGE_DISCLOSURE_JOINED_V1 | CUI_BRIDGE_DISCLOSURE_BORDERED_V1 |
        CUI_BRIDGE_DISCLOSURE_SINGLE_V1 | CUI_BRIDGE_DISCLOSURE_ALLOW_COLLAPSE_V1;
    const uint32_t allowed =
        CUI_BRIDGE_DISCLOSURE_CUSTOM_CARD_V1 | CUI_BRIDGE_DISCLOSURE_CLIP_CARD_V1 |
        (kind == CUI_BRIDGE_ACCORDION ? accordion_flags : CUI_BRIDGE_DISCLOSURE_BORDERLESS_V1);
    bool custom = (c->flags & CUI_BRIDGE_DISCLOSURE_CUSTOM_CARD_V1) != 0;
    return !(c->flags & ~allowed) && c->design_size <= CUI_BRIDGE_SIZE_9XL &&
           (c->width == -1 || bridge_disclosure_dimension(c->width)) &&
           (custom ? bridge_disclosure_dimension(c->card_radius) &&
                         bridge_disclosure_dimension(c->card_border_width) &&
                         bridge_disclosure_dimension(c->card_padding) &&
                         bridge_disclosure_dimension(c->card_gap)
                   : !(c->flags & CUI_BRIDGE_DISCLOSURE_CLIP_CARD_V1)) &&
           bridge_disclosure_icon(c->collapsed_icon, sizeof c->collapsed_icon) &&
           bridge_disclosure_icon(c->expanded_icon, sizeof c->expanded_icon);
}

cui_bridge_status
cui_bridge_disclosure_configure_v1(cui_bridge_app *app, cui_bridge_node handle,
                                   const cui_bridge_disclosure_config_v1 *config) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_ACCORDION && node->kind != CUI_BRIDGE_COLLAPSIBLE)
        return CUI_BRIDGE_INVALID_STATE;
    if (!config) {
        free(node->disclosure_config);
        node->disclosure_config = NULL;
    } else {
        if (!bridge_icon_header(config, sizeof *config))
            return CUI_BRIDGE_INVALID_ARGUMENT;
        cui_bridge_disclosure_config_v1 next;
        memcpy(&next, config, sizeof next);
        if (!bridge_disclosure_config_valid(&next, node->kind))
            return CUI_BRIDGE_INVALID_ARGUMENT;
        if (!node->disclosure_config) {
            node->disclosure_config = malloc(sizeof *node->disclosure_config);
            if (!node->disclosure_config)
                return CUI_BRIDGE_OUT_OF_MEMORY;
        }
        *node->disclosure_config = next;
    }
    cui_invalidate(app->ui);
    return CUI_BRIDGE_OK;
}

cui_bridge_status
cui_bridge_disclosure_set_trigger_style_v1(cui_bridge_app *app, cui_bridge_node handle,
                                           const cui_bridge_disclosure_trigger_style_v1 *style) {
    static const cui_cursor cursors[] = {
        CUI_CURSOR_DEFAULT,
        CUI_CURSOR_POINTER,
        CUI_CURSOR_TEXT,
        CUI_CURSOR_RESIZE_HORIZONTAL,
        CUI_CURSOR_RESIZE_VERTICAL,
        CUI_CURSOR_MOVE,
        CUI_CURSOR_NOT_ALLOWED,
        CUI_CURSOR_CROSSHAIR,
        CUI_CURSOR_GRAB,
        CUI_CURSOR_GRABBING,
        CUI_CURSOR_ZOOM_IN,
        CUI_CURSOR_ZOOM_OUT,
        CUI_CURSOR_WAIT,
        CUI_CURSOR_HELP,
    };
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_COLLAPSIBLE &&
        (node->kind != CUI_BRIDGE_ITEM || !node->parent ||
         app->nodes[node->parent].kind != CUI_BRIDGE_ACCORDION))
        return CUI_BRIDGE_INVALID_STATE;
    cui_bridge_disclosure_trigger_style_v1 next = {0};
    if (style) {
        if (!bridge_icon_header(style, sizeof *style))
            return CUI_BRIDGE_INVALID_ARGUMENT;
        memcpy(&next, style, sizeof next);
    }
    const uint32_t fields = CUI_BRIDGE_DISCLOSURE_HOVER_V1 | CUI_BRIDGE_DISCLOSURE_PRESSED_V1 |
                            CUI_BRIDGE_DISCLOSURE_EXPANDED_V1 | CUI_BRIDGE_DISCLOSURE_DISABLED_V1 |
                            CUI_BRIDGE_DISCLOSURE_FOCUS_V1 | CUI_BRIDGE_DISCLOSURE_CURSOR_V1;
    bool cursor = (next.fields & CUI_BRIDGE_DISCLOSURE_CURSOR_V1) != 0;
    if (next.reserved || (next.fields & ~fields) ||
        (cursor && next.cursor >= sizeof cursors / sizeof cursors[0]))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (!next.fields) {
        free(node->disclosure_style);
        node->disclosure_style = NULL;
    } else {
        if (!node->disclosure_style) {
            node->disclosure_style = malloc(sizeof *node->disclosure_style);
            if (!node->disclosure_style)
                return CUI_BRIDGE_OUT_OF_MEMORY;
        }
        *node->disclosure_style = (cui_disclosure_trigger_style){
            .custom_hover = (next.fields & CUI_BRIDGE_DISCLOSURE_HOVER_V1) != 0,
            .custom_pressed = (next.fields & CUI_BRIDGE_DISCLOSURE_PRESSED_V1) != 0,
            .custom_expanded = (next.fields & CUI_BRIDGE_DISCLOSURE_EXPANDED_V1) != 0,
            .custom_disabled = (next.fields & CUI_BRIDGE_DISCLOSURE_DISABLED_V1) != 0,
            .custom_focus = (next.fields & CUI_BRIDGE_DISCLOSURE_FOCUS_V1) != 0,
            .hover = bridge_icon_rgba(next.hover_rgba),
            .pressed = bridge_icon_rgba(next.pressed_rgba),
            .expanded = bridge_icon_rgba(next.expanded_rgba),
            .disabled = bridge_icon_rgba(next.disabled_rgba),
            .focus = bridge_icon_rgba(next.focus_rgba),
            .custom_cursor = cursor,
            .cursor = cursor ? cursors[next.cursor] : CUI_CURSOR_DEFAULT,
        };
    }
    cui_invalidate(app->ui);
    return CUI_BRIDGE_OK;
}

static cui_disclosure_presentation
bridge_disclosure_presentation(const cui_bridge_disclosure_config_v1 *config, cui_box *card) {
    if (!config)
        return (cui_disclosure_presentation){0};
    cui_disclosure_presentation result = {
        .design_size = (cui_design_size)config->design_size,
        .width = config->width,
        .collapsed_icon = cui_string(config->collapsed_icon),
        .expanded_icon = cui_string(config->expanded_icon),
        .joined = (config->flags & CUI_BRIDGE_DISCLOSURE_JOINED_V1) != 0,
    };
    if (config->flags & CUI_BRIDGE_DISCLOSURE_CUSTOM_CARD_V1) {
        *card = (cui_box){
            .background = bridge_icon_rgba(config->card_background_rgba),
            .border = bridge_icon_rgba(config->card_border_rgba),
            .border_width = config->card_border_width,
            .radius = config->card_radius,
            .padding = config->card_padding,
            .gap = config->card_gap,
            .clip = (config->flags & CUI_BRIDGE_DISCLOSURE_CLIP_CARD_V1) != 0,
        };
        result.container_box = card;
    }
    return result;
}
#endif
