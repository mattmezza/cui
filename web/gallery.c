/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
#include "cui/cui_components.h"
#include "cui/cui_content.h"
#include "cui/cui_disclosure.h"
#include "cui/cui_dock.h"
#include "cui/cui_numeric.h"
#include "cui/cui_picker.h"
#include "cui/cui_plot.h"
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
static int radio, clicks;
static bool alert_visible = true, open[3] = {true, false, false}, collapsed;
static cui_id selected_tab;
static cui_theme current_theme;
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
static float slider = 40;
static char input_buffer[512] = "Hello, CUI";
static cui_edit *edit;
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
    return ui && edit && dock && query && number_state.edit && rebuilt == CUI_VIRTUAL_OK;
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
        cui_overlay_close(ui, &select_popup.overlay);
        cui_overlay_close(ui, &menu_popup.overlay);
        cui_overlay_close(ui, &combo_popup.overlay);
        cui_overlay_close(ui, &date_picker.overlay);
        cui_overlay_close(ui, &trap);
        previous_story = story;
    }
    redraw = false;
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
        cui_checkbox(ui, cui_id_from(ui, "check"), &t, cui_string("Receive updates"), &checked,
                     disabled);
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
        cui_input(ui, cui_id_from(ui, "input"), &t, cui_string("Display name"), edit, disabled);
        break;
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
        cui_accordion(
            ui, cui_id_from(ui, "accordion"), &t, items, 3, open,
            (cui_accordion_options){.collapsible = true, .disabled = disabled, .bordered = true},
            panel, NULL);
        break;
    }
    case 21:
        cui_collapsible(ui, cui_id_from(ui, "collapsible"), &t, cui_string("Show details"),
                        &collapsed, disabled, panel, NULL);
        break;
    case 22: {
        cui_choice_item items[3] = {{cui_id_from(ui, "tab1"), cui_string("Overview"), false},
                                    {cui_id_from(ui, "tab2"), cui_string("Activity"), false},
                                    {cui_id_from(ui, "tab3"), cui_string("Settings"), false}};
        if (!selected_tab)
            selected_tab = items[0].id;
        cui_tabs(ui, cui_id_from(ui, "tabs"), &t, items, 3, &selected_tab,
                 (cui_tabs_options){.disabled = disabled, .variant = CUI_TAB_UNDERLINE}, panel,
                 NULL);
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
    case 30:
    case 31:
    case 32: {
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
    semantics = cui_semantics(ui, &semantic_count);
    return cui_last_error(ui) == CUI_OK;
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
int gallery_clicks(void) {
    return clicks;
}

int gallery_redraw(void) {
    return redraw;
}

void gallery_time(float seconds, float dt, int reduced) {
    host_seconds = seconds;
    host_dt = dt > 0 && dt < 1 ? dt : 1.0f / 60;
    reduced_motion = reduced != 0;
}

int gallery_modal(void) {
    return select_popup.overlay.open || menu_popup.overlay.open || combo_popup.overlay.open ||
           trap.open || date_picker.overlay.open;
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
