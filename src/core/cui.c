/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif
#ifndef CLAY_IMPLEMENTATION
#define CLAY_IMPLEMENTATION
#define CUI_CORE_DEFINED_CLAY_IMPLEMENTATION
#endif
#include "clay.h"
#ifdef CUI_CORE_DEFINED_CLAY_IMPLEMENTATION
#undef CLAY_IMPLEMENTATION
#undef CUI_CORE_DEFINED_CLAY_IMPLEMENTATION
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#define CUI_CORE_STACK_MAX 256
#define CUI_CORE_EVENT_MAX 256
typedef struct {
    uint32_t clay_id;
    size_t parent;
    bool clip, layout_only, inert;
    int16_t layer;
    cui_bitmap_paint bitmap;
    cui_rect underlay_bounds;
    cui_underlay_layout underlay_layout;
    cui_color underlay_color;
    cui_corner_radii underlay_radii;
} cui_core_box_node;
typedef struct {
    cui_id id;
    size_t parent;
    bool modal, active;
} cui_core_focus_scope;
typedef struct {
    cui_id id;
    cui_rect *retained;
} cui_core_bounds_track;
struct cui_context {
    cui_core_focus_scope focus_scopes[CUI_CORE_STACK_MAX],
        previous_focus_scopes[CUI_CORE_STACK_MAX];
    size_t focus_stack[CUI_CORE_STACK_MAX], focus_depth, focus_scope_count,
        previous_focus_scope_count, top_modal, previous_top_modal;
    size_t *semantic_scopes, *previous_semantic_scopes;
    Clay_Context *clay;
    void *arena;
    Clay__ScrollContainerDataInternal *clip_containers;
    cui_config config;
    cui_draw_command *commands;
    size_t count;
    cui_core_box_node *nodes;
    cui_core_bounds_track *bounds_tracks;
    size_t bounds_track_count;
    size_t *semantic_nodes;
    size_t node_count, node_stack[CUI_CORE_STACK_MAX];
    cui_rect *previous_hits;
    int16_t *previous_layers;
    cui_semantic *semantics, *previous;
    size_t semantic_count, previous_count;
    cui_id *ids, *activations;
    size_t id_count, activation_count;
    cui_event queued[CUI_CORE_EVENT_MAX], events[CUI_CORE_EVENT_MAX];
    size_t queued_count, event_count;
    cui_id event_targets[CUI_CORE_EVENT_MAX], event_activations[CUI_CORE_EVENT_MAX];
    uint64_t scopes[CUI_CORE_STACK_MAX];
    size_t scope_count;
    bool opened[CUI_CORE_STACK_MAX];
    size_t depth, overflow_depth, element_count;
    char *strings;
    size_t string_used;
    cui_id focus, capture, space_capture;
    float pointer_x, pointer_y, dt;
    cui_error error;
    bool in_frame, focus_visible, dirty;
};
static bool cui_core_layout_only(const cui_context *ui) {
    return ui && ui->in_frame && ui->depth && !ui->overflow_depth && ui->opened[ui->depth - 1] &&
           ui->nodes[ui->node_stack[ui->depth - 1]].layout_only;
}
static bool cui_core_inert(const cui_context *ui) {
    return ui && ui->in_frame && ui->depth && !ui->overflow_depth && ui->opened[ui->depth - 1] &&
           ui->nodes[ui->node_stack[ui->depth - 1]].inert;
}
static void cui_core_fail(cui_context *ui, cui_error e) {
    if (ui && ui->error == CUI_OK)
        ui->error = e;
}
static void cui_core_select_context(cui_context *ui) {
    Clay_SetCurrentContext(ui->clay);
}
static bool cui_core_active(cui_context *ui) {
    if (!ui)
        return false;
    if (!ui->in_frame) {
        cui_core_fail(ui, CUI_ERROR_LIFECYCLE);
        return false;
    }
    cui_core_select_context(ui);
    return true;
}
cui_size cui_measure_text(cui_context *ui, cui_str text, float size) {
    if (!ui || (!text.data && text.length) || !isfinite(size) || size <= 0)
        return (cui_size){0};
    size = size > 65535 ? 65535 : (float)(uint16_t)size;
    cui_size result = ui->config.measure ? ui->config.measure(ui->config.measure_user, text, size)
                                         : (cui_size){text.length * size * .6f, size * 1.3f};
    if (!isfinite(result.width) || !isfinite(result.height) || result.width < 0 ||
        result.height < 0)
        return (cui_size){0};
    return result;
}
static Clay_Dimensions cui_core_measure(Clay_StringSlice s, Clay_TextElementConfig *c, void *u) {
    cui_size z = cui_measure_text(u, (cui_str){s.chars, (size_t)s.length}, c->fontSize);
    return (Clay_Dimensions){z.width, z.height};
}

static void cui_core_clay_error(Clay_ErrorData e) {
    cui_error kind = CUI_ERROR_LAYOUT;
    if (e.errorType == CLAY_ERROR_TYPE_DUPLICATE_ID)
        kind = CUI_ERROR_DUPLICATE_ID;
    if (e.errorType == CLAY_ERROR_TYPE_ELEMENTS_CAPACITY_EXCEEDED ||
        e.errorType == CLAY_ERROR_TYPE_ARENA_CAPACITY_EXCEEDED ||
        e.errorType == CLAY_ERROR_TYPE_TEXT_MEASUREMENT_CAPACITY_EXCEEDED ||
        e.errorType == CLAY_ERROR_TYPE_HASH_MAP_CAPACITY_EXCEEDED)
        kind = CUI_ERROR_CAPACITY;
    cui_core_fail(e.userData, kind);
}
cui_str cui_string(const char *s) {
    return (cui_str){s, s ? strlen(s) : 0};
}
static float cui_core_linear_channel(uint8_t channel) {
    float x = channel / 255.0f;
    return x <= .04045f ? x / 12.92f : powf((x + .055f) / 1.055f, 2.4f);
}
static uint8_t cui_core_srgb_channel(float x) {
    x = x <= .0031308f ? 12.92f * x : 1.055f * powf(x, 1 / 2.4f) - .055f;
    return (uint8_t)lroundf(fmaxf(0, fminf(1, x)) * 255);
}
static void cui_core_color_lab(cui_color c, float lab[3]) {
    float r = cui_core_linear_channel(c.r), g = cui_core_linear_channel(c.g),
          b = cui_core_linear_channel(c.b);
    float l = cbrtf(.4122214708f * r + .5363325363f * g + .0514459929f * b),
          m = cbrtf(.2119034982f * r + .6806995451f * g + .1073969566f * b),
          s = cbrtf(.0883024619f * r + .2817188376f * g + .6299787005f * b);
    lab[0] = .2104542553f * l + .7936177850f * m - .0040720468f * s;
    lab[1] = 1.9779984951f * l - 2.4285922050f * m + .4505937099f * s;
    lab[2] = .0259040371f * l + .7827717662f * m - .8086757660f * s;
}
cui_color cui_color_mix_oklab(cui_color first, cui_color second, float amount) {
    if (!isfinite(amount) || amount <= 0)
        return first;
    if (amount >= 1)
        return second;
    float a = first.a * (1 - amount), b = second.a * amount, alpha = a + b;
    if (alpha <= 0)
        return (cui_color){0};
    float x[3], y[3];
    cui_core_color_lab(first, x);
    cui_core_color_lab(second, y);
    for (size_t i = 0; i < 3; i++)
        x[i] = (x[i] * a + y[i] * b) / alpha;
    float l = x[0] + .3963377774f * x[1] + .2158037573f * x[2],
          m = x[0] - .1055613458f * x[1] - .0638541728f * x[2],
          s = x[0] - .0894841775f * x[1] - 1.2914855480f * x[2];
    l = l * l * l;
    m = m * m * m;
    s = s * s * s;
    return (cui_color){
        cui_core_srgb_channel(4.0767416621f * l - 3.3077115913f * m + .2309699292f * s),
        cui_core_srgb_channel(-1.2684380046f * l + 2.6097574011f * m - .3413193965f * s),
        cui_core_srgb_channel(-.0041960863f * l - .7034186147f * m + 1.7076147010f * s),
        (uint8_t)lroundf(alpha)};
}

cui_context *cui_create(cui_config config) {
    cui_context *ui = calloc(1, sizeof(*ui));
    if (!ui)
        return NULL;
    if (!config.max_elements)
        config.max_elements = 4096;
    if (!config.max_commands)
        config.max_commands = 8192;
    if (!config.frame_bytes)
        config.frame_bytes = 262144;
    /* Clay reports arena sizes through uint32_t, so retain a conservative
       supported element ceiling before its sizing or any allocations. */
    if (config.frame_bytes > (size_t)PTRDIFF_MAX || config.max_elements > 1000000 ||
        config.max_commands > (size_t)PTRDIFF_MAX / sizeof(cui_draw_command) ||
        config.max_elements > SIZE_MAX / sizeof(cui_semantic) ||
        config.max_elements > SIZE_MAX / sizeof(cui_id) ||
        config.max_elements > SIZE_MAX / sizeof(cui_core_box_node) ||
        config.max_elements > SIZE_MAX / sizeof(size_t) ||
        config.max_elements > SIZE_MAX / sizeof(cui_rect) ||
        config.max_elements > SIZE_MAX / sizeof(cui_core_bounds_track) ||
        config.max_elements > SIZE_MAX / sizeof(Clay__ScrollContainerDataInternal)) {
        free(ui);
        return NULL;
    }
    ui->config = config;
    ui->dirty = true;
    ui->top_modal = ui->previous_top_modal = SIZE_MAX;
    ui->commands = calloc(config.max_commands, sizeof(*ui->commands));
    ui->semantics = calloc(config.max_elements, sizeof(*ui->semantics));
    ui->previous = calloc(config.max_elements, sizeof(*ui->previous));
    ui->ids = calloc(config.max_elements, sizeof(*ui->ids));
    ui->activations = calloc(CUI_CORE_EVENT_MAX, sizeof(*ui->activations));
    ui->semantic_scopes = calloc(config.max_elements, sizeof(*ui->semantic_scopes));
    ui->previous_semantic_scopes =
        calloc(config.max_elements, sizeof(*ui->previous_semantic_scopes));
    ui->strings = malloc(config.frame_bytes);
    ui->clip_containers = calloc(config.max_elements, sizeof(*ui->clip_containers));
    ui->nodes = calloc(config.max_elements, sizeof(*ui->nodes));
    ui->bounds_tracks = calloc(config.max_elements, sizeof(*ui->bounds_tracks));
    ui->semantic_nodes = calloc(config.max_elements, sizeof(*ui->semantic_nodes));
    ui->previous_hits = calloc(config.max_elements, sizeof(*ui->previous_hits));
    ui->previous_layers = calloc(config.max_elements, sizeof(*ui->previous_layers));
    Clay_Context *saved = Clay_GetCurrentContext();
    Clay_SetCurrentContext(NULL);
    /* Clay's ID map retains the prior layout until EndLayout retires old IDs.
     * Reserve both generations while CUI's own limits bound current elements. */
    Clay_SetMaxElementCount((int32_t)(config.max_elements * 2 + 32));
    size_t n = Clay_MinMemorySize();
    ui->arena = malloc(n);
    if (!ui->commands || !ui->semantics || !ui->previous || !ui->ids || !ui->activations ||
        !ui->semantic_scopes || !ui->previous_semantic_scopes || !ui->strings || !ui->nodes ||
        !ui->semantic_nodes || !ui->previous_hits || !ui->bounds_tracks || !ui->previous_layers ||
        !ui->clip_containers || !ui->arena) {
        Clay_SetCurrentContext(saved);
        cui_destroy(ui);
        return NULL;
    }
    ui->clay = Clay_Initialize(Clay_CreateArenaWithCapacityAndMemory(n, ui->arena),
                               (Clay_Dimensions){config.width, config.height},
                               (Clay_ErrorHandler){cui_core_clay_error, ui});
    if (ui->clay) {
        /* The pinned Clay version defaults to only 100 scroll/clip containers.
         * Replace its private array with bounded context-owned storage so clip
         * capacity obeys CUI's configured element budget. */
        ui->clay->scrollContainerDatas = (Clay__ScrollContainerDataInternalArray){
            .capacity = (int32_t)config.max_elements, .internalArray = ui->clip_containers};
        Clay_SetMeasureTextFunction(cui_core_measure, ui);
    }
    Clay_SetCurrentContext(saved);
    if (!ui->clay || ui->error) {
        cui_destroy(ui);
        return NULL;
    }
    return ui;
}
void cui_destroy(cui_context *ui) {
    if (!ui)
        return;
    if (Clay_GetCurrentContext() == ui->clay)
        Clay_SetCurrentContext(NULL);
    free(ui->semantic_scopes);
    free(ui->previous_semantic_scopes);
    free(ui->nodes);
    free(ui->bounds_tracks);
    free(ui->semantic_nodes);
    free(ui->previous_hits);
    free(ui->previous_layers);
    free(ui->commands);
    free(ui->semantics);
    free(ui->previous);
    free(ui->ids);
    free(ui->activations);
    free(ui->strings);
    free(ui->clip_containers);
    free(ui->arena);
    free(ui);
}
static bool cui_core_inside(cui_rect r, float x, float y) {
    return r.w > 0 && r.h > 0 && x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}
static bool cui_core_focusable(const cui_semantic *s) {
    if (s->disabled)
        return false;
    if (s->details.focusable)
        return true;
    switch (s->role) {
    case CUI_ROLE_BUTTON:
    case CUI_ROLE_LINK:
    case CUI_ROLE_CHECKBOX:
    case CUI_ROLE_SWITCH:
    case CUI_ROLE_RADIO:
    case CUI_ROLE_SCROLLBAR:
    case CUI_ROLE_SLIDER:
    case CUI_ROLE_TEXTBOX:
    case CUI_ROLE_TAB:
    case CUI_ROLE_OPTION:
    case CUI_ROLE_MENUITEM:
    case CUI_ROLE_MENUITEMCHECKBOX:
    case CUI_ROLE_MENUITEMRADIO:
    case CUI_ROLE_ROWHEADER:
    case CUI_ROLE_COLUMNHEADER:
    case CUI_ROLE_GRIDCELL:
    case CUI_ROLE_TREEITEM:
        return true;
    default:
        return false;
    }
}
static bool cui_core_scope_contains(const cui_core_focus_scope *scopes, size_t child,
                                    size_t ancestor) {
    if (ancestor == SIZE_MAX)
        return true;
    while (child != SIZE_MAX) {
        if (child == ancestor)
            return true;
        child = scopes[child].parent;
    }
    return false;
}
static bool cui_core_previous_allowed(cui_context *ui, size_t i) {
    return cui_core_scope_contains(ui->previous_focus_scopes, ui->previous_semantic_scopes[i],
                                   ui->previous_top_modal);
}
static cui_id cui_core_pointer_focus_target(cui_context *ui, size_t index) {
    const cui_semantic *s = &ui->previous[index];
    if (s->disabled)
        return 0;
    if (!s->details.focus_target)
        return cui_core_focusable(s) ? s->id : 0;
    for (size_t i = 0; i < ui->previous_count; i++)
        if (ui->previous[i].id == s->details.focus_target && cui_core_focusable(&ui->previous[i]) &&
            cui_core_previous_allowed(ui, i))
            return ui->previous[i].id;
    return 0;
}
static cui_id cui_core_hit(cui_context *ui, float x, float y) {
    cui_id hit = 0;
    int16_t layer = INT16_MIN;
    for (size_t i = 0; i < ui->previous_count; i++)
        if ((cui_core_pointer_focus_target(ui, i) ||
             (!ui->previous[i].disabled && ui->previous[i].details.pointer_activatable)) &&
            cui_core_previous_allowed(ui, i) && cui_core_inside(ui->previous_hits[i], x, y) &&
            ui->previous_layers[i] >= layer) {
            hit = ui->previous[i].id;
            layer = ui->previous_layers[i];
        }
    return hit;
}
static cui_id cui_core_scroll_hit(cui_context *ui, cui_event event) {
    cui_id hit = 0;
    int16_t layer = INT16_MIN, blocking_layer = INT16_MIN;
    for (size_t i = 0; i < ui->previous_count; ++i) {
        const cui_semantic *node = &ui->previous[i];
        if (!cui_core_previous_allowed(ui, i) ||
            !cui_core_inside(ui->previous_hits[i], event.x, event.y))
            continue;
        if ((cui_core_focusable(node) || (!node->disabled && node->details.pointer_activatable)) &&
            ui->previous_layers[i] > blocking_layer)
            blocking_layer = ui->previous_layers[i];
        cui_semantic_details d = node->details;
        bool can_x =
            (event.dx > 0 && d.scroll_x > 0) || (event.dx < 0 && d.scroll_x < d.scroll_max_x);
        bool can_y =
            (event.dy > 0 && d.scroll_y > 0) || (event.dy < 0 && d.scroll_y < d.scroll_max_y);
        if (d.scroll_axis_lock) {
            bool vertical = fabsf(event.dy) >= fabsf(event.dx);
            if (d.orientation == CUI_ORIENTATION_VERTICAL) {
                can_x = false;
                can_y = can_y && vertical;
            } else if (d.orientation == CUI_ORIENTATION_HORIZONTAL) {
                can_x = can_x && !vertical;
                can_y = false;
            }
        }
        if (!node->disabled && d.scrollable && (can_x || can_y) &&
            ui->previous_layers[i] >= layer) {
            hit = node->id;
            layer = ui->previous_layers[i];
        }
    }
    return layer >= blocking_layer ? hit : 0;
}
static void cui_core_activate(cui_context *ui, cui_id id) {
    if (id && ui->activation_count < CUI_CORE_EVENT_MAX)
        ui->activations[ui->activation_count++] = id;
}
static void cui_core_tab(cui_context *ui, bool reverse) {
    size_t found = ui->previous_count;
    for (size_t i = 0; i < ui->previous_count; i++)
        if (ui->previous[i].id == ui->focus)
            found = i;
    for (size_t step = 1; step <= ui->previous_count; step++) {
        size_t i = reverse ? (found == ui->previous_count
                                  ? ui->previous_count - step
                                  : (found + ui->previous_count - step) % ui->previous_count)
                           : (found == ui->previous_count ? step - 1
                                                          : (found + step) % ui->previous_count);
        if (cui_core_focusable(&ui->previous[i]) && ui->previous[i].tab_index >= 0 &&
            cui_core_previous_allowed(ui, i)) {
            ui->focus = ui->previous[i].id;
            break;
        }
    }
    ui->focus_visible = true;
}
bool cui_queue_event(cui_context *ui, cui_event e) {
    if (!ui)
        return false;
    if (ui->in_frame) {
        cui_core_fail(ui, CUI_ERROR_LIFECYCLE);
        return false;
    }
    if (ui->queued_count == CUI_CORE_EVENT_MAX) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return false;
    }
    ui->queued[ui->queued_count++] = e;
    ui->dirty = true;
    return true;
}
bool cui_begin_frame(cui_context *ui, float w, float h, float dt) {
    if (!ui)
        return false;
    if (ui->in_frame) {
        cui_core_fail(ui, CUI_ERROR_LIFECYCLE);
        return false;
    }
    if (!isfinite(w) || !isfinite(h) || w < 0 || h < 0 || !isfinite(dt) || dt < 0) {
        cui_core_fail(ui, CUI_ERROR_LAYOUT);
        return false;
    }
    if (ui->previous_top_modal != SIZE_MAX) {
        bool allowed = false;
        for (size_t i = 0; i < ui->previous_count; i++)
            if (ui->previous[i].id == ui->focus && cui_core_focusable(&ui->previous[i]) &&
                cui_core_previous_allowed(ui, i))
                allowed = true;
        if (!allowed) {
            ui->focus = 0;
            for (size_t i = 0; i < ui->previous_count; i++)
                if (cui_core_focusable(&ui->previous[i]) && cui_core_previous_allowed(ui, i)) {
                    ui->focus = ui->previous[i].id;
                    break;
                }
        }
    }
    ui->event_count = ui->queued_count;
    memcpy(ui->events, ui->queued, ui->queued_count * sizeof(*ui->events));
    ui->queued_count = 0;
    ui->activation_count = 0;
    for (size_t i = 0; i < ui->event_count; i++) {
        cui_event e = ui->events[i];
        size_t before_activation = ui->activation_count;
        ui->event_activations[i] = 0;
        ui->event_targets[i] = ui->focus;
        if (e.type == CUI_EVENT_POINTER_MOVE || e.type == CUI_EVENT_POINTER_UP)
            ui->event_targets[i] = ui->capture;
        if (e.type == CUI_EVENT_POINTER_DOWN ||
            (e.type == CUI_EVENT_POINTER_UP && e.pointer_button != CUI_POINTER_PRIMARY)) {
            ui->event_targets[i] = cui_core_hit(ui, e.x, e.y);
            if (!ui->event_targets[i] && ui->previous_top_modal != SIZE_MAX)
                ui->event_targets[i] = ui->previous_focus_scopes[ui->previous_top_modal].id;
        }
        if (e.type == CUI_EVENT_SCROLL)
            ui->event_targets[i] = cui_core_scroll_hit(ui, e);
        if (e.type == CUI_EVENT_KEY_DOWN && e.key == CUI_KEY_ESCAPE &&
            ui->previous_focus_scope_count) {
            size_t scope = ui->previous_focus_scope_count;
            while (scope && !ui->previous_focus_scopes[scope - 1].active)
                scope--;
            if (scope)
                scope--;
            for (size_t j = 0; j < ui->previous_count; j++)
                if (ui->previous[j].id == ui->focus &&
                    ui->previous_semantic_scopes[j] != SIZE_MAX) {
                    scope = ui->previous_semantic_scopes[j];
                    break;
                }
            if (scope < ui->previous_focus_scope_count && ui->previous_focus_scopes[scope].active)
                ui->event_targets[i] = ui->previous_focus_scopes[scope].id;
        }
        if (e.type == CUI_EVENT_POINTER_MOVE || e.type == CUI_EVENT_POINTER_DOWN ||
            e.type == CUI_EVENT_POINTER_UP) {
            ui->pointer_x = e.x;
            ui->pointer_y = e.y;
        }
        if (e.type == CUI_EVENT_POINTER_DOWN && e.pointer_button == CUI_POINTER_PRIMARY) {
            cui_id hit = cui_core_hit(ui, e.x, e.y), target = hit;
            bool pointer_only = false;
            for (size_t n = 0; n < ui->previous_count; n++)
                if (ui->previous[n].id == hit) {
                    target = cui_core_pointer_focus_target(ui, n);
                    pointer_only = !target && ui->previous[n].details.pointer_activatable;
                    break;
                }
            ui->capture = pointer_only || target == hit ? hit : 0;
            if (!pointer_only) {
                if (hit || ui->previous_top_modal == SIZE_MAX)
                    ui->focus = target;
                ui->focus_visible = false;
            }
        } else if (e.type == CUI_EVENT_POINTER_UP && e.pointer_button == CUI_POINTER_PRIMARY) {
            if (ui->capture == cui_core_hit(ui, e.x, e.y))
                cui_core_activate(ui, ui->capture);
            ui->capture = 0;
        } else if (e.type == CUI_EVENT_CANCEL) {
            ui->capture = 0;
            ui->space_capture = 0;
        } else if (e.type == CUI_EVENT_KEY_DOWN && !e.repeat) {
            if (e.key == CUI_KEY_TAB)
                cui_core_tab(ui, (e.modifiers & CUI_MOD_SHIFT) != 0);
            else if (e.key == CUI_KEY_ENTER)
                cui_core_activate(ui, ui->focus);
            else if (e.key == CUI_KEY_SPACE)
                ui->space_capture = ui->focus;
            else if (e.key == CUI_KEY_ESCAPE) {
                ui->capture = 0;
                ui->space_capture = 0;
            }
        } else if (e.type == CUI_EVENT_KEY_UP && e.key == CUI_KEY_SPACE) {
            if (ui->space_capture == ui->focus)
                cui_core_activate(ui, ui->focus);
            ui->space_capture = 0;
        }
        if (ui->activation_count > before_activation)
            ui->event_activations[i] = ui->activations[ui->activation_count - 1];
    }
    ui->node_count = 0;
    ui->bounds_track_count = 0;
    ui->overflow_depth = 0;
    ui->focus_depth = ui->focus_scope_count = 0;
    ui->top_modal = SIZE_MAX;
    ui->semantic_count = ui->id_count = ui->element_count = ui->scope_count = ui->depth =
        ui->string_used = 0;
    ui->count = 0;
    cui_core_select_context(ui);
    Clay_SetLayoutDimensions((Clay_Dimensions){w, h});
    /* Clay retains clipping containers between layouts. Reap absent IDs before
     * its ephemeral element storage is reset; CUI owns scrolling through box
     * child offsets, so do not feed Clay wheel deltas or enable drag momentum. */
    Clay_UpdateScrollContainers(false, (Clay_Vector2){0, 0}, 0);
    /* CUI does not retain Clay scroll positions. Start registrations afresh so
     * changing every clipped ID cannot temporarily consume two frames' budget. */
    ui->clay->scrollContainerDatas.length = 0;
    Clay_BeginLayout();
    ui->config.width = w;
    ui->config.height = h;
    ui->dt = dt;
    ui->in_frame = true;
    ui->dirty = false;
    return true;
}
static uint32_t cui_core_folded(cui_id id) {
    /* Avalanche in a CUI domain: Clay auto IDs combine parent ID + child
       ordinal, so consecutive raw public IDs can alias auto text IDs. */
    uint64_t x = id ^ UINT64_C(0x4355492d454c454d);
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x ^= x >> 31;
    uint32_t out = (uint32_t)x ^ (uint32_t)(x >> 32);
    return out ? out : 1;
}
static Clay_SizingAxis cui_core_axis(float n) {
    if (n > 0)
        return (Clay_SizingAxis){.size = {.minMax = {n, n}}, .type = CLAY__SIZING_TYPE_FIXED};
    return (Clay_SizingAxis){.size = {.minMax = {0, 0}},
                             .type = n < 0 ? CLAY__SIZING_TYPE_GROW : CLAY__SIZING_TYPE_FIT};
}
static Clay_SizingAxis cui_core_box_axis(float n, bool fixed, float maximum) {
    if (!fixed) {
        Clay_SizingAxis axis = cui_core_axis(n);
        if (isfinite(maximum) && maximum > 0) {
            axis.size.minMax.max = maximum;
            axis.size.minMax.min = fminf(axis.size.minMax.min, maximum);
        }
        return axis;
    }
    n = isfinite(n) ? fmaxf(0, n) : 0;
    if (isfinite(maximum) && maximum > 0)
        n = fminf(n, maximum);
    /* Clay normalizes fixed max=0 to unbounded. Zero percent is exactly zero
     * for every parent extent and avoids an epsilon-sized collapsed panel. */
    if (n == 0)
        return (Clay_SizingAxis){.size = {.percent = 0}, .type = CLAY__SIZING_TYPE_PERCENT};
    return cui_core_axis(n);
}
static uint16_t cui_core_scalar(float x) {
    return !isfinite(x) || x < 0 ? 0 : x > 65535 ? 65535 : (uint16_t)x;
}
static void cui_core_begin_box(cui_context *ui, cui_id id, cui_box b, const cui_rect *portal,
                               int16_t layer, bool absolute) {
    if (!cui_core_active(ui))
        return;
    if (ui->depth == CUI_CORE_STACK_MAX || ui->overflow_depth) {
        ui->overflow_depth++;
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return;
    }
    bool ok = ui->element_count < ui->config.max_elements &&
              (ui->depth == 0 || ui->opened[ui->depth - 1]);
    if (id)
        for (size_t i = 0; i < ui->id_count; i++)
            if (ui->ids[i] == id || cui_core_folded(ui->ids[i]) == cui_core_folded(id)) {
                cui_core_fail(ui, CUI_ERROR_DUPLICATE_ID);
                ok = false;
                break;
            }
    ui->opened[ui->depth++] = ok;
    if (!ok) {
        if (ui->element_count >= ui->config.max_elements)
            cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return;
    }
    ui->element_count++;
    if (id)
        ui->ids[ui->id_count++] = id;
    uint32_t internal_id = cui_core_folded(id ? id : (cui_id)(ui->element_count + 0x80000000u));
    size_t parent = ui->depth > 1 ? ui->node_stack[ui->depth - 2] : SIZE_MAX;
    bool layout_only = b.layout_only || (parent != SIZE_MAX && ui->nodes[parent].layout_only);
    bool inert = layout_only || b.inert || (parent != SIZE_MAX && ui->nodes[parent].inert);
    if (portal && !absolute)
        parent = SIZE_MAX;
    else if (parent != SIZE_MAX)
        layer = ui->nodes[parent].layer;
    ui->nodes[ui->node_count] = (cui_core_box_node){.clay_id = internal_id,
                                                    .parent = parent,
                                                    .clip = b.clip,
                                                    .layout_only = layout_only,
                                                    .inert = inert,
                                                    .layer = layer};
    ui->node_stack[ui->depth - 1] = ui->node_count++;
    Clay__OpenElementWithId((Clay_ElementId){.id = internal_id});
    uint16_t bw = cui_core_scalar(b.border_width);
    cui_corner_radii cr = b.custom_corners
                              ? b.corner_radii
                              : (cui_corner_radii){b.radius, b.radius, b.radius, b.radius};
    float *radii[] = {&cr.top_left, &cr.top_right, &cr.bottom_left, &cr.bottom_right};
    for (size_t i = 0; i < 4; i++)
        *radii[i] = isfinite(*radii[i]) ? fmaxf(0, *radii[i]) : 0;
    Clay_BorderWidth edges = b.custom_border_edges
                                 ? (Clay_BorderWidth){cui_core_scalar(b.border_edges.left),
                                                      cui_core_scalar(b.border_edges.right),
                                                      cui_core_scalar(b.border_edges.top),
                                                      cui_core_scalar(b.border_edges.bottom), 0}
                                 : (Clay_BorderWidth){bw, bw, bw, bw, 0};
    Clay__ConfigureOpenElement((Clay_ElementDeclaration){
        .userData = layout_only ? ui : NULL,
        .layout = {.sizing = {cui_core_box_axis(b.width, b.fixed_width, b.max_width),
                              cui_core_box_axis(b.height, b.fixed_height, b.max_height)},
                   .padding = CLAY_PADDING_ALL(cui_core_scalar(b.padding)),
                   .childGap = cui_core_scalar(b.gap),
                   .childAlignment = {.x = b.align_x == CUI_ALIGN_CENTER ? CLAY_ALIGN_X_CENTER
                                           : b.align_x == CUI_ALIGN_END  ? CLAY_ALIGN_X_RIGHT
                                                                         : CLAY_ALIGN_X_LEFT,
                                      .y = b.align_y == CUI_ALIGN_CENTER ? CLAY_ALIGN_Y_CENTER
                                           : b.align_y == CUI_ALIGN_END  ? CLAY_ALIGN_Y_BOTTOM
                                                                         : CLAY_ALIGN_Y_TOP},
                   .layoutDirection = b.row ? CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM},
        .backgroundColor = {b.background.r, b.background.g, b.background.b, b.background.a},
        .cornerRadius = {cr.top_left, cr.top_right, cr.bottom_left, cr.bottom_right},
        .floating = {.offset = portal ? (Clay_Vector2){portal->x, portal->y} : (Clay_Vector2){0},
                     .zIndex = layer,
                     .attachTo = absolute ? CLAY_ATTACH_TO_PARENT
                                 : portal ? CLAY_ATTACH_TO_ROOT
                                          : CLAY_ATTACH_TO_NONE,
                     .clipTo = absolute ? CLAY_CLIP_TO_ATTACHED_PARENT : CLAY_CLIP_TO_NONE},
        .clip = {.horizontal = b.clip,
                 .vertical = b.clip,
                 .childOffset = {isfinite(b.offset_x) ? b.offset_x : 0,
                                 isfinite(b.offset_y) ? b.offset_y : 0}},
        .border = {.color = {b.border.r, b.border.g, b.border.b, b.border.a}, .width = edges}});
}
bool cui_bitmap_valid(cui_bitmap b) {
    return b.pixels && b.width && b.height && b.width <= 16384 && b.height <= 16384 &&
           b.stride >= (size_t)b.width * 4 && b.stride <= SIZE_MAX / b.height;
}
bool cui_paint_bitmap(cui_context *ui, cui_id id, cui_bitmap_paint paint, cui_box box) {
    if (!ui || !ui->in_frame || !cui_bitmap_valid(paint.bitmap) || !isfinite(paint.opacity) ||
        paint.opacity < 0 || paint.opacity > 1)
        return false;
    cui_rect *s = &paint.source;
    if (!s->x && !s->y && !s->w && !s->h)
        *s = (cui_rect){0, 0, (float)paint.bitmap.width, (float)paint.bitmap.height};
    if (!isfinite(s->x) || !isfinite(s->y) || !isfinite(s->w) || !isfinite(s->h) || s->x < 0 ||
        s->y < 0 || s->w <= 0 || s->h <= 0 || s->x + s->w > paint.bitmap.width ||
        s->y + s->h > paint.bitmap.height)
        return false;
    if (!box.width && !box.fixed_width)
        box.width = s->w;
    if (!box.height && !box.fixed_height)
        box.height = s->h;
    cui_core_begin_box(ui, id, box, NULL, 0, false);
    bool ok = !ui->overflow_depth && ui->depth && ui->opened[ui->depth - 1];
    if (ok) {
        cui_core_box_node *node = &ui->nodes[ui->node_stack[ui->depth - 1]];
        node->bitmap = paint;
        Clay__GetOpenLayoutElement()->config.image.imageData = &node->bitmap;
    }
    cui_end_box(ui);
    return ok;
}
bool cui_set_underlay_ex(cui_context *ui, cui_underlay_layout layout, cui_color color,
                         cui_corner_radii radii) {
    cui_rect bounds = layout.bounds;
    if (!ui || !ui->in_frame || !ui->depth || ui->overflow_depth || !ui->opened[ui->depth - 1] ||
        !isfinite(bounds.x) || !isfinite(bounds.y) || !isfinite(bounds.w) || !isfinite(bounds.h) ||
        (!layout.grow_width && bounds.w < 0) || (!layout.grow_height && bounds.h < 0) ||
        layout.horizontal < CUI_ALIGN_START || layout.horizontal > CUI_ALIGN_END ||
        layout.vertical < CUI_ALIGN_START || layout.vertical > CUI_ALIGN_END ||
        !isfinite(radii.top_left) || !isfinite(radii.top_right) || !isfinite(radii.bottom_left) ||
        !isfinite(radii.bottom_right) || radii.top_left < 0 || radii.top_right < 0 ||
        radii.bottom_left < 0 || radii.bottom_right < 0)
        return false;
    cui_core_box_node *node = &ui->nodes[ui->node_stack[ui->depth - 1]];
    node->underlay_bounds = bounds;
    node->underlay_layout = layout;
    node->underlay_color = color;
    node->underlay_radii = radii;
    Clay__GetOpenLayoutElement()->config.custom.customData = node;
    return true;
}
bool cui_set_underlay(cui_context *ui, cui_rect bounds, cui_color color, cui_corner_radii radii) {
    return cui_set_underlay_ex(ui, (cui_underlay_layout){.bounds = bounds}, color, radii);
}
void cui_begin_box(cui_context *ui, cui_id id, cui_box b) {
    cui_core_begin_box(ui, id, b, NULL, 0, false);
}
void cui_begin_portal(cui_context *ui, cui_id id, cui_box box, cui_rect bounds, int16_t layer) {
    if (!isfinite(bounds.x) || !isfinite(bounds.y) || !isfinite(bounds.w) || !isfinite(bounds.h) ||
        bounds.w < 0 || bounds.h < 0 || layer <= 0) {
        /* Preserve the begin/end stack contract while suppressing malformed content. */
        if (!cui_core_active(ui))
            return;
        size_t previous_depth = ui->depth;
        cui_begin_box(ui, id, (cui_box){0});
        if (!ui->overflow_depth && ui->depth > previous_depth && ui->opened[ui->depth - 1]) {
            Clay__CloseElement();
            ui->opened[ui->depth - 1] = false;
        }
        cui_core_fail(ui, CUI_ERROR_LAYOUT);
        return;
    }
    box.width = bounds.w;
    box.height = bounds.h;
    cui_core_begin_box(ui, id, box, &bounds, layer, false);
}
void cui_begin_absolute_box(cui_context *ui, cui_id id, cui_box box, cui_rect bounds) {
    if (!ui || !ui->in_frame)
        return;
    if (!ui->depth || !isfinite(bounds.x) || !isfinite(bounds.y) || !isfinite(bounds.w) ||
        !isfinite(bounds.h) || bounds.w < 0 || bounds.h < 0) {
        /* Reuse the balanced malformed-portal suppression path. */
        cui_begin_portal(ui, id, box, (cui_rect){0, 0, -1, 0}, 1);
        return;
    }
    box.width = bounds.w;
    box.height = bounds.h;
    box.fixed_width = box.fixed_height = true;
    cui_core_begin_box(ui, id, box, &bounds, 0, true);
}
void cui_end_box(cui_context *ui) {
    if (!cui_core_active(ui))
        return;
    if (ui->overflow_depth) {
        ui->overflow_depth--;
        return;
    }
    if (!ui->depth) {
        cui_core_fail(ui, CUI_ERROR_LIFECYCLE);
        return;
    }
    if (ui->opened[--ui->depth])
        Clay__CloseElement();
}
static cui_str cui_core_copy_string(cui_context *ui, cui_str s) {
    if (!s.length)
        return (cui_str){"", 0};
    if (!s.data || s.length > (size_t)INT32_MAX ||
        s.length >= ui->config.frame_bytes - ui->string_used) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return (cui_str){"", 0};
    }
    char *p = ui->strings + ui->string_used;
    memcpy(p, s.data, s.length);
    p[s.length] = 0;
    ui->string_used += s.length + 1;
    return (cui_str){p, s.length};
}
void cui_text_with_line_height(cui_context *ui, cui_str t, float size, cui_color c,
                               float line_height) {
    if (!cui_core_active(ui))
        return;
    if (ui->overflow_depth || (ui->depth && !ui->opened[ui->depth - 1]))
        return;
    if (ui->element_count == ui->config.max_elements) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return;
    }
    ui->element_count++;
    t = cui_core_copy_string(ui, t);
    Clay__OpenTextElement((Clay_String){false, (int32_t)t.length, t.data},
                          (Clay_TextElementConfig){.userData = cui_core_layout_only(ui) ? ui : NULL,
                                                   .fontSize = cui_core_scalar(size),
                                                   .lineHeight = cui_core_scalar(line_height),
                                                   .textColor = {c.r, c.g, c.b, c.a}});
}
void cui_text(cui_context *ui, cui_str text, float size, cui_color color) {
    cui_text_with_line_height(ui, text, size, color, 0);
}
static uint64_t cui_core_hash(uint64_t h, const char *s) {
    if (s)
        while (*s) {
            h ^= (unsigned char)*s++;
            h *= 1099511628211ULL;
        }
    return h ? h : 1;
}
cui_id cui_id_from(cui_context *ui, const char *s) {
    return cui_core_hash(
        ui && ui->scope_count ? ui->scopes[ui->scope_count - 1] : 14695981039346656037ULL, s);
}
bool cui_push_id(cui_context *ui, const char *key) {
    if (!cui_core_active(ui))
        return false;
    if (ui->scope_count == CUI_CORE_STACK_MAX) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return false;
    }
    uint64_t h = cui_id_from(ui, key);
    h ^= 255;
    h *= 1099511628211ULL;
    ui->scopes[ui->scope_count++] = h;
    return true;
}
void cui_pop_id(cui_context *ui) {
    if (!cui_core_active(ui))
        return;
    if (!ui->scope_count)
        cui_core_fail(ui, CUI_ERROR_LIFECYCLE);
    else
        ui->scope_count--;
}
cui_rect cui_bounds(cui_context *ui, cui_id id) {
    if (!ui || !id)
        return (cui_rect){0};
    for (size_t i = 0; i < ui->previous_count; i++)
        if (ui->previous[i].id == id)
            return ui->previous[i].bounds;
    cui_core_select_context(ui);
    Clay_ElementData d = Clay_GetElementData((Clay_ElementId){.id = cui_core_folded(id)});
    return (cui_rect){d.boundingBox.x, d.boundingBox.y, d.boundingBox.width, d.boundingBox.height};
}
bool cui_track_bounds(cui_context *ui, cui_id id, cui_rect *retained) {
    if (!ui || !id || !retained || !cui_core_active(ui))
        return false;
    for (size_t i = 0; i < ui->bounds_track_count; i++) {
        cui_core_bounds_track track = ui->bounds_tracks[i];
        if (track.id == id && track.retained == retained)
            return true;
        if (track.id == id || track.retained == retained) {
            cui_core_fail(ui, CUI_ERROR_DUPLICATE_ID);
            return false;
        }
    }
    if (ui->bounds_track_count == ui->config.max_elements) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return false;
    }
    ui->bounds_tracks[ui->bounds_track_count++] = (cui_core_bounds_track){id, retained};
    return true;
}
cui_interaction cui_interact(cui_context *ui, cui_id id, cui_role role, cui_str name, bool disabled,
                             bool checked, float value) {
    cui_interaction out = {0};
    if (!cui_core_active(ui) || !id || cui_core_inert(ui))
        return out;
    if (ui->overflow_depth || (ui->depth && !ui->opened[ui->depth - 1]))
        return out;
    if (ui->focus_depth && !ui->focus_scopes[ui->focus_stack[ui->focus_depth - 1]].active)
        return out;
    for (size_t i = 0; i < ui->semantic_count; i++)
        if (ui->semantics[i].id == id) {
            cui_core_fail(ui, CUI_ERROR_DUPLICATE_ID);
            return out;
        }
    if (ui->semantic_count == ui->config.max_elements) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return out;
    }
    if (disabled) {
        if (ui->focus == id)
            ui->focus = 0;
        if (ui->capture == id)
            ui->capture = 0;
        if (ui->space_capture == id)
            ui->space_capture = 0;
    }
    cui_rect bounds = cui_bounds(ui, id);
    if (!disabled) {
        out.hovered = cui_core_inside(bounds, ui->pointer_x, ui->pointer_y);
        out.pressed = ui->capture == id || ui->space_capture == id;
        out.focused = ui->focus == id;
        out.focus_visible = out.focused && ui->focus_visible;
    }
    for (size_t i = 0; i < ui->activation_count; i++)
        if (ui->activations[i] == id) {
            out.activated = !disabled;
            ui->activations[i] = 0;
        }
    ui->semantic_scopes[ui->semantic_count] =
        ui->focus_depth ? ui->focus_stack[ui->focus_depth - 1] : SIZE_MAX;
    ui->semantic_nodes[ui->semantic_count] =
        ui->depth && ui->opened[ui->depth - 1] ? ui->node_stack[ui->depth - 1] : SIZE_MAX;
    ui->semantics[ui->semantic_count++] = (cui_semantic){.id = id,
                                                         .role = role,
                                                         .bounds = bounds,
                                                         .name = cui_core_copy_string(ui, name),
                                                         .disabled = disabled,
                                                         .checked = checked,
                                                         .focused = out.focused,
                                                         .value = value};
    return out;
}
static cui_color cui_core_color(Clay_Color c) {
    return (cui_color){(uint8_t)c.r, (uint8_t)c.g, (uint8_t)c.b, (uint8_t)c.a};
}
cui_draw_list cui_end_frame(cui_context *ui) {
    if (!cui_core_active(ui))
        return (cui_draw_list){0};
    if (ui->depth || ui->overflow_depth || ui->scope_count || ui->focus_depth)
        cui_core_fail(ui, CUI_ERROR_LIFECYCLE);
    ui->overflow_depth = 0;
    while (ui->depth)
        cui_end_box(ui);
    Clay_RenderCommandArray a = Clay_EndLayout(ui->dt);
    ui->count = 0;
    uint32_t underlay_background_id = 0;
    for (int32_t i = 0; i < a.length; i++) {
        Clay_RenderCommand *c = &a.internalArray[i];
        if (c->userData == ui && c->commandType != CLAY_RENDER_COMMAND_TYPE_SCISSOR_START &&
            c->commandType != CLAY_RENDER_COMMAND_TYPE_SCISSOR_END)
            continue;
        cui_draw_command d = {.bounds = {c->boundingBox.x, c->boundingBox.y, c->boundingBox.width,
                                         c->boundingBox.height}};
        switch (c->commandType) {
        case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
            /* The custom underlay already painted this element background. */
            if (c->id == underlay_background_id)
                continue;
            d.type = CUI_DRAW_RECT;
            d.color = cui_core_color(c->renderData.rectangle.backgroundColor);
            d.radius = c->renderData.rectangle.cornerRadius.topLeft;
            d.corner_radii = (cui_corner_radii){c->renderData.rectangle.cornerRadius.topLeft,
                                                c->renderData.rectangle.cornerRadius.topRight,
                                                c->renderData.rectangle.cornerRadius.bottomLeft,
                                                c->renderData.rectangle.cornerRadius.bottomRight};
            d.custom_corners = true;
            break;
        case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
            d.type = CUI_DRAW_IMAGE;
            d.bitmap = *(const cui_bitmap_paint *)c->renderData.image.imageData;
            Clay_CornerRadius radius = c->renderData.image.cornerRadius;
            d.corner_radii = (cui_corner_radii){radius.topLeft, radius.topRight, radius.bottomLeft,
                                                radius.bottomRight};
            d.custom_corners = true;
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_BORDER:
            d.type = CUI_DRAW_BORDER;
            d.color = cui_core_color(c->renderData.border.color);
            d.radius = c->renderData.border.cornerRadius.topLeft;
            d.stroke = c->renderData.border.width.left;
            d.corner_radii = (cui_corner_radii){c->renderData.border.cornerRadius.topLeft,
                                                c->renderData.border.cornerRadius.topRight,
                                                c->renderData.border.cornerRadius.bottomLeft,
                                                c->renderData.border.cornerRadius.bottomRight};
            d.border_edges = (cui_border_edges){
                c->renderData.border.width.left, c->renderData.border.width.right,
                c->renderData.border.width.top, c->renderData.border.width.bottom};
            d.custom_corners = d.custom_border_edges = true;
            break;
        case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
            underlay_background_id = c->id;
            const cui_core_box_node *node = c->renderData.custom.customData;
            cui_color background = cui_core_color(c->renderData.custom.backgroundColor);
            size_t needed = 3 + (background.a ? 1 : 0);
            if (needed > ui->config.max_commands - ui->count) {
                cui_core_fail(ui, CUI_ERROR_CAPACITY);
                ui->count = 0;
                goto cui_core_commands_done;
            }
            if (background.a) {
                Clay_CornerRadius radius = c->renderData.custom.cornerRadius;
                ui->commands[ui->count++] =
                    (cui_draw_command){.type = CUI_DRAW_RECT,
                                       .bounds = d.bounds,
                                       .color = background,
                                       .custom_corners = true,
                                       .corner_radii = {radius.topLeft, radius.topRight,
                                                        radius.bottomLeft, radius.bottomRight}};
            }
            ui->commands[ui->count++] =
                (cui_draw_command){.type = CUI_DRAW_CLIP_PUSH, .bounds = d.bounds};
            cui_underlay_layout layout = node->underlay_layout;
            cui_rect underlay = node->underlay_bounds;
            if (layout.grow_width)
                underlay.w = fmaxf(0, d.bounds.w + underlay.w);
            if (layout.grow_height)
                underlay.h = fmaxf(0, d.bounds.h + underlay.h);
            float align_x = layout.horizontal == CUI_ALIGN_END      ? 1
                            : layout.horizontal == CUI_ALIGN_CENTER ? .5f
                                                                    : 0;
            float align_y = layout.vertical == CUI_ALIGN_END      ? 1
                            : layout.vertical == CUI_ALIGN_CENTER ? .5f
                                                                  : 0;
            underlay.x += d.bounds.x + (d.bounds.w - underlay.w) * align_x;
            underlay.y += d.bounds.y + (d.bounds.h - underlay.h) * align_y;
            ui->commands[ui->count++] = (cui_draw_command){.type = CUI_DRAW_RECT,
                                                           .bounds = underlay,
                                                           .color = node->underlay_color,
                                                           .corner_radii = node->underlay_radii,
                                                           .custom_corners = true};
            ui->commands[ui->count++] = (cui_draw_command){.type = CUI_DRAW_CLIP_POP};
            continue;
        }
        case CLAY_RENDER_COMMAND_TYPE_TEXT:
            d.type = CUI_DRAW_TEXT;
            d.text = (cui_str){c->renderData.text.stringContents.chars,
                               (size_t)c->renderData.text.stringContents.length};
            d.font_size = c->renderData.text.fontSize;
            d.color = cui_core_color(c->renderData.text.textColor);
            break;
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
            d.type = CUI_DRAW_CLIP_PUSH;
            break;
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
            d.type = CUI_DRAW_CLIP_POP;
            break;
        default:
            continue;
        }
        if (ui->count == ui->config.max_commands) {
            cui_core_fail(ui, CUI_ERROR_CAPACITY);
            ui->count = 0;
            break;
        }
        ui->commands[ui->count++] = d;
    }
cui_core_commands_done:
    for (size_t i = 0; i < ui->bounds_track_count; i++) {
        cui_core_bounds_track track = ui->bounds_tracks[i];
        uint32_t folded = cui_core_folded(track.id);
        cui_rect bounds = {0};
        for (size_t n = 0; n < ui->node_count; n++) {
            if (ui->nodes[n].clay_id == folded) {
                Clay_ElementData data = Clay_GetElementData((Clay_ElementId){.id = folded});
                bounds = (cui_rect){data.boundingBox.x, data.boundingBox.y, data.boundingBox.width,
                                    data.boundingBox.height};
                break;
            }
        }
        cui_rect previous = *track.retained;
        if (previous.x != bounds.x || previous.y != bounds.y || previous.w != bounds.w ||
            previous.h != bounds.h)
            ui->dirty = true;
        *track.retained = bounds;
    }
    bool retained = false, captured = false, space = false;
    for (size_t i = 0; i < ui->semantic_count; i++) {
        cui_semantic *s = &ui->semantics[i];
        Clay_ElementData d = Clay_GetElementData((Clay_ElementId){.id = cui_core_folded(s->id)});
        s->bounds =
            (cui_rect){d.boundingBox.x, d.boundingBox.y, d.boundingBox.width, d.boundingBox.height};
        cui_rect clipped = s->bounds;
        for (size_t n = ui->semantic_nodes[i]; n != SIZE_MAX; n = ui->nodes[n].parent) {
            if (ui->nodes[n].clip) {
                Clay_ElementData cd =
                    Clay_GetElementData((Clay_ElementId){.id = ui->nodes[n].clay_id});
                float left = fmaxf(clipped.x, cd.boundingBox.x),
                      top = fmaxf(clipped.y, cd.boundingBox.y);
                float right = fminf(clipped.x + clipped.w, cd.boundingBox.x + cd.boundingBox.width),
                      bottom =
                          fminf(clipped.y + clipped.h, cd.boundingBox.y + cd.boundingBox.height);
                clipped = (cui_rect){left, top, fmaxf(0, right - left), fmaxf(0, bottom - top)};
            }
        }
        ui->previous_hits[i] = clipped;
        ui->previous_layers[i] =
            ui->semantic_nodes[i] == SIZE_MAX ? 0 : ui->nodes[ui->semantic_nodes[i]].layer;
        if (cui_core_focusable(s)) {
            retained |= s->id == ui->focus;
            if (cui_core_scope_contains(ui->focus_scopes, ui->semantic_scopes[i], ui->top_modal)) {
                captured |= s->id == ui->capture;
                space |= s->id == ui->space_capture;
            }
        }
        s->focused = s->id == ui->focus && !s->disabled;
    }
    if (!retained)
        ui->focus = 0;
    if (ui->top_modal != SIZE_MAX) {
        bool allowed = false;
        for (size_t i = 0; i < ui->semantic_count; i++)
            if (ui->semantics[i].id == ui->focus && cui_core_focusable(&ui->semantics[i]) &&
                cui_core_scope_contains(ui->focus_scopes, ui->semantic_scopes[i], ui->top_modal))
                allowed = true;
        if (!allowed) {
            ui->focus = 0;
            for (size_t i = 0; i < ui->semantic_count; i++)
                if (cui_core_focusable(&ui->semantics[i]) &&
                    cui_core_scope_contains(ui->focus_scopes, ui->semantic_scopes[i],
                                            ui->top_modal)) {
                    ui->focus = ui->semantics[i].id;
                    ui->focus_visible = true;
                    ui->dirty = true;
                    break;
                }
        }
    }
    for (size_t i = 0; i < ui->semantic_count; i++)
        ui->semantics[i].focused = ui->semantics[i].id == ui->focus && !ui->semantics[i].disabled;
    if (!captured)
        ui->capture = 0;
    if (!space)
        ui->space_capture = 0;
    memcpy(ui->previous_focus_scopes, ui->focus_scopes,
           ui->focus_scope_count * sizeof(*ui->focus_scopes));
    ui->previous_focus_scope_count = ui->focus_scope_count;
    ui->previous_top_modal = ui->top_modal;
    memcpy(ui->previous_semantic_scopes, ui->semantic_scopes,
           ui->semantic_count * sizeof(*ui->semantic_scopes));
    memcpy(ui->previous, ui->semantics, ui->semantic_count * sizeof(*ui->previous));
    ui->previous_count = ui->semantic_count;
    ui->in_frame = false;
    return (cui_draw_list){ui->commands, ui->count, ui->config.width, ui->config.height};
}
const cui_semantic *cui_semantics(cui_context *ui, size_t *count) {
    if (count)
        *count = ui ? ui->semantic_count : 0;
    return ui ? ui->semantics : NULL;
}
cui_id cui_focused(cui_context *ui) {
    return ui && !cui_core_inert(ui) ? ui->focus : 0;
}
void cui_focus(cui_context *ui, cui_id id) {
    if (ui && !cui_core_inert(ui)) {
        ui->focus = id;
        ui->focus_visible = true;
        ui->dirty = true;
    }
}
const cui_event *cui_frame_events(cui_context *ui, size_t *count) {
    if (count)
        *count = ui ? ui->event_count : 0;
    return ui ? ui->events : NULL;
}
cui_error cui_last_error(cui_context *ui) {
    return ui ? ui->error : CUI_ERROR_LIFECYCLE;
}
void cui_clear_error(cui_context *ui) {
    if (ui)
        ui->error = CUI_OK;
}
bool cui_needs_redraw(cui_context *ui) {
    return ui && ui->dirty;
}
void cui_invalidate(cui_context *ui) {
    if (ui)
        ui->dirty = true;
}

void cui_update_semantic(cui_context *ui, cui_id id, bool checked, float value) {
    if (!cui_core_active(ui))
        return;
    for (size_t i = 0; i < ui->semantic_count; i++)
        if (ui->semantics[i].id == id) {
            ui->semantics[i].checked = checked;
            ui->semantics[i].value = value;
            return;
        }
}
cui_id cui_event_target(cui_context *ui, size_t index) {
    return ui && !cui_core_inert(ui) && index < ui->event_count ? ui->event_targets[index] : 0;
}

cui_id cui_event_activation(cui_context *ui, size_t index) {
    return ui && !cui_core_inert(ui) && index < ui->event_count ? ui->event_activations[index] : 0;
}

bool cui_pointer_scope_allowed(cui_context *ui, cui_id owned_scope) {
    if (!ui || !ui->in_frame || cui_core_inert(ui))
        return false;
    size_t current = ui->focus_depth ? ui->focus_stack[ui->focus_depth - 1] : SIZE_MAX;
    if (ui->top_modal != SIZE_MAX && ui->focus_scopes[ui->top_modal].active &&
        ui->focus_scopes[ui->top_modal].id != owned_scope &&
        !cui_core_scope_contains(ui->focus_scopes, current, ui->top_modal))
        return false;
    if (ui->previous_top_modal == SIZE_MAX)
        return true;
    cui_id previous = ui->previous_focus_scopes[ui->previous_top_modal].id;
    if (previous == owned_scope)
        return true;
    while (current != SIZE_MAX) {
        if (ui->focus_scopes[current].active && ui->focus_scopes[current].id == previous)
            return true;
        current = ui->focus_scopes[current].parent;
    }
    return false;
}
bool cui_begin_focus_scope(cui_context *ui, cui_id id, bool modal) {
    if (!cui_core_active(ui))
        return false;
    if (!id) {
        cui_core_fail(ui, CUI_ERROR_LIFECYCLE);
        return false;
    }
    if (ui->focus_depth == CUI_CORE_STACK_MAX || ui->focus_scope_count == CUI_CORE_STACK_MAX) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return false;
    }
    for (size_t i = 0; i < ui->focus_scope_count; i++)
        if (ui->focus_scopes[i].id == id) {
            cui_core_fail(ui, CUI_ERROR_DUPLICATE_ID);
            return false;
        }
    bool active = !cui_core_inert(ui);
    size_t n = ui->focus_scope_count++;
    ui->focus_scopes[n] = (cui_core_focus_scope){
        id, ui->focus_depth ? ui->focus_stack[ui->focus_depth - 1] : SIZE_MAX, modal, active};
    ui->focus_stack[ui->focus_depth++] = n;
    if (modal && active)
        ui->top_modal = n;
    return true;
}
void cui_end_focus_scope(cui_context *ui) {
    if (!cui_core_active(ui))
        return;
    if (!ui->focus_depth)
        cui_core_fail(ui, CUI_ERROR_LIFECYCLE);
    else
        ui->focus_depth--;
}
bool cui_focus_within(cui_context *ui, cui_id scope) {
    if (!ui || !scope || !ui->focus || cui_core_inert(ui))
        return false;
    for (size_t i = 0; i < ui->focus_scope_count; i++)
        if (ui->focus_scopes[i].id == scope && !ui->focus_scopes[i].active)
            return false;
    for (int pass = 0; pass < 2; pass++) {
        const cui_core_focus_scope *scopes = pass ? ui->previous_focus_scopes : ui->focus_scopes;
        size_t scope_count = pass ? ui->previous_focus_scope_count : ui->focus_scope_count;
        const cui_semantic *sem = pass ? ui->previous : ui->semantics;
        const size_t *owners = pass ? ui->previous_semantic_scopes : ui->semantic_scopes;
        size_t count = pass ? ui->previous_count : ui->semantic_count;
        for (size_t i = 0; i < count; i++)
            if (sem[i].id == ui->focus) {
                if (sem[i].disabled)
                    return false;
                size_t owner = owners[i];
                while (owner < scope_count) {
                    if (!scopes[owner].active)
                        return false;
                    if (scopes[owner].id == scope)
                        return true;
                    owner = scopes[owner].parent;
                }
                return false;
            }
    }
    return false;
}
bool cui_hover_within(cui_context *ui, cui_id scope) {
    if (!ui || !scope || cui_core_inert(ui))
        return false;
    for (size_t i = 0; i < ui->focus_scope_count; i++)
        if (ui->focus_scopes[i].id == scope && !ui->focus_scopes[i].active)
            return false;
    size_t hit = SIZE_MAX;
    int16_t layer = INT16_MIN;
    for (size_t i = 0; i < ui->previous_count; i++)
        if (cui_core_previous_allowed(ui, i) && ui->previous_layers[i] >= layer &&
            cui_core_inside(ui->previous_hits[i], ui->pointer_x, ui->pointer_y)) {
            hit = i;
            layer = ui->previous_layers[i];
        }
    if (hit == SIZE_MAX)
        return false;
    size_t owner = ui->previous_semantic_scopes[hit];
    while (owner < ui->previous_focus_scope_count) {
        const cui_core_focus_scope *s = &ui->previous_focus_scopes[owner];
        if (!s->active)
            return false;
        if (s->id == scope)
            return true;
        owner = s->parent;
    }
    return false;
}
bool cui_focus_restore(cui_context *ui, cui_id trigger) {
    if (!ui)
        return false;
    for (size_t i = 0; i < ui->previous_count; i++)
        if (ui->previous[i].id == trigger && cui_core_focusable(&ui->previous[i])) {
            ui->focus = trigger;
            ui->focus_visible = true;
            ui->dirty = true;
            return true;
        }
    ui->focus = 0;
    return false;
}

bool cui_set_semantic_role(cui_context *ui, cui_id id, cui_role role) {
    if (!cui_core_active(ui) || !id || cui_core_inert(ui) || role < CUI_ROLE_NONE ||
        role > CUI_ROLE_LOG)
        return false;
    for (size_t i = 0; i < ui->semantic_count; i++)
        if (ui->semantics[i].id == id) {
            ui->semantics[i].role = role;
            return true;
        }
    return false;
}
void cui_set_semantic_details(cui_context *ui, cui_id id, cui_semantic_details details) {
    if (!cui_core_active(ui))
        return;
    for (size_t i = 0; i < ui->semantic_count; i++)
        if (ui->semantics[i].id == id) {
            details.description = cui_core_copy_string(ui, details.description);
            ui->semantics[i].details = details;
            return;
        }
}
void cui_set_tab_stop(cui_context *ui, cui_id id, bool enabled) {
    if (!cui_core_active(ui))
        return;
    for (size_t i = 0; i < ui->semantic_count; i++)
        if (ui->semantics[i].id == id) {
            ui->semantics[i].tab_index = enabled ? 0 : -1;
            return;
        }
}

bool cui_pressed(cui_context *ui, cui_id id) {
    return ui && id && !cui_core_inert(ui) && (ui->capture == id || ui->space_capture == id);
}

bool cui_hovered(cui_context *ui, cui_id id) {
    if (!ui || !id || cui_core_inert(ui))
        return false;
    for (size_t i = 0; i < ui->previous_count; i++)
        if (ui->previous[i].id == id && cui_core_previous_allowed(ui, i) &&
            cui_core_inside(ui->previous_hits[i], ui->pointer_x, ui->pointer_y)) {
            for (size_t j = 0; j < ui->previous_count; j++)
                if (ui->previous_layers[j] > ui->previous_layers[i] &&
                    cui_core_previous_allowed(ui, j) && cui_core_focusable(&ui->previous[j]) &&
                    cui_core_inside(ui->previous_hits[j], ui->pointer_x, ui->pointer_y))
                    return false;
            return true;
        }
    return false;
}

bool cui_dismiss_focus_scope(cui_context *ui, cui_id scope, cui_id trigger) {
    if (!cui_core_active(ui) || cui_core_inert(ui))
        return false;
    size_t dismissed = SIZE_MAX;
    for (size_t i = 0; i < ui->focus_scope_count; i++)
        if (ui->focus_scopes[i].id == scope && ui->focus_scopes[i].active)
            dismissed = i;
    if (dismissed == SIZE_MAX)
        return false;
    for (size_t i = 0; i < ui->focus_scope_count; i++)
        if (cui_core_scope_contains(ui->focus_scopes, i, dismissed))
            ui->focus_scopes[i].active = false;
    size_t retained = 0;
    for (size_t i = 0; i < ui->semantic_count; i++) {
        if (cui_core_scope_contains(ui->focus_scopes, ui->semantic_scopes[i], dismissed))
            continue;
        ui->semantics[retained] = ui->semantics[i];
        ui->semantic_scopes[retained] = ui->semantic_scopes[i];
        ui->semantic_nodes[retained] = ui->semantic_nodes[i];
        retained++;
    }
    ui->semantic_count = retained;
    ui->top_modal = SIZE_MAX;
    for (size_t i = 0; i < ui->focus_scope_count; i++)
        if (ui->focus_scopes[i].active && ui->focus_scopes[i].modal)
            ui->top_modal = i;
    cui_focus_restore(ui, trigger);
    ui->dirty = true;
    return true;
}
