/* SPDX-License-Identifier: MIT */
#include <cui/cui_bridge.h>
#include <float.h>
#include <math.h>
#include <node_api.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    cui_bridge_app *bridge;
} js_app;
typedef struct {
    js_app *owner;
    cui_bridge_node node;
    napi_ref app_ref;
} js_node;
static const napi_type_tag app_tag = {0xd60f0a84f38740dfULL, 0x9b60ca1b25ce5a82ULL};
static const napi_type_tag node_tag = {0x6d74ed9190c54b22ULL, 0x8af239ce73bd1174ULL};
static int tagged(napi_env env, napi_value value, const napi_type_tag *tag) {
    bool match = false;
    return napi_check_object_type_tag(env, value, tag, &match) == napi_ok && match;
}

static napi_value fail(napi_env env, const char *message) {
    napi_throw_error(env, NULL, message);
    return NULL;
}
static napi_value status(napi_env env, cui_bridge_status code) {
    return code == CUI_BRIDGE_OK ? NULL : fail(env, cui_bridge_status_message(code));
}
static void app_final(napi_env env, void *data, void *hint) {
    js_app *app = data;
    (void)env;
    (void)hint;
    if (app) {
        if (app->bridge)
            cui_bridge_destroy(app->bridge);
        free(app);
    }
}
static void node_final(napi_env env, void *data, void *hint) {
    js_node *node = data;
    (void)hint;
    if (node) {
        if (node->app_ref)
            (void)napi_delete_reference(env, node->app_ref);
        free(node);
    }
}
static int args(napi_env env, napi_callback_info info, size_t required, napi_value *v) {
    size_t count = required;
    if (napi_get_cb_info(env, info, &count, v, NULL, NULL) != napi_ok || count < required) {
        fail(env, "missing argument");
        return 0;
    }
    return 1;
}
static int app_arg(napi_env env, napi_value v, js_app **out) {
    if (!tagged(env, v, &app_tag) || napi_get_value_external(env, v, (void **)out) != napi_ok ||
        !*out || !(*out)->bridge) {
        fail(env, "app is disposed or invalid");
        return 0;
    }
    return 1;
}
static int node_arg(napi_env env, napi_value v, js_app *app, js_node **out) {
    if (!tagged(env, v, &node_tag) || napi_get_value_external(env, v, (void **)out) != napi_ok ||
        !*out || (*out)->owner != app) {
        fail(env, "node belongs to another app or is invalid");
        return 0;
    }
    return 1;
}
static int string_arg(napi_env env, napi_value v, char **out) {
    size_t len;
    if (napi_get_value_string_utf8(env, v, NULL, 0, &len) != napi_ok) {
        fail(env, "expected string");
        return 0;
    }
    if (len > 4096) {
        fail(env, "string exceeds 4096 UTF-8 bytes");
        return 0;
    }
    *out = malloc(len + 1);
    if (!*out) {
        fail(env, "allocation failed");
        return 0;
    }
    if (napi_get_value_string_utf8(env, v, *out, len + 1, &len) != napi_ok) {
        free(*out);
        *out = NULL;
        fail(env, "invalid string");
        return 0;
    }
    if (strlen(*out) != len) {
        free(*out);
        *out = NULL;
        fail(env, "embedded NUL is not allowed");
        return 0;
    }
    return 1;
}
static int number_arg(napi_env env, napi_value v, double *out) {
    if (napi_get_value_double(env, v, out) != napi_ok || !isfinite(*out) || fabs(*out) > FLT_MAX) {
        fail(env, "expected finite number");
        return 0;
    }
    return 1;
}
static int uint_arg(napi_env env, napi_value v, uint32_t *out) {
    double value;
    if (!number_arg(env, v, &value))
        return 0;
    if (value < 0 || value > UINT32_MAX || floor(value) != value) {
        fail(env, "expected unsigned integer");
        return 0;
    }
    *out = (uint32_t)value;
    return 1;
}
static napi_value undefined(napi_env env) {
    napi_value v;
    return napi_get_undefined(env, &v) == napi_ok ? v : NULL;
}
static int field(napi_env env, napi_value object, const char *name, napi_value *out) {
    return napi_get_named_property(env, object, name, out) == napi_ok;
}
static int optional_string(napi_env env, napi_value object, const char *name, char **out) {
    napi_value value;
    napi_valuetype type;
    *out = NULL;
    if (!field(env, object, name, &value) || napi_typeof(env, value, &type) != napi_ok)
        return 0;
    if (type == napi_undefined || type == napi_null)
        return 1;
    return string_arg(env, value, out);
}
static napi_value create(napi_env env, napi_callback_info info) {
    napi_value v[1], value, result;
    double d;
    uint32_t backend, max_nodes, max_events;
    char *title = NULL, *font = NULL;
    if (!args(env, info, 1, v) || !field(env, v[0], "width", &value) || !number_arg(env, value, &d))
        return NULL;
    cui_bridge_options options = {0};
    options.struct_size = cui_bridge_options_size();
    options.abi_version = CUI_BRIDGE_ABI_VERSION;
    options.width = (float)d;
    if (!field(env, v[0], "height", &value) || !number_arg(env, value, &d))
        return NULL;
    options.height = (float)d;
    if (!field(env, v[0], "backend", &value) || !uint_arg(env, value, &backend))
        return NULL;
    options.backend = (cui_bridge_backend)backend;
    if (!field(env, v[0], "maxNodes", &value) || !uint_arg(env, value, &max_nodes))
        return NULL;
    options.max_nodes = max_nodes;
    if (!field(env, v[0], "maxEvents", &value) || !uint_arg(env, value, &max_events))
        return NULL;
    options.max_events = max_events;
    if (!optional_string(env, v[0], "title", &title) ||
        !optional_string(env, v[0], "fontPath", &font)) {
        free(title);
        free(font);
        return NULL;
    }
    options.title_utf8 = title;
    options.font_path_utf8 = font;
    js_app *app = calloc(1, sizeof(*app));
    if (!app) {
        free(title);
        free(font);
        return fail(env, "allocation failed");
    }
    cui_bridge_status code;
    app->bridge = cui_bridge_create(&options, &code);
    free(title);
    free(font);
    if (!app->bridge) {
        free(app);
        return fail(env, cui_bridge_status_message(code));
    }
    if (napi_create_external(env, app, app_final, NULL, &result) != napi_ok) {
        app_final(env, app, NULL);
        return NULL;
    }
    if (napi_type_tag_object(env, result, &app_tag) != napi_ok)
        return fail(env, "failed to tag app");
    return result;
}
static napi_value dispose(napi_env env, napi_callback_info info) {
    napi_value v[1];
    js_app *app;
    if (!args(env, info, 1, v) || !tagged(env, v[0], &app_tag) ||
        napi_get_value_external(env, v[0], (void **)&app) != napi_ok || !app)
        return fail(env, "invalid app");
    if (app->bridge) {
        cui_bridge_destroy(app->bridge);
        app->bridge = NULL;
    }
    return undefined(env);
}
static napi_value make_node(napi_env env, napi_callback_info info) {
    napi_value v[4], result;
    js_app *app;
    js_node *parent = NULL, *node;
    char *key = NULL;
    uint32_t kind;
    napi_valuetype type;
    if (!args(env, info, 4, v) || !app_arg(env, v[0], &app) || !uint_arg(env, v[2], &kind) ||
        !string_arg(env, v[3], &key))
        return NULL;
    if (napi_typeof(env, v[1], &type) != napi_ok) {
        free(key);
        return NULL;
    }
    if (type != napi_null && !node_arg(env, v[1], app, &parent)) {
        free(key);
        return NULL;
    }
    node = calloc(1, sizeof(*node));
    if (!node) {
        free(key);
        return fail(env, "allocation failed");
    }
    node->owner = app;
    cui_bridge_node root = {0, 0};
    cui_bridge_status code = cui_bridge_node_create(app->bridge, parent ? parent->node : root,
                                                    (cui_bridge_kind)kind, key, &node->node);
    free(key);
    if (code != CUI_BRIDGE_OK) {
        free(node);
        return status(env, code);
    }
    if (napi_create_reference(env, v[0], 1, &node->app_ref) != napi_ok) {
        (void)cui_bridge_node_remove(app->bridge, node->node);
        free(node);
        return fail(env, "failed to retain app");
    }
    if (napi_create_external(env, node, node_final, NULL, &result) != napi_ok) {
        (void)cui_bridge_node_remove(app->bridge, node->node);
        node_final(env, node, NULL);
        return NULL;
    }
    if (napi_type_tag_object(env, result, &node_tag) != napi_ok) {
        (void)cui_bridge_node_remove(app->bridge, node->node);
        return fail(env, "failed to tag node");
    }
    return result;
}
static napi_value node_id(napi_env env, napi_callback_info info) {
    napi_value v[1], result, x;
    js_node *node;
    if (!args(env, info, 1, v) || !tagged(env, v[0], &node_tag) ||
        napi_get_value_external(env, v[0], (void **)&node) != napi_ok || !node)
        return fail(env, "invalid node");
    if (napi_create_object(env, &result) != napi_ok ||
        napi_create_uint32(env, node->node.slot, &x) != napi_ok ||
        napi_set_named_property(env, result, "slot", x) != napi_ok ||
        napi_create_uint32(env, node->node.generation, &x) != napi_ok ||
        napi_set_named_property(env, result, "generation", x) != napi_ok)
        return NULL;
    return result;
}
static napi_value remove_node(napi_env env, napi_callback_info info) {
    napi_value v[2];
    js_app *app;
    js_node *node;
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node))
        return NULL;
    {
        cui_bridge_status code = cui_bridge_node_remove(app->bridge, node->node);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
    }
    return undefined(env);
}
static napi_value set_text(napi_env env, napi_callback_info info) {
    napi_value v[3];
    js_app *app;
    js_node *node;
    char *value;
    if (!args(env, info, 3, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !string_arg(env, v[2], &value))
        return NULL;
    cui_bridge_status code = cui_bridge_set_text(app->bridge, node->node, value);
    free(value);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    return undefined(env);
}
static napi_value set_num(napi_env env, napi_callback_info info) {
    napi_value v[3];
    js_app *app;
    js_node *node;
    double number;
    uint32_t which;
    void *data;
    size_t argc = 3;
    if (napi_get_cb_info(env, info, &argc, v, NULL, &data) != napi_ok || argc < 3 ||
        !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !number_arg(env, v[2], &number))
        return NULL;
    which = (uint32_t)(uintptr_t)data;
    if (which != 1 && which != 7 && (number < 0 || number > UINT32_MAX || floor(number) != number))
        return fail(env, "expected unsigned integer");
    cui_bridge_status code = CUI_BRIDGE_INVALID_ARGUMENT;
    if (which == 1)
        code = cui_bridge_set_gap(app->bridge, node->node, (float)number);
    if (which == 2)
        code = cui_bridge_set_button_variant(app->bridge, node->node,
                                             (cui_bridge_button_variant)(int)number);
    if (which == 3)
        code = cui_bridge_set_button_size(app->bridge, node->node,
                                          (cui_bridge_button_size)(int)number);
    if (which == 4)
        code = cui_bridge_set_button_rounding(app->bridge, node->node,
                                              (cui_bridge_button_rounding)(int)number, 0);
    if (which == 5)
        code = cui_bridge_set_disabled(app->bridge, node->node, (uint32_t)number);
    if (which == 6)
        code = cui_bridge_set_checked(app->bridge, node->node, (uint32_t)number);
    if (which == 8)
        code = cui_bridge_set_open(app->bridge, node->node, (uint32_t)number);
    if (which == 7)
        code = cui_bridge_set_value(app->bridge, node->node, number);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    return undefined(env);
}
static napi_value set_range(napi_env env, napi_callback_info info) {
    napi_value v[5];
    js_app *app;
    js_node *node;
    double a, b, c;
    if (!args(env, info, 5, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !number_arg(env, v[2], &a) || !number_arg(env, v[3], &b) || !number_arg(env, v[4], &c))
        return NULL;
    cui_bridge_status code = cui_bridge_set_range(app->bridge, node->node, a, b, c);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value get_state(napi_env env, napi_callback_info info) {
    napi_value v[2], result;
    js_app *app;
    js_node *node;
    void *data;
    size_t count = 2;
    if (napi_get_cb_info(env, info, &count, v, NULL, &data) != napi_ok || count < 2 ||
        !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node))
        return NULL;
    cui_bridge_status code;
    uint32_t checked = 0, required = 0;
    double value = 0;
    char text[4097];
    if ((uintptr_t)data == 1) {
        code = cui_bridge_get_checked(app->bridge, node->node, &checked);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
        if (napi_get_boolean(env, checked != 0, &result) != napi_ok)
            return NULL;
    } else if ((uintptr_t)data == 2) {
        code = cui_bridge_get_value(app->bridge, node->node, &value);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
        if (napi_create_double(env, value, &result) != napi_ok)
            return NULL;
    } else {
        code = cui_bridge_get_text(app->bridge, node->node, text, sizeof text, &required);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
        if (napi_create_string_utf8(env, text, NAPI_AUTO_LENGTH, &result) != napi_ok)
            return NULL;
    }
    return result;
}
static napi_value queue_text(napi_env env, napi_callback_info info) {
    napi_value v[2];
    js_app *app;
    char *text;
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app) || !string_arg(env, v[1], &text))
        return NULL;
    cui_bridge_status code = cui_bridge_queue_text(app->bridge, text);
    free(text);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value set_detail(napi_env env, napi_callback_info info) {
    napi_value v[3];
    js_app *app;
    js_node *node;
    char *text;
    if (!args(env, info, 3, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !string_arg(env, v[2], &text))
        return NULL;
    cui_bridge_status code = cui_bridge_set_detail_text(app->bridge, node->node, text);
    free(text);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value set_selected(napi_env env, napi_callback_info info) {
    napi_value v[3];
    js_app *app;
    js_node *group, *item;
    if (!args(env, info, 3, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &group) ||
        !node_arg(env, v[2], app, &item))
        return NULL;
    cui_bridge_status code = cui_bridge_set_selected(app->bridge, group->node, item->node);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value get_selected(napi_env env, napi_callback_info info) {
    napi_value v[2], result, x;
    js_app *app;
    js_node *node;
    cui_bridge_node selected = {0};
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node))
        return NULL;
    cui_bridge_status code = cui_bridge_get_selected(app->bridge, node->node, &selected);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (napi_create_object(env, &result) != napi_ok ||
        napi_create_uint32(env, selected.slot, &x) != napi_ok ||
        napi_set_named_property(env, result, "slot", x) != napi_ok ||
        napi_create_uint32(env, selected.generation, &x) != napi_ok ||
        napi_set_named_property(env, result, "generation", x) != napi_ok)
        return NULL;
    return result;
}
static napi_value dropdown_bounds(napi_env env, napi_callback_info info) {
    napi_value v[3], result, x;
    js_app *app;
    js_node *node;
    uint32_t part;
    cui_bridge_rect rect;
    if (!args(env, info, 3, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !uint_arg(env, v[2], &part))
        return NULL;
    cui_bridge_status code = cui_bridge_dropdown_part_bounds(app->bridge, node->node, part, &rect);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (napi_create_object(env, &result) != napi_ok)
        return NULL;
    const char *names[] = {"x", "y", "width", "height"};
    float values[] = {rect.x, rect.y, rect.width, rect.height};
    for (size_t i = 0; i < 4; i++)
        if (napi_create_double(env, values[i], &x) != napi_ok ||
            napi_set_named_property(env, result, names[i], x) != napi_ok)
            return NULL;
    return result;
}
static napi_value set_round(napi_env env, napi_callback_info info) {
    napi_value v[4];
    js_app *app;
    js_node *node;
    uint32_t mode;
    double radius;
    if (!args(env, info, 4, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !uint_arg(env, v[2], &mode) || !number_arg(env, v[3], &radius))
        return NULL;
    {
        cui_bridge_status code = cui_bridge_set_button_rounding(
            app->bridge, node->node, (cui_bridge_button_rounding)mode, (float)radius);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
    }
    return undefined(env);
}
static napi_value set_hover(napi_env env, napi_callback_info info) {
    napi_value v[6];
    js_app *app;
    js_node *node;
    uint32_t rgba[4];
    if (!args(env, info, 6, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node))
        return NULL;
    for (int i = 0; i < 4; i++)
        if (!uint_arg(env, v[i + 2], &rgba[i]) || rgba[i] > 255)
            return fail(env, "RGBA channel must be 0..255");
    {
        cui_bridge_status code =
            cui_bridge_set_button_hover_rgba(app->bridge, node->node, (uint8_t)rgba[0],
                                             (uint8_t)rgba[1], (uint8_t)rgba[2], (uint8_t)rgba[3]);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
    }
    return undefined(env);
}
static napi_value pointer(napi_env env, napi_callback_info info) {
    napi_value v[4];
    js_app *app;
    uint32_t phase;
    double x, y;
    if (!args(env, info, 4, v) || !app_arg(env, v[0], &app) || !uint_arg(env, v[1], &phase) ||
        !number_arg(env, v[2], &x) || !number_arg(env, v[3], &y))
        return NULL;
    {
        cui_bridge_status code = cui_bridge_queue_pointer(
            app->bridge, (cui_bridge_pointer_phase)phase, (float)x, (float)y);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
    }
    return undefined(env);
}
static napi_value key_input(napi_env env, napi_callback_info info) {
    napi_value v[5];
    js_app *app;
    uint32_t phase, key, mods, repeat;
    if (!args(env, info, 5, v) || !app_arg(env, v[0], &app) || !uint_arg(env, v[1], &phase) ||
        !uint_arg(env, v[2], &key) || !uint_arg(env, v[3], &mods) || !uint_arg(env, v[4], &repeat))
        return NULL;
    cui_bridge_status code = cui_bridge_queue_key(app->bridge, (cui_bridge_key_phase)phase,
                                                  (cui_bridge_key)key, mods, repeat);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    return undefined(env);
}
static napi_value step(napi_env env, napi_callback_info info) {
    napi_value v[2];
    js_app *app;
    double dt;
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app) || !number_arg(env, v[1], &dt))
        return NULL;
    {
        cui_bridge_status code = cui_bridge_step(app->bridge, (float)dt);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
    }
    return undefined(env);
}
static napi_value poll(napi_env env, napi_callback_info info) {
    napi_value v[1], result, x;
    js_app *app;
    cui_bridge_event_ex event;
    char text[4097];
    uint32_t required;
    if (!args(env, info, 1, v) || !app_arg(env, v[0], &app))
        return NULL;
    cui_bridge_status code =
        cui_bridge_poll_event_ex(app->bridge, &event, text, sizeof text, &required);
    if (code == CUI_BRIDGE_EMPTY)
        return undefined(env);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (napi_create_object(env, &result) != napi_ok ||
        napi_create_uint32(env, (uint32_t)event.type, &x) != napi_ok ||
        napi_set_named_property(env, result, "type", x) != napi_ok ||
        napi_create_uint32(env, event.node.slot, &x) != napi_ok ||
        napi_set_named_property(env, result, "slot", x) != napi_ok ||
        napi_create_uint32(env, event.node.generation, &x) != napi_ok ||
        napi_set_named_property(env, result, "generation", x) != napi_ok ||
        napi_create_double(env, (double)event.frame_sequence, &x) != napi_ok ||
        napi_set_named_property(env, result, "frame", x) != napi_ok ||
        napi_get_boolean(env, event.checked != 0, &x) != napi_ok ||
        napi_set_named_property(env, result, "checked", x) != napi_ok ||
        napi_create_double(env, event.value, &x) != napi_ok ||
        napi_set_named_property(env, result, "value", x) != napi_ok ||
        napi_create_string_utf8(env, text, NAPI_AUTO_LENGTH, &x) != napi_ok ||
        napi_set_named_property(env, result, "text", x) != napi_ok ||
        napi_create_uint32(env, event.selected.slot, &x) != napi_ok ||
        napi_set_named_property(env, result, "selectedSlot", x) != napi_ok ||
        napi_create_uint32(env, event.selected.generation, &x) != napi_ok ||
        napi_set_named_property(env, result, "selectedGeneration", x) != napi_ok)
        return NULL;
    return result;
}
static napi_value bounds(napi_env env, napi_callback_info info) {
    napi_value v[2], result, x;
    js_app *app;
    js_node *node;
    cui_bridge_rect rect;
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node))
        return NULL;
    {
        cui_bridge_status code = cui_bridge_node_bounds(app->bridge, node->node, &rect);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
    }
    if (napi_create_object(env, &result) != napi_ok)
        return NULL;
#define RECT_FIELD(name)                                                                           \
    if (napi_create_double(env, rect.name, &x) != napi_ok ||                                       \
        napi_set_named_property(env, result, #name, x) != napi_ok)                                 \
    return NULL
    RECT_FIELD(x);
    RECT_FIELD(y);
    RECT_FIELD(width);
    RECT_FIELD(height);
#undef RECT_FIELD
    return result;
}
static napi_value close_app(napi_env env, napi_callback_info info) {
    napi_value v[1];
    js_app *app;
    if (!args(env, info, 1, v) || !app_arg(env, v[0], &app))
        return NULL;
    {
        cui_bridge_status code = cui_bridge_request_close(app->bridge);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
    }
    return undefined(env);
}
static int export_fn(napi_env env, napi_value exports, const char *name, napi_callback fn,
                     uintptr_t data) {
    napi_value value;
    return napi_create_function(env, name, NAPI_AUTO_LENGTH, fn, (void *)data, &value) == napi_ok &&
           napi_set_named_property(env, exports, name, value) == napi_ok;
}
static napi_value init(napi_env env, napi_value exports) {
#define EXPORT(name, fn, data)                                                                     \
    if (!export_fn(env, exports, name, fn, data))                                                  \
    return NULL
    EXPORT("create", create, 0);
    EXPORT("dispose", dispose, 0);
    EXPORT("makeNode", make_node, 0);
    EXPORT("nodeId", node_id, 0);
    EXPORT("removeNode", remove_node, 0);
    EXPORT("setText", set_text, 0);
    EXPORT("setGap", set_num, 1);
    EXPORT("setVariant", set_num, 2);
    EXPORT("setSize", set_num, 3);
    EXPORT("setRounding", set_num, 4);
    EXPORT("setDisabled", set_num, 5);
    EXPORT("setDetail", set_detail, 0);
    EXPORT("setOpen", set_num, 8);
    EXPORT("setSelected", set_selected, 0);
    EXPORT("getSelected", get_selected, 0);
    EXPORT("setChecked", set_num, 6);
    EXPORT("setValue", set_num, 7);
    EXPORT("setRange", set_range, 0);
    EXPORT("getChecked", get_state, 1);
    EXPORT("getValue", get_state, 2);
    EXPORT("getText", get_state, 3);
    EXPORT("textInput", queue_text, 0);
    EXPORT("setCustomRounding", set_round, 0);
    EXPORT("setHoverRgba", set_hover, 0);
    EXPORT("pointer", pointer, 0);
    EXPORT("key", key_input, 0);
    EXPORT("step", step, 0);
    EXPORT("poll", poll, 0);
    EXPORT("bounds", bounds, 0);
    EXPORT("dropdownBounds", dropdown_bounds, 0);
    EXPORT("close", close_app, 0);
#undef EXPORT
    return exports;
}
NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
