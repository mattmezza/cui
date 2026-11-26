/* SPDX-License-Identifier: MIT */
#ifndef CUI_COLUMN_WIDTH_INTERNAL_H
#define CUI_COLUMN_WIDTH_INTERNAL_H
#include "cui/cui.h"

/* Inner width of the open, fixed-width, unclipped column. Zero when the
 * available width depends on content, sibling allocation or overflow. */
float cui_core_fixed_column_inner_width(cui_context *);
#endif
