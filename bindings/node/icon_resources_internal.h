/* SPDX-License-Identifier: MIT */
/* V1 owned Icon resources. This group also works with the alpha.4 provider. */
typedef struct {
    uint32_t (*versions)(void), (*record_size)(uint32_t);
    cui_bridge_status (*default_limits)(cui_bridge_icon_limits_v1 *);
    cui_bridge_status (*create)(cui_bridge_app *, const cui_bridge_icon_limits_v1 *,
                                cui_bridge_icon_handle_v1 *);
    cui_bridge_status (*destroy)(cui_bridge_app *, cui_bridge_icon_handle_v1);
    cui_bridge_status (*get_info)(cui_bridge_app *, cui_bridge_icon_handle_v1,
                                  cui_bridge_icon_info_v1 *);
    cui_bridge_status (*get_limits)(cui_bridge_app *, cui_bridge_icon_handle_v1,
                                    cui_bridge_icon_limits_v1 *);
    cui_bridge_status (*set_limits)(cui_bridge_app *, cui_bridge_icon_handle_v1,
                                    const cui_bridge_icon_limits_v1 *);
    cui_bridge_status (*set_source)(cui_bridge_app *, cui_bridge_icon_handle_v1, uint32_t,
                                    const char *, uint32_t);
    cui_bridge_status (*get_source)(cui_bridge_app *, cui_bridge_icon_handle_v1, uint32_t *, char *,
                                    uint32_t, uint32_t *);
    cui_bridge_status (*reload)(cui_bridge_app *, cui_bridge_icon_handle_v1);
    cui_bridge_status (*collect)(cui_bridge_app *, cui_bridge_icon_handle_v1);
    cui_bridge_status (*asset_put)(cui_bridge_app *, cui_bridge_icon_handle_v1, const char *,
                                   uint32_t, const char *, uint32_t);
    cui_bridge_status (*asset_remove)(cui_bridge_app *, cui_bridge_icon_handle_v1, const char *,
                                      uint32_t);
    cui_bridge_status (*asset_clear)(cui_bridge_app *, cui_bridge_icon_handle_v1);
    uint32_t (*catalog_count)(void);
    cui_bridge_status (*catalog_entry)(uint32_t, uint32_t *, char *, uint32_t, uint32_t *, char *,
                                       uint32_t, uint32_t *);
    cui_bridge_status (*query)(const char *, uint32_t, uint32_t *);
    cui_bridge_status (*asset_copy)(const char *, uint32_t, char *, uint32_t, uint32_t *);
    cui_bridge_status (*prepare)(cui_bridge_app *, cui_bridge_icon_handle_v1, float, float,
                                 const float *, uint32_t *);
    cui_bridge_status (*bitmap_copy)(cui_bridge_app *, cui_bridge_icon_handle_v1,
                                     cui_bridge_icon_bitmap_v1 *, unsigned char *, uint32_t,
                                     uint32_t *);
} icon_resources_api;

_Static_assert(sizeof(cui_bridge_icon_limits_v1) == 48, "Icon limits V1 ABI");
_Static_assert(sizeof(cui_bridge_icon_info_v1) == 80, "Icon info V1 ABI");
_Static_assert(sizeof(cui_bridge_icon_bitmap_v1) == 64, "Icon bitmap V1 ABI");

static bool icon_resources_resolve(icon_resources_api *api) {
#define ICON_SYMBOL(name, suffix)                                                                  \
    optional_bridge_symbol(&api->name, sizeof api->name, "cui_bridge_icon_" suffix)
    return ICON_SYMBOL(versions, "versions") && ICON_SYMBOL(record_size, "record_size_v1") &&
           ICON_SYMBOL(default_limits, "default_limits_v1") && ICON_SYMBOL(create, "create_v1") &&
           ICON_SYMBOL(destroy, "destroy_v1") && ICON_SYMBOL(get_info, "get_info_v1") &&
           ICON_SYMBOL(get_limits, "get_limits_v1") && ICON_SYMBOL(set_limits, "set_limits_v1") &&
           ICON_SYMBOL(set_source, "set_source_v1") && ICON_SYMBOL(get_source, "get_source_v1") &&
           ICON_SYMBOL(reload, "reload_v1") && ICON_SYMBOL(collect, "collect_v1") &&
           ICON_SYMBOL(asset_put, "asset_put_v1") && ICON_SYMBOL(asset_remove, "asset_remove_v1") &&
           ICON_SYMBOL(asset_clear, "asset_clear_v1") &&
           ICON_SYMBOL(catalog_count, "catalog_count_v1") &&
           ICON_SYMBOL(catalog_entry, "catalog_entry_v1") && ICON_SYMBOL(query, "query_v1") &&
           ICON_SYMBOL(asset_copy, "asset_copy_v1") && ICON_SYMBOL(prepare, "prepare_v1") &&
           ICON_SYMBOL(bitmap_copy, "bitmap_copy_v1") && (api->versions() & 1) &&
           api->record_size(CUI_BRIDGE_ICON_LIMITS_RECORD_V1) ==
               sizeof(cui_bridge_icon_limits_v1) &&
           api->record_size(CUI_BRIDGE_ICON_INFO_RECORD_V1) == sizeof(cui_bridge_icon_info_v1) &&
           api->record_size(CUI_BRIDGE_ICON_BITMAP_RECORD_V1) == sizeof(cui_bridge_icon_bitmap_v1);
#undef ICON_SYMBOL
}

enum { ICON_TEXT_MAX = 4096, ICON_SVG_MAX = 1 << 20, ICON_BITMAP_MAX = 64 << 20 };

typedef struct {
    js_app *owner;
    cui_bridge_icon_handle_v1 handle;
    napi_ref app_ref;
} js_icon;

static const napi_type_tag icon_tag = {0xb129dccee78a4b92ULL, 0xacc70655ac04c3a6ULL};

static void icon_final(napi_env env, void *data, void *hint) {
    js_icon *icon = data;
    (void)hint;
    if (icon->app_ref)
        (void)napi_delete_reference(env, icon->app_ref);
    free(icon);
}

static bool icon_arg(napi_env env, napi_value value, js_app *app, js_icon **out) {
    if (!tagged(env, value, &icon_tag) ||
        napi_get_value_external(env, value, (void **)out) != napi_ok || !*out ||
        (*out)->owner != app || !(*out)->handle.generation) {
        fail(env, "Icon resource is closed or belongs to another app");
        return false;
    }
    return true;
}

static napi_value icon_uint(napi_env env, uint32_t value) {
    napi_value result;
    return napi_create_uint32(env, value, &result) == napi_ok ? result : NULL;
}

static bool icon_property(napi_env env, napi_value obj, const char *name, napi_value value) {
    return value && napi_set_named_property(env, obj, name, value) == napi_ok;
}

static bool icon_bigint_property(napi_env env, napi_value obj, const char *name, uint64_t number) {
    napi_value value;
    return napi_create_bigint_uint64(env, number, &value) == napi_ok &&
           icon_property(env, obj, name, value);
}

/* JS supplies copied UTF-8 bytes. The addon also accepts direct diagnostic calls. */
static bool icon_bytes(napi_env env, napi_value value, uint32_t maximum, const char **text,
                       uint32_t *length) {
    void *bytes;
    size_t size;
    if (napi_get_buffer_info(env, value, &bytes, &size) != napi_ok || size > maximum ||
        (size && (!bytes || memchr(bytes, 0, size) || !strict_utf8(bytes, (uint32_t)size)))) {
        fail(env, "expected bounded UTF-8 bytes without NUL");
        return false;
    }
    *text = bytes;
    *length = (uint32_t)size;
    return true;
}

static napi_value icon_string(napi_env env, const char *text, uint32_t size, uint32_t capacity) {
    if (!size || size > capacity || text[size - 1] != 0 || memchr(text, 0, size - 1) ||
        !strict_utf8(text, size - 1))
        return fail(env, "invalid copied Icon string");
    napi_value result;
    return napi_create_string_utf8(env, text, size - 1, &result) == napi_ok ? result : NULL;
}

static bool icon_float(napi_env env, napi_value value, float *out) {
    double number;
    if (!number_arg(env, value, &number))
        return false;
    *out = (float)number;
    if (number != 0 && *out == 0) {
        fail(env, "Icon value underflows a bridge float");
        return false;
    }
    return true;
}

static bool icon_limits_input(napi_env env, const napi_value *values,
                              cui_bridge_icon_limits_v1 *limits) {
    uint32_t fields[4];
    for (size_t i = 0; i < sizeof fields / sizeof fields[0]; ++i)
        if (!uint_arg(env, values[i], &fields[i]))
            return false;
    limits->max_svg_bytes = fields[0];
    limits->max_pixels = fields[1];
    limits->max_cache_bytes = fields[2];
    limits->max_versions = fields[3];
    return true;
}

static napi_value icon_limits_output(napi_env env, const cui_bridge_icon_limits_v1 *limits) {
    if (limits->struct_size != sizeof *limits || limits->schema_version != 1 ||
        limits->reserved[0] || limits->reserved[1] || !limits->max_svg_bytes ||
        limits->max_svg_bytes > ICON_SVG_MAX || !limits->max_pixels ||
        limits->max_pixels > (16u << 20) || !limits->max_cache_bytes ||
        limits->max_cache_bytes > ICON_BITMAP_MAX || !limits->max_versions ||
        limits->max_versions > 128)
        return fail(env, "invalid copied Icon limits");
    const char *const names[] = {"maxSvgBytes", "maxPixels", "maxCacheBytes", "maxVersions"};
    const double values[] = {(double)limits->max_svg_bytes, (double)limits->max_pixels,
                             (double)limits->max_cache_bytes, (double)limits->max_versions};
    return numeric_object(env, names, values, 4);
}

static napi_value icon_new(napi_env env, napi_value app_value, js_app *app,
                           const icon_resources_api *api, const cui_bridge_icon_limits_v1 *limits) {
    js_icon *icon = calloc(1, sizeof *icon);
    if (!icon)
        return fail(env, "allocation failed");
    icon->owner = app;
    napi_value result;
    if (napi_create_reference(env, app_value, 1, &icon->app_ref) != napi_ok ||
        napi_create_external(env, icon, icon_final, NULL, &result) != napi_ok) {
        icon_final(env, icon, NULL);
        return fail(env, "could not create an owned Icon handle");
    }
    if (napi_type_tag_object(env, result, &icon_tag) != napi_ok)
        return fail(env, "could not tag owned Icon handle");
    cui_bridge_status code = api->create(app->bridge, limits, &icon->handle);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (!icon->handle.slot || !icon->handle.generation)
        return fail(env, "invalid created Icon handle");
    return result;
}

static napi_value icon_copy_string(napi_env env, const icon_resources_api *api, cui_bridge_app *app,
                                   cui_bridge_icon_handle_v1 handle, const char *name,
                                   uint32_t name_size, bool source) {
    uint32_t required = 0, kind = UINT32_MAX;
    cui_bridge_status code = source ? api->get_source(app, handle, &kind, NULL, 0, &required)
                                    : api->asset_copy(name, name_size, NULL, 0, &required);
    if (code == CUI_BRIDGE_EMPTY && !source)
        return undefined(env);
    if (code != CUI_BRIDGE_CAPACITY)
        return code == CUI_BRIDGE_OK ? fail(env, "invalid Icon string sizing response")
                                     : status(env, code);
    if (required == 0 || required > ICON_SVG_MAX + 1)
        return fail(env, "invalid Icon string capacity");
    uint32_t capacity = required;
    char *text = calloc(capacity, 1);
    if (!text)
        return fail(env, "allocation failed");
    code = source ? api->get_source(app, handle, &kind, text, capacity, &required)
                  : api->asset_copy(name, name_size, text, capacity, &required);
    napi_value value = code == CUI_BRIDGE_OK && required <= capacity
                           ? icon_string(env, text, required, capacity)
                       : code == CUI_BRIDGE_OK ? fail(env, "invalid Icon string size")
                                               : status(env, code);
    free(text);
    if (!source || !value)
        return value;
    if (kind > CUI_BRIDGE_ICON_DATA_V1 || (!kind && required != 1) ||
        (kind < CUI_BRIDGE_ICON_DATA_V1 && required > ICON_TEXT_MAX + 1))
        return fail(env, "invalid copied Icon source");
    napi_value result;
    if (napi_create_object(env, &result) != napi_ok ||
        !icon_property(env, result, "kind", icon_uint(env, kind)) ||
        !icon_property(env, result, "text", value))
        return NULL;
    return result;
}

static napi_value icon_catalog_entry(napi_env env, const icon_resources_api *api, uint32_t index) {
    char name[ICON_TEXT_MAX + 1] = {0}, path[ICON_TEXT_MAX + 1] = {0};
    uint32_t is_default = UINT32_MAX, name_size = 0, path_size = 0;
    cui_bridge_status code = api->catalog_entry(index, &is_default, name, sizeof name, &name_size,
                                                path, sizeof path, &path_size);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (is_default > 1 || name_size <= 1 || path_size <= 1)
        return fail(env, "invalid copied Icon catalog entry");
    napi_value result, embedded;
    if (napi_create_object(env, &result) != napi_ok ||
        napi_get_boolean(env, is_default != 0, &embedded) != napi_ok ||
        !icon_property(env, result, "defaultEmbedded", embedded) ||
        !icon_property(env, result, "name", icon_string(env, name, name_size, sizeof name)) ||
        !icon_property(env, result, "path", icon_string(env, path, path_size, sizeof path)))
        return NULL;
    return result;
}

static napi_value icon_read_info(napi_env env, const icon_resources_api *api, js_icon *icon) {
    cui_bridge_icon_info_v1 info = {.struct_size = sizeof info, .schema_version = 1};
    cui_bridge_status code = api->get_info(icon->owner->bridge, icon->handle, &info);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (info.struct_size != sizeof info || info.schema_version != 1 || info.icon_error > 5 ||
        info.source_kind > CUI_BRIDGE_ICON_DATA_V1 || !info.source_revision || info.flags ||
        info.reserved || info.handle.slot != icon->handle.slot ||
        info.handle.generation != icon->handle.generation)
        return fail(env, "invalid copied Icon info");
    const char *const names[] = {"error", "references", "sourceKind"};
    const double values[] = {info.icon_error, info.reference_count, info.source_kind};
    napi_value result = numeric_object(env, names, values, 3);
    if (!result || !icon_bigint_property(env, result, "parseCount", info.parse_count) ||
        !icon_bigint_property(env, result, "rasterCount", info.raster_count) ||
        !icon_bigint_property(env, result, "cacheBytes", info.cache_bytes) ||
        !icon_bigint_property(env, result, "cacheVersions", info.cache_versions) ||
        !icon_bigint_property(env, result, "sourceRevision", info.source_revision))
        return NULL;
    return result;
}

static napi_value icon_read_bitmap(napi_env env, const icon_resources_api *api, js_icon *icon) {
    cui_bridge_icon_bitmap_v1 bitmap = {.struct_size = sizeof bitmap, .schema_version = 1};
    cui_bridge_app *app = icon->owner->bridge;
    uint32_t required = 0, capacity = 0;
    cui_bridge_status code = api->bitmap_copy(app, icon->handle, &bitmap, NULL, 0, &required);
    if (code != CUI_BRIDGE_OK && code != CUI_BRIDGE_CAPACITY)
        return status(env, code);
    napi_value rgba, out;
    void *pixels;
    if (code == CUI_BRIDGE_CAPACITY) {
        if (!required || required > ICON_BITMAP_MAX)
            return fail(env, "invalid copied Icon bitmap capacity");
        capacity = required;
        if (napi_create_buffer(env, capacity, &pixels, &rgba) != napi_ok)
            return NULL;
        memset(pixels, 0, capacity);
        code = api->bitmap_copy(app, icon->handle, &bitmap, pixels, capacity, &required);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
    } else if (napi_create_buffer(env, 0, NULL, &rgba) != napi_ok)
        return NULL;
    if (bitmap.struct_size != sizeof bitmap || bitmap.schema_version != 1 || bitmap.format ||
        bitmap.reserved || bitmap.reserved2 || required != capacity ||
        bitmap.required_bytes != required || bitmap.width > 4096 || bitmap.height > 4096 ||
        bitmap.stride != bitmap.width * 4u || (uint64_t)bitmap.stride * bitmap.height != capacity ||
        (capacity ? bitmap.revision == 0 : bitmap.width || bitmap.height || bitmap.revision) ||
        !isfinite(bitmap.paint_x) || !isfinite(bitmap.paint_y) || !isfinite(bitmap.paint_width) ||
        !isfinite(bitmap.paint_height) || bitmap.paint_width < 0 || bitmap.paint_height < 0)
        return fail(env, "invalid copied Icon bitmap");
    const char *const names[] = {"width", "height", "stride"};
    const double values[] = {bitmap.width, bitmap.height, bitmap.stride};
    const char *const rect_names[] = {"x", "y", "width", "height"};
    const double rect[] = {bitmap.paint_x, bitmap.paint_y, bitmap.paint_width, bitmap.paint_height};
    out = numeric_object(env, names, values, 3);
    if (!out || !icon_bigint_property(env, out, "revision", bitmap.revision) ||
        !icon_property(env, out, "rgba", rgba) ||
        !icon_property(env, out, "paint", numeric_object(env, rect_names, rect, 4)))
        return NULL;
    return out;
}

enum icon_resource_op {
    ICON_PROBE,
    ICON_DEFAULT_LIMITS,
    ICON_CATALOG_COUNT,
    ICON_CATALOG_ENTRY,
    ICON_QUERY,
    ICON_COPY_ASSET,
    ICON_CREATE,
    ICON_DESTROY,
    ICON_INFO,
    ICON_GET_LIMITS,
    ICON_SET_LIMITS,
    ICON_GET_SOURCE,
    ICON_SET_SOURCE,
    ICON_PUT_ASSET,
    ICON_REMOVE_ASSET,
    ICON_CLEAR_ASSETS,
    ICON_RELOAD,
    ICON_COLLECT,
    ICON_PREPARE,
    ICON_BITMAP
};

static napi_value icon_resource(napi_env env, napi_callback_info info) {
    static const size_t arity[] = {2, 2, 2, 3, 3, 3, 6, 3, 3, 3, 7, 3, 5, 5, 4, 3, 3, 3, 11, 3};
    napi_value values[11];
    size_t count = sizeof values / sizeof values[0];
    js_app *owner;
    uint32_t op;
    if (napi_get_cb_info(env, info, &count, values, NULL, NULL) != napi_ok || count < 2)
        return fail(env, "missing Icon resource arguments");
    if (!app_arg(env, values[0], &owner) || !uint_arg(env, values[1], &op))
        return NULL;
    if (op >= sizeof arity / sizeof arity[0] || count < arity[op])
        return fail(env, "invalid Icon resource operation");
    icon_resources_api api;
    bool supported = icon_resources_resolve(&api);
    if (op == ICON_PROBE) {
        napi_value result;
        return napi_get_boolean(env, supported, &result) == napi_ok ? result : NULL;
    }
    if (!supported)
        return fail(env, "owned Icon V1 requires a complete, compatible bridge symbol group");
    js_icon *icon = NULL;
    if (op >= ICON_DESTROY && !icon_arg(env, values[2], owner, &icon))
        return NULL;
    cui_bridge_app *app = owner->bridge;
    cui_bridge_icon_handle_v1 handle = icon ? icon->handle : (cui_bridge_icon_handle_v1){0};
    cui_bridge_icon_limits_v1 limits = {.struct_size = sizeof limits, .schema_version = 1};
    cui_bridge_status code;
    const char *text, *svg;
    uint32_t length, svg_length, kind, index, answer = UINT32_MAX;
    switch (op) {
    case ICON_DEFAULT_LIMITS:
    case ICON_GET_LIMITS:
        code = op == ICON_DEFAULT_LIMITS ? api.default_limits(&limits)
                                         : api.get_limits(app, handle, &limits);
        return code == CUI_BRIDGE_OK ? icon_limits_output(env, &limits) : status(env, code);
    case ICON_CATALOG_COUNT:
        return icon_uint(env, api.catalog_count());
    case ICON_CATALOG_ENTRY:
        return uint_arg(env, values[2], &index) ? icon_catalog_entry(env, &api, index) : NULL;
    case ICON_QUERY:
    case ICON_COPY_ASSET:
        if (!icon_bytes(env, values[2], ICON_TEXT_MAX, &text, &length))
            return NULL;
        if (op == ICON_COPY_ASSET)
            return icon_copy_string(env, &api, app, handle, text, length, false);
        code = api.query(text, length, &answer);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
        return answer <= 2 ? icon_uint(env, answer) : fail(env, "invalid Icon availability");
    case ICON_CREATE:
    case ICON_SET_LIMITS:
        if (!icon_limits_input(env, values + (op == ICON_CREATE ? 2 : 3), &limits))
            return NULL;
        if (op == ICON_CREATE)
            return icon_new(env, values[0], owner, &api, &limits);
        code = api.set_limits(app, handle, &limits);
        break;
    case ICON_DESTROY:
        code = api.destroy(app, handle);
        if (code == CUI_BRIDGE_OK)
            icon->handle = (cui_bridge_icon_handle_v1){0};
        break;
    case ICON_INFO:
        return icon_read_info(env, &api, icon);
    case ICON_GET_SOURCE:
        return icon_copy_string(env, &api, app, handle, NULL, 0, true);
    case ICON_SET_SOURCE:
        if (!uint_arg(env, values[3], &kind) ||
            !icon_bytes(env, values[4],
                        kind == CUI_BRIDGE_ICON_DATA_V1 ? ICON_SVG_MAX : ICON_TEXT_MAX, &text,
                        &length))
            return NULL;
        code = api.set_source(app, handle, kind, text, length);
        break;
    case ICON_PUT_ASSET:
    case ICON_REMOVE_ASSET:
        if (!icon_bytes(env, values[3], ICON_TEXT_MAX, &text, &length))
            return NULL;
        if (op == ICON_PUT_ASSET) {
            if (!icon_bytes(env, values[4], ICON_SVG_MAX, &svg, &svg_length))
                return NULL;
            code = api.asset_put(app, handle, text, length, svg, svg_length);
        } else
            code = api.asset_remove(app, handle, text, length);
        break;
    case ICON_CLEAR_ASSETS:
        code = api.asset_clear(app, handle);
        break;
    case ICON_RELOAD:
        code = api.reload(app, handle);
        break;
    case ICON_COLLECT:
        code = api.collect(app, handle);
        break;
    case ICON_PREPARE: {
        float width, height, transform[6];
        if (!icon_float(env, values[3], &width) || !icon_float(env, values[4], &height))
            return NULL;
        for (size_t i = 0; i < 6; ++i)
            if (!icon_float(env, values[5 + i], &transform[i]))
                return NULL;
        code = api.prepare(app, handle, width, height, transform, &answer);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
        return answer <= 5 ? icon_uint(env, answer) : fail(env, "invalid Icon prepare result");
    }
    case ICON_BITMAP:
        return icon_read_bitmap(env, &api, icon);
    default:
        return fail(env, "invalid Icon resource operation");
    }
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
