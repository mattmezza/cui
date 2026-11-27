/* SPDX-License-Identifier: MIT */
#ifndef CUI_ROOT_INTERNAL_H
#define CUI_ROOT_INTERNAL_H
#include "cui/cui_root.h"
bool cui_root_dialog_stack_valid(const cui_root *);
bool cui_root_dialog_register(cui_root *, cui_root_surface);
#endif
