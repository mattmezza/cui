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
static int string_arg_limit(napi_env env, napi_value v, char **out, size_t limit) {
    size_t len;
    if (napi_get_value_string_utf8(env, v, NULL, 0, &len) != napi_ok) {
        fail(env, "expected string");
        return 0;
    }
    if (len > limit) {
        fail(env, limit == 4096 ? "string exceeds 4096 UTF-8 bytes"
                                : "string exceeds 65536 UTF-8 bytes");
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
static int string_arg(napi_env env, napi_value v, char **out) {
    return string_arg_limit(env, v, out, 4096);
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
    if ((which == 2 && number > 3) || (which == 3 && number > 14) || (which == 4 && number > 4) ||
        (which == 11 && number > 4))
        return fail(env, "enum value is outside its supported range");
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
    if (which == 9)
        code = cui_bridge_set_total(app->bridge, node->node, (uint32_t)number);
    if (which == 10)
        code = cui_bridge_set_step_status(app->bridge, node->node, (uint32_t)number);
    if (which == 13)
        code = cui_bridge_set_text_view_format(app->bridge, node->node, (uint32_t)number);
    if (which == 11)
        code = cui_bridge_set_attachment_status(app->bridge, node->node,
                                                (cui_bridge_attachment_status)number);
    if (which == 12)
        code = cui_bridge_set_color_rgba(app->bridge, node->node, (uint32_t)number);
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
    } else if ((uintptr_t)data == 5) {
        code = cui_bridge_get_color_rgba(app->bridge, node->node, &checked);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
        if (napi_create_uint32(env, checked, &result) != napi_ok)
            return NULL;
    } else if ((uintptr_t)data == 4) {
        code = cui_bridge_get_open(app->bridge, node->node, &checked);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
        if (napi_get_boolean(env, checked != 0, &result) != napi_ok)
            return NULL;
    } else {
        code = (uintptr_t)data == 7 ? cui_bridge_speech_transcript(app->bridge, node->node, text,
                                                                   sizeof text, &required)
               : (uintptr_t)data == 6
                   ? cui_bridge_text_view_copy_selection(app->bridge, node->node, text, sizeof text,
                                                         &required)
                   : cui_bridge_get_text(app->bridge, node->node, text, sizeof text, &required);
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
static napi_value set_anchor(napi_env env, napi_callback_info info) {
    napi_value v[3];
    js_app *app;
    js_node *node, *anchor;
    if (!args(env, info, 3, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !node_arg(env, v[2], app, &anchor))
        return NULL;
    cui_bridge_status code = cui_bridge_set_anchor(app->bridge, node->node, anchor->node);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value set_visual(napi_env env, napi_callback_info info) {
    napi_value v[4];
    js_app *app;
    js_node *node;
    double w, h;
    if (!args(env, info, 4, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !number_arg(env, v[2], &w) || !number_arg(env, v[3], &h))
        return NULL;
    cui_bridge_status code =
        cui_bridge_set_visual_size(app->bridge, node->node, (float)w, (float)h);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value numeric_bounds(napi_env env, napi_callback_info info) {
    napi_value v[3], result, x;
    js_app *app;
    js_node *node;
    uint32_t part;
    cui_bridge_rect rect;
    if (!args(env, info, 3, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !uint_arg(env, v[2], &part))
        return NULL;
    cui_bridge_status code = cui_bridge_numeric_part_bounds(app->bridge, node->node, part, &rect);
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
static napi_value set_scroll_extent(napi_env env, napi_callback_info info) {
    napi_value v[4];
    js_app *app;
    js_node *node;
    double w, h;
    if (!args(env, info, 4, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !number_arg(env, v[2], &w) || !number_arg(env, v[3], &h))
        return NULL;
    cui_bridge_status code =
        cui_bridge_set_scroll_extent(app->bridge, node->node, (float)w, (float)h);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value queue_scroll(napi_env env, napi_callback_info info) {
    napi_value v[5];
    js_app *app;
    double x, y, dx, dy;
    if (!args(env, info, 5, v) || !app_arg(env, v[0], &app) || !number_arg(env, v[1], &x) ||
        !number_arg(env, v[2], &y) || !number_arg(env, v[3], &dx) || !number_arg(env, v[4], &dy))
        return NULL;
    cui_bridge_status code =
        cui_bridge_queue_scroll(app->bridge, (float)x, (float)y, (float)dx, (float)dy);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value select_text(napi_env env, napi_callback_info info) {
    napi_value v[4];
    js_app *app;
    js_node *node;
    uint32_t anchor, cursor;
    if (!args(env, info, 4, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !uint_arg(env, v[2], &anchor) || !uint_arg(env, v[3], &cursor))
        return NULL;
    cui_bridge_status code = cui_bridge_text_view_select(app->bridge, node->node, anchor, cursor);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value resizable_bounds(napi_env env, napi_callback_info info) {
    napi_value v[3], result, x;
    js_app *app;
    js_node *group, *item;
    cui_bridge_rect rect;
    if (!args(env, info, 3, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &group) ||
        !node_arg(env, v[2], app, &item))
        return NULL;
    cui_bridge_status code =
        cui_bridge_resizable_handle_bounds(app->bridge, group->node, item->node, &rect);
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
static napi_value set_font_fallbacks(napi_env env, napi_callback_info info) {
    napi_value v[2], entry;
    js_app *app;
    bool is_array = false;
    uint32_t count = 0;
    char *paths[32] = {0};
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app))
        return NULL;
    if (napi_is_array(env, v[1], &is_array) != napi_ok || !is_array ||
        napi_get_array_length(env, v[1], &count) != napi_ok || count > 32)
        return fail(env, "fallbacks must be an array of at most 32 paths");
    for (uint32_t i = 0; i < count; i++) {
        if (napi_get_element(env, v[1], i, &entry) != napi_ok ||
            !string_arg(env, entry, &paths[i])) {
            for (uint32_t j = 0; j < count; j++)
                free(paths[j]);
            return NULL;
        }
    }
    cui_bridge_status code =
        cui_bridge_set_font_fallbacks(app->bridge, (const char *const *)paths, count);
    for (uint32_t i = 0; i < count; i++)
        free(paths[i]);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value set_image(napi_env env, napi_callback_info info) {
    napi_value v[6];
    js_app *app;
    js_node *node;
    uint32_t w, h, stride;
    bool typed = false;
    napi_typedarray_type type;
    size_t length, offset;
    void *data;
    napi_value buffer;
    if (!args(env, info, 6, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !uint_arg(env, v[3], &w) || !uint_arg(env, v[4], &h) || !uint_arg(env, v[5], &stride))
        return NULL;
    if (napi_is_typedarray(env, v[2], &typed) != napi_ok || !typed ||
        napi_get_typedarray_info(env, v[2], &type, &length, &data, &buffer, &offset) != napi_ok ||
        (type != napi_uint8_array && type != napi_uint8_clamped_array) || !data ||
        length > 16777216)
        return fail(env, "pixels must be a nonempty Uint8Array of at most 16 MiB");
    cui_bridge_status code =
        cui_bridge_set_image_rgba(app->bridge, node->node, data, w, h, stride, (uint32_t)length);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value set_temporal(napi_env env, napi_callback_info info) {
    napi_value v[5];
    js_app *app;
    js_node *node;
    uint32_t a, b, c;
    size_t count = 5;
    void *which;
    if (napi_get_cb_info(env, info, &count, v, NULL, &which) != napi_ok || count < 5 ||
        !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) || !uint_arg(env, v[2], &a) ||
        !uint_arg(env, v[3], &b) || !uint_arg(env, v[4], &c))
        return NULL;
    if (a > 9999 || b > UINT16_MAX || c > UINT16_MAX)
        return fail(env, "invalid time/date fields");
    cui_bridge_status code =
        (uintptr_t)which == 1
            ? cui_bridge_set_time(app->bridge, node->node, (cui_bridge_time){a, b, c})
            : cui_bridge_set_date(app->bridge, node->node,
                                  (cui_bridge_date){(int32_t)a, (int32_t)b, (int32_t)c});
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value get_temporal(napi_env env, napi_callback_info info) {
    napi_value v[2], result, x;
    js_app *app;
    js_node *node;
    size_t count = 2;
    void *which;
    if (napi_get_cb_info(env, info, &count, v, NULL, &which) != napi_ok || count < 2 ||
        !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node))
        return NULL;
    cui_bridge_time time = {0};
    cui_bridge_date date = {0};
    cui_bridge_status code = (uintptr_t)which == 1
                                 ? cui_bridge_get_time(app->bridge, node->node, &time)
                                 : cui_bridge_get_date(app->bridge, node->node, &date);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (napi_create_array_with_length(env, 3, &result) != napi_ok)
        return NULL;
    int32_t values[3] = {(int32_t)time.hour, (int32_t)time.minute, (int32_t)time.second};
    if ((uintptr_t)which != 1) {
        values[0] = date.year;
        values[1] = date.month;
        values[2] = date.day;
    }
    for (uint32_t i = 0; i < 3; i++)
        if (napi_create_int32(env, values[i], &x) != napi_ok ||
            napi_set_element(env, result, i, x) != napi_ok)
            return NULL;
    return result;
}
static napi_value specialized_bounds(napi_env env, napi_callback_info info) {
    napi_value v[5], result, x;
    js_app *app;
    js_node *node;
    size_t count = 5;
    void *which;
    uint32_t a, b = 0, c = 0;
    if (napi_get_cb_info(env, info, &count, v, NULL, &which) != napi_ok || count < 3 ||
        !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) || !uint_arg(env, v[2], &a))
        return NULL;
    if ((uintptr_t)which == 2 &&
        (count < 5 || !uint_arg(env, v[3], &b) || !uint_arg(env, v[4], &c)))
        return NULL;
    if ((uintptr_t)which == 2 && (a > 9999 || b > 12 || c > 31))
        return fail(env, "invalid date");
    cui_bridge_rect rect;
    cui_bridge_status code =
        (uintptr_t)which == 1
            ? cui_bridge_color_part_bounds(app->bridge, node->node, a, &rect)
            : cui_bridge_calendar_day_bounds(app->bridge, node->node,
                                             (cui_bridge_date){(int32_t)a, (int32_t)b, (int32_t)c},
                                             &rect);
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
static int array_count(napi_env env, napi_value value, uint32_t limit, uint32_t *count) {
    bool array = false;
    if (napi_is_array(env, value, &array) != napi_ok || !array ||
        napi_get_array_length(env, value, count) != napi_ok || *count > limit) {
        fail(env, "invalid model array length");
        return 0;
    }
    return 1;
}
static napi_value set_chart(napi_env env, napi_callback_info info) {
    napi_value v[3], pair, a, b;
    js_app *app;
    js_node *node;
    uint32_t count = 0, len = 0;
    cui_bridge_chart_datum data[64] = {{0}};
    char *labels[64] = {0};
    cui_bridge_status code = CUI_BRIDGE_OK;
    if (!args(env, info, 3, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !array_count(env, v[2], 64, &count))
        return NULL;
    for (uint32_t i = 0; i < count; i++) {
        if (napi_get_element(env, v[2], i, &pair) != napi_ok || !array_count(env, pair, 2, &len) ||
            len != 2 || napi_get_element(env, pair, 0, &a) != napi_ok ||
            napi_get_element(env, pair, 1, &b) != napi_ok) {
            fail(env, "expected [label,value]");
            goto cleanup;
        }
        if (!string_arg(env, a, &labels[i]) || !number_arg(env, b, &data[i].value))
            goto cleanup;
        data[i].label_utf8 = labels[i];
    }
    code = cui_bridge_chart_set_data(app->bridge, node->node, data, count);
cleanup:
    for (uint32_t i = 0; i < count; i++)
        free(labels[i]);
    bool pending = false;
    napi_is_exception_pending(env, &pending);
    if (pending)
        return NULL;
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value chart_kind(napi_env env, napi_callback_info info) {
    napi_value v[3];
    js_app *app;
    js_node *node;
    uint32_t kind;
    if (!args(env, info, 3, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !uint_arg(env, v[2], &kind))
        return NULL;
    cui_bridge_status code = cui_bridge_chart_set_kind(app->bridge, node->node, kind);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value get_chart(napi_env env, napi_callback_info info) {
    napi_value v[3], result, a, b;
    js_app *app;
    js_node *node;
    uint32_t index, required;
    double value;
    char label[4097];
    if (!args(env, info, 3, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !uint_arg(env, v[2], &index))
        return NULL;
    cui_bridge_status code = cui_bridge_chart_get_datum(app->bridge, node->node, index, label,
                                                        sizeof label, &required, &value);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (napi_create_array_with_length(env, 2, &result) != napi_ok ||
        napi_create_string_utf8(env, label, NAPI_AUTO_LENGTH, &a) != napi_ok ||
        napi_create_double(env, value, &b) != napi_ok ||
        napi_set_element(env, result, 0, a) != napi_ok ||
        napi_set_element(env, result, 1, b) != napi_ok)
        return NULL;
    return result;
}
static napi_value set_plot(napi_env env, napi_callback_info info) {
    napi_value v[3], pair, a, b;
    js_app *app;
    js_node *node;
    uint32_t count, len;
    cui_bridge_plot_point points[256];
    if (!args(env, info, 3, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !array_count(env, v[2], 256, &count))
        return NULL;
    for (uint32_t i = 0; i < count; i++) {
        if (napi_get_element(env, v[2], i, &pair) != napi_ok || !array_count(env, pair, 2, &len) ||
            len != 2 || napi_get_element(env, pair, 0, &a) != napi_ok ||
            napi_get_element(env, pair, 1, &b) != napi_ok)
            return fail(env, "expected [x,y]");
        if (!number_arg(env, a, &points[i].x) || !number_arg(env, b, &points[i].y))
            return NULL;
    }
    cui_bridge_status code = cui_bridge_plot_set_points(app->bridge, node->node, points, count);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value get_plot(napi_env env, napi_callback_info info) {
    napi_value v[3], result, a, b;
    js_app *app;
    js_node *node;
    uint32_t index;
    cui_bridge_plot_point point;
    if (!args(env, info, 3, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !uint_arg(env, v[2], &index))
        return NULL;
    cui_bridge_status code = cui_bridge_plot_get_point(app->bridge, node->node, index, &point);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (napi_create_array_with_length(env, 2, &result) != napi_ok ||
        napi_create_double(env, point.x, &a) != napi_ok ||
        napi_create_double(env, point.y, &b) != napi_ok ||
        napi_set_element(env, result, 0, a) != napi_ok ||
        napi_set_element(env, result, 1, b) != napi_ok)
        return NULL;
    return result;
}
static napi_value set_matrix(napi_env env, napi_callback_info info) {
    napi_value v[4], entry, row;
    js_app *app;
    js_node *node;
    uint32_t columns, rows, len;
    char *headers[16] = {0}, *cells[512] = {0};
    cui_bridge_status code = CUI_BRIDGE_OK;
    if (!args(env, info, 4, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !array_count(env, v[2], 16, &columns) || !array_count(env, v[3], 32, &rows))
        return NULL;
    if (!columns)
        return fail(env, "matrix requires headers");
    for (uint32_t i = 0; i < columns; i++) {
        if (napi_get_element(env, v[2], i, &entry) != napi_ok ||
            !string_arg(env, entry, &headers[i]))
            goto cleanup;
    }
    for (uint32_t r = 0; r < rows; r++) {
        if (napi_get_element(env, v[3], r, &row) != napi_ok || !array_count(env, row, 16, &len) ||
            len != columns) {
            fail(env, "matrix rows must match headers");
            goto cleanup;
        }
        for (uint32_t col = 0; col < columns; col++)
            if (napi_get_element(env, row, col, &entry) != napi_ok ||
                !string_arg(env, entry, &cells[r * columns + col]))
                goto cleanup;
    }
    code = cui_bridge_data_table_set(app->bridge, node->node, (const char *const *)headers, columns,
                                     (const char *const *)cells, rows);
cleanup:
    for (uint32_t i = 0; i < 16; i++)
        free(headers[i]);
    for (uint32_t i = 0; i < 512; i++)
        free(cells[i]);
    bool pending = false;
    napi_is_exception_pending(env, &pending);
    if (pending)
        return NULL;
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value get_matrix_cell(napi_env env, napi_callback_info info) {
    napi_value v[4], result;
    js_app *app;
    js_node *node;
    uint32_t row, col, required;
    char text[4097];
    if (!args(env, info, 4, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !uint_arg(env, v[2], &row) || !uint_arg(env, v[3], &col))
        return NULL;
    cui_bridge_status code = cui_bridge_data_table_get_cell(app->bridge, node->node, row, col, text,
                                                            sizeof text, &required);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (napi_create_string_utf8(env, text, NAPI_AUTO_LENGTH, &result) != napi_ok)
        return NULL;
    return result;
}
static napi_value matrix_state(napi_env env, napi_callback_info info) {
    napi_value v[2], result, value;
    js_app *app;
    js_node *node;
    uint32_t values[4];
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node))
        return NULL;
    cui_bridge_status code = cui_bridge_data_table_get_state(app->bridge, node->node, &values[0],
                                                             &values[1], &values[2], &values[3]);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    const char *names[] = {"row", "column", "sortColumn", "sortDirection"};
    if (napi_create_object(env, &result) != napi_ok)
        return NULL;
    for (uint32_t i = 0; i < 4; i++)
        if (napi_create_uint32(env, values[i], &value) != napi_ok ||
            napi_set_named_property(env, result, names[i], value) != napi_ok)
            return NULL;
    return result;
}
static napi_value matrix_bounds(napi_env env, napi_callback_info info) {
    napi_value v[4], result, value;
    js_app *app;
    js_node *node;
    uint32_t row, col;
    void *data;
    size_t argc = 4;
    cui_bridge_rect rect;
    if (napi_get_cb_info(env, info, &argc, v, NULL, &data) != napi_ok || argc < 4 ||
        !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !uint_arg(env, v[2], &row) || !uint_arg(env, v[3], &col))
        return NULL;
    cui_bridge_status code =
        data ? cui_bridge_data_table_header_bounds(app->bridge, node->node, col, &rect)
             : cui_bridge_data_table_cell_bounds(app->bridge, node->node, row, col, &rect);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    const char *names[] = {"x", "y", "width", "height"};
    float values[] = {rect.x, rect.y, rect.width, rect.height};
    if (napi_create_object(env, &result) != napi_ok)
        return NULL;
    for (uint32_t i = 0; i < 4; i++)
        if (napi_create_double(env, values[i], &value) != napi_ok ||
            napi_set_named_property(env, result, names[i], value) != napi_ok)
            return NULL;
    return result;
}

static napi_value keyed_bounds(napi_env env, napi_callback_info info) {
    napi_value v[3], result, value;
    js_app *app;
    js_node *group, *item;
    void *data;
    size_t argc = 3;
    cui_bridge_rect rect;
    if (napi_get_cb_info(env, info, &argc, v, NULL, &data) != napi_ok || argc < 3 ||
        !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &group) ||
        !node_arg(env, v[2], app, &item))
        return NULL;
    cui_bridge_status code =
        data ? cui_bridge_virtual_list_item_bounds(app->bridge, group->node, item->node, &rect)
             : cui_bridge_command_item_bounds(app->bridge, group->node, item->node, &rect);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    const char *names[] = {"x", "y", "width", "height"};
    float values[] = {rect.x, rect.y, rect.width, rect.height};
    if (napi_create_object(env, &result) != napi_ok)
        return NULL;
    for (uint32_t i = 0; i < 4; i++)
        if (napi_create_double(env, values[i], &value) != napi_ok ||
            napi_set_named_property(env, result, names[i], value) != napi_ok)
            return NULL;
    return result;
}

static napi_value table_selection_mode(napi_env env, napi_callback_info info) {
    napi_value v[3];
    js_app *app;
    js_node *node;
    uint32_t mode;
    if (!args(env, info, 3, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !uint_arg(env, v[2], &mode))
        return NULL;
    cui_bridge_status code =
        cui_bridge_data_table_set_selection_mode(app->bridge, node->node, mode);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value theme_presets(napi_env env, napi_callback_info info) {
    (void)info;
    napi_value result, value;
    uint32_t count = cui_bridge_theme_preset_count();
    if (napi_create_array_with_length(env, count, &result) != napi_ok)
        return NULL;
    for (uint32_t i = 0; i < count; i++) {
        char text[128];
        uint32_t length;
        cui_bridge_status code = cui_bridge_theme_preset_name(i, text, sizeof text, &length);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
        if (napi_create_string_utf8(env, text, NAPI_AUTO_LENGTH, &value) != napi_ok ||
            napi_set_element(env, result, i, value) != napi_ok)
            return NULL;
    }
    return result;
}
static napi_value set_theme_preset(napi_env env, napi_callback_info info) {
    napi_value v[2];
    js_app *app;
    uint32_t index;
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app) || !uint_arg(env, v[1], &index))
        return NULL;
    cui_bridge_status code = cui_bridge_set_theme_preset(app->bridge, index);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value get_theme_preset(napi_env env, napi_callback_info info) {
    napi_value v[1], result;
    js_app *app;
    uint32_t index;
    if (!args(env, info, 1, v) || !app_arg(env, v[0], &app))
        return NULL;
    cui_bridge_status code = cui_bridge_get_theme_preset(app->bridge, &index);
    if (code == CUI_BRIDGE_EMPTY)
        return undefined(env);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (napi_create_uint32(env, index, &result) != napi_ok)
        return NULL;
    return result;
}

static napi_value set_root_padding(napi_env env, napi_callback_info info) {
    napi_value v[3];
    js_app *app;
    js_node *node;
    double value;
    if (!args(env, info, 3, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !number_arg(env, v[2], &value))
        return NULL;
    cui_bridge_status code = cui_bridge_set_root_padding(app->bridge, node->node, (float)value);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}

static napi_value set_clipboard(napi_env env, napi_callback_info info) {
    napi_value v[2];
    js_app *app;
    char *text;
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app) ||
        !string_arg_limit(env, v[1], &text, 65536))
        return NULL;
    cui_bridge_status code = cui_bridge_clipboard_set(app->bridge, text);
    free(text);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value get_clipboard(napi_env env, napi_callback_info info) {
    napi_value v[1], result;
    js_app *app;
    uint32_t length;
    if (!args(env, info, 1, v) || !app_arg(env, v[0], &app))
        return NULL;
    char *text = malloc(65537);
    if (!text)
        return fail(env, "allocation failed");
    cui_bridge_status code = cui_bridge_clipboard_get(app->bridge, text, 65537, &length);
    if (code != CUI_BRIDGE_OK) {
        free(text);
        return status(env, code);
    }
    napi_status result_status = napi_create_string_utf8(env, text, NAPI_AUTO_LENGTH, &result);
    free(text);
    return result_status == napi_ok ? result : NULL;
}

static napi_value speech_control(napi_env env, napi_callback_info info) {
    napi_value v[2];
    js_app *app;
    js_node *node;
    void *data;
    size_t argc = 2;
    if (napi_get_cb_info(env, info, &argc, v, NULL, &data) != napi_ok || argc < 2 ||
        !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node))
        return NULL;
    cui_bridge_status code = data ? cui_bridge_speech_finish(app->bridge, node->node)
                                  : cui_bridge_speech_start(app->bridge, node->node);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value speech_report(napi_env env, napi_callback_info info) {
    napi_value v[4];
    js_app *app;
    js_node *node;
    uint32_t final;
    char *text;
    if (!args(env, info, 4, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !uint_arg(env, v[3], &final) || !string_arg(env, v[2], &text))
        return NULL;
    cui_bridge_status code = cui_bridge_speech_report(app->bridge, node->node, text, final);
    free(text);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}

static napi_value question_config(napi_env env, napi_callback_info info) {
    napi_value v[4];
    js_app *app;
    js_node *node;
    uint32_t required, multiple;
    if (!args(env, info, 4, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !uint_arg(env, v[2], &required) || !uint_arg(env, v[3], &multiple))
        return NULL;
    cui_bridge_status code =
        cui_bridge_questionnaire_set_question(app->bridge, node->node, required, multiple);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value question_state(napi_env env, napi_callback_info info) {
    napi_value v[2], result, value;
    js_app *app;
    js_node *node;
    cui_bridge_node current;
    uint32_t complete;
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node))
        return NULL;
    cui_bridge_status code =
        cui_bridge_questionnaire_get_state(app->bridge, node->node, &current, &complete);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (napi_create_object(env, &result) != napi_ok ||
        napi_create_uint32(env, current.slot, &value) != napi_ok ||
        napi_set_named_property(env, result, "slot", value) != napi_ok ||
        napi_create_uint32(env, current.generation, &value) != napi_ok ||
        napi_set_named_property(env, result, "generation", value) != napi_ok ||
        napi_get_boolean(env, complete != 0, &value) != napi_ok ||
        napi_set_named_property(env, result, "complete", value) != napi_ok)
        return NULL;
    return result;
}
static napi_value choice_selected(napi_env env, napi_callback_info info) {
    napi_value v[2], result;
    js_app *app;
    js_node *node;
    uint32_t selected;
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node))
        return NULL;
    cui_bridge_status code =
        cui_bridge_questionnaire_choice_selected(app->bridge, node->node, &selected);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (napi_get_boolean(env, selected != 0, &result) != napi_ok)
        return NULL;
    return result;
}
static napi_value question_move(napi_env env, napi_callback_info info) {
    napi_value v[2];
    js_app *app;
    js_node *node;
    void *data;
    size_t argc = 2;
    if (napi_get_cb_info(env, info, &argc, v, NULL, &data) != napi_ok || argc < 2 ||
        !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node))
        return NULL;
    cui_bridge_status code = (uintptr_t)data == 2
                                 ? cui_bridge_questionnaire_submit(app->bridge, node->node)
                             : data ? cui_bridge_questionnaire_previous(app->bridge, node->node)
                                    : cui_bridge_questionnaire_next(app->bridge, node->node);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}

static napi_value dock_activate(napi_env env, napi_callback_info info) {
    napi_value v[3];
    js_app *app;
    js_node *dock, *panel;
    if (!args(env, info, 3, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &dock) ||
        !node_arg(env, v[2], app, &panel))
        return NULL;
    cui_bridge_status code = cui_bridge_dock_activate(app->bridge, dock->node, panel->node);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value dock_move(napi_env env, napi_callback_info info) {
    napi_value v[7];
    js_app *app;
    js_node *dock, *panel, *reference;
    uint32_t axis, before;
    double ratio;
    if (!args(env, info, 7, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &dock) ||
        !node_arg(env, v[2], app, &panel) || !node_arg(env, v[3], app, &reference) ||
        !uint_arg(env, v[4], &axis) || !number_arg(env, v[5], &ratio) ||
        !uint_arg(env, v[6], &before))
        return NULL;
    cui_bridge_status code = cui_bridge_dock_move_split(app->bridge, dock->node, panel->node,
                                                        reference->node, axis, ratio, before);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value dock_active(napi_env env, napi_callback_info info) {
    napi_value v[2], result, value;
    js_app *app;
    js_node *node;
    cui_bridge_node active;
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node))
        return NULL;
    cui_bridge_status code = cui_bridge_dock_get_active(app->bridge, node->node, &active);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (napi_create_object(env, &result) != napi_ok ||
        napi_create_uint32(env, active.slot, &value) != napi_ok ||
        napi_set_named_property(env, result, "slot", value) != napi_ok ||
        napi_create_uint32(env, active.generation, &value) != napi_ok ||
        napi_set_named_property(env, result, "generation", value) != napi_ok)
        return NULL;
    return result;
}
static napi_value dock_bounds(napi_env env, napi_callback_info info) {
    napi_value v[3], result, value;
    js_app *app;
    js_node *dock, *panel;
    cui_bridge_rect rect;
    if (!args(env, info, 3, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &dock) ||
        !node_arg(env, v[2], app, &panel))
        return NULL;
    cui_bridge_status code =
        cui_bridge_dock_panel_bounds(app->bridge, dock->node, panel->node, &rect);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    const char *names[] = {"x", "y", "width", "height"};
    float values[] = {rect.x, rect.y, rect.width, rect.height};
    if (napi_create_object(env, &result) != napi_ok)
        return NULL;
    for (uint32_t i = 0; i < 4; i++)
        if (napi_create_double(env, values[i], &value) != napi_ok ||
            napi_set_named_property(env, result, names[i], value) != napi_ok)
            return NULL;
    return result;
}
static napi_value dock_serialize(napi_env env, napi_callback_info info) {
    napi_value v[2], result;
    js_app *app;
    js_node *node;
    uint32_t required = 0;
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node))
        return NULL;
    cui_bridge_status code = cui_bridge_dock_serialize(app->bridge, node->node, NULL, 0, &required);
    if (code != CUI_BRIDGE_CAPACITY)
        return status(env, code);
    if (required == 0 || required > 1048576)
        return fail(env, "invalid dock serialization size");
    char *text = malloc(required);
    if (!text)
        return fail(env, "allocation failed");
    code = cui_bridge_dock_serialize(app->bridge, node->node, text, required, &required);
    if (code != CUI_BRIDGE_OK) {
        free(text);
        return status(env, code);
    }
    napi_status result_status = napi_create_string_utf8(env, text, NAPI_AUTO_LENGTH, &result);
    free(text);
    return result_status == napi_ok ? result : NULL;
}

static napi_value settings_page(napi_env env, napi_callback_info info) {
    napi_value v[2], result, value;
    js_app *app;
    js_node *node;
    cui_bridge_node active;
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node))
        return NULL;
    cui_bridge_status code = cui_bridge_settings_get_page(app->bridge, node->node, &active);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (napi_create_object(env, &result) != napi_ok ||
        napi_create_uint32(env, active.slot, &value) != napi_ok ||
        napi_set_named_property(env, result, "slot", value) != napi_ok ||
        napi_create_uint32(env, active.generation, &value) != napi_ok ||
        napi_set_named_property(env, result, "generation", value) != napi_ok)
        return NULL;
    return result;
}
static napi_value settings_bounds(napi_env env, napi_callback_info info) {
    napi_value v[3], result, value;
    js_app *app;
    js_node *dock, *panel;
    cui_bridge_rect rect;
    if (!args(env, info, 3, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &dock) ||
        !node_arg(env, v[2], app, &panel))
        return NULL;
    cui_bridge_status code =
        cui_bridge_settings_field_bounds(app->bridge, dock->node, panel->node, &rect);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    const char *names[] = {"x", "y", "width", "height"};
    float values[] = {rect.x, rect.y, rect.width, rect.height};
    if (napi_create_object(env, &result) != napi_ok)
        return NULL;
    for (uint32_t i = 0; i < 4; i++)
        if (napi_create_double(env, values[i], &value) != napi_ok ||
            napi_set_named_property(env, result, names[i], value) != napi_ok)
            return NULL;
    return result;
}
static napi_value settings_query(napi_env env, napi_callback_info info) {
    napi_value v[3];
    js_app *app;
    js_node *node;
    char *query = NULL;
    if (!args(env, info, 3, v) || !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !string_arg(env, v[2], &query))
        return NULL;
    cui_bridge_status code = cui_bridge_settings_set_query(app->bridge, node->node, query);
    free(query);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
static napi_value settings_change_page(napi_env env, napi_callback_info info) {
    napi_value v[3];
    js_app *app;
    js_node *node, *page;
    size_t argc = 3;
    void *data = NULL;
    if (napi_get_cb_info(env, info, &argc, v, NULL, &data) != napi_ok || argc < 3 ||
        !app_arg(env, v[0], &app) || !node_arg(env, v[1], app, &node) ||
        !node_arg(env, v[2], app, &page))
        return NULL;
    cui_bridge_status code =
        data ? cui_bridge_settings_reset_page(app->bridge, node->node, page->node)
             : cui_bridge_settings_set_page(app->bridge, node->node, page->node);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
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
    EXPORT("numericBounds", numeric_bounds, 0);
    EXPORT("setAnchor", set_anchor, 0);
    EXPORT("setVisualSize", set_visual, 0);
    EXPORT("getOpen", get_state, 4);
    EXPORT("setTotal", set_num, 9);
    EXPORT("setScrollExtent", set_scroll_extent, 0);
    EXPORT("scroll", queue_scroll, 0);
    EXPORT("setStepStatus", set_num, 10);
    EXPORT("setAttachmentStatus", set_num, 11);
    EXPORT("setColor", set_num, 12);
    EXPORT("getColor", get_state, 5);
    EXPORT("setImage", set_image, 0);
    EXPORT("resizableBounds", resizable_bounds, 0);
    EXPORT("setTextFormat", set_num, 13);
    EXPORT("selectText", select_text, 0);
    EXPORT("getSelection", get_state, 6);
    EXPORT("commandBounds", keyed_bounds, 0);
    EXPORT("virtualBounds", keyed_bounds, 1);
    EXPORT("setSelectionMode", table_selection_mode, 0);
    EXPORT("themePresets", theme_presets, 0);
    EXPORT("setThemePreset", set_theme_preset, 0);
    EXPORT("getThemePreset", get_theme_preset, 0);
    EXPORT("setRootPadding", set_root_padding, 0);
    EXPORT("setClipboard", set_clipboard, 0);
    EXPORT("getClipboard", get_clipboard, 0);
    EXPORT("startSpeech", speech_control, 0);
    EXPORT("finishSpeech", speech_control, 1);
    EXPORT("reportSpeech", speech_report, 0);
    EXPORT("getSpeech", get_state, 7);
    EXPORT("questionConfig", question_config, 0);
    EXPORT("questionState", question_state, 0);
    EXPORT("choiceSelected", choice_selected, 0);
    EXPORT("nextQuestion", question_move, 0);
    EXPORT("previousQuestion", question_move, 1);
    EXPORT("submitQuestion", question_move, 2);
    EXPORT("querySettings", settings_query, 0);
    EXPORT("pageSettings", settings_page, 0);
    EXPORT("selectSettings", settings_change_page, 0);
    EXPORT("resetSettings", settings_change_page, 1);
    EXPORT("boundsSettings", settings_bounds, 0);
    EXPORT("activateDock", dock_activate, 0);
    EXPORT("moveDock", dock_move, 0);
    EXPORT("activeDock", dock_active, 0);
    EXPORT("boundsDock", dock_bounds, 0);
    EXPORT("serializeDock", dock_serialize, 0);
    EXPORT("setChart", set_chart, 0);
    EXPORT("setChartKind", chart_kind, 0);
    EXPORT("getChart", get_chart, 0);
    EXPORT("setPlot", set_plot, 0);
    EXPORT("getPlot", get_plot, 0);
    EXPORT("setMatrix", set_matrix, 0);
    EXPORT("getMatrixCell", get_matrix_cell, 0);
    EXPORT("getMatrixState", matrix_state, 0);
    EXPORT("matrixCellBounds", matrix_bounds, 0);
    EXPORT("matrixHeaderBounds", matrix_bounds, 1);
    EXPORT("setFontFallbacks", set_font_fallbacks, 0);
    EXPORT("setTime", set_temporal, 1);
    EXPORT("getTime", get_temporal, 1);
    EXPORT("setDate", set_temporal, 2);
    EXPORT("getDate", get_temporal, 2);
    EXPORT("colorBounds", specialized_bounds, 1);
    EXPORT("dayBounds", specialized_bounds, 2);
    EXPORT("close", close_app, 0);
#undef EXPORT
    return exports;
}
NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
