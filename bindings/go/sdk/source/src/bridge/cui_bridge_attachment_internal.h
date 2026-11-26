/* SPDX-License-Identifier: MIT */
#ifndef CUI_BRIDGE_ATTACHMENT_INTERNAL_H
#define CUI_BRIDGE_ATTACHMENT_INTERNAL_H

_Static_assert(sizeof(cui_bridge_attachment_text_v1) == 24 &&
                   offsetof(cui_bridge_attachment_text_v1, font_size) == 8 &&
                   sizeof(cui_bridge_attachment_config_v1) == 80 &&
                   offsetof(cui_bridge_attachment_config_v1, title) == 32 &&
                   offsetof(cui_bridge_attachment_config_v1, description) == 56,
               "Attachment V1 ABI");

static bool bridge_attachment_single_line(const char *text) {
    return !text || strpbrk(text, "\r\n") == NULL;
}

static bool bridge_attachment_text_valid(cui_bridge_attachment_text_v1 text, uint32_t caps) {
    const uint32_t style = CUI_BRIDGE_ATTACHMENT_TEXT_STYLE_V1;
    const uint32_t style_flags =
        CUI_BRIDGE_ATTACHMENT_TEXT_ITALIC_V1 | CUI_BRIDGE_ATTACHMENT_TEXT_MONOSPACE_V1;
    const uint32_t flags = style | style_flags | CUI_BRIDGE_ATTACHMENT_TEXT_COLOR_V1;
    if (text.reserved || (text.flags & ~flags) || !(text.font_size >= 0 && text.font_size <= 4096))
        return false;
    if (!(text.flags & style))
        return !(text.flags & style_flags);
    return text.weight <= CUI_TEXT_WEIGHT_BOLD &&
           (text.letter_spacing >= -4096 && text.letter_spacing <= 4096) &&
           (text.letter_spacing == 0 || (caps & CUI_TEXT_CAP_LETTER_SPACING));
}

cui_bridge_status
cui_bridge_attachment_configure_v1(cui_bridge_app *app, cui_bridge_node handle,
                                   const cui_bridge_attachment_config_v1 *config) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_ATTACHMENT)
        return CUI_BRIDGE_INVALID_STATE;
    if (!config) {
        free(node->attachment_config);
        node->attachment_config = NULL;
        cui_invalidate(app->ui);
        return CUI_BRIDGE_OK;
    }
    if (!bridge_icon_header(config, sizeof *config))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_attachment_config_v1 next;
    memcpy(&next, config, sizeof next);
    const uint32_t flags = CUI_BRIDGE_ATTACHMENT_VERTICAL_V1 | CUI_BRIDGE_ATTACHMENT_MEDIA_V1 |
                           CUI_BRIDGE_ATTACHMENT_INDETERMINATE_V1 |
                           CUI_BRIDGE_ATTACHMENT_TITLE_STATUS_V1 |
                           CUI_BRIDGE_ATTACHMENT_DESCRIPTION_STATUS_V1;
    const uint32_t caps = cui_text_capabilities(app->ui);
    if ((next.flags & ~flags) || next.reserved || next.reserved2 ||
        !(next.media_aspect == 0 || (next.media_aspect >= .0625f && next.media_aspect <= 16)) ||
        ((next.flags & CUI_BRIDGE_ATTACHMENT_TITLE_STATUS_V1) &&
         next.title_status > CUI_BRIDGE_ATTACHMENT_COMPLETE) ||
        ((next.flags & CUI_BRIDGE_ATTACHMENT_DESCRIPTION_STATUS_V1) &&
         next.description_status > CUI_BRIDGE_ATTACHMENT_COMPLETE) ||
        !bridge_attachment_text_valid(next.title, caps) ||
        !bridge_attachment_text_valid(next.description, caps) ||
        !bridge_attachment_single_line(node->text) || !bridge_attachment_single_line(node->detail))
        return CUI_BRIDGE_INVALID_ARGUMENT;

    if (!node->attachment_config) {
        node->attachment_config = malloc(sizeof *node->attachment_config);
        if (!node->attachment_config)
            return CUI_BRIDGE_OUT_OF_MEMORY;
    }
    *node->attachment_config = next;
    cui_invalidate(app->ui);
    return CUI_BRIDGE_OK;
}

static cui_text_style bridge_attachment_text_style(cui_bridge_attachment_text_v1 text) {
    if (!(text.flags & CUI_BRIDGE_ATTACHMENT_TEXT_STYLE_V1))
        return (cui_text_style){0};
    return (cui_text_style){
        .weight = (cui_text_weight)text.weight,
        .letter_spacing = text.letter_spacing,
        .italic = (text.flags & CUI_BRIDGE_ATTACHMENT_TEXT_ITALIC_V1) != 0,
        .monospace = (text.flags & CUI_BRIDGE_ATTACHMENT_TEXT_MONOSPACE_V1) != 0,
    };
}

static void bridge_attachment_apply_text(cui_attachment_child *child,
                                         cui_bridge_attachment_text_v1 text, bool override_status,
                                         uint32_t status) {
    child->font_size = text.font_size;
    child->custom_color = (text.flags & CUI_BRIDGE_ATTACHMENT_TEXT_COLOR_V1) != 0;
    child->color = bridge_icon_rgba(text.color_rgba);
    child->status_override = override_status;
    if (override_status)
        child->status = (cui_attachment_status)status;
}
#endif
