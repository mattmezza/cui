/* SPDX-License-Identifier: MIT */
#ifndef CUI_SCROLL_X_INTERNAL_H
#define CUI_SCROLL_X_INTERNAL_H
#include "cui/cui.h"

typedef struct {
    float seed, submitted, text_width;
    float caret; /* Negative when caret reveal is inactive. */
    float placeholder_width;
    cui_id placeholder;
    cui_align align;
} cui_scroll_x_projection;

/* Called on the open named viewport, after its paint children were submitted. */
void cui_core_project_scroll_x(cui_context *, cui_id, cui_scroll_x_projection);
bool cui_core_painted_scroll_x(const cui_context *, cui_id, float *);
float cui_core_prior_scroll_x(const cui_context *, cui_id, float fallback);

static inline float cui_core_scroll_x_clamp(float seed, float width, float text_width,
                                            float caret) {
    if (width > 0 && caret >= 0) {
        if (caret < seed)
            seed = caret;
        if (caret + 1 > seed + width)
            seed = caret + 1 - width;
    }
    float maximum = text_width + (caret >= 0 ? 1 : 0) - width;
    if (maximum < 0)
        maximum = 0;
    return seed < maximum ? seed : maximum;
}

static inline float cui_core_scroll_x_alignment(float width, float content, cui_align align) {
    float extra = width > content ? width - content : 0;
    return align == CUI_ALIGN_CENTER ? extra * .5f : align == CUI_ALIGN_END ? extra : 0;
}
#endif
