/* SPDX-License-Identifier: MIT */
#ifndef CUI_EXAMPLE_BACKGROUND_H
#define CUI_EXAMPLE_BACKGROUND_H
#include "cui/cui_effects.h"
/* Keep one instance per caller; initialize once, then reuse across frames. */
typedef struct {
    unsigned char pixels[128 * 128 * 4];
    uint64_t revision;
} cui_example_background;
bool cui_example_background_init(cui_example_background *background);
bool cui_example_background_paint(cui_context *ui, cui_id id,
                                  const cui_example_background *background, float width,
                                  float height);
#endif
