/* SPDX-License-Identifier: MIT */
#ifndef CUI_DRAW_V2_INTERNAL_H
#define CUI_DRAW_V2_INTERNAL_H
static bool cui_core_begin_frame(cui_context *, float, float, float, bool);
static size_t cui_core_command_limit(const cui_context *ui) {
    return ui->frame_v2 ? ui->v2->command_limit : ui->config.max_commands;
}
static void cui_core_emit(cui_context *ui, cui_draw_command command) {
    if (ui->frame_v2) {
        if (ui->v2_failed)
            return;
        if (ui->count >= ui->v2->command_limit) {
            cui_core_fail(ui, CUI_ERROR_CAPACITY);
            ui->v2_failed = true;
            ui->count = 0;
            return;
        }
        ui->v2->commands[ui->count++] =
            (cui_draw_command_v2){.kind = (uint32_t)command.type, .payload.legacy = command};
    } else
        ui->commands[ui->count++] = command;
}
static size_t cui_core_v2_min(size_t a, size_t b) {
    return a < b ? a : b;
}
bool cui_begin_frame(cui_context *ui, float w, float h, float dt) {
    return cui_core_begin_frame(ui, w, h, dt, false);
}
/* A rejected V2 begin expires new input/numeric authority, including failures
 * before common begin. Preserve existing diagnostics, queues and V1 dispatch. */
static bool cui_core_v2_begin_rejected(cui_context *ui) {
    if (ui)
        ui->numeric_snapshot_ready = false;
    return false;
}
bool cui_begin_frame_v2(cui_context *ui, float w, float h, float dt,
                        const cui_frame_v2_options *options) {
    if (!cui_core_mutation(ui) || !ui)
        return cui_core_v2_begin_rejected(ui);
    if (ui->in_frame) {
        cui_core_fail(ui, CUI_ERROR_LIFECYCLE);
        return cui_core_v2_begin_rejected(ui);
    }
    if (!isfinite(w) || !isfinite(h) || w < 0 || h < 0 || !isfinite(dt) || dt < 0) {
        cui_core_fail(ui, CUI_ERROR_LAYOUT);
        return cui_core_v2_begin_rejected(ui);
    }
    if (!options || options->size != sizeof(*options) ||
        options->support.size != sizeof(options->support) ||
        options->support.abi_version != CUI_DRAW_ABI_V2 ||
        options->support.background_output != CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1 ||
        (options->support.features & ~CUI_DRAW_V2_FEATURE_BACKGROUNDS) ||
        (options->required_features & ~options->support.features)) {
        cui_core_fail(ui, CUI_ERROR_INVALID_ACTION);
        return cui_core_v2_begin_rejected(ui);
    }
    cui_frame_v2_options value = *options;
    size_t cb = value.command_bytes ? value.command_bytes : CUI_DRAW_V2_DEFAULT_COMMAND_BYTES;
    size_t bb =
        value.background_bytes ? value.background_bytes : CUI_DRAW_V2_DEFAULT_BACKGROUND_BYTES;
    if (cb > CUI_DRAW_V2_MAX_BYTES || bb > CUI_DRAW_V2_MAX_BYTES || cb > PTRDIFF_MAX ||
        bb > PTRDIFF_MAX) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return cui_core_v2_begin_rejected(ui);
    }
    size_t commands = cui_core_v2_min(ui->config.max_commands, 16384);
    commands = cui_core_v2_min(commands, value.support.max_commands);
    commands = cui_core_v2_min(commands, cb / sizeof(cui_draw_command_v2));
    bool backgrounds = (value.required_features & CUI_DRAW_V2_FEATURE_BACKGROUNDS) != 0;
    size_t owners = backgrounds ? cui_core_v2_min(ui->config.max_elements, 4096) : 0;
    owners = cui_core_v2_min(owners, value.support.max_backgrounds);
    size_t map_bytes = 0;
    if (backgrounds) {
        if (ui->config.max_elements > bb / sizeof(uint32_t)) {
            cui_core_fail(ui, CUI_ERROR_CAPACITY);
            return cui_core_v2_begin_rejected(ui);
        }
        map_bytes = ui->config.max_elements * sizeof(uint32_t);
        size_t alignment = _Alignof(cui_core_background_owner);
        size_t padding = (alignment - map_bytes % alignment) % alignment;
        if (padding > bb - map_bytes) {
            cui_core_fail(ui, CUI_ERROR_CAPACITY);
            return cui_core_v2_begin_rejected(ui);
        }
        map_bytes += padding;
    }
    if (!commands ||
        (backgrounds &&
         (!owners || !value.support.max_pixels || !value.support.max_pixels_per_background ||
          !value.support.cpu_bytes || !value.support.texture_bytes ||
          !value.support.transfer_bytes || !value.support.metadata_bytes ||
          owners > (bb - map_bytes) / sizeof(cui_core_background_owner)))) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return cui_core_v2_begin_rejected(ui);
    }
    cui_core_v2 *state = ui->v2;
    bool fresh = !state;
    if (fresh)
        state = calloc(1, sizeof(*state));
    if (!state) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return cui_core_v2_begin_rejected(ui);
    }
    /* Prior V2 lists expire at begin attempt. Free replaced frame-only blocks
     * BEFORE allocating successors: even transient payload stays <=16MiB+16MiB.
     * Failure publishes no mode/frame/input; remaining capacities remain usable. */
    if (commands > state->command_capacity) {
        free(state->commands);
        state->commands = NULL;
        state->command_capacity = 0;
        state->commands = malloc(commands * sizeof(*state->commands));
        if (state->commands)
            state->command_capacity = commands;
    }
    if (!state->commands)
        goto capacity;
    if (owners > state->owner_capacity) {
        free(state->background_storage);
        state->background_storage = NULL;
        state->owners = NULL;
        state->map = NULL;
        state->owner_capacity = state->owner_count = 0;
        state->background_storage = malloc(map_bytes + owners * sizeof(*state->owners));
        if (!state->background_storage)
            goto capacity;
        state->map = state->background_storage;
        state->owners =
            (cui_core_background_owner *)((char *)state->background_storage + map_bytes);
        state->owner_capacity = owners;
    }
    state->command_limit = commands;
    state->owner_limit = owners;
    state->features = value.required_features;
    ui->v2 = state;
    if (!cui_core_begin_frame(ui, w, h, dt, true))
        return cui_core_v2_begin_rejected(ui);
    return true;
capacity:
    if (fresh) {
        free(state->commands);
        free(state->background_storage);
        free(state);
    }
    cui_core_fail(ui, CUI_ERROR_CAPACITY);
    return cui_core_v2_begin_rejected(ui);
}
static bool cui_core_background_mode(cui_context *ui) {
    if (!cui_core_active(ui))
        return false;
    if (!ui->frame_v2 || !(ui->v2->features & CUI_DRAW_V2_FEATURE_BACKGROUNDS)) {
        cui_core_fail(ui, CUI_ERROR_LIFECYCLE);
        return false;
    }
    return true;
}
static bool cui_core_background_set(cui_context *ui, size_t index, Clay_LayoutElement *element,
                                    const cui_background *background) {
    if (background && !cui_background_value_valid_internal(background)) {
        cui_core_fail(ui, CUI_ERROR_INVALID_ACTION);
        return false;
    }
    cui_core_v2 *v = ui->v2;
    uint32_t slot = v->map[index];
    if (!slot && !background)
        return true;
    if (!slot) {
        if (v->owner_count == v->owner_limit) {
            cui_core_fail(ui, CUI_ERROR_CAPACITY);
            return false;
        }
        slot = (uint32_t)++v->owner_count;
        v->map[index] = slot;
    }
    cui_background canonical = {0};
    if (background) {
        canonical.kind = background->kind;
        if (background->kind == CUI_BACKGROUND_SOLID)
            canonical.solid = background->solid;
        else {
            canonical.model = background->model;
            canonical.space = background->space;
            canonical.angle_degrees = background->angle_degrees;
            canonical.stops[0] = background->stops[0];
            canonical.stops[1] = background->stops[1];
        }
    }
    v->owners[slot - 1] = (cui_core_background_owner){canonical, background != NULL};
    if (background)
        element->config.custom.customData = &ui->nodes[index];
    return true;
}
bool cui_set_background(cui_context *ui, const cui_background *background) {
    if (!cui_core_background_mode(ui))
        return false;
    if (!ui->depth || ui->overflow_depth || !ui->opened[ui->depth - 1] ||
        cui_core_layout_only(ui) || ui->ambiguous_box_ids) {
        cui_core_fail(ui, CUI_ERROR_INVALID_ACTION);
        return false;
    }
    return cui_core_background_set(ui, ui->node_stack[ui->depth - 1], Clay__GetOpenLayoutElement(),
                                   background);
}
bool cui_element_set_background(cui_context *ui, cui_id id, const cui_background *background) {
    if (!cui_core_background_mode(ui))
        return false;
    if (!id || ui->ambiguous_box_ids || cui_core_layout_only(ui) || ui->overflow_depth ||
        (ui->depth && !ui->opened[ui->depth - 1])) {
        cui_core_fail(ui, CUI_ERROR_INVALID_ACTION);
        return false;
    }
    size_t index = 0;
    while (index < ui->node_count && ui->nodes[index].id != id)
        index++;
    if (index == ui->node_count || ui->nodes[index].layout_only) {
        cui_core_fail(ui, CUI_ERROR_INVALID_ACTION);
        return false;
    }
    cui_core_box_node *node = &ui->nodes[index];
    Clay_LayoutElementHashMapItem *item = Clay__GetHashMapItem(node->clay_id);
    Clay_LayoutElement *element = item->layoutElement;
    if (!element || element->id != node->clay_id || element->isTextElement || element->exiting ||
        element->config.userData != node) {
        cui_core_fail(ui, CUI_ERROR_INVALID_ACTION);
        return false;
    }
    return cui_core_background_set(ui, index, element, background);
}
static const cui_background *cui_core_background_for(cui_context *ui,
                                                     const cui_core_box_node *node) {
    if (!ui->frame_v2 || !ui->v2->map)
        return NULL;
    uint32_t slot = ui->v2->map[(size_t)(node - ui->nodes)];
    return slot && ui->v2->owners[slot - 1].active ? &ui->v2->owners[slot - 1].brush : NULL;
}
static void cui_core_background_emit(cui_context *ui, cui_rect bounds, Clay_CornerRadius radius,
                                     const cui_background *brush, float opacity) {
    if (ui->v2_failed)
        return;
    if (ui->count >= ui->v2->command_limit) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        ui->v2_failed = true;
        ui->count = 0;
        return;
    }
    float limit = fmaxf(0, fminf(bounds.w, bounds.h) / 2);
    cui_corner_radii radii = {fminf(radius.topLeft, limit), fminf(radius.topRight, limit),
                              fminf(radius.bottomLeft, limit), fminf(radius.bottomRight, limit)};
    ui->v2->commands[ui->count++] = (cui_draw_command_v2){
        .kind = CUI_DRAW_V2_BACKGROUND, .payload.background = {bounds, radii, *brush, opacity}};
}
#endif
