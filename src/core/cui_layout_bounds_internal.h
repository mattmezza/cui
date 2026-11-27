/* SPDX-License-Identifier: MIT */
#ifndef CUI_LAYOUT_BOUNDS_INTERNAL_H
#define CUI_LAYOUT_BOUNDS_INTERNAL_H
#include "cui/cui.h"

/* Final named box from the latest successful layout only. The box need not
 * have semantics. Missing, inert and layout-only boxes produce no bounds. */
bool cui_core_completed_box_bounds(cui_context *, cui_id, cui_rect *);
#endif
