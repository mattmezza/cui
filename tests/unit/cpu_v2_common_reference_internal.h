/* SPDX-License-Identifier: MIT */
#ifndef CUI_TEST_CPU_V2_COMMON_REFERENCE_INTERNAL_H
#define CUI_TEST_CPU_V2_COMMON_REFERENCE_INTERNAL_H
#include "cui/cui_draw_v2.h"
#include "cui/cui_text.h"
typedef void *(*cpu_common_allocate)(void *, size_t);
typedef void (*cpu_common_release)(void *, void *);
size_t cpu_common_original_layout(size_t *, size_t);
cui_error cpu_common_original_paint(unsigned char *, unsigned, unsigned, size_t, size_t,
                                    const cui_draw_list_v2 *, cui_color,
                                    const cui_render_v2_options *, cui_font *, cpu_common_allocate,
                                    cpu_common_release, void *, bool *);
size_t cpu_common_original_samples(void);
#endif
