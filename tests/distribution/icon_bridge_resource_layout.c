/* SPDX-License-Identifier: MIT */
/* Compile-only native/WASM measurements; no engine or desktop window. */
#include "../../src/bridge/cui_bridge_internal.h"
#include "../reference/bridge_icon_app_dbfb7945.h"
#define OLD_FIELD(f)                                                                               \
    _Static_assert(offsetof(struct bridge_icon_old_app_layout, f) == offsetof(cui_bridge_app, f),  \
                   "old app offset: " #f)
OLD_FIELD(ui);
OLD_FIELD(theme);
OLD_FIELD(root_view);
OLD_FIELD(theme_preset_index);
OLD_FIELD(nodes);
OLD_FIELD(max_nodes);
OLD_FIELD(root);
OLD_FIELD(next_id);
OLD_FIELD(last_core_frame);
OLD_FIELD(events);
OLD_FIELD(event_payloads);
OLD_FIELD(max_events);
OLD_FIELD(event_head);
OLD_FIELD(event_count);
OLD_FIELD(frame_error);
OLD_FIELD(render_disabled);
OLD_FIELD(stepping);
OLD_FIELD(closed);
OLD_FIELD(running);
OLD_FIELD(run_dispatching);
OLD_FIELD(destroy_requested);
OLD_FIELD(width);
OLD_FIELD(height);
OLD_FIELD(now_seconds);
OLD_FIELD(notifications);
OLD_FIELD(notification_entries);
OLD_FIELD(backend);
OLD_FIELD(native);
OLD_FIELD(slider);
OLD_FIELD(time_fields);
#undef OLD_FIELD
const unsigned cui_bridge_icon_resource_layout[] = {sizeof(struct bridge_icon_old_app_layout),
                                                    _Alignof(struct bridge_icon_old_app_layout),
                                                    sizeof(cui_bridge_app),
                                                    _Alignof(cui_bridge_app),
                                                    offsetof(cui_bridge_app, icons),
                                                    sizeof(cui_bridge_icon_handle_v1),
                                                    _Alignof(cui_bridge_icon_handle_v1),
                                                    sizeof(cui_bridge_icon_limits_v1),
                                                    _Alignof(cui_bridge_icon_limits_v1),
                                                    sizeof(cui_bridge_icon_info_v1),
                                                    _Alignof(cui_bridge_icon_info_v1),
                                                    sizeof(cui_bridge_icon_bitmap_v1),
                                                    _Alignof(cui_bridge_icon_bitmap_v1)};
