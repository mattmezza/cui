/* SPDX-License-Identifier: MIT */
#ifndef CUI_SDL3_TIMED_FRAMES_INTERNAL_H
#define CUI_SDL3_TIMED_FRAMES_INTERNAL_H

#include "cui/cui_sdl3.h"

/* During a callback frame, request another iteration within the Notification
 * manager's 50 ms idle cadence. Redraw demand can still run at native cadence.
 * The request expires if the next frame leaves it unset. */
void cui_sdl3_request_timed_frame(cui_sdl3 *);

#endif
