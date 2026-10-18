/* SPDX-License-Identifier: MIT */
#include "cui/cui_media.h"
#include "cui/cui_message.h"
#include "cui_bridge_internal.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct bridge_text_view_owned {
    cui_text_view_state state;
    char *source, *rendered;
    cui_text_view_mapping *mapping;
    cui_text_view_span *spans;
    cui_text_view_block *blocks;
    cui_text_view_glyph *glyphs;
};
struct bridge_chart_owned {
    cui_chart_state state;
    cui_chart_datum data[64];
    char *labels[64];
    uint32_t count, kind;
    unsigned width, height;
};
struct bridge_speech_owned {
    cui_speech_state state;
    cui_speech_sink sink;
    char transcript[4097];
    cui_speech_message messages[64];
    unsigned char queue[8192];
    cui_bridge_node_record *node;
};
struct bridge_questionnaire_owned {
    cui_questionnaire_state state;
    cui_questionnaire_item items[16];
    cui_questionnaire_choice choices[256];
    cui_questionnaire_item_runtime item_runtime[16];
    cui_questionnaire_choice_runtime choice_runtime[256];
    uint32_t question_slots[16], choice_slots[256];
    uint32_t count, choice_count, last_item, last_event;
    cui_bridge_node_record *node;
};
struct bridge_dock_owned {
    cui_dock *model;
    cui_dock_view *view;
    cui_dock_panel panels[64];
    uint32_t slots[64], count;
    cui_bridge_node_record *node;
    bool initialized;
};
struct bridge_settings_owned {
    cui_settings_state state;
    cui_settings_page_state page_state[8];
    cui_setting_page pages[8];
    cui_setting_group groups[32];
    cui_setting_item items[128];
    uint32_t page_slots[8], group_slots[32], field_slots[128];
    uint8_t field_pages[128], field_groups[128], field_items[128];
    char *defaults[128];
    uint32_t page_count, group_count, field_count;
    char query[257];
    cui_bridge_node_record *node;
    bool initialized;
};
static void bridge_change(cui_bridge_app *app, uint32_t slot);
static void bridge_settings_field_change(void *user) {
    cui_bridge_node_record *field = user;
    bridge_change(field->owner, field->own_slot);
}
static void bridge_questionnaire_event(const cui_questionnaire_state *state,
                                       cui_questionnaire_event event, void *user) {
    bridge_questionnaire_owned *owned = user;
    cui_bridge_node_record *node = owned->node;
    owned->last_item = event.item < UINT32_MAX ? (uint32_t)event.item : UINT32_MAX;
    owned->last_event = (uint32_t)event.kind;
    node->value = event.kind;
    node->checked = state->complete;
    bridge_change(node->owner, node->own_slot);
}
static cui_speech_error bridge_speech_recognizer_start(void *user, cui_speech_sink sink,
                                                       void **session) {
    bridge_speech_owned *speech = user;
    speech->sink = sink;
    *session = speech;
    return CUI_SPEECH_OK;
}
static void bridge_speech_audio(void *user, void *session, const int16_t *samples, size_t count) {
    (void)user;
    (void)session;
    (void)samples;
    (void)count;
}
static void bridge_speech_finish_provider(void *user, void *session) {
    (void)user;
    (void)session;
}
static void bridge_speech_close_provider(void *user, void *session) {
    (void)user;
    (void)session;
}
static cui_speech_error bridge_speech_input_start(void *user, cui_speech_format format,
                                                  cui_speech_sink sink, void **capture) {
    (void)format;
    (void)sink;
    *capture = user;
    return CUI_SPEECH_OK;
}
static void bridge_speech_event(void *user, cui_speech_event event) {
    bridge_speech_owned *speech = user;
    cui_bridge_node_record *node = speech->node;
    node->checked = speech->state.status != CUI_SPEECH_IDLE;
    node->value = event.type;
    bridge_change(node->owner, node->own_slot);
}
static bridge_speech_owned *bridge_speech_create(cui_bridge_node_record *node) {
    bridge_speech_owned *speech = calloc(1, sizeof *speech);
    if (!speech)
        return NULL;
    speech->node = node;
    cui_speech_workspace workspace = {.transcript = speech->transcript,
                                      .transcript_capacity = sizeof speech->transcript,
                                      .messages = speech->messages,
                                      .message_capacity = 64,
                                      .queue_bytes = speech->queue,
                                      .queue_capacity = sizeof speech->queue};
    cui_speech_recognizer recognizer = {.user = speech,
                                        .start = bridge_speech_recognizer_start,
                                        .push_audio = bridge_speech_audio,
                                        .finish = bridge_speech_finish_provider,
                                        .close = bridge_speech_close_provider};
    cui_speech_input input = {
        .user = speech, .start = bridge_speech_input_start, .close = bridge_speech_close_provider};
    if (!cui_speech_init(&speech->state, workspace, recognizer, input, bridge_speech_event,
                         speech)) {
        free(speech);
        return NULL;
    }
    return speech;
}
struct bridge_data_table_owned {
    cui_table_model *model;
    cui_table_state state;
    cui_table_row rows[32];
    cui_table_column columns[16];
    char *headers[16];
    char *cells[512];
    uint32_t row_count, column_count;
};
static void bridge_data_table_free(bridge_data_table_owned *table) {
    if (!table)
        return;
    cui_table_destroy(table->model);
    for (size_t i = 0; i < 16; i++)
        free(table->headers[i]);
    for (size_t i = 0; i < 512; i++)
        free(table->cells[i]);
    free(table);
}
static void bridge_chart_free(bridge_chart_owned *chart) {
    if (!chart)
        return;
    for (size_t i = 0; i < chart->count; i++)
        free(chart->labels[i]);
    free(chart->state.pixels);
    free(chart);
}
static void bridge_text_view_free(bridge_text_view_owned *owned) {
    if (!owned)
        return;
    free(owned->source);
    free(owned->rendered);
    free(owned->mapping);
    free(owned->spans);
    free(owned->blocks);
    free(owned->glyphs);
    free(owned);
}
static bridge_text_view_owned *bridge_text_view_create(void) {
    bridge_text_view_owned *owned = calloc(1, sizeof *owned);
    if (!owned)
        return NULL;
    owned->source = calloc(4097, 1);
    owned->rendered = calloc(8192, 1);
    owned->mapping = calloc(8192, sizeof *owned->mapping);
    owned->spans = calloc(2048, sizeof *owned->spans);
    owned->blocks = calloc(1024, sizeof *owned->blocks);
    owned->glyphs = calloc(8192, sizeof *owned->glyphs);
    if (!owned->source || !owned->rendered || !owned->mapping || !owned->spans || !owned->blocks ||
        !owned->glyphs ||
        !cui_text_view_init(&owned->state,
                            (cui_text_view_workspace){.source = owned->source,
                                                      .source_capacity = 4097,
                                                      .rendered = owned->rendered,
                                                      .rendered_capacity = 8192,
                                                      .mapping = owned->mapping,
                                                      .mapping_capacity = 8192,
                                                      .spans = owned->spans,
                                                      .span_capacity = 2048,
                                                      .blocks = owned->blocks,
                                                      .block_capacity = 1024,
                                                      .glyphs = owned->glyphs,
                                                      .glyph_capacity = 8192},
                            CUI_TEXT_VIEW_MARKDOWN)) {
        bridge_text_view_free(owned);
        return NULL;
    }
    return owned;
}

static void bridge_notification_click(void *user, cui_notification_id id);
static void bridge_notification_close(void *user, cui_notification_id id);
static cui_notification_entry *bridge_notification_entry(cui_bridge_app *app, cui_id id) {
    if (!app->notifications)
        return NULL;
    for (size_t i = 0; i < app->notifications->count; i++)
        if (app->notifications->entries[i].notification.id.key == id)
            return &app->notifications->entries[i];
    return NULL;
}

#ifndef CUI_BRIDGE_WITH_SDL
cui_bridge_status cui_bridge_native_run(cui_bridge_app *app, cui_bridge_run_callback callback,
                                        void *user) {
    (void)app;
    (void)callback;
    (void)user;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_create(cui_bridge_app *app, const char *title,
                                           const char *font_path, cui_config *config) {
    (void)app;
    (void)title;
    (void)font_path;
    (void)config;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
void cui_bridge_native_destroy(cui_bridge_app *app) {
    (void)app;
}
cui_bridge_status cui_bridge_native_poll(cui_bridge_app *app) {
    (void)app;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_render(cui_bridge_app *app, const cui_draw_list *list) {
    (void)app;
    (void)list;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_set_font_fallbacks(cui_bridge_app *app,
                                                       const char *const *paths, uint32_t count) {
    (void)app;
    (void)paths;
    (void)count;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_clipboard_set(cui_bridge_app *app, cui_str text) {
    (void)app;
    (void)text;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
cui_bridge_status cui_bridge_native_clipboard_get(cui_bridge_app *app, char *out, uint32_t capacity,
                                                  uint32_t *length) {
    (void)app;
    (void)out;
    (void)capacity;
    (void)length;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
#endif

static cui_size bridge_measure(void *user, cui_str text, float size) {
    (void)user;
    return (cui_size){(float)text.length * size * .55f, size};
}
static cui_bridge_status bridge_utf8_limit(const char *s, size_t maximum, size_t *length) {
    if (!s)
        return CUI_BRIDGE_INVALID_UTF8;
    size_t n = 0;
    while (s[n] && n <= maximum)
        n++;
    if (n > maximum)
        return CUI_BRIDGE_CAPACITY;
    for (size_t i = 0; i < n;) {
        unsigned c = (unsigned char)s[i++];
        if (c < 0x80)
            continue;
        unsigned extra;
        uint32_t scalar, minimum;
        if (c >= 0xc2 && c <= 0xdf) {
            extra = 1;
            scalar = c & 31;
            minimum = 0x80;
        } else if (c >= 0xe0 && c <= 0xef) {
            extra = 2;
            scalar = c & 15;
            minimum = 0x800;
        } else if (c >= 0xf0 && c <= 0xf4) {
            extra = 3;
            scalar = c & 7;
            minimum = 0x10000;
        } else
            return CUI_BRIDGE_INVALID_UTF8;
        if (extra > n - i)
            return CUI_BRIDGE_INVALID_UTF8;
        while (extra--) {
            unsigned b = (unsigned char)s[i++];
            if ((b & 0xc0) != 0x80)
                return CUI_BRIDGE_INVALID_UTF8;
            scalar = (scalar << 6) | (b & 63);
        }
        if (scalar < minimum || scalar > 0x10ffff || (scalar >= 0xd800 && scalar <= 0xdfff))
            return CUI_BRIDGE_INVALID_UTF8;
    }
    *length = n;
    return CUI_BRIDGE_OK;
}
static bool bridge_utf8(const char *s, size_t *length) {
    return bridge_utf8_limit(s, 4096, length) == CUI_BRIDGE_OK;
}
static cui_bridge_status bridge_copy(const char *s, char **out) {
    size_t n = 0;
    if (!bridge_utf8(s, &n))
        return CUI_BRIDGE_INVALID_UTF8;
    char *copy = malloc(n + 1);
    if (!copy)
        return CUI_BRIDGE_OUT_OF_MEMORY;
    memcpy(copy, s, n + 1);
    *out = copy;
    return CUI_BRIDGE_OK;
}
static cui_bridge_status bridge_ready(cui_bridge_app *app) {
    if (!app)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (app->closed)
        return CUI_BRIDGE_CLOSED;
    if (app->stepping)
        return CUI_BRIDGE_INVALID_STATE;
    return CUI_BRIDGE_OK;
}
static cui_bridge_node_record *bridge_node(cui_bridge_app *app, cui_bridge_node handle) {
    if (!handle.slot || handle.slot > app->max_nodes)
        return NULL;
    cui_bridge_node_record *node = &app->nodes[handle.slot];
    return node->live && node->generation == handle.generation ? node : NULL;
}
static cui_bridge_status bridge_valid_node(cui_bridge_app *app, cui_bridge_node handle,
                                           cui_bridge_node_record **out) {
    cui_bridge_status ready = bridge_ready(app);
    if (ready != CUI_BRIDGE_OK)
        return ready;
    cui_bridge_node_record *node = bridge_node(app, handle);
    if (!node)
        return CUI_BRIDGE_STALE_HANDLE;
    *out = node;
    return CUI_BRIDGE_OK;
}
uint32_t cui_bridge_abi_version(void) {
    return CUI_BRIDGE_ABI_VERSION;
}
uint32_t cui_bridge_options_size(void) {
    return (uint32_t)sizeof(cui_bridge_options);
}
const char *cui_bridge_status_message(cui_bridge_status status) {
    switch (status) {
    case CUI_BRIDGE_OK:
        return "ok";
    case CUI_BRIDGE_EMPTY:
        return "empty";
    case CUI_BRIDGE_CLOSED:
        return "closed";
    case CUI_BRIDGE_INVALID_ARGUMENT:
        return "invalid argument";
    case CUI_BRIDGE_STALE_HANDLE:
        return "stale handle";
    case CUI_BRIDGE_CAPACITY:
        return "capacity";
    case CUI_BRIDGE_OUT_OF_MEMORY:
        return "out of memory";
    case CUI_BRIDGE_INVALID_UTF8:
        return "invalid UTF-8";
    case CUI_BRIDGE_INVALID_STATE:
        return "invalid state";
    case CUI_BRIDGE_BACKEND_UNAVAILABLE:
        return "backend unavailable";
    case CUI_BRIDGE_CORE_ERROR:
        return "Core error";
    }
    return "unknown status";
}
cui_bridge_app *cui_bridge_create(const cui_bridge_options *options,
                                  cui_bridge_status *out_status) {
    cui_bridge_status status = CUI_BRIDGE_INVALID_ARGUMENT;
    if (out_status)
        *out_status = status;
    if (!options || options->struct_size != sizeof(*options) ||
        options->abi_version != CUI_BRIDGE_ABI_VERSION ||
        (options->backend != CUI_BRIDGE_BACKEND_NONE &&
         options->backend != CUI_BRIDGE_BACKEND_SDL) ||
        !isfinite(options->width) || options->width <= 0 || options->width > 32768 ||
        !isfinite(options->height) || options->height <= 0 || options->height > 32768)
        return NULL;
    size_t text_length = 0;
    if ((options->title_utf8 && !bridge_utf8(options->title_utf8, &text_length)) ||
        (options->font_path_utf8 && !bridge_utf8(options->font_path_utf8, &text_length))) {
        if (out_status)
            *out_status = CUI_BRIDGE_INVALID_UTF8;
        return NULL;
    }
    uint32_t max_nodes = options->max_nodes ? options->max_nodes : 256;
    uint32_t max_events = options->max_events ? options->max_events : 256;
    if (max_nodes > 1024 || max_events > 4096 || !max_nodes || !max_events) {
        if (out_status)
            *out_status = CUI_BRIDGE_CAPACITY;
        return NULL;
    }
    cui_bridge_app *app = calloc(1, sizeof(*app));
    if (!app) {
        if (out_status)
            *out_status = CUI_BRIDGE_OUT_OF_MEMORY;
        return NULL;
    }
    app->nodes = calloc((size_t)max_nodes + 1, sizeof(*app->nodes));
    app->events = calloc(max_events, sizeof(*app->events));
    app->event_payloads = calloc(max_events, sizeof(*app->event_payloads));
    if (!app->nodes || !app->events || !app->event_payloads) {
        status = CUI_BRIDGE_OUT_OF_MEMORY;
        goto failed;
    }
    app->max_nodes = max_nodes;
    app->max_events = max_events;
    app->width = options->width;
    app->height = options->height;
    app->backend = options->backend;
    app->next_id = 2;
    app->theme = cui_theme_light();
    app->theme_preset_index = UINT32_MAX;
    if (!cui_root_init(&app->root_view, NULL, 0, NULL, NULL)) {
        status = CUI_BRIDGE_CORE_ERROR;
        goto failed;
    }
    cui_config config = {.width = app->width, .height = app->height, .measure = bridge_measure};
    if (app->backend == CUI_BRIDGE_BACKEND_SDL) {
        status =
            cui_bridge_native_create(app, options->title_utf8, options->font_path_utf8, &config);
        if (status != CUI_BRIDGE_OK)
            goto failed;
    }
    app->ui = cui_create(config);
    if (!app->ui) {
        status = CUI_BRIDGE_OUT_OF_MEMORY;
        goto failed;
    }
    if (out_status)
        *out_status = CUI_BRIDGE_OK;
    return app;
failed:
    cui_bridge_destroy(app);
    if (out_status)
        *out_status = status;
    return NULL;
}
static void bridge_free_node_resources(cui_bridge_node_record *node) {
    free(node->key);
    free(node->text);
    free(node->detail);
    cui_edit_destroy(node->edit);
    free(node->edit_buffer);
    free(node->textarea_lines);
    free(node->image_pixels);
    cui_tree_destroy(node->tree_model);
    free(node->tree_nodes);
    free(node->tree_expanded);
    free(node->otp_buffer);
    cui_message_scroller_destroy(node->message_scroller);
    free(node->carousel_geometry);
    free(node->carousel_state);
    free(node->resizable_slots);
    free(node->resizable_state);
    bridge_text_view_free(node->text_view);
    bridge_chart_free(node->chart);
    bridge_data_table_free(node->data_table);
    free(node->plot_samples);
    free(node->command_rows);
    free(node->command_entries);
    free(node->command_keywords);
    free(node->command_slots);
    cui_virtual_destroy(node->virtual_list);
    free(node->sidebar_cache);
    if (node->speech) {
        cui_speech_dispose(&node->speech->state);
        free(node->speech);
    }
    free(node->questionnaire);
    if (node->dock) {
        cui_dock_view_destroy(node->dock->view);
        cui_dock_destroy(node->dock->model);
        free(node->dock);
    }
    if (node->settings) {
        for (size_t i = 0; i < 128; i++)
            free(node->settings->defaults[i]);
        free(node->settings);
    }
    node->key = node->text = node->detail = NULL;
    node->edit = NULL;
    node->edit_buffer = NULL;
    node->textarea_lines = NULL;
    node->image_pixels = NULL;
    node->tree_model = NULL;
    node->tree_nodes = NULL;
    node->tree_expanded = NULL;
    node->otp_buffer = NULL;
    node->message_scroller = NULL;
    node->carousel_geometry = NULL;
    node->carousel_state = NULL;
    node->resizable_slots = NULL;
    node->resizable_state = NULL;
    node->text_view = NULL;
    node->chart = NULL;
    node->data_table = NULL;
    node->plot_samples = NULL;
    node->plot_count = 0;
    node->command_rows = NULL;
    node->command_entries = NULL;
    node->command_keywords = NULL;
    node->command_slots = NULL;
    node->virtual_list = NULL;
    node->sidebar_cache = NULL;
    node->speech = NULL;
    node->questionnaire = NULL;
    node->dock = NULL;
    node->settings = NULL;
}
uint32_t cui_bridge_theme_preset_count(void) {
    size_t count = cui_theme_preset_count();
    return count > UINT32_MAX ? UINT32_MAX : (uint32_t)count;
}
cui_bridge_status cui_bridge_theme_preset_name(uint32_t index, char *out_utf8, uint32_t capacity,
                                               uint32_t *out_length) {
    if (!out_length || index >= cui_bridge_theme_preset_count())
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_str name = cui_theme_preset_name(index);
    if (!name.data || name.length > UINT32_MAX)
        return CUI_BRIDGE_CORE_ERROR;
    if (!out_utf8 || capacity <= name.length)
        return CUI_BRIDGE_CAPACITY;
    memcpy(out_utf8, name.data, name.length);
    out_utf8[name.length] = '\0';
    *out_length = (uint32_t)name.length;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_theme_preset(cui_bridge_app *app, uint32_t index) {
    if (!app || index >= cui_bridge_theme_preset_count())
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_theme theme;
    if (!cui_theme_preset_get(index, &theme))
        return CUI_BRIDGE_CORE_ERROR;
    app->theme = theme;
    app->theme_preset_index = index;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_get_theme_preset(const cui_bridge_app *app, uint32_t *out_index) {
    if (!app || !out_index)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (app->theme_preset_index == UINT32_MAX)
        return CUI_BRIDGE_EMPTY;
    *out_index = app->theme_preset_index;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_clipboard_set(cui_bridge_app *app, const char *text_utf8) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    size_t length = 0;
    status = bridge_utf8_limit(text_utf8, 65536, &length);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (app->backend != CUI_BRIDGE_BACKEND_SDL)
        return CUI_BRIDGE_BACKEND_UNAVAILABLE;
    return cui_bridge_native_clipboard_set(app, (cui_str){text_utf8, length});
}
cui_bridge_status cui_bridge_clipboard_get(cui_bridge_app *app, char *out_utf8, uint32_t capacity,
                                           uint32_t *out_length) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out_utf8 || !out_length || !capacity)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (app->backend != CUI_BRIDGE_BACKEND_SDL)
        return CUI_BRIDGE_BACKEND_UNAVAILABLE;
    return cui_bridge_native_clipboard_get(app, out_utf8, capacity, out_length);
}
static cui_bridge_status bridge_speech_node(cui_bridge_app *app, cui_bridge_node handle,
                                            cui_bridge_node_record **out) {
    cui_bridge_status status = bridge_valid_node(app, handle, out);
    if (status != CUI_BRIDGE_OK)
        return status;
    return (*out)->kind == CUI_BRIDGE_SPEECH && (*out)->speech ? CUI_BRIDGE_OK
                                                               : CUI_BRIDGE_INVALID_STATE;
}
cui_bridge_status cui_bridge_speech_start(cui_bridge_app *app, cui_bridge_node handle) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_speech_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->disabled || node->speech->state.status != CUI_SPEECH_IDLE)
        return CUI_BRIDGE_INVALID_STATE;
    if (app->event_count == app->max_events)
        return CUI_BRIDGE_CAPACITY;
    return cui_speech_start(&node->speech->state) ? CUI_BRIDGE_OK : CUI_BRIDGE_INVALID_STATE;
}
cui_bridge_status cui_bridge_speech_report(cui_bridge_app *app, cui_bridge_node handle,
                                           const char *text_utf8, uint32_t final_phrase) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_speech_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    size_t length = 0;
    if (!bridge_utf8(text_utf8, &length))
        return CUI_BRIDGE_INVALID_UTF8;
    if (final_phrase > 1 || length > 4096)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    bool queued =
        final_phrase
            ? cui_speech_report_phrase(node->speech->sink, (cui_str){text_utf8, length})
            : cui_speech_report_hypothesis(node->speech->sink, (cui_str){text_utf8, length});
    return queued ? CUI_BRIDGE_OK : CUI_BRIDGE_INVALID_STATE;
}
cui_bridge_status cui_bridge_speech_finish(cui_bridge_app *app, cui_bridge_node handle) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_speech_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    return cui_speech_report_finish(node->speech->sink) ? CUI_BRIDGE_OK : CUI_BRIDGE_INVALID_STATE;
}
cui_bridge_status cui_bridge_speech_transcript(cui_bridge_app *app, cui_bridge_node handle,
                                               char *out_utf8, uint32_t capacity,
                                               uint32_t *out_length) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_speech_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out_utf8 || !out_length || !capacity)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_str value = cui_speech_transcript(&node->speech->state);
    if (value.length >= capacity)
        return CUI_BRIDGE_CAPACITY;
    memcpy(out_utf8, value.data, value.length);
    out_utf8[value.length] = '\0';
    *out_length = (uint32_t)value.length;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_questionnaire_set_question(cui_bridge_app *app, cui_bridge_node handle,
                                                        uint32_t required, uint32_t multiple) {
    cui_bridge_node_record *question;
    cui_bridge_status status = bridge_valid_node(app, handle, &question);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (question->kind != CUI_BRIDGE_ITEM || !question->parent ||
        app->nodes[question->parent].kind != CUI_BRIDGE_QUESTIONNAIRE)
        return CUI_BRIDGE_INVALID_STATE;
    bridge_questionnaire_owned *owned = app->nodes[question->parent].questionnaire;
    if (owned->state.initialized)
        return CUI_BRIDGE_INVALID_STATE;
    if (required > 1 || multiple > 1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    question->step_status = required | (multiple << 1);
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_questionnaire_get_state(cui_bridge_app *app, cui_bridge_node handle,
                                                     cui_bridge_node *current, uint32_t *complete) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_QUESTIONNAIRE || !node->questionnaire ||
        !node->questionnaire->state.initialized)
        return CUI_BRIDGE_INVALID_STATE;
    if (!current || !complete)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    bridge_questionnaire_owned *owned = node->questionnaire;
    size_t index = owned->state.current;
    *current = index < owned->count
                   ? (cui_bridge_node){owned->question_slots[index],
                                       app->nodes[owned->question_slots[index]].generation}
                   : (cui_bridge_node){0};
    *complete = owned->state.complete;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_questionnaire_choice_selected(cui_bridge_app *app,
                                                           cui_bridge_node handle,
                                                           uint32_t *selected) {
    cui_bridge_node_record *choice;
    cui_bridge_status status = bridge_valid_node(app, handle, &choice);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!selected)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (choice->kind != CUI_BRIDGE_ITEM || !choice->parent)
        return CUI_BRIDGE_INVALID_STATE;
    cui_bridge_node_record *question = &app->nodes[choice->parent];
    if (question->kind != CUI_BRIDGE_ITEM || !question->parent)
        return CUI_BRIDGE_INVALID_STATE;
    cui_bridge_node_record *node = &app->nodes[question->parent];
    if (node->kind != CUI_BRIDGE_QUESTIONNAIRE || !node->questionnaire ||
        !node->questionnaire->state.initialized)
        return CUI_BRIDGE_INVALID_STATE;
    bridge_questionnaire_owned *owned = node->questionnaire;
    for (uint32_t q = 0; q < owned->count; q++)
        if (owned->question_slots[q] == choice->parent)
            for (uint32_t c = 0; c < owned->items[q].choice_count; c++)
                if (owned->choice_slots[owned->item_runtime[q].choice_offset + c] == handle.slot) {
                    *selected = cui_questionnaire_choice_selected(&owned->state, q, c);
                    return CUI_BRIDGE_OK;
                }
    return CUI_BRIDGE_INVALID_STATE;
}
static cui_bridge_status bridge_questionnaire_move(cui_bridge_app *app, cui_bridge_node handle,
                                                   bool next) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_QUESTIONNAIRE || !node->questionnaire ||
        !node->questionnaire->state.initialized || node->disabled)
        return CUI_BRIDGE_INVALID_STATE;
    if (app->event_count == app->max_events)
        return CUI_BRIDGE_CAPACITY;
    return (next ? cui_questionnaire_next(&node->questionnaire->state)
                 : cui_questionnaire_previous(&node->questionnaire->state))
               ? CUI_BRIDGE_OK
               : CUI_BRIDGE_INVALID_STATE;
}
cui_bridge_status cui_bridge_questionnaire_next(cui_bridge_app *app, cui_bridge_node handle) {
    return bridge_questionnaire_move(app, handle, true);
}
cui_bridge_status cui_bridge_questionnaire_previous(cui_bridge_app *app, cui_bridge_node handle) {
    return bridge_questionnaire_move(app, handle, false);
}
cui_bridge_status cui_bridge_questionnaire_submit(cui_bridge_app *app, cui_bridge_node handle) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_QUESTIONNAIRE || !node->questionnaire ||
        !node->questionnaire->state.initialized || node->disabled)
        return CUI_BRIDGE_INVALID_STATE;
    if (app->max_events - app->event_count < (node->questionnaire->state.complete ? 1u : 2u))
        return CUI_BRIDGE_CAPACITY;
    return cui_questionnaire_submit(&node->questionnaire->state) ? CUI_BRIDGE_OK
                                                                 : CUI_BRIDGE_INVALID_STATE;
}
static cui_bridge_status bridge_dock_panel(cui_bridge_app *app, cui_bridge_node dock_handle,
                                           cui_bridge_node panel_handle,
                                           cui_bridge_node_record **dock_out,
                                           cui_bridge_node_record **panel_out) {
    cui_bridge_node_record *dock, *panel;
    cui_bridge_status status = bridge_valid_node(app, dock_handle, &dock);
    if (status != CUI_BRIDGE_OK)
        return status;
    status = bridge_valid_node(app, panel_handle, &panel);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (dock->kind != CUI_BRIDGE_DOCK || !dock->dock || !dock->dock->initialized ||
        panel->kind != CUI_BRIDGE_ITEM || panel->parent != dock_handle.slot)
        return CUI_BRIDGE_INVALID_STATE;
    *dock_out = dock;
    *panel_out = panel;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_dock_activate(cui_bridge_app *app, cui_bridge_node dock_handle,
                                           cui_bridge_node panel_handle) {
    cui_bridge_node_record *dock, *panel;
    cui_bridge_status status = bridge_dock_panel(app, dock_handle, panel_handle, &dock, &panel);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (dock->disabled)
        return CUI_BRIDGE_INVALID_STATE;
    if ((uint32_t)dock->value != panel_handle.slot && app->event_count == app->max_events)
        return CUI_BRIDGE_CAPACITY;
    if (cui_dock_activate(dock->dock->model, panel->id) != CUI_DOCK_OK)
        return CUI_BRIDGE_INVALID_STATE;
    if ((uint32_t)dock->value != panel_handle.slot) {
        dock->value = panel_handle.slot;
        bridge_change(app, dock_handle.slot);
    }
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_dock_move_split(cui_bridge_app *app, cui_bridge_node dock_handle,
                                             cui_bridge_node panel_handle,
                                             cui_bridge_node reference_handle, uint32_t axis,
                                             double ratio, uint32_t before) {
    cui_bridge_node_record *dock, *panel, *reference;
    cui_bridge_status status = bridge_dock_panel(app, dock_handle, panel_handle, &dock, &panel);
    if (status != CUI_BRIDGE_OK)
        return status;
    status = bridge_dock_panel(app, dock_handle, reference_handle, &dock, &reference);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (dock->disabled || panel_handle.slot == reference_handle.slot)
        return CUI_BRIDGE_INVALID_STATE;
    if (axis > 1 || before > 1 || !isfinite(ratio) || ratio <= 0 || ratio >= 1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_dock_node tab;
    if (!cui_dock_panel_find(dock->dock->model, reference->id, &tab, NULL))
        return CUI_BRIDGE_INVALID_STATE;
    cui_dock_result result = cui_dock_move_split(
        dock->dock->model, panel->id, tab,
        axis == 0 ? CUI_DOCK_SPLIT_HORIZONTAL : CUI_DOCK_SPLIT_VERTICAL, ratio, before != 0, NULL);
    return result == CUI_DOCK_OK         ? CUI_BRIDGE_OK
           : result == CUI_DOCK_CAPACITY ? CUI_BRIDGE_CAPACITY
                                         : CUI_BRIDGE_INVALID_STATE;
}
cui_bridge_status cui_bridge_dock_get_active(cui_bridge_app *app, cui_bridge_node handle,
                                             cui_bridge_node *out_panel) {
    cui_bridge_node_record *dock;
    cui_bridge_status status = bridge_valid_node(app, handle, &dock);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out_panel)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (dock->kind != CUI_BRIDGE_DOCK || !dock->dock || !dock->dock->initialized)
        return CUI_BRIDGE_INVALID_STATE;
    uint32_t slot = (uint32_t)dock->value;
    *out_panel = slot && slot <= app->max_nodes && app->nodes[slot].live &&
                         app->nodes[slot].parent == handle.slot
                     ? (cui_bridge_node){slot, app->nodes[slot].generation}
                     : (cui_bridge_node){0};
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_dock_panel_bounds(cui_bridge_app *app, cui_bridge_node dock_handle,
                                               cui_bridge_node panel_handle,
                                               cui_bridge_rect *out_bounds) {
    cui_bridge_node_record *dock, *panel;
    cui_bridge_status status = bridge_dock_panel(app, dock_handle, panel_handle, &dock, &panel);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out_bounds)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_rect bounds = cui_bounds(app->ui, cui_dock_tab_id(dock->id, panel->id));
    if (!isfinite(bounds.x) || !isfinite(bounds.y) || bounds.w <= 0 || bounds.h <= 0)
        return CUI_BRIDGE_INVALID_STATE;
    *out_bounds = (cui_bridge_rect){bounds.x, bounds.y, bounds.w, bounds.h};
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_dock_serialize(cui_bridge_app *app, cui_bridge_node handle,
                                            char *destination, uint32_t capacity,
                                            uint32_t *required_capacity) {
    cui_bridge_node_record *dock;
    cui_bridge_status status = bridge_valid_node(app, handle, &dock);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!required_capacity)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (dock->kind != CUI_BRIDGE_DOCK || !dock->dock || !dock->dock->initialized)
        return CUI_BRIDGE_INVALID_STATE;
    size_t length = 0;
    cui_dock_result result = cui_dock_serialize(dock->dock->model, NULL, 0, &length);
    if (result != CUI_DOCK_OK || length >= UINT32_MAX)
        return CUI_BRIDGE_CORE_ERROR;
    *required_capacity = (uint32_t)length + 1;
    if (!destination || capacity <= length)
        return CUI_BRIDGE_CAPACITY;
    result = cui_dock_serialize(dock->dock->model, destination, capacity, &length);
    if (result != CUI_DOCK_OK)
        return result == CUI_DOCK_CAPACITY ? CUI_BRIDGE_CAPACITY : CUI_BRIDGE_CORE_ERROR;
    destination[length] = 0;
    return CUI_BRIDGE_OK;
}
static cui_bridge_status bridge_settings_node(cui_bridge_app *app, cui_bridge_node handle,
                                              cui_bridge_node_record **out) {
    cui_bridge_status status = bridge_valid_node(app, handle, out);
    if (status != CUI_BRIDGE_OK)
        return status;
    return (*out)->kind == CUI_BRIDGE_SETTINGS && (*out)->settings ? CUI_BRIDGE_OK
                                                                   : CUI_BRIDGE_INVALID_STATE;
}
cui_bridge_status cui_bridge_settings_set_query(cui_bridge_app *app, cui_bridge_node handle,
                                                const char *query_utf8) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_settings_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    size_t length = 0;
    status = bridge_utf8_limit(query_utf8, 256, &length);
    if (status != CUI_BRIDGE_OK)
        return status;
    memcpy(node->settings->query, query_utf8, length + 1);
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_settings_get_page(cui_bridge_app *app, cui_bridge_node handle,
                                               cui_bridge_node *out_page) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_settings_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out_page)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    bridge_settings_owned *owned = node->settings;
    if (!owned->initialized || owned->state.selected_page >= owned->page_count)
        return CUI_BRIDGE_INVALID_STATE;
    uint32_t slot = owned->page_slots[owned->state.selected_page];
    *out_page = (cui_bridge_node){slot, app->nodes[slot].generation};
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_settings_set_page(cui_bridge_app *app, cui_bridge_node handle,
                                               cui_bridge_node page_handle) {
    cui_bridge_node_record *node, *page;
    cui_bridge_status status = bridge_settings_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    status = bridge_valid_node(app, page_handle, &page);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_settings_owned *owned = node->settings;
    if (!owned->initialized || page->parent != handle.slot || node->disabled)
        return CUI_BRIDGE_INVALID_STATE;
    for (uint32_t i = 0; i < owned->page_count; i++)
        if (owned->page_slots[i] == page_handle.slot) {
            if (owned->state.selected_page != i) {
                if (app->event_count == app->max_events)
                    return CUI_BRIDGE_CAPACITY;
                owned->state.selected_page = i;
                owned->state.selected_group = CUI_SETTINGS_NO_GROUP;
                node->value = page_handle.slot;
                bridge_change(app, handle.slot);
            }
            return CUI_BRIDGE_OK;
        }
    return CUI_BRIDGE_INVALID_STATE;
}
cui_bridge_status cui_bridge_settings_reset_page(cui_bridge_app *app, cui_bridge_node handle,
                                                 cui_bridge_node page_handle) {
    cui_bridge_node_record *node, *page;
    cui_bridge_status status = bridge_settings_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    status = bridge_valid_node(app, page_handle, &page);
    if (status != CUI_BRIDGE_OK)
        return status;
    bridge_settings_owned *owned = node->settings;
    if (!owned->initialized || page->parent != handle.slot || node->disabled)
        return CUI_BRIDGE_INVALID_STATE;
    for (uint32_t i = 0; i < owned->page_count; i++)
        if (owned->page_slots[i] == page_handle.slot) {
            uint32_t events_needed = 0;
            const cui_setting_page *model = &owned->pages[i];
            for (size_t g = 0; g < model->count; g++)
                events_needed += (uint32_t)model->groups[g].count;
            if (app->max_events - app->event_count < events_needed)
                return CUI_BRIDGE_CAPACITY;
            (void)cui_settings_reset_page(&owned->pages[i], (cui_str){0});
            return CUI_BRIDGE_OK;
        }
    return CUI_BRIDGE_INVALID_STATE;
}
cui_bridge_status cui_bridge_settings_field_bounds(cui_bridge_app *app, cui_bridge_node handle,
                                                   cui_bridge_node field_handle,
                                                   cui_bridge_rect *out_bounds) {
    cui_bridge_node_record *node, *field;
    cui_bridge_status status = bridge_settings_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    status = bridge_valid_node(app, field_handle, &field);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out_bounds)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    bridge_settings_owned *owned = node->settings;
    if (!owned->initialized)
        return CUI_BRIDGE_INVALID_STATE;
    for (uint32_t i = 0; i < owned->field_count; i++)
        if (owned->field_slots[i] == field_handle.slot) {
            if (owned->state.selected_page != owned->field_pages[i])
                return CUI_BRIDGE_INVALID_STATE;
            cui_rect bounds = cui_bounds(
                app->ui, cui_settings_part_id(node->id, owned->field_pages[i],
                                              owned->field_groups[i], owned->field_items[i], 24));
            if (!isfinite(bounds.x) || !isfinite(bounds.y) || bounds.w <= 0 || bounds.h <= 0)
                return CUI_BRIDGE_INVALID_STATE;
            *out_bounds = (cui_bridge_rect){bounds.x, bounds.y, bounds.w, bounds.h};
            return CUI_BRIDGE_OK;
        }
    return CUI_BRIDGE_INVALID_STATE;
}
void cui_bridge_destroy(cui_bridge_app *app) {
    if (!app)
        return;
    if (app->running) {
        app->destroy_requested = true;
        app->closed = true;
        return;
    }
    if (app->nodes)
        for (uint32_t i = 1; i <= app->max_nodes; i++)
            bridge_free_node_resources(&app->nodes[i]);
    cui_root_dispose(&app->root_view);
    cui_destroy(app->ui);
    cui_bridge_native_destroy(app);
    if (app->event_payloads)
        for (uint32_t i = 0; i < app->max_events; i++)
            free(app->event_payloads[i].text);
    free(app->event_payloads);
    free(app->events);
    free(app->notification_entries);
    free(app->notifications);
    free(app->nodes);
    free(app);
}
cui_bridge_status cui_bridge_node_create(cui_bridge_app *app, cui_bridge_node parent,
                                         cui_bridge_kind kind, const char *key_utf8,
                                         cui_bridge_node *out_node) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out_node || kind < CUI_BRIDGE_ROW || kind > CUI_BRIDGE_SETTINGS)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    bool root = parent.slot == 0 && parent.generation == 0;
    cui_bridge_node_record *p = NULL;
    if (root) {
        if (app->root ||
            (kind != CUI_BRIDGE_ROW && kind != CUI_BRIDGE_COLUMN && kind != CUI_BRIDGE_ROOT_VIEW))
            return CUI_BRIDGE_INVALID_STATE;
    } else {
        p = bridge_node(app, parent);
        if (!p)
            return CUI_BRIDGE_STALE_HANDLE;
        if (kind == CUI_BRIDGE_ROOT_VIEW)
            return CUI_BRIDGE_INVALID_STATE;
        if (p->kind == CUI_BRIDGE_DOCK && p->dock && p->dock->initialized)
            return CUI_BRIDGE_INVALID_STATE;
        for (uint32_t ancestor = parent.slot; ancestor; ancestor = app->nodes[ancestor].parent)
            if (app->nodes[ancestor].kind == CUI_BRIDGE_SETTINGS && app->nodes[ancestor].settings &&
                app->nodes[ancestor].settings->initialized)
                return CUI_BRIDGE_INVALID_STATE;
        for (uint32_t ancestor = parent.slot; ancestor; ancestor = app->nodes[ancestor].parent)
            if (app->nodes[ancestor].kind == CUI_BRIDGE_QUESTIONNAIRE &&
                app->nodes[ancestor].questionnaire &&
                app->nodes[ancestor].questionnaire->state.initialized)
                return CUI_BRIDGE_INVALID_STATE;
        if (p->kind != CUI_BRIDGE_ROW && p->kind != CUI_BRIDGE_COLUMN &&
            p->kind != CUI_BRIDGE_ROOT_VIEW && p->kind != CUI_BRIDGE_QUESTIONNAIRE &&
            p->kind != CUI_BRIDGE_COLLAPSIBLE && p->kind != CUI_BRIDGE_ITEM &&
            p->kind != CUI_BRIDGE_RADIO_GROUP && p->kind != CUI_BRIDGE_TABS &&
            p->kind != CUI_BRIDGE_ACCORDION && p->kind != CUI_BRIDGE_SELECT &&
            p->kind != CUI_BRIDGE_COMBOBOX && p->kind != CUI_BRIDGE_DROPDOWN_BUTTON &&
            p->kind != CUI_BRIDGE_DIALOG && p->kind != CUI_BRIDGE_SHEET &&
            p->kind != CUI_BRIDGE_POPOVER && p->kind != CUI_BRIDGE_HOVER_CARD &&
            p->kind != CUI_BRIDGE_GROUP_BOX && p->kind != CUI_BRIDGE_LIST &&
            p->kind != CUI_BRIDGE_MENU && p->kind != CUI_BRIDGE_STEPPER &&
            p->kind != CUI_BRIDGE_DESCRIPTION_LIST && p->kind != CUI_BRIDGE_SCROLLABLE &&
            p->kind != CUI_BRIDGE_STATUS_BAR && p->kind != CUI_BRIDGE_TOOLBAR &&
            p->kind != CUI_BRIDGE_TITLE_BAR && p->kind != CUI_BRIDGE_TREE &&
            p->kind != CUI_BRIDGE_FORM && p->kind != CUI_BRIDGE_INPUT_GROUP &&
            p->kind != CUI_BRIDGE_FOCUS_TRAP && p->kind != CUI_BRIDGE_MESSAGE_SCROLLER &&
            p->kind != CUI_BRIDGE_CAROUSEL && p->kind != CUI_BRIDGE_RESIZABLE &&
            p->kind != CUI_BRIDGE_TABLE && p->kind != CUI_BRIDGE_COMMAND &&
            p->kind != CUI_BRIDGE_VIRTUAL_LIST && p->kind != CUI_BRIDGE_SIDEBAR &&
            p->kind != CUI_BRIDGE_DOCK && p->kind != CUI_BRIDGE_SETTINGS)
            return CUI_BRIDGE_INVALID_STATE;
        if (p->kind == CUI_BRIDGE_ITEM && p->parent &&
            app->nodes[p->parent].kind == CUI_BRIDGE_ITEM &&
            app->nodes[app->nodes[p->parent].parent].kind == CUI_BRIDGE_SETTINGS &&
            kind != CUI_BRIDGE_SWITCH && kind != CUI_BRIDGE_CHECKBOX && kind != CUI_BRIDGE_INPUT &&
            kind != CUI_BRIDGE_NUMBER_INPUT)
            return CUI_BRIDGE_INVALID_STATE;
        if (p->kind == CUI_BRIDGE_ITEM && p->parent &&
            (app->nodes[p->parent].kind == CUI_BRIDGE_TREE ||
             app->nodes[p->parent].kind == CUI_BRIDGE_INPUT_GROUP))
            return CUI_BRIDGE_INVALID_STATE;
        if (p->kind == CUI_BRIDGE_ITEM && p->parent &&
            app->nodes[p->parent].kind == CUI_BRIDGE_TABLE && kind != CUI_BRIDGE_ITEM)
            return CUI_BRIDGE_INVALID_STATE;
        if (p->kind == CUI_BRIDGE_ITEM && p->parent &&
            app->nodes[p->parent].kind == CUI_BRIDGE_SIDEBAR)
            return CUI_BRIDGE_INVALID_STATE;
        if (p->kind == CUI_BRIDGE_ITEM && p->parent &&
            app->nodes[p->parent].kind == CUI_BRIDGE_QUESTIONNAIRE && kind != CUI_BRIDGE_ITEM)
            return CUI_BRIDGE_INVALID_STATE;
        if (p->kind == CUI_BRIDGE_ITEM && p->parent &&
            app->nodes[p->parent].kind == CUI_BRIDGE_ITEM &&
            app->nodes[app->nodes[p->parent].parent].kind == CUI_BRIDGE_QUESTIONNAIRE)
            return CUI_BRIDGE_INVALID_STATE;
        if ((p->kind == CUI_BRIDGE_RADIO_GROUP || p->kind == CUI_BRIDGE_TABS ||
             p->kind == CUI_BRIDGE_ACCORDION || p->kind == CUI_BRIDGE_SELECT ||
             p->kind == CUI_BRIDGE_COMBOBOX || p->kind == CUI_BRIDGE_DROPDOWN_BUTTON ||
             p->kind == CUI_BRIDGE_LIST || p->kind == CUI_BRIDGE_MENU ||
             p->kind == CUI_BRIDGE_STEPPER || p->kind == CUI_BRIDGE_DESCRIPTION_LIST ||
             p->kind == CUI_BRIDGE_STATUS_BAR || p->kind == CUI_BRIDGE_TOOLBAR ||
             p->kind == CUI_BRIDGE_TITLE_BAR || p->kind == CUI_BRIDGE_TREE ||
             p->kind == CUI_BRIDGE_FORM || p->kind == CUI_BRIDGE_INPUT_GROUP ||
             p->kind == CUI_BRIDGE_MESSAGE_SCROLLER || p->kind == CUI_BRIDGE_CAROUSEL ||
             p->kind == CUI_BRIDGE_RESIZABLE || p->kind == CUI_BRIDGE_TABLE ||
             p->kind == CUI_BRIDGE_COMMAND || p->kind == CUI_BRIDGE_VIRTUAL_LIST ||
             p->kind == CUI_BRIDGE_SIDEBAR || p->kind == CUI_BRIDGE_QUESTIONNAIRE ||
             p->kind == CUI_BRIDGE_DOCK || p->kind == CUI_BRIDGE_SETTINGS ||
             (p->kind == CUI_BRIDGE_ITEM && p->parent &&
              app->nodes[p->parent].kind == CUI_BRIDGE_SETTINGS) ||
             (p->kind == CUI_BRIDGE_ITEM && p->parent &&
              (app->nodes[p->parent].kind == CUI_BRIDGE_TABLE ||
               app->nodes[p->parent].kind == CUI_BRIDGE_QUESTIONNAIRE))) !=
            (kind == CUI_BRIDGE_ITEM &&
             !(p->kind == CUI_BRIDGE_ITEM && p->parent &&
               app->nodes[p->parent].kind == CUI_BRIDGE_ITEM &&
               app->nodes[app->nodes[p->parent].parent].kind == CUI_BRIDGE_SETTINGS)))
            return CUI_BRIDGE_INVALID_STATE;
        if (kind == CUI_BRIDGE_FORM)
            for (uint32_t ancestor = parent.slot; ancestor; ancestor = app->nodes[ancestor].parent)
                if (app->nodes[ancestor].kind == CUI_BRIDGE_FORM)
                    return CUI_BRIDGE_INVALID_STATE;
        if (kind == CUI_BRIDGE_CAROUSEL)
            for (uint32_t ancestor = parent.slot; ancestor; ancestor = app->nodes[ancestor].parent)
                if (app->nodes[ancestor].kind == CUI_BRIDGE_CAROUSEL)
                    return CUI_BRIDGE_INVALID_STATE;
        if (kind == CUI_BRIDGE_ITEM) {
            uint32_t count = 0;
            for (uint32_t sibling = p->first_child; sibling;
                 sibling = app->nodes[sibling].next_sibling)
                count++;
            if (count >= ((p->kind == CUI_BRIDGE_QUESTIONNAIRE ||
                           (p->kind == CUI_BRIDGE_ITEM && p->parent &&
                            app->nodes[p->parent].kind == CUI_BRIDGE_QUESTIONNAIRE))
                              ? 16u
                              : 64u))
                return CUI_BRIDGE_CAPACITY;
        }
        uint32_t depth = 1;
        for (uint32_t ancestor = parent.slot; ancestor; ancestor = app->nodes[ancestor].parent)
            if (++depth > CUI_BRIDGE_MAX_DEPTH)
                return CUI_BRIDGE_CAPACITY;
    }
    size_t length = 0;
    if (!bridge_utf8(key_utf8, &length) || !length)
        return CUI_BRIDGE_INVALID_UTF8;
    for (uint32_t i = 1; i <= app->max_nodes; i++)
        if (app->nodes[i].live && strcmp(app->nodes[i].key, key_utf8) == 0)
            return CUI_BRIDGE_INVALID_ARGUMENT;
    uint32_t slot = 0;
    for (uint32_t i = 1; i <= app->max_nodes; i++)
        if (!app->nodes[i].live && app->nodes[i].generation != UINT32_MAX) {
            slot = i;
            break;
        }
    if (!slot || app->next_id == UINT64_MAX ||
        (kind == CUI_BRIDGE_EDITOR && app->next_id > UINT32_MAX))
        return CUI_BRIDGE_CAPACITY;
    char *key = NULL, *text = NULL, *detail = NULL;
    status = bridge_copy(key_utf8, &key);
    if (status != CUI_BRIDGE_OK)
        return status;
    status = bridge_copy("", &text);
    if (status != CUI_BRIDGE_OK) {
        free(key);
        return status;
    }
    status = bridge_copy("", &detail);
    if (status != CUI_BRIDGE_OK) {
        free(key);
        free(text);
        return status;
    }
    char *edit_buffer = NULL;
    char *otp_buffer = NULL;
    cui_edit *edit = NULL;
    cui_textarea_line *lines = NULL;
    if (kind == CUI_BRIDGE_INPUT || kind == CUI_BRIDGE_TEXTAREA || kind == CUI_BRIDGE_EDITOR ||
        kind == CUI_BRIDGE_COMMAND || kind == CUI_BRIDGE_COMBOBOX ||
        kind == CUI_BRIDGE_INPUT_GROUP || kind == CUI_BRIDGE_NUMBER_INPUT) {
        size_t capacity = kind == CUI_BRIDGE_NUMBER_INPUT ? 128 : 4097;
        edit_buffer = calloc(capacity, 1);
        if (kind == CUI_BRIDGE_TEXTAREA || kind == CUI_BRIDGE_EDITOR)
            lines = calloc(512, sizeof(*lines));
        if (edit_buffer && ((kind != CUI_BRIDGE_TEXTAREA && kind != CUI_BRIDGE_EDITOR) || lines))
            edit = cui_edit_create((cui_edit_config){.buffer = edit_buffer,
                                                     .capacity = capacity,
                                                     .multiline = kind == CUI_BRIDGE_TEXTAREA ||
                                                                  kind == CUI_BRIDGE_EDITOR,
                                                     .history_bytes = 8192,
                                                     .max_history_steps = 32});
        if (!edit) {
            free(key);
            free(text);
            free(detail);
            free(edit_buffer);
            free(lines);
            return CUI_BRIDGE_OUT_OF_MEMORY;
        }
    }
    if (kind == CUI_BRIDGE_OTP_INPUT) {
        otp_buffer = calloc(4097, 1);
        if (!otp_buffer) {
            free(key);
            free(text);
            free(detail);
            return CUI_BRIDGE_OUT_OF_MEMORY;
        }
    }
    cui_bridge_node_record *node = &app->nodes[slot];
    uint32_t generation = node->generation + 1;
    *node = (cui_bridge_node_record){
        .generation = generation,
        .live = true,
        .kind = kind,
        .owner = app,
        .own_slot = slot,
        .parent = root ? 0 : parent.slot,
        .id = kind == CUI_BRIDGE_EDITOR ? app->next_id++ << 32 : app->next_id++,
        .key = key,
        .text = text,
        .detail = detail,
        .edit_buffer = edit_buffer,
        .edit = edit,
        .textarea_lines = lines,
        .otp_buffer = otp_buffer,
        .minimum = 0,
        .maximum = kind == CUI_BRIDGE_NUMBER_INPUT ? 100 : 1,
        .step = kind == CUI_BRIDGE_NUMBER_INPUT ? 1 : 0.01,
        .value = kind == CUI_BRIDGE_PAGINATION ? 1 : 0,
        .total = kind == CUI_BRIDGE_RATING ? 5 : 10,
        .size = kind == CUI_BRIDGE_BUTTON ? CUI_BRIDGE_SIZE_BASE : CUI_BRIDGE_SIZE_INHERIT};
    if (kind == CUI_BRIDGE_COLOR_PICKER) {
        node->color = (cui_color){0, 0, 0, 255};
        node->value = 255;
    }
    if (kind == CUI_BRIDGE_CALENDAR || kind == CUI_BRIDGE_DATE_PICKER) {
        node->date_selection.start = (cui_date){2000, 1, 1};
        node->value = 20000101;
        cui_calendar_init(&node->calendar_state, node->date_selection.start);
        if (kind == CUI_BRIDGE_DATE_PICKER)
            cui_date_picker_init(&node->date_picker_state, node->id | (UINT64_C(1) << 62),
                                 node->date_selection.start);
    }
    if (kind == CUI_BRIDGE_TEXTAREA &&
        !cui_textarea_state_init(&node->textarea_state, lines, 512)) {
        cui_edit_destroy(edit);
        free(edit_buffer);
        free(lines);
        free(key);
        free(text);
        free(detail);
        *node = (cui_bridge_node_record){.generation = generation - 1};
        return CUI_BRIDGE_CORE_ERROR;
    }
    if (kind == CUI_BRIDGE_EDITOR &&
        !cui_editor_state_init(&node->editor_state, edit, lines, 512)) {
        cui_edit_destroy(edit);
        free(edit_buffer);
        free(lines);
        free(key);
        free(text);
        free(detail);
        *node = (cui_bridge_node_record){.generation = generation - 1};
        return CUI_BRIDGE_CORE_ERROR;
    }
    if (kind == CUI_BRIDGE_OTP_INPUT &&
        cui_otp_init(&node->otp_state, otp_buffer, 4097, 6) != CUI_EDIT_OK) {
        free(otp_buffer);
        free(key);
        free(text);
        free(detail);
        *node = (cui_bridge_node_record){.generation = generation - 1};
        return CUI_BRIDGE_CORE_ERROR;
    }
    if (kind == CUI_BRIDGE_SPEECH) {
        node->speech = bridge_speech_create(node);
        if (!node->speech) {
            bridge_free_node_resources(node);
            *node = (cui_bridge_node_record){.generation = generation - 1};
            return CUI_BRIDGE_OUT_OF_MEMORY;
        }
    }
    if (kind == CUI_BRIDGE_QUESTIONNAIRE) {
        node->questionnaire = calloc(1, sizeof *node->questionnaire);
        if (!node->questionnaire) {
            bridge_free_node_resources(node);
            *node = (cui_bridge_node_record){.generation = generation - 1};
            return CUI_BRIDGE_OUT_OF_MEMORY;
        }
        node->questionnaire->node = node;
    }
    if (kind == CUI_BRIDGE_DOCK) {
        node->dock = calloc(1, sizeof *node->dock);
        if (!node->dock) {
            bridge_free_node_resources(node);
            *node = (cui_bridge_node_record){.generation = generation - 1};
            return CUI_BRIDGE_OUT_OF_MEMORY;
        }
        node->dock->node = node;
        node->dock->model = cui_dock_create(
            (cui_dock_config){.node_capacity = 64, .panel_capacity = 64, .max_depth = 16});
        node->dock->view = cui_dock_view_create(64, 64);
        if (!node->dock->model || !node->dock->view) {
            bridge_free_node_resources(node);
            *node = (cui_bridge_node_record){.generation = generation - 1};
            return CUI_BRIDGE_OUT_OF_MEMORY;
        }
    }
    if (kind == CUI_BRIDGE_SETTINGS) {
        node->settings = calloc(1, sizeof *node->settings);
        if (!node->settings ||
            !cui_settings_state_init(&node->settings->state, node->settings->page_state, 8, NULL)) {
            bridge_free_node_resources(node);
            *node = (cui_bridge_node_record){.generation = generation - 1};
            return CUI_BRIDGE_OUT_OF_MEMORY;
        }
        node->settings->node = node;
    }
    if (kind == CUI_BRIDGE_NUMBER_INPUT)
        node->number_state.edit = edit;
    if (kind == CUI_BRIDGE_COMMAND) {
        node->command_rows = calloc(64, sizeof *node->command_rows);
        node->command_entries = calloc(64, sizeof *node->command_entries);
        node->command_keywords = calloc(64, sizeof *node->command_keywords);
        node->command_slots = calloc(64, sizeof *node->command_slots);
        if (!node->command_rows || !node->command_entries || !node->command_keywords ||
            !node->command_slots ||
            !cui_command_init(&node->command_state, node->command_rows, 64, edit)) {
            bridge_free_node_resources(node);
            *node = (cui_bridge_node_record){.generation = generation - 1};
            return CUI_BRIDGE_OUT_OF_MEMORY;
        }
    }
    if (kind == CUI_BRIDGE_SELECT || kind == CUI_BRIDGE_COMBOBOX || kind == CUI_BRIDGE_MENU)
        cui_choice_popup_init(&node->choice_popup, node->id | (UINT64_C(1) << 62));
    if (kind == CUI_BRIDGE_DROPDOWN_BUTTON)
        cui_menu_tree_init(&node->menu_state, node->id | (UINT64_C(1) << 62));
    if (kind >= CUI_BRIDGE_DIALOG && kind <= CUI_BRIDGE_POPOVER)
        cui_surface_init(&node->surface_state, node->id | (UINT64_C(1) << 62),
                         kind == CUI_BRIDGE_DIALOG         ? CUI_SURFACE_DIALOG
                         : kind == CUI_BRIDGE_ALERT_DIALOG ? CUI_SURFACE_ALERT
                         : kind == CUI_BRIDGE_SHEET        ? CUI_SURFACE_SHEET
                                                           : CUI_SURFACE_POPOVER);
    if (root)
        app->root = slot;
    else if (p->last_child)
        app->nodes[p->last_child].next_sibling = slot;
    else
        p->first_child = slot;
    if (p)
        p->last_child = slot;
    *out_node = (cui_bridge_node){slot, generation};
    return CUI_BRIDGE_OK;
}
static void bridge_remove_subtree(cui_bridge_app *app, uint32_t slot) {
    cui_bridge_node_record *node = &app->nodes[slot];
    if (node->kind == CUI_BRIDGE_NOTIFICATION && app->notifications)
        for (size_t i = 0; i < app->notifications->count; i++)
            if (app->notifications->entries[i].notification.id.key == node->id) {
                for (size_t j = i + 1; j < app->notifications->count; j++)
                    app->notifications->entries[j - 1] = app->notifications->entries[j];
                app->notifications->count--;
                break;
            }
    for (uint32_t i = 1; i <= app->max_nodes; i++) {
        cui_bridge_node_record *other = &app->nodes[i];
        if (other->live && other->anchor.slot == slot &&
            other->anchor.generation == node->generation) {
            other->anchor = (cui_bridge_node){0};
            other->hover_state = (cui_hover_state){0};
            if (other->kind == CUI_BRIDGE_POPOVER && other->surface_state.overlay.open)
                cui_surface_request_close(app->ui, &other->surface_state, CUI_SURFACE_CLOSED);
        }
    }
    for (uint32_t child = node->first_child; child;) {
        uint32_t next = app->nodes[child].next_sibling;
        bridge_remove_subtree(app, child);
        child = next;
    }
    bridge_free_node_resources(node);
    node->live = false;
    node->first_child = node->last_child = node->next_sibling = node->parent = 0;
}
cui_bridge_status cui_bridge_node_remove(cui_bridge_app *app, cui_bridge_node handle) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->parent && app->nodes[node->parent].kind == CUI_BRIDGE_DOCK &&
        app->nodes[node->parent].dock && app->nodes[node->parent].dock->initialized)
        return CUI_BRIDGE_INVALID_STATE;
    for (uint32_t ancestor = node->parent; ancestor; ancestor = app->nodes[ancestor].parent)
        if ((app->nodes[ancestor].kind == CUI_BRIDGE_QUESTIONNAIRE &&
             app->nodes[ancestor].questionnaire &&
             app->nodes[ancestor].questionnaire->state.initialized) ||
            (app->nodes[ancestor].kind == CUI_BRIDGE_SETTINGS && app->nodes[ancestor].settings &&
             app->nodes[ancestor].settings->initialized))
            return CUI_BRIDGE_INVALID_STATE;
    if (node->parent) {
        cui_bridge_node_record *parent = &app->nodes[node->parent];
        if (node->kind == CUI_BRIDGE_ITEM &&
            (parent->kind == CUI_BRIDGE_RADIO_GROUP || parent->kind == CUI_BRIDGE_TABS ||
             parent->kind == CUI_BRIDGE_SELECT || parent->kind == CUI_BRIDGE_COMBOBOX ||
             parent->kind == CUI_BRIDGE_DROPDOWN_BUTTON || parent->kind == CUI_BRIDGE_LIST ||
             parent->kind == CUI_BRIDGE_MENU || parent->kind == CUI_BRIDGE_STEPPER ||
             parent->kind == CUI_BRIDGE_TREE || parent->kind == CUI_BRIDGE_CAROUSEL) &&
            (uint32_t)parent->value == handle.slot)
            parent->value = 0;
        uint32_t *link = &parent->first_child;
        while (*link && *link != handle.slot)
            link = &app->nodes[*link].next_sibling;
        if (*link)
            *link = node->next_sibling;
        if (parent->last_child == handle.slot) {
            parent->last_child = 0;
            for (uint32_t i = parent->first_child; i; i = app->nodes[i].next_sibling)
                parent->last_child = i;
        }
    } else
        app->root = 0;
    bridge_remove_subtree(app, handle.slot);
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_text(cui_bridge_app *app, cui_bridge_node handle,
                                      const char *text_utf8) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->parent && app->nodes[node->parent].kind == CUI_BRIDGE_DOCK &&
        app->nodes[node->parent].dock && app->nodes[node->parent].dock->initialized)
        return CUI_BRIDGE_INVALID_STATE;
    for (uint32_t ancestor = node->parent; ancestor; ancestor = app->nodes[ancestor].parent)
        if (app->nodes[ancestor].kind == CUI_BRIDGE_SETTINGS && app->nodes[ancestor].settings &&
            app->nodes[ancestor].settings->initialized && node->kind != CUI_BRIDGE_INPUT)
            return CUI_BRIDGE_INVALID_STATE;
    for (uint32_t ancestor = node->parent; ancestor; ancestor = app->nodes[ancestor].parent)
        if (app->nodes[ancestor].kind == CUI_BRIDGE_QUESTIONNAIRE &&
            app->nodes[ancestor].questionnaire &&
            app->nodes[ancestor].questionnaire->state.initialized)
            return CUI_BRIDGE_INVALID_STATE;
    if (node->kind < CUI_BRIDGE_LABEL)
        return CUI_BRIDGE_INVALID_STATE;
    if (node->kind == CUI_BRIDGE_NOTIFICATION && bridge_notification_entry(app, node->id))
        return CUI_BRIDGE_INVALID_STATE;
    if (node->kind == CUI_BRIDGE_TEXT_VIEW) {
        char *copy = NULL;
        status = bridge_copy(text_utf8, &copy);
        if (status != CUI_BRIDGE_OK)
            return status;
        if (!node->text_view) {
            node->text_view = bridge_text_view_create();
            if (!node->text_view) {
                free(copy);
                return CUI_BRIDGE_OUT_OF_MEMORY;
            }
        }
        cui_text_view_error result =
            cui_text_view_set_text(&node->text_view->state, cui_string(copy));
        if (result != CUI_TEXT_VIEW_OK) {
            free(copy);
            return result == CUI_TEXT_VIEW_CAPACITY ? CUI_BRIDGE_CAPACITY
                                                    : CUI_BRIDGE_INVALID_ARGUMENT;
        }
        free(node->text);
        node->text = copy;
        return CUI_BRIDGE_OK;
    }
    if (node->kind == CUI_BRIDGE_OTP_INPUT) {
        size_t length = 0;
        if (!bridge_utf8(text_utf8, &length))
            return CUI_BRIDGE_INVALID_UTF8;
        cui_edit_result result = cui_otp_set_value(&node->otp_state, (cui_str){text_utf8, length});
        return result == CUI_EDIT_OK ? CUI_BRIDGE_OK : CUI_BRIDGE_CAPACITY;
    }
    if (node->edit && node->kind != CUI_BRIDGE_NUMBER_INPUT) {
        size_t length = 0;
        if (!bridge_utf8(text_utf8, &length))
            return CUI_BRIDGE_INVALID_UTF8;
        cui_str old = cui_edit_get_text(node->edit);
        return cui_edit_replace_range(node->edit, 0, old.length, (cui_str){text_utf8, length}) ==
                       CUI_EDIT_OK
                   ? CUI_BRIDGE_OK
                   : CUI_BRIDGE_CAPACITY;
    }
    char *copy = NULL;
    status = bridge_copy(text_utf8, &copy);
    if (status != CUI_BRIDGE_OK)
        return status;
    free(node->text);
    node->text = copy;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_chart_set_data(cui_bridge_app *app, cui_bridge_node handle,
                                            const cui_bridge_chart_datum *data, uint32_t count) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_CHART)
        return CUI_BRIDGE_INVALID_STATE;
    if (count > 64 || (count && !data))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    bridge_chart_owned *next = calloc(1, sizeof *next);
    if (!next)
        return CUI_BRIDGE_OUT_OF_MEMORY;
    next->kind = node->chart ? node->chart->kind : 0;
    next->count = count;
    next->state.hovered = SIZE_MAX;
    for (uint32_t i = 0; i < count; i++) {
        size_t length = 0;
        if (!bridge_utf8(data[i].label_utf8, &length) || !isfinite(data[i].value) ||
            (next->kind == CUI_CHART_PIE && data[i].value < 0) || length > 128 ||
            data[i].value < -1000000000 || data[i].value > 1000000000) {
            bridge_chart_free(next);
            return CUI_BRIDGE_INVALID_ARGUMENT;
        }
        next->labels[i] = malloc(length + 1);
        if (!next->labels[i]) {
            bridge_chart_free(next);
            return CUI_BRIDGE_OUT_OF_MEMORY;
        }
        memcpy(next->labels[i], data[i].label_utf8, length + 1);
        next->data[i] =
            (cui_chart_datum){.label = cui_string(next->labels[i]), .value = data[i].value};
    }
    bridge_chart_free(node->chart);
    node->chart = next;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_chart_set_kind(cui_bridge_app *app, cui_bridge_node handle,
                                            uint32_t kind) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_CHART)
        return CUI_BRIDGE_INVALID_STATE;
    if (kind > CUI_CHART_PIE)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (kind == CUI_CHART_PIE && node->chart)
        for (uint32_t i = 0; i < node->chart->count; i++)
            if (node->chart->data[i].value < 0)
                return CUI_BRIDGE_INVALID_ARGUMENT;
    if (!node->chart) {
        node->chart = calloc(1, sizeof *node->chart);
        if (!node->chart)
            return CUI_BRIDGE_OUT_OF_MEMORY;
        node->chart->state.hovered = SIZE_MAX;
    }
    node->chart->kind = kind;
    node->chart->state.revision++;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_chart_get_datum(cui_bridge_app *app, cui_bridge_node handle,
                                             uint32_t index, char *label, uint32_t capacity,
                                             uint32_t *required_capacity, double *value) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_CHART)
        return CUI_BRIDGE_INVALID_STATE;
    if (!node->chart || index >= node->chart->count)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (!required_capacity || !value)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    const char *text = node->chart->labels[index];
    size_t need = strlen(text) + 1;
    *required_capacity = (uint32_t)need;
    if (!label || capacity < need)
        return CUI_BRIDGE_CAPACITY;
    memcpy(label, text, need);
    *value = node->chart->data[index].value;
    return CUI_BRIDGE_OK;
}
static cui_bridge_status bridge_data_table_copy(const char *source, char **destination) {
    size_t length = 0;
    if (!bridge_utf8(source, &length) || length > 128)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    char *copy = malloc(length + 1);
    if (!copy)
        return CUI_BRIDGE_OUT_OF_MEMORY;
    memcpy(copy, source, length + 1);
    *destination = copy;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_data_table_set(cui_bridge_app *app, cui_bridge_node handle,
                                            const char *const *headers, uint32_t column_count,
                                            const char *const *cells, uint32_t row_count) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_DATA_TABLE)
        return CUI_BRIDGE_INVALID_STATE;
    if (!column_count || column_count > 16 || row_count > 32 || !headers || (row_count && !cells))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    bridge_data_table_owned *next = calloc(1, sizeof *next);
    if (!next)
        return CUI_BRIDGE_OUT_OF_MEMORY;
    next->row_count = row_count;
    next->column_count = column_count;
    for (uint32_t c = 0; c < column_count; c++) {
        status = bridge_data_table_copy(headers[c], &next->headers[c]);
        if (status != CUI_BRIDGE_OK)
            goto fail;
        next->columns[c] = cui_table_column_default(c + 1, cui_string(next->headers[c]));
        next->columns[c].sortable = true;
        next->columns[c].resizable = false;
        next->columns[c].movable = false;
    }
    for (uint32_t r = 0; r < row_count; r++) {
        for (uint32_t c = 0; c < column_count; c++) {
            uint32_t index = r * column_count + c;
            status = bridge_data_table_copy(cells[index], &next->cells[index]);
            if (status != CUI_BRIDGE_OK)
                goto fail;
        }
        next->rows[r] =
            (cui_table_row){.key = r + 1, .label = cui_string(next->cells[r * column_count])};
    }
    next->model = cui_table_create(32);
    if (!next->model) {
        status = CUI_BRIDGE_OUT_OF_MEMORY;
        goto fail;
    }
    if (cui_table_bind(next->model, next->rows, row_count, next->columns, column_count) !=
        CUI_TABLE_OK) {
        status = CUI_BRIDGE_CORE_ERROR;
        goto fail;
    }
    bridge_data_table_free(node->data_table);
    node->data_table = next;
    node->value = 0;
    node->checked = 0;
    return CUI_BRIDGE_OK;
fail:
    bridge_data_table_free(next);
    return status;
}
cui_bridge_status cui_bridge_data_table_get_cell(cui_bridge_app *app, cui_bridge_node handle,
                                                 uint32_t row, uint32_t column, char *destination,
                                                 uint32_t capacity, uint32_t *required_capacity) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_DATA_TABLE)
        return CUI_BRIDGE_INVALID_STATE;
    bridge_data_table_owned *table = node->data_table;
    if (!table || row >= table->row_count || column >= table->column_count || !required_capacity)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    const char *source = table->cells[row * table->column_count + column];
    size_t required = strlen(source) + 1;
    *required_capacity = (uint32_t)required;
    if (!destination || capacity < required)
        return CUI_BRIDGE_CAPACITY;
    memcpy(destination, source, required);
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_data_table_get_state(cui_bridge_app *app, cui_bridge_node handle,
                                                  uint32_t *row, uint32_t *column,
                                                  uint32_t *sort_column, uint32_t *sort_direction) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_DATA_TABLE)
        return CUI_BRIDGE_INVALID_STATE;
    if (!row || !column || !sort_column || !sort_direction)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    const cui_table_state *state = node->data_table ? &node->data_table->state : NULL;
    *row = state ? (uint32_t)state->row : 0;
    *column = state ? (uint32_t)state->column : 0;
    *sort_column = state ? (uint32_t)state->sort_column : 0;
    *sort_direction = state ? (uint32_t)state->sort : 0;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_data_table_set_selection_mode(cui_bridge_app *app,
                                                           cui_bridge_node handle, uint32_t mode) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_DATA_TABLE)
        return CUI_BRIDGE_INVALID_STATE;
    if (mode > CUI_TABLE_SELECT_CELL)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->table_selection_mode = mode;
    if (node->data_table) {
        node->data_table->state.row = 0;
        node->data_table->state.column = 0;
    }
    node->value = 0;
    node->checked = 0;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_data_table_cell_bounds(cui_bridge_app *app, cui_bridge_node handle,
                                                    uint32_t row, uint32_t column,
                                                    cui_bridge_rect *out_bounds) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_DATA_TABLE)
        return CUI_BRIDGE_INVALID_STATE;
    bridge_data_table_owned *table = node->data_table;
    if (!table || row >= table->row_count || column >= table->column_count || !out_bounds)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_rect rect = cui_bounds(app->ui, cui_table_cell_id(node->id, row + 1, column + 1));
    *out_bounds = (cui_bridge_rect){rect.x, rect.y, rect.w, rect.h};
    return rect.w > 0 && rect.h > 0 ? CUI_BRIDGE_OK : CUI_BRIDGE_INVALID_STATE;
}
cui_bridge_status cui_bridge_data_table_header_bounds(cui_bridge_app *app, cui_bridge_node handle,
                                                      uint32_t column,
                                                      cui_bridge_rect *out_bounds) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_DATA_TABLE)
        return CUI_BRIDGE_INVALID_STATE;
    bridge_data_table_owned *table = node->data_table;
    if (!table || column >= table->column_count || !out_bounds)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_rect rect = cui_bounds(app->ui, cui_table_header_id(node->id, column + 1));
    *out_bounds = (cui_bridge_rect){rect.x, rect.y, rect.w, rect.h};
    return rect.w > 0 && rect.h > 0 ? CUI_BRIDGE_OK : CUI_BRIDGE_INVALID_STATE;
}
cui_bridge_status cui_bridge_command_item_bounds(cui_bridge_app *app, cui_bridge_node command,
                                                 cui_bridge_node item,
                                                 cui_bridge_rect *out_bounds) {
    cui_bridge_node_record *node, *child;
    cui_bridge_status status = bridge_valid_node(app, command, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_COMMAND)
        return CUI_BRIDGE_INVALID_STATE;
    status = bridge_valid_node(app, item, &child);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out_bounds || child->parent != command.slot)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    uint32_t index = 0;
    for (uint32_t current = node->first_child; current;
         current = app->nodes[current].next_sibling, index++)
        if (current == item.slot) {
            cui_rect rect =
                cui_bounds(app->ui, cui_command_row_id(node->id, (cui_command_index){0, index}));
            *out_bounds = (cui_bridge_rect){rect.x, rect.y, rect.w, rect.h};
            return rect.w > 0 && rect.h > 0 ? CUI_BRIDGE_OK : CUI_BRIDGE_INVALID_STATE;
        }
    return CUI_BRIDGE_INVALID_ARGUMENT;
}
cui_bridge_status cui_bridge_virtual_list_item_bounds(cui_bridge_app *app, cui_bridge_node list,
                                                      cui_bridge_node item,
                                                      cui_bridge_rect *out_bounds) {
    cui_bridge_node_record *node, *child;
    cui_bridge_status status = bridge_valid_node(app, list, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_VIRTUAL_LIST)
        return CUI_BRIDGE_INVALID_STATE;
    status = bridge_valid_node(app, item, &child);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out_bounds || child->parent != list.slot)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_rect rect = cui_bounds(app->ui, cui_virtual_item_id(node->id, child->id));
    *out_bounds = (cui_bridge_rect){rect.x, rect.y, rect.w, rect.h};
    return rect.w > 0 && rect.h > 0 ? CUI_BRIDGE_OK : CUI_BRIDGE_INVALID_STATE;
}
cui_bridge_status cui_bridge_plot_set_points(cui_bridge_app *app, cui_bridge_node handle,
                                             const cui_bridge_plot_point *points, uint32_t count) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_PLOT)
        return CUI_BRIDGE_INVALID_STATE;
    if (count > 256 || (count && !points))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_plot_sample *samples = count ? calloc(count, sizeof *samples) : NULL;
    if (count && !samples)
        return CUI_BRIDGE_OUT_OF_MEMORY;
    for (uint32_t i = 0; i < count; i++) {
        if (!isfinite(points[i].x) || !isfinite(points[i].y) || fabs(points[i].x) > 16384 ||
            fabs(points[i].y) > 16384) {
            free(samples);
            return CUI_BRIDGE_INVALID_ARGUMENT;
        }
        samples[i] = (cui_plot_sample){.x = points[i].x, .y1 = points[i].y, .defined = true};
    }
    free(node->plot_samples);
    node->plot_samples = samples;
    node->plot_count = count;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_plot_get_point(cui_bridge_app *app, cui_bridge_node handle,
                                            uint32_t index, cui_bridge_plot_point *out_point) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_PLOT)
        return CUI_BRIDGE_INVALID_STATE;
    if (index >= node->plot_count || !out_point)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    *out_point = (cui_bridge_plot_point){node->plot_samples[index].x, node->plot_samples[index].y1};
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_text_view_format(cui_bridge_app *app, cui_bridge_node handle,
                                                  uint32_t format) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_TEXT_VIEW)
        return CUI_BRIDGE_INVALID_STATE;
    if (format > CUI_TEXT_VIEW_PLAIN)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (!node->text_view) {
        node->text_view = bridge_text_view_create();
        if (!node->text_view)
            return CUI_BRIDGE_OUT_OF_MEMORY;
    }
    cui_text_view_state *state = &node->text_view->state;
    cui_text_view_format old = state->format;
    state->format = (cui_text_view_format)format;
    cui_text_view_error result = cui_text_view_set_text(state, cui_string(node->text));
    if (result != CUI_TEXT_VIEW_OK) {
        state->format = old;
        return result == CUI_TEXT_VIEW_CAPACITY ? CUI_BRIDGE_CAPACITY : CUI_BRIDGE_INVALID_ARGUMENT;
    }
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_detail_text(cui_bridge_app *app, cui_bridge_node handle,
                                             const char *text_utf8) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    for (uint32_t ancestor = node->parent; ancestor; ancestor = app->nodes[ancestor].parent)
        if ((app->nodes[ancestor].kind == CUI_BRIDGE_QUESTIONNAIRE &&
             app->nodes[ancestor].questionnaire &&
             app->nodes[ancestor].questionnaire->state.initialized) ||
            (app->nodes[ancestor].kind == CUI_BRIDGE_SETTINGS && app->nodes[ancestor].settings &&
             app->nodes[ancestor].settings->initialized))
            return CUI_BRIDGE_INVALID_STATE;
    if (node->kind != CUI_BRIDGE_ALERT && node->kind != CUI_BRIDGE_SELECT &&
        node->kind != CUI_BRIDGE_COMBOBOX && node->kind != CUI_BRIDGE_DROPDOWN_BUTTON &&
        (node->kind < CUI_BRIDGE_DIALOG || node->kind > CUI_BRIDGE_POPOVER) &&
        node->kind != CUI_BRIDGE_EMPTY_VIEW && node->kind != CUI_BRIDGE_ITEM &&
        node->kind != CUI_BRIDGE_MESSAGE && node->kind != CUI_BRIDGE_ATTACHMENT &&
        node->kind != CUI_BRIDGE_INPUT_GROUP && node->kind != CUI_BRIDGE_NOTIFICATION &&
        node->kind != CUI_BRIDGE_EDITOR && node->kind != CUI_BRIDGE_INPUT &&
        node->kind != CUI_BRIDGE_NUMBER_INPUT)
        return CUI_BRIDGE_INVALID_STATE;
    if (node->kind == CUI_BRIDGE_NOTIFICATION && bridge_notification_entry(app, node->id))
        return CUI_BRIDGE_INVALID_STATE;
    char *copy = NULL;
    status = bridge_copy(text_utf8, &copy);
    if (status != CUI_BRIDGE_OK)
        return status;
    free(node->detail);
    node->detail = copy;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_visual_size(cui_bridge_app *app, cui_bridge_node handle,
                                             float width, float height) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_AVATAR && node->kind != CUI_BRIDGE_EMPTY_VIEW &&
        node->kind != CUI_BRIDGE_GROUP_BOX && node->kind != CUI_BRIDGE_SCROLLABLE &&
        node->kind != CUI_BRIDGE_SKELETON && node->kind != CUI_BRIDGE_MESSAGE &&
        node->kind != CUI_BRIDGE_ICON && node->kind != CUI_BRIDGE_ATTACHMENT &&
        node->kind != CUI_BRIDGE_IMAGE && node->kind != CUI_BRIDGE_TREE &&
        node->kind != CUI_BRIDGE_COLOR_PICKER && node->kind != CUI_BRIDGE_DATE_PICKER &&
        node->kind != CUI_BRIDGE_FORM && node->kind != CUI_BRIDGE_INPUT_GROUP &&
        node->kind != CUI_BRIDGE_SPINNER && node->kind != CUI_BRIDGE_MESSAGE_SCROLLER &&
        node->kind != CUI_BRIDGE_CAROUSEL && node->kind != CUI_BRIDGE_RESIZABLE &&
        node->kind != CUI_BRIDGE_TEXT_VIEW && node->kind != CUI_BRIDGE_EDITOR &&
        node->kind != CUI_BRIDGE_CHART && node->kind != CUI_BRIDGE_TABLE &&
        node->kind != CUI_BRIDGE_DATA_TABLE && node->kind != CUI_BRIDGE_PLOT &&
        node->kind != CUI_BRIDGE_COMMAND && node->kind != CUI_BRIDGE_VIRTUAL_LIST &&
        node->kind != CUI_BRIDGE_SIDEBAR && node->kind != CUI_BRIDGE_QUESTIONNAIRE &&
        node->kind != CUI_BRIDGE_DOCK && node->kind != CUI_BRIDGE_SETTINGS)
        return CUI_BRIDGE_INVALID_STATE;
    if (!isfinite(width) || !isfinite(height) || width < 0 || height < 0 || width > 4096 ||
        height > 4096 || (node->kind == CUI_BRIDGE_CHART && (width > 512 || height > 512)))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->visual_width = width;
    node->visual_height = height;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_scroll_extent(cui_bridge_app *app, cui_bridge_node handle,
                                               float width, float height) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_SCROLLABLE)
        return CUI_BRIDGE_INVALID_STATE;
    if (!isfinite(width) || !isfinite(height) || width < 0 || height < 0 || width > 32768 ||
        height > 32768)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->content_width = width;
    node->content_height = height;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_open(cui_bridge_app *app, cui_bridge_node handle, uint32_t open) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind == CUI_BRIDGE_SIDEBAR) {
        if (open > 1)
            return CUI_BRIDGE_INVALID_ARGUMENT;
        node->checked = open == 0;
        return CUI_BRIDGE_OK;
    }
    if (node->kind == CUI_BRIDGE_NOTIFICATION) {
        if (open > 1)
            return CUI_BRIDGE_INVALID_ARGUMENT;
        if (!open) {
            if (bridge_notification_entry(app, node->id))
                (void)cui_notification_dismiss(
                    app->notifications, (cui_notification_id){1, node->id}, app->now_seconds);
            return CUI_BRIDGE_OK;
        }
        if (!app->notifications) {
            cui_notification_state *state = calloc(1, sizeof *state);
            size_t capacity = app->max_nodes < CUI_NOTIFICATION_MAX_ITEMS
                                  ? app->max_nodes
                                  : CUI_NOTIFICATION_MAX_ITEMS;
            cui_notification_entry *entries = calloc(capacity, sizeof *entries);
            if (!state || !entries) {
                free(state);
                free(entries);
                return CUI_BRIDGE_OUT_OF_MEMORY;
            }
            if (!cui_notification_init(state, entries, capacity,
                                       cui_notification_settings_default(),
                                       (cui_notification_host){0})) {
                free(state);
                free(entries);
                return CUI_BRIDGE_CORE_ERROR;
            }
            app->notifications = state;
            app->notification_entries = entries;
        }
        if (bridge_notification_entry(app, node->id))
            return CUI_BRIDGE_INVALID_STATE;
        cui_notification note = cui_notification_default();
        note.id = (cui_notification_id){1, node->id};
        note.title = cui_string(node->text);
        note.message = cui_string(node->detail);
        note.autohide = false;
        note.user = node;
        note.on_click = bridge_notification_click;
        note.on_close = bridge_notification_close;
        return cui_notification_push(app->notifications, note, app->now_seconds)
                   ? CUI_BRIDGE_OK
                   : CUI_BRIDGE_CAPACITY;
    }
    if (node->kind == CUI_BRIDGE_FOCUS_TRAP) {
        if (open > 1)
            return CUI_BRIDGE_INVALID_ARGUMENT;
        node->checked = open != 0;
        return CUI_BRIDGE_OK;
    }
    if (node->kind == CUI_BRIDGE_DATE_PICKER) {
        if (open > 1)
            return CUI_BRIDGE_INVALID_ARGUMENT;
        if (open)
            cui_overlay_open(app->ui, &node->date_picker_state.overlay, node->id);
        else
            cui_overlay_close(app->ui, &node->date_picker_state.overlay);
        return CUI_BRIDGE_OK;
    }
    if (node->kind >= CUI_BRIDGE_DIALOG && node->kind <= CUI_BRIDGE_POPOVER) {
        if (open > 1)
            return CUI_BRIDGE_INVALID_ARGUMENT;
        if (open && node->kind == CUI_BRIDGE_POPOVER && !bridge_node(app, node->anchor))
            return CUI_BRIDGE_INVALID_STATE;
        if (open)
            cui_surface_open(app->ui, &node->surface_state, 0);
        else
            cui_surface_request_close(app->ui, &node->surface_state, CUI_SURFACE_CLOSED);
        return CUI_BRIDGE_OK;
    }
    if (node->kind != CUI_BRIDGE_COLLAPSIBLE &&
        !(node->kind == CUI_BRIDGE_ITEM && node->parent &&
          app->nodes[node->parent].kind == CUI_BRIDGE_ACCORDION))
        return CUI_BRIDGE_INVALID_STATE;
    if (open > 1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->checked = open != 0;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_get_open(cui_bridge_app *app, cui_bridge_node handle,
                                      uint32_t *out_open) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out_open)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (node->kind == CUI_BRIDGE_SIDEBAR) {
        *out_open = !node->checked;
        return CUI_BRIDGE_OK;
    }
    if (node->kind == CUI_BRIDGE_COLLAPSIBLE ||
        (node->kind == CUI_BRIDGE_ITEM && node->parent &&
         app->nodes[node->parent].kind == CUI_BRIDGE_ACCORDION)) {
        *out_open = node->checked;
        return CUI_BRIDGE_OK;
    }
    if (node->kind == CUI_BRIDGE_NOTIFICATION) {
        cui_notification_entry *entry = bridge_notification_entry(app, node->id);
        *out_open = entry && entry->phase != CUI_NOTIFICATION_ENDING;
        return CUI_BRIDGE_OK;
    }
    if (node->kind == CUI_BRIDGE_FOCUS_TRAP) {
        *out_open = node->checked;
        return CUI_BRIDGE_OK;
    }
    if (node->kind == CUI_BRIDGE_TOOLTIP || node->kind == CUI_BRIDGE_HOVER_CARD)
        *out_open = node->hover_state.open;
    else if (node->kind == CUI_BRIDGE_DATE_PICKER)
        *out_open = node->date_picker_state.overlay.open;
    else if (node->kind >= CUI_BRIDGE_DIALOG && node->kind <= CUI_BRIDGE_POPOVER)
        *out_open = node->surface_state.overlay.open;
    else
        return CUI_BRIDGE_INVALID_STATE;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_anchor(cui_bridge_app *app, cui_bridge_node surface,
                                        cui_bridge_node anchor) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, surface, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_POPOVER && node->kind != CUI_BRIDGE_TOOLTIP &&
        node->kind != CUI_BRIDGE_HOVER_CARD)
        return CUI_BRIDGE_INVALID_STATE;
    cui_bridge_node_record *trigger = bridge_node(app, anchor);
    if (!trigger)
        return CUI_BRIDGE_STALE_HANDLE;
    for (uint32_t ancestor = anchor.slot; ancestor; ancestor = app->nodes[ancestor].parent)
        if (ancestor == surface.slot)
            return CUI_BRIDGE_INVALID_ARGUMENT;
    node->anchor = anchor;
    return CUI_BRIDGE_OK;
}
static bool bridge_group_kind(cui_bridge_kind kind) {
    return kind == CUI_BRIDGE_RADIO_GROUP || kind == CUI_BRIDGE_TABS ||
           kind == CUI_BRIDGE_ACCORDION || kind == CUI_BRIDGE_SELECT ||
           kind == CUI_BRIDGE_COMBOBOX || kind == CUI_BRIDGE_DROPDOWN_BUTTON ||
           kind == CUI_BRIDGE_LIST || kind == CUI_BRIDGE_MENU || kind == CUI_BRIDGE_STEPPER;
}
cui_bridge_status cui_bridge_set_selected(cui_bridge_app *app, cui_bridge_node group,
                                          cui_bridge_node item) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, group, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_RADIO_GROUP && node->kind != CUI_BRIDGE_TABS &&
        node->kind != CUI_BRIDGE_SELECT && node->kind != CUI_BRIDGE_COMBOBOX &&
        node->kind != CUI_BRIDGE_DROPDOWN_BUTTON && node->kind != CUI_BRIDGE_LIST &&
        node->kind != CUI_BRIDGE_MENU && node->kind != CUI_BRIDGE_STEPPER &&
        node->kind != CUI_BRIDGE_TREE && node->kind != CUI_BRIDGE_CAROUSEL)
        return CUI_BRIDGE_INVALID_STATE;
    if (!item.slot && !item.generation) {
        if (node->kind == CUI_BRIDGE_CAROUSEL && node->first_child)
            return CUI_BRIDGE_INVALID_ARGUMENT;
        node->value = 0;
        return CUI_BRIDGE_OK;
    }
    cui_bridge_node_record *child = bridge_node(app, item);
    if (!child)
        return CUI_BRIDGE_STALE_HANDLE;
    if (child->parent != group.slot || child->kind != CUI_BRIDGE_ITEM)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->value = item.slot;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_get_selected(cui_bridge_app *app, cui_bridge_node group,
                                          cui_bridge_node *out_item) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, group, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out_item)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (node->kind != CUI_BRIDGE_RADIO_GROUP && node->kind != CUI_BRIDGE_TABS &&
        node->kind != CUI_BRIDGE_SELECT && node->kind != CUI_BRIDGE_COMBOBOX &&
        node->kind != CUI_BRIDGE_DROPDOWN_BUTTON && node->kind != CUI_BRIDGE_LIST &&
        node->kind != CUI_BRIDGE_MENU && node->kind != CUI_BRIDGE_STEPPER &&
        node->kind != CUI_BRIDGE_TREE && node->kind != CUI_BRIDGE_CAROUSEL &&
        node->kind != CUI_BRIDGE_COMMAND && node->kind != CUI_BRIDGE_SIDEBAR)
        return CUI_BRIDGE_INVALID_STATE;
    uint32_t slot = (uint32_t)node->value;
    if (slot && (!app->nodes[slot].live || app->nodes[slot].parent != group.slot))
        slot = 0;
    *out_item = slot ? (cui_bridge_node){slot, app->nodes[slot].generation} : (cui_bridge_node){0};
    return CUI_BRIDGE_OK;
}
static bool bridge_boolean_kind(cui_bridge_kind kind) {
    return kind == CUI_BRIDGE_CHECKBOX || kind == CUI_BRIDGE_RADIO || kind == CUI_BRIDGE_SWITCH ||
           kind == CUI_BRIDGE_TOGGLE || kind == CUI_BRIDGE_COLLAPSIBLE || kind == CUI_BRIDGE_ITEM;
}
cui_bridge_status cui_bridge_set_checked(cui_bridge_app *app, cui_bridge_node handle,
                                         uint32_t checked) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!bridge_boolean_kind(node->kind))
        return CUI_BRIDGE_INVALID_STATE;
    if (node->kind == CUI_BRIDGE_ITEM &&
        (!node->parent || (app->nodes[node->parent].kind != CUI_BRIDGE_ACCORDION &&
                           app->nodes[node->parent].kind != CUI_BRIDGE_TREE)))
        return CUI_BRIDGE_INVALID_STATE;
    if (checked > 1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->checked = checked != 0;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_disabled(cui_bridge_app *app, cui_bridge_node handle,
                                          uint32_t disabled) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind == CUI_BRIDGE_ITEM && node->parent &&
        (app->nodes[node->parent].kind == CUI_BRIDGE_SETTINGS ||
         (app->nodes[node->parent].kind == CUI_BRIDGE_ITEM && app->nodes[node->parent].parent &&
          app->nodes[app->nodes[node->parent].parent].kind == CUI_BRIDGE_SETTINGS)))
        return CUI_BRIDGE_INVALID_STATE;
    for (uint32_t ancestor = node->parent; ancestor; ancestor = app->nodes[ancestor].parent)
        if (app->nodes[ancestor].kind == CUI_BRIDGE_QUESTIONNAIRE &&
            app->nodes[ancestor].questionnaire &&
            app->nodes[ancestor].questionnaire->state.initialized)
            return CUI_BRIDGE_INVALID_STATE;
    if (node->kind < CUI_BRIDGE_BUTTON ||
        (node->kind > CUI_BRIDGE_SLIDER && node->kind != CUI_BRIDGE_INPUT &&
         node->kind != CUI_BRIDGE_TEXTAREA && node->kind != CUI_BRIDGE_NUMBER_INPUT &&
         node->kind != CUI_BRIDGE_COLLAPSIBLE && node->kind != CUI_BRIDGE_RADIO_GROUP &&
         node->kind != CUI_BRIDGE_TABS && node->kind != CUI_BRIDGE_ACCORDION &&
         node->kind != CUI_BRIDGE_ITEM && node->kind != CUI_BRIDGE_SELECT &&
         node->kind != CUI_BRIDGE_COMBOBOX && node->kind != CUI_BRIDGE_DROPDOWN_BUTTON &&
         node->kind != CUI_BRIDGE_TOOLTIP && node->kind != CUI_BRIDGE_HOVER_CARD &&
         node->kind != CUI_BRIDGE_LIST && node->kind != CUI_BRIDGE_MENU &&
         node->kind != CUI_BRIDGE_STEPPER && node->kind != CUI_BRIDGE_PAGINATION &&
         node->kind != CUI_BRIDGE_RATING && node->kind != CUI_BRIDGE_TOOLBAR &&
         node->kind != CUI_BRIDGE_TREE && node->kind != CUI_BRIDGE_TIME_FIELD &&
         node->kind != CUI_BRIDGE_COLOR_PICKER && node->kind != CUI_BRIDGE_CALENDAR &&
         node->kind != CUI_BRIDGE_DATE_PICKER && node->kind != CUI_BRIDGE_OTP_INPUT &&
         node->kind != CUI_BRIDGE_FORM && node->kind != CUI_BRIDGE_INPUT_GROUP &&
         node->kind != CUI_BRIDGE_CAROUSEL && node->kind != CUI_BRIDGE_RESIZABLE &&
         node->kind != CUI_BRIDGE_TEXT_VIEW && node->kind != CUI_BRIDGE_EDITOR &&
         node->kind != CUI_BRIDGE_CHART && node->kind != CUI_BRIDGE_DATA_TABLE &&
         node->kind != CUI_BRIDGE_COMMAND && node->kind != CUI_BRIDGE_VIRTUAL_LIST &&
         node->kind != CUI_BRIDGE_SIDEBAR && node->kind != CUI_BRIDGE_SPEECH &&
         node->kind != CUI_BRIDGE_QUESTIONNAIRE && node->kind != CUI_BRIDGE_DOCK))
        return CUI_BRIDGE_INVALID_STATE;
    if (disabled > 1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->disabled = disabled != 0;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_total(cui_bridge_app *app, cui_bridge_node handle,
                                       uint32_t total) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_PAGINATION && node->kind != CUI_BRIDGE_RATING &&
        node->kind != CUI_BRIDGE_OTP_INPUT)
        return CUI_BRIDGE_INVALID_STATE;
    if (node->kind == CUI_BRIDGE_OTP_INPUT) {
        if (!total || total > 256)
            return CUI_BRIDGE_INVALID_ARGUMENT;
        return cui_otp_init(&node->otp_state, node->otp_buffer, 4097, total) == CUI_EDIT_OK
                   ? CUI_BRIDGE_OK
                   : CUI_BRIDGE_CORE_ERROR;
    }
    if (!total || (node->kind == CUI_BRIDGE_RATING ? total > 10 : total > 1000000))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->total = total;
    if (node->value > total)
        node->value = total;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_step_status(cui_bridge_app *app, cui_bridge_node handle,
                                             uint32_t step_status) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_ITEM || !node->parent ||
        (app->nodes[node->parent].kind != CUI_BRIDGE_STEPPER &&
         app->nodes[node->parent].kind != CUI_BRIDGE_STATUS_BAR &&
         app->nodes[node->parent].kind != CUI_BRIDGE_TREE &&
         app->nodes[node->parent].kind != CUI_BRIDGE_INPUT_GROUP &&
         app->nodes[node->parent].kind != CUI_BRIDGE_MESSAGE_SCROLLER))
        return CUI_BRIDGE_INVALID_STATE;
    if (step_status > (app->nodes[node->parent].kind == CUI_BRIDGE_MESSAGE_SCROLLER ? 1u
                       : app->nodes[node->parent].kind == CUI_BRIDGE_INPUT_GROUP    ? 3u
                       : app->nodes[node->parent].kind == CUI_BRIDGE_TREE           ? 63u
                       : app->nodes[node->parent].kind == CUI_BRIDGE_STATUS_BAR
                           ? 2u
                           : (uint32_t)CUI_STEP_ERROR))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->step_status = step_status;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_attachment_status(cui_bridge_app *app, cui_bridge_node handle,
                                                   cui_bridge_attachment_status value) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_ATTACHMENT)
        return CUI_BRIDGE_INVALID_STATE;
    if (value < CUI_BRIDGE_ATTACHMENT_PENDING || value > CUI_BRIDGE_ATTACHMENT_COMPLETE)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->attachment_status = value;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_image_rgba(cui_bridge_app *app, cui_bridge_node handle,
                                            const uint8_t *pixels, uint32_t width, uint32_t height,
                                            uint32_t stride, uint32_t byte_count) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_IMAGE)
        return CUI_BRIDGE_INVALID_STATE;
    if (!pixels || !width || !height || width > 2048 || height > 2048 || stride < width * 4u ||
        (uint64_t)stride * height > UINT32_C(16777216) || byte_count != (uint64_t)stride * height)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    unsigned char *copy = malloc(byte_count);
    if (!copy)
        return CUI_BRIDGE_OUT_OF_MEMORY;
    memcpy(copy, pixels, byte_count);
    free(node->image_pixels);
    node->image_pixels = copy;
    node->image_frame = (cui_image_frame){.pixels = copy,
                                          .width = width,
                                          .height = height,
                                          .stride = stride,
                                          .revision = node->image_frame.revision + 1};
    cui_image_state_reset(&node->image_state);
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_time(cui_bridge_app *app, cui_bridge_node handle,
                                      cui_bridge_time value) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_TIME_FIELD)
        return CUI_BRIDGE_INVALID_STATE;
    if (value.hour > 23 || value.minute > 59 || value.second > 59)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->time = (cui_time){value.hour, value.minute, value.second};
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_get_time(cui_bridge_app *app, cui_bridge_node handle,
                                      cui_bridge_time *out_time) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_TIME_FIELD)
        return CUI_BRIDGE_INVALID_STATE;
    if (!out_time)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    *out_time = (cui_bridge_time){node->time.hour, node->time.minute, node->time.second};
    return CUI_BRIDGE_OK;
}
static uint32_t bridge_pack_color(cui_color color) {
    return ((uint32_t)color.r << 24) | ((uint32_t)color.g << 16) | ((uint32_t)color.b << 8) |
           color.a;
}
cui_bridge_status cui_bridge_set_color_rgba(cui_bridge_app *app, cui_bridge_node handle,
                                            uint32_t rgba) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_COLOR_PICKER)
        return CUI_BRIDGE_INVALID_STATE;
    node->color = (cui_color){(uint8_t)(rgba >> 24), (uint8_t)(rgba >> 16), (uint8_t)(rgba >> 8),
                              (uint8_t)rgba};
    node->value = rgba;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_get_color_rgba(cui_bridge_app *app, cui_bridge_node handle,
                                            uint32_t *out_rgba) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_COLOR_PICKER)
        return CUI_BRIDGE_INVALID_STATE;
    if (!out_rgba)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    *out_rgba = bridge_pack_color(node->color);
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_date(cui_bridge_app *app, cui_bridge_node handle,
                                      cui_bridge_date value) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_CALENDAR && node->kind != CUI_BRIDGE_DATE_PICKER)
        return CUI_BRIDGE_INVALID_STATE;
    cui_date date = {value.year, value.month, value.day};
    if (!cui_date_valid(date))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->date_selection = (cui_date_selection){.start = date};
    node->value = (double)date.year * 10000 + date.month * 100 + date.day;
    cui_calendar_init(&node->calendar_state, date);
    cui_calendar_init(&node->date_picker_state.calendar, date);
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_get_date(cui_bridge_app *app, cui_bridge_node handle,
                                      cui_bridge_date *out_date) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_CALENDAR && node->kind != CUI_BRIDGE_DATE_PICKER)
        return CUI_BRIDGE_INVALID_STATE;
    if (!out_date)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_date date = node->date_selection.start;
    *out_date = (cui_bridge_date){date.year, date.month, date.day};
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_value(cui_bridge_app *app, cui_bridge_node handle, double value) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind == CUI_BRIDGE_ITEM && node->parent &&
        app->nodes[node->parent].kind == CUI_BRIDGE_RESIZABLE) {
        if (!isfinite(value) || value < 100 || value > 4096)
            return CUI_BRIDGE_INVALID_ARGUMENT;
        cui_bridge_node_record *group = &app->nodes[node->parent];
        if (group->resizable_state)
            for (size_t i = 0; i < group->resizable_state->count; i++)
                if (group->resizable_state->slots[i].id == node->id) {
                    (void)cui_resizable_resize_panel(group->resizable_state, i, (float)value);
                    for (uint32_t child = group->first_child; child;
                         child = app->nodes[child].next_sibling)
                        for (size_t j = 0; j < group->resizable_state->count; j++)
                            if (group->resizable_state->slots[j].id == app->nodes[child].id)
                                app->nodes[child].value = group->resizable_state->slots[j].size;
                    return CUI_BRIDGE_OK;
                }
        node->value = value;
        return CUI_BRIDGE_OK;
    }
    if (node->kind == CUI_BRIDGE_VIRTUAL_LIST) {
        if (!isfinite(value) || value < 0 || value > 1000000)
            return CUI_BRIDGE_INVALID_ARGUMENT;
        node->virtual_state.scroll.y = (float)value;
        node->value = value;
        return CUI_BRIDGE_OK;
    }
    if (node->kind == CUI_BRIDGE_MESSAGE_SCROLLER) {
        if (!isfinite(value) || value < 0 || value > 1000000)
            return CUI_BRIDGE_INVALID_ARGUMENT;
        if (!node->message_scroller)
            return CUI_BRIDGE_INVALID_STATE;
        return cui_message_scroller_scroll(node->message_scroller, value) ? CUI_BRIDGE_OK
                                                                          : CUI_BRIDGE_CORE_ERROR;
    }
    if (node->kind != CUI_BRIDGE_SLIDER && node->kind != CUI_BRIDGE_PROGRESS &&
        node->kind != CUI_BRIDGE_NUMBER_INPUT && node->kind != CUI_BRIDGE_PAGINATION &&
        node->kind != CUI_BRIDGE_RATING && node->kind != CUI_BRIDGE_ATTACHMENT)
        return CUI_BRIDGE_INVALID_STATE;
    if (node->kind == CUI_BRIDGE_PAGINATION || node->kind == CUI_BRIDGE_RATING) {
        if (!isfinite(value) || value < (node->kind == CUI_BRIDGE_PAGINATION ? 1 : 0) ||
            value > node->total || (node->kind == CUI_BRIDGE_PAGINATION && floor(value) != value))
            return CUI_BRIDGE_INVALID_ARGUMENT;
        node->value = value;
        return CUI_BRIDGE_OK;
    }
    if (!isfinite(value) || value < node->minimum || value > node->maximum)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->value = value;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_range(cui_bridge_app *app, cui_bridge_node handle, double minimum,
                                       double maximum, double step) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_SLIDER && node->kind != CUI_BRIDGE_NUMBER_INPUT)
        return CUI_BRIDGE_INVALID_STATE;
    if (!isfinite(minimum) || !isfinite(maximum) || !isfinite(step) || minimum >= maximum ||
        step <= 0 ||
        (node->kind == CUI_BRIDGE_SLIDER &&
         (minimum < -16777216.0 || maximum > 16777216.0 || step > 16777216.0)))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->minimum = minimum;
    node->maximum = maximum;
    node->step = step;
    if (node->value < minimum)
        node->value = minimum;
    if (node->value > maximum)
        node->value = maximum;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_get_checked(cui_bridge_app *app, cui_bridge_node handle,
                                         uint32_t *out_checked) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out_checked)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (!bridge_boolean_kind(node->kind) && node->kind != CUI_BRIDGE_SPEECH)
        return CUI_BRIDGE_INVALID_STATE;
    if (node->kind == CUI_BRIDGE_ITEM &&
        (!node->parent || (app->nodes[node->parent].kind != CUI_BRIDGE_ACCORDION &&
                           app->nodes[node->parent].kind != CUI_BRIDGE_TREE)))
        return CUI_BRIDGE_INVALID_STATE;
    *out_checked = node->checked;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_get_value(cui_bridge_app *app, cui_bridge_node handle,
                                       double *out_value) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out_value)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (node->kind == CUI_BRIDGE_ITEM && node->parent &&
        app->nodes[node->parent].kind == CUI_BRIDGE_RESIZABLE) {
        *out_value = node->value;
        return CUI_BRIDGE_OK;
    }
    if (node->kind != CUI_BRIDGE_SLIDER && node->kind != CUI_BRIDGE_PROGRESS &&
        node->kind != CUI_BRIDGE_NUMBER_INPUT && node->kind != CUI_BRIDGE_PAGINATION &&
        node->kind != CUI_BRIDGE_RATING && node->kind != CUI_BRIDGE_SCROLLABLE &&
        node->kind != CUI_BRIDGE_ATTACHMENT && node->kind != CUI_BRIDGE_MESSAGE_SCROLLER &&
        node->kind != CUI_BRIDGE_VIRTUAL_LIST)
        return CUI_BRIDGE_INVALID_STATE;
    *out_value = node->value;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_get_text(cui_bridge_app *app, cui_bridge_node handle,
                                      char *destination, uint32_t capacity,
                                      uint32_t *required_capacity) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!required_capacity)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (node->kind < CUI_BRIDGE_LABEL)
        return CUI_BRIDGE_INVALID_STATE;
    cui_str value = node->kind == CUI_BRIDGE_OTP_INPUT ? cui_otp_value(&node->otp_state)
                    : node->edit && node->kind != CUI_BRIDGE_NUMBER_INPUT
                        ? cui_edit_get_text(node->edit)
                        : cui_string(node->text);
    uint32_t required = (uint32_t)value.length + 1;
    if (capacity < required || !destination) {
        *required_capacity = required;
        return CUI_BRIDGE_CAPACITY;
    }
    memcpy(destination, value.data, value.length);
    destination[value.length] = 0;
    *required_capacity = required;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_text_view_copy_selection(cui_bridge_app *app, cui_bridge_node handle,
                                                      char *destination, uint32_t capacity,
                                                      uint32_t *required_capacity) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_TEXT_VIEW)
        return CUI_BRIDGE_INVALID_STATE;
    if (!required_capacity)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (!node->text_view) {
        *required_capacity = 1;
        return CUI_BRIDGE_INVALID_STATE;
    }
    size_t length = 0;
    cui_text_view_error result = cui_text_view_copy_selection(
        &node->text_view->state, CUI_TEXT_VIEW_COPY_PLAIN, destination, capacity, &length);
    *required_capacity = (uint32_t)length + 1;
    if (result == CUI_TEXT_VIEW_OK)
        return CUI_BRIDGE_OK;
    return result == CUI_TEXT_VIEW_CAPACITY ? CUI_BRIDGE_CAPACITY : CUI_BRIDGE_CORE_ERROR;
}
cui_bridge_status cui_bridge_text_view_select(cui_bridge_app *app, cui_bridge_node handle,
                                              uint32_t anchor, uint32_t cursor) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_TEXT_VIEW)
        return CUI_BRIDGE_INVALID_STATE;
    if (!node->text_view)
        return CUI_BRIDGE_INVALID_STATE;
    cui_str rendered = cui_text_view_rendered_text(&node->text_view->state);
    if (anchor > rendered.length || cursor > rendered.length ||
        (anchor < rendered.length && ((unsigned char)rendered.data[anchor] & 0xc0) == 0x80) ||
        (cursor < rendered.length && ((unsigned char)rendered.data[cursor] & 0xc0) == 0x80))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    (void)cui_text_view_select(&node->text_view->state, anchor, cursor);
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_queue_text(cui_bridge_app *app, const char *text_utf8) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    size_t length = 0;
    if (!bridge_utf8(text_utf8, &length))
        return CUI_BRIDGE_INVALID_UTF8;
    cui_event event = {.type = CUI_EVENT_TEXT, .text_data = text_utf8, .text_length = length};
    return cui_queue_event(app->ui, event) ? CUI_BRIDGE_OK : CUI_BRIDGE_CAPACITY;
}
cui_bridge_status cui_bridge_set_gap(cui_bridge_app *app, cui_bridge_node handle, float gap) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if ((node->kind != CUI_BRIDGE_ROW && node->kind != CUI_BRIDGE_COLUMN) || !isfinite(gap) ||
        gap < 0 || gap > 4096)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->gap = gap;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_root_padding(cui_bridge_app *app, cui_bridge_node handle,
                                              float padding) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_ROOT_VIEW)
        return CUI_BRIDGE_INVALID_STATE;
    if (!isfinite(padding) || padding < 0 || padding > 256)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->value = padding;
    return CUI_BRIDGE_OK;
}
static cui_bridge_status bridge_button(cui_bridge_app *app, cui_bridge_node handle,
                                       cui_bridge_node_record **out) {
    cui_bridge_status status = bridge_valid_node(app, handle, out);
    if (status != CUI_BRIDGE_OK)
        return status;
    return (*out)->kind == CUI_BRIDGE_BUTTON ? CUI_BRIDGE_OK : CUI_BRIDGE_INVALID_STATE;
}
cui_bridge_status cui_bridge_set_button_variant(cui_bridge_app *app, cui_bridge_node handle,
                                                cui_bridge_button_variant variant) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_button(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (variant < CUI_BRIDGE_BUTTON_PRIMARY || variant > CUI_BRIDGE_BUTTON_GHOST ||
        (variant == CUI_BRIDGE_BUTTON_GHOST && node->custom_hover))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->variant = variant;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_button_size(cui_bridge_app *app, cui_bridge_node handle,
                                             cui_bridge_button_size size) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_button(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (size < CUI_BRIDGE_SIZE_INHERIT || size > CUI_BRIDGE_SIZE_9XL)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->size = size;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_button_rounding(cui_bridge_app *app, cui_bridge_node handle,
                                                 cui_bridge_button_rounding rounding,
                                                 float custom_radius) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_button(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (rounding < CUI_BRIDGE_ROUND_THEME || rounding > CUI_BRIDGE_ROUND_CUSTOM ||
        (rounding == CUI_BRIDGE_ROUND_CUSTOM &&
         (!isfinite(custom_radius) || custom_radius < 0 || custom_radius > 4096)))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->rounding = rounding;
    node->custom_radius = custom_radius;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_button_disabled(cui_bridge_app *app, cui_bridge_node handle,
                                                 uint32_t disabled) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_button(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (disabled > 1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->disabled = disabled != 0;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_button_hover_rgba(cui_bridge_app *app, cui_bridge_node handle,
                                                   uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_button(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->variant == CUI_BRIDGE_BUTTON_GHOST)
        return CUI_BRIDGE_INVALID_STATE;
    node->custom_hover = true;
    node->hover = (cui_color){r, g, b, a};
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_queue_pointer(cui_bridge_app *app, cui_bridge_pointer_phase phase,
                                           float x, float y) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (phase < CUI_BRIDGE_POINTER_MOVE || phase > CUI_BRIDGE_POINTER_UP || !isfinite(x) ||
        !isfinite(y))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_event e = {.type = phase == CUI_BRIDGE_POINTER_MOVE   ? CUI_EVENT_POINTER_MOVE
                           : phase == CUI_BRIDGE_POINTER_DOWN ? CUI_EVENT_POINTER_DOWN
                                                              : CUI_EVENT_POINTER_UP,
                   .x = x,
                   .y = y,
                   .pointer_type = CUI_POINTER_MOUSE,
                   .pointer_button = CUI_POINTER_PRIMARY};
    return cui_queue_event(app->ui, e) ? CUI_BRIDGE_OK : CUI_BRIDGE_CAPACITY;
}
cui_bridge_status cui_bridge_queue_key(cui_bridge_app *app, cui_bridge_key_phase phase,
                                       cui_bridge_key key, uint32_t modifiers, uint32_t repeat) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (phase < CUI_BRIDGE_KEY_DOWN || phase > CUI_BRIDGE_KEY_UP || key < CUI_BRIDGE_KEY_TAB ||
        key > CUI_BRIDGE_KEY_DOWN_ARROW || (modifiers & ~15u) || repeat > 1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_event e = {.type = phase == CUI_BRIDGE_KEY_DOWN ? CUI_EVENT_KEY_DOWN : CUI_EVENT_KEY_UP,
                   .key = (cui_key)key,
                   .modifiers = modifiers,
                   .repeat = repeat != 0};
    return cui_queue_event(app->ui, e) ? CUI_BRIDGE_OK : CUI_BRIDGE_CAPACITY;
}
cui_bridge_status cui_bridge_queue_scroll(cui_bridge_app *app, float x, float y, float dx,
                                          float dy) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!isfinite(x) || !isfinite(y) || !isfinite(dx) || !isfinite(dy))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_event event = {.type = CUI_EVENT_SCROLL, .x = x, .y = y, .dx = dx, .dy = dy};
    return cui_queue_event(app->ui, event) ? CUI_BRIDGE_OK : CUI_BRIDGE_CAPACITY;
}
cui_bridge_status cui_bridge_set_font_fallbacks(cui_bridge_app *app, const char *const *paths,
                                                uint32_t count) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (count > 32 || (count && !paths))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    for (uint32_t i = 0; i < count; i++) {
        size_t length = 0;
        if (!bridge_utf8(paths[i], &length) || !length)
            return CUI_BRIDGE_INVALID_UTF8;
    }
    if (app->backend != CUI_BRIDGE_BACKEND_SDL)
        return CUI_BRIDGE_BACKEND_UNAVAILABLE;
    return cui_bridge_native_set_font_fallbacks(app, paths, count);
}
typedef struct {
    cui_bridge_app *app;
    uint32_t slot;
} bridge_click_binding;
static void bridge_click(void *user, const cui_button_click_event *event) {
    bridge_click_binding *binding = user;
    cui_bridge_app *app = binding->app;
    if (app->event_count == app->max_events) {
        app->frame_error = CUI_BRIDGE_CAPACITY;
        return;
    }
    cui_bridge_node_record *node = &app->nodes[binding->slot];
    uint32_t tail = (app->event_head + app->event_count) % app->max_events;
    app->event_payloads[tail] = (cui_bridge_event_payload){
        .node_kind = app->nodes[binding->slot].kind, .payload_kind = CUI_BRIDGE_PAYLOAD_NONE};
    app->events[tail] = (cui_bridge_event){.type = CUI_BRIDGE_EVENT_CLICK,
                                           .node = {binding->slot, node->generation},
                                           .frame_sequence = event->frame_sequence};
    app->event_count++;
}
static void bridge_change(cui_bridge_app *app, uint32_t slot) {
    if (app->event_count == app->max_events) {
        app->frame_error = CUI_BRIDGE_CAPACITY;
        return;
    }
    uint32_t tail = (app->event_head + app->event_count) % app->max_events;
    cui_bridge_node_record *node = &app->nodes[slot];
    cui_bridge_event_payload payload = {.checked = node->checked, .value = node->value,
                                       .node_kind = node->kind,
                                       .payload_kind = CUI_BRIDGE_PAYLOAD_LEGACY};
    if (node->kind == CUI_BRIDGE_CHECKBOX || node->kind == CUI_BRIDGE_SWITCH ||
        node->kind == CUI_BRIDGE_TOGGLE) {
        payload.payload_kind = CUI_BRIDGE_PAYLOAD_BOOLEAN;
        payload.notification.boolean_value = node->checked ? 1u : 0u;
    }
    if (node->kind == CUI_BRIDGE_SLIDER || node->kind == CUI_BRIDGE_NUMBER_INPUT ||
        node->kind == CUI_BRIDGE_RATING || node->kind == CUI_BRIDGE_PAGINATION) {
        payload.payload_kind = CUI_BRIDGE_PAYLOAD_SCALAR;
        payload.notification.scalar_value = node->value;
    }
    if ((node->kind == CUI_BRIDGE_RADIO_GROUP || node->kind == CUI_BRIDGE_TABS ||
         node->kind == CUI_BRIDGE_SELECT || node->kind == CUI_BRIDGE_COMBOBOX ||
         node->kind == CUI_BRIDGE_DROPDOWN_BUTTON || node->kind == CUI_BRIDGE_LIST ||
         node->kind == CUI_BRIDGE_MENU || node->kind == CUI_BRIDGE_STEPPER ||
         node->kind == CUI_BRIDGE_TREE || node->kind == CUI_BRIDGE_CAROUSEL ||
         node->kind == CUI_BRIDGE_COMMAND || node->kind == CUI_BRIDGE_SIDEBAR ||
         node->kind == CUI_BRIDGE_DOCK || node->kind == CUI_BRIDGE_SETTINGS) &&
        node->value > 0) {
        uint32_t selected = (uint32_t)node->value;
        if (selected <= app->max_nodes && app->nodes[selected].live &&
            app->nodes[selected].parent == slot)
            payload.selected = (cui_bridge_node){selected, app->nodes[selected].generation};
    }
    if ((node->edit && node->kind != CUI_BRIDGE_NUMBER_INPUT) ||
        node->kind == CUI_BRIDGE_OTP_INPUT || node->kind == CUI_BRIDGE_TEXT_VIEW ||
        node->kind == CUI_BRIDGE_SPEECH) {
        cui_str value = node->kind == CUI_BRIDGE_OTP_INPUT ? cui_otp_value(&node->otp_state)
                        : node->kind == CUI_BRIDGE_TEXT_VIEW
                            ? cui_text_view_selected_text(&node->text_view->state)
                        : node->kind == CUI_BRIDGE_SPEECH
                            ? cui_speech_transcript(&node->speech->state)
                            : cui_edit_get_text(node->edit);
        payload.text = malloc(value.length + 1);
        if (!payload.text) {
            app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
            return;
        }
        memcpy(payload.text, value.data, value.length);
        payload.text[value.length] = 0;
        payload.text_length = (uint32_t)value.length;
    } else if (node->kind == CUI_BRIDGE_TIME_FIELD) {
        char formatted[16];
        size_t length = cui_time_format(node->time, true, formatted, sizeof formatted);
        if (!length) {
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
            return;
        }
        payload.text = malloc(length + 1);
        if (!payload.text) {
            app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
            return;
        }
        memcpy(payload.text, formatted, length + 1);
        payload.text_length = (uint32_t)length;
    } else if (node->kind == CUI_BRIDGE_COLOR_PICKER) {
        char formatted[16];
        size_t length = cui_color_format_hex(node->color, true, formatted, sizeof formatted);
        if (!length) {
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
            return;
        }
        payload.text = malloc(length + 1);
        if (!payload.text) {
            app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
            return;
        }
        memcpy(payload.text, formatted, length + 1);
        payload.text_length = (uint32_t)length;
    } else if (node->kind == CUI_BRIDGE_QUESTIONNAIRE && node->questionnaire) {
        bridge_questionnaire_owned *owned = node->questionnaire;
        char snapshot[192];
        int written =
            snprintf(snapshot, sizeof snapshot,
                     "{\"event\":%u,\"item\":%u,\"current\":%u,\"complete\":%u,\"selected\":[",
                     owned->last_event, owned->last_item == UINT32_MAX ? 0 : owned->last_item + 1,
                     owned->state.current < owned->count ? (uint32_t)owned->state.current + 1 : 0,
                     (unsigned)owned->state.complete);
        if (written < 0 || (size_t)written >= sizeof snapshot) {
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
            return;
        }
        size_t length = (size_t)written;
        bool separator = false;
        if (owned->last_item < owned->count)
            for (size_t c = 0; c < owned->items[owned->last_item].choice_count; c++)
                if (cui_questionnaire_choice_selected(&owned->state, owned->last_item, c)) {
                    written = snprintf(snapshot + length, sizeof snapshot - length, "%s%u",
                                       separator ? "," : "", (unsigned)c + 1);
                    if (written < 0 || (size_t)written >= sizeof snapshot - length) {
                        app->frame_error = CUI_BRIDGE_CORE_ERROR;
                        return;
                    }
                    length += (size_t)written;
                    separator = true;
                }
        if (sizeof snapshot - length < 3) {
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
            return;
        }
        snapshot[length++] = ']';
        snapshot[length++] = '}';
        snapshot[length] = 0;
        payload.text = malloc(length + 1);
        if (!payload.text) {
            app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
            return;
        }
        memcpy(payload.text, snapshot, length + 1);
        payload.text_length = (uint32_t)length;
    } else if (node->kind == CUI_BRIDGE_CALENDAR || node->kind == CUI_BRIDGE_DATE_PICKER) {
        char formatted[16];
        size_t length =
            cui_date_format_locale(node->date_selection.start, NULL, formatted, sizeof formatted);
        if (!length || length >= sizeof formatted) {
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
            return;
        }
        payload.text = malloc(length + 1);
        if (!payload.text) {
            app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
            return;
        }
        memcpy(payload.text, formatted, length + 1);
        payload.text_length = (uint32_t)length;
    }
    app->event_payloads[tail] = payload;
    app->events[tail] = (cui_bridge_event){.type = CUI_BRIDGE_EVENT_CHANGE,
                                           .node = {slot, app->nodes[slot].generation},
                                           .frame_sequence = cui_frame_sequence(app->ui)};
    app->event_count++;
}
static void bridge_data_table_change(cui_bridge_app *app, uint32_t slot, const char *reason) {
    if (app->event_count == app->max_events) {
        app->frame_error = CUI_BRIDGE_CAPACITY;
        return;
    }
    cui_bridge_node_record *node = &app->nodes[slot];
    cui_table_state *state = &node->data_table->state;
    char snapshot[128];
    int length = snprintf(
        snapshot, sizeof snapshot,
        "{\"reason\":\"%s\",\"row\":%u,\"column\":%u,\"sortColumn\":%u,\"sortDirection\":%u}",
        reason, (unsigned)state->row, (unsigned)state->column, (unsigned)state->sort_column,
        (unsigned)state->sort);
    if (length < 0 || (size_t)length >= sizeof snapshot) {
        app->frame_error = CUI_BRIDGE_CORE_ERROR;
        return;
    }
    char *copy = malloc((size_t)length + 1);
    if (!copy) {
        app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
        return;
    }
    memcpy(copy, snapshot, (size_t)length + 1);
    uint32_t tail = (app->event_head + app->event_count) % app->max_events;
    app->event_payloads[tail] = (cui_bridge_event_payload){
        .node_kind = node->kind, .payload_kind = CUI_BRIDGE_PAYLOAD_TABLE,
        .notification.table = {.reason = strcmp(reason, "sort") == 0 ? CUI_BRIDGE_TABLE_SORT
                                                                       : CUI_BRIDGE_TABLE_SELECTION,
                               .row = (uint32_t)state->row, .column = (uint32_t)state->column,
                               .sort_column = (uint32_t)state->sort_column,
                               .sort_direction = (uint32_t)state->sort},
        .checked = (uint32_t)state->column,
                                                           .value = (double)state->row,
                                                           .text = copy,
                                                           .text_length = (uint32_t)length};
    app->events[tail] = (cui_bridge_event){.type = CUI_BRIDGE_EVENT_CHANGE,
                                           .node = {slot, node->generation},
                                           .frame_sequence = cui_frame_sequence(app->ui)};
    app->event_count++;
}
static void bridge_click_simple(cui_bridge_app *app, uint32_t slot) {
    if (app->event_count == app->max_events) {
        app->frame_error = CUI_BRIDGE_CAPACITY;
        return;
    }
    uint32_t tail = (app->event_head + app->event_count) % app->max_events;
    app->event_payloads[tail] = (cui_bridge_event_payload){
        .node_kind = app->nodes[slot].kind, .payload_kind = CUI_BRIDGE_PAYLOAD_NONE};
    app->events[tail] = (cui_bridge_event){.type = CUI_BRIDGE_EVENT_CLICK,
                                           .node = {slot, app->nodes[slot].generation},
                                           .frame_sequence = cui_frame_sequence(app->ui)};
    app->event_count++;
}
static void bridge_surface_close(cui_bridge_app *app, uint32_t slot, cui_surface_result result) {
    if (app->event_count == app->max_events) {
        app->frame_error = CUI_BRIDGE_CAPACITY;
        return;
    }
    uint32_t tail = (app->event_head + app->event_count) % app->max_events;
    app->event_payloads[tail] = (cui_bridge_event_payload){
        .node_kind = app->nodes[slot].kind, .payload_kind = CUI_BRIDGE_PAYLOAD_SURFACE,
        .notification.surface_result = (uint32_t)result, .value = result};
    app->events[tail] = (cui_bridge_event){.type = CUI_BRIDGE_EVENT_CLOSE,
                                           .node = {slot, app->nodes[slot].generation},
                                           .frame_sequence = cui_frame_sequence(app->ui)};
    app->event_count++;
}
static void bridge_notification_click(void *user, cui_notification_id id) {
    cui_bridge_node_record *node = user;
    if (node && node->id == id.key && node->live)
        bridge_click_simple(node->owner, node->own_slot);
}
static void bridge_notification_close(void *user, cui_notification_id id) {
    cui_bridge_node_record *node = user;
    if (node && node->id == id.key && node->live)
        bridge_surface_close(node->owner, node->own_slot, CUI_SURFACE_CLOSED);
}
static void bridge_render_node(cui_bridge_app *app, uint32_t slot);
typedef struct {
    cui_bridge_app *app;
    uint32_t slot;
} bridge_panel_binding;
static void bridge_panel(cui_context *ui, size_t index, void *user) {
    (void)ui;
    (void)index;
    bridge_panel_binding *binding = user;
    cui_bridge_node_record *node = &binding->app->nodes[binding->slot];
    for (uint32_t child = node->first_child; child; child = binding->app->nodes[child].next_sibling)
        bridge_render_node(binding->app, child);
}
static void bridge_root_content(cui_context *ui, void *user) {
    bridge_panel(ui, 0, user);
}
static void bridge_dock_content(cui_context *ui, uint64_t panel, void *user) {
    (void)ui;
    (void)panel;
    cui_bridge_node_record *item = user;
    for (uint32_t child = item->first_child; child; child = item->owner->nodes[child].next_sibling)
        bridge_render_node(item->owner, child);
}
typedef struct {
    cui_bridge_app *app;
    uint32_t slots[64];
} bridge_group_binding;
typedef struct {
    bridge_group_binding binding;
    cui_radio_group_item radios[64];
    cui_choice_item tabs[64];
    cui_menu_entry menu_entries[64];
    cui_step steps[64];
    cui_disclosure_item disclosures[64];
    bool opened[64];
    cui_choice_item filtered[64];
} bridge_group_scratch;
static void bridge_group_panel(cui_context *ui, size_t index, void *user) {
    (void)ui;
    bridge_group_binding *binding = user;
    uint32_t slot = binding->slots[index];
    for (uint32_t child = binding->app->nodes[slot].first_child; child;
         child = binding->app->nodes[child].next_sibling)
        bridge_render_node(binding->app, child);
}
static void bridge_toolbar_render(cui_context *ui, const cui_theme *theme,
                                  const cui_toolbar_presentation *presentation, void *user) {
    (void)ui;
    (void)presentation;
    bridge_panel_binding *binding = user;
    cui_bridge_node_record *item = &binding->app->nodes[binding->slot];
    if (!item->first_child)
        cui_text(binding->app->ui, cui_string(item->text), theme->font_size, theme->text);
    bool previous = binding->app->render_disabled;
    binding->app->render_disabled =
        previous || binding->app->nodes[item->parent].disabled || item->disabled;
    for (uint32_t child = item->first_child; child; child = binding->app->nodes[child].next_sibling)
        bridge_render_node(binding->app, child);
    binding->app->render_disabled = previous;
}
static void bridge_form_render(cui_context *ui, const cui_theme *theme, void *user, float width) {
    (void)ui;
    (void)theme;
    (void)width;
    bridge_panel_binding *binding = user;
    cui_bridge_node_record *item = &binding->app->nodes[binding->slot];
    bool previous = binding->app->render_disabled;
    binding->app->render_disabled =
        previous || item->disabled || binding->app->nodes[item->parent].disabled;
    for (uint32_t child = item->first_child; child; child = binding->app->nodes[child].next_sibling)
        bridge_render_node(binding->app, child);
    binding->app->render_disabled = previous;
}
static void bridge_render_form(cui_bridge_app *app, cui_bridge_node_record *node) {
    typedef struct {
        cui_form_field fields[64];
        bridge_panel_binding bindings[64];
    } scratch_type;
    scratch_type *scratch = malloc(sizeof *scratch);
    if (!scratch) {
        app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
        return;
    }
    size_t count = 0;
    for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling) {
        cui_bridge_node_record *item = &app->nodes[child];
        scratch->bindings[count] = (bridge_panel_binding){app, child};
        scratch->fields[count] =
            (cui_form_field){.id = item->id,
                             .control = item->first_child ? app->nodes[item->first_child].id : 0,
                             .label = cui_string(item->text),
                             .description = cui_string(item->detail),
                             .disabled = item->disabled || node->disabled,
                             .render_control = bridge_form_render,
                             .user = &scratch->bindings[count]};
        count++;
    }
    cui_form_options options = cui_form_options_default();
    options.width = node->visual_width > 0 ? node->visual_width : 480;
    options.name = cui_string(node->text);
    if (cui_form(app->ui, node->id, &app->theme, scratch->fields, count, options) != CUI_FORM_OK)
        app->frame_error = CUI_BRIDGE_CORE_ERROR;
    free(scratch);
}
static void bridge_render_input_group(cui_bridge_app *app, uint32_t slot,
                                      cui_bridge_node_record *node) {
    cui_input_group_addon addons[64];
    uint32_t slots[64];
    size_t count = 0;
    for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling) {
        cui_bridge_node_record *item = &app->nodes[child];
        slots[count] = child;
        addons[count] =
            (cui_input_group_addon){.id = item->id,
                                    .alignment = (cui_input_group_alignment)item->step_status,
                                    .kind = CUI_INPUT_GROUP_BUTTON,
                                    .label = cui_string(item->text),
                                    .disabled = item->disabled};
        count++;
    }
    cui_input_group_options options = {.width = node->visual_width > 0 ? node->visual_width : 280,
                                       .name = cui_string(node->key),
                                       .placeholder = cui_string(node->detail),
                                       .input = {.disabled = node->disabled}};
    cui_input_group_result result =
        cui_input_group(app->ui, node->id, node->id | (UINT64_C(1) << 61), &app->theme, node->edit,
                        &node->input_group_state, addons, count, options);
    if (result.error != CUI_FORM_OK || result.input.error != CUI_EDIT_OK)
        app->frame_error = CUI_BRIDGE_CORE_ERROR;
    if (result.input.changed)
        bridge_change(app, slot);
    if (result.activated_addon)
        for (size_t i = 0; i < count; ++i)
            if (addons[i].id == result.activated_addon)
                bridge_click_simple(app, slots[i]);
}
static void bridge_render_group(cui_bridge_app *app, uint32_t slot, cui_bridge_node_record *node) {
    bridge_group_scratch *scratch = malloc(sizeof *scratch);
    if (!scratch) {
        app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
        return;
    }
    scratch->binding.app = app;
    size_t count = 0;
    for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling) {
        cui_bridge_node_record *item = &app->nodes[child];
        scratch->binding.slots[count] = child;
        scratch->radios[count] =
            (cui_radio_group_item){item->id, cui_string(item->text), item->disabled};
        scratch->tabs[count] =
            (cui_choice_item){item->id, cui_string(item->text), item->disabled || node->disabled};
        scratch->menu_entries[count] = (cui_menu_entry){.id = item->id,
                                                        .label = cui_string(item->text),
                                                        .disabled = item->disabled,
                                                        .kind = CUI_MENU_ACTION};
        scratch->steps[count] = (cui_step){.title = cui_string(item->text),
                                           .description = cui_string(item->detail),
                                           .status = (cui_step_status)item->step_status,
                                           .disabled = item->disabled};
        scratch->disclosures[count] =
            (cui_disclosure_item){item->id, cui_string(item->text), item->disabled};
        scratch->opened[count] = item->checked;
        count++;
    }
    if (node->kind == CUI_BRIDGE_RADIO_GROUP) {
        cui_id selected = 0;
        if (node->value > 0) {
            uint32_t index = (uint32_t)node->value;
            if (index <= app->max_nodes && app->nodes[index].live &&
                app->nodes[index].parent == slot)
                selected = app->nodes[index].id;
        }
        cui_radio_group_result result = cui_radio_group(
            app->ui, node->id, &app->theme, scratch->radios, count, &selected,
            &node->radio_group_state,
            (cui_radio_group_options){.vertical = true, .disabled = node->disabled});
        if (result.changed) {
            for (size_t i = 0; i < count; i++)
                if (scratch->radios[i].id == selected)
                    node->value = scratch->binding.slots[i];
            bridge_change(app, slot);
        }
    } else if (node->kind == CUI_BRIDGE_TABS) {
        cui_id selected = 0;
        if (node->value > 0) {
            uint32_t index = (uint32_t)node->value;
            if (index <= app->max_nodes && app->nodes[index].live &&
                app->nodes[index].parent == slot)
                selected = app->nodes[index].id;
        }
        if (cui_tabs(app->ui, node->id, &app->theme, scratch->tabs, count, &selected,
                     (cui_tabs_options){.disabled = node->disabled}, bridge_group_panel,
                     &scratch->binding)) {
            for (size_t i = 0; i < count; i++)
                if (scratch->tabs[i].id == selected)
                    node->value = scratch->binding.slots[i];
            bridge_change(app, slot);
        }
    } else if (node->kind == CUI_BRIDGE_LIST || node->kind == CUI_BRIDGE_MENU) {
        cui_id selected = 0;
        if (node->value > 0) {
            uint32_t index = (uint32_t)node->value;
            if (index <= app->max_nodes && app->nodes[index].live &&
                app->nodes[index].parent == slot)
                selected = app->nodes[index].id;
        }
        bool changed =
            node->kind == CUI_BRIDGE_LIST
                ? cui_list(app->ui, node->id, &app->theme, scratch->tabs, count, &selected,
                           &node->list_state, (cui_list_options){0})
                : cui_menu(app->ui, node->id, &app->theme, cui_string(node->text), scratch->tabs,
                           count, &selected, &node->choice_popup,
                           (cui_choice_popup_options){.disabled = node->disabled,
                                                      .viewport = {0, 0, app->width, app->height},
                                                      .dt = .016f});
        if (changed) {
            for (size_t i = 0; i < count; i++)
                if (scratch->tabs[i].id == selected)
                    node->value = scratch->binding.slots[i];
            bridge_change(app, slot);
        }
    } else if (node->kind == CUI_BRIDGE_STEPPER) {
        if (count) {
            size_t current = 0;
            for (size_t i = 0; i < count; i++)
                if (scratch->binding.slots[i] == (uint32_t)node->value)
                    current = i;
            if (cui_stepper(app->ui, node->id, &app->theme, scratch->steps, count, &current, false,
                            node->disabled)) {
                node->value = scratch->binding.slots[current];
                bridge_change(app, slot);
            }
        }
    } else if (node->kind == CUI_BRIDGE_DROPDOWN_BUTTON) {
        cui_id action = 0;
        cui_dropdown_button_options options = {
            .disabled = node->disabled,
            .menu = {.popup = {.viewport = {0, 0, app->width, app->height}, .dt = .016f}}};
        cui_dropdown_button_result result = cui_dropdown_button(
            app->ui, node->id, node->id | (UINT64_C(1) << 61), node->id | (UINT64_C(1) << 60),
            &app->theme, cui_string(node->text), cui_string(node->detail), scratch->menu_entries,
            count, &action, &node->menu_state, options);
        if (result.action_activated)
            bridge_click_simple(app, slot);
        if (result.menu_activated) {
            for (size_t i = 0; i < count; i++)
                if (scratch->menu_entries[i].id == action)
                    node->value = scratch->binding.slots[i];
            bridge_change(app, slot);
        }
    } else if (node->kind == CUI_BRIDGE_SELECT || node->kind == CUI_BRIDGE_COMBOBOX) {
        cui_id selected = 0;
        if (node->value > 0) {
            uint32_t index = (uint32_t)node->value;
            if (index <= app->max_nodes && app->nodes[index].live &&
                app->nodes[index].parent == slot)
                selected = app->nodes[index].id;
        }
        cui_choice_popup_options options = {
            .disabled = node->disabled, .viewport = {0, 0, app->width, app->height}, .dt = .016f};
        bool changed = false;
        if (node->kind == CUI_BRIDGE_SELECT) {
            changed = cui_select(app->ui, node->id, &app->theme, cui_string(node->text),
                                 cui_string(node->detail), scratch->tabs, count, &selected,
                                 &node->choice_popup, options);
        } else {
            cui_combobox_result result = cui_combobox(
                app->ui, node->id, &app->theme, cui_string(node->detail), node->edit, scratch->tabs,
                count, scratch->filtered, 64, &selected, &node->choice_popup, options);
            changed = result.changed;
            if (result.query_changed)
                bridge_change(app, slot);
            if (result.error != CUI_EDIT_OK)
                app->frame_error = CUI_BRIDGE_CORE_ERROR;
        }
        if (changed) {
            for (size_t i = 0; i < count; i++)
                if (scratch->tabs[i].id == selected)
                    node->value = scratch->binding.slots[i];
            bridge_change(app, slot);
        }
    } else {
        if (cui_accordion(app->ui, node->id, &app->theme, scratch->disclosures, count,
                          scratch->opened,
                          (cui_accordion_options){.multiple = true, .disabled = node->disabled},
                          bridge_group_panel, &scratch->binding)) {
            for (size_t i = 0; i < count; i++) {
                cui_bridge_node_record *item = &app->nodes[scratch->binding.slots[i]];
                if (item->checked != scratch->opened[i]) {
                    item->checked = scratch->opened[i];
                    bridge_change(app, scratch->binding.slots[i]);
                }
            }
        }
    }
    free(scratch);
}
static void bridge_render_chrome(cui_bridge_app *app, uint32_t slot, cui_bridge_node_record *node) {
    (void)slot;
    cui_chrome_item regions[3][64];
    size_t counts[3] = {0};
    for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling) {
        cui_bridge_node_record *item = &app->nodes[child];
        unsigned region = node->kind == CUI_BRIDGE_STATUS_BAR ? item->step_status : 0;
        regions[region][counts[region]++] =
            (cui_chrome_item){.id = item->id, .text = cui_string(item->text)};
    }
    if (node->kind == CUI_BRIDGE_STATUS_BAR) {
        if (!cui_status_bar(app->ui, node->id, &app->theme,
                            (cui_chrome_region){regions[0], counts[0]},
                            (cui_chrome_region){regions[1], counts[1]},
                            (cui_chrome_region){regions[2], counts[2]},
                            (cui_status_bar_options){.name = cui_string(node->text)}))
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
    } else {
        (void)cui_title_bar(app->ui, node->id, &app->theme, &node->title_bar_state, regions[0],
                            counts[0], (cui_title_bar_options){.name = cui_string(node->text)});
    }
}
static void bridge_render_toolbar(cui_bridge_app *app, uint32_t slot,
                                  cui_bridge_node_record *node) {
    (void)slot;
    cui_toolbar_item items[64];
    bridge_panel_binding bindings[64];
    size_t count = 0;
    for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling) {
        bindings[count] = (bridge_panel_binding){app, child};
        items[count] = (cui_toolbar_item){
            .id = app->nodes[child].id, .render = bridge_toolbar_render, .user = &bindings[count]};
        count++;
    }
    if (!cui_toolbar(
            app->ui, node->id, &app->theme, items, count,
            (cui_toolbar_options){.label = cui_string(node->text), .disabled = node->disabled}))
        app->frame_error = CUI_BRIDGE_CORE_ERROR;
}
static cui_image_load bridge_image_loader(void *user, cui_image_source source) {
    (void)source;
    cui_bridge_node_record *node = user;
    if (!node->image_pixels)
        return (cui_image_load){.status = CUI_IMAGE_PENDING};
    return (cui_image_load){.status = CUI_IMAGE_READY, .frames = &node->image_frame, .count = 1};
}
static void bridge_render_tree(cui_bridge_app *app, uint32_t slot, cui_bridge_node_record *node) {
    if (!node->tree_model) {
        cui_tree_model *model = cui_tree_create(64);
        cui_tree_node *nodes = calloc(64, sizeof *nodes);
        bool *expanded = calloc(64, sizeof *expanded);
        if (!model || !nodes || !expanded) {
            cui_tree_destroy(model);
            free(nodes);
            free(expanded);
            app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
            return;
        }
        node->tree_model = model;
        node->tree_nodes = nodes;
        node->tree_expanded = expanded;
    }
    uint32_t slots[64];
    size_t count = 0;
    for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling) {
        cui_bridge_node_record *item = &app->nodes[child];
        slots[count] = child;
        node->tree_nodes[count] = (cui_tree_node){.key = item->id,
                                                  .label = cui_string(item->text),
                                                  .depth = item->step_status,
                                                  .disabled = item->disabled};
        node->tree_expanded[count] = item->checked;
        count++;
    }
    cui_tree_error error =
        cui_tree_rebuild(node->tree_model, node->tree_nodes, node->tree_expanded, count);
    if (error != CUI_TREE_OK) {
        app->frame_error = CUI_BRIDGE_INVALID_STATE;
        return;
    }
    cui_id selected = 0;
    uint32_t selected_slot = (uint32_t)node->value;
    if (selected_slot && selected_slot <= app->max_nodes && app->nodes[selected_slot].live &&
        app->nodes[selected_slot].parent == slot)
        selected = app->nodes[selected_slot].id;
    cui_tree_options options = {.width = node->visual_width > 0 ? node->visual_width : 240,
                                .height = node->visual_height > 0 ? node->visual_height : 200,
                                .row_height = 28,
                                .dt = .016f,
                                .disabled = node->disabled};
    cui_tree_result result = cui_tree(app->ui, node->id, &app->theme, node->tree_model,
                                      &node->tree_state, &selected, options);
    if (result.error != CUI_TREE_OK) {
        app->frame_error = CUI_BRIDGE_CORE_ERROR;
        return;
    }
    if (result.selection_changed) {
        for (size_t i = 0; i < count; ++i)
            if (node->tree_nodes[i].key == selected)
                node->value = slots[i];
        bridge_change(app, slot);
    }
    if (result.expansion_changed) {
        for (size_t i = 0; i < count; ++i) {
            cui_bridge_node_record *item = &app->nodes[slots[i]];
            if (item->checked != node->tree_expanded[i]) {
                item->checked = node->tree_expanded[i];
                bridge_change(app, slots[i]);
            }
        }
    }
}
typedef struct {
    cui_bridge_app *app;
    uint32_t slots[64];
} bridge_message_binding;
static void bridge_message_row(cui_context *ui, cui_id row, size_t index, void *user, float width) {
    (void)row;
    bridge_message_binding *binding = user;
    cui_bridge_node_record *item = &binding->app->nodes[binding->slots[index]];
    cui_message_body body = {.slot = {.text = cui_string(item->text)},
                             .typed_bubble = true,
                             .bubble = CUI_BUBBLE_FILLED};
    cui_message_options options = cui_message_options_default();
    options.width = width;
    options.alignment = item->step_status ? CUI_MESSAGE_END : CUI_MESSAGE_START;
    options.header = (cui_message_slot){.text = cui_string(item->detail)};
    options.body = &body;
    options.body_count = 1;
    if (!cui_message(ui, item->id, &binding->app->theme, options))
        binding->app->frame_error = CUI_BRIDGE_CORE_ERROR;
}
static void bridge_render_message_scroller(cui_bridge_app *app, cui_bridge_node_record *node) {
    bridge_message_binding binding = {.app = app};
    uint32_t count = 0;
    for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling)
        binding.slots[count++] = child;
    if (!node->message_scroller) {
        node->message_scroller = cui_message_scroller_create((cui_message_scroller_config){
            .capacity = 64, .item_count = count, .estimated_height = 64});
        if (!node->message_scroller) {
            app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
            return;
        }
        node->message_count = count;
    } else if (count != node->message_count) {
        bool updated =
            count > node->message_count
                ? cui_message_scroller_append(node->message_scroller, count - node->message_count)
                : cui_message_scroller_reset(node->message_scroller, count);
        if (!updated) {
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
            return;
        }
        node->message_count = count;
    }
    cui_message_scroller_options options = cui_message_scroller_options_default();
    options.width = node->visual_width > 0 ? node->visual_width : 320;
    options.height = node->visual_height > 0 ? node->visual_height : 200;
    options.now_seconds = app->now_seconds;
    cui_message_scroller_result result =
        cui_message_scroller(app->ui, node->id, &app->theme, node->message_scroller,
                             bridge_message_row, &binding, options);
    if (!result.valid)
        app->frame_error = CUI_BRIDGE_CORE_ERROR;
    else if (result.view.scroll != node->value) {
        node->value = result.view.scroll;
        bridge_change(app, node->own_slot);
    }
}
typedef struct {
    cui_bridge_app *app;
    uint32_t slot;
    uint32_t slots[64];
    cui_carousel_item items[64];
} bridge_carousel_binding;
static void bridge_carousel_slide(cui_context *ui, const cui_theme *theme, size_t index,
                                  void *user) {
    (void)theme;
    bridge_carousel_binding *binding = user;
    cui_bridge_node_record *item = &binding->app->nodes[binding->slots[index]];
    cui_text(ui, cui_string(item->text), binding->app->theme.font_size, binding->app->theme.text);
    for (uint32_t child = item->first_child; child; child = binding->app->nodes[child].next_sibling)
        bridge_render_node(binding->app, child);
}
static void bridge_carousel_change(size_t index, void *user) {
    bridge_carousel_binding *binding = user;
    if (index >= 64)
        return;
    binding->app->nodes[binding->slot].value = binding->slots[index];
    bridge_change(binding->app, binding->slot);
}
static void bridge_render_carousel(cui_bridge_app *app, uint32_t slot,
                                   cui_bridge_node_record *node) {
    if (!node->carousel_state) {
        cui_carousel_state *state = calloc(1, sizeof *state);
        cui_carousel_geometry *geometry = calloc(64, sizeof *geometry);
        if (!state || !geometry || !cui_carousel_state_init(state, geometry, 64, 0)) {
            free(state);
            free(geometry);
            app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
            return;
        }
        node->carousel_state = state;
        node->carousel_geometry = geometry;
    }
    bridge_carousel_binding *binding = calloc(1, sizeof *binding);
    if (!binding) {
        app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
        return;
    }
    binding->app = app;
    binding->slot = slot;
    size_t count = 0, selected = SIZE_MAX;
    for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling) {
        binding->slots[count] = child;
        binding->items[count] = (cui_carousel_item){.id = app->nodes[child].id,
                                                    .label = cui_string(app->nodes[child].text),
                                                    .render = bridge_carousel_slide,
                                                    .user = binding};
        if ((uint32_t)node->value == child)
            selected = count;
        count++;
    }
    if (selected != SIZE_MAX)
        cui_carousel_set_selected(node->carousel_state, selected);
    node->carousel_state->on_change = bridge_carousel_change;
    node->carousel_state->change_user = binding;
    cui_carousel_options options = cui_carousel_default();
    options.width = node->visual_width > 0 ? node->visual_width : 300;
    options.height = node->visual_height > 0 ? node->visual_height : 200;
    options.now_seconds = app->now_seconds;
    options.disabled = node->disabled;
    options.label = cui_string(node->text);
    if (!cui_carousel(app->ui, node->id, &app->theme, node->carousel_state, binding->items, count,
                      options))
        app->frame_error = CUI_BRIDGE_CORE_ERROR;
    if (node->carousel_state->selected < count)
        node->value = binding->slots[node->carousel_state->selected];
    else
        node->value = 0;
    node->carousel_state->on_change = NULL;
    node->carousel_state->change_user = NULL;
    free(binding);
}
static void bridge_resizable_panel_content(cui_context *ui, void *user) {
    bridge_panel_binding *binding = user;
    cui_bridge_node_record *item = &binding->app->nodes[binding->slot];
    cui_text(ui, cui_string(item->text), binding->app->theme.font_size, binding->app->theme.text);
    for (uint32_t child = item->first_child; child; child = binding->app->nodes[child].next_sibling)
        bridge_render_node(binding->app, child);
}
typedef struct {
    cui_resizable_panel panels[64];
    bridge_panel_binding bindings[64];
    uint32_t slots[64];
    double previous[64];
} bridge_resizable_scratch;
static void bridge_render_resizable(cui_bridge_app *app, cui_bridge_node_record *node) {
    if (!node->resizable_state) {
        cui_resizable_state *state = calloc(1, sizeof *state);
        cui_resizable_slot *slots = calloc(64, sizeof *slots);
        if (!state || !slots || !cui_resizable_state_init(state, slots, 64)) {
            free(state);
            free(slots);
            app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
            return;
        }
        node->resizable_state = state;
        node->resizable_slots = slots;
    }
    bridge_resizable_scratch *scratch = calloc(1, sizeof *scratch);
    if (!scratch) {
        app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
        return;
    }
    size_t count = 0;
    for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling) {
        cui_bridge_node_record *item = &app->nodes[child];
        scratch->bindings[count] = (bridge_panel_binding){app, child};
        scratch->slots[count] = child;
        scratch->previous[count] = item->value;
        scratch->panels[count] = cui_resizable_panel_default(item->id);
        scratch->panels[count].accessibility_label = cui_string(item->text);
        scratch->panels[count].has_initial_size = item->value >= 100;
        scratch->panels[count].initial_size = (float)item->value;
        scratch->panels[count].content = bridge_resizable_panel_content;
        scratch->panels[count].content_user = &scratch->bindings[count];
        count++;
    }
    cui_resizable_options options = cui_resizable_options_default();
    options.axis = CUI_RESIZABLE_HORIZONTAL;
    options.extent = node->visual_width > 0 ? node->visual_width : 420;
    options.cross_size = node->visual_height > 0 ? node->visual_height : 160;
    options.disabled = node->disabled;
    options.now_seconds = app->now_seconds;
    cui_resizable_error result = cui_resizable(
        app->ui, node->id, &app->theme, node->resizable_state, scratch->panels, count, options);
    if (result != CUI_RESIZABLE_OK && result != CUI_RESIZABLE_INFEASIBLE)
        app->frame_error =
            result == CUI_RESIZABLE_CAPACITY ? CUI_BRIDGE_CAPACITY : CUI_BRIDGE_CORE_ERROR;
    else
        for (size_t i = 0; i < count; i++) {
            cui_bridge_node_record *item = &app->nodes[scratch->slots[i]];
            double size = node->resizable_state->slots[i].size;
            item->value = size;
            if (scratch->previous[i] > 0 && scratch->previous[i] != size)
                bridge_change(app, scratch->slots[i]);
        }
    free(scratch);
}
static void bridge_render_chart(cui_bridge_app *app, cui_bridge_node_record *node) {
    if (!node->chart || !node->chart->count) {
        cui_text(app->ui, cui_string("No data"), app->theme.font_size, app->theme.muted);
        return;
    }
    bridge_chart_owned *chart = node->chart;
    unsigned width = node->visual_width > 0 ? (unsigned)node->visual_width : 320;
    unsigned height = node->visual_height > 0 ? (unsigned)node->visual_height : 200;
    if (!width || !height || width > 512 || height > 512) {
        app->frame_error = CUI_BRIDGE_INVALID_ARGUMENT;
        return;
    }
    if (!chart->state.pixels || chart->width != width || chart->height != height) {
        unsigned char *pixels = calloc((size_t)width * height * 4, 1);
        if (!pixels) {
            app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
            return;
        }
        free(chart->state.pixels);
        chart->state.pixels = pixels;
        chart->state.byte_capacity = (size_t)width * height * 4;
        chart->width = width;
        chart->height = height;
        chart->state.revision++;
    }
    cui_chart_options options = {.kind = (cui_chart_kind)chart->kind,
                                 .data = chart->data,
                                 .count = chart->count,
                                 .width = width,
                                 .height = height,
                                 .disable_appear = true,
                                 .reduced_motion = true,
                                 .noninteractive = node->disabled};
    options = cui_chart_theme(options, &app->theme);
    if (!cui_chart(app->ui, node->id, &chart->state, &options, cui_string(node->key)))
        app->frame_error = CUI_BRIDGE_CORE_ERROR;
}
static void bridge_render_table(cui_bridge_app *app, cui_bridge_node_record *node) {
    unsigned rows = 0, columns = 0;
    for (uint32_t row = node->first_child; row; row = app->nodes[row].next_sibling) {
        rows++;
        unsigned cells = 0;
        for (uint32_t cell = app->nodes[row].first_child; cell;
             cell = app->nodes[cell].next_sibling)
            cells++;
        if (cells > columns)
            columns = cells;
    }
    float width = node->visual_width > 0 ? node->visual_width : 480;
    if (!cui_table_begin_ex(app->ui, node->id, &app->theme,
                            (cui_table_basic_options){.width = width,
                                                      .accessible_name = cui_string(node->key),
                                                      .row_count = rows,
                                                      .column_count = columns})) {
        app->frame_error = CUI_BRIDGE_CORE_ERROR;
        return;
    }
    unsigned row_index = 0;
    for (uint32_t row = node->first_child; row; row = app->nodes[row].next_sibling) {
        cui_bridge_node_record *row_node = &app->nodes[row];
        bool header = row_index == 0;
        if (!cui_table_begin_row_checked(app->ui, row_node->id, &app->theme,
                                         (cui_table_row_options){.height = 32,
                                                                 .header = header,
                                                                 .name = cui_string(row_node->text),
                                                                 .parent = node->id,
                                                                 .row_index = row_index + 1})) {
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
            break;
        }
        unsigned column_index = 0;
        for (uint32_t cell = row_node->first_child; cell; cell = app->nodes[cell].next_sibling) {
            cui_bridge_node_record *cell_node = &app->nodes[cell];
            if (!cui_table_begin_cell_ex(
                    app->ui, cell_node->id, &app->theme,
                    (cui_table_cell_options){.width = columns ? width / columns : width,
                                             .height = 32,
                                             .header = header,
                                             .name = cui_string(cell_node->text),
                                             .parent = row_node->id,
                                             .row_index = row_index + 1,
                                             .column_index = column_index + 1})) {
                app->frame_error = CUI_BRIDGE_CORE_ERROR;
                break;
            }
            cui_text(app->ui, cui_string(cell_node->text), app->theme.font_size, app->theme.text);
            cui_table_end(app->ui);
            column_index++;
        }
        cui_table_end(app->ui);
        row_index++;
        if (app->frame_error != CUI_BRIDGE_OK)
            break;
    }
    cui_table_end(app->ui);
}
static cui_str bridge_data_table_label(void *user, size_t row, cui_id column) {
    bridge_data_table_owned *table = user;
    if (row >= table->row_count || !column || column > table->column_count)
        return cui_string("");
    return cui_string(table->cells[row * table->column_count + (size_t)column - 1]);
}
static void bridge_render_data_table(cui_bridge_app *app, uint32_t slot,
                                     cui_bridge_node_record *node) {
    bridge_data_table_owned *table = node->data_table;
    if (!table) {
        cui_text(app->ui, cui_string("No data"), app->theme.font_size, app->theme.muted);
        return;
    }
    cui_table_options options = cui_table_options_default();
    options.width = node->visual_width > 0 ? node->visual_width : 480;
    options.height = node->visual_height > 0 ? node->visual_height : 240;
    options.row_height = 32;
    options.header_height = 32;
    options.disabled = node->disabled;
    options.selection_mode = (cui_table_selection_mode)node->table_selection_mode;
    options.accessible_name = cui_string(node->key);
    cui_table_result result =
        cui_data_table(app->ui, node->id, &app->theme, table->model, &table->state, options,
                       bridge_data_table_label, NULL, table);
    if (result.error != CUI_TABLE_OK) {
        app->frame_error =
            result.error == CUI_TABLE_CAPACITY ? CUI_BRIDGE_CAPACITY : CUI_BRIDGE_CORE_ERROR;
        return;
    }
    if (result.selection_changed || result.sort_changed) {
        node->value = table->state.row;
        node->checked = (uint32_t)table->state.column;
        if (result.selection_changed)
            bridge_data_table_change(app, slot, "selection");
        if (result.sort_changed)
            bridge_data_table_change(app, slot, "sort");
    }
}
static void bridge_render_plot(cui_bridge_app *app, cui_bridge_node_record *node) {
    if (!node->plot_count) {
        cui_text(app->ui, cui_string("No points"), app->theme.font_size, app->theme.muted);
        return;
    }
    float width = node->visual_width > 0 ? node->visual_width : 320;
    float height = node->visual_height > 0 ? node->visual_height : 200;
    cui_plot_dot_options options = cui_plot_dot_defaults();
    options.size = 6;
    cui_plot_result result =
        cui_plot_dots(app->ui, node->id, &app->theme, (cui_size){width, height}, node->plot_samples,
                      node->plot_count, options);
    if (result != CUI_PLOT_OK)
        app->frame_error =
            result == CUI_PLOT_CAPACITY ? CUI_BRIDGE_CAPACITY : CUI_BRIDGE_CORE_ERROR;
}
static void bridge_command_select(cui_command_index index, void *user) {
    cui_bridge_node_record *node = user;
    if (index.section || index.row >= 64)
        return;
    uint32_t slot = node->command_slots[index.row];
    if (!slot || !node->owner->nodes[slot].live)
        return;
    node->value = slot;
    bridge_change(node->owner, node->own_slot);
}
static void bridge_command_confirm(cui_command_index index, void *user) {
    cui_bridge_node_record *node = user;
    if (index.section || index.row >= 64)
        return;
    uint32_t slot = node->command_slots[index.row];
    if (slot && node->owner->nodes[slot].live)
        bridge_click_simple(node->owner, slot);
}
static void bridge_command_query(cui_str query, void *user) {
    (void)query;
    cui_bridge_node_record *node = user;
    bridge_change(node->owner, node->own_slot);
}
static void bridge_render_command(cui_bridge_app *app, cui_bridge_node_record *node) {
    size_t count = 0;
    for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling) {
        cui_bridge_node_record *item = &app->nodes[child];
        node->command_slots[count] = child;
        node->command_keywords[count] = cui_string(item->detail);
        node->command_entries[count] =
            (cui_command_entry){.kind = CUI_COMMAND_ITEM,
                                .item = {.label = cui_string(item->text),
                                         .disabled = item->disabled,
                                         .keywords = &node->command_keywords[count],
                                         .keyword_count = item->detail[0] ? 1 : 0}};
        count++;
    }
    cui_command_options options = cui_command_default();
    options.width = node->visual_width > 0 ? node->visual_width : 380;
    options.max_height = node->visual_height > 0 ? node->visual_height : 300;
    options.disabled = node->disabled;
    options.label = cui_string(node->key);
    options.on_select = bridge_command_select;
    options.on_confirm = bridge_command_confirm;
    options.on_query = bridge_command_query;
    options.user = node;
    if (!cui_command(app->ui, node->id, &app->theme, &node->command_state, node->command_entries,
                     count, options))
        app->frame_error = CUI_BRIDGE_CORE_ERROR;
    cui_command_index selected;
    if (cui_command_selected(&node->command_state, &selected) && !selected.section &&
        selected.row < count)
        node->value = node->command_slots[selected.row];
    else
        node->value = 0;
}
typedef struct {
    cui_bridge_app *app;
    uint32_t slots[64];
} bridge_virtual_binding;
static void bridge_virtual_item(cui_context *ui, const cui_theme *theme, void *user, size_t index,
                                uint64_t key, cui_id item, cui_size size) {
    (void)key;
    (void)item;
    (void)size;
    bridge_virtual_binding *binding = user;
    if (index >= 64 || !binding->slots[index])
        return;
    cui_bridge_node_record *node = &binding->app->nodes[binding->slots[index]];
    cui_text(ui, cui_string(node->text), theme->font_size, theme->text);
    for (uint32_t child = node->first_child; child; child = binding->app->nodes[child].next_sibling)
        bridge_render_node(binding->app, child);
}
static void bridge_render_virtual_list(cui_bridge_app *app, uint32_t slot,
                                       cui_bridge_node_record *node) {
    if (!node->virtual_list) {
        node->virtual_list = cui_virtual_create((cui_virtual_config){64, 32});
        if (!node->virtual_list) {
            app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
            return;
        }
    }
    uint64_t keys[64] = {0};
    bridge_virtual_binding binding = {.app = app};
    size_t count = 0;
    for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling) {
        keys[count] = app->nodes[child].id;
        binding.slots[count] = child;
        count++;
    }
    cui_virtual_result rebuilt = cui_virtual_rebuild(node->virtual_list, keys, NULL, count);
    if (rebuilt != CUI_VIRTUAL_OK) {
        app->frame_error =
            rebuilt == CUI_VIRTUAL_CAPACITY ? CUI_BRIDGE_CAPACITY : CUI_BRIDGE_CORE_ERROR;
        return;
    }
    cui_virtual_widget_options options = {
        .scroll = {.width = node->visual_width > 0 ? node->visual_width : 300,
                   .height = node->visual_height > 0 ? node->visual_height : 160,
                   .vertical = true,
                   .disabled = node->disabled,
                   .wheel_step = 32},
        .overscan = 1};
    float before = node->virtual_state.scroll.y;
    cui_virtual_widget_result result =
        cui_virtual_list_draw(app->ui, node->id, &app->theme, node->virtual_list,
                              &node->virtual_state, options, bridge_virtual_item, &binding);
    if (result.error != CUI_VIRTUAL_OK) {
        app->frame_error =
            result.error == CUI_VIRTUAL_CAPACITY ? CUI_BRIDGE_CAPACITY : CUI_BRIDGE_CORE_ERROR;
        return;
    }
    node->value = node->virtual_state.scroll.y;
    if (result.scroll_changed && before != node->virtual_state.scroll.y)
        bridge_change(app, slot);
}
static void bridge_sidebar_activate(void *user) {
    cui_bridge_node_record *item = user;
    cui_bridge_app *app = item->owner;
    if (!item->live || !item->parent || !app->nodes[item->parent].live)
        return;
    cui_bridge_node_record *sidebar = &app->nodes[item->parent];
    sidebar->value = item->own_slot;
    bridge_change(app, item->parent);
    bridge_click_simple(app, item->own_slot);
}
static void bridge_sidebar_entry_content(cui_context *ui, const cui_theme *theme, bool collapsed,
                                         void *user) {
    cui_bridge_node_record *item = user;
    cui_bridge_node_record *sidebar = &item->owner->nodes[item->parent];
    item->sidebar_item_state.id = item->id;
    cui_sidebar_menu_item menu = {.id = item->id,
                                  .label = cui_string(item->text),
                                  .accessibility_label = cui_string(item->text),
                                  .state = &item->sidebar_item_state,
                                  .active = (uint32_t)sidebar->value == item->own_slot,
                                  .disabled = sidebar->disabled || item->disabled,
                                  .on_activate = bridge_sidebar_activate,
                                  .user = item};
    if (!cui_sidebar_menu(ui, item->id | (UINT64_C(1) << 61), theme, &menu, 1, collapsed))
        item->owner->frame_error = CUI_BRIDGE_CORE_ERROR;
}
static void bridge_render_sidebar(cui_bridge_app *app, cui_bridge_node_record *node) {
    if (!node->sidebar_cache) {
        node->sidebar_cache = calloc(64, sizeof *node->sidebar_cache);
        if (!node->sidebar_cache) {
            app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
            return;
        }
        if (!cui_sidebar_state_init(&node->sidebar_state, node->sidebar_cache, 64)) {
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
            return;
        }
    }
    cui_sidebar_entry entries[64] = {0};
    size_t count = 0;
    for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling) {
        cui_bridge_node_record *item = &app->nodes[child];
        entries[count++] = (cui_sidebar_entry){.id = item->id | (UINT64_C(1) << 60),
                                               .render = bridge_sidebar_entry_content,
                                               .user = item,
                                               .estimated_height = 40,
                                               .revision = 1};
    }
    cui_sidebar_options options = cui_sidebar_default();
    options.collapse = CUI_SIDEBAR_ICON;
    options.collapsed = node->checked;
    options.width = node->visual_width > 0 ? node->visual_width : 255;
    options.height = node->visual_height > 0 ? node->visual_height : 320;
    options.now_seconds = app->now_seconds;
    options.duration_seconds = 0;
    options.label = cui_string(node->key);
    if (!cui_sidebar(app->ui, node->id, &app->theme, &node->sidebar_state, entries, count, options))
        app->frame_error = CUI_BRIDGE_CORE_ERROR;
}
static void bridge_render_description_list(cui_bridge_app *app, cui_bridge_node_record *node) {
    cui_description_item items[64];
    size_t count = 0;
    for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling) {
        cui_bridge_node_record *item = &app->nodes[child];
        items[count++] = (cui_description_item){
            .id = item->id, .label = cui_string(item->text), .value = cui_string(item->detail)};
    }
    if (!cui_description_list(app->ui, node->id, &app->theme, items, count,
                              cui_description_options_default()))
        app->frame_error = CUI_BRIDGE_CORE_ERROR;
}
static void bridge_render_dock(cui_bridge_app *app, uint32_t slot, cui_bridge_node_record *node) {
    bridge_dock_owned *owned = node->dock;
    if (!owned->initialized) {
        owned->count = 0;
        for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling) {
            if (owned->count == 64) {
                app->frame_error = CUI_BRIDGE_CAPACITY;
                return;
            }
            cui_bridge_node_record *item = &app->nodes[child];
            owned->slots[owned->count] = child;
            owned->panels[owned->count++] =
                (cui_dock_panel){.key = item->id,
                                 .title = cui_string(item->text[0] ? item->text : item->key),
                                 .render = bridge_dock_content,
                                 .user = item,
                                 .zoomable = true,
                                 .disabled = item->disabled};
            if (cui_dock_insert(owned->model, cui_dock_root(owned->model), item->id,
                                owned->count - 1) != CUI_DOCK_OK) {
                app->frame_error = CUI_BRIDGE_INVALID_STATE;
                return;
            }
            if (!node->value)
                node->value = child;
        }
        owned->initialized = true;
    }
    for (uint32_t i = 0; i < owned->count; i++)
        owned->panels[i].disabled = app->nodes[owned->slots[i]].disabled;
    cui_dock_view_options options = cui_dock_view_options_default();
    options.width = node->visual_width > 0 ? node->visual_width : app->width - 32;
    options.height = node->visual_height > 0 ? node->visual_height : app->height - 32;
    options.disabled = node->disabled;
    options.hide_menu = true;
    options.now_seconds = app->now_seconds;
    cui_dock_view_result result =
        cui_dock_view_render(app->ui, node->id, &app->theme, owned->model, owned->view,
                             owned->panels, owned->count, options);
    if (result.error != CUI_DOCK_OK) {
        app->frame_error = CUI_BRIDGE_CORE_ERROR;
        return;
    }
    if (result.selected) {
        for (uint32_t i = 0; i < owned->count; i++)
            if (owned->panels[i].key == result.selected_panel &&
                (uint32_t)node->value != owned->slots[i]) {
                node->value = owned->slots[i];
                bridge_change(app, slot);
            }
    }
}
static void bridge_render_settings(cui_bridge_app *app, uint32_t slot,
                                   cui_bridge_node_record *node) {
    bridge_settings_owned *owned = node->settings;
    if (!owned->initialized) {
        for (uint32_t i = 0; i < owned->field_count; i++) {
            free(owned->defaults[i]);
            owned->defaults[i] = NULL;
        }
        owned->page_count = owned->group_count = owned->field_count = 0;
        for (uint32_t pslot = node->first_child; pslot; pslot = app->nodes[pslot].next_sibling) {
            if (owned->page_count == 8) {
                app->frame_error = CUI_BRIDGE_CAPACITY;
                return;
            }
            cui_bridge_node_record *page_node = &app->nodes[pslot];
            uint32_t p = owned->page_count++;
            owned->page_slots[p] = pslot;
            cui_setting_page *page = &owned->pages[p];
            *page = (cui_setting_page){
                .title = cui_string(page_node->text[0] ? page_node->text : page_node->key),
                .description = cui_string(page_node->detail),
                .groups = &owned->groups[owned->group_count],
                .default_open = true};
            for (uint32_t gslot = page_node->first_child; gslot;
                 gslot = app->nodes[gslot].next_sibling) {
                if (owned->group_count == 32 || page->count == 8) {
                    app->frame_error = CUI_BRIDGE_CAPACITY;
                    return;
                }
                cui_bridge_node_record *group_node = &app->nodes[gslot];
                uint32_t g = owned->group_count++;
                owned->group_slots[g] = gslot;
                cui_setting_group *group = &owned->groups[g];
                *group = (cui_setting_group){
                    .title = cui_string(group_node->text[0] ? group_node->text : group_node->key),
                    .description = cui_string(group_node->detail),
                    .items = &owned->items[owned->field_count]};
                page->count++;
                for (uint32_t fslot = group_node->first_child; fslot;
                     fslot = app->nodes[fslot].next_sibling) {
                    if (owned->field_count == 128 || group->count == 16) {
                        app->frame_error = CUI_BRIDGE_CAPACITY;
                        return;
                    }
                    cui_bridge_node_record *field = &app->nodes[fslot];
                    uint32_t f = owned->field_count++;
                    owned->field_slots[f] = fslot;
                    owned->field_pages[f] = (uint8_t)p;
                    owned->field_groups[f] = (uint8_t)(page->count - 1);
                    owned->field_items[f] = (uint8_t)group->count;
                    cui_setting_item *item = &owned->items[f];
                    *item = (cui_setting_item){
                        .title = cui_string((field->kind == CUI_BRIDGE_INPUT ||
                                             field->kind == CUI_BRIDGE_NUMBER_INPUT)
                                                ? (field->detail[0] ? field->detail : field->key)
                                                : (field->text[0] ? field->text : field->key)),
                        .description = cui_string(field->detail),
                        .disabled = field->disabled,
                        .has_default = true,
                        .on_change = bridge_settings_field_change,
                        .user = field};
                    if (field->kind == CUI_BRIDGE_SWITCH || field->kind == CUI_BRIDGE_CHECKBOX) {
                        item->kind = field->kind == CUI_BRIDGE_SWITCH ? CUI_SETTING_SWITCH
                                                                      : CUI_SETTING_CHECKBOX;
                        item->boolean = &field->checked;
                        item->switch_state = &field->switch_state;
                        item->default_boolean = field->checked;
                    } else if (field->kind == CUI_BRIDGE_INPUT) {
                        item->kind = CUI_SETTING_INPUT;
                        item->edit = field->edit;
                        item->input_state = &field->input_state;
                        cui_str value = cui_edit_get_text(field->edit);
                        owned->defaults[f] = malloc(value.length + 1);
                        if (!owned->defaults[f]) {
                            app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
                            return;
                        }
                        memcpy(owned->defaults[f], value.data, value.length);
                        owned->defaults[f][value.length] = 0;
                        item->default_text = cui_string(owned->defaults[f]);
                    } else if (field->kind == CUI_BRIDGE_NUMBER_INPUT) {
                        item->kind = CUI_SETTING_NUMBER;
                        item->number = &field->value;
                        item->number_state = &field->number_state;
                        item->number_options = (cui_number_options){.minimum = field->minimum,
                                                                    .maximum = field->maximum,
                                                                    .has_minimum = true,
                                                                    .has_maximum = true,
                                                                    .step = field->step};
                        item->default_number = field->value;
                    } else {
                        app->frame_error = CUI_BRIDGE_INVALID_STATE;
                        return;
                    }
                    group->count++;
                }
            }
        }
        if (!owned->page_count) {
            app->frame_error = CUI_BRIDGE_INVALID_STATE;
            return;
        }
        node->value = owned->page_slots[0];
    }
    for (uint32_t i = 0; i < owned->field_count; i++) {
        cui_bridge_node_record *field = &app->nodes[owned->field_slots[i]];
        owned->items[i].disabled = field->disabled;
        if (field->kind == CUI_BRIDGE_NUMBER_INPUT)
            owned->items[i].number_options = (cui_number_options){.minimum = field->minimum,
                                                                  .maximum = field->maximum,
                                                                  .has_minimum = true,
                                                                  .has_maximum = true,
                                                                  .step = field->step};
    }
    for (uint32_t p = 0; p < owned->page_count; p++) {
        uint32_t events_needed = 0;
        for (size_t g = 0; g < owned->pages[p].count; g++)
            events_needed += (uint32_t)owned->pages[p].groups[g].count;
        owned->pages[p].no_reset = app->max_events - app->event_count < events_needed;
    }
    cui_settings_options options = cui_settings_default();
    options.width = node->visual_width > 0 ? node->visual_width : app->width - 32;
    options.height = node->visual_height > 0 ? node->visual_height : app->height - 32;
    options.query = cui_string(owned->query);
    cui_settings_result result = cui_settings(app->ui, node->id, &app->theme, owned->pages,
                                              owned->page_count, &owned->state, options);
    if (result.invalid || result.edit_error != CUI_EDIT_OK) {
        app->frame_error = CUI_BRIDGE_CORE_ERROR;
        return;
    }
    owned->initialized = true;
    if (owned->state.selected_page < owned->page_count) {
        uint32_t selected = owned->page_slots[owned->state.selected_page];
        if ((uint32_t)node->value != selected) {
            node->value = selected;
            bridge_change(app, slot);
        }
    }
}
static void bridge_render_questionnaire(cui_bridge_app *app, uint32_t slot,
                                        cui_bridge_node_record *node) {
    bridge_questionnaire_owned *owned = node->questionnaire;
    if (!owned->state.initialized) {
        owned->count = owned->choice_count = 0;
        for (uint32_t qslot = node->first_child; qslot; qslot = app->nodes[qslot].next_sibling) {
            if (owned->count == 16) {
                app->frame_error = CUI_BRIDGE_CAPACITY;
                return;
            }
            cui_bridge_node_record *question = &app->nodes[qslot];
            uint32_t index = owned->count++;
            owned->question_slots[index] = qslot;
            cui_questionnaire_item *item = &owned->items[index];
            *item = (cui_questionnaire_item){0};
            item->name = cui_string(question->key);
            item->label = cui_string(question->text[0] ? question->text : question->key);
            item->description = cui_string(question->detail);
            item->required = !!(question->step_status & 1);
            item->multiple = !!(question->step_status & 2);
            item->disabled = question->disabled;
            item->choices = &owned->choices[owned->choice_count];
            for (uint32_t cslot = question->first_child; cslot;
                 cslot = app->nodes[cslot].next_sibling) {
                if (owned->choice_count == 256) {
                    app->frame_error = CUI_BRIDGE_CAPACITY;
                    return;
                }
                cui_bridge_node_record *choice = &app->nodes[cslot];
                owned->choice_slots[owned->choice_count] = cslot;
                owned->choices[owned->choice_count++] = (cui_questionnaire_choice){
                    .value = cui_string(choice->key),
                    .label = cui_string(choice->text[0] ? choice->text : choice->key),
                    .description = cui_string(choice->detail),
                    .disabled = choice->disabled};
                item->choice_count++;
            }
        }
        cui_questionnaire_workspace workspace = {.items = owned->item_runtime,
                                                 .item_capacity = 16,
                                                 .choices = owned->choice_runtime,
                                                 .choice_capacity = 256};
        if (cui_questionnaire_init(&owned->state, owned->items, owned->count, workspace,
                                   bridge_questionnaire_event, owned) != CUI_QUESTIONNAIRE_OK) {
            app->frame_error = CUI_BRIDGE_INVALID_STATE;
            return;
        }
    }
    cui_questionnaire_options options = {.width = node->visual_width > 0 ? node->visual_width : 320,
                                         .disabled = node->disabled};
    if (!cui_questionnaire_begin(app->ui, node->id, &app->theme, &owned->state, options)) {
        app->frame_error = CUI_BRIDGE_CORE_ERROR;
        return;
    }
    cui_questionnaire_progress(app->ui, node->id | (UINT64_C(1) << 60), &app->theme, &owned->state,
                               options);
    size_t current = owned->state.current;
    if (current < owned->count) {
        uint32_t qslot = owned->question_slots[current];
        cui_questionnaire_heading(app->ui, app->nodes[qslot].id, &app->theme, &owned->state,
                                  current, options);
        size_t offset = owned->item_runtime[current].choice_offset;
        for (size_t i = 0; i < owned->items[current].choice_count; i++)
            (void)cui_questionnaire_choice_control(app->ui,
                                                   app->nodes[owned->choice_slots[offset + i]].id,
                                                   &app->theme, &owned->state, current, i, options);
        cui_questionnaire_error_control(app->ui, node->id | (UINT64_C(1) << 61), &app->theme,
                                        &owned->state, current, options);
        cui_begin_box(app->ui, node->id | (UINT64_C(1) << 62), (cui_box){.row = true, .gap = 8});
        for (unsigned action = CUI_QUESTIONNAIRE_PREVIOUS; action <= CUI_QUESTIONNAIRE_FINISH;
             action++)
            (void)cui_questionnaire_action_control(
                app->ui, (node->id << 8) | UINT64_C(0x1000000000000000) | action, &app->theme,
                &owned->state, (cui_questionnaire_action)action, (cui_str){0}, options);
        cui_end_box(app->ui);
    }
    cui_questionnaire_end(app->ui);
    node->checked = owned->state.complete;
    (void)slot;
}
static void bridge_render_node_other(cui_bridge_app *app, uint32_t slot) {
    cui_bridge_node_record *node = &app->nodes[slot];
    bool previous_disabled = node->disabled;
    node->disabled = node->disabled || app->render_disabled;
    if (node->kind == CUI_BRIDGE_ROOT_VIEW) {
        bridge_panel_binding binding = {app, slot};
        cui_root_style_override style = {.padding = true, .value = {.padding = (float)node->value}};
        if (!cui_root_render(app->ui, node->id, &app->root_view, &app->theme,
                             (cui_rect){0, 0, app->width, app->height}, &style, bridge_root_content,
                             &binding, app->now_seconds))
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
    } else if (node->kind == CUI_BRIDGE_DOCK) {
        bridge_render_dock(app, slot, node);
    } else if (node->kind == CUI_BRIDGE_SETTINGS) {
        bridge_render_settings(app, slot, node);
    } else if (node->kind == CUI_BRIDGE_QUESTIONNAIRE) {
        bridge_render_questionnaire(app, slot, node);
    } else if (node->kind == CUI_BRIDGE_SPEECH) {
        if (cui_speech_button(app->ui, node->id, &app->theme, &node->speech->state,
                              (cui_speech_button_options){.disabled = node->disabled},
                              app->now_seconds)) {
            node->checked = node->speech->state.status != CUI_SPEECH_IDLE;
        }
    } else if (node->kind == CUI_BRIDGE_SCROLLABLE) {
        float width = node->visual_width > 0 ? node->visual_width : 200;
        float height = node->visual_height > 0 ? node->visual_height : 100;
        cui_scroll_options options = {
            .width = width,
            .height = height,
            .content_width = node->content_width > 0 ? node->content_width : width,
            .content_height = node->content_height > 0 ? node->content_height : height,
            .vertical = true};
        float before = node->scroll_state.y;
        if (cui_scrollable_begin(app->ui, node->id, &app->theme, &node->scroll_state, options)) {
            for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling)
                bridge_render_node(app, child);
            cui_scrollable_end_ex(app->ui, node->id, &app->theme, &node->scroll_state, options);
            if (node->scroll_state.y != before) {
                node->value = node->scroll_state.y;
                bridge_change(app, slot);
            }
        }
    } else if (node->kind == CUI_BRIDGE_STATUS_BAR || node->kind == CUI_BRIDGE_TITLE_BAR) {
        bridge_render_chrome(app, slot, node);
    } else if (node->kind == CUI_BRIDGE_TOOLBAR) {
        bridge_render_toolbar(app, slot, node);
    } else if (node->kind == CUI_BRIDGE_FORM) {
        bridge_render_form(app, node);
    } else if (node->kind == CUI_BRIDGE_INPUT_GROUP) {
        bridge_render_input_group(app, slot, node);
    } else if (node->kind == CUI_BRIDGE_NOTIFICATION) {
        /* One shared native stack is rendered after the complete tree. */
    } else if (node->kind == CUI_BRIDGE_FOCUS_TRAP) {
        cui_begin_box(app->ui, node->id, (cui_box){.gap = node->gap});
        bool scoped =
            node->checked && cui_begin_focus_scope(app->ui, node->id | (UINT64_C(1) << 62), true);
        for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling)
            bridge_render_node(app, child);
        if (scoped)
            cui_end_focus_scope(app->ui);
        cui_end_box(app->ui);
    } else if (node->kind == CUI_BRIDGE_MESSAGE_SCROLLER) {
        bridge_render_message_scroller(app, node);
    } else if (node->kind == CUI_BRIDGE_CAROUSEL) {
        bridge_render_carousel(app, slot, node);
    } else if (node->kind == CUI_BRIDGE_RESIZABLE) {
        bridge_render_resizable(app, node);
    } else if (node->kind == CUI_BRIDGE_TEXT_VIEW) {
        if (!node->text_view) {
            node->text_view = bridge_text_view_create();
            if (!node->text_view) {
                app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
                return;
            }
        }
        cui_text_view_state *state = &node->text_view->state;
        size_t anchor = state->anchor, cursor = state->cursor;
        cui_text_view_options options = cui_text_view_default();
        options.width = node->visual_width > 0 ? node->visual_width : 360;
        options.height = node->visual_height > 0 ? node->visual_height : 200;
        options.scrollable = true;
        options.disabled = node->disabled;
        if (!cui_text_view(app->ui, node->id, &app->theme, state, options))
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
        else if (state->anchor != anchor || state->cursor != cursor)
            bridge_change(app, slot);
    } else if (node->kind == CUI_BRIDGE_TREE) {
        bridge_render_tree(app, slot, node);
    } else if (node->kind == CUI_BRIDGE_TIME_FIELD) {
        if (cui_time_field(app->ui, node->id, &app->theme, cui_string(node->text),
                           &node->time_state, &node->time,
                           (cui_time_options){.seconds = true, .disabled = node->disabled}))
            bridge_change(app, slot);
    } else if (node->kind == CUI_BRIDGE_COLOR_PICKER) {
        if (cui_color_picker(app->ui, node->id, &app->theme, &node->color_state, &node->color,
                             (cui_color_picker_options){
                                 .alpha = true,
                                 .disabled = node->disabled,
                                 .width = node->visual_width > 0 ? node->visual_width : 240})) {
            node->value = bridge_pack_color(node->color);
            bridge_change(app, slot);
        }
    } else if (node->kind == CUI_BRIDGE_CALENDAR) {
        cui_calendar_result result =
            cui_calendar(app->ui, node->id, &app->theme, &node->calendar_state,
                         &node->date_selection, (cui_calendar_options){.disabled = node->disabled});
        if (result.changed) {
            cui_date date = node->date_selection.start;
            node->value = (double)date.year * 10000 + date.month * 100 + date.day;
            bridge_change(app, slot);
        }
    } else if (node->kind == CUI_BRIDGE_DATE_PICKER) {
        cui_date_picker_options options = {.calendar = {.disabled = node->disabled},
                                           .viewport = {0, 0, app->width, app->height},
                                           .placeholder = cui_string(node->text),
                                           .disabled = node->disabled,
                                           .width =
                                               node->visual_width > 0 ? node->visual_width : 240};
        if (cui_date_picker(app->ui, node->id, &app->theme, cui_string(node->text),
                            &node->date_picker_state, &node->date_selection, options)) {
            cui_date date = node->date_selection.start;
            node->value = (double)date.year * 10000 + date.month * 100 + date.day;
            bridge_change(app, slot);
        }
    } else if (node->kind == CUI_BRIDGE_OTP_INPUT) {
        cui_otp_options options = cui_otp_options_default();
        options.name = cui_string(node->key);
        options.disabled = node->disabled;
        options.now_seconds = cui_frame_time(app->ui);
        cui_otp_result result = cui_otp(app->ui, node->id, &app->theme, &node->otp_state, options);
        if (result.error != CUI_EDIT_OK)
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
        if (result.changed)
            bridge_change(app, slot);
    } else if (node->kind == CUI_BRIDGE_DESCRIPTION_LIST) {
        bridge_render_description_list(app, node);
    } else if (node->kind == CUI_BRIDGE_TOOLTIP || node->kind == CUI_BRIDGE_HOVER_CARD) {
        cui_bridge_node_record *trigger = bridge_node(app, node->anchor);
        if (trigger) {
            cui_hover_options options = {.trigger = trigger->id,
                                         .viewport = {0, 0, app->width, app->height},
                                         .dt = .016f,
                                         .disabled = node->disabled};
            bool before = node->hover_state.open;
            if (node->kind == CUI_BRIDGE_TOOLTIP) {
                (void)cui_tooltip(app->ui, node->id, &app->theme, &node->hover_state,
                                  cui_string(node->text), options);
            } else if (cui_begin_hover_card(app->ui, node->id, &app->theme, &node->hover_state,
                                            cui_string(node->text), options)) {
                for (uint32_t child = node->first_child; child;
                     child = app->nodes[child].next_sibling)
                    bridge_render_node(app, child);
                cui_end_hover_card(app->ui);
            }
            if (node->hover_state.open != before) {
                node->checked = node->hover_state.open;
                bridge_change(app, slot);
            }
        }
    } else if (node->kind >= CUI_BRIDGE_DIALOG && node->kind <= CUI_BRIDGE_POPOVER) {
        cui_surface_options options = {.viewport = {0, 0, app->width, app->height}};
        if (node->kind == CUI_BRIDGE_POPOVER) {
            cui_bridge_node_record *anchor = bridge_node(app, node->anchor);
            if (anchor)
                options.anchor = cui_bounds(app->ui, anchor->id);
        }
        if (node->kind == CUI_BRIDGE_ALERT_DIALOG) {
            cui_alert_dialog(app->ui, &app->theme, &node->surface_state, cui_string(node->text),
                             cui_string(node->detail), cui_string("Confirm"), cui_string("Cancel"),
                             options);
        } else if (cui_surface_begin(app->ui, &app->theme, &node->surface_state,
                                     cui_string(node->text), cui_string(node->detail), options)) {
            for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling)
                bridge_render_node(app, child);
            cui_surface_end(app->ui);
        }
        cui_surface_result result = cui_surface_take_result(&node->surface_state);
        if (result != CUI_SURFACE_NONE)
            bridge_surface_close(app, slot, result);
    } else if (bridge_group_kind(node->kind)) {
        bridge_render_group(app, slot, node);
    } else if (node->kind == CUI_BRIDGE_ITEM) {
        /* Items are rendered by their owning composite. */
    } else if (node->kind == CUI_BRIDGE_LABEL) {
        cui_begin_box(app->ui, node->id, (cui_box){0});
        cui_text(app->ui, cui_string(node->text), app->theme.font_size, app->theme.text);
        cui_end_box(app->ui);
    } else if (node->kind == CUI_BRIDGE_COLLAPSIBLE) {
        bridge_panel_binding binding = {app, slot};
        if (cui_collapsible(app->ui, node->id, &app->theme, cui_string(node->text), &node->checked,
                            node->disabled, bridge_panel, &binding))
            bridge_change(app, slot);
    } else if (node->kind == CUI_BRIDGE_ALERT) {
        cui_alert_options options = {.kind = CUI_ALERT_INFO,
                                     .title = cui_string(node->text),
                                     .message = cui_string(node->detail)};
        (void)cui_alert(app->ui, node->id, &app->theme, options);
    } else if (node->kind == CUI_BRIDGE_AVATAR) {
        cui_avatar(app->ui, node->id, &app->theme, cui_string(node->text),
                   node->visual_width > 0 ? node->visual_width : 40);
    } else if (node->kind == CUI_BRIDGE_BUBBLE) {
        cui_bubble(app->ui, node->id, &app->theme,
                   (cui_bubble_options){.message = cui_string(node->text)});
    } else if (node->kind == CUI_BRIDGE_EMPTY_VIEW) {
        cui_empty(app->ui, node->id, &app->theme, cui_string(node->text), cui_string(node->detail),
                  node->visual_width);
    } else if (node->kind == CUI_BRIDGE_GROUP_BOX) {
        cui_begin_group_box(app->ui, node->id, &app->theme, cui_string(node->text),
                            node->visual_width);
        for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling)
            bridge_render_node(app, child);
        cui_end_group_box(app->ui);
    } else if (node->kind == CUI_BRIDGE_KBD) {
        cui_kbd(app->ui, node->id, &app->theme, cui_string(node->text));
    } else if (node->kind == CUI_BRIDGE_ICON) {
        cui_icon_options options = {0};
        if (node->visual_width > 0) {
            options.custom_width = true;
            options.width = node->visual_width;
        }
        if (node->visual_height > 0) {
            options.custom_height = true;
            options.height = node->visual_height;
        }
        if (cui_icon_named(app->ui, node->id, &app->theme, cui_string(node->text), options) !=
            CUI_ICON_OK)
            app->frame_error = CUI_BRIDGE_INVALID_STATE;
    } else if (node->kind == CUI_BRIDGE_MESSAGE) {
        cui_message_body body = {.slot = {.present = true, .text = cui_string(node->text)},
                                 .typed_bubble = true,
                                 .bubble = CUI_BUBBLE_FILLED};
        cui_message_options options = cui_message_options_default();
        options.width = node->visual_width > 0 ? node->visual_width : 300;
        options.header =
            (cui_message_slot){.present = node->detail[0] != 0, .text = cui_string(node->detail)};
        options.body = &body;
        options.body_count = 1;
        if (!cui_message(app->ui, node->id, &app->theme, options))
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
    } else if (node->kind == CUI_BRIDGE_ATTACHMENT) {
        cui_attachment_child children[2] = {
            {.kind = CUI_ATTACHMENT_TITLE, .text = cui_string(node->text)},
            {.kind = CUI_ATTACHMENT_DESCRIPTION, .text = cui_string(node->detail)}};
        cui_attachment_options options = cui_attachment_options_default();
        options.model = cui_attachment_model_default();
        options.model.status = (cui_attachment_status)node->attachment_status;
        options.model.progress_known = true;
        options.model.progress = (float)node->value;
        options.model.width = node->visual_width;
        options.children = children;
        options.child_count = node->detail[0] ? 2 : 1;
        options.name = cui_string(node->text);
        options.now_seconds = cui_frame_time(app->ui);
        cui_attachment_result result =
            cui_attachment(app->ui, node->id, &app->theme, &node->attachment_state, options);
        if (!result.valid)
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
    } else if (node->kind == CUI_BRIDGE_IMAGE) {
        cui_image_options options = cui_image_options_default();
        options.geometry.width = node->visual_width > 0    ? node->visual_width
                                 : node->image_frame.width ? node->image_frame.width
                                                           : 64;
        options.geometry.height = node->visual_height > 0    ? node->visual_height
                                  : node->image_frame.height ? node->image_frame.height
                                                             : 64;
        options.description = cui_string(node->text);
        options.clock.now_seconds = cui_frame_time(app->ui);
        cui_image_result result =
            cui_image(app->ui, node->id, &node->image_state,
                      (cui_image_source){.kind = CUI_IMAGE_CUSTOM, .identity = node->id},
                      bridge_image_loader, node, options);
        if (!result.valid)
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
    } else if (node->kind == CUI_BRIDGE_SKELETON) {
        float phase = cui_loading_phase(cui_frame_time(app->ui), 2);
        cui_skeleton(app->ui, node->id, &app->theme,
                     node->visual_width > 0 ? node->visual_width : 120,
                     node->visual_height > 0 ? node->visual_height : 16, 4, phase, false, false);
    } else if (node->kind == CUI_BRIDGE_SPINNER) {
        float phase = cui_loading_phase(cui_frame_time(app->ui), .8f);
        cui_spinner(app->ui, node->id, &app->theme,
                    node->visual_width > 0 ? node->visual_width : 16, phase, false);
    } else if (node->kind == CUI_BRIDGE_TAG) {
        if (!cui_tag(app->ui, node->id, &app->theme, cui_string(node->text),
                     cui_tag_options_default()))
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
    } else if (node->kind == CUI_BRIDGE_MARKER) {
        cui_marker_child child = {.kind = CUI_MARKER_TEXT, .text = cui_string(node->text)};
        if (!cui_marker(app->ui, node->id, &app->theme, &node->marker_state, &child, 1,
                        cui_marker_options_default()))
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
    } else if (node->kind == CUI_BRIDGE_SHIMMER) {
        cui_shimmer_options options = cui_shimmer_options_default();
        options.now_seconds = cui_frame_time(app->ui);
        if (!cui_shimmer_text(app->ui, node->id, &app->theme, &node->shimmer_state,
                              cui_string(node->text), options))
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
    } else if (node->kind == CUI_BRIDGE_PAGINATION) {
        size_t page = (size_t)node->value;
        if (cui_pagination(app->ui, node->id, &app->theme, &page, node->total, false,
                           node->disabled)) {
            node->value = page;
            bridge_change(app, slot);
        }
    } else if (node->kind == CUI_BRIDGE_RATING) {
        float value = (float)node->value;
        if (cui_rating(app->ui, node->id, &app->theme, cui_string(node->text), &value,
                       (cui_rating_options){.maximum = node->total, .disabled = node->disabled})) {
            node->value = value;
            bridge_change(app, slot);
        }
    } else if (node->kind == CUI_BRIDGE_BADGE) {
        cui_badge(app->ui, node->id, &app->theme, cui_string(node->text));
    } else if (node->kind == CUI_BRIDGE_INPUT) {
        cui_input_result result =
            cui_input_ex(app->ui, node->id, &app->theme, cui_string(node->text), node->edit,
                         &node->input_state, (cui_input_options){.disabled = node->disabled});
        if (result.changed)
            bridge_change(app, slot);
        if (result.error != CUI_EDIT_OK)
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
    } else if (node->kind == CUI_BRIDGE_TEXTAREA) {
        cui_textarea_result result = cui_textarea(
            app->ui, node->id, &app->theme, cui_string(node->text), node->edit,
            &node->textarea_state, (cui_textarea_options){.disabled = node->disabled, .rows = 3});
        if (result.changed)
            bridge_change(app, slot);
        if (result.error != CUI_TEXTAREA_OK || result.edit_error != CUI_EDIT_OK)
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
    } else if (node->kind == CUI_BRIDGE_EDITOR) {
        cui_editor_options options = cui_editor_default();
        options.textarea.disabled = node->disabled;
        options.textarea.width = node->visual_width > 0 ? node->visual_width : 480;
        options.textarea.height = node->visual_height > 0 ? node->visual_height : 240;
        options.language = cui_string(node->detail);
        cui_textarea_result result = cui_editor(
            app->ui, node->id, &app->theme, cui_string(node->key), &node->editor_state, options);
        if (result.changed)
            bridge_change(app, slot);
        if (result.error != CUI_TEXTAREA_OK || result.edit_error != CUI_EDIT_OK)
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
    } else if (node->kind == CUI_BRIDGE_CHART) {
        bridge_render_chart(app, node);
    } else if (node->kind == CUI_BRIDGE_TABLE) {
        bridge_render_table(app, node);
    } else if (node->kind == CUI_BRIDGE_DATA_TABLE) {
        bridge_render_data_table(app, slot, node);
    } else if (node->kind == CUI_BRIDGE_PLOT) {
        bridge_render_plot(app, node);
    } else if (node->kind == CUI_BRIDGE_COMMAND) {
        bridge_render_command(app, node);
    } else if (node->kind == CUI_BRIDGE_VIRTUAL_LIST) {
        bridge_render_virtual_list(app, slot, node);
    } else if (node->kind == CUI_BRIDGE_SIDEBAR) {
        bridge_render_sidebar(app, node);
    } else if (node->kind == CUI_BRIDGE_NUMBER_INPUT) {
        cui_number_options options = {.disabled = node->disabled,
                                      .step = node->step,
                                      .minimum = node->minimum,
                                      .maximum = node->maximum,
                                      .has_minimum = true,
                                      .has_maximum = true};
        cui_number_result result =
            cui_number_input(app->ui, node->id, &app->theme, cui_string(node->text),
                             &node->number_state, &node->value, options);
        if (result.changed)
            bridge_change(app, slot);
        if (result.error != CUI_EDIT_OK)
            app->frame_error = CUI_BRIDGE_CORE_ERROR;
    } else if (node->kind == CUI_BRIDGE_PROGRESS) {
        cui_progress(app->ui, node->id, &app->theme, (float)node->value, 160);
    } else if (node->kind == CUI_BRIDGE_CHECKBOX) {
        bool value = node->checked;
        if (cui_checkbox(app->ui, node->id, &app->theme, cui_string(node->text), &value,
                         node->disabled)) {
            node->checked = value;
            bridge_change(app, slot);
        }
    } else if (node->kind == CUI_BRIDGE_RADIO) {
        int value = node->checked ? 1 : 0;
        if (cui_radio(app->ui, node->id, &app->theme, cui_string(node->text), &value, 1,
                      node->disabled)) {
            node->checked = value == 1;
            bridge_change(app, slot);
        }
    } else if (node->kind == CUI_BRIDGE_SWITCH) {
        bool value = node->checked;
        if (cui_switch_ex(app->ui, node->id, &app->theme, cui_string(node->text), &value,
                          &node->switch_state, (cui_switch_options){.disabled = node->disabled})) {
            node->checked = value;
            bridge_change(app, slot);
        }
    } else if (node->kind == CUI_BRIDGE_TOGGLE) {
        bool value = node->checked;
        if (cui_toggle(app->ui, node->id, &app->theme, cui_string(node->text), &value,
                       node->disabled)) {
            node->checked = value;
            bridge_change(app, slot);
        }
    } else if (node->kind == CUI_BRIDGE_SLIDER) {
        float value = (float)node->value;
        if (cui_slider(app->ui, node->id, &app->theme, cui_string(node->text), &value,
                       (float)node->minimum, (float)node->maximum, (float)node->step,
                       node->disabled)) {
            node->value = value;
            bridge_change(app, slot);
        }
    } else {
        cui_button_options options = {.variant = (cui_button_variant)node->variant,
                                      .size = CUI_CONTROL_MEDIUM,
                                      .design_size = (cui_design_size)node->size,
                                      .rounding = (cui_button_rounding)node->rounding,
                                      .radius = node->custom_radius,
                                      .disabled = node->disabled,
                                      .state = &node->button_state};
        bridge_click_binding binding = {app, slot};
        options.on_click_event = bridge_click;
        options.action_user = &binding;
        if (node->custom_hover) {
            cui_theme_button_kind kind =
                node->variant == CUI_BRIDGE_BUTTON_SECONDARY ? CUI_THEME_BUTTON_SECONDARY
                : node->variant == CUI_BRIDGE_BUTTON_OUTLINE ? CUI_THEME_BUTTON_DEFAULT
                                                             : CUI_THEME_BUTTON_PRIMARY;
            options.custom_colors = true;
            options.colors = app->theme.buttons[kind];
            if (node->variant == CUI_BRIDGE_BUTTON_OUTLINE)
                options.colors.outline_hover = node->hover;
            else
                options.colors.hover = node->hover;
        }
        (void)cui_button_ex(app->ui, node->id, &app->theme, cui_string(node->text), options);
    }
    node->disabled = previous_disabled;
}
/* Keep legal structural recursion away from component-specific stack locals. */
static void bridge_render_node(cui_bridge_app *app, uint32_t slot) {
    cui_bridge_node_record *node = &app->nodes[slot];
    if (node->kind != CUI_BRIDGE_ROW && node->kind != CUI_BRIDGE_COLUMN) {
        bridge_render_node_other(app, slot);
        return;
    }
    bool previous_disabled = node->disabled;
    node->disabled = node->disabled || app->render_disabled;
    cui_box box = {.row = node->kind == CUI_BRIDGE_ROW, .gap = node->gap};
    if (slot == app->root) {
        box.width = -1;
        box.height = -1;
        box.padding = 16;
    }
    cui_begin_box(app->ui, node->id, box);
    for (uint32_t child = node->first_child; child; child = app->nodes[child].next_sibling)
        bridge_render_node(app, child);
    cui_end_box(app->ui);
    node->disabled = previous_disabled;
}
cui_bridge_status cui_bridge_run(cui_bridge_app *app, cui_bridge_run_callback callback,
                                 void *user) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!app->root || app->stepping || app->running)
        return CUI_BRIDGE_INVALID_STATE;
    if (app->backend != CUI_BRIDGE_BACKEND_SDL)
        return CUI_BRIDGE_BACKEND_UNAVAILABLE;
    app->running = true;
    status = cui_bridge_native_run(app, callback, user);
    app->running = false;
    if (app->destroy_requested) {
        cui_bridge_destroy(app);
        return CUI_BRIDGE_CLOSED;
    }
    return status;
}
cui_bridge_status cui_bridge_step(cui_bridge_app *app, float dt) {
    if (app && app->run_dispatching)
        return CUI_BRIDGE_INVALID_STATE;
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!isfinite(dt) || dt < 0 || dt > 1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (!app->root)
        return CUI_BRIDGE_INVALID_STATE;
    app->now_seconds += dt;
    app->stepping = true;
    app->frame_error = CUI_BRIDGE_OK;
    if (app->backend == CUI_BRIDGE_BACKEND_SDL) {
        status = cui_bridge_native_poll(app);
        if (status != CUI_BRIDGE_OK)
            goto done;
    }
    if (!cui_begin_frame(app->ui, app->width, app->height, dt)) {
        status = CUI_BRIDGE_CORE_ERROR;
        goto done;
    }
    for (uint32_t i = 1; i <= app->max_nodes; i++)
        if (app->nodes[i].live && app->nodes[i].speech)
            (void)cui_speech_pump(&app->nodes[i].speech->state, app->now_seconds);
    cui_begin_box(app->ui, 1,
                  (cui_box){.width = -1, .height = -1, .background = app->theme.background});
    bridge_render_node(app, app->root);
    cui_end_box(app->ui);
    if (app->notifications)
        (void)cui_notifications(app->ui, UINT64_C(0x4e4f544946594255), &app->theme,
                                app->notifications, (cui_rect){0, 0, app->width, app->height},
                                app->now_seconds);
    cui_draw_list list = cui_end_frame(app->ui);
    status = cui_last_error(app->ui) == CUI_OK ? CUI_BRIDGE_OK : CUI_BRIDGE_CORE_ERROR;
    if (status == CUI_BRIDGE_OK && app->backend == CUI_BRIDGE_BACKEND_SDL)
        status = cui_bridge_native_render(app, &list);
    if (status == CUI_BRIDGE_OK)
        status = app->frame_error;
done:
    app->stepping = false;
    if (status == CUI_BRIDGE_CLOSED)
        app->closed = true;
    return status;
}
cui_bridge_status cui_bridge_poll_event(cui_bridge_app *app, cui_bridge_event *out_event) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out_event)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    while (app->event_count) {
        cui_bridge_event event = app->events[app->event_head];
        free(app->event_payloads[app->event_head].text);
        app->event_payloads[app->event_head] = (cui_bridge_event_payload){0};
        app->event_head = (app->event_head + 1) % app->max_events;
        app->event_count--;
        if (!bridge_node(app, event.node) || event.type != CUI_BRIDGE_EVENT_CLICK)
            continue;
        *out_event = event;
        return CUI_BRIDGE_OK;
    }
    return CUI_BRIDGE_EMPTY;
}
cui_bridge_status cui_bridge_poll_event_ex(cui_bridge_app *app, cui_bridge_event_ex *out_event,
                                           char *destination, uint32_t capacity,
                                           uint32_t *required_capacity) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out_event || !required_capacity)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    while (app->event_count) {
        uint32_t index = app->event_head;
        cui_bridge_event event = app->events[index];
        cui_bridge_event_payload payload = app->event_payloads[index];
        if (!bridge_node(app, event.node)) {
            free(payload.text);
            app->event_payloads[index] = (cui_bridge_event_payload){0};
            app->event_head = (index + 1) % app->max_events;
            app->event_count--;
            continue;
        }
        uint32_t required = payload.text_length + 1;
        if (!destination || capacity < required) {
            *required_capacity = required;
            return CUI_BRIDGE_CAPACITY;
        }
        if (payload.text_length)
            memcpy(destination, payload.text, payload.text_length);
        destination[payload.text_length] = 0;
        *required_capacity = required;
        *out_event = (cui_bridge_event_ex){.type = event.type,
                                           .node = event.node,
                                           .frame_sequence = event.frame_sequence,
                                           .checked = payload.checked,
                                           .value = payload.value,
                                           .text_length = payload.text_length,
                                           .selected = payload.selected};
        free(payload.text);
        app->event_payloads[index] = (cui_bridge_event_payload){0};
        app->event_head = (index + 1) % app->max_events;
        app->event_count--;
        return CUI_BRIDGE_OK;
    }
    return CUI_BRIDGE_EMPTY;
}
cui_bridge_status cui_bridge_node_bounds(cui_bridge_app *app, cui_bridge_node handle,
                                         cui_bridge_rect *out_bounds) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out_bounds)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_id id = node->id;
    if (node->kind == CUI_BRIDGE_NOTIFICATION) {
        cui_notification_entry *entry = bridge_notification_entry(app, node->id);
        if (!entry)
            return CUI_BRIDGE_INVALID_STATE;
        cui_rect bounds = entry->bounds;
        if (!isfinite(bounds.x) || !isfinite(bounds.y) || bounds.w <= 0 || bounds.h <= 0)
            return CUI_BRIDGE_INVALID_STATE;
        *out_bounds = (cui_bridge_rect){bounds.x, bounds.y, bounds.w, bounds.h};
        return CUI_BRIDGE_OK;
    }
    if (node->kind == CUI_BRIDGE_ITEM && node->parent &&
        app->nodes[node->parent].kind == CUI_BRIDGE_TREE)
        id = cui_tree_row_id(app->nodes[node->parent].id, node->id);
    cui_rect rect = cui_bounds(app->ui, id);
    if (!isfinite(rect.x) || !isfinite(rect.y) || rect.w <= 0 || rect.h <= 0)
        return CUI_BRIDGE_INVALID_STATE;
    *out_bounds = (cui_bridge_rect){rect.x, rect.y, rect.w, rect.h};
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_resizable_handle_bounds(cui_bridge_app *app,
                                                     cui_bridge_node group_handle,
                                                     cui_bridge_node item_handle,
                                                     cui_bridge_rect *out_bounds) {
    cui_bridge_node_record *group;
    cui_bridge_status status = bridge_valid_node(app, group_handle, &group);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (group->kind != CUI_BRIDGE_RESIZABLE)
        return CUI_BRIDGE_INVALID_STATE;
    if (!out_bounds)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_bridge_node_record *item = bridge_node(app, item_handle);
    if (!item)
        return CUI_BRIDGE_STALE_HANDLE;
    if (item->parent != group_handle.slot || !item->next_sibling)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_rect rect = cui_bounds(app->ui, cui_resizable_handle_id(group->id, item->id));
    if (!isfinite(rect.x) || !isfinite(rect.y) || rect.w <= 0 || rect.h <= 0)
        return CUI_BRIDGE_INVALID_STATE;
    *out_bounds = (cui_bridge_rect){rect.x, rect.y, rect.w, rect.h};
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_color_part_bounds(cui_bridge_app *app, cui_bridge_node handle,
                                               uint32_t part, cui_bridge_rect *out_bounds) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_COLOR_PICKER)
        return CUI_BRIDGE_INVALID_STATE;
    if (!out_bounds || part < 1 || part > 3)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_rect rect = cui_bounds(app->ui, cui_color_part_id(node->id, part));
    if (!isfinite(rect.x) || !isfinite(rect.y) || rect.w <= 0 || rect.h <= 0)
        return CUI_BRIDGE_INVALID_STATE;
    *out_bounds = (cui_bridge_rect){rect.x, rect.y, rect.w, rect.h};
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_calendar_day_bounds(cui_bridge_app *app, cui_bridge_node handle,
                                                 cui_bridge_date day, cui_bridge_rect *out_bounds) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_CALENDAR)
        return CUI_BRIDGE_INVALID_STATE;
    if (!out_bounds || !cui_date_valid((cui_date){day.year, day.month, day.day}))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_rect rect = cui_bounds(
        app->ui, cui_calendar_day_id(node->id, (cui_date){day.year, day.month, day.day}));
    if (!isfinite(rect.x) || !isfinite(rect.y) || rect.w <= 0 || rect.h <= 0)
        return CUI_BRIDGE_INVALID_STATE;
    *out_bounds = (cui_bridge_rect){rect.x, rect.y, rect.w, rect.h};
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_dropdown_part_bounds(cui_bridge_app *app, cui_bridge_node handle,
                                                  uint32_t menu_part, cui_bridge_rect *out_bounds) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_DROPDOWN_BUTTON)
        return CUI_BRIDGE_INVALID_STATE;
    if (menu_part > 1 || !out_bounds)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_id id = node->id | (UINT64_C(1) << (menu_part ? 60 : 61));
    cui_rect rect = cui_bounds(app->ui, id);
    if (!isfinite(rect.x) || !isfinite(rect.y) || rect.w <= 0 || rect.h <= 0)
        return CUI_BRIDGE_INVALID_STATE;
    *out_bounds = (cui_bridge_rect){rect.x, rect.y, rect.w, rect.h};
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_numeric_part_bounds(cui_bridge_app *app, cui_bridge_node handle,
                                                 uint32_t part, cui_bridge_rect *out_bounds) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_PAGINATION && node->kind != CUI_BRIDGE_RATING &&
        node->kind != CUI_BRIDGE_STEPPER)
        return CUI_BRIDGE_INVALID_STATE;
    if (!part || part > 100 || !out_bounds)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    cui_rect rect = cui_bounds(app->ui, cui_numeric_part_id(node->id, part));
    if (!isfinite(rect.x) || !isfinite(rect.y) || rect.w <= 0 || rect.h <= 0)
        return CUI_BRIDGE_INVALID_STATE;
    *out_bounds = (cui_bridge_rect){rect.x, rect.y, rect.w, rect.h};
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_request_close(cui_bridge_app *app) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    app->closed = true;
    return CUI_BRIDGE_OK;
}

uint32_t cui_bridge_notification_v1_size(void) {
    return (uint32_t)sizeof(cui_bridge_notification_v1);
}
uint32_t cui_bridge_notification_versions(void) {
    return 1u;
}
static cui_bridge_status bridge_poll_notification_v1(cui_bridge_app *app, uint32_t struct_size,
                                                  cui_bridge_notification_v1 *out,
                                                  char *text, uint32_t capacity,
                                                  uint32_t *required_capacity, uint32_t enabled_payloads) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out || !required_capacity || struct_size != sizeof(*out) ||
        (enabled_payloads & ~CUI_BRIDGE_VALUE_SCALAR_V1))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    while (app->event_count) {
        uint32_t index = app->event_head;
        cui_bridge_event event = app->events[index];
        cui_bridge_event_payload payload = app->event_payloads[index];
        if (!bridge_node(app, event.node)) {
            free(payload.text);
            app->event_payloads[index] = (cui_bridge_event_payload){0};
            app->event_head = (index + 1) % app->max_events;
            --app->event_count;
            continue;
        }
        uint32_t required = payload.text_length + 1;
        if (!text || capacity < required) {
            *required_capacity = required;
            return CUI_BRIDGE_CAPACITY;
        }
        cui_bridge_notification_v1 value;
        memset(&value, 0, sizeof(value));
        value.struct_size = (uint32_t)sizeof(value);
        value.schema_version = CUI_BRIDGE_NOTIFICATION_V1;
        value.kind = event.type == CUI_BRIDGE_EVENT_CLICK ? CUI_BRIDGE_NOTIFY_ACTIVATE
                     : event.type == CUI_BRIDGE_EVENT_CLOSE ? CUI_BRIDGE_NOTIFY_LIFECYCLE
                                                           : CUI_BRIDGE_NOTIFY_CHANGE;
        value.payload_kind = payload.payload_kind == CUI_BRIDGE_PAYLOAD_SCALAR &&
                             !(enabled_payloads & CUI_BRIDGE_VALUE_SCALAR_V1)
                                 ? CUI_BRIDGE_PAYLOAD_LEGACY : payload.payload_kind;
        value.node_kind = payload.node_kind;
        value.node = event.node;
        value.frame_sequence = event.frame_sequence;
        value.flags = event.frame_sequence ? CUI_BRIDGE_NOTIFY_HAS_FRAME : 0;
        value.text_length = payload.text_length;
        if (value.payload_kind != CUI_BRIDGE_PAYLOAD_LEGACY)
            value.payload = payload.notification;
        value.legacy = (cui_bridge_event_ex){.type = event.type, .node = event.node,
            .frame_sequence = event.frame_sequence, .checked = payload.checked,
            .value = payload.value, .text_length = payload.text_length, .selected = payload.selected};
        if (payload.text_length)
            memcpy(text, payload.text, payload.text_length);
        text[payload.text_length] = 0;
        *required_capacity = required;
        *out = value;
        free(payload.text);
        app->event_payloads[index] = (cui_bridge_event_payload){0};
        app->event_head = (index + 1) % app->max_events;
        --app->event_count;
        return CUI_BRIDGE_OK;
    }
    return CUI_BRIDGE_EMPTY;
}

uint32_t cui_bridge_notification_value_payloads_v1(void) {
    return CUI_BRIDGE_VALUE_SCALAR_V1;
}
cui_bridge_status cui_bridge_poll_notification_v1(cui_bridge_app *app, uint32_t struct_size,
    cui_bridge_notification_v1 *out, char *text, uint32_t capacity, uint32_t *required_capacity) {
    return bridge_poll_notification_v1(app, struct_size, out, text, capacity, required_capacity, 0);
}
cui_bridge_status cui_bridge_poll_value_notification_v1(cui_bridge_app *app, uint32_t struct_size,
    uint32_t enabled_payloads, cui_bridge_notification_v1 *out, char *text, uint32_t capacity,
    uint32_t *required_capacity) {
    return bridge_poll_notification_v1(app, struct_size, out, text, capacity, required_capacity,
                                       enabled_payloads);
}
