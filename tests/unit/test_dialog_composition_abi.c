/* SPDX-License-Identifier: MIT */
/* Portable metadata-only compilation oracle, no window or Core implementation. */
#include "../../src/base/cui_dialog_composition_private.h"
#include "cui/cui_dialog_composition.h"
#include <stddef.h>
_Static_assert(sizeof(cui_dialog_slot_style_v1) == 32, "slot style32");
_Static_assert(sizeof(cui_dialog_refinement_v1) == 232, "refinement232");
_Static_assert(sizeof(cui_dialog_sizing_v1) == 96, "sizing96");
_Static_assert(offsetof(cui_dialog_composition_v1, omit) == 12, "omit12");
_Static_assert(offsetof(cui_dialog_sizing_v1, content_sizing) == 32, "content policy32");
_Static_assert(offsetof(cui_dialog_sizing_v1, body_sizing) == 56, "body policy56");
_Static_assert(offsetof(cui_dialog_sizing_v1, placement) == 80, "placement80");
_Static_assert(offsetof(cui_dialog_sizing_v1, layer_offset) == 92, "layer offset92");
#if UINTPTR_MAX == UINT64_MAX
_Static_assert(sizeof(cui_dialog_slot) == 16, "LP64 callback16");
_Static_assert(sizeof(cui_dialog_composition_v1) == 152, "LP64 composition152");
_Static_assert(sizeof(cui_dialog_layout_v1) == 208, "LP64 layout208");
_Static_assert(offsetof(cui_dialog_layout_v1, completed) == 40, "LP64 retained40");
_Static_assert(offsetof(cui_dialog_layout_v1, private_submit_frame) == 200, "LP64 sequence200");
#else
_Static_assert(sizeof(cui_dialog_slot) == 8, "ILP32 callback8");
_Static_assert(sizeof(cui_dialog_composition_v1) == 84, "ILP32 composition84");
_Static_assert(sizeof(cui_dialog_layout_v1) == 200, "ILP32 layout200");
_Static_assert(offsetof(cui_dialog_layout_v1, completed) == 32, "ILP32 retained32");
_Static_assert(offsetof(cui_dialog_layout_v1, private_submit_frame) == 192, "ILP32 sequence192");
#endif
const unsigned cui_dialog_candidate_sizes[] = {
    sizeof(cui_dialog_slot_style_v1),  sizeof(cui_dialog_refinement_v1),
    sizeof(cui_dialog_sizing_v1),      sizeof(cui_dialog_slot),
    sizeof(cui_dialog_composition_v1), sizeof(cui_dialog_layout_v1),
    sizeof(cui_dialog_frame),          sizeof(cui_dialog_resolved),
    sizeof(cui_scroll_state)};
