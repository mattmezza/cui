/* SPDX-License-Identifier: MIT */
/* Production Core test seam: only private sizeof reporting; no algorithm patch. */
#ifdef CUI_SCROLLBAR_TRACK_DISTRIBUTION
#define CUI_IMPLEMENTATION
#include "cui.h"
#else
#include "../../src/core/cui.c"
#endif
#include "scrollbar_track_probe.h"
#include <assert.h>
#include <math.h>
#include <string.h>
_Static_assert(sizeof(prototype_track_record) == sizeof(cui_scrollbar_track_layout_v1),
               "frozen test record size");
/* This independent golden formula was frozen before moving production Base math.
 * Do not replace it with a call to the new Core function. */
static float thumb_positive(float value, float fallback) {
    return isfinite(value) && value > 0 ? value : fallback;
}
static float thumb_limit(float value, float maximum) {
    return isfinite(value) ? fminf(fmaxf(value, 0), maximum) : 0;
}
cui_rect prototype_thumb_geometry(cui_rect track, float scroll, float viewport, float content,
                                  bool vertical, float minimum) {
    float length = vertical ? track.h : track.w;
    if (!isfinite(length) || length <= 0)
        return (cui_rect){0};
    viewport = thumb_positive(viewport, length);
    content = thumb_positive(content, viewport);
    float maximum = fmaxf(0, content - viewport);
    minimum = thumb_positive(minimum, 12);
    float thumb =
        fminf(length, fmaxf(fminf(minimum, length), length * fminf(1, viewport / content)));
    float position = maximum > 0 ? thumb_limit(scroll, maximum) / maximum * (length - thumb) : 0;
    if (vertical) {
        track.y += position;
        track.h = thumb;
    } else {
        track.x += position;
        track.w = thumb;
    }
    return track;
}

bool prototype_scrollbar_track(cui_context *ui, const prototype_track_record *record) {
    if (!record)
        return cui_layout_scrollbar_track_v1(ui, NULL);
    cui_scrollbar_track_layout_v1 copied;
    memcpy(&copied, record, sizeof copied);
    return cui_layout_scrollbar_track_v1(ui, &copied);
}
size_t prototype_track_resource_bytes(void) {
    return sizeof(cui_core_sl_state);
}
size_t prototype_track_node_bytes(void) {
    return sizeof(cui_core_box_node);
}
