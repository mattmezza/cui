/* SPDX-License-Identifier: MIT */
#include "cui/cui_media.h"
#include "cui/cui_message.h"
#include "cui_bridge_internal.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

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
#endif

static cui_size bridge_measure(void *user, cui_str text, float size) {
    (void)user;
    return (cui_size){(float)text.length * size * .55f, size};
}
static bool bridge_utf8(const char *s, size_t *length) {
    if (!s)
        return false;
    size_t n = 0;
    while (s[n] && n <= 4096)
        n++;
    if (n > 4096)
        return false;
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
            return false;
        if (extra > n - i)
            return false;
        while (extra--) {
            unsigned b = (unsigned char)s[i++];
            if ((b & 0xc0) != 0x80)
                return false;
            scalar = (scalar << 6) | (b & 63);
        }
        if (scalar < minimum || scalar > 0x10ffff || (scalar >= 0xd800 && scalar <= 0xdfff))
            return false;
    }
    *length = n;
    return true;
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
    node->key = node->text = node->detail = NULL;
    node->edit = NULL;
    node->edit_buffer = NULL;
    node->textarea_lines = NULL;
    node->image_pixels = NULL;
    node->tree_model = NULL;
    node->tree_nodes = NULL;
    node->tree_expanded = NULL;
    node->otp_buffer = NULL;
}
void cui_bridge_destroy(cui_bridge_app *app) {
    if (!app)
        return;
    if (app->nodes)
        for (uint32_t i = 1; i <= app->max_nodes; i++)
            bridge_free_node_resources(&app->nodes[i]);
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
    if (!out_node || kind < CUI_BRIDGE_ROW || kind > CUI_BRIDGE_FOCUS_TRAP)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    bool root = parent.slot == 0 && parent.generation == 0;
    cui_bridge_node_record *p = NULL;
    if (root) {
        if (app->root || (kind != CUI_BRIDGE_ROW && kind != CUI_BRIDGE_COLUMN))
            return CUI_BRIDGE_INVALID_STATE;
    } else {
        p = bridge_node(app, parent);
        if (!p)
            return CUI_BRIDGE_STALE_HANDLE;
        if (p->kind != CUI_BRIDGE_ROW && p->kind != CUI_BRIDGE_COLUMN &&
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
            p->kind != CUI_BRIDGE_FOCUS_TRAP)
            return CUI_BRIDGE_INVALID_STATE;
        if (p->kind == CUI_BRIDGE_ITEM && p->parent &&
            (app->nodes[p->parent].kind == CUI_BRIDGE_TREE ||
             app->nodes[p->parent].kind == CUI_BRIDGE_INPUT_GROUP))
            return CUI_BRIDGE_INVALID_STATE;
        if ((p->kind == CUI_BRIDGE_RADIO_GROUP || p->kind == CUI_BRIDGE_TABS ||
             p->kind == CUI_BRIDGE_ACCORDION || p->kind == CUI_BRIDGE_SELECT ||
             p->kind == CUI_BRIDGE_COMBOBOX || p->kind == CUI_BRIDGE_DROPDOWN_BUTTON ||
             p->kind == CUI_BRIDGE_LIST || p->kind == CUI_BRIDGE_MENU ||
             p->kind == CUI_BRIDGE_STEPPER || p->kind == CUI_BRIDGE_DESCRIPTION_LIST ||
             p->kind == CUI_BRIDGE_STATUS_BAR || p->kind == CUI_BRIDGE_TOOLBAR ||
             p->kind == CUI_BRIDGE_TITLE_BAR || p->kind == CUI_BRIDGE_TREE ||
             p->kind == CUI_BRIDGE_FORM || p->kind == CUI_BRIDGE_INPUT_GROUP) !=
            (kind == CUI_BRIDGE_ITEM))
            return CUI_BRIDGE_INVALID_STATE;
        if (kind == CUI_BRIDGE_FORM)
            for (uint32_t ancestor = parent.slot; ancestor; ancestor = app->nodes[ancestor].parent)
                if (app->nodes[ancestor].kind == CUI_BRIDGE_FORM)
                    return CUI_BRIDGE_INVALID_STATE;
        if (kind == CUI_BRIDGE_ITEM) {
            uint32_t count = 0;
            for (uint32_t sibling = p->first_child; sibling;
                 sibling = app->nodes[sibling].next_sibling)
                count++;
            if (count >= 64)
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
    if (!slot || app->next_id == UINT64_MAX)
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
    if (kind == CUI_BRIDGE_INPUT || kind == CUI_BRIDGE_TEXTAREA || kind == CUI_BRIDGE_COMBOBOX ||
        kind == CUI_BRIDGE_INPUT_GROUP || kind == CUI_BRIDGE_NUMBER_INPUT) {
        size_t capacity = kind == CUI_BRIDGE_NUMBER_INPUT ? 128 : 4097;
        edit_buffer = calloc(capacity, 1);
        if (kind == CUI_BRIDGE_TEXTAREA)
            lines = calloc(512, sizeof(*lines));
        if (edit_buffer && (kind != CUI_BRIDGE_TEXTAREA || lines))
            edit = cui_edit_create((cui_edit_config){.buffer = edit_buffer,
                                                     .capacity = capacity,
                                                     .multiline = kind == CUI_BRIDGE_TEXTAREA,
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
    *node = (cui_bridge_node_record){.generation = generation,
                                     .live = true,
                                     .kind = kind,
                                     .owner = app,
                                     .own_slot = slot,
                                     .parent = root ? 0 : parent.slot,
                                     .id = app->next_id++,
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
                                     .size = kind == CUI_BRIDGE_BUTTON ? CUI_BRIDGE_SIZE_BASE
                                                                       : CUI_BRIDGE_SIZE_INHERIT};
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
    if (lines && !cui_textarea_state_init(&node->textarea_state, lines, 512)) {
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
    if (kind == CUI_BRIDGE_NUMBER_INPUT)
        node->number_state.edit = edit;
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
    if (node->parent) {
        cui_bridge_node_record *parent = &app->nodes[node->parent];
        if (node->kind == CUI_BRIDGE_ITEM &&
            (parent->kind == CUI_BRIDGE_RADIO_GROUP || parent->kind == CUI_BRIDGE_TABS ||
             parent->kind == CUI_BRIDGE_SELECT || parent->kind == CUI_BRIDGE_COMBOBOX ||
             parent->kind == CUI_BRIDGE_DROPDOWN_BUTTON || parent->kind == CUI_BRIDGE_LIST ||
             parent->kind == CUI_BRIDGE_MENU || parent->kind == CUI_BRIDGE_STEPPER ||
             parent->kind == CUI_BRIDGE_TREE) &&
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
    if (node->kind < CUI_BRIDGE_LABEL)
        return CUI_BRIDGE_INVALID_STATE;
    if (node->kind == CUI_BRIDGE_NOTIFICATION && bridge_notification_entry(app, node->id))
        return CUI_BRIDGE_INVALID_STATE;
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
cui_bridge_status cui_bridge_set_detail_text(cui_bridge_app *app, cui_bridge_node handle,
                                             const char *text_utf8) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_ALERT && node->kind != CUI_BRIDGE_SELECT &&
        node->kind != CUI_BRIDGE_COMBOBOX && node->kind != CUI_BRIDGE_DROPDOWN_BUTTON &&
        (node->kind < CUI_BRIDGE_DIALOG || node->kind > CUI_BRIDGE_POPOVER) &&
        node->kind != CUI_BRIDGE_EMPTY_VIEW && node->kind != CUI_BRIDGE_ITEM &&
        node->kind != CUI_BRIDGE_MESSAGE && node->kind != CUI_BRIDGE_ATTACHMENT &&
        node->kind != CUI_BRIDGE_INPUT_GROUP && node->kind != CUI_BRIDGE_NOTIFICATION)
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
        node->kind != CUI_BRIDGE_SPINNER)
        return CUI_BRIDGE_INVALID_STATE;
    if (!isfinite(width) || !isfinite(height) || width < 0 || height < 0 || width > 4096 ||
        height > 4096)
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
        node->kind != CUI_BRIDGE_TREE)
        return CUI_BRIDGE_INVALID_STATE;
    if (!item.slot && !item.generation) {
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
        node->kind != CUI_BRIDGE_TREE)
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
         node->kind != CUI_BRIDGE_FORM && node->kind != CUI_BRIDGE_INPUT_GROUP))
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
         app->nodes[node->parent].kind != CUI_BRIDGE_INPUT_GROUP))
        return CUI_BRIDGE_INVALID_STATE;
    if (step_status > (app->nodes[node->parent].kind == CUI_BRIDGE_INPUT_GROUP ? 3u
                       : app->nodes[node->parent].kind == CUI_BRIDGE_TREE      ? 63u
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
    if (!bridge_boolean_kind(node->kind))
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
    if (node->kind != CUI_BRIDGE_SLIDER && node->kind != CUI_BRIDGE_PROGRESS &&
        node->kind != CUI_BRIDGE_NUMBER_INPUT && node->kind != CUI_BRIDGE_PAGINATION &&
        node->kind != CUI_BRIDGE_RATING && node->kind != CUI_BRIDGE_SCROLLABLE &&
        node->kind != CUI_BRIDGE_ATTACHMENT)
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
    app->event_payloads[tail] = (cui_bridge_event_payload){0};
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
    cui_bridge_event_payload payload = {.checked = node->checked, .value = node->value};
    if ((node->kind == CUI_BRIDGE_RADIO_GROUP || node->kind == CUI_BRIDGE_TABS ||
         node->kind == CUI_BRIDGE_SELECT || node->kind == CUI_BRIDGE_COMBOBOX ||
         node->kind == CUI_BRIDGE_DROPDOWN_BUTTON || node->kind == CUI_BRIDGE_LIST ||
         node->kind == CUI_BRIDGE_MENU || node->kind == CUI_BRIDGE_STEPPER ||
         node->kind == CUI_BRIDGE_TREE) &&
        node->value > 0) {
        uint32_t selected = (uint32_t)node->value;
        if (selected <= app->max_nodes && app->nodes[selected].live &&
            app->nodes[selected].parent == slot)
            payload.selected = (cui_bridge_node){selected, app->nodes[selected].generation};
    }
    if ((node->edit && node->kind != CUI_BRIDGE_NUMBER_INPUT) ||
        node->kind == CUI_BRIDGE_OTP_INPUT) {
        cui_str value = node->kind == CUI_BRIDGE_OTP_INPUT ? cui_otp_value(&node->otp_state)
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
static void bridge_click_simple(cui_bridge_app *app, uint32_t slot) {
    if (app->event_count == app->max_events) {
        app->frame_error = CUI_BRIDGE_CAPACITY;
        return;
    }
    uint32_t tail = (app->event_head + app->event_count) % app->max_events;
    app->event_payloads[tail] = (cui_bridge_event_payload){0};
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
    app->event_payloads[tail] = (cui_bridge_event_payload){.value = result};
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
static void bridge_render_node(cui_bridge_app *app, uint32_t slot) {
    cui_bridge_node_record *node = &app->nodes[slot];
    bool previous_disabled = node->disabled;
    node->disabled = node->disabled || app->render_disabled;
    if (node->kind == CUI_BRIDGE_ROW || node->kind == CUI_BRIDGE_COLUMN) {
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
cui_bridge_status cui_bridge_step(cui_bridge_app *app, float dt) {
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
