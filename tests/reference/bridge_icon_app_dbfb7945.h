/* SPDX-License-Identifier: MIT */
/* Frozen app layout authority dbfb7945, original snippet SHA256 7c703d8dcfb666f7c15a254aec461f00e2ff8247d67a2e27bb76e32f5278f415.
 * Only the struct tag is renamed; no full bridge source copy. */
struct bridge_icon_old_app_layout {
    cui_context *ui;
    cui_theme theme;
    cui_root root_view;
    uint32_t theme_preset_index;
    cui_bridge_node_record *nodes;
    uint32_t max_nodes, root;
    uint64_t next_id;
    uint64_t last_core_frame; /* Actual Core sequence captured inside bridge begin. */
    cui_bridge_event *events;
    cui_bridge_event_payload *event_payloads;
    uint32_t max_events, event_head, event_count;
    cui_bridge_status frame_error;
    bool render_disabled;
    bool stepping, closed;
    bool running, run_dispatching, destroy_requested;
    float width, height;
    double now_seconds;
    cui_notification_state *notifications;
    cui_notification_entry *notification_entries;
    cui_bridge_backend backend;
    void *native;
    bridge_slider_app *slider; /* Lazy controlled route; no old node growth. */
    bridge_time_app *time_fields; /* Lazy localized kind55 route. */
};
