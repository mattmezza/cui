/* SPDX-License-Identifier: MIT */
#ifndef CUI_DIALOG_INTERNAL_H
#define CUI_DIALOG_INTERNAL_H
#include "cui/cui_surface.h"
/* Private route: only legacy Body wheel and reserved action reduction differ. */
bool cui_dialog_surface_update(cui_context *, cui_surface_state *, cui_surface_options);
#endif
