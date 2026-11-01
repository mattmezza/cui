/* SPDX-License-Identifier: MIT */
#include "cpu_v2_frame_layout_proposal.h"
#include <stddef.h>
/* Object constants allow native/WASM measurement without linking or execution.
 * Keep labels stable for the bounded assembly/object inspection driver. */
#define MEASURE(name, expression) const unsigned cui_frame_layout_##name = (unsigned)(expression)
MEASURE(frame_size, sizeof(cui_cpu_v2_frame_proposed));
MEASURE(frame_alignment, _Alignof(cui_cpu_v2_frame_proposed));
MEASURE(request_size, sizeof(cui_cpu_v2_frame_request_proposed));
MEASURE(state_offset, offsetof(cui_cpu_v2_frame_proposed, state));
MEASURE(self_offset, offsetof(cui_cpu_v2_frame_proposed, self));
MEASURE(workspace_offset, offsetof(cui_cpu_v2_frame_proposed, workspace));
MEASURE(pixels_offset, offsetof(cui_cpu_v2_frame_proposed, pixels));
MEASURE(width_offset, offsetof(cui_cpu_v2_frame_proposed, width));
MEASURE(stride_offset, offsetof(cui_cpu_v2_frame_proposed, stride));
MEASURE(bytes_offset, offsetof(cui_cpu_v2_frame_proposed, bytes));
MEASURE(workspace_size, sizeof(cui_cpu_v2_workspace));
MEASURE(clip_stack_size, 64 * sizeof(cui_cpu_v2_clip));
MEASURE(plan_size, sizeof(cui_cpu_v2_plan));
#ifdef CUI_HAS_TEXT
MEASURE(text_owner_offset, offsetof(cui_cpu_v2_frame_proposed, text_owner));
MEASURE(text_session_offset, offsetof(cui_cpu_v2_frame_proposed, text_session));
MEASURE(text_owner_size, sizeof(cui_text_owner_internal));
MEASURE(text_session_size, sizeof(cui_text_prepare_session));
#endif
