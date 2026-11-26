/* SPDX-License-Identifier: MIT */
/* Configured Badge V1 uses the ordinary Badge painter and retained children. */

_Static_assert(sizeof(cui_bridge_badge_config_v1) == 64 &&
                   offsetof(cui_bridge_badge_config_v1, count) == 32 &&
                   offsetof(cui_bridge_badge_config_v1, artwork) == 56,
               "Badge configuration V1 ABI");
_Static_assert(sizeof(cui_bridge_badge_result_v1) == 40 &&
                   offsetof(cui_bridge_badge_result_v1, frame_sequence) == 24 &&
                   offsetof(cui_bridge_badge_result_v1, flags) == 36,
               "Badge result V1 ABI");

struct bridge_badge_configured {
    cui_bridge_badge_config_v1 config;
    cui_bridge_icon_options_v1 artwork;
    bridge_icon_decoded decoded;
    bridge_icon_owned *resource;
    cui_badge_result result;
    uint64_t frame;
    uint32_t name_bytes, icon_bytes;
    char text[];
};

static bridge_badge_configured *bridge_badge_node(cui_bridge_app *app, uint32_t slot) {
    return app && app->icons && app->icons->badges ? app->icons->badges[slot] : NULL;
}

static void bridge_badge_remove(cui_bridge_app *app, uint32_t slot) {
    bridge_badge_configured *value = bridge_badge_node(app, slot);
    if (!value)
        return;
    if (value->resource)
        value->resource->references--;
    free(value);
    app->icons->badges[slot] = NULL;
}

static cui_bridge_status bridge_badge_get(cui_bridge_app *app, cui_bridge_node n,
                                          bridge_badge_configured **out) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, n, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_BADGE_CONFIGURED)
        return CUI_BRIDGE_INVALID_STATE;
    *out = bridge_badge_node(app, n.slot);
    return *out ? CUI_BRIDGE_OK : CUI_BRIDGE_INVALID_STATE;
}

uint32_t cui_bridge_badge_versions(void) {
    return cui_bridge_icon_presentation_versions();
}

uint32_t cui_bridge_badge_record_size_v1(uint32_t kind) {
    switch (kind) {
    case CUI_BRIDGE_BADGE_CONFIG_RECORD_V1:
        return sizeof(cui_bridge_badge_config_v1);
    case CUI_BRIDGE_BADGE_RESULT_RECORD_V1:
        return sizeof(cui_bridge_badge_result_v1);
    default:
        return 0;
    }
}

cui_bridge_status cui_bridge_badge_default_config_v1(cui_bridge_badge_config_v1 *out) {
    if (!bridge_icon_header(out, sizeof *out))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_badge_options defaults = cui_badge_defaults();
    *out = (cui_bridge_badge_config_v1){
        .struct_size = sizeof *out,
        .schema_version = 1,
        .variant = defaults.variant,
        .size = defaults.size,
        .design_size = defaults.design_size,
        .count = defaults.count,
        .maximum = defaults.maximum,
    };
    return CUI_BRIDGE_OK;
}

static cui_bridge_status bridge_badge_stage(cui_bridge_app *app,
                                            const cui_bridge_badge_config_v1 *config,
                                            const cui_bridge_icon_options_v1 *artwork, cui_str name,
                                            cui_str named_icon, bridge_badge_configured **out) {
    if (!bridge_icon_header(config, sizeof *config) || config->reserved[0] || config->reserved[1] ||
        config->reserved_tail || config->variant > CUI_BADGE_ICON ||
        config->size > CUI_CONTROL_XSMALL || config->design_size > CUI_DESIGN_9XL ||
        (config->flags & ~CUI_BRIDGE_BADGE_CUSTOM_COLOR_V1) ||
        (!(config->flags & CUI_BRIDGE_BADGE_CUSTOM_COLOR_V1) && config->color_rgba))
        return CUI_BRIDGE_INVALID_ARGUMENT;
#if SIZE_MAX < UINT64_MAX
    if (config->count > SIZE_MAX || config->maximum > SIZE_MAX)
        return CUI_BRIDGE_INVALID_ARGUMENT;
#endif
    bool owned = config->artwork.slot && config->artwork.generation;
    if ((!owned && (config->artwork.slot || config->artwork.generation)) ||
        (owned != (artwork != NULL)) ||
        (config->variant == CUI_BADGE_ICON ? (owned == (named_icon.length != 0))
                                           : (owned || named_icon.length)))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_status status = bridge_icon_bytes(name.data, (uint32_t)name.length, 4096);
    if (status == CUI_BRIDGE_OK)
        status = bridge_icon_bytes(named_icon.data, (uint32_t)named_icon.length, 4096);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_icon_owned *resource = NULL;
    bridge_icon_decoded decoded = {0};
    if (owned) {
        if (!bridge_icon_options_decode(artwork, (cui_str){0}, &decoded))
            return CUI_BRIDGE_INVALID_ARGUMENT;
        status = bridge_icon_get(app, config->artwork, &resource);
        if (status != CUI_BRIDGE_OK)
            return status;
    }
    bridge_badge_configured *next = calloc(1, sizeof *next + name.length + named_icon.length + 2);
    if (!next)
        return CUI_BRIDGE_OUT_OF_MEMORY;
    next->config = *config;
    next->resource = resource;
    next->name_bytes = (uint32_t)name.length;
    next->icon_bytes = (uint32_t)named_icon.length;
    if (name.length)
        memcpy(next->text, name.data, name.length);
    if (named_icon.length)
        memcpy(next->text + name.length + 1, named_icon.data, named_icon.length);
    next->artwork = artwork ? *artwork
                            : (cui_bridge_icon_options_v1){.struct_size = sizeof next->artwork,
                                                           .schema_version = 1};
    if (resource) {
        next->decoded = decoded;
        bridge_icon_native_options(&next->artwork, (cui_str){0}, &next->decoded);
    }
    *out = next;
    return CUI_BRIDGE_OK;
}

cui_bridge_status cui_bridge_badge_create_v1(cui_bridge_app *app, cui_bridge_node parent,
                                             const char *key,
                                             const cui_bridge_badge_config_v1 *config,
                                             const cui_bridge_icon_options_v1 *artwork,
                                             const char *name, uint32_t name_bytes,
                                             const char *named_icon, uint32_t icon_bytes,
                                             cui_bridge_node *out) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!BRIDGE_SLIDER_SUPPORTED)
        return CUI_BRIDGE_INVALID_STATE;
    if (!parent.slot && !parent.generation)
        return CUI_BRIDGE_INVALID_STATE;
    /* Ordinary node creation sees a plain Badge until this factory publishes
     * the configured kind. Check its final kind's restriction before staging. */
    if (bridge_node(app, parent))
        for (uint32_t slot = parent.slot; slot; slot = app->nodes[slot].parent)
            if (app->nodes[slot].radio_item_content)
                return CUI_BRIDGE_INVALID_STATE;
    size_t key_size = 0;
    if (!bridge_utf8(key, &key_size) || !key_size)
        return CUI_BRIDGE_INVALID_UTF8;
    bridge_icon_range ranges[] = {
        {config, sizeof *config}, {artwork, artwork ? sizeof *artwork : 0},
        {name, name_bytes},       {named_icon, icon_bytes},
        {key, key_size + 1},      {out, sizeof *out}};
    if (!bridge_icon_ranges(ranges, sizeof ranges / sizeof ranges[0]))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    bridge_badge_configured *next;
    status = bridge_badge_stage(app, config, artwork, (cui_str){name, name_bytes},
                                (cui_str){named_icon, icon_bytes}, &next);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_icon_app *pool = app->icons;
    bool fresh_pool = !pool;
    if (fresh_pool)
        pool = calloc(1, sizeof *pool);
    bridge_badge_configured **table = pool ? pool->badges : NULL;
    bool fresh_table = !table;
    if (pool && fresh_table)
        table = calloc((size_t)app->max_nodes + 1, sizeof *table);
    cui_bridge_node handle;
    /* As with the typed Slider factory, use ordinary key and parent admission.
     * The final kind is published only after all copied storage is ready. */
    status = table ? cui_bridge_node_create(app, parent, CUI_BRIDGE_BADGE, key, &handle)
                   : CUI_BRIDGE_OUT_OF_MEMORY;
    if (status != CUI_BRIDGE_OK) {
        if (fresh_table)
            free(table);
        if (fresh_pool)
            free(pool);
        free(next);
        return status;
    }
    if (fresh_pool)
        pool->owner = bridge_slider_owner();
    app->icons = pool;
    pool->badges = table;
    if (next->resource)
        next->resource->references++;
    table[handle.slot] = next;
    app->nodes[handle.slot].kind = CUI_BRIDGE_BADGE_CONFIGURED;
    *out = handle;
    return CUI_BRIDGE_OK;
}

cui_bridge_status cui_bridge_badge_configure_v1(cui_bridge_app *app, cui_bridge_node n,
                                                const cui_bridge_badge_config_v1 *config,
                                                const cui_bridge_icon_options_v1 *artwork,
                                                const char *name, uint32_t name_bytes,
                                                const char *named_icon, uint32_t icon_bytes) {
    bridge_badge_configured *next;
    cui_bridge_status status = bridge_badge_get(app, n, &next);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_icon_range ranges[] = {{config, sizeof *config},
                                  {artwork, artwork ? sizeof *artwork : 0},
                                  {name, name_bytes},
                                  {named_icon, icon_bytes}};
    if (!bridge_icon_ranges(ranges, sizeof ranges / sizeof ranges[0]))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    status = bridge_badge_stage(app, config, artwork, (cui_str){name, name_bytes},
                                (cui_str){named_icon, icon_bytes}, &next);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_badge_remove(app, n.slot);
    if (next->resource)
        next->resource->references++;
    app->icons->badges[n.slot] = next;
    return CUI_BRIDGE_OK;
}

cui_bridge_status cui_bridge_badge_get_config_v1(cui_bridge_app *app, cui_bridge_node n,
                                                 cui_bridge_badge_config_v1 *config,
                                                 cui_bridge_icon_options_v1 *artwork) {
    bridge_badge_configured *value;
    cui_bridge_status status = bridge_badge_get(app, n, &value);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_icon_range ranges[] = {{config, sizeof *config}, {artwork, sizeof *artwork}};
    if (!bridge_icon_ranges(ranges, 2) || !bridge_icon_header(config, sizeof *config) ||
        !bridge_icon_header(artwork, sizeof *artwork))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    *config = value->config;
    *artwork = value->artwork;
    return CUI_BRIDGE_OK;
}

static cui_str bridge_badge_name(const bridge_badge_configured *value) {
    return (cui_str){value->text, value->name_bytes};
}

static cui_str bridge_badge_named_icon(const bridge_badge_configured *value) {
    return (cui_str){value->text + value->name_bytes + 1, value->icon_bytes};
}

cui_bridge_status cui_bridge_badge_copy_text_v1(cui_bridge_app *app, cui_bridge_node n,
                                                uint32_t field, char *out, uint32_t capacity,
                                                uint32_t *required) {
    bridge_badge_configured *value;
    cui_bridge_status status = bridge_badge_get(app, n, &value);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (field > CUI_BRIDGE_BADGE_NAMED_ICON_V1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_str text = field == CUI_BRIDGE_BADGE_NAME_V1 ? bridge_badge_name(value)
                                                     : bridge_badge_named_icon(value);
    return bridge_icon_string(text, out, capacity, required);
}

static bool bridge_badge_observed(cui_bridge_app *app, const bridge_badge_configured *value) {
    return value->frame && value->frame == app->icons->presentation_frame;
}

cui_bridge_status cui_bridge_badge_get_result_v1(cui_bridge_app *app, cui_bridge_node n,
                                                 cui_bridge_badge_result_v1 *out) {
    bridge_badge_configured *value;
    cui_bridge_status status = bridge_badge_get(app, n, &value);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!bridge_icon_header(out, sizeof *out))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (!bridge_badge_observed(app, value))
        return CUI_BRIDGE_EMPTY;
    *out = (cui_bridge_badge_result_v1){
        .struct_size = sizeof *out,
        .schema_version = 1,
        .submitted = value->result.submitted,
        .icon_error = value->result.icon_error,
        .node = n,
        .frame_sequence = value->frame,
        .frame_status = app->icons->presentation_status,
        .flags = value->resource ? CUI_BRIDGE_BADGE_ICON_OBSERVED_V1 : 0,
    };
    return CUI_BRIDGE_OK;
}

cui_bridge_status cui_bridge_badge_indicator_bounds_v1(cui_bridge_app *app, cui_bridge_node n,
                                                       cui_bridge_rect *out) {
    bridge_badge_configured *value;
    cui_bridge_status status = bridge_badge_get(app, n, &value);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (!bridge_badge_observed(app, value) || !value->result.submitted ||
        app->icons->presentation_status != CUI_BRIDGE_OK ||
        (value->config.variant == CUI_BADGE_COUNT && !value->config.count))
        return CUI_BRIDGE_EMPTY;
    cui_rect rect = cui_bounds(app->ui, cui_badge_indicator_id(app->nodes[n.slot].id));
    if (!isfinite(rect.x) || !isfinite(rect.y) || !isfinite(rect.w) || !isfinite(rect.h) ||
        rect.w <= 0 || rect.h <= 0)
        return CUI_BRIDGE_EMPTY;
    *out = (cui_bridge_rect){rect.x, rect.y, rect.w, rect.h};
    return CUI_BRIDGE_OK;
}

static void bridge_badge_render(cui_bridge_app *app, cui_id id, bridge_badge_configured *value,
                                cui_button_content_fn content, void *user) {
    const cui_bridge_badge_config_v1 *c = &value->config;
    cui_badge_options options = {
        .variant = (cui_badge_variant)c->variant,
        .size = (cui_control_size)c->size,
        .design_size = (cui_design_size)c->design_size,
        .count = (size_t)c->count,
        .maximum = (size_t)c->maximum,
        .custom_color = (c->flags & CUI_BRIDGE_BADGE_CUSTOM_COLOR_V1) != 0,
        .color = bridge_icon_rgba(c->color_rgba),
        .name = bridge_badge_name(value),
        .icon = bridge_badge_named_icon(value),
    };
    if (value->resource) {
        cui_badge_icon_content artwork = {.state = &value->resource->state,
                                          .options = value->decoded.options};
        value->result =
            cui_badge_with_icon(app->ui, id, &app->theme, options, &artwork, content, user);
    } else {
        value->result = (cui_badge_result){
            .submitted = cui_badge_ex(app->ui, id, &app->theme, options, content, user)};
    }
    value->frame = app->last_core_frame;
}
