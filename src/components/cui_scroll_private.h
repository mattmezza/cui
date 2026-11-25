/* SPDX-License-Identifier: MIT */
#ifndef CUI_SCROLL_PRIVATE_H
#define CUI_SCROLL_PRIVATE_H
#include "cui/cui_scroll.h"

/* Vertical-only layout. The supplied width estimates the scrollbar geometry;
 * the viewport and content fill the current horizontal allocation. */
bool cui_scroll_private_begin_vertical_grow(cui_context *, cui_id, const cui_theme *,
                                            cui_scroll_state *, cui_scroll_options);
#endif
