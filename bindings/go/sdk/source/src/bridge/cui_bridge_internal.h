/* SPDX-License-Identifier: MIT */
#ifndef CUI_BRIDGE_INTERNAL_H
#define CUI_BRIDGE_INTERNAL_H
#include "cui/cui_bridge.h"
#include "cui/cui_carousel.h"
#include "cui/cui_chart.h"
#include "cui/cui_command.h"
#include "cui/cui_components.h"
#include "cui/cui_dock.h"
#include "cui/cui_editor.h"
#include "cui/cui_media.h"
#include "cui/cui_message.h"
#include "cui/cui_plot_view.h"
#include "cui/cui_questionnaire.h"
#include "cui/cui_resizable.h"
#include "cui/cui_root.h"
#include "cui/cui_settings.h"
#include "cui/cui_sidebar.h"
#include "cui/cui_speech.h"
#include "cui/cui_table.h"
#include "cui/cui_text.h"
#include "cui/cui_text_view.h"
#include "cui/cui_virtual_scroll.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct bridge_text_view_owned bridge_text_view_owned;
typedef struct bridge_chart_owned bridge_chart_owned;
typedef struct bridge_speech_owned bridge_speech_owned;
typedef struct bridge_questionnaire_owned bridge_questionnaire_owned;
typedef struct bridge_dock_owned bridge_dock_owned;
typedef struct bridge_settings_owned bridge_settings_owned;
typedef struct bridge_data_table_owned bridge_data_table_owned;
typedef struct {
    uint32_t generation;
    bool live;
    cui_bridge_kind kind;
    uint32_t parent, first_child, last_child, next_sibling;
    cui_id id;
    char *key, *text, *detail;
    float gap, custom_radius;
    cui_bridge_button_variant variant;
    cui_bridge_button_size size;
    cui_bridge_button_rounding rounding;
    bool disabled, custom_hover;
    cui_color hover;
    cui_button_state button_state;
    cui_checkbox_state checkbox_state;
    cui_switch_state switch_state;
    cui_hover_state toggle_state;
    bool checked;
    double value, minimum, maximum, step;
    cui_edit *edit;
    char *edit_buffer;
    cui_input_state input_state;
    cui_textarea_state textarea_state;
    cui_textarea_line *textarea_lines;
    cui_number_state number_state;
    cui_radio_group_state radio_group_state;
    cui_choice_popup choice_popup;
    cui_menu_tree_state menu_state;
    cui_surface_state surface_state;
    cui_bridge_node anchor;
    cui_hover_state hover_state;
    cui_marker_state marker_state;
    cui_shimmer_state shimmer_state;
    float visual_width, visual_height;
    cui_list_state list_state;
    uint32_t total;
    uint32_t step_status;
    cui_scroll_state scroll_state;
    float content_width, content_height;
    cui_title_bar_state title_bar_state;
    cui_attachment_state attachment_state;
    cui_bridge_attachment_status attachment_status;
    cui_image_state image_state;
    cui_image_frame image_frame;
    unsigned char *image_pixels;
    cui_tree_model *tree_model;
    cui_tree_node *tree_nodes;
    bool *tree_expanded;
    cui_tree_state tree_state;
    cui_time_state time_state;
    cui_time time;
    cui_color_state color_state;
    cui_color color;
    cui_date_selection date_selection;
    cui_calendar_state calendar_state;
    cui_date_picker_state date_picker_state;
    cui_otp_state otp_state;
    char *otp_buffer;
    cui_input_group_state input_group_state;
    cui_message_scroller_state *message_scroller;
    uint32_t message_count;
    cui_carousel_state *carousel_state;
    cui_carousel_geometry *carousel_geometry;
    cui_resizable_state *resizable_state;
    cui_resizable_slot *resizable_slots;
    bridge_text_view_owned *text_view;
    cui_editor_state editor_state;
    bridge_chart_owned *chart;
    bridge_data_table_owned *data_table;
    uint32_t table_selection_mode;
    cui_plot_sample *plot_samples;
    uint32_t plot_count;
    cui_command_state command_state;
    cui_command_row *command_rows;
    cui_command_entry *command_entries;
    cui_str *command_keywords;
    uint32_t *command_slots;
    cui_virtual_list *virtual_list;
    cui_virtual_widget_state virtual_state;
    cui_sidebar_state sidebar_state;
    cui_sidebar_cache *sidebar_cache;
    cui_sidebar_item_state sidebar_item_state;
    bridge_speech_owned *speech;
    bridge_questionnaire_owned *questionnaire;
    bridge_dock_owned *dock;
    bridge_settings_owned *settings;
    cui_bridge_app *owner;
    uint32_t own_slot;
} cui_bridge_node_record;
typedef struct {
    uint32_t node_kind, payload_kind;
    cui_bridge_notification_value_v1 notification;
    uint32_t checked;
    double value;
    char *text;
    uint32_t text_length;
    cui_bridge_node selected;
} cui_bridge_event_payload;
struct cui_bridge_app {
    cui_context *ui;
    cui_theme theme;
    cui_root root_view;
    uint32_t theme_preset_index;
    cui_bridge_node_record *nodes;
    uint32_t max_nodes, root;
    uint64_t next_id;
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
};
/* Native functions are implemented only in the optional SDL bridge target. */
cui_bridge_status cui_bridge_native_create(cui_bridge_app *, const char *, const char *,
                                           cui_config *);
void cui_bridge_native_destroy(cui_bridge_app *);
cui_bridge_status cui_bridge_native_run(cui_bridge_app *, cui_bridge_run_callback, void *);
cui_bridge_status cui_bridge_native_poll(cui_bridge_app *);
cui_bridge_status cui_bridge_native_render(cui_bridge_app *, const cui_draw_list *);
cui_bridge_status cui_bridge_native_set_font_fallbacks(cui_bridge_app *, const char *const *,
                                                       uint32_t);
cui_bridge_status cui_bridge_native_clipboard_set(cui_bridge_app *, cui_str);
cui_bridge_status cui_bridge_native_clipboard_get(cui_bridge_app *, char *, uint32_t, uint32_t *);
#endif
