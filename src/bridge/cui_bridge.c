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
    if (!app->nodes || !app->events) {
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
        }
    cui_destroy(app->ui);
    cui_bridge_native_destroy(app);
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
    if (!out_node || kind < CUI_BRIDGE_ROW || kind > CUI_BRIDGE_BUTTON)
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
        if (p->kind != CUI_BRIDGE_ROW && p->kind != CUI_BRIDGE_COLUMN)
            return CUI_BRIDGE_INVALID_STATE;
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
    char *key = NULL, *text = NULL;
    status = bridge_copy(key_utf8, &key);
    if (status != CUI_BRIDGE_OK)
        return status;
    status = bridge_copy("", &text);
    if (status != CUI_BRIDGE_OK) {
        free(key);
        return status;
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
                                     .size = kind == CUI_BRIDGE_BUTTON ? CUI_BRIDGE_SIZE_BASE
                                                                       : CUI_BRIDGE_SIZE_INHERIT};
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
    node->key = node->text = NULL;
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
    if (node->kind != CUI_BRIDGE_LABEL && node->kind != CUI_BRIDGE_BUTTON)
        return CUI_BRIDGE_INVALID_STATE;
    char *copy = NULL;
    status = bridge_copy(text_utf8, &copy);
    if (status != CUI_BRIDGE_OK)
        return status;
    free(node->text);
    node->text = copy;
    return CUI_BRIDGE_OK;
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
    app->events[tail] = (cui_bridge_event){.type = CUI_BRIDGE_EVENT_CLICK,
                                           .node = {binding->slot, node->generation},
                                           .frame_sequence = event->frame_sequence};
    app->event_count++;
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
    } else if (node->kind == CUI_BRIDGE_LABEL) {
        cui_begin_box(app->ui, node->id, (cui_box){0});
        cui_text(app->ui, cui_string(node->text), app->theme.font_size, app->theme.text);
        cui_end_box(app->ui);
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
        app->event_head = (app->event_head + 1) % app->max_events;
        app->event_count--;
        if (!bridge_node(app, event.node))
            continue;
        *out_event = event;
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
cui_bridge_status cui_bridge_request_close(cui_bridge_app *app) {
    cui_bridge_status status = bridge_ready(app);
    if (status != CUI_BRIDGE_OK)
        return status;
    app->closed = true;
    return CUI_BRIDGE_OK;
}
