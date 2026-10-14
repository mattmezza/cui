/* SPDX-License-Identifier: MIT */
#include "cui_bridge_internal.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

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
void cui_bridge_destroy(cui_bridge_app *app) {
    if (!app)
        return;
    if (app->nodes)
        for (uint32_t i = 1; i <= app->max_nodes; i++) {
            free(app->nodes[i].key);
            free(app->nodes[i].text);
            free(app->nodes[i].detail);
            cui_edit_destroy(app->nodes[i].edit);
            free(app->nodes[i].edit_buffer);
            free(app->nodes[i].textarea_lines);
        }
    cui_destroy(app->ui);
    cui_bridge_native_destroy(app);
    if (app->event_payloads)
        for (uint32_t i = 0; i < app->max_events; i++)
            free(app->event_payloads[i].text);
    free(app->event_payloads);
    free(app->events);
    free(app->nodes);
    free(app);
}
cui_bridge_status cui_bridge_node_create(cui_bridge_app *app, cui_bridge_node parent,
                                         cui_bridge_kind kind, const char *key_utf8,
                                         cui_bridge_node *out_node) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!out_node || kind < CUI_BRIDGE_ROW || kind > CUI_BRIDGE_DROPDOWN_BUTTON)
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
            p->kind != CUI_BRIDGE_COMBOBOX && p->kind != CUI_BRIDGE_DROPDOWN_BUTTON)
            return CUI_BRIDGE_INVALID_STATE;
        if ((p->kind == CUI_BRIDGE_RADIO_GROUP || p->kind == CUI_BRIDGE_TABS ||
             p->kind == CUI_BRIDGE_ACCORDION || p->kind == CUI_BRIDGE_SELECT ||
             p->kind == CUI_BRIDGE_COMBOBOX || p->kind == CUI_BRIDGE_DROPDOWN_BUTTON) !=
            (kind == CUI_BRIDGE_ITEM))
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
    cui_edit *edit = NULL;
    cui_textarea_line *lines = NULL;
    if (kind == CUI_BRIDGE_INPUT || kind == CUI_BRIDGE_TEXTAREA || kind == CUI_BRIDGE_COMBOBOX ||
        kind == CUI_BRIDGE_NUMBER_INPUT) {
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
    cui_bridge_node_record *node = &app->nodes[slot];
    uint32_t generation = node->generation + 1;
    *node = (cui_bridge_node_record){.generation = generation,
                                     .live = true,
                                     .kind = kind,
                                     .parent = root ? 0 : parent.slot,
                                     .id = app->next_id++,
                                     .key = key,
                                     .text = text,
                                     .detail = detail,
                                     .edit_buffer = edit_buffer,
                                     .edit = edit,
                                     .textarea_lines = lines,
                                     .minimum = 0,
                                     .maximum = kind == CUI_BRIDGE_NUMBER_INPUT ? 100 : 1,
                                     .step = kind == CUI_BRIDGE_NUMBER_INPUT ? 1 : 0.01,
                                     .size = kind == CUI_BRIDGE_BUTTON ? CUI_BRIDGE_SIZE_BASE
                                                                       : CUI_BRIDGE_SIZE_INHERIT};
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
    if (kind == CUI_BRIDGE_NUMBER_INPUT)
        node->number_state.edit = edit;
    if (kind == CUI_BRIDGE_SELECT || kind == CUI_BRIDGE_COMBOBOX)
        cui_choice_popup_init(&node->choice_popup, node->id | (UINT64_C(1) << 62));
    if (kind == CUI_BRIDGE_DROPDOWN_BUTTON)
        cui_menu_tree_init(&node->menu_state, node->id | (UINT64_C(1) << 62));
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
    for (uint32_t child = node->first_child; child;) {
        uint32_t next = app->nodes[child].next_sibling;
        bridge_remove_subtree(app, child);
        child = next;
    }
    free(node->key);
    free(node->text);
    free(node->detail);
    cui_edit_destroy(node->edit);
    free(node->edit_buffer);
    free(node->textarea_lines);
    node->key = node->text = NULL;
    node->detail = NULL;
    node->edit = NULL;
    node->edit_buffer = NULL;
    node->textarea_lines = NULL;
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
             parent->kind == CUI_BRIDGE_DROPDOWN_BUTTON) &&
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
        node->kind != CUI_BRIDGE_COMBOBOX && node->kind != CUI_BRIDGE_DROPDOWN_BUTTON)
        return CUI_BRIDGE_INVALID_STATE;
    char *copy = NULL;
    status = bridge_copy(text_utf8, &copy);
    if (status != CUI_BRIDGE_OK)
        return status;
    free(node->detail);
    node->detail = copy;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_open(cui_bridge_app *app, cui_bridge_node handle, uint32_t open) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_COLLAPSIBLE &&
        !(node->kind == CUI_BRIDGE_ITEM && node->parent &&
          app->nodes[node->parent].kind == CUI_BRIDGE_ACCORDION))
        return CUI_BRIDGE_INVALID_STATE;
    if (open > 1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->checked = open != 0;
    return CUI_BRIDGE_OK;
}
static bool bridge_group_kind(cui_bridge_kind kind) {
    return kind == CUI_BRIDGE_RADIO_GROUP || kind == CUI_BRIDGE_TABS ||
           kind == CUI_BRIDGE_ACCORDION || kind == CUI_BRIDGE_SELECT ||
           kind == CUI_BRIDGE_COMBOBOX || kind == CUI_BRIDGE_DROPDOWN_BUTTON;
}
cui_bridge_status cui_bridge_set_selected(cui_bridge_app *app, cui_bridge_node group,
                                          cui_bridge_node item) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, group, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_RADIO_GROUP && node->kind != CUI_BRIDGE_TABS &&
        node->kind != CUI_BRIDGE_SELECT && node->kind != CUI_BRIDGE_COMBOBOX &&
        node->kind != CUI_BRIDGE_DROPDOWN_BUTTON)
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
        node->kind != CUI_BRIDGE_DROPDOWN_BUTTON)
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
        (!node->parent || app->nodes[node->parent].kind != CUI_BRIDGE_ACCORDION))
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
         node->kind != CUI_BRIDGE_COMBOBOX && node->kind != CUI_BRIDGE_DROPDOWN_BUTTON))
        return CUI_BRIDGE_INVALID_STATE;
    if (disabled > 1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    node->disabled = disabled != 0;
    return CUI_BRIDGE_OK;
}
cui_bridge_status cui_bridge_set_value(cui_bridge_app *app, cui_bridge_node handle, double value) {
    cui_bridge_node_record *node;
    cui_bridge_status status = bridge_valid_node(app, handle, &node);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (node->kind != CUI_BRIDGE_SLIDER && node->kind != CUI_BRIDGE_PROGRESS &&
        node->kind != CUI_BRIDGE_NUMBER_INPUT)
        return CUI_BRIDGE_INVALID_STATE;
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
        (!node->parent || app->nodes[node->parent].kind != CUI_BRIDGE_ACCORDION))
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
        node->kind != CUI_BRIDGE_NUMBER_INPUT)
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
    cui_str value = node->edit && node->kind != CUI_BRIDGE_NUMBER_INPUT
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
         node->kind == CUI_BRIDGE_DROPDOWN_BUTTON) &&
        node->value > 0) {
        uint32_t selected = (uint32_t)node->value;
        if (selected <= app->max_nodes && app->nodes[selected].live &&
            app->nodes[selected].parent == slot)
            payload.selected = (cui_bridge_node){selected, app->nodes[selected].generation};
    }
    if (node->edit && node->kind != CUI_BRIDGE_NUMBER_INPUT) {
        cui_str value = cui_edit_get_text(node->edit);
        payload.text = malloc(value.length + 1);
        if (!payload.text) {
            app->frame_error = CUI_BRIDGE_OUT_OF_MEMORY;
            return;
        }
        memcpy(payload.text, value.data, value.length);
        payload.text[value.length] = 0;
        payload.text_length = (uint32_t)value.length;
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
static void bridge_render_node(cui_bridge_app *app, uint32_t slot) {
    cui_bridge_node_record *node = &app->nodes[slot];
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
    } else if (bridge_group_kind(node->kind)) {
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
                (cui_choice_item){item->id, cui_string(item->text), item->disabled};
            scratch->menu_entries[count] = (cui_menu_entry){.id = item->id,
                                                            .label = cui_string(item->text),
                                                            .disabled = item->disabled,
                                                            .kind = CUI_MENU_ACTION};
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
        } else if (node->kind == CUI_BRIDGE_DROPDOWN_BUTTON) {
            cui_id action = 0;
            cui_dropdown_button_options options = {
                .disabled = node->disabled,
                .menu = {.popup = {.viewport = {0, 0, app->width, app->height}, .dt = .016f}}};
            cui_dropdown_button_result result = cui_dropdown_button(
                app->ui, node->id, node->id | (UINT64_C(1) << 61), node->id | (UINT64_C(1) << 60),
                &app->theme, cui_string(node->text), cui_string(node->detail),
                scratch->menu_entries, count, &action, &node->menu_state, options);
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
            cui_choice_popup_options options = {.disabled = node->disabled,
                                                .viewport = {0, 0, app->width, app->height},
                                                .dt = .016f};
            bool changed = false;
            if (node->kind == CUI_BRIDGE_SELECT) {
                changed = cui_select(app->ui, node->id, &app->theme, cui_string(node->text),
                                     cui_string(node->detail), scratch->tabs, count, &selected,
                                     &node->choice_popup, options);
            } else {
                cui_combobox_result result =
                    cui_combobox(app->ui, node->id, &app->theme, cui_string(node->detail),
                                 node->edit, scratch->tabs, count, scratch->filtered, 64, &selected,
                                 &node->choice_popup, options);
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
}
cui_bridge_status cui_bridge_step(cui_bridge_app *app, float dt) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    if (!isfinite(dt) || dt < 0 || dt > 1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (!app->root)
        return CUI_BRIDGE_INVALID_STATE;
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
    cui_rect rect = cui_bounds(app->ui, node->id);
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
cui_bridge_status cui_bridge_request_close(cui_bridge_app *app) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    app->closed = true;
    return CUI_BRIDGE_OK;
}
