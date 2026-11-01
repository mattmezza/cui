/* SPDX-License-Identifier: MIT */
#ifdef CUI_TABLE_TOUCH_ALL
#ifndef CUI_DIST_ALL_SHARED
#define CUI_IMPLEMENTATION_ALL
#endif
#include "cui_all.h"
#else
#define CUI_IMPLEMENTATION
#include "cui.h"
#define CUI_SCROLL_IMPLEMENTATION
#include "cui_scroll.h"
#define CUI_TABLE_IMPLEMENTATION
#include "cui_table.h"
#endif
#include "cui_table_events.h"
#define CUI_TABLE_TOUCH_DISTRIBUTION
#include "../unit/test_table_touch_admission.c"
