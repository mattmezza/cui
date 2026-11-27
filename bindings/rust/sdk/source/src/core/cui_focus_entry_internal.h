/* SPDX-License-Identifier: MIT */
#ifndef CUI_FOCUS_ENTRY_INTERNAL_H
#define CUI_FOCUS_ENTRY_INTERNAL_H
#include "cui/cui.h"

/* Select among the current scope's eligible Tab stops submitted at or after
 * first_semantic. Run after submitting the content and before layout. */
bool cui_core_focus_enter(cui_context *, cui_id scope, size_t first_semantic);
#endif
