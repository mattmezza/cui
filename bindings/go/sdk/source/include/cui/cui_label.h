/* SPDX-License-Identifier: MIT */
#ifndef CUI_LABEL_H
#define CUI_LABEL_H
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { CUI_LABEL_MATCH_FULL, CUI_LABEL_MATCH_PREFIX } cui_label_match;
typedef struct {
    cui_str primary, secondary, highlight;
    bool has_secondary, masked;
    bool no_wrap; /* Default wraps at available parent width. */
    cui_label_match match;
    float font_size, line_height; /* zero: scoped or theme font, natural height when scoped. */
    cui_text_style style;
    bool custom_color;
    cui_color color;
    cui_id semantic_id, control; /* Optional label node and controls/focus_target. */
    cui_text_align alignment;    /* Physical left (default), center or right. */
} cui_label_options;
typedef struct {
    size_t text_length, span_count;
} cui_label_prepared;
/* Strict UTF-8; failures preserve all outputs. Text has no appended NUL.
 * Buffers must not overlap each other or inputs. Masked outputs contain bullets
 * only, including for the separator, and have zero color spans. */
bool cui_label_prepare(const cui_theme *, cui_label_options, char *text, size_t text_capacity,
                       cui_text_color_span *spans, size_t span_capacity, cui_label_prepared *);
/* Copies prepared display bytes into frame storage. No retained secret text.
 * Absent a scoped size, zero line height uses 20 logical px. */
bool cui_label_source(cui_context *, const cui_theme *, cui_label_options);
#ifdef __cplusplus
}
#endif
#endif
