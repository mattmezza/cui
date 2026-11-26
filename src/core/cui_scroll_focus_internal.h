/* SPDX-License-Identifier: MIT */
#ifndef CUI_SCROLL_FOCUS_INTERNAL_H
#define CUI_SCROLL_FOCUS_INTERNAL_H
#include "cui/cui.h"

/* Use the last completed semantic layout. Returns untrimmed bounds when
 * visible focus newly enters a live layout descendant of this viewport. */
bool cui_core_new_focus_bounds(const cui_context *, cui_id viewport, cui_rect *);
#endif
