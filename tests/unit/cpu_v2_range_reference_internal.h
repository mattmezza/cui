/* SPDX-License-Identifier: MIT */
#ifndef CUI_CPU_V2_RANGE_REFERENCE_INTERNAL_H
#define CUI_CPU_V2_RANGE_REFERENCE_INTERNAL_H
#include "cui/cui_draw_v2.h"
enum {
    RANGE_OUT_NONE,
    RANGE_OUT_LIST,
    RANGE_OUT_OPTIONS,
    RANGE_OUT_COMMANDS,
    RANGE_OUT_IMAGE,
    RANGE_OUT_MESH,
    RANGE_OUT_TEXT,
    RANGE_OUT_SPANS,
    RANGE_OUT_STOPS
};
typedef struct {
    cui_error error;
    bool unchanged;
    size_t fields[9];
    bool text, shadow;
} cpu_range_result;
cpu_range_result cpu_range_original(const cui_draw_list_v2 *, const cui_render_v2_options *,
                                    unsigned flags, const void *owner, size_t owner_bytes,
                                    unsigned output_alias);
#endif
