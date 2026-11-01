/* SPDX-License-Identifier: MIT */
#include CUI_CPU_V2_RANGE_FROZEN_HEADER
#define CPU_RANGE_RESULT cpu_range_original_private
#include "cpu_v2_range_case_internal.h"
cpu_range_result cpu_range_original(const cui_draw_list_v2 *list,
                                    const cui_render_v2_options *options, unsigned flags,
                                    const void *owner, size_t owner_bytes, unsigned output_alias) {
    return cpu_range_original_private(list, options, flags, owner, owner_bytes, output_alias);
}
