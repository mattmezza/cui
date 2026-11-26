/* SPDX-License-Identifier: MIT */
#ifndef CUI_STATUS_INTERNAL_H
#define CUI_STATUS_INTERNAL_H
#include "cui/cui_content.h"

/* Components can use Marker shimmer with a pre-fitted styled line. */
bool cui_marker_styled(cui_context *, cui_id, const cui_theme *, cui_marker_state *,
                       const cui_marker_child *, size_t, cui_marker_options,
                       const cui_text_style *);
#endif
