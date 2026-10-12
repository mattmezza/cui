/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
#include "cui/cui_components.h"
#include "cui/cui_content.h"
#include "cui/cui_disclosure.h"
#include "cui/cui_dock.h"
#include "cui/cui_form.h"
#include "cui/cui_message.h"
#include "cui/cui_numeric.h"
#include "cui/cui_otp.h"
#include "cui/cui_picker.h"
#include "cui/cui_plot.h"
#include "cui/cui_scroll.h"
#include "cui/cui_status.h"
#include "cui/cui_surface.h"
#include "cui/cui_table.h"
#include "cui/cui_virtual.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__wasm__)
__attribute__((import_module("host"), import_name("measure"))) extern float
host_measure(const char *, size_t, float);
#else
static float host_measure(const char *s, size_t n, float size) {
    (void)s;
    return n * size * .6f;
}
#endif
static cui_context *ui;
static cui_draw_list draw;
static const cui_semantic *semantics;
static size_t semantic_count;
static bool checked, enabled = true, toggled;
static cui_check_state check_state;
static const unsigned char image_story_pixels[80] = {
    255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255, 0,0,0,0,
    255,255,255,255, 200,100,0,255, 200,100,0,255, 255,255,255,255, 0,0,0,0,
    255,255,255,255, 200,100,0,255, 200,100,0,255, 255,255,255,255, 0,0,0,0,
    255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255, 0,0,0,0};
static int radio, clicks;
static bool alert_visible = true, open[3] = {true, false, false}, collapsed;
static int dropdown_variant, dropdown_size, dropdown_halves, dropdown_action, dropdown_anchor;
static bool dropdown_outline, dropdown_selected;
static cui_id selected_tab;
static int tabs_mode, tabs_size, tabs_variant = CUI_TAB_UNDERLINE;
static bool tabs_manual;
static cui_tab_scroll_state tabs_scroll;
static cui_tab_overflow tabs_overflow;
static cui_menu_entry tabs_entries[12];
static int disclosure_mode, disclosure_actions;
static bool disclosure_multiple, disclosure_checked;
static cui_disclosure_motion accordion_motion[3], collapsible_motion;
static cui_theme current_theme;
static void disclosure_header(cui_context *context, const cui_disclosure_item *item, size_t index,
                              bool expanded, void *user) {
    (void)index;
    (void)user;
    cui_text(context, cui_string(expanded ? "-" : "+"), current_theme.font_size,
             current_theme.primary);
    cui_text(context, item->label, current_theme.font_size,
             item->disabled ? current_theme.muted : current_theme.text);
    cui_text(context, cui_string(" (custom)"), current_theme.font_size, current_theme.muted);
}

static int message_alignment, message_variant = CUI_BUBBLE_SECONDARY, message_inset;
static bool message_avatar = true, message_acknowledged, message_scrollbar = true;
static unsigned message_actions, conversation_serial = 80, conversation_count = 80;
static unsigned conversation_ids[256], conversation_rendered;
static char conversation_names[256][32], conversation_texts[256][64];
static int conversation_mutation;
static cui_message_scroller_state *conversation;
static cui_message_scroller_result conversation_result;
static void message_rich(cui_context *context, cui_id id, void *user, cui_message_style style) {
    (void)user;
    cui_text(context, cui_string("Workspace is ready."), style.font_size, style.color);
    cui_checkbox(context, id ^ 1, &current_theme, cui_string("Acknowledge message"),
                 &message_acknowledged, false);
    if (cui_button(context, id ^ 2, &current_theme, cui_string("Like message"), false))
        message_actions++;
}
static void conversation_labels(void) {
    for (unsigned i = 0; i < conversation_count; i++) {
        snprintf(conversation_names[i], sizeof(conversation_names[i]), "Message %u",
                 conversation_ids[i]);
        snprintf(conversation_texts[i], sizeof(conversation_texts[i]), "Workspace update %u",
                 conversation_ids[i]);
    }
}
static void conversation_row(cui_context *context, cui_id id, size_t index, void *user,
                             float width) {
    (void)user;
    conversation_rendered++;
    cui_message_body bodies[2] = {
        {.slot = {.present = true, .text = cui_string(conversation_texts[index])},
         .typed_bubble = true,
         .bubble = conversation_ids[index] % 2 ? CUI_BUBBLE_TINTED : CUI_BUBBLE_SECONDARY},
        {.slot = {.present = true, .text = cui_string("Additional application-owned detail")}}};
    cui_message_options options = cui_message_options_default();
    options.width = width;
    options.role = CUI_ROLE_LISTITEM;
    options.name = cui_string(conversation_names[index]);
    options.alignment = conversation_ids[index] % 2 ? CUI_MESSAGE_END : CUI_MESSAGE_START;
    options.header = (cui_message_slot){.present = true, .text = cui_string("Alex Morgan")};
    options.body = bodies;
    options.body_count = conversation_ids[index] % 3 ? 1 : 2;
    options.footer = (cui_message_slot){.present = true, .text = cui_string("Delivered")};
    cui_message(context, id, &current_theme, options);
}
static cui_hover_state tooltip_state, card_state;
static cui_virtual_list *virtual_list;
static float scroll_percent;
static cui_dock *dock;
static cui_date date = {2026, 10, 12};
static cui_calendar_state calendar;
static cui_date_selection date_selection = {{2026, 10, 12}, {0}};
static cui_date_picker_state date_picker;
static cui_time time_value = {9, 30, 0};
static cui_time_state time_state;
static bool multiple_mode, multi_selected[4];
static cui_color chosen_color = {35, 102, 80, 255};
static cui_color_state color_state;
static cui_plot_scale plot_scale = {CUI_PLOT_LINEAR, 0, 100, 0, 300, false};
static bool redraw;
static float host_seconds, host_dt = 1.0f / 60;
static bool reduced_motion, compact_density;
static cui_overlay trap;
static int clipboard_request;
static char clipboard_status[128] = "Clipboard access requires a secure browser context.";
static cui_choice_popup select_popup, menu_popup, combo_popup;
static cui_menu_tree_state menu_tree, context_menu;
static bool menu_checked;
static int menu_radio;
static cui_edit *query;
static char query_buffer[128];
static cui_id combo_selection;
static cui_list_state list_state;
static cui_id list_selection, select_selection, menu_action;
static int menu_invocations, previous_story = -1;
static char number_buffer[128] = "42";
static cui_number_state number_state;
static double number = 42;
static size_t page_number = 3, current_step;
static float rating = 3;
static cui_id dock_serial = 3;
static void panel(cui_context *context, size_t index, void *user) {
    (void)user;
    char text[64];
    snprintf(text, sizeof(text), "Content for item %zu", index + 1);
    cui_label(context, &current_theme, cui_string(text));
}
static void disclosure_panel(cui_context *context, size_t index, void *user) {
    (void)user;
    cui_checkbox(context, cui_id_from(context, "panel-updates") ^ (index + 1), &current_theme,
                 cui_string("Panel updates"), &disclosure_checked, false);
    if (cui_button(context, cui_id_from(context, "panel-apply") ^ (index + 1), &current_theme,
                   cui_string("Apply panel"), false))
        disclosure_actions++;
    cui_label(context, &current_theme, cui_string("Application-owned interactive content."));
}
static float slider = 40;
static char input_buffer[512] = "Hello, CUI";
static cui_edit *edit;
static cui_input_state input_state;
static int input_mode;
static int form_mode, form_submissions;
static int group_mode, group_actions;
static cui_otp_state otp_state;
static char otp_buffer[32];
static int otp_completions, otp_groups = 2;
static double next_wake;
static cui_shimmer_state shimmer_state;
static int shimmer_mode;
static float shimmer_started;
static bool otp_masked;
static int tag_variant, tag_color = CUI_STATUS_COLOR_BLUE;
static bool tag_outline, tag_pill;
static int marker_variant, marker_loading, marker_alignment;
static bool marker_icon;
static cui_marker_state marker_state;
static cui_notification_state notification_state;
static cui_notification_entry notification_entries[8];
static char notification_titles[8][40];
static bool notification_pins[8], notification_autohide = true;
static int notification_kind = CUI_NOTIFICATION_INFO, notification_pushes;
static bool notification_clear;
static unsigned notification_serial, notification_clicks, notification_actions, notification_closes;
static void notification_clicked(void *user, cui_notification_id id) {
    (void)user;
    (void)id;
    notification_clicks++;
}
static void notification_action(void *user, cui_notification_id id) {
    (void)user;
    (void)id;
    notification_actions++;
}
static void notification_closed(void *user, cui_notification_id id) {
    (void)user;
    (void)id;
    notification_closes++;
}
static void notification_content(cui_context *context, cui_id id, void *user, float font,
                                 cui_color color, float opacity) {
    (void)font;
    cui_theme theme = current_theme;
    theme.text = color;
    cui_color *colors[] = {&theme.surface, &theme.background, &theme.border, &theme.focus,
                           &theme.primary, &theme.on_primary, &theme.muted};
    for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); i++)
        colors[i]->a = (uint8_t)(colors[i]->a * opacity);
    cui_checkbox(context, id, &theme, cui_string("Pin alert"), user, false);
}
static void notification_push(void) {
    size_t slot = 8;
    for (size_t candidate = 0; candidate < 8; candidate++) {
        bool used = false;
        for (size_t i = 0; i < notification_state.count; i++)
            used |= notification_state.entries[i].notification.title.data ==
                    notification_titles[candidate];
        if (!used) {
            slot = candidate;
            break;
        }
    }
    if (slot == 8)
        return;
    notification_serial++;
    notification_pins[slot] = false;
    snprintf(notification_titles[slot], sizeof(notification_titles[slot]), "Workspace update %u",
             notification_serial);
    cui_notification descriptor = cui_notification_default();
    descriptor.id = (cui_notification_id){1, notification_serial};
    descriptor.title = cui_string(notification_titles[slot]);
    descriptor.message = cui_string("Tasks are ready.");
    descriptor.action_label = cui_string("Review");
    descriptor.kind = (cui_notification_kind)notification_kind;
    descriptor.autohide = notification_autohide;
    descriptor.content = notification_content;
    descriptor.content_user = &notification_pins[slot];
    descriptor.on_click = notification_clicked;
    descriptor.on_action = notification_action;
    descriptor.on_close = notification_closed;
    cui_notification_push(&notification_state, descriptor, host_seconds);
}
static void marker_content(cui_context *context, cui_id id, void *user, float font, cui_color color,
                           float opacity) {
    (void)id;
    (void)user;
    color.a = (uint8_t)(color.a * opacity);
    cui_text(context, cui_string("3 tasks"), font, color);
}
static void marker_icon_content(cui_context *context, cui_id id, void *user, float font,
                                cui_color color, float opacity) {
    (void)user;
    (void)font;
    color.a = (uint8_t)(color.a * opacity);
    cui_begin_box(context, id,
                  (cui_box){.width = 10, .height = 10, .radius = 5, .background = color});
    cui_end_box(context);
}
static char group_buffer[128] = "cui";
static cui_edit *group_edit;
static cui_input_group_state group_state;
static char form_name_buffer[128] = "Ada Lovelace";
static char form_email_buffer[128] = "ada@example.com";
static cui_edit *form_name_edit, *form_email_edit;
static cui_input_state form_name_state, form_email_state;
static char form_status[96] = "Values and validation belong to the application.";
typedef struct {
    cui_id id;
    cui_edit *edit;
    cui_input_state *state;
    const char *name;
    bool disabled, readonly, invalid;
} gallery_form_control;
static bool form_email_valid(void) {
    return strchr(form_email_buffer, '@') && strchr(form_email_buffer, '.');
}
static void form_control(cui_context *context, const cui_theme *theme, void *user, float width) {
    gallery_form_control *control = user;
    cui_input_result result = cui_input_ex(context, control->id, theme, cui_string(control->name),
                                           control->edit, control->state,
                                           (cui_input_options){.width = width,
                                                               .disabled = control->disabled,
                                                               .readonly = control->readonly,
                                                               .required = true,
                                                               .invalid = control->invalid});
    if (result.changed)
        redraw = true;
}
static void form_footer(cui_context *context, const cui_theme *theme, void *user, float width) {
    bool disabled = *(bool *)user;
    (void)width;
    if (cui_button(context, cui_id_from(context, "form-save"), theme, cui_string("Save profile"),
                   disabled)) {
        if (form_name_buffer[0] && form_email_valid()) {
            form_submissions++;
            snprintf(form_status, sizeof(form_status), "Profile saved. Submissions: %d",
                     form_submissions);
        } else
            snprintf(form_status, sizeof(form_status), "Correct the highlighted fields.");
    }
}
static cui_scroll_state scroll_state;
static cui_table_model *table;
static cui_table_state table_state;
static cui_table_selection table_selection[64];
static bool table_pinned;
static int table_mode, table_row_hooks, table_inspections;
static cui_table_selection_mode table_selection_mode = CUI_TABLE_SELECT_CELL;
static cui_table_row table_rows[1000];
static cui_table_column table_columns[3];
static char table_names[1000][24], table_sizes[1000][24];
static cui_surface_state dialog_surface, alert_dialog_surface, sheet_surface, popover_surface,
    nested_surface;
static int surface_variant, alert_attempts;
static bool dialog_updates = true, popover_updates;
static bool alert_decision(void *user, cui_surface_result result) {
    int *attempts = user;
    if (result != CUI_SURFACE_ACCEPTED)
        return true;
    (*attempts)++;
    redraw = true;
    return *attempts > 1;
}
static cui_str table_label(void *user, size_t row, cui_id column) {
    (void)user;
    if (row >= 1000)
        return cui_string("");
    return cui_string(column == 101   ? table_names[row]
                      : column == 102 ? "Ready"
                                      : table_sizes[row]);
}
static void table_row_hook(cui_context *context, const cui_theme *theme, void *user, size_t row) {
    (void)context;
    (void)theme;
    (void)user;
    (void)row;
    table_row_hooks++;
}
static void table_trailing(cui_context *context, const cui_theme *theme, void *user, bool header,
                           size_t row) {
    (void)user;
    if (header)
        cui_label(context, theme, cui_string("Action"));
    else if (cui_button_ex(
                 context, cui_id_from(context, "inspect-row") + row, theme, cui_string("Inspect"),
                 (cui_button_options){.size = CUI_CONTROL_SMALL, .variant = CUI_BUTTON_GHOST}))
        table_inspections++;
}
static void table_empty(cui_context *context, const cui_theme *theme, void *user) {
    (void)user;
    cui_label(context, theme, cui_string("No files in this table."));
}
static void table_loading(cui_context *context, const cui_theme *theme, void *user) {
    (void)user;
    cui_label(context, theme, cui_string("Loading file metadata…"));
}
static cui_tree_model *tree;
static cui_tree_state tree_state;
static cui_id tree_selected;
static cui_tree_node tree_nodes[6];
static bool tree_expanded[6] = {true, true, false, false, false, false};
static cui_size measure(void *user, cui_str text, float size) {
    (void)user;
    return (cui_size){host_measure(text.data, text.length, size), size * 1.25f};
}
int gallery_init(void) {
    if (ui)
        return 1;
    ui = cui_create((cui_config){.width = 720,
                                 .height = 300,
                                 .max_elements = 512,
                                 .max_commands = 2048,
                                 .frame_bytes = 65536,
                                 .measure = measure});
    edit = cui_edit_create((cui_edit_config){.buffer = input_buffer,
                                             .capacity = sizeof(input_buffer),
                                             .history_bytes = 8192,
                                             .max_history_steps = 64});
    form_name_edit = cui_edit_create((cui_edit_config){.buffer = form_name_buffer,
                                                       .capacity = sizeof(form_name_buffer),
                                                       .history_bytes = 2048,
                                                       .max_history_steps = 16});
    form_email_edit = cui_edit_create((cui_edit_config){.buffer = form_email_buffer,
                                                        .capacity = sizeof(form_email_buffer),
                                                        .history_bytes = 2048,
                                                        .max_history_steps = 16});
    cui_notification_settings notification_settings = cui_notification_settings_default();
    notification_settings.width = 330;
    notification_settings.margin_top = 164;
    notification_settings.max_visible = 4;
    if (!cui_notification_init(&notification_state, notification_entries, 8, notification_settings,
                               (cui_notification_host){0}))
        return 0;
    conversation = cui_message_scroller_create((cui_message_scroller_config){256, 80, 96});
    if (!conversation)
        return 0;
    for (unsigned i = 0; i < conversation_count; i++)
        conversation_ids[i] = i + 1;
    conversation_labels();
    if (cui_otp_init(&otp_state, otp_buffer, sizeof(otp_buffer), 6) != CUI_EDIT_OK)
        return 0;
    group_edit = cui_edit_create((cui_edit_config){.buffer = group_buffer,
                                                   .capacity = sizeof(group_buffer),
                                                   .history_bytes = 2048,
                                                   .max_history_steps = 16});
    virtual_list = cui_virtual_create((cui_virtual_config){100000, 24});
    uint64_t *keys = malloc(100000 * sizeof(*keys));
    if (!keys || !virtual_list)
        return 0;
    for (size_t i = 0; i < 100000; i++)
        keys[i] = i + 1;
    cui_virtual_result rebuilt = cui_virtual_rebuild(virtual_list, keys, NULL, 100000);
    free(keys);
    dock = cui_dock_create((cui_dock_config){.node_capacity = 15, .panel_capacity = 16});
    if (dock) {
        cui_dock_insert(dock, cui_dock_root(dock), 1, 0);
        cui_dock_insert(dock, cui_dock_root(dock), 2, 1);
    }
    table = cui_table_create(1000);
    table_state.selection = table_selection;
    table_state.selection_capacity = 64;
    for (size_t i = 0; i < 1000; i++) {
        snprintf(table_names[i], sizeof(table_names[i]), "File %04zu", i + 1);
        snprintf(table_sizes[i], sizeof(table_sizes[i]), "%zu KB", i + 1);
        table_rows[i] = (cui_table_row){.key = i + 1, .label = cui_string(table_names[i])};
    }
    const char *column_names[] = {"Name", "Status", "Size"};
    for (size_t i = 0; i < 3; i++)
        table_columns[i] = (cui_table_column){.key = 101 + i,
                                              .label = cui_string(column_names[i]),
                                              .width = 150,
                                              .minimum = 80,
                                              .maximum = 300,
                                              .sortable = true,
                                              .resizable = true,
                                              .movable = true};
    if (!table || cui_table_bind(table, table_rows, 1000, table_columns, 3) != CUI_TABLE_OK)
        return 0;
    tree = cui_tree_create(6);
    const char *tree_labels[6] = {"Workspace", "src", "core.c", "base.c", "docs", "README.md"};
    const unsigned tree_depths[6] = {0, 1, 2, 2, 1, 2};
    for (size_t i = 0; i < 6; i++)
        tree_nodes[i] = (cui_tree_node){
            .key = i + 1, .label = cui_string(tree_labels[i]), .depth = tree_depths[i]};
    if (!tree || cui_tree_rebuild(tree, tree_nodes, tree_expanded, 6) != CUI_TREE_OK)
        return 0;
    number_state.edit = cui_edit_create((cui_edit_config){.buffer = number_buffer,
                                                          .capacity = sizeof(number_buffer),
                                                          .history_bytes = 2048,
                                                          .max_history_steps = 16});
    cui_calendar_init(&calendar, date);
    cui_date_picker_init(&date_picker, cui_id_from(ui, "date-popup"), date);
    cui_overlay_init(&trap, cui_id_from(ui, "focus-trap"), true);
    cui_choice_popup_init(&select_popup, cui_id_from(ui, "select-popup"));
    query = cui_edit_create((cui_edit_config){.buffer = query_buffer,
                                              .capacity = sizeof(query_buffer),
                                              .history_bytes = 2048,
                                              .max_history_steps = 16});
    cui_choice_popup_init(&combo_popup, cui_id_from(ui, "combo-popup"));
    cui_choice_popup_init(&menu_popup, cui_id_from(ui, "menu-popup"));
    cui_menu_tree_init(&menu_tree, cui_id_from(ui, "nested-menu-popup"));
    cui_menu_tree_init(&context_menu, cui_id_from(ui, "context-menu-popup"));
    cui_surface_init(&dialog_surface, cui_id_from(ui, "dialog-surface"), CUI_SURFACE_DIALOG);
    cui_surface_init(&nested_surface, cui_id_from(ui, "nested-surface"), CUI_SURFACE_DIALOG);
    cui_surface_init(&alert_dialog_surface, cui_id_from(ui, "alert-dialog-surface"),
                     CUI_SURFACE_ALERT);
    cui_surface_init(&sheet_surface, cui_id_from(ui, "sheet-surface"), CUI_SURFACE_SHEET);
    cui_surface_init(&popover_surface, cui_id_from(ui, "popover-surface"), CUI_SURFACE_POPOVER);
    return ui && edit && group_edit && form_name_edit && form_email_edit && dock && query &&
           number_state.edit && rebuilt == CUI_VIRTUAL_OK;
}
void gallery_event(int type, float x, float y, int key, unsigned mods, int repeat, int button) {
    if (ui)
        cui_queue_event(ui, (cui_event){.type = (cui_event_type)type,
                                        .x = x,
                                        .y = y,
                                        .key = (cui_key)key,
                                        .modifiers = mods,
                                        .repeat = repeat != 0,
                                        .pointer_button = (cui_pointer_button)button});
}
void gallery_scroll(float x, float y, float dx, float dy) {
    if (ui)
        cui_queue_event(ui,
                        (cui_event){.type = CUI_EVENT_SCROLL, .x = x, .y = y, .dx = dx, .dy = dy});
}
void gallery_text(const char *text, size_t length) {
    cui_event event = {.type = CUI_EVENT_TEXT};
    if (length > 63 || !text)
        return;
    memcpy(event.text, text, length);
    cui_queue_event(ui, event);
}
void gallery_focus(unsigned index) {
    if (index < semantic_count)
        cui_focus(ui, semantics[index].id);
    else
        cui_focus(ui, 0);
}
int gallery_frame(int story, int theme, int disabled, float width, float height) {
    if (!ui || width < 1 || height < 1)
        return 0;
    cui_theme t = theme == 1   ? cui_theme_dark()
                  : theme == 2 ? cui_theme_high_contrast()
                               : cui_theme_light();
    if (compact_density) {
        t.font_size *= .875f;
        t.control_height *= .8f;
        t.padding *= .7f;
        t.gap *= .75f;
    }
    if (previous_story != story) {
        if (story == 59)
            shimmer_started = host_seconds;
        cui_overlay_close(ui, &select_popup.overlay);
        cui_overlay_close(ui, &menu_popup.overlay);
        cui_overlay_close(ui, &menu_tree.overlay);
        cui_overlay_close(ui, &context_menu.overlay);
        cui_overlay_close(ui, &combo_popup.overlay);
        cui_overlay_close(ui, &date_picker.overlay);
        cui_overlay_close(ui, &trap);
        cui_overlay_close(ui, &dialog_surface.overlay);
        cui_overlay_close(ui, &nested_surface.overlay);
        cui_overlay_close(ui, &alert_dialog_surface.overlay);
        cui_overlay_close(ui, &sheet_surface.overlay);
        cui_overlay_close(ui, &popover_surface.overlay);
        previous_story = story;
    }
    redraw = false;
    next_wake = 0;
    current_theme = t;
    cui_begin_frame(ui, width, height, host_dt);
    cui_begin_box(ui, cui_id_from(ui, "preview"),
                  (cui_box){.width = width,
                            .height = height,
                            .padding = 24,
                            .gap = 16,
                            .background = t.background});
    switch (story) {
    case 0: {
        char label[64];
        if (cui_button(ui, cui_id_from(ui, "button"), &t, cui_string("Run action"), disabled))
            clicks++;
        snprintf(label, sizeof(label), "Clicked %d times", clicks);
        cui_label(ui, &t, cui_string(label));
        for (int row = 0; row < 2; row++) {
            cui_begin_box(ui, cui_id_from(ui, row ? "variant-row-2" : "variant-row-1"),
                          (cui_box){.row = true, .gap = 8});
            const char *names[6] = {"Secondary", "Outline", "Ghost", "Danger", "Link", "Loading"};
            cui_button_variant variants[6] = {CUI_BUTTON_SECONDARY, CUI_BUTTON_OUTLINE,
                                              CUI_BUTTON_GHOST,     CUI_BUTTON_DANGER,
                                              CUI_BUTTON_LINK,      CUI_BUTTON_PRIMARY};
            for (int column = 0; column < 3; column++) {
                int v = row * 3 + column;
                cui_button_ex(ui, cui_id_from(ui, names[v]), &t, cui_string(names[v]),
                              (cui_button_options){.variant = variants[v],
                                                   .size = CUI_CONTROL_SMALL,
                                                   .disabled = disabled || v == 2,
                                                   .loading = v == 5});
            }
            cui_end_box(ui);
        }
        break;
    }
    case 1:
        cui_checkbox_ex(ui, cui_id_from(ui, "check"), &t, cui_string("Receive updates"),
                        &check_state, (cui_checkbox_options){.disabled = disabled});
        break;
    case 2:
        cui_switch(ui, cui_id_from(ui, "switch"), &t, cui_string("Notifications"), &enabled,
                   disabled);
        break;
    case 3:
        cui_toggle(ui, cui_id_from(ui, "toggle"), &t, cui_string("Pin this item"), &toggled,
                   disabled);
        break;
    case 4:
        cui_radio(ui, cui_id_from(ui, "radio-a"), &t, cui_string("Standard"), &radio, 0, disabled);
        cui_radio(ui, cui_id_from(ui, "radio-b"), &t, cui_string("Priority"), &radio, 1, disabled);
        break;
    case 5:
        cui_slider(ui, cui_id_from(ui, "slider"), &t, cui_string("Volume"), &slider, 0, 100, 1,
                   disabled);
        break;
    case 6:
        cui_input_ex(ui, cui_id_from(ui, "input"), &t, cui_string("Display name"), edit,
                     &input_state,
                     (cui_input_options){.disabled = disabled,
                                         .readonly = input_mode == 1,
                                         .required = input_mode == 2,
                                         .invalid = input_mode == 2,
                                         .width = width > 360 ? 280 : width - 48});
        break;
    case 60: {
        cui_message_body bodies[2] = {
            {.slot = {.present = true, .render = message_rich},
             .typed_bubble = true,
             .bubble = (cui_bubble_variant)message_variant},
            {.slot = {.present = true, .text = cui_string("A second message body")}}};
        cui_message_options options = cui_message_options_default();
        options.width = width - 48;
        options.alignment = (cui_message_alignment)message_alignment;
        options.role = CUI_ROLE_LISTITEM;
        options.name = cui_string("Workspace conversation");
        options.avatar =
            (cui_message_slot){.present = message_avatar,
                               .text = message_avatar ? cui_string("AM") : cui_string(""),
                               .width = 32,
                               .height = 32};
        options.header = (cui_message_slot){.present = true,
                                            .text = cui_string("Alex Morgan"),
                                            .inset = (cui_message_inset)message_inset};
        options.footer = (cui_message_slot){.present = true,
                                            .text = cui_string("Delivered at 09:30"),
                                            .inset = (cui_message_inset)message_inset};
        options.body = bodies;
        options.body_count = 2;
        cui_begin_message_group(ui, cui_id_from(ui, "message-group"), width - 48, 24);
        cui_message(ui, cui_id_from(ui, "message"), &t, options);
        cui_end_message_group(ui);
        cui_begin_box(ui, cui_id_from(ui, "image-clip"),
                      (cui_box){.width = 32, .height = 32, .clip = true});
        cui_paint_bitmap(ui, cui_id_from(ui, "image-example"),
                         (cui_bitmap_paint){.bitmap = {image_story_pixels, 4, 4, 20, 1},
                                            .source = {1, 1, 2, 2},
                                            .opacity = .5f,
                                            .grayscale = true,
                                            .custom_tint = true,
                                            .tint = {255, 128, 0, 128}},
                         (cui_box){.width = 60, .height = 60, .radius = 12});
        cui_end_box(ui);
        break;
    }
    case 61: {
        cui_begin_box(ui, cui_id_from(ui, "conversation-actions"), (cui_box){.gap = t.gap});
        if (cui_button(ui, cui_id_from(ui, "conversation-append"), &t, cui_string("Add message"),
                       conversation_count >= 256))
            conversation_mutation = 1;
        if (cui_button(ui, cui_id_from(ui, "conversation-prepend"), &t,
                       cui_string("Prepend earlier"), conversation_count >= 256))
            conversation_mutation = 2;
        if (cui_button(ui, cui_id_from(ui, "conversation-remove"), &t, cui_string("Remove first"),
                       conversation_count == 0))
            conversation_mutation = 3;
        cui_end_box(ui);
        cui_message_scroller_options options = cui_message_scroller_options_default();
        options.width = width - 48;
        options.height = 300;
        options.scrollbar = message_scrollbar;
        options.bottom_fade = true;
        options.fade_color = t.background;
        options.now_seconds = host_seconds;
        options.reduced_motion = reduced_motion;
        conversation_rendered = 0;
        conversation_result = cui_message_scroller(ui, cui_id_from(ui, "conversation"), &t,
                                                   conversation, conversation_row, NULL, options);
        next_wake = conversation_result.next_wake_seconds;
        break;
    }
    case 59: {
        cui_shimmer_options options = cui_shimmer_options_default();
        options.now_seconds = host_seconds - shimmer_started;
        options.duration = 1.2;
        options.reverse = shimmer_mode == 1;
        options.once = shimmer_mode == 2;
        options.absolute_spread = shimmer_mode == 3;
        options.spread = shimmer_mode == 3 ? 48 : .3f;
        options.font_size = 24;
        options.max_width = width - 48;
        options.role = CUI_ROLE_STATUS;
        options.reduced_motion = reduced_motion;
        cui_shimmer_plan plan;
        if (cui_base_shimmer(options, &plan) && plan.next_wake_seconds > 0)
            next_wake = shimmer_started + plan.next_wake_seconds;
        cui_shimmer_text(ui, cui_id_from(ui, "shimmer"), &t, &shimmer_state,
                         cui_string("Preparing your workspace"), options);
        break;
    }
    case 58: {
        if (cui_button(ui, cui_id_from(ui, "notification-add"), &t, cui_string("Add notification"),
                       disabled || notification_state.count >= 8))
            notification_pushes = 1;
        if (cui_button(ui, cui_id_from(ui, "notification-three"), &t,
                       cui_string("Add three alerts"), disabled || notification_state.count > 5))
            notification_pushes = 3;
        if (cui_button(ui, cui_id_from(ui, "notification-clear"), &t, cui_string("Dismiss alerts"),
                       disabled || notification_state.count == 0))
            notification_clear = true;
        notification_state.settings.reduced_motion = reduced_motion;
        cui_notification_advance result =
            cui_notifications(ui, cui_id_from(ui, "notifications"), &t, &notification_state,
                              (cui_rect){0, 0, width, height}, host_seconds);
        next_wake = result.next_wake_seconds;
        break;
    }
    case 56: {
        cui_tag_options options = {.variant = (cui_tag_variant)tag_variant,
                                   .size = compact_density ? CUI_STATUS_SMALL : CUI_STATUS_MEDIUM,
                                   .outline = tag_outline,
                                   .pill = tag_pill,
                                   .background = {29, 100, 220, 255},
                                   .foreground = {255, 255, 255, 255},
                                   .border = {19, 70, 150, 255}};
        cui_tag_named_palette((cui_status_color)tag_color,
                              t.background.r + t.background.g + t.background.b < 384,
                              &options.palette);
        cui_tag(ui, cui_id_from(ui, "workspace-tag"), &t, cui_string("Workspace"), options);
        cui_tag(ui, cui_id_from(ui, "reviewed-tag"), &t, cui_string("Reviewed"),
                (cui_tag_options){.variant = CUI_TAG_SUCCESS, .outline = true});
        cui_tag_style style;
        cui_id rich_id = cui_id_from(ui, "rich-tag");
        if (cui_begin_tag(ui, rich_id, &t, options, &style)) {
            cui_begin_box(
                ui, 0,
                (cui_box){.width = 6, .height = 6, .radius = 3, .background = style.foreground});
            cui_end_box(ui);
            cui_text(ui, cui_string("  Rich content"), style.font_size, style.foreground);
            cui_end_tag(ui, rich_id, &style);
        }
        break;
    }
    case 57: {
        cui_marker_child children[] = {
            {.kind = CUI_MARKER_ICON, .width = 16, .render = marker_icon_content},
            {.kind = CUI_MARKER_TEXT, .text = cui_string("Synchronizing workspace")},
            {.kind = CUI_MARKER_CONTENT, .width = 52, .render = marker_content}};
        size_t first = marker_icon ? 0 : 1;
        cui_id id = cui_id_from(ui, "workspace-marker");
        cui_marker_options options = {.variant = (cui_marker_variant)marker_variant,
                                      .alignment = (cui_marker_alignment)marker_alignment,
                                      .loading_style = marker_loading == 2 ? CUI_MARKER_SHIMMER
                                                                           : CUI_MARKER_SPINNER,
                                      .loading = marker_loading != 0,
                                      .reduced_motion = reduced_motion,
                                      .role = CUI_ROLE_STATUS,
                                      .name = cui_string("Workspace synchronization"),
                                      .width = width - 48,
                                      .now_seconds = host_seconds,
                                      .duration = 2,
                                      .shimmer_spread = .3f};
        cui_marker_plan plan;
        if (cui_base_marker(options, children + first, 3 - first, &plan))
            next_wake = plan.next_wake_seconds;
        cui_marker(ui, id, &t, &marker_state, children + first, 3 - first, options);
        cui_set_semantic_details(ui, id, (cui_semantic_details){.busy = marker_loading != 0});
        cui_label(ui, &t, cui_string("Typed text, rich content and optional icon."));
        break;
    }
    case 55: {
        cui_label(ui, &t, cui_string("Enter a six-digit verification code."));
        otp_state.masked = otp_masked;
        cui_otp_result result =
            cui_otp(ui, cui_id_from(ui, "verification-code"), &t, &otp_state,
                    (cui_otp_options){.name = cui_string("Verification code"),
                                      .groups = (size_t)otp_groups,
                                      .size = compact_density ? CUI_OTP_SMALL : CUI_OTP_MEDIUM,
                                      .disabled = disabled,
                                      .now_seconds = host_seconds});
        next_wake = result.next_wake_seconds;
        otp_completions += (int)result.complete_count;
        if (cui_button(ui, cui_id_from(ui, "otp-reset"), &t, cui_string("Clear code"), disabled)) {
            cui_otp_set_value(&otp_state, cui_string(""));
            redraw = true;
        }
        char status[96];
        snprintf(status, sizeof(status), "%zu of 6 digits; completions: %d",
                 cui_otp_count(&otp_state), otp_completions);
        cui_id status_id = cui_id_from(ui, "otp-status");
        cui_begin_box(ui, status_id, (cui_box){.width = -1});
        cui_interact(ui, status_id, CUI_ROLE_STATUS, cui_string(status), false, false, 0);
        cui_label(ui, &t, cui_string(status));
        cui_end_box(ui);
        break;
    }
    case 54: {
        cui_id action = cui_id_from(ui, "group-open");
        const cui_input_group_addon addons[] = {{.id = cui_id_from(ui, "group-prefix"),
                                                 .alignment = CUI_INPUT_GROUP_INLINE_START,
                                                 .label = cui_string("https://"),
                                                 .width = 70},
                                                {.id = action,
                                                 .alignment = CUI_INPUT_GROUP_INLINE_END,
                                                 .kind = CUI_INPUT_GROUP_BUTTON,
                                                 .variant = CUI_INPUT_GROUP_PRIMARY,
                                                 .label = cui_string("Open address"),
                                                 .width = 100,
                                                 .loading = group_mode == 3},
                                                {.id = cui_id_from(ui, "group-title"),
                                                 .alignment = CUI_INPUT_GROUP_BLOCK_START,
                                                 .label = cui_string("Workspace address"),
                                                 .width = 190},
                                                {.id = cui_id_from(ui, "group-suffix"),
                                                 .alignment = CUI_INPUT_GROUP_BLOCK_END,
                                                 .label = cui_string(".example.com"),
                                                 .width = 130}};
        cui_input_group_result result = cui_input_group(
            ui, cui_id_from(ui, "address-group"), cui_id_from(ui, "address-input"), &t, group_edit,
            &group_state, addons, 4,
            (cui_input_group_options){
                .width = width > 460 ? 400 : width - 48,
                .size = compact_density ? CUI_FORM_SMALL : CUI_FORM_MEDIUM,
                .name = cui_string("Workspace address"),
                .placeholder = cui_string("workspace"),
                .input = {.disabled = disabled || group_mode == 2, .readonly = group_mode == 1}});
        if (result.activated_addon == action)
            group_actions++;
        if (result.input.changed)
            redraw = true;
        char status[160];
        snprintf(status, sizeof(status), "Address: https://%s.example.com; opens: %d", group_buffer,
                 group_actions);
        cui_id status_id = cui_id_from(ui, "group-status");
        cui_begin_box(ui, status_id, (cui_box){.width = -1});
        cui_interact(ui, status_id, CUI_ROLE_STATUS, cui_string(status), false, false, 0);
        cui_label(ui, &t, cui_string(status));
        cui_end_box(ui);
        break;
    }
    case 53: {
        bool readonly = form_mode == 2;
        bool field_disabled = disabled || form_mode == 3;
        bool name_invalid = !form_name_buffer[0], email_invalid = !form_email_valid();
        gallery_form_control controls[] = {
            {cui_id_from(ui, "form-name"), form_name_edit, &form_name_state, "Name", field_disabled,
             readonly, name_invalid},
            {cui_id_from(ui, "form-email"), form_email_edit, &form_email_state, "Work email",
             field_disabled, readonly, email_invalid}};
        const cui_form_field fields[] = {
            {.id = cui_id_from(ui, "form-name-field"),
             .control = controls[0].id,
             .label = cui_string("Name"),
             .description = cui_string("Shown to other workspace members."),
             .error = cui_string("Enter a name."),
             .required = true,
             .invalid = name_invalid,
             .disabled = field_disabled,
             .readonly = readonly,
             .render_control = form_control,
             .user = &controls[0]},
            {.id = cui_id_from(ui, "form-email-field"),
             .control = controls[1].id,
             .label = cui_string("Work email"),
             .description = cui_string("Used for workspace notifications."),
             .error = cui_string("Enter an address containing @ and a dot."),
             .required = true,
             .invalid = email_invalid,
             .disabled = field_disabled,
             .readonly = readonly,
             .render_control = form_control,
             .user = &controls[1]}};
        bool save_disabled = field_disabled || readonly;
        cui_form(ui, cui_id_from(ui, "profile-form"), &t, fields, 2,
                 (cui_form_options){.width = width - 48,
                                    .columns = width > 580 ? 2 : 1,
                                    .name = cui_string("Profile form"),
                                    .render_footer = form_footer,
                                    .footer_user = &save_disabled});
        cui_id status_id = cui_id_from(ui, "form-status");
        cui_begin_box(ui, status_id, (cui_box){.width = -1});
        cui_interact(ui, status_id, CUI_ROLE_STATUS, cui_string(form_status), false, false, 0);
        cui_label(ui, &t, cui_string(form_status));
        cui_end_box(ui);
        break;
    }
    case 7:
        cui_label(ui, &t, cui_string("A useful label, rendered by CUI."));
        break;
    case 8:
        cui_badge(ui, cui_id_from(ui, "badge"), &t, cui_string("In review"));
        break;
    case 9:
        cui_progress(ui, cui_id_from(ui, "progress"), &t, slider / 100,
                     width > 360 ? 300 : width - 48);
        cui_slider(ui, cui_id_from(ui, "progress-control"), &t, cui_string("Progress"), &slider, 0,
                   100, 1, disabled);
        break;
    case 10:
        cui_label(ui, &t, cui_string("Before the separator"));
        cui_separator(ui, cui_id_from(ui, "separator"), &t, width - 48);
        cui_label(ui, &t, cui_string("After the separator"));
        break;
    case 11:
        cui_label(ui, &t, cui_string("Semantic tokens and runtime themes"));
        cui_button(ui, cui_id_from(ui, "theme-button"), &t, cui_string("Primary action"), disabled);
        cui_badge(ui, cui_id_from(ui, "theme-badge"), &t, cui_string("Surface / primary"));
        cui_begin_box(ui, cui_id_from(ui, "shape-sample"),
                      (cui_box){.width = 120,
                                .height = 64,
                                .background = {29, 100, 220, 255},
                                .border = {240, 150, 20, 255},
                                .custom_corners = true,
                                .corner_radii = {24, 0, 0, 18},
                                .custom_border_edges = true,
                                .border_edges = {0, 8, 4, 0}});
        cui_end_box(ui);
        for (unsigned focusable = 0; focusable < 2; focusable++) {
            cui_id id = cui_id_from(ui, focusable ? "focusable-log" : "passive-log");
            cui_str name =
                cui_string(focusable ? "Focusable semantic log" : "Passive semantic log");
            cui_begin_box(ui, id,
                          (cui_box){.width = width - 48,
                                    .height = 30,
                                    .padding = 5,
                                    .border = cui_focused(ui) == id ? t.focus : t.border,
                                    .border_width = 1});
            cui_interact(ui, id, CUI_ROLE_LOG, name, false, false, 0);
            cui_set_semantic_details(ui, id, (cui_semantic_details){.focusable = focusable != 0});
            cui_set_tab_stop(ui, id, focusable != 0);
            cui_text(ui, name, t.font_size, t.text);
            cui_end_box(ui);
        }
        break;
    case 12:
        cui_alert(ui, cui_id_from(ui, "alert"), &t,
                  (cui_alert_options){.kind = CUI_ALERT_INFO,
                                      .title = cui_string("Your changes are saved"),
                                      .message = cui_string("You can keep working."),
                                      .width = width - 48,
                                      .visible = &alert_visible,
                                      .dismissible = true});
        if (cui_button(ui, cui_id_from(ui, "reset-alert"), &t, cui_string("Show alert"), disabled))
            alert_visible = true;
        break;
    case 13:
        cui_avatar(ui, cui_id_from(ui, "avatar"), &t, cui_string("Alex Morgan"), 56);
        break;
    case 14:
        cui_begin_bubble(ui, cui_id_from(ui, "bubble"), &t, CUI_BUBBLE_TINTED, width - 48);
        cui_label(ui, &t, cui_string("A composed message surface."));
        cui_end_bubble(ui);
        break;
    case 15:
        cui_empty(ui, cui_id_from(ui, "empty"), &t, cui_string("No items yet"),
                  cui_string("Add your first item to get started."), width - 48);
        break;
    case 16:
        cui_begin_group_box(ui, cui_id_from(ui, "groupbox"), &t, cui_string("Preferences"),
                            width - 48);
        cui_checkbox(ui, cui_id_from(ui, "group-check"), &t, cui_string("Receive updates"),
                     &checked, disabled);
        cui_end_group_box(ui);
        break;
    case 17:
        cui_kbd(ui, cui_id_from(ui, "kbd"), &t, cui_string("Ctrl + K"));
        break;
    case 18:
        cui_skeleton(ui, cui_id_from(ui, "skeleton"), &t, width - 48, 36, 6,
                     cui_loading_phase(host_seconds, 1.4f), false, reduced_motion);
        redraw = !reduced_motion;
        break;
    case 19:
        cui_spinner(ui, cui_id_from(ui, "spinner"), &t, 32, cui_loading_phase(host_seconds, 1.0f),
                    reduced_motion);
        redraw = !reduced_motion;
        break;
    case 20: {
        cui_disclosure_item items[3] = {
            {cui_id_from(ui, "a1"), cui_string("Getting started"), false},
            {cui_id_from(ui, "a2"), cui_string("Configuration"), false},
            {cui_id_from(ui, "a3"), cui_string("More details"), false}};
        cui_box triggers[3], panels[3];
        for (size_t i = 0; i < 3; i++) {
            triggers[i] = (cui_box){.width = width - 48,
                                    .height = t.control_height,
                                    .padding = t.padding,
                                    .row = true,
                                    .gap = t.gap,
                                    .radius = t.radius,
                                    .background = t.background,
                                    .border = t.success,
                                    .border_width = 1};
            panels[i] = (cui_box){.width = width - 48,
                                  .padding = t.padding,
                                  .gap = t.gap,
                                  .background = t.surface,
                                  .border = t.border,
                                  .border_width = 1};
        }
        cui_accordion(ui, cui_id_from(ui, "accordion"), &t, items, 3, open,
                      (cui_accordion_options){
                          .collapsible = true,
                          .disabled = disabled,
                          .bordered = true,
                          .multiple = disclosure_multiple,
                          .render_header = disclosure_mode == 2 ? disclosure_header : NULL,
                          .trigger_boxes = disclosure_mode == 2 ? triggers : NULL,
                          .panel_boxes = disclosure_mode == 2 ? panels : NULL,
                          .style_count = 3,
                          .motion = disclosure_mode ? accordion_motion : NULL,
                          .motion_count = 3,
                          .dt = host_dt,
                          .response = .45f,
                          .reduced_motion = reduced_motion},
                      disclosure_mode ? disclosure_panel : panel, NULL);
        break;
    }
    case 21: {
        cui_box trigger_box = {.width = width - 48,
                               .height = t.control_height,
                               .padding = t.padding,
                               .row = true,
                               .gap = t.gap,
                               .radius = t.radius,
                               .background = t.background,
                               .border = t.success,
                               .border_width = 1};
        cui_box panel_box = {.width = width - 48,
                             .padding = t.padding,
                             .gap = t.gap,
                             .background = t.surface,
                             .border = t.border,
                             .border_width = 1};
        cui_collapsible_ex(ui, cui_id_from(ui, "collapsible"), &t, cui_string("Show details"),
                           &collapsed,
                           (cui_collapsible_options){
                               .disabled = disabled,
                               .render_header = disclosure_mode == 2 ? disclosure_header : NULL,
                               .trigger_box = disclosure_mode == 2 ? &trigger_box : NULL,
                               .panel_box = disclosure_mode == 2 ? &panel_box : NULL,
                               .motion = disclosure_mode ? &collapsible_motion : NULL,
                               .dt = host_dt,
                               .response = .45f,
                               .reduced_motion = reduced_motion},
                           disclosure_mode ? disclosure_panel : panel, NULL);
        break;
    }
    case 22: {
        const char *names[12] = {"Overview",      "Activity",        "Settings",
                                 "Team members",  "Deployments",     "Build history",
                                 "Permissions",   "Integrations",    "Notifications",
                                 "Usage reports", "Billing details", "Audit history"};
        cui_choice_item items[12];
        for (size_t i = 0; i < 12; i++) {
            char id[24];
            snprintf(id, sizeof(id), "tab%zu", i + 1);
            items[i] = (cui_choice_item){cui_id_from(ui, id), cui_string(names[i]), false};
        }
        if (!selected_tab)
            selected_tab = items[0].id;
        if (!tabs_overflow.trigger)
            cui_tab_overflow_init(&tabs_overflow, cui_id_from(ui, "tabs-popup"),
                                  cui_id_from(ui, "tabs-all"), tabs_entries, 12);
        tabs_overflow.options.popup.viewport = (cui_rect){0, 0, width, height};
        tabs_overflow.options.popup.dt = host_dt;
        tabs_overflow.options.popup.width = 200;
        if (tabs_mode && cui_button(ui, cui_id_from(ui, "tabs-reveal"), &t,
                                    cui_string("Reveal last tab"), disabled)) {
            selected_tab = items[11].id;
            tabs_scroll.reveal_id = selected_tab;
        }
        cui_tabs(ui, cui_id_from(ui, "tabs"), &t, items, tabs_mode ? 12 : 3, &selected_tab,
                 (cui_tabs_options){.disabled = disabled,
                                    .variant = (cui_tab_variant)tabs_variant,
                                    .size = (cui_tab_size)tabs_size,
                                    .vertical = tabs_mode == 2,
                                    .manual_activation = tabs_manual,
                                    .scroll = tabs_mode ? &tabs_scroll : NULL,
                                    .bar_extent = tabs_mode == 2 ? 140 : width - 100,
                                    .max_width = tabs_mode == 2 ? 140 : 0,
                                    .overflow = tabs_mode ? &tabs_overflow : NULL,
                                    .reveal_selected = true,
                                    .scrollbar = true},
                 panel, NULL);
        break;
    }
    case 23:
    case 24: {
        cui_id trigger = cui_id_from(ui, "hover-trigger");
        cui_button(ui, trigger, &t, cui_string("Hover or focus here"), disabled);
        cui_hover_options options = {.trigger = trigger,
                                     .viewport = {0, 0, width, height},
                                     .side = CUI_POPUP_BELOW,
                                     .width = width > 360 ? 280 : width - 48,
                                     .height = 100,
                                     .gap = 8,
                                     .dt = host_dt,
                                     .open_delay = .25f,
                                     .close_delay = .1f,
                                     .disabled = disabled};
        if (story == 23)
            cui_tooltip(ui, cui_id_from(ui, "tooltip"), &t, &tooltip_state,
                        cui_string("A real CUI floating tooltip"), options);
        else if (cui_begin_hover_card(ui, cui_id_from(ui, "hover-card"), &t, &card_state,
                                      cui_string("Profile details"), options)) {
            cui_label(ui, &t, cui_string("Alex Morgan"));
            cui_label(ui, &t, cui_string("A composable floating card."));
            cui_end_hover_card(ui);
        }
        redraw = story == 23 ? tooltip_state.pending : card_state.pending;
        break;
    }
    case 25: {
        cui_label(ui, &t, cui_string("Base model: scalar editing and undo history"));
        cui_input(ui, cui_id_from(ui, "editor-model"), &t, cui_string("Model text"), edit,
                  disabled);
        cui_begin_box(ui, cui_id_from(ui, "edit-actions"), (cui_box){.row = true, .gap = 8});
        if (cui_button(ui, cui_id_from(ui, "undo"), &t, cui_string("Undo"), disabled))
            cui_edit_undo(edit);
        if (cui_button(ui, cui_id_from(ui, "redo"), &t, cui_string("Redo"), disabled))
            cui_edit_redo(edit);
        cui_end_box(ui);
        break;
    }
    case 26: {
        cui_label(ui, &t, cui_string("Base model: 100,000 retained rows"));
        cui_slider(ui, cui_id_from(ui, "virtual-scroll"), &t, cui_string("Scroll position"),
                   &scroll_percent, 0, 100, 1, disabled);
        cui_virtual_view view = cui_virtual_viewport(
            virtual_list, cui_virtual_total(virtual_list) * scroll_percent / 100, 144, 1);
        char text[128];
        snprintf(text, sizeof(text), "Visible range: %zu .. %zu / 100000", view.begin, view.end);
        cui_label(ui, &t, cui_string(text));
        snprintf(text, sizeof(text), "Offset: %.0f px; total: %.0f px", view.before, view.total);
        cui_label(ui, &t, cui_string(text));
        break;
    }
    case 27: {
        cui_label(ui, &t, cui_string("Base model: dock serialization and geometry"));
        if (cui_button(ui, cui_id_from(ui, "split"), &t, cui_string("Split panel"), disabled)) {
            cui_dock_node tab;
            size_t index;
            if (cui_dock_panel_find(dock, 1, &tab, &index)) {
                cui_dock_node fresh;
                if (cui_dock_split(dock, tab, CUI_DOCK_SPLIT_HORIZONTAL, .5, dock_serial, false,
                                   &fresh) == CUI_DOCK_OK)
                    dock_serial++;
            }
        }
        cui_dock_geometry regions[16];
        size_t count = 0;
        bool fits;
        cui_dock_layout(dock, (cui_rect){0, 0, width - 48, 100}, 4, regions, 16, &count, &fits);
        char text[128];
        snprintf(text, sizeof(text), "%zu tab regions; minimum sizes %s", count,
                 fits ? "fit" : "compressed");
        cui_label(ui, &t, cui_string(text));
        size_t bytes;
        cui_dock_serialize(dock, NULL, 0, &bytes);
        snprintf(text, sizeof(text), "Serialized layout: %zu bytes", bytes);
        cui_label(ui, &t, cui_string(text));
        break;
    }
    case 28: {
        cui_label(ui, &t, cui_string("Base model: plot scale and tick calculation"));
        if (cui_button(ui, cui_id_from(ui, "plot-zoom"), &t, cui_string("Zoom in at center"),
                       disabled))
            cui_plot_zoom(&plot_scale, 1.5, 150);
        double ticks[32];
        size_t count = 0;
        cui_plot_ticks(&plot_scale, 5, ticks, 32, &count);
        char text[128];
        snprintf(text, sizeof(text), "Domain: %.2f .. %.2f; ticks: %zu", plot_scale.domain_min,
                 plot_scale.domain_max, count);
        cui_label(ui, &t, cui_string(text));
        break;
    }
    case 29: {
        cui_calendar(ui, cui_id_from(ui, "calendar"), &t, &calendar, &date_selection,
                     (cui_calendar_options){.disabled = disabled,
                                            .first_weekday = 1,
                                            .cell_size = width > 360 ? 32 : 28});
        char text[64];
        snprintf(text, sizeof(text), "Selected: %04d-%02d-%02d", date_selection.start.year,
                 date_selection.start.month, date_selection.start.day);
        cui_label(ui, &t, cui_string(text));
        break;
    }
    case 44: {
        cui_scroll_options options = {.width = width - 48,
                                      .height = 192,
                                      .content_width = 500,
                                      .content_height = 640,
                                      .horizontal = true,
                                      .vertical = true,
                                      .disabled = disabled,
                                      .wheel_step = 32,
                                      .gap = 12};
        if (cui_scrollable_begin(ui, cui_id_from(ui, "scrollable"), &t, &scroll_state, options)) {
            for (size_t i = 0; i < 16; i++) {
                char text[32];
                snprintf(text, sizeof(text), "Scrollable item %zu", i + 1);
                cui_label(ui, &t, cui_string(text));
            }
            cui_scrollable_end(ui, cui_id_from(ui, "scrollable"), &t, &scroll_state, options);
        }
        break;
    }
    case 45: {
        const cui_table_column_group groups[] = {{cui_string("File details"), 2},
                                                 {cui_string("Storage"), 1}};
        const cui_table_group_row group_row = {groups, 2};
        table_row_hooks = 0;
        cui_table_result result = cui_data_table(
            ui, cui_id_from(ui, "data-table"), &t, table, &table_state,
            (cui_table_options){.width = width - 48,
                                .height = 224,
                                .row_height = 32,
                                .header_height = 32,
                                .disabled = disabled,
                                .striped = true,
                                .bordered = true,
                                .multiple_selection = multiple_mode,
                                .selection_mode = table_selection_mode,
                                .row_header = table_mode == 1,
                                .row_header_width = 52,
                                .scrollbars = table_mode == 1,
                                .group_headers = &group_row,
                                .group_header_count = table_mode == 1 ? 1 : 0,
                                .render_row = table_mode == 1 ? table_row_hook : NULL,
                                .render_trailing = table_mode == 1 ? table_trailing : NULL,
                                .trailing_width = 80,
                                .loading = table_mode == 2,
                                .render_loading = table_loading,
                                .render_empty = table_empty},
            table_label, NULL, NULL);
        if (result.selection_mode_requested) {
            table_selection_mode = result.requested_selection_mode;
            redraw = true;
        }
        char text[72];
        snprintf(text, sizeof(text), "%zu rows / visible %zu-%zu / sort %d", cui_table_rows(table),
                 result.rows.end > result.rows.begin ? result.rows.begin + 1 : 0, result.rows.end,
                 (int)table_state.sort);
        if (table_mode == 1) {
            snprintf(text, sizeof(text), "Visible row hooks: %d / inspections: %d", table_row_hooks,
                     table_inspections);
        }
        cui_label(ui, &t, cui_string(text));
        break;
    }
    case 46: {
        float cell_width = (width - 48) / 3;
        cui_id table_id = cui_id_from(ui, "composition-table");
        cui_table_begin(ui, table_id, &t, width - 48);
        cui_id section_id = table_id + 50;
        cui_table_begin_section(ui, section_id, &t, CUI_TABLE_SECTION_HEADER);
        cui_set_semantic_details(ui, section_id, (cui_semantic_details){.parent = table_id});
        for (size_t r = 0; r < 4; r++) {
            if (r == 1) {
                cui_table_end(ui);
                section_id++;
                cui_table_begin_section(ui, section_id, &t, CUI_TABLE_SECTION_BODY);
                cui_set_semantic_details(ui, section_id,
                                         (cui_semantic_details){.parent = table_id});
            }
            cui_id row_id = table_id + r + 1;
            cui_table_begin_row(ui, row_id, &t, 40, r == 0);
            cui_set_semantic_details(ui, row_id, (cui_semantic_details){.parent = section_id});
            for (size_t c = 0; c < 3; c++) {
                cui_id cell_id = table_id + 100 + r * 3 + c;
                const char *headers[] = {"Name", "Status", "Size"};
                cui_str label = r == 0 ? cui_string(headers[c]) : table_label(NULL, r - 1, 101 + c);
                if (r == 0)
                    cui_table_begin_head_cell(ui, cell_id, cell_width, 40, CUI_TABLE_ALIGN_LEFT,
                                              label);
                else
                    cui_table_begin_cell(ui, cell_id, cell_width, 40, CUI_TABLE_ALIGN_LEFT);
                cui_set_semantic_details(ui, cell_id,
                                         (cui_semantic_details){.parent = row_id,
                                                                .row_index = (unsigned)r,
                                                                .column_index = (unsigned)c + 1,
                                                                .readonly = r == 0,
                                                                .description = label});
                cui_label(ui, &t, label);
                cui_table_end(ui);
            }
            cui_table_end(ui);
        }
        cui_table_end(ui);
        cui_table_end(ui);
        break;
    }
    case 48: {
        cui_id trigger = cui_id_from(ui, "dialog-trigger");
        if (cui_button(ui, trigger, &t, cui_string("Edit preferences"), disabled))
            cui_surface_open(ui, &dialog_surface, trigger);
        cui_surface_options options = {.viewport = {0, 0, width, height},
                                       .width = width > 520 ? 420 : width - 32,
                                       .height = 250,
                                       .margin = 16};
        if (cui_surface_begin(ui, &t, &dialog_surface, cui_string("Notification preferences"),
                              cui_string("Choose how CUI should keep you informed."), options)) {
            cui_checkbox(ui, cui_id_from(ui, "dialog-updates"), &t, cui_string("Product updates"),
                         &dialog_updates, false);
            cui_id nested_trigger = cui_id_from(ui, "nested-trigger");
            if (cui_button(ui, nested_trigger, &t, cui_string("Advanced preferences"), false))
                cui_surface_open(ui, &nested_surface, nested_trigger);
            if (cui_surface_begin(ui, &t, &nested_surface, cui_string("Advanced preferences"),
                                  cui_string("Escape returns to the parent dialog."),
                                  (cui_surface_options){.viewport = {0, 0, width, height},
                                                        .width = options.width - 32,
                                                        .height = 190,
                                                        .margin = 16,
                                                        .layer = 200})) {
                cui_checkbox(ui, cui_id_from(ui, "nested-updates"), &t,
                             cui_string("Preview releases"), &popover_updates, false);
                cui_surface_end(ui);
            }
            cui_begin_box(ui, 0, (cui_box){.height = 300});
            cui_label(ui, &t, cui_string("Long body content remains clipped above the footer."));
            cui_end_box(ui);
            cui_surface_footer_begin(ui, &t);
            if (cui_button(ui, cui_id_from(ui, "dialog-save"), &t, cui_string("Save changes"),
                           false))
                cui_surface_close(ui, &dialog_surface, CUI_SURFACE_ACCEPTED);
            cui_surface_end(ui);
        }
        break;
    }
    case 49: {
        cui_id trigger = cui_id_from(ui, "alert-dialog-trigger");
        if (cui_button(ui, trigger, &t, cui_string("Delete workspace"), disabled)) {
            alert_attempts = 0;
            cui_surface_open(ui, &alert_dialog_surface, trigger);
        }
        const char *description = alert_attempts == 1
                                      ? "Deletion was rejected once. Confirm again to proceed."
                                      : "The first confirmation is deliberately rejected.";
        cui_alert_dialog(ui, &t, &alert_dialog_surface, cui_string("Delete workspace?"),
                         cui_string(description), cui_string("Delete"),
                         cui_string("Keep workspace"),
                         (cui_surface_options){.viewport = {0, 0, width, height},
                                               .width = width > 520 ? 420 : width - 32,
                                               .height = 220,
                                               .margin = 16,
                                               .decide = alert_decision,
                                               .decision_user = &alert_attempts});
        break;
    }
    case 50: {
        cui_id trigger = cui_id_from(ui, "sheet-trigger");
        if (cui_button(ui, trigger, &t, cui_string("Open settings sheet"), disabled))
            cui_surface_open(ui, &sheet_surface, trigger);
        cui_sheet_edge edge =
            surface_variant >= CUI_SHEET_LEFT && surface_variant <= CUI_SHEET_BOTTOM
                ? (cui_sheet_edge)surface_variant
                : CUI_SHEET_LEFT;
        cui_surface_options options = {.viewport = {0, 0, width, height},
                                       .width = width > 520 ? 320 : width * .72f,
                                       .height = height * .62f,
                                       .edge = edge,
                                       .sheet_minimum = 100,
                                       .sheet_maximum =
                                           edge < CUI_SHEET_TOP ? width - 48 : height - 32};
        if (cui_surface_begin(ui, &t, &sheet_surface, cui_string("Workspace settings"),
                              cui_string("The same composed surface supports all four edges."),
                              options)) {
            cui_checkbox(ui, cui_id_from(ui, "sheet-updates"), &t, cui_string("Automatic updates"),
                         &dialog_updates, false);
            cui_label(ui, &t, cui_string("Escape or Close restores focus to the trigger."));
            cui_surface_end(ui);
        }
        break;
    }
    case 51: {
        cui_id trigger = cui_id_from(ui, "popover-trigger");
        if (cui_button(ui, trigger, &t, cui_string("Open account card"), disabled))
            cui_surface_open(ui, &popover_surface, trigger);
        cui_rect anchor = cui_bounds(ui, trigger);
        cui_surface_options options = {.viewport = {0, 0, width, height},
                                       .anchor = anchor,
                                       .width = width > 500 ? 340 : width - 48,
                                       .height = 190,
                                       .margin = 12,
                                       .side = CUI_POPUP_BELOW};
        if (cui_surface_begin(ui, &t, &popover_surface, cui_string("Alex Morgan"),
                              cui_string("Maintainer · Portable UI systems"), options)) {
            cui_avatar(ui, cui_id_from(ui, "popover-avatar"), &t, cui_string("Alex Morgan"), 40);
            cui_checkbox(ui, cui_id_from(ui, "popover-updates"), &t,
                         cui_string("Notify me about releases"), &popover_updates, false);
            if (cui_button(ui, cui_id_from(ui, "popover-profile"), &t, cui_string("View profile"),
                           false))
                cui_surface_close(ui, &popover_surface, CUI_SURFACE_ACCEPTED);
            cui_surface_end(ui);
        }
        break;
    }
    case 43: {
        cui_tree(ui, cui_id_from(ui, "tree"), &t, tree, &tree_state, &tree_selected,
                 (cui_tree_options){.disabled = disabled,
                                    .width = width - 48,
                                    .height = 192,
                                    .row_height = 32,
                                    .dt = host_dt});
        break;
    }
    case 32:
    case 47:
    case 52: {
        const cui_menu_entry children[] = {
            {.id = cui_id_from(ui, "export-json"),
             .label = cui_string("JSON"),
             .shortcut = cui_string("Ctrl+J")},
            {.id = cui_id_from(ui, "export-text"), .label = cui_string("Plain text")}};
        const cui_menu_entry entries[] = {
            {.id = cui_id_from(ui, "menu-standard"), .label = cui_string("Standard")},
            {.id = cui_id_from(ui, "menu-priority"), .label = cui_string("Priority")},
            {.id = cui_id_from(ui, "menu-unavailable"),
             .label = cui_string("Unavailable"),
             .disabled = true},
            {.id = cui_id_from(ui, "menu-separator"), .kind = CUI_MENU_SEPARATOR},
            {.id = cui_id_from(ui, "menu-export"),
             .label = cui_string("Export"),
             .kind = CUI_MENU_SUBMENU,
             .children = children,
             .child_count = 2},
            {.id = cui_id_from(ui, "menu-hidden"),
             .label = cui_string("Show hidden"),
             .kind = CUI_MENU_CHECK,
             .checked = &menu_checked},
            {.id = cui_id_from(ui, "menu-density"),
             .label = cui_string("Density"),
             .kind = CUI_MENU_LABEL},
            {.id = cui_id_from(ui, "menu-comfortable"),
             .label = cui_string("Comfortable"),
             .kind = CUI_MENU_RADIO,
             .radio_value = &menu_radio,
             .radio_option = 0},
            {.id = cui_id_from(ui, "menu-compact"),
             .label = cui_string("Compact"),
             .kind = CUI_MENU_RADIO,
             .radio_value = &menu_radio,
             .radio_option = 1}};
        cui_menu_tree_options options = {.popup = {.disabled = disabled,
                                                   .viewport = {0, 0, width, height},
                                                   .width = width > 500 ? 240 : (width - 48) * .7f,
                                                   .dt = host_dt},
                                         .keep_open_checks = true};
        bool invoked;
        if (story == 47) {
            cui_id target = cui_id_from(ui, "context-target");
            cui_button_ex(ui, target, &t, cui_string("Right-click this area"),
                          (cui_button_options){.disabled = disabled, .width = width - 48});
            invoked = cui_context_menu(ui, target, &t, cui_string("Context actions"), entries, 9,
                                       &menu_action, &context_menu, options);
        } else if (story == 52) {
            cui_dropdown_button_result result = cui_dropdown_button(
                ui, cui_id_from(ui, "split-button"), cui_id_from(ui, "split-action"),
                cui_id_from(ui, "split-trigger"), &t, cui_string("Export workspace"),
                cui_string("Export options"), entries, 9, &menu_action, &menu_tree,
                (cui_dropdown_button_options){.disabled = disabled,
                                              .leading = cui_string("↓"),
                                              .variant =
                                                  (cui_dropdown_button_variant)dropdown_variant,
                                              .size = (cui_dropdown_button_size)dropdown_size,
                                              .outline = dropdown_outline,
                                              .selected = dropdown_selected,
                                              .action_disabled = dropdown_action == 1,
                                              .action_loading = dropdown_action == 2,
                                              .hide_action = dropdown_halves == 1,
                                              .hide_menu = dropdown_halves == 2,
                                              .anchor = (cui_dropdown_button_anchor)dropdown_anchor,
                                              .custom = {{65, 40, 160, 255},
                                                         {255, 255, 255, 255},
                                                         {90, 60, 190, 255},
                                                         {40, 20, 120, 255}},
                                              .menu = options});
            if (result.action_activated)
                clicks++;
            invoked = result.menu_activated;
        } else
            invoked = cui_menu_tree(ui, cui_id_from(ui, "menu"), &t, cui_string("Actions"), entries,
                                    9, &menu_action, &menu_tree, options);
        if (invoked)
            menu_invocations++;
        char text[64];
        snprintf(text, sizeof(text), "Actions invoked: %d", menu_invocations);
        cui_label(ui, &t, cui_string(text));
        if (story == 52) {
            snprintf(text, sizeof(text), "Primary exports: %d", clicks);
            cui_label(ui, &t, cui_string(text));
        }
        break;
    }
    case 30:
    case 31: {
        cui_choice_item items[3] = {{cui_id_from(ui, "choice-a"), cui_string("Standard"), false},
                                    {cui_id_from(ui, "choice-b"), cui_string("Priority"), false},
                                    {cui_id_from(ui, "choice-c"), cui_string("Unavailable"), true}};
        cui_choice_popup_options options = {.disabled = disabled,
                                            .viewport = {0, 0, width, height},
                                            .width = width > 360 ? 280 : width - 48,
                                            .dt = host_dt};
        if (story == 30 && multiple_mode)
            cui_list_multi(ui, cui_id_from(ui, "list"), &t, items, 3, multi_selected, &list_state,
                           (cui_list_options){.disabled = disabled, .dt = host_dt});
        else if (story == 30)
            cui_list(ui, cui_id_from(ui, "list"), &t, items, 3, &list_selection, &list_state,
                     (cui_list_options){.disabled = disabled, .dt = host_dt});
        else if (story == 31 && multiple_mode)
            cui_select_multi(ui, cui_id_from(ui, "select"), &t, cui_string("Delivery"),
                             cui_string("Select services"), items, 3, multi_selected, &select_popup,
                             options);
        else if (story == 31)
            cui_select(ui, cui_id_from(ui, "select"), &t, cui_string("Delivery"),
                       cui_string("Select a service"), items, 3, &select_selection, &select_popup,
                       options);
        else if (cui_menu(ui, cui_id_from(ui, "menu"), &t, cui_string("Actions"), items, 3,
                          &menu_action, &menu_popup, options))
            menu_invocations++;
        if (story == 32) {
            char text[64];
            snprintf(text, sizeof(text), "Actions invoked: %d", menu_invocations);
            cui_label(ui, &t, cui_string(text));
        }
        break;
    }
    case 33: {
        cui_number_input(ui, cui_id_from(ui, "number"), &t, cui_string("Quantity"), &number_state,
                         &number,
                         (cui_number_options){.minimum = 0,
                                              .maximum = 100,
                                              .step = 1,
                                              .has_minimum = true,
                                              .has_maximum = true,
                                              .disabled = disabled,
                                              .width = width > 360 ? 280 : width - 48});
        char text[64];
        snprintf(text, sizeof(text), "Controlled value: %.2f", number);
        cui_label(ui, &t, cui_string(text));
        break;
    }
    case 34: {
        cui_pagination(ui, cui_id_from(ui, "pagination"), &t, &page_number, 20, true, disabled);
        char text[64];
        snprintf(text, sizeof(text), "Page %zu of 20", page_number);
        cui_label(ui, &t, cui_string(text));
        break;
    }
    case 35:
        cui_rating(ui, cui_id_from(ui, "rating"), &t, cui_string("Rate your experience"), &rating,
                   (cui_rating_options){.maximum = 5,
                                        .half_steps = true,
                                        .clearable = true,
                                        .disabled = disabled,
                                        .width = width > 360 ? 280 : width - 128});
        break;
    case 36: {
        cui_step steps[3] = {{cui_string("Details"), cui_string(""), CUI_STEP_COMPLETE, false},
                             {cui_string("Review"), cui_string(""), CUI_STEP_ACTIVE, false},
                             {cui_string("Finish"), cui_string(""), CUI_STEP_PENDING, false}};
        cui_stepper(ui, cui_id_from(ui, "stepper"), &t, steps, 3, &current_step, true, disabled);
        break;
    }
    case 37: {
        cui_choice_item items[4] = {{cui_id_from(ui, "combo-a"), cui_string("Apple"), false},
                                    {cui_id_from(ui, "combo-b"), cui_string("Apricot"), false},
                                    {cui_id_from(ui, "combo-c"), cui_string("Banana"), false},
                                    {cui_id_from(ui, "combo-d"), cui_string("Unavailable"), true}},
                        filtered[4];
        cui_combobox_result result;
        cui_choice_popup_options options = {.disabled = disabled,
                                            .viewport = {0, 0, width, height},
                                            .width = width > 360 ? 280 : width - 48,
                                            .dt = host_dt};
        if (multiple_mode) {
            bool filtered_selected[4];
            result = cui_combobox_multi(
                ui, cui_id_from(ui, "combobox"), &t, cui_string("Find fruits"), query, items, 4,
                filtered, filtered_selected, 4, multi_selected, &combo_popup, options);
        } else
            result =
                cui_combobox(ui, cui_id_from(ui, "combobox"), &t, cui_string("Find a fruit"), query,
                             items, 4, filtered, 4, &combo_selection, &combo_popup, options);
        char text[64];
        snprintf(text, sizeof(text), "%zu matching options", result.matching_count);
        cui_label(ui, &t, cui_string(text));
        break;
    }
    case 38: {
        cui_id trigger = cui_id_from(ui, "trap-trigger"), close = cui_id_from(ui, "trap-close");
        cui_overlay_update(ui, &trap);
        if (cui_button(ui, trigger, &t, cui_string("Open focus trap"), disabled))
            cui_overlay_open(ui, &trap, trigger);
        size_t count;
        cui_frame_events(ui, &count);
        for (size_t i = 0; i < count; i++)
            if (cui_event_activation(ui, i) == close)
                cui_overlay_close(ui, &trap);
        if (trap.open) {
            cui_rect bounds = {24, 80, width - 48, 156};
            cui_overlay_set_bounds(&trap, bounds);
            if (cui_overlay_begin(ui, &trap)) {
                cui_begin_portal(ui, trap.id,
                                 (cui_box){.padding = 12,
                                           .gap = 8,
                                           .background = t.surface,
                                           .border = t.border,
                                           .border_width = 1,
                                           .radius = t.radius},
                                 bounds, 100);
                cui_label(ui, &t, cui_string("Tab cycles within this scope."));
                cui_button(ui, cui_id_from(ui, "trap-first"), &t, cui_string("First control"),
                           false);
                cui_button(ui, close, &t, cui_string("Close scope"), false);
                cui_end_box(ui);
                cui_overlay_end(ui);
            }
        }
        break;
    }
    case 39: {
        cui_label(ui, &t, cui_string("Browser host clipboard integration"));
        cui_begin_box(ui, cui_id_from(ui, "clipboard-actions"), (cui_box){.row = true, .gap = 8});
        if (cui_button(ui, cui_id_from(ui, "copy"), &t, cui_string("Copy text"), disabled))
            clipboard_request = 1;
        if (cui_button(ui, cui_id_from(ui, "paste"), &t, cui_string("Read text"), disabled))
            clipboard_request = 2;
        cui_end_box(ui);
        char preview[132];
        size_t length = strlen(clipboard_status);
        memcpy(preview, clipboard_status, length + 1);
        bool shortened = false;
        while (length && host_measure(preview, length, t.font_size) > width - 68) {
            shortened = true;
            length--;
            while (length && ((unsigned char)preview[length] & 0xc0) == 0x80)
                length--;
            preview[length] = 0;
        }
        if (shortened)
            memcpy(preview + length, "...", 4);
        cui_label(ui, &t, cui_string(preview));
        break;
    }
    case 40:
        cui_date_picker(ui, cui_id_from(ui, "date-picker"), &t, cui_string("Date"), &date_picker,
                        &date_selection,
                        (cui_date_picker_options){.calendar = {.first_weekday = 1, .cell_size = 28},
                                                  .viewport = {0, 0, width, height},
                                                  .disabled = disabled,
                                                  .width = width > 360 ? 280 : width - 48});
        break;
    case 41: {
        cui_time_field(ui, cui_id_from(ui, "time-field"), &t, cui_string("Time"), &time_state,
                       &time_value, (cui_time_options){.seconds = true, .disabled = disabled});
        char text[32];
        cui_time_format(time_value, true, text, sizeof(text));
        cui_label(ui, &t, cui_string(text));
        break;
    }
    case 42: {
        const cui_color palette[] = {
            {35, 102, 80, 255}, {88, 101, 242, 255}, {227, 134, 49, 255}, {221, 71, 82, 255}};
        cui_color_picker(ui, cui_id_from(ui, "color-picker"), &t, &color_state, &chosen_color,
                         (cui_color_picker_options){.alpha = true,
                                                    .disabled = disabled,
                                                    .width = width > 360 ? 280 : width - 68,
                                                    .palette = palette,
                                                    .palette_count = 4});
        break;
    }
    default:
        cui_label(ui, &t, cui_string("This component has no browser story yet."));
        break;
    }
    cui_end_box(ui);
    draw = cui_end_frame(ui);
    if (notification_clear) {
        cui_notification_clear(&notification_state, host_seconds);
        notification_clear = false;
        redraw = true;
    }
    while (notification_pushes > 0) {
        notification_push();
        notification_pushes--;
        redraw = true;
    }
    if (conversation_mutation) {
        if (conversation_mutation == 1 && conversation_count < 256) {
            conversation_ids[conversation_count++] = ++conversation_serial;
            cui_message_scroller_append(conversation, 1);
        } else if (conversation_mutation == 2 && conversation_count < 256) {
            memmove(conversation_ids + 1, conversation_ids,
                    conversation_count * sizeof(*conversation_ids));
            conversation_ids[0] = ++conversation_serial;
            conversation_count++;
            cui_message_scroller_prepend(conversation, 1);
        } else if (conversation_mutation == 3 && conversation_count) {
            memmove(conversation_ids, conversation_ids + 1,
                    (--conversation_count) * sizeof(*conversation_ids));
            cui_message_scroller_splice(conversation, 0, 1, 0);
        }
        conversation_labels();
        conversation_mutation = 0;
        redraw = true;
    }
    semantics = cui_semantics(ui, &semantic_count);
    return cui_last_error(ui) == CUI_OK;
}
int gallery_error(void) {
    return ui ? cui_last_error(ui) : CUI_ERROR_LIFECYCLE;
}
unsigned gallery_count(void) {
    return (unsigned)draw.count;
}
float gallery_command(unsigned i, unsigned field) {
    if (i >= draw.count)
        return 0;
    const cui_draw_command *c = &draw.commands[i];
    switch (field) {
    case 0:
        return c->type;
    case 1:
        return c->bounds.x;
    case 2:
        return c->bounds.y;
    case 3:
        return c->bounds.w;
    case 4:
        return c->bounds.h;
    case 5:
        return c->color.r;
    case 6:
        return c->color.g;
    case 7:
        return c->color.b;
    case 8:
        return c->color.a;
    case 9:
        return c->radius;
    case 10:
        return c->stroke;
    case 11:
        return c->font_size;
    case 12:
        return c->custom_corners;
    case 13:
        return c->corner_radii.top_left;
    case 14:
        return c->corner_radii.top_right;
    case 15:
        return c->corner_radii.bottom_left;
    case 16:
        return c->corner_radii.bottom_right;
    case 17:
        return c->custom_border_edges;
    case 18:
        return c->border_edges.left;
    case 19:
        return c->border_edges.right;
    case 20:
        return c->border_edges.top;
    case 21:
        return c->border_edges.bottom;
    default:
        return 0;
    }
}
const char *gallery_command_text(unsigned i) {
    return i < draw.count ? draw.commands[i].text.data : NULL;
}
unsigned gallery_command_length(unsigned i) {
    return i < draw.count ? (unsigned)draw.commands[i].text.length : 0;
}
unsigned gallery_bitmap_pixels(unsigned i) {
    return i < draw.count && draw.commands[i].type == CUI_DRAW_IMAGE
               ? (unsigned)(uintptr_t)draw.commands[i].bitmap.bitmap.pixels
               : 0;
}
double gallery_bitmap(unsigned i, unsigned field) {
    if (i >= draw.count || draw.commands[i].type != CUI_DRAW_IMAGE)
        return 0;
    const cui_bitmap_paint *paint = &draw.commands[i].bitmap;
    switch (field) {
    case 0: return paint->bitmap.width;
    case 1: return paint->bitmap.height;
    case 2: return (double)paint->bitmap.stride;
    case 3: return paint->source.x;
    case 4: return paint->source.y;
    case 5: return paint->source.w;
    case 6: return paint->source.h;
    case 7: return paint->opacity;
    case 8: return paint->grayscale;
    case 9: return paint->custom_tint;
    case 10: return paint->tint.r;
    case 11: return paint->tint.g;
    case 12: return paint->tint.b;
    case 13: return paint->tint.a;
    default: return 0;
    }
}
unsigned gallery_semantic_count(void) {
    return (unsigned)semantic_count;
}
float gallery_semantic(unsigned i, unsigned field) {
    if (i >= semantic_count)
        return 0;
    const cui_semantic *s = &semantics[i];
    switch (field) {
    case 0:
        return s->role;
    case 1:
        return s->bounds.x;
    case 2:
        return s->bounds.y;
    case 3:
        return s->bounds.w;
    case 4:
        return s->bounds.h;
    case 5:
        return s->disabled;
    case 6:
        return s->checked;
    case 7:
        return s->focused;
    case 8:
        return s->value;
    case 9:
        return s->tab_index;
    case 10:
        return s->details.expanded;
    case 11:
        return s->details.selected;
    case 12:
        return s->details.minimum;
    case 13:
        return s->details.maximum;
    case 14:
        return s->details.busy;
    case 15:
        return s->details.multiselectable;
    case 16:
        return s->details.orientation;
    case 17:
        return s->details.scrollable;
    case 18:
        return s->details.scroll_x;
    case 19:
        return s->details.scroll_y;
    case 20:
        return s->details.scroll_max_x;
    case 21:
        return s->details.scroll_max_y;
    case 22:
        return s->details.row_index;
    case 23:
        return s->details.column_index;
    case 24:
        return s->details.row_count;
    case 25:
        return s->details.column_count;
    case 26:
        return s->details.readonly;
    case 27:
        return s->details.required;
    case 28:
        return s->details.invalid;
    case 29:
        return s->details.multiline;
    case 30:
        return s->details.level;
    case 31:
        return s->details.pos_in_set;
    case 32:
        return s->details.set_size;
    case 33:
        return s->details.focusable;
    case 34:
        return s->details.pointer_activatable;
    default:
        return 0;
    }
}
const char *gallery_semantic_name(unsigned i) {
    return i < semantic_count ? semantics[i].name.data : NULL;
}
unsigned gallery_semantic_length(unsigned i) {
    return i < semantic_count ? (unsigned)semantics[i].name.length : 0;
}
const char *gallery_semantic_description(unsigned i) {
    return i < semantic_count ? semantics[i].details.description.data : NULL;
}
unsigned gallery_semantic_description_length(unsigned i) {
    return i < semantic_count ? (unsigned)semantics[i].details.description.length : 0;
}
void gallery_table_pin(int pinned) {
    if (!table || table_pinned == (pinned != 0))
        return;
    if (pinned)
        cui_table_move_column(table, 101, 0);
    for (size_t c = 0; c < 3; c++)
        table_columns[c].fixed_left = pinned && table_columns[c].key == 101;
    if (cui_table_bind(table, table_rows, table_mode == 3 ? 0 : 1000, table_columns, 3) ==
        CUI_TABLE_OK)
        table_pinned = pinned != 0;
}
void gallery_table_mode(int mode) {
    if (!table || mode == table_mode || mode < 0 || mode > 3)
        return;
    table_mode = mode;
    cui_table_bind(table, table_rows, mode == 3 ? 0 : 1000, table_columns, 3);
    table_state = (cui_table_state){.selection = table_selection, .selection_capacity = 64};
}
int gallery_table_selection_mode(int mode) {
    if (mode >= CUI_TABLE_SELECT_ROW && mode <= CUI_TABLE_SELECT_CELL)
        table_selection_mode = (cui_table_selection_mode)mode;
    return table_selection_mode;
}
void gallery_disclosure_options(int mode, int multiple) {
    if (mode >= 0 && mode <= 2 && mode != disclosure_mode) {
        disclosure_mode = mode;
        memset(accordion_motion, 0, sizeof(accordion_motion));
        memset(&collapsible_motion, 0, sizeof(collapsible_motion));
    }
    if (disclosure_multiple && !multiple) {
        bool kept = false;
        for (size_t i = 0; i < 3; i++) {
            if (open[i] && !kept)
                kept = true;
            else
                open[i] = false;
        }
    }
    disclosure_multiple = multiple != 0;
}
float gallery_disclosure_progress(int index) {
    const cui_disclosure_motion *motion = previous_story == 20 && index >= 0 && index < 3
                                              ? &accordion_motion[index]
                                              : &collapsible_motion;
    return motion->initialized ? motion->progress : -1;
}

int gallery_disclosure_actions(void) {
    return disclosure_actions;
}
void gallery_notification_options(int anchor, int kind, int autohide) {
    if (anchor >= 0 && anchor <= CUI_NOTIFICATION_RIGHT_CENTER)
        notification_state.settings.placement = (cui_notification_anchor)anchor;
    notification_kind = kind >= 0 && kind <= CUI_NOTIFICATION_ERROR ? kind : CUI_NOTIFICATION_INFO;
    notification_autohide = autohide != 0;
}
unsigned gallery_notification_stats(unsigned field) {
    switch (field) {
    case 0:
        return (unsigned)notification_state.count;
    case 1:
        return notification_clicks;
    case 2:
        return notification_actions;
    case 3:
        return notification_closes;
    default:
        return 0;
    }
}
float gallery_notification_entry(unsigned index, unsigned field) {
    if (index >= notification_state.count)
        return 0;
    const cui_notification_entry *entry = &notification_state.entries[index];
    switch (field) {
    case 0:
        return entry->phase;
    case 1:
        return (float)entry->remaining;
    case 2:
        return entry->bounds.x;
    case 3:
        return entry->bounds.y;
    case 4:
        return entry->bounds.w;
    case 5:
        return entry->bounds.h;
    default:
        return 0;
    }
}
void gallery_tag_options(int variant, int color, int outline, int pill) {
    tag_variant = variant >= 0 && variant <= CUI_TAG_PALETTE ? variant : 0;
    tag_color = color >= 0 && color <= CUI_STATUS_COLOR_ROSE ? color : CUI_STATUS_COLOR_BLUE;
    tag_outline = outline != 0;
    tag_pill = pill != 0;
}
void gallery_marker_options(int variant, int loading, int alignment, int icon) {
    marker_variant = variant >= 0 && variant <= CUI_MARKER_BORDER ? variant : 0;
    marker_loading = loading >= 0 && loading <= 2 ? loading : 0;
    marker_alignment = alignment >= 0 && alignment <= CUI_MARKER_ALIGN_END ? alignment : 0;
    marker_icon = icon != 0;
}
void gallery_otp_options(int groups, int masked) {
    otp_groups = groups >= 1 && groups <= 3 ? groups : 2;
    otp_masked = masked != 0;
}
int gallery_otp_completions(void) {
    return otp_completions;
}
int gallery_otp_cursor(void) {
    return otp_state.cursor_visible;
}
double gallery_next_wake(void) {
    return next_wake;
}
void gallery_group_mode(int mode) {
    if (mode >= 0 && mode <= 3)
        group_mode = mode;
}
int gallery_group_actions(void) {
    return group_actions;
}
void gallery_form_mode(int mode) {
    if (mode == form_mode || mode < 0 || mode > 3)
        return;
    form_mode = mode;
    if (mode == 0 || mode == 1) {
        cui_edit_select(form_email_edit, 0, cui_edit_get_text(form_email_edit).length);
        cui_edit_insert(form_email_edit,
                        cui_string(mode == 1 ? "ada.example.com" : "ada@example.com"));
    }
}
int gallery_form_submissions(void) {
    return form_submissions;
}
void gallery_surface_variant(int variant) {
    if (surface_variant != variant)
        sheet_surface.sheet_size = 0;
    surface_variant = variant;
}
int gallery_surface_open(int surface) {
    const cui_surface_state *states[] = {&dialog_surface, &alert_dialog_surface, &sheet_surface,
                                         &popover_surface, &nested_surface};
    return surface >= 0 && surface < 5 && states[surface]->overlay.open;
}
int gallery_alert_attempts(void) {
    return alert_attempts;
}
void gallery_input_mode(int mode) {
    input_mode = mode;
}
unsigned gallery_input_selection(unsigned field) {
    cui_edit_state state = cui_edit_get_state(edit);
    return (unsigned)(field == 0 ? state.anchor : field == 2 ? state.length : state.cursor);
}
int gallery_clicks(void) {
    return clicks;
}

int gallery_redraw(void) {
    return redraw || cui_needs_redraw(ui);
}
float gallery_hover_state(unsigned field) {
    switch (field) {
    case 0:
        return cui_hovered(ui, cui_id_from(ui, "hover-trigger"));
    case 1:
        return cui_hovered(ui, cui_id_from(ui, "hover-card"));
    case 2:
        return card_state.open;
    case 3:
        return card_state.pending;
    case 4:
        return card_state.elapsed;
    case 5:
        return host_dt;
    default:
        return 0;
    }
}

void gallery_time(float seconds, float dt, int reduced) {
    host_seconds = seconds;
    host_dt = dt > 0 && dt < 1 ? dt : 1.0f / 60;
    reduced_motion = reduced != 0;
}

int gallery_modal(void) {
    if (dialog_surface.overlay.open || nested_surface.overlay.open ||
        alert_dialog_surface.overlay.open || sheet_surface.overlay.open)
        return 1;
    return select_popup.overlay.open || menu_popup.overlay.open || combo_popup.overlay.open ||
           trap.open || date_picker.overlay.open || menu_tree.overlay.open ||
           context_menu.overlay.open;
}

int gallery_clipboard_request(void) {
    int request = clipboard_request;
    clipboard_request = 0;
    return request;
}
void gallery_clipboard_result(int okay, const char *text, size_t length) {
    if (!okay) {
        snprintf(clipboard_status, sizeof(clipboard_status),
                 "Clipboard access denied or unavailable.");
        return;
    }
    if (length > sizeof(clipboard_status) - 1) {
        length = sizeof(clipboard_status) - 1;
        while (length && ((unsigned char)text[length] & 0xc0) == 0x80)
            length--;
    }
    memcpy(clipboard_status, text, length);
    clipboard_status[length] = 0;
}

void gallery_density(int compact) {
    compact_density = compact != 0;
}

void gallery_selection_mode(int multiple) {
    if (multiple_mode != (multiple != 0)) {
        cui_overlay_close(ui, &select_popup.overlay);
        cui_overlay_close(ui, &combo_popup.overlay);
    }
    multiple_mode = multiple != 0;
}

uint64_t gallery_semantic_id(unsigned i) {
    return i < semantic_count ? semantics[i].id : 0;
}
uint64_t gallery_semantic_parent(unsigned i) {
    return i < semantic_count ? semantics[i].details.parent : 0;
}
uint64_t gallery_semantic_controls(unsigned i) {
    return i < semantic_count ? semantics[i].details.controls : 0;
}
uint64_t gallery_semantic_labelled_by(unsigned i) {
    return i < semantic_count ? semantics[i].details.labelled_by : 0;
}

void gallery_shimmer_mode(int mode) {
    if (mode != shimmer_mode) {
        shimmer_mode = mode;
        shimmer_started = host_seconds;
    }
}
float gallery_shimmer_phase(void) {
    return shimmer_state.last_phase;
}

void gallery_tabs_options(int mode, int size, int variant, int manual) {
    if (mode != tabs_mode) {
        tabs_mode = mode;
        memset(&tabs_scroll, 0, sizeof(tabs_scroll));
        selected_tab = 0;
        cui_overlay_close(ui, &tabs_overflow.menu.overlay);
    }
    tabs_size = size;
    tabs_variant = variant;
    tabs_manual = manual != 0;
}
float gallery_tabs_scroll(unsigned field) {
    switch (field) {
    case 0:
        return tabs_scroll.offset;
    case 1:
        return tabs_scroll.maximum;
    case 2:
        return tabs_scroll.viewport.x;
    case 3:
        return tabs_scroll.viewport.y;
    case 4:
        return tabs_scroll.viewport.w;
    case 5:
        return tabs_scroll.viewport.h;
    default:
        return 0;
    }
}

void gallery_dropdown_options(int variant, int size, int halves, int action, int anchor,
                              int outline, int selected) {
    dropdown_variant = variant;
    dropdown_size = size;
    dropdown_halves = halves;
    dropdown_action = action;
    dropdown_anchor = anchor;
    dropdown_outline = outline != 0;
    dropdown_selected = selected != 0;
}

void gallery_message_options(int alignment, int variant, int inset, int avatar, int scrollbar) {
    message_alignment = alignment;
    message_variant = variant;
    message_inset = inset;
    message_avatar = avatar != 0;
    message_scrollbar = scrollbar != 0;
}
unsigned gallery_message_actions(void) {
    return message_actions;
}
int gallery_checkbox_state(int state) {
    if (state >= CUI_CHECK_UNCHECKED && state <= CUI_CHECK_MIXED)
        check_state = (cui_check_state)state;
    return check_state;
}
double gallery_conversation(unsigned field) {
    switch (field) {
    case 0:
        return conversation_count;
    case 1:
        return conversation_result.view.scroll;
    case 2:
        return conversation_result.view.total;
    case 3:
        return conversation_result.view.following_tail;
    case 4:
        return conversation_result.view.scrolled_up;
    case 5:
        return conversation_result.jump_opacity;
    case 6:
        return conversation_result.fade_opacity;
    case 7:
        return conversation_rendered;
    default:
        return 0;
    }
}
