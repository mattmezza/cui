/* SPDX-License-Identifier: MIT */
#ifndef CUI_BRIDGE_INTERNAL_H
#define CUI_BRIDGE_INTERNAL_H
#include "cui/cui_bridge.h"
#include "cui/cui_components.h"
#include "cui/cui_text.h"
#include <stdbool.h>
#include <stddef.h>

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
} cui_bridge_node_record;
typedef struct {
    uint32_t checked;
    double value;
    char *text;
    uint32_t text_length;
    cui_bridge_node selected;
} cui_bridge_event_payload;
struct cui_bridge_app {
    cui_context *ui;
    cui_theme theme;
    cui_bridge_node_record *nodes;
    uint32_t max_nodes, root;
    uint64_t next_id;
    cui_bridge_event *events;
    cui_bridge_event_payload *event_payloads;
    uint32_t max_events, event_head, event_count;
    cui_bridge_status frame_error;
    bool stepping, closed;
    float width, height;
    cui_bridge_backend backend;
    void *native;
};
/* Native functions are implemented only in the optional SDL bridge target. */
cui_bridge_status cui_bridge_native_create(cui_bridge_app *, const char *, const char *,
                                           cui_config *);
void cui_bridge_native_destroy(cui_bridge_app *);
cui_bridge_status cui_bridge_native_poll(cui_bridge_app *);
cui_bridge_status cui_bridge_native_render(cui_bridge_app *, const cui_draw_list *);
#endif
