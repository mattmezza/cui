/* SPDX-License-Identifier: MIT */
#ifndef CUI_BRIDGE_PAGINATION_INTERNAL_H
#define CUI_BRIDGE_PAGINATION_INTERNAL_H

_Static_assert(sizeof(cui_bridge_pagination_config_v1) == 916 &&
                   offsetof(cui_bridge_pagination_config_v1, available_width) == 16 &&
                   offsetof(cui_bridge_pagination_config_v1, group) == 20 &&
                   offsetof(cui_bridge_pagination_config_v1, later_pages) == 788,
               "Pagination configuration V1 ABI");

uint32_t cui_bridge_pagination_versions(void) {
    return CUI_BRIDGE_PAGINATION_V1;
}
uint32_t cui_bridge_pagination_config_size_v1(void) {
    return sizeof(cui_bridge_pagination_config_v1);
}

static cui_bridge_pagination_config_v1 *bridge_pagination_node(cui_bridge_app *app, uint32_t slot) {
    return app->pagination ? app->pagination[slot] : NULL;
}
static void bridge_pagination_remove(cui_bridge_app *app, uint32_t slot) {
    if (app && app->pagination) {
        free(app->pagination[slot]);
        app->pagination[slot] = NULL;
    }
}
static cui_bridge_status bridge_pagination_validate_node(cui_bridge_app *app,
                                                         cui_bridge_node handle) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    return node->kind == CUI_BRIDGE_PAGINATION ? CUI_BRIDGE_OK : CUI_BRIDGE_INVALID_STATE;
}
static cui_bridge_status bridge_pagination_copy_labels(const cui_bridge_pagination_config_v1 *src,
                                                       cui_bridge_pagination_config_v1 *dst) {
    const char *from[] = {src->group, src->first,         src->previous,   src->next,
                          src->last,  src->earlier_pages, src->later_pages};
    char *to[] = {dst->group, dst->first,         dst->previous,   dst->next,
                  dst->last,  dst->earlier_pages, dst->later_pages};
    for (size_t field = 0; field < sizeof from / sizeof from[0]; ++field) {
        const char *label = from[field];
        if (!memchr(label, 0, CUI_BRIDGE_PAGINATION_LABEL_BYTES))
            return CUI_BRIDGE_CAPACITY;
        size_t length;
        cui_bridge_status status =
            bridge_utf8_limit(label, CUI_BRIDGE_PAGINATION_LABEL_BYTES - 1, &length);
        if (status != CUI_BRIDGE_OK)
            return status;
        for (size_t i = 0; i < length; ++i)
            if ((unsigned char)label[i] < 0x20 || (unsigned char)label[i] == 0x7f)
                return CUI_BRIDGE_INVALID_ARGUMENT;
        if (strstr(label, "\xc2\x85") || strstr(label, "\xe2\x80\xa8") ||
            strstr(label, "\xe2\x80\xa9"))
            return CUI_BRIDGE_INVALID_ARGUMENT;
        memcpy(to[field], label, length);
    }
    return CUI_BRIDGE_OK;
}

cui_bridge_status
cui_bridge_pagination_configure_v1(cui_bridge_app *app, cui_bridge_node handle,
                                   const cui_bridge_pagination_config_v1 *config) {
    cui_bridge_status status = bridge_pagination_validate_node(app, handle);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!config) {
        bridge_pagination_remove(app, handle.slot);
        cui_invalidate(app->ui);
        return CUI_BRIDGE_OK;
    }
    const uint32_t flags = CUI_BRIDGE_PAGINATION_COUNTER | CUI_BRIDGE_PAGINATION_ICON_ONLY |
                           CUI_BRIDGE_PAGINATION_CHEVRONS | CUI_BRIDGE_PAGINATION_JUMP_MENU;
    if (config->struct_size != sizeof *config ||
        config->schema_version != CUI_BRIDGE_PAGINATION_V1 || (config->flags & ~flags) ||
        (config->visible_pages &&
         (config->visible_pages < 5 || config->visible_pages > CUI_PAGINATION_MAX_VISIBLE_PAGES)) ||
        !isfinite(config->available_width) || config->available_width < 0 ||
        config->available_width > 32768)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_pagination_config_v1 copied = {
        .struct_size = sizeof copied,
        .schema_version = CUI_BRIDGE_PAGINATION_V1,
        .flags = config->flags,
        .visible_pages = config->visible_pages,
        .available_width = config->available_width,
    };
    status = bridge_pagination_copy_labels(config, &copied);
    if (status != CUI_BRIDGE_OK)
        return status;
    cui_bridge_pagination_config_v1 *owned = bridge_pagination_node(app, handle.slot);
    if (!owned) {
        owned = malloc(sizeof *owned);
        if (!owned)
            return CUI_BRIDGE_OUT_OF_MEMORY;
        if (!app->pagination) {
            app->pagination = calloc((size_t)app->max_nodes + 1, sizeof *app->pagination);
            if (!app->pagination) {
                free(owned);
                return CUI_BRIDGE_OUT_OF_MEMORY;
            }
        }
        app->pagination[handle.slot] = owned;
    }
    *owned = copied;
    cui_invalidate(app->ui);
    return CUI_BRIDGE_OK;
}

cui_bridge_status cui_bridge_pagination_get_config_v1(cui_bridge_app *app, cui_bridge_node handle,
                                                      cui_bridge_pagination_config_v1 *out) {
    if (!out || out->struct_size != sizeof *out || out->schema_version != CUI_BRIDGE_PAGINATION_V1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_status status = bridge_pagination_validate_node(app, handle);
    if (status != CUI_BRIDGE_OK)
        return status;
    const cui_bridge_pagination_config_v1 *owned = bridge_pagination_node(app, handle.slot);
    if (!owned)
        return CUI_BRIDGE_EMPTY;
    *out = *owned;
    return CUI_BRIDGE_OK;
}

cui_bridge_status cui_bridge_pagination_set_custom_size(cui_bridge_app *app, cui_bridge_node handle,
                                                        float size) {
    cui_bridge_status status = bridge_pagination_validate_node(app, handle);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!isfinite(size) || (size != 0 && (size < CUI_PAGINATION_MIN_CUSTOM_SIZE ||
                                          size > CUI_PAGINATION_MAX_CUSTOM_SIZE)))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_node_record *node = &app->nodes[handle.slot];
    if (node->pagination_custom_size != size) {
        node->pagination_custom_size = size;
        cui_invalidate(app->ui);
    }
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_pagination_get_custom_size(cui_bridge_app *app, cui_bridge_node handle,
                                                        float *out) {
    if (!out)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_status status = bridge_pagination_validate_node(app, handle);
    if (status != CUI_BRIDGE_OK)
        return status;
    *out = app->nodes[handle.slot].pagination_custom_size;
    return CUI_BRIDGE_OK;
}

static bool bridge_pagination_draw(cui_bridge_app *app, const cui_bridge_node_record *node,
                                   size_t *page) {
    const cui_bridge_pagination_config_v1 *config = bridge_pagination_node(app, node->own_slot);
    if (!config && node->size == CUI_BRIDGE_SIZE_INHERIT && !node->pagination_custom_size)
        return cui_pagination(app->ui, node->id, &app->theme, page, node->total, false,
                              node->disabled);
    cui_pagination_options options = {.disabled = node->disabled,
                                      .design_size = (cui_design_size)node->size,
                                      .custom_size = node->pagination_custom_size};
    cui_pagination_labels labels = {0};
    if (config) {
        uint32_t flags = config->flags;
        options.compact = (flags & CUI_BRIDGE_PAGINATION_COUNTER) != 0;
        options.icon_only = (flags & CUI_BRIDGE_PAGINATION_ICON_ONLY) != 0;
        if (flags & CUI_BRIDGE_PAGINATION_CHEVRONS)
            options.navigation = CUI_PAGINATION_NAV_CHEVRONS;
        if (flags & (CUI_BRIDGE_PAGINATION_ICON_ONLY | CUI_BRIDGE_PAGINATION_CHEVRONS))
            options.tooltip_viewport = (cui_rect){0, 0, app->width, app->height};
        if ((flags & CUI_BRIDGE_PAGINATION_JUMP_MENU) && app->width > 0 && app->height > 8)
            options.ellipsis_viewport = (cui_rect){0, 0, app->width, app->height};
        options.visible_pages = config->visible_pages;
        options.available_width = config->available_width;
        labels = (cui_pagination_labels){
            .group = cui_string(config->group),
            .first = cui_string(config->first),
            .previous = cui_string(config->previous),
            .next = cui_string(config->next),
            .last = cui_string(config->last),
            .earlier_pages = cui_string(config->earlier_pages),
            .later_pages = cui_string(config->later_pages),
        };
        options.labels = &labels;
    }
    return cui_pagination_ex(app->ui, node->id, &app->theme, page, node->total, options);
}
#endif
