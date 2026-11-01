/* SPDX-License-Identifier: MIT */
#if defined(CUI_TIME_BRIDGE_WIDGET_FIRST)
#include "cui/cui_time_field_v1.h"
#include "cui/cui_bridge.h"
#else
#include "cui/cui_bridge.h"
#include "cui/cui_time_field_v1.h"
#endif
static_assert(sizeof(cui_bridge_time_field_config_v1) == 616, "config");
static_assert(sizeof(cui_bridge_time_field_result_v1) == 80, "result");
static_assert(sizeof(cui_bridge_time_field_change_v1) == 64, "change");
int main() { return 0; }
