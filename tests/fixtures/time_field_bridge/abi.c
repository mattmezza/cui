/* SPDX-License-Identifier: MIT */
#include "cui/cui_bridge.h"
#include <stddef.h>
_Static_assert(sizeof(cui_bridge_time) == 12, "legacy time");
_Static_assert(sizeof(cui_bridge_node) == 8, "legacy node");
_Static_assert(sizeof(cui_bridge_time_token_v1) == 20, "token");
_Static_assert(sizeof(cui_bridge_time_name_v1) == 68, "name");
_Static_assert(sizeof(cui_bridge_time_field_config_v1) == 616, "config");
_Static_assert(sizeof(cui_bridge_time_field_result_v1) == 80, "result");
_Static_assert(sizeof(cui_bridge_time_field_change_v1) == 64, "change");
_Static_assert(offsetof(cui_bridge_time_field_config_v1, edit_generation) == 56, "generation");
_Static_assert(offsetof(cui_bridge_time_field_config_v1, tokens) == 64, "tokens");
_Static_assert(offsetof(cui_bridge_time_field_config_v1, names) == 344, "names");
_Static_assert(offsetof(cui_bridge_time_field_result_v1, frame_sequence) == 24, "frame");
_Static_assert(offsetof(cui_bridge_time_field_result_v1, previous) == 48, "previous");
_Static_assert(offsetof(cui_bridge_time_field_change_v1, previous) == 32, "change previous");
int main(void) { return 0; }
