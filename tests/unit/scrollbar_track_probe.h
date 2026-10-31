/* SPDX-License-Identifier: MIT */
#ifndef CUI_TEST_SCROLLBAR_TRACK_PROBE_H
#define CUI_TEST_SCROLLBAR_TRACK_PROBE_H
#include "cui/cui.h"
/* Frozen independent test record; adapter copies to the actual public record. */
enum { PROTOTYPE_TRACK_VISIBLE = 1, PROTOTYPE_TRACK_INTERACTIVE = 2 };
typedef struct {
    uint32_t struct_size, version, flags, reserved;
    cui_id viewport, bar, thumb;
    float thickness, minimum_thumb;
} prototype_track_record;
bool prototype_scrollbar_track(cui_context *, const prototype_track_record *);
cui_rect prototype_thumb_geometry(cui_rect, float, float, float, bool, float);
size_t prototype_track_resource_bytes(void);
size_t prototype_track_node_bytes(void);
#endif
