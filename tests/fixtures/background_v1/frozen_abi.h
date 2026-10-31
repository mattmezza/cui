/* SPDX-License-Identifier: MIT */
/* Frozen declarations copied ONCE from immutable 44fb63f5 V1 headers.
 * Test-only ABI mirror; do not regenerate to accept a later public ABI change.
 * Public callback typedefs have independent signature checks in the test TU. */
#ifndef CUI_TEST_FROZEN_V1_ABI_H
#define CUI_TEST_FROZEN_V1_ABI_H
typedef struct {
    const char *data;
    size_t length;
} v1_frozen_cui_str;
typedef struct {
    v1_frozen_cui_str paragraph;
    size_t byte_start, byte_end;
} v1_frozen_cui_text_line_source;
typedef struct {
    float x, y, w, h;
} v1_frozen_cui_rect;
typedef struct {
    uint8_t r, g, b, a;
} v1_frozen_cui_color;
typedef enum {
    V1_FROZEN_CUI_OK,
    V1_FROZEN_CUI_ERROR_CAPACITY,
    V1_FROZEN_CUI_ERROR_DUPLICATE_ID,
    V1_FROZEN_CUI_ERROR_LIFECYCLE,
    V1_FROZEN_CUI_ERROR_LAYOUT,
    V1_FROZEN_CUI_ERROR_INVALID_ACTION
} v1_frozen_cui_error;
typedef enum {
    V1_FROZEN_CUI_ALIGN_START,
    V1_FROZEN_CUI_ALIGN_CENTER,
    V1_FROZEN_CUI_ALIGN_END
} v1_frozen_cui_align;
typedef struct {
    float top_left, top_right, bottom_left, bottom_right;
} v1_frozen_cui_corner_radii;
typedef struct {
    float offset_x, offset_y, sigma, spread;
    v1_frozen_cui_color color;
} v1_frozen_cui_drop_shadow;
typedef struct {
    v1_frozen_cui_rect caster;
    v1_frozen_cui_corner_radii radii;
    v1_frozen_cui_drop_shadow layer;
} v1_frozen_cui_drop_shadow_paint;
typedef struct {
    float left, right, top, bottom;
} v1_frozen_cui_border_edges;
typedef struct {
    float width, height, padding, gap, radius;
    bool row, clip;
    v1_frozen_cui_color background, border;
    float border_width;
    v1_frozen_cui_align align_x, align_y;
    float offset_x, offset_y;
    v1_frozen_cui_corner_radii corner_radii;
    v1_frozen_cui_border_edges border_edges;

    bool custom_corners, custom_border_edges;

    bool fixed_width, fixed_height;

    bool layout_only;

    bool inert;

    float max_width, max_height;

    float opacity;
    bool custom_opacity;

    bool width_percent, height_percent;

    v1_frozen_cui_border_edges padding_edges;
    bool custom_padding;
} v1_frozen_cui_box;
typedef struct {
    const unsigned char *pixels;
    unsigned width, height;
    size_t stride;
    uint64_t revision;
} v1_frozen_cui_bitmap;
typedef struct {
    v1_frozen_cui_bitmap bitmap;
    v1_frozen_cui_rect source;
    float opacity;
    bool grayscale;
    v1_frozen_cui_color tint;
    bool custom_tint;
} v1_frozen_cui_bitmap_paint;
typedef struct {
    float x, y, s, t;
} v1_frozen_cui_mesh_vertex;
typedef struct {
    float r, g, b, a;
} v1_frozen_cui_linear_color;
typedef struct {
    const v1_frozen_cui_mesh_vertex *vertices;
    size_t vertex_count;
    float origin_x, origin_y, scale;
    v1_frozen_cui_linear_color color;
} v1_frozen_cui_mesh_paint;
typedef enum {
    V1_FROZEN_CUI_DRAW_RECT,
    V1_FROZEN_CUI_DRAW_BORDER,
    V1_FROZEN_CUI_DRAW_TEXT,
    V1_FROZEN_CUI_DRAW_CLIP_PUSH,
    V1_FROZEN_CUI_DRAW_CLIP_POP,
    V1_FROZEN_CUI_DRAW_IMAGE,
    V1_FROZEN_CUI_DRAW_MESH,
    V1_FROZEN_CUI_DRAW_DROP_SHADOW,
    V1_FROZEN_CUI_DRAW_INSET_SHADOW
} v1_frozen_cui_draw_type;
typedef enum {
    V1_FROZEN_CUI_TEXT_WEIGHT_NORMAL,
    V1_FROZEN_CUI_TEXT_WEIGHT_MEDIUM,
    V1_FROZEN_CUI_TEXT_WEIGHT_SEMIBOLD,
    V1_FROZEN_CUI_TEXT_WEIGHT_BOLD
} v1_frozen_cui_text_weight;
typedef struct {
    v1_frozen_cui_text_weight weight;
    bool italic, monospace;
    uint32_t family;

    float letter_spacing;
} v1_frozen_cui_text_style;
typedef struct {
    size_t byte_start, byte_end;
    v1_frozen_cui_color color;
} v1_frozen_cui_text_color_span;
typedef enum {
    V1_FROZEN_CUI_TEXT_PAINT_SOLID,
    V1_FROZEN_CUI_TEXT_PAINT_LINEAR
} v1_frozen_cui_text_paint_kind;
typedef enum {
    V1_FROZEN_CUI_TEXT_ANCHOR_BLOCK,
    V1_FROZEN_CUI_TEXT_ANCHOR_LINE
} v1_frozen_cui_text_paint_anchor;
typedef struct {
    float position;
    v1_frozen_cui_color color;
} v1_frozen_cui_text_gradient_stop;
typedef struct {
    v1_frozen_cui_text_paint_kind kind;
    v1_frozen_cui_color color;
    float x0, y0, x1, y1;
    v1_frozen_cui_text_paint_anchor anchor;
    const v1_frozen_cui_text_gradient_stop *stops;
    size_t stop_count;
} v1_frozen_cui_text_paint;
typedef struct {
    v1_frozen_cui_draw_type type;
    v1_frozen_cui_rect bounds;
    v1_frozen_cui_color color;
    float radius, stroke, font_size;
    v1_frozen_cui_text_style text_style;
    v1_frozen_cui_str text;
    v1_frozen_cui_corner_radii corner_radii;
    v1_frozen_cui_border_edges border_edges;
    bool custom_corners, custom_border_edges;
    v1_frozen_cui_bitmap_paint bitmap;
    v1_frozen_cui_mesh_paint mesh;
    v1_frozen_cui_text_line_source text_source;
    const v1_frozen_cui_text_color_span *text_colors;
    size_t text_color_count;
    v1_frozen_cui_text_paint text_paint;
    v1_frozen_cui_drop_shadow_paint shadow;
} v1_frozen_cui_draw_command;
typedef struct {
    const v1_frozen_cui_draw_command *commands;
    size_t count;
    float width, height;
} v1_frozen_cui_draw_list;
typedef struct {
    float width, height;
} v1_frozen_cui_size;
typedef struct {
    float width, height;
    size_t max_elements, max_commands, frame_bytes;
    cui_measure_fn measure;
    cui_measure_styled_fn measure_styled;
    void *measure_user;

    size_t max_resources, resource_bytes;
    size_t event_text_bytes;
    cui_font_metrics_fn font_metrics;
    cui_text_geometry_fn text_geometry;
    cui_text_geometry_line_fn text_geometry_line;

    uint32_t text_capabilities;
} v1_frozen_cui_config;
typedef struct {
    void *user;
    const char *name;
    unsigned capabilities;
    bool (*render)(void *user, const v1_frozen_cui_draw_list *list, v1_frozen_cui_color clear);
    void (*destroy)(void *user);
} v1_frozen_cui_renderer;
_Static_assert(sizeof(cui_str) == sizeof(v1_frozen_cui_str), "cui_str size");
_Static_assert(_Alignof(cui_str) == _Alignof(v1_frozen_cui_str), "cui_str alignment");
_Static_assert(offsetof(cui_str, data) == offsetof(v1_frozen_cui_str, data), "cui_str.data offset");
_Static_assert(offsetof(cui_str, length) == offsetof(v1_frozen_cui_str, length),
               "cui_str.length offset");
_Static_assert(sizeof(cui_text_line_source) == sizeof(v1_frozen_cui_text_line_source),
               "cui_text_line_source size");
_Static_assert(_Alignof(cui_text_line_source) == _Alignof(v1_frozen_cui_text_line_source),
               "cui_text_line_source alignment");
_Static_assert(offsetof(cui_text_line_source, paragraph) ==
                   offsetof(v1_frozen_cui_text_line_source, paragraph),
               "cui_text_line_source.paragraph offset");
_Static_assert(offsetof(cui_text_line_source, byte_start) ==
                   offsetof(v1_frozen_cui_text_line_source, byte_start),
               "cui_text_line_source.byte_start offset");
_Static_assert(offsetof(cui_text_line_source, byte_end) ==
                   offsetof(v1_frozen_cui_text_line_source, byte_end),
               "cui_text_line_source.byte_end offset");
_Static_assert(sizeof(cui_rect) == sizeof(v1_frozen_cui_rect), "cui_rect size");
_Static_assert(_Alignof(cui_rect) == _Alignof(v1_frozen_cui_rect), "cui_rect alignment");
_Static_assert(offsetof(cui_rect, x) == offsetof(v1_frozen_cui_rect, x), "cui_rect.x offset");
_Static_assert(offsetof(cui_rect, y) == offsetof(v1_frozen_cui_rect, y), "cui_rect.y offset");
_Static_assert(offsetof(cui_rect, w) == offsetof(v1_frozen_cui_rect, w), "cui_rect.w offset");
_Static_assert(offsetof(cui_rect, h) == offsetof(v1_frozen_cui_rect, h), "cui_rect.h offset");
_Static_assert(sizeof(cui_color) == sizeof(v1_frozen_cui_color), "cui_color size");
_Static_assert(_Alignof(cui_color) == _Alignof(v1_frozen_cui_color), "cui_color alignment");
_Static_assert(offsetof(cui_color, r) == offsetof(v1_frozen_cui_color, r), "cui_color.r offset");
_Static_assert(offsetof(cui_color, g) == offsetof(v1_frozen_cui_color, g), "cui_color.g offset");
_Static_assert(offsetof(cui_color, b) == offsetof(v1_frozen_cui_color, b), "cui_color.b offset");
_Static_assert(offsetof(cui_color, a) == offsetof(v1_frozen_cui_color, a), "cui_color.a offset");
_Static_assert(sizeof(cui_error) == sizeof(v1_frozen_cui_error), "cui_error size");
_Static_assert(_Alignof(cui_error) == _Alignof(v1_frozen_cui_error), "cui_error alignment");
_Static_assert((int)CUI_OK == (int)V1_FROZEN_CUI_OK, "CUI_OK value");
_Static_assert((int)CUI_ERROR_CAPACITY == (int)V1_FROZEN_CUI_ERROR_CAPACITY,
               "CUI_ERROR_CAPACITY value");
_Static_assert((int)CUI_ERROR_DUPLICATE_ID == (int)V1_FROZEN_CUI_ERROR_DUPLICATE_ID,
               "CUI_ERROR_DUPLICATE_ID value");
_Static_assert((int)CUI_ERROR_LIFECYCLE == (int)V1_FROZEN_CUI_ERROR_LIFECYCLE,
               "CUI_ERROR_LIFECYCLE value");
_Static_assert((int)CUI_ERROR_LAYOUT == (int)V1_FROZEN_CUI_ERROR_LAYOUT, "CUI_ERROR_LAYOUT value");
_Static_assert((int)CUI_ERROR_INVALID_ACTION == (int)V1_FROZEN_CUI_ERROR_INVALID_ACTION,
               "CUI_ERROR_INVALID_ACTION value");
_Static_assert(sizeof(cui_align) == sizeof(v1_frozen_cui_align), "cui_align size");
_Static_assert(_Alignof(cui_align) == _Alignof(v1_frozen_cui_align), "cui_align alignment");
_Static_assert((int)CUI_ALIGN_START == (int)V1_FROZEN_CUI_ALIGN_START, "CUI_ALIGN_START value");
_Static_assert((int)CUI_ALIGN_CENTER == (int)V1_FROZEN_CUI_ALIGN_CENTER, "CUI_ALIGN_CENTER value");
_Static_assert((int)CUI_ALIGN_END == (int)V1_FROZEN_CUI_ALIGN_END, "CUI_ALIGN_END value");
_Static_assert(sizeof(cui_corner_radii) == sizeof(v1_frozen_cui_corner_radii),
               "cui_corner_radii size");
_Static_assert(_Alignof(cui_corner_radii) == _Alignof(v1_frozen_cui_corner_radii),
               "cui_corner_radii alignment");
_Static_assert(offsetof(cui_corner_radii, top_left) ==
                   offsetof(v1_frozen_cui_corner_radii, top_left),
               "cui_corner_radii.top_left offset");
_Static_assert(offsetof(cui_corner_radii, top_right) ==
                   offsetof(v1_frozen_cui_corner_radii, top_right),
               "cui_corner_radii.top_right offset");
_Static_assert(offsetof(cui_corner_radii, bottom_left) ==
                   offsetof(v1_frozen_cui_corner_radii, bottom_left),
               "cui_corner_radii.bottom_left offset");
_Static_assert(offsetof(cui_corner_radii, bottom_right) ==
                   offsetof(v1_frozen_cui_corner_radii, bottom_right),
               "cui_corner_radii.bottom_right offset");
_Static_assert(sizeof(cui_drop_shadow) == sizeof(v1_frozen_cui_drop_shadow),
               "cui_drop_shadow size");
_Static_assert(_Alignof(cui_drop_shadow) == _Alignof(v1_frozen_cui_drop_shadow),
               "cui_drop_shadow alignment");
_Static_assert(offsetof(cui_drop_shadow, offset_x) == offsetof(v1_frozen_cui_drop_shadow, offset_x),
               "cui_drop_shadow.offset_x offset");
_Static_assert(offsetof(cui_drop_shadow, offset_y) == offsetof(v1_frozen_cui_drop_shadow, offset_y),
               "cui_drop_shadow.offset_y offset");
_Static_assert(offsetof(cui_drop_shadow, sigma) == offsetof(v1_frozen_cui_drop_shadow, sigma),
               "cui_drop_shadow.sigma offset");
_Static_assert(offsetof(cui_drop_shadow, spread) == offsetof(v1_frozen_cui_drop_shadow, spread),
               "cui_drop_shadow.spread offset");
_Static_assert(offsetof(cui_drop_shadow, color) == offsetof(v1_frozen_cui_drop_shadow, color),
               "cui_drop_shadow.color offset");
_Static_assert(sizeof(cui_drop_shadow_paint) == sizeof(v1_frozen_cui_drop_shadow_paint),
               "cui_drop_shadow_paint size");
_Static_assert(_Alignof(cui_drop_shadow_paint) == _Alignof(v1_frozen_cui_drop_shadow_paint),
               "cui_drop_shadow_paint alignment");
_Static_assert(offsetof(cui_drop_shadow_paint, caster) ==
                   offsetof(v1_frozen_cui_drop_shadow_paint, caster),
               "cui_drop_shadow_paint.caster offset");
_Static_assert(offsetof(cui_drop_shadow_paint, radii) ==
                   offsetof(v1_frozen_cui_drop_shadow_paint, radii),
               "cui_drop_shadow_paint.radii offset");
_Static_assert(offsetof(cui_drop_shadow_paint, layer) ==
                   offsetof(v1_frozen_cui_drop_shadow_paint, layer),
               "cui_drop_shadow_paint.layer offset");
_Static_assert(sizeof(cui_border_edges) == sizeof(v1_frozen_cui_border_edges),
               "cui_border_edges size");
_Static_assert(_Alignof(cui_border_edges) == _Alignof(v1_frozen_cui_border_edges),
               "cui_border_edges alignment");
_Static_assert(offsetof(cui_border_edges, left) == offsetof(v1_frozen_cui_border_edges, left),
               "cui_border_edges.left offset");
_Static_assert(offsetof(cui_border_edges, right) == offsetof(v1_frozen_cui_border_edges, right),
               "cui_border_edges.right offset");
_Static_assert(offsetof(cui_border_edges, top) == offsetof(v1_frozen_cui_border_edges, top),
               "cui_border_edges.top offset");
_Static_assert(offsetof(cui_border_edges, bottom) == offsetof(v1_frozen_cui_border_edges, bottom),
               "cui_border_edges.bottom offset");
_Static_assert(sizeof(cui_box) == sizeof(v1_frozen_cui_box), "cui_box size");
_Static_assert(_Alignof(cui_box) == _Alignof(v1_frozen_cui_box), "cui_box alignment");
_Static_assert(offsetof(cui_box, width) == offsetof(v1_frozen_cui_box, width),
               "cui_box.width offset");
_Static_assert(offsetof(cui_box, height) == offsetof(v1_frozen_cui_box, height),
               "cui_box.height offset");
_Static_assert(offsetof(cui_box, padding) == offsetof(v1_frozen_cui_box, padding),
               "cui_box.padding offset");
_Static_assert(offsetof(cui_box, gap) == offsetof(v1_frozen_cui_box, gap), "cui_box.gap offset");
_Static_assert(offsetof(cui_box, radius) == offsetof(v1_frozen_cui_box, radius),
               "cui_box.radius offset");
_Static_assert(offsetof(cui_box, row) == offsetof(v1_frozen_cui_box, row), "cui_box.row offset");
_Static_assert(offsetof(cui_box, clip) == offsetof(v1_frozen_cui_box, clip), "cui_box.clip offset");
_Static_assert(offsetof(cui_box, background) == offsetof(v1_frozen_cui_box, background),
               "cui_box.background offset");
_Static_assert(offsetof(cui_box, border) == offsetof(v1_frozen_cui_box, border),
               "cui_box.border offset");
_Static_assert(offsetof(cui_box, border_width) == offsetof(v1_frozen_cui_box, border_width),
               "cui_box.border_width offset");
_Static_assert(offsetof(cui_box, align_x) == offsetof(v1_frozen_cui_box, align_x),
               "cui_box.align_x offset");
_Static_assert(offsetof(cui_box, align_y) == offsetof(v1_frozen_cui_box, align_y),
               "cui_box.align_y offset");
_Static_assert(offsetof(cui_box, offset_x) == offsetof(v1_frozen_cui_box, offset_x),
               "cui_box.offset_x offset");
_Static_assert(offsetof(cui_box, offset_y) == offsetof(v1_frozen_cui_box, offset_y),
               "cui_box.offset_y offset");
_Static_assert(offsetof(cui_box, corner_radii) == offsetof(v1_frozen_cui_box, corner_radii),
               "cui_box.corner_radii offset");
_Static_assert(offsetof(cui_box, border_edges) == offsetof(v1_frozen_cui_box, border_edges),
               "cui_box.border_edges offset");
_Static_assert(offsetof(cui_box, custom_corners) == offsetof(v1_frozen_cui_box, custom_corners),
               "cui_box.custom_corners offset");
_Static_assert(offsetof(cui_box, custom_border_edges) ==
                   offsetof(v1_frozen_cui_box, custom_border_edges),
               "cui_box.custom_border_edges offset");
_Static_assert(offsetof(cui_box, fixed_width) == offsetof(v1_frozen_cui_box, fixed_width),
               "cui_box.fixed_width offset");
_Static_assert(offsetof(cui_box, fixed_height) == offsetof(v1_frozen_cui_box, fixed_height),
               "cui_box.fixed_height offset");
_Static_assert(offsetof(cui_box, layout_only) == offsetof(v1_frozen_cui_box, layout_only),
               "cui_box.layout_only offset");
_Static_assert(offsetof(cui_box, inert) == offsetof(v1_frozen_cui_box, inert),
               "cui_box.inert offset");
_Static_assert(offsetof(cui_box, max_width) == offsetof(v1_frozen_cui_box, max_width),
               "cui_box.max_width offset");
_Static_assert(offsetof(cui_box, max_height) == offsetof(v1_frozen_cui_box, max_height),
               "cui_box.max_height offset");
_Static_assert(offsetof(cui_box, opacity) == offsetof(v1_frozen_cui_box, opacity),
               "cui_box.opacity offset");
_Static_assert(offsetof(cui_box, custom_opacity) == offsetof(v1_frozen_cui_box, custom_opacity),
               "cui_box.custom_opacity offset");
_Static_assert(offsetof(cui_box, width_percent) == offsetof(v1_frozen_cui_box, width_percent),
               "cui_box.width_percent offset");
_Static_assert(offsetof(cui_box, height_percent) == offsetof(v1_frozen_cui_box, height_percent),
               "cui_box.height_percent offset");
_Static_assert(offsetof(cui_box, padding_edges) == offsetof(v1_frozen_cui_box, padding_edges),
               "cui_box.padding_edges offset");
_Static_assert(offsetof(cui_box, custom_padding) == offsetof(v1_frozen_cui_box, custom_padding),
               "cui_box.custom_padding offset");
_Static_assert(sizeof(cui_bitmap) == sizeof(v1_frozen_cui_bitmap), "cui_bitmap size");
_Static_assert(_Alignof(cui_bitmap) == _Alignof(v1_frozen_cui_bitmap), "cui_bitmap alignment");
_Static_assert(offsetof(cui_bitmap, pixels) == offsetof(v1_frozen_cui_bitmap, pixels),
               "cui_bitmap.pixels offset");
_Static_assert(offsetof(cui_bitmap, width) == offsetof(v1_frozen_cui_bitmap, width),
               "cui_bitmap.width offset");
_Static_assert(offsetof(cui_bitmap, height) == offsetof(v1_frozen_cui_bitmap, height),
               "cui_bitmap.height offset");
_Static_assert(offsetof(cui_bitmap, stride) == offsetof(v1_frozen_cui_bitmap, stride),
               "cui_bitmap.stride offset");
_Static_assert(offsetof(cui_bitmap, revision) == offsetof(v1_frozen_cui_bitmap, revision),
               "cui_bitmap.revision offset");
_Static_assert(sizeof(cui_bitmap_paint) == sizeof(v1_frozen_cui_bitmap_paint),
               "cui_bitmap_paint size");
_Static_assert(_Alignof(cui_bitmap_paint) == _Alignof(v1_frozen_cui_bitmap_paint),
               "cui_bitmap_paint alignment");
_Static_assert(offsetof(cui_bitmap_paint, bitmap) == offsetof(v1_frozen_cui_bitmap_paint, bitmap),
               "cui_bitmap_paint.bitmap offset");
_Static_assert(offsetof(cui_bitmap_paint, source) == offsetof(v1_frozen_cui_bitmap_paint, source),
               "cui_bitmap_paint.source offset");
_Static_assert(offsetof(cui_bitmap_paint, opacity) == offsetof(v1_frozen_cui_bitmap_paint, opacity),
               "cui_bitmap_paint.opacity offset");
_Static_assert(offsetof(cui_bitmap_paint, grayscale) ==
                   offsetof(v1_frozen_cui_bitmap_paint, grayscale),
               "cui_bitmap_paint.grayscale offset");
_Static_assert(offsetof(cui_bitmap_paint, tint) == offsetof(v1_frozen_cui_bitmap_paint, tint),
               "cui_bitmap_paint.tint offset");
_Static_assert(offsetof(cui_bitmap_paint, custom_tint) ==
                   offsetof(v1_frozen_cui_bitmap_paint, custom_tint),
               "cui_bitmap_paint.custom_tint offset");
_Static_assert(sizeof(cui_mesh_vertex) == sizeof(v1_frozen_cui_mesh_vertex),
               "cui_mesh_vertex size");
_Static_assert(_Alignof(cui_mesh_vertex) == _Alignof(v1_frozen_cui_mesh_vertex),
               "cui_mesh_vertex alignment");
_Static_assert(offsetof(cui_mesh_vertex, x) == offsetof(v1_frozen_cui_mesh_vertex, x),
               "cui_mesh_vertex.x offset");
_Static_assert(offsetof(cui_mesh_vertex, y) == offsetof(v1_frozen_cui_mesh_vertex, y),
               "cui_mesh_vertex.y offset");
_Static_assert(offsetof(cui_mesh_vertex, s) == offsetof(v1_frozen_cui_mesh_vertex, s),
               "cui_mesh_vertex.s offset");
_Static_assert(offsetof(cui_mesh_vertex, t) == offsetof(v1_frozen_cui_mesh_vertex, t),
               "cui_mesh_vertex.t offset");
_Static_assert(sizeof(cui_linear_color) == sizeof(v1_frozen_cui_linear_color),
               "cui_linear_color size");
_Static_assert(_Alignof(cui_linear_color) == _Alignof(v1_frozen_cui_linear_color),
               "cui_linear_color alignment");
_Static_assert(offsetof(cui_linear_color, r) == offsetof(v1_frozen_cui_linear_color, r),
               "cui_linear_color.r offset");
_Static_assert(offsetof(cui_linear_color, g) == offsetof(v1_frozen_cui_linear_color, g),
               "cui_linear_color.g offset");
_Static_assert(offsetof(cui_linear_color, b) == offsetof(v1_frozen_cui_linear_color, b),
               "cui_linear_color.b offset");
_Static_assert(offsetof(cui_linear_color, a) == offsetof(v1_frozen_cui_linear_color, a),
               "cui_linear_color.a offset");
_Static_assert(sizeof(cui_mesh_paint) == sizeof(v1_frozen_cui_mesh_paint), "cui_mesh_paint size");
_Static_assert(_Alignof(cui_mesh_paint) == _Alignof(v1_frozen_cui_mesh_paint),
               "cui_mesh_paint alignment");
_Static_assert(offsetof(cui_mesh_paint, vertices) == offsetof(v1_frozen_cui_mesh_paint, vertices),
               "cui_mesh_paint.vertices offset");
_Static_assert(offsetof(cui_mesh_paint, vertex_count) ==
                   offsetof(v1_frozen_cui_mesh_paint, vertex_count),
               "cui_mesh_paint.vertex_count offset");
_Static_assert(offsetof(cui_mesh_paint, origin_x) == offsetof(v1_frozen_cui_mesh_paint, origin_x),
               "cui_mesh_paint.origin_x offset");
_Static_assert(offsetof(cui_mesh_paint, origin_y) == offsetof(v1_frozen_cui_mesh_paint, origin_y),
               "cui_mesh_paint.origin_y offset");
_Static_assert(offsetof(cui_mesh_paint, scale) == offsetof(v1_frozen_cui_mesh_paint, scale),
               "cui_mesh_paint.scale offset");
_Static_assert(offsetof(cui_mesh_paint, color) == offsetof(v1_frozen_cui_mesh_paint, color),
               "cui_mesh_paint.color offset");
_Static_assert(sizeof(cui_draw_type) == sizeof(v1_frozen_cui_draw_type), "cui_draw_type size");
_Static_assert(_Alignof(cui_draw_type) == _Alignof(v1_frozen_cui_draw_type),
               "cui_draw_type alignment");
_Static_assert((int)CUI_DRAW_RECT == (int)V1_FROZEN_CUI_DRAW_RECT, "CUI_DRAW_RECT value");
_Static_assert((int)CUI_DRAW_BORDER == (int)V1_FROZEN_CUI_DRAW_BORDER, "CUI_DRAW_BORDER value");
_Static_assert((int)CUI_DRAW_TEXT == (int)V1_FROZEN_CUI_DRAW_TEXT, "CUI_DRAW_TEXT value");
_Static_assert((int)CUI_DRAW_CLIP_PUSH == (int)V1_FROZEN_CUI_DRAW_CLIP_PUSH,
               "CUI_DRAW_CLIP_PUSH value");
_Static_assert((int)CUI_DRAW_CLIP_POP == (int)V1_FROZEN_CUI_DRAW_CLIP_POP,
               "CUI_DRAW_CLIP_POP value");
_Static_assert((int)CUI_DRAW_IMAGE == (int)V1_FROZEN_CUI_DRAW_IMAGE, "CUI_DRAW_IMAGE value");
_Static_assert((int)CUI_DRAW_MESH == (int)V1_FROZEN_CUI_DRAW_MESH, "CUI_DRAW_MESH value");
_Static_assert((int)CUI_DRAW_DROP_SHADOW == (int)V1_FROZEN_CUI_DRAW_DROP_SHADOW,
               "CUI_DRAW_DROP_SHADOW value");
_Static_assert((int)CUI_DRAW_INSET_SHADOW == (int)V1_FROZEN_CUI_DRAW_INSET_SHADOW,
               "CUI_DRAW_INSET_SHADOW value");
_Static_assert(sizeof(cui_text_weight) == sizeof(v1_frozen_cui_text_weight),
               "cui_text_weight size");
_Static_assert(_Alignof(cui_text_weight) == _Alignof(v1_frozen_cui_text_weight),
               "cui_text_weight alignment");
_Static_assert((int)CUI_TEXT_WEIGHT_NORMAL == (int)V1_FROZEN_CUI_TEXT_WEIGHT_NORMAL,
               "CUI_TEXT_WEIGHT_NORMAL value");
_Static_assert((int)CUI_TEXT_WEIGHT_MEDIUM == (int)V1_FROZEN_CUI_TEXT_WEIGHT_MEDIUM,
               "CUI_TEXT_WEIGHT_MEDIUM value");
_Static_assert((int)CUI_TEXT_WEIGHT_SEMIBOLD == (int)V1_FROZEN_CUI_TEXT_WEIGHT_SEMIBOLD,
               "CUI_TEXT_WEIGHT_SEMIBOLD value");
_Static_assert((int)CUI_TEXT_WEIGHT_BOLD == (int)V1_FROZEN_CUI_TEXT_WEIGHT_BOLD,
               "CUI_TEXT_WEIGHT_BOLD value");
_Static_assert(sizeof(cui_text_style) == sizeof(v1_frozen_cui_text_style), "cui_text_style size");
_Static_assert(_Alignof(cui_text_style) == _Alignof(v1_frozen_cui_text_style),
               "cui_text_style alignment");
_Static_assert(offsetof(cui_text_style, weight) == offsetof(v1_frozen_cui_text_style, weight),
               "cui_text_style.weight offset");
_Static_assert(offsetof(cui_text_style, italic) == offsetof(v1_frozen_cui_text_style, italic),
               "cui_text_style.italic offset");
_Static_assert(offsetof(cui_text_style, monospace) == offsetof(v1_frozen_cui_text_style, monospace),
               "cui_text_style.monospace offset");
_Static_assert(offsetof(cui_text_style, family) == offsetof(v1_frozen_cui_text_style, family),
               "cui_text_style.family offset");
_Static_assert(offsetof(cui_text_style, letter_spacing) ==
                   offsetof(v1_frozen_cui_text_style, letter_spacing),
               "cui_text_style.letter_spacing offset");
_Static_assert(sizeof(cui_text_color_span) == sizeof(v1_frozen_cui_text_color_span),
               "cui_text_color_span size");
_Static_assert(_Alignof(cui_text_color_span) == _Alignof(v1_frozen_cui_text_color_span),
               "cui_text_color_span alignment");
_Static_assert(offsetof(cui_text_color_span, byte_start) ==
                   offsetof(v1_frozen_cui_text_color_span, byte_start),
               "cui_text_color_span.byte_start offset");
_Static_assert(offsetof(cui_text_color_span, byte_end) ==
                   offsetof(v1_frozen_cui_text_color_span, byte_end),
               "cui_text_color_span.byte_end offset");
_Static_assert(offsetof(cui_text_color_span, color) ==
                   offsetof(v1_frozen_cui_text_color_span, color),
               "cui_text_color_span.color offset");
_Static_assert(sizeof(cui_text_paint_kind) == sizeof(v1_frozen_cui_text_paint_kind),
               "cui_text_paint_kind size");
_Static_assert(_Alignof(cui_text_paint_kind) == _Alignof(v1_frozen_cui_text_paint_kind),
               "cui_text_paint_kind alignment");
_Static_assert((int)CUI_TEXT_PAINT_SOLID == (int)V1_FROZEN_CUI_TEXT_PAINT_SOLID,
               "CUI_TEXT_PAINT_SOLID value");
_Static_assert((int)CUI_TEXT_PAINT_LINEAR == (int)V1_FROZEN_CUI_TEXT_PAINT_LINEAR,
               "CUI_TEXT_PAINT_LINEAR value");
_Static_assert(sizeof(cui_text_paint_anchor) == sizeof(v1_frozen_cui_text_paint_anchor),
               "cui_text_paint_anchor size");
_Static_assert(_Alignof(cui_text_paint_anchor) == _Alignof(v1_frozen_cui_text_paint_anchor),
               "cui_text_paint_anchor alignment");
_Static_assert((int)CUI_TEXT_ANCHOR_BLOCK == (int)V1_FROZEN_CUI_TEXT_ANCHOR_BLOCK,
               "CUI_TEXT_ANCHOR_BLOCK value");
_Static_assert((int)CUI_TEXT_ANCHOR_LINE == (int)V1_FROZEN_CUI_TEXT_ANCHOR_LINE,
               "CUI_TEXT_ANCHOR_LINE value");
_Static_assert(sizeof(cui_text_gradient_stop) == sizeof(v1_frozen_cui_text_gradient_stop),
               "cui_text_gradient_stop size");
_Static_assert(_Alignof(cui_text_gradient_stop) == _Alignof(v1_frozen_cui_text_gradient_stop),
               "cui_text_gradient_stop alignment");
_Static_assert(offsetof(cui_text_gradient_stop, position) ==
                   offsetof(v1_frozen_cui_text_gradient_stop, position),
               "cui_text_gradient_stop.position offset");
_Static_assert(offsetof(cui_text_gradient_stop, color) ==
                   offsetof(v1_frozen_cui_text_gradient_stop, color),
               "cui_text_gradient_stop.color offset");
_Static_assert(sizeof(cui_text_paint) == sizeof(v1_frozen_cui_text_paint), "cui_text_paint size");
_Static_assert(_Alignof(cui_text_paint) == _Alignof(v1_frozen_cui_text_paint),
               "cui_text_paint alignment");
_Static_assert(offsetof(cui_text_paint, kind) == offsetof(v1_frozen_cui_text_paint, kind),
               "cui_text_paint.kind offset");
_Static_assert(offsetof(cui_text_paint, color) == offsetof(v1_frozen_cui_text_paint, color),
               "cui_text_paint.color offset");
_Static_assert(offsetof(cui_text_paint, x0) == offsetof(v1_frozen_cui_text_paint, x0),
               "cui_text_paint.x0 offset");
_Static_assert(offsetof(cui_text_paint, y0) == offsetof(v1_frozen_cui_text_paint, y0),
               "cui_text_paint.y0 offset");
_Static_assert(offsetof(cui_text_paint, x1) == offsetof(v1_frozen_cui_text_paint, x1),
               "cui_text_paint.x1 offset");
_Static_assert(offsetof(cui_text_paint, y1) == offsetof(v1_frozen_cui_text_paint, y1),
               "cui_text_paint.y1 offset");
_Static_assert(offsetof(cui_text_paint, anchor) == offsetof(v1_frozen_cui_text_paint, anchor),
               "cui_text_paint.anchor offset");
_Static_assert(offsetof(cui_text_paint, stops) == offsetof(v1_frozen_cui_text_paint, stops),
               "cui_text_paint.stops offset");
_Static_assert(offsetof(cui_text_paint, stop_count) ==
                   offsetof(v1_frozen_cui_text_paint, stop_count),
               "cui_text_paint.stop_count offset");
_Static_assert(sizeof(cui_draw_command) == sizeof(v1_frozen_cui_draw_command),
               "cui_draw_command size");
_Static_assert(_Alignof(cui_draw_command) == _Alignof(v1_frozen_cui_draw_command),
               "cui_draw_command alignment");
_Static_assert(offsetof(cui_draw_command, type) == offsetof(v1_frozen_cui_draw_command, type),
               "cui_draw_command.type offset");
_Static_assert(offsetof(cui_draw_command, bounds) == offsetof(v1_frozen_cui_draw_command, bounds),
               "cui_draw_command.bounds offset");
_Static_assert(offsetof(cui_draw_command, color) == offsetof(v1_frozen_cui_draw_command, color),
               "cui_draw_command.color offset");
_Static_assert(offsetof(cui_draw_command, radius) == offsetof(v1_frozen_cui_draw_command, radius),
               "cui_draw_command.radius offset");
_Static_assert(offsetof(cui_draw_command, stroke) == offsetof(v1_frozen_cui_draw_command, stroke),
               "cui_draw_command.stroke offset");
_Static_assert(offsetof(cui_draw_command, font_size) ==
                   offsetof(v1_frozen_cui_draw_command, font_size),
               "cui_draw_command.font_size offset");
_Static_assert(offsetof(cui_draw_command, text_style) ==
                   offsetof(v1_frozen_cui_draw_command, text_style),
               "cui_draw_command.text_style offset");
_Static_assert(offsetof(cui_draw_command, text) == offsetof(v1_frozen_cui_draw_command, text),
               "cui_draw_command.text offset");
_Static_assert(offsetof(cui_draw_command, corner_radii) ==
                   offsetof(v1_frozen_cui_draw_command, corner_radii),
               "cui_draw_command.corner_radii offset");
_Static_assert(offsetof(cui_draw_command, border_edges) ==
                   offsetof(v1_frozen_cui_draw_command, border_edges),
               "cui_draw_command.border_edges offset");
_Static_assert(offsetof(cui_draw_command, custom_corners) ==
                   offsetof(v1_frozen_cui_draw_command, custom_corners),
               "cui_draw_command.custom_corners offset");
_Static_assert(offsetof(cui_draw_command, custom_border_edges) ==
                   offsetof(v1_frozen_cui_draw_command, custom_border_edges),
               "cui_draw_command.custom_border_edges offset");
_Static_assert(offsetof(cui_draw_command, bitmap) == offsetof(v1_frozen_cui_draw_command, bitmap),
               "cui_draw_command.bitmap offset");
_Static_assert(offsetof(cui_draw_command, mesh) == offsetof(v1_frozen_cui_draw_command, mesh),
               "cui_draw_command.mesh offset");
_Static_assert(offsetof(cui_draw_command, text_source) ==
                   offsetof(v1_frozen_cui_draw_command, text_source),
               "cui_draw_command.text_source offset");
_Static_assert(offsetof(cui_draw_command, text_colors) ==
                   offsetof(v1_frozen_cui_draw_command, text_colors),
               "cui_draw_command.text_colors offset");
_Static_assert(offsetof(cui_draw_command, text_color_count) ==
                   offsetof(v1_frozen_cui_draw_command, text_color_count),
               "cui_draw_command.text_color_count offset");
_Static_assert(offsetof(cui_draw_command, text_paint) ==
                   offsetof(v1_frozen_cui_draw_command, text_paint),
               "cui_draw_command.text_paint offset");
_Static_assert(offsetof(cui_draw_command, shadow) == offsetof(v1_frozen_cui_draw_command, shadow),
               "cui_draw_command.shadow offset");
_Static_assert(sizeof(cui_draw_list) == sizeof(v1_frozen_cui_draw_list), "cui_draw_list size");
_Static_assert(_Alignof(cui_draw_list) == _Alignof(v1_frozen_cui_draw_list),
               "cui_draw_list alignment");
_Static_assert(offsetof(cui_draw_list, commands) == offsetof(v1_frozen_cui_draw_list, commands),
               "cui_draw_list.commands offset");
_Static_assert(offsetof(cui_draw_list, count) == offsetof(v1_frozen_cui_draw_list, count),
               "cui_draw_list.count offset");
_Static_assert(offsetof(cui_draw_list, width) == offsetof(v1_frozen_cui_draw_list, width),
               "cui_draw_list.width offset");
_Static_assert(offsetof(cui_draw_list, height) == offsetof(v1_frozen_cui_draw_list, height),
               "cui_draw_list.height offset");
_Static_assert(sizeof(cui_size) == sizeof(v1_frozen_cui_size), "cui_size size");
_Static_assert(_Alignof(cui_size) == _Alignof(v1_frozen_cui_size), "cui_size alignment");
_Static_assert(offsetof(cui_size, width) == offsetof(v1_frozen_cui_size, width),
               "cui_size.width offset");
_Static_assert(offsetof(cui_size, height) == offsetof(v1_frozen_cui_size, height),
               "cui_size.height offset");
_Static_assert(sizeof(cui_config) == sizeof(v1_frozen_cui_config), "cui_config size");
_Static_assert(_Alignof(cui_config) == _Alignof(v1_frozen_cui_config), "cui_config alignment");
_Static_assert(offsetof(cui_config, width) == offsetof(v1_frozen_cui_config, width),
               "cui_config.width offset");
_Static_assert(offsetof(cui_config, height) == offsetof(v1_frozen_cui_config, height),
               "cui_config.height offset");
_Static_assert(offsetof(cui_config, max_elements) == offsetof(v1_frozen_cui_config, max_elements),
               "cui_config.max_elements offset");
_Static_assert(offsetof(cui_config, max_commands) == offsetof(v1_frozen_cui_config, max_commands),
               "cui_config.max_commands offset");
_Static_assert(offsetof(cui_config, frame_bytes) == offsetof(v1_frozen_cui_config, frame_bytes),
               "cui_config.frame_bytes offset");
_Static_assert(offsetof(cui_config, measure) == offsetof(v1_frozen_cui_config, measure),
               "cui_config.measure offset");
_Static_assert(offsetof(cui_config, measure_styled) ==
                   offsetof(v1_frozen_cui_config, measure_styled),
               "cui_config.measure_styled offset");
_Static_assert(offsetof(cui_config, measure_user) == offsetof(v1_frozen_cui_config, measure_user),
               "cui_config.measure_user offset");
_Static_assert(offsetof(cui_config, max_resources) == offsetof(v1_frozen_cui_config, max_resources),
               "cui_config.max_resources offset");
_Static_assert(offsetof(cui_config, resource_bytes) ==
                   offsetof(v1_frozen_cui_config, resource_bytes),
               "cui_config.resource_bytes offset");
_Static_assert(offsetof(cui_config, event_text_bytes) ==
                   offsetof(v1_frozen_cui_config, event_text_bytes),
               "cui_config.event_text_bytes offset");
_Static_assert(offsetof(cui_config, font_metrics) == offsetof(v1_frozen_cui_config, font_metrics),
               "cui_config.font_metrics offset");
_Static_assert(offsetof(cui_config, text_geometry) == offsetof(v1_frozen_cui_config, text_geometry),
               "cui_config.text_geometry offset");
_Static_assert(offsetof(cui_config, text_geometry_line) ==
                   offsetof(v1_frozen_cui_config, text_geometry_line),
               "cui_config.text_geometry_line offset");
_Static_assert(offsetof(cui_config, text_capabilities) ==
                   offsetof(v1_frozen_cui_config, text_capabilities),
               "cui_config.text_capabilities offset");
_Static_assert(sizeof(cui_renderer) == sizeof(v1_frozen_cui_renderer), "cui_renderer size");
_Static_assert(_Alignof(cui_renderer) == _Alignof(v1_frozen_cui_renderer),
               "cui_renderer alignment");
_Static_assert(offsetof(cui_renderer, user) == offsetof(v1_frozen_cui_renderer, user),
               "cui_renderer.user offset");
_Static_assert(offsetof(cui_renderer, name) == offsetof(v1_frozen_cui_renderer, name),
               "cui_renderer.name offset");
_Static_assert(offsetof(cui_renderer, capabilities) ==
                   offsetof(v1_frozen_cui_renderer, capabilities),
               "cui_renderer.capabilities offset");
_Static_assert(offsetof(cui_renderer, render) == offsetof(v1_frozen_cui_renderer, render),
               "cui_renderer.render offset");
_Static_assert(offsetof(cui_renderer, destroy) == offsetof(v1_frozen_cui_renderer, destroy),
               "cui_renderer.destroy offset");
#endif
