/* SPDX-License-Identifier: MIT */
#ifndef CUI_BRIDGE_SPINNER_INTERNAL_H
#define CUI_BRIDGE_SPINNER_INTERNAL_H

_Static_assert(sizeof(cui_bridge_spinner_config_v1) == 100 &&
                   offsetof(cui_bridge_spinner_config_v1, pixels) == 16 &&
                   offsetof(cui_bridge_spinner_config_v1, rgba) == 28 &&
                   offsetof(cui_bridge_spinner_config_v1, easing) == 32 &&
                   offsetof(cui_bridge_spinner_config_v1, icon) == 36,
               "Spinner configuration V1 ABI");
_Static_assert(sizeof(cui_bridge_spinner_presentation_v1) == 40 &&
                   offsetof(cui_bridge_spinner_presentation_v1, frame_sequence) == 8 &&
                   offsetof(cui_bridge_spinner_presentation_v1, core_time) == 16 &&
                   offsetof(cui_bridge_spinner_presentation_v1, next_wake_seconds) == 24 &&
                   offsetof(cui_bridge_spinner_presentation_v1, phase) == 32,
               "Spinner presentation V1 ABI");
_Static_assert((int)CUI_BRIDGE_SPINNER_CUSTOM == (int)CUI_SPINNER_CUSTOM &&
                   (int)CUI_BRIDGE_SPINNER_XSMALL == (int)CUI_SPINNER_XSMALL &&
                   (int)CUI_BRIDGE_SPINNER_SMALL == (int)CUI_SPINNER_SMALL &&
                   (int)CUI_BRIDGE_SPINNER_MEDIUM == (int)CUI_SPINNER_MEDIUM &&
                   (int)CUI_BRIDGE_SPINNER_LARGE == (int)CUI_SPINNER_LARGE,
               "Bridge Spinner size mapping");

typedef struct {
    cui_bridge_spinner_config_v1 config;
    cui_loading_motion motion;
    cui_bridge_spinner_presentation_v1 presentation;
    bool sampled;
} bridge_spinner_owned;

struct bridge_spinner_app {
    bool step_ok;
    bridge_spinner_owned *nodes[];
};

static bridge_spinner_owned *bridge_spinner_node(cui_bridge_app *app, uint32_t slot) {
    return app->spinners ? app->spinners->nodes[slot] : NULL;
}

static void bridge_spinner_remove(cui_bridge_app *app, uint32_t slot) {
    if (app && app->spinners) {
        free(app->spinners->nodes[slot]);
        app->spinners->nodes[slot] = NULL;
    }
}

uint32_t cui_bridge_spinner_versions(void) {
    return CUI_BRIDGE_SPINNER_V1;
}

uint32_t cui_bridge_spinner_config_size_v1(void) {
    return sizeof(cui_bridge_spinner_config_v1);
}

uint32_t cui_bridge_spinner_presentation_size_v1(void) {
    return sizeof(cui_bridge_spinner_presentation_v1);
}

static cui_bridge_status bridge_spinner_validate_node(cui_bridge_app *app, cui_bridge_node handle) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    return node->kind == CUI_BRIDGE_SPINNER ? CUI_BRIDGE_OK : CUI_BRIDGE_INVALID_STATE;
}

static bool bridge_spinner_valid_size(const cui_bridge_spinner_config_v1 *config) {
    if (config->size == CUI_BRIDGE_SPINNER_CUSTOM)
        return config->pixels > 0 && config->pixels <= 4096 && config->rem_size == 0;
    if (config->size > CUI_BRIDGE_SPINNER_LARGE || config->pixels != 0 ||
        !(config->rem_size >= 0 && config->rem_size <= 4096))
        return false;
    const float ratios[] = {0, .75f, .875f, 1, 1.5f};
    float pixels = (config->rem_size ? config->rem_size : 16) * ratios[config->size];
    return pixels > 0 && pixels <= 4096;
}

static cui_bridge_status bridge_spinner_validate_config(const cui_bridge_spinner_config_v1 *config,
                                                        size_t *icon_length) {
    const uint32_t flags = CUI_BRIDGE_SPINNER_CUSTOM_COLOR | CUI_BRIDGE_SPINNER_REDUCED_MOTION;
    if (!config || config->struct_size != sizeof *config ||
        config->schema_version != CUI_BRIDGE_SPINNER_V1 || (config->flags & ~flags) ||
        (!(config->flags & CUI_BRIDGE_SPINNER_CUSTOM_COLOR) && config->rgba) ||
        !(config->duration >= 0 && config->duration <= 60) ||
        config->easing > CUI_BRIDGE_SPINNER_LINEAR || !bridge_spinner_valid_size(config))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    const char *end = memchr(config->icon, 0, sizeof config->icon);
    if (!end)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    *icon_length = (size_t)(end - config->icon);
    if (!*icon_length)
        return CUI_BRIDGE_OK;
    cui_bridge_status status =
        bridge_icon_bytes(config->icon, (uint32_t)*icon_length, CUI_BRIDGE_SPINNER_ICON_BYTES - 1);
    if (status != CUI_BRIDGE_OK)
        return status;
    return cui_icon_query((cui_str){config->icon, *icon_length}) == CUI_ICON_EMBEDDED
               ? CUI_BRIDGE_OK
               : CUI_BRIDGE_INVALID_ARGUMENT;
}

static float bridge_spinner_period(float duration) {
    return duration ? duration : .8f;
}

cui_bridge_status cui_bridge_spinner_configure_v1(cui_bridge_app *app, cui_bridge_node handle,
                                                  const cui_bridge_spinner_config_v1 *config) {
    size_t icon_length;
    cui_bridge_status status = bridge_spinner_validate_config(config, &icon_length);
    if (status != CUI_BRIDGE_OK)
        return status;
    status = bridge_spinner_validate_node(app, handle);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_spinner_owned *owned = bridge_spinner_node(app, handle.slot);
    if (!owned) {
        owned = calloc(1, sizeof *owned);
        if (!owned)
            return CUI_BRIDGE_OUT_OF_MEMORY;
        if (!app->spinners) {
            size_t slots = (size_t)app->max_nodes + 1;
            app->spinners = calloc(1, sizeof *app->spinners + slots * sizeof owned);
            if (!app->spinners) {
                free(owned);
                return CUI_BRIDGE_OUT_OF_MEMORY;
            }
        }
        app->spinners->nodes[handle.slot] = owned;
    }
    if (bridge_spinner_period(owned->config.duration) != bridge_spinner_period(config->duration) ||
        ((owned->config.flags ^ config->flags) & CUI_BRIDGE_SPINNER_REDUCED_MOTION))
        owned->motion = (cui_loading_motion){0};
    owned->config = *config;
    memset(owned->config.icon + icon_length, 0, sizeof owned->config.icon - icon_length);
    owned->sampled = false;
    return CUI_BRIDGE_OK;
}

cui_bridge_status cui_bridge_spinner_get_config_v1(cui_bridge_app *app, cui_bridge_node handle,
                                                   cui_bridge_spinner_config_v1 *out) {
    if (!out || out->struct_size != sizeof *out || out->schema_version != CUI_BRIDGE_SPINNER_V1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_status status = bridge_spinner_validate_node(app, handle);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_spinner_owned *owned = bridge_spinner_node(app, handle.slot);
    if (!owned)
        return CUI_BRIDGE_EMPTY;
    *out = owned->config;
    return CUI_BRIDGE_OK;
}

cui_bridge_status cui_bridge_spinner_get_presentation_v1(cui_bridge_app *app,
                                                         cui_bridge_node handle,
                                                         cui_bridge_spinner_presentation_v1 *out) {
    if (!out || out->struct_size != sizeof *out || out->schema_version != CUI_BRIDGE_SPINNER_V1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_status status = bridge_spinner_validate_node(app, handle);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_spinner_owned *owned = bridge_spinner_node(app, handle.slot);
    if (!owned || !owned->sampled || !app->spinners->step_ok ||
        owned->presentation.frame_sequence != app->last_core_frame)
        return CUI_BRIDGE_EMPTY;
    *out = owned->presentation;
    return CUI_BRIDGE_OK;
}

static float bridge_spinner_linear(float phase, void *user) {
    (void)user;
    return phase;
}

static bool bridge_spinner_paint_icon(cui_context *ui, cui_id id, const cui_theme *theme,
                                      float size, cui_color color, float radians, void *user) {
    return cui_icon_named(ui, id, theme, cui_string(user),
                          (cui_icon_options){.size = CUI_ICON_CUSTOM,
                                             .custom_size = size,
                                             .custom_color = true,
                                             .color = color,
                                             .custom_transform = true,
                                             .transform = cui_icon_rotation(radians)}) ==
           CUI_ICON_OK;
}

static void bridge_spinner_render(cui_bridge_app *app, cui_id id, bridge_spinner_owned *owned) {
    cui_bridge_spinner_config_v1 *config = &owned->config;
    uint32_t rgba = config->rgba;
    cui_spinner_sizing sizing = {.size = (cui_spinner_size)config->size,
                                 .pixels = config->pixels,
                                 .rem_size = config->rem_size};
    cui_spinner_options options = {
        .sizing = &sizing,
        .now_seconds = cui_frame_time(app->ui),
        .duration = config->duration,
        .custom_color = (config->flags & CUI_BRIDGE_SPINNER_CUSTOM_COLOR) != 0,
        .reduced_motion = (config->flags & CUI_BRIDGE_SPINNER_REDUCED_MOTION) != 0,
        .color = {(uint8_t)(rgba >> 24), (uint8_t)(rgba >> 16), (uint8_t)(rgba >> 8),
                  (uint8_t)rgba},
        .ease = config->easing == CUI_BRIDGE_SPINNER_LINEAR ? bridge_spinner_linear : NULL,
        .paint = config->icon[0] ? bridge_spinner_paint_icon : NULL,
        .paint_user = config->icon,
    };
    uint64_t frame = app->last_core_frame;
    cui_loading_motion motion = owned->motion;
    if (owned->presentation.frame_sequence < frame - 1)
        motion = (cui_loading_motion){0};
    cui_loading_motion_result result;
    if (!cui_spinner_ex(app->ui, id, &app->theme, &motion, options, &result)) {
        app->frame_error = CUI_BRIDGE_CORE_ERROR;
        return;
    }
    if (cui_frame_paint_enabled(app->ui)) {
        owned->motion = motion;
        owned->sampled = true;
        owned->presentation = (cui_bridge_spinner_presentation_v1){
            .struct_size = sizeof owned->presentation,
            .schema_version = CUI_BRIDGE_SPINNER_V1,
            .frame_sequence = frame,
            .core_time = options.now_seconds,
            .next_wake_seconds = result.next_wake_seconds,
            .phase = result.phase,
        };
    }
}
#endif
