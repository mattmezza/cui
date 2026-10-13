/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <float.h>
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
#define CUI_CORE_DISPATCH_MAX (2 * CUI_CORE_EVENT_MAX)
typedef struct {
    uint32_t clay_id;
    cui_id id;
    size_t parent, focus_scope;
    bool hover_hitbox;
    size_t prepaint;
    bool clip, layout_only, inert;
    int16_t layer;
    float opacity;
    uint32_t paint_root;
    cui_bitmap_paint bitmap;
    cui_mesh_paint mesh;
    bool has_mesh;
    bool bitmap_underlay, has_underlay;
    float outline_width;
    cui_color outline_color;
    cui_corner_radii outline_radii;
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
    size_t node, offset, count, budget;
    cui_prepaint_options options;
    cui_prepaint_info info;
    bool done, deferred_done;
} cui_core_prepaint;
typedef struct {
    cui_rect bounds, clip, visible;
    bool found, modal_allowed;
} cui_core_geometry;
struct cui_prepaint_writer {
    cui_context *ui;
    size_t registration;
};
typedef struct {
    cui_id id;
    cui_rect *retained;
} cui_core_bounds_track;
typedef struct {
    float size, line_height;
    cui_text_style style;
} cui_core_text_style;
typedef struct {
    cui_id owner, namespace_id;
    void *payload;
    size_t bytes;
    cui_resource_fn collect, destroy;
    bool touched;
} cui_core_resource;
typedef struct {
    cui_str submitted;
    cui_text_line_source source;
    const cui_text_color_span *colors;
    size_t color_count;
} cui_core_text_source;
struct cui_context {
    cui_core_focus_scope focus_scopes[CUI_CORE_STACK_MAX],
        previous_focus_scopes[CUI_CORE_STACK_MAX];
    size_t focus_stack[CUI_CORE_STACK_MAX], focus_depth, focus_scope_count,
        previous_focus_scope_count, top_modal, previous_top_modal;
    size_t *semantic_scopes, *previous_semantic_scopes;
    cui_scroll_gesture *scroll_gestures, *previous_scroll_gestures;
    Clay_Context *clay;
    void *arena;
    Clay__ScrollContainerDataInternal *clip_containers;
    cui_config config;
    cui_core_text_source *text_sources;
    size_t text_source_count;
    cui_core_text_style *text_styles;
    size_t text_style_count, text_style_capacity;
    uint64_t text_metrics_revision;
    cui_draw_command *commands;
    size_t count;
    cui_core_prepaint *prepaints;
    cui_core_geometry *geometry;
    cui_draw_command *prepaint_commands;
    bool *prepaint_deferred;
    size_t prepaint_count, prepaint_reserved;
    bool prepainting, prepaint_failed, prepaint_capacity;
    cui_prepaint_writer *prepaint_writer;
    cui_core_box_node *nodes;
    cui_core_bounds_track *bounds_tracks;
    size_t bounds_track_count;
    size_t *semantic_nodes, *previous_parents;
    size_t node_count, node_stack[CUI_CORE_STACK_MAX];
    cui_rect *previous_hits;
    int16_t *previous_layers;
    uint64_t *previous_order;
    uint32_t paint_root_count;
    cui_semantic *semantics, *previous;
    size_t semantic_count, previous_count;
    cui_id *ids, *activations;
    size_t id_count, activation_count;
    cui_event queued[CUI_CORE_EVENT_MAX], events[CUI_CORE_DISPATCH_MAX];
    size_t queued_count, event_count;
    char *queued_text[CUI_CORE_EVENT_MAX], *frame_text[CUI_CORE_EVENT_MAX];
    size_t queued_text_bytes;
    bool event_consumed[CUI_CORE_DISPATCH_MAX];
    cui_id event_targets[CUI_CORE_DISPATCH_MAX], event_activations[CUI_CORE_DISPATCH_MAX];
    cui_id event_touch_owners[CUI_CORE_DISPATCH_MAX];
    cui_id event_previous_focus[CUI_CORE_DISPATCH_MAX];
    size_t event_activation_slots[CUI_CORE_DISPATCH_MAX];
    uint64_t scopes[CUI_CORE_STACK_MAX];
    size_t scope_count;
    bool opened[CUI_CORE_STACK_MAX];
    size_t depth, overflow_depth, element_count;
    char *strings;
    size_t string_used;
    cui_id focus, capture, space_capture;
    float pointer_x, pointer_y, dt;
    struct {
        uint64_t id, device;
        float x, y, start_x, start_y;
        cui_id owner, viewport;
        bool active, primary, dragging, blocked;
    } contacts[16];
    double frame_time;
    cui_id text_input_owner, composition_owner;
    cui_rect text_input_bounds;
    cui_id text_geometry_ime_owner;
    cui_rect text_geometry_ime_bounds;
    float text_geometry_ime_height;
    bool reduced_motion;
    cui_core_resource *resources;
    size_t resource_bytes;
    cui_error error;
    bool in_frame, focus_visible, dirty;
};
static _Thread_local cui_context *cui_core_paint_context;
static bool cui_core_layout_only(const cui_context *ui) {
    return ui && ui->in_frame && ui->depth && !ui->overflow_depth && ui->opened[ui->depth - 1] &&
           ui->nodes[ui->node_stack[ui->depth - 1]].layout_only;
}
static bool cui_core_inert(const cui_context *ui) {
    return ui && ui->in_frame && ui->depth && !ui->overflow_depth && ui->opened[ui->depth - 1] &&
           ui->nodes[ui->node_stack[ui->depth - 1]].inert;
}
static void cui_core_fail(cui_context *ui, cui_error e) {
    if (ui && ui->prepainting) {
        ui->prepaint_failed = true;
        if (e == CUI_ERROR_CAPACITY)
            ui->prepaint_capacity = true;
    }
    if (ui && ui->error == CUI_OK)
        ui->error = e;
}
static void cui_core_select_context(cui_context *ui) {
    Clay_SetCurrentContext(ui->clay);
}
static bool cui_core_mutation(cui_context *ui) {
    if (cui_core_paint_context) {
        cui_core_fail(cui_core_paint_context, CUI_ERROR_LIFECYCLE);
        return false;
    }
    if (ui && ui->prepainting) {
        cui_core_fail(ui, CUI_ERROR_LIFECYCLE);
        return false;
    }
    return true;
}
static bool cui_core_active(cui_context *ui) {
    if (!ui)
        return false;
    if (!cui_core_mutation(ui) || !ui->in_frame) {
        cui_core_fail(ui, CUI_ERROR_LIFECYCLE);
        return false;
    }
    cui_core_select_context(ui);
    return true;
}
static bool cui_core_text_style_valid(cui_text_style style) {
    return style.weight >= CUI_TEXT_WEIGHT_NORMAL && style.weight <= CUI_TEXT_WEIGHT_BOLD;
}
cui_size cui_measure_text_ex(cui_context *ui, cui_str text, float size, cui_text_style style) {
    if (!ui || (!text.data && text.length) || !isfinite(size) || size <= 0 ||
        !cui_core_text_style_valid(style))
        return (cui_size){0};
    size = fminf(size, 65535);
    cui_size result = ui->config.measure_styled
                          ? ui->config.measure_styled(ui->config.measure_user, text, size, style)
                      : ui->config.measure ? ui->config.measure(ui->config.measure_user, text, size)
                                           : (cui_size){text.length * size * .6f, size * 1.3f};
    if (!isfinite(result.width) || !isfinite(result.height) || result.width < 0 ||
        result.height < 0)
        return (cui_size){0};
    return result;
}
static bool cui_core_text_scalar(cui_str text, size_t *offset, uint32_t *out) {
    size_t i = *offset;
    if (!text.data || i >= text.length)
        return false;
    uint32_t c = (unsigned char)text.data[i++], minimum = 0;
    unsigned extra = 0;
    if (c >= 0xc2 && c <= 0xdf) {
        c &= 31;
        extra = 1;
        minimum = 0x80;
    } else if (c >= 0xe0 && c <= 0xef) {
        c &= 15;
        extra = 2;
        minimum = 0x800;
    } else if (c >= 0xf0 && c <= 0xf4) {
        c &= 7;
        extra = 3;
        minimum = 0x10000;
    } else if (c >= 0x80)
        return false;
    if (extra > text.length - i)
        return false;
    while (extra--) {
        unsigned b = (unsigned char)text.data[i++];
        if ((b & 0xc0) != 0x80)
            return false;
        c = (c << 6) | (b & 63);
    }
    if (c < minimum || c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff))
        return false;
    *offset = i;
    *out = c;
    return true;
}
bool cui_text_line_source_valid(cui_text_line_source source) {
    if ((!source.paragraph.data && source.paragraph.length) ||
        source.byte_start > source.byte_end || source.byte_end > source.paragraph.length)
        return false;
    bool start = source.byte_start == 0, end = source.byte_end == 0;
    size_t offset = 0;
    uint32_t cp;
    while (offset < source.paragraph.length) {
        if (!cui_core_text_scalar(source.paragraph, &offset, &cp) || cp == 10 || cp == 13 ||
            cp == 0x85 || cp == 0x2029 || cp == 0x1c || cp == 0x1d || cp == 0x1e)
            return false;
        start |= offset == source.byte_start;
        end |= offset == source.byte_end;
    }
    return start && end;
}
bool cui_frame_active(const cui_context *ui) {
    return ui && ui->in_frame;
}
bool cui_has_text_geometry_line(const cui_context *ui) {
    return ui && (ui->config.text_geometry_line || ui->config.text_geometry);
}
cui_text_layout_status cui_get_text_geometry_line(cui_context *ui, cui_text_line_source source,
                                                  float size, cui_text_style style,
                                                  cui_text_geometry *out) {
    if (!ui || !out || !cui_text_line_source_valid(source) || !isfinite(size) || size <= 0 ||
        !cui_core_text_style_valid(style))
        return CUI_TEXT_LAYOUT_INVALID;
    if (!ui->config.text_geometry_line) {
        if (source.byte_start == 0 && source.byte_end == source.paragraph.length)
            return cui_get_text_geometry(ui, source.paragraph, size, style, out);
        return CUI_TEXT_LAYOUT_UNAVAILABLE;
    }
    cui_text_geometry result = {0};
    cui_text_layout_status status = ui->config.text_geometry_line(
        ui->config.measure_user, source, fminf(size, 65535), style, &result);
    if (status != CUI_TEXT_LAYOUT_OK) {
        cui_release_text_geometry(&result);
        return status;
    }
    if (!result.ops || !result.ops->carets || !result.ops->hit_test || !result.ops->selection ||
        !result.ops->release || !isfinite(result.width) || !isfinite(result.height) ||
        result.width < 0 || result.height < 0 || result.byte_length != source.paragraph.length) {
        cui_release_text_geometry(&result);
        return CUI_TEXT_LAYOUT_INVALID;
    }
    *out = result;
    return CUI_TEXT_LAYOUT_OK;
}
bool cui_has_text_geometry(const cui_context *ui) {
    return ui && ui->config.text_geometry;
}
void cui_release_text_geometry(cui_text_geometry *geometry) {
    if (geometry && geometry->ops && geometry->ops->release)
        geometry->ops->release(geometry->session);
    if (geometry)
        *geometry = (cui_text_geometry){0};
}
cui_text_layout_status cui_get_text_geometry(cui_context *ui, cui_str text, float size,
                                             cui_text_style style, cui_text_geometry *out) {
    if (!ui || !out || (!text.data && text.length) || !isfinite(size) || size <= 0 ||
        !cui_core_text_style_valid(style))
        return CUI_TEXT_LAYOUT_INVALID;
    if (!ui->config.text_geometry)
        return CUI_TEXT_LAYOUT_UNAVAILABLE;
    cui_text_geometry result = {0};
    cui_text_layout_status status =
        ui->config.text_geometry(ui->config.measure_user, text, fminf(size, 65535), style, &result);
    if (status != CUI_TEXT_LAYOUT_OK) {
        cui_release_text_geometry(&result);
        return status;
    }
    if (!result.ops || !result.ops->carets || !result.ops->hit_test || !result.ops->selection ||
        !result.ops->release || !isfinite(result.width) || !isfinite(result.height) ||
        result.width < 0 || result.height < 0 || result.byte_length != text.length) {
        cui_release_text_geometry(&result);
        return CUI_TEXT_LAYOUT_INVALID;
    }
    *out = result;
    return CUI_TEXT_LAYOUT_OK;
}
bool cui_measure_font_metrics(cui_context *ui, float size, cui_text_style style,
                              cui_font_metrics *out) {
    if (!ui || !out || !ui->config.font_metrics || !isfinite(size) || size <= 0 ||
        !cui_core_text_style_valid(style))
        return false;
    cui_font_metrics result = {0};
    if (!ui->config.font_metrics(ui->config.measure_user, fminf(size, 65535), style, &result) ||
        !isfinite(result.ascent) || !isfinite(result.descent) || !isfinite(result.line_height) ||
        result.ascent < 0 || result.descent < 0 || result.line_height <= 0 ||
        result.ascent + result.descent <= 0 || !isfinite(result.ascent + result.descent))
        return false;
    *out = result;
    return true;
}
cui_size cui_measure_text(cui_context *ui, cui_str text, float size) {
    return cui_measure_text_ex(ui, text, size, (cui_text_style){0});
}
void cui_text_metrics_changed(cui_context *ui) {
    if (!cui_core_mutation(ui))
        return;
    if (!ui || ui->in_frame)
        return;
    cui_core_select_context(ui);
    Clay_ResetMeasureTextCache();
    if (++ui->text_metrics_revision == 0)
        ui->text_metrics_revision = 1;
    cui_invalidate(ui);
}
uint64_t cui_text_metrics_revision(const cui_context *ui) {
    return ui ? ui->text_metrics_revision : 0;
}
static Clay_Dimensions cui_core_measure(Clay_StringSlice s, Clay_TextElementConfig *c, void *u) {
    cui_context *ui = u;
    cui_core_text_style style = c->fontId && c->fontId <= ui->text_style_count
                                    ? ui->text_styles[c->fontId - 1]
                                    : (cui_core_text_style){c->fontSize, 0, {0}};
    cui_size z =
        cui_measure_text_ex(ui, (cui_str){s.chars, (size_t)s.length}, style.size, style.style);
    return (Clay_Dimensions){z.width, style.line_height > 0 ? style.line_height : z.height};
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
    if (cui_core_paint_context) {
        cui_core_fail(cui_core_paint_context, CUI_ERROR_LIFECYCLE);
        return NULL;
    }
    cui_context *ui = calloc(1, sizeof(*ui));
    if (!ui)
        return NULL;
    if (!config.max_elements)
        config.max_elements = 4096;
    if (!config.max_commands)
        config.max_commands = 8192;
    if (!config.frame_bytes)
        config.frame_bytes = 262144;
    if (!config.max_resources)
        config.max_resources = 128;
    if (!config.resource_bytes)
        config.resource_bytes = 1024 * 1024;
    /* Clay reports arena sizes through uint32_t, so retain a conservative
       supported element ceiling before its sizing or any allocations. */
    if (config.max_resources > 1000000 || config.resource_bytes > (size_t)PTRDIFF_MAX ||
        config.max_resources > SIZE_MAX / sizeof(cui_core_resource) ||
        config.frame_bytes > (size_t)PTRDIFF_MAX || config.max_elements > 1000000 ||
        config.max_commands > (size_t)PTRDIFF_MAX / sizeof(cui_draw_command) ||
        config.max_elements > SIZE_MAX / sizeof(cui_semantic) ||
        config.max_elements > SIZE_MAX / sizeof(cui_id) ||
        config.max_elements > SIZE_MAX / sizeof(cui_core_box_node) ||
        config.max_elements > SIZE_MAX / sizeof(cui_core_prepaint) ||
        config.max_elements > SIZE_MAX / sizeof(cui_core_geometry) ||
        config.max_elements > SIZE_MAX / sizeof(size_t) ||
        config.max_elements > SIZE_MAX / sizeof(cui_rect) ||
        config.max_elements > SIZE_MAX / sizeof(cui_core_bounds_track) ||
        config.max_elements > SIZE_MAX / sizeof(Clay__ScrollContainerDataInternal)) {
        free(ui);
        return NULL;
    }
    if (!config.event_text_bytes)
        config.event_text_bytes = 65536;
    ui->config = config;
    ui->text_metrics_revision = 1;
    ui->dirty = true;
    ui->top_modal = ui->previous_top_modal = SIZE_MAX;
    ui->text_style_capacity = config.max_elements > 32767 ? 65535 : config.max_elements * 2;
    ui->text_sources = calloc(config.max_elements, sizeof(*ui->text_sources));
    ui->text_styles = calloc(ui->text_style_capacity, sizeof(*ui->text_styles));
    ui->resources = calloc(config.max_resources, sizeof(*ui->resources));
    ui->commands = calloc(config.max_commands, sizeof(*ui->commands));
    ui->prepaints = calloc(config.max_elements, sizeof(*ui->prepaints));
    ui->geometry = calloc(config.max_elements, sizeof(*ui->geometry));
    ui->prepaint_commands = calloc(config.max_commands, sizeof(*ui->prepaint_commands));
    ui->prepaint_deferred = calloc(config.max_commands, sizeof(*ui->prepaint_deferred));
    ui->semantics = calloc(config.max_elements, sizeof(*ui->semantics));
    ui->previous = calloc(config.max_elements, sizeof(*ui->previous));
    ui->ids = calloc(config.max_elements, sizeof(*ui->ids));
    ui->activations = calloc(CUI_CORE_EVENT_MAX, sizeof(*ui->activations));
    ui->semantic_scopes = calloc(config.max_elements, sizeof(*ui->semantic_scopes));
    ui->previous_semantic_scopes =
        calloc(config.max_elements, sizeof(*ui->previous_semantic_scopes));
    ui->scroll_gestures = calloc(config.max_elements, sizeof(*ui->scroll_gestures));
    ui->previous_scroll_gestures =
        calloc(config.max_elements, sizeof(*ui->previous_scroll_gestures));
    ui->strings = malloc(config.frame_bytes);
    ui->clip_containers = calloc(config.max_elements, sizeof(*ui->clip_containers));
    ui->nodes = calloc(config.max_elements, sizeof(*ui->nodes));
    ui->bounds_tracks = calloc(config.max_elements, sizeof(*ui->bounds_tracks));
    ui->semantic_nodes = calloc(config.max_elements, sizeof(*ui->semantic_nodes));
    ui->previous_parents = calloc(config.max_elements, sizeof(*ui->previous_parents));
    ui->previous_hits = calloc(config.max_elements, sizeof(*ui->previous_hits));
    ui->previous_layers = calloc(config.max_elements, sizeof(*ui->previous_layers));
    ui->previous_order = calloc(config.max_elements, sizeof(*ui->previous_order));
    Clay_Context *saved = Clay_GetCurrentContext();
    Clay_SetCurrentContext(NULL);
    /* Clay's ID map retains the prior layout until EndLayout retires old IDs.
     * Reserve both generations while CUI's own limits bound current elements. */
    Clay_SetMaxElementCount((int32_t)(config.max_elements * 2 + 32));
    size_t n = Clay_MinMemorySize();
    ui->arena = malloc(n);
    if (!ui->prepaints || !ui->geometry || !ui->prepaint_commands || !ui->prepaint_deferred ||
        !ui->text_sources || !ui->resources || !ui->text_styles || !ui->commands ||
        !ui->semantics || !ui->previous || !ui->ids || !ui->activations || !ui->semantic_scopes ||
        !ui->previous_semantic_scopes || !ui->scroll_gestures || !ui->previous_scroll_gestures ||
        !ui->strings || !ui->nodes || !ui->semantic_nodes || !ui->previous_parents ||
        !ui->previous_hits || !ui->bounds_tracks || !ui->previous_layers || !ui->previous_order ||
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
    if (!cui_core_mutation(ui))
        return;
    if (!ui)
        return;
    if (Clay_GetCurrentContext() == ui->clay)
        Clay_SetCurrentContext(NULL);
    if (ui->resources)
        for (size_t i = 0; i < ui->config.max_resources; ++i) {
            cui_core_resource *r = &ui->resources[i];
            if (r->payload && r->destroy)
                r->destroy(r->payload);
            free(r->payload);
        }
    for (size_t i = 0; i < CUI_CORE_EVENT_MAX; i++) {
        free(ui->queued_text[i]);
        free(ui->frame_text[i]);
    }
    free(ui->scroll_gestures);
    free(ui->previous_scroll_gestures);
    free(ui->resources);
    free(ui->semantic_scopes);
    free(ui->previous_semantic_scopes);
    free(ui->text_sources);
    free(ui->text_styles);
    free(ui->nodes);
    free(ui->bounds_tracks);
    free(ui->semantic_nodes);
    free(ui->previous_parents);
    free(ui->previous_hits);
    free(ui->previous_layers);
    free(ui->previous_order);
    free(ui->prepaints);
    free(ui->geometry);
    free(ui->prepaint_commands);
    free(ui->prepaint_deferred);
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
    if (s->disabled || s->details.no_focus)
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
static bool cui_core_previous_allowed(const cui_context *ui, size_t i) {
    return cui_core_scope_contains(ui->previous_focus_scopes, ui->previous_semantic_scopes[i],
                                   ui->previous_top_modal);
}
static cui_id cui_core_pointer_focus_target(cui_context *ui, size_t index) {
    const cui_semantic *s = &ui->previous[index];
    if (s->disabled || s->details.no_pointer_focus)
        return 0;
    if (!s->details.focus_target)
        return cui_core_focusable(s) ? s->id : 0;
    for (size_t i = 0; i < ui->previous_count; i++)
        if (ui->previous[i].id == s->details.focus_target && cui_core_focusable(&ui->previous[i]) &&
            cui_core_previous_allowed(ui, i))
            return ui->previous[i].id;
    return 0;
}
static bool cui_core_above(cui_context *ui, size_t candidate, size_t current) {
    return current == SIZE_MAX || ui->previous_layers[candidate] > ui->previous_layers[current] ||
           (ui->previous_layers[candidate] == ui->previous_layers[current] &&
            ui->previous_order[candidate] >= ui->previous_order[current]);
}
static cui_id cui_core_hit(cui_context *ui, float x, float y) {
    size_t hit = SIZE_MAX;
    for (size_t i = 0; i < ui->previous_count; i++)
        if ((cui_core_pointer_focus_target(ui, i) ||
             (!ui->previous[i].disabled && ui->previous[i].details.pointer_activatable)) &&
            cui_core_previous_allowed(ui, i) && cui_core_inside(ui->previous_hits[i], x, y) &&
            cui_core_above(ui, i, hit))
            hit = i;
    return hit == SIZE_MAX ? 0 : ui->previous[hit].id;
}
cui_cursor cui_pointer_cursor(cui_context *ui) {
    if (!ui)
        return CUI_CURSOR_DEFAULT;
    cui_id id = ui->capture;
    if (!id) {
        size_t hit = SIZE_MAX;
        for (size_t i = 0; i < ui->previous_count; i++) {
            const cui_semantic *s = &ui->previous[i];
            if (!s->disabled && cui_core_previous_allowed(ui, i) &&
                (cui_core_focusable(s) || s->details.pointer_activatable ||
                 s->details.cursor != CUI_CURSOR_DEFAULT) &&
                cui_core_inside(ui->previous_hits[i], ui->pointer_x, ui->pointer_y) &&
                cui_core_above(ui, i, hit))
                hit = i;
        }
        if (hit != SIZE_MAX)
            id = ui->previous[hit].id;
    }
    for (size_t i = 0; i < ui->previous_count; i++) {
        const cui_semantic *s = &ui->previous[i];
        if (s->id == id && !s->disabled && cui_core_previous_allowed(ui, i) &&
            s->details.cursor >= CUI_CURSOR_DEFAULT && s->details.cursor <= CUI_CURSOR_HELP)
            return s->details.cursor;
    }
    return CUI_CURSOR_DEFAULT;
}
static cui_id cui_core_scroll_hit(cui_context *ui, cui_event event, size_t from, bool gesture_probe,
                                  cui_id touch_owner_id) {
    cui_id hit = 0;
    size_t touch_owner = SIZE_MAX;
    if (event.pointer_type == CUI_POINTER_TOUCH) {
        for (size_t n = 0; n < ui->previous_count; n++)
            if (ui->previous[n].id == touch_owner_id && !ui->previous[n].disabled &&
                cui_core_previous_allowed(ui, n))
                touch_owner = n;
        if (touch_owner == SIZE_MAX)
            return 0;
    }
    int16_t layer = INT16_MIN, blocking_layer = INT16_MIN;
    for (size_t i = 0; i < ui->previous_count; ++i) {
        const cui_semantic *node = &ui->previous[i];
        if (!cui_core_previous_allowed(ui, i) ||
            !cui_core_inside(ui->previous_hits[i], event.x, event.y))
            continue;
        if ((cui_core_focusable(node) || (!node->disabled && node->details.pointer_activatable)) &&
            ui->previous_layers[i] > blocking_layer)
            blocking_layer = ui->previous_layers[i];
        if (from != SIZE_MAX) {
            size_t parent = ui->previous_parents[from];
            while (parent != SIZE_MAX && parent != i)
                parent = ui->previous_parents[parent];
            if (parent == SIZE_MAX)
                continue;
        }
        if (touch_owner != SIZE_MAX && i != touch_owner) {
            size_t ancestor = ui->previous_parents[touch_owner];
            while (ancestor != SIZE_MAX && ancestor != i)
                ancestor = ui->previous_parents[ancestor];
            if (ancestor == SIZE_MAX)
                continue;
        }
        cui_semantic_details d = node->details;
        bool can_x =
            (event.dx > 0 && d.scroll_x > 0) || (event.dx < 0 && d.scroll_x < d.scroll_max_x);
        bool can_y =
            (event.dy > 0 && d.scroll_y > 0) || (event.dy < 0 && d.scroll_y < d.scroll_max_y);
        if (d.scroll_trap_horizontal && fabsf(event.dx) > fabsf(event.dy))
            can_x = true;
        if (gesture_probe) {
            can_x = d.scroll_gesture_lock;
            can_y = false;
        }
        if (d.scroll_axis_lock && !gesture_probe) {
            bool vertical = fabsf(event.dy) >= fabsf(event.dx);
            if (d.orientation == CUI_ORIENTATION_VERTICAL) {
                can_x = false;
                can_y = can_y && vertical;
            } else if (d.orientation == CUI_ORIENTATION_HORIZONTAL) {
                can_x = can_x && !vertical;
                can_y = false;
            }
        }
        if (!node->disabled && d.scrollable &&
            !(event.pointer_type == CUI_POINTER_TOUCH && d.touch_scroll_disabled) &&
            (can_x || can_y ||
             (i == touch_owner && event.scroll_precise &&
              (event.scroll_phase == CUI_SCROLL_PHASE_ENDED ||
               event.scroll_phase == CUI_SCROLL_PHASE_CANCELLED))) &&
            ui->previous_layers[i] >= layer) {
            hit = node->id;
            layer = ui->previous_layers[i];
        }
    }
    return layer >= blocking_layer ? hit : 0;
}
/* Touch contacts are bounded and retained across immediate-mode frames. Pan
 * reuses the same scroll event/reducer path as precise wheel input. */
static cui_id cui_core_touch_owner(cui_context *ui, cui_event e, bool viewport) {
    cui_id result = 0;
    int16_t layer = INT16_MIN, blocking = INT16_MIN;
    for (size_t i = 0; i < ui->previous_count; i++) {
        cui_semantic *s = &ui->previous[i];
        if (!cui_core_previous_allowed(ui, i) || !cui_core_inside(ui->previous_hits[i], e.x, e.y))
            continue;
        if ((cui_core_focusable(s) || s->details.pointer_activatable) &&
            ui->previous_layers[i] > blocking)
            blocking = ui->previous_layers[i];
        bool eligible = viewport ? s->details.gesture_pan || s->details.gesture_zoom
                                 : s->details.scrollable && !s->details.touch_scroll_disabled;
        if (!s->disabled && eligible && ui->previous_layers[i] >= layer) {
            result = s->id;
            layer = ui->previous_layers[i];
        }
    }
    return layer >= blocking ? result : 0;
}
static float cui_core_touch_float(double value) {
    return (float)fmin((double)FLT_MAX, fmax(-(double)FLT_MAX, value));
}
static bool cui_core_touch(cui_context *ui, cui_event *e, cui_id *target, cui_id *scroll_owner) {
    if (e->type == CUI_EVENT_CANCEL && e->pointer_type != CUI_POINTER_TOUCH) {
        memset(ui->contacts, 0, sizeof ui->contacts);
        return false;
    }
    if (e->pointer_type != CUI_POINTER_TOUCH ||
        (e->type != CUI_EVENT_POINTER_DOWN && e->type != CUI_EVENT_POINTER_MOVE &&
         e->type != CUI_EVENT_POINTER_UP && e->type != CUI_EVENT_CANCEL))
        return false;
    if (e->type != CUI_EVENT_CANCEL && (!isfinite(e->x) || !isfinite(e->y)))
        return true;
    size_t slot = 16, count = 0;
    for (size_t n = 0; n < 16; n++) {
        if (ui->contacts[n].active) {
            count++;
            if (ui->contacts[n].id == e->pointer_id && ui->contacts[n].device == e->pointer_device)
                slot = n;
        }
    }
    if (e->type == CUI_EVENT_POINTER_DOWN) {
        if (slot != 16)
            return true;
        for (size_t n = 0; n < 16; n++)
            if (!ui->contacts[n].active) {
                slot = n;
                break;
            }
        if (slot == 16)
            return true;
        for (size_t n = 0; n < 16; n++)
            if (n != slot && !ui->contacts[n].active && ui->contacts[n].id == e->pointer_id &&
                ui->contacts[n].device == e->pointer_device)
                memset(&ui->contacts[n], 0, sizeof ui->contacts[n]);
        ui->contacts[slot].id = e->pointer_id;
        ui->contacts[slot].device = e->pointer_device;
        ui->contacts[slot].x = ui->contacts[slot].start_x = e->x;
        ui->contacts[slot].y = ui->contacts[slot].start_y = e->y;
        ui->contacts[slot].owner = cui_core_touch_owner(ui, *e, false);
        ui->contacts[slot].viewport = cui_core_touch_owner(ui, *e, true);
        cui_id pointer_control = cui_core_hit(ui, e->x, e->y);
        for (size_t n = 0; n < ui->previous_count; n++)
            if (ui->previous[n].id == pointer_control &&
                (ui->previous[n].details.touch_drag_priority ||
                 ui->previous[n].role == CUI_ROLE_SLIDER ||
                 ui->previous[n].role == CUI_ROLE_SCROLLBAR ||
                 ui->previous[n].role == CUI_ROLE_SEPARATOR)) {
                ui->contacts[slot].owner = ui->contacts[slot].viewport = 0;
                break;
            }
        ui->contacts[slot].active = true;
        ui->contacts[slot].primary = count == 0;
        ui->contacts[slot].dragging = false;
        ui->contacts[slot].blocked = false;
        if (count) {
            ui->capture = 0;
            for (size_t n = 0; n < 16; n++)
                if (ui->contacts[n].active)
                    ui->contacts[n].blocked = true;
            return true;
        }
        return false;
    }
    if (slot == 16)
        return true;
    bool cancelled = e->type == CUI_EVENT_CANCEL;
    float contact_x = e->x, contact_y = e->y;
    if (cancelled) {
        contact_x = ui->contacts[slot].x;
        contact_y = ui->contacts[slot].y;
        e->x = contact_x;
        e->y = contact_y;
        ui->capture = 0;
    }
    float old_x = ui->contacts[slot].x, old_y = ui->contacts[slot].y;
    double dx = (double)e->x - old_x, dy = (double)e->y - old_y;
    double distance_x = (double)e->x - ui->contacts[slot].start_x;
    double distance_y = (double)e->y - ui->contacts[slot].start_y;
    bool ending = e->type == CUI_EVENT_POINTER_UP || e->type == CUI_EVENT_CANCEL;
    cui_id viewport = ui->contacts[slot].viewport;
    bool pan = false, zoom = false;
    cui_event anchor = *e;
    anchor.x = ui->contacts[slot].start_x;
    anchor.y = ui->contacts[slot].start_y;
    bool viewport_visible = viewport && cui_core_touch_owner(ui, anchor, true) == viewport;
    for (size_t n = 0; n < ui->previous_count; n++)
        if (viewport_visible && ui->previous[n].id == viewport && !ui->previous[n].disabled &&
            cui_core_previous_allowed(ui, n)) {
            pan = ui->previous[n].details.gesture_pan;
            zoom = ui->previous[n].details.gesture_zoom;
        }
    if (!ui->contacts[slot].owner && !viewport &&
        distance_x * distance_x + distance_y * distance_y >= 64)
        ui->contacts[slot].dragging = true;
    size_t partner = 16;
    for (size_t n = 0; n < 16; n++)
        if (n != slot && ui->contacts[n].active && ui->contacts[n].viewport == viewport) {
            partner = n;
            break;
        }
    bool pinch = zoom && viewport && count == 2 && partner != 16;
    if (pinch || (pan && count == 1 && !ui->contacts[slot].blocked)) {
        bool start = !ui->contacts[slot].dragging;
        if (pinch || distance_x * distance_x + distance_y * distance_y >= 64 || !start) {
            float scale = 1;
            float center_x = e->x, center_y = e->y;
            if (pinch) {
                float px = ui->contacts[partner].x, py = ui->contacts[partner].y;
                double previous = hypot((double)old_x - px, (double)old_y - py);
                if (previous > 0.001f && !ending)
                    scale = cui_core_touch_float(hypot((double)e->x - px, (double)e->y - py) /
                                                 previous);
                center_x = cui_core_touch_float(((double)e->x + px) * 0.5);
                center_y = cui_core_touch_float(((double)e->y + py) * 0.5);
                dx *= 0.5f;
                dy *= 0.5f;
                ui->contacts[partner].dragging = true;
            }
            ui->contacts[slot].dragging = true;
            ui->capture = 0;
            e->type = CUI_EVENT_GESTURE;
            e->dx = pan && !ending ? cui_core_touch_float(dx) : 0;
            e->dy = pan && !ending ? cui_core_touch_float(dy) : 0;
            e->gesture_scale = scale;
            e->x = center_x;
            e->y = center_y;
            e->scroll_phase =
                ending  ? (cancelled ? CUI_SCROLL_PHASE_CANCELLED : CUI_SCROLL_PHASE_ENDED)
                : start ? CUI_SCROLL_PHASE_STARTED
                        : CUI_SCROLL_PHASE_MOVED;
            *target = viewport;
        }
    } else if (count == 1 && !ui->contacts[slot].blocked) {
        bool start = !ui->contacts[slot].dragging;
        if (ui->contacts[slot].owner &&
            (distance_x * distance_x + distance_y * distance_y >= 64 || !start)) {
            ui->contacts[slot].dragging = true;
            ui->capture = 0;
            e->type = CUI_EVENT_SCROLL;
            *scroll_owner = ui->contacts[slot].owner;
            e->dx = ending ? 0 : cui_core_touch_float(start ? distance_x : dx);
            e->dy = ending ? 0 : cui_core_touch_float(start ? distance_y : dy);
            e->x = ui->contacts[slot].start_x;
            e->y = ui->contacts[slot].start_y;
            e->scroll_precise = true;
            e->scroll_phase =
                ending  ? (cancelled ? CUI_SCROLL_PHASE_CANCELLED : CUI_SCROLL_PHASE_ENDED)
                : start ? CUI_SCROLL_PHASE_STARTED
                        : CUI_SCROLL_PHASE_MOVED;
        }
    }
    ui->contacts[slot].x = contact_x;
    ui->contacts[slot].y = contact_y;
    bool suppress = ui->contacts[slot].blocked || !ui->contacts[slot].primary;
    if (e->type == CUI_EVENT_GESTURE)
        suppress = false;
    if (ending) {
        ui->contacts[slot].active = false;
    }
    return suppress;
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
static bool cui_core_accessibility_activate(const cui_semantic *s) {
    if (s->disabled)
        return false;
    if (s->details.pointer_activatable)
        return true;
    if (s->details.no_focus)
        return false;
    switch (s->role) {
    case CUI_ROLE_BUTTON:
    case CUI_ROLE_LINK:
    case CUI_ROLE_CHECKBOX:
    case CUI_ROLE_SWITCH:
    case CUI_ROLE_RADIO:
    case CUI_ROLE_MENUITEM:
    case CUI_ROLE_MENUITEMCHECKBOX:
    case CUI_ROLE_MENUITEMRADIO:
        return true;
    default:
        return false;
    }
}
static bool cui_core_accessibility_supported(const cui_context *ui, cui_id id,
                                             cui_accessibility_action action) {
    if (!ui || !id || (action != CUI_ACCESSIBILITY_FOCUS && action != CUI_ACCESSIBILITY_ACTIVATE))
        return false;
    for (size_t i = 0; i < ui->previous_count; i++)
        if (ui->previous[i].id == id && cui_core_previous_allowed(ui, i))
            return action == CUI_ACCESSIBILITY_FOCUS
                       ? cui_core_focusable(&ui->previous[i])
                       : cui_core_accessibility_activate(&ui->previous[i]);
    return false;
}
bool cui_accessibility_action_supported(const cui_context *ui, cui_id id,
                                        cui_accessibility_action action) {
    return ui && !ui->in_frame && cui_core_accessibility_supported(ui, id, action);
}
bool cui_queue_accessibility_action(cui_context *ui, cui_id id, cui_accessibility_action action) {
    if (!cui_core_mutation(ui))
        return false;
    return cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                           .accessibility_target = id,
                                           .accessibility_action = action});
}
bool cui_queue_event(cui_context *ui, cui_event e) {
    if (!cui_core_mutation(ui))
        return false;
    if (!ui)
        return false;
    if (ui->in_frame) {
        cui_core_fail(ui, CUI_ERROR_LIFECYCLE);
        return false;
    }
    if (e.type == CUI_EVENT_ACCESSIBILITY_ACTION) {
        if (!cui_core_accessibility_supported(ui, e.accessibility_target, e.accessibility_action)) {
            cui_core_fail(ui, CUI_ERROR_INVALID_ACTION);
            return false;
        }
        e = (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                        .accessibility_target = e.accessibility_target,
                        .accessibility_action = e.accessibility_action};
    }
    if (ui->queued_count == CUI_CORE_EVENT_MAX) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return false;
    }
    char *copy = NULL;
    if (e.text_data || e.text_length) {
        if (!e.text_data || e.text_length > ui->config.event_text_bytes - ui->queued_text_bytes) {
            cui_core_fail(ui, CUI_ERROR_CAPACITY);
            return false;
        }
        if (e.text_length) {
            copy = malloc(e.text_length);
            if (!copy) {
                cui_core_fail(ui, CUI_ERROR_CAPACITY);
                return false;
            }
            memcpy(copy, e.text_data, e.text_length);
        }
        e.text_data = copy;
        ui->queued_text_bytes += e.text_length;
    }
    ui->queued_text[ui->queued_count] = copy;
    ui->queued[ui->queued_count++] = e;
    ui->dirty = true;
    return true;
}
bool cui_begin_frame(cui_context *ui, float w, float h, float dt) {
    if (!cui_core_mutation(ui))
        return false;
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
    for (size_t i = 0; i < ui->config.max_resources; ++i) {
        cui_core_resource *r = &ui->resources[i];
        if (!r->payload)
            continue;
        if (!r->touched) {
            if (r->destroy)
                r->destroy(r->payload);
            ui->resource_bytes -= r->bytes;
            free(r->payload);
            *r = (cui_core_resource){0};
        } else {
            if (r->collect)
                r->collect(r->payload);
            r->touched = false;
        }
    }
    if (isfinite(ui->frame_time + (double)dt))
        ui->frame_time += dt;
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
    memset(ui->event_consumed, 0, sizeof ui->event_consumed);
    for (size_t i = 0; i < CUI_CORE_EVENT_MAX; i++) {
        free(ui->frame_text[i]);
        ui->frame_text[i] = ui->queued_text[i];
        ui->queued_text[i] = NULL;
    }
    ui->queued_text_bytes = 0;
    ui->event_count = ui->queued_count;
    memcpy(ui->events, ui->queued, ui->queued_count * sizeof(*ui->events));
    ui->queued_count = 0;
    ui->activation_count = 0;
    cui_id touch_cancellations[CUI_CORE_DISPATCH_MAX] = {0};
    for (size_t i = 0; i < ui->event_count; i++) {
        cui_event e = ui->events[i];
        ui->event_previous_focus[i] = ui->focus;
        if (e.pointer_type == CUI_POINTER_TOUCH &&
            (e.type == CUI_EVENT_POINTER_DOWN || e.type == CUI_EVENT_POINTER_MOVE ||
             e.type == CUI_EVENT_POINTER_UP) &&
            isfinite(e.x) && isfinite(e.y)) {
            ui->pointer_x = e.x;
            ui->pointer_y = e.y;
        }
        cui_id gesture_target = 0;
        cui_id before_capture = ui->capture;
        cui_id touch_owner = 0;
        bool suppressed =
            !touch_cancellations[i] && cui_core_touch(ui, &e, &gesture_target, &touch_owner);
        ui->event_touch_owners[i] = touch_owner;
        if (!touch_cancellations[i] && e.pointer_type == CUI_POINTER_TOUCH && before_capture &&
            !ui->capture && e.type != CUI_EVENT_CANCEL) {
            /* Notify the captured component's local pointer reducer before later
             * queued contacts. This handoff must not reset Core's touch stream. */
            if (ui->event_count < CUI_CORE_DISPATCH_MAX) {
                size_t next = i + 1, remaining = ui->event_count - next;
                memmove(ui->events + next + 1, ui->events + next, remaining * sizeof(*ui->events));
                memmove(touch_cancellations + next + 1, touch_cancellations + next,
                        remaining * sizeof(*touch_cancellations));
                ui->events[next] = (cui_event){.type = CUI_EVENT_CANCEL,
                                               .pointer_type = CUI_POINTER_TOUCH,
                                               .pointer_id = e.pointer_id,
                                               .pointer_device = e.pointer_device};
                touch_cancellations[next] = before_capture;
                ui->event_count++;
            } else {
                cui_core_fail(ui, CUI_ERROR_CAPACITY);
            }
        }
        if (suppressed) {
            ui->event_consumed[i] = true;
            ui->event_targets[i] = 0;
            ui->event_activations[i] = 0;
            ui->event_activation_slots[i] = SIZE_MAX;
            continue;
        }
        ui->events[i] = e;
        size_t before_activation = ui->activation_count;
        ui->event_activations[i] = 0;
        ui->event_activation_slots[i] = SIZE_MAX;
        if (e.type == CUI_EVENT_ACCESSIBILITY_ACTION) {
            ui->event_targets[i] = 0;
            if (!cui_core_accessibility_supported(ui, e.accessibility_target,
                                                  e.accessibility_action)) {
                ui->event_consumed[i] = true;
                continue;
            }
            ui->event_targets[i] = e.accessibility_target;
            if (e.accessibility_action == CUI_ACCESSIBILITY_FOCUS) {
                ui->focus = e.accessibility_target;
                ui->focus_visible = true;
            } else {
                cui_core_activate(ui, e.accessibility_target);
                ui->event_activations[i] = e.accessibility_target;
                ui->event_activation_slots[i] = ui->activation_count - 1;
            }
            continue;
        }
        cui_id cancellation_target = touch_cancellations[i];
        if (!cancellation_target && e.type == CUI_EVENT_CANCEL &&
            e.pointer_type == CUI_POINTER_TOUCH)
            cancellation_target = before_capture;
        ui->event_targets[i] = cancellation_target           ? cancellation_target
                               : e.type == CUI_EVENT_GESTURE ? gesture_target
                                                             : ui->focus;
        if (e.type == CUI_EVENT_POINTER_MOVE || e.type == CUI_EVENT_POINTER_UP)
            ui->event_targets[i] = ui->capture;
        if (e.type == CUI_EVENT_POINTER_DOWN ||
            (e.type == CUI_EVENT_POINTER_UP && e.pointer_button != CUI_POINTER_PRIMARY)) {
            ui->event_targets[i] = cui_core_hit(ui, e.x, e.y);
            if (!ui->event_targets[i] && ui->previous_top_modal != SIZE_MAX)
                ui->event_targets[i] = ui->previous_focus_scopes[ui->previous_top_modal].id;
        }
        if (e.type == CUI_EVENT_SCROLL) {
            cui_id owner = cui_core_scroll_hit(ui, e, SIZE_MAX, true, touch_owner);
            if (owner)
                for (size_t n = 0; n < ui->previous_count; n++)
                    if (ui->previous[n].id == owner) {
                        (void)cui_scroll_gesture_filter(&ui->previous_scroll_gestures[n], &e,
                                                        ui->frame_time);
                        break;
                    }
            ui->events[i] = e;
            ui->event_targets[i] = cui_core_scroll_hit(ui, e, SIZE_MAX, false, touch_owner);
        }
        if (e.type >= CUI_EVENT_COMPOSITION_START && e.type <= CUI_EVENT_COMPOSITION_CANCEL &&
            e.composition_target && e.composition_target != ui->focus) {
            ui->event_consumed[i] = true;
            ui->event_targets[i] = 0;
            continue;
        }
        if (e.type == CUI_EVENT_COMPOSITION_START || e.type == CUI_EVENT_COMPOSITION_UPDATE)
            ui->composition_owner = ui->focus;
        else if (e.type == CUI_EVENT_COMPOSITION_COMMIT || e.type == CUI_EVENT_COMPOSITION_CANCEL ||
                 e.type == CUI_EVENT_CANCEL || e.type == CUI_EVENT_POINTER_DOWN)
            ui->composition_owner = 0;
        if ((e.type == CUI_EVENT_KEY_DOWN || e.type == CUI_EVENT_KEY_UP) && ui->composition_owner &&
            ui->composition_owner == ui->focus) {
            e.ime_composing = true;
            ui->events[i] = e;
        }
        if ((e.type == CUI_EVENT_KEY_DOWN || e.type == CUI_EVENT_KEY_UP) &&
            (e.ime_composing || e.prefer_character_input)) {
            if (e.key == CUI_KEY_SPACE)
                ui->space_capture = 0;
            continue;
        }
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
            bool pointer_only = false, pointer_activation = false;
            for (size_t n = 0; n < ui->previous_count; n++)
                if (ui->previous[n].id == hit) {
                    target = cui_core_pointer_focus_target(ui, n);
                    pointer_activation = ui->previous[n].details.pointer_activatable;
                    pointer_only = !target && pointer_activation;
                    break;
                }
            ui->capture = pointer_activation || target == hit ? hit : 0;
            if (!pointer_only) {
                if (hit || ui->previous_top_modal == SIZE_MAX)
                    ui->focus = target;
                ui->focus_visible = false;
            }
        } else if (e.type == CUI_EVENT_POINTER_UP && e.pointer_button == CUI_POINTER_PRIMARY) {
            bool touch_drag = false;
            if (e.pointer_type == CUI_POINTER_TOUCH)
                for (size_t n = 0; n < 16; n++)
                    if (ui->contacts[n].id == e.pointer_id &&
                        ui->contacts[n].device == e.pointer_device)
                        touch_drag |= ui->contacts[n].dragging;
            if (!touch_drag && ui->capture == cui_core_hit(ui, e.x, e.y))
                cui_core_activate(ui, ui->capture);
            ui->capture = 0;
        } else if (e.type == CUI_EVENT_CANCEL && !touch_cancellations[i]) {
            ui->capture = 0;
            ui->space_capture = 0;
        } else if (e.type == CUI_EVENT_KEY_DOWN && !e.repeat) {
            if (e.key == CUI_KEY_TAB) {
                bool handled = false;
                if (!(e.modifiers & (CUI_MOD_CTRL | CUI_MOD_META)))
                    for (size_t n = 0; n < ui->previous_count; n++)
                        if (ui->previous[n].id == ui->focus &&
                            ui->previous[n].details.handles_tab && !ui->previous[n].disabled &&
                            cui_core_previous_allowed(ui, n)) {
                            handled = true;
                            break;
                        }
                if (!handled)
                    cui_core_tab(ui, (e.modifiers & CUI_MOD_SHIFT) != 0);
            } else if (e.key == CUI_KEY_ENTER)
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
        if (ui->activation_count > before_activation) {
            ui->event_activations[i] = ui->activations[ui->activation_count - 1];
            ui->event_activation_slots[i] = ui->activation_count - 1;
        }
    }
    /* Routing completes before component reducers run. Present capture loss
     * before the replacement scroll/gesture event, retaining every event's
     * target, ownership and activation metadata. Cancelling after STARTED would
     * otherwise clear the new owner's freshly created gesture state. */
    for (size_t i = 1; i < ui->event_count; ++i) {
        if (!touch_cancellations[i])
            continue;
        size_t previous = i - 1;
        cui_event event = ui->events[previous];
        ui->events[previous] = ui->events[i];
        ui->events[i] = event;
        cui_id target = ui->event_targets[previous];
        ui->event_targets[previous] = ui->event_targets[i];
        ui->event_targets[i] = target;
        cui_id owner = ui->event_touch_owners[previous];
        ui->event_touch_owners[previous] = ui->event_touch_owners[i];
        ui->event_touch_owners[i] = owner;
        cui_id previous_focus = ui->event_previous_focus[previous];
        ui->event_previous_focus[previous] = ui->event_previous_focus[i];
        ui->event_previous_focus[i] = previous_focus;
        cui_id activation = ui->event_activations[previous];
        ui->event_activations[previous] = ui->event_activations[i];
        ui->event_activations[i] = activation;
        size_t slot = ui->event_activation_slots[previous];
        ui->event_activation_slots[previous] = ui->event_activation_slots[i];
        ui->event_activation_slots[i] = slot;
        bool consumed = ui->event_consumed[previous];
        ui->event_consumed[previous] = ui->event_consumed[i];
        ui->event_consumed[i] = consumed;
    }
    ui->text_source_count = 0;
    ui->node_count = 0;
    ui->paint_root_count = 0;
    ui->bounds_track_count = 0;
    ui->prepaint_count = ui->prepaint_reserved = 0;
    ui->prepaint_capacity = false;
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
    /* IDs are stable while cached. Reclaim old style generations only between
     * frames, before Clay stores any references into its measurement cache. */
    if (ui->text_style_count > ui->text_style_capacity / 2) {
        Clay_ResetMeasureTextCache();
        ui->text_style_count = 0;
    }
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
static Clay_SizingAxis cui_core_box_axis(float n, bool fixed, float maximum, bool percent) {
    if (percent)
        return (Clay_SizingAxis){.size = {.percent = isfinite(n) ? fminf(1, fmaxf(0, n)) : 0},
                                 .type = CLAY__SIZING_TYPE_PERCENT};
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
    uint32_t paint_root = portal               ? ++ui->paint_root_count
                          : parent != SIZE_MAX ? ui->nodes[parent].paint_root
                                               : 0;
    float opacity =
        b.custom_opacity ? (isfinite(b.opacity) ? fminf(1, fmaxf(0, b.opacity)) : 0) : 1;
    if (parent != SIZE_MAX)
        opacity *= ui->nodes[parent].opacity;
    if (portal && !absolute)
        parent = SIZE_MAX;
    else if (parent != SIZE_MAX)
        layer = ui->nodes[parent].layer;
    ui->nodes[ui->node_count] = (cui_core_box_node){
        .clay_id = internal_id,
        .id = id,
        .focus_scope = ui->focus_depth ? ui->focus_stack[ui->focus_depth - 1] : SIZE_MAX,
        .prepaint = SIZE_MAX,
        .parent = parent,
        .clip = b.clip,
        .layout_only = layout_only,
        .inert = inert,
        .layer = layer,
        .opacity = opacity,
        .paint_root = paint_root};
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
        .userData = layout_only ? (void *)ui : &ui->nodes[ui->node_count - 1],
        .layout = {.sizing = {cui_core_box_axis(b.width, b.fixed_width, b.max_width,
                                                b.width_percent),
                              cui_core_box_axis(b.height, b.fixed_height, b.max_height,
                                                b.height_percent)},
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
bool cui_set_box_scroll_offset(cui_context *ui, cui_id id, float x, float y) {
    if (!cui_core_mutation(ui))
        return false;
    if (!ui || !ui->in_frame || !id || !isfinite(x) || !isfinite(y) || cui_core_inert(ui))
        return false;
    for (size_t i = 0; i < ui->node_count; ++i)
        if (ui->nodes[i].clay_id == cui_core_folded(id) && ui->nodes[i].clip &&
            !ui->nodes[i].inert) {
            Clay_SetCurrentContext(ui->clay);
            Clay_LayoutElementHashMapItem *item = Clay__GetHashMapItem(ui->nodes[i].clay_id);
            if (!item->layoutElement)
                return false;
            item->layoutElement->config.clip.childOffset = (Clay_Vector2){x, y};
            return true;
        }
    return false;
}

bool cui_bitmap_valid(cui_bitmap b) {
    return b.pixels && b.width && b.height && b.width <= 16384 && b.height <= 16384 &&
           b.stride >= (size_t)b.width * 4 && b.stride <= SIZE_MAX / b.height;
}
static bool cui_core_bitmap_paint_valid(cui_bitmap_paint *paint) {
    if (!cui_bitmap_valid(paint->bitmap) || !isfinite(paint->opacity) || paint->opacity < 0 ||
        paint->opacity > 1)
        return false;
    cui_rect *s = &paint->source;
    if (!s->x && !s->y && !s->w && !s->h)
        *s = (cui_rect){0, 0, (float)paint->bitmap.width, (float)paint->bitmap.height};
    return isfinite(s->x) && isfinite(s->y) && isfinite(s->w) && isfinite(s->h) && s->x >= 0 &&
           s->y >= 0 && s->w > 0 && s->h > 0 && s->x + s->w <= paint->bitmap.width &&
           s->y + s->h <= paint->bitmap.height;
}
bool cui_paint_bitmap(cui_context *ui, cui_id id, cui_bitmap_paint paint, cui_box box) {
    if (!cui_core_mutation(ui))
        return false;
    if (!ui || !ui->in_frame || !cui_core_bitmap_paint_valid(&paint))
        return false;
    cui_rect *s = &paint.source;
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
static bool cui_core_mesh_bounds(cui_mesh_paint paint, cui_rect *bounds) {
    if (!paint.vertices || !paint.vertex_count || paint.vertex_count % 3 ||
        paint.vertex_count > SIZE_MAX / sizeof(*paint.vertices) || !isfinite(paint.origin_x) ||
        !isfinite(paint.origin_y) || !isfinite(paint.scale) || paint.scale <= 0 ||
        !isfinite(paint.color.r) || !isfinite(paint.color.g) || !isfinite(paint.color.b) ||
        !isfinite(paint.color.a) || paint.color.a < 0 || paint.color.a > 1)
        return false;
    float left = INFINITY, top = INFINITY, right = -INFINITY, bottom = -INFINITY;
    for (size_t i = 0; i < paint.vertex_count; ++i) {
        cui_mesh_vertex vertex = paint.vertices[i];
        float x = vertex.x * paint.scale + paint.origin_x;
        float y = vertex.y * paint.scale + paint.origin_y;
        if (!isfinite(vertex.x) || !isfinite(vertex.y) || !isfinite(vertex.s) ||
            !isfinite(vertex.t) || !isfinite(x) || !isfinite(y))
            return false;
        left = fminf(left, x);
        top = fminf(top, y);
        right = fmaxf(right, x);
        bottom = fmaxf(bottom, y);
    }
    float width = right - left, height = bottom - top;
    if (!isfinite(width) || !isfinite(height))
        return false;
    if (bounds)
        *bounds = (cui_rect){left, top, width, height};
    return true;
}
bool cui_mesh_valid(cui_mesh_paint paint) {
    return cui_core_mesh_bounds(paint, NULL);
}
bool cui_paint_mesh(cui_context *ui, cui_id id, cui_mesh_paint paint, cui_box box) {
    if (!cui_core_mutation(ui))
        return false;
    cui_rect bounds;
    if (!ui || !ui->in_frame || !cui_core_mesh_bounds(paint, &bounds))
        return false;
    if (!box.width && !box.fixed_width)
        box.width = bounds.w;
    if (!box.height && !box.fixed_height)
        box.height = bounds.h;
    cui_core_begin_box(ui, id, box, NULL, 0, false);
    bool ok = !ui->overflow_depth && ui->depth && ui->opened[ui->depth - 1];
    if (ok) {
        cui_core_box_node *node = &ui->nodes[ui->node_stack[ui->depth - 1]];
        node->mesh = paint;
        node->has_mesh = true;
        Clay__GetOpenLayoutElement()->config.custom.customData = node;
    }
    cui_end_box(ui);
    return ok;
}
bool cui_set_outline(cui_context *ui, float width, cui_color color, cui_corner_radii radii) {
    if (!cui_core_mutation(ui))
        return false;
    if (!ui || !ui->in_frame || !ui->depth || ui->overflow_depth || !ui->opened[ui->depth - 1] ||
        !isfinite(width) || width < 0 || !isfinite(radii.top_left) || !isfinite(radii.top_right) ||
        !isfinite(radii.bottom_left) || !isfinite(radii.bottom_right) || radii.top_left < 0 ||
        radii.top_right < 0 || radii.bottom_left < 0 || radii.bottom_right < 0)
        return false;
    cui_core_box_node *node = &ui->nodes[ui->node_stack[ui->depth - 1]];
    node->outline_width = width;
    node->outline_color = color;
    node->outline_radii = radii;
    Clay__GetOpenLayoutElement()->config.custom.customData = node;
    return true;
}
bool cui_set_underlay_ex(cui_context *ui, cui_underlay_layout layout, cui_color color,
                         cui_corner_radii radii) {
    if (!cui_core_mutation(ui))
        return false;
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
    node->has_underlay = true;
    node->bitmap_underlay = false;
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
bool cui_set_bitmap_underlay(cui_context *ui, cui_underlay_layout layout, cui_bitmap_paint paint,
                             cui_corner_radii radii) {
    if (!cui_core_mutation(ui))
        return false;
    if (!cui_core_bitmap_paint_valid(&paint) ||
        !cui_set_underlay_ex(ui, layout, (cui_color){0}, radii))
        return false;
    cui_core_box_node *node = &ui->nodes[ui->node_stack[ui->depth - 1]];
    node->bitmap = paint;
    node->bitmap_underlay = true;
    return true;
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
    box.width_percent = box.height_percent = false;
    cui_core_begin_box(ui, id, box, &bounds, layer, false);
}
void cui_begin_absolute_box(cui_context *ui, cui_id id, cui_box box, cui_rect bounds) {
    if (!ui || !ui->in_frame)
        return;
    if (!ui->depth || !isfinite(bounds.x) || !isfinite(bounds.y) || !isfinite(bounds.w) ||
        !isfinite(bounds.h) || (bounds.w < 0 && bounds.w != -1) ||
        (bounds.h < 0 && bounds.h != -1)) {
        /* Reuse the balanced malformed-portal suppression path. */
        cui_begin_portal(ui, id, box, (cui_rect){0, 0, -1, 0}, 1);
        return;
    }
    box.width = bounds.w;
    box.height = bounds.h;
    box.fixed_width = bounds.w >= 0;
    box.fixed_height = bounds.h >= 0;
    box.width_percent = box.height_percent = false;
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
static uint16_t cui_core_text_style_id(cui_context *ui, float size, float line_height,
                                       cui_text_style style) {
    for (size_t i = 0; i < ui->text_style_count; i++)
        if (ui->text_styles[i].size == size && ui->text_styles[i].line_height == line_height &&
            ui->text_styles[i].style.weight == style.weight &&
            ui->text_styles[i].style.italic == style.italic &&
            ui->text_styles[i].style.monospace == style.monospace &&
            ui->text_styles[i].style.family == style.family)
            return (uint16_t)(i + 1);
    if (ui->text_style_count == ui->text_style_capacity) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return 0;
    }
    ui->text_styles[ui->text_style_count++] = (cui_core_text_style){size, line_height, style};
    return (uint16_t)ui->text_style_count;
}
static void cui_core_text(cui_context *ui, cui_str t, float size, cui_color c, float line_height,
                          cui_text_style text_style, bool nowrap,
                          const cui_text_line_source *source, const cui_text_color_span *colors,
                          size_t color_count) {
    if (!cui_core_active(ui))
        return;
    if (!cui_core_text_style_valid(text_style))
        return;
    if (ui->overflow_depth || (ui->depth && !ui->opened[ui->depth - 1]))
        return;
    if (ui->element_count == ui->config.max_elements) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return;
    }
    size = isfinite(size) ? fminf(65535, fmaxf(0, size)) : 0;
    line_height = isfinite(line_height) ? fminf(65535, fmaxf(0, line_height)) : 0;
    bool special = size != floorf(size) || line_height != floorf(line_height) ||
                   text_style.weight || text_style.italic || text_style.monospace ||
                   text_style.family;
    cui_str original = source ? source->paragraph : t;
    cui_str retained_paragraph = {0};
    if (source)
        for (size_t i = 0; i < ui->text_source_count; i++) {
            cui_str candidate = ui->text_sources[i].source.paragraph;
            if (candidate.length == original.length &&
                (!original.length || !memcmp(candidate.data, original.data, original.length))) {
                retained_paragraph = candidate;
                break;
            }
        }
    size_t available = ui->config.frame_bytes - ui->string_used;
    size_t paragraph_bytes = retained_paragraph.data || !original.length ? 0 : original.length + 1;
    size_t line_bytes = source ? source->byte_end - source->byte_start + 1 : 0;
    if ((!original.data && original.length) || original.length > INT32_MAX ||
        paragraph_bytes > available || line_bytes > available - paragraph_bytes) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return;
    }
    uint16_t style = special ? cui_core_text_style_id(ui, size, line_height, text_style) : 0;
    if (special && !style)
        return;
    ui->element_count++;
    original = retained_paragraph.data ? retained_paragraph : cui_core_copy_string(ui, original);
    cui_text_line_source retained =
        source ? *source : (cui_text_line_source){original, 0, original.length};
    retained.paragraph = original;
    if (source) {
        /* Unique slice storage gives Clay exact submission identity even when shared
         * paragraphs have the same start with different ranges/styles/colors. */
        char *slice = ui->strings + ui->string_used;
        size_t length = source->byte_end - source->byte_start;
        if (length)
            memcpy(slice, original.data + source->byte_start, length);
        slice[length] = 0;
        ui->string_used += length + 1;
        t = (cui_str){slice, length};
    } else
        t = original;
    ui->text_sources[ui->text_source_count++] =
        (cui_core_text_source){t, retained, colors, color_count};
    Clay__OpenTextElement(
        (Clay_String){false, (int32_t)t.length, t.data},
        (Clay_TextElementConfig){.userData = cui_core_layout_only(ui) ? (void *)ui
                                             : ui->depth ? &ui->nodes[ui->node_stack[ui->depth - 1]]
                                                         : NULL,
                                 .fontSize = cui_core_scalar(size),
                                 .wrapMode = nowrap ? CLAY_TEXT_WRAP_NONE : CLAY_TEXT_WRAP_WORDS,
                                 .fontId = style,
                                 .lineHeight = style ? 0 : cui_core_scalar(line_height),
                                 .textColor = {c.r, c.g, c.b, c.a}});
}
bool cui_text_context_line(cui_context *ui, cui_text_line_source source, float size,
                           cui_color color, float line_height, cui_text_style style) {
    if (!ui || !cui_text_line_source_valid(source) || !isfinite(size) || size <= 0 ||
        !isfinite(line_height) || line_height < 0 || !cui_core_text_style_valid(style))
        return false;
    size_t before = ui->text_source_count;
    cui_core_text(ui, (cui_str){0}, size, color, line_height, style, true, &source, NULL, 0);
    return ui->text_source_count != before;
}
bool cui_text_context_line_colored(cui_context *ui, cui_text_line_source source, float size,
                                   cui_color color, float line_height, cui_text_style style,
                                   const cui_text_color_span *colors, size_t count) {
    if (!ui || !cui_frame_active(ui) || !cui_text_line_source_valid(source) || (!colors && count) ||
        !isfinite(size) || size <= 0 || !isfinite(line_height) || line_height < 0 ||
        !cui_core_text_style_valid(style))
        return false;
    size_t scan = source.byte_start;
    uint32_t cp;
    while (scan < source.byte_end)
        if (!cui_core_text_scalar(source.paragraph, &scan, &cp) || cp == 0x2028)
            return false;
    if (count > CUI_TEXT_COLOR_SPAN_MAX) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return false;
    }
    if (!count)
        return cui_text_context_line(ui, source, size, color, line_height, style);
    size_t previous = source.byte_start;
    for (size_t i = 0; i < count; i++) {
        cui_text_line_source range = {source.paragraph, colors[i].byte_start, colors[i].byte_end};
        if (range.byte_start < previous || range.byte_end > source.byte_end ||
            !cui_text_line_source_valid(range))
            return false;
        previous = range.byte_end;
    }
    size_t padding =
        (_Alignof(cui_text_color_span) - ui->string_used % _Alignof(cui_text_color_span)) %
        _Alignof(cui_text_color_span);
    size_t available = ui->config.frame_bytes - ui->string_used;
    if (padding > available || count > (available - padding) / sizeof *colors) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return false;
    }
    size_t before_bytes = ui->string_used, before_count = ui->text_source_count;
    cui_text_color_span *copy = (void *)(ui->strings + ui->string_used + padding);
    if (count)
        memcpy(copy, colors, count * sizeof *colors);
    float opacity = ui->depth && !cui_core_layout_only(ui)
                        ? ui->nodes[ui->node_stack[ui->depth - 1]].opacity
                        : 1;
    for (size_t i = 0; i < count; i++)
        copy[i].color.a = (uint8_t)(copy[i].color.a * opacity);
    ui->string_used += padding + count * sizeof *colors;
    cui_core_text(ui, (cui_str){0}, size, color, line_height, style, true, &source, copy, count);
    if (ui->text_source_count == before_count) {
        ui->string_used = before_bytes;
        return false;
    }
    return true;
}
void cui_text_ex(cui_context *ui, cui_str t, float size, cui_color c, float line_height,
                 cui_text_style style) {
    cui_core_text(ui, t, size, c, line_height, style, false, NULL, NULL, 0);
}
void cui_text_line(cui_context *ui, cui_str t, float size, cui_color c, float line_height,
                   cui_text_style style) {
    cui_core_text(ui, t, size, c, line_height, style, true, NULL, NULL, 0);
}
void cui_text_with_line_height(cui_context *ui, cui_str t, float size, cui_color c,
                               float line_height) {
    cui_text_ex(ui, t, size, c, line_height, (cui_text_style){0});
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
        if (ui->focus == id) {
            cui_id restored = 0;
            for (size_t e = ui->event_count; e-- > 0;)
                if (ui->events[e].type == CUI_EVENT_ACCESSIBILITY_ACTION &&
                    ui->events[e].accessibility_action == CUI_ACCESSIBILITY_FOCUS &&
                    ui->events[e].accessibility_target == id && !ui->event_consumed[e]) {
                    restored = ui->event_previous_focus[e];
                    ui->event_consumed[e] = true;
                    ui->event_targets[e] = 0;
                    break;
                }
            ui->focus = restored;
        }
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
            bool allowed = !disabled;
            for (size_t e = 0; e < ui->event_count; e++)
                if (ui->events[e].type == CUI_EVENT_ACCESSIBILITY_ACTION &&
                    ui->event_activation_slots[e] == i) {
                    bool same_role = false;
                    for (size_t n = 0; n < ui->previous_count; n++)
                        same_role |= ui->previous[n].id == id && ui->previous[n].role == role;
                    size_t scope =
                        ui->focus_depth ? ui->focus_stack[ui->focus_depth - 1] : SIZE_MAX;
                    allowed &= same_role && !ui->event_consumed[e] &&
                               cui_core_scope_contains(ui->focus_scopes, scope, ui->top_modal);
                    if (!allowed) {
                        ui->event_consumed[e] = true;
                        ui->event_targets[e] = 0;
                        ui->event_activations[e] = 0;
                    }
                }
            out.activated |= allowed;
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
/* Clay emits floating-root scissor commands with NULL userData and only the
 * nearest clipping ancestor. That root is painted outside its parent's draw
 * stack, so restore the complete CUI clip ancestry before renderer conversion.
 * Detached portals have parent == SIZE_MAX and retain their escape semantics. */
static cui_rect cui_core_ancestral_clip(cui_context *ui, const Clay_RenderCommand *command,
                                        cui_rect bounds) {
    uint32_t clip_id = command->id;
    if (!command->userData) {
        clip_id = 0;
        for (int32_t i = 0; i < ui->clay->layoutElementTreeRoots.length; i++) {
            const Clay__LayoutElementTreeRoot *root =
                &ui->clay->layoutElementTreeRoots.internalArray[i];
            const Clay_LayoutElement *element =
                &ui->clay->layoutElements.internalArray[root->layoutElementIndex];
            if (root->clipElementId &&
                Clay__HashNumber(element->id, element->children.length + 10).id == command->id) {
                clip_id = root->clipElementId;
                break;
            }
        }
    }
    for (size_t i = 0; i < ui->node_count; i++) {
        if (ui->nodes[i].clay_id != clip_id)
            continue;
        for (size_t n = i; n != SIZE_MAX; n = ui->nodes[n].parent) {
            if (!ui->nodes[n].clip)
                continue;
            Clay_ElementData data =
                Clay_GetElementData((Clay_ElementId){.id = ui->nodes[n].clay_id});
            if (!data.found)
                continue;
            float left = fmaxf(bounds.x, data.boundingBox.x);
            float top = fmaxf(bounds.y, data.boundingBox.y);
            float right = fminf(bounds.x + bounds.w, data.boundingBox.x + data.boundingBox.width);
            float bottom = fminf(bounds.y + bounds.h, data.boundingBox.y + data.boundingBox.height);
            bounds = (cui_rect){left, top, fmaxf(0, right - left), fmaxf(0, bottom - top)};
        }
        break;
    }
    return bounds;
}
static void *cui_core_frame_copy(cui_context *ui, const void *data, size_t bytes) {
    size_t alignment = _Alignof(max_align_t);
    size_t padding = (alignment - ui->string_used % alignment) % alignment;
    size_t available = ui->config.frame_bytes - ui->string_used;
    if (padding > available || bytes > available - padding) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return NULL;
    }
    void *copy = ui->strings + ui->string_used + padding;
    if (bytes)
        memcpy(copy, data, bytes);
    ui->string_used += padding + bytes;
    return copy;
}
bool cui_set_hover_hitbox(cui_context *ui, bool enabled) {
    if (!cui_core_active(ui) || !ui->depth || ui->overflow_depth || !ui->opened[ui->depth - 1])
        return false;
    cui_core_box_node *node = &ui->nodes[ui->node_stack[ui->depth - 1]];
    if (!node->id)
        return false;
    node->hover_hitbox = enabled;
    return true;
}
bool cui_set_prepaint(cui_context *ui, cui_prepaint_options options) {
    if (!cui_core_active(ui) || !ui->depth || ui->overflow_depth || !ui->opened[ui->depth - 1])
        return false;
    size_t n = ui->node_stack[ui->depth - 1];
    cui_core_box_node *node = &ui->nodes[n];
    if (!node->id || !options.callback || !options.command_budget ||
        (options.payload_bytes && !options.payload))
        return false;
    if (node->prepaint != SIZE_MAX) {
        cui_core_fail(ui, CUI_ERROR_DUPLICATE_ID);
        return false;
    }
    if (options.deferred_layer == 0)
        options.deferred_layer = node->layer;
    if (options.deferred_layer < node->layer)
        return false;
    if (ui->prepaint_count == ui->config.max_elements ||
        options.command_budget > ui->config.max_commands - ui->prepaint_reserved) {
        ui->prepaint_capacity = true;
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return false;
    }
    const void *payload = NULL;
    if (options.payload_bytes) {
        payload = cui_core_frame_copy(ui, options.payload, options.payload_bytes);
        if (!payload) {
            ui->prepaint_capacity = true;
            return false;
        }
    }
    options.payload = payload;
    node->prepaint = ui->prepaint_count;
    if (options.hover_hitbox)
        node->hover_hitbox = true;
    ui->prepaints[ui->prepaint_count++] = (cui_core_prepaint){.node = n,
                                                              .offset = ui->prepaint_reserved,
                                                              .budget = options.command_budget,
                                                              .options = options};
    ui->prepaint_reserved += options.command_budget;
    Clay__GetOpenLayoutElement()->config.custom.customData = node;
    return true;
}
static bool cui_core_writer(cui_prepaint_writer *writer) {
    return writer && writer->ui && writer->ui->prepainting &&
           writer->ui->prepaint_writer == writer &&
           writer->registration < writer->ui->prepaint_count;
}
static bool cui_core_prepaint_rect(cui_rect r) {
    return isfinite(r.x) && isfinite(r.y) && isfinite(r.w) && isfinite(r.h) && r.w >= 0 &&
           r.h >= 0 && isfinite(r.x + r.w) && isfinite(r.y + r.h);
}
static bool cui_core_prepaint_command(cui_draw_command *c) {
    if (!cui_core_prepaint_rect(c->bounds) || c->type < CUI_DRAW_RECT || c->type > CUI_DRAW_MESH ||
        c->type == CUI_DRAW_CLIP_PUSH || c->type == CUI_DRAW_CLIP_POP)
        return false;
    if (!isfinite(c->radius) || c->radius < 0 || !isfinite(c->stroke) || c->stroke < 0)
        return false;
    if (c->custom_corners) {
        const float radii[] = {c->corner_radii.top_left, c->corner_radii.top_right,
                               c->corner_radii.bottom_left, c->corner_radii.bottom_right};
        for (size_t i = 0; i < 4; i++)
            if (!isfinite(radii[i]) || radii[i] < 0)
                return false;
    }
    if (c->custom_border_edges) {
        const float edges[] = {c->border_edges.left, c->border_edges.right, c->border_edges.top,
                               c->border_edges.bottom};
        for (size_t i = 0; i < 4; i++)
            if (!isfinite(edges[i]) || edges[i] < 0)
                return false;
    }
    if (c->type == CUI_DRAW_IMAGE)
        return cui_core_bitmap_paint_valid(&c->bitmap);
    if (c->type == CUI_DRAW_MESH)
        return cui_core_mesh_bounds(c->mesh, &c->bounds);
    if (c->type != CUI_DRAW_TEXT)
        return true;
    if (!isfinite(c->font_size) || c->font_size <= 0 || c->font_size > 65535 ||
        !cui_core_text_style_valid(c->text_style))
        return false;
    if (c->text_source.paragraph.length) {
        if (!cui_text_line_source_valid(c->text_source) ||
            c->text_color_count > CUI_TEXT_COLOR_SPAN_MAX ||
            (c->text_color_count && !c->text_colors))
            return false;
        size_t previous = c->text_source.byte_start;
        for (size_t i = 0; i < c->text_color_count; i++) {
            cui_text_line_source range = {c->text_source.paragraph, c->text_colors[i].byte_start,
                                          c->text_colors[i].byte_end};
            if (range.byte_start < previous || range.byte_end > c->text_source.byte_end ||
                !cui_text_line_source_valid(range))
                return false;
            previous = range.byte_end;
        }
    } else if ((c->text.length && !c->text.data) || c->text.length > INT32_MAX ||
               c->text_color_count)
        return false;
    return true;
}
bool cui_prepaint_emit(cui_prepaint_writer *writer, const cui_draw_command *command,
                       bool deferred) {
    if (!cui_core_writer(writer))
        return false;
    cui_context *ui = writer->ui;
    cui_core_prepaint *p = &ui->prepaints[writer->registration];
    if (ui->prepaint_failed)
        return false;
    cui_draw_command c = command ? *command : (cui_draw_command){.type = (cui_draw_type)-1};
    if (!command || !cui_core_prepaint_command(&c) || (deferred && !p->info.modal_allowed)) {
        cui_core_fail(ui, CUI_ERROR_LAYOUT);
        return false;
    }
    bool local = false, late = false;
    for (size_t i = 0; i < p->count; i++)
        if (ui->prepaint_deferred[p->offset + i])
            late = true;
        else
            local = true;
    size_t needed = p->count + 1 + 2 * ((local || !deferred) + (late || deferred));
    if (needed > p->budget) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return false;
    }
    if (c.type == CUI_DRAW_TEXT) {
        if (c.text_source.paragraph.length) {
            c.text_source.paragraph = cui_core_copy_string(ui, c.text_source.paragraph);
            if (ui->prepaint_failed)
                return false;
            c.text = (cui_str){c.text_source.paragraph.data + c.text_source.byte_start,
                               c.text_source.byte_end - c.text_source.byte_start};
            if (c.text_color_count) {
                c.text_colors = cui_core_frame_copy(ui, c.text_colors,
                                                    c.text_color_count * sizeof(*c.text_colors));
                if (!c.text_colors)
                    return false;
            }
        } else {
            c.text = cui_core_copy_string(ui, c.text);
            if (ui->prepaint_failed)
                return false;
        }
    }
    ui->prepaint_commands[p->offset + p->count] = c;
    ui->prepaint_deferred[p->offset + p->count] = deferred;
    p->count++;
    return true;
}
cui_size cui_prepaint_measure(cui_prepaint_writer *writer, cui_str text, float size,
                              cui_text_style style) {
    if (!cui_core_writer(writer))
        return (cui_size){0};
    return cui_measure_text_ex(writer->ui, text, size, style);
}
void cui_prepaint_request_frame(cui_prepaint_writer *writer) {
    if (cui_core_writer(writer))
        writer->ui->dirty = true;
}
static cui_rect cui_core_prepaint_intersection(cui_rect a, cui_rect b) {
    float x = fmaxf(a.x, b.x), y = fmaxf(a.y, b.y);
    return (cui_rect){x, y, fmaxf(0, fminf(a.x + a.w, b.x + b.w) - x),
                      fmaxf(0, fminf(a.y + a.h, b.y + b.h) - y)};
}
static bool cui_core_prepaint_scope(cui_context *ui, size_t scope) {
    for (size_t n = scope; n != SIZE_MAX; n = ui->focus_scopes[n].parent)
        if (!ui->focus_scopes[n].active)
            return false;
    return cui_core_scope_contains(ui->focus_scopes, scope, ui->top_modal);
}
static bool cui_core_prepaint_above(cui_context *ui, size_t candidate, size_t previous) {
    if (previous == SIZE_MAX)
        return true;
    cui_core_box_node *a = &ui->nodes[candidate], *b = &ui->nodes[previous];
    uint64_t ao = ((uint64_t)a->paint_root << 32) | (candidate + 1),
             bo = ((uint64_t)b->paint_root << 32) | (previous + 1);
    return a->layer > b->layer || (a->layer == b->layer && ao >= bo);
}
static void cui_core_prepaint_prepare(cui_context *ui, Clay_RenderCommandArray commands) {
    cui_rect viewport = {0, 0, ui->config.width, ui->config.height};
    for (size_t i = 0; i < ui->node_count; i++) {
        Clay_ElementData d = Clay_GetElementData((Clay_ElementId){.id = ui->nodes[i].clay_id});
        cui_rect bounds = {d.boundingBox.x, d.boundingBox.y, d.boundingBox.width,
                           d.boundingBox.height};
        bool valid = cui_core_prepaint_rect(bounds);
        if (!valid) {
            bounds = (cui_rect){0};
            cui_core_fail(ui, CUI_ERROR_LAYOUT);
        }
        ui->geometry[i] = (cui_core_geometry){
            .bounds = bounds,
            .clip = viewport,
            .found = d.found && valid,
            .modal_allowed = cui_core_prepaint_scope(ui, ui->nodes[i].focus_scope)};
    }
    size_t hit = SIZE_MAX;
    cui_id hit_id = 0;
    for (size_t i = 0; i < ui->node_count; i++) {
        cui_core_geometry *g = &ui->geometry[i];
        for (size_t n = i; n != SIZE_MAX; n = ui->nodes[n].parent)
            if (ui->nodes[n].clip)
                g->clip = cui_core_prepaint_intersection(g->clip, ui->geometry[n].bounds);
        g->visible = cui_core_prepaint_intersection(g->bounds, g->clip);
        if (g->found && g->modal_allowed && !ui->nodes[i].layout_only &&
            ui->nodes[i].hover_hitbox &&
            cui_core_inside(g->visible, ui->pointer_x, ui->pointer_y) &&
            cui_core_prepaint_above(ui, i, hit)) {
            hit = i;
            hit_id = ui->nodes[i].id;
        }
    }
    for (size_t i = 0; i < ui->semantic_count; i++) {
        cui_semantic *s = &ui->semantics[i];
        if (s->disabled ||
            (!cui_core_focusable(s) && !s->details.pointer_activatable && !s->details.scrollable) ||
            !cui_core_prepaint_scope(ui, ui->semantic_scopes[i]))
            continue;
        for (size_t n = 0; n < ui->node_count; n++)
            if (ui->nodes[n].id == s->id) {
                cui_core_geometry *g = &ui->geometry[n];
                if (g->found && !ui->nodes[n].inert && !ui->nodes[n].layout_only &&
                    g->modal_allowed && cui_core_inside(g->visible, ui->pointer_x, ui->pointer_y) &&
                    cui_core_prepaint_above(ui, n, hit)) {
                    hit = n;
                    hit_id = s->id;
                }
                break;
            }
    }
    for (size_t i = 0; i < ui->prepaint_count; i++) {
        cui_core_prepaint *p = &ui->prepaints[i];
        cui_core_box_node *n = &ui->nodes[p->node];
        cui_core_geometry *g = &ui->geometry[p->node];
        p->info = (cui_prepaint_info){
            .owner = n->id,
            .bounds = g->bounds,
            .clip = g->clip,
            .visible_bounds = g->visible,
            .pointer_x = ui->pointer_x,
            .pointer_y = ui->pointer_y,
            .pointer_valid = isfinite(ui->pointer_x) && isfinite(ui->pointer_y),
            .pointer_inside = g->found && cui_core_inside(g->visible, ui->pointer_x, ui->pointer_y),
            .modal_allowed = g->modal_allowed,
            .inert = n->inert || !g->found,
            .pointer_hit = hit_id,
            .capture = ui->capture,
            .opacity = n->opacity,
            .frame_time = ui->frame_time,
            .frame_delta = ui->dt};
        p->info.pointer_visible =
            p->info.pointer_inside && p->info.modal_allowed && !p->info.inert && hit == p->node;
    }
    (void)commands;
    for (size_t iteration = 0; iteration < ui->prepaint_count; iteration++) {
        size_t selected = SIZE_MAX;
        for (size_t i = 0; i < ui->prepaint_count; i++)
            if (!ui->prepaints[i].done &&
                (selected == SIZE_MAX ||
                 cui_core_prepaint_above(ui, ui->prepaints[selected].node, ui->prepaints[i].node)))
                selected = i;
        if (selected == SIZE_MAX)
            break;
        cui_core_prepaint *p = &ui->prepaints[selected];
        p->done = true;
        if (ui->nodes[p->node].layout_only)
            continue;
        size_t bytes = ui->string_used;
        cui_prepaint_writer writer = {ui, selected};
        ui->prepaint_writer = &writer;
        ui->prepainting = true;
        ui->prepaint_failed = false;
        cui_core_paint_context = ui;
        bool ok = p->options.callback(&p->info, p->options.payload, &writer);
        if (!ok)
            cui_core_fail(ui, CUI_ERROR_LAYOUT);
        cui_core_paint_context = NULL;
        ui->prepainting = false;
        ui->prepaint_writer = NULL;
        cui_core_select_context(ui);
        if (ui->prepaint_failed) {
            p->count = 0;
            ui->string_used = bytes;
        }
        if (ui->prepaint_capacity)
            break;
    }
}
static bool cui_core_prepaint_output(cui_context *ui, cui_core_prepaint *p, bool deferred) {
    size_t count = 0;
    for (size_t i = 0; i < p->count; i++)
        count += ui->prepaint_deferred[p->offset + i] == deferred;
    if (!count)
        return true;
    if (count + 2 > ui->config.max_commands - ui->count) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        ui->count = 0;
        ui->prepaint_capacity = true;
        return false;
    }
    ui->commands[ui->count++] = (cui_draw_command){
        .type = CUI_DRAW_CLIP_PUSH,
        .bounds = deferred ? (cui_rect){0, 0, ui->config.width, ui->config.height}
                           : p->info.visible_bounds};
    for (size_t i = 0; i < p->count; i++)
        if (ui->prepaint_deferred[p->offset + i] == deferred) {
            cui_draw_command c = ui->prepaint_commands[p->offset + i];
            if (c.type == CUI_DRAW_IMAGE)
                c.bitmap.opacity *= p->info.opacity;
            else if (c.type == CUI_DRAW_MESH) {
                c.mesh.color.r *= p->info.opacity;
                c.mesh.color.g *= p->info.opacity;
                c.mesh.color.b *= p->info.opacity;
                c.mesh.color.a *= p->info.opacity;
            } else
                c.color.a = (uint8_t)(c.color.a * p->info.opacity);
            ui->commands[ui->count++] = c;
        }
    ui->commands[ui->count++] = (cui_draw_command){.type = CUI_DRAW_CLIP_POP};
    return true;
}
static bool cui_core_prepaint_defer(cui_context *ui, int32_t next_layer) {
    for (;;) {
        size_t selected = SIZE_MAX;
        for (size_t i = 0; i < ui->prepaint_count; i++) {
            cui_core_prepaint *p = &ui->prepaints[i];
            if (p->deferred_done || p->options.deferred_layer >= next_layer)
                continue;
            if (selected == SIZE_MAX ||
                p->options.deferred_layer < ui->prepaints[selected].options.deferred_layer ||
                (p->options.deferred_layer == ui->prepaints[selected].options.deferred_layer &&
                 cui_core_prepaint_above(ui, ui->prepaints[selected].node, p->node)))
                selected = i;
        }
        if (selected == SIZE_MAX)
            return true;
        cui_core_prepaint *p = &ui->prepaints[selected];
        p->deferred_done = true;
        if (!cui_core_prepaint_output(ui, p, true))
            return false;
    }
}

/* Clay's floating-root scissor wrappers omit zIndex. Resolve their root layer
 * from the first painted command before inserting a deferred stream. */
static int32_t cui_core_prepaint_layer(cui_context *ui, Clay_RenderCommandArray a, int32_t start) {
    for (int32_t i = start; i < a.length; i++) {
        Clay_RenderCommand *c = &a.internalArray[i];
        if (c->commandType == CLAY_RENDER_COMMAND_TYPE_SCISSOR_START ||
            c->commandType == CLAY_RENDER_COMMAND_TYPE_SCISSOR_END)
            continue;
        if (c->userData && c->userData != ui)
            return ((cui_core_box_node *)c->userData)->layer;
        return c->zIndex;
    }
    return INT32_MAX;
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
    if (!ui->prepaint_capacity)
        cui_core_prepaint_prepare(ui, a);
    ui->count = 0;
    if (ui->prepaint_capacity)
        goto cui_core_commands_done;
    uint32_t underlay_background_id = 0;
    size_t conversion_clip_depth = 0;
    for (int32_t i = 0; i < a.length; i++) {
        Clay_RenderCommand *c = &a.internalArray[i];
        if (!conversion_clip_depth && c->commandType != CLAY_RENDER_COMMAND_TYPE_SCISSOR_END &&
            !cui_core_prepaint_defer(ui, cui_core_prepaint_layer(ui, a, i)))
            goto cui_core_commands_done;
        if (c->commandType == CLAY_RENDER_COMMAND_TYPE_SCISSOR_START)
            conversion_clip_depth++;
        else if (c->commandType == CLAY_RENDER_COMMAND_TYPE_SCISSOR_END && conversion_clip_depth)
            conversion_clip_depth--;
        if (c->userData == ui && c->commandType != CLAY_RENDER_COMMAND_TYPE_SCISSOR_START &&
            c->commandType != CLAY_RENDER_COMMAND_TYPE_SCISSOR_END)
            continue;
        float opacity = c->userData && c->userData != ui
                            ? ((const cui_core_box_node *)c->userData)->opacity
                            : 1;
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
            background.a = (uint8_t)(background.a * opacity);
            size_t needed = (node->has_underlay ? 3 : 0) + (background.a ? 1 : 0) +
                            (node->outline_width > 0 ? 1 : 0) + (node->has_mesh ? 1 : 0);
            if (needed > ui->config.max_commands - ui->count) {
                cui_core_fail(ui, CUI_ERROR_CAPACITY);
                ui->count = 0;
                goto cui_core_commands_done;
            }
            if (node->outline_width > 0) {
                float w = node->outline_width;
                cui_color color = node->outline_color;
                color.a = (uint8_t)(color.a * opacity);
                cui_corner_radii r = node->outline_radii;
                ui->commands[ui->count++] =
                    (cui_draw_command){.type = CUI_DRAW_BORDER,
                                       .bounds = {d.bounds.x - w, d.bounds.y - w,
                                                  d.bounds.w + 2 * w, d.bounds.h + 2 * w},
                                       .color = color,
                                       .stroke = w,
                                       .custom_corners = true,
                                       .corner_radii = {r.top_left + w, r.top_right + w,
                                                        r.bottom_left + w, r.bottom_right + w}};
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
            if (node->has_mesh) {
                cui_mesh_paint mesh = node->mesh;
                mesh.origin_x += d.bounds.x;
                mesh.origin_y += d.bounds.y;
                mesh.color.r *= opacity;
                mesh.color.g *= opacity;
                mesh.color.b *= opacity;
                mesh.color.a *= opacity;
                cui_rect mesh_bounds;
                if (!cui_core_mesh_bounds(mesh, &mesh_bounds)) {
                    cui_core_fail(ui, CUI_ERROR_CAPACITY);
                    ui->count = 0;
                    goto cui_core_commands_done;
                }
                ui->commands[ui->count++] =
                    (cui_draw_command){.type = CUI_DRAW_MESH, .bounds = mesh_bounds, .mesh = mesh};
            }
            if (node->has_underlay) {
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
                cui_color underlay_color = node->underlay_color;
                underlay_color.a = (uint8_t)(underlay_color.a * opacity);
                cui_bitmap_paint bitmap = node->bitmap;
                bitmap.opacity *= opacity;
                ui->commands[ui->count++] = (cui_draw_command){
                    .type = node->bitmap_underlay ? CUI_DRAW_IMAGE : CUI_DRAW_RECT,
                    .bitmap = bitmap,
                    .bounds = underlay,
                    .color = underlay_color,
                    .corner_radii = node->underlay_radii,
                    .custom_corners = true};
                ui->commands[ui->count++] = (cui_draw_command){.type = CUI_DRAW_CLIP_POP};
            }
            if (node->prepaint != SIZE_MAX &&
                !cui_core_prepaint_output(ui, &ui->prepaints[node->prepaint], false))
                goto cui_core_commands_done;
            continue;
        }
        case CLAY_RENDER_COMMAND_TYPE_TEXT:
            d.type = CUI_DRAW_TEXT;
            d.text = (cui_str){c->renderData.text.stringContents.chars,
                               (size_t)c->renderData.text.stringContents.length};
            for (size_t source = 0; source < ui->text_source_count; source++) {
                const cui_core_text_source *record = &ui->text_sources[source];
                if (record->submitted.data != c->renderData.text.stringContents.baseChars)
                    continue;
                size_t offset = (size_t)(d.text.data - record->submitted.data);
                if (offset > record->submitted.length ||
                    d.text.length > record->submitted.length - offset)
                    break;
                d.text_source = record->source;
                d.text_colors = record->colors;
                d.text_color_count = record->color_count;
                d.text_source.byte_start += offset;
                d.text_source.byte_end = d.text_source.byte_start + d.text.length;
                /* Narrow an ordinary multi-paragraph submission to its original hard paragraph. */
                size_t scan = 0, paragraph_start = 0,
                       paragraph_end = record->source.paragraph.length;
                uint32_t cp;
                while (scan < record->source.paragraph.length) {
                    size_t start = scan;
                    if (!cui_core_text_scalar(record->source.paragraph, &scan, &cp)) {
                        d.text_source = (cui_text_line_source){0};
                        break;
                    }
                    if (cp == 10 || cp == 13 || cp == 0x85 || cp == 0x2029 || cp == 0x1c ||
                        cp == 0x1d || cp == 0x1e) {
                        if (start < d.text_source.byte_start)
                            paragraph_start = scan;
                        else {
                            paragraph_end = start;
                            break;
                        }
                    }
                }
                if (d.text_source.paragraph.data && d.text_source.byte_end > paragraph_end &&
                    paragraph_end >= d.text_source.byte_start) {
                    /* Clay splits LF but can retain a trailing CR. Trim only separator
                     * suffixes; never discard a later paragraph in the same command. */
                    size_t trailing = paragraph_end;
                    bool suffix = true;
                    while (trailing < d.text_source.byte_end) {
                        uint32_t separator;
                        if (!cui_core_text_scalar(record->source.paragraph, &trailing,
                                                  &separator) ||
                            !(separator == 10 || separator == 13 || separator == 0x85 ||
                              separator == 0x2029 || separator == 0x1c || separator == 0x1d ||
                              separator == 0x1e)) {
                            suffix = false;
                            break;
                        }
                    }
                    if (suffix) {
                        d.text.length = paragraph_end - d.text_source.byte_start;
                        d.text_source.byte_end = paragraph_end;
                    }
                }
                if (d.text_source.paragraph.data && d.text_source.byte_end <= paragraph_end) {
                    d.text_source.paragraph.data += paragraph_start;
                    d.text_source.paragraph.length = paragraph_end - paragraph_start;
                    d.text_source.byte_start -= paragraph_start;
                    d.text_source.byte_end -= paragraph_start;
                } else
                    d.text_source = (cui_text_line_source){0};
                break;
            }
            d.font_size = c->renderData.text.fontSize;
            if (c->renderData.text.fontId && c->renderData.text.fontId <= ui->text_style_count) {
                cui_core_text_style style = ui->text_styles[c->renderData.text.fontId - 1];
                d.font_size = style.size;
                d.text_style = style.style;
                if (style.line_height > 0) {
                    float natural = cui_measure_text_ex(ui, d.text, style.size, style.style).height;
                    d.bounds.y += (style.line_height - natural) * .5f;
                }
            }
            d.color = cui_core_color(c->renderData.text.textColor);
            break;
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
            d.bounds = cui_core_ancestral_clip(ui, c, d.bounds);
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
        if (d.type == CUI_DRAW_IMAGE)
            d.bitmap.opacity *= opacity;
        else if (d.type == CUI_DRAW_RECT || d.type == CUI_DRAW_BORDER || d.type == CUI_DRAW_TEXT)
            d.color.a = (uint8_t)(d.color.a * opacity);
        ui->commands[ui->count++] = d;
    }
    if (!cui_core_prepaint_defer(ui, INT32_MAX))
        goto cui_core_commands_done;
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
    /* Behavior may register before its own layout box. Bind to the actual
     * painted node so clipping, layers and hit order do not depend on API order. */
    size_t visible = 0;
    for (size_t i = 0; i < ui->semantic_count; i++) {
        uint32_t folded = cui_core_folded(ui->semantics[i].id);
        Clay_LayoutElementHashMapItem *item = Clay__GetHashMapItem(folded);
        size_t node = ui->semantic_nodes[i];
        if (item != &Clay_LayoutElementHashMapItem_DEFAULT && item->layoutElement) {
            void *data = item->layoutElement->config.userData;
            if (data && data != ui)
                node = (size_t)((cui_core_box_node *)data - ui->nodes);
            else if (data == ui)
                for (size_t n = 0; n < ui->node_count; n++)
                    if (ui->nodes[n].clay_id == folded) {
                        node = n;
                        break;
                    }
        }
        if (node != SIZE_MAX && (ui->nodes[node].inert || ui->nodes[node].layout_only))
            continue;
        ui->semantics[visible] = ui->semantics[i];
        ui->semantic_scopes[visible] = ui->semantic_scopes[i];
        ui->semantic_nodes[visible++] = node;
    }
    ui->semantic_count = visible;
    /* Withdraw targeted actions whose widget disappeared or lost eligibility in
     * this frame. They never survive into a later frame or reused widget ID. */
    for (size_t e = ui->event_count; e-- > 0;) {
        if (ui->events[e].type != CUI_EVENT_ACCESSIBILITY_ACTION)
            continue;
        bool allowed = false;
        for (size_t n = 0; n < ui->semantic_count; n++)
            if (ui->semantics[n].id == ui->events[e].accessibility_target &&
                cui_core_scope_contains(ui->focus_scopes, ui->semantic_scopes[n], ui->top_modal))
                for (size_t p = 0; p < ui->previous_count; p++)
                    if (ui->previous[p].id == ui->semantics[n].id &&
                        ui->previous[p].role == ui->semantics[n].role)
                        allowed = ui->events[e].accessibility_action == CUI_ACCESSIBILITY_FOCUS
                                      ? cui_core_focusable(&ui->semantics[n])
                                      : cui_core_accessibility_activate(&ui->semantics[n]);
        if (!allowed) {
            if (ui->events[e].accessibility_action == CUI_ACCESSIBILITY_FOCUS &&
                ui->focus == ui->events[e].accessibility_target)
                ui->focus = ui->event_previous_focus[e];
            size_t slot = ui->event_activation_slots[e];
            if (slot < ui->activation_count)
                ui->activations[slot] = 0;
            ui->event_consumed[e] = true;
            ui->event_targets[e] = 0;
            ui->event_activations[e] = 0;
        }
    }
    bool retained = false, captured = false, space = false;
    for (size_t i = 0; i < ui->semantic_count; i++) {
        cui_semantic *s = &ui->semantics[i];
        Clay_ElementData d = Clay_GetElementData((Clay_ElementId){.id = cui_core_folded(s->id)});
        s->bounds =
            (cui_rect){d.boundingBox.x, d.boundingBox.y, d.boundingBox.width, d.boundingBox.height};
        ui->previous_parents[i] = SIZE_MAX;
        size_t ancestor = ui->semantic_nodes[i];
        if (ancestor != SIZE_MAX)
            ancestor = ui->nodes[ancestor].parent;
        for (; ancestor != SIZE_MAX; ancestor = ui->nodes[ancestor].parent) {
            for (size_t j = 0; j < ui->semantic_count; ++j)
                if (ui->semantic_nodes[j] == ancestor &&
                    cui_core_folded(ui->semantics[j].id) == ui->nodes[ancestor].clay_id) {
                    ui->previous_parents[i] = j;
                    break;
                }
            if (ui->previous_parents[i] != SIZE_MAX)
                break;
        }
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
        size_t node = ui->semantic_nodes[i];
        ui->previous_order[i] =
            node == SIZE_MAX ? 0
                             : ((uint64_t)ui->nodes[node].paint_root << 32) | (uint64_t)(node + 1);
        if (cui_core_focusable(s)) {
            retained |= s->id == ui->focus;
            if (cui_core_scope_contains(ui->focus_scopes, ui->semantic_scopes[i], ui->top_modal)) {
                space |= s->id == ui->space_capture;
            }
        }
        if (!s->disabled && (cui_core_focusable(s) || s->details.pointer_activatable) &&
            cui_core_scope_contains(ui->focus_scopes, ui->semantic_scopes[i], ui->top_modal))
            captured |= s->id == ui->capture;
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
    for (size_t n = 0; n < ui->semantic_count; n++) {
        ui->scroll_gestures[n] = (cui_scroll_gesture){0};
        if (ui->semantics[n].details.scroll_gesture_lock && !ui->semantics[n].disabled)
            for (size_t p = 0; p < ui->previous_count; p++)
                if (ui->previous[p].id == ui->semantics[n].id) {
                    ui->scroll_gestures[n] = ui->previous_scroll_gestures[p];
                    break;
                }
    }
    memcpy(ui->previous_scroll_gestures, ui->scroll_gestures,
           ui->semantic_count * sizeof(*ui->scroll_gestures));
    memcpy(ui->previous, ui->semantics, ui->semantic_count * sizeof(*ui->previous));
    ui->previous_count = ui->semantic_count;
    if (ui->composition_owner != ui->focus)
        ui->composition_owner = 0;
    ui->in_frame = false;
    return (cui_draw_list){ui->commands, ui->count, ui->config.width, ui->config.height};
}
const cui_semantic *cui_semantics(cui_context *ui, size_t *count) {
    if (count)
        *count = ui ? ui->semantic_count : 0;
    return ui ? ui->semantics : NULL;
}
cui_id cui_semantic_parent(cui_context *ui, cui_id id) {
    if (!ui || !id || ui->in_frame)
        return 0;
    for (size_t i = 0; i < ui->semantic_count; i++)
        if (ui->semantics[i].id == id) {
            if (ui->semantics[i].details.parent)
                return ui->semantics[i].details.parent;
            size_t parent = ui->previous_parents[i];
            return parent < ui->semantic_count ? ui->semantics[parent].id : 0;
        }
    return 0;
}
cui_id cui_focused(cui_context *ui) {
    return ui && !cui_core_inert(ui) ? ui->focus : 0;
}
void cui_focus(cui_context *ui, cui_id id) {
    if (!cui_core_mutation(ui))
        return;
    if (ui && !cui_core_inert(ui)) {
        if (id) {
            const cui_semantic *items = ui->in_frame ? ui->semantics : ui->previous;
            size_t count = ui->in_frame ? ui->semantic_count : ui->previous_count;
            bool found = false;
            for (size_t i = 0; i < count; ++i)
                if (items[i].id == id) {
                    if (items[i].details.no_focus)
                        return;
                    found = true;
                    break;
                }
            if (ui->in_frame && !found)
                for (size_t i = 0; i < ui->previous_count; ++i)
                    if (ui->previous[i].id == id && ui->previous[i].details.no_focus)
                        return;
        }
        if (ui->focus != id)
            ui->composition_owner = 0;
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
    if (!cui_core_mutation(ui))
        return;
    if (ui)
        ui->error = CUI_OK;
}
bool cui_needs_redraw(cui_context *ui) {
    return ui && ui->dirty;
}
void cui_invalidate(cui_context *ui) {
    if (!cui_core_mutation(ui))
        return;
    if (ui)
        ui->dirty = true;
}

void *cui_resource_find(cui_context *ui, cui_id owner, cui_id namespace_id) {
    if (!ui || !ui->in_frame || !owner || !namespace_id)
        return NULL;
    for (size_t i = 0; i < ui->config.max_resources; i++) {
        cui_core_resource *resource = &ui->resources[i];
        if (resource->payload && resource->owner == owner &&
            resource->namespace_id == namespace_id) {
            resource->touched = true;
            return resource->payload;
        }
    }
    return NULL;
}
void *cui_resource_get(cui_context *ui, cui_id owner, cui_id namespace_id, size_t bytes,
                       cui_resource_fn collect, cui_resource_fn destroy) {
    if (!cui_core_mutation(ui))
        return NULL;
    if (!ui || !ui->in_frame || !owner || !namespace_id || !bytes) {
        if (ui)
            cui_core_fail(ui, CUI_ERROR_LIFECYCLE);
        return NULL;
    }
    size_t free_slot = SIZE_MAX;
    for (size_t i = 0; i < ui->config.max_resources; ++i) {
        cui_core_resource *r = &ui->resources[i];
        if (!r->payload) {
            if (free_slot == SIZE_MAX)
                free_slot = i;
        } else if (r->owner == owner && r->namespace_id == namespace_id) {
            if (r->bytes != bytes || r->collect != collect || r->destroy != destroy) {
                cui_core_fail(ui, CUI_ERROR_LIFECYCLE);
                return NULL;
            }
            r->touched = true;
            return r->payload;
        }
    }
    if (free_slot == SIZE_MAX || bytes > ui->config.resource_bytes - ui->resource_bytes) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return NULL;
    }
    void *payload = calloc(1, bytes);
    if (!payload) {
        cui_core_fail(ui, CUI_ERROR_CAPACITY);
        return NULL;
    }
    ui->resources[free_slot] =
        (cui_core_resource){owner, namespace_id, payload, bytes, collect, destroy, true};
    ui->resource_bytes += bytes;
    return payload;
}
float cui_frame_delta(cui_context *ui) {
    return ui ? ui->dt : 0;
}
double cui_frame_time(cui_context *ui) {
    return ui ? ui->frame_time : 0;
}
bool cui_reduced_motion(cui_context *ui) {
    return ui && ui->reduced_motion;
}
void cui_set_reduced_motion(cui_context *ui, bool reduced) {
    if (!cui_core_mutation(ui))
        return;
    if (ui && ui->reduced_motion != reduced) {
        ui->reduced_motion = reduced;
        cui_invalidate(ui);
    }
}

void cui_update_semantic(cui_context *ui, cui_id id, bool checked, float value) {
    if (!cui_core_mutation(ui))
        return;
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

bool cui_defer_scroll_event(cui_context *ui, size_t index, cui_id from) {
    if (!cui_core_mutation(ui))
        return false;
    if (!ui || !ui->in_frame || cui_core_inert(ui) || !from || index >= ui->event_count ||
        ui->event_consumed[index] || ui->events[index].type != CUI_EVENT_SCROLL ||
        ui->event_targets[index] != from)
        return false;
    for (size_t i = 0; i < ui->previous_count; ++i)
        if (ui->previous[i].id == from && cui_core_previous_allowed(ui, i)) {
            cui_id target =
                cui_core_scroll_hit(ui, ui->events[index], i, false, ui->event_touch_owners[index]);
            if (!target)
                return false;
            ui->event_targets[index] = target;
            return true;
        }
    return false;
}

cui_id cui_event_activation(cui_context *ui, size_t index) {
    return ui && !cui_core_inert(ui) && index < ui->event_count ? ui->event_activations[index] : 0;
}

bool cui_event_consumed(cui_context *ui, size_t index) {
    return ui && index < ui->event_count && ui->event_consumed[index];
}
bool cui_consume_event(cui_context *ui, size_t index) {
    if (!cui_core_mutation(ui))
        return false;
    if (!ui || !ui->in_frame || cui_core_inert(ui) || index >= ui->event_count)
        return false;
    ui->event_consumed[index] = true;
    if (ui->events[index].type == CUI_EVENT_ACCESSIBILITY_ACTION) {
        (void)cui_cancel_event_activation(ui, index);
        ui->event_activations[index] = 0;
    }
    return true;
}
bool cui_cancel_event_activation(cui_context *ui, size_t index) {
    if (!cui_core_mutation(ui))
        return false;
    if (!ui || !ui->in_frame || cui_core_inert(ui) || index >= ui->event_count)
        return false;
    size_t slot = ui->event_activation_slots[index];
    if (slot >= ui->activation_count || !ui->activations[slot])
        return false;
    ui->activations[slot] = 0;
    ui->event_activations[index] = 0;
    return true;
}
static bool cui_core_event_target_in_scope(cui_context *ui, size_t index, cui_id scope,
                                           cui_id target) {
    if (!ui || !ui->in_frame || cui_core_inert(ui) || !scope || index >= ui->event_count)
        return false;
    bool active = false;
    for (size_t i = 0; i < ui->focus_scope_count; ++i)
        if (ui->focus_scopes[i].id == scope && ui->focus_scopes[i].active)
            active = true;
    if (!active || !cui_pointer_scope_allowed(ui, scope))
        return false;
    for (size_t i = 0; target && i < ui->previous_count; ++i) {
        if (ui->previous[i].id != target || !cui_core_previous_allowed(ui, i))
            continue;
        if (target == scope)
            return true;
        size_t parent = ui->previous_semantic_scopes[i];
        while (parent != SIZE_MAX) {
            if (!ui->previous_focus_scopes[parent].active)
                return false;
            if (ui->previous_focus_scopes[parent].id == scope)
                return true;
            parent = ui->previous_focus_scopes[parent].parent;
        }
    }
    return false;
}

bool cui_event_within_scope(cui_context *ui, size_t index, cui_id scope) {
    return ui && index < ui->event_count &&
           cui_core_event_target_in_scope(ui, index, scope, ui->event_targets[index]);
}
bool cui_event_hit_within_scope(cui_context *ui, size_t index, cui_id scope) {
    if (!ui || !ui->in_frame || index >= ui->event_count || cui_core_inert(ui))
        return false;
    cui_event e = ui->events[index];
    if (e.type != CUI_EVENT_SCROLL && e.type != CUI_EVENT_POINTER_DOWN &&
        e.type != CUI_EVENT_POINTER_MOVE && e.type != CUI_EVENT_POINTER_UP)
        return false;
    size_t hit = SIZE_MAX;
    for (size_t i = 0; i < ui->previous_count; ++i) {
        const cui_semantic *s = &ui->previous[i];
        if (!s->disabled &&
            (cui_core_focusable(s) || s->details.pointer_activatable || s->details.scrollable) &&
            cui_core_previous_allowed(ui, i) && cui_core_inside(ui->previous_hits[i], e.x, e.y) &&
            cui_core_above(ui, i, hit))
            hit = i;
    }
    return hit != SIZE_MAX &&
           cui_core_event_target_in_scope(ui, index, scope, ui->previous[hit].id);
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
bool cui_focus_step_within(cui_context *ui, cui_id scope, bool forward) {
    if (!cui_core_mutation(ui))
        return false;
    if (!ui || !ui->in_frame || !scope || !ui->focus || cui_core_inert(ui))
        return false;
    size_t owner = SIZE_MAX, start = SIZE_MAX;
    for (size_t i = 0; i < ui->focus_scope_count; ++i)
        if (ui->focus_scopes[i].id == scope && ui->focus_scopes[i].active)
            owner = i;
    if (owner == SIZE_MAX)
        return false;
    for (size_t i = 0; i < ui->semantic_count; ++i)
        if (ui->semantics[i].id == ui->focus &&
            cui_core_scope_contains(ui->focus_scopes, ui->semantic_scopes[i], owner))
            start = i;
    if (start == SIZE_MAX)
        return false;
    for (size_t step = 1; step <= ui->semantic_count; ++step) {
        size_t i = forward ? (start + step) % ui->semantic_count
                           : (start + ui->semantic_count - step) % ui->semantic_count;
        cui_semantic *candidate = &ui->semantics[i];
        if (!cui_core_focusable(candidate) || candidate->tab_index < 0 ||
            !cui_core_scope_contains(ui->focus_scopes, ui->semantic_scopes[i], owner) ||
            !cui_core_scope_contains(ui->focus_scopes, ui->semantic_scopes[i], ui->top_modal))
            continue;
        bool active = true;
        for (size_t n = ui->semantic_scopes[i]; n != SIZE_MAX; n = ui->focus_scopes[n].parent)
            if (!ui->focus_scopes[n].active)
                active = false;
        /* A control may submit behavior before its own inert paint box. */
        uint32_t folded = cui_core_folded(candidate->id);
        for (size_t n = 0; n < ui->node_count; ++n)
            if (ui->nodes[n].clay_id == folded && (ui->nodes[n].inert || ui->nodes[n].layout_only))
                active = false;
        if (!active)
            continue;
        cui_focus(ui, candidate->id);
        return true;
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
    for (size_t i = 0; i < ui->previous_count; i++)
        if (cui_core_previous_allowed(ui, i) && cui_core_above(ui, i, hit) &&
            cui_core_inside(ui->previous_hits[i], ui->pointer_x, ui->pointer_y)) {
            hit = i;
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
    if (!cui_core_mutation(ui))
        return false;
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
    if (!cui_core_mutation(ui))
        return false;
    if (!cui_core_active(ui) || !id || cui_core_inert(ui) || role < CUI_ROLE_NONE ||
        role > CUI_ROLE_RADIOGROUP)
        return false;
    for (size_t i = 0; i < ui->semantic_count; i++)
        if (ui->semantics[i].id == id) {
            ui->semantics[i].role = role;
            return true;
        }
    return false;
}
bool cui_set_cursor(cui_context *ui, cui_id id, cui_cursor cursor) {
    if (!cui_core_mutation(ui))
        return false;
    if (!cui_core_active(ui) || cursor < CUI_CURSOR_DEFAULT || cursor > CUI_CURSOR_HELP)
        return false;
    for (size_t i = 0; i < ui->semantic_count; i++)
        if (ui->semantics[i].id == id) {
            ui->semantics[i].details.cursor = cursor;
            return true;
        }
    return false;
}
bool cui_set_touch_drag_priority(cui_context *ui, cui_id id, bool enabled) {
    if (!cui_core_mutation(ui))
        return false;
    if (!cui_core_active(ui))
        return false;
    for (size_t i = 0; i < ui->semantic_count; i++)
        if (ui->semantics[i].id == id) {
            ui->semantics[i].details.touch_drag_priority = enabled;
            return true;
        }
    return false;
}
bool cui_set_gestures(cui_context *ui, cui_id id, bool pan, bool zoom) {
    if (!cui_core_mutation(ui))
        return false;
    if (!cui_core_active(ui))
        return false;
    for (size_t i = 0; i < ui->semantic_count; i++)
        if (ui->semantics[i].id == id) {
            ui->semantics[i].details.gesture_pan = pan;
            ui->semantics[i].details.gesture_zoom = zoom;
            return true;
        }
    return false;
}
cui_interaction cui_interaction_state(const cui_context *ui, cui_id id) {
    cui_interaction result = {0};
    if (!ui || !id)
        return result;
    const cui_semantic *items = ui->in_frame ? ui->semantics : ui->previous;
    size_t count = ui->in_frame ? ui->semantic_count : ui->previous_count;
    for (size_t n = 0; n < count; n++)
        if (items[n].id == id && !items[n].disabled) {
            result.hovered = cui_hovered((cui_context *)ui, id);
            result.pressed = ui->capture == id || ui->space_capture == id;
            result.focused = ui->focus == id;
            result.focus_visible = result.focused && ui->focus_visible;
            for (size_t i = 0; i < ui->event_count; i++)
                if (ui->event_activations[i] == id)
                    result.activated = true;
            for (size_t i = 0; i < ui->activation_count; i++)
                if (ui->activations[i] == id)
                    result.activated = true;
            break;
        }
    return result;
}
void cui_set_semantic_details(cui_context *ui, cui_id id, cui_semantic_details details) {
    if (!cui_core_mutation(ui))
        return;
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
    if (!cui_core_mutation(ui))
        return;
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
                if ((ui->previous_layers[j] > ui->previous_layers[i] ||
                     (ui->previous_layers[j] == ui->previous_layers[i] &&
                      (ui->previous_order[j] >> 32) > (ui->previous_order[i] >> 32))) &&
                    cui_core_previous_allowed(ui, j) && cui_core_focusable(&ui->previous[j]) &&
                    cui_core_inside(ui->previous_hits[j], ui->pointer_x, ui->pointer_y))
                    return false;
            return true;
        }
    return false;
}

bool cui_dismiss_focus_scope(cui_context *ui, cui_id scope, cui_id trigger) {
    if (!cui_core_mutation(ui))
        return false;
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

cui_key cui_key_from_character(char character) {
    static const cui_key letters[] = {
        CUI_KEY_A, CUI_KEY_B, CUI_KEY_C, CUI_KEY_D, CUI_KEY_E, CUI_KEY_F, CUI_KEY_G,
        CUI_KEY_H, CUI_KEY_I, CUI_KEY_J, CUI_KEY_K, CUI_KEY_L, CUI_KEY_M, CUI_KEY_N,
        CUI_KEY_O, CUI_KEY_P, CUI_KEY_Q, CUI_KEY_R, CUI_KEY_S, CUI_KEY_T, CUI_KEY_U,
        CUI_KEY_V, CUI_KEY_W, CUI_KEY_X, CUI_KEY_Y, CUI_KEY_Z};
    if (character >= 'a' && character <= 'z')
        character = (char)(character - 'a' + 'A');
    if (character >= 'A' && character <= 'Z')
        return letters[character - 'A'];
    if (character >= '0' && character <= '9')
        return (cui_key)(CUI_KEY_0 + character - '0');
    if (character == '[')
        return CUI_KEY_LEFT_BRACKET;
    if (character == ']')
        return CUI_KEY_RIGHT_BRACKET;
    return CUI_KEY_NONE;
}
char cui_key_character(cui_key key) {
    if (key == CUI_KEY_LEFT_BRACKET)
        return '[';
    if (key == CUI_KEY_RIGHT_BRACKET)
        return ']';
    if (key >= CUI_KEY_0 && key <= CUI_KEY_9)
        return (char)('0' + key - CUI_KEY_0);
    for (char c = 'A'; c <= 'Z'; c++)
        if (cui_key_from_character(c) == key)
            return c;
    return 0;
}

bool cui_scroll_gesture_filter(cui_scroll_gesture *state, cui_event *event, double now) {
    if (!state || !event || event->type != CUI_EVENT_SCROLL || !isfinite(now) ||
        !isfinite(event->dx) || !isfinite(event->dy) ||
        event->scroll_phase < CUI_SCROLL_PHASE_NONE ||
        event->scroll_phase > CUI_SCROLL_PHASE_CANCELLED)
        return false;
    if (!event->scroll_precise || event->scroll_phase == CUI_SCROLL_PHASE_ENDED ||
        event->scroll_phase == CUI_SCROLL_PHASE_CANCELLED) {
        *state = (cui_scroll_gesture){0};
        return true;
    }
    float x = fabsf(event->dx), y = fabsf(event->dy);
    if (x == 0 && y == 0) {
        if (event->scroll_phase == CUI_SCROLL_PHASE_STARTED)
            *state = (cui_scroll_gesture){0};
        return true;
    }
    bool fresh = event->scroll_phase == CUI_SCROLL_PHASE_STARTED || !state->has_last ||
                 now < state->last_time || now - state->last_time >= .028;
    if (fresh) {
        state->locked = true;
        state->horizontal = x > y;
    } else if (fmaxf(x, y) >= 6 &&
               (state->horizontal ? y > x && y >= x * 1.9f : x > y && x >= y * 1.9f)) {
        state->locked = false;
    }
    state->last_time = now;
    state->has_last = true;
    if (state->locked) {
        if (state->horizontal)
            event->dy = 0;
        else
            event->dx = 0;
    }
    return true;
}

bool cui_pointer_position(const cui_context *ui, float *x, float *y) {
    if (!ui || !x || !y || x == y || !isfinite(ui->pointer_x) || !isfinite(ui->pointer_y))
        return false;
    *x = ui->pointer_x;
    *y = ui->pointer_y;
    return true;
}

void cui_set_text_input_rect(cui_context *ui, cui_id owner, cui_rect bounds) {
    if (!cui_core_mutation(ui))
        return;
    if (!ui || !owner || !isfinite(bounds.x) || !isfinite(bounds.y) || !isfinite(bounds.w) ||
        !isfinite(bounds.h) || bounds.w < 0 || bounds.h < 0)
        return;
    ui->text_input_owner = owner;
    ui->text_input_bounds = bounds;
    ui->text_geometry_ime_owner = 0;
}
bool cui_set_text_input_geometry_rect(cui_context *ui, cui_id owner, cui_id caret, float height) {
    if (!cui_core_mutation(ui))
        return false;
    if (!ui || !ui->in_frame || !owner || !caret || !isfinite(height) || height < 0 ||
        !cui_track_bounds(ui, caret, &ui->text_geometry_ime_bounds))
        return false;
    ui->text_input_owner = owner;
    ui->text_geometry_ime_owner = owner;
    ui->text_geometry_ime_height = height;
    return true;
}
bool cui_text_input_rect(const cui_context *ui, cui_rect *out) {
    if (!ui || !out || !ui->focus || ui->focus != ui->text_input_owner)
        return false;
    if (ui->text_geometry_ime_owner == ui->focus && !ui->in_frame) {
        cui_rect bounds = ui->text_geometry_ime_bounds;
        cui_rect result = {bounds.x, bounds.y - (ui->text_geometry_ime_height - bounds.h) * .5f, 1,
                           ui->text_geometry_ime_height};
        if (!isfinite(result.x) || !isfinite(result.y) || !isfinite(result.h))
            return false;
        *out = result;
    } else
        *out = ui->text_input_bounds;
    return true;
}
