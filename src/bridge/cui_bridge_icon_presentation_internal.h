/* SPDX-License-Identifier: MIT */
/* Shared retained Icon options and standalone Icon node presentation. */
#include "../base/cui_icon_layout.h"

_Static_assert(sizeof(cui_bridge_icon_style_v1) == 128 &&
                   offsetof(cui_bridge_icon_style_v1, corners) == 64 &&
                   offsetof(cui_bridge_icon_style_v1, padding_edges) == 108,
               "Icon style V1 ABI");
_Static_assert(sizeof(cui_bridge_icon_constraints_v1) == 32, "Icon constraints V1 ABI");
_Static_assert(sizeof(cui_bridge_icon_options_v1) == 256 &&
                   offsetof(cui_bridge_icon_options_v1, transform) == 64 &&
                   offsetof(cui_bridge_icon_options_v1, style) == 88 &&
                   offsetof(cui_bridge_icon_options_v1, constraints) == 216,
               "Icon options V1 ABI");
_Static_assert(sizeof(cui_bridge_icon_result_v1) == 40 &&
                   offsetof(cui_bridge_icon_result_v1, frame_sequence) == 24,
               "Icon result V1 ABI");

typedef struct {
    cui_icon_options options;
    cui_box style;
    cui_icon_constraints constraints;
} bridge_icon_decoded;

struct bridge_icon_presented {
    cui_bridge_icon_options_v1 config;
    bridge_icon_decoded decoded;
    cui_bridge_icon_handle_v1 handle;
    bridge_icon_owned *resource;
    uint64_t frame;
    uint32_t error, name_bytes;
    char name[];
};

static cui_color bridge_icon_rgba(uint32_t c) {
    return (cui_color){(uint8_t)(c >> 24), (uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c};
}

static bool bridge_icon_zero(const void *record, size_t size) {
    const unsigned char *bytes = record;
    for (size_t i = 0; i < size; ++i)
        if (bytes[i])
            return false;
    return true;
}

static bool bridge_icon_box_decode(const cui_bridge_icon_style_v1 *s, cui_box *box) {
    if (!bridge_icon_header(s, sizeof *s) || s->reserved || s->reserved_tail ||
        (s->flags & ~UINT32_C(4095)) || s->align_x > CUI_ALIGN_END || s->align_y > CUI_ALIGN_END ||
        !isfinite(s->width) || !isfinite(s->height) || !isfinite(s->opacity) ||
        !isfinite(s->offset_x) || !isfinite(s->offset_y))
        return false;
    const float sizes[] = {s->padding,      s->gap,       s->radius,
                           s->border_width, s->max_width, s->max_height};
    for (size_t i = 0; i < sizeof sizes / sizeof sizes[0]; ++i)
        if (!isfinite(sizes[i]) || sizes[i] < 0)
            return false;
    for (size_t i = 0; i < 4; ++i)
        if (!isfinite(s->corners[i]) || s->corners[i] < 0 || !isfinite(s->border_edges[i]) ||
            s->border_edges[i] < 0 || !isfinite(s->padding_edges[i]) || s->padding_edges[i] < 0)
            return false;
    *box = (cui_box){
        .width = s->width,
        .height = s->height,
        .padding = s->padding,
        .gap = s->gap,
        .radius = s->radius,
        .row = (s->flags & CUI_BRIDGE_ICON_BOX_ROW_V1) != 0,
        .clip = (s->flags & CUI_BRIDGE_ICON_BOX_CLIP_V1) != 0,
        .background = bridge_icon_rgba(s->background_rgba),
        .border = bridge_icon_rgba(s->border_rgba),
        .border_width = s->border_width,
        .align_x = (cui_align)s->align_x,
        .align_y = (cui_align)s->align_y,
        .offset_x = s->offset_x,
        .offset_y = s->offset_y,
        .corner_radii = {s->corners[0], s->corners[1], s->corners[2], s->corners[3]},
        .border_edges = {s->border_edges[0], s->border_edges[1], s->border_edges[2],
                         s->border_edges[3]},
        .custom_corners = (s->flags & CUI_BRIDGE_ICON_BOX_CORNERS_V1) != 0,
        .custom_border_edges = (s->flags & CUI_BRIDGE_ICON_BOX_BORDER_EDGES_V1) != 0,
        .fixed_width = (s->flags & CUI_BRIDGE_ICON_BOX_FIXED_WIDTH_V1) != 0,
        .fixed_height = (s->flags & CUI_BRIDGE_ICON_BOX_FIXED_HEIGHT_V1) != 0,
        .layout_only = (s->flags & CUI_BRIDGE_ICON_BOX_LAYOUT_ONLY_V1) != 0,
        .inert = (s->flags & CUI_BRIDGE_ICON_BOX_INERT_V1) != 0,
        .max_width = s->max_width,
        .max_height = s->max_height,
        .opacity = s->opacity,
        .custom_opacity = (s->flags & CUI_BRIDGE_ICON_BOX_OPACITY_V1) != 0,
        .width_percent = (s->flags & CUI_BRIDGE_ICON_BOX_WIDTH_PERCENT_V1) != 0,
        .height_percent = (s->flags & CUI_BRIDGE_ICON_BOX_HEIGHT_PERCENT_V1) != 0,
        .padding_edges = {s->padding_edges[0], s->padding_edges[1], s->padding_edges[2],
                          s->padding_edges[3]},
        .custom_padding = (s->flags & CUI_BRIDGE_ICON_BOX_PADDING_EDGES_V1) != 0,
    };
    return true;
}

static bool bridge_icon_constraints_decode(const cui_bridge_icon_constraints_v1 *c,
                                           cui_icon_constraints *out) {
    if (!bridge_icon_header(c, sizeof *c) || c->reserved || (c->flags & ~UINT32_C(15)))
        return false;
    const float dims[] = {c->min_width, c->min_height, c->max_width, c->max_height};
    for (size_t i = 0; i < sizeof dims / sizeof dims[0]; ++i)
        if ((c->flags & (1u << i)) ? !cui_icon_extent_valid(dims[i]) : dims[i] != 0)
            return false;
    *out = (cui_icon_constraints){
        .min_width = c->min_width,
        .min_height = c->min_height,
        .max_width = c->max_width,
        .max_height = c->max_height,
        .custom_min_width = (c->flags & CUI_BRIDGE_ICON_MIN_WIDTH_V1) != 0,
        .custom_min_height = (c->flags & CUI_BRIDGE_ICON_MIN_HEIGHT_V1) != 0,
        .custom_max_width = (c->flags & CUI_BRIDGE_ICON_MAX_WIDTH_V1) != 0,
        .custom_max_height = (c->flags & CUI_BRIDGE_ICON_MAX_HEIGHT_V1) != 0,
    };
    return true;
}

static void bridge_icon_native_options(const cui_bridge_icon_options_v1 *config, cui_str name,
                                       bridge_icon_decoded *out) {
    uint32_t f = config->flags;
    out->options = (cui_icon_options){
        .size = (cui_icon_size)config->size,
        .custom_size = config->custom_size,
        .inherited_text_size = config->inherited_text_size,
        .width = config->width,
        .height = config->height,
        .custom_width = (f & CUI_BRIDGE_ICON_WIDTH_V1) != 0,
        .custom_height = (f & CUI_BRIDGE_ICON_HEIGHT_V1) != 0,
        .color = bridge_icon_rgba(config->color_rgba),
        .inherited_color = bridge_icon_rgba(config->inherited_color_rgba),
        .custom_color = (f & CUI_BRIDGE_ICON_COLOR_V1) != 0,
        .custom_inherited_color = (f & CUI_BRIDGE_ICON_INHERITED_COLOR_V1) != 0,
        .opacity = config->opacity,
        .custom_opacity = (f & CUI_BRIDGE_ICON_OPACITY_V1) != 0,
        .transform = {config->transform[0], config->transform[1], config->transform[2],
                      config->transform[3], config->transform[4], config->transform[5]},
        .custom_transform = (f & CUI_BRIDGE_ICON_TRANSFORM_V1) != 0,
        .design_size = (cui_design_size)config->design_size,
        .rem_size = config->rem_size,
        .accessible_name = name,
        .decorative = (f & CUI_BRIDGE_ICON_DECORATIVE_V1) != 0,
        .style = (f & CUI_BRIDGE_ICON_STYLE_V1) ? &out->style : NULL,
        .debug_placeholder = (f & CUI_BRIDGE_ICON_DEBUG_PLACEHOLDER_V1) != 0,
        .constraints = (f & CUI_BRIDGE_ICON_CONSTRAINTS_V1) ? &out->constraints : NULL,
    };
}

static bool bridge_icon_options_decode(const cui_bridge_icon_options_v1 *c, cui_str name,
                                       bridge_icon_decoded *out) {
    if (!bridge_icon_header(c, sizeof *c) || c->size > CUI_ICON_CUSTOM ||
        c->design_size > CUI_DESIGN_9XL || (c->flags & ~UINT32_C(1023)) ||
        !bridge_icon_zero(c->reserved, sizeof c->reserved) ||
        !bridge_icon_zero(c->reserved_tail, sizeof c->reserved_tail) ||
        !cui_icon_extent_valid(c->custom_size) || !cui_icon_extent_valid(c->inherited_text_size) ||
        !cui_icon_extent_valid(c->rem_size) ||
        ((c->flags & CUI_BRIDGE_ICON_WIDTH_V1) ? !cui_icon_extent_valid(c->width)
                                               : c->width != 0) ||
        ((c->flags & CUI_BRIDGE_ICON_HEIGHT_V1) ? !cui_icon_extent_valid(c->height)
                                                : c->height != 0) ||
        ((c->flags & CUI_BRIDGE_ICON_OPACITY_V1) ? !isfinite(c->opacity) : c->opacity != 0) ||
        (!(c->flags & CUI_BRIDGE_ICON_COLOR_V1) && c->color_rgba) ||
        (!(c->flags & CUI_BRIDGE_ICON_INHERITED_COLOR_V1) && c->inherited_color_rgba))
        return false;
    for (size_t i = 0; i < sizeof c->transform / sizeof c->transform[0]; ++i)
        if ((c->flags & CUI_BRIDGE_ICON_TRANSFORM_V1)
                ? !isfinite(c->transform[i]) || fabsf(c->transform[i]) > (i < 4 ? 64 : 1000000)
                : c->transform[i] != 0)
            return false;
    if ((c->flags & CUI_BRIDGE_ICON_STYLE_V1) ? !bridge_icon_box_decode(&c->style, &out->style)
                                              : !bridge_icon_zero(&c->style, sizeof c->style))
        return false;
    if ((c->flags & CUI_BRIDGE_ICON_CONSTRAINTS_V1)
            ? !bridge_icon_constraints_decode(&c->constraints, &out->constraints)
            : !bridge_icon_zero(&c->constraints, sizeof c->constraints))
        return false;
    bridge_icon_native_options(c, name, out);
    float w = 0, h = 0;
    return cui_icon_layout_resolve(out->options, &w, &h);
}

static bridge_icon_presented *bridge_icon_node(cui_bridge_app *app, uint32_t slot) {
    return app && app->icons && app->icons->presentations ? app->icons->presentations[slot] : NULL;
}

static void bridge_icon_presentation_remove(cui_bridge_app *app, uint32_t slot) {
    bridge_icon_presented *value = bridge_icon_node(app, slot);
    if (value) {
        value->resource->references--;
        free(value);
        app->icons->presentations[slot] = NULL;
    }
}

static cui_bridge_status bridge_icon_presentation_node(cui_bridge_app *app, cui_bridge_node n) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, n, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    return node->kind == CUI_BRIDGE_ICON ? CUI_BRIDGE_OK : CUI_BRIDGE_INVALID_STATE;
}

uint32_t cui_bridge_icon_presentation_versions(void) {
    return cui_bridge_icon_versions();
}

uint32_t cui_bridge_icon_presentation_record_size_v1(uint32_t kind) {
    switch (kind) {
    case CUI_BRIDGE_ICON_STYLE_RECORD_V1:
        return sizeof(cui_bridge_icon_style_v1);
    case CUI_BRIDGE_ICON_CONSTRAINTS_RECORD_V1:
        return sizeof(cui_bridge_icon_constraints_v1);
    case CUI_BRIDGE_ICON_OPTIONS_RECORD_V1:
        return sizeof(cui_bridge_icon_options_v1);
    case CUI_BRIDGE_ICON_RESULT_RECORD_V1:
        return sizeof(cui_bridge_icon_result_v1);
    default:
        return 0;
    }
}

cui_bridge_status cui_bridge_icon_configure_node_v1(cui_bridge_app *app, cui_bridge_node n,
                                                    cui_bridge_icon_handle_v1 handle,
                                                    const cui_bridge_icon_options_v1 *config,
                                                    const char *name, uint32_t name_bytes) {
    cui_bridge_status status = bridge_icon_presentation_node(app, n);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!BRIDGE_SLIDER_SUPPORTED)
        return CUI_BRIDGE_INVALID_STATE;
    bridge_icon_range ranges[] = {{config, sizeof *config}, {name, name_bytes}};
    bridge_icon_decoded decoded = {0};
    if (!bridge_icon_ranges(ranges, 2) ||
        !bridge_icon_options_decode(config, (cui_str){name, name_bytes}, &decoded))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    status = bridge_icon_bytes(name, name_bytes, 4096);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_icon_owned *resource;
    status = bridge_icon_get(app, handle, &resource);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_icon_presented *next = calloc(1, sizeof *next + name_bytes + 1);
    if (!next)
        return CUI_BRIDGE_OUT_OF_MEMORY;
    if (!app->icons->presentations) {
        size_t count = (size_t)app->max_nodes + 1;
        app->icons->presentations = calloc(count, sizeof *app->icons->presentations);
        if (!app->icons->presentations) {
            free(next);
            return CUI_BRIDGE_OUT_OF_MEMORY;
        }
    }
    next->config = *config;
    next->handle = handle;
    next->resource = resource;
    next->name_bytes = name_bytes;
    if (name_bytes)
        memcpy(next->name, name, name_bytes);
    next->decoded = decoded;
    bridge_icon_native_options(&next->config, (cui_str){next->name, name_bytes}, &next->decoded);
    bridge_icon_presentation_remove(app, n.slot);
    resource->references++;
    app->icons->presentations[n.slot] = next;
    return CUI_BRIDGE_OK;
}

cui_bridge_status cui_bridge_icon_get_node_config_v1(cui_bridge_app *app, cui_bridge_node n,
                                                     cui_bridge_icon_handle_v1 *handle,
                                                     cui_bridge_icon_options_v1 *out) {
    cui_bridge_status status = bridge_icon_presentation_node(app, n);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_icon_range ranges[] = {{out, sizeof *out}, {handle, sizeof *handle}};
    if (!bridge_icon_ranges(ranges, 2) || !bridge_icon_header(out, sizeof *out))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    const bridge_icon_presented *value = bridge_icon_node(app, n.slot);
    if (!value)
        return CUI_BRIDGE_EMPTY;
    *handle = value->handle;
    *out = value->config;
    return CUI_BRIDGE_OK;
}

cui_bridge_status cui_bridge_icon_copy_node_name_v1(cui_bridge_app *app, cui_bridge_node n,
                                                    char *out, uint32_t capacity,
                                                    uint32_t *required) {
    cui_bridge_status status = bridge_icon_presentation_node(app, n);
    if (status != CUI_BRIDGE_OK)
        return status;
    const bridge_icon_presented *value = bridge_icon_node(app, n.slot);
    return value ? bridge_icon_string((cui_str){value->name, value->name_bytes}, out, capacity,
                                      required)
                 : CUI_BRIDGE_EMPTY;
}

cui_bridge_status cui_bridge_icon_reset_node_v1(cui_bridge_app *app, cui_bridge_node n) {
    cui_bridge_status status = bridge_icon_presentation_node(app, n);
    if (status == CUI_BRIDGE_OK)
        bridge_icon_presentation_remove(app, n.slot);
    return status;
}

cui_bridge_status cui_bridge_icon_get_node_result_v1(cui_bridge_app *app, cui_bridge_node n,
                                                     cui_bridge_icon_result_v1 *out) {
    cui_bridge_status status = bridge_icon_presentation_node(app, n);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!bridge_icon_header(out, sizeof *out))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    const bridge_icon_presented *value = bridge_icon_node(app, n.slot);
    if (!value || !value->frame || value->frame != app->icons->presentation_frame)
        return CUI_BRIDGE_EMPTY;
    *out = (cui_bridge_icon_result_v1){
        .struct_size = sizeof *out,
        .schema_version = 1,
        .icon_error = value->error,
        .frame_status = app->icons->presentation_status,
        .node = n,
        .frame_sequence = value->frame,
    };
    return CUI_BRIDGE_OK;
}

static void bridge_icon_presentation_render(cui_bridge_app *app, cui_id id,
                                            bridge_icon_presented *value) {
    value->error =
        cui_icon(app->ui, id, &app->theme, &value->resource->state, value->decoded.options);
    value->frame = app->last_core_frame;
}
