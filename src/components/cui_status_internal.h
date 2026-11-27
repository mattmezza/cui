/* SPDX-License-Identifier: MIT */
#ifndef CUI_STATUS_INTERNAL_H
#define CUI_STATUS_INTERNAL_H
#include "cui/cui_content.h"

/* Components can use Marker shimmer with a pre-fitted styled line. */
bool cui_marker_styled(cui_context *, cui_id, const cui_theme *, cui_marker_state *,
                       const cui_marker_child *, size_t, cui_marker_options,
                       const cui_text_style *);
/* Internal retained action slot. action_user marks cards with an attached
 * Button. Its width and inherited opacity are established by the renderer. */
typedef void (*cui_notification_retained_action_fn)(void *user, float width);
bool cui_notifications_retained(cui_context *, cui_id, const cui_theme *, cui_notification_state *,
                                cui_rect, double, bool responsive,
                                cui_notification_retained_action_fn, cui_notification_advance *);
#endif
