/* SPDX-License-Identifier: MIT */
#include "../examples/composed_apps/app.h"
#include "gallery_avatars.h"
#include "touch_page_host.h"
static cui_touch_page_host page_touch_host;
static unsigned page_touch_cancels;
#include "cui/cui_calendar_presentation.h"
#include "cui/cui_carousel.h"
#include "cui/cui_chart.h"
#include "cui/cui_choice.h"
#include "cui/cui_chrome.h"
#include "cui/cui_color_entry.h"
#include "cui/cui_command.h"
#include "cui/cui_components.h"
#ifdef CUI_GALLERY_REGEX
#include "cui/cui_regex.h"
#endif
#include "cui/cui_content.h"
#include "cui/cui_date_picker_calendar.h"
#include "cui/cui_description.h"
#include "cui/cui_disclosure.h"
#include "cui/cui_dock.h"
#include "cui/cui_effects.h"
#include "cui/cui_form.h"
#include "cui/cui_icon.h"
#include "cui/cui_kbd.h"
#include "cui/cui_media.h"
#include "cui/cui_message.h"
#include "cui/cui_numeric.h"
#include "cui/cui_otp.h"
#include "cui/cui_picker.h"
#include "cui/cui_plot.h"
#include "cui/cui_questionnaire.h"
#include "cui/cui_radio_group.h"
#include "cui/cui_resizable.h"
#include "cui/cui_root.h"
#include "cui/cui_scroll.h"
#include "cui/cui_settings.h"
#include "cui/cui_sidebar.h"
#include "cui/cui_speech.h"
#include "cui/cui_status.h"
#include "cui/cui_stepper_navigation.h"
#include "cui/cui_surface.h"
#include "cui/cui_table.h"
#include "cui/cui_text_view.h"
#include "cui/cui_textarea.h"
#include "cui/cui_toolbar.h"
#include "cui/cui_video_controls.h"
#include "cui/cui_virtual.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__wasm__)
__attribute__((import_module("host"),
               import_name("measure"))) extern float host_measure(const char *, size_t, float);
__attribute__((import_module("host"), import_name("measure_styled"))) extern float
host_measure_styled(const char *, size_t, float, int, int, int, uint32_t);
__attribute__((import_module("host"), import_name("font_metrics"))) extern int
host_font_metrics(float, int, int, int, uint32_t, float *);
#else
static float host_measure(const char *s, size_t n, float size) {
    (void)s;
    return n * size * .6f;
}
static float host_measure_styled(const char *s, size_t n, float size, int weight, int italic,
                                 int monospace, uint32_t family) {
    (void)family;
    (void)weight;
    (void)italic;
    (void)monospace;
    return host_measure(s, n, size);
}
static int host_font_metrics(float size, int weight, int italic, int monospace, uint32_t family,
                             float *out) {
    (void)family;
    (void)size;
    (void)weight;
    (void)italic;
    (void)monospace;
    (void)out;
    return 0;
}
#endif
static char editor_buffer[8192] =
    "/* A native CUI component */\n#include <cui/cui_components.h>\n\nvoid "
    "draw_profile(cui_context *ui, const cui_theme *theme) {\n    cui_label(ui, theme, "
    "cui_string(\"Welcome, Ada\"));\n    if (cui_button(ui, cui_id_from(ui, \"save\"), theme,\n    "
    "               cui_string(\"Save profile\"), false)) {\n        save_profile();\n    }\n}\n";
static char editor_query_buffer[128], editor_replacement_buffer[128], editor_replace_scratch[8192];
static cui_edit *editor_replacement;
static cui_editor_match editor_matches[512];
static cui_editor_matcher editor_matcher;
static cui_edit *editor_edit, *editor_query;
static cui_editor_state editor_state;
static cui_edit_selection editor_selections[8];
static cui_edit_selection_snapshot editor_selection_history[65];
static cui_editor_cursor_anchor editor_cursor_anchors[8];
static cui_editor_search_state editor_search;
static const cui_editor_completion_item editor_completions[] = {
    {{"cui_label", 9}, {"Draw a themed label", 18}, {"cui_label", 9}},
    {{"cui_button", 10}, {"Draw an interactive button", 26}, {"cui_button", 10}},
    {{"cui_begin_box", 13}, {"Compose a layout", 16}, {"cui_begin_box", 13}},
};
static cui_editor_fold editor_folds[] = {{.start_line = 3, .end_line = 9}};
static cui_textarea_line editor_lines[512];
double gallery_editor_stat(int field) {
    cui_edit_state state = cui_edit_get_state(editor_edit);
    switch (field) {
    case 0:
        return (double)state.cursor;
    case 1:
        return (double)state.anchor;
    case 2:
        return (double)state.length;
    case 3:
        return (double)editor_matcher.count;
    case 4:
        return editor_search.open;
    case 5:
        return (double)editor_state.view.fold_count;
    case 6:
        return editor_folds[0].collapsed;
    case 7:
        return editor_state.completion.open;
    case 8:
        return (double)editor_state.completion.selected;
    case 9:
        return (double)editor_state.completion.count;
    case 10:
        return editor_search.replace_mode;
    case 11:
        return editor_matcher.case_insensitive;
    case 12:
        return (double)editor_matcher.current;
    case 13: {
        size_t count = 0;
        cui_edit_get_selections(editor_edit, &count);
        return (double)count;
    }
    case 14:
        return editor_state.view.viewport.x;
    case 15:
        return editor_state.view.viewport.y;
    case 16:
        return editor_state.view.viewport.w;
    case 17:
        return editor_state.view.line_height;
    case 18: {
        size_t count = 0, selected = 0;
        const cui_edit_selection *selections = cui_edit_get_selections(editor_edit, &count);
        for (size_t i = 0; i < count; ++i)
            selected += selections[i].cursor > selections[i].anchor
                            ? selections[i].cursor - selections[i].anchor
                            : selections[i].anchor - selections[i].cursor;
        return (double)selected;
    }
    default:
        return -1;
    }
}
static cui_context *ui;
static cui_theme current_theme;
static int scrollbar_policy, scrollbar_width = 10, scrollbar_radius;
static bool scrollbar_track = true;
void gallery_scrollbar_options(int policy, int width, int radius, int track) {
    scrollbar_policy = policy >= 0 && policy <= 3 ? policy : 0;
    scrollbar_width = width >= 4 && width <= 24 ? width : 10;
    scrollbar_radius = radius >= 0 && radius <= 12 ? radius : 0;
    scrollbar_track = track != 0;
}
static void gallery_progress_content(cui_context *, void *);
static int gallery_preset_index = -1;
static uint32_t typography_family;
static float typography_size = 24;
static int typography_weight;
static bool typography_italic;
static bool typography_gradient;
void gallery_typography_options(uint32_t family, float size, int weight, int italic, int gradient) {
    typography_gradient = gradient != 0;
    typography_family = family;
    typography_size = isfinite(size) && size >= 8 && size <= 128 ? size : 24;
    typography_weight = weight >= 0 && weight <= 3 ? weight : 0;
    typography_italic = italic != 0;
}

void gallery_theme_preset(int index) {
    gallery_preset_index = index >= 0 && (size_t)index < cui_theme_preset_count() ? index : -1;
}

int gallery_theme_channel(int field, int channel) {
    cui_color color = field == 1   ? current_theme.text
                      : field == 2 ? current_theme.primary
                                   : current_theme.background;
    return channel == 0 ? color.r : channel == 1 ? color.g : channel == 2 ? color.b : color.a;
}
static bool redraw, reduced_motion, compact_density;
static bool progress_indeterminate;
static int progress_shape;
static unsigned char progress_circle_pixels[1056 * 1056 * 4];
static cui_progress_circle_state progress_circle_state = {progress_circle_pixels,
                                                          sizeof progress_circle_pixels, 0};
static cui_progress_motion progress_motion;
static cui_loading_motion spinner_motion, skeleton_motion;
static float spinner_size = 16;
static int spinner_icon, spinner_color, spinner_easing;
void gallery_spinner_options(float size, int icon, int color, int easing) {
    spinner_size = isfinite(size) && size >= 1 && size <= 128 ? size : 16;
    spinner_icon = icon >= 0 && icon <= 2 ? icon : 0;
    spinner_color = color >= 0 && color <= 2 ? color : 0;
    spinner_easing = easing == 1;
}
static float gallery_spinner_linear(float phase, void *user) {
    (void)user;
    return phase;
}
static bool gallery_spinner_icon(cui_context *context, cui_id id, const cui_theme *theme,
                                 float size, cui_color color, float radians, void *user) {
    return cui_icon_named(context, id, theme, cui_string(user),
                          (cui_icon_options){.size = CUI_ICON_CUSTOM,
                                             .custom_size = size,
                                             .custom_color = true,
                                             .color = color,
                                             .custom_transform = true,
                                             .transform = cui_icon_rotation(radians)}) ==
           CUI_ICON_OK;
}

static float host_seconds, host_dt = 1.0f / 60;
static cui_draw_list draw;
static const cui_semantic *semantics;
static size_t semantic_count;
static bool checked, enabled = true, toggled;
static int description_columns = 3, description_vertical, description_plain, description_size;
static cui_scroll_state description_preview_scroll;
static int status_right = 1, status_custom;
static int resizable_vertical, resizable_disabled;
static unsigned resizable_document;
static cui_resizable_slot resizable_slots[2];
static cui_resizable_state resizable_state;
static cui_carousel_state carousel_state;
static cui_carousel_geometry carousel_geometry[4];
static unsigned carousel_changes;
static int carousel_axis, carousel_looping;
static cui_settings_state settings_state;
static cui_scroll_state settings_preview_scroll;
static cui_settings_page_state settings_pages_state[2];
static char settings_search_buffer[96], settings_name_buffer[96] = "Ada";
static char settings_number_buffer[64] = "15";
static cui_edit *settings_search_edit, *settings_name_edit;
static cui_input_state settings_name_state;
static cui_number_state settings_number_state;
static bool settings_email = true, settings_desktop, settings_usage;
static double settings_refresh = 15;
static cui_id settings_mode;
static cui_settings_dropdown_state settings_mode_menu;
static cui_menu_entry settings_mode_entries[3];
static bool settings_mode_checks[3];
static int settings_size, settings_variant;
static cui_settings_result settings_result;
static char command_query_buffer[256];
static cui_edit *command_query;
static cui_command_row command_rows[32];
static cui_command_state command_state;
static cui_surface_state command_surface;
static int command_modal, command_loading, command_filterable = 1;
static unsigned command_actions, command_confirms, command_cancels;
static cui_command_index command_last;
static bool command_focus_pending;
static cui_text_view_state text_view_state;
static char text_view_source[4096], text_view_rendered[4096], text_view_document[4096];
static cui_text_view_mapping text_view_mapping[4096];
static cui_text_view_span text_view_spans[512];
static cui_text_view_block text_view_blocks[256];
static cui_text_view_glyph text_view_glyphs[4096];
static cui_text_view_highlight text_view_highlights[32];
static int text_view_format, text_view_scrollable, text_view_fade;
static cui_text_view_motion text_view_motion = {.duration = .35f};
static int text_view_links;
static cui_text_view_error text_view_error;
static bool gallery_text_view_load(void);
static unsigned char chart_pixels[1600 * 800 * 4];
static cui_chart_state chart_state;
static int chart_kind, chart_curve, chart_dots, chart_labels, chart_animate;
static bool chart_custom_rows;
static int chart_tooltip_appearance;
static unsigned chart_tooltip_calls;
static cui_plot_result chart_result;
static unsigned chart_revision = 1;
static cui_questionnaire_state questionnaire_state;
static cui_questionnaire_item_runtime questionnaire_runtime[2];
static cui_questionnaire_choice_runtime questionnaire_choice_runtime[3];
static char questionnaire_text[768];
static char questionnaire_input_buffer[128];
static cui_edit *questionnaire_edit;
static cui_questionnaire_input_state questionnaire_input_view;
static unsigned questionnaire_events[4];
static int questionnaire_size, questionnaire_numbers, questionnaire_custom;
static const cui_questionnaire_choice questionnaire_choices[] = {
    {.value = {"build", 5},
     .label = {"Build a native app", 18},
     .description = {"Portable desktop UI", 19}},
    {.value = {"explore", 7},
     .label = {"Explore components", 18},
     .description = {"Browse the CUI catalog", 22}},
    {.value = {"prototype", 9},
     .label = {"Prototype a screen", 18},
     .description = {"Try a focused example", 21}}};
static void questionnaire_indicator(cui_context *context, cui_id id,
                                    const cui_questionnaire_choice_view *view, void *user) {
    const cui_theme *theme = user;
    (void)cui_icon_named(context, id, theme,
                         view->selected ? cui_string("circle-check") : cui_string("circle-user"),
                         (cui_icon_options){.size = CUI_ICON_SMALL,
                                            .color = view->selected ? theme->primary : theme->muted,
                                            .custom_color = true});
}
static void questionnaire_content(cui_context *context, cui_id id,
                                  const cui_questionnaire_choice_view *view, void *user) {
    const cui_theme *theme = user;
    (void)id;
    cui_text(context, questionnaire_choices[view->choice].label, theme->font_size, theme->text);
    cui_text(context, cui_string("Custom content slot"), theme->font_size - 2, theme->muted);
}
static void questionnaire_shortcut(cui_context *context, cui_id id,
                                   const cui_questionnaire_choice_view *view, void *user) {
    const cui_theme *theme = user;
    (void)id;
    char key[2] = {view->shortcut ? view->shortcut : '-', 0};
    cui_text(context, cui_string(key), theme->font_size - 2, theme->muted);
}
static cui_questionnaire_item questionnaire_items[2];
static void gallery_questionnaire_event(const cui_questionnaire_state *state,
                                        cui_questionnaire_event event, void *user) {
    (void)state;
    (void)user;
    if ((unsigned)event.kind < 4)
        questionnaire_events[event.kind]++;
    redraw = true;
}
void gallery_questionnaire_options(int size, int numbers, int custom) {
    questionnaire_size = size >= 0 && size <= 3 ? size : 0;
    questionnaire_numbers = numbers != 0;
    questionnaire_custom = custom != 0;
}
float gallery_questionnaire_stats(int field) {
    return field == 0    ? (float)questionnaire_state.current
           : field == 1  ? (float)questionnaire_state.complete
           : field == 2  ? (float)questionnaire_state.pending_confirm
           : field == 3  ? (float)questionnaire_events[CUI_QUESTIONNAIRE_CURRENT_CHANGED]
           : field == 4  ? (float)questionnaire_events[CUI_QUESTIONNAIRE_ANSWER_CHANGED]
           : field == 5  ? (float)questionnaire_events[CUI_QUESTIONNAIRE_COMPLETED]
           : field == 6  ? (float)questionnaire_events[CUI_QUESTIONNAIRE_SUBMIT]
           : field == 7  ? (float)cui_questionnaire_item_error(&questionnaire_state,
                                                               questionnaire_state.current)
                               .kind
           : field == 8  ? (float)cui_questionnaire_choice_selected(&questionnaire_state, 0, 0)
           : field == 9  ? (float)cui_questionnaire_choice_selected(&questionnaire_state, 0, 1)
           : field == 10 ? (float)cui_questionnaire_choice_selected(&questionnaire_state, 0, 2)
           : field == 11 ? (float)cui_questionnaire_item_status(&questionnaire_state, 1)
           : field == 12 ? (float)cui_questionnaire_freeform(&questionnaire_state, 1).length
                         : 0;
}
static cui_speech_state speech_state;
static char speech_transcript[512];
static cui_speech_message speech_messages[48];
static unsigned char speech_queue[32768];
static cui_speech_sink speech_sink;
static unsigned speech_events[5], speech_recognizer_closed, speech_capture_closed;
static unsigned speech_samples, speech_finishes;
static cui_speech_error speech_last_error;
static int speech_size, speech_unavailable;
static bool gallery_speech_available(void *user) {
    (void)user;
    return !speech_unavailable;
}
static cui_speech_error gallery_speech_start_recognizer(void *user, cui_speech_sink sink,
                                                        void **session) {
    (void)user;
    speech_sink = sink;
    *session = &speech_state;
    (void)cui_speech_report_ready(sink);
    return CUI_SPEECH_OK;
}
static cui_speech_error gallery_speech_start_input(void *user, cui_speech_format format,
                                                   cui_speech_sink sink, void **session) {
    (void)user;
    (void)format;
    (void)sink;
    *session = &speech_state;
    return CUI_SPEECH_OK;
}
static void gallery_speech_push_audio(void *user, void *session, const int16_t *pcm, size_t count) {
    (void)user;
    (void)session;
    (void)pcm;
    speech_samples += (unsigned)count;
}
static void gallery_speech_finish_recognizer(void *user, void *session) {
    (void)user;
    (void)session;
    speech_finishes++;
}
static void gallery_speech_close_recognizer(void *user, void *session) {
    (void)user;
    (void)session;
    speech_recognizer_closed++;
}
static void gallery_speech_close_input(void *user, void *session) {
    (void)user;
    (void)session;
    speech_capture_closed++;
}
static void gallery_speech_event(void *user, cui_speech_event event) {
    (void)user;
    speech_events[event.type]++;
    if (event.type == CUI_SPEECH_ERROR)
        speech_last_error = event.error;
}
void gallery_speech_options(int size, int unavailable) {
    speech_size = size >= CUI_SPEECH_MEDIUM && size <= CUI_SPEECH_XSMALL ? size : 0;
    speech_unavailable = unavailable != 0;
}
float gallery_speech_stats(int field) {
    return field == 0    ? (float)speech_state.status
           : field == 1  ? (float)cui_speech_transcript(&speech_state).length
           : field == 2  ? (float)speech_state.meter.length
           : field == 3  ? (float)speech_events[CUI_SPEECH_STARTED]
           : field == 4  ? (float)speech_events[CUI_SPEECH_PARTIAL]
           : field == 5  ? (float)speech_events[CUI_SPEECH_FINAL]
           : field == 6  ? (float)speech_events[CUI_SPEECH_CANCELLED]
           : field == 7  ? (float)speech_events[CUI_SPEECH_ERROR]
           : field == 8  ? (float)speech_samples
           : field == 9  ? (float)speech_capture_closed
           : field == 10 ? (float)speech_recognizer_closed
           : field == 11 ? (float)speech_finishes
                         : (float)speech_last_error;
}
const char *gallery_speech_text(void) {
    return cui_speech_transcript(&speech_state).data;
}
void gallery_chart_options(int kind, int curve, int dots, int labels, int animate) {
    if (kind < CUI_CHART_LINE || kind > CUI_CHART_SANKEY)
        kind = CUI_CHART_LINE;
    if (chart_kind != kind || chart_curve != curve || chart_dots != dots ||
        chart_labels != labels || chart_animate != animate) {
        chart_revision++;
        redraw = true;
    }
    chart_kind = kind;
    chart_curve = curve;
    chart_dots = dots;
    chart_labels = labels;
    chart_animate = animate;
}
void gallery_chart_content(int custom_rows) {
    bool next = custom_rows != 0;
    if (next != chart_custom_rows) {
        chart_custom_rows = next;
        redraw = true;
    }
}
void gallery_chart_appearance(int appearance) {
    if (appearance < 0 || appearance > 2)
        return;
    if (chart_tooltip_appearance != appearance) {
        chart_tooltip_appearance = appearance;
        redraw = true;
    }
}
static bool gallery_chart_tooltip(const cui_chart_tooltip_info *info,
                                  cui_chart_tooltip_builder *builder, void *user) {
    (void)user;
    chart_tooltip_calls++;
    if (info->has_fallback_title && !cui_chart_tooltip_set_title(builder, info->fallback_title))
        return false;
    for (size_t i = 0; i < info->value_count; i++) {
        char value[48];
        int written = snprintf(value, sizeof value, "%.0f", info->values[i]);
        if (written < 0 || (size_t)written >= sizeof value)
            return false;
        cui_chart_tooltip_row row = {.has_swatch = true,
                                     .swatch = info->series_colors[i],
                                     .label = info->series_labels[i].length
                                                  ? info->series_labels[i]
                                                  : cui_string("Activity"),
                                     .value = {value, (size_t)written}};
        if (!cui_chart_tooltip_add_row(builder, &row))
            return false;
    }
    return cui_chart_tooltip_add_row(
        builder, &(cui_chart_tooltip_row){.label = {"Reporting period", 16}, .value = {"2026", 4}});
}
float gallery_chart_stats(int field) {
    return field == 0   ? (float)chart_result
           : field == 1 ? (float)chart_state.hovered
           : field == 2 ? chart_state.rendered_reveal_progress
           : field == 3 ? (float)chart_state.revision
           : field == 5 ? (float)chart_tooltip_calls
           : field == 6 ? (float)chart_custom_rows
           : field == 7 ? (float)chart_tooltip_appearance
                        : (float)chart_revision;
}
typedef struct {
    unsigned builds, prepares, styles, overlays, content, dialogs, sheets, notices;
} gallery_root_counts;
static gallery_root_counts root_counts;
static cui_root_registry root_registry;
static cui_root_plugin root_plugin_entries[1];
static cui_root_plugin_instance root_plugin_instances[1];
static cui_root root_view;
static cui_root_surface root_dialog_slots[2];
static cui_surface_state root_dialog_surface, root_sheet_surface;
static cui_notification_state root_notices;
static cui_notification_entry root_notice_entries[4];
static char root_notice_titles[4][40];
static bool root_dialog_checked;
static bool root_disabled;
static void *gallery_root_plugin_build(void *registration_user, void *window_user) {
    (void)window_user;
    gallery_root_counts *counts = registration_user;
    counts->builds++;
    return counts;
}
static void gallery_root_plugin_prepare(cui_context *context, void *state) {
    (void)context;
    ((gallery_root_counts *)state)->prepares++;
}
static void gallery_root_plugin_style(cui_context *context, void *state, cui_root_style *style) {
    (void)context;
    ((gallery_root_counts *)state)->styles++;
    style->padding = 18;
}
static void gallery_root_plugin_overlay(cui_context *context, void *state, cui_rect viewport) {
    ((gallery_root_counts *)state)->overlays++;
    cui_id id = cui_id_from(context, "root-plugin-overlay");
    cui_begin_absolute_box(context, id,
                           (cui_box){.width = 152,
                                     .height = 30,
                                     .padding = 5,
                                     .background = current_theme.secondary,
                                     .radius = current_theme.radius},
                           (cui_rect){viewport.w - 168, 12, 152, 30});
    cui_interact(context, id, CUI_ROLE_GROUP, cui_string("Root plugin overlay"), false, false, 0);
    cui_text(context, cui_string("Plugin overlay"), 12, current_theme.text);
    cui_end_box(context);
}
static void gallery_root_dialog_content(cui_context *context, void *user) {
    (void)user;
    root_counts.dialogs++;
    cui_checkbox(context, cui_id_from(context, "root-dialog-option"), &current_theme,
                 cui_string("Enable plugin option"), &root_dialog_checked, false);
    if (cui_button(context, cui_id_from(context, "root-dialog-apply"), &current_theme,
                   cui_string("Apply root dialog"), false))
        cui_surface_close(context, &root_dialog_surface, CUI_SURFACE_ACCEPTED);
}
static void gallery_root_sheet_content(cui_context *context, void *user) {
    (void)user;
    root_counts.sheets++;
    cui_label(context, &current_theme, cui_string("Sheet mounted by Root"));
    if (cui_button(context, cui_id_from(context, "root-sheet-close"), &current_theme,
                   cui_string("Close root sheet"), false))
        cui_surface_close(context, &root_sheet_surface, CUI_SURFACE_CLOSED);
}
static void gallery_root_content(cui_context *context, void *user) {
    (void)user;
    root_counts.content++;
    cui_begin_box(context, cui_id_from(context, "root-app-content"),
                  (cui_box){.width = 260, .gap = 10});
    cui_label(context, &current_theme, cui_string("Application content renders once"));
    cui_id dialog = cui_id_from(context, "root-dialog-trigger");
    if (cui_button(context, dialog, &current_theme, cui_string("Open root dialog"), root_disabled))
        (void)cui_root_dialog_open(
            &root_view, context,
            (cui_root_surface){.surface = &root_dialog_surface,
                               .title = cui_string("Root dialog"),
                               .description = cui_string("Mounted after application content."),
                               .options = {.width = 400, .height = 220, .margin = 16},
                               .content = gallery_root_dialog_content},
            dialog);
    cui_id sheet = cui_id_from(context, "root-sheet-trigger");
    if (cui_button(context, sheet, &current_theme, cui_string("Open root sheet"), root_disabled))
        (void)cui_root_sheet_open(
            &root_view, context,
            (cui_root_surface){.surface = &root_sheet_surface,
                               .title = cui_string("Root sheet"),
                               .description = cui_string("Notifications avoid this sheet."),
                               .options = {.edge = CUI_SHEET_RIGHT, .width = 320, .margin = 16},
                               .content = gallery_root_sheet_content},
            sheet);
    if (cui_button(context, cui_id_from(context, "root-notify"), &current_theme,
                   cui_string("Notify workspace"), root_disabled) &&
        root_counts.notices < 4) {
        unsigned index = root_counts.notices++;
        snprintf(root_notice_titles[index], sizeof(root_notice_titles[index]), "Root notice %u",
                 index + 1);
        cui_notification notice = cui_notification_default();
        notice.id = (cui_notification_id){2, index + 1};
        notice.title = cui_string(root_notice_titles[index]);
        notice.message = cui_string("Mounted above app content.");
        notice.autohide = false;
        (void)cui_root_notification_push(&root_view, notice, host_seconds);
    }
    cui_end_box(context);
}
float gallery_root_stats(int field) {
    return field == 0   ? root_counts.builds
           : field == 1 ? root_counts.prepares
           : field == 2 ? root_counts.styles
           : field == 3 ? root_counts.overlays
           : field == 4 ? root_counts.content
           : field == 5 ? root_counts.dialogs
           : field == 6 ? root_counts.sheets
           : field == 7 ? root_notices.count
           : field == 8 ? root_dialog_surface.overlay.open
           : field == 9 ? root_sheet_surface.overlay.open
                        : 0;
}
static void gallery_command_action(void *user) {
    (void)user;
    command_actions++;
    redraw = true;
}
static cui_str gallery_command_binding(void *user) {
    (void)user;
    return cui_string("Ctrl + O");
}
static void gallery_command_confirm(cui_command_index index, void *user) {
    (void)user;
    command_last = index;
    command_confirms++;
    redraw = true;
}
static void gallery_command_cancel(void *user) {
    (void)user;
    command_cancels++;
    if (command_modal)
        cui_surface_request_close(ui, &command_surface, CUI_SURFACE_CANCELLED);
    redraw = true;
}
void gallery_command_palette_options(int modal, int loading, int filterable) {
    command_modal = modal != 0;
    command_loading = loading != 0;
    command_filterable = filterable != 0;
}
float gallery_command_palette_stats(int field) {
    return field == 0   ? (float)command_state.row_count
           : field == 1 ? (float)command_state.matched_count
           : field == 2 ? (float)command_confirms
           : field == 3 ? (float)command_actions
           : field == 4 ? (float)command_cancels
           : field == 5 ? (float)cui_edit_get_text(command_query).length
           : field == 6 ? (float)command_last.section
           : field == 7 ? (float)command_last.row
           : field == 8 ? (float)command_surface.overlay.open
                        : 0;
}
void gallery_settings_options(int size, int variant) {
    settings_size =
        size >= CUI_SETTINGS_MEDIUM && size <= CUI_SETTINGS_LARGE ? size : CUI_SETTINGS_MEDIUM;
    settings_variant = variant >= CUI_SETTINGS_NORMAL && variant <= CUI_SETTINGS_OUTLINE
                           ? variant
                           : CUI_SETTINGS_NORMAL;
}
float gallery_settings_stats(int field) {
    return field == 0   ? (float)settings_state.selected_page
           : field == 1 ? (float)settings_result.visible_items
           : field == 2 ? (float)settings_result.visible_pages
           : field == 3 ? (float)settings_email
           : field == 4 ? (float)settings_desktop
           : field == 5 ? (float)settings_usage
           : field == 6 ? (float)settings_refresh
           : field == 7 ? settings_state.sidebar_width
           : field == 8 ? (settings_mode == cui_id_from(ui, "settings-standard")  ? 0.f
                           : settings_mode == cui_id_from(ui, "settings-focused") ? 1.f
                                                                                  : 2.f)
           : field == 9 ? (float)settings_mode_menu.menu.overlay.open
                        : 0;
}
void gallery_carousel_options(int axis, int looping) {
    carousel_axis = axis != 0;
    carousel_looping = looping != 0;
}
float gallery_carousel_stats(int field) {
    return field == 0   ? (float)carousel_state.selected
           : field == 1 ? (float)carousel_changes
           : field == 2 ? carousel_state.offset
           : field == 3 ? carousel_state.target
                        : 0;
}
static int icon_layout;
static int icon_choice, icon_size = CUI_ICON_MEDIUM, icon_rotate, icon_color, icon_loaded = -1;
static cui_icon_state icon_state;
static unsigned empty_created;
static bool empty_disabled;
static void gallery_empty_media(cui_context *ctx, const cui_theme *theme, cui_color color,
                                void *user) {
    (void)user;
    cui_icon_named(
        ctx, cui_id_from(ctx, "empty-folder"), theme, cui_string("folder"),
        (cui_icon_options){
            .size = CUI_ICON_CUSTOM, .custom_size = 20, .color = color, .custom_color = true});
}
static void gallery_empty_action(cui_context *ctx, const cui_theme *theme, cui_color color,
                                 void *user) {
    (void)color;
    (void)user;
    if (cui_button(ctx, cui_id_from(ctx, "empty-create"), theme, cui_string("Create project"),
                   empty_disabled))
        empty_created++;
}

static int toolbar_size = CUI_TOOLBAR_MEDIUM, toolbar_grouped = 1, toolbar_show_input = 1;
static bool toolbar_disabled;
static unsigned toolbar_activations[4];
static char toolbar_query_buffer[64] = "find";
static cui_edit *toolbar_query;
static cui_input_state toolbar_query_state;
static unsigned toolbar_query_changes;
static const char *const toolbar_document[] = {"Find a quiet place to write.",
                                               "Save your draft before sharing it.",
                                               "Find collaborators in the workspace."};
static bool toolbar_contains(const char *line, cui_str query) {
    if (!query.length)
        return false;
    size_t length = strlen(line);
    for (size_t i = 0; i + query.length <= length; i++) {
        size_t j = 0;
        for (; j < query.length; j++) {
            unsigned char a = (unsigned char)line[i + j], b = (unsigned char)query.data[j];
            if (a >= 'A' && a <= 'Z')
                a += 'a' - 'A';
            if (b >= 'A' && b <= 'Z')
                b += 'a' - 'A';
            if (a != b)
                break;
        }
        if (j == query.length)
            return true;
    }
    return false;
}
static int sidebar_mode, sidebar_side, sidebar_collapsed;
static int sidebar_selected;
static unsigned sidebar_actions;
static cui_sidebar_state sidebar_state;
static cui_sidebar_cache sidebar_cache[2];
static cui_sidebar_item_state sidebar_items[12];
static int sidebar_item_values[12];
static int sidebar_group_values[2] = {0, 1};
static char textarea_buffer[4096] = "First line of a project note.\nSecond line with enough words "
                                    "to wrap when the field narrows.\nThird line.";
static cui_edit *textarea_edit;
static cui_textarea_state textarea_state;
static cui_textarea_line textarea_lines[256];
static cui_textarea_result textarea_result;
static int textarea_rows = 4, textarea_auto_grow, textarea_readonly, textarea_no_wrap;
static void sidebar_toggle(bool collapsed, void *user) {
    (void)user;
    sidebar_collapsed = collapsed;
    redraw = true;
}
static void sidebar_activate(void *user) {
    sidebar_selected = *(int *)user;
    sidebar_actions++;
    redraw = true;
}
static void sidebar_header_inner(cui_context *context, const cui_theme *theme, bool collapsed,
                                 void *user) {
    (void)user;
    cui_sidebar_toggle_button(
        context, cui_id_from(context, "sidebar-toggle"), theme,
        (cui_sidebar_toggle_options){.side = (cui_sidebar_side)sidebar_side,
                                     .collapsed = sidebar_collapsed != 0,
                                     .accessibility_label = cui_string("Toggle sidebar"),
                                     .on_change = sidebar_toggle});
    if (!collapsed)
        cui_label(context, theme, cui_string("Workspace"));
}
static void sidebar_header_content(cui_context *context, const cui_theme *theme, bool collapsed,
                                   void *user) {
    (void)user;
    cui_sidebar_header(
        context, cui_id_from(context, "sidebar-header"), theme,
        (cui_sidebar_section_options){.collapsed = collapsed, .content = sidebar_header_inner});
}
static void sidebar_footer_inner(cui_context *context, const cui_theme *theme, bool collapsed,
                                 void *user) {
    (void)user;
    cui_icon_named(context, cui_id_from(context, "sidebar-footer-icon"), theme,
                   cui_string("settings"), (cui_icon_options){.size = CUI_ICON_SMALL});
    if (!collapsed)
        cui_label(context, theme, cui_string("Settings"));
}
static void sidebar_footer_content(cui_context *context, const cui_theme *theme, bool collapsed,
                                   void *user) {
    (void)user;
    cui_sidebar_footer(
        context, cui_id_from(context, "sidebar-footer"), theme,
        (cui_sidebar_section_options){.collapsed = collapsed, .content = sidebar_footer_inner});
}
static void sidebar_menu_content(cui_context *context, const cui_theme *theme, bool collapsed,
                                 void *user) {
    int group = *(int *)user;
    static const char *labels[12] = {"Overview", "Files",   "Activity", "Reviews",
                                     "Alerts",   "Search",  "Projects", "Deployments",
                                     "Usage",    "Billing", "Team",     "Help"};
    static const char *icons[6] = {"house", "folder", "file-text", "settings", "bell", "search"};
    cui_sidebar_menu_item items[6];
    for (int index = 0; index < 6; index++) {
        int position = group * 6 + index;
        sidebar_item_values[position] = position;
        items[index] = (cui_sidebar_menu_item){
            .id = cui_id_from(context, labels[position]),
            .label = cui_string(labels[position]),
            .icon = cui_string(icons[index]),
            .active = sidebar_selected == position,
            .state = &sidebar_items[position],
            .on_activate = sidebar_activate,
            .user = &sidebar_item_values[position],
            .tooltip_options = {.viewport = {0, 0, 900, 500}, .dt = host_dt, .open_delay = .5f}};
    }
    cui_sidebar_menu(context, cui_id_from(context, group ? "sidebar-menu-2" : "sidebar-menu-1"),
                     theme, items, 6, collapsed);
}
static void sidebar_group_content(cui_context *context, const cui_theme *theme, bool collapsed,
                                  void *user) {
    int group = *(int *)user;
    cui_sidebar_group(context, cui_id_from(context, group ? "sidebar-group-2" : "sidebar-group-1"),
                      theme, cui_string(group ? "Projects" : "Workspace"), sidebar_menu_content,
                      user, collapsed);
}
static int title_decoration, title_override, title_fullscreen;
static bool title_maximized;
static unsigned title_actions[7], title_closed;
static cui_title_bar_state title_state;
static cui_window_info title_query(void *user) {
    (void)user;
    return (cui_window_info){
        .decoration = (cui_window_decoration)title_decoration,
        .actions = title_decoration != CUI_WINDOW_BROWSER && !title_fullscreen ? 127 : 0,
        .maximized = title_maximized,
        .fullscreen = title_fullscreen != 0};
}
static bool title_perform(void *user, cui_window_action action, float x, float y) {
    (void)user;
    (void)x;
    (void)y;
    if ((unsigned)action >= 7)
        return false;
    title_actions[action]++;
    if (action == CUI_WINDOW_MAXIMIZE)
        title_maximized = true;
    if (action == CUI_WINDOW_RESTORE)
        title_maximized = false;
    redraw = true;
    return true;
}
static void title_close(void *user) {
    (void)user;
    title_closed++;
    redraw = true;
}
static cui_window_host title_host = {.query = title_query, .perform = title_perform};
static void toolbar_activate(void *user) {
    (*(unsigned *)user)++;
}
static void toolbar_query_render(cui_context *context, const cui_theme *theme,
                                 const cui_toolbar_presentation *presentation, void *user) {
    (void)user;
    cui_input_result result =
        cui_input_ex(context, cui_id_from(context, "toolbar-query"), theme, cui_string("Find"),
                     toolbar_query, &toolbar_query_state,
                     (cui_input_options){.width = 112,
                                         .design_size = presentation->metrics.design_size,
                                         .placeholder = cui_string("Find"),
                                         .disabled = toolbar_disabled});
    if (result.changed)
        toolbar_query_changes++;
}
static void title_simulated_controls(cui_context *context, const cui_theme *theme, bool disabled) {
    static const char *const names[] = {"Simulated close", "Simulated minimize",
                                        "Simulated maximize"};
    const cui_color colors[] = {{255, 95, 87, 255}, {254, 188, 46, 255}, {40, 200, 64, 255}};
    const cui_window_action actions[] = {CUI_WINDOW_CLOSE, CUI_WINDOW_MINIMIZE,
                                         title_maximized ? CUI_WINDOW_RESTORE
                                                         : CUI_WINDOW_MAXIMIZE};
    cui_begin_absolute_box(context, cui_id_from(context, "title-simulated-controls"),
                           (cui_box){.row = true, .align_y = CUI_ALIGN_CENTER, .gap = 2},
                           (cui_rect){8, 0, 68, 34});
    for (unsigned i = 0; i < 3; i++) {
        cui_id id = cui_id_from(context, names[i]);
        const char *name = i == 2 && title_maximized ? "Simulated restore" : names[i];
        cui_interaction input =
            cui_interact(context, id, CUI_ROLE_BUTTON, cui_string(name), disabled, false, 0);
        cui_begin_box(context, id,
                      (cui_box){.width = 20,
                                .height = 24,
                                .align_x = CUI_ALIGN_CENTER,
                                .align_y = CUI_ALIGN_CENTER});
        cui_begin_box(context, 0,
                      (cui_box){.width = 12,
                                .height = 12,
                                .radius = 6,
                                .background = disabled ? theme->muted : colors[i],
                                .border = input.focused ? theme->text : colors[i],
                                .border_width = input.focused ? 2 : 0});
        cui_end_box(context);
        cui_end_box(context);
        if (input.activated) {
            if (!i && title_override)
                title_close(NULL);
            else
                title_perform(NULL, actions[i], 0, 0);
        }
    }
    cui_end_box(context);
}
static void toolbar_group_render(cui_context *context, const cui_theme *theme,
                                 const cui_toolbar_presentation *presentation, void *user) {
    cui_toolbar_command *commands = user;
    cui_toolbar_item items[2] = {{.id = cui_id_from(context, "toolbar-bold-wrap"),
                                  .sized = true,
                                  .render = cui_toolbar_command_render,
                                  .user = &commands[0]},
                                 {.id = cui_id_from(context, "toolbar-italic-wrap"),
                                  .sized = true,
                                  .render = cui_toolbar_command_render,
                                  .user = &commands[1]}};
    cui_toolbar_group(context, cui_id_from(context, "toolbar-format-group"), theme, items, 2,
                      (cui_toolbar_options){.size = presentation->metrics.size,
                                            .design_size = presentation->metrics.design_size,
                                            .label = cui_string("Formatting")});
}
static void resizable_content(cui_context *context, void *user) {
    static const char *files[] = {"main.c", "layout.c", "theme.c"};
    static const char *code[][3] = {
        {"if (cui_button(ui, save_id, &theme,", "    cui_string(\"Save\"), false))",
         "    save_document(application);"},
        {"cui_begin_box(ui, panel_id,", "    (cui_box){.row = true, .gap = 8});",
         "cui_end_box(ui);"},
        {"cui_theme theme = cui_theme_dark();", "theme.radius = 8;", "theme.gap = 12;"}};
    cui_begin_box(context, 0,
                  (cui_box){.width = -1, .height = -1, .padding = 10, .gap = 8, .clip = true});
    bool explorer = (uintptr_t)user == 0;
    cui_text_ex(context, cui_string(explorer ? "Explorer" : files[resizable_document]), 14,
                current_theme.text, 18, (cui_text_style){.weight = CUI_TEXT_WEIGHT_SEMIBOLD});
    if (explorer) {
        for (unsigned i = 0; i < 3; i++)
            if (cui_button_ex(context, cui_id_from(context, files[i]), &current_theme,
                              cui_string(files[i]),
                              (cui_button_options){.width = -1,
                                                   .size = CUI_CONTROL_XSMALL,
                                                   .variant = CUI_BUTTON_GHOST,
                                                   .selected = resizable_document == i,
                                                   .icon = cui_string("file-text")})) {
                resizable_document = i;
                cui_invalidate(context);
            }
    } else {
        cui_begin_box(context, 0,
                      (cui_box){.width = -1,
                                .height = -1,
                                .padding = 8,
                                .gap = 2,
                                .background = current_theme.surface,
                                .radius = 4,
                                .clip = true});
        for (unsigned i = 0; i < 3; i++)
            cui_text_line(context, cui_string(code[resizable_document][i]), 12, current_theme.text,
                          16, (cui_text_style){.monospace = true});
        cui_end_box(context);
    }
    cui_end_box(context);
}
static void gallery_carousel_changed(size_t index, void *user) {
    (void)index;
    (void)user;
    carousel_changes++;
}
static void gallery_carousel_item(cui_context *context, const cui_theme *theme, size_t index,
                                  void *user) {
    (void)user;
    static const char *titles[] = {"Build", "Preview", "Test", "Ship"};
    cui_begin_box(context, 0,
                  (cui_box){.width = -1,
                            .height = -1,
                            .align_x = CUI_ALIGN_CENTER,
                            .align_y = CUI_ALIGN_CENTER,
                            .radius = theme->radius,
                            .background = theme->secondary});
    cui_label(context, theme, cui_string(titles[index]));
    cui_end_box(context);
}
static void gallery_carousel_page_content(cui_context *context, const cui_theme *theme,
                                          size_t index, void *user) {
    (void)user;
    static const char *numbers[] = {"1", "2", "3", "4"};
    cui_text(context, cui_string(numbers[index]), theme->font_size * .8f, theme->text);
}
static void gallery_carousel_controls(cui_context *context, const cui_theme *theme,
                                      cui_carousel_state *state, cui_id root, void *user) {
    (void)user;
    cui_carousel_control(context, cui_id_from(context, "carousel-prev"), theme, state, root, false,
                         (cui_carousel_control_options){0});
    cui_carousel_control(context, cui_id_from(context, "carousel-next"), theme, state, root, true,
                         (cui_carousel_control_options){0});
}
static void gallery_carousel_pagination(cui_context *context, const cui_theme *theme,
                                        cui_carousel_state *state, cui_id root, void *user) {
    (void)user;
    if (!cui_carousel_pagination_begin(context, cui_id_from(context, "carousel-pages"),
                                       cui_string("Slides"), NULL))
        return;
    static const char *names[] = {"Go to slide 1", "Go to slide 2", "Go to slide 3",
                                  "Go to slide 4"};
    for (size_t i = 0; i < 4; i++)
        cui_carousel_pagination_item(
            context, cui_id_from(context, names[i]), theme, state, root, i,
            (cui_carousel_control_options){.label = cui_string(names[i]),
                                           .content = gallery_carousel_page_content});
    cui_carousel_pagination_end(context);
}
static cui_check_state check_state;
static cui_checkbox_state checkbox_motion;
static cui_scroll_state checkbox_scroll;

static cui_design_size design_size;
static void gallery_checkbox_detail(cui_context *ui, void *user) {
    const cui_theme *theme = user;
    cui_design_metrics metrics;
    float font = theme->font_size * .875f;
    if (design_size && cui_design_measure(design_size, theme->font_size / 16, &metrics))
        font = metrics.font_size * .875f;
    cui_text(ui, cui_string("Email alerts for releases and replies."), font, theme->muted);
}
static int design_radius = -1;
static float design_spacing = -1;
static int control_size, choice_required, choice_invalid, choice_skip_tab;
static int gallery_xsmall_second_size(void) {
    return control_size == 1 ? 2 : control_size == 2 ? 3 : control_size == 3 ? 1 : 0;
}
static int switch_label_left, switch_custom_color, toggle_variant, toggle_segmented,
    toggle_selection_mode;
static cui_switch_state gallery_switch_state;
static bool toggle_group_values[3] = {true, false, false}, toggle_group_requested[3];
static unsigned toggle_group_changes;
static void gallery_toggle_group_change(void *user, const bool *requested, size_t count) {
    (void)user;
    if (count != 3)
        return;
    toggle_group_changes++;
    if (toggle_selection_mode) {
        memcpy(toggle_group_values, requested, count * sizeof(*requested));
    } else {
        for (size_t i = 0; i < count; i++)
            if (requested[i] && !toggle_group_values[i]) {
                for (size_t j = 0; j < count; j++)
                    toggle_group_values[j] = j == i;
                return;
            }
        if (requested[0] || requested[1] || requested[2])
            memcpy(toggle_group_values, requested, count * sizeof(*requested));
    }
}
static bool renderer_fixture_enabled;
static const unsigned char renderer_fixture_pixels[80] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0,
    255, 255, 255, 255, 200, 100, 0,   255, 200, 100, 0,   255, 255, 255, 255, 255, 0, 0, 0, 0,
    255, 255, 255, 255, 200, 100, 0,   255, 200, 100, 0,   255, 255, 255, 255, 255, 0, 0, 0, 0,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0};
/* Browser pixel regression fixture; absent from the ordinary component preview. */
void gallery_renderer_fixture(int enabled) {
    renderer_fixture_enabled = enabled != 0;
}
static void gallery_paint_renderer_fixture(cui_context *ui) {
    cui_begin_box(ui, cui_id_from(ui, "renderer-fixture-clip"),
                  (cui_box){.width = 32, .height = 32, .clip = true});
    cui_paint_bitmap(ui, cui_id_from(ui, "renderer-fixture-image"),
                     (cui_bitmap_paint){.bitmap = {renderer_fixture_pixels, 4, 4, 20, 1},
                                        .source = {1, 1, 2, 2},
                                        .opacity = .5f,
                                        .grayscale = true,
                                        .custom_tint = true,
                                        .tint = {255, 128, 0, 128}},
                     (cui_box){.width = 60, .height = 60, .radius = 12});
    cui_end_box(ui);
}
#include "gallery_image.inc"
static const cui_image_frame image_frames[] = {{gallery_lake_pixels, 256, 144, 1024, 1, 0}};
static cui_image_state image_state, attachment_images[6];
static cui_attachment_state attachment_states[6];
static cui_attachment_group_state attachment_group_state;
static int image_mode, image_fit, image_grayscale, image_half_opacity;
static int attachment_status = CUI_ATTACHMENT_COMPLETE, attachment_vertical;
static unsigned attachment_removed_mask, attachment_clicks, attachment_removes, attachment_retries,
    attachment_actions;
static cui_attachment_group_result attachment_group_result;
static cui_image_result image_result;
static cui_image_load gallery_image_loader(void *user, cui_image_source source) {
    (void)user;
    return (cui_image_load){.status = source.identity == 2   ? CUI_IMAGE_PENDING
                                      : source.identity == 3 ? CUI_IMAGE_FAILED
                                                             : CUI_IMAGE_READY,
                            .frames = image_frames,
                            .count = 1};
}
static void gallery_image_placeholder(cui_context *context, cui_id id, void *user, float width,
                                      float height) {
    (void)id;
    (void)user;
    (void)width;
    (void)height;
    cui_label(context, &current_theme,
              cui_string(image_mode == 1 ? "Image loading" : "Image failed"));
}
static void gallery_attachment_click(void *user, cui_id id) {
    (void)user;
    (void)id;
    attachment_clicks++;
}
static void gallery_attachment_remove(void *user, cui_id id) {
    (void)id;
    attachment_removed_mask |= 1u << (uintptr_t)user;
    attachment_removes++;
    redraw = true;
}
static void gallery_attachment_retry(void *user, cui_id id) {
    (void)user;
    (void)id;
    attachment_status = CUI_ATTACHMENT_UPLOADING;
    attachment_retries++;
    redraw = true;
}
static void gallery_attachment_actions(cui_context *context, cui_id id, void *user, float width,
                                       float height) {
    (void)user;
    (void)width;
    (void)height;
    if (cui_button_ex(context, id, &current_theme, cui_string("Open"),
                      (cui_button_options){
                          .size = CUI_CONTROL_XSMALL, .width = width, .design_size = design_size}))
        attachment_actions++;
}
static void gallery_attachment_cards(cui_context *context, cui_id id, void *user, float width,
                                     float height) {
    (void)id;
    (void)user;
    (void)width;
    (void)height;
    for (unsigned index = 0; index < 6; index++) {
        if (attachment_removed_mask & (1u << index))
            continue;
        char name[32];
        snprintf(name, sizeof(name), "Attachment %u", index + 1);
        cui_attachment_child children[2] = {
            {.kind = CUI_ATTACHMENT_TITLE, .text = cui_string(name)},
            {.kind = CUI_ATTACHMENT_DESCRIPTION, .text = cui_string("Workspace file")}};
        cui_attachment_options options = cui_attachment_options_default();
        options.name = cui_string(name);
        options.model.status =
            index ? CUI_ATTACHMENT_COMPLETE : (cui_attachment_status)attachment_status;
        options.model.size = (cui_media_size)gallery_xsmall_second_size();
        options.model.design_size = design_size;
        cui_design_metrics attachment_design;
        float attachment_font = current_theme.font_size;
        if (design_size &&
            cui_design_measure(design_size, current_theme.font_size / 16, &attachment_design))
            attachment_font = attachment_design.font_size;
        options.model.width = fmaxf(260, 16.25f * attachment_font);
        options.model.vertical = attachment_vertical != 0;
        options.model.identified = true;
        options.model.progress_known = true;
        options.model.progress = 45;
        options.children = children;
        options.child_count = 2;
        options.image_state = &attachment_images[index];
        options.image_source = (cui_image_source){
            .kind = CUI_IMAGE_ASSET, .name = cui_string(name), .identity = 10 + index};
        options.image_loader = gallery_image_loader;
        options.actions = gallery_attachment_actions;
        options.actions_width =
            fmaxf(64, cui_measure_text(context, cui_string("Open"), attachment_font).width +
                          (design_size ? 2 * attachment_design.padding_x : 16));
        options.on_click = gallery_attachment_click;
        options.on_remove = gallery_attachment_remove;
        options.on_retry = gallery_attachment_retry;
        options.action_user = (void *)(uintptr_t)index;
        options.remove_label = cui_string("Remove attachment");
        options.retry_label = cui_string("Retry attachment");
        options.now_seconds = host_seconds;
        options.delta_seconds = host_dt;
        options.reduced_motion = reduced_motion;
        options.active = true;
        cui_attachment(context, cui_id_from(context, name), &current_theme,
                       &attachment_states[index], options);
    }
}
static int clicks;
static cui_button_state gallery_button_label;
static int button_collection;
static bool button_collection_selected[3] = {true, false, false};
static unsigned button_collection_requests;
static cui_button_group_style_workspace button_collection_workspace;
static void gallery_button_collection_change(void *user, const cui_button_group_request *request) {
    (void)user;
    memset(button_collection_selected, 0, sizeof button_collection_selected);
    for (size_t i = 0; i < request->selected_count; ++i)
        if (request->selected_indices[i] < 3)
            button_collection_selected[request->selected_indices[i]] = true;
    ++button_collection_requests;
}
void gallery_button_collection(int mode) {
    button_collection = mode >= 0 && mode <= 3 ? mode : 0;
}
unsigned gallery_button_collection_stat(unsigned field) {
    return field < 3 ? button_collection_selected[field] : button_collection_requests;
}

static void gallery_button_action(void *user) {
    (void)user;
    clicks++;
}
static void gallery_button_badge(cui_context *context, void *user) {
    const cui_theme *theme = user;
    cui_text(context, cui_string("NEW"), theme->font_size * .75f, theme->text);
}
static cui_id radio_selected;
static cui_radio_group_state radio_group_state;
static unsigned radio_requests;
static void gallery_radio_change(void *user, cui_id selected) {
    (void)user;
    radio_requests++;
    radio_selected = selected;
}
unsigned gallery_radio_stat(unsigned field) {
    return field == 0 ? radio_requests : 0;
}
static cui_bubble_state gallery_bubble_states[4];
typedef struct {
    const char *key;
    bool liked, disabled;
} gallery_bubble_reaction_data;
static gallery_bubble_reaction_data gallery_bubble_reactions[4];
static void gallery_bubble_reaction(cui_context *context, const cui_theme *theme,
                                    cui_color foreground, void *user) {
    gallery_bubble_reaction_data *data = user;
    cui_push_id(context, data->key);
    if (cui_button(context, cui_id_from(context, "reaction"), theme,
                   cui_string(data->liked ? "Liked · 3" : "Liked · 2"), data->disabled)) {
        data->liked = !data->liked;
        cui_invalidate(context);
    }
    cui_pop_id(context);
    (void)foreground;
}
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

static unsigned char avatar_mask_pixels[3][96 * 96 * 4];
static cui_avatar_image avatar_images[3];
static bool avatar_images_ready;
static int avatar_selected;
static int avatar_source = 1, avatar_palette = 1, avatar_hue = -1, avatar_placeholder;
static float avatar_rem = 16;
static cui_icon_state avatar_custom_placeholder;
static bool avatar_custom_placeholder_ready;
static bool message_demo_disabled;
static const char *avatar_names[3] = {"Alex Morgan", "Jamie Chen", "Robin Hayes"};
static cui_bitmap gallery_avatar_source(unsigned index) {
    const unsigned char *pixels[3] = {gallery_avatar_alex, gallery_avatar_jamie,
                                      gallery_avatar_robin};
    if (!avatar_images_ready) {
        for (unsigned i = 0; i < 3; i++)
            cui_avatar_image_init(&avatar_images[i], avatar_mask_pixels[i],
                                  sizeof avatar_mask_pixels[i], 96);
        avatar_images_ready = true;
    }
    return (cui_bitmap){pixels[index % 3], 96, 96, 96 * 4, 1};
}
static void gallery_message_avatar(cui_context *context, cui_id id, void *user,
                                   cui_message_style style) {
    unsigned index = (unsigned)(uintptr_t)user % 3;
    cui_avatar_ex(context, id, &current_theme, cui_string(avatar_names[index]),
                  (cui_avatar_options){.size = style.width,
                                       .ring_width = 2,
                                       .image = gallery_avatar_source(index),
                                       .image_state = &avatar_images[index]});
}
static int message_alignment, message_variant = CUI_BUBBLE_SECONDARY, message_inset;
static bool message_avatar = true, message_acknowledged, message_scrollbar = true;
static unsigned message_actions, conversation_serial = 80, conversation_count = 80;
static unsigned conversation_ids[256], conversation_rendered;
static char conversation_names[256][32], conversation_texts[256][64];
static int conversation_mutation;
static cui_message_scroller_state *conversation;
static cui_message_scroller_result conversation_result;
static void message_header(cui_context *context, cui_id id, void *user, cui_message_style style) {
    (void)id;
    unsigned index = (unsigned)(uintptr_t)user % 3;
    cui_text(context, cui_string(avatar_names[index]), style.font_size, style.color);
    cui_text(context, cui_string(index ? "09:32" : "09:30"), style.font_size, style.color);
}
static void message_footer(cui_context *context, cui_id id, void *user, cui_message_style style) {
    (void)user;
    cui_begin_box(context, id, (cui_box){.width = style.width, .gap = 4});
    cui_text(context, cui_string("Delivered · 09:30"), style.font_size, style.color);
    cui_begin_box(context, id ^ 8, (cui_box){.row = true, .gap = 8, .align_y = CUI_ALIGN_CENTER});
    cui_button_options button = {.size = CUI_CONTROL_SMALL, .variant = CUI_BUTTON_SECONDARY};
    button.disabled = message_demo_disabled;
    if (cui_button_ex(context, id ^ 2, &current_theme, cui_string("Like message"), button))
        message_actions++;
    if (message_actions)
        cui_text(context, cui_string("Liked"), style.font_size, style.color);
    cui_end_box(context);
    cui_checkbox(context, id ^ 1, &current_theme, cui_string("Acknowledge message"),
                 &message_acknowledged, message_demo_disabled);
    cui_end_box(context);
}
static void conversation_labels(void) {
    for (unsigned i = 0; i < conversation_count; i++) {
        snprintf(conversation_names[i], sizeof(conversation_names[i]), "Message %u",
                 conversation_ids[i]);
        static const char *messages[] = {
            "Can you review the release notes?", "Looks good. Ready to ship.",
            "Thanks! I'll post the update.", "The screenshots look great.",
            "I'll check the mobile layout."};
        snprintf(conversation_texts[i], sizeof conversation_texts[i], "%s",
                 messages[conversation_ids[i] % 5]);
    }
}
static void conversation_row(cui_context *context, cui_id id, size_t index, void *user,
                             float width) {
    (void)user;
    conversation_rendered++;
    unsigned person = conversation_ids[index] % 3;
    cui_message_body body = {
        .slot = {.present = true, .text = cui_string(conversation_texts[index])},
        .typed_bubble = true,
        .bubble = conversation_ids[index] % 2 ? CUI_BUBBLE_FILLED : CUI_BUBBLE_SECONDARY};
    cui_message_options options = cui_message_options_default();
    options.width = width;
    options.max_bubble_width = fminf(280, width - 40);
    options.role = CUI_ROLE_LISTITEM;
    options.name = cui_string(conversation_names[index]);
    options.alignment = conversation_ids[index] % 2 ? CUI_MESSAGE_END : CUI_MESSAGE_START;
    options.avatar = (cui_message_slot){.present = message_avatar,
                                        .width = 32,
                                        .height = 32,
                                        .render = message_avatar ? gallery_message_avatar : NULL,
                                        .user = (void *)(uintptr_t)person};
    options.header = (cui_message_slot){.present = true, .text = cui_string(avatar_names[person])};
    options.body = &body;
    options.body_count = 1;
    options.footer = (cui_message_slot){.present = true, .text = cui_string("09:32 · Read")};
    cui_message(context, id, &current_theme, options);
}
static cui_hover_state tooltip_state, card_state;
static cui_virtual_list *virtual_list;
static cui_virtual_list *virtual_horizontal;
static cui_virtual_widget_state virtual_state;
static int virtual_mode;
static cui_virtual_widget_result virtual_result;
static bool virtual_disabled;
void gallery_virtual_mode(int mode) {
    if (virtual_mode != (mode != 0)) {
        virtual_mode = mode != 0;
        virtual_state = (cui_virtual_widget_state){0};
    }
}
unsigned gallery_virtual_window_stat(int field) {
    return field == 0   ? (unsigned)virtual_result.view.begin
           : field == 1 ? (unsigned)virtual_result.view.end
           : field == 2
               ? (unsigned)cui_virtual_count(virtual_mode ? virtual_horizontal : virtual_list)
               : (unsigned)virtual_result.error;
}
static void gallery_virtual_row(cui_context *context, const cui_theme *theme, void *user,
                                size_t index, uint64_t key, cui_id item, cui_size size) {
    (void)user;
    char label[48];
    snprintf(label, sizeof(label), "Task %06llu", (unsigned long long)key);
    cui_begin_box(context, item ^ UINT64_C(0x71d59f493a6ea132),
                  (cui_box){.width = size.width,
                            .height = size.height,
                            .align_y = CUI_ALIGN_CENTER,
                            .background = index % 2 ? theme->muted_background : theme->surface});
    cui_interact(context, item, CUI_ROLE_LISTITEM, cui_string(label), virtual_disabled, false, 0);
    cui_set_semantic_details(
        context, item,
        (cui_semantic_details){.parent = cui_id_from(context, "virtual-viewport"),
                               .pos_in_set = (unsigned)index + 1,
                               .set_size = (unsigned)cui_virtual_count(
                                   virtual_mode ? virtual_horizontal : virtual_list)});
    cui_text(context, cui_string(label), 14, theme->text);
    cui_end_box(context);
}

static cui_dock *dock;
static cui_dock_view *dock_view;
static cui_dock *dock_edges[3];
static cui_dock_view *dock_edge_views[3];
static cui_dock_workspace *dock_workspace;
int gallery_dock_panel_region(unsigned key) {
    if (cui_dock_panel_find(dock, key, NULL, NULL))
        return CUI_DOCK_CENTER;
    for (int i = 0; i < 3; i++)
        if (cui_dock_panel_find(dock_edges[i], key, NULL, NULL))
            return i + 1;
    return -1;
}
double gallery_dock_stat(int field) {
    cui_dock_workspace_snapshot snapshot;
    if (!cui_dock_workspace_save(dock_workspace, &snapshot))
        return -1;
    if (field >= 0 && field < 3)
        return snapshot.open[field];
    if (field >= 3 && field < 6)
        return snapshot.size[field - 3];
    if (field == 6)
        return cui_dock_view_zoomed(dock_view, NULL);
    cui_dock_node_info root;
    if ((field == 7 || field == 8) && cui_dock_node_get(dock, cui_dock_root(dock), &root))
        return field == 7 ? (double)root.child_count : root.nary;
    return -1;
}
static bool dock_disabled;
static unsigned dock_refreshes[12];
static cui_date date = {2026, 10, 12};
static cui_calendar_state calendar;
static cui_date calendar_today;
static bool calendar_month_menu, calendar_neighbors, calendar_mark_today;
static int date_locale = -1;
static cui_date_selection date_selection = {{2026, 10, 12}, {0}};
static cui_date_picker_state date_picker;
static cui_time time_value = {9, 30, 0};
static cui_time_state time_state;
static bool multiple_mode, multi_selected[4];
static cui_color chosen_color = {35, 102, 80, 255};
static cui_color_state color_state;
static cui_color_entry_state color_entry;
static bool color_entry_restore;
static size_t color_entry_commits;
static cui_plot_viewport plot_viewport;
static unsigned char plot_pixels[640 * 180 * 4];
static cui_plot_numeric_tick plot_x_ticks[16], plot_y_ticks[16];
static cui_plot_axis_text plot_x_labels[16], plot_y_labels[16];
static uint64_t plot_revision;
static cui_plot_scale plot_cached_scale, plot_cached_y;
static cui_color plot_cached_primary, plot_cached_surface;
static unsigned plot_cached_width;
static int plot_mode, plot_cached_mode = -1;
void gallery_plot_mode(int mode) {
    plot_mode = mode >= 0 && mode <= 6 ? mode : 0;
}
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
static bool stepper_details_confirmed, stepper_validation_failed;
static unsigned stepper_request_count;
static void gallery_stepper_request(void *user, const cui_stepper_navigation_request *request) {
    (void)user;
    stepper_request_count++;
    stepper_validation_failed = request->requested > 0 && !stepper_details_confirmed;
    if (!stepper_validation_failed)
        current_step = request->requested;
    /* The request callback changes owner data; borrowed descriptors stay stable. */
    cui_invalidate(ui);
}
static float rating = 3;
static cui_id dock_serial = 3;
static uint64_t dock_active_panel = 1;
static void gallery_dock_reset(void) {
    cui_dock *fresh =
        cui_dock_create_nary((cui_dock_config){.node_capacity = 15, .panel_capacity = 16});
    if (!fresh)
        return;
    if (cui_dock_insert(fresh, cui_dock_root(fresh), 1, 0) != CUI_DOCK_OK ||
        cui_dock_insert(fresh, cui_dock_root(fresh), 2, 1) != CUI_DOCK_OK) {
        cui_dock_destroy(fresh);
        return;
    }
    cui_dock *fresh_edges[3] = {0};
    for (int i = 0; i < 3; i++) {
        fresh_edges[i] =
            cui_dock_create_nary((cui_dock_config){.node_capacity = 15, .panel_capacity = 16});
        if (!fresh_edges[i] || cui_dock_insert(fresh_edges[i], cui_dock_root(fresh_edges[i]),
                                               (uint64_t)i + 10, 0) != CUI_DOCK_OK) {
            for (int j = 0; j < 3; j++)
                cui_dock_destroy(fresh_edges[j]);
            cui_dock_destroy(fresh);
            return;
        }
    }
    cui_dock_destroy(dock);
    dock = fresh;
    for (int i = 0; i < 3; i++) {
        cui_dock_destroy(dock_edges[i]);
        dock_edges[i] = fresh_edges[i];
    }
    dock_serial = 3;
    for (int i = 0; i < 3; i++) {
        cui_dock_workspace_set_edge(dock_workspace, (cui_dock_region)(i + 1), false,
                                    i == 2 ? 100 : 160);
        cui_dock_view_reset(dock_edge_views[i]);
    }
    dock_active_panel = 1;
    cui_dock_view_reset(dock_view);
    redraw = true;
}
static uint64_t gallery_dock_first_active(cui_dock_node node) {
    cui_dock_node_info info;
    if (!cui_dock_node_get(dock, node, &info))
        return 0;
    if (info.kind == CUI_DOCK_TABS)
        return info.has_active ? info.active_panel : 0;
    for (size_t i = 0; i < info.child_count; i++) {
        cui_dock_node child;
        if (cui_dock_child_at(dock, node, i, &child, NULL)) {
            uint64_t active = gallery_dock_first_active(child);
            if (active)
                return active;
        }
    }
    return 0;
}
static void gallery_dock_event(void *user, cui_dock_event event, uint64_t key, cui_dock_node node) {
    (void)user;
    (void)node;
    if ((event == CUI_DOCK_SELECTED || event == CUI_DOCK_MOVED || event == CUI_DOCK_SPLIT) &&
        cui_dock_panel_find(dock, key, NULL, NULL))
        dock_active_panel = key;
    if (event == CUI_DOCK_CLOSED && !cui_dock_panel_find(dock, dock_active_panel, NULL, NULL))
        dock_active_panel = gallery_dock_first_active(cui_dock_root(dock));
}
static void gallery_dock_transfer(void *user, const cui_dock_transfer_request *request) {
    (void)user;
    (void)request;
    dock_active_panel = gallery_dock_first_active(cui_dock_root(dock));
}
/* Deliberately small demo lexer; applications can supply their language service. */
static size_t gallery_editor_token(void *user, cui_str language, cui_str text, size_t start,
                                   size_t end, cui_color *color) {
    (void)user;
    (void)language;
    *color = current_theme.text;
    if (start >= end)
        return end;
    char c = text.data[start];
    if (c == '/' && start + 1 < end && text.data[start + 1] == '/') {
        *color = current_theme.muted;
        return end;
    }
    if (c == '"') {
        size_t i = start + 1;
        while (i < end) {
            if (text.data[i] == '\\' && i + 1 < end)
                i += 2;
            else if (text.data[i++] == '"')
                break;
        }
        *color = current_theme.primary;
        return i;
    }
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
        size_t i = start + 1;
        while (i < end && ((text.data[i] >= 'a' && text.data[i] <= 'z') ||
                           (text.data[i] >= 'A' && text.data[i] <= 'Z') ||
                           (text.data[i] >= '0' && text.data[i] <= '9') || text.data[i] == '_'))
            i++;
        static const char *words[] = {"void", "const", "if", "false", "true", "return"};
        for (size_t w = 0; w < sizeof(words) / sizeof(words[0]); w++)
            if (strlen(words[w]) == i - start && !memcmp(text.data + start, words[w], i - start))
                *color = current_theme.primary;
        return i;
    }
    /* Advance one UTF-8 scalar so ordinary Unicode remains intact. */
    size_t i = start + 1;
    while (i < end && ((unsigned char)text.data[i] & 0xc0) == 0x80)
        i++;
    return i;
}
static size_t badge_count = 7;
static bool badge_disabled;
static void gallery_badge_content(cui_context *context, void *user) {
    const char *name = user;
    if (cui_button_ex(context, cui_id_from(context, name) ^ UINT64_C(0x7f3368229a10ce4b),
                      &current_theme, cui_string(name),
                      (cui_button_options){.size = (cui_control_size)control_size,
                                           .width = 100,
                                           .disabled = badge_disabled})) {
        badge_count++;
        cui_invalidate(context);
    }
}
static void gallery_dock_toolbar(cui_context *context, uint64_t key, void *user) {
    (void)user;
    if (cui_button_ex(context, cui_id_from(context, "refresh-panel") ^ key, &current_theme,
                      cui_string("Refresh panel"),
                      (cui_button_options){.size = CUI_CONTROL_SMALL,
                                           .variant = CUI_BUTTON_GHOST,
                                           .icon = cui_string("refresh-cw"),
                                           .icon_only = true,
                                           .disabled = dock_disabled}) &&
        key >= 1 && key <= 12)
        dock_refreshes[key - 1]++;
}
static void gallery_dock_content(cui_context *context, uint64_t key, void *user) {
    (void)user;
    char name[64], content[64];
    snprintf(name, sizeof(name), "Workspace panel %llu", (unsigned long long)key);
    snprintf(content, sizeof(content), "Active panel: %llu", (unsigned long long)key);
    cui_id id = UINT64_C(0x79a6b12c693048fe) ^ key;
    cui_begin_box(context, id, (cui_box){.width = -1, .height = -1, .padding = 12, .gap = 8});
    cui_interact(context, id, CUI_ROLE_REGION, cui_string(name), dock_disabled, false, 0);
    cui_label(context, &current_theme, cui_string(content));
    snprintf(content, sizeof(content), "Refreshes: %u",
             key >= 1 && key <= 12 ? dock_refreshes[key - 1] : 0);
    cui_label(context, &current_theme, cui_string(content));
    cui_label(context, &current_theme, cui_string("Content callback"));
    cui_end_box(context);
}

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
static char input_format_workspace[1024];
static int input_mode;
#ifdef CUI_GALLERY_REGEX
static cui_regex input_regex;
static cui_regex_session input_regex_session;
#endif
static bool input_regex_ready;
int gallery_regex_available(void) {
    return input_regex_ready;
}
static void gallery_input_regex_init(void) {
#ifdef CUI_GALLERY_REGEX
    if (cui_regex_compile(cui_string("^[\\p{L} ]*$"),
                          (cui_regex_options){.max_pattern_bytes = 64, .max_text_bytes = 1024},
                          &input_regex) == CUI_REGEX_OK &&
        cui_regex_session_create(&input_regex, 1024 * 1024, &input_regex_session) == CUI_REGEX_OK) {
        input_regex_ready = true;
        return;
    }
    (void)cui_regex_session_destroy(&input_regex_session);
    (void)cui_regex_destroy(&input_regex);
#endif
}
static cui_text_match gallery_input_regex_matcher(void) {
#ifdef CUI_GALLERY_REGEX
    if (input_regex_ready)
        return cui_regex_matcher(&input_regex_session);
#endif
    return (cui_text_match){0};
}
static bool input_rejected;
static bool gallery_name_acceptance(void *user, cui_str value) {
    (void)user;
    for (size_t i = 0; i < value.length; i++)
        if (value.data[i] >= '0' && value.data[i] <= '9')
            return false;
    return true;
}
static int form_mode, form_submissions;
static int group_mode, group_actions;
static void gallery_group_action_content(cui_context *ui, const cui_theme *theme, void *user,
                                         bool disabled, bool readonly, float width) {
    (void)user;
    (void)readonly;
    (void)width;
    cui_text(ui, cui_string("Open"), theme->font_size, disabled ? theme->muted : theme->on_primary);
}
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
    cui_input_result result = cui_input_ex(
        context, control->id, theme, cui_string(control->name), control->edit, control->state,
        (cui_input_options){.width = width,
                            .design_size = design_size,
                            .density = current_theme.font_size / 16,
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
    cui_theme control_theme = *theme;
    if (design_size != CUI_DESIGN_INHERIT)
        control_theme.font_size = current_theme.font_size;
    if (cui_button_ex(context, cui_id_from(context, "form-save"), &control_theme,
                      cui_string("Save profile"),
                      (cui_button_options){.disabled = disabled, .design_size = design_size})) {
        if (form_name_buffer[0] && form_email_valid()) {
            form_submissions++;
            snprintf(form_status, sizeof(form_status), "Profile saved. Submissions: %d",
                     form_submissions);
        } else
            snprintf(form_status, sizeof(form_status), "Correct the highlighted fields.");
    }
}
static cui_scroll_state scroll_state;
static cui_scroll_state table_composition_scroll;
static cui_table_model *table;
static cui_table_state table_state;
static cui_table_selection table_selection[64];
static bool table_pinned, table_actions, table_selection_locked, table_source_sort = true;
static bool table_story_disabled;
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
    size_t record = (size_t)table_rows[row].key - 1;
    return cui_string(column == 101   ? table_names[record]
                      : column == 102 ? "Ready"
                                      : table_sizes[record]);
}
static int table_compare_rows(const void *left, const void *right) {
    const cui_table_row *a = left, *b = right;
    int order = (a->key > b->key) - (a->key < b->key);
    /* Name is zero-padded and Size is numeric; Status ties preserve original key order. */
    if (table_state.sort == CUI_TABLE_SORT_DESCENDING && table_state.sort_column != 102)
        order = -order;
    return order;
}
static bool gallery_apply_table_sort(void) {
    for (size_t c = 0; c < 3; c++)
        if (!cui_table_get_column(table, c, &table_columns[c]))
            return false;
    qsort(table_rows, 1000, sizeof(*table_rows), table_compare_rows);
    return cui_table_bind(table, table_rows, table_mode == 3 ? 0 : 1000, table_columns, 3) ==
           CUI_TABLE_OK;
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
static void table_action_cell(cui_context *context, const cui_theme *theme, void *user, size_t row,
                              cui_id column) {
    (void)user;
    if (column != 102) {
        bool selected = cui_table_is_selected(
            &table_state,
            (cui_table_options){.selection_mode = table_selection_mode,
                                .multiple_selection = multiple_mode,
                                .row_selection_disabled = table_selection_locked,
                                .column_selection_disabled = table_selection_locked,
                                .cell_selection_disabled = table_selection_locked},
            table_rows[row].key, column);
        cui_text(context, table_label(NULL, row, column), theme->font_size,
                 selected ? theme->on_primary : theme->text);
        return;
    }
    cui_id action =
        cui_table_cell_id(cui_id_from(context, "data-table-action"), table_rows[row].key, column);
    if (cui_button_ex(context, action, theme, cui_string("Inspect"),
                      (cui_button_options){.size = CUI_CONTROL_XSMALL,
                                           .width = -1,
                                           .disabled = table_story_disabled})) {
        table_inspections++;
        cui_invalidate(context);
    }
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
static cui_size measure_styled(void *user, cui_str text, float size, cui_text_style style) {
    (void)user;
    return (cui_size){host_measure_styled(text.data, text.length, size, style.weight, style.italic,
                                          style.monospace, style.family),
                      size * 1.25f};
}
static bool measure_font_metrics(void *user, float size, cui_text_style style,
                                 cui_font_metrics *out) {
    (void)user;
    float metrics[3];
    if (!host_font_metrics(size, style.weight, style.italic, style.monospace, style.family,
                           metrics))
        return false;
    *out = (cui_font_metrics){metrics[0], metrics[1], metrics[2]};
    return true;
}
int gallery_font_metrics(float size, int weight, int italic, int monospace, float *out) {
    if (!out)
        return 0;
    cui_font_metrics metrics;
    if (!cui_measure_font_metrics(ui, size,
                                  (cui_text_style){.weight = (cui_text_weight)weight,
                                                   .italic = italic != 0,
                                                   .monospace = monospace != 0},
                                  &metrics))
        return 0;
    out[0] = metrics.ascent;
    out[1] = metrics.descent;
    out[2] = metrics.line_height;
    return 1;
}
static void gallery_text_view_link(cui_str url, const cui_event *event, void *user) {
    (void)url;
    (void)event;
    (void)user;
    text_view_links++;
}
void gallery_text_view_options(int format, int scrollable, int fade) {
    if (format < 0 || format > 2)
        format = 0;
    if (format != text_view_format) {
        text_view_format = format;
        gallery_text_view_load();
    }
    text_view_scrollable = scrollable != 0;
    text_view_fade = fade != 0;
    redraw = true;
}
void gallery_text_view_motion(float duration, float stagger, int easing) {
    cui_text_view_motion motion = {
        .duration = duration, .stagger = stagger, .easing = (cui_text_view_fade_easing)easing};
    if (cui_text_view_set_motion(&text_view_state, motion))
        text_view_motion = motion;
}
float gallery_text_view_motion_stat(int field) {
    if (field == 0)
        return text_view_state.fade_duration;
    if (field == 1)
        return text_view_state.fade_stagger;
    if (field == 2)
        return (float)text_view_state.fade_easing;
    return cui_text_view_stream_opacity(
        &text_view_state, text_view_state.text_length ? text_view_state.text_length - 1 : 0);
}
int gallery_text_view_stats(int field) {
    return field == 0   ? (int)text_view_state.text_length
           : field == 1 ? (int)text_view_state.block_count
           : field == 2 ? (int)text_view_state.cursor
           : field == 3 ? (int)text_view_state.anchor
           : field == 4 ? text_view_links
           : field == 5 ? (int)text_view_error
           : field == 6 ? (int)text_view_state.scroll.y
                        : (int)text_view_state.highlight_count;
}
static bool gallery_text_view_load(void) {
    static const char *documents[] = {
        "# Project notes\n\n```c\n"
        "cui_text_view(ui, id, theme, &state, options);\n```\n\n"
        "A **bold** idea with *emphasis* and `inline code`. "
        "Read the [CUI guide](https://example.com/cui).\n\n"
        "- First task\n- Second task\n\n"
        "> A portable interface starts with clear content.\n\n"
        "| Stage | Owner |\n| --- | --- |\n| Design | Ada |\n| Test | Lin |\n",
        "<h1>Project notes</h1><p>A <strong>bold</strong> idea with <em>emphasis</em> "
        "and <code>inline code</code>. Read the <a href=\"https://example.com/cui\">CUI "
        "guide</a>.</p>"
        "<ul><li>First task</li><li>Second task</li></ul>",
        "Project notes\nA bold idea with emphasis and inline code.\nFirst task\nSecond task"};
    if (!cui_text_view_init(
            &text_view_state,
            (cui_text_view_workspace){.source = text_view_source,
                                      .rendered = text_view_rendered,
                                      .source_capacity = sizeof(text_view_source),
                                      .rendered_capacity = sizeof(text_view_rendered),
                                      .mapping = text_view_mapping,
                                      .mapping_capacity = 4096,
                                      .spans = text_view_spans,
                                      .span_capacity = 512,
                                      .blocks = text_view_blocks,
                                      .block_capacity = 256,
                                      .glyphs = text_view_glyphs,
                                      .glyph_capacity = 4096,
                                      .highlights = text_view_highlights,
                                      .highlight_capacity = 32},
            (cui_text_view_format)text_view_format))
        return false;
    snprintf(text_view_document, sizeof(text_view_document), "%s", documents[text_view_format]);
    text_view_error = cui_text_view_set_text(&text_view_state, cui_string(text_view_document));
    if (text_view_error != CUI_TEXT_VIEW_OK)
        return false;
    if (text_view_format == CUI_TEXT_VIEW_MARKDOWN) {
        cui_str rendered = cui_text_view_rendered_text(&text_view_state);
        const char *hit = strstr(rendered.data, "bold");
        if (hit) {
            cui_text_view_highlight highlight = {
                .range = {(size_t)(hit - rendered.data), (size_t)(hit - rendered.data) + 4},
                .background = {230, 190, 70, 90}};
            text_view_error = cui_text_view_set_highlights(&text_view_state, &highlight, 1);
        }
    }
    return text_view_error == CUI_TEXT_VIEW_OK;
}
static unsigned char effect_source[128 * 64 * 4], effect_pixels[128 * 64 * 4];
static float effect_scratch[128 * 64 * 4], effect_angle = 35, effect_opacity = 1;
static unsigned effect_blur;
static uint64_t effect_revision;
static bool effect_dirty = true;
void gallery_effects_options(float angle, float opacity, unsigned blur) {
    if (!isfinite(angle) || !isfinite(opacity) || opacity < 0 || opacity > 1 || blur > 16)
        return;
    if (effect_angle != angle || effect_opacity != opacity || effect_blur != blur) {
        effect_angle = angle;
        effect_opacity = opacity;
        effect_blur = blur;
        effect_dirty = true;
    }
}
float gallery_effect_pixel(unsigned x, unsigned y, unsigned channel) {
    return x < 128 && y < 64 && channel < 4 ? effect_pixels[(y * 128 + x) * 4 + channel] : -1;
}
static void gallery_effect_preview(const cui_theme *theme, float width) {
    if (effect_dirty) {
        const cui_gradient_stop stops[] = {{0, {12, 23, 43, 255}},
                                           {.45f, {29, 49, 77, 255}},
                                           {.55f, {36, 116, 129, 255}},
                                           {1, {62, 103, 112, 255}}};
        cui_effect_buffer source = {effect_source, 128, 64, 128 * 4, sizeof(effect_source)};
        cui_effect_buffer output = {effect_pixels, 128, 64, 128 * 4, sizeof(effect_pixels)};
        if (!cui_effect_linear_gradient(source, stops, 4, effect_angle))
            return;
        for (unsigned y = 20; y < 44; y++)
            for (unsigned x = 50; x < 78; x++) {
                unsigned char *pixel = effect_source + (y * 128 + x) * 4;
                pixel[0] = 235;
                pixel[1] = 244;
                pixel[2] = 245;
                pixel[3] = 255;
            }
        cui_bitmap bitmap = {effect_source, 128, 64, 128 * 4, ++effect_revision};
        if (!cui_effect_blur(bitmap, output, effect_blur, effect_scratch, 128 * 64 * 4))
            return;
        bitmap.pixels = effect_pixels;
        if (!cui_effect_opacity(bitmap, output, effect_opacity))
            return;
        effect_dirty = false;
    }
    cui_label(ui, theme, cui_string("Gradient, image blur and opacity"));
    cui_begin_box(ui, cui_id_from(ui, "effects-row"), (cui_box){.row = true, .gap = 12});
    float panel = fminf(260, (width - 60) * .5f);
    cui_paint_bitmap(
        ui, cui_id_from(ui, "gradient-original"),
        (cui_bitmap_paint){.bitmap = {effect_source, 128, 64, 128 * 4, effect_revision},
                           .opacity = 1},
        (cui_box){.width = panel, .height = 100, .radius = theme->radius_lg});
    cui_paint_bitmap(
        ui, cui_id_from(ui, "gradient-processed"),
        (cui_bitmap_paint){.bitmap = {effect_pixels, 128, 64, 128 * 4, effect_revision},
                           .opacity = 1},
        (cui_box){.width = panel, .height = 100, .radius = theme->radius_lg});
    cui_end_box(ui);
}
static composed_app *gallery_app;
float gallery_app_stat(int field) {
    app_snapshot state = composed_app_snapshot(gallery_app);
    return field == 0   ? state.page
           : field == 1 ? state.plan
           : field == 2 ? state.logged_in
           : field == 3 ? state.saves
           : field == 4 ? state.notifications
                        : state.sync;
}
int gallery_init(void) {
    if (ui)
        return 1;
    if (!cui_color_entry_init(&color_entry, (cui_color_entry_value){true, chosen_color}))
        return 0;
    gallery_app = composed_app_create(NULL, NULL);
    if (!gallery_app)
        return 0;
    if (!cui_speech_init(
            &speech_state,
            (cui_speech_workspace){speech_transcript, sizeof(speech_transcript), speech_messages,
                                   sizeof(speech_messages) / sizeof(speech_messages[0]),
                                   speech_queue, sizeof(speech_queue)},
            (cui_speech_recognizer){.available = gallery_speech_available,
                                    .start = gallery_speech_start_recognizer,
                                    .push_audio = gallery_speech_push_audio,
                                    .finish = gallery_speech_finish_recognizer,
                                    .close = gallery_speech_close_recognizer},
            (cui_speech_input){.start = gallery_speech_start_input,
                               .close = gallery_speech_close_input},
            gallery_speech_event, NULL))
        return 0;
    questionnaire_edit =
        cui_edit_create((cui_edit_config){.buffer = questionnaire_input_buffer,
                                          .capacity = sizeof(questionnaire_input_buffer),
                                          .history_bytes = 8192,
                                          .max_history_steps = 32});
    if (!questionnaire_edit)
        return 0;
    questionnaire_items[0] =
        (cui_questionnaire_item){.name = cui_string("goal"),
                                 .label = cui_string("What would you like to do?"),
                                 .description = cui_string("Choose one direction to get started."),
                                 .required = true,
                                 .choices = questionnaire_choices,
                                 .choice_count = 3};
    questionnaire_items[1] = (cui_questionnaire_item){
        .name = cui_string("details"),
        .label = cui_string("Anything else we should know?"),
        .description = cui_string("Add a note or skip this optional question."),
        .input = questionnaire_edit,
        .input_label = cui_string("Your note")};
    if (cui_questionnaire_init(
            &questionnaire_state, questionnaire_items, 2,
            (cui_questionnaire_workspace){.items = questionnaire_runtime,
                                          .item_capacity = 2,
                                          .choices = questionnaire_choice_runtime,
                                          .choice_capacity = 3,
                                          .text = questionnaire_text,
                                          .text_capacity = sizeof(questionnaire_text),
                                          .max_answer_bytes = 120},
            gallery_questionnaire_event, NULL) != CUI_QUESTIONNAIRE_OK)
        return 0;
    chart_state.pixels = chart_pixels;
    chart_state.byte_capacity = sizeof(chart_pixels);
    chart_state.hovered = SIZE_MAX;
    cui_icon_state_init(&icon_state);
    toolbar_query = cui_edit_create((cui_edit_config){.buffer = toolbar_query_buffer,
                                                      .capacity = sizeof(toolbar_query_buffer)});
    if (!toolbar_query)
        return 0;
    if (!cui_resizable_state_init(&resizable_state, resizable_slots, 2))
        return 0;
    if (!cui_sidebar_state_init(&sidebar_state, sidebar_cache, 2) ||
        !cui_textarea_state_init(&textarea_state, textarea_lines, 256))
        return 0;
    textarea_edit = cui_edit_create((cui_edit_config){.buffer = textarea_buffer,
                                                      .capacity = sizeof(textarea_buffer),
                                                      .multiline = true,
                                                      .history_bytes = 32768,
                                                      .max_history_steps = 64});
    if (!textarea_edit)
        return 0;
    editor_edit = cui_edit_create((cui_edit_config){.buffer = editor_buffer,
                                                    .capacity = sizeof(editor_buffer),
                                                    .multiline = true,
                                                    .history_bytes = 32768,
                                                    .max_history_steps = 64});
    editor_query = cui_edit_create(
        (cui_edit_config){.buffer = editor_query_buffer, .capacity = sizeof(editor_query_buffer)});
    editor_replacement = cui_edit_create((cui_edit_config){
        .buffer = editor_replacement_buffer, .capacity = sizeof editor_replacement_buffer});
    if (!editor_edit || !editor_query || !editor_replacement ||
        cui_edit_attach_selections(editor_edit, editor_selections, 8, editor_selection_history,
                                   65) != CUI_EDIT_OK ||
        !cui_editor_state_init(&editor_state, editor_edit, editor_lines, 512) ||
        !cui_editor_multicursor_init(&editor_state, editor_cursor_anchors, 8) ||
        !cui_editor_search_init(&editor_search, editor_query) ||
        !cui_editor_matcher_init(&editor_matcher, editor_matches, 512) ||
        !cui_editor_search_enable_replace(&editor_search, &editor_matcher, editor_replacement,
                                          editor_replace_scratch, sizeof editor_replace_scratch))
        return 0;
    if (!cui_editor_set_folds(&editor_state, editor_folds, 1))
        return 0;
    ui = cui_create((cui_config){.width = 720,
                                 .height = 300,
                                 .max_elements = 512,
                                 .max_commands = 2048,
                                 .frame_bytes = 65536,
                                 .measure = measure,
                                 .measure_styled = measure_styled,
                                 .font_metrics = measure_font_metrics});
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
    notification_settings.shadow_policy = CUI_NOTIFICATION_SHADOW_SOURCE;
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
    virtual_horizontal = cui_virtual_create((cui_virtual_config){100, 140});
    if (!virtual_horizontal ||
        cui_virtual_rebuild(virtual_horizontal, keys, NULL, 100) != CUI_VIRTUAL_OK) {
        free(keys);
        return 0;
    }
    free(keys);
    dock_view = cui_dock_view_create(15, 16);
    dock = cui_dock_create_nary((cui_dock_config){.node_capacity = 15, .panel_capacity = 16});
    if (dock) {
        cui_dock_insert(dock, cui_dock_root(dock), 1, 0);
        cui_dock_insert(dock, cui_dock_root(dock), 2, 1);
    }
    dock_workspace = cui_dock_workspace_create();
    if (!dock_workspace)
        return 0;
    for (int i = 0; i < 3; i++) {
        dock_edges[i] =
            cui_dock_create_nary((cui_dock_config){.node_capacity = 15, .panel_capacity = 16});
        dock_edge_views[i] = cui_dock_view_create(15, 16);
        if (!dock_edges[i] || !dock_edge_views[i] ||
            cui_dock_insert(dock_edges[i], cui_dock_root(dock_edges[i]), (uint64_t)i + 10, 0) !=
                CUI_DOCK_OK ||
            !cui_dock_workspace_set_edge(dock_workspace, (cui_dock_region)(i + 1), false,
                                         i == 2 ? 100 : 160))
            return 0;
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
    if (!cui_carousel_state_init(&carousel_state, carousel_geometry, 4, 4))
        return 0;
    carousel_state.on_change = gallery_carousel_changed;
    settings_search_edit = cui_edit_create((cui_edit_config){
        .buffer = settings_search_buffer, .capacity = sizeof(settings_search_buffer)});
    settings_name_edit = cui_edit_create((cui_edit_config){
        .buffer = settings_name_buffer, .capacity = sizeof(settings_name_buffer)});
    settings_number_state.edit = cui_edit_create((cui_edit_config){
        .buffer = settings_number_buffer, .capacity = sizeof(settings_number_buffer)});
    if (!settings_search_edit || !settings_name_edit || !settings_number_state.edit ||
        !cui_settings_state_init(&settings_state, settings_pages_state, 2, settings_search_edit))
        return 0;
    settings_mode = cui_id_from(ui, "settings-standard");
    if (!cui_settings_dropdown_init(&settings_mode_menu, cui_id_from(ui, "settings-mode-popup"),
                                    settings_mode_entries, settings_mode_checks, 3))
        return 0;
    command_query = cui_edit_create((cui_edit_config){.buffer = command_query_buffer,
                                                      .capacity = sizeof(command_query_buffer),
                                                      .history_bytes = 4096,
                                                      .max_history_steps = 8});
    if (!command_query || !cui_command_init(&command_state, command_rows, 32, command_query))
        return 0;
    cui_surface_init(&command_surface, cui_id_from(ui, "command-dialog"), CUI_SURFACE_DIALOG);
    if (!cui_root_registry_init(&root_registry, root_plugin_entries, 1) ||
        !cui_root_register(&root_registry, (cui_root_plugin){.key = 1,
                                                             .build = gallery_root_plugin_build,
                                                             .prepare = gallery_root_plugin_prepare,
                                                             .style = gallery_root_plugin_style,
                                                             .overlay = gallery_root_plugin_overlay,
                                                             .registration_user = &root_counts}) ||
        !cui_root_init(&root_view, root_plugin_instances, 1, &root_registry, NULL))
        return 0;
    cui_surface_init(&root_dialog_surface, cui_id_from(ui, "root-dialog"), CUI_SURFACE_DIALOG);
    cui_surface_init(&root_sheet_surface, cui_id_from(ui, "root-sheet"), CUI_SURFACE_SHEET);
    cui_notification_settings root_notice_settings = cui_notification_settings_default();
    root_notice_settings.shadow_policy = CUI_NOTIFICATION_SHADOW_SOURCE;
    root_notice_settings.width = 280;
    root_notice_settings.max_visible = 2;
    if (!cui_notification_init(&root_notices, root_notice_entries, 4, root_notice_settings,
                               (cui_notification_host){0}) ||
        !cui_root_bind_facilities(&root_view, root_dialog_slots, 2, &root_notices))
        return 0;
    if (!gallery_text_view_load())
        return 0;
    bool initialized = ui && edit && group_edit && form_name_edit && form_email_edit && dock &&
                       dock_view && query && number_state.edit && rebuilt == CUI_VIRTUAL_OK;
    if (initialized)
        gallery_input_regex_init();
    return initialized;
}
static int gallery_last_key_down, gallery_last_key_up;
static unsigned gallery_last_pointer_down_clicks, gallery_last_pointer_up_clicks;
static unsigned gallery_pointer_down_count, gallery_pointer_up_count;
static unsigned gallery_key_down_count, gallery_key_up_count;
static int gallery_last_composing, gallery_last_prefer_character;
void gallery_keyboard_event(int type, int key, unsigned mods, int repeat, int composing,
                            int prefer_character) {
    gallery_last_composing = composing != 0;
    gallery_last_prefer_character = prefer_character != 0;
    if (type == CUI_EVENT_KEY_DOWN) {
        gallery_last_key_down = key;
        gallery_key_down_count++;
    } else if (type == CUI_EVENT_KEY_UP) {
        gallery_last_key_up = key;
        gallery_key_up_count++;
    }
    if (ui)
        cui_queue_event(ui, (cui_event){.type = (cui_event_type)type,
                                        .key = (cui_key)key,
                                        .modifiers = mods,
                                        .repeat = repeat != 0,
                                        .ime_composing = composing != 0,
                                        .prefer_character_input = prefer_character != 0});
}
int gallery_key_identity(int character) {
    return character >= 0 && character <= 127 ? (int)cui_key_from_character((char)character) : 0;
}
int gallery_key_character(int key) {
    return (int)cui_key_character((cui_key)key);
}
int gallery_key_stats(int field) {
    return field == 0   ? gallery_last_key_down
           : field == 1 ? gallery_last_key_up
           : field == 2 ? (int)gallery_key_down_count
           : field == 3 ? (int)gallery_key_up_count
           : field == 4 ? gallery_last_composing
                        : gallery_last_prefer_character;
}
int gallery_pointer_stats(int field) {
    return field == 0   ? (int)gallery_last_pointer_down_clicks
           : field == 1 ? (int)gallery_last_pointer_up_clicks
           : field == 2 ? (int)gallery_pointer_down_count
                        : (int)gallery_pointer_up_count;
}
void gallery_event(int type, float x, float y, int key, unsigned mods, int repeat, int button,
                   unsigned click_count) {
    if (type == CUI_EVENT_KEY_DOWN) {
        gallery_last_key_down = key;
        gallery_key_down_count++;
    } else if (type == CUI_EVENT_KEY_UP) {
        gallery_last_key_up = key;
        gallery_key_up_count++;
    } else if (type == CUI_EVENT_POINTER_DOWN) {
        gallery_last_pointer_down_clicks = click_count;
        gallery_pointer_down_count++;
    } else if (type == CUI_EVENT_POINTER_UP) {
        gallery_last_pointer_up_clicks = click_count;
        gallery_pointer_up_count++;
    }
    if (ui)
        cui_queue_event(ui, (cui_event){.type = (cui_event_type)type,
                                        .x = x,
                                        .y = y,
                                        .key = (cui_key)key,
                                        .modifiers = mods,
                                        .repeat = repeat != 0,
                                        .pointer_button = (cui_pointer_button)button,
                                        .click_count = click_count});
}
unsigned gallery_touch_page_cancels(void) {
    return page_touch_cancels;
}
int gallery_touch_page_available(unsigned pointer_id) {
    return cui_touch_page_available(&page_touch_host, pointer_id);
}
void gallery_touch(int type, float x, float y, unsigned pointer_id, int pointer_type) {
    if (type == CUI_EVENT_CANCEL) {
        page_touch_cancels++;
        cui_touch_page_end(&page_touch_host, pointer_id);
    }
    if (type == CUI_EVENT_POINTER_DOWN) {
        gallery_last_pointer_down_clicks = 1;
        gallery_pointer_down_count++;
    } else if (type == CUI_EVENT_POINTER_UP) {
        gallery_last_pointer_up_clicks = 1;
        gallery_pointer_up_count++;
    }
    if (ui)
        cui_queue_event(ui, (cui_event){.type = (cui_event_type)type,
                                        .x = x,
                                        .y = y,
                                        .pointer_id = pointer_id,
                                        .pointer_type = (cui_pointer_type)pointer_type,
                                        .click_count = type == CUI_EVENT_POINTER_DOWN ||
                                                               type == CUI_EVENT_POINTER_UP
                                                           ? 1
                                                           : 0});
}
void gallery_scroll_pixels(float x, float y, float dx, float dy) {
    if (ui)
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                        .x = x,
                                        .y = y,
                                        .dx = dx,
                                        .dy = dy,
                                        .scroll_precise = true});
}
void gallery_scroll(float x, float y, float dx, float dy) {
    if (ui)
        cui_queue_event(ui,
                        (cui_event){.type = CUI_EVENT_SCROLL, .x = x, .y = y, .dx = dx, .dy = dy});
}
void gallery_text(const char *text, size_t length) {
    cui_event event = {.type = CUI_EVENT_TEXT, .text_data = text, .text_length = length};
    if (!text)
        return;
    if (length < sizeof(event.text))
        memcpy(event.text, text, length);
    cui_queue_event(ui, event);
}
/* UTF-16 host offsets in preedit, translated to public UTF-8 byte offsets. */
static size_t gallery_utf16_byte(const char *text, size_t length, unsigned units) {
    size_t byte = 0;
    while (byte < length && units) {
        unsigned char lead = (unsigned char)text[byte];
        size_t width = lead < 0x80 ? 1 : lead < 0xe0 ? 2 : lead < 0xf0 ? 3 : 4;
        unsigned count = width == 4 ? 2 : 1;
        if (units < count || width > length - byte)
            break;
        units -= count;
        byte += width;
    }
    return byte;
}
static cui_id gallery_ime_owner;
static bool gallery_ime_active;
void gallery_composition(int phase, const char *text, size_t length, unsigned anchor,
                         unsigned cursor) {
    if (!ui || phase < 0 || phase > 3 || (!text && length))
        return;
    if (phase == 0 || (phase == 1 && !gallery_ime_active)) {
        gallery_ime_owner = cui_focused(ui);
        gallery_ime_active = true;
    }
    if (phase == 2 && !gallery_ime_active)
        return;
    cui_event event = {.type = (cui_event_type)(CUI_EVENT_COMPOSITION_START + phase),
                       .composition_target = gallery_ime_owner,
                       .text_data = text,
                       .text_length = length,
                       .composition_anchor = gallery_utf16_byte(text, length, anchor),
                       .composition_cursor = gallery_utf16_byte(text, length, cursor)};
    (void)cui_queue_event(ui, event);
    if (phase >= 2) {
        gallery_ime_active = false;
        gallery_ime_owner = 0;
    }
}
/* Finalized portable configuration; preserve all 64 bits across the JS ABI. */
unsigned gallery_text_config(unsigned field) {
    cui_text_input_state state;
    if (!cui_get_text_input_config(ui, &state))
        return 0;
    switch (field) {
    case 0:
        return 1;
    case 1:
        return (unsigned)state.owner;
    case 2:
        return (unsigned)(state.owner >> 32);
    case 3:
        return (unsigned)state.generation;
    case 4:
        return (unsigned)(state.generation >> 32);
    case 5:
        return (unsigned)state.config.type;
    case 6:
        return (unsigned)state.config.purpose;
    case 7:
        return state.multiline;
    default:
        return 0;
    }
}
void gallery_composition_bytes(int phase, const char *text, size_t length, unsigned anchor,
                               unsigned cursor, unsigned owner_low, unsigned owner_high) {
    if (!ui || phase < 0 || phase > 3 || (!text && length))
        return;
    cui_id owner = (cui_id)owner_low | ((cui_id)owner_high << 32);
    if (!owner)
        return;
    (void)cui_queue_event(ui,
                          (cui_event){.type = (cui_event_type)(CUI_EVENT_COMPOSITION_START + phase),
                                      .composition_target = owner,
                                      .text_data = text,
                                      .text_length = length,
                                      .composition_anchor = anchor,
                                      .composition_cursor = cursor});
}
float gallery_ime_rect(unsigned field) {
    cui_rect rect;
    if (!cui_text_input_rect(ui, &rect))
        return -1;
    return field == 0 ? rect.x : field == 1 ? rect.y : field == 2 ? rect.w : rect.h;
}
void gallery_focus(unsigned index) {
    if (index < semantic_count)
        cui_focus(ui, semantics[index].id);
    else
        cui_focus(ui, 0);
}
/* Browser host owns decoding. This fixed staging area is borrowed only during
 * the synchronous Canvas draw-list consumption; JS never writes during render. */
static unsigned char video_pixels[320 * 180 * 4];
static cui_video_view video_view;
static cui_video_controls_state video_controls;
static cui_video_request video_requests[32];
static unsigned video_request_count, video_session, video_epoch, video_revision;
static bool video_frame_valid, video_request_overflow;
static double video_timestamp;
static void video_request(void *user, const cui_video_request *request) {
    (void)user;
    if (video_request_count < 32)
        video_requests[video_request_count++] = *request;
    else
        video_request_overflow = true;
}
void gallery_video_session(unsigned session) {
    video_session = session;
    video_epoch = 1;
    video_revision = 0;
    video_frame_valid = false;
    video_request_count = 0;
    video_request_overflow = false;
    memset(&video_controls, 0, sizeof video_controls);
    memset(&video_view, 0, sizeof video_view);
    (void)cui_video_init(&video_view.timeline, (cui_video_config){.loop = true});
    video_view.source_id = session;
    video_view.status = CUI_VIDEO_LOADING;
}
void gallery_video_sync(unsigned session, unsigned epoch, double position, double duration,
                        double rate, int playing, int status, unsigned caps, int fullscreen) {
    if (!session || session != video_session || !epoch || epoch < video_epoch ||
        !isfinite(position) || position < 0 || !isfinite(duration) || duration < 0 ||
        !isfinite(rate) || rate <= 0 || status < 0 || status > CUI_VIDEO_FAILED || (caps & ~15u))
        return;
    if (epoch != video_epoch)
        video_frame_valid = false;
    video_epoch = epoch;
    video_view.timeline = (cui_video){.position = fmin(position, duration),
                                      .duration = duration,
                                      .duration_known = duration > 0,
                                      .seekable = (caps & CUI_VIDEO_CAP_SEEK) != 0,
                                      .rate = rate,
                                      .generation = epoch,
                                      .playing = playing != 0,
                                      .loop = true};
    video_view.capabilities = caps;
    video_view.status = (cui_video_status)status;
    video_view.fullscreen = fullscreen != 0;
    video_view.status_description =
        status == CUI_VIDEO_LOADING     ? cui_string("Loading browser-decoded sample")
        : status == CUI_VIDEO_BUFFERING ? cui_string("Seeking / buffering")
        : status == CUI_VIDEO_FAILED ? cui_string("Browser media operation failed; see host status")
                                     : (cui_str){0};
}
unsigned char *gallery_video_buffer(unsigned session, unsigned epoch) {
    return session && session == video_session && epoch == video_epoch ? video_pixels : NULL;
}
int gallery_video_commit(unsigned session, unsigned epoch, double timestamp) {
    if (!gallery_video_buffer(session, epoch) || !isfinite(timestamp) || timestamp < 0 ||
        !video_view.timeline.duration_known || timestamp > video_view.timeline.duration ||
        video_revision == UINT32_MAX)
        return 0;
    video_timestamp = timestamp;
    ++video_revision;
    video_frame_valid = true;
    return 1;
}
unsigned gallery_video_request_count(void) {
    return video_request_count;
}
double gallery_video_request(unsigned i, unsigned field) {
    if (i >= video_request_count)
        return -1;
    const cui_video_request *r = &video_requests[i];
    switch (field) {
    case 0:
        return r->kind;
    case 1:
        return r->phase;
    case 2:
        return r->source_id;
    case 3:
        return r->generation;
    case 4:
        return r->kind == CUI_VIDEO_PLAYING || r->kind == CUI_VIDEO_FULLSCREEN
                   ? (double)r->value.enabled
               : r->kind == CUI_VIDEO_RATE ? r->value.rate
                                           : r->value.seconds;
    default:
        return -1;
    }
}
unsigned gallery_video_overflow(void) {
    return video_request_overflow;
}
void gallery_video_drain(void) {
    video_request_count = 0;
    video_request_overflow = false;
}

int gallery_frame(int story, int theme, int disabled, float width, float height) {
    if (!ui || width < 1 || height < 1)
        return 0;
    cui_theme t = theme == 1   ? cui_theme_dark()
                  : theme == 2 ? cui_theme_high_contrast()
                               : cui_theme_light();
    if (gallery_preset_index >= 0)
        cui_theme_preset_get((size_t)gallery_preset_index, &t);
    if (compact_density) {
        t.font_size *= .875f;
        t.control_height *= .8f;
        t.padding *= .7f;
        t.gap *= .75f;
    }
    if (design_radius >= CUI_RADIUS_NONE && design_radius <= CUI_RADIUS_FULL)
        t.radius = t.radius_lg = cui_radius((cui_radius_size)design_radius, 1);
    if (design_spacing >= 0)
        t.gap = cui_space(design_spacing);
    if (previous_story != story) {
        if (previous_story == 78)
            (void)cui_speech_cancel(&speech_state);
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
        cui_overlay_close(ui, &root_dialog_surface.overlay);
        cui_overlay_close(ui, &root_sheet_surface.overlay);
        previous_story = story;
    }
    redraw = false;
    next_wake = 0;
    /* The host has synchronously painted the preceding draw list by this call. */
    cui_icon_state_collect(&icon_state);
    if (avatar_custom_placeholder_ready)
        cui_icon_state_collect(&avatar_custom_placeholder);
    t.scrollbar = (cui_scrollbar_style){.visibility = (cui_scrollbar_visibility)scrollbar_policy,
                                        .thickness = scrollbar_policy ? (float)scrollbar_width : 0,
                                        .thumb_radius = (float)scrollbar_radius,
                                        .track_radius = (float)scrollbar_radius,
                                        .no_track = !scrollbar_track,
                                        .idle_seconds = .8f,
                                        .fade_seconds = .2f,
                                        .reduced_motion = reduced_motion};
    if ((story == 81 || story == 82) && video_view.fullscreen)
        t = cui_theme_dark();
    current_theme = t;
    if (story == 80)
        composed_app_set_theme(gallery_app, &t);
    /* Owner synchronization runs before submission and after prior draw consumption. A dirty draft
     * survives picker changes until the owner explicitly replaces it. */
    cui_color selected = color_entry.committed.color;
    bool color_changed = !color_entry.committed.present || selected.r != chosen_color.r ||
                         selected.g != chosen_color.g || selected.b != chosen_color.b ||
                         selected.a != chosen_color.a;
    if (color_entry_restore || (!color_entry.dirty && color_changed)) {
        if (cui_color_entry_sync(&color_entry, (cui_color_entry_value){true, chosen_color},
                                 color_entry_restore) == CUI_COLOR_ENTRY_OK)
            redraw = true;
    }
    color_entry_restore = false;
    cui_begin_frame(ui, width, height, host_dt);
    cui_set_reduced_motion(ui, reduced_motion);
    if (story != 75 && story != 80)
        cui_begin_box(ui, cui_id_from(ui, "preview"),
                      (cui_box){.width = width,
                                .height = height,
                                .padding = 24,
                                .align_x = ((story == 81 || story == 82) && video_view.fullscreen)
                                               ? CUI_ALIGN_CENTER
                                               : CUI_ALIGN_START,
                                .align_y = ((story == 81 || story == 82) && video_view.fullscreen)
                                               ? CUI_ALIGN_CENTER
                                               : CUI_ALIGN_START,
                                .gap = design_spacing >= 0 ? t.gap : 16,
                                .background = t.background});
    switch (story) {
    case 81:
    case 82: {
        float player_width =
            video_view.fullscreen
                ? fmaxf(1, fminf(width - 48, (height - (width - 48 < 350 ? 200 : 160)) * 320 / 180))
                : fmaxf(160, fminf(width - 48, 640));
        cui_video_player_options o = cui_video_player_defaults();
        o.controls = story == 82;
        o.surface.geometry = (cui_image_geometry){
            .width = player_width, .height = player_width * 180 / 320, .fit = CUI_IMAGE_CONTAIN};
        o.surface.description = cui_string("Decoded moving color test sample");
        o.controls_options.width = player_width;
        o.controls_options.visible |= CUI_VIDEO_CONTROL_FULLSCREEN | CUI_VIDEO_CONTROL_RATE;
        o.controls_options.skip_seconds = 2;
        o.controls_options.requested_rate = video_view.timeline.rate == 1 ? 1.5 : 1;
        o.controls_options.disabled = disabled != 0;
        o.controls_options.on_request = video_request;
        cui_video_player_frame frame = {.source_id = video_session,
                                        .frame = {.bitmap = {.pixels = video_pixels,
                                                             .width = 320,
                                                             .height = 180,
                                                             .stride = 320 * 4,
                                                             .revision = video_revision},
                                                  .generation = video_epoch,
                                                  .timestamp = video_timestamp}};
        if (story == 81) {
            if (video_frame_valid)
                (void)cui_video_surface(ui, cui_id_from(ui, "browser-video-surface"),
                                        &video_view.timeline, &frame.frame, o.surface);
            else {
                cui_begin_box(ui, cui_id_from(ui, "browser-video-loading"),
                              (cui_box){.width = player_width, .height = player_width * 180 / 320});
                cui_end_box(ui);
            }
        } else
            (void)cui_video_player(ui, cui_id_from(ui, "browser-video"), &t, &video_controls,
                                   video_view, video_frame_valid ? &frame : NULL, o);
        break;
    }
    case 80:
        composed_app_scene(ui, gallery_app, width, height);
        break;
    case 64: {
        cui_design_metrics metrics;
        bool scaled = design_size != CUI_DESIGN_INHERIT &&
                      cui_design_measure(design_size, t.font_size / 16, &metrics);
        float font = scaled ? metrics.font_size : t.font_size;
        float padding = scaled ? metrics.padding_x : 8;
        float label_need = cui_measure_text(ui, cui_string("Created"), font * .875f).width;
        float value_need = fmaxf(cui_measure_text(ui, cui_string("October"), font).width,
                                 cui_measure_text(ui, cui_string("Lovelace"), font).width);
        float inner_width = fmaxf(width, fmaxf(label_need, value_need) + 2 * padding + 48);
        bool pan = inner_width > width + 1;
        cui_id pan_id = cui_id_from(ui, "description-horizontal-preview");
        cui_scroll_options pan_options = {.width = width - 48,
                                          .height = height - 48,
                                          .content_width = inner_width - 48,
                                          .content_height = height - 48,
                                          .horizontal = true,
                                          .wheel_step = 32,
                                          .bar_thickness = 8};
        if (pan && !cui_scrollable_begin(ui, pan_id, &t, &description_preview_scroll, pan_options))
            break;
        cui_description_item items[] = {
            {.id = cui_id_from(ui, "description-owner"),
             .label = cui_string("Owner"),
             .value = cui_string("Ada Lovelace")},
            {.id = cui_id_from(ui, "description-team"),
             .label = cui_string("Team"),
             .value = cui_string("Core UI")},
            {.id = cui_id_from(ui, "description-separator"), .separator = true},
            {.id = cui_id_from(ui, "description-status"),
             .label = cui_string("Status"),
             .value = cui_string("In progress")},
            {.id = cui_id_from(ui, "description-created"),
             .label = cui_string("Created"),
             .value = cui_string("12 October 2026")}};
        cui_description_options options = cui_description_options_default();
        options.columns = (unsigned)description_columns;
        options.vertical = description_vertical != 0;
        options.unbordered = description_plain != 0;
        options.size = (cui_description_size)description_size;
        options.width = inner_width - 48;
        options.name = cui_string("Project details");
        options.design_size = design_size;
        if (options.width > 0) {
            static const float design_fonts[] = {16, 12, 14, 16, 16, 18, 20, 24,
                                                 30, 36, 48, 60, 72, 96, 128};
            float font = design_fonts[design_size >= CUI_DESIGN_XS && design_size <= CUI_DESIGN_9XL
                                          ? design_size
                                          : 0] *
                         t.font_size / 16;
            unsigned fitting = (unsigned)(options.width / (font * 8 + 32));
            if (fitting < 1)
                fitting = 1;
            if (options.columns > fitting)
                options.columns = fitting;
            if (font * 6 + 120 > options.width)
                options.vertical = true;
        }
        cui_description_list(ui, cui_id_from(ui, "description-list"), &t, items, 5, options);
        if (pan)
            cui_scrollable_end(ui, pan_id, &t, &description_preview_scroll, pan_options);
        break;
    }
    case 65: {
        cui_chrome_item left[] = {
            {.id = cui_id_from(ui, "status-branch"), .text = cui_string("main")}};
        cui_chrome_item center[] = {
            {.id = cui_id_from(ui, "status-file"), .text = cui_string("gallery.c")}};
        cui_chrome_item right[] = {
            {.id = cui_id_from(ui, "status-position"), .text = cui_string("Ln 42, Col 8")}};
        cui_box custom = {.width = width - 48,
                          .background = t.status_bar,
                          .border = t.status_bar_border,
                          .border_width = 1,
                          .radius = 8};
        cui_status_bar(ui, cui_id_from(ui, "status-bar"), &t, (cui_chrome_region){left, 1},
                       (cui_chrome_region){center, 1},
                       (cui_chrome_region){right, status_right ? 1u : 0u},
                       (cui_status_bar_options){.width = width - 48,
                                                .name = cui_string("Editor status"),
                                                .style = status_custom ? &custom : NULL});
        break;
    }
    case 66: {
        cui_resizable_panel panels[2];
        panels[0] = cui_resizable_panel_default(cui_id_from(ui, "resizable-explorer"));
        panels[1] = cui_resizable_panel_default(cui_id_from(ui, "resizable-editor"));
        panels[0].accessibility_label = cui_string("Explorer panel");
        panels[1].accessibility_label = cui_string("Editor panel");
        panels[0].initial_size = resizable_vertical ? 105 : 140;
        panels[0].has_initial_size = true;
        panels[0].minimum = 80;
        panels[0].maximum = 300;
        panels[0].custom_range = true;
        panels[1].minimum = 80;
        /* The flexible editor must fit the responsive container. A fixed 600 px
         * cap plus the explorer cap made wide layouts infeasible and locked both panes. */
        panels[1].maximum = resizable_vertical ? 210 : width - 48;
        panels[1].custom_range = true;
        panels[0].content = resizable_content;
        panels[1].content = resizable_content;
        panels[0].content_user = (void *)(uintptr_t)0;
        panels[1].content_user = (void *)(uintptr_t)1;
        panels[0].box.background = t.secondary;
        panels[1].box.background = t.background;
        panels[0].box.border = panels[1].box.border = t.border;
        panels[0].box.border_width = panels[1].box.border_width = 1;
        cui_resizable_options options = cui_resizable_options_default();
        options.axis = resizable_vertical ? CUI_RESIZABLE_VERTICAL : CUI_RESIZABLE_HORIZONTAL;
        options.extent = resizable_vertical ? 210 : width - 48;
        options.cross_size = resizable_vertical ? width - 48 : 160;
        options.disabled = disabled || resizable_disabled;
        options.reduced_motion = reduced_motion;
        options.now_seconds = host_seconds;
        cui_resizable(ui, cui_id_from(ui, "resizable-group"), &t, &resizable_state, panels, 2,
                      options);
        char sizes[96];
        snprintf(sizes, sizeof sizes, "Pane %s: Explorer %.0f px / Editor %.0f px",
                 resizable_vertical ? "heights" : "widths", resizable_state.slots[0].size,
                 resizable_state.slots[1].size);
        cui_label(ui, &t, cui_string(sizes));
        cui_label(ui, &t, cui_string("Drag the divider, or focus it and use the arrow keys."));
        break;
    }
    case 67: {
        const char *names[] = {"check", "bell", "arrow-right"};
        if (icon_loaded != icon_choice) {
            cui_icon_set_name(&icon_state, cui_string(names[icon_choice]), NULL, NULL);
            icon_loaded = icon_choice;
        }
        cui_box icon_style = {.padding = icon_layout == 1   ? 4
                                         : icon_layout == 2 ? 24
                                                            : 0,
                              .max_width = icon_layout == 3 ? 8 : 0};
        cui_icon_options options = {
            .size = (cui_icon_size)icon_size, .design_size = design_size, .style = &icon_style};
        if (icon_rotate) {
            options.custom_transform = true;
            options.transform = cui_icon_rotation(.785398163f);
        }
        if (icon_color) {
            options.custom_color = true;
            options.color = t.primary;
        }
        cui_icon(ui, cui_id_from(ui, "icon-preview"), &t, &icon_state, options);
        cui_label(ui, &t, cui_string(names[icon_choice]));
        break;
    }
    case 68: {
        toolbar_disabled = disabled != 0;
        size_t event_count = 0;
        const cui_event *events = cui_frame_events(ui, &event_count);
        if (!toolbar_disabled && toolbar_show_input)
            for (size_t i = 0; i < event_count; i++) {
                const cui_event *event = &events[i];
                if (event->type == CUI_EVENT_KEY_DOWN && !event->repeat && !event->ime_composing &&
                    !event->prefer_character_input &&
                    !(event->modifiers & (CUI_MOD_ALT | CUI_MOD_SHIFT)) &&
                    (event->modifiers & (CUI_MOD_CTRL | CUI_MOD_META)) &&
                    event->key == cui_key_from_character('F') && !cui_event_consumed(ui, i)) {
                    cui_consume_event(ui, i);
                    cui_focus(ui, cui_id_from(ui, "toolbar-query"));
                }
            }
        const char *labels[] = {"Save", "Undo", "Bold", "Italic"};
        cui_toolbar_command commands[4];
        for (unsigned i = 0; i < 4; i++)
            commands[i] = (cui_toolbar_command){.id = cui_id_from(ui, labels[i]),
                                                .label = cui_string(labels[i]),
                                                .disabled = toolbar_disabled,
                                                .on_activate = toolbar_activate,
                                                .activate_user = &toolbar_activations[i]};
        cui_toolbar_item items[5];
        size_t count = 0;
        for (unsigned i = 0; i < 2; i++)
            items[count++] =
                (cui_toolbar_item){.id = cui_id_from(ui, i ? "undo-wrap" : "save-wrap"),
                                   .sized = true,
                                   .render = cui_toolbar_command_render,
                                   .user = &commands[i]};
        if (toolbar_grouped) {
            items[count++] =
                (cui_toolbar_item){.render = toolbar_group_render, .user = &commands[2]};
        } else {
            for (unsigned i = 2; i < 4; i++)
                items[count++] =
                    (cui_toolbar_item){.id = cui_id_from(ui, i == 2 ? "bold-wrap" : "italic-wrap"),
                                       .sized = true,
                                       .render = cui_toolbar_command_render,
                                       .user = &commands[i]};
        }
        if (toolbar_show_input)
            items[count++] = (cui_toolbar_item){.render = toolbar_query_render};
        cui_toolbar(ui, cui_id_from(ui, "toolbar-preview"), &t, items, count,
                    (cui_toolbar_options){.size = (cui_toolbar_size)toolbar_size,
                                          .design_size = design_size,
                                          .custom_size = 20,
                                          .label = cui_string("Editing toolbar"),
                                          .disabled = disabled != 0});
        cui_str query = cui_edit_get_text(toolbar_query);
        unsigned matches = 0;
        for (unsigned i = 0; i < 3; i++)
            matches += toolbar_contains(toolbar_document[i], query);
        char summary[128];
        if (query.length)
            snprintf(summary, sizeof summary, "Find: %u matching %s", matches,
                     matches == 1 ? "line" : "lines");
        else
            snprintf(summary, sizeof summary, "Find: enter a search term");
        cui_label(ui, &t, cui_string(summary));
        if (query.length && !matches)
            cui_label(ui, &t, cui_string("No results in this document."));
        cui_label(ui, &t, cui_string("Document"));
        if (toolbar_show_input)
            cui_label(ui, &t, cui_string("Ctrl/Cmd+F focuses Find while the preview has focus."));
        for (unsigned i = 0; i < 3; i++) {
            bool matched = toolbar_contains(toolbar_document[i], query);
            cui_begin_box(ui, 0,
                          (cui_box){.width = width - 48,
                                    .fixed_width = true,
                                    .padding = 6,
                                    .background = matched ? t.secondary : t.surface});
            char line[160];
            snprintf(line, sizeof line, "%s%s", matched ? "Match: " : "", toolbar_document[i]);
            cui_label(ui, &t, cui_string(line));
            cui_end_box(ui);
        }
        break;
    }
    case 69: {
        cui_chrome_item items[] = {
            {.id = cui_id_from(ui, "title-workspace"), .text = cui_string("CUI workspace")},
            {.id = cui_id_from(ui, "title-document"), .text = cui_string("Untitled document")}};
        cui_begin_box(ui, cui_id_from(ui, "title-simulation-wrapper"),
                      (cui_box){.width = width - 48, .height = 34});
        cui_title_bar(ui, cui_id_from(ui, "title-preview"), &t, &title_state, items, 2,
                      (cui_title_bar_options){.width = width - 48,
                                              .name = cui_string("Window title bar"),
                                              .host = &title_host,
                                              .on_close = title_override ? title_close : NULL});
        if (title_decoration == CUI_WINDOW_NATIVE_TRAFFIC_LIGHTS && !title_fullscreen)
            title_simulated_controls(ui, &t, disabled != 0);
        cui_end_box(ui);
        cui_label(
            ui, &t,
            cui_string(
                title_decoration == CUI_WINDOW_NATIVE_TRAFFIC_LIGHTS
                    ? title_fullscreen
                          ? "Browser simulation: native controls hidden in fullscreen."
                          : "Browser simulation of OS traffic lights; actions affect this preview."
                    : "Window actions are simulated in this browser preview."));
        char status[160];
        snprintf(status, sizeof status,
                 "Preview: %s; minimized %u; close requests %u; overrides %u",
                 title_maximized ? "maximized" : "normal", title_actions[CUI_WINDOW_MINIMIZE],
                 title_actions[CUI_WINDOW_CLOSE], title_closed);
        cui_label(ui, &t, cui_string(status));
        break;
    }
    case 70: {
        cui_sidebar_entry entries[2] = {{.id = cui_id_from(ui, "sidebar-entry-1"),
                                         .render = sidebar_group_content,
                                         .user = &sidebar_group_values[0],
                                         .estimated_height = 300},
                                        {.id = cui_id_from(ui, "sidebar-entry-2"),
                                         .render = sidebar_group_content,
                                         .user = &sidebar_group_values[1],
                                         .estimated_height = 300}};
        cui_sidebar_options options = cui_sidebar_default();
        options.width = 250;
        options.height = height - 48;
        options.collapse = (cui_sidebar_collapse)sidebar_mode;
        options.side = (cui_sidebar_side)sidebar_side;
        options.collapsed = sidebar_collapsed != 0;
        options.reduced_motion = reduced_motion;
        options.now_seconds = host_seconds;
        options.header = sidebar_header_content;
        options.footer = sidebar_footer_content;
        options.label = cui_string("Workspace sidebar");
        cui_begin_box(ui, cui_id_from(ui, "sidebar-layout"),
                      (cui_box){.width = -1, .height = height - 48, .row = true});
        if (sidebar_side == CUI_SIDEBAR_RIGHT) {
            cui_begin_box(ui, cui_id_from(ui, "sidebar-main-right"),
                          (cui_box){.width = -1, .height = -1, .padding = 20});
            if (width >= 500)
                cui_label(ui, &t, cui_string("Project workspace"));
            cui_end_box(ui);
        }
        cui_sidebar(ui, cui_id_from(ui, "sidebar-preview"), &t, &sidebar_state, entries, 2,
                    options);
        if (sidebar_side == CUI_SIDEBAR_LEFT) {
            cui_begin_box(ui, cui_id_from(ui, "sidebar-main-left"),
                          (cui_box){.width = -1, .height = -1, .padding = 20});
            if (width >= 500)
                cui_label(ui, &t, cui_string("Project workspace"));
            cui_end_box(ui);
        }
        cui_end_box(ui);
        break;
    }
    case 71: {
        cui_label(ui, &t, cui_string("Project notes"));
        textarea_result = cui_textarea(
            ui, cui_id_from(ui, "textarea-preview"), &t, cui_string("Project notes"), textarea_edit,
            &textarea_state,
            (cui_textarea_options){.size = (cui_textarea_size)gallery_xsmall_second_size(),
                                   .design_size = design_size,
                                   .width = width > 430 ? 380 : width - 48,
                                   .rows = (size_t)textarea_rows,
                                   .auto_grow = textarea_auto_grow != 0,
                                   .max_rows = 8,
                                   .no_wrap = textarea_no_wrap != 0,
                                   .readonly = textarea_readonly != 0,
                                   .disabled = disabled != 0,
                                   .placeholder = cui_string("Write project notes")});
        break;
    }
    case 72: {
        cui_design_metrics metrics;
        float font = design_size != CUI_DESIGN_INHERIT &&
                             cui_design_measure(design_size, t.font_size / 16, &metrics)
                         ? metrics.font_size
                         : t.font_size;
        float required =
            fmaxf(cui_measure_text(ui, cui_string("Preferences"), font).width,
                  cui_measure_text(ui, cui_string("Receive updates"), font * .875f).width);
        float inner_width = fmaxf(width, required + 48 + font * 4);
        bool pan = inner_width > width + 1;
        cui_id pan_id = cui_id_from(ui, "settings-horizontal-preview");
        cui_scroll_options pan_options = {.width = width - 48,
                                          .height = height - 48,
                                          .content_width = inner_width - 48,
                                          .content_height = height - 48,
                                          .horizontal = true,
                                          .wheel_step = 32,
                                          .bar_thickness = 8};
        if (pan && !cui_scrollable_begin(ui, pan_id, &t, &settings_preview_scroll, pan_options))
            break;
        cui_choice_item modes[] = {
            {cui_id_from(ui, "settings-standard"), cui_string("Standard"), false},
            {cui_id_from(ui, "settings-focused"), cui_string("Focused"), false},
            {cui_id_from(ui, "settings-minimal"), cui_string("Minimal"), false}};
        cui_setting_item general[] = {
            {.title = cui_string("Email alerts"),
             .description = cui_string("Receive updates by email"),
             .kind = CUI_SETTING_CHECKBOX,
             .boolean = &settings_email,
             .has_default = true,
             .default_boolean = false},
            {.title = cui_string("Desktop alerts"),
             .description = cui_string("Show workspace notifications"),
             .kind = CUI_SETTING_SWITCH,
             .boolean = &settings_desktop,
             .has_default = true,
             .default_boolean = false},
            {.title = cui_string("Display name"),
             .kind = CUI_SETTING_INPUT,
             .edit = settings_name_edit,
             .input_state = &settings_name_state,
             .has_default = true,
             .default_text = cui_string("Ada")},
            {.title = cui_string("Refresh minutes"),
             .kind = CUI_SETTING_NUMBER,
             .number = &settings_refresh,
             .number_state = &settings_number_state,
             .number_options =
                 {.step = 1, .minimum = 1, .maximum = 60, .has_minimum = true, .has_maximum = true},
             .has_default = true,
             .default_number = 10},
            {.title = cui_string("Workspace mode"),
             .kind = CUI_SETTING_DROPDOWN,
             .choices = modes,
             .choice_count = 3,
             .choice = &settings_mode,
             .dropdown_state = &settings_mode_menu,
             .has_default = true,
             .default_choice = modes[0].id}};
        cui_setting_item privacy[] = {{.title = cui_string("Usage metrics"),
                                       .description = cui_string("Share anonymous statistics"),
                                       .kind = CUI_SETTING_CHECKBOX,
                                       .boolean = &settings_usage,
                                       .has_default = true,
                                       .default_boolean = false}};
        cui_setting_group groups[] = {
            {.title = cui_string("General"), .items = general, .count = 2},
            {.title = cui_string("Profile"), .items = general + 2, .count = 3}};
        cui_setting_group privacy_group = {
            .title = cui_string("Privacy"), .items = privacy, .count = 1};
        cui_setting_page pages[] = {{.title = cui_string("Preferences"),
                                     .icon = cui_string("settings"),
                                     .groups = groups,
                                     .count = 2,
                                     .default_open = true},
                                    {.title = cui_string("Advanced"),
                                     .icon = cui_string("shield"),
                                     .groups = &privacy_group,
                                     .count = 1}};
        cui_settings_options options = cui_settings_default();
        options.width = inner_width - 48;
        options.height = height - 48;
        options.sidebar_width = options.width < 400 ? 160 : 220;
        options.size = (cui_settings_size)settings_size;
        options.design_size = design_size;
        options.variant = (cui_settings_variant)settings_variant;
        settings_result = cui_settings(ui, cui_id_from(ui, "settings-preview"), &t, pages, 2,
                                       &settings_state, options);
        if (pan)
            cui_scrollable_end(ui, pan_id, &t, &settings_preview_scroll, pan_options);
        break;
    }
    case 73: {
        cui_carousel_set_axis(&carousel_state, carousel_axis ? CUI_ORIENTATION_VERTICAL
                                                             : CUI_ORIENTATION_HORIZONTAL);
        cui_carousel_set_looping(&carousel_state, carousel_looping != 0);
        static const char *labels[] = {"Build slide", "Preview slide", "Test slide", "Ship slide"};
        cui_carousel_item items[4];
        for (size_t i = 0; i < 4; i++)
            items[i] = (cui_carousel_item){.id = cui_id_from(ui, labels[i]),
                                           .label = cui_string(labels[i]),
                                           .render = gallery_carousel_item};
        cui_begin_box(ui, cui_id_from(ui, "carousel-margin"),
                      (cui_box){.width = -1, .padding = 48});
        cui_carousel_options options = cui_carousel_default();
        options.width = width - 144;
        options.height = 200;
        options.label = cui_string("Workflow carousel");
        options.now_seconds = host_seconds;
        options.reduced_motion = reduced_motion;
        options.disabled = disabled != 0;
        options.controls = gallery_carousel_controls;
        options.pagination = gallery_carousel_pagination;
        cui_carousel(ui, cui_id_from(ui, "carousel-preview"), &t, &carousel_state, items, 4,
                     options);
        cui_end_box(ui);
        break;
    }
    case 74: {
        cui_command_item workspace[] = {
            {.label = cui_string("Switch workspace"), .icon = cui_string("folder")},
            {.label = cui_string("Copy path"), .icon = cui_string("copy")}};
        cui_command_entry entries[] = {
            {.kind = CUI_COMMAND_ITEM,
             .item = {.label = cui_string("Open file"),
                      .icon = cui_string("folder-open"),
                      .action = gallery_command_action,
                      .binding = gallery_command_binding}},
            {.kind = CUI_COMMAND_ITEM,
             .item = {.label = cui_string("Save file"),
                      .icon = cui_string("save"),
                      .checked = true}},
            {.kind = CUI_COMMAND_ITEM,
             .item = {.label = cui_string("Disabled action"), .disabled = true}},
            {.kind = CUI_COMMAND_SEPARATOR},
            {.kind = CUI_COMMAND_GROUP,
             .heading = cui_string("Workspace"),
             .items = workspace,
             .item_count = 2}};
        cui_command_options options = cui_command_default();
        options.width = fminf(width - 64, 520);
        options.max_height = 260;
        options.label = cui_string("Command palette");
        options.on_confirm = gallery_command_confirm;
        options.on_cancel = gallery_command_cancel;
        options.filterable = command_filterable != 0;
        options.disabled = disabled != 0;
        command_state.loading = command_loading != 0;
        if (command_modal) {
            cui_id trigger = cui_id_from(ui, "command-trigger");
            if (cui_button(ui, trigger, &t, cui_string("Open command palette"), disabled)) {
                cui_surface_open(ui, &command_surface, trigger);
                command_focus_pending = true;
            }
            if (cui_surface_begin(ui, &t, &command_surface, cui_string("Command palette"),
                                  cui_string("Search workspace actions."),
                                  (cui_surface_options){.viewport = {0, 0, width, height},
                                                        .width = fminf(width - 32, 600),
                                                        .height = 400,
                                                        .margin = 16,
                                                        .no_escape_dismiss = true,
                                                        .hide_close = true})) {
                options.width = -1;
                cui_command(ui, cui_id_from(ui, "command-preview"), &t, &command_state, entries,
                            sizeof(entries) / sizeof(entries[0]), options);
                if (command_focus_pending) {
                    cui_command_focus(ui, cui_id_from(ui, "command-preview"), options);
                    command_focus_pending = false;
                }
                cui_surface_end(ui);
            }
        } else {
            cui_command(ui, cui_id_from(ui, "command-preview"), &t, &command_state, entries,
                        sizeof(entries) / sizeof(entries[0]), options);
        }
        break;
    }
    case 75:
        root_disabled = disabled != 0;
        (void)cui_root_render(ui, cui_id_from(ui, "root-view"), &root_view, &t,
                              (cui_rect){0, 0, width, height}, NULL, gallery_root_content, NULL,
                              host_seconds);
        break;
    case 76: {
        if (cui_button(ui, cui_id_from(ui, "text-view-append"), &t, cui_string("Append update"),
                       disabled)) {
            size_t length = strlen(text_view_document);
            const char *suffix = text_view_format == CUI_TEXT_VIEW_HTML
                                     ? "<p>Update: review completed.</p>"
                                     : "\n\nUpdate: review completed.";
            size_t extra = strlen(suffix);
            if (length + extra < sizeof(text_view_document)) {
                memcpy(text_view_document + length, suffix, extra + 1);
                text_view_error =
                    cui_text_view_set_text(&text_view_state, cui_string(text_view_document));
            }
        }
        cui_text_view_options options = cui_text_view_default();
        options.width = width - 48;
        options.height = text_view_scrollable ? 320 : -1;
        options.dt = host_dt;
        options.scrollable = text_view_scrollable != 0;
        options.stream_fade = text_view_fade != 0;
        options.custom_motion = true;
        options.motion = text_view_motion;
        options.reduced_motion = reduced_motion;
        options.disabled = disabled != 0;
        options.code_actions_height = 0;
        options.table_actions_height = 0;
        options.on_link = gallery_text_view_link;
        (void)cui_text_view(ui, cui_id_from(ui, "text-view-preview"), &t, &text_view_state,
                            options);
        break;
    }
    case 77: {
        static const cui_chart_datum data[] = {
            {.label = {"Jan", 3}, .value = 28, .open = 21, .high = 34, .low = 18, .close = 28},
            {.label = {"Feb", 3}, .value = 36, .open = 28, .high = 40, .low = 24, .close = 36},
            {.label = {"Mar", 3}, .value = 31, .open = 36, .high = 42, .low = 27, .close = 31},
            {.label = {"Apr", 3}, .value = 46, .open = 31, .high = 50, .low = 29, .close = 46},
            {.label = {"May", 3}, .value = 39, .open = 46, .high = 49, .low = 35, .close = 39},
            {.label = {"Jun", 3}, .value = 53, .open = 39, .high = 57, .low = 37, .close = 53}};
        static const cui_chart_link links[] = {{0, 2, 16, {0}}, {0, 3, 12, {0}}, {1, 3, 18, {0}},
                                               {2, 4, 16, {0}}, {3, 5, 30, {0}}, {4, 5, 16, {0}}};
        float logical_width = fminf(width - 48, 720);
        cui_chart_options options = {
            .kind = (cui_chart_kind)chart_kind,
            .interaction_mode = CUI_PLOT_INTERACTION_CURRENT_FRAME,
            .layout_mode = (chart_kind == CUI_CHART_LINE || chart_kind == CUI_CHART_AREA)
                               ? CUI_CHART_LAYOUT_SOURCE_POINT
                               : CUI_CHART_LAYOUT_LEGACY,
            .data = data,
            .count = sizeof(data) / sizeof(data[0]),
            .links = links,
            .link_count = sizeof(links) / sizeof(links[0]),
            .width = (unsigned)(logical_width * 2),
            .height = 560,
            .logical_width = logical_width,
            .logical_height = 280,
            .data_revision = chart_revision,
            .appear_key = chart_revision,
            .disable_appear = !chart_animate,
            .reduced_motion = reduced_motion,
            .curve = (cui_chart_curve)chart_curve,
            .show_dots = chart_dots != 0,
            .show_bar_values = chart_labels != 0,
            .pie_show_labels = chart_labels != 0,
            .sankey_show_labels = chart_labels != 0,
            .sankey_show_values = chart_labels != 0,
            .inner_radius = chart_labels ? 42 : 0};
        options = cui_chart_theme(options, &t);
        options.tooltip_build = chart_custom_rows ? gallery_chart_tooltip : NULL;
        cui_chart_tooltip_style tooltip_style;
        if (chart_tooltip_appearance) {
            float line_height = roundf(.75f * t.font_size * 1.618034f);
            if (!cui_chart_tooltip_style_project(&t, t.font_size, line_height, &tooltip_style)) {
                chart_result = CUI_PLOT_INVALID;
                break;
            }
            tooltip_style.appearance =
                chart_tooltip_appearance == 2 ? CUI_CHART_TOOLTIP_BARE : CUI_CHART_TOOLTIP_BOXED;
            options.tooltip_style = &tooltip_style;
        }
        chart_result = cui_chart_render_animated(ui, &chart_state, &options);
        if (chart_result == CUI_PLOT_OK)
            (void)cui_chart(ui, cui_id_from(ui, "chart-preview"), &chart_state, &options,
                            cui_string("Project activity chart"));
        break;
    }
    case 78: {
        (void)cui_speech_pump(&speech_state, host_seconds);
        cui_label(ui, &t, cui_string("Scripted provider demo — no microphone access"));
        (void)cui_speech_button(ui, cui_id_from(ui, "speech-toggle"), &t, &speech_state,
                                (cui_speech_button_options){.size = (cui_speech_size)speech_size,
                                                            .design_size = design_size,
                                                            .disabled = disabled != 0,
                                                            .show_when_unsupported = true},
                                host_seconds);
        (void)cui_speech_waveform(
            ui, cui_id_from(ui, "speech-waveform"), &t, &speech_state,
            (cui_speech_waveform_options){.size = (cui_speech_size)speech_size,
                                          .design_size = design_size,
                                          .width = 260,
                                          .reduced_motion = reduced_motion},
            host_seconds);
        cui_begin_box(ui, cui_id_from(ui, "speech-actions-a"), (cui_box){.row = true, .gap = 8});
        if (cui_button(ui, cui_id_from(ui, "speech-interim"), &t, cui_string("Emit interim"),
                       disabled || speech_state.status != CUI_SPEECH_RECORDING)) {
            (void)cui_speech_report_hypothesis(speech_sink, cui_string("draft notes"));
            (void)cui_speech_pump(&speech_state, host_seconds);
            redraw = true;
        }
        if (cui_button(ui, cui_id_from(ui, "speech-phrase"), &t, cui_string("Emit phrase"),
                       disabled || speech_state.status != CUI_SPEECH_RECORDING)) {
            (void)cui_speech_report_phrase(speech_sink, cui_string("Meeting notes "));
            (void)cui_speech_pump(&speech_state, host_seconds);
            redraw = true;
        }
        if (cui_button(ui, cui_id_from(ui, "speech-audio"), &t, cui_string("Emit PCM"),
                       disabled || speech_state.status != CUI_SPEECH_RECORDING)) {
            int16_t pcm[1280];
            for (size_t i = 0; i < 1280; ++i)
                pcm[i] = (i / 160) % 2 ? 12000 : 6000;
            (void)cui_speech_report_audio(speech_sink, pcm, 1280, host_seconds);
            (void)cui_speech_pump(&speech_state, host_seconds);
            redraw = true;
        }
        cui_end_box(ui);
        cui_begin_box(ui, cui_id_from(ui, "speech-actions-b"), (cui_box){.row = true, .gap = 8});
        if (cui_button(ui, cui_id_from(ui, "speech-final"), &t, cui_string("Deliver final"),
                       disabled || speech_state.status != CUI_SPEECH_STOPPING)) {
            (void)cui_speech_report_finish(speech_sink);
            (void)cui_speech_pump(&speech_state, host_seconds);
            redraw = true;
        }
        if (cui_button(ui, cui_id_from(ui, "speech-cancel"), &t, cui_string("Cancel session"),
                       disabled || speech_state.status == CUI_SPEECH_IDLE)) {
            (void)cui_speech_cancel(&speech_state);
            redraw = true;
        }
        if (cui_button(ui, cui_id_from(ui, "speech-error"), &t, cui_string("Report error"),
                       disabled || speech_state.status == CUI_SPEECH_IDLE)) {
            (void)cui_speech_report_error(speech_sink, CUI_SPEECH_INPUT_ERROR);
            (void)cui_speech_pump(&speech_state, host_seconds);
            redraw = true;
        }
        cui_end_box(ui);
        cui_str spoken = cui_speech_transcript(&speech_state);
        cui_label(ui, &t, spoken.length ? spoken : cui_string("Transcript is empty"));
        break;
    }
    case 79: {
        cui_questionnaire_options options = {.size = (cui_questionnaire_size)questionnaire_size,
                                             .design_size = design_size,
                                             .width = width > 520 ? 480 : width - 40,
                                             .disabled = disabled,
                                             .numeric_shortcuts = questionnaire_numbers != 0};
        if (questionnaire_custom) {
            options.render_indicator = questionnaire_indicator;
            options.render_content = questionnaire_content;
            options.render_shortcut = questionnaire_shortcut;
            options.render_user = &t;
        }
        cui_id root_id = cui_id_from(ui, "questionnaire-root");
        size_t item = questionnaire_state.current;
        if (cui_questionnaire_begin(ui, root_id, &t, &questionnaire_state, options)) {
            cui_questionnaire_progress(ui, cui_id_from(ui, "questionnaire-progress"), &t,
                                       &questionnaire_state, options);
            if (cui_questionnaire_item_begin(ui, cui_id_from(ui, "questionnaire-item"),
                                             &questionnaire_state, item, options)) {
                cui_questionnaire_heading(ui, cui_id_from(ui, "questionnaire-heading"), &t,
                                          &questionnaire_state, item, options);
                cui_id choice_ids[3] = {cui_id_from(ui, "questionnaire-build"),
                                        cui_id_from(ui, "questionnaire-explore"),
                                        cui_id_from(ui, "questionnaire-prototype")};
                cui_id input_id = cui_id_from(ui, "questionnaire-input");
                size_t count = questionnaire_items[item].choice_count;
                (void)cui_questionnaire_confirm_keys(ui, root_id, &questionnaire_state, choice_ids,
                                                     count, input_id, CUI_MOD_CTRL);
                (void)cui_questionnaire_radio_keys(ui, &questionnaire_state, choice_ids, count);
                (void)cui_questionnaire_answer_keys(ui, root_id, &questionnaire_state, choice_ids,
                                                    count, input_id);
                (void)cui_questionnaire_shortcut_keys(ui, root_id, &questionnaire_state, choice_ids,
                                                      count, input_id, options.numeric_shortcuts);
                if (cui_questionnaire_choices_begin(ui, cui_id_from(ui, "questionnaire-choices"),
                                                    &questionnaire_state, item, options)) {
                    for (size_t choice = 0; choice < count; choice++)
                        (void)cui_questionnaire_choice_control(ui, choice_ids[choice], &t,
                                                               &questionnaire_state, item, choice,
                                                               options);
                    cui_questionnaire_part_end(ui);
                }
                if (questionnaire_items[item].input)
                    (void)cui_questionnaire_input_control(ui, input_id, &t, &questionnaire_state,
                                                          item, &questionnaire_input_view, NULL,
                                                          options);
                cui_questionnaire_error_control(ui, cui_id_from(ui, "questionnaire-error"), &t,
                                                &questionnaire_state, item, options);
                cui_questionnaire_part_end(ui);
            }
            if (cui_questionnaire_actions_begin(ui, cui_id_from(ui, "questionnaire-actions"),
                                                &questionnaire_state, options)) {
                (void)cui_questionnaire_action_control(
                    ui, cui_id_from(ui, "questionnaire-previous"), &t, &questionnaire_state,
                    CUI_QUESTIONNAIRE_PREVIOUS, (cui_str){0}, options);
                (void)cui_questionnaire_action_control(
                    ui, cui_id_from(ui, "questionnaire-skip"), &t, &questionnaire_state,
                    CUI_QUESTIONNAIRE_SKIP, (cui_str){0}, options);
                (void)cui_questionnaire_action_control(
                    ui, cui_id_from(ui, "questionnaire-next"), &t, &questionnaire_state,
                    CUI_QUESTIONNAIRE_NEXT, (cui_str){0}, options);
                (void)cui_questionnaire_action_control(
                    ui, cui_id_from(ui, "questionnaire-finish"), &t, &questionnaire_state,
                    CUI_QUESTIONNAIRE_FINISH, (cui_str){0}, options);
                cui_questionnaire_part_end(ui);
            }
            cui_questionnaire_end(ui);
        }
        if (cui_button_ex(ui, cui_id_from(ui, "questionnaire-reset"), &t, cui_string("Start over"),
                          (cui_button_options){.disabled = disabled})) {
            (void)cui_questionnaire_reset(&questionnaire_state);
            redraw = true;
        }
        if (questionnaire_state.complete)
            cui_label(ui, &t, cui_string("Responses submitted"));
        break;
    }
    case 0: {
        char label[64];
        if (button_collection) {
            cui_button_group_styled_item items[] = {
                {.id = cui_id_from(ui, "collection-day"), .label = cui_string("Day")},
                {.id = cui_id_from(ui, "collection-week"), .label = cui_string("Week")},
                {.id = cui_id_from(ui, "collection-month"), .label = cui_string("Month")},
            };
            for (size_t i = 0; i < 3; ++i) {
                items[i].options.custom_cursor = true;
                items[i].options.cursor = CUI_CURSOR_POINTER;
            }
            cui_button_group_ex(
                ui, cui_id_from(ui, "button-collection"), &t, items, button_collection_selected, 3,
                &button_collection_workspace,
                (cui_button_group_style_options){
                    .behavior = {.multiple = button_collection == 2,
                                 .vertical = button_collection == 3,
                                 .disabled = disabled,
                                 .activation_policy = CUI_BUTTON_GROUP_ALL_ACTIVATIONS,
                                 .accessibility_label = cui_string("Calendar view"),
                                 .on_change = gallery_button_collection_change},
                    .override_size = true,
                    .size = (cui_control_size)control_size,
                    .design_size = design_size,
                    .override_variant = true,
                    .variant = CUI_BUTTON_OUTLINE},
                NULL);
            snprintf(label, sizeof label, "Selection callbacks: %u", button_collection_requests);
            cui_label(ui, &t, cui_string(label));
            break;
        }
        cui_button_ex(ui, cui_id_from(ui, "button"), &t, cui_string("Run action"),
                      (cui_button_options){.size = (cui_control_size)control_size,
                                           .design_size = design_size,
                                           .disabled = disabled,
                                           .on_click = gallery_button_action,
                                           .prevent_pointer_focus = true,
                                           .custom_cursor = true,
                                           .cursor = CUI_CURSOR_POINTER});
        snprintf(label, sizeof(label), "Clicked %d times", clicks);
        cui_label(ui, &t, cui_string(label));
        int columns = design_size == CUI_DESIGN_INHERIT ? 3 : width < 600 ? 1 : 2;
        for (int row = 0; row < (12 + columns - 1) / columns; row++) {
            cui_begin_box(ui, cui_id_from(ui, "variant-row") ^ (cui_id)(row + 1),
                          (cui_box){.row = true, .gap = design_spacing >= 0 ? t.gap : 8});
            const char *names[12] = {"Secondary", "Outline", "Ghost",   "Danger", "Link", "Loading",
                                     "Default",   "Warning", "Success", "Info",   "Text", "Custom"};
            cui_button_variant variants[12] = {
                CUI_BUTTON_SECONDARY, CUI_BUTTON_OUTLINE, CUI_BUTTON_GHOST,   CUI_BUTTON_DANGER,
                CUI_BUTTON_LINK,      CUI_BUTTON_PRIMARY, CUI_BUTTON_DEFAULT, CUI_BUTTON_WARNING,
                CUI_BUTTON_SUCCESS,   CUI_BUTTON_INFO,    CUI_BUTTON_TEXT,    CUI_BUTTON_CUSTOM};
            for (int column = 0; column < columns; column++) {
                int v = row * columns + column;
                if (v >= 12)
                    break;
                cui_button_ex(ui, cui_id_from(ui, names[v]), &t, cui_string(names[v]),
                              (cui_button_options){.variant = variants[v],
                                                   .size = CUI_CONTROL_SMALL,
                                                   .design_size = design_size,
                                                   .disabled = disabled || v == 2,
                                                   .loading = v == 5,
                                                   .custom_colors = v == 11,
                                                   .colors = t.buttons[CUI_THEME_BUTTON_INFO]});
            }
            cui_end_box(ui);
        }
        cui_button_options extra = {.size = (cui_control_size)control_size,
                                    .design_size = design_size,
                                    .disabled = disabled};
        extra.icon = cui_string("check");
        extra.trailing_icon = cui_string("arrow-right");
        extra.dropdown_caret = true;
        cui_button_ex(ui, cui_id_from(ui, "button-icons"), &t, cui_string("With icon"), extra);
        extra.icon = (cui_str){0};
        extra.trailing_icon = (cui_str){0};
        extra.dropdown_caret = false;
        extra.children = gallery_button_badge;
        extra.children_user = &t;
        cui_button_ex(ui, cui_id_from(ui, "button-children"), &t, cui_string("Custom content"),
                      extra);
        extra.children = NULL;
        extra.width = fminf(width - 48, 340);
        extra.label_width = fmaxf(1, extra.width - 48);
        extra.state = &gallery_button_label;
        cui_button_ex(ui, cui_id_from(ui, "button-truncated"), &t,
                      cui_string("A long accessible action label with bounded ellipsis"), extra);
        break;
    }
    case 1: {
        cui_design_metrics metrics;
        float font = t.font_size * .875f, indicator = t.font_size;
        if (design_size && cui_design_measure(design_size, t.font_size / 16, &metrics)) {
            font = metrics.font_size;
            indicator = metrics.indicator_size;
        }
        float control_width = fmaxf(width - 48, indicator + 8 * t.font_size / 16 + 8 * font);
        cui_id owner = cui_id_from(ui, "checkbox-label-scroll");
        cui_scroll_options scroll = {.width = width - 48,
                                     .height = height - 48,
                                     .content_width = control_width,
                                     .horizontal = true,
                                     .hide_scrollbars = true};
        bool began = cui_scrollable_begin(ui, owner, &t, &checkbox_scroll, scroll);
        cui_checkbox_ex(
            ui, cui_id_from(ui, "check"), &t, cui_string("Receive updates"), &check_state,
            (cui_checkbox_options){.size = (cui_control_size)control_size,
                                   .design_size = design_size,
                                   .disabled = disabled,
                                   .required = choice_required != 0,
                                   .invalid = choice_invalid != 0,
                                   .skip_tab = choice_skip_tab != 0,
                                   .state = &checkbox_motion,
                                   .width = control_width,
                                   .content = gallery_checkbox_detail,
                                   .content_user = &t,
                                   .tooltip = cui_string("Change email notification preferences"),
                                   .tooltip_options = {.viewport = {0, 0, width, height},
                                                       .width = fminf(300, width - 48),
                                                       .height = 80,
                                                       .open_delay = .2f,
                                                       .close_delay = .1f},
                                   .reduced_motion = reduced_motion});
        if (began)
            cui_scrollable_end_ex(ui, owner, &t, &checkbox_scroll, scroll);
        next_wake =
            checkbox_motion.next_wake_seconds > 0
                ? host_seconds + fmax(0, checkbox_motion.next_wake_seconds - cui_frame_time(ui))
                : 0;
        break;
    }
    case 2: {
        cui_switch_options options = cui_switch_options_default();
        options.size = (cui_control_size)control_size;
        options.design_size = design_size;
        options.disabled = disabled;
        options.label_left = switch_label_left != 0;
        options.custom_color = switch_custom_color != 0;
        options.color = (cui_color){180, 74, 44, 255};
        options.now_seconds = host_seconds;
        options.reduced_motion = reduced_motion;
        cui_switch_ex(ui, cui_id_from(ui, "switch"), &t, cui_string("Notifications"), &enabled,
                      &gallery_switch_state, options);
        next_wake = gallery_switch_state.next_wake_seconds;
        break;
    }
    case 3: {
        cui_toggle_options standalone = cui_toggle_options_default();
        standalone.size = (cui_control_size)control_size;
        standalone.design_size = design_size;
        standalone.variant = (cui_toggle_variant)toggle_variant;
        standalone.disabled = disabled;
        standalone.focus_ring = true;
        cui_toggle_ex(ui, cui_id_from(ui, "toggle"), &t, cui_string("Pin this item"), &toggled,
                      NULL, standalone);
        cui_toggle_item items[3] = {
            {.id = cui_id_from(ui, "toggle-bold"), .label = cui_string("Bold")},
            {.id = cui_id_from(ui, "toggle-italic"), .label = cui_string("Italic")},
            {.id = cui_id_from(ui, "toggle-underline"), .label = cui_string("Underline")}};
        cui_toggle_group_style_options group = {.size = (cui_control_size)control_size,
                                                .design_size = design_size,
                                                .variant = (cui_toggle_variant)toggle_variant,
                                                .disabled = disabled,
                                                .segmented = toggle_segmented != 0,
                                                .accessibility_label =
                                                    cui_string("Formatting options"),
                                                .on_change = gallery_toggle_group_change,
                                                .requested_values = toggle_group_requested,
                                                .requested_capacity = 3};
        cui_toggle_group_ex(ui, cui_id_from(ui, "toggle-group"), &t, items, toggle_group_values, 3,
                            group);
        break;
    }
    case 4: {
        cui_radio_group_item items[] = {
            {cui_id_from(ui, "radio-a"), cui_string("Standard"), false},
            {cui_id_from(ui, "radio-b"), cui_string("Priority"), false},
            {cui_id_from(ui, "radio-c"), cui_string("Express unavailable"), true}};
        if (!radio_selected)
            radio_selected = items[0].id;
        cui_radio_group(ui, cui_id_from(ui, "radio-group"), &t, items, 3, &radio_selected,
                        &radio_group_state,
                        (cui_radio_group_options){.name = cui_string("Delivery options"),
                                                  .vertical = true,
                                                  .size = (cui_control_size)control_size,
                                                  .design_size = design_size,
                                                  .disabled = disabled,
                                                  .required = choice_required != 0,
                                                  .invalid = choice_invalid != 0,
                                                  .skip_tab = choice_skip_tab != 0,
                                                  .on_change = gallery_radio_change});
        break;
    }
    case 5:
        cui_slider(ui, cui_id_from(ui, "slider"), &t, cui_string("Volume"), &slider, 0, 100, 1,
                   disabled);
        break;
    case 6: {
        cui_input_result result = cui_input_ex(
            ui, cui_id_from(ui, "input"), &t,
            cui_string(input_mode == 7   ? "Phone number"
                       : input_mode == 8 ? "Amount"
                       : input_mode == 9 ? "Unicode name"
                       : input_mode >= 4 ? "Password"
                                         : "Display name"),
            edit, &input_state,
            (cui_input_options){
                .disabled = disabled,
                .masked = input_mode == 4 || input_mode == 6,
                .content_type = input_mode == 6 ? CUI_TEXT_INPUT_PURPOSE_NEW_PASSWORD
                                : input_mode >= 4 && input_mode <= 6
                                    ? CUI_TEXT_INPUT_PURPOSE_PASSWORD
                                    : CUI_TEXT_INPUT_PURPOSE_UNSPECIFIED,
                .format = input_mode == 7   ? (cui_edit_format){.kind = CUI_EDIT_FORMAT_PATTERN,
                                                                .pattern = {"(999) 999-9999", 14}}
                          : input_mode == 8 ? (cui_edit_format){.kind = CUI_EDIT_FORMAT_NUMBER,
                                                                .has_separator = true,
                                                                .separator = ',',
                                                                .fraction_limited = true,
                                                                .fraction_digits = 2}
                                            : (cui_edit_format){0},
                .format_workspace = input_mode == 7 || input_mode == 8
                                        ? (cui_edit_workspace){input_format_workspace,
                                                               sizeof input_format_workspace}
                                        : (cui_edit_workspace){0},
                .readonly = input_mode == 1,
                .validate = input_mode == 3 ? gallery_name_acceptance : NULL,
                .matcher = input_mode == 9 ? gallery_input_regex_matcher() : (cui_text_match){0},
                .normalization = input_mode == 3 || input_mode == 9
                                     ? CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF
                                     : CUI_EDIT_NORMALIZE_NONE,
                .required = input_mode == 2,
                .invalid = input_mode == 2,
                .size = (cui_input_size)control_size,
                .design_size = design_size,
                .width = width > 360 ? 280 : width - 48});
        if (result.rejected)
            input_rejected = true;
        else if (result.changed)
            input_rejected = false;
        if (input_mode == 3)
            cui_label(ui, &t,
                      cui_string(input_rejected
                                     ? "Number rejected; your name is unchanged."
                                     : "Names can contain Unicode text, but no digits."));
        if (input_mode == 9)
            cui_label(ui, &t,
                      cui_string(input_rejected
                                     ? "Regex rejected; your name is unchanged."
                                     : "Unicode letters and spaces only · real regex provider."));
        if (input_mode == 7 || input_mode == 8)
            cui_label(ui, &t,
                      cui_string(input_mode == 7 ? "Select all and type 4155550123: (415) 555-0123."
                                                 : "Select all and type 12345.678: 12,345.67."));
        break;
    }
    case 60: {
        message_demo_disabled = disabled != 0;
        cui_message_body body = {
            .slot = {.present = true, .text = cui_string("Can you review the release notes?")},
            .typed_bubble = true,
            .bubble = (cui_bubble_variant)message_variant};
        cui_message_options options = cui_message_options_default();
        options.width = width - 48;
        options.max_bubble_width = fminf(280, width - 88);
        options.alignment = (cui_message_alignment)message_alignment;
        options.role = CUI_ROLE_LISTITEM;
        options.name = cui_string("Workspace conversation");
        options.avatar =
            (cui_message_slot){.present = message_avatar,
                               .width = 32,
                               .height = 32,
                               .render = message_avatar ? gallery_message_avatar : NULL};
        options.header = (cui_message_slot){
            .present = true, .render = message_header, .inset = (cui_message_inset)message_inset};
        options.footer = (cui_message_slot){
            .present = true, .render = message_footer, .inset = (cui_message_inset)message_inset};
        options.body = &body;
        options.body_count = 1;
        cui_begin_message_group(ui, cui_id_from(ui, "message-group"), width - 48, 16);
        cui_message(ui, cui_id_from(ui, "message"), &t, options);
        cui_message_body reply = {
            .slot = {.present = true, .text = cui_string("Looks good. I'll ship the update.")},
            .typed_bubble = true,
            .bubble = CUI_BUBBLE_FILLED};
        options.alignment = message_alignment ? CUI_MESSAGE_START : CUI_MESSAGE_END;
        options.name = cui_string("Reply from Jamie Chen");
        options.avatar.user = (void *)(uintptr_t)1;
        options.header.user = (void *)(uintptr_t)1;
        options.body = &reply;
        options.footer = (cui_message_slot){.present = true,
                                            .text = cui_string("09:32 · Read"),
                                            .inset = (cui_message_inset)message_inset};
        cui_message(ui, cui_id_from(ui, "message-reply"), &t, options);
        cui_end_message_group(ui);
        if (renderer_fixture_enabled)
            gallery_paint_renderer_fixture(ui);
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
    case 62: {
        cui_image_options options = cui_image_options_default();
        options.geometry = (cui_image_geometry){280, 160, 0, (cui_image_fit)image_fit};
        options.corners = (cui_corner_radii){18, 4, 18, 4};
        options.grayscale = image_grayscale != 0;
        options.opacity = image_half_opacity ? .5f : 1;
        options.description = cui_string("Workspace preview image");
        options.loading = gallery_image_placeholder;
        options.fallback = gallery_image_placeholder;
        options.clock = (cui_image_clock){true, true, reduced_motion, host_seconds};
        image_result = cui_image(ui, cui_id_from(ui, "image-story"), &image_state,
                                 (cui_image_source){.kind = CUI_IMAGE_ASSET,
                                                    .name = cui_string("workspace-preview"),
                                                    .identity = (unsigned)image_mode + 1},
                                 gallery_image_loader, NULL, options);
        next_wake = image_result.next_wake_seconds;
        break;
    }
    case 63: {
        cui_attachment_group_options options = cui_attachment_group_options_default();
        options.scrollbar_theme = &t;
        options.width = width - 48;
        options.height = 0;
        options.gap = 12;
        options.edge_fade = true;
        options.now_seconds = host_seconds;
        options.reduced_motion = reduced_motion;
        options.children = gallery_attachment_cards;
        attachment_group_result = cui_attachment_group(ui, cui_id_from(ui, "attachment-group"),
                                                       &attachment_group_state, options);
        next_wake = attachment_group_result.next_wake_seconds;
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
                                   .size = (cui_status_size)gallery_xsmall_second_size(),
                                   .design_size = design_size,
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
                (cui_tag_options){
                    .variant = CUI_TAG_SUCCESS, .outline = true, .design_size = design_size});
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
            {.kind = CUI_MARKER_CONTENT,
             .width = cui_measure_text(ui, cui_string("3 tasks"), t.font_size).width,
             .render = marker_content}};
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
                                      .size = (cui_otp_size)gallery_xsmall_second_size(),
                                      .design_size = design_size,
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
        const cui_input_group_addon addons[] = {
            {.id = cui_id_from(ui, "group-prefix"),
             .alignment = CUI_INPUT_GROUP_INLINE_START,
             .label = cui_string("https://"),
             .width = 64},
            {.id = cui_id_from(ui, "group-suffix"),
             .alignment = CUI_INPUT_GROUP_INLINE_END,
             .label = cui_string(".example.com"),
             .width = 112},
            {.id = action,
             .alignment = CUI_INPUT_GROUP_INLINE_END,
             .kind = CUI_INPUT_GROUP_BUTTON,
             .variant = CUI_INPUT_GROUP_PRIMARY,
             .label = cui_string("Open address"),
             .width = 56,
             .render = group_mode == 3 ? NULL : gallery_group_action_content,
             .loading = group_mode == 3},
            {.id = cui_id_from(ui, "group-title"),
             .alignment = CUI_INPUT_GROUP_BLOCK_START,
             .label = cui_string("Workspace address"),
             .width = 190},
            {.id = cui_id_from(ui, "group-help"),
             .alignment = CUI_INPUT_GROUP_BLOCK_END,
             .label = cui_string("Click Open to continue"),
             .width = 260}};
        cui_input_group_result result = cui_input_group(
            ui, cui_id_from(ui, "address-group"), cui_id_from(ui, "address-input"), &t, group_edit,
            &group_state, addons, group_mode == 4 ? 5 : 3,
            (cui_input_group_options){
                .width = width > 460 ? 400 : width - 48,
                .size = (cui_form_size)gallery_xsmall_second_size(),
                .design_size = design_size,
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
                                    .size = (cui_form_size)gallery_xsmall_second_size(),
                                    .design_size = design_size,
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
        cui_label_source(ui, &t,
                         (cui_label_options){.primary = cui_string("Workspace"),
                                             .secondary = cui_string("Ready to collaborate"),
                                             .has_secondary = true,
                                             .semantic_id = cui_id_from(ui, "label-secondary")});
        cui_label_source(
            ui, &t,
            (cui_label_options){.primary = cui_string("Unicode highlight: CAFÉ and café"),
                                .highlight = cui_string("café"),
                                .semantic_id = cui_id_from(ui, "label-highlight")});
        cui_label_source(ui, &t,
                         (cui_label_options){.primary = cui_string("Private workspace"),
                                             .masked = true,
                                             .semantic_id = cui_id_from(ui, "label-masked")});
        break;
    case 8: {
        badge_disabled = disabled != 0;
        cui_label(ui, &t, cui_string("Counts, overflow, dot and SVG icon badges on any content."));
        const char *names[] = {"Inbox", "Alerts", "Online", "Profile", "Quiet"};
        for (unsigned i = 0; i < 5; i++) {
            cui_badge_options options = cui_badge_defaults();
            options.size = (cui_control_size)control_size;
            options.design_size = design_size;
            options.count = i == 0 ? badge_count : i == 1 ? 125 : 0;
            options.variant = i == 2 ? CUI_BADGE_DOT : i == 3 ? CUI_BADGE_ICON : CUI_BADGE_COUNT;
            options.icon = cui_string("check");
            cui_badge_ex(ui, cui_id_from(ui, names[i]), &t, options, gallery_badge_content,
                         (void *)names[i]);
        }
        break;
    }
    case 9: {
        cui_progress_options options = cui_progress_defaults();
        options.indeterminate = progress_indeterminate;
        options.reduced_motion = reduced_motion;
        options.now_seconds = host_seconds;
        options.size = (cui_control_size)control_size;
        options.design_size = design_size;
        options.motion = &progress_motion;
        if (progress_shape) {
            cui_progress_circle_options circle = cui_progress_circle_defaults();
            circle.progress = options;
            circle.diameter = progress_shape == 2 ? 64 : 0;
            if (design_size && progress_shape == 2) {
                cui_design_metrics metrics;
                if (cui_design_measure(design_size, t.font_size / 16, &metrics))
                    circle.diameter = 2 * metrics.control_height;
            }
            cui_progress_circle(ui, cui_id_from(ui, "progress"), &t, slider / 100,
                                &progress_circle_state, circle,
                                progress_shape == 2 ? gallery_progress_content : NULL, &slider);
        } else {
            cui_progress_ex(ui, cui_id_from(ui, "progress"), &t, slider / 100,
                            width > 360 ? 300 : width - 48, options);
        }
        cui_slider(ui, cui_id_from(ui, "progress-control"), &t, cui_string("Progress"), &slider, 0,
                   100, 1, disabled);
        break;
    }
    case 10:
        cui_label(ui, &t, cui_string("Before the separator"));
        cui_separator(ui, cui_id_from(ui, "separator"), &t, width - 48);
        cui_label(ui, &t, cui_string("After the separator"));
        break;
    case 11: {
        cui_label(ui, &t, cui_string("Themes are reusable design tokens"));
        cui_label(ui, &t,
                  cui_string("Use the palette button in the website header to switch themes."));
        const char *names[] = {"Background", "Surface", "Text", "Primary", "Border", "Focus"};
        cui_color colors[] = {t.background, t.surface, t.text, t.primary, t.border, t.focus};
        for (unsigned row = 0; row < 3; row++) {
            cui_push_id(ui, names[row * 2]);
            cui_begin_box(ui, cui_id_from(ui, "token-row"),
                          (cui_box){.row = true, .width = width - 48, .gap = 12});
            for (unsigned column = 0; column < 2; column++) {
                unsigned token = row * 2 + column;
                cui_push_id(ui, names[token]);
                cui_begin_box(ui, cui_id_from(ui, "token"),
                              (cui_box){.row = true,
                                        .width = (width - 60) / 2,
                                        .height = 28,
                                        .gap = 8,
                                        .align_y = CUI_ALIGN_CENTER});
                cui_begin_box(ui, cui_id_from(ui, token ? "token-swatch" : "background-swatch"),
                              (cui_box){.width = 24,
                                        .height = 24,
                                        .radius = t.radius,
                                        .background = colors[token],
                                        .border = t.border,
                                        .border_width = 1});
                cui_end_box(ui);
                cui_label(ui, &t, cui_string(names[token]));
                cui_end_box(ui);
                cui_pop_id(ui);
            }
            cui_end_box(ui);
            cui_pop_id(ui);
        }
        cui_label(ui, &t,
                  cui_string("The same controls inherit the colors, font size and corner radius."));
        cui_text_ex(ui, cui_string("Semibold heading"), 24, t.text, 30,
                    (cui_text_style){.weight = CUI_TEXT_WEIGHT_SEMIBOLD});
        cui_text_ex(ui, cui_string("Italic emphasis"), 14, t.muted, 20,
                    (cui_text_style){.italic = true});
        cui_text_ex(ui, cui_string("Monospace code"), 14, t.text, 20,
                    (cui_text_style){.monospace = true});
        cui_button(ui, cui_id_from(ui, "theme-button"), &t, cui_string("Primary action"), disabled);
        cui_badge(ui, cui_id_from(ui, "theme-badge"), &t, cui_string("Surface / primary"));
        cui_begin_box(ui, cui_id_from(ui, "shape-samples"),
                      (cui_box){.width = width - 48, .height = 64, .row = true, .gap = 24});
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
        cui_begin_box(ui, cui_id_from(ui, "shadow-shape-sample"),
                      (cui_box){.width = 120,
                                .height = 64,
                                .background = {29, 100, 220, 255},
                                .border = {240, 150, 20, 255},
                                .custom_corners = true,
                                .corner_radii = {24, 0, 0, 18},
                                .custom_border_edges = true,
                                .border_edges = {0, 8, 4, 0}});
        const cui_box_shadow shape_depth[] = {
            {.layer = {.offset_y = 3, .sigma = 3, .color = {0, 0, 0, 64}},
             .kind = CUI_BOX_SHADOW_DROP},
            {.layer = {.offset_y = 1, .sigma = 2, .spread = 2, .color = {0, 0, 0, 96}},
             .kind = CUI_BOX_SHADOW_INSET}};
        cui_set_box_shadows(ui, shape_depth, 2, (cui_corner_radii){24, 0, 0, 18});
        cui_end_box(ui);
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
        gallery_effect_preview(&t, width);
        cui_label(ui, &t, cui_string("Typography: host-mapped family, size, weight and italic"));
        cui_begin_box(
            ui, cui_id_from(ui, "typography-sample"),
            (cui_box){.width = width - 48, .height = typography_size * 1.5f, .clip = true});
        const cui_text_style typography_style = {.family = typography_family,
                                                 .weight = (cui_text_weight)typography_weight,
                                                 .italic = typography_italic};
        if (typography_gradient) {
            const cui_text_gradient_stop stops[] = {{0, {244, 40, 90, 255}},
                                                    {1, {30, 90, 250, 255}}};
            cui_text_paragraph_painted(ui, cui_string("WiiW: Build with CUI"), typography_size,
                                       typography_size * 1.25f, typography_style, true,
                                       CUI_TEXT_ALIGN_LEFT,
                                       (cui_text_paint){.kind = CUI_TEXT_PAINT_LINEAR,
                                                        .x1 = 1,
                                                        .anchor = CUI_TEXT_ANCHOR_BLOCK,
                                                        .stops = stops,
                                                        .stop_count = 2});
        } else {
            cui_text_ex(ui, cui_string("WiiW: Build with CUI"), typography_size, t.text,
                        typography_size * 1.25f, typography_style);
        }
        cui_end_box(ui);

        break;
    }
    case 12: {
        static const char *const titles[] = {"Workspace update", "Your changes are saved",
                                             "Storage is almost full", "Upload failed"};
        static const char *const messages[] = {
            "A new version is available.", "You can keep working.",
            "Archive unused projects to free space.", "Check your connection and try again."};
        for (unsigned kind = 0; kind < 4; kind++)
            cui_alert(ui, cui_id_from(ui, titles[kind]), &t,
                      (cui_alert_options){.kind = (cui_alert_kind)kind,
                                          .title = cui_string(titles[kind]),
                                          .message = cui_string(messages[kind]),
                                          .width = width - 48,
                                          .visible = kind == 1 ? &alert_visible : NULL,
                                          .dismissible = kind == 1});
        if (cui_button(ui, cui_id_from(ui, "reset-alert"), &t, cui_string("Show alert"), disabled))
            alert_visible = true;
        break;
    }
    case 13: {
        cui_begin_box(ui, cui_id_from(ui, "avatar-examples"), (cui_box){.gap = 8});
        cui_label(ui, &t, cui_string("Project team · choose a profile"));
        cui_avatar_item people[8] = {0};
        for (unsigned i = 0; i < 8; i++)
            people[i] = (cui_avatar_item){
                .key = i + 1,
                .name = cui_string(i < 3 ? avatar_names[i] : "Additional teammate"),
                .image = i < 3 ? gallery_avatar_source(i) : (cui_bitmap){0},
                .image_state = i < 3 ? &avatar_images[i] : NULL};
        size_t selected = cui_avatar_stack(
            ui, cui_id_from(ui, "avatar-team"), &t, people, 8,
            (cui_avatar_stack_options){.size = 48,
                                       .width = width - 48,
                                       .ring_width = 2,
                                       .interactive = true,
                                       .disabled = disabled != 0,
                                       .name = cui_string("Project team avatars")});
        if (selected != SIZE_MAX)
            avatar_selected = (int)selected;
        char selected_profile[64];
        snprintf(selected_profile, sizeof selected_profile, "Selected profile: %s",
                 avatar_names[avatar_selected % 3]);
        cui_text(ui, cui_string(selected_profile), 14, t.text);
        cui_label(ui, &t, cui_string("Small spaces keep the team count"));
        cui_begin_box(ui, cui_id_from(ui, "avatar-compact-row"),
                      (cui_box){.row = true, .gap = 20, .align_y = CUI_ALIGN_CENTER});
        cui_avatar_stack(ui, cui_id_from(ui, "avatar-team-compact"), &t, people, 8,
                         (cui_avatar_stack_options){.size = 40,
                                                    .width = 96,
                                                    .ring_width = 2,
                                                    .name = cui_string("Compact team avatars")});
        cui_avatar(ui, cui_id_from(ui, "avatar-fallback"), &t, cui_string("Sam Patel"), 40);
        cui_end_box(ui);
        cui_label(ui, &t, cui_string("Palette, size and anonymous SVG fallback"));
        if (!avatar_custom_placeholder_ready) {
            cui_icon_state_init(&avatar_custom_placeholder);
            cui_icon_set_data(
                &avatar_custom_placeholder,
                cui_string("<svg viewBox='0 0 24 24'><path d='M12 2L22 12L12 22L2 12Z'/></svg>"));
            avatar_custom_placeholder_ready = true;
        }
        cui_avatar_ex(
            ui, cui_id_from(ui, "avatar-variant"), &t,
            avatar_source == 2 ? cui_string("") : cui_string("Taylor Reed"),
            (cui_avatar_options){
                .size = 48,
                .design_size = design_size,
                .rem_size = avatar_rem,
                .identity_colors = avatar_palette != 0,
                .custom_identity_hue = avatar_hue >= 0,
                .identity_hue = avatar_hue >= 0 ? (unsigned)avatar_hue : 0,
                .anonymous_icon = true,
                .placeholder_name =
                    avatar_placeholder == 1 ? cui_string("circle-check") : cui_string(""),
                .placeholder = avatar_placeholder == 2 ? &avatar_custom_placeholder : NULL,
                .image = avatar_source == 0 ? gallery_avatar_source(0) : (cui_bitmap){0},
                .image_state = &avatar_images[0]});
        cui_label(ui, &t,
                  cui_string("Photos win; names select initials; anonymous icons are decorative."));
        cui_end_box(ui);
        break;
    }
    case 14: {
        static const char *const messages[] = {
            "Info: a new version is available.", "Success: your changes are saved.",
            "Warning: storage is almost full.", "Error: upload failed. Please retry."};
        for (unsigned kind = 0; kind < 4; kind++) {
            gallery_bubble_reactions[kind].key = messages[kind];
            gallery_bubble_reactions[kind].disabled = disabled != 0;
            cui_bubble(
                ui, cui_id_from(ui, messages[kind]), &t,
                (cui_bubble_options){
                    .variant = (cui_bubble_variant)(CUI_BUBBLE_INFO + kind),
                    .alignment = kind % 2 ? CUI_BUBBLE_OUTGOING : CUI_BUBBLE_INCOMING,
                    .reaction_alignment = kind % 2 ? CUI_BUBBLE_OUTGOING : CUI_BUBBLE_INCOMING,
                    .reaction_side =
                        kind == 0 ? CUI_BUBBLE_REACTIONS_TOP : CUI_BUBBLE_REACTIONS_BOTTOM,
                    .width = width - 48,
                    .message = cui_string(messages[kind]),
                    .reactions = gallery_bubble_reaction,
                    .state = &gallery_bubble_states[kind],
                    .user = &gallery_bubble_reactions[kind]});
        }
        break;
    }
    case 15: {
        empty_disabled = disabled;
        const cui_empty_slot media = {.children = gallery_empty_media};
        const cui_empty_slot title = {.text = cui_string("No projects yet")};
        const cui_empty_slot description = {
            .text = cui_string("Create a project to organize your team's work.")};
        const cui_empty_slot content = {.children = gallery_empty_action};
        const cui_empty_header header = {.media = &media,
                                         .title = &title,
                                         .description = &description,
                                         .media_variant = CUI_EMPTY_MEDIA_ICON};
        cui_empty_style style =
            cui_empty_default_style(&t, CUI_EMPTY_ROOT, CUI_EMPTY_MEDIA_DEFAULT, 16);
        style.box.width = width - 48;
        style.box.height = 220;
        style.box.border_width = 1;
        cui_empty_ex(ui, cui_id_from(ui, "empty"), &t,
                     (cui_empty_options){.style = &style,
                                         .header = &header,
                                         .content = &content,
                                         .accessible_name = cui_string("Empty project workspace")});
        char status[80];
        snprintf(status, sizeof status, "Projects created: %u", empty_created);
        cui_label(ui, &t, cui_string(status));
        break;
    }
    case 16:
        cui_begin_group_box(ui, cui_id_from(ui, "groupbox"), &t, cui_string("Preferences"),
                            width - 48);
        cui_checkbox(ui, cui_id_from(ui, "group-check"), &t, cui_string("Receive updates"),
                     &checked, disabled);
        cui_end_group_box(ui);
        break;
    case 17:
        cui_label(ui, &t, cui_string("Shortcut hints"));
        cui_begin_box(ui, cui_id_from(ui, "shortcut-hints"), (cui_box){.row = true, .gap = t.gap});
        cui_kbd_ex(ui, cui_id_from(ui, "kbd"), &t,
                   (cui_kbd_stroke){.modifiers = CUI_MOD_CTRL | CUI_MOD_SHIFT, .key = CUI_STR("k")},
                   (cui_kbd_options){0});
        cui_kbd_ex(ui, cui_id_from(ui, "kbd-outline"), &t,
                   (cui_kbd_stroke){.key = CUI_STR("escape")}, (cui_kbd_options){.outline = true});
        cui_kbd_ex(ui, cui_id_from(ui, "kbd-plain"), &t, (cui_kbd_stroke){.key = CUI_STR("enter")},
                   (cui_kbd_options){.plain = true});
        cui_end_box(ui);
        cui_label(ui, &t, cui_string("Hints do not register shortcuts."));
        break;
    case 18: {
        cui_skeleton_options options = cui_skeleton_options_default();
        options.width = width - 48;
        options.height = 36;
        options.radius = 6;
        options.now_seconds = cui_frame_time(ui);
        options.reduced_motion = reduced_motion;
        cui_loading_motion_result motion;
        if (cui_skeleton_ex(ui, cui_id_from(ui, "skeleton"), &t, &skeleton_motion, options,
                            &motion))
            next_wake = motion.next_wake_seconds > 0
                            ? host_seconds + fmax(0, motion.next_wake_seconds - cui_frame_time(ui))
                            : 0;
        break;
    }
    case 19: {
        cui_spinner_options options = cui_spinner_options_default();
        options.size = spinner_size;
        options.custom_color = spinner_color != 0;
        options.color = (cui_color){220, 40, 90, spinner_color == 2 ? 0 : 255};
        options.ease = spinner_easing ? gallery_spinner_linear : NULL;
        if (spinner_icon) {
            options.paint = gallery_spinner_icon;
            options.paint_user = spinner_icon == 1 ? "loader-circle" : "arrow-right";
        }
        options.now_seconds = cui_frame_time(ui);
        options.reduced_motion = reduced_motion;
        cui_loading_motion_result motion;
        if (cui_spinner_ex(ui, cui_id_from(ui, "spinner"), &t, &spinner_motion, options, &motion))
            next_wake = motion.next_wake_seconds > 0
                            ? host_seconds + fmax(0, motion.next_wake_seconds - cui_frame_time(ui))
                            : 0;
        break;
    }
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
                                    .design_size = design_size,
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
                                     .height = story == 23 ? 100 : 0,
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
        cui_id editor_id = cui_id_from(ui, "code-editor");
        cui_label(
            ui, &t,
            cui_string("Editable C code: line numbers, syntax colors, undo and Ctrl/Meta-F."));
        cui_text(ui,
                 cui_string(
                     "Ctrl/Alt+Up/Down adds cursors; Shift+Alt-drag selects a block; Esc resets."),
                 12, t.muted);
        cui_begin_box(ui, cui_id_from(ui, "edit-actions"),
                      (cui_box){.row = true, .gap = 8, .align_y = CUI_ALIGN_CENTER});
        if (cui_button(ui, cui_id_from(ui, "editor-find-open"), &t, cui_string("Find"), disabled))
            cui_editor_search_open(&editor_state, &editor_search, false);
        if (cui_button(ui, cui_id_from(ui, "undo"), &t, cui_string("Undo"), disabled))
            cui_edit_undo(editor_edit);
        if (cui_button(ui, cui_id_from(ui, "redo"), &t, cui_string("Redo"), disabled))
            cui_edit_redo(editor_edit);
        bool show_completions = cui_button_ex(ui, cui_id_from(ui, "editor-completions"), &t,
                                              cui_string("Show completions"),
                                              (cui_button_options){.size = CUI_CONTROL_SMALL,
                                                                   .icon = cui_string("code"),
                                                                   .icon_only = true,
                                                                   .disabled = disabled != 0});
        cui_end_box(ui);
        cui_editor_find_bar(ui, cui_id_from(ui, "code-find"), editor_id, &t, &editor_state,
                            &editor_search, width - 48, disabled != 0);
        cui_editor_options options = cui_editor_default();
        options.textarea.width = width - 48;
        options.textarea.height = 280;
        options.textarea.disabled = disabled != 0;
        options.language = cui_string("c");
        options.search = &editor_matcher;
        options.highlight = gallery_editor_token;
        cui_editor(ui, editor_id, &t, cui_string("C code editor"), &editor_state, options);
        if (show_completions) {
            cui_edit_state selection = cui_edit_get_state(editor_edit);
            size_t start =
                selection.cursor < selection.anchor ? selection.cursor : selection.anchor;
            size_t end = selection.cursor > selection.anchor ? selection.cursor : selection.anchor;
            cui_focus(ui, editor_id);
            cui_editor_show_completions(&editor_state, editor_completions,
                                        sizeof(editor_completions) / sizeof(editor_completions[0]),
                                        start, end);
            cui_invalidate(ui);
        }
        break;
    }
    case 26: {
        cui_label(ui, &t,
                  cui_string(virtual_mode ? "100 wide tasks; scroll horizontally or use Left/Right."
                                          : "100,000 tasks; only visible rows are drawn."));
        cui_id owner = cui_id_from(ui, "virtual-viewport");
        virtual_disabled = disabled != 0;
        virtual_result = cui_virtual_list_draw(
            ui, owner, &t, virtual_mode ? virtual_horizontal : virtual_list, &virtual_state,
            (cui_virtual_widget_options){.scroll = {.width = width - 48,
                                                    .height = 144,
                                                    .disabled = disabled,
                                                    .wheel_step = 24},
                                         .overscan = 1,
                                         .horizontal = virtual_mode != 0,
                                         .no_item_semantics = true},
            gallery_virtual_row, NULL);
        char text[128];
        snprintf(text, sizeof(text), "Submitted %zu rows: %zu .. %zu / %zu",
                 virtual_result.view.end - virtual_result.view.begin, virtual_result.view.begin + 1,
                 virtual_result.view.end,
                 cui_virtual_count(virtual_mode ? virtual_horizontal : virtual_list));
        cui_label(ui, &t, cui_string(text));
        break;
    }
    case 27: {
        cui_label(
            ui, &t,
            cui_string("Select or close tabs; drag tabs and dividers, or split the active pane."));
        cui_begin_box(ui, 0, (cui_box){.row = true, .gap = 8});
        if (cui_button(ui, cui_id_from(ui, "split"), &t, cui_string("Split panel"),
                       disabled || dock_serial >= 10)) {
            cui_dock_node tab;
            size_t index;
            if (cui_dock_panel_find(dock, dock_active_panel, &tab, &index)) {
                cui_dock_node fresh;
                if (cui_dock_split(dock, tab, CUI_DOCK_SPLIT_HORIZONTAL, .5, dock_serial, false,
                                   &fresh) == CUI_DOCK_OK) {
                    dock_active_panel = dock_serial++;
                    redraw = true;
                }
            }
        }
        if (cui_button(ui, cui_id_from(ui, "dock-reset"), &t, cui_string("Reset layout"), disabled))
            gallery_dock_reset();
        cui_end_box(ui);
        static const char *titles[] = {"Panel 1", "Panel 2",  "Panel 3",   "Panel 4",
                                       "Panel 5", "Panel 6",  "Panel 7",   "Panel 8",
                                       "Panel 9", "Explorer", "Inspector", "Terminal"};
        cui_dock_panel registry[12];
        for (unsigned i = 0; i < 12; ++i)
            registry[i] = (cui_dock_panel){.key = i + 1,
                                           .title = cui_string(titles[i]),
                                           .closable = true,
                                           .zoomable = true,
                                           .toolbar = gallery_dock_toolbar,
                                           .render = gallery_dock_content};
        dock_disabled = disabled != 0;
        cui_dock_view_options view_options = cui_dock_view_options_default();
        view_options.width = width - 48;
        view_options.height = 180;
        view_options.disabled = disabled != 0;
        view_options.now_seconds = host_seconds;
        view_options.reduced_motion = reduced_motion;
        view_options.on_event = gallery_dock_event;
        cui_dock_workspace_snapshot snapshot;
        cui_dock_workspace_save(dock_workspace, &snapshot);
        static const char *edge_labels[] = {"Toggle Explorer", "Toggle Inspector",
                                            "Toggle Terminal"};
        cui_begin_box(ui, cui_id_from(ui, "dock-edges"), (cui_box){.row = true, .gap = 8});
        for (int i = 0; i < 3; i++) {
            if (cui_button(ui, cui_id_from(ui, edge_labels[i]), &t, cui_string(edge_labels[i]),
                           disabled))
                cui_dock_workspace_set_edge(dock_workspace, (cui_dock_region)(i + 1),
                                            !snapshot.open[i], snapshot.size[i]);
        }
        cui_end_box(ui);
        view_options.height = 260;
        cui_dock_region_content workspace_regions[4] = {
            {.model = dock, .view = dock_view, .panels = registry, .panel_count = 12}};
        for (int i = 0; i < 3; i++)
            workspace_regions[i + 1] = (cui_dock_region_content){.model = dock_edges[i],
                                                                 .view = dock_edge_views[i],
                                                                 .panels = registry,
                                                                 .panel_count = 12};
        cui_dock_workspace_render(ui, cui_id_from(ui, "dock-workspace"), &t, dock_workspace,
                                  workspace_regions,
                                  (cui_dock_workspace_options){.view = view_options,
                                                               .on_transfer = gallery_dock_transfer,
                                                               .collapsible_bottom = true});
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
        float plot_width = fminf(640, fmaxf(80, width - 48));
        if (!plot_viewport.initialized)
            cui_plot_viewport_init(
                &plot_viewport,
                (cui_plot_scale){CUI_PLOT_LINEAR, 0, 100, 28, plot_width - 12, false},
                (cui_plot_scale){CUI_PLOT_LINEAR, -1.2, 1.2, 142, 12, false});
        plot_viewport.x.range_start = 28;
        plot_viewport.x.range_end = plot_width - 12;
        cui_id plot_id = cui_id_from(ui, "plot-wave");
        cui_plot_viewport_options navigation = cui_plot_viewport_defaults();
        navigation.disabled = disabled != 0;
        navigation.x_limit = (cui_plot_viewport_limit){
            .bounded = true, .minimum = -100, .maximum = 200, .min_span = 10, .max_span = 300};
        navigation.y_limit = (cui_plot_viewport_limit){
            .bounded = true, .minimum = -5, .maximum = 5, .min_span = .1, .max_span = 10};
        cui_plot_viewport_reduce(ui, plot_id, &plot_viewport, navigation);
        float plot_anchor = (float)(plot_viewport.x.range_start + plot_viewport.x.range_end) / 2;
        cui_text(ui,
                 cui_string("Shift-drag to pan; Ctrl-wheel or pinch to zoom; touch-drag to pan"),
                 14, t.text);
        cui_begin_box(ui, 0, (cui_box){.row = true, .gap = 8});
        if (cui_button(ui, cui_id_from(ui, "plot-zoom"), &t, cui_string("Zoom in"), disabled))
            cui_plot_viewport_transform(&plot_viewport, navigation, 0, 0, 1.5,
                                        (cui_plot_point){plot_anchor, 77});
        if (cui_button(ui, cui_id_from(ui, "plot-zoom-out"), &t, cui_string("Zoom out"), disabled))
            cui_plot_viewport_transform(&plot_viewport, navigation, 0, 0, 1 / 1.5,
                                        (cui_plot_point){plot_anchor, 77});
        if (cui_button(ui, cui_id_from(ui, "plot-reset"), &t, cui_string("Reset"), disabled))
            cui_plot_viewport_reset(&plot_viewport);
        cui_end_box(ui);
        cui_begin_box(ui, 0, (cui_box){.row = true, .gap = 8});
        if (cui_button(ui, cui_id_from(ui, "plot-left"), &t, cui_string("Pan left"), disabled))
            cui_plot_viewport_transform(&plot_viewport, navigation, plot_width * .15, 0, 1,
                                        (cui_plot_point){plot_anchor, 77});
        if (cui_button(ui, cui_id_from(ui, "plot-right"), &t, cui_string("Pan right"), disabled))
            cui_plot_viewport_transform(&plot_viewport, navigation, -plot_width * .15, 0, 1,
                                        (cui_plot_point){plot_anchor, 77});
        cui_end_box(ui);
        unsigned pw = (unsigned)plot_width;
        bool pixels_changed =
            !plot_revision || plot_cached_width != pw || plot_cached_mode != plot_mode ||
            memcmp(&plot_cached_scale, &plot_viewport.x, sizeof(plot_viewport.x)) ||
            memcmp(&plot_cached_y, &plot_viewport.y, sizeof(plot_viewport.y)) ||
            memcmp(&plot_cached_primary, &t.primary, sizeof(t.primary)) ||
            memcmp(&plot_cached_surface, &t.surface, sizeof(t.surface));
        if (pixels_changed) {
            cui_plot_sample samples[33];
            for (size_t i = 0; i < 33; i++) {
                double x = 28 + (plot_width - 40) * i / 32, value;
                cui_plot_inverse(&plot_viewport.x, x, &value);
                double y, baseline;
                cui_plot_map(&plot_viewport.y, sin(value / 12), &y);
                cui_plot_map(&plot_viewport.y, 0, &baseline);
                samples[i] = (cui_plot_sample){x, baseline, y, true};
            }
            cui_plot_path_command commands[256];
            size_t command_count = 0;
            cui_plot_result result;
            if (plot_mode == 3) {
                cui_plot_radial_sample radial[12];
                for (size_t i = 0; i < 12; i++)
                    radial[i] = (cui_plot_radial_sample){i * 6.283185307179586 / 12, 0,
                                                         38 + 22 * (sin(i * 1.7) + 1) / 2, true};
                result = cui_plot_radial_path(radial, 12, CUI_PLOT_GAPS_SKIP, true, true, false,
                                              commands, 256, &command_count);
                for (size_t i = 0; i < command_count; i++) {
                    double data_x, data_y;
                    cui_plot_scale initial_x = plot_viewport.initial_x;
                    initial_x.range_start = plot_viewport.x.range_start;
                    initial_x.range_end = plot_viewport.x.range_end;
                    cui_plot_inverse(&initial_x, commands[i].point.x + plot_width / 2, &data_x);
                    cui_plot_inverse(&plot_viewport.initial_y, commands[i].point.y + 77, &data_y);
                    cui_plot_map(&plot_viewport.x, data_x, &commands[i].point.x);
                    cui_plot_map(&plot_viewport.y, data_y, &commands[i].point.y);
                }
            } else if (plot_mode == 5 || plot_mode == 6) {
                /* The same projected samples feed source rounded bars/dot geometry.
                 * Every eighth sample keeps the small software preview inexpensive. */
                result = CUI_PLOT_OK;
                for (size_t i = 0; i < 33 && result == CUI_PLOT_OK; i += 8) {
                    double cross = samples[i].x - 4, baseline = samples[i].y0;
                    double value = samples[i].y1, band = 8;
                    double radii[4] = {3, 3, 0, 0};
                    if (plot_mode == 6) {
                        cui_plot_dot_geometry dot;
                        result = cui_plot_dot_layout((cui_plot_point){samples[i].x, samples[i].y1},
                                                     8, &dot);
                        if (result != CUI_PLOT_OK)
                            break;
                        cross = dot.frame.x_min;
                        baseline = dot.frame.y_max;
                        value = dot.frame.y_min;
                        band = dot.frame.x_max - dot.frame.x_min;
                        for (size_t corner = 0; corner < 4; corner++)
                            radii[corner] = dot.radius;
                    }
                    size_t written = 0;
                    result =
                        cui_plot_bar_path(CUI_PLOT_BAR_BOTTOM, cross, band, baseline, value, radii,
                                          commands + command_count, 256 - command_count, &written);
                    /* Zero-valued bars have no geometry, not a failed frame. */
                    if (result == CUI_PLOT_EMPTY)
                        result = CUI_PLOT_OK;
                    if (result == CUI_PLOT_OK)
                        command_count += written;
                }
            } else
                result = cui_plot_path(samples, 33,
                                       plot_mode == 2   ? CUI_PLOT_CURVE_STEP_AFTER
                                       : plot_mode == 4 ? CUI_PLOT_CURVE_NATURAL
                                                        : CUI_PLOT_CURVE_LINEAR,
                                       CUI_PLOT_GAPS_SKIP, plot_mode == 1, commands, 256,
                                       &command_count);
            cui_color fill = t.primary;
            fill.a = plot_mode >= 5 ? 255 : (plot_mode == 1 || plot_mode == 3) ? 70 : 0;
            cui_plot_shape shape = {.commands = commands,
                                    .count = command_count,
                                    .fill = fill,
                                    .stroke = t.primary,
                                    .stroke_width = 2};
            if (result == CUI_PLOT_OK &&
                cui_plot_raster(plot_pixels, pw, 180, pw * 4, sizeof(plot_pixels), &shape, 1,
                                t.surface) == CUI_PLOT_OK) {
                plot_cached_width = pw;
                plot_cached_scale = plot_viewport.x;
                plot_cached_y = plot_viewport.y;
                plot_cached_mode = plot_mode;
                plot_cached_primary = t.primary;
                plot_cached_surface = t.surface;
                plot_revision++;
            }
        }
        size_t nx = 0, ny = 0;
        cui_plot_axis_ticks(&plot_viewport.x, 5, plot_x_ticks, 16, &nx);
        cui_plot_scale y_scale = plot_viewport.y;
        cui_plot_axis_ticks(&y_scale, 5, plot_y_ticks, 16, &ny);
        for (size_t i = 0; i < nx; i++)
            plot_x_labels[i] = plot_x_ticks[i].label;
        for (size_t i = 0; i < ny; i++) {
            plot_y_labels[i] = plot_y_ticks[i].label;
            plot_y_labels[i].align = CUI_ALIGN_END;
        }
        cui_plot_axis_options axis = cui_plot_axis_defaults(plot_width, 180);
        axis.has_x = axis.has_y = plot_mode != 3;
        axis.x = 142;
        axis.y = 28;
        axis.y_side = CUI_PLOT_AXIS_START;
        axis.x_labels = plot_x_labels;
        axis.x_count = nx;
        axis.y_labels = plot_y_labels;
        axis.y_count = ny;
        cui_plot_view_options view = {.bitmap = {plot_pixels, pw, 180, pw * 4, plot_revision},
                                      .axis = &axis,
                                      .name = cui_string("Projected sine wave"),
                                      .disabled = disabled != 0};
        cui_plot_view(ui, plot_id, &t, &view);
        cui_plot_viewport_bind(ui, plot_id, &plot_viewport, navigation);
        double ticks[32];
        size_t count = 0;
        cui_plot_ticks(&plot_viewport.x, 5, ticks, 32, &count);
        char text[128];
        snprintf(text, sizeof(text), "Domain: %.2f .. %.2f; %zu scale ticks",
                 plot_viewport.x.domain_min, plot_viewport.x.domain_max, count);
        cui_text(ui, cui_string(text), 14, t.muted);
        break;
    }
    case 29: {
        cui_begin_box(ui, cui_id_from(ui, "calendar-presentation-controls"),
                      (cui_box){.row = width > 540, .gap = 8});
        cui_checkbox(ui, cui_id_from(ui, "calendar-month-menu"), &t, cui_string("Month menu"),
                     &calendar_month_menu, disabled);
        cui_checkbox(ui, cui_id_from(ui, "calendar-neighbors"), &t, cui_string("Adjacent days"),
                     &calendar_neighbors, disabled);
        cui_checkbox(ui, cui_id_from(ui, "calendar-today"), &t, cui_string("Mark today"),
                     &calendar_mark_today, disabled);
        cui_end_box(ui);
        cui_calendar_ex(ui, cui_id_from(ui, "calendar"), &t, &calendar, &date_selection,
                        (cui_calendar_options){
                            .disabled = disabled,
                            .locale = cui_date_locale_builtin((cui_date_locale_kind)date_locale),
                            .first_weekday = 1,
                            .cell_size = width > 360 ? 32 : 28},
                        (cui_calendar_presentation_options){
                            .month_toggle = calendar_month_menu,
                            .neighboring_days = calendar_neighbors,
                            .today = calendar_mark_today ? calendar_today : (cui_date){0}});
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
        table_story_disabled = disabled != 0;
        cui_begin_box(ui, cui_id_from(ui, "table-contract-controls"),
                      (cui_box){.row = true, .width = width - 48, .gap = 4});
        const cui_button_options toggle_options = {
            .width = (width - 56) / 3, .size = CUI_CONTROL_SMALL, .disabled = disabled != 0};
        if (cui_button_ex(ui, cui_id_from(ui, "table-selection-policy"), &t,
                          cui_string(table_selection_locked ? "Select off" : "Select on"),
                          toggle_options)) {
            table_selection_locked = !table_selection_locked;
            cui_invalidate(ui);
        }
        if (cui_button_ex(ui, cui_id_from(ui, "table-action-policy"), &t,
                          cui_string(table_actions ? "Actions on" : "Actions off"),
                          toggle_options)) {
            table_actions = !table_actions;
            cui_invalidate(ui);
            for (size_t c = 0; c < 3; c++)
                table_columns[c].nonselectable = table_actions && table_columns[c].key == 102;
        }
        if (cui_button_ex(ui, cui_id_from(ui, "table-sort-policy"), &t,
                          cui_string(table_source_sort ? "Source sort" : "Legacy sort"),
                          toggle_options)) {
            table_source_sort = !table_source_sort;
            cui_invalidate(ui);
        }
        cui_end_box(ui);
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
                                .render_empty = table_empty,
                                .row_selection_disabled = table_selection_locked,
                                .column_selection_disabled = table_selection_locked,
                                .cell_selection_disabled = table_selection_locked,
                                .sort_cycle = table_source_sort ? CUI_TABLE_SORT_CYCLE_SOURCE
                                                                : CUI_TABLE_SORT_CYCLE_LEGACY,
                                .accessible_name = cui_string("Workspace files")},
            table_label, table_actions ? table_action_cell : NULL, NULL);
        if (result.selection_mode_requested) {
            table_selection_mode = result.requested_selection_mode;
            redraw = true;
        }
        if (result.sort_changed && gallery_apply_table_sort()) {
            cui_invalidate(ui);
            redraw = true;
        }
        char text[72];
        snprintf(text, sizeof(text), "%zu rows / visible %zu-%zu / sort %d", cui_table_rows(table),
                 result.rows.end > result.rows.begin ? result.rows.begin + 1 : 0, result.rows.end,
                 (int)table_state.sort);
        if (table_mode == 1 || table_actions) {
            snprintf(text, sizeof(text), "Visible row hooks: %d / inspections: %d", table_row_hooks,
                     table_inspections);
        }
        cui_label(ui, &t, cui_string(text));
        break;
    }
    case 46: {
        float cell_width = (width - 48) / 3;
        cui_design_metrics metrics;
        bool scaled = design_size != CUI_DESIGN_INHERIT &&
                      cui_design_measure(design_size, t.font_size / 16, &metrics);
        float cell_height = scaled ? metrics.control_height : 40;
        if (scaled)
            cell_width = fmaxf(
                cell_width, cui_measure_text(ui, cui_string("File 1000"), metrics.font_size).width +
                                2 * metrics.padding_y + 2);
        cui_scroll_options table_scroll = {.width = width - 48,
                                           .height = 4 * cell_height + 12,
                                           .content_width = 3 * cell_width,
                                           .content_height = 4 * cell_height,
                                           .horizontal = true,
                                           .disabled = disabled != 0};
        cui_id scroll_id = cui_id_from(ui, "table-composition-scroll");
        if (!cui_scrollable_begin(ui, scroll_id, &t, &table_composition_scroll, table_scroll))
            break;
        cui_id table_id = cui_id_from(ui, "composition-table");
        if (!cui_table_begin_ex(
                ui, table_id, &t,
                (cui_table_basic_options){.width = 3 * cell_width,
                                          .accessible_name = cui_string("Recent workspace files"),
                                          .row_count = 4,
                                          .column_count = 3})) {
            cui_scrollable_end(ui, scroll_id, &t, &table_composition_scroll, table_scroll);
            break;
        }
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
            if (!cui_table_begin_row_checked(
                    ui, row_id, &t,
                    (cui_table_row_options){.height = cell_height,
                                            .header = r == 0,
                                            .parent = section_id,
                                            .row_index = (unsigned)r + 1,
                                            .custom_background = true,
                                            .background = r == 0  ? t.accent
                                                          : r % 2 ? t.surface
                                                                  : t.background}))
                continue;
            for (size_t c = 0; c < 3; c++) {
                cui_id cell_id = table_id + 100 + r * 3 + c;
                const char *headers[] = {"Name", "Status", "Size"};
                cui_str label = r == 0 ? cui_string(headers[c]) : table_label(NULL, r - 1, 101 + c);
                if (cui_table_begin_cell_ex(
                        ui, cell_id, &t,
                        (cui_table_cell_options){.width = cell_width,
                                                 .height = cell_height,
                                                 .align = CUI_TABLE_ALIGN_LEFT,
                                                 .name = label,
                                                 .parent = row_id,
                                                 .row_index = (unsigned)r + 1,
                                                 .column_index = (unsigned)c + 1,
                                                 .header = r == 0,
                                                 .design_size = design_size})) {
                    if (scaled)
                        cui_text_with_line_height(ui, label, metrics.font_size, t.text,
                                                  metrics.line_height);
                    else
                        cui_label(ui, &t, label);
                    cui_table_end(ui);
                }
            }
            cui_table_end(ui);
        }
        cui_table_end(ui);
        cui_table_end(ui);
        cui_scrollable_end(ui, scroll_id, &t, &table_composition_scroll, table_scroll);
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
                                       .sheet_animation_seconds = .22f,
                                       .reduced_motion = reduced_motion,
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
                                       .side = CUI_POPUP_BELOW,
                                       .popover_appearance = CUI_POPOVER_APPEARANCE_SOURCE};
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
                                              .design_size = design_size,
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
                                            .size = (cui_list_size)control_size,
                                            .design_size = design_size,
                                            .dt = host_dt};
        if (story == 30 && multiple_mode)
            cui_list_multi(ui, cui_id_from(ui, "list"), &t, items, 3, multi_selected, &list_state,
                           (cui_list_options){
                               .disabled = disabled, .dt = host_dt, .design_size = design_size});
        else if (story == 30)
            cui_list(ui, cui_id_from(ui, "list"), &t, items, 3, &list_selection, &list_state,
                     (cui_list_options){
                         .disabled = disabled, .dt = host_dt, .design_size = design_size});
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
                                              .size = (cui_number_size)control_size,
                                              .design_size = design_size,
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
        if (cui_checkbox(ui, cui_id_from(ui, "stepper-details"), &t,
                         cui_string("Details confirmed"), &stepper_details_confirmed, disabled))
            stepper_validation_failed = false;
        const cui_step steps[3] = {
            {cui_string("Details"), cui_string("Confirm before continuing"),
             stepper_validation_failed ? CUI_STEP_ERROR
             : current_step > 0        ? CUI_STEP_COMPLETE
                                       : CUI_STEP_ACTIVE,
             false},
            {cui_string("Review"), cui_string("Review application"),
             current_step > 1 ? CUI_STEP_COMPLETE : CUI_STEP_PENDING, false},
            {cui_string("Finish"), cui_string("Ready to submit"), CUI_STEP_PENDING, false}};
        cui_stepper_navigation(ui, cui_id_from(ui, "stepper"), &t, steps, 3, current_step, true,
                               disabled,
                               (cui_stepper_navigation_handler){gallery_stepper_request, NULL});
        char status[160];
        snprintf(status, sizeof status, "Step %zu of 3", current_step + 1);
        cui_label(ui, &t, cui_string(status));
        snprintf(status, sizeof status, "Navigation requests: %u", stepper_request_count);
        cui_label(ui, &t, cui_string(status));
        if (stepper_validation_failed)
            cui_label(ui, &t, cui_string("Confirm details to continue."));
        cui_label(ui, &t, cui_string("Your app validates each request."));
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
                                            .size = (cui_list_size)control_size,
                                            .design_size = design_size,
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
        cui_date_picker_ex(
            ui, cui_id_from(ui, "date-picker"), &t, cui_string("Date"), &date_picker,
            &date_selection,
            (cui_date_picker_options){
                .calendar = {.first_weekday = 1,
                             .cell_size = 28,
                             .locale = cui_date_locale_builtin((cui_date_locale_kind)date_locale)},
                .viewport = {0, 0, width, height},
                .disabled = disabled,
                .width = width > 360 ? 280 : width - 48},
            (cui_date_picker_calendar_options){.presentation = {.month_toggle = true,
                                                                .neighboring_days = true,
                                                                .today = calendar_today}});
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
        cui_label(ui, &t, cui_string("Hex draft · Enter commits"));
        cui_color_entry_result entry = cui_color_entry(
            ui, cui_id_from(ui, "color-hex-entry"), &t, cui_string("Hex color"), &color_entry,
            (cui_color_entry_options){.disabled = disabled,
                                      .width = width > 360 ? 280 : width - 68},
            (cui_color_entry_handler){0});
        if (entry.commit_count && entry.committed.present) {
            chosen_color = entry.committed.color;
            color_entry_commits += entry.commit_count;
            redraw = true;
        }
        char entry_status[96];
        snprintf(entry_status, sizeof entry_status, "Hex commits: %zu%s", color_entry_commits,
                 entry.status == CUI_COLOR_HEX_INCOMPLETE ? " · incomplete draft" : "");
        cui_label(ui, &t, cui_string(entry_status));
        color_entry_restore = cui_button(ui, cui_id_from(ui, "color-hex-restore"), &t,
                                         cui_string("Use picker color"), disabled);
        if (color_entry_restore)
            redraw = true;
        break;
    }
    default:
        cui_label(ui, &t, cui_string("This component has no browser story yet."));
        break;
    }
    if (story != 75 && story != 80)
        cui_end_box(ui);
    cui_touch_page_dispatch(&page_touch_host, ui);
    draw = cui_end_frame(ui);
    cui_touch_page_finish(&page_touch_host, ui);
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
int gallery_cursor(void) {
    return (int)cui_pointer_cursor(ui);
}
void gallery_sidebar_options(int mode, int side, int collapsed) {
    sidebar_mode = mode >= CUI_SIDEBAR_ICON && mode <= CUI_SIDEBAR_NONE ? mode : CUI_SIDEBAR_ICON;
    sidebar_side = side == CUI_SIDEBAR_RIGHT ? CUI_SIDEBAR_RIGHT : CUI_SIDEBAR_LEFT;
    sidebar_collapsed = collapsed != 0;
}
float gallery_sidebar_stats(unsigned field) {
    switch (field) {
    case 0:
        return sidebar_collapsed;
    case 1:
        return sidebar_state.width;
    case 2:
        return sidebar_state.scroll.y;
    case 3:
        return sidebar_actions;
    case 4:
        return sidebar_selected;
    case 5:
        return sidebar_state.render_child;
    default:
        return 0;
    }
}
void gallery_textarea_options(int rows, int auto_grow, int readonly, int no_wrap) {
    textarea_rows = rows >= 1 && rows <= 8 ? rows : 4;
    textarea_auto_grow = auto_grow != 0;
    textarea_readonly = readonly != 0;
    textarea_no_wrap = no_wrap != 0;
}
float gallery_textarea_stats(unsigned field) {
    cui_edit_state state = cui_edit_get_state(textarea_edit);
    switch (field) {
    case 0:
        return (float)cui_edit_get_text(textarea_edit).length;
    case 1:
        return (float)textarea_state.count;
    case 2:
        return (float)state.cursor;
    case 3:
        return (float)state.anchor;
    case 4:
        return textarea_state.scroll_y;
    case 5:
        return textarea_state.viewport.h;
    case 6:
        return (float)textarea_result.error;
    default:
        return 0;
    }
}
const char *gallery_textarea_text(void) {
    return cui_edit_get_text(textarea_edit).data;
}
void gallery_description_options(int columns, int vertical, int plain, int size) {
    description_columns = columns >= 1 && columns <= 3 ? columns : 3;
    description_vertical = vertical != 0;
    description_plain = plain != 0;
    description_size = size >= 0 && size <= 3 ? size : 0;
}
void gallery_status_options(int right, int custom) {
    status_right = right != 0;
    status_custom = custom != 0;
}
void gallery_date_locale(int locale) {
    date_locale = locale >= -1 && locale <= CUI_DATE_LOCALE_JA ? locale : -1;
}
void gallery_calendar_today(int year, unsigned month, unsigned day) {
    cui_date supplied = {year, month, day};
    calendar_today = cui_date_valid(supplied) ? supplied : (cui_date){0};
}
unsigned gallery_date_value(void) {
    return (unsigned)(date_selection.start.year * 10000 + date_selection.start.month * 100 +
                      date_selection.start.day);
}
void gallery_resizable_options(int vertical, int disabled) {
    if (resizable_vertical != (vertical != 0))
        cui_resizable_clear(&resizable_state);
    resizable_vertical = vertical != 0;
    resizable_disabled = disabled != 0;
}
float gallery_resizable_size(unsigned index) {
    return index < resizable_state.count ? resizable_state.slots[index].size : 0;
}
void gallery_icon_options(int choice, int size, int rotate, int color, int layout) {
    icon_layout = layout >= 0 && layout <= 3 ? layout : 0;
    icon_choice = choice >= 0 && choice < 3 ? choice : 0;
    icon_size = size >= CUI_ICON_INHERITED && size <= CUI_ICON_LARGE ? size : CUI_ICON_MEDIUM;
    icon_rotate = rotate != 0;
    icon_color = color != 0;
}
float gallery_icon_stats(unsigned field) {
    switch (field) {
    case 0:
        return (float)icon_state.parse_count;
    case 1:
        return (float)icon_state.raster_count;
    case 2:
        return (float)icon_state.cache_bytes;
    case 3:
        return (float)icon_state.error;
    default:
        return 0;
    }
}
/* A host picker supplies opaque sRGB bytes; the controlled model preserves alpha. */
int gallery_screen_color(unsigned rgb) {
    if (rgb > 0xffffffu)
        return 0;
    chosen_color.r = (uint8_t)(rgb >> 16);
    chosen_color.g = (uint8_t)(rgb >> 8);
    chosen_color.b = (uint8_t)rgb;
    return 1;
}
unsigned gallery_color_rgba(void) {
    return ((unsigned)chosen_color.r << 24) | ((unsigned)chosen_color.g << 16) |
           ((unsigned)chosen_color.b << 8) | chosen_color.a;
}
void gallery_toolbar_options(int size, int grouped, int show_input) {
    toolbar_size =
        size >= CUI_TOOLBAR_SMALL && size <= CUI_TOOLBAR_CUSTOM ? size : CUI_TOOLBAR_MEDIUM;
    toolbar_grouped = grouped != 0;
    toolbar_show_input = show_input != 0;
}
unsigned gallery_toolbar_activations(unsigned index) {
    return index < 4 ? toolbar_activations[index] : index == 4 ? toolbar_query_changes : 0;
}
unsigned gallery_toolbar_cursor(void) {
    return toolbar_query ? (unsigned)cui_edit_get_state(toolbar_query).cursor : 0;
}
void gallery_title_options(int decoration, int override_close, int fullscreen) {
    title_decoration =
        decoration >= CUI_WINDOW_BROWSER && decoration <= CUI_WINDOW_NATIVE_TRAFFIC_LIGHTS
            ? decoration
            : CUI_WINDOW_BROWSER;
    title_override = override_close != 0;
    title_fullscreen = fullscreen != 0;
}
unsigned gallery_title_action(unsigned index) {
    return index < 7 ? title_actions[index] : index == 7 ? title_closed : title_maximized;
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
    case 22:
        return c->text_style.weight;
    case 23:
        return c->text_style.italic;
    case 24:
        return c->text_style.monospace;
    case 25:
        return c->text_paint.kind;
    case 26:
        return c->text_paint.x0;
    case 27:
        return c->text_paint.y0;
    case 28:
        return c->text_paint.x1;
    case 29:
        return c->text_paint.y1;
    case 30:
        return (float)c->text_paint.stop_count;
    default:
        if (field >= 31 && field < 71) {
            size_t stop = (field - 31) / 5;
            if (stop >= c->text_paint.stop_count || !c->text_paint.stops)
                return 0;
            cui_text_gradient_stop s = c->text_paint.stops[stop];
            const float values[] = {s.position, s.color.r, s.color.g, s.color.b, s.color.a};
            return values[(field - 31) % 5];
        }
        return 0;
    }
}
float gallery_shadow_value(unsigned i, unsigned field) {
    if (i >= draw.count ||
        (draw.commands[i].type != CUI_DRAW_DROP_SHADOW &&
         draw.commands[i].type != CUI_DRAW_INSET_SHADOW) ||
        field >= 16)
        return 0;
    const cui_drop_shadow_paint *p = &draw.commands[i].shadow;
    const float values[] = {
        p->caster.x,       p->caster.y,        p->caster.w,          p->caster.h,
        p->radii.top_left, p->radii.top_right, p->radii.bottom_left, p->radii.bottom_right,
        p->layer.offset_x, p->layer.offset_y,  p->layer.sigma,       p->layer.spread,
        p->layer.color.r,  p->layer.color.g,   p->layer.color.b,     p->layer.color.a};
    return values[field];
}
const char *gallery_command_text(unsigned i) {
    return i < draw.count ? draw.commands[i].text.data : NULL;
}
uint32_t gallery_command_family(unsigned i) {
    return i < draw.count ? draw.commands[i].text_style.family : 0;
}
unsigned gallery_command_length(unsigned i) {
    return i < draw.count ? (unsigned)draw.commands[i].text.length : 0;
}
unsigned gallery_bitmap_pixels(unsigned i) {
    return i < draw.count && draw.commands[i].type == CUI_DRAW_IMAGE
               ? (unsigned)(uintptr_t)draw.commands[i].bitmap.bitmap.pixels
               : 0;
}
unsigned gallery_bitmap_revision(unsigned i, unsigned high) {
    if (i >= draw.count || draw.commands[i].type != CUI_DRAW_IMAGE)
        return 0;
    uint64_t revision = draw.commands[i].bitmap.bitmap.revision;
    return (unsigned)(high ? revision >> 32 : revision);
}
double gallery_bitmap(unsigned i, unsigned field) {
    if (i >= draw.count || draw.commands[i].type != CUI_DRAW_IMAGE)
        return 0;
    const cui_bitmap_paint *paint = &draw.commands[i].bitmap;
    switch (field) {
    case 0:
        return paint->bitmap.width;
    case 1:
        return paint->bitmap.height;
    case 2:
        return (double)paint->bitmap.stride;
    case 3:
        return paint->source.x;
    case 4:
        return paint->source.y;
    case 5:
        return paint->source.w;
    case 6:
        return paint->source.h;
    case 7:
        return paint->opacity;
    case 8:
        return paint->grayscale;
    case 9:
        return paint->custom_tint;
    case 10:
        return paint->tint.r;
    case 11:
        return paint->tint.g;
    case 12:
        return paint->tint.b;
    case 13:
        return paint->tint.a;
    default:
        return 0;
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
    case 35:
        return s->details.handles_tab;
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
    if (mode >= 0 && mode <= 4)
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
double gallery_sheet_motion(unsigned field) {
    return field == 0 ? sheet_surface.sheet_progress : sheet_surface.sheet_closing;
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
    if (mode < 0 || mode > 9 || (mode == 9 && !input_regex_ready))
        mode = 0;
    if (input_mode != mode)
        input_rejected = false;
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
    return i < semantic_count ? cui_semantic_parent(ui, semantics[i].id) : 0;
}
uint64_t gallery_semantic_controls(unsigned i) {
    return i < semantic_count ? semantics[i].details.controls : 0;
}
uint64_t gallery_semantic_labelled_by(unsigned i) {
    return i < semantic_count ? semantics[i].details.labelled_by : 0;
}

static void gallery_progress_content(cui_context *ui, void *user) {
    float value = *(float *)user;
    char text[16];
    snprintf(text, sizeof text, "%.0f%%", (double)value);
    float font = 14;
    cui_design_metrics metrics;
    if (design_size && cui_design_measure(design_size, current_theme.font_size / 16, &metrics))
        font = metrics.font_size;
    cui_text_with_line_height(ui, cui_string(text), font, current_theme.text, font);
}
void gallery_progress_shape(int shape) {
    progress_shape = shape >= 0 && shape <= 2 ? shape : 0;
}
void gallery_progress_mode(int mode) {
    progress_indeterminate = mode != 0;
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

void gallery_avatar_options(int source, int palette, int hue, int placeholder, float rem) {
    avatar_source = source >= 0 && source <= 2 ? source : 1;
    avatar_palette = palette != 0;
    avatar_hue = hue >= 0 && hue < 12 ? hue : -1;
    avatar_placeholder = placeholder >= 0 && placeholder <= 2 ? placeholder : 0;
    avatar_rem = isfinite(rem) && rem > 0 && rem <= 512 ? rem : 16;
}
float gallery_avatar_metric(int size, float rem, int field) {
    cui_avatar_metrics metrics;
    if (!cui_avatar_measure((cui_design_size)size, rem, &metrics))
        return field ? 48 * .38f : 48;
    return field ? metrics.font_size : metrics.diameter;
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
void gallery_design_options(int size, int radius, float spacing) {
    design_size = size >= CUI_DESIGN_INHERIT && size <= CUI_DESIGN_9XL ? (cui_design_size)size
                                                                       : CUI_DESIGN_INHERIT;
    design_radius = radius >= CUI_RADIUS_NONE && radius <= CUI_RADIUS_FULL ? radius : -1;
    design_spacing = isfinite(spacing) && spacing >= 0 && spacing <= 16 ? spacing : -1;
}
void gallery_control_options(int size, int required, int invalid, int skip_tab, int label_left,
                             int custom_color, int variant, int segmented, int multiple) {
    control_size =
        size >= CUI_CONTROL_MEDIUM && size <= CUI_CONTROL_XSMALL ? size : CUI_CONTROL_MEDIUM;
    choice_required = required;
    choice_invalid = invalid;
    choice_skip_tab = skip_tab;
    switch_label_left = label_left;
    switch_custom_color = custom_color;
    toggle_variant = variant;
    toggle_segmented = segmented;
    if (toggle_selection_mode && !multiple) {
        bool selected = false;
        for (size_t i = 0; i < 3; i++) {
            bool retain = toggle_group_values[i] && !selected;
            toggle_group_values[i] = retain;
            selected |= retain;
        }
    }
    toggle_selection_mode = multiple;
}
double gallery_control_stats(unsigned field) {
    switch (field) {
    case 0:
        return gallery_switch_state.position;
    case 1:
        return gallery_switch_state.target;
    case 2:
        return gallery_switch_state.next_wake_seconds;
    case 3:
        return toggle_group_changes;
    case 4:
        return toggle_group_values[0];
    case 5:
        return toggle_group_values[1];
    case 6:
        return toggle_group_values[2];
    default:
        return 0;
    }
}
void gallery_image_options(int mode, int fit, int grayscale, int half_opacity) {
    image_mode = mode;
    image_fit = fit;
    image_grayscale = grayscale;
    image_half_opacity = half_opacity;
}
double gallery_image_stats(unsigned field) {
    switch (field) {
    case 0:
        return image_result.valid;
    case 1:
        return image_result.status;
    case 2:
        return image_result.loading_visible;
    case 3:
        return image_state.frame;
    case 4:
        return image_result.next_wake_seconds;
    default:
        return 0;
    }
}
void gallery_attachment_options(int status, int vertical) {
    attachment_status = status;
    attachment_vertical = vertical;
}
double gallery_attachment_stats(unsigned field) {
    switch (field) {
    case 0:
        return attachment_clicks;
    case 1:
        return attachment_removes;
    case 2:
        return attachment_retries;
    case 3:
        return attachment_actions;
    case 4:
        return attachment_removed_mask;
    case 5:
        return attachment_group_result.maximum;
    case 6:
        return attachment_group_state.scroll.x;
    case 7:
        return attachment_group_result.leading_opacity;
    case 8:
        return attachment_group_result.trailing_opacity;
    case 9:
        return attachment_status;
    default:
        return 0;
    }
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

/* Host wheel claim is Plot-only. Core performs the authoritative hit/consumption
 * during dispatch; the browser preflight follows the topmost semantic surface. */
void gallery_plot_wheel(float x, float y, float delta) {
    if (ui)
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                        .x = x,
                                        .y = y,
                                        .dy = -delta,
                                        .scroll_precise = true,
                                        .modifiers = CUI_MOD_CTRL});
}
double gallery_plot_domain(int axis, int end) {
    const cui_plot_scale *s = axis ? &plot_viewport.y : &plot_viewport.x;
    return end ? s->domain_max : s->domain_min;
}
