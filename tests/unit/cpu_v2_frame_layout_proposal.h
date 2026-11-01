/* SPDX-License-Identifier: MIT */
#ifndef CUI_CPU_V2_FRAME_LAYOUT_PROPOSAL_H
#define CUI_CPU_V2_FRAME_LAYOUT_PROPOSAL_H
/* Measurement-only proposal: no production frame operations or public ABI. */
#include "../../backends/common/cui_cpu_v2_paint_internal.h"
typedef enum {
    CUI_CPU_V2_FRAME_PROPOSED_EMPTY,
    CUI_CPU_V2_FRAME_PROPOSED_PREPARING,
    CUI_CPU_V2_FRAME_PROPOSED_READY,
    CUI_CPU_V2_FRAME_PROPOSED_RELEASING
} cui_cpu_v2_frame_proposed_state;
typedef struct cui_cpu_v2_frame_proposed {
    cui_cpu_v2_frame_proposed_state state;
    const struct cui_cpu_v2_frame_proposed *self;
    cui_cpu_v2_workspace workspace;
    unsigned char *pixels;
    unsigned width, height;
    size_t stride, bytes;
#ifdef CUI_HAS_TEXT
    cui_text_owner_internal text_owner;
    cui_text_prepare_session text_session;
#endif
} cui_cpu_v2_frame_proposed;
typedef struct {
    const void *pointer;
    size_t bytes;
} cui_cpu_v2_exclusion_proposed;
typedef struct {
    unsigned width, height;
    cui_cpu_v2_exclusion_proposed excluded[3];
    size_t exclusion_count;
    bool *owner_busy;
} cui_cpu_v2_frame_request_proposed;
_Static_assert(sizeof(cui_cpu_v2_frame_proposed) <= 2048, "proposed fixed frame exceeds guard");
#endif
