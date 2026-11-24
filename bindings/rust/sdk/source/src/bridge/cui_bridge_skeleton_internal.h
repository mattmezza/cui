/* SPDX-License-Identifier: MIT */
#ifndef CUI_BRIDGE_SKELETON_INTERNAL_H
#define CUI_BRIDGE_SKELETON_INTERNAL_H
#include "cui/cui_skeleton_presentation.h"

_Static_assert(sizeof(cui_bridge_skeleton_config_v1) == 40 &&
                   offsetof(cui_bridge_skeleton_config_v1, width) == 16 &&
                   offsetof(cui_bridge_skeleton_config_v1, background_rgba) == 36,
               "Skeleton configuration V1 ABI");
_Static_assert(sizeof(cui_bridge_skeleton_presentation_v1) == 40 &&
                   offsetof(cui_bridge_skeleton_presentation_v1, frame_sequence) == 8 &&
                   offsetof(cui_bridge_skeleton_presentation_v1, core_time) == 16 &&
                   offsetof(cui_bridge_skeleton_presentation_v1, next_wake_seconds) == 24 &&
                   offsetof(cui_bridge_skeleton_presentation_v1, phase) == 32,
               "Skeleton presentation V1 ABI");

typedef struct {
    cui_bridge_skeleton_config_v1 config;
    cui_loading_motion motion;
    cui_bridge_skeleton_presentation_v1 presentation;
    bool sampled;
} bridge_skeleton_owned;

struct bridge_skeleton_app {
    bool step_ok;
    bridge_skeleton_owned *nodes[];
};

static bridge_skeleton_owned *bridge_skeleton_node(cui_bridge_app *app, uint32_t slot) {
    return app->skeletons ? app->skeletons->nodes[slot] : NULL;
}

static void bridge_skeleton_remove(cui_bridge_app *app, uint32_t slot) {
    if (app && app->skeletons) {
        free(app->skeletons->nodes[slot]);
        app->skeletons->nodes[slot] = NULL;
    }
}

uint32_t cui_bridge_skeleton_versions(void) {
    return CUI_BRIDGE_SKELETON_V1;
}

uint32_t cui_bridge_skeleton_config_size_v1(void) {
    return sizeof(cui_bridge_skeleton_config_v1);
}

uint32_t cui_bridge_skeleton_presentation_size_v1(void) {
    return sizeof(cui_bridge_skeleton_presentation_v1);
}

static cui_bridge_status bridge_skeleton_validate_node(cui_bridge_app *app,
                                                       cui_bridge_node handle) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    return node->kind == CUI_BRIDGE_SKELETON ? CUI_BRIDGE_OK : CUI_BRIDGE_INVALID_STATE;
}

static bool bridge_skeleton_valid_config(const cui_bridge_skeleton_config_v1 *config) {
    uint32_t flags = CUI_BRIDGE_SKELETON_SECONDARY | CUI_BRIDGE_SKELETON_CUSTOM_COLOR |
                     CUI_BRIDGE_SKELETON_REDUCED_MOTION;
    if (!config || config->struct_size != sizeof *config ||
        config->schema_version != CUI_BRIDGE_SKELETON_V1 || (config->flags & ~flags) ||
        (!(config->flags & CUI_BRIDGE_SKELETON_CUSTOM_COLOR) && config->background_rgba) ||
        !isfinite(config->height) || config->height <= 0 || config->height > 16384 ||
        !isfinite(config->max_width) || config->max_width < 0 || config->max_width > 16384 ||
        !isfinite(config->radius) || config->radius < 0 || config->radius > 16384 ||
        !isfinite(config->duration) || config->duration < 0 || config->duration > 60 ||
        !isfinite(config->width))
        return false;
    switch (config->width_mode) {
    case CUI_BRIDGE_SKELETON_FIXED:
        return config->width > 0 && config->width <= 16384;
    case CUI_BRIDGE_SKELETON_FRACTION:
        return config->width > 0 && config->width <= 1 && config->max_width == 0;
    case CUI_BRIDGE_SKELETON_GROW:
        return config->width == 0;
    default:
        return false;
    }
}

static float bridge_skeleton_period(float duration) {
    return duration ? duration : 2;
}

cui_bridge_status cui_bridge_skeleton_configure_v1(cui_bridge_app *app, cui_bridge_node handle,
                                                   const cui_bridge_skeleton_config_v1 *config) {
    if (!bridge_skeleton_valid_config(config))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_status status = bridge_skeleton_validate_node(app, handle);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_skeleton_owned *owned = bridge_skeleton_node(app, handle.slot);
    if (!owned) {
        owned = calloc(1, sizeof *owned);
        if (!owned)
            return CUI_BRIDGE_OUT_OF_MEMORY;
        if (!app->skeletons) {
            size_t slots = (size_t)app->max_nodes + 1;
            app->skeletons = calloc(1, sizeof *app->skeletons + slots * sizeof owned);
            if (!app->skeletons) {
                free(owned);
                return CUI_BRIDGE_OUT_OF_MEMORY;
            }
        }
        app->skeletons->nodes[handle.slot] = owned;
    }
    if (bridge_skeleton_period(owned->config.duration) !=
            bridge_skeleton_period(config->duration) ||
        ((owned->config.flags ^ config->flags) & CUI_BRIDGE_SKELETON_REDUCED_MOTION))
        owned->motion = (cui_loading_motion){0};
    owned->config = *config;
    owned->sampled = false;
    return CUI_BRIDGE_OK;
}

cui_bridge_status cui_bridge_skeleton_get_config_v1(cui_bridge_app *app, cui_bridge_node handle,
                                                    cui_bridge_skeleton_config_v1 *out) {
    if (!out || out->struct_size != sizeof *out || out->schema_version != CUI_BRIDGE_SKELETON_V1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_status status = bridge_skeleton_validate_node(app, handle);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_skeleton_owned *owned = bridge_skeleton_node(app, handle.slot);
    if (!owned)
        return CUI_BRIDGE_EMPTY;
    *out = owned->config;
    return CUI_BRIDGE_OK;
}

cui_bridge_status
cui_bridge_skeleton_get_presentation_v1(cui_bridge_app *app, cui_bridge_node handle,
                                        cui_bridge_skeleton_presentation_v1 *out) {
    if (!out || out->struct_size != sizeof *out || out->schema_version != CUI_BRIDGE_SKELETON_V1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_status status = bridge_skeleton_validate_node(app, handle);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_skeleton_owned *owned = bridge_skeleton_node(app, handle.slot);
    if (!owned || !owned->sampled || !app->skeletons->step_ok ||
        owned->presentation.frame_sequence != app->last_core_frame)
        return CUI_BRIDGE_EMPTY;
    *out = owned->presentation;
    return CUI_BRIDGE_OK;
}

static void bridge_skeleton_render(cui_bridge_app *app, cui_id id, bridge_skeleton_owned *owned) {
    const cui_bridge_skeleton_config_v1 *config = &owned->config;
    uint32_t rgba = config->background_rgba;
    cui_skeleton_presentation presentation = {
        .root = {.width = config->width_mode == CUI_BRIDGE_SKELETON_GROW ? -1 : config->width,
                 .width_percent = config->width_mode == CUI_BRIDGE_SKELETON_FRACTION,
                 .height = config->height,
                 .max_width = config->max_width,
                 .radius = config->radius,
                 .background = {(uint8_t)(rgba >> 24), (uint8_t)(rgba >> 16), (uint8_t)(rgba >> 8),
                                (uint8_t)rgba}},
        .custom_background = (config->flags & CUI_BRIDGE_SKELETON_CUSTOM_COLOR) != 0,
        .secondary = (config->flags & CUI_BRIDGE_SKELETON_SECONDARY) != 0,
        .duration = config->duration,
        .now_seconds = cui_frame_time(app->ui),
        .reduced_motion = (config->flags & CUI_BRIDGE_SKELETON_REDUCED_MOTION) != 0,
    };
    cui_skeleton_palette palette;
    if (!cui_skeleton_palette_preset_get(app->theme_preset_index, &palette))
        cui_skeleton_palette_from_theme(&app->theme, &palette);

    bool painted = cui_frame_paint_enabled(app->ui);
    uint64_t frame = app->last_core_frame;
    cui_loading_motion motion = owned->motion;
    if (owned->presentation.frame_sequence < frame - 1)
        motion = (cui_loading_motion){0};
    cui_loading_motion_result result;
    if (!cui_skeleton_present(app->ui, id, &palette, &motion, presentation, &result)) {
        app->frame_error = CUI_BRIDGE_CORE_ERROR;
        return;
    }
    if (painted) {
        owned->motion = motion;
        owned->sampled = true;
        owned->presentation = (cui_bridge_skeleton_presentation_v1){
            .struct_size = sizeof owned->presentation,
            .schema_version = CUI_BRIDGE_SKELETON_V1,
            .frame_sequence = frame,
            .core_time = presentation.now_seconds,
            .next_wake_seconds = result.next_wake_seconds,
            .phase = result.phase,
            .opacity = result.opacity,
        };
    }
}
#endif
