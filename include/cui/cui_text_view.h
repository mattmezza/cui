/* SPDX-License-Identifier: MIT */
#ifndef CUI_TEXT_VIEW_H
#define CUI_TEXT_VIEW_H
#include "cui.h"
#include "cui_platform.h"
#include "cui_scroll.h"
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
#define CUI_TEXT_VIEW_BYTE_LIMIT 65536u
#define CUI_TEXT_VIEW_LIST_DEPTH_LIMIT 32
#define CUI_TEXT_VIEW_NODE_LIMIT 8192u
typedef enum {
    CUI_TEXT_VIEW_MARKDOWN,
    CUI_TEXT_VIEW_HTML,
    CUI_TEXT_VIEW_PLAIN
} cui_text_view_format;
typedef enum {
    CUI_TEXT_VIEW_OK,
    CUI_TEXT_VIEW_INVALID,
    CUI_TEXT_VIEW_CAPACITY,
    CUI_TEXT_VIEW_UNSUPPORTED
} cui_text_view_error;
typedef struct {
    size_t begin, end;
} cui_text_view_range;
enum {
    CUI_TEXT_VIEW_BOLD = 1u,
    CUI_TEXT_VIEW_ITALIC = 2u,
    CUI_TEXT_VIEW_CODE = 4u,
    CUI_TEXT_VIEW_STRIKE = 8u,
    CUI_TEXT_VIEW_MARK = 16u,
    CUI_TEXT_VIEW_MEDIUM = 32u,
    CUI_TEXT_VIEW_SEMIBOLD = 64u
};
typedef enum {
    CUI_TEXT_VIEW_PARAGRAPH,
    CUI_TEXT_VIEW_HEADING,
    CUI_TEXT_VIEW_QUOTE,
    CUI_TEXT_VIEW_LIST_ITEM,
    CUI_TEXT_VIEW_CODE_BLOCK,
    CUI_TEXT_VIEW_RULE,
    CUI_TEXT_VIEW_TABLE_CELL,
    CUI_TEXT_VIEW_IMAGE
} cui_text_view_block_kind;
typedef struct {
    cui_text_view_range source;
    unsigned flags;
    float born;
} cui_text_view_mapping;
typedef struct {
    cui_text_view_range text, source, link;
    size_t block;
    unsigned flags;
} cui_text_view_span;
typedef enum {
    CUI_TEXT_VIEW_ALIGN_LEFT,
    CUI_TEXT_VIEW_ALIGN_CENTER,
    CUI_TEXT_VIEW_ALIGN_RIGHT
} cui_text_view_alignment;
typedef struct {
    cui_text_view_block_kind kind;
    cui_text_view_range text, source, resource;
    unsigned level, table, row, column, table_columns;
    cui_text_view_range list_marker;
    unsigned list_number, list_start, list_depth, list_source_indent;
    size_t list_parent;
    char marker_text[12], prefix_text[16];
    bool table_header, image_ready, task, checked, list_continuation, list_repeat_marker;
    cui_text_view_alignment alignment;
    cui_bitmap image;
    cui_rect bounds, table_frame;
    float table_track_width;
    cui_scroll_state table_scroll;
    float actions_height, text_inset;
} cui_text_view_block;
typedef struct {
    cui_text_view_range text;
    size_t span, block;
    cui_rect bounds;
    float font_size, line_height;
    unsigned flags;
} cui_text_view_glyph;
typedef struct {
    cui_text_view_range range;
    cui_color background;
} cui_text_view_highlight;
typedef struct {
    char *source, *rendered;
    size_t source_capacity, rendered_capacity;
    cui_text_view_mapping *mapping;
    size_t mapping_capacity;
    cui_text_view_span *spans;
    size_t span_capacity;
    cui_text_view_block *blocks;
    size_t block_capacity;
    cui_text_view_glyph *glyphs;
    size_t glyph_capacity;
    cui_text_view_highlight *highlights;
    size_t highlight_capacity;
} cui_text_view_workspace;
typedef struct {
    cui_text_view_workspace workspace;
    cui_text_view_format format;
    size_t source_length, text_length, span_count, block_count, glyph_count, highlight_count;
    size_t anchor, cursor;
    bool selecting, dragged, clamped, stream_fade, reduced_motion;
    float pointer_x, pointer_y, press_x, press_y;
    cui_id secondary_link;
    float time, fade_duration, width, height;
    cui_scroll_state scroll;
    cui_rect viewport;
    bool reveal_pending;
    cui_text_view_range reveal;
    float reveal_age;
} cui_text_view_state;
bool cui_text_view_init(cui_text_view_state *, cui_text_view_workspace, cui_text_view_format);
/* Safe, bounded subset parser. No scripts, networking, filesystem access or allocation.
 * Capacity/UTF-8 errors leave the existing document unchanged. */
cui_text_view_error cui_text_view_set_text(cui_text_view_state *, cui_str);
cui_str cui_text_view_source(const cui_text_view_state *);
cui_str cui_text_view_rendered_text(const cui_text_view_state *);
/* Decode a resource source range into a separate caller buffer. Supports the
 * parser's entity subset and Markdown punctuation escapes; HTML keeps backslashes.
 * Capacity failure reports required bytes excluding zero and leaves output intact. */
cui_text_view_error cui_text_view_decode_resource(const cui_text_view_state *,
                                                  cui_text_view_range source, char *output,
                                                  size_t capacity, size_t *length);
bool cui_text_view_select(cui_text_view_state *, size_t anchor, size_t cursor);
cui_str cui_text_view_selected_text(const cui_text_view_state *);
typedef enum { CUI_TEXT_VIEW_COPY_PLAIN, CUI_TEXT_VIEW_COPY_SOURCE } cui_text_view_copy_format;
/* Writes a zero-terminated copy. On capacity failure, *length reports required
 * bytes excluding zero and output is untouched. HTML Source falls back to Plain.
 * Output must be separate from document workspace.
 * Source select-all is verbatim; partial unsupported blocks return UNSUPPORTED. */
cui_text_view_error cui_text_view_copy_selection(const cui_text_view_state *,
                                                 cui_text_view_copy_format, char *output,
                                                 size_t capacity, size_t *length);
bool cui_text_view_selected_source_range(const cui_text_view_state *, cui_text_view_range *);
bool cui_text_view_range_for_source(const cui_text_view_state *, cui_text_view_range,
                                    cui_text_view_range *);
cui_text_view_error cui_text_view_set_highlights(cui_text_view_state *,
                                                 const cui_text_view_highlight *, size_t);
cui_text_view_error cui_text_view_reveal(cui_text_view_state *, cui_text_view_range);
/* Paint clock is application-driven. Returns true while animation/reveal needs a frame. */
bool cui_text_view_advance(cui_text_view_state *, float dt);
typedef cui_size (*cui_text_view_measure_fn)(void *, cui_str, float font_size, unsigned flags);
typedef enum {
    CUI_TEXT_VIEW_TABLE_WRAP,
    CUI_TEXT_VIEW_TABLE_ADAPTIVE,
    CUI_TEXT_VIEW_TABLE_NOWRAP
} cui_text_view_table_mode;
/* Layout dimensions and measured text widths are bounded to 10,000,000 logical pixels. */
typedef struct {
    float width, font_size, line_height, paragraph_gap;
    float code_actions_height, table_actions_height;
    size_t max_lines;
    bool scrollable;
    cui_text_view_table_mode table_mode;
} cui_text_view_layout_options;
cui_text_view_error cui_text_view_layout(cui_text_view_state *, cui_text_view_layout_options,
                                         cui_text_view_measure_fn, void *);
size_t cui_text_view_hit(const cui_text_view_state *, float x, float y);
typedef struct {
    cui_color foreground, muted, link, selection, code_background, border, mark, table_background;
    float font_size, line_height, paragraph_gap;
} cui_text_view_style;
cui_text_view_style cui_text_view_style_default(void);
cui_text_view_style cui_text_view_style_from_theme(const cui_theme *);
typedef struct {
    float width, height, dt;
    cui_text_view_table_mode table_mode;
    float code_actions_height, table_actions_height;
    bool selectable, scrollable, disabled, stream_fade, reduced_motion;
    size_t max_lines;
    bool custom_style;
    cui_text_view_style style;
    const cui_platform *clipboard;
    cui_text_view_copy_format copy_format;
    char *copy_buffer;
    size_t copy_capacity;
    /* Decoded URIs are borrowed only during each callback. Copy for deferred use. */
    void (*on_link)(cui_str url, const cui_event *, void *);
    bool (*open_url)(cui_str url, void *);
    void *link_user;
    bool (*image)(cui_str uri, cui_bitmap *out, void *);
    void *image_user;
    void (*code_actions)(cui_context *, cui_id actions_id, const cui_theme *, cui_str code,
                         cui_str language, void *);
    void (*table_actions)(cui_context *, cui_id actions_id, const cui_theme *,
                          const cui_text_view_state *, unsigned table, void *);
    void *actions_user;
    bool (*highlighter)(cui_str code, cui_str language, size_t code_byte, cui_color *foreground,
                        void *);
    void *highlighter_user;
    void (*on_reveal)(cui_rect window_line, void *);
    void *reveal_user;
} cui_text_view_options;
cui_text_view_options cui_text_view_default(void);
/* Retained per-view selection and document render. Shared window selection,
 * touch handles and complete CommonMark/HTML5 parsing are tracked separately. */
bool cui_text_view(cui_context *, cui_id, const cui_theme *, cui_text_view_state *,
                   cui_text_view_options);
#ifdef __cplusplus
}
#endif
#endif
