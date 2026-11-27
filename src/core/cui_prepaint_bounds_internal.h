/* SPDX-License-Identifier: MIT */
#ifndef CUI_PREPAINT_BOUNDS_INTERNAL_H
#define CUI_PREPAINT_BOUNDS_INTERNAL_H
#include "cui/cui.h"

/* Current, unclipped named box bounds within the paint owner's layer. */
bool cui_core_prepaint_bounds(cui_prepaint_writer *, cui_id, cui_rect *);
#endif
