/* SPDX-License-Identifier: MIT */
#ifndef CUI_TEXT_OVERFLOW_INTERNAL_H
#define CUI_TEXT_OVERFLOW_INTERNAL_H
#include "cui/cui_edit.h"

typedef float (*cui_ellipsis_measure_fn)(void *, cui_str);
typedef struct {
    size_t prefix;
    float prefix_width, tail_width;
    bool show_tail;
} cui_ellipsis_fit;

static inline cui_ellipsis_fit cui_text_fit_ellipsis(cui_str text, float width, float natural_width,
                                                     cui_str tail, cui_ellipsis_measure_fn measure,
                                                     void *user) {
    if (natural_width <= width)
        return (cui_ellipsis_fit){.prefix = text.length, .prefix_width = natural_width};
    float tail_width = measure(user, tail);
    if (tail_width > width)
        return (cui_ellipsis_fit){0};
    cui_ellipsis_fit result = {.tail_width = tail_width, .show_tail = true};
    cui_grapheme_iterator iterator;
    if (!cui_grapheme_iterator_init(&iterator, text))
        return result;
    size_t next;
    while ((next = cui_grapheme_iterator_next(&iterator)) > result.prefix && next < text.length) {
        float prefix_width = measure(user, (cui_str){text.data, next});
        if (prefix_width + tail_width > width)
            break;
        result.prefix = next;
        result.prefix_width = prefix_width;
    }
    return result;
}
#endif
