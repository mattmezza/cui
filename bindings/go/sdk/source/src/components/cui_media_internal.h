/* SPDX-License-Identifier: MIT */
#ifndef CUI_MEDIA_INTERNAL_H
#define CUI_MEDIA_INTERNAL_H

#include "cui/cui_media_events.h"

enum {
    CUI_ATTACHMENT_FOCUSABLE_OPEN = 1u << 0,
    CUI_ATTACHMENT_PERSISTENT_REMOVE = 1u << 1,
    CUI_ATTACHMENT_DISABLED = 1u << 2,
    CUI_ATTACHMENT_RETAINED_SLOTS = 1u << 3
};

/* Retained presentation uses the same renderer and action observer as the
 * immediate API. The interaction policy belongs to the submitter. */
cui_attachment_event_result
cui_attachment_present_events(cui_context *, cui_id, const cui_theme *, cui_attachment_state *,
                              cui_attachment_options, cui_attachment_handler,
                              const cui_attachment_metadata_style *, unsigned interaction_flags);

#endif
