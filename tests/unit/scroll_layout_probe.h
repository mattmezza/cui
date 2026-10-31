/* SPDX-License-Identifier: MIT */
#ifndef CUI_TEST_SCROLL_LAYOUT_PROBE_H
#define CUI_TEST_SCROLL_LAYOUT_PROBE_H
#include "cui/cui.h"
#define PROTOTYPE_LINK_MAX 32
/* Private test record, not a proposed/frozen public ABI. */
typedef struct {
    cui_id panel, outer, column, frame, viewport, content;
    float requested_height, max_height;
    float *outer_y, *body_y;
    bool automatic, inactive, reserved_scrollbar;
} prototype_link;
typedef struct {
    unsigned first, second, collections, commits;
    size_t bytes, links, wrapped_before, wrapped_after, commands;
    size_t strings_before, strings_after, sources_before, sources_after;
    bool input_unchanged, resources_unchanged, text_unchanged;
} prototype_stats;
bool prototype_register(cui_context *, prototype_link);
prototype_stats prototype_get_stats(cui_context *);
bool prototype_second_pass(void);

#endif
