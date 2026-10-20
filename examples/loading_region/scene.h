/* SPDX-License-Identifier: MIT */
#ifndef LOADING_REGION_SCENE_H
#define LOADING_REGION_SCENE_H
#include "cui/cui_content.h"
typedef enum { LOADING_REGION_LOADING, LOADING_REGION_READY, LOADING_REGION_FAILED } loading_phase;
typedef struct {
    loading_phase phase;
    bool alert_visible;
    const char *alert_name;
    unsigned retries;
    cui_loading_motion placeholders[3];
} loading_scene;
/* Owner-thread composition; caller owns operation state and all borrowed strings. */
void loading_scene_frame(cui_context *, const cui_theme *, loading_scene *, double now);
#endif
