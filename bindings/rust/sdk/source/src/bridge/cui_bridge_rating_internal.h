/* SPDX-License-Identifier: MIT */
#ifndef CUI_BRIDGE_RATING_INTERNAL_H
#define CUI_BRIDGE_RATING_INTERNAL_H

_Static_assert(sizeof(cui_bridge_rating_config_v1) == 48 &&
                   offsetof(cui_bridge_rating_config_v1, width) == 16 &&
                   offsetof(cui_bridge_rating_config_v1, outline_icon) == 32 &&
                   offsetof(cui_bridge_rating_config_v1, fill_icon) == 40,
               "Rating configuration V1 ABI");

struct bridge_rating_configured {
    cui_bridge_rating_config_v1 config;
    bridge_icon_owned *outline, *fill;
};

static void bridge_rating_release(const bridge_rating_configured *state) {
    if (state->outline)
        --state->outline->references;
    if (state->fill)
        --state->fill->references;
}

static void bridge_rating_remove(cui_bridge_node_record *node) {
    if (node->rating) {
        bridge_rating_release(node->rating);
        free(node->rating);
        node->rating = NULL;
    }
}

uint32_t cui_bridge_rating_config_size_v1(void) {
    return sizeof(cui_bridge_rating_config_v1);
}

static cui_bridge_status bridge_rating_node(cui_bridge_app *app, cui_bridge_node handle,
                                            cui_bridge_node_record **node) {
    cui_bridge_status status = bridge_valid_node(app, handle, node);
    if (status != CUI_BRIDGE_OK)
        return status;
    return (*node)->kind == CUI_BRIDGE_RATING ? CUI_BRIDGE_OK : CUI_BRIDGE_INVALID_STATE;
}

static cui_bridge_status bridge_rating_mask(cui_bridge_app *app, cui_bridge_icon_handle_v1 handle,
                                            bridge_icon_owned **resource) {
    if (!handle.slot && !handle.generation)
        return CUI_BRIDGE_OK;
    if (!handle.slot || !handle.generation)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    return bridge_icon_get(app, handle, resource);
}

cui_bridge_status cui_bridge_rating_configure_v1(cui_bridge_app *app, cui_bridge_node handle,
                                                 const cui_bridge_rating_config_v1 *config) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_rating_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!config) {
        bridge_rating_remove(node);
        cui_invalidate(app->ui);
        return CUI_BRIDGE_OK;
    }
    if (!bridge_icon_header(config, sizeof *config))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    bridge_rating_configured next = {0};
    memcpy(&next.config, config, sizeof next.config);
    const cui_bridge_rating_config_v1 *c = &next.config;
    const uint32_t flags = CUI_BRIDGE_RATING_HALF_STEPS_V1 | CUI_BRIDGE_RATING_HOVER_PREVIEW_V1 |
                           CUI_BRIDGE_RATING_STEP_DOWN_FILLED_V1 | CUI_BRIDGE_RATING_CLEARABLE_V1 |
                           CUI_BRIDGE_RATING_READONLY_V1 | CUI_BRIDGE_RATING_CUSTOM_FILL_V1 |
                           CUI_BRIDGE_RATING_CUSTOM_OUTLINE_V1 |
                           CUI_BRIDGE_RATING_CUSTOM_PREVIEW_V1;
    if ((c->flags & ~flags) || c->design_size > CUI_BRIDGE_SIZE_9XL || !isfinite(c->width) ||
        (c->width != 0 && (c->width < 1 || c->width > 4096)) ||
        (c->fill_rgba && !(c->flags & CUI_BRIDGE_RATING_CUSTOM_FILL_V1)) ||
        (c->outline_rgba && !(c->flags & CUI_BRIDGE_RATING_CUSTOM_OUTLINE_V1)) ||
        (c->preview_rgba && !(c->flags & CUI_BRIDGE_RATING_CUSTOM_PREVIEW_V1)))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    status = bridge_rating_mask(app, c->outline_icon, &next.outline);
    if (status == CUI_BRIDGE_OK)
        status = bridge_rating_mask(app, c->fill_icon, &next.fill);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_rating_configured *owned = node->rating;
    if (owned)
        bridge_rating_release(owned);
    else {
        owned = malloc(sizeof *owned);
        if (!owned)
            return CUI_BRIDGE_OUT_OF_MEMORY;
        node->rating = owned;
    }
    if (next.outline)
        ++next.outline->references;
    if (next.fill)
        ++next.fill->references;
    *owned = next;
    cui_invalidate(app->ui);
    return CUI_BRIDGE_OK;
}

cui_bridge_status cui_bridge_rating_get_config_v1(cui_bridge_app *app, cui_bridge_node handle,
                                                  cui_bridge_rating_config_v1 *out) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_rating_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!bridge_icon_header(out, sizeof *out))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (!node->rating)
        return CUI_BRIDGE_EMPTY;
    memcpy(out, &node->rating->config, sizeof *out);
    return CUI_BRIDGE_OK;
}

static bool bridge_rating_draw(cui_bridge_app *app, cui_bridge_node_record *node, float *value) {
    const bridge_rating_configured *style = node->rating;
    cui_rating_options options = {.maximum = node->total, .disabled = node->disabled};
    cui_str name = cui_string(node->text);
    if (!style)
        return cui_rating(app->ui, node->id, &app->theme, name, value, options);
    uint32_t flags = style->config.flags;
    options.half_steps = (flags & CUI_BRIDGE_RATING_HALF_STEPS_V1) != 0;
    options.clearable = (flags & CUI_BRIDGE_RATING_CLEARABLE_V1) != 0;
    options.readonly = (flags & CUI_BRIDGE_RATING_READONLY_V1) != 0;
    options.width = style->config.width;
    cui_rating_presentation paint = {
        .design_size = (cui_design_size)style->config.design_size,
        .selection = flags & CUI_BRIDGE_RATING_STEP_DOWN_FILLED_V1 ? CUI_RATING_STEP_DOWN_FILLED
                                                                   : CUI_RATING_SELECT_VALUE,
        .hover_preview = (flags & CUI_BRIDGE_RATING_HOVER_PREVIEW_V1) != 0,
        .custom_fill = (flags & CUI_BRIDGE_RATING_CUSTOM_FILL_V1) != 0,
        .custom_outline = (flags & CUI_BRIDGE_RATING_CUSTOM_OUTLINE_V1) != 0,
        .custom_preview = (flags & CUI_BRIDGE_RATING_CUSTOM_PREVIEW_V1) != 0,
        .fill = bridge_icon_rgba(style->config.fill_rgba),
        .outline = bridge_icon_rgba(style->config.outline_rgba),
        .preview = bridge_icon_rgba(style->config.preview_rgba),
        .outline_icon = style->outline ? &style->outline->state : NULL,
        .fill_icon = style->fill ? &style->fill->state : NULL};
    return cui_rating_present(app->ui, node->id, &app->theme, name, value, options, paint);
}
#endif
